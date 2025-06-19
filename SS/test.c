#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 4096
#define SUCCESS 0
#define ERROR -1

// Function prototypes
int send_entity(const char *path, int socket_fd);
int receive_entity(int socket_fd);

// Main function for testing
int main() {
    const char *test_path = "hi"; // Hardcoded path to test file
    const char *server_ip = "127.0.0.1";

    int server_fd, client_fd;
    struct sockaddr_in address, client_addr;
    socklen_t addr_len = sizeof(address);

    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed");
        exit(ERROR);
    }

    if (pid == 0) {
        // Child process: Receiver (acts as the server)
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            perror("Socket creation failed");
            exit(ERROR);
        }

        address.sin_family = AF_INET;
        address.sin_port = htons(PORT);
        address.sin_addr.s_addr = INADDR_ANY;

        if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
            perror("Bind failed");
            close(server_fd);
            exit(ERROR);
        }

        if (listen(server_fd, 1) < 0) {
            perror("Listen failed");
            close(server_fd);
            exit(ERROR);
        }

        printf("Receiver: Waiting for sender...\n");

        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_fd < 0) {
            perror("Accept failed");
            close(server_fd);
            exit(ERROR);
        }

        printf("Receiver: Connection established.\n");

        if (receive_entity(client_fd) == ERROR) {
            fprintf(stderr, "Receiver: Error receiving file.\n");
        } else {
            printf("Receiver: File received successfully.\n");
        }

        close(client_fd);
        close(server_fd);
    } else {
        // Parent process: Sender (acts as the client)
        sleep(1); // Give the receiver time to set up

        client_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (client_fd < 0) {
            perror("Socket creation failed");
            exit(ERROR);
        }

        address.sin_family = AF_INET;
        address.sin_port = htons(PORT);
        if (inet_pton(AF_INET, server_ip, &address.sin_addr) <= 0) {
            perror("Invalid address");
            close(client_fd);
            exit(ERROR);
        }

        if (connect(client_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
            perror("Connection failed");
            close(client_fd);
            exit(ERROR);
        }

        printf("Sender: Connected to receiver.\n");

        if (send_entity(test_path, client_fd) == ERROR) {
            fprintf(stderr, "Sender: Error sending file.\n");
        } else {
            printf("Sender: File sent successfully.\n");
        }

        close(client_fd);
    }

    return SUCCESS;
}

// Function to send a file
int send_entity(const char *path, int socket_fd) {
    char compressed_path[512];
    snprintf(compressed_path, sizeof(compressed_path), "%s.zip", path);

    // Step 1: Compress the file or folder
    char command[1024];
    snprintf(command, sizeof(command), "zip -r %s %s", compressed_path, path);
    if (system(command) != 0) {
        perror("Error compressing the file or folder");
        return ERROR;
    }

    // Step 2: Open the compressed file
    int fd = open(compressed_path, O_RDONLY);
    if (fd < 0) {
        perror("Error opening compressed file");
        unlink(compressed_path); // Clean up
        return ERROR;
    }

    // Step 3: Read and send file
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, BUFFER_SIZE)) > 0) {
        if (send(socket_fd, buffer, bytes_read, 0) <= 0) {
            perror("Error sending file chunk");
            close(fd);
            unlink(compressed_path); // Clean up
            return ERROR;
        }
    }

    close(fd);

    // Step 4: Clean up compressed file
    unlink(compressed_path);
    return SUCCESS;
}

// Function to receive a file
int receive_entity(int socket_fd) {
    char output_file[512] = "received_file.zip";
    int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Error creating output file");
        return ERROR;
    }

    // Step 1: Receive file data
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    while ((bytes_received = recv(socket_fd, buffer, BUFFER_SIZE, 0)) > 0) {
        if (write(fd, buffer, bytes_received) != bytes_received) {
            perror("Error writing to output file");
            close(fd);
            unlink(output_file); // Clean up
            return ERROR;
        }
    }

    close(fd);

    // Step 2: Unzip the file
    char command[1024];
    snprintf(command, sizeof(command), "unzip -o %s -d ./received", output_file);
    if (system(command) != 0) {
        perror("Error unzipping received file");
        unlink(output_file); // Clean up
        return ERROR;
    }

    // Clean up the zip file
    unlink(output_file);
    return SUCCESS;
}
