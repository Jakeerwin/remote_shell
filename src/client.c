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

    setvbuf(stdout, NULL, _IONBF, 0); // Disable stdout buffering
    setvbuf(stderr, NULL, _IONBF, 0); // Disable stderr buffering

    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char recv_buffer[BUFFER_SIZE];

    // Create socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);

    // Set up server address struct
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    memset(server_addr.sin_zero, '\0', sizeof server_addr.sin_zero);

    // Attempt to connect to the server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(1);
    }

    printf("Connected to the server (%s:%d) successfully\n", argv[1], PORT);

    // Command loop: send command to server and print response
    while (1) {
        printf("remote-shell> ");
        fflush(stdout);
        if (!fgets(buffer, BUFFER_SIZE, stdin)) break;

        // Send input command to server
        send(client_socket, buffer, strlen(buffer), 0);

        // Exit loop on "quit"
        if (strncmp(buffer, "quit", 4) == 0) break;

        // Receive output from server until "__CMD_DONE__" marker is received
        while (1) {
            int n = recv(client_socket, recv_buffer, BUFFER_SIZE - 1, 0);
            if (n <= 0) break;
            recv_buffer[n] = '\0';

            // Check for done marker and strip it
            char *done_marker = strstr(recv_buffer, "__CMD_DONE__");
            if (done_marker) {
                *done_marker = '\0';
                printf("%s", recv_buffer);
                break;
            }

            // Print output as it is received
            printf("%s", recv_buffer);
        }
    }

    // Close the socket before exiting
    close(client_socket);
    return 0;
}