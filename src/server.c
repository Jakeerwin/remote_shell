// Standard, socket, process, and signal handling includes
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

int server_fd; // Server socket file descriptor (for cleanup on exit)

// Handle Ctrl+C or termination gracefully
void handle_sigint(int sig) {
    printf("\n[INFO] Caught signal %d. Cleaning up and exiting...\n", sig);
    if (server_fd > 0) close(server_fd);
    exit(0);
}

// Handles command execution for one connected client
void handle_client(int client_socket, struct sockaddr_in client_addr) {
    char buffer[BUFFER_SIZE];
    char cmd_output[BUFFER_SIZE];
    ssize_t bytes_received;

    printf("Client (%s:%d) Connected to the server successfully\n",
           inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    // Command handling loop
    while ((bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[bytes_received] = '\0';

        // Exit condition
        if (strcmp(buffer, "quit\n") == 0 || strcmp(buffer, "quit") == 0) {
            break;
        }

        int pipefd[2];
        pipe(pipefd);  // Create pipe for capturing command output

        pid_t pid = fork();  // Fork to execute command

        if (pid == 0) {
            // Child process: redirect stdout and stderr to pipe
            dup2(pipefd[1], STDOUT_FILENO);
            dup2(pipefd[1], STDERR_FILENO);
            close(pipefd[0]);
            close(pipefd[1]);

            // Split input into args for execvp
            char *args[64];
            buffer[strcspn(buffer, "\n")] = 0;  // Trim newline
            char *token = strtok(buffer, " ");
            int i = 0;
            while (token != NULL && i < BUFFER_SIZE - 1) {
                args[i++] = token;
                token = strtok(NULL, " ");
            }
            args[i] = NULL;

            execvp(args[0], args);  // Run the command
            perror("execvp failed");  // Only reached if execvp fails
            exit(1);
        } else {
            // Parent process: read output from child via pipe
            close(pipefd[1]);  // Close unused write end

            int n;
            while ((n = read(pipefd[0], cmd_output, BUFFER_SIZE - 1)) > 0) {
                cmd_output[n] = '\0';
                send(client_socket, cmd_output, strlen(cmd_output), 0);
            }

            close(pipefd[0]);  // Close read end
            wait(NULL);        // Wait for child to finish

            // Signal to client that command output is done
            send(client_socket, "__CMD_DONE__", strlen("__CMD_DONE__"), 0);
        }
    }

    close(client_socket);  // Cleanup
    exit(0);
}

int main() {
    int client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size = sizeof(client_addr);

    // Register signal handlers for clean shutdown
    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint);

    // Create TCP socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    // Set up server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    memset(server_addr.sin_zero, '\0', sizeof server_addr.sin_zero);

    bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_fd, 5);  // Start listening with a backlog of 5

    printf("Server listening on port %d\n", PORT);

    // Accept and handle incoming clients
    while (1) {
        client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &addr_size);
        if (client_socket < 0) continue;

        // Fork new process to handle each client
        if (fork() == 0) {
            close(server_fd);  // Child doesn't need the listener
            handle_client(client_socket, client_addr);
        }
        close(client_socket);  // Parent closes the client socket
    }

    return 0;
}