#include "RUDP_API.h"


// NULL - problem
// sock - success
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
        free(sock);
        printf("Problem creating socket\n");
        return NULL;
    }

    sock->socket_fd = sock_fd;
    sock->Server = if_server;
    sock->Connection = 0;

    memset(&sock->destination_addr, 0, sizeof(struct sockaddr_in));

    return sock;
}

// 0 - problem
// 1 - success
int rudp_connect(p_RUDP_Sock sock, const char* dest_ip, unsigned short int dest_port)
{
    if (sock == NULL)
    {
        printf("RUDP Socket is null\n");
        return 0;
    }
    
    if(sock->Server || sock->Connection) // Connect is only a client function
    {
        printf("RUDP Socket is a server or is already connected\n");
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
    if(sock == NULL)
    {
        printf("RUDP Socket is null\n");
        return 0;
    }

    p_rudp_pack recv_pack = create_packet();

    if(recv_pack == NULL)
    {
        perror("Problem creating recv packet\n");
        return 0;
    }

    memset(recv_pack, 0, sizeof(rudp_pack));
    socklen_t dest_addr_len = sizeof(struct sockaddr);

    int bytes_received;
    bytes_received = recvfrom(sock->socket_fd, recv_pack, sizeof(rudp_pack), 0, (struct sockaddr *)&sock->destination_addr, &dest_addr_len);

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

        unsigned short int check = recv_pack->checksum;
        recv_pack->checksum = 0;

        if(calculate_checksum(recv_pack, sizeof(rudp_pack)) != check)
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
// 2 - sending FIN-ACK
// packet resend - timeout or need to resend a packet:
// 3 - received FIN
// 4 - over max tries
// bytes_sent - success
int rudp_send(p_RUDP_Sock sock, p_rudp_pack pack)
{
    if (sock == NULL)
    {
        printf("RUDP Socket is null\n");
        return 0;
    }

    int bytes_sent;
    bytes_sent = sendto(sock->socket_fd, pack, sizeof(rudp_pack), 0, (struct sockaddr *)&sock->destination_addr, sizeof(struct sockaddr));
    
    if(bytes_sent <= 0)
    {
        perror("Send error\n");
        return 0;
    }

    rudp_pack ack_pack;
    memset(&ack_pack, 0, sizeof(rudp_pack));
    
    int bytes_received;
    bytes_received = recvfrom(sock->socket_fd, &ack_pack, sizeof(rudp_pack), 0, NULL, 0);

    if(bytes_received == -1)
    {
        if(errno == EWOULDBLOCK || errno == EAGAIN)
        {
            return packet_resend(sock, pack);
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
                    unsigned short int check = ack_pack.checksum;
                    ack_pack.checksum = 0;

                    if(calculate_checksum(&ack_pack, sizeof(rudp_pack)) == check) // Checking if the checksum is right
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
                int i;
                for(i = 0; i < MAX_TRIES; i++)
                {
                    bytes_sent = send_FIN_ACK(sock, ack_pack.sequence + ack_pack.length);

                    if(bytes_sent >= 0) // Means that we sent a FIN-ACK or that the connection is already closed
                    {
                        return 2;
                    }
                }
            }
        }
        return packet_resend(sock, pack);
    }
}

// // 0 - problem
// // -6 - nothing to be done
// // -2 - resending SYN-ACK
// // -3 - resending ACK
// // -4 - sending FIN-ACK
// // -5 - too many tries to send FIN-ACK
// // bytes_received - success
// int rudp_recv(p_RUDP_Sock sock, p_rudp_pack pack, p_rudp_pack prev_pack)
// {
//     if (sock == NULL || pack == NULL)
//     {
//         printf("RUDP Socket or RUDP Packet is null\n");
//         return 0;
//     }
    
//     int bytes_received;
//     bytes_received = recvfrom(sock->socket_fd, pack, sizeof(rudp_pack), 0, NULL, 0);
    
//     if(bytes_received == -1)
//     {
//         perror("Receiving failed\n");
//         return 0;
//     }

//     if(bytes_received == 0)
//     {
//         perror("Connection closed from sender's side\n");
//         return 0;
//     }

//     if(pack->packet_flags.SYN) // There was a problem in the connection establishment
//     {
//         if(send_SYN_ACK(sock, pack->length))
//         {
//             printf("Resending SYN-ACK\n");
//             return -2;
//         }

//         printf("Error in resending SYN-ACK\n");
//         return 0;      
//     }

//     if(pack->packet_flags.DATA) // Checking if it is a data packet
//     {
//         if((pack->packet_flags.SYN | (pack->packet_flags.ACK | pack->packet_flags.FIN)) == 0)
//         {
//             if (prev_pack == NULL)
//             {
//                 printf("Prev packet is null\n");
//                 return 0;
//             }
            
//             if(pack->sequence == prev_pack->sequence + prev_pack->length || !prev_pack->packet_flags.DATA) // Checking if the sequence is right 
//             {
//                 unsigned short int check = pack->checksum;
//                 pack->checksum = 0;
//                 prev_pack->packet_flags.REACK = 0;

//                 if(calculate_checksum(pack, sizeof(rudp_pack)) == check) // Checking if the checksum is right
//                 {
//                     pack->checksum = check;
//                     copy_packet(prev_pack, pack); // The prev is now equals to the current packet
//                     pack->sequence += pack->length;

//                     if(send_ACK(sock, pack->sequence))
//                     {
//                         return bytes_received;
//                     }
//                     printf("ACK error\n");
//                     return 0;
//                 }
//                 printf("Checksum error\n");
//                 return 0;
//             }
//             else if(pack->sequence == prev_pack->sequence)
//             {
//                 if(pack->checksum == prev_pack->checksum)
//                 {
//                     if (prev_pack->packet_flags.DATA == 1)
//                     {                   
//                         copy_packet(prev_pack, pack);
//                         pack->sequence = prev_pack->sequence + prev_pack->length;

//                         if(send_ACK(sock, pack->sequence))
//                         {
//                             prev_pack->packet_flags.REACK = 1; // Now we know that the current packet is a packet that we resent an ACK on
//                             return -3;
//                         }
//                         printf("ACK error\n");
//                         return 0;  
//                     }                  
//                 }
//             } 
//         }
//     }

//     if(pack->packet_flags.FIN) // Checking if it is a FIN packet
//     {
//         if((pack->packet_flags.SYN | (pack->packet_flags.ACK | pack->packet_flags.DATA)) == 0)
//         {
//             int bytes_sent, i;
//             for(i = 0; i < MAX_TRIES; i++)
//             {
//                 bytes_sent = send_FIN_ACK(sock, pack->sequence + pack->length);
                
//                 if(bytes_sent >= 0) // Means that we sent a FIN-ACK or that the connection is already closed
//                 {
//                     return -4;
//                 }
//             }
//             return -5;
//         }
//     }
//     return -6;
// }


int rudp_recv(p_RUDP_Sock sockfd, p_rudp_pack p, p_rudp_pack p_prev)
{
    int bytes_received = recvfrom(sockfd->socket_fd, p, sizeof(rudp_pack), 0,
     NULL, 0);
    // If the message receiving failed, print an error message and return 1.
    // If the amount of received bytes is 0, the client sent an empty message, so we ignore it.
    if (bytes_received < 0)
    {
        perror("recvfrom(2)");
        return -1;
    }
    else if(bytes_received == 0)
    {
        return 0;
    }
    // if we got SYN after accpet that means our synAck packet didnt arrive sender
    if(p->packet_flags.SYN) 
    {
        if(send_SYN_ACK(sockfd, p->length))
        {
            return -4;
        }
        else
        {
            return 0;
        }
    }
    // Check if thats a data packet
    if(p->packet_flags.DATA && (p->packet_flags.ACK | p->packet_flags.FIN) | p->packet_flags.SYN == 0)
    {
        // check if sequence is right
        if(p->sequence == p_prev->sequence + p_prev->length || p_prev->packet_flags.DATA == 0) 
        {   
            p_prev->packet_flags.REACK = 0;
            unsigned short int check = p->checksum;
            p->checksum = 0;
            // check if checksum is right and the packet isnt corrupted
            if(calculate_checksum(p, sizeof(rudp_pack)) == check)
            {
                p->checksum = check;
                copy_packet(p_prev, p);
                p->sequence += p->length;
                if(send_ACK(sockfd, p->sequence))
                {
                    return bytes_received; 
                }
            }
        }
        // if the seq is the same as the prev packet
        else if(p->sequence == p_prev->sequence)
        {
            // check if the checksum of the prev packet is the received packet
            if(p->checksum == p_prev->checksum)
            {
                if(p_prev->packet_flags.DATA == 1) 
                {
                    copy_packet(p, p_prev);
                    p->sequence = p_prev->sequence + p_prev->length;
                    if(send_ACK(sockfd, p->sequence))
                    {
                        if(p_prev->packet_flags.REACK == 1)
                        {
                            return -2;
                        }
                        p_prev->packet_flags.REACK = 1;
                        return -3;
                    }
                }
            }
        }
    }
    // If received FIN packet send finack
    if(p->packet_flags.FIN && ((p->packet_flags.ACK | p->packet_flags.SYN) | p->packet_flags.DATA) | p->packet_flags.REACK == 0)
    {
       for(int i = 0; i<MAX_TRIES; i++)
            {
                // when the sender receive the packet he will close the socket
                // so the next sendto function will return 0 and thus the loop
                // will stop
                int bytes_sent = send_FIN_ACK(sockfd, p->sequence + p->length); 
                if(bytes_sent >= 0)
                {
                    return 0;
                }  
            }
    }
    return -2;
}


// 0 - problem
// 1 - success
int rudp_disconnect(p_RUDP_Sock sock, int seq)
{
    if (sock == NULL)
    {
        printf("RUDP Socket is null\n");
        return 0;
    }

    p_rudp_pack FIN_pack = create_packet();

    if(FIN_pack == NULL)
    {
        perror("Problem creating FIN packet\n");
        return 0;
    }

    FIN_packet(FIN_pack, seq);

    int bytes_sent;
    if(bytes_sent = send_FIN(sock, FIN_pack))
    {
        return 1;
    }
    return 0;
}

// 0 - problem
// 1 - success
void rudp_close(p_RUDP_Sock sock)
{
    if(sock == NULL)
    {
        printf("RUDP Socket is already null\n");
        return;
    }

    close(sock->socket_fd);
    free(sock);
    printf("Socket is closed\n");
}

// 0 - problem
// sock - success
int create_socket()
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    int opt = 1;

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) // Set the socket option to reuse the address
    {
        perror("setsockopt error\n");
        return 0;
    }
    return sock;
}

