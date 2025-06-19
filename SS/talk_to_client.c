#include "../header.h"

void send_file_size(int socket, long size)
{
    send(socket, &size, sizeof(long), 0);
}

void *handle_stream_request(void *args)
{
    StreamArgs *stream_args = (StreamArgs *)args;
    int client_socket = stream_args->client_socket;
    char *filepath = stream_args->filepath;

    printf("[Server] Starting streaming for file: %s\n", filepath);

    // Open the audio file
    FILE *audio_file = fopen(filepath, "rb");
    if (audio_file == NULL)
    {
        printf("[Server] Error: Cannot open file %s\n", filepath);
        char *error_msg = "ERROR: File not found";
        send(client_socket, error_msg, strlen(error_msg), 0);
        close(client_socket);
        free(filepath);
        free(stream_args);
        return NULL;
    }

    // Get file size
    fseek(audio_file, 0, SEEK_END);
    long file_size = ftell(audio_file);
    fseek(audio_file, 0, SEEK_SET);

    printf("[Server] File size: %ld bytes\n", file_size);

    // Send file size to client
    send_file_size(client_socket, file_size);

    // Initialize the read lock
    sem_init(&stream_args->rw_mutex, 0, 1);

    // Acquire read lock
    sem_wait(&stream_args->rw_mutex);

    // Stream the file
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    long total_sent = 0;

    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, audio_file)) > 0)
    {
        ssize_t bytes_sent = send(client_socket, buffer, bytes_read, 0);
        if (bytes_sent < 0)
        {
            printf("[Server] Error sending data: %s\n", strerror(errno));
            break;
        }
        total_sent += bytes_sent;
        printf("[Server] Progress: %.2f%%\r", (float)total_sent / file_size * 100);
        fflush(stdout);
        usleep(1000); // Small delay to prevent overwhelming the network
    }

    printf("\n[Server] Streaming complete. Total sent: %ld bytes\n", total_sent);

    // Release read lock
    sem_post(&stream_args->rw_mutex);

    // Destroy the read lock
    sem_destroy(&stream_args->rw_mutex);

    fclose(audio_file);
    close(client_socket);
    free(filepath);
    free(stream_args);
    return NULL;
}

sem_t lock;

typedef struct
{
    StorageServer *ss;
    direct_to_ss_packet *req;
    int client_socket;
    sem_t w_mutex;
} AsyncWriteArgs;

void *async_write_handler(void *arg)
{
    AsyncWriteArgs *args = (AsyncWriteArgs *)arg;
    int fd = open(args->req->path, O_WRONLY | O_TRUNC);
    if (fd == -1)
    {
        printf(RED "ERROR %d: File cannot be opened\n" DEFAULT, UNAUTHORIZED);
        free(args);
        return NULL;
    }

    // Acquire write lock
    sem_wait(&args->w_mutex);

    if (write(fd, args->req->data, strlen(args->req->data)) == -1)
    {
        printf(RED "ERROR %d: File cannot be written\n" DEFAULT, UNAUTHORIZED);
    }
    else
    {
        printf(CYAN "SUCCESS %d: Asynchronous write executed successfully\n" DEFAULT, SUCCESS);
    }

    // Release write lock
    sem_post(&args->w_mutex);

    close(fd);
    free(args->req);
    free(args);
    return NULL;
}

