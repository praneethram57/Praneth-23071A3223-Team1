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
#define SERVER_IP "127.0.0.1"
#define PORT 8080
#define BUFFER_SIZE 1024

void chat(int sock) {
    char buffer[BUFFER_SIZE];
    fd_set fds;

    while (1) {
        FD_ZERO(&fds);
        FD_SET(sock, &fds);
        FD_SET(STDIN_FILENO, &fds);

        if (select(sock + 1, &fds, NULL, NULL, NULL) < 0) {
            perror("Select error");
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(STDIN_FILENO, &fds)) {
            fgets(buffer, BUFFER_SIZE, stdin);
            send(sock, buffer, strlen(buffer), 0);
        }

        if (FD_ISSET(sock, &fds)) {
            int bytes_received = recv(sock, buffer, BUFFER_SIZE, 0);
            if (bytes_received <= 0) {
                printf("Disconnected from server.\n");
                break;
            }
            buffer[bytes_received] = '\0';
            printf("Server: %s", buffer);
        }
    }
}

int main(int argc, char *argv[]) {
    int sock;
    struct sockaddr_in server_addr;
    struct sockaddr_un unix_addr;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <unix/internet>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (strcmp(argv[1], "internet") == 0) {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            perror("Socket error");
            exit(EXIT_FAILURE);
        }

        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(PORT);
        inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

        if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            perror("Connection failed");
            exit(EXIT_FAILURE);
        }
        printf("Connected to Internet Server.\n");

    } else if (strcmp(argv[1], "unix") == 0) {
        sock = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sock < 0) {
            perror("Socket error");
            exit(EXIT_FAILURE);
        }

        unix_addr.sun_family = AF_UNIX;
        strcpy(unix_addr.sun_path, UNIX_SOCKET_PATH);

        if (connect(sock, (struct sockaddr *)&unix_addr, sizeof(unix_addr)) < 0) {
            perror("Connection failed");
            exit(EXIT_FAILURE);
        }
        printf("Connected to UNIX Server.\n");

    } else {
        fprintf(stderr, "Invalid option. Use 'unix' or 'internet'.\n");
        exit(EXIT_FAILURE);
    }

    chat(sock);
    close(sock);
    return 0;
}