// NULL - problem
// pack - success
p_rudp_pack create_packet()
{
    p_rudp_pack pack = (p_rudp_pack)malloc(sizeof(rudp_pack));
    if(pack == NULL)
    {
        printf("Allocation problem\n");
        return NULL;
    }

    memset(pack, 0, sizeof(rudp_pack));
    return pack;
}


void data_packet(p_rudp_pack pack, int seq, char* data)
{
    if(pack == NULL)
    {
        printf("RUDP Packet is null\n");
        return;
    }

    memset(pack, 0, sizeof(rudp_pack));
    pack->length = BUFFER_SIZE;
    pack->packet_flags.DATA = 1;
    pack->sequence = seq;
    strncpy(pack->data, data, BUFFER_SIZE);
    pack->checksum = calculate_checksum(pack, sizeof(rudp_pack));
}


void ACK_packet(p_rudp_pack pack, int seq)
{
    if(pack == NULL)
    {
        printf("RUDP Packet is null\n");
        return;
    }

    memset(pack, 0, sizeof(rudp_pack));
    pack->packet_flags.ACK = 1;
    pack->sequence = seq;
    pack->checksum = calculate_checksum(pack, sizeof(rudp_pack));
}


void FIN_packet(p_rudp_pack pack, int seq)
{
    if(pack == NULL)
    {
        printf("RUDP Packet is null\n");
        return;
    }

    memset(pack, 0, sizeof(rudp_pack));
    pack->packet_flags.FIN = 1;
    pack->sequence = seq;
    pack->checksum = calculate_checksum(pack, sizeof(rudp_pack));
}


