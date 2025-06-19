#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>

#define BUFFER_SIZE 4096
#define SUCCESS 0
#define ERROR -1

int main() {
    int server_port = 9090;

    // Step 1: Create the server socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        return ERROR;
    }

    // Step 2: Bind the socket
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Socket bind failed");
        close(server_fd);
        return ERROR;
    }

    printf("Server listening on port %d\n", server_port);

    // Step 3: Listen for incoming connections
    if (listen(server_fd, 1) < 0) {
        perror("Listen failed");
        close(server_fd);
        return ERROR;
    }

    // Step 4: Accept a connection
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client_fd < 0) {
        perror("Connection accept failed");
        close(server_fd);
        return ERROR;
    }

    printf("Client connected.\n");

    // Step 5: Receive the file
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    int fd = open("received.zip", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Error creating file");
        close(client_fd);
        close(server_fd);
        return ERROR;
    }

    while ((bytes_received = recv(client_fd, buffer, sizeof(buffer), 0)) > 0) {
        if (write(fd, buffer, bytes_received) != bytes_received) {
            perror("Error writing to file");
            close(fd);
            close(client_fd);
            close(server_fd);
            return ERROR;
        }
    }

    close(fd);
    close(client_fd);
    close(server_fd);

    // Step 6: Extract the received file
    if (system("unzip -o received.zip -d ./received") != 0) {
        perror("Error extracting file");
        return ERROR;
    }

    unlink("received.zip");
    printf("File received and extracted successfully.\n");

    return SUCCESS;
}
