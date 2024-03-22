#include "RUDP_API.h"



p_RUDP_Sock rudp_socket(unsigned short int listen_port, int if_server)
{
    p_RUDP_Sock sock = (p_RUDP_Sock)malloc(sizeof(RUDP_Sock));

    if(!sock)
    {
        printf("Allocation error\n");
        return Null;
    }

    int sock_fd = create_socket();
    
    if(!sock_fd)
    {
        printf("Problem creating socket\n");
        return Null;
    }

    sock->sock_fd = sock_fd;
    sock->Server = if_server;
    sock->Connection = 0;

    memset(&sock->destination_addr, 0, sizeof(struct sockaddr_in));

    return sock;
}


int rudp_connect(p_RUDP_Sock sock, const char* dest_ip, unsigned short int dest_port)
{
    if(sock->Server || sock->Connection)
    {
        return 0;
    }

    if(inet_pton(AF_INET, dest_ip, &sock->destination_addr.sin_addr) < 1)
    {
        perror("inet_pton error");
        close(sock->sock_fd);
        return 0;
    }

    sock->destination_addr.sin_family = AF_INET;
    sock->destination_addr.sin_port = htons(dest_port);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec =10000;

    if(setsockopt(sock->socket_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)) == -1)
    {
        perror("setsockopt error");
        close(sock->socket_fd);
        return 0;
    }

    if(handshake(sock) == 0)
    {
        return 0;
    }
    return 1;
}
