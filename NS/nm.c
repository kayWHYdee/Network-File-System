#include "../header.h"
#define MAX_STORAGE_SERVERS 10
#define MAX_CHUNKS 100
extern NamingServer nm;
extern Trie_Struct *ALL_PATHS;
extern pthread_mutex_t response_mutex;

void initialize_server(void);
void cleanup_server(void);

void get_public_ip(void);
int create_server_socket(int port);
int setup_socket_options(int server_socket);

void *client_handling_thread(void *arg);
void *handle_client_messages(void *arg);
int process_client_request(int client_socket, request_packet *req);
int handle_client_connection(int client_socket, struct sockaddr_in *client_addr);

void *ss_handling_thread(void *arg);
void *handle_ss_messages(void *arg);
int handle_ss_connection(int ss_socket, struct sockaddr_in *ss_addr);
int process_ss_heartbeat(StorageServer *ss);
// int handle_ss_initialization(StorageServer *ss);
char *get_ip();

// Backup handling functions
int handle_backup_operations(StorageServer *current_ss, request_packet *req, int client_socket);
// int handle_file_operations(int client_socket, request_packet *req, StorageServer *ss);
void setup_backup_configuration(StorageServer *ss, int num_ss);
int handle_copy(StorageServer *current_ss, request_packet *req, int backup_flag);
// #endif

// server.c
// #include "server.h"

NamingServer nm;
Trie_Struct *ALL_PATHS;
pthread_mutex_t response_mutex = PTHREAD_MUTEX_INITIALIZER;

void log_message(const char *msg)
{
    FILE *file = fopen("log.txt", "a");
    if (file == NULL)
    {
        perror("Failed to open log file");
        return;
    }

    fprintf(file, "%s\n", msg);
    fclose(file);
}

void initialize_server(void)
{
    ALL_PATHS = initializeTrie();
    printf("Initializing cache\n");
    init_cache_array(); // Initialize cache array
    printf("Cache initialized\n");

    memset(&nm, 0, sizeof(NamingServer));
    if (pthread_mutex_init(&nm.lock, NULL) != 0)
    {
        perror("Mutex initialization failed");
        exit(1);
    }
}

void cleanup_server(void)
{
    pthread_mutex_destroy(&nm.lock);
    freeTrie(ALL_PATHS); // Free the Trie
    // Add cleanup for cache here if needed
}

int create_server_socket(int port)
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

    return server_socket;
}

int create_server_socket_connect(char *ip, int port)
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
        .sin_addr.s_addr = inet_addr(ip)};

    if (connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Connection to the server failed");
        close(server_socket);
        return -1;
    }

    return server_socket;
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

void *backup_all(void *args)
{
    int *indices = (int *)args;
    int backup_from = indices[0];
    int backup_to = indices[1];
    printf("Backup start from %d to %d\n", backup_from, backup_to);
    StorageServer *ss = &nm.ss[backup_from];

    // char *backup_path = (char *)malloc(strlen("backup/SS_") + 10);
    // snprintf(backup_path, strlen("backup/SS_") + 10, "backup/SS_%d", backup_from);
    // printf("Backup path: %s\n", backup_path);
    // req.command = CMD_CREATE_FOLDER;
    // strcpy(req.path, backup_path);
    // if (send(nm.ss[backup_to].socket_fd, &req, sizeof(req), 0))
    // {
    //     perror("Failed to send create folder request");
    // }

    // sleep(2);
    for (int i = 0; i < nm.ss[backup_from].num_paths; i++)
    {
        char *path = nm.ss[backup_from].accessible_paths[i];
        printf("file path: %s\n", path);

        char *destination;
        destination = (char *)malloc(strlen("backup/SS_") + strlen(path) + 10);
        snprintf(destination, strlen("backup/SS_") + strlen(path) + 10, "backup/SS_%d/%s", backup_to, path);
        char *backup_path = (char *)malloc(strlen("backup/SS_") + 10);
        snprintf(backup_path, strlen("backup/SS_") + 10, "backup/SS_%d", backup_to);

        printf("Destination: %s\n", backup_path);

        request_packet req;
        req.command = CMD_COPY_FILE;

        strcpy(req.path, path);
        strcpy(req.new_path, backup_path);

        handle_copy(ss, &req, backup_to );
    }
    printf("Backup done from %d to %d\n", backup_from, backup_to);
    return NULL;
}
void setup_backup_configuration(StorageServer *ss, int num_ss)
{
    if (num_ss == 1)
    {
        nm.ss[0].backup_index = -1;
        nm.ss[0].backup2_index = -1;
    }
    else if (num_ss == 2)
    {
        nm.ss[0].backup_index = -1;
        nm.ss[0].backup2_index = -1;
        nm.ss[1].backup_index = -1;
        nm.ss[1].backup2_index = -1;
    }
    if (num_ss == 3)
    {

        nm.ss[0].backup_index = 1;
        nm.ss[0].backup2_index = 2;
        nm.ss[1].backup_index = 2;
        nm.ss[1].backup2_index = 0;
        nm.ss[2].backup_index = 0;
        nm.ss[2].backup2_index = 1;

        pthread_t threads[6];
        int indices[6][2] = {0, 1};
        for (int i = 0; i < 3; i++)
        {
            int backup_from = i;
            int backup_to = nm.ss[i].backup_index;

            memcpy(indices[i], (int[]){backup_from, backup_to}, sizeof(indices[i]));
            printf("Backuppppfrom %d to %d\n", backup_from, backup_to);
            if (pthread_create(&threads[i], NULL, backup_all, &(indices[i])))
            {
                perror("Failed to create backup thread");
            }
            pthread_join(threads[i], NULL);
        }
        for (int i = 3; i < 6; i++)
        {
            int backup_from = i % 3;
            int backup_to = nm.ss[i % 3].backup2_index;

            memcpy(indices[i], (int[]){backup_from, backup_to}, sizeof(indices[i]));
            printf("Backudoen ppfrom %d to %d\n", backup_from, backup_to);
            if (pthread_create(&threads[i], NULL, backup_all, &(indices[i])))
            {
                perror("Failed to create backup thread");
            }
            pthread_join(threads[i], NULL);
        }
    }
    else if (num_ss > 3)
    {
        ss->backup_index = num_ss - 1;
        ss->backup2_index = num_ss - 2;

        pthread_t thread1, thread2;
        int indices1[2] = {num_ss, num_ss - 1};
        pthread_t threads[num_ss];

        if (pthread_create(&thread1, NULL, backup_all, &(indices1)))
        {
            perror("Failed to create backup thread");
        }
        int indices2[2] = {num_ss, num_ss - 2};
        if (pthread_create(&thread2, NULL, backup_all, &(indices2)))
        {
            perror("Failed to create backup thread");
        }
    }
}

