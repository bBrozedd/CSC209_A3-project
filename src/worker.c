//
// Created by User on 2026/3/25.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/socket.h>

#include "protocol.h"
#include "model.h"

#define SAMPLE_SIZE 100

/*
 * Load data from a CSV partition
 * Expected row format:
 *   f1,f2,f3,label
 */
static int load_data(const char *filename, double *features, double *labels) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("fopen");
        return -1;
    }

    int i = 0;
    int flag = 1;
    while (i < SAMPLE_SIZE && flag) {
        for (int j = 0; j < DIM && flag; j++) {
            if (fscanf(fp, "%lf,", &features[i * DIM + j]) != 1) {
                flag = 0;
            }
        }

        if (flag && fscanf(fp, "%lf", &labels[i]) != 1) {
            flag = 0;
        }

        if (flag) {
            i++;
        }
    }

    fclose(fp);
    return i;
}

/**
 * Load generated data for testing
 */
int load_generated_data(double *features, double *labels) {
    for (int i = 0; i < SAMPLE_SIZE; i++) {
        for (int j = 0; j < DIM; j++) {
            features[i * DIM + j] = rand() / (double)RAND_MAX;
        }
        labels[i] = (features[i * DIM] > 0.5) ? 1.0 : 0.0;
    }
    return SAMPLE_SIZE; // number of samples
}

static int recv_all(int server_fd, void *buf, size_t len) {
    size_t total = 0;
    while (total < len) {
        ssize_t n = recv(server_fd, (char *)buf + total, len - total, 0);
        if (n == 0) {
            return -1;  // peer closed connection
        }
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("recv");
            return -1;
        }
        total += (size_t)n;
    }
    return 0;
}

static int send_all(int server_fd, const void *buf, size_t len) {
    size_t total = 0;
    while (total < len) {
        ssize_t n = send(server_fd, (const char *)buf + total, len - total, 0);
        if (n <= 0) {
            if (n < 0 && errno == EINTR) {
                continue;
            }
            perror("send");
            return -1;
        }
        total += (size_t)n;
    }
    return 0;
}



int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: ./worker <server_ip> <data_file.csv>\n");
        return 1;
    }

    // socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) != 1) {
        fprintf(stderr, "[Worker] Invalid server IP address\n");
        close(sock);
        return 1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        close(sock);
        return 1;
    }

    double features[SAMPLE_SIZE * DIM];
    double labels[SAMPLE_SIZE];
    int n_samples = load_data(argv[2], features, labels);
    if (n_samples <= 0) {
        fprintf(stderr, "[Worker] Failed to load data\n");
        close(sock);
        return 1;
    }

    while (1) {
        Message msg;
        memset(&msg, 0, sizeof(msg));

        if (recv_all(sock, &msg, sizeof(msg)) < 0) {
            fprintf(stderr, "[Worker] Failed to receive message from server\n");
            break;
        }

        if (msg.type == TRAIN_DONE) {
            printf("[Worker] Training done. Final loss: %f\n", msg.loss);
            printf("[Worker] Final weight: ");
            for (int j = 0; j < DIM; j++) {
                printf("%f ", msg.weight[j]);
            }
            printf("\n");
            break;
        }

        if (msg.type == PARAM) {
            double gradient[DIM] = {0.0};
            double loss = 0.0;

            compute_gradient(features, labels, msg.weight, gradient, n_samples, DIM);
            loss = compute_loss(features, labels, msg.weight, n_samples, DIM);

            printf("[Worker] loss: %.6f\n", loss);

            Message reply;
            memset(&reply, 0, sizeof(reply));
            reply.type = SEND_GRAD;
            reply.loss = loss;
            reply.n_samples = n_samples;

            memcpy(reply.gradient, gradient, sizeof(gradient));

            if (send_all(sock, &reply, sizeof(reply)) < 0) {
                fprintf(stderr, "[Worker] Failed to send message to server\n");
                break;
            }
        } else {
            fprintf(stderr, "[Worker] Unknown message type from server: %d\n", msg.type);
            break;
        }
    }

    close(sock);
    printf("[Worker] exiting...\n");
    return 0;
}