#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <strings.h>
#include <stdbool.h>
#include <string.h>

#define SERVER_PORT 6000
#define SERVER_ADDR "192.168.28.96"
#define MAX_SIZE_STRING 100
#define DEFAULT_PORT 21

typedef struct {

    char user[MAX_SIZE_STRING];
    char password[MAX_SIZE_STRING];
    char host[MAX_SIZE_STRING];
    char path[MAX_SIZE_STRING];
    char filename[MAX_SIZE_STRING];

} info_ftp;