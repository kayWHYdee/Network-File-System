#include "header.h"
 
int main()
{
    char hostbuffer[256];
    struct hostent *host_entry;
    int hostname;
    struct in_addr **addr_list;

    // retrieve hostname
    hostname = gethostname(hostbuffer, sizeof(hostbuffer));
    if (hostname == -1) {
        perror("gethostname error");
        exit(1);
    }
    printf("Hostname: %s\n", hostbuffer);

    // Retrieve IP addresses
    host_entry = gethostbyname(hostbuffer);
    if (host_entry == NULL) {
        perror("gethostbyname error");
        exit(1);
    }
    addr_list = (struct in_addr **)host_entry->h_addr_list;

    // print the last IP address from that array
    for (int i = 0; addr_list[i] != NULL; i++) {
        printf("IP Address %d: %s\n", i + 1, inet_ntoa(*addr_list[i]));
    }


    

    return 0;
}