#include "application.h"
#include "protocol.h"

#define PACKAGE_SIZE 8
struct termios oldtio; 
static int sequenceNumber = 0;
bool trans;

void createDataPackage(unsigned char *package, int indice)
{
    unsigned char* buffer;
    buffer = malloc(sizeof(unsigned char)*(strlen(package) + 5));
    unsigned char aux[2];
    buffer[0]='1';
    sprintf(aux, "%1x", indice%255);
    buffer[1]=aux[0];
    sprintf(aux, "%d", strlen(package)/8);
    buffer[2]=aux[0];
    sprintf(aux, "%d", strlen(package)%8);
    buffer[3]=aux[0];
    buffer[4]='\0';
    strcat(buffer, package);
    strcpy(package, buffer);
    free(buffer);
}

void createControlPackage(unsigned char *buffer, int controlCamp, int fileSize, unsigned char *path)
{
    unsigned char aux[2];
    sprintf(aux, "%d",controlCamp);
    buffer[0]=aux[0];
    buffer[1]='0';
    buffer[2]='1';
    buffer[3]=fileSize;
    buffer[4]='1';
    sprintf(aux, "%d",strlen(path));
    buffer[5]=aux[0];
    buffer[6]='\0';
    strcat(buffer, path);
    printf("path: %s\n",path);
    printf("buf: %s\n", buffer);
}

void llopen_image(unsigned char *path, int fd)
{
    FILE* file;
    file = fopen(path, "r");
    fseek(file, 0L, SEEK_END);
    int size = ftell(file);
    fseek(file,0L, SEEK_SET);
    unsigned char packageControl[5 + strlen(path)];
    createControlPackage(packageControl, 0x02, size, path);
    llwrite(fd,packageControl, strlen(packageControl));
    int num, i=0;
    do{
        unsigned char *packageBuf;
        packageBuf = malloc(sizeof(unsigned char) * (PACKAGE_SIZE + 5));
        num = fread(packageBuf, 1, PACKAGE_SIZE, file);
        packageBuf[PACKAGE_SIZE] = '\0';
        createDataPackage(packageBuf, i);
        llwrite(fd, packageBuf, strlen(packageBuf));
        i++;
        free(packageBuf);
    } while(num == PACKAGE_SIZE);
    createControlPackage(packageControl,3, size, path);
    llwrite(fd,packageControl, strlen(packageControl));
}

int llopen_transmitter(unsigned char *porta)
{
    int fd=open_port(porta, &oldtio);
    send_set(fd);
    receive_ua_snd(fd);
    return fd;
}

int llopen_receiver(unsigned char *porta)
{
    int fd=open_port(porta, &oldtio);  
    receive_set(fd);
    send_ua_rcv(fd); 
    return fd;
}

int llopen(unsigned char *porta, bool transmitter)
{
    trans=transmitter;
    if (transmitter)
        llopen_transmitter(porta);
    else
        llopen_receiver(porta);
}

int llwrite(int fd, unsigned char * buffer, int length){
    int num=send_msg(fd, buffer, length);
    receive_data_rsp(fd);
    return num;
}

int processPackage(unsigned char *buffer, unsigned char *filename)
{
    switch(buffer[0]){
        case '1':{
            unsigned char aux[2];
            sprintf(aux, "%d", sequenceNumber);
            //if (buffer[1] == aux[0])
            //{
                int k = (int)buffer[2] + (int)buffer[3] * 8;
                unsigned char aux2[PACKAGE_SIZE+5];
                strcpy(aux2, buffer);
                strncpy(buffer, &aux2[4], k);
                sequenceNumber++;
            //}
            printf("buf: %s\n", buffer);
            return 1;
        }
         
        case '2':
            if(buffer[4]=='1'){
                strncpy(filename, &buffer[6], (int)buffer[5]-1);
            }
            return 2;
        case '3':
            return 3;
        default: 
            return -1;
    }
}

int llread(int fd, unsigned char *buffer){
    int num = receive_data(fd, buffer);
    return num;
}

int llreadFile(int fd ){
    unsigned char filename[128];
    unsigned char buf[1024];
    llread(fd, buf);
    processPackage(buf, filename);

    FILE *file;
    file = fopen(filename, "a");

    unsigned char *aux;
    int num, i=0;
    int ret;
    do{
        unsigned char buf[1024];
        num = llread(fd, buf);
        ret = processPackage(buf, filename);
        if(ret==1)
            fwrite(buf, sizeof(unsigned char), strlen(buf), file);
        i++;
    }  while(ret != 3);

    fclose(file);

    return strlen(buf);
}

int llclose_transmitter(int fd){
    send_disc_snd(fd);
    receive_disc_snd(fd);
    send_ua_snd(fd);
    close_port(fd, &oldtio);
}

int llclose_receiver(int fd){
    receive_disc_rcv(fd);
    send_disc_rcv(fd);
    receive_ua_rcv(fd);
    close_port(fd, &oldtio);
}

int llclose(int fd){
     if (trans)
        llclose_transmitter(fd);
    else
        llclose_receiver(fd);
}