//
// server.c
//

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include "protocol.h"

#define MAX_WORKERS 3
#define LR 1
#define MAX_ITER 10000
#define LOSS_THRESHOLD 0.05

static int worker_fds[MAX_WORKERS];

static int recv_all(int fd, void *buf, size_t len) {
    size_t total = 0;
    while (total < len) {
        ssize_t n = recv(fd, (char *)buf + total, len - total, 0);
        if (n == 0) return -1;
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        total += n;
    }
    return 0;
}

static int send_all(int fd, const void *buf, size_t len) {
    size_t total = 0;
    while (total < len) {
        ssize_t n = send(fd, (const char *)buf + total, len - total, 0);
        if (n <= 0) {
            if (n < 0 && errno == EINTR) continue;
            return -1;
        }
        total += n;
    }
    return 0;
}


static void close_worker(int i) {
    if (worker_fds[i] >= 0) {
        close(worker_fds[i]);
        worker_fds[i] = -1;
    }
}

static int accept_workers(int server_fd) {
    int count = 0;

    while (count < MAX_WORKERS) {
        int fd = accept(server_fd, NULL, NULL);
        if (fd < 0) {
            if (errno == EINTR) continue;
            perror("accept");
            return -1;
        }

        worker_fds[count] = fd;
        printf("[Server] Worker %d connected\n", count);
        count++;
    }

    return count;
}

static int train_loop(int num_workers, Model *model) {
    int active[MAX_WORKERS];
    int responded[MAX_WORKERS];
    int active_count = num_workers;

    for (int i = 0; i < num_workers; i++) {
        active[i] = 1;
    }

    for (int iter = 0; iter < MAX_ITER && active_count > 0; iter++) {

        // reset response tracking
        for (int i = 0; i < num_workers; i++) {
            responded[i] = 0;
        }

        // broadcast PARAM
        for (int i = 0; i < num_workers; i++) {
            if (!active[i]) continue;

            Message msg;
            memset(&msg, 0, sizeof(msg));
            msg.type = PARAM;

            memcpy(msg.weight, model->weight, sizeof(model->weight));

            if (send_all(worker_fds[i], &msg, sizeof(msg)) < 0) {
                printf("[Server] Worker %d disconnected\n", i);
                close_worker(i);
                active[i] = 0;
                active_count--;
            }
        }

        if (active_count == 0) break;

        // collect gradients using select()
        double sum_grad[DIM] = {0};
        double total_loss = 0.0;
        int total_samples = 0;

        int pending = active_count;

        while (pending > 0) {
            fd_set read_fds;
            FD_ZERO(&read_fds);

            int max_fd = -1;

            for (int i = 0; i < num_workers; i++) {
                if (active[i] && !responded[i]) {
                    FD_SET(worker_fds[i], &read_fds);
                    if (worker_fds[i] > max_fd) {
                        max_fd = worker_fds[i];
                    }
                }
            }

            int ready = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
            if (ready < 0) {
                if (errno == EINTR) continue;
                perror("select");
                return -1;
            }

            for (int i = 0; i < num_workers; i++) {
                if (!active[i] || responded[i]) continue;
                if (!FD_ISSET(worker_fds[i], &read_fds)) continue;

                Message msg;
                if (recv_all(worker_fds[i], &msg, sizeof(msg)) < 0) {
                    printf("[Server] Worker %d disconnected\n", i);
                    close_worker(i);
                    active[i] = 0;
                    active_count--;
                    pending--;
                    continue;
                }

                if (msg.type != SEND_GRAD) {
                    printf("[Server] Invalid message type\n");
                    continue;
                }

                for (int j = 0; j < DIM; j++) {
                    sum_grad[j] += msg.gradient[j];
                }

                total_loss += msg.loss * msg.n_samples;
                total_samples += msg.n_samples;

                responded[i] = 1;
                pending--;
            }
        }

        if (total_samples == 0) return -1;

        double avg_loss = total_loss / total_samples;

        printf("[Server] Iter %d: loss = %f\n", iter, avg_loss);

        // update model
        for (int j = 0; j < DIM; j++) {
            model->weight[j] -= LR * sum_grad[j];
        }

        model->loss = avg_loss;

        if (avg_loss < LOSS_THRESHOLD) {
            printf("[Server] Converged.\n");
            break;
        }
    }

    return 0;
}

int main(void) {
    int server_fd;
    struct sockaddr_in addr;

    for (int i = 0; i < MAX_WORKERS; i++) {
        worker_fds[i] = -1;
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }

    if (listen(server_fd, MAX_WORKERS) < 0) {
        perror("listen");
        return 1;
    }

    printf("[Server] Listening on port %d\n", PORT);

    int num_workers = accept_workers(server_fd);
    if (num_workers < 0) return 1;

    Model model;
    memset(&model, 0, sizeof(model));

    if (train_loop(num_workers, &model) < 0) {
        printf("[Server] Training failed\n");
    } else {
        printf("[Server] Training done. Final loss: %f\n", model.loss);

        printf("[Server] Final weight: ");
        for (int i = 0; i < DIM; i++) {
            printf("%f ", model.weight[i]);
        }
        printf("\n");
    }

    // send TRAIN_DONE
    for (int i = 0; i < num_workers; i++) {
        if (worker_fds[i] >= 0) {
            Message msg;
            memset(&msg, 0, sizeof(msg));
            msg.type = TRAIN_DONE;

            memcpy(msg.weight, model.weight, sizeof(model.weight));
            msg.loss = model.loss;

            send_all(worker_fds[i], &msg, sizeof(msg));
            close_worker(i);
        }
    }

    close(server_fd);
    printf("[Server] shutdown\n");
    return 0;
}