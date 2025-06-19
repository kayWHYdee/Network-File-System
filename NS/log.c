#include "../header.h"

#define LOG_FILE "log.txt"

// log the details of the request and response going to and from the naming server
void log_request_response(request_packet req, response_packet res)
{
    FILE *file = fopen(LOG_FILE, "a");
    if (file == NULL)
    {
        perror("Failed to open log file");
        return;
    }

    fprintf(file, "Request: command=%d, path=%s\n", req.command, req.path);
    fprintf(file, "Response: error_code=%d\n ----------------------\n\n", res.error_code);

    fclose(file);
}

// log the details of the request and response going to and from the storage server
void log_ss_request_response(direct_to_ss_packet req, direct_to_ss_response_packet res)
{
    FILE *file = fopen(LOG_FILE, "a");
    if (file == NULL)
    {
        perror("Failed to open log file");
        return;
    }

    fprintf(file, "Request: command=%d, path=%s\n", req.command, req.path);
    fprintf(file, "Response: error_code=%d\n ----------------------\n\n", res.error_code);

    fclose(file);
}

// log the details of the storage server registration
void log_ss_registration(StorageServer ss)
{
    FILE *file = fopen(LOG_FILE, "a");
    if (file == NULL)
    {
        perror("Failed to open log file");
        return;
    }

    // print all details of the storage server , ip, ports for connection, server number, accessible paths, etc
    fprintf(file, "\nStorage server registered:\n ip=%s\n port_client_conn=%d\n port_ns_conn=%d\n server number=%d\n ------------------\n\n", ss.ip, ss.port_client, ss.port_nm, ss.ss_index);
    
    fclose(file);
}

void log_ss_failure(char *ip, int port_nm)
{
    FILE *file = fopen(LOG_FILE, "a");
    if (file == NULL)
    {
        perror("Failed to open log file");
        return;
    }

    fprintf(file, "Storage server failed: ip=%s, port=%d\n ----------------------\n\n", ip, port_nm);

    fclose(file);
}
