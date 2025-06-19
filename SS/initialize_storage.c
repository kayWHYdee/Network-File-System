#include "../header.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ifaddrs.h>
#include "funcdef.h"

char naming_server_ip[30];

// Function to handle client requests (placeholder for 
char * get_ip();

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <Naming Server IP>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    StorageServer ss;
    // also have to initialize the different strings and everything that are being stored inside a function to malloc and free everything to be in place
    char *ns_ip = argv[1];
    strcpy(naming_server_ip,ns_ip);
    printf("%s %s\n",ns_ip,naming_server_ip);
    // Input ports
    char *current_ip = (char *)malloc(16);
    current_ip = get_ip();

    strcpy(ss.ip, current_ip);
    printf("Current IP: %s\n", current_ip);

    printf("Enter the port for Naming Server communication: ");
    scanf("%d", &ss.port_nm);
    printf("Enter the port for Client communication: ");
    scanf("%d", &ss.port_client);

    printf("Storage Server IP: %s\n", ns_ip);
    // log_ss_registration(ss); // Log storage server registration

    // Connect to Naming Server
    struct sockaddr_in naming_server_addr;
    ss.socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (ss.socket_fd < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    printf("Socket created successfully. Socket FD: %d\n", ss.socket_fd);

    naming_server_addr.sin_family = AF_INET;
    // naming_server_addr.sin_addr.s_addr = INADDR_ANY;
    naming_server_addr.sin_port = htons(NM_SS_PORT);
    printf("Port set to: %d (network byte order: %d)\n", ss.port_nm, naming_server_addr.sin_port);

    if (inet_pton(AF_INET, argv[1], &naming_server_addr.sin_addr) <= 0)
    {
        perror("Invalid Naming Server IP address");
        close(ss.socket_fd);
        exit(EXIT_FAILURE);
    }
    printf("Naming Server IP address set to: %s\n", argv[1]);

    printf("Attempting to connect to the Naming Server...\n");

    if (connect(ss.socket_fd, (struct sockaddr *)&naming_server_addr, sizeof(naming_server_addr)) < 0)
    {
        perror("Connection to Naming Server failed");
        close(ss.socket_fd);
        exit(EXIT_FAILURE);
    }
    printf("Connected to Naming Server.\n");

    int counter = 0;
    char str[MAX_PATHS] = {0};
    printf(GREEN "Enter Accessible Paths: \n" DEFAULT);
    while (fgets(str, MAX_PATHS, stdin) != NULL)
    {
        if (str[0] == '\n')
            continue; // Eat a newline character
        str[strcspn(str, "\n")] = '\0';
        // memset()
        strcpy(ss.accessible_paths[counter++], str);
        ss.num_paths = counter;
    }
    ss.is_connected = 1;

    printf("Storage Server information being sent to Naming Server...\n");
    int bytes_sent = send(ss.socket_fd, &ss, sizeof(StorageServer), 0);
    printf("bytes %d", bytes_sent);
    if (bytes_sent < 0)
    {
        perror("Failed to send StorageServer information");
        close(ss.socket_fd);
        exit(EXIT_FAILURE);
    }
    printf("Storage Server information sent to Naming Server.\n");

    char ack_message[BUFFER_SIZE];
    if (recv(ss.socket_fd, ack_message, sizeof(ack_message), 0) > 0)
    {
        printf("Acknowledgment from Naming Server: %s\n", ack_message);
    }
    else
    {
        perror("Failed to receive acknowledgment from Naming Server");
        close(ss.socket_fd);
        exit(EXIT_FAILURE);
    }

    // Create threads
    pthread_t nm_thread, client_thread;
    pthread_create(&nm_thread, NULL, &ns_direct_con, (void *)&ss);
    pthread_create(&client_thread, NULL, connect_to_client, (void *)&ss);

    // Join threads (placeholder, may adjust later)
    pthread_join(nm_thread, NULL);
    pthread_join(client_thread, NULL);

    close(ss.socket_fd);
    return 0;
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