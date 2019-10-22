#include "application.h"
#include "protocol.h"

#define PACKAGE_SIZE 8
struct termios oldtio; 
bool trans;

void createDataPackage(char* package, int indice){
    char buffer[strlen(package)+5];
    buffer[0]=1;
    buffer[1]=indice%255;
    buffer[2]=strlen(package)/8;
    buffer[3]=strlen(package)%8;
    for(int i=0; i<strlen(package); i++)
        buffer[i+4]=package[i];
     for(int i=0; i<strlen(buffer); i++)
        package[i]=buffer[i];
    printf("package: %s\n", package);
}

void createControlPackage(char* buffer, int controlCamp, int fileSize, char* path){
    buffer[0]=controlCamp;
    buffer[1]=0;
    buffer[2]=1;
    buffer[3]=fileSize;
    buffer[4]=1;
    buffer[5]=strlen(path);
    buffer[6]=*path;

    printf("buffer: %s \n", buffer);
}

void llopen_image(char* path, int fd){
    FILE* file;
    file = fopen(path, "r");
    fseek(file, 0L, SEEK_END);
    int size = ftell(file);
    fseek(file,0L, SEEK_SET);
    char packageBuf[PACKAGE_SIZE+1];
    createControlPackage(packageBuf, 2, size, path);
    llwrite(fd,packageBuf, strlen(packageBuf));
    int num, i=1;
    do{
        printf("i: %d\n",i);
       num = fread(packageBuf, 1, PACKAGE_SIZE, file);
       packageBuf[PACKAGE_SIZE]='\0';
       createDataPackage(packageBuf, i-1);
           printf("aqui\n");
       llwrite(fd, packageBuf,strlen(packageBuf));
           printf("aqui2 \n");

       i++;
    } while(num == PACKAGE_SIZE);
    createControlPackage(packageBuf,3, size, path);
    llwrite(fd,packageBuf, strlen(packageBuf));
}

int llopen_transmitter(char* porta){
    int fd=open_port(porta, &oldtio);
    send_set(fd);
    receive_ua_snd(fd);
    return fd;
}

int llopen_receiver(char* porta){
    int fd=open_port(porta, &oldtio);  
    receive_set(fd);
    send_ua_rcv(fd); 
    return fd;
}

int llopen(char* porta, bool transmitter){
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

int llread(int fd, unsigned char * buffer){
    return receive_data(fd, buffer);
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