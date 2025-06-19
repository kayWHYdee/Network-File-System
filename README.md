[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/l9Jxgebc)


# Network File System

## Overview

This project implements a Network File System (NFS) that allows clients to interact with files and directories stored across multiple storage servers. The system includes a Naming Server (NS) that manages the metadata and coordinates operations between clients and storage servers.

## Components

1. **Naming Server (NS)**: The central coordination hub that maintains a directory of files and their corresponding storage locations. It facilitates communication between clients and storage servers, ensuring efficient file access and retrieval.  
2. **Storage Servers (SS)**: Responsible for physically storing and managing files and folders. They ensure data persistence and provide file operations to clients under the guidance of the Naming Server. 
3. **Client**: Clients act as the user interface for interacting with the NFS. They initiate file-related operations such as reading, writing, deleting, streaming, and more.  


## Features

- **File and Directory Operations**: Create, delete, read, write, and list files and directories.
- **Copy Operations**: Copy files and directories between different storage servers.
- **Backup and Redundancy**: Supports backup configurations to ensure data redundancy.
- **Caching**: Implements a caching mechanism to improve performance by reducing metadata lookup times.
- **File and Folder Operations**: Create, read, write (synchronous and asynchronous), delete, and list files or folders.  
- **Metadata Access**: Retrieve additional file details such as size, permissions, and timestamps.  
- **Streaming Audio Files**: Stream audio files directly from storage servers.  
- **Dynamic Scalability**: Add new storage servers dynamically during runtime.  
- **Client Feedback**: Timely acknowledgments and updates on task status.
- **Logging**: All details of communication are logged in a file for easier tracking.

## Usage

### Setup

1. Clone the repository to your local system.  
2. Navigate to the respective folders for the Naming Server, Storage Server, and Client.  

### 1. Starting the Naming Server

To start the Naming Server, go to the NS folder, using `cd NS` and run the following command

```sh
    make clean ; make ; ./nm
```

### 2. Start the Storage Servers 

To start the Storage Server, go to the NS folder, using `cd SS` and run the following command
 
```sh
   make clean ; make ; ./ss <Naming_Server_IP>
```

### 3. Start the Client 

To start the Storage Server, go to the NS folder, using `cd SS` and run the following command

```sh
   make clean ; make ;  ./client <Naming_Server_IP>
```
  
- Note: Look for the IP of the naming server by looking in the system where the Naming Server is run.


## Details

### 1. Client
The client will connect to the Naming Server and provide a menu for file operations.

Client Menu
The client menu provides the following options:

1. Read, Write, Stream and Retrieve File Info: Access files for reading, writing, streaming, and retrieving file information.
2. Create or Delete Files and Folders: Create or delete files and directories.
3. Copy Files/Directories Between Servers: Copy files or directories from one storage server to another.
4. List All Paths: List all files and directories in the system.
5. Exit: Exit the client application.

### 2. Naming and Storage Servers

#### 1.1 Initialization

1. Naming Server (NS):
   - Initializes as the central coordination point.
   - Registers incoming Storage Servers dynamically during startup or runtime.
   - Does not hardcode IP (127.0.0.1) or port for testing on multiple systems.

2. Storage Servers (SS):
   - Registers with the NS on startup, providing:
     - IP address and ports for communication.
     - A list of accessible file paths.
   - Supports dynamic addition of new servers.

3. Execution Flow:
   - Start the NS.
   - Initialize storage servers (SS_1, SS_2, ..., SS_n).
   - NS begins accepting client requests once storage servers are registered.

#### 1.2 Storage Server Functionalities

- Create, Delete, Copy Files/Folders: Executed based on NS commands.  
- Client Interactions: Handle client requests for reading, writing, streaming, and retrieving metadata.  

#### 1.3 Naming Server Functionalities

- Stores and maintains data about connected Storage Servers.  
- Provides clients with relevant server information for their requests.  

---

### 2. Clients

- Clients initiate communication with the NS to perform the following operations:

- 1. Pathfinding: NS determines the appropriate Storage Server for the file/folder path and shares the server's IP and port.  
- 2. Direct Communication: Once the Storage Server is identified, clients interact directly with it for operations like:
   - Reading/Writing files.
   - Streaming audio.
   - Retrieving file details (size, permissions, etc.).
   - Copying files/folders.
   - Listing all accessible paths.


## Conclusion
This Network File System provides a robust and scalable solution for managing files and directories across multiple storage servers. It ensures data redundancy, supports various file operations, and offers a user-friendly client interface for interacting with the system.


## Assumptions

- trie has a `function int searchFile(Trie_Struct *root, const char *path);` that returns the ID of the SS in which the corresponding path is stored. 

- We are assuming that the root is only in Storage Server 0 and it is enetered as an accessible path i.e enter "." as accessible path then sab chal jayega root mai creat delete everything

- No two files have the same path and same name at the same time i.e two files with the same name cant have ths same relative accessible path even if in different storage servers
Eg. abc/cd.txt exists in SS0
    bce/cd.txt exists in SS2 
    This is allowed

    abc/cd.txt exists in SS0
    abc/cd.txt exists in SS2 
    This is NOT allowed


- Copying a file/folder doesnt mean that it's path is accessible now for usage further

- Normal path goes in create, in delete we enter the absolute path

- port "9998, 9999" are constantly being used for connecting to handle the copy function hence using these two anywhere else will create a problem.

- the number of maximum paths can only be 20.

- Many Others, didnt have time couldnt write all


## Contributors

- Khooshi Asmi
- Kushagra Dhingra 
- Sujal Deoda 
- Uday Bindal 


---------------------------




