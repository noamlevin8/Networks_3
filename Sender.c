#include <stdio.h>

// Linux and other UNIXes
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SENDER_PORT 5060  // The port that the server listens
#define BUFFER_SIZE 1024
#define FILE_SIZE 2000000
#define FILE_NAME "random_file"

/*
* @brief A random data generator function based on srand() and rand().
* @param size The size of the data to generate (up to 2^32 bytes).
* @return A pointer to the buffer.
*/
// void util_generate_random_data(const char* filename, unsigned int size) {
//     FILE* file = fopen(filename, "wb");
//     if (file == NULL)
//         return NULL;
    
//     // Argument check.
//     if (size == 0)
//         return NULL;
//     // Randomize the seed of the random number generator.
//     srand((unsigned int)time(NULL));
//     for (unsigned int i = 0; i < size; i++){
//         char randomByte = ((unsigned int)rand() % 256);
//         fwrite(&randomByte, 1, 1, file);
//     }
//     fclose(file);
// }

/*
* @brief A random data generator function based on srand() and rand().
* @param size The size of the data to generate (up to 2^32 bytes).
* @return A pointer to the buffer.
*/
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

int main() {
    // signal(SIGPIPE, SIG_IGN);  // on linux to prevent crash on closing socket

    // Open the listening (server) socket
    int listeningSocket = -1;
    listeningSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // 0 means default protocol for stream sockets (Equivalently, IPPROTO_TCP)
    if (listeningSocket == -1) {
        printf("Could not create listening socket : %d", errno);
        return 1;
    }

    // Reuse the address if the server socket on was closed
    // and remains for 45 seconds in TIME-WAIT state till the final removal.
    //
    int enableReuse = 1;
    int ret = setsockopt(listeningSocket, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(int));
    if (ret < 0) {
        printf("setsockopt() failed with error code : %d", errno);
        return 1;
    }

    // "sockaddr_in" is the "derived" from sockaddr structure
    struct sockaddr_in senderAddress;
    memset(&senderAddress, 0, sizeof(senderAddress));

    senderAddress.sin_family = AF_INET;
    senderAddress.sin_addr.s_addr = INADDR_ANY; // any IP at this port (Address to accept any incoming messages)
    senderAddress.sin_port = htons(SENDER_PORT);  // network order (makes byte order consistent)

    // Bind the socket to the port with any IP at this port
    int bindResult = bind(listeningSocket, (struct sockaddr *)&senderAddress, sizeof(senderAddress));
    if (bindResult == -1) {
        printf("Bind failed with error code : %d", errno);
        // close the socket
        close(listeningSocket);
        return -1;
    }

    printf("Bind() success\n");

    // Make the socket listening; actually mother of all client sockets.
    // 500 is a Maximum size of queue connection requests
    // number of concurrent connections
    int listenResult = listen(listeningSocket, 3);
    if (listenResult == -1) {
        printf("listen() failed with error code : %d", errno);
        // close the socket
        close(listeningSocket);
        return -1;
    }

    // Accept and incoming connection
    printf("Waiting for incoming TCP-connections...\n");
    struct sockaddr_in receiverAddress;  //
    socklen_t receiverAddressLen = sizeof(receiverAddress);

    char* message;
    int messageLen;

    while (1) {
        memset(&receiverAddress, 0, sizeof(receiverAddress));
        receiverAddressLen = sizeof(receiverAddress);
        int receiverSocket = accept(listeningSocket, (struct sockaddr *)&receiverAddress, &receiverAddressLen);
        if (receiverSocket == -1) {
            printf("listen failed with error code : %d", errno);
            // close the sockets
            close(listeningSocket);
            return -1;
        }

        printf("A new client connection accepted\n");

        // // Receive a message from client
        // char buffer[BUFFER_SIZE];
        // memset(buffer, 0, BUFFER_SIZE);
        // int bytesReceived = recv(receiverSocket, buffer, BUFFER_SIZE, 0);
        // if (bytesReceived == -1) {
        //     printf("recv failed with error code : %d", errno);
        //     // close the sockets
        //     close(listeningSocket);
        //     close(receiverSocket);
        //     return -1;
        // }
        
        // printf("Received: %s", buffer);

        // Reply to 
        //util_generate_random_data(FILE_NAME, FILE_SIZE);
        message = util_generate_random_data(FILE_SIZE);
        messageLen = strlen(FILE_NAME) + 1;

        int bytesSent = send(receiverSocket, message, messageLen, 0);
        if (bytesSent == -1) {
            printf("send() failed with error code : %d", errno);
            close(listeningSocket);
            close(receiverSocket);
            return -1;
        } else if (bytesSent == 0) {
            printf("peer has closed the TCP connection prior to send().\n");
        } else if (bytesSent < messageLen) {
            printf("sent only %d bytes from the required %d.\n", messageLen, bytesSent);
        } else {
            printf("message was successfully sent.\n");
        }
    }

    close(listeningSocket);
    // Exit;
    return 0;
}