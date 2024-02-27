#include <stdio.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <time.h>
#include <arpa/inet.h>

void get_info(int argc, char* argv[], char** ip, int* port, char** algo);

char *util_generate_random_data(unsigned int size);