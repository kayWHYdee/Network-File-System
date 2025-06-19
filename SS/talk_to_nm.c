#include "../header.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include "funcdef.h"
// #define SUCCESS 1
#define ERROR -1
sem_t nslock;
int get_free_port()
{
    int port = 0;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("Socket creation failed");
        return -1;
    }

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY};

    while (1)
    {
        port = rand() % 64511 + 1024;
        server_addr.sin_port = htons(port);

        if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
            if (errno == EADDRINUSE)
            {
                continue;
            }
            else
            {
                perror("Bind failed");
                return -1;
            }
        }
        break;
    }

    close(sock);
    return port;
}
int setup_socket_options(int server_socket)
{
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt failed");
        return -1;
    }
    return 0;
}

int create_server_socket_bind(char *ip, int port)
{
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        perror("Socket creation failed");
        return -1;
    }

    if (setup_socket_options(server_socket) < 0)
    {
        close(server_socket);
        return -1;
    }

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = INADDR_ANY};

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Bind failed");
        close(server_socket);
        return -1;
    }
    if (listen(server_socket, SOMAXCONN) < 0)
    {
        perror("Listen failed");
        close(server_socket);
        return -1;
    }
    printf("Listening for incoming connections...\n");

    // Step 3: Accept an incoming connection
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int new_fd = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
    if (new_fd < 0)
    {
        perror("Accept failed");
        close(new_fd);
        return -1;
    }
    printf("Accepted a connection from client\n");

    return new_fd;
}

// Function to create an empty file or directory
int create_entity(const char *base_path, const char *entity_name, int is_directory)
{
    char absolute_path[MAX_PATH_LENGTH];
    strcpy(absolute_path, base_path);
    // Build the absolute path
    if (snprintf(absolute_path, sizeof(absolute_path), "%s/%s", base_path, entity_name) >= sizeof(absolute_path))
    {
        fprintf(stderr, "Error: Path length exceeds maximum limit.\n");
        return -1; // Error: Path too long
    }

    // Check if the path already exists
    struct stat st;
    if (stat(absolute_path, &st) == 0)
    {
        fprintf(stderr, "Error: Entity already exists at path: %s\n", absolute_path);
        return -2; // Error: Entity already exists
    }

    // Create directory or file
    if (is_directory)
    {
        // Create a directory
        if (mkdir(absolute_path, 0755) == 0)
        {
            printf("Success: Directory created at %s\n", absolute_path);
            return 1; // Success
        }
        else
        {
            perror("Error creating directory");
            return -3; // Error creating directory
        }
    }
    else
    {
        // Create an empty file
        FILE *file = fopen(absolute_path, "w");
        if (file)
        {
            fclose(file);
            printf("Success: File created at %s\n", absolute_path);
            return 1; // Success
        }
        else
        {
            perror("Error creating file");
            return -4; // Error creating file
        }
    }
}

// Function to delete a file or directory
int delete_entity(const char *path)
{
    struct stat st;

    // Check if the path exists
    if (stat(path, &st) < 0)
    {
        perror("Error: Failed to stat the path");
        return ERROR;
    }

    if (S_ISDIR(st.st_mode))
    {
        // If it's a directory, delete contents recursively
        DIR *dir = opendir(path);
        if (!dir)
        {
            perror("Error: Failed to open directory");
            return ERROR;
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL)
        {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            {
                continue; // Skip . and ..
            }

            char sub_path[MAX_PATH_LENGTH];
            snprintf(sub_path, sizeof(sub_path), "%s/%s", path, entry->d_name);

            // Recursively delete sub-entries
            if (delete_entity(sub_path) == ERROR)
            {
                closedir(dir);
                return ERROR;
            }
        }

        closedir(dir);

        // Delete the directory itself
        if (rmdir(path) < 0)
        {
            perror("Error: Failed to delete directory");
            return ERROR;
        }
    }
    else
    {
        // If it's a file, delete it
        if (unlink(path) < 0)
        {
            perror("Error: Failed to delete file");
            return ERROR;
        }
    }

    printf("Deleted: %s\n", path);
    return 1;
}
int receive_entity(void* arg)
{
    request_packet *req = (request_packet *)arg;
    int nm_fd = *(req->index);
    printf("old path : %s\n",req->path);
    request_packet *hi = malloc(sizeof(request_packet));
    strcpy(hi->contents,req->contents);
    printf("contenct :%s %s\n",hi->contents,req->contents);
    printf("contenct : %s\n",req->new_path);
    hi->command=req->command;
    hi->index=req->index;
    hi->sync_flag=req->sync_flag;
    strcpy(hi->path,req->path);
    char* ip = get_ip();
    int new_fd = create_server_socket_bind(ip, 9999);
    printf("New fd %d\n", new_fd);
    printf("Server socket created and bound to port 9999\n");
    
    printf("olddd path : %s\n",hi->path);
    req=hi;
    char received_path[MAX_PATH_LENGTH];    
    char r1_path[MAX_PATH_LENGTH];    
    snprintf(received_path, sizeof(received_path), "%s_rcvd.gz", hi->path);
    snprintf(r1_path, sizeof(r1_path), "%s_rcvd", hi->path);
    printf("rcved %s\n",received_path);
    
    int fd = open(received_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
    {
        printf(RED "Error creating received file: %s\n" DEFAULT, received_path);
        return ERROR;
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    long long total_received = 0;
    ssize_t result = recv(new_fd, buffer, BUFFER_SIZE, 0);
    printf("RESULT = %d\n",result);
    long long total_size = atoi(buffer);
    printf("Total size = %lld\n",total_size);
    strcpy(buffer,"ACKNOWLEDGED");
    if (send(new_fd, buffer, BUFFER_SIZE, 0) < 0)
    {
        printf(RED "Error sending acknowledgment for path: %s\n" DEFAULT, req->path);
        return ERROR;
    }
    printf("Receiving file/folder at : %s\n", req->path);
    // Step 2: Receive the file in chunks
    while (total_received < total_size)
    {
        ssize_t bytes_received = recv(new_fd, buffer, BUFFER_SIZE, 0);

        printf("Received %ld bytes\n", bytes_received);

        if (write(fd, buffer, bytes_received) != bytes_received)
        {
            printf(RED "Error writing to file: %s\n" DEFAULT, received_path);
            close(fd);
            unlink(received_path); // Clean up partial file
            return ERROR;
        }
        total_received += bytes_received;
        if (strcmp(buffer, "STOP") == 0)
        {
            break;
        }
    }
    printf("Total received: %lld bytes\n", total_received);

    close(fd);

    // Step 3: Extract the received file
    char extract_command[1024];
        // snprintf(extract_command, sizeof(extract_command), "unzip -o %s -d ./received", received_path);
    printf("old path : %s\n",req->path);
    printf("new path : %s\n",req->new_path);
    snprintf(extract_command, sizeof(extract_command), "gzip -d %s",received_path);
    if (system(extract_command) != 0)
    {
        printf(RED "Error extracting received file: %s\n" DEFAULT, received_path);
        unlink(received_path); // Clean up corrupted zip file
        return ERROR;
    }
    char move_cmd[1024];
    snprintf(move_cmd, sizeof(move_cmd), "mv %s %s/%s", r1_path, req->path, req->contents);
    printf("move command : %s\n",move_cmd);
    if (system(move_cmd) != 0) {
        perror("Error moving and renaming file");
        return ERROR;
    }    

    // Step 4: Clean up the temporary received .zip file
    // if (unlink(received_path) < 0)
    // {
    //     printf(RED "Error removing temporary received file: %s\n" DEFAULT, received_path);
    //     return ERROR;
    // }

    // Step 5: Send acknowledgment
    response_packet response;
    response.error_code = SUCCESS;
    strcpy(response.path, req->path);
    response.ack = 1;

    if (send(nm_fd, &response, sizeof(response_packet), 0) < 0)
    {
        printf(RED "Error sending acknowledgment for path: %s\n" DEFAULT, response.path);
        return ERROR;
    }

    printf(GREEN "File/folder copied successfully: %s\n" DEFAULT, response.path);
    return SUCCESS;
}

int send_entity(void * arg)
{
    request_packet *Command = (request_packet *)arg;
    printf("%s\n",Command->path);
    int nm_fd = *(Command->index);
    char compressed_path[BUFFER_SIZE];
    printf("------ %s\n",Command->path);
    snprintf(compressed_path, BUFFER_SIZE, "%s.gz", Command->path);
    char * ip = get_ip();

    // int freeport = get_free_port();
    
    int new_fd = create_server_socket_bind(ip, 9998);
    printf("New fd %d\n", new_fd);
    if (new_fd < 0)
    {
        printf(RED "Error creating server socket\n" DEFAULT);
        return ERROR;
    }

    printf("Compressed path %s\n",compressed_path);
    // Step 1: Compress the file or folder
    char zip_command[1024];
    snprintf(zip_command, sizeof(zip_command), "gzip -c %s > %s", Command->path,compressed_path);
    printf("zip command %s\n",zip_command);
    if (system(zip_command) != 0)
    {
        printf(RED "Error compressing the file or folder at path: %s\n" DEFAULT, Command->path);
        return ERROR;
    }
    printf("COMPRESSED\n");
    // Step 2: Open the compressed file
    int fd = open(compressed_path, O_RDONLY);
    if (fd < 0)
    {
        printf(RED "Error opening compressed file: %s\n" DEFAULT, compressed_path);
        return ERROR;
    }

    // Step 3: Get the file size
    struct stat file_stat;
    if (fstat(fd, &file_stat) < 0)
    {
        printf(RED "Error retrieving file stats for: %s\n" DEFAULT, compressed_path);
        close(fd);
        return ERROR;
    }
    long long total_size = file_stat.st_size;
    printf("file size %d\n",total_size);
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    memset(buffer, 0, BUFFER_SIZE);
    snprintf(buffer,BUFFER_SIZE,"%ld", total_size);
    ssize_t result = send(new_fd, buffer, BUFFER_SIZE, 0);
    int check = recv(new_fd, buffer, BUFFER_SIZE,0);
    printf("%d check\n",check);



    // Step 4: Send the file in chunks
    long long bytes_sent = 0;
    while ((bytes_read = read(fd, buffer, BUFFER_SIZE)) > 0)
    {
        ssize_t result = send(new_fd, buffer, bytes_read, 0);
        printf("RESULT = %d\n",result);
        if (result < 0)
        {
            printf(RED "Error sending chunk of compressed file\n" DEFAULT);
            close(fd);
            return ERROR;
        }
        bytes_sent += result;
    }
    printf("SENT %d\n",bytes_sent);

    close(fd);

    if (bytes_sent != total_size)
    {
        printf(RED "Mismatch in file size sent and actual size\n" DEFAULT);
        return ERROR;
    }
    else
    {
        printf("Sent %lld bytes\n", bytes_sent);
        printf("%s sent successfully\n", buffer);
    }

    response_packet response;
    response.error_code = SUCCESS;
    strcpy(response.path, Command->path);
    response.ack = 1;
    printf("Sending completion response\n");

    if (send(nm_fd, &response, sizeof(response_packet), 0) < 0)
    {
        printf(RED "Error sending completion response for path: %s\n" DEFAULT, Command->path);
        return ERROR;
    }

    // Step 6: Clean up the temporary compressed file
    if (unlink(compressed_path) < 0)
    {
        printf(RED "Error removing temporary file: %s\n" DEFAULT, compressed_path);
        return ERROR;
    }
    else
        printf("Temporary file removed: %s\n", compressed_path);
    printf(GREEN "File/folder copying started: %s\n" DEFAULT, Command->path);
    return SUCCESS;
}

void *process_req(void *arg)
{
    request_packet *Command = (request_packet *)arg;

    int nm_fd = *(Command->index);
    if (Command->command == CMD_HEARTBEAT)
    {
        // printf(GREEN "Heartbeat received\n" DEFAULT);
        if (send(nm_fd, Command, sizeof(request_packet), 0) <= 0)
        {
            printf(RED "Failed to send heartbeat\n" DEFAULT);
        }
        return NULL;
    }
    printf(CYAN "[SS]: Request Received from Naming Server\n" DEFAULT);
    printf("--%d--%s\n", Command->command,Command->path);
    response_packet *response = (response_packet *)malloc(sizeof(response_packet));
    if (Command->command == CMD_CREATE_FILE)
    {

        printf(GREEN "Creating file at path: %s\n" DEFAULT, Command->path);
        // command->contents assuming the file/folder name to be created

        int result = create_entity(Command->path, Command->contents, Command->file_or_folder_flag);
        if (result == 1)
        {
            // send success code to nm
            response->error_code = SUCCESS;

            // success code to nm at every level;
            printf(GREEN "File created successfully\n" DEFAULT);
        }
        else
        {
            response->error_code = ERROR;
            printf(RED "Failed to create file\n" DEFAULT);
        }

        strcpy(response->path, Command->path);
        send(nm_fd, response, sizeof(response_packet), 0);
    }
    else if (Command->command == CMD_CREATE_FOLDER)
    {
        printf(GREEN "Creating folder at path: %s\n" DEFAULT, Command->path);
        int result = create_entity(Command->path, Command->contents, FOLDER_TYPE);
        if (result == 1)
        {

            response->error_code = SUCCESS;

            printf(GREEN "Folder created successfully\n" DEFAULT);
        }
        else
        {
            response->error_code = ERROR;
            printf(RED "Failed to create folder\n" DEFAULT);
        }
        strcpy(response->path, Command->path);
        send(nm_fd, response, sizeof(response_packet), 0);
    }
    else if (Command->command == CMD_DELETE_FILE)
    {
        printf(GREEN "Deleting file at path: %s\n" DEFAULT, Command->path);
        int result = delete_entity(Command->path);
        // printf("%d\n", result);
        if (result == 1)
        {

            response->error_code = SUCCESS;
            // strcpy(response->path, Command->path);
            printf(GREEN "File deleted successfully\n" DEFAULT);
        }
        else
        {
            response->error_code = ERROR;
            printf(RED "Failed to delete file\n" DEFAULT);
        }
        send(nm_fd, response, sizeof(response_packet), 0);
    }
    else if (Command->command == CMD_DELETE_FOLDER)
    {
        printf(GREEN "Deleting folder at path: %s\n" DEFAULT, Command->path);
        int result = delete_entity(Command->path);
        if (result == 1)
        {
            response->error_code = SUCCESS;
            printf(GREEN "Folder deleted successfully\n" DEFAULT);
        }
        else
        {
            response->error_code = ERROR;
            printf(RED "Failed to delete folder\n" DEFAULT);
        }
        send(nm_fd, response, sizeof(response_packet), 0);
    }
    // else if (Command->command == CMD_COPY_FILE || Command->command == CMD_COPY_FOLDER)
    else if (Command->command == CMD_SEND_TO_NM)
    {
        // if (Command->command == CMD_COPY_FOLDER)
        // {
        // printf(GREEN "Copying Folder from %s to %s\n" DEFAULT, Command->path, Command->new_path);
        // }
        printf(GREEN "Copying from %s to %s\n" DEFAULT, Command->path, Command->new_path);
        int result = send_entity(Command);
        if (result == SUCCESS)
        {
            // printing twice just to double check
            printf(GREEN "File/Folder sent successfully\n" DEFAULT);
        }
        else
        {
            printf(RED "Failed to send file\n" DEFAULT);
        }
    }
    else if (Command->command == CMD_RECEIVE_FROM_NM)
    {
        printf(GREEN "Receiving file/folder\n" DEFAULT);
        int result = receive_entity(Command);
        if (result == SUCCESS)
        {
            printf(GREEN "File/Folder received successfully\n" DEFAULT);
        }
        else
        {
            printf(RED "Failed to receive file\n" DEFAULT);
        }
    }
    else if (Command->command == CMD_READ)
    {
        // do similar thing or if you want I can add the specificities here
        // the path that will come will be of the file and inside that will be the file where it will happen
        // just ensure ki wahi hai and paste it there
    }
    else if (Command->command == CMD_WRITE)
    {
        // do similar thing or if you want I can add the specificities here
        // the path that will come will be of the file and inside that will be the file where it will happen
        // just ensure ki wahi hai and paste it there
    }
    else if (Command->command == CMD_STREAM)
    {
        // do similar thing or if you want I can add the specificities here
        // the path that will come will be of the file and inside that will be the file where it will happen
        // just ensure ki wahi hai and paste it there
    }
    else if (Command->command == 19) // backup
    {
        // do similar thing or if you want I can add the specificities here
        // the path that will come will be of the backup folder and inside that will be the backup where it will happen
        // just ensure ki wahi hai and paste it there
    }

    return 0;
}
// Function to copy files and directories

void *ns_req(void *arg)
{
    printf(CYAN "[SS]: Thread Created For Request From Naming Server\n" DEFAULT);
    int *index = (int *)arg;
    request_packet *request = (request_packet *)malloc(sizeof(request_packet));
    while (1)
    {
        if (recv(*index, request, sizeof(request_packet), 0) <= 0)
        {
            printf("[SS]: Naming Server Closed The Connection\n");
            close(*index);
            free(index);
            return NULL;
        }
        // printf("Size: %lld\n", command->size);
        // printf(CYAN "[SS]: Request Received from Naming Server\n" DEFAULT);
        // cont_thread *temp = (cont_thread *)malloc(sizeof(cont_thread));
        // temp->command = (packet *)malloc(sizeof(packet));
        // temp->conn = (int *)malloc(sizeof(int));
        // *(temp->command) = *command;
        // *(temp->conn) = *index;
        request->index = (int *)index;
        pthread_t thread;
        pthread_create(&thread, NULL, &process_req, request);
    }
    return NULL;
}

//  cross check once and then comment out
void *ns_direct_con(void *arg)
{
    // Extract storage server information from argument
    StorageServer *storage_server = (StorageServer *)arg;
    int sfd_nserver, length;
    struct sockaddr_in serverAddress, client;

    printf(CYAN "Starting connection on port: %d\n" DEFAULT, storage_server->port_nm);

    // Create a socket for the naming server
    sfd_nserver = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd_nserver == -1)
    {
        printf(RED "ERROR %d: Socket not created\n" DEFAULT, INIT_ERROR);
        exit(EXIT_FAILURE);
    }

    // Configure server address structure
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(storage_server->port_nm);

    // Bind the socket to the specified port
    if (bind(sfd_nserver, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
    {
        printf(RED "ERROR %d: Couldn't bind socket\n" DEFAULT, INIT_ERROR);
        close(sfd_nserver);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(sfd_nserver, SOMAXCONN) == -1)
    {
        printf(RED "ERROR %d: Listening failed\n" DEFAULT, INIT_ERROR);
        close(sfd_nserver);
        exit(EXIT_FAILURE);
    }

    // Initialize semaphore for thread-safe operations
    sem_init(&nslock, 0, 1);

    int count = 0;
    length = sizeof(serverAddress);
    pthread_t *nsrequest = (pthread_t *)malloc(sizeof(pthread_t));
    int nscon;
    // printf("

    while (1)
    {
        sem_wait(&nslock);
        // Accept incoming connection from naming server
        nscon = accept(sfd_nserver, (struct sockaddr *)&serverAddress, (socklen_t *)&length);
        if (nscon < 0)
        {
            printf(RED "ERROR %d: Couldn't establish connection\n" DEFAULT, BAD_REQUEST);
            sem_post(&nslock);
        }
        else
        {
            printf(CYAN "SUCCESS %d: Connection Established Successfully\n" DEFAULT, SUCCESS);
            count++;

            // Allocate memory for the new connection index
            int *index = (int *)malloc(sizeof(int));
            *index = nscon;

            // Reallocate thread array for the new thread
            nsrequest = (pthread_t *)realloc(nsrequest, count * sizeof(pthread_t));
            sem_post(&nslock);

            // Create a thread to handle the request
            pthread_create(&nsrequest[count - 1], NULL, &ns_req, index);
        }
    }

    // Join all threads before exiting
    for (int i = 0; i < count; i++)
    {
        pthread_join(nsrequest[i], NULL);
    }

    // Clean up resources
    close(sfd_nserver);
    free(nsrequest);
    return NULL;
}
