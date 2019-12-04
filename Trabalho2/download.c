#include "download.h"

bool parseURL(char* url, info_ftp* info){

    if(strncmp(url, "ftp://", 6) != 0)
        return false;

    char* divider = strchr(url + 6, ':');
    char* at = strchr(url + 6, '@');
    char *slash = strrchr(url + 6, '/');

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

int main(int argc, char **argv)
{
    info_ftp info;

    if(!parseURL(argv[1], &info)){
        printf("Usage:\tftp://[<user>:<password>@]<host>/<url-path>\n\tex: ftp://username:password@fe.up.pt/example.txt\n");
        exit(1);
    }
        
    printf("User: %s\n", info.user);
    printf("Password: %s\n", info.password);
    printf("Host: %s\n", info.host);
    printf("Path: %s\n", info.path);
    printf("File: %s\n", info.filename);

    char ip[MAX_SIZE_STRING];

    strcpy(ip, getHostIP(&info));

    printf("\nIP: %s\n", ip);

    return 0;
}
