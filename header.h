#ifndef HEADER_H
#define HEADER_H

#include "./NS/trie.h"
#include "./NS/cache.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <ifaddrs.h>
#include <dirent.h> 
#include <sys/types.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netdb.h>
#include <zlib.h>
#include <semaphore.h>

// #define NAMING_SERVER_IP "127.0.0.1"
extern char naming_server_ip[30];

#define NM_SS_PORT 7890
#define NM_CLIENT_PORT 7891

#define MAX_STORAGE_SERVERS 10
#define MAX_CLIENTS 20

#define BUFFER_SIZE 4096
#define MAX_PATHS 20
#define MAX_CACHE_SIZE 10
#define MAX_PATH_LENGTH 100

#define HEARTBEAT_INTERVAL 1


typedef struct
{
    char ip[16];
    int port_nm;
    int port_client;
    char accessible_paths[MAX_PATHS][256];
    int num_paths;
    int socket_fd;
    int is_connected;
    pthread_mutex_t response_lock;
    int ss_index;
    int backup_index;
    int backup2_index;
    
} StorageServer;

typedef struct
{
    char ip[16];
    int port;
    int socket_fd;
    pthread_t handler_thread;
    int is_connected;
} Client;

typedef struct
{
    StorageServer ss[MAX_STORAGE_SERVERS];
    int num_ss;
    Client clients[MAX_CLIENTS];
    int num_clients;
    pthread_mutex_t lock;
} NamingServer;

typedef int flag;

typedef struct
{
    // for all
    int command;
    char path[100];
    char contents[BUFFER_SIZE];
    // for forwarded commands
    char new_path[100]; // for copy
    int file_or_folder_flag;
    int sync_flag;
    int *index; // for communication between nm and ss while processing request

} request_packet;

typedef struct
{ //  nm -> client & ss -> nm
    // for both
    // why send the command again?
    int ack;
    int error_code;
    int command;
    char path[100];
    // only for read, write, stream, get_file_size_permission
    char ip[16];
    int port;
    int *index; // for communication between nm and ss while processing request

} response_packet;

// for audio and video Streaming
typedef struct
{
    int client_socket;
    char *filepath;
    sem_t rw_mutex; // Add this line
} StreamArgs;

// For direct communication between client and storage server
// tumlog change kar sakte ho if you want
typedef struct
{
    char path[100];
    int command;

    long long size;         // write size
    char data[BUFFER_SIZE]; // for write
    int sync_flag;
    sem_t rw_mutex; // Add this line
    sem_t w_mutex;  // Add this line
} direct_to_ss_packet;

typedef struct
{
    int error_code;
    long long size;
    char data[BUFFER_SIZE];
    // how to get a binary file // for stream
} direct_to_ss_response_packet;

typedef enum
{
    STOP = (-1),
    CMD_HEARTBEAT = 0,
    CMD_READ = 1,
    CMD_WRITE,
    CMD_GET_INFO,
    CMD_STREAM,
    CMD_DELETE_FILE,
    CMD_DELETE_FOLDER,
    CMD_CREATE_FILE,
    CMD_CREATE_FOLDER,
    CMD_COPY_FILE,
    CMD_COPY_FOLDER,
    CMD_GET_PATH,
    CMD_SEND_TO_NM,
    CMD_RECEIVE_FROM_NM,
    ACK_WRITE,
    ACK,
} Command;

enum file_or_folder
{
    FILE_TYPE = 0,
    FOLDER_TYPE = 1
};

typedef enum
{
    // ERROR = -1,
    SUCCESS = 0,
    PATH_NOT_FOUND = 404,
    TIME_OUT = 408,
    UNAUTHORIZED = 402,
    // baki tumlog define kar dena
    SS_DOWN_AND_NO_BACKUP = 405,
    SS_DOWN_AND_BOTH_BACKUP_DOWN = 406,
    BACKUP_ACTIVE = 407,
    FILE_ALREADY_EXISTS = 409, // for create and copy
    INVALID_COMMAND = 410,
    CLIENT_DISCONNECTED = 411,
    INIT_ERROR = 454,
    BAD_REQUEST = 400,
    NOT_SENT = 420,
    SOURCE_ERROR = 421,
    DEST_ERROR = 422,

} ERROR;

typedef struct List
{
    int npaths;
    char paths[MAX_PATHS][MAX_PATH_LENGTH];
} List;


#define DEFAULT "\033[1;0m" // Reset
#define YELLOW "\033[1;93m" // Client
#define CYAN "\033[1;96m"   // Storage Server
#define GREEN "\033[1;92m"  // Naming Server
#define RED "\033[1;31m"    // Error

#include "NS/log.h"

#endif