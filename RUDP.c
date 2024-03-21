#include "RUDP_API.h"

#define PACKETSIZE 64

struct _rudp_hdr
{
    struct icmphdr hdr;    
    u_int16_t dstport;
    u_int16_t srcport;
    u_int16_t checksum;
    u_int16_t hdrlen;
    u_int16_t flags;
};


struct packet
{
	rudphdr hdr;
	char msg[PACKETSIZE-sizeof(rudphdr)];
};

int rudp_socket()
{
    int rudp_socket = socket(AF_PACKET, SOCK_RAW, IPPROTO_ICMP);
}