void copy_packet(p_rudp_pack pack_1, p_rudp_pack pack_2)
{
    if(pack_1 == NULL || pack_2 == NULL)
    {
        printf("RUDP Packet 1 or 2 is null\n");
        return;
    }

    pack_1->length = pack_2->length;
    pack_1->checksum = pack_2->checksum;
    pack_1->packet_flags.DATA = pack_2->packet_flags.DATA;
    pack_1->sequence = pack_2->sequence;
    strncpy(pack_1->data, pack_2->data, pack_2->length);
}

// 0 - problem
// 1 - success
// bytes_sent - successful resend SYN
int handshake(p_RUDP_Sock sock)
{
    // Sending the SYN packet
    rudp_pack recv_pack;
    p_rudp_pack SYN_pack = create_packet();

    if(SYN_pack == NULL)
    {
        perror("Problem creating SYN packet\n");
        return 0;
    }

    memset(&recv_pack, 0, sizeof(rudp_pack));
    memset(SYN_pack, 0, sizeof(rudp_pack));

    SYN_pack->packet_flags.SYN = 1;
    SYN_pack->checksum = calculate_checksum(SYN_pack, sizeof(rudp_pack));

    int bytes_sent;
    bytes_sent = sendto(sock->socket_fd, SYN_pack, sizeof(rudp_pack), 0, (struct sockaddr *)&sock->destination_addr, sizeof(struct sockaddr));


    // Checking if the message was sent or if no data was sent
    if (bytes_sent <= 0)
    {
        perror("Send error\n");
        free(SYN_pack);
        return 0;
    }

    printf("Sent a handshake request to the Receiver\n");
    printf("Waiting for the Receiver response\n");


    int bytes_received, timeout = 0;
    bytes_received = recvfrom(sock->socket_fd, &recv_pack, sizeof(rudp_pack), 0, NULL, 0);

    if(bytes_received == -1)
    {
        if(errno == EWOULDBLOCK || errno == EAGAIN)
        {
            timeout = 1;
        }
        else
        {
            perror("Receive error\n");
            free(SYN_pack);
            return 0;
        }
    }

    else if(bytes_received == 0)
    {
        perror("Socket is closed from other side\n");
        free(SYN_pack);
        return 0;
    }

    else
    {
        if(recv_pack.packet_flags.SYN & recv_pack.packet_flags.ACK)
        {
            if(((recv_pack.packet_flags.FIN | recv_pack.packet_flags.DATA) | recv_pack.packet_flags.REACK) == 0)
            {
                if(recv_pack.sequence == 0)
                {
                    unsigned short int check = recv_pack.checksum;
                    recv_pack.checksum = 0;

                    if(calculate_checksum(&recv_pack, sizeof(rudp_pack)) == check)
                    {
                        printf("Connection established\n");
                        free(SYN_pack);
                        return 1;
                    }
                    printf("Checksum error\n");
                    free(SYN_pack);
                    return 0;
                }
            }
        }
    }

    if(timeout)
    {
        bytes_sent = packet_resend(sock, SYN_pack);
        //
        if(bytes_sent == -1)
        {
            printf("Handshake incompleted from server's (Receiver's) side\n");
            free(SYN_pack);
            return 0;
        }

        free(SYN_pack);
        return bytes_sent;
    }
}

