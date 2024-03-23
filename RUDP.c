#include "RUDP_API.h"



p_RUDP_Sock rudp_socket(unsigned short int listen_port, int if_server)
{
    p_RUDP_Sock sock = (p_RUDP_Sock)malloc(sizeof(RUDP_Sock));

    if(!sock)
    {
        printf("Allocation error\n");
        return NULL;
    }

    int sock_fd = create_socket();
    
    if(!sock_fd)
    {
        printf("Problem creating socket\n");
        return NULL;
    }

    sock->socket_fd = sock_fd;
    sock->Server = if_server;
    sock->Connection = 0;

    memset(&sock->destination_addr, 0, sizeof(struct sockaddr_in));

    return sock;
}


int rudp_connect(p_RUDP_Sock sock, const char* dest_ip, unsigned short int dest_port)
{
    if(sock->Server || sock->Connection) // Connect is only a client function
    {
        return 0;
    }

    if(inet_pton(AF_INET, dest_ip, &sock->destination_addr.sin_addr) < 1)
    {
        perror("inet_pton error\n");
        close(sock->socket_fd);
        return 0;
    }

    sock->destination_addr.sin_family = AF_INET;
    sock->destination_addr.sin_port = htons(dest_port);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec =10000;

    if(setsockopt(sock->socket_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)) == -1) // Timeout
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

// 0 - problem
// 1 - success
// 2 - connection closed
int rudp_accept(p_RUDP_Sock sock)
{
    p_rudp_pack recv_pack = create_packet();
    memset(recv_pack, 0, sizeof(rudp_pack));
    size_t dest_addr_len = sizeof(struct sockaddr);
    int bytes_received = recvfrom(sock->socket_fd, recv_pack, sizeof(rudp_pack), 0, (struct sockaddr *)&sock->destination_addr, &dest_addr_len);

    if(!sock->Server)
    {
        printf("This function is only valid to servers\n");
        return 0;
    }

    if(bytes_received == -1)
    {
        perror("Receiving failed\n");
        return 0;
    }

    if(bytes_received == 0)
    {
        perror("Connection closed from client's (sender's) side\n");
        return 2;
    }

    if(recv_pack->packet_flags.SYN)
    {
        if(((recv_pack->packet_flags.ACK | recv_pack->packet_flags.FIN) | recv_pack->packet_flags.REACK) != 0)
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
        printf("Sending SYN-ACK error\n");
        return 0;       
    }
    printf("Not a valid packet for connection establishment\n");
    return 0;
}

// 0 - problem
// 4 - sending FIN-ACK
// bytes_sent - success
int rudp_send(p_RUDP_Sock sock, p_rudp_pack pack)
{
    int bytes_sent = sendto(sock->socket_fd, pack, sizeof(rudp_pack), 0, (struct sockaddr *)&sock->destination_addr, sizeof(struct sockaddr));
    
    if(bytes_sent == -1)
    {
        perror("Send error\n");
        return 0;
    }

    rudp_pack ack_pack;
    memset(&ack_pack, 0, sizeof(rudp_pack));
    
    int bytes_received = recvfrom(sock->socket_fd, &ack_pack, sizeof(rudp_pack), 0, NULL, 0);
    int timeout = 0;

    if(bytes_received == -1)
    {
        if(errno == EWOULDBLOCK || errno == EAGAIN)
        {
            return resend(sock, pack);
        }
        else
        {
            perror("Receive error\n");
            return 0;
        }
    }

    else if(bytes_received == 0)
    {
        perror("Connection closed from other side\n");
        return 0;
    }
    
    else
    {
        if(ack_pack.packet_flags.ACK)
        {
            if((((ack_pack.packet_flags.SYN | ack_pack.packet_flags.FIN) | ack_pack.packet_flags.DATA) | ack_pack.packet_flags.REACK) == 0)
            {
                if(ack_pack.sequence == pack->sequence + pack->length) // Checking if the sequence is right 
                {
                    if(calculate_checksum(&ack_pack, sizeof(rudp_pack)) == ack_pack.checksum) // Checking if the checksum is right
                    {
                        return bytes_sent;
                    }
                    printf("Checksum error\n");
                    return 0;
                }
            }
        }
        if(ack_pack.packet_flags.FIN)
        {
            if((((ack_pack.packet_flags.SYN | ack_pack.packet_flags.ACK) | ack_pack.packet_flags.DATA) | ack_pack.packet_flags.REACK) == 0)
            {
                while(bytes_sent = send_FIN_ACK(sock, ack_pack.sequence + ack_pack.length))
                {
                    if(bytes_sent >= 0) // Means that we sent a FIN-ACK or that the connection is already closed
                    {
                        return 4;
                    }
                }
            }
        }
        return resend(sock, pack);
    }
}

// 0 - problem
// 1 - nothing to bo done
// 2 - resending SYN-ACK
// 3 - resending ACK
// 4 - sending FIN-ACK
// bytes_received - success
int rudp_recv(p_RUDP_Sock sock, p_rudp_pack pack, p_rudp_pack prev_pack)
{
    int bytes_received = recvfrom(sock->socket_fd, pack, sizeof(rudp_pack), 0, NULL, 0);
    
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

    if(pack->packet_flags.SYN) // There was a problem in the connection establishment
    {
        if(send_SYN_ACK(sock, pack->length))
        {
            printf("Resending SYN-ACK\n");
            return 2;
        }

        printf("Error in resending SYN-ACK\n");
        return 0;      
    }

    if(pack->packet_flags.DATA) // Checking if it is a data packet
    {
        if((pack->packet_flags.SYN | (pack->packet_flags.ACK | pack->packet_flags.FIN)) == 0)
        {
            if(pack->sequence == prev_pack->sequence + prev_pack->length || !prev_pack->packet_flags.DATA) // Checking if the sequence is right 
            {
                if(calculate_checksum(pack, sizeof(rudp_pack)) == pack->checksum) // Checking if the checksum is right
                {
                    if(send_ACK(sock, pack->sequence + pack->length))
                    {
                        copy_packet(prev_pack, pack); // The prev is now equals to the current packet 
                        return bytes_received;
                    }
                    printf("ACK error\n");
                    return 0;
                }
                printf("Checksum error\n");
                return 0;
            }
            else if(pack->sequence == prev_pack->sequence)
            {
                if(pack->checksum == prev_pack->checksum)
                {
                    copy_packet(prev_pack, pack);

                    if(send_ACK(sock, pack->sequence + pack->length))
                    {
                        prev_pack->packet_flags.REACK = 1; // Now we know that the current packet is a packet that we resend an ACK on
                        return 3;
                    }
                    printf("ACK error\n");
                    return 0;                    
                }
            }
            
        }
    }

    if(pack->packet_flags.FIN) // Checking if it is a FIN packet
    {
        if((pack->packet_flags.SYN | (pack->packet_flags.ACK | pack->packet_flags.DATA)) == 0)
        {
            int bytes_sent;
            while(bytes_sent = send_FIN_ACK(sock, pack->sequence + pack->length))
            {
                if(bytes_sent >= 0) // Means that we sent a FIN-ACK or that the connection is already closed
                {
                    return 4;
                }
            }
        }
    }
    return 1;
}