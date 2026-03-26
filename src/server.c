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