int handle_client_connection(int client_socket, struct sockaddr_in *client_addr)
{
    pthread_mutex_lock(&nm.lock);

    if (nm.num_clients >= MAX_CLIENTS)
    {
        const char *msg = "Server full";
        send(client_socket, msg, strlen(msg), 0);
        pthread_mutex_unlock(&nm.lock);
        return -1;
    }

    Client *client = &nm.clients[nm.num_clients++];
    client->socket_fd = client_socket;
    client->is_connected = 1;
    inet_ntop(AF_INET, &client_addr->sin_addr, client->ip, sizeof(client->ip));
    client->port = ntohs(client_addr->sin_port);

    // log the client connection
    char *message = (char *)malloc(100);
    sprintf(message, "Client connected: IP=%s, Port=%d\n", client->ip, client->port);
    log_message(message);

    int *client_socket_ptr = malloc(sizeof(int));
    *client_socket_ptr = client_socket;

    if (pthread_create(&client->handler_thread, NULL, handle_client_messages, client_socket_ptr) != 0)
    {
        perror("Failed to create client handler thread");
        free(client_socket_ptr);
        nm.num_clients--;
        pthread_mutex_unlock(&nm.lock);
        return -1;
    }

    pthread_mutex_unlock(&nm.lock);
    return 0;
}

void *client_handling_thread(void *arg)
{
    int server_socket = create_server_socket(NM_CLIENT_PORT);
    if (server_socket < 0)
        return NULL;

    if (listen(server_socket, MAX_CLIENTS) < 0)
    {
        perror("Listen failed");
        close(server_socket);
        return NULL;
    }

    printf("Naming server listening for clients on port %d\n", NM_CLIENT_PORT);

    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);

        if (client_socket < 0)
        {
            perror("Accept failed");
            continue;
        }

        if (handle_client_connection(client_socket, &client_addr) < 0)
        {
            close(client_socket);
        }
    }

    close(server_socket);
    return NULL;
}

void *ss_handling_thread(void *arg)
{
    int server_socket = create_server_socket(NM_SS_PORT); // TODO
    if (server_socket < 0)
        return NULL;

    if (listen(server_socket, MAX_STORAGE_SERVERS) < 0)
    {
        perror("Listen failed");
        close(server_socket);
        return NULL;
    }

    printf("Naming server listening for storage servers on port %d\n", NM_SS_PORT);

    while (1)
    {
        struct sockaddr_in ss_addr;
        socklen_t addr_len = sizeof(ss_addr);
        int ss_socket = accept(server_socket, (struct sockaddr *)&ss_addr, &addr_len);
        printf("HERE\n");

        if (ss_socket < 0)
        {
            perror("Accept failed");
            continue;
        }

        if (handle_ss_connection(ss_socket, &ss_addr) < 0)
        {
            close(ss_socket);
        }
    }

    close(server_socket);
    return NULL;
}