void *client_request(void *arg)
{
    int *client_des = (int *)arg;
    printf("%d\n", *client_des);
    direct_to_ss_packet *server_client = (direct_to_ss_packet *)malloc(sizeof(direct_to_ss_packet));
    direct_to_ss_response_packet *server_client_receive = (direct_to_ss_response_packet *)malloc(sizeof(direct_to_ss_response_packet));
    while (1)
    {
        if (recv(*client_des, server_client, sizeof(direct_to_ss_packet), 0) <= 0)
        {
            printf(CYAN "[SS]: Client Closed The Connection\n" DEFAULT);
            close(*client_des);
            free(server_client);
            return NULL;
        }
        printf(GREEN "[SS]: Request Received from Client\n");
        printf("[SS]: Starting Executing Request of Client\n" DEFAULT);
        server_client_receive->error_code = 0;
        printf("Command for storage: %d  stream = %d\n", server_client->command, CMD_STREAM);
        
        if(server_client->command == CMD_STREAM)
        {
            printf("I AM HERE LOL\n");
        }

        if (server_client->command == CMD_READ)
        {
            printf(GREEN "Executing Command %d\n" DEFAULT, server_client->command);
            int fd = open(server_client->path, O_RDONLY);
            if (fd == -1)
            {
                printf(RED "ERROR %d: File cannot be opened\n" DEFAULT, UNAUTHORIZED);
                server_client_receive->error_code = UNAUTHORIZED;
            }
            else
            {
                // Initialize the read lock
                sem_init(&server_client->rw_mutex, 0, 1);

                // Acquire read lock
                sem_wait(&server_client->rw_mutex);

                memset(server_client_receive->data, '\0', sizeof(server_client_receive->data));
                long long int total_data = lseek(fd, 0, SEEK_END);
                lseek(fd, 0, SEEK_SET);

                while (total_data > 0)
                {
                    server_client_receive->error_code = 0;
                    int bytes_read = read(fd, server_client_receive->data, sizeof(server_client_receive->data) - 1);
                    if (bytes_read <= 0)
                    {
                        printf(RED "ERROR: Couldn't read from file\n" DEFAULT);
                        break;
                    }
                    server_client_receive->data[bytes_read] = '\0';
                    server_client_receive->size = bytes_read;
                    // printf("ghjwfgqegvugwrvghuutvguerhtv\n");
                    // printf("%d\n %s\n",server_client_receive->error_code, server_client_receive->data);

                    if (send(*client_des, server_client_receive, sizeof(direct_to_ss_response_packet), 0) == -1)
                    {
                        printf(RED "ERROR %d: Couldn't send packet\n" DEFAULT, NOT_SENT);
                        close(fd);
                        close(*client_des);
                        free(client_des);
                        return NULL;
                    }
                    usleep(1000);
                    total_data -= bytes_read;
                    memset(server_client_receive->data, '\0', sizeof(server_client_receive->data));
                }

                // Release read lock
                sem_post(&server_client->rw_mutex);

                // Destroy the read lock
                sem_destroy(&server_client->rw_mutex);

                server_client_receive->error_code = STOP;
                printf(GREEN "SUCCESS %d: Request Executed Successfully\n" DEFAULT, SUCCESS);
                // command->ack = 1;
                close(fd);
            }
            printf("last error code %d\n", server_client_receive->error_code);
            if (send(*client_des, server_client_receive, sizeof(direct_to_ss_response_packet), 0) == -1)
            {
                printf(RED "ERROR %d: Couldn't send packet\n" DEFAULT, NOT_SENT);
                close(*client_des);
                free(client_des);
                return NULL;
            }
        }
        else if (server_client->command == CMD_GET_INFO)
        {
            printf(GREEN "Executing Command %d\n" DEFAULT, server_client->command);
            struct stat stats;
            char permissions[BUFFER_SIZE] = {0};
            char size_buffer[256] = {0};

            if (lstat(server_client->path, &stats) == 0)
            {
                char *type = S_ISDIR(stats.st_mode) ? "d" : "-";
                snprintf(permissions, sizeof(permissions), "%s%c%c%c%c%c%c%c%c%c ",
                         type,
                         (stats.st_mode & S_IRUSR) ? 'r' : '-', (stats.st_mode & S_IWUSR) ? 'w' : '-', (stats.st_mode & S_IXUSR) ? 'x' : '-',
                         (stats.st_mode & S_IRGRP) ? 'r' : '-', (stats.st_mode & S_IWGRP) ? 'w' : '-', (stats.st_mode & S_IXGRP) ? 'x' : '-',
                         (stats.st_mode & S_IROTH) ? 'r' : '-', (stats.st_mode & S_IWOTH) ? 'w' : '-', (stats.st_mode & S_IXOTH) ? 'x' : '-');

                snprintf(size_buffer, sizeof(size_buffer), "%ld", stats.st_size);
                strcat(permissions, size_buffer);
                strcpy(server_client_receive->data, permissions);
                // server_client_receive->size = siz
                if (send(*client_des, server_client_receive, sizeof(direct_to_ss_response_packet), 0) == -1)
                {
                    perror(RED "ERROR: Couldn't Send Permissions Packet" DEFAULT);
                    close(*client_des);
                    free(client_des);
                    return NULL;
                }

                printf(CYAN "SUCCESS: File Information Sent Successfully\n" DEFAULT);
                server_client_receive->error_code = STOP;
            }
            else
            {
                perror(RED "ERROR: File Cannot Be Opened or Does Not Exist" DEFAULT);
                server_client_receive->error_code = UNAUTHORIZED;
            }

            if (send(*client_des, server_client_receive, sizeof(direct_to_ss_response_packet), 0) == -1)
            {
                perror(RED "ERROR: Couldn't Send Command Packet" DEFAULT);
                close(*client_des);
                free(client_des);
                return NULL;
            }
        }
        else if (server_client->command == CMD_WRITE)
        {
            printf(GREEN "Executing Command %d\n" DEFAULT, server_client->command);

            printf("The sync Flag is : %d", server_client->sync_flag);

            int fd = open(server_client->path, O_WRONLY | O_TRUNC);
            if (fd == -1)
            {
                printf(RED "ERROR %d: File cannot be opened\n" DEFAULT, UNAUTHORIZED);
                server_client_receive->error_code = UNAUTHORIZED;
            }
            else
            {
                // Initialize the write lock
                sem_init(&server_client->w_mutex, 0, 1);

                // Acquire write lock
                sem_wait(&server_client->w_mutex);

                direct_to_ss_packet *packet_to_write = (direct_to_ss_packet *)malloc(sizeof(direct_to_ss_packet));
                if (recv(*client_des, packet_to_write, sizeof(direct_to_ss_packet), 0) <= 0)
                {
                    printf(CYAN "[SS]: Failed to receive the data for writing\n" DEFAULT);
                    free(packet_to_write);
                    close(*client_des);
                    return NULL;
                }

                printf("Received data: %s\n", packet_to_write->data);
                printf("Received path: %s\n", packet_to_write->path);
                printf("Sync Flag: %d\n", server_client->sync_flag);

                if (server_client->sync_flag == 1)
                {
                    // Synchronous write
                    if (write(fd, packet_to_write->data, strlen(packet_to_write->data)) == (-1))
                    {
                        printf(RED "ERROR %d: File cannot be written\n" DEFAULT, UNAUTHORIZED);
                        server_client_receive->error_code = UNAUTHORIZED;
                    }
                    else
                    {
                        printf(CYAN "SUCCESS %d: Request Executed Successfully\n" DEFAULT, SUCCESS);
                        server_client_receive->error_code = STOP;
                    }
                }
                else
                {
                    // Asynchronous write
                    server_client_receive->error_code = SUCCESS;
                    if (send(*client_des, server_client_receive, sizeof(direct_to_ss_response_packet), 0) == -1)
                    {
                        printf(RED "ERROR %d: Couldn't send acknowledgment packet\n" DEFAULT, NOT_SENT);
                        close(fd);
                        close(*client_des);
                        free(client_des);
                        return NULL;
                    }

                    pthread_t async_write_thread;
                    AsyncWriteArgs *args = malloc(sizeof(AsyncWriteArgs));

                    args->req = packet_to_write;
                    args->client_socket = *client_des;

                    if (pthread_create(&async_write_thread, NULL, async_write_handler, args) != 0)
                    {
                        perror("Failed to create async write thread");
                        free(args);
                        close(fd);
                        close(*client_des);
                        free(client_des);
                        return NULL;
                    }
                    pthread_detach(async_write_thread);
                }
            }
            if (server_client->sync_flag == 1 && send(*client_des, server_client_receive, sizeof(direct_to_ss_response_packet), 0) == -1)
            {
                printf(RED "ERROR %d: Couldn't send packet\n" DEFAULT, NOT_SENT);
                close(fd);
                close(*client_des);
                free(client_des);
                return NULL;
            }

            // Release write lock
            sem_post(&server_client->w_mutex);

            // Destroy the write lock
            sem_destroy(&server_client->w_mutex);

            close(fd);
        }
        else if (server_client->command == 4)
        {
            printf("HERE for audio file %s\n", server_client->path);
            char filepath[256];
            strcpy(filepath, server_client->path);
            filepath[strlen(filepath)] = '\0';
            printf("FILEPATH %s\n",filepath);


            // Create args for streaming thread
            StreamArgs *args = malloc(sizeof(StreamArgs));
            args->client_socket = *client_des ;
            args->filepath = strdup(filepath);

            // Create thread to handle streaming
            pthread_t thread;
            if (pthread_create(&thread, NULL, handle_stream_request, args) != 0)
            {
                printf("[Server] Failed to create thread\n");
                close(*client_des);
                free(args->filepath);
                free(args);
                continue;
            }
            pthread_detach(thread);
        }
    }
    return NULL;
}

