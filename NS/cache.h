#ifndef __CACHE_H
#define __CACHE_H

#include <stdio.h>
#include "../header.h"

typedef struct cache {
    int ss_index;
    char *path;
    time_t time_stamp;
} cache;

void init_cache_array();
int get_storage_server_index(char *path);
int least_recently_used();
int add_to_cache(int ss_index, char *path);
int update_cache(int ss_index, char *path);
int remove_from_cache(char *path);

#endif