int main(void)
{
    initialize_server();
    // get_public_ip();
    char *ip = get_ip();
    printf("Naming Server IP: %s\n", ip);

    pthread_t client_thread, ss_thread;

    if (pthread_create(&client_thread, NULL, client_handling_thread, NULL) != 0)
    {
        perror("Failed to create client handling thread");
        return 1;
    }

    if (pthread_create(&ss_thread, NULL, ss_handling_thread, NULL) != 0)
    {
        perror("Failed to create storage server handling thread");
        return 1;
    }

    pthread_join(client_thread, NULL);
    pthread_join(ss_thread, NULL);

    cleanup_server();
    return 0;
}

// client_handler.c
// #include "server.h"

void *handle_client_messages(void *arg)
{
    int client_socket = *(int *)arg;
    free(arg);

    while (1)
    {
        request_packet *req = (request_packet *)malloc(sizeof(request_packet));

        ssize_t bytes_received = recv(client_socket, req, sizeof(request_packet), 0);
        if (bytes_received <= 0)
        {
            printf("Client disconnected\n");
            break;
        }

        // log the request
        char *message = (char *)malloc(100);
        sprintf(message, "Request: command=%d, path=%s\n", req->command, req->path);
        log_message(message);

        if (req->command == ACK)
        {
            printf(GREEN "Received ACK from client\n" DEFAULT);
            continue;
        }

        if (process_client_request(client_socket, req) != 0)
        {
            printf("Failed to process client request\n");
            // break;
        }
    }

    close(client_socket);
    return NULL;
}

int handle_get_path_request(int client_socket, request_packet *req)
{
    printf("Handling Request to Get Paths in NS\n");

    char *path = req->path;
    char *result[MAX_PATH_LENGTH];
    int count = 0;

    listFilesAndFolders(ALL_PATHS, path, result, &count);
    printf("Retrieved files to be Listed, Count: %d\n", count);

    if (count == 0)
    {
        response_packet res = {.error_code = PATH_NOT_FOUND};
        send(client_socket, &res, sizeof(res), MSG_NOSIGNAL);
        return PATH_NOT_FOUND;
    }

    List list_of_paths;
    list_of_paths.npaths = count;
    for (int i = 0; i < count; i++)
    {
        strncpy(list_of_paths.paths[i], result[i], MAX_PATH_LENGTH);
        free(result[i]);
    }

    if (send(client_socket, &list_of_paths, sizeof(list_of_paths), MSG_NOSIGNAL) < 0)
    {
        perror("Send failed");
        return -1;
    }

    printf("Sent paths to client\n");

    return 0;
}

int handle_read_on_server_failure(StorageServer *current_ss, request_packet *req, int client_socket)
{
    // int client_socket = current_ss->socket_fd;
    printf("In handle_read_on_server_failure\n");

    if (current_ss->backup_index == -1)
    {
        response_packet res = {.error_code = SS_DOWN_AND_NO_BACKUP};
        printf("No backup server available\n");

        if (send(client_socket, &res, sizeof(res), MSG_NOSIGNAL) < 0)
        {
            perror("Send failed");
        }
        return SS_DOWN_AND_NO_BACKUP;
    }
    if (req->command != CMD_READ)
    {
        printf("Backup server is active. Only read operations are allowed\n");
        response_packet res = {.error_code = BACKUP_ACTIVE};
        if (send(client_socket, &res, sizeof(res), MSG_NOSIGNAL) < 0)
        {
            perror("Send failed");
        }
        return BACKUP_ACTIVE;
    }

    int backup_index = current_ss->backup_index;
    StorageServer *backup_ss = &nm.ss[backup_index];

    response_packet res;
    memset(&res, 0, sizeof(res));

    if (backup_ss->is_connected)
    {
        res.error_code = SUCCESS;
        res.port = backup_ss->port_client;
        strncpy(res.ip, backup_ss->ip, sizeof(res.ip) - 1);
        if (send(client_socket, &res, sizeof(res), 0))
        {
            perror("Send failed");
            return -1;
        }
        return SUCCESS;
    }

    int backup2_index = current_ss->backup2_index;
    StorageServer *backup2_ss = &nm.ss[backup2_index];

    if (backup2_ss->is_connected)
    {
        res.error_code = SUCCESS;
        res.port = backup2_ss->port_client;
        strncpy(res.ip, backup2_ss->ip, sizeof(res.ip) - 1);
        if (send(client_socket, &res, sizeof(res), MSG_NOSIGNAL))
        {
            perror("Send failed");
            return -1;
        }
        return SUCCESS;
    }

    res.error_code = -1;
    res.port = 0;
    res.ip[0] = '\0';
    if (send(client_socket, &res, sizeof(res), MSG_NOSIGNAL))
    {
        perror("Send failed");
        return -1;
    }
    return 0;
}

#include <pthread.h>

typedef struct
{
    StorageServer *backup_ss;
    request_packet *req;
} BackupArgs;

