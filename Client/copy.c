#include "client.h"

void move_function()
{
    request_packet *packet_to_send = (request_packet *)malloc(sizeof(request_packet));
    response_packet *packet_to_receive = (response_packet *)malloc(sizeof(response_packet));
    if (!packet_to_send || !packet_to_receive)
    {
        perror(RED "[CLIENT] Memory Allocation Failed\n" DEFAULT);
        return;
    }
    char file_name[MAX_PATH_LENGTH], destination_path[MAX_PATH_LENGTH];

    while (1)
    {
        printf(YELLOW "\n[CLIENT] Please select the type of item to move:\n" DEFAULT);
        printf(CYAN "  1 File\n" DEFAULT);
        printf(CYAN "  2 Folder\n" DEFAULT);
        printf(YELLOW "[Client] Enter your choice: " DEFAULT);
        scanf("%u", &packet_to_send->file_or_folder_flag);

        if (packet_to_send->file_or_folder_flag != 1 && packet_to_send->file_or_folder_flag != 2)
        {
            printf(RED "\n[Client] Invalid type!\n" DEFAULT);
            continue;
        }

        if (packet_to_send->file_or_folder_flag == 1)
        {
            packet_to_send->command = CMD_COPY_FILE;
            printf(YELLOW "[Client] Enter File Name (with path): " DEFAULT);
            scanf("%s", file_name);
            break;
        }
        else if (packet_to_send->file_or_folder_flag == 2)
        {
            packet_to_send->command = CMD_COPY_FOLDER;
            printf(YELLOW "[Client] Enter File Name (with path): " DEFAULT);
            scanf("%s", file_name);
            break;
        }
    }
    file_name[strlen(file_name)] = '\0';
    strcpy(packet_to_send->path, file_name);

    printf(YELLOW "[CLIENT] Enter the destination path: " DEFAULT);
    scanf("%s", destination_path);
    destination_path[strlen(destination_path)] = '\0';
    strcpy(packet_to_send->new_path, destination_path);

    printf(YELLOW "[CLIENT] Sending MOVE request to Naming Server...\n" DEFAULT);
    while (send_packet_to_naming_server(packet_to_send, packet_to_receive) != 1)
    {
        printf(RED "[CLIENT] Retrying MOVE request to Naming Server...\n" DEFAULT);
    }

    if (packet_to_receive->ack == 1)
    {
        printf(GREEN "[CLIENT] Successfully moved  %s to %s\n" DEFAULT,
               packet_to_send->path,
               packet_to_send->new_path);
    }
    else
    {
        printf(RED "[CLIENT] Error: Unable to move %s\n" DEFAULT, packet_to_send->path);
        printf(RED "[CLIENT] Error Code: %d\n" DEFAULT, packet_to_receive->command);
    }
    free(packet_to_send);
    free(packet_to_receive);
    return;
}