void *connect_to_client(void *arg)
{
    printf("CONNECT TO CLIENT\n");
    pthread_t *request = NULL;
    struct sockaddr_in serverAddress;
    int sfd_client, acc;
    StorageServer *ss = (StorageServer *)arg;
    sfd_client = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd_client == -1)
    {
        printf(RED "ERROR %d: Socket not created\n" DEFAULT, INIT_ERROR);
        exit(0);
    }

    memset(&serverAddress, '\0', sizeof(serverAddress));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(ss->port_client);

    if (bind(sfd_client, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
    {
        fprintf(stderr, RED "ERROR %d: Couldn't bind socket\n" DEFAULT, INIT_ERROR);
        close(sfd_client);
        exit(EXIT_FAILURE);
    }
    printf("BINDED\n");

    if (listen(sfd_client, SOMAXCONN) == -1)
    {
        fprintf(stderr, RED "ERROR %d: Listening failed\n" DEFAULT, INIT_ERROR);
        close(sfd_client);
        exit(EXIT_FAILURE);
    }
    printf("LISTENING\n");
    sem_init(&lock, 0, 1);
    int count = 0;
    while (1)
    {
        sem_wait(&lock);
        printf("ACCEPTING\n");
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);

        acc = accept(sfd_client, (struct sockaddr *)&client_addr, &addr_len);
        if (acc < 0)
        {
            printf(RED "ERROR %d: Couldn't establish connection\n" DEFAULT, BAD_REQUEST);
            sem_post(&lock);
            continue;
        }

        printf(CYAN "SUCCESS %d: Connection Established Successfully\n" DEFAULT, SUCCESS);
        count++;
        int *index = (int *)malloc(sizeof(int));
        if (index == NULL)
        {
            fprintf(stderr, RED "ERROR: Memory allocation failed for client index\n" DEFAULT);
            close(acc);
            sem_post(&lock);
            continue;
        }
        *index = acc;
        request = (pthread_t *)realloc(request, count * (sizeof(pthread_t)));
        if (request == NULL)
        {
            fprintf(stderr, RED "ERROR: Memory reallocation failed for request threads\n" DEFAULT);
            close(acc);
            free(index);
            sem_post(&lock);
            continue;
        }
        // con = (int *)realloc(con, count * (sizeof(int)));
        printf("HERE\n");
        if (pthread_create(&request[count - 1], NULL, client_request, index) != 0)
        {
            fprintf(stderr, RED "ERROR: Failed to create client request thread\n" DEFAULT);
            close(acc);
            free(index);
            sem_post(&lock);
            continue;
        }
        sem_post(&lock);
    }
    for (int i = 0; i < count; i++)
    {
        pthread_join(request[i], NULL);
    }

    free(request);
    close(sfd_client);
    sem_destroy(&lock);

    return NULL;
}