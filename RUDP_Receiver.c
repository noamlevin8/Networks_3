#include "RUDP_API.h"


int main(int argc, char* argv[]) 
{
    int Receiver_Port = atoi(argv[2]);

    char* str = malloc(34);
    strcpy(str, "-       * Statistics *        -\n");

    struct sockaddr_in ReceiverAddress;
    p_RUDP_Sock sock = rudp_socket(Receiver_Port, 1);

    if(!sock)
    {
        return -1;
    }

    memset(&ReceiverAddress, 0, sizeof(ReceiverAddress));
    ReceiverAddress.sin_family = AF_INET; // IPV4
    ReceiverAddress.sin_addr.s_addr = INADDR_ANY; // Random address
    ReceiverAddress.sin_port = htons(Receiver_Port); //Port

    // Bind the socket to the port with any IP at this port
    if (bind(sock->socket_fd, (struct sockaddr *)&ReceiverAddress, sizeof(ReceiverAddress)) == -1) 
    {
        printf("Bind failed with error code : %d", errno);
        // close the socket
        rudp_close(sock);
        return -1;
    }
}