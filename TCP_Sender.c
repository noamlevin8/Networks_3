#include "TCP_Sender.h"

#define BUFFER_SIZE 1024
#define FILE_SIZE 2000000
#define FILE_NAME "random_file"

// To insert the given parameters to our variables
void get_info(int argc, char* argv[], char** ip, int* port, char** algo)
{
        *ip = argv[2];
        *port = atoi(argv[4]);
        *algo = argv[6];
}

// Generate random data
char *util_generate_random_data(unsigned int size) {
    char *buffer = NULL;

    // Argument check.
    if (size == 0)
        return NULL;

    buffer = (char *)calloc(size, sizeof(char));

    // Error checking.
    if (buffer == NULL)
        return NULL;

    // Randomize the seed of the random number generator.
    srand(time(NULL));
    for (unsigned int i = 0; i < size; i++)
        *(buffer + i) = ((unsigned int)rand() % 256);

    return buffer;
}

int main(int argc, char* argv[])
{
    int Receiver_Port;
    char* CC_Algo;
    char* Receiver_IP;
    int opt = 1;

    get_info(argc, argv, &Receiver_IP, &Receiver_Port, &CC_Algo);

    // Creating the socket
    int sender_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sender_socket == -1) {
        printf("Could not create socket : %d", errno);
        return -1;
    }

    // Setting TCP congestion control algorithm
    if (setsockopt(sender_socket, IPPROTO_TCP, TCP_CONGESTION, CC_Algo, strlen(CC_Algo) + 1) == -1) 
    {
        perror("Setting congestion control algorithm failed");
        close(sender_socket);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in senderAddress;
    memset(&senderAddress, 0, sizeof(senderAddress));

    senderAddress.sin_family = AF_INET; // IPV4
    senderAddress.sin_port = htons(Receiver_Port); // Port

    // Setting IP address in correct form
    if (inet_pton(AF_INET, Receiver_IP, &senderAddress.sin_addr) == -1)
    {
        perror("IP error");
        close(sender_socket);
        exit(EXIT_FAILURE);   
    }

    // Connect
    int connectResult = connect(sender_socket, (struct sockaddr *)&senderAddress, sizeof(senderAddress));
    if (connectResult == -1) {
        printf("connect() failed with error code : %d", errno);
        close(sender_socket);
        return -1;
    }
    
    int resend = 1;

    while (resend)
    {
        char* message = util_generate_random_data(FILE_SIZE);
        int messageLen = FILE_SIZE;

        int bytesSent = send(sender_socket, message, messageLen, 0);
        if (bytesSent == -1) 
        {
            printf("send() failed with error code : %d", errno);
            close(sender_socket);
            return -1;
        } 
        else if (bytesSent == 0) 
        {
            printf("peer has closed the TCP connection prior to send().\n");
        } 
        else if (bytesSent < messageLen) 
        {
            printf("sent only %d bytes from the required %d.\n", bytesSent, messageLen);
        } 
        else 
        {
            printf("message was successfully sent.\n");
        }

        free(message);

        printf("1 - to resend, 0 - to exit\n");
        scanf(" %d", &resend);

        char repete[2];

        if(resend)
        {
            strcpy(repete, "y");
        } 
        else
        {
            strcpy(repete, "n");
        }

        // Send a massage whether or not to expect another file
        bytesSent = send(sender_socket, repete, 2, 0);
        
        if (bytesSent == -1) 
        {
            printf("send() failed with error code : %d", errno);
            close(sender_socket);
            return -1;
        } 
        else if (bytesSent == 0) 
        {
            printf("peer has closed the TCP connection prior to send().\n");
        } 
        else if (bytesSent < 2) 
        {
            printf("sent only %d bytes from the required %d.\n", bytesSent, messageLen);
        }
    }
    close(sender_socket);
    // Exit;
    return 0;
}