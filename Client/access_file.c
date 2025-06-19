#include "client.h"

int storage_server_file_descriptor;

void play_audio(const char *temp_file)
{
    char command[BUFFER_SIZE];
    // Using mpv player to play the audio
    snprintf(command, sizeof(command), "mpv %s", temp_file);
    system(command);
}

void interface_with_storage_server(response_packet *packet_to_receive, int command, char *string_to_write, int sync_flag)
{
    printf("Main string %s\n", string_to_write);
    printf("interface\n");
    storage_server_file_descriptor = initialize_socket(2);
    memset(&storage_server_address, '\0', sizeof(storage_server_address));

    printf("%s %d\n", packet_to_receive->ip, packet_to_receive->port);
    storage_server_address.sin_family = AF_INET;
    storage_server_address.sin_addr.s_addr = inet_addr(packet_to_receive->ip);
    storage_server_address.sin_port = htons(packet_to_receive->port);
    // connection established
    printf(YELLOW "[Client] Connecting directly to Storage Server.\n" DEFAULT);

    connect_to_server(storage_server_file_descriptor, &storage_server_address);

    printf(GREEN "[Client] Connection with Storage Server established.\n" DEFAULT);

    printf(YELLOW "\n[Client] Sending Packet to Storage Server\n" DEFAULT);
    request_packet *final_packet = (request_packet *)malloc(sizeof(request_packet));

    direct_to_ss_packet *client_server_send = (direct_to_ss_packet *)malloc(sizeof(direct_to_ss_packet));

    strcpy(client_server_send->path, packet_to_receive->path);
    client_server_send->command = command;
    client_server_send->sync_flag = sync_flag;
    // client_server_send->sync_flag = packet_to_receive->sync_flag; // Add this line
    // printf("command %d %d\n", client_server_send->command, sizeof(client_server_send));

    int send_status = send(storage_server_file_descriptor, client_server_send, sizeof(direct_to_ss_packet), 0);
    if (send_status == -1)
    {
        perror(RED "Error sending packet to Storage Server" DEFAULT);
    }
    else
    {
        printf(YELLOW "Packet sent to Storage Server. Waiting for response...\n" DEFAULT);
    }

    direct_to_ss_response_packet *client_server_receive = (direct_to_ss_response_packet *)malloc(sizeof(direct_to_ss_response_packet));
    if (client_server_receive == NULL)
    {
        perror(RED "[CLIENT] Memory allocation failed for response packet" DEFAULT);
        close(storage_server_file_descriptor);
        return;
    }

    int success = 0;
    if (command == CMD_READ)
    {
        printf("Contents of file %s:\n ", packet_to_receive->path);
        while (1)
        {
            client_server_receive->error_code = 0;
            // client_server_packet->command = command;

            int recv_status = recv(storage_server_file_descriptor, client_server_receive, sizeof(direct_to_ss_response_packet), 0);
            if (recv_status == -1)
            {
                perror(RED "Error receiving packet from Storage Server" DEFAULT);
            }
            else if (recv_status == 0)
            {
                printf(RED "Connection closed by Storage Server\n" DEFAULT);
            }
            // else
            // {
            //     printf(YELLOW "Packet successfully received from Storage Server\n" DEFAULT);
            // }
            // printf("error code %d\n",client_server_receive->error_code);
            if (client_server_receive->error_code == 0) // Success
            {
                // printf("HERE BECAUSE NO STOP\n");
                if (client_server_receive->size > 0)
                {

                    printf("%s ", client_server_receive->data);
                    fflush(stdout);
                }
            }
            else if (client_server_receive->error_code == STOP)
            {

                // printf("\n");
                success = 1;
                printf(YELLOW "\n[Client] Received complete file data.\n" DEFAULT);
                fflush(stdout);
                break;
            }
            else if (client_server_receive->error_code == UNAUTHORIZED)
            {
                printf(RED "[Client] Error %d : Unauthorized Access\n" DEFAULT, client_server_receive->error_code);
                fflush(stdout);
                close(storage_server_file_descriptor);
                free(client_server_receive);
                break;
            }
            else if (client_server_receive->error_code == PATH_NOT_FOUND)
            {
                printf(RED "[Client] Error %d : Invalid Path!\n" DEFAULT, client_server_receive->error_code);
                fflush(stdout);
                close(storage_server_file_descriptor);
                free(client_server_receive);
                break;
            }
            else
            {
                printf(RED "[Client] Error %d in Read Operation\n" DEFAULT, client_server_receive->error_code);
                close(storage_server_file_descriptor);
                free(client_server_receive);
                return;
            }

            if (client_server_receive->size == 0 || client_server_receive->error_code == STOP)
            {
                // printf("\n");
                success = 1;
                printf(YELLOW "\n[Client] Received complete file data.\n" DEFAULT);
                fflush(stdout);
                break;
            }
        }
        if (success)
        {

            printf(GREEN "\n[Client] Read Operation Successful\n" DEFAULT);
        }
        else
        {
            printf(RED "[Client] Read Operation Failed\n" DEFAULT);
        }
        // close(storage_server_file_descriptor);
        free(client_server_receive);
        // printf(YELLOW "[CLIENT] Connection to Storage Server closed\n" DEFAULT);
        final_packet->command = ACK;
    }
    else if (command == CMD_GET_INFO)
    {
        while (1)
        {
            char file_info[BUFFER_SIZE] = {0};
            int recv_status = recv(storage_server_file_descriptor, client_server_receive, sizeof(direct_to_ss_response_packet), 0);
            if (recv_status < 0)
            {
                perror(RED "[CLient] Error receiving file info from Storage Server\n" DEFAULT);
                close(storage_server_file_descriptor);
                return;
            }
            else if (recv_status == 0)
            {
                printf(RED "[Client] Connection closed by Storage Server during GET_INFO\n" DEFAULT);
                close(storage_server_file_descriptor);
                return;
            }
            strcpy(file_info, client_server_receive->data);

            if (client_server_receive->error_code == 0)
            {

                printf(YELLOW "[Client] Received file info from Storage Server\n" DEFAULT);

                char *token = strtok(file_info, " ");
                if (token == NULL)
                {
                    printf(RED "[Client] Error: Received malformed file info\n" DEFAULT);
                    close(storage_server_file_descriptor);
                    return;
                }

                printf(CYAN "File       : \033[1;92m%s\n" DEFAULT, client_server_send->path);
                printf(CYAN "Permissions: \033[1;92m%s\n" DEFAULT, token);

                token = strtok(NULL, " ");
                if (token == NULL)
                {
                    printf(RED "[CLIENT] Error: Missing size information in file info\n" DEFAULT);
                    close(storage_server_file_descriptor);
                    return;
                }

                printf(CYAN "Size       : \033[1;92m%s bytes\n" DEFAULT, token);
                fflush(stdout);
            }
            else if (client_server_receive->error_code == STOP)
            {

                // printf("\n");
                success = 1;
                printf(YELLOW "\n[Client] Received complete file data.\n" DEFAULT);
                fflush(stdout);
                break;
            }
            else if (client_server_receive->error_code == UNAUTHORIZED)
            {
                printf(RED "[Client] Error %d : Unauthorized Access\n" DEFAULT, client_server_receive->error_code);
                fflush(stdout);
                close(storage_server_file_descriptor);
                free(client_server_receive);
                break;
            }
            else if (client_server_receive->error_code == PATH_NOT_FOUND)
            {
                printf(RED "[Client] Error %d : Invalid Path!\n" DEFAULT, client_server_receive->error_code);
                fflush(stdout);
                close(storage_server_file_descriptor);
                free(client_server_receive);
                break;
            }
            else
            {
                printf(RED "[Client] Error %d in GET_INFO Operation\n" DEFAULT, client_server_receive->error_code);
                close(storage_server_file_descriptor);
                free(client_server_receive);
                return;
            }
        }
        if (success)
        {

            printf(GREEN "\n[Client] GET_INFO Operation Successful\n" DEFAULT);
        }
        else
        {
            printf(RED "[Client] GET_INFO Operation Failed\n" DEFAULT);
        }
        free(client_server_receive);
        final_packet->command = ACK;
        // close(storage_server_file_descriptor);
        // printf(YELLOW "[CLIENT] Connection to Storage Server closed\n" DEFAULT);
    }
    else if (command == CMD_STREAM)
    {
        printf("The path of the AUDIO file is : %s\n", client_server_send->path);
        // Send file path to server
        // send(storage_server_file_descriptor, client_server_send, sizeof(direct_to_ss_packet), 0);

        // Receive file size
        long file_size;
        recv(storage_server_file_descriptor, &file_size, sizeof(long), 0);
        printf("[Client] File size: %ld bytes\n", file_size);

        // Create temporary file for streaming
        char temp_file[] = "/tmp/nfs_audio_XXXXXX";
        int temp_fd = mkstemp(temp_file);
        if (temp_fd == -1)
        {
            perror("Failed to create temp file");
            close(storage_server_file_descriptor);
            return;
        }

        FILE *temp_fp = fdopen(temp_fd, "wb");
        if (temp_fp == NULL)
        {
            perror("Failed to open temp file");
            close(temp_fd);
            close(storage_server_file_descriptor);
            return;
        }

        // Receive and write data
        char buffer[BUFFER_SIZE];
        long total_received = 0;
        ssize_t bytes_received;

        while (total_received < file_size &&
               (bytes_received = recv(storage_server_file_descriptor, buffer, BUFFER_SIZE, 0)) > 0)
        {
            fwrite(buffer, 1, bytes_received, temp_fp);
            total_received += bytes_received;
            printf("[Client] Progress: %.2f%%\r", (float)total_received / file_size * 100);
            fflush(stdout);
        }

        printf("\n[Client] Download complete. Playing audio...\n");

        fclose(temp_fp);
        close(storage_server_file_descriptor);

        // Play the audio file
        play_audio(temp_file);

        // Clean up temp file
        unlink(temp_file);

        return;
    }
    else if (command == CMD_WRITE)
    {
        direct_to_ss_packet *packet_to_send_write = (direct_to_ss_packet *)malloc(sizeof(direct_to_ss_packet));
        if (packet_to_send_write == NULL)
        {
            perror(RED "[Client] Memory allocation failed for write packet" DEFAULT);
            close(storage_server_file_descriptor);
            return;
        }

        strcpy(packet_to_send_write->path, packet_to_receive->path);
        packet_to_send_write->command = CMD_WRITE;
        packet_to_send_write->size = strlen(string_to_write);
        // packet_to_send_write->sync_flag = packet_to_receive->sync_flag; // Add this line
        strcpy(packet_to_send_write->data, string_to_write);
        // printf("%s\n",string_to_write);
        // printf("%s\n",packet_to_send_write->data);

        printf(YELLOW "[Client] Sending Write Packet to Storage Server\n" DEFAULT);

        if (send(storage_server_file_descriptor, packet_to_send_write, sizeof(*packet_to_send_write), 0) < 0)
        {
            perror(RED "[Client] Write Operation Error" DEFAULT);
            close(storage_server_file_descriptor);
            free(packet_to_send_write);
            return;
        }

        if (recv(storage_server_file_descriptor, client_server_receive, sizeof(*client_server_receive), 0) < 0)
        {
            perror(RED "[CLIENT] Error receiving response from Storage Server" DEFAULT);
            close(storage_server_file_descriptor);
            free(packet_to_send_write);
            free(client_server_receive);
            return;
        }
        printf("ERROR CODE : %d\n", client_server_receive->error_code);
        if (client_server_receive->error_code == 0 || client_server_receive->error_code == STOP)
        {
            printf(GREEN "[Client] Write Operation Successful\n" DEFAULT);
        }
        else if (client_server_receive->error_code == UNAUTHORIZED)
        {
            printf(RED "[Client] Error %d : Unauthorized Access\n" DEFAULT, client_server_receive->error_code);
        }
        else if (client_server_receive->error_code == PATH_NOT_FOUND)
        {
            printf(RED "[Client] Error %d : Invalid Path!\n" DEFAULT, client_server_receive->error_code);
        }
        else
        {
            printf(RED "[Client] Error %d in Write Operation\n" DEFAULT, client_server_receive->error_code);
        }

        final_packet->command = ACK_WRITE;
        strcpy(final_packet->path, packet_to_receive->path);

        free(packet_to_send_write);
        free(client_server_receive);
    }

    printf(YELLOW "[Client] Sending Ack to NS.\n" DEFAULT);

    if (send(naming_server_file_descriptor, final_packet, sizeof(final_packet), 0) < 0)
    {
        perror(RED "[Client] Error Sending Ack to NS" DEFAULT);
    }

    printf(YELLOW "[Client] Closing Connection with Storage Server.\n" DEFAULT);

    close(storage_server_file_descriptor);

    return;
}