// 0 - problem
// 3 - received FIN
// 4 - over max tries
// bytes_sent - success
int packet_resend(p_RUDP_Sock sock, p_rudp_pack pack)
{
    rudp_pack recv_pack;
    memset(&recv_pack, 0, sizeof(rudp_pack));

    int i, timeout = 0, bytes_sent, bytes_received;
    for (i = 0; i < MAX_TRIES; i++)
    {
        bytes_sent = sendto(sock->socket_fd, pack, sizeof(rudp_pack), 0, (struct sockaddr *)&sock->destination_addr, sizeof(struct sockaddr));

        // Checking if the message was sent or if no data was sent
        if (bytes_sent <= 0)
        {
            perror("Send error\n");
            return 0;
        }

        bytes_received = recvfrom(sock->socket_fd, &recv_pack, sizeof(rudp_pack), 0, NULL, 0);

        if(bytes_received == -1)
        {
            if(errno == EWOULDBLOCK || errno == EAGAIN)
            {
                timeout = 1;
            }
            else
            {
                perror("Receive error\n");
                return 0;
            }
        }

        else if(bytes_received == 0)
        {
            perror("Socket is closed from other side\n");
            return 0;
        }

        if(!timeout)
        {
            if(recv_pack.packet_flags.ACK)
            {
                if((((recv_pack.packet_flags.SYN | recv_pack.packet_flags.FIN) | recv_pack.packet_flags.DATA) | recv_pack.packet_flags.REACK) == 0)
                {
                    if(recv_pack.sequence == pack->sequence + pack->length)
                    {
                        unsigned short int check = recv_pack.checksum;
                        recv_pack.checksum = 0;

                        if(calculate_checksum(&recv_pack, sizeof(rudp_pack)) == check)
                        {
                            return bytes_sent;
                        }
                        printf("Checksum error\n");
                        return 0;
                    }
                }
            }

            else if(recv_pack.packet_flags.ACK & recv_pack.packet_flags.SYN)
            {
                if(((recv_pack.packet_flags.FIN | recv_pack.packet_flags.DATA) | recv_pack.packet_flags.REACK) == 0)
                {
                    if(recv_pack.sequence == pack->sequence + pack->length)
                    {
                        unsigned short int check = recv_pack.checksum;
                        recv_pack.checksum = 0;

                        if(calculate_checksum(&recv_pack, sizeof(rudp_pack)) == check)
                        {
                            return bytes_sent;
                        }
                        printf("Checksum error\n");
                        return 0;
                    }
                }
            }

            else if(recv_pack.packet_flags.FIN)
            {
                if((((recv_pack.packet_flags.SYN | recv_pack.packet_flags.ACK) | recv_pack.packet_flags.DATA) | recv_pack.packet_flags.REACK) == 0)
                {
                    return 3;
                }
            }

            else if(recv_pack.packet_flags.FIN & recv_pack.packet_flags.ACK)
            {
                if(((recv_pack.packet_flags.SYN | recv_pack.packet_flags.DATA) | recv_pack.packet_flags.REACK) == 0)
                {
                    return bytes_sent;
                }
            }
        }
    }
    return 4;
}