void *backup_thread(void *args)
{
    BackupArgs *backup_args = (BackupArgs *)args;
    StorageServer *backup_ss = backup_args->backup_ss;
    request_packet *req = backup_args->req;

    if (backup_ss->socket_fd >= 0)
    {
        if (send(backup_ss->socket_fd, req, sizeof(*req), MSG_NOSIGNAL) < 0)
        {
            perror("Failed to send to backup server");
        }
        else
        {
            printf("Backup successful on backup server\n");
        }
    }

    free(backup_args);
    return NULL;
}

void create_backup(StorageServer *current_ss, request_packet *req)
{
    int backup_index = current_ss->backup_index;
    int backup2_index = current_ss->backup2_index;

    StorageServer *backup_ss = &nm.ss[backup_index];
    StorageServer *backup2_ss = &nm.ss[backup2_index];

    pthread_t thread1, thread2;

    BackupArgs *args1 = malloc(sizeof(BackupArgs));
    args1->backup_ss = backup_ss;
    args1->req = req;

    if (pthread_create(&thread1, NULL, backup_thread, args1) != 0)
    {
        perror("Failed to create thread for backup_ss");
        free(args1);
    }
    else
    {
        pthread_detach(thread1); // Detach the thread to ensure NM doesn't wait
    }

    BackupArgs *args2 = malloc(sizeof(BackupArgs));
    args2->backup_ss = backup2_ss;
    args2->req = req;

    if (pthread_create(&thread2, NULL, backup_thread, args2) != 0)
    {
        perror("Failed to create thread for backup2_ss");
        free(args2);
    }
    else
    {
        pthread_detach(thread2); // Detach the thread to ensure NM doesn't wait
    }
}

