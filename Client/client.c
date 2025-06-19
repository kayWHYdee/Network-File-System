#include "client.h"

char ns_ip[16];
int naming_server_file_descriptor;
struct sockaddr_in nm_server_address, storage_server_address;

int initialize_socket(int type)
{
    printf("MAKING SOCKET\n");
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1)
    {
        if (type == 1)
            perror(RED "[Client] Error creating socket.\n" DEFAULT);
        else if (type == 2)
            perror(RED "[SS] Error creating socket.\n" DEFAULT);

        exit(EXIT_FAILURE);
    }
    return socket_fd;
}

void connect_to_server(int socket_fd, struct sockaddr_in *server_address)
{
    // printf(YELLOW "[CLIENT] Attempting to connect to server...\n" DEFAULT);
    if (connect(socket_fd, (struct sockaddr *)server_address, sizeof(*server_address)) == -1)
    {
        perror(RED "[CLIENT] Connection failed.\n" DEFAULT);
        close(socket_fd);
        exit(EXIT_FAILURE);
    }
    // printf(GREEN "[CLIENT] Connection established.\n" DEFAULT);
}

void handle_menu_choice(int choice)
{
    switch (choice)
    {
    case 1:
        printf(YELLOW "Accessing File\n" DEFAULT);
        access_file();
        break;
    case 2:
        create_delete_function();
        break;
    case 3:
        move_function();
        break;
    case 4:
        list_paths();
        break;
    default:
        printf(RED "[CLIENT] Invalid selection. Please choose from 1-5.\n" DEFAULT);
        break;
    }
}

void print_menu()
{
    printf("\n===========================================\n");
    printf("            FILE MANAGEMENT MENU           \n");
    printf("===========================================\n");
    printf(" 1. Read, Write, Stream and Retrieve File Info    \n");
    printf(" 2. Create or Delete Files and Folders     \n");
    printf(" 3. Copy Files/Directories Between Servers \n");
    printf(" 4. List All Paths                         \n");
    printf(" 5. Exit                                   \n");
    printf("===========================================\n");
    printf(" Enter your choice: ");
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <Naming Server IP>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    char ns_ip[16];
    strcpy(ns_ip, argv[1]);
    naming_server_file_descriptor = initialize_socket(1);

    memset(&nm_server_address, '\0', sizeof(nm_server_address));

    nm_server_address.sin_family = AF_INET;
    nm_server_address.sin_addr.s_addr = inet_addr(ns_ip);
    nm_server_address.sin_port = htons(NM_CLIENT_PORT); 

    printf(YELLOW "[CLIENT] Attempting to connect to server...\n" DEFAULT);
    connect_to_server(naming_server_file_descriptor, &nm_server_address);
    printf(GREEN "[CLIENT] Connection established.\n" DEFAULT);

    while (1)
    {
        int choice = 0;
        print_menu();
        if (scanf("%d", &choice) != 1)
        {
            printf(RED "[CLIENT] Invalid input. Please enter a valid choice.\n" DEFAULT);
            while (getchar() != '\n'); // Clear the input buffer
            continue;
        }
        printf("\n");
        if (choice == 5)
        {
            printf(RED "[CLIENT] Shutting down client...\n" DEFAULT);
            break;
        }
        handle_menu_choice(choice);

        fflush(stdin);
    }
    printf("\n");
    close(naming_server_file_descriptor);
    return 0;
}