// 0 - problem
// 1 - success
int send_SYN_ACK(p_RUDP_Sock sock, int seq)
{
    p_rudp_pack SYN_ACK_pack = create_packet();

    if(SYN_ACK_pack == NULL)
    {
        perror("Problem creating SYN-ACK packet\n");
        return 0;
    }

    ACK_packet(SYN_ACK_pack, seq);

    SYN_ACK_pack->packet_flags.SYN = 1;
    SYN_ACK_pack->checksum = 0;

    SYN_ACK_pack->checksum = calculate_checksum(SYN_ACK_pack, sizeof(rudp_pack));

    int bytes_sent;
    bytes_sent = sendto(sock->socket_fd, SYN_ACK_pack, sizeof(rudp_pack), 0, (struct sockaddr *)&sock->destination_addr, sizeof(struct sockaddr));

    if(bytes_sent == -1)
    {
        perror("Send error\n");
        free(SYN_ACK_pack);
        return 0;
    }

    free(SYN_ACK_pack);
    return 1;
}

// 0 - problem
// 1 - success
int send_ACK(p_RUDP_Sock sock, int seq)
{
    p_rudp_pack ACK_pack = create_packet();

    if(ACK_pack == NULL)
    {
        perror("Problem creating ACK packet\n");
        return 0;
    }

    ACK_packet(ACK_pack, seq);

    int bytes_sent;
    bytes_sent = sendto(sock->socket_fd, ACK_pack, sizeof(rudp_pack), 0, (struct sockaddr *)&sock->destination_addr, sizeof(struct sockaddr));

    if(bytes_sent == -1)
    {
        perror("Send error\n");
        free(ACK_pack);
        return 0;
    }

    free(ACK_pack);
    return 1;
}

// 0 - problem
// 1 - success
int send_FIN_ACK(p_RUDP_Sock sock, int seq)
{
    p_rudp_pack FIN_ACK_pack = create_packet();

    if(FIN_ACK_pack == NULL)
    {
        perror("Problem creating FIN-ACK packet\n");
        return 0;
    }

    ACK_packet(FIN_ACK_pack, seq);

    FIN_ACK_pack->packet_flags.FIN = 1;
    FIN_ACK_pack->checksum = 0;

    FIN_ACK_pack->checksum = calculate_checksum(FIN_ACK_pack, sizeof(rudp_pack));

    int bytes_sent;
    bytes_sent = sendto(sock->socket_fd, FIN_ACK_pack, sizeof(rudp_pack), 0, (struct sockaddr *)&sock->destination_addr, sizeof(struct sockaddr));

    if(bytes_sent == -1)
    {
        perror("Send error\n");
        free(FIN_ACK_pack);
        return 0;
    }

    free(FIN_ACK_pack);
    return 1;
}

// 0 - problem
// 1 - success
int send_FIN(p_RUDP_Sock sock, p_rudp_pack pack)
{
    int bytes_sent;
    bytes_sent = packet_resend(sock, pack);

    if(bytes_sent <= 0)
    {
        return 0;
    }

    return 1;
}


unsigned short int calculate_checksum(void* data, unsigned int bytes)
{
    unsigned short int *data_pointer = (unsigned short int *)data;
    unsigned int total_sum = 0;
    while(bytes > 1)
    {
        total_sum += *data_pointer++;
        bytes -=2;
    }
    if(bytes > 0)
    {
        total_sum += *((unsigned char *)data_pointer);
    }
    while(total_sum >> 16)
    {
        total_sum = (total_sum & 0xFFFF) + (total_sum >> 16);
    }
    return (~((unsigned short int)total_sum));
}