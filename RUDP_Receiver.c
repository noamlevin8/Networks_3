#include "RUDP_API.h"


int main(int argc, char* argv[]) 
{
    int Receiver_Port = atoi(argv[2]);

    char* str = malloc(34);
    strcpy(str, "-       * Statistics *        -\n");

    struct sockaddr_in ReceiverAddress;
    p_RUDP_Sock sock = rudp_socket(Receiver_Port, 1);

    if(!sock)
    {
        return -1;
    }

    memset(&ReceiverAddress, 0, sizeof(ReceiverAddress));
    ReceiverAddress.sin_family = AF_INET; // IPV4
    ReceiverAddress.sin_addr.s_addr = INADDR_ANY; // Random address
    ReceiverAddress.sin_port = htons(Receiver_Port); //Port

    // Bind the socket to the port with any IP at this port
    if (bind(sock->socket_fd, (struct sockaddr *)&ReceiverAddress, sizeof(ReceiverAddress)) == -1) 
    {
        printf("Bind failed with error code : %d", errno);
        // close the socket
        rudp_close(sock);
        return -1;
    }

    printf("Waiting for RUDP connection...\n");

    int accept = rudp_accept(sock);

    while (accept != 1)
    {
        if(accept == 2) // Connection closed
        {
            return -1;
        }

        accept = rudp_accept(sock);
    }
    
    printf("Sender connected, beginning to receive file...\n");
    
    int bytesReceived, TotalBytesReceived, if_break = 0;
    double time_last, time_cur, total_time;
    int count_runs = 0;
    double sum_times = 0;
    double sum_bandwidth = 0;
    struct timeval time;
    double bandwidth = 0;
    p_rudp_pack pack = create_packet();
    p_rudp_pack prev_pack = create_packet();

    while (1)
    {  
        TotalBytesReceived = 0;
        bytesReceived = 0;

        time_last = 0;
        total_time = 0;

        while (pack->sequence <= 2000000)
        {   
            bytesReceived = rudp_recv(sock, pack, prev_pack);

            gettimeofday(&time, NULL);
            time_cur = (time.tv_sec * 1000.0) + (time.tv_usec / 1000.0);

            if(bytesReceived == 0) // Sender disconnected
            {
                if_break = 1;
                break;
            }

            else if(bytesReceived == -2 || bytesReceived == -3)
            {
                bytesReceived = 0;
            }

            else if(bytesReceived == -4)
            {
                memset(prev_pack, 0, sizeof(rudp_pack));
                TotalBytesReceived = 0;
                bytesReceived = 0;
                time_last = 0;
                gettimeofday(&time, NULL);
                time_cur = (time.tv_sec * 1000.0) + (time.tv_usec / 1000.0);
            }

            else if(bytesReceived < 0)
            {
                if_break = 1;
                break;
            }

            if(time_last != 0)
                total_time += time_cur - time_last;
            
            time_last = time_cur;

            TotalBytesReceived += bytesReceived;
        }
        
        if(bytesReceived == 0 || if_break) 
        {
            free(pack);
            free(prev_pack);
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

        memset(pack, 0, sizeof(rudp_pack));
        memset(prev_pack, 0, sizeof(rudp_pack));

        bytesReceived = rudp_recv(sock,pack, prev_pack);
        gettimeofday(&time, NULL);
        time_cur = (time.tv_sec * 1000.0) + (time.tv_usec / 1000.0);

        if(bytesReceived == 0) 
        {
            free(pack);
            free(prev_pack);
            break;
        }
        memset(prev_pack, 0, sizeof(rudp_pack));
    }

    printf("Sender sent exit message.\n");

    printf("-----------------------------\n");
    printf("%s\n\n", str);
    printf("- Avrage time: %f ms\n", sum_times/count_runs);
    printf("- Avrage bandwidth: %f MB/s\n", sum_bandwidth/count_runs);
    printf("-----------------------------\n");

    rudp_close(sock);
    printf("Receiver end.\n");
    free(str);
    return 0;
}