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
        perror("inet_pton error\n");
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
        perror("setsockopt error\n");
        close(sock->socket_fd);
        return 0;
    }

    if(handshake(sock) == 0)
    {
        return 0;
    }
    return 1;
}


int rudp_accept(p_RUDP_Sock sock)
{
    p_rudp_pack recv_pack = create_packet();
    memset(recv_pack, 0, sizeof(rudp_pack));
    int bytes_received = recvfrom(sock->socket_fd, recv_pack, sizeof(rudp_pack), 0, (struct sockaddr *)&sock->destination_addr, &dest_addr_len);

    if(!sock->Server)
    {
        printf("This function is only valid to servers\n");
        return 0;
    }

    if(bytes_received < 0)
    {
        perror("Receiving failed\n");
        return 0;
    }

    if(bytes_received == 0)
    {
        perror("Connection closed from sender's side\n");
        return 0;
    }

    if(recv_pack->packet_flags.SYN)
    {
        if((recv_pack->packet_flags.ACK | recv_pack->packet_flags.FIN) != 0)
        {
            printf("Not a valid packet for connection establishment\n");
            return 0;
        }

        if(recv_pack->sequence != 0)
        {
            printf("Not a valid sequence number for connection establishment\n");
            return 0;
        }

        if(calculate_checksum(recv_pack, sizeof(rudp_pack)) != recv_pack->checksum)
        {
            printf("Checksum error\n");
            return 0;
        }

        if(send_SYN_ACK(sock, recv_pack->length))
        {
            printf("Connection established\n");
            return 1;
        }       
    }
    printf("Not a valid packet for connection establishment\n");
    return 0;
}

