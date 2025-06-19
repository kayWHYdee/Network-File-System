// #include "trie.h"
// #include "header.h"

#include <stdio.h>
#include <string.h>


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

#define MAX_PATH_LENGTH 256
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

void initTrieNode(Trie_Struct *trie)
{
    for (int i = 0; i < alphabets; i++)
    {
        trie->children[i] = NULL;
    }
    trie->exists_in_server = -1;
    trie->num_read_counts = 0;
    sem_init(&trie->w_mutex, 0, 1);  // Initialize write semaphore (binary semaphore)
    sem_init(&trie->rw_mutex, 0, 1); // Initialize read-write semaphore (binary semaphore)
}

// Function to initialize and return the root node of the Trie
Trie_Struct *initializeTrie()
{
    Trie_Struct *root = (Trie_Struct *)malloc(sizeof(Trie_Struct));
    if (root == NULL)
    {
        printf("Memory allocation failed for root node.\n");
        exit(1);
    }
    initTrieNode(root);
    return root;
}

// Function to initialize a TrieNode (each node of the Trie)


// Function to convert a character into an index based on the provided alphabet and special characters
int charToIndex(char c)
{
    if (c >= 'a' && c <= 'z')
    {
        return c - 'a';
    }
    else if (c >= 'A' && c <= 'Z')
    {
        return c - 'A' + 26;
    }
    else if (c >= '0' && c <= '9')
    {
        return c - '0' + 52;
    }
    else if (c == '/')
    {
        return 62;
    }
    else if (c == '.')
    {
        return 63;
    }
    else
    {
        return -1; // Invalid character
    }
}

// Function to insert a file into the trie
void insertFile(Trie_Struct *root, const char *path, int server_number)
{
    int length = strlen(path);
    Trie_Struct *current = root;

    // Traverse the path and create nodes if needed
    for (int i = 0; i < length; i++)
    {
        int index = charToIndex(path[i]);
        if (index == -1)
        {
            printf("Invalid character in path: %c\n", path[i]);
            return;
        }

        if (current->children[index] == NULL)
        {
            current->children[index] = (Trie_Struct *)malloc(sizeof(Trie_Struct));
            initTrieNode(current->children[index]);
        }
        current = current->children[index];
    }

    // Lock the write semaphore for inserting into the trie
    sem_wait(&current->w_mutex);

    // If the file is being inserted for the first time, assign the storage server
    for (int i = 0; i < 3; i++)
    {
        if (current->exists_in_server == -1)
        {
            current->exists_in_server = server_number;
            break;
        }
    }

    // Update the read count
    current->num_read_counts = 0;

    // Print out for debugging
    printf("Inserted file: %s in server %d\n", path, server_number);
    printf("File exists in servers: %d\n", current->exists_in_server);

    sem_post(&current->w_mutex); // Unlock the write lock
}

// Function to search for a file in the trie
int searchFile(Trie_Struct *root, const char *path)
{
    int length = strlen(path);
    Trie_Struct *current = root;

    // Traverse the path and check for the file
    for (int i = 0; i < length; i++)
    {
        int index = charToIndex(path[i]);
        if (index == -1 || current->children[index] == NULL)
        {
            printf("File not found: %s\n", path);
            return -1; // File not found
        }
        current = current->children[index];
    }

    // Lock the read-write semaphore to read the file details
    sem_wait(&current->rw_mutex);

    // Return the storage servers for the file
    if (current->exists_in_server != -1)
    {
        printf("Searching file: %s, found in %d server.\n", path, current->exists_in_server);
        sem_post(&current->rw_mutex); // Unlock the read-write lock
        return current->exists_in_server;
    }
    else
    {
        printf("File not found: %s\n", path);
        sem_post(&current->rw_mutex); // Unlock the read-write lock
        return -1;
    }

    sem_post(&current->rw_mutex); // Unlock the read-write lock
}


