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
#include <ctype.h>

#define SUCCESS 0
#define ERROR 1
#define FAIL 2

// VALUES
#define MAX_STRING_LENGTH 100
#define MAX_COMMAND_LENGTH 106  
#define MAX_LINE_LENGTH 100
#define CODE_LENGTH 3
#define DEFAULT_PORT 21

// STATES
#define START 0
#define LINE 1
#define IP1 2
#define IP2 3
#define IP3 4
#define IP4 5
#define PORT1 6
#define PORT2 7
#define DISCARD 8
#define END 9

typedef struct {

    char user[MAX_STRING_LENGTH];
    char password[MAX_STRING_LENGTH];
    char host[MAX_STRING_LENGTH];
    char path[MAX_STRING_LENGTH];
    char filename[MAX_STRING_LENGTH];

} info_ftp;