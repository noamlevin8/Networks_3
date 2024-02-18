#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>

#define PORT "6000"
#define ADDR "127.0.0.1"
#define RECIVER 1

int main(){
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sock < 0)
    {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in sender_addr;
    memset(&sender_addr, 0, sizeof(sender_addr));
    sender_addr.sin_family = AF_INET;
    sender_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, ADDR, &sender_addr.sin_addr) < 0)
    {
        close(sock);
        perror("inet_pton");
        exit(1);
    }
    
    if (bind(sock, (struct sockaddr*)&sender_addr, sizeof(sender_addr)) < 0)
    {
        close(sock);
        perror("bind");
        exit(1);
    }
    
    if (listen(sock, RECIVER) < 0)
    {
        close(sock);
        perror("listen");
        exit(1);
    }
    
    while (1)
    {
        int reciver = -1;
        reciver = accept(sock, (struct sockaddr*)&reciver_addr, &reciver_size);
        if (reciver < 0)
        {
            close(sock);
            perror("accept");
            exit(1);
        }  
    }
    
    int bytes_sent = 0;
    const char *msg = "hello world";
    bytes_sent = send(reciver, msg, strlen(msg) + 1, 0);
    if (bytes_sent < 0)
    {
        close(reciver);
        close(sock);
        perror("send");
        exit(1);
    }
    
    char buffer[1024] = {0};
    int bytes_recv = 0;
    bytes_recv = recv(sock, buffer, 1024, 0);
    if (bytes_recv < 0)
    {
        close(reciver);
        close(sock);
        perror("recive");
        exit(1);
    }
    else if (bytes_recv == 0)
    {
        close(sock);
        exit(1);
    }
    
}