//
// Created by User on 2026/3/25.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 58800

int main() {
    int sock;
    struct sockaddr_in serv_addr;
    char *message = "hello from worker";
    char buffer[1024] = {0};

    // socket
    sock = socket(AF_INET, SOCK_STREAM, 0);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    // connect
    connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    // send
    send(sock, message, strlen(message), 0);

    // receive
    recv(sock, buffer, 1024, 0);
    printf("Server says: %s\n", buffer);

    close(sock);
}