// Function to increment the read count for a file
void incrementReadCount(Trie_Struct *root, const char *path)
{
    int length = strlen(path);
    Trie_Struct *current = root;

    // Traverse the path to find the file node
    for (int i = 0; i < length; i++)
    {
        int index = charToIndex(path[i]);
        if (index == -1 || current->children[index] == NULL)
        {
            printf("File not found for incrementing read count: %s\n", path);
            return;
        }
        current = current->children[index];
    }

    // Lock the read-write semaphore to modify the read count
    sem_wait(&current->rw_mutex);

    // Increment the read count
    current->num_read_counts++;

    printf("Read count incremented for %s. Current count: %d\n", path, current->num_read_counts);

    sem_post(&current->rw_mutex); // Unlock the read-write lock
}

// Function to delete a file from the trie (if necessary)
void deleteFile(Trie_Struct *root, const char *path)
{
    int length = strlen(path);
    Trie_Struct *current = root;

    // Traverse the path and delete nodes if needed
    for (int i = 0; i < length; i++)
    {
        int index = charToIndex(path[i]);
        if (index == -1 || current->children[index] == NULL)
        {
            // printf("File not found for deletion: %s\n", path);
            return;
        }
        current = current->children[index];
    }

    // Lock the write semaphore for deleting the file
    sem_wait(&current->w_mutex);

    // Mark the file as deleted by clearing the server list
    current->exists_in_server = -1;

    // Reset the read count
    current->num_read_counts = 0;

    printf("Deleted file: %s\n", path);

    sem_post(&current->w_mutex); // Unlock the write lock
}


int indexToChar(int index)
{
    if (index >= 0 && index < 26)
    {
        return 'a' + index;
    }
    else if (index >= 26 && index < 52)
    {
        return 'A' + index - 26;
    }
    else if (index >= 52 && index < 62)
    {
        return '0' + index - 52;
    }
    else if (index == 62)
    {
        return '/';
    }
    else if (index == 63)
    {
        return '.';
    }
    else
    {
        return -1; // Invalid character
    }
}

// Function to recursively delete a folder from the trie
void deleteFolder(Trie_Struct *root, const char *path, int server_number)
{
    int length = strlen(path);
    Trie_Struct *current = root;

    // Traverse the path to find the folder node
    for (int i = 0; i < length; i++)
    {
        int index = charToIndex(path[i]);
        if (index == -1 || current->children[index] == NULL)
        {
            // printf("Folder not found for deletion: %s\n", path);
            return;
        }
        current = current->children[index];
    }

    // Lock the write semaphore for deleting the folder
    sem_wait(&current->w_mutex);

    // Recursively delete all children nodes that match the server number
    for (int i = 0; i < alphabets; i++)
    {
        if (current->children[i] != NULL)
        {
            char new_path[MAX_PATH_LENGTH];
            snprintf(new_path, sizeof(new_path), "%s%c", path, indexToChar(i));
            deleteFolder(current, new_path, server_number);
            if (current->children[i]->exists_in_server == server_number)
            {
                free(current->children[i]);
                current->children[i] = NULL;
            }
        }
    }

    // Mark the folder as deleted by clearing the server list if it matches the server number
    if (current->exists_in_server == server_number)
    {
        current->exists_in_server = -1;
        current->num_read_counts = 0;
        printf("Deleted folder: %s\n", path);
    }

    sem_post(&current->w_mutex); // Unlock the write lock
}

// Recursive helper function to list all files and subfolders within a specified directory
void listHelper(Trie_Struct *current, char *currentPath, int length, char **result, int *count)
{
    // Base case: If the current node is a file, add the path to the result array
    if (current->exists_in_server != -1)
    {
        result[*count] = (char *)malloc(MAX_PATH_LENGTH);
        strcpy(result[*count], currentPath);
        (*count)++;
    }

    // Recursive case: Traverse all children nodes
    for (int i = 0; i < alphabets; i++)
    {
        if (current->children[i] != NULL)
        {
            char newPath[MAX_PATH_LENGTH] = {0};
            snprintf(newPath, sizeof(newPath), "%s%c", currentPath, indexToChar(i));
            listHelper(current->children[i], newPath, length + 1, result, count);
        }
    }
}

