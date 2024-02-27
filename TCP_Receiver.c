#include "TCP_Receiver.h"

#define BUFFER_SIZE 1024

void get_info(int argc, char* argv[], int* port, char** algo)
{
        *port = atoi(argv[2]);
        *algo = argv[4];
}

int main(int argc, char* argv[]) 
{
    int Receiver_Port;
    char* CC_Algo;
    char* str = "       * Statistics *        -\n";
    int count_runs = 0;
    double sum_times = 0;
    double sum_bandwidth = 0;
    struct timeval stop, start;
    double time = 0;
    double bandwidth = 0;

    get_info(argc, argv, &Receiver_Port, &CC_Algo);

    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (sock == -1) {
        printf("Could not create socket : %d", errno);
        return -1;
    }

    // Setting TCP congestion control algorithm
    if (setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, CC_Algo, strlen(CC_Algo) + 1) == -1) 
    {
        perror("Setting congestion control algorithm failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("Starting Receiver...\n");

    struct sockaddr_in ReceiverAddress;
    memset(&ReceiverAddress, 0, sizeof(ReceiverAddress));

    ReceiverAddress.sin_family = AF_INET;
    ReceiverAddress.sin_addr.s_addr = INADDR_ANY;
    ReceiverAddress.sin_port = htons(Receiver_Port);

    // Bind the socket to the port with any IP at this port
    if (bind(sock, (struct sockaddr *)&ReceiverAddress, sizeof(ReceiverAddress)) == -1) {
        printf("Bind failed with error code : %d", errno);
        // close the socket
        close(sock);
        return -1;
    }

    // Listen
    if (listen(sock, 1) == -1) {
        printf("listen() failed with error code : %d", errno);
        // close the socket
        close(sock);
        return -1;
    }
    printf("Waiting for TCP connection...\n");

    // Accept an incoming connection
    struct sockaddr_in SenderAddress;  
    socklen_t SenderAddressLen = sizeof(SenderAddress);
    memset(&SenderAddress, 0, sizeof(SenderAddress));
    SenderAddressLen = sizeof(SenderAddress);

    int sender_socket = accept(sock, (struct sockaddr *)&SenderAddress, &SenderAddressLen);
    if (sender_socket == -1) {
        printf("listen failed with error code : %d", errno);
        // close the sockets
        close(sender_socket);
        return -1;
    }
    printf("Sender connected, beginning to receive file...\n");
    gettimeofday(&start, NULL);
    
    char bufferReply[BUFFER_SIZE] = {'\0'};
    int bytesReceived;

    while (1)
    {  
        // Receive data from server
        bytesReceived = recv(sender_socket, bufferReply, BUFFER_SIZE, 0);
        if (bytesReceived == -1) {
            printf("recv() failed with error code : %d", errno);
        } else if (bytesReceived == 0) {
            printf("peer has closed the TCP connection prior to recv().\n");
        } else {
            //printf("received %d bytes from server: %s\n", bytesReceived, bufferReply);
            
            printf("File transfer completed.\n");

            gettimeofday(&stop, NULL);
            time = (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec;
            bandwidth = bytesReceived/time;

            sum_bandwidth += bandwidth;
            sum_times += time;
            count_runs++;

            char* s = "- Run #%d Data: Time=%f ms; Speed=%f MB/s;\n", count_runs, time, bandwidth;
            strcat(str, s);
        }
        
        printf("Waiting for Sender response...\n");
        gettimeofday(&start, NULL);
    }

    printf("Sender sent exit message.\n");

    printf("-----------------------------");
    printf("%s\n\n", str);
    printf("- Avrage time: %f ms\n", sum_times/count_runs);
    printf("- Avrage bandwidth: %f MB/s\n", sum_bandwidth/count_runs);
    printf("-----------------------------");

    close(sock);
    printf("Receiver end.\n");
    return 0;
}