#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <resolv.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>

#include <arpa/inet.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>

struct _rudphdr;
typedef struct _rudphdr rudphdr;

// Create RUDP socket
int rudp_socket();

// Send after connection establishment
int rudp_send(int socket);

// Receiving data
int rudp_recv(int socket);

// Close socket
int rudp_close(int socket);