int handle_copy(StorageServer *current_ss, request_packet *req, int backup_flag) // TODO async karna hai isko
{
    printf("Handling copy request\n");
    printf("Path %s\n", req->path);
    printf("New path %s\n", req->new_path);
    int path_index = searchFile(ALL_PATHS, req->path);
    if (path_index == -1)
    {
        printf("File does not exist\n");
        return PATH_NOT_FOUND;
    }
    char *combined_path = (char *)malloc(strlen(req->path) + strlen(req->new_path) + 2);
    strcpy(combined_path, req->path);
    strcat(combined_path, "/");
    strcat(combined_path, req->new_path);

    int combined_index = searchFile(ALL_PATHS, combined_path);
    if (combined_index != -1)
    {
        printf("Another file with same path exists\n");
        return FILE_ALREADY_EXISTS;
    }
    int dest_index;

    if (backup_flag != -1)
    {
        dest_index = backup_flag;
    }
    else
    {
        // create_backup(current_ss, req);
        dest_index = searchFile(ALL_PATHS, req->new_path);
        if (dest_index == -1)
        {
            printf("Destination does not exists\n");
            return PATH_NOT_FOUND;
        }
    }

    printf("Sending request to source\n");
    request_packet copy_message_for_source;
    // memcpy(copy_message_for_source, req, sizeof(request_packet));
    printf("Path %s\n", req->path);
    strcpy(copy_message_for_source.path, req->path);

    copy_message_for_source.command = CMD_SEND_TO_NM;
    // copy_message_for_source.path = req->path;
    printf("source_index %d\n", current_ss->ss_index);
    printf("source_fd %d\n", current_ss->socket_fd);
    strcpy(copy_message_for_source.new_path, req->new_path);
    int source_fd = current_ss->socket_fd;
    if (send(source_fd, &copy_message_for_source, sizeof(request_packet), 0) < 0)
    {
        perror("Send failed");
        return SOURCE_ERROR;
    }
    sleep(1);

    int source_fd_new = create_server_socket_connect(current_ss->ip, 9998);
    printf("source index %d source_fd %d\n", current_ss->ss_index, source_fd_new);

    request_packet copy_message_for_dest;

    int dest_fd = nm.ss[dest_index].socket_fd;
    printf("dest_fd %d\n", dest_fd);

    printf("Sending request to dest\n");
    copy_message_for_dest.command = CMD_RECEIVE_FROM_NM;
    strcpy(copy_message_for_dest.path, req->new_path);
    strcpy(copy_message_for_dest.contents, req->path);

    if (send(dest_fd, &copy_message_for_dest, sizeof(request_packet), 0) < 0)
    {
        perror("Send failed");
        return DEST_ERROR;
    }
    printf("New path %s\n", req->new_path);
    sleep(1);

    printf("Sending data start\n");
    char buffer[BUFFER_SIZE];
    char received_data[MAX_CHUNKS][BUFFER_SIZE];
    size_t chunk_sizes[MAX_CHUNKS];
    int chunk_count = 0;

    // First loop: Receive all chunks
    memset(buffer, 0, BUFFER_SIZE);
    ssize_t hi = recv(source_fd_new, buffer, BUFFER_SIZE, 0);
    if (hi < 0)
    {
        perror("Receive failed");
        return SOURCE_ERROR;
    }
    printf("buffer = %s\n", buffer);
    int received_int = atoi(buffer);
    printf("Received integer: %d\n", received_int);
    strcpy(buffer, "ACKNOWLEDGED");
    int reii = send(source_fd_new, buffer, BUFFER_SIZE, 0);
    if (reii < 0)
    {
        perror("Send failed");
        return SOURCE_ERROR;
    }

    int total_bytes_received = 0;
    while (1)
    {
        ssize_t bytes_received = recv(source_fd_new, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0)
        {
            perror("Receive failed");
            return SOURCE_ERROR;
        }
        printf("Received %ld bytes\n", bytes_received);
        total_bytes_received += bytes_received;
        printf("The biffer is %s\n", buffer);

        printf("Received chunk %d: %ld bytes\n", chunk_count, bytes_received);
        printf("Buffer: %s\n", buffer);

        // Store the received data and its size
        memcpy(received_data[chunk_count], buffer, bytes_received);
        chunk_sizes[chunk_count] = bytes_received;

        // Check if this is the last chunk

        chunk_count++;
        if (chunk_count >= MAX_CHUNKS)
        {
            printf("Maximum chunk limit reached\n");
            break;
        }
        if (strcmp(buffer, "STOP") == 0 || total_bytes_received >= received_int)
        {

            printf("All chunks received\n");
            break;
        }
    }
    printf("Received all chunks\n");
    // Second loop: Send all received chunks
    int dest_fd_new = create_server_socket_connect(current_ss->ip, 9999);
    printf("dest index %d dest_fd %d\n", dest_index, dest_fd_new);
    if (dest_fd_new < 0)
    {
        perror("Destination socket creation failed");
        return DEST_ERROR;
    }
    snprintf(buffer, BUFFER_SIZE, "%d", total_bytes_received);
    if (send(dest_fd_new, buffer, BUFFER_SIZE, 0) < 0)
    {
        perror("Send failed");
        return DEST_ERROR;
    }
    printf("Sent total bytes: %d\n", total_bytes_received);
    if (recv(dest_fd_new, buffer, BUFFER_SIZE, 0) <= 0)
    {
        perror("Receive failed");
        return DEST_ERROR;
    }
    for (int i = 0; i <= chunk_count; i++)
    {
        if (send(dest_fd_new, received_data[i], chunk_sizes[i], 0) < 0)
        {
            perror("Send failed");
            return DEST_ERROR;
        }
        printf("Sent chunk %d: %ld bytes\n", i, chunk_sizes[i]);
    }

    printf("File sent successfully\n");

    response_packet res;
    if (recv(source_fd, &res, sizeof(res), 0) <= 0)
    {
        perror("Receive failed");
        return SOURCE_ERROR;
    }
    printf("Received response from source\n");
    printf("Error code: %d\n", res.error_code);
    if (recv(dest_fd, &res, sizeof(res), 0) <= 0)
    {
        perror("Receive failed");
        return DEST_ERROR;
    }

    printf("Received response from dest\n");
    printf("Error code: %d\n", res.error_code);

    printf("Sending response to client\n");
    printf("Error code: %d\n", res.error_code);

    // insertF
    // sujal todo copy file
    printf("Files copied successfully\n");
    return SUCCESS;
}