void listFilesAndFolders(Trie_Struct *root, const char *path, char **result, int *count)
{
    int length = strlen(path);
    Trie_Struct *current = root;

    // Traverse the path to find the directory node
    for (int i = 0; i < length; i++)
    {
        int index = charToIndex(path[i]);
        if (index == -1 || current->children[index] == NULL)
        {
            printf("Directory not found: %s\n", path);
            return;
        }
        current = current->children[index];
    }

    char currentPath[MAX_PATH_LENGTH] = {0};
    strcpy(currentPath, path);
    listHelper(current, currentPath, length, result, count);
}


void freeTrie(Trie_Struct *root)
{
    if (root == NULL)
    {
        return;
    }

    for (int i = 0; i < alphabets; i++)
    {
        if (root->children[i] != NULL)
        {
            freeTrie(root->children[i]);
        }
    }

    sem_destroy(&root->w_mutex);
    sem_destroy(&root->rw_mutex);
    free(root);
}

// #include "trie.h"
// #include "header.h"

// int main() {
//     // Initialize the Trie
//     Trie_Struct *root = initializeTrie();
//     printf("Trie initialized successfully\n\n");

//     // Test case 1: Insert files into different servers
//     printf("=== Testing File Insertions ===\n");
//     insertFile(root, "folder1/file1.txt", 1);
//     insertFile(root, "folder1/file2.txt", 2);
//     insertFile(root, "folder2/file3.txt", 3);
//     insertFile(root, "test.txt", 1);
//     printf("\n");

//     // Test case 2: Search for existing files
//     printf("=== Testing File Search ===\n");
//     printf("Searching for folder1/file1.txt...\n");
//     int server = searchFile(root, "folder1/file1.txt");
//     printf("Server returned: %d\n", server);

//     printf("\nSearching for test.txt...\n");
//     server = searchFile(root, "test.txt");
//     printf("Server returned: %d\n", server);

//     // Test case 3: Search for non-existent file
//     printf("\nSearching for nonexistent.txt...\n");
//     server = searchFile(root, "nonexistent.txt");
//     printf("Server returned: %d\n\n", server);

// //     // Test case 4: Increment read counts
// //     printf("=== Testing Read Count Increment ===\n");
// //     incrementReadCount(root, "folder1/file1.txt");
// //     incrementReadCount(root, "folder1/file1.txt");
// //     incrementReadCount(root, "folder1/file1.txt");
// //     printf("\n");

//     // Test case 5: Delete individual file
//     printf("=== Testing File Deletion ===\n");
//     deleteFile(root, "test.txt");
//     printf("\nTrying to search deleted file test.txt...\n");
//     server = searchFile(root, "test.txt");
//     printf("Server returned: %d\n\n", server);

//     // Test case 6: Delete folder and its contents
//     printf("=== Testing Folder Deletion ===\n");
//     server = searchFile(root, "folder1");
//     deleteFolder(root, "folder1", server);
//     printf("\nTrying to search file in deleted folder (folder1/file1.txt)...\n");
//     server = searchFile(root, "folder1/file1.txt");
//     printf("Server returned: %d\n\n", server);
//     server = searchFile(root, "folder1/file2.txt");
//     printf("Server returned: %d\n\n", server);

//     // Test case 7: Insert file with invalid characters
//     printf("=== Testing Invalid Input ===\n");
//     insertFile(root, "invalid@file.txt", 1);
//     printf("\n");

//     // Clean up (basic cleanup - in a real application, you'd want to free all allocated memory)
//     free(root);
//     printf("=== Test Complete ===\n");

//     return 0;
// }