#ifndef CLIENT_H
#define CLIENT_H

#include "../header.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>

extern char ns_ip[16];
extern int naming_server_file_descriptor, storage_server_file_descriptor;
extern struct sockaddr_in nm_server_address, storage_server_address;

// Function Prototypes
int initialize_socket(int type);
void connect_to_server(int socket_fd, struct sockaddr_in *server_address);
void print_menu();
void access_file();              // Read, Write, and Retrieve File Info
void create_delete_function();  // Create/Delete Files and Folders
void move_function();             // Copy Files/Directories Between Servers
void list_paths();                // List All Paths
void handle_menu_choice(int choice);
int send_packet_to_naming_server(request_packet *packet_to_send, response_packet *packet_to_receive);


#endif  
