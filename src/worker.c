//
// Created by User on 2026/3/25.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "protocol.h"
#include "model.h"

#define SAMPLE_SIZE 100

/*
 * Load data from a CSV partition
 */
int load_data(const char *filename, double *features, double *labels) {
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

        // increment i if a valid sample was read to maintain sample size tracking
        if (flag) {
            i++;
        }
    }
    fclose(fp);

    return i; // sample size
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

int recv_all(int server_fd, void *buf, size_t len) {
    size_t total = 0;

    while (total < len) {
        int n = recv(server_fd, buf + total, len - total, 0);
        if (n <= 0) {
            return -1;
        }
        total += n;
    }
    return 0;
}

int send_all(int server_fd, void *buf, size_t len) {
    size_t total = 0;
    while (total < len) {
        int n = send(server_fd, buf + total, len - total, 0);
        if (n <= 0) {
            return -1;
        }
        total += n;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("[Worker] Usage: ./worker <server_ip> <data_file.csv>");
        exit(1);
    }

    int sock;
    struct sockaddr_in serv_addr;

    // socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        return(1);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) != 1) {
        printf("[Worker] Invalid server IP address");
        exit(1);
    }

    // connect
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        close(sock);
        exit(1);
    };

    // load data (random sample)
    double features[SAMPLE_SIZE * DIM];
    double labels[SAMPLE_SIZE];
    int n_samples = load_data(argv[2], features, labels);

    // training loop
    while (1) {
        Message msg;
        memset(&msg, 0, sizeof(msg));

        // Receive weight from server
        if (recv_all(sock, &msg, sizeof(msg)) < 0) {
            printf("[Worker] Failed to receive message from server\n");
            break;
        }

        // check message type (TRAIN_DONE or PARAM)
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

            // get weight
            double *weight = msg.weight;

            // compute gradient and loss
            double gradient[DIM] = {0};
            double loss = 0;

            compute_gradient(features, labels, weight, gradient, n_samples, DIM);
            loss = compute_loss(features, labels, weight, n_samples, DIM);

            printf("[Worker] loss: %.6f\n", loss);

            // Prepare reply message
            Message reply;
            memset(&reply, 0, sizeof(reply));
            reply.type = SEND_GRAD;
            reply.loss = loss;
            reply.n_samples = n_samples;
            memcpy(reply.gradient, gradient, sizeof(gradient));

            // Send gradient and loss back to server
            if (send_all(sock, &reply, sizeof(reply)) < 0) {
                printf("[Worker] Failed to send message to server\n");
                break;
            }

        }

        else {
            printf("[Worker] Unknown message type from server: %d\n", msg.type);
            break;
        }

    }

    close(sock);
    printf("[Worker] exiting...\n");
    return 0;
}
