#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SENDER_PORT 5060
#define SENDER_IP_ADDRESS "127.0.0.1"
#define BUFFER_SIZE 1024

int main() {
    char* str = "";
    int count_runs = 0;
    float sum_times = 0;
    float sum_bandwidth = 0;

    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (sock == -1) {
        printf("Could not create socket : %d", errno);
        return -1;
    }

    printf("Starting Receiver...\n");

    // "sockaddr_in" is the "derived" from sockaddr structure
    struct sockaddr_in senderAddress;
    memset(&senderAddress, 0, sizeof(senderAddress));

    senderAddress.sin_family = AF_INET;
    senderAddress.sin_port = htons(SENDER_PORT);                                              // (5001 = 0x89 0x13) little endian => (0x13 0x89) network endian (big endian)
    int rval = inet_pton(AF_INET, (const char *)SENDER_IP_ADDRESS, &senderAddress.sin_addr);  // convert IPv4 and IPv6 addresses from text to binary form
    if (rval <= 0) {
        printf("inet_pton() failed");
        return -1;
    }

    printf("Waiting for TCP connection...\n");

    // Make a connection to the server with socket SendingSocket.
    int connectResult = connect(sock, (struct sockaddr *)&senderAddress, sizeof(senderAddress));
    if (connectResult == -1) {
        printf("connect() failed with error code : %d", errno);
        // cleanup the socket;
        close(sock);
        return -1;
    }

    printf("Sender connected, beginning to receive file...\n");

    // // Sends some data to server
    // char buffer[BUFFER_SIZE] = {'\0'};
    // char message[] = "Hello, from the Client\n";
    // int messageLen = strlen(message) + 1;
    
    // int bytesSent = send(sock, message, messageLen, 0);

    // if (bytesSent == -1) {
    //     printf("send() failed with error code : %d", errno);
    // } else if (bytesSent == 0) {
    //     printf("peer has closed the TCP connection prior to send().\n");
    // } else if (bytesSent < messageLen) {
    //     printf("sent only %d bytes from the required %d.\n", messageLen, bytesSent);
    // } else {
    //     printf("message was successfully sent.\n");
    // }
    
    char bufferReply[BUFFER_SIZE] = {'\0'};
    int bytesReceived;

    while (1)
    {  
        // Receive data from server
        bytesReceived = recv(sock, bufferReply, BUFFER_SIZE, 0);
        if (bytesReceived == -1) {
            printf("recv() failed with error code : %d", errno);
        } else if (bytesReceived == 0) {
            printf("peer has closed the TCP connection prior to recv().\n");
        } else {
            //printf("received %d bytes from server: %s\n", bytesReceived, bufferReply);
            printf("File transfer completed.\n");
            //sum_bandwidth += ;
            //sum_times += ;
            //count_runs++;
        }
        
        printf("Waiting for Sender response...\n");
    }

    printf("Sender sent exit message.\n");

    printf("-----------------------------");
    printf("%s\n\n", str);
    printf("Avrage time: %f\n", sum_times/count_runs);
    printf("Avrage bandwidth: %f\n", sum_bandwidth/count_runs);
    printf("-----------------------------");

    close(sock);
    printf("Receiver end.\n");
    return 0;
}