#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <fcntl.h>

#define UNIX_SOCKET_PATH "/tmp/chat_socket"
#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

void handle_client_message(int client_sock, fd_set *master_set, int *max_fd) {
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(client_sock, buffer, BUFFER_SIZE, 0);

    if (bytes_received <= 0) {
        printf("Client disconnected\n");
        close(client_sock);
        FD_CLR(client_sock, master_set);
    } else {
        buffer[bytes_received] = '\0';
        printf("Client: %s\n", buffer);

        // Broadcast message to all clients
        for (int i = 0; i <= *max_fd; i++) {
            if (FD_ISSET(i, master_set) && i != client_sock) {
                send(i, buffer, bytes_received, 0);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    int internet_sock, unix_sock, client_sock, max_fd, new_sock;
    struct sockaddr_in internet_addr;
    struct sockaddr_un unix_addr;
    fd_set master_set, read_fds;

    // Creating Internet domain socket
    internet_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (internet_sock < 0) {
        perror("Internet socket failed");
        exit(EXIT_FAILURE);
    }

    internet_addr.sin_family = AF_INET;
    internet_addr.sin_addr.s_addr = INADDR_ANY;
    internet_addr.sin_port = htons(PORT);

    if (bind(internet_sock, (struct sockaddr *)&internet_addr, sizeof(internet_addr)) < 0) {
        perror("Internet socket bind failed");
        exit(EXIT_FAILURE);
    }

    listen(internet_sock, MAX_CLIENTS);

    // Creating UNIX domain socket
    unix_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (unix_sock < 0) {
        perror("UNIX socket failed");
        exit(EXIT_FAILURE);
    }

    unix_addr.sun_family = AF_UNIX;
    strcpy(unix_addr.sun_path, UNIX_SOCKET_PATH);
    unlink(UNIX_SOCKET_PATH);

    if (bind(unix_sock, (struct sockaddr *)&unix_addr, sizeof(unix_addr)) < 0) {
        perror("UNIX socket bind failed");
        exit(EXIT_FAILURE);
    }

    listen(unix_sock, MAX_CLIENTS);

    // Initialize fd_set
    FD_ZERO(&master_set);
    FD_SET(internet_sock, &master_set);
    FD_SET(unix_sock, &master_set);
    max_fd = (internet_sock > unix_sock) ? internet_sock : unix_sock;

    printf("Server is running...\n");

    while (1) {
        read_fds = master_set;
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("Select error");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i <= max_fd; i++) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == internet_sock || i == unix_sock) {
                    struct sockaddr client_addr;
                    socklen_t client_len = sizeof(client_addr);
                    new_sock = accept(i, &client_addr, &client_len);

                    if (new_sock < 0) {
                        perror("Accept error");
                        continue;
                    }

                    printf("New client connected.\n");
                    FD_SET(new_sock, &master_set);
                    if (new_sock > max_fd) {
                        max_fd = new_sock;
                    }
                } else {
                    handle_client_message(i, &master_set, &max_fd);
                }
            }
        }
    }

    close(internet_sock);
    close(unix_sock);
    unlink(UNIX_SOCKET_PATH);

    return 0;
}
