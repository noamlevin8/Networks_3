#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <sys.time.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/udp.h>
#include <time.h>

typedef struct _flags
{
    unsigned short int ACK :1;
    unsigned short int SYN :1;
    unsigned short int FIN :1;
}flags, *p_flags;

typedef struct _rudp_packet
{
    struct flags pocket_flags;
    unsigned int seq;
    unsigned short int checksum;    
    unsigned short int length;
    char data[BUFFER_SIZE];     
  
}rudp_pack, *p_rudp_pack;

typedef struct _rudp_socket{
    int socket_fd;
    int isServer;
    int isConnected;
    struct sockaddr_in dest_addr;
}RUDP_Sock, *p_RUDP_Sock;

// Create RUDP socket
int rudp_socket();

// Send after connection establishment
int rudp_send(int socket);

// Receiving data
int rudp_recv(int socket);

// Close socket
int rudp_close(int socket);