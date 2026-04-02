//
// Created by User on 2026/3/25.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "protocol.h"
#include <sys/select.h>

#define MAX_WORKERS 1
#define LR 0.1
#define MAX_ITER 100
#define LOSS_THRESHOLD 0.01

int worker_fds[MAX_WORKERS] = {0};

double weight[DIM];

double global_gradient[DIM];
double global_loss;
int total_samples;

int responded[MAX_WORKERS];
int num_responded;

int recv_all(int client_fd, void *buf, size_t len) {
    size_t total = 0;

    while (total < len) {
        int n = recv(client_fd, buf + total, len - total, 0);
        if (n <= 0) {
            return -1;
        }
        total += n;
    }
    return 0;
}

int send_all(int client_fd, void *buf, size_t len) {
    size_t total = 0;
    while (total < len) {
        int n = send(client_fd, buf + total, len - total, 0);
        if (n <= 0) {
            return -1;
        }
        total += n;
    }
    return 0;
}

int accept_workers(int server_fd, int worker_fds[]) {
    int num_workers = 0;

    while (num_workers < MAX_WORKERS) {
        int client_fd = accept(server_fd, NULL, NULL); // Client address?
        worker_fds[num_workers] = client_fd;
        printf("[Server] Worker %d connected\n", num_workers);
        num_workers++;
    }
    return num_workers;
}

int train_loop(int worker_fds[], int num_workers, Model *model) {
    double weight[DIM] = {0};

    int iteration = 0;

    while (iteration < MAX_ITER) {

        // Broadcast
        Message msg;
        memset(&msg, 0, sizeof(msg));
        msg.type = PARAM;

        for (int j = 0; j < DIM; j++) {
            msg.weight[j] = weight[j];
        }
        
        for (int i = 0; i < num_workers; i++) {
            if (send_all(worker_fds[i], &msg, sizeof(msg)) < 0) {
                printf("[Server] Failed to send message to worker %d\n", i);
                exit(1);
            }
        }

        // reset aggregation
        memset(&global_gradient, 0, sizeof(global_gradient));
        global_loss = 0;
        total_samples = 0;

        memset(responded, 0, sizeof(responded));
        num_responded = 0;

        // collect gradient
        while (num_responded < num_workers) {

            fd_set read_fds;
            FD_ZERO(&read_fds);

            int max_fd = -1;
            for (int i = 0; i < num_workers; i++) {
                if (!responded[i]) {
                    FD_SET(worker_fds[i], &read_fds);
                    if (worker_fds[i] > max_fd) {
                        max_fd = worker_fds[i];
                    }
                }
            }

            select(max_fd + 1, &read_fds, NULL, NULL, NULL);

            for (int i = 0; i < num_workers; i++) {
                if (!responded[i] && FD_ISSET(worker_fds[i], &read_fds)) {
                    Message recv_msg;
                    memset(&recv_msg, 0, sizeof(recv_msg));
                    if (recv_all(worker_fds[i], &recv_msg, sizeof(recv_msg)) < 0) {
                        printf("[Server] Failed to receive message from worker %d\n", i);
                        exit(1);
                    }

                    if (recv_msg.type != SEND_GRAD) {
                        printf("[Server] Unexpected message type from worker %d\n", i);
                        continue;
                    }

                    if (recv_msg.type == SEND_GRAD) {
                        for (int j = 0; j < DIM; j++) {
                            global_gradient[j] += recv_msg.gradient[j];
                        }
                        global_loss += recv_msg.loss * recv_msg.n_samples;
                        total_samples += recv_msg.n_samples;

                        responded[i] = 1;
                        num_responded++;
                    }
                }
            }
        }

        global_loss /= total_samples;

        printf("[Server] Iteration %d: Loss = %f\n", iteration, global_loss);

        // Update weight
        double lr = LR;

        for (int j = 0; j < DIM; j++) {
            weight[j] -= lr * global_gradient[j];
        }

        // Stop condition
        if (global_loss < LOSS_THRESHOLD) {
            printf("[Server] Loss below threshold. Stopping training.\n");
            break;
        }

        iteration++;
    }

    // Fill final model
    model->loss = global_loss;
    for (int j = 0; j < DIM; j++) {
        model->weight[j] = weight[j];
    }

    // Broadcast train done and final model
    Message done_msg;
    memset(&done_msg, 0, sizeof(done_msg));
    done_msg.type = TRAIN_DONE;
    done_msg.loss = global_loss;

    for (int j = 0; j < DIM; j++) {
        done_msg.weight[j] = weight[j];
    }

    for (int i = 0; i < num_workers; i++) {
        if (send_all(worker_fds[i], &done_msg, sizeof(done_msg)) < 0) {
            printf("[Server] Failed to send done message to worker %d\n", i);
            exit(1);
        }
    }

    return 0;
}

int main() {
    int server_fd = 0;
    struct sockaddr_in address;
    int addrlen = sizeof(address);


    // socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    // bind
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (struct sockaddr *)&address, addrlen);

    // listen
    listen(server_fd, 3);
    printf("[Server] listening on port %d\n", PORT);

    // accept workers
    int num_workers = accept_workers(server_fd, worker_fds);
    printf("[Server] after accept\n");

    // train loop (and broadcast final model to workers)
    Model final_model;
    memset(&final_model, 0, sizeof(final_model));
    train_loop(worker_fds, num_workers, &final_model);

    // Print final model
    printf("[Server] Training done. Final loss: %f\n", final_model.loss);
    printf("[Server] Final weight: ");
    for (int j = 0; j < DIM; j++) {
        printf("%f ", final_model.weight[j]);
    }
    printf("\n");

    // close connections
    for (int i = 0; i < num_workers; i++) {
        if (close(worker_fds[i]) < 0) {
            printf("[Server] Failed to close connection with worker %d\n", i);
        }
    }
    if (close(server_fd) < 0) {
        printf("[Server] Failed to close server socket\n");
    }
    printf("[Server] shutdown\n");
    
}