void display_file_access_menu(int *choice)
{
    while (1)
    {
        printf("\n== FILE ACCESS MENU ==\n");
        printf(CYAN "1. Read from file\n" DEFAULT);
        printf(CYAN "2. Write to file\n" DEFAULT);
        printf(CYAN "3. Get file information\n" DEFAULT);
        printf(CYAN "4. Stream a file\n" DEFAULT);
        printf("Please select an operation: ");
        scanf("%d", choice);
        printf("\n");
        if (*choice < 1 || *choice > 4)
        {
            printf(RED "Invalid Input. Please enter 1, 2, 3 or 4.\n" DEFAULT);
        }
        else
        {
            break;
        }
    }
}

void get_file_name(char *file_path)
{
    printf(YELLOW "Enter File Name (with path): " DEFAULT);

    scanf("%s", file_path);
    file_path[strlen(file_path)] = '\0';
}

int send_packet_to_naming_server(request_packet *packet_to_send, response_packet *packet_to_receive)
{
    printf(YELLOW "Sending packets to Naming Server...\n" DEFAULT);
    // printf("CONTENT IN PACKET - %s\n",packet_to_send->contents);
    int send_status = send(naming_server_file_descriptor, packet_to_send, sizeof(request_packet), 0);
    if (send_status == -1)
    {
        perror(RED "Error sending packet to Naming Server" DEFAULT);
        return 0;
    }
    else
    {
        printf(YELLOW "Packet sent to Naming Server. Waiting for response...\n" DEFAULT);
    }

    int recv_status = recv(naming_server_file_descriptor, packet_to_receive, sizeof(*packet_to_receive), 0);
    if (recv_status == -1)
    {
        perror(RED "Error receiving packet from Naming Server" DEFAULT);
        return 0;
    }
    else if (recv_status == 0)
    {
        printf(RED "Connection closed by Naming Server\n" DEFAULT);
    }
    else
    {
        printf(YELLOW "Packet successfully received from Naming Server\n" DEFAULT);
        printf("error code %d\n", packet_to_receive->error_code);
        printf("ack %d\n", packet_to_receive->ack);
        printf("path %s\n", packet_to_receive->path);
        printf("ip %s\n", packet_to_receive->ip);
        printf("port %d\n", packet_to_receive->port);
    }

    return 1;
}

