#include "../header.h"


cache *cacheing_array[MAX_CACHE_SIZE];

// initialize the cache array
void init_cache_array()
{
    for(int i=0;i<MAX_CACHE_SIZE;i++)
    {   
        cacheing_array[i] = (cache*)malloc(sizeof(cache));
        cacheing_array[i]->ss_index = -1;
        cacheing_array[i]->path = (char*)malloc(MAX_PATH_LENGTH*sizeof(char));
        for (int j = 0; j < MAX_PATH_LENGTH; j++)
        {
            cacheing_array[i]->path[j] = '\0';
        }
        cacheing_array[i]->time_stamp = -1;
    }
}

// get the storage server index of the path
int get_storage_server_index(char *path)
{
    for (int i = 0; i < MAX_CACHE_SIZE; i++)
    {
        if (strcmp(cacheing_array[i]->path, path) == 0)
        {
            cacheing_array[i]->time_stamp = time(NULL);
            return cacheing_array[i]->ss_index;
        }
    }

    return -1;
}

// get the index of the least recently used path
int least_recently_used()
{
    time_t min_time = cacheing_array[0]->time_stamp;
    int index = 0;
    for (int i = 1; i < MAX_CACHE_SIZE; i++)
    {
        if (cacheing_array[i]->time_stamp < min_time)
        {
            min_time = cacheing_array[i]->time_stamp;
            index = i;
        }
    }
    return index;
}

// add the path to the cache
int add_to_cache(int ss_index, char *path)
{
    for (int i = 0; i < MAX_CACHE_SIZE; i++)
    {
        if (cacheing_array[i]->ss_index == -1)
        {
            cacheing_array[i]->ss_index = ss_index;
            strcpy(cacheing_array[i]->path, path);
            cacheing_array[i]->time_stamp = time(NULL);
            return 0;
        }
    }

    int lru_index = least_recently_used();
    cacheing_array[lru_index]->ss_index = ss_index;
    strcpy(cacheing_array[lru_index]->path, path);
    cacheing_array[lru_index]->time_stamp = time(NULL);
    return 0;
}

// update the cache with the new storage server index
int update_cache(int ss_index, char *path)
{
    for (int i = 0; i < MAX_CACHE_SIZE; i++)
    {
        if (strcmp(cacheing_array[i]->path, path) == 0)
        {
            cacheing_array[i]->ss_index = ss_index;
            cacheing_array[i]->time_stamp = time(NULL);
            return 0;
        }
    }
    return -1;
}

// remove the path from the cache
int remove_from_cache(char *path)
{
    for (int i = 0; i < MAX_CACHE_SIZE; i++)
    {
        if (strcmp(cacheing_array[i]->path, path) == 0)
        {
            cacheing_array[i]->ss_index = -1;
            for (int j = 0; j < MAX_PATH_LENGTH; j++)
            {
                cacheing_array[i]->path[j] = '\0';
            }
            cacheing_array[i]->time_stamp = -1;
            return 0;
        }
    }
    return -1;
}

// int main()
// {
//     init_cache_array();
//     return 0;
// }


