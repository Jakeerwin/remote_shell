#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 7891
#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <server-ip>\n", argv[0]);
        exit(1);
    }

    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char recv_buffer[BUFFER_SIZE];

    client_socket = socket(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    memset(server_addr.sin_zero, '\0', sizeof server_addr.sin_zero);

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(1);
    }

    printf("Connected to the server (%s:%d) successfully\n", argv[1], PORT);

    while (1) {
        printf("remote-shell> ");
        fflush(stdout);
        if (!fgets(buffer, BUFFER_SIZE, stdin)) break;

        send(client_socket, buffer, strlen(buffer), 0);

        if (strncmp(buffer, "quit", 4) == 0) break;

        while (1) {
            int n = recv(client_socket, recv_buffer, BUFFER_SIZE - 1, 0);
            if (n <= 0) break;
            recv_buffer[n] = '\0';
            if (strstr(recv_buffer, "__CMD_DONE__") != NULL) break;

            printf("%s", recv_buffer);
        }
    }

    close(client_socket);
    return 0;
}