void send_packet(request_packet *packet_to_send, response_packet *packet_to_receive)
{

    if (packet_to_send->command == CMD_WRITE)
    {
        char write_to_file_input[BUFFER_SIZE];
        printf(YELLOW "Enter data to write to file: " DEFAULT);
        getchar();
        fgets(write_to_file_input, BUFFER_SIZE, stdin);
        write_to_file_input[strlen(write_to_file_input)] = '\0';
        // printf("INPUT text %s\n",write_to_file_input);
        strcpy(packet_to_send->contents, write_to_file_input);
    }
    // printf("INPUT text %s\n",packet_to_send->contents);

    while (send_packet_to_naming_server(packet_to_send, packet_to_receive) != 1)
    {
        printf(RED "Failed to send packet. Retrying...\n" DEFAULT);
    }
}

void get_sync_flag(int *sync_flag)
{
    printf("Do you want to write synchronously? (1 for Yes, 0 for No): ");
    scanf("%d", sync_flag);
}

void access_file()
{

    request_packet *packet_to_send = (request_packet *)malloc(sizeof(request_packet));
    response_packet *packet_to_receive = (response_packet *)malloc(sizeof(response_packet));
    char file_path[MAX_PATHS];
    int choice;
    char *write_to_file_input = (char *)calloc(BUFFER_SIZE, sizeof(char));

    display_file_access_menu(&choice);

    if (choice == 1)
        packet_to_send->command = CMD_READ;
    else if (choice == 2)
    {
        get_sync_flag(&packet_to_send->sync_flag);
        packet_to_send->command = CMD_WRITE;
    }
    else if (choice == 3)
        packet_to_send->command = CMD_GET_INFO;
    else
        packet_to_send->command = CMD_STREAM;

    packet_to_send->file_or_folder_flag = FILE_TYPE;

    get_file_name(file_path);
    strcpy(packet_to_send->path, file_path);

    send_packet(packet_to_send, packet_to_receive);
    // printf("ACKNOWLEDGE %d\n",packet_to_receive->ack);

    if (packet_to_receive->error_code == TIME_OUT)
    {
        printf(RED "Error %d : Operation Timed Out!\n" DEFAULT, packet_to_receive->error_code);
        return;
    }
    else if (packet_to_receive->error_code == PATH_NOT_FOUND)
    {
        printf(RED "Error %d : Invalid Path!\n" DEFAULT, packet_to_receive->error_code);
        return;
    }
    else if (packet_to_receive->ack != 1)
    {
        // printf("I WILL GIVE ERROR\n");
        printf(RED "Ack Error %d\n" DEFAULT, packet_to_receive->error_code);
        return;
    }

    strcpy(packet_to_receive->path, file_path);
    printf(YELLOW "Packet Successfully Received from Naming Server\n" DEFAULT);
    packet_to_receive->command = packet_to_send->command;

    strcpy(write_to_file_input, packet_to_send->contents);
    // printf("Command: %d and string to write %s\n", packet_to_receive->command, write_to_file_input);

    interface_with_storage_server(packet_to_receive, packet_to_send->command, write_to_file_input, packet_to_send->sync_flag);

    return;
}