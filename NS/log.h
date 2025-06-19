#ifndef __LOG_H__
#define __LOG_H__
#include "../header.h"

#define LOG_FILE "log.txt"

void log_request_response(request_packet req, response_packet res);
void log_ss_request_response(direct_to_ss_packet req, direct_to_ss_response_packet res);
void log_ss_registration(StorageServer ss);
void log_ss_failure(char *ip, int port_nm);

#endif