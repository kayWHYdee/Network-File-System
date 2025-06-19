#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define BUFFER_SIZE 4096
#define SUCCESS 0
#define ERROR -1

int main() {
    // Hardcoded file path and server IP
    const char *file_path = "hi";  // Replace with your file or folder
    const char *server_ip = "127.0.0.1";
    int server_port = 9090;

    // Step 1: Create the socket
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("Socket creation failed");
        return ERROR;
    }

    // Step 2: Set up the server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);

    // Step 3: Connect to the server
    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection to server failed");
        close(sock_fd);
        return ERROR;
    }

    printf("Connected to server %s:%d\n", server_ip, server_port);

    // Step 4: Compress the file
    char compressed_path[BUFFER_SIZE];
    snprintf(compressed_path, sizeof(compressed_path), "%s.zip", file_path);
    char zip_command[1024];
    snprintf(zip_command, sizeof(zip_command), "zip -r %s %s", compressed_path, file_path);
    if (system(zip_command) != 0) {
        perror("Error compressing file");
        close(sock_fd);
        return ERROR;
    }

    // Step 5: Open the compressed file
    int fd = open(compressed_path, O_RDONLY);
    if (fd < 0) {
        perror("Error opening compressed file");
        close(sock_fd);
        return ERROR;
    }

    // Step 6: Send the file
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        if (send(sock_fd, buffer, bytes_read, 0) < 0) {
            perror("Error sending file data");
            close(fd);
            close(sock_fd);
            return ERROR;
        }
    }

    // Step 7: Clean up
    close(fd);
    unlink(compressed_path);
    close(sock_fd);
    printf("File sent successfully.\n");

    return SUCCESS;
}