int handle_file_operations(StorageServer *current_ss, request_packet *req, int client_socket)
{
    int ss_socket = current_ss->socket_fd;
    if (ss_socket < 0)
    {
        perror("Storage server socket creation failed");
        return -1;
    }
    printf("%d\n", req->command);

    response_packet *res = (response_packet *)malloc(sizeof(response_packet));
    // memset(&res, 0, sizeof(res));
    if (req->command <= 4)
    {
        printf("ip %s, prt %d\n", current_ss->ip, current_ss->port_client);
        res->error_code = SUCCESS;
        strncpy(res->ip, current_ss->ip, sizeof(res->ip));
        res->port = current_ss->port_client;
        res->ack = 1;
        printf("ip %s, prt %d\n", res->ip, res->port);

        log_request_response(*req, *res); // Log the response

        if (send(client_socket, res, sizeof(*res), 0) < 0)
        {
            perror("Send failed");
            return -1;
        }
        return 0;
    }

    if (req->command == CMD_CREATE_FILE || req->command == CMD_CREATE_FOLDER)
    {
        printf("Creating file/folder at the path: %s\n", req->path);
        printf("Contents (file name): %s\n", req->contents);
        // int sexy = searchFile(ALL_PATHS, file_name_with_path);
        // printf("Index to add the file %d\n", sexy);

        // content has file_name
        // path has path
        char *file_name = req->contents;
        char *file_name_with_path = (char *)malloc(strlen(req->path) + strlen(file_name) + 2);
        strcpy(file_name_with_path, req->path);
        strcat(file_name_with_path, "/");
        strcat(file_name_with_path, file_name);

        printf("File name with path %s\n", file_name_with_path);

        int new_path_index = searchFile(ALL_PATHS, file_name_with_path);
        if (new_path_index != -1)
        {
            printf("Another file with same path exists\n");
            response_packet res;
            memset(&res, 0, sizeof(res));
            res.error_code = FILE_ALREADY_EXISTS;

            log_request_response(*req, res); // Log the response

            if (send(client_socket, &res, sizeof(res), MSG_NOSIGNAL) < 0)
            {
                perror("Send failed");
                return -1;
            }
            return FILE_ALREADY_EXISTS;
        }
        // int nm_fd = create_server_socket(current_ss->port_nm);
        int nm_fd = current_ss->socket_fd; // might need to change this
        printf("Sending request to storage server\n");
        if (send(nm_fd, req, sizeof(*req), MSG_NOSIGNAL) < 0)
        {
            perror("Send failed");
            return -1;
        }
        response_packet ss_res;
        if (recv(nm_fd, &ss_res, sizeof(ss_res), 0) <= 0)
        {
            perror("Receive failed");
            return -1;
        }
        printf("Received response from storage server\n");
        printf("Error code: %d\n", ss_res.error_code);

        log_request_response(*req, ss_res); // Log the response

        if (send(client_socket, &ss_res, sizeof(ss_res), MSG_NOSIGNAL) < 0)
        {
            perror("Send failed");
            return -1;
        }
        insertFile(ALL_PATHS, file_name_with_path, current_ss->ss_index);
        printf("added %s\n", file_name_with_path);
        add_to_cache(current_ss->ss_index, file_name_with_path); // Add to cache

        // create_backup(current_ss, req);
        return 0;
    }
    if (req->command == CMD_DELETE_FILE || req->command == CMD_DELETE_FOLDER)
    {
        // char *file_name = req->contents;
        char *file_name_with_path = (char *)malloc(strlen(req->path) + 2);
        strcpy(file_name_with_path, req->path);
        printf("File name with path %s\n", file_name_with_path);
        // strcat(file_name_with_path, "/");
        // strcat(file_name_with_path, file_name);

        int current_ss_index = searchFile(ALL_PATHS, req->path);
        if (current_ss_index == -1)
        {
            printf("File does not exist file\n");
            response_packet res;
            memset(&res, 0, sizeof(res));
            res.error_code = PATH_NOT_FOUND;

            log_request_response(*req, res); // Log the response

            if (send(client_socket, &res, sizeof(res), MSG_NOSIGNAL) < 0)
            {
                perror("Send failed");
                return -1;
            }
            return PATH_NOT_FOUND;
        }
        strcpy(req->path, file_name_with_path);
        int nm_fd = current_ss->socket_fd;

        printf("%s\n", req->path);
        printf("Sending request to storage server\n");
        if (send(nm_fd, req, sizeof(request_packet), 0) < 0)
        {
            perror("Send failed");
            return -1;
        }
        response_packet ss_res;
        if (recv(nm_fd, &ss_res, sizeof(ss_res), 0) <= 0)
        {
            perror("Receive failed");
            return -1;
        }
        printf("Received response from storage server\n");
        printf("Error code: %d\n", ss_res.error_code);

        log_request_response(*req, ss_res); // Log the response

        if (send(client_socket, &ss_res, sizeof(ss_res), MSG_NOSIGNAL) < 0)
        {
            perror("Send failed");
            return -1;
        }
        if (req->command == CMD_DELETE_FILE)
            deleteFile(ALL_PATHS, req->path);
        else if (req->command == CMD_DELETE_FOLDER)
            deleteFolder(ALL_PATHS, req->path, current_ss->ss_index); // SUSAL

        remove_from_cache(req->path); // Remove from cache
        // create_backup(current_ss, req);
        return 0;
    }

    if (req->command == CMD_COPY_FILE || req->command == CMD_COPY_FOLDER)
    {

        int check = handle_copy(current_ss, req, -1);
        if (check != SUCCESS)
        {
            response_packet res = {.error_code = check};
            log_request_response(*req, res); // Log the response
            if (send(client_socket, &res, sizeof(res), MSG_NOSIGNAL) < 0)
            {
                perror("Send failed");
                return -1;
            }
            return 0;
        }
        response_packet res = {.error_code = SUCCESS};
        res.ack = 1;

        if (send(client_socket, &res, sizeof(res), MSG_NOSIGNAL) < 0)
        {
            perror("Send failed");
            return -1;
        }

        // create_backup(current_ss, req);

        char *combined_path = (char *)malloc(strlen(req->path) + strlen(req->new_path) + 2);
        strcpy(combined_path, req->path);
        strcat(combined_path, "/");
        strcat(combined_path, req->new_path);
        add_to_cache(current_ss->ss_index, combined_path);
        insertFile(ALL_PATHS, combined_path, current_ss->ss_index);
        return 0;
    }

    return 0;
}
int process_client_request(int client_socket, request_packet *req)
{
    printf("Inside the fn process_client_request; Path: %s, Command:%d \n", req->path, req->command);
    // Log the client request
    response_packet res;
    memset(&res, 0, sizeof(res));
    log_request_response(*req, res);
    if (req->command == CMD_GET_PATH)
    {
        handle_get_path_request(client_socket, req);
        return 0;
    }

    int index = get_storage_server_index(req->path); // Check cache first
    if (index == -1)
    {
        index = searchFile(ALL_PATHS, req->path);
        if (index != -1)
        {
            add_to_cache(index, req->path); // Add to cache if found
        }
    }
    printf("INdex %d \n", index);
    if (index == -1)
    {
        response_packet res = {.error_code = PATH_NOT_FOUND};
        log_request_response(*req, res); // Log the response
        return send(client_socket, &res, sizeof(res), MSG_NOSIGNAL);
    }

    StorageServer *current_ss = &nm.ss[index];
    if (!current_ss->is_connected)
    {
        printf("Storage server not connected\n");
        int check = handle_read_on_server_failure(current_ss, req, client_socket);
        printf("Check: %d\n", check);
        return check;
    }

    if (req->command == CMD_STREAM)
    {
        response_packet res;
        res.error_code = SUCCESS;
        strncpy(res.ip, current_ss->ip, sizeof(res.ip) - 1);
        res.port = current_ss->port_client;
        res.ack = 1;

        log_request_response(*req, res); // Log the response

        if (send(client_socket, &res, sizeof(res), MSG_NOSIGNAL) < 0)
        {
            perror("Send failed");
            return -1;
        }
        return 0;
    }

    return handle_file_operations(current_ss, req, client_socket);
}
int process_ss_heartbeat(StorageServer *ss)
{
    request_packet heartbeat = {.command = CMD_HEARTBEAT};

    if (send(ss->socket_fd, &heartbeat, sizeof(request_packet), 0) < 0)
    {
        perror("Send failed");
        printf("Storage server %d disconnected\n", ss->ss_index);
        // log_ss_failure(ss->ip, ss->port_nm);
        ss->is_connected = 0;
        return -1;
    }
    ssize_t bytes_received = recv(ss->socket_fd, &heartbeat, sizeof(heartbeat), 0);

    if (bytes_received <= 0)
    {
        ss->is_connected = 0;
        printf("Storage server %d disconnected\n", ss->ss_index);
        log_ss_failure(ss->ip, ss->port_nm); // Log the failure
        return -1;
    }

    sleep(HEARTBEAT_INTERVAL);

    return 0;
}
// void send_request(StorageServer *ss, int nm_socket)
// {
//     request_packet req;
//     // req.command = CMD_HEARTBEAT;
//     req.command = CMD_CREATE_FILE;
//     // int socket = create_server_socket(ss->port_nm);
//     if (send(nm_socket, &req, sizeof(req), MSG_NOSIGNAL) < 0)
//     {
//         perror("Send failed");
//         printf("Storage server %d disconnected\n", ss->ss_index);
//         // log_ss_failure(ss->ip, ss->port_nm);
//         ss->is_connected = 0;
//     }
// }
void *handle_ss_messages(void *arg)
{
    printf("Inside handle_ss_messages\n");
    int initial_socket = *(int *)arg;
    free(arg);
    // int initial_socket = &arg;
    printf("Initial socket: %d\n", initial_socket);
    StorageServer received_ss;

    ssize_t bytes_received = recv(initial_socket, &received_ss, sizeof(StorageServer), 0);
    if (bytes_received <= 0)
    {
        perror("Failed to receive initial setup from storage server");
        close(initial_socket);
        return NULL;
    }
    printf("Received storage server: %s:%d\n", received_ss.ip, received_ss.port_nm);
    StorageServer *ss = (StorageServer *)malloc(sizeof(StorageServer));
    int new_ss = 1;
    for (int i = 0; i < nm.num_ss; i++)
    {
        printf("SS %d: %d\n", i, nm.ss[i].port_nm);
        if (nm.ss[i].port_nm == received_ss.port_nm)
        {
            if (nm.ss[i].is_connected)
            {
                printf("Port in use by another storage server\n");
            }
            else
            {
                printf("Storage server %d id back online\n", i + 1);
                ss = &nm.ss[i];
                nm.ss[i].is_connected = 1;
                nm.ss[i].port_nm = received_ss.port_nm;
                nm.ss[i].port_client = received_ss.port_client;
                nm.ss[i].ss_index = i;
                new_ss = 0;
            }
        }
    }

    const char *init_msg = "Storage server registered with naming server";
    if (send(initial_socket, init_msg, strlen(init_msg) + 1, MSG_NOSIGNAL) < 0)
    {
        perror("Failed to send initialization acknowledgment");
        return NULL;
    }

    if (new_ss)
    {
        ss = &nm.ss[nm.num_ss++];
        ss->port_nm = received_ss.port_nm;
        ss->port_client = received_ss.port_client;
        strcpy(ss->ip, received_ss.ip);
        ss->ss_index = nm.num_ss - 1;

        ss->num_paths = received_ss.num_paths;
        ss->is_connected = 1;

        for (int i = 0; i < received_ss.num_paths; i++)
        {
            strncpy(ss->accessible_paths[i], received_ss.accessible_paths[i], MAX_PATHS - 1);
            ss->accessible_paths[i][MAX_PATHS - 1] = '\0';
        }

        printf("Storage server connected from %s:%d\n", ss->ip, ss->port_nm);

        printf("Received paths from storage server: %d\n", ss->num_paths);
        for (int i = 0; i < ss->num_paths; i++)
        {
            printf("Path %d: %s\n", i + 1, ss->accessible_paths[i]);
            insertFile(ALL_PATHS, ss->accessible_paths[i], ss->ss_index);
        }
        printf("Files inserted into trie\n");
        setup_backup_configuration(ss, nm.num_ss);
    }

    // Log storage server registration
    log_ss_registration(nm.ss[ss->ss_index]);

    int new_port = received_ss.port_nm;
    sleep(2);

    int nm_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (nm_socket < 0)
    {
        perror("Socket creation failed");
        return NULL;
    }

    char *ip = get_ip();

    struct sockaddr_in nm_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(new_port),
        // .sin_addr.s_addr = inet_addr(ip)
    };

    if (inet_pton(AF_INET, ss->ip, &nm_addr.sin_addr) <= 0)
    {
        perror("Invalid address/ Address not supported");
        close(nm_socket);
        return NULL;
    }

    printf("Port set to: %d (network byte order: %d)\n", new_port, nm_addr.sin_port);
    printf("Attempting to connect to ss at IP: %s, Port: %d\n", ip, new_port);

    if (connect(nm_socket, (struct sockaddr *)&nm_addr, sizeof(nm_addr)) < 0)
    {
        perror("Connection to storage server failed");
        close(nm_socket);
        return NULL;
    }
    printf("Connected to ss at IP: %d, Port: %d\n", ip, new_port);

    ss->socket_fd = nm_socket;

    while (ss->is_connected)
    {
        if (process_ss_heartbeat(ss) != 0)
        {
            ss->is_connected = 0;
            return NULL;
        }
        sleep(HEARTBEAT_INTERVAL);
    }

    return NULL;
    // close(ss->socket_fd);
}

