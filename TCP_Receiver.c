#include "TCP_Receiver.h"

#define BUFFER_SIZE 1024

// To insert the given parameters to our variables
void get_info(int argc, char* argv[], int* port, char** algo)
{
        *port = atoi(argv[2]);
        *algo = argv[4];
}

int main(int argc, char* argv[]) 
{
    int Receiver_Port;
    char* CC_Algo;
    char* str = malloc(34);
    strcpy(str, "-       * Statistics *        -\n");
    int count_runs = 0;
    double sum_times = 0;
    double sum_bandwidth = 0;
    struct timeval time;
    double bandwidth = 0;
    int opt = 1;

    get_info(argc, argv, &Receiver_Port, &CC_Algo);

    // Creating the socket
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

    // Set the socket option to reuse the address
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        perror("Setting reuse failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("Starting Receiver...\n");

    struct sockaddr_in ReceiverAddress;
    memset(&ReceiverAddress, 0, sizeof(ReceiverAddress));

    ReceiverAddress.sin_family = AF_INET; // IPV4
    ReceiverAddress.sin_addr.s_addr = INADDR_ANY; // Random address
    ReceiverAddress.sin_port = htons(Receiver_Port); //Port

    // Bind the socket to the port with any IP at this port
    if (bind(sock, (struct sockaddr *)&ReceiverAddress, sizeof(ReceiverAddress)) == -1) {
        printf("Bind failed with error code : %d", errno);
        // close the socket
        close(sock);
        return -1;
    }

    // Listen with queue size of 1
    if (listen(sock, 1) == -1) {
        printf("listen() failed with error code : %d", errno);
        // close the socket
        close(sock);
        return -1;
    }
    printf("Waiting for TCP connection...\n");

    struct sockaddr_in SenderAddress;  
    socklen_t SenderAddressLen = sizeof(SenderAddress);
    memset(&SenderAddress, 0, sizeof(SenderAddress));

    // Accept an incoming connection
    int sender_socket = accept(sock, (struct sockaddr *)&SenderAddress, &SenderAddressLen);
    if (sender_socket == -1) {
        printf("listen failed with error code : %d", errno);
        // close the sockets
        close(sender_socket);
        return -1;
    }
    printf("Sender connected, beginning to receive file...\n");
    
    int bytesReceived, TotalBytesReceived, count_buffer;
    double time_last, time_cur, total_time;

    while (1)
    {  
        TotalBytesReceived = 0;
        bytesReceived = 0;
        count_buffer = 0;
        char buffer[BUFFER_SIZE] = {0};

        time_last = 0;
        total_time = 0;

        while (TotalBytesReceived < 2000000)
        {   
            bytesReceived = recv(sender_socket, buffer, BUFFER_SIZE, 0);
            TotalBytesReceived += bytesReceived;

            gettimeofday(&time, NULL);
            time_cur = (time.tv_sec * 1000.0) + (time.tv_usec / 1000.0);
                        
            if (bytesReceived == 0 || bytesReceived == -1)
            {
                break;
            }

            count_buffer++;
            
            if (count_buffer != 1)
            {
                total_time += time_cur - time_last;
            }
            
            time_last = time_cur;
        }

        if (bytesReceived == 0 || bytesReceived == -1) 
        {
            break;
        } 
        else 
        {            
            printf("File transfer completed.\n");

            bandwidth = (TotalBytesReceived / 1024.0) / (total_time);
            sum_bandwidth += bandwidth;
            sum_times += total_time;
            count_runs++;

            char* s = (char*)malloc(100);
            sprintf(s, "- Run #%d Data: Time =%f ms; Speed =%f MB/s;\n", count_runs, total_time, bandwidth);
            str = (char*)realloc(str, strlen(str)+strlen(s)+1);
            strcat(str, s);
            free(s);
        }
        
        printf("Waiting for Sender response...\n");
        int n = read(sender_socket, buffer, BUFFER_SIZE);
        if(n <= 0 || strncmp(buffer,"n",1) == 0)
            break;
    }

    printf("Sender sent exit message.\n");

    printf("-----------------------------\n");
    printf("%s\n\n", str);
    printf("- Avrage time: %f ms\n", sum_times/count_runs);
    printf("- Avrage bandwidth: %f MB/s\n", sum_bandwidth/count_runs);
    printf("-----------------------------\n");

    close(sock);
    printf("Receiver end.\n");
    free(str);
    return 0;
}