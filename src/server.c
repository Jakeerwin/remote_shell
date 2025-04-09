#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT 7891
#define BUFFER_SIZE 1024

int server_fd; // For cleanup use

void handle_sigint(int sig) {
    printf("\n[INFO] Caught signal %d. Cleaning up and exiting...\n", sig);
    if (server_fd > 0) close(server_fd);
    exit(0);
}

void handle_client(int client_socket, struct sockaddr_in client_addr) {
    char buffer[BUFFER_SIZE];
    char cmd_output[BUFFER_SIZE];
    ssize_t bytes_received;

    printf("Client (%s:%d) Connected to the server successfully\n",
           inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    while ((bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[bytes_received] = '\0';

        if (strcmp(buffer, "quit\n") == 0 || strcmp(buffer, "quit") == 0) {
            break;
        }

        int pipefd[2];
        pipe(pipefd);

        pid_t pid = fork();

        if (pid == 0) {
            dup2(pipefd[1], STDOUT_FILENO);
            dup2(pipefd[1], STDERR_FILENO);
            close(pipefd[0]);
            close(pipefd[1]);

            char *args[64];
            buffer[strcspn(buffer, "\n")] = 0;  // Trim trailing newline
            char *token = strtok(buffer, " ");  // Split only by space
            int i = 0;
            while (token != NULL && i < BUFFER_SIZE - 1) {
                args[i++] = token;
                token = strtok(NULL, " ");
            }
            args[i] = NULL;

            for (int j = 0; args[j] != NULL; j++) {
                fprintf(stderr, "[DEBUG] args[%d] = %s\n", j, args[j]);
            }

            execvp(args[0], args);
            perror("execvp failed");
            exit(1);
        } else {
            close(pipefd[1]);  // Close write end immediately in parent

            int n;
            while ((n = read(pipefd[0], cmd_output, BUFFER_SIZE - 1)) > 0) {
                cmd_output[n] = '\0';
                send(client_socket, cmd_output, strlen(cmd_output), 0);
            }

            close(pipefd[0]);  // Ensure pipe is fully closed before sending done signal

            wait(NULL); // Wait for child AFTER pipe is read

            send(client_socket, "__CMD_DONE__", strlen("__CMD_DONE__"), 0);  // Signal done
        }
    }

    close(client_socket);
    exit(0);
}

int main() {
    int client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size = sizeof(client_addr);

    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    memset(server_addr.sin_zero, '\0', sizeof server_addr.sin_zero);

    bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_fd, 5);

    printf("Server listening on port %d\n", PORT);

    while (1) {
        client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &addr_size);
        if (client_socket < 0) continue;

        if (fork() == 0) {
            close(server_fd);
            handle_client(client_socket, client_addr);
        }
        close(client_socket);
    }

    return 0;
}