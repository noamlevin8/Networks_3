#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/udp.h>
#include <time.h>

#define BUFFER_SIZE 1024


typedef struct _flags
{
    unsigned short int ACK :1;
    unsigned short int SYN :1;
    unsigned short int FIN :1;
    unsigned short int DATA :1;
    unsigned short int REACK :1;
}flags, *p_flags;


typedef struct _rudp_packet
{
    unsigned short int length;
    unsigned short int checksum;
    struct _flags packet_flags;
    unsigned int sequence;
    char data[BUFFER_SIZE];      
}rudp_pack, *p_rudp_pack;


typedef struct _rudp_socket
{
    struct sockaddr_in destination_addr;
    int socket_fd;
    int Server; // 0 - if client, 1 - if server
    int Connection; // 0 - if disconnected, 1 - if connected
}RUDP_Sock, *p_RUDP_Sock;


// Create RUDP socket
p_RUDP_Sock rudp_socket(unsigned short int listen_port, int if_server);


// Client asking server to connect
int rudp_connect(p_RUDP_Sock sock, const char* dest_ip, unsigned short int dest_port);


// Server accepts connection from client
int rudp_accept(p_RUDP_Sock sock);


// Send packet after connection establishment
int rudp_send(p_RUDP_Sock sock, p_rudp_pack pack);


// Receiving from established connection
int rudp_recv(p_RUDP_Sock sock, p_rudp_pack pack, p_rudp_pack prev_pack); 


// Disconnecting from an active connection
int rudp_disconnect(p_RUDP_Sock sock, int seq); 


// Close the socket and free all the allocated memory
int rudp_close(p_RUDP_Sock sock); 

// 
int create_socket();


// Create empty packet
p_rudp_pack create_packet();


// Creating a data pocket 
void data_packet(p_rudp_pack pack, int seq, char* data);


// Creating an acknowledgment packet 
void ACK_packet(p_rudp_pack pack, int seq);


// Creating a FIN packet  
void FIN_packet(p_rudp_pack pack, int seq);


// Copying one packet to another
void copy_packet(p_rudp_pack pack_1, p_rudp_pack pack_2);


// Starting a handshake between the client and the server.
// 0 - fail, 1 - success
int handshake(p_RUDP_Sock sock);


// Sanding a packet again when an ack on the pocket didn't arrive
int packet_resend(p_RUDP_Sock sock, p_rudp_pack pack);


// Sending SYN-ACK
int send_SYN_ACK(p_RUDP_Sock sock, int seq);


// Sending ACK
int send_ACK(p_RUDP_Sock sock, int seq);


// Sending FIN-ACK
int send_FIN_ACK(p_RUDP_Sock sock, int seq);


// Sending FIN
int send_FIN(p_RUDP_Sock sock, p_rudp_pack pack);


// Calculating the checksum of the packet
unsigned short int calculate_checksum(void* data, unsigned int bytes);