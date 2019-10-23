#include "application.h"
#include "protocol.h"

#define PACKAGE_SIZE 8
struct termios oldtio; 
static int sequenceNumber = 0;
bool trans;

void createDataPackage(char* package, int indice){
    char buffer[strlen(package)+4];
    char aux[2];
    buffer[0]='1';
    sprintf(aux, "%d", indice%255);
    buffer[1]=aux[0];
    sprintf(aux, "%d", strlen(package)/8);
    buffer[2]=aux[0];
    sprintf(aux, "%d", strlen(package)%8);
    buffer[3]=aux[0];
    buffer[4]='\0';
    strcat(buffer, package);
    strcpy(package, buffer);
}

void createControlPackage(char* buffer, int controlCamp, int fileSize, char* path){
    char aux[2];
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
}

void llopen_image(char* path, int fd){
    FILE* file;
    file = fopen(path, "r");
    fseek(file, 0L, SEEK_END);
    int size = ftell(file);
    fseek(file,0L, SEEK_SET);
    char packageControl[5+strlen(path)];
    createControlPackage(packageControl, 0x02, size, path);
    llwrite(fd,packageControl, strlen(packageControl));
    int num, i=0;
    do{
       char *packageBuf;
       packageBuf = malloc(sizeof(char)*(PACKAGE_SIZE+4));
       num = fread(packageBuf, 1, PACKAGE_SIZE, file);
       packageBuf[PACKAGE_SIZE]='\0';
       createDataPackage(packageBuf, i);
       llwrite(fd, packageBuf,strlen(packageBuf));
       i++;
       free(packageBuf);
    } while(num == PACKAGE_SIZE);
    createControlPackage(packageControl,3, size, path);
    llwrite(fd,packageControl, strlen(packageControl));
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

int processPackage(char* buffer, char* filename){
    switch(buffer[0]){
        case '1':{
                char aux[2];
                sprintf(aux, "%d", sequenceNumber);
                if(buffer[1]==aux[0]){
                    int k = (int)buffer[2]+(int)buffer[3]*8;
                    strncpy(buffer, &buffer[4], k);
                    sequenceNumber++;
                }
                return 1;
            }
         
        case '2':
            if(buffer[4]=='1'){
                strncpy(filename, &buffer[6], (int)buffer[5]);
            }
            return 2;
        case '3':
            return 3;
        default: 
            return -1;
    }
}

int llread(int fd, unsigned char *buffer){
    int num =  receive_data(fd, buffer);
    return num;
}

int llreadFile(int fd, unsigned char * buffer, unsigned char* filename){
    char* aux;
    int num, i=0;
    int ret;
    do{  
        char buf[1024];
        num = llread(fd, buf);
        ret = processPackage(buf, filename);
        if(ret== 1)
            strcat(buffer,buf);
        i++;
    }  while(ret != 3);

    return strlen(buffer);
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