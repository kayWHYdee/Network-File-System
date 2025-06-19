#ifndef __TRIE_H__
#define __TRIE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <semaphore.h>

#define alphabets 68 // 52 alphabets + 10 numbers + '/' + '.'

// ASSUMPTION: Two files with the same name cant exist in two different storage servers

typedef struct Trie_Struct
{
    struct Trie_Struct *children[alphabets];
    int exists_in_server;        // has the number of the storage server, 0 index is the primary server,
    int num_read_counts;            // tells the number of read counts on the file
    sem_t w_mutex;                  // Write lock for thread safety
    sem_t rw_mutex;                 // Read-write lock for thread safety
    
} Trie_Struct;

/* 

For backup, if in
(a) SS1 -> SS2, SS3
(b) SS2 -> SS1, SS3
(c) SS3 -> SS1, SS2
For n>3
(d) SS[n] -> SS[n-1], SS[n-2]

*/ 

// Function to initialize and return the root node of the Trie
Trie_Struct* initializeTrie();

// Function to initialize a TrieNode (each node of the Trie)
void initTrieNode(Trie_Struct *trie);

// Function to convert a character into an index based on the provided alphabet and special characters
int charToIndex(char c);

// Function to insert a file into the trie
void insertFile(Trie_Struct *root, const char *path, int server_number);

// Function to search for a file in the trie
int searchFile(Trie_Struct *root, const char *path);

// Function to increment the read count for a file
void incrementReadCount(Trie_Struct *root, const char *path);

// Function to delete a file from the trie (if necessary)
void deleteFile(Trie_Struct *root, const char *path);

// Function to recursively delete a folder from the trie
void deleteFolder(Trie_Struct *root, const char *path, int server_number);

// Function to list all files and subfolders within a specified directory
void listFilesAndFolders(Trie_Struct *root, const char *path, char **result, int *count);

// Free Trie
void freeTrie(Trie_Struct *root);

#endif