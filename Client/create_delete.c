#include "client.h"

void create_delete_function()
{
    request_packet *packet_to_send = (request_packet *)malloc(sizeof(request_packet));
    response_packet *packet_to_receive = (response_packet *)malloc(sizeof(response_packet));
    int create_or_delete_choice, file_or_folder_choice;
    char file_name[MAX_PATH_LENGTH];

    while (1)
    {
        printf(CYAN "\n[CLIENT] Please choose an operation:\n");
        printf("  1. Create a File/Folder\n");
        printf("  2. Delete a File/Folder\n");
        printf(DEFAULT "[Client] Enter your choice (1 or 2): ");
        scanf("%d", &create_or_delete_choice);

        if (create_or_delete_choice != 1 && create_or_delete_choice != 2)
        {
            printf(RED "\n[Client] Invalid function type!\n" DEFAULT);
            continue;
        }
        else
            break;
    }
    while (1)
    {
        if (create_or_delete_choice == 1)
        {
            printf(CYAN "\n[CLIENT] Please choose the type of entity to create:\n");
            printf("  1. File\n");
            printf("  2. Folder\n");
            printf(DEFAULT "[Client] Enter your choice (1 or 2): ");
            scanf("%d", &file_or_folder_choice);
        }
        else if (create_or_delete_choice == 2)
        {
            printf(CYAN "\n[CLIENT] Please choose the type of entity to delete:\n");
            printf("  1. File\n");
            printf("  2. Folder\n");
            printf(DEFAULT "[Client] Enter your choice (1 or 2): ");
            scanf("%d", &file_or_folder_choice);
        }

        if (file_or_folder_choice != 1 && file_or_folder_choice != 2)
        {
            printf(RED "\n[Client] Invalid function type!\n" DEFAULT);
            continue;
        }

        if (create_or_delete_choice == 1)
        {
            if (file_or_folder_choice == 1)
            {
                packet_to_send->command = CMD_CREATE_FILE;
                packet_to_send->file_or_folder_flag = FILE_TYPE;
            }
            else if (file_or_folder_choice == 2)
            {
                packet_to_send->command = CMD_CREATE_FOLDER;
                packet_to_send->file_or_folder_flag = FOLDER_TYPE;
            }
            break;
        }
        else if (create_or_delete_choice == 2)
        {
            if (file_or_folder_choice == 1)
            {
                packet_to_send->command = CMD_DELETE_FILE;
            }
            else if (file_or_folder_choice == 2)
            {
                packet_to_send->command = CMD_DELETE_FOLDER;
            }
            break;
        }
    }

    if (packet_to_send->file_or_folder_flag == FILE_TYPE)
    {
        printf(YELLOW "\n[Client] Enter File Name: " DEFAULT);
        scanf(" %s", file_name);
    }
    else if (packet_to_send->file_or_folder_flag == FOLDER_TYPE)
    {
        printf(YELLOW "\n[Client] Enter Folder Name: " DEFAULT);
        scanf(" %s", file_name);
    }

    file_name[strlen(file_name)] = '\0';
    strcpy(packet_to_send->contents,file_name);
    if((packet_to_send->command == CMD_CREATE_FILE || packet_to_send->command == CMD_CREATE_FOLDER))
        printf(YELLOW "[Client] Enter Path : " DEFAULT);
    else if((packet_to_send->command == CMD_DELETE_FILE || packet_to_send->command == CMD_DELETE_FOLDER))
        printf(YELLOW "[Client] Enter full path (path/filename or foldername): " DEFAULT);
    // for deleted the path is similar to the accessible path stored while registering the storage server

    scanf("%s", file_name);
    // if (strcmp)
    // {
    //     strcpy(file_name, "/");
    // }
    file_name[strlen(file_name)] = '\0';
    printf("File name %s\n",file_name);
    strcpy(packet_to_send->path,file_name);
    printf("%s\n",packet_to_send->path);

    while (send_packet_to_naming_server(packet_to_send, packet_to_receive) != 1)
    { }

    if (packet_to_receive->ack == 1 || packet_to_receive->command == STOP || packet_to_receive->error_code == SUCCESS)
    {
        printf(YELLOW "[Client] Acknowledgment Received from Naming Server\n" DEFAULT);
        if(packet_to_send->command == CMD_CREATE_FILE || packet_to_send->command == CMD_CREATE_FOLDER)
            printf(GREEN "[CLIENT] CREATE Operation Successful!\n" DEFAULT);
        else if(packet_to_send->command == CMD_DELETE_FILE || packet_to_send->command == CMD_DELETE_FOLDER)
            printf(GREEN "[CLIENT] DELETE Operation Successful!\n" DEFAULT);
    }
    else if (packet_to_receive->error_code == UNAUTHORIZED)
    {
        if(packet_to_send->command == CMD_CREATE_FILE || packet_to_send->command == CMD_CREATE_FOLDER)
            printf(RED "[Client] `CREATE - %s` Operation Failed! Permission Denied!\n" DEFAULT, packet_to_send->path);
        else if(packet_to_send->command == CMD_DELETE_FILE || packet_to_send->command == CMD_DELETE_FOLDER)
            printf(RED "[Client] `DELETE - %s` Operation Failed! Permission Denied!\n" DEFAULT, packet_to_send->path);
    }
    else if (packet_to_receive->error_code == PATH_NOT_FOUND)
    {
        if(packet_to_send->command == CMD_CREATE_FILE || packet_to_send->command == CMD_CREATE_FOLDER)
            printf(RED "[Client] `CREATE - %s` Operation Failed! Path Not Found!\n" DEFAULT, packet_to_send->path);
        else if(packet_to_send->command == CMD_DELETE_FILE || packet_to_send->command == CMD_DELETE_FOLDER)
            printf(RED "[Client] `DELETE - %s` Operation Failed! Path Not Found!\n" DEFAULT, packet_to_send->path);
    }
    else
    {
        printf(RED "[Client] Error %d: Operation Failed!\n" DEFAULT, packet_to_receive->command);
    }


    free(packet_to_send);
    free(packet_to_receive);

    return;

    return;
}
