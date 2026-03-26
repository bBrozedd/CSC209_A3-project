//
// Created by User on 2026/3/25.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "protocol.h"

#define MAX_WORKERS 3

int worker_fds[MAX_WORKERS];
int num_workers;

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
        num_workers++;

        printf("Worker %d connected\n", num_workers);
    }
}

int train_loop(int worker_fds[], int num_workers) {
    double weight[DIM] = {0};

    int iteration = 0;

    while (iteration < 1000) {

        // Broadcast
        Message msg;
        msg.type = PARAM;

        for (int i = 0; i < num_workers; i++) {
            send_all(worker_fds[i], &msg, sizeof(msg));
        }

        // reset aggregation
        memset(&global_gradient, 0, sizeof(global_gradient));
        global_loss = 0;
        total_samples = 0;

        memset(responded, 0, sizeof(responded));
        num_responded = 0;

        // collect gradient
        ...
    }
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    char buffer[1024] = {0};

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
    printf("Server listening on port %d\n", PORT);

    struct sockaddr_in client_address;
    memset(&client_address, 0, sizeof(client_address));
    client_address.sin_family = AF_INET;
    unsigned int client_len = sizeof(client_address);

    // accept
    client_fd = accept(server_fd, (struct sockaddr *)&client_address, (socklen_t *)&client_len);

    // receive
    recv(client_fd, buffer, 1024, 0);
    printf("Server received message: %s\n", buffer);

    // send response
    char *reply = "Hello World!";
    send(client_fd, reply, strlen(reply), 0);

    close(client_fd);
    close(server_fd);
}