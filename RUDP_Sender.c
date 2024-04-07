#include "RUDP_API.h"

// To insert the given parameters to our variables
void get_info(int argc, char* argv[], char** ip, int* port)
{
        *ip = argv[2];
        *port = atoi(argv[4]);
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
    int port;
    char* ip;

    get_info(argc, argv, &ip, &port);

    p_RUDP_Sock sock = rudp_socket(port, 0);

    if(!sock)
    {
        return -1;
    }

    if(!rudp_connect(sock, ip, port))
    {
        rudp_close(sock);
        return -1;
    }


    p_rudp_pack pack = create_packet();

    while (pack == NULL)
    {
        pack = create_packet();
    }
    
    int resend = 1, total_bytes_sent = 0, if_break = 0;

    while (resend)
    {
        char* data = util_generate_random_data(DATA_SIZE);

        data_packet(pack, data, BUFFER_SIZE);

        while (pack->sequence <= DATA_SIZE)
        {
            int bytes_sent = rudp_send(sock, pack);

            if(!bytes_sent)
            {
                if_break = 1;
                break;
            }

            if(bytes_sent > 0)
            {
                total_bytes_sent += bytes_sent;
                pack->sequence += pack->length;
            }

            data_packet(pack, pack->sequence, data + pack->sequence);//
        }

        if(!if_break)
        {
            printf("File was successfully sent.\n");
        }
        else
        {
            printf("File was not successfully sent.\n");
        }

        free(data);

        printf("1 - to resend, 0 - to exit\n");
        scanf(" %d", &resend); 
    }

    if(!if_break)
    {
        rudp_disconnect(sock, pack->sequence);
        printf("Connection closed\n");
    }

    free(pack);
    rudp_close(sock);
    return 0;
}