int handle_ss_connection(int ss_socket, struct sockaddr_in *ss_addr)
{
    pthread_mutex_lock(&nm.lock);

    if (nm.num_ss >= MAX_STORAGE_SERVERS)
    {
        const char *msg = "Server full";
        send(ss_socket, msg, strlen(msg), 0);
        pthread_mutex_unlock(&nm.lock);
        return -1;
    }

    printf("hihi\n");
    pthread_t handler_thread;

    printf("hihi\n");
    printf("SS Socket: %d\n", ss_socket);
    int *ss_socket_ptr = malloc(sizeof(int));
    *ss_socket_ptr = ss_socket;
    if (pthread_create(&handler_thread, NULL, &handle_ss_messages, ss_socket_ptr) != 0)
    {
        perror("Failed to create storage server handler thread");
        pthread_mutex_unlock(&nm.lock);
        return -1;
    }

    pthread_mutex_unlock(&nm.lock);
    return 0;
}

void get_public_ip(void)
{
    char hostbuffer[256];
    struct hostent *host_entry;
    int hostname;
    struct in_addr **addr_list;

    hostname = gethostname(hostbuffer, sizeof(hostbuffer));
    if (hostname == -1)
    {
        perror("gethostname error");
        exit(1);
    }

    host_entry = gethostbyname(hostbuffer);
    if (host_entry == NULL)
    {
        perror("gethostbyname error");
        exit(1);
    }

    addr_list = (struct in_addr **)host_entry->h_addr_list;

    for (int i = 0; addr_list[i] != NULL; i++)
    {
        printf("IP Address %d: %s\n", i + 1, inet_ntoa(*addr_list[i]));
    }
}

char *get_ip()
{
    struct ifaddrs *ifap, *ifa;
    struct sockaddr_in *sa;
    char *addr;
    getifaddrs(&ifap);
    for (ifa = ifap; ifa; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr->sa_family == AF_INET)
        {
            sa = (struct sockaddr_in *)ifa->ifa_addr;
            addr = inet_ntoa(sa->sin_addr);
            if (strcmp(ifa->ifa_name, "enp0s3") == 0)
            {
                break;
            }
        }
    }
    freeifaddrs(ifap);
    return addr;
}