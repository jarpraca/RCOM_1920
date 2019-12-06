#include "download.h"

bool parseURL(char* url, info_ftp* info){

    if(strncmp(url, "ftp://", 6) != 0)
        return false;

    char* divider = strchr(url + 6, ':');
    char* at = strchr(url + 6, '@');
    char *slash = strrchr(url + 6, '/');

    // User and Password
    if (divider != NULL && at != NULL && slash != NULL){

        // USER
        int i = 6;

        while(url[i] != ':'){
            info->user[i - 6] = url[i];
            i++;
        }

        info->user[i - 6] = '\0';

        // PASSWORD
        i = 0;
        divider++;

        while (divider[i] != '@')
        {
            info->password[i] = divider[i];
            i++;
        }

        info->password[i] = '\0';

        // HOST
        i = 0;
        at++;

        while (at[i] != '/')
        {
            info->host[i] = at[i];
            i++;
        }

        info->host[i] = '\0';
        at = at + i;

        // FILENAME
        strcpy(info->filename, strrchr(at, '/')+1);

        // PATH
        at++;
        strcpy(info->path, at);
    }
    // User and no Password
    else if (at != NULL && slash != NULL){
        // USER
        int i = 6;

        while (url[i] != '@')
        {
            info->user[i - 6] = url[i];
            i++;
        }

        info->user[i - 6] = '\0';

        // PASSWORD
        printf("Please enter a password (WARNING: password visible): ");
        scanf("%100s", info->password);
        printf("\n");

        // HOST
        i = 0;
        at++;

        while (at[i] != '/')
        {
            info->host[i] = at[i];
            i++;
        }

        info->host[i] = '\0';
        at = at + i;

        // FILENAME
        strcpy(info->filename, strrchr(at, '/') + 1);

        // PATH
        at++;
        strcpy(info->path, at);
    }
    // No User and no Password
    else if(slash != NULL){
        // USER
        strcpy(info->user, "anonymous");

        // PASSWORD
        printf("Please enter an email: ");
        scanf("%100s", info->password);
        printf("\n");

        // HOST
        int i = 0;
        at = url + 6;

        while (at[i] != '/')
        {
            info->host[i] = at[i];
            i++;
        }

        info->host[i] = '\0';
        at = at + i;

        // FILENAME
        strcpy(info->filename, slash + 1);

        // PATH
        at++;
        strcpy(info->path, at);
    }
    else{
        return false;
    }

    return true;
}

char* getHostIP(info_ftp* info){
    struct hostent *h;

    if ((h = gethostbyname(info->host)) == NULL)
    {
        herror("gethostbyname");
        exit(1);
    }

    return inet_ntoa(*((struct in_addr *)h->h_addr));
}

int connectTCP(info_ftp* info, int port)
{
    char *ip;
    ip = getHostIP(info);
    int sockfd;
    struct sockaddr_in server_addr;
    int bytes;

    /*server address handling*/
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);

    /*open an TCP socket*/
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket()");
        exit(0);
    }
    /*connect to the server*/
    if (connect(sockfd,
                (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0)
    {
        perror("connect()");
        exit(0);
    }

    printf("> telnet %s %d\n", info->host, port);

    return sockfd;
}

void readFTPreply(int fd, char* code){
    int state = START;
    int res;

    while(state != END){
        char line[MAX_LINE_LENGTH];
        res = read(fd, line, MAX_LINE_LENGTH);
        line[res-1] = '\0';
        printf("< %s\n", line);
        if(res < 0)
            continue;

        switch (state)
        {
        case START:
            if (isdigit(line[0]) && isdigit(line[1]) && isdigit(line[2])){
                if (line[3] == ' '){
                    strncpy(code, line, 3);
                    state = END;
                }
                else if (line[3] == '-'){
                    strncpy(code, line, 3);
                    state = LINE;
                }
            }
            break;

        case LINE:
            if (strncmp(code, line, 3) && line[3]==' '){
                state = END;
            }
            break;

        default:
            break;
        }
    }
}

int sendFTPcommand(int fd, char* command){
    printf("> %s", command);
    return write(fd, command, strlen(command));
}

int login(int fd, info_ftp* info){
    // User
    char user_command[MAX_STRING_LENGTH+6];
    char user_reply[CODE_LENGTH];

    sprintf(user_command, "user %s\n", info->user);
    sendFTPcommand(fd, user_command);
    readFTPreply(fd, user_reply);

    if (user_reply[0] != '2' && user_reply[0] != '3')
        return (int)user_reply[0];

    // Password
    char pass_command[MAX_STRING_LENGTH+6];
    char pass_reply[CODE_LENGTH];
    
    sprintf(pass_command, "pass %s\n", info->password);
    sendFTPcommand(fd, pass_command);
    readFTPreply(fd, pass_reply);

    return (int)pass_reply[0];
}

int getHostPort(int fd){
    int state = START;
    int res;
    char port1[CODE_LENGTH+1];
    char port2[CODE_LENGTH+1];
    int port1_counter = 0;
    int port2_counter = 0;

    while (state != END)
    {
        char character[2];
        res = read(fd, character, 1);
        char c = character[0];
        if (res < 0)
            continue;

        switch (state)
        {
        case START:
            if (c == '('){
                state = IP1;
            }
            break;

        case IP1:
            if (c == ','){
                state = IP2;
            }
            break;

        case IP2:
            if (c == ','){
                state = IP3;
            }
            break;

        case IP3:
            if (c == ','){
                state = IP4;
            }
            break;

        case IP4:
            if (c == ','){
                state = PORT1;
            }
            break;

        case PORT1:
            if (c == ','){
                port1[port1_counter] = '\0';
                state = PORT2;
            }
            else{
                port1[port1_counter] = c;
                port1_counter++;
            }
            break;

        case PORT2:
            if (c == ')')
            {
                port2[port2_counter] = '\0';
                state = DISCARD;
            }
            else
            {
                port2[port2_counter] = c;
                port2_counter++;
            }
            break;

        case DISCARD:
            if (c == '\n'){
                state = END;
            }
            break;

        default:
            break;
        }
    }

    return atoi(port1) * 256 + atoi(port2);
}

int downloadFile(int fd, char* filename){
    FILE *file;
    file = fopen(filename, "wb+");
    char buffer[MAX_LINE_LENGTH];
    int ret;

    while((ret = read(fd, buffer, MAX_LINE_LENGTH)) > 0){
        fwrite(buffer, sizeof(char), ret, file);
    }

    fclose(file);

    return 0;
}

int retrieveFile(int fd1, int fd2, info_ftp* info){
    char command[MAX_STRING_LENGTH+6];

    sprintf(command, "retr %s\n", info->path);
    sendFTPcommand(fd1, command);

    downloadFile(fd2, info->filename);

    return 0;
}

int main(int argc, char **argv)
{
    info_ftp info;

    if(argc != 2 || !parseURL(argv[1], &info)){
        printf("Usage:\tdownload ftp://[<user>:<password>@]<host>/<url-path>\n\tex: download ftp://anonymous:password@speedtest.tele2.net/1KB.zip\n");
        exit(1);
    }

    int fd1 = connectTCP(&info, DEFAULT_PORT);

    char reply[CODE_LENGTH];
    readFTPreply(fd1, reply);

    login(fd1, &info);

    sendFTPcommand(fd1, "pasv\n");

    int port = getHostPort(fd1);

    int fd2 = connectTCP(&info, port);

    retrieveFile(fd1, fd2, &info);

    sendFTPcommand(fd1, "quit\n");

    close(fd1);
    close(fd2);
    return 0;
}
