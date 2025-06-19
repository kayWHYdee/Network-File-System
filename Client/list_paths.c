#include "client.h"

void list_paths()
{
    request_packet *packet_to_send = (request_packet *)malloc(sizeof(request_packet));
    response_packet *packet_to_receive = (response_packet *)malloc(sizeof(response_packet));

    packet_to_send->command = CMD_GET_PATH;

    printf(CYAN "[Client] Enter the path to list: " DEFAULT);
    scanf("%s", packet_to_send->path);

    printf(CYAN "[Client] Requesting all accessible paths from Naming Server...\n" DEFAULT);
    if (send(naming_server_file_descriptor, packet_to_send, sizeof(*packet_to_send), 0) < 0)
    {
        perror(RED "[Client] Error sending request to Naming Server\n" DEFAULT);
        free(packet_to_send);
        free(packet_to_receive);
        return;
    }

    List list_of_paths;
    if (recv(naming_server_file_descriptor, &list_of_paths, sizeof(list_of_paths), 0) < 0)
    {
        perror(RED "[Client] Error receiving response from Naming Server\n" DEFAULT);
        free(packet_to_send);
        free(packet_to_receive);
        return;
    }

    if (list_of_paths.npaths == 0)
    {
        printf(RED "[Client] No paths found!\n" DEFAULT);
    }
    else
    {
        printf(YELLOW "[Client] Received paths from Naming Server\n" DEFAULT);
        for (int i = 0; i < list_of_paths.npaths; i++)
        {
            printf("%s\n", list_of_paths.paths[i]);
        }
    }

    // Send an acknowledgement to the naming server
    packet_to_send->command = ACK;
    if (send(naming_server_file_descriptor, packet_to_send, sizeof(*packet_to_send), 0) < 0)
    {
        perror(RED "[Client] Error sending acknowledgement to Naming Server\n" DEFAULT);
    }
    else
    {
        printf(GREEN "[Client] Acknowledgement sent to Naming Server\n" DEFAULT);
    }

    free(packet_to_send);
    free(packet_to_receive);
}