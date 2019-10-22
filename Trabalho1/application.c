#include "application.h"
#include "protocol.h"

#define PACKAGE_SIZE 8
struct termios oldtio; 
static int sequenceNumber = 0;
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
    for(int i=0; i<strlen(path);i++)
        buffer[6+i]=path[i];
}

void llopen_image(char* path, int fd){
    FILE* file;
    file = fopen(path, "r");
    fseek(file, 0L, SEEK_END);
    int size = ftell(file);
    fseek(file,0L, SEEK_SET);
    char packageBuf[PACKAGE_SIZE+strlen(path)];
    createControlPackage(packageBuf, 0x02, size, path);
    llwrite(fd,packageBuf, strlen(packageBuf));
    int num, i=1;
    do{
       num = fread(packageBuf, 1, PACKAGE_SIZE, file);
       packageBuf[PACKAGE_SIZE]='\0';
       createDataPackage(packageBuf, i-1);
       llwrite(fd, packageBuf,strlen(packageBuf));
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

int processPackage(char* buffer, char* filename){
    printf(" buffer package : %x \n", buffer[0]);
    switch(buffer[0]){
        case '1':
          if(buffer[1]==sequenceNumber){
            int k = atoi(buffer[2])+atoi(buffer[3])*8;
            for(int i=0; i<k;i++){
                buffer[i]=buffer[4+i];
            }
            buffer[k]='\0';
            sequenceNumber++;
        }
        printf("1\n");
        return 1;
        case '2':
        printf("2\n");
            if(buffer[4]==1){
                for(int i=0; i<buffer[5];i++) {
                    filename[i]=buffer[6+i];
                }
            }
            return 2;
        case '3':
        printf("3\n");
            return 3;
        default: 
            return -1;
    }
}

int llread(int fd, unsigned char *buffer){
    int num =  receive_data(fd, buffer);
    printf("buffer read %x", buffer[0]);
    return num;
}

int llreadFile(int fd, unsigned char * buffer, unsigned char* filename){
    char* aux;
    int num, i=0;
    int ret;
    do{    
        char buf[1024];
        num = llread(fd, buf);
        printf("buffer read file %x \n", buf[0]);
        ret = processPackage(buf, filename);
        printf("i: %d, num: %d\n", i, ret);
        if(ret== 1)
            strcat(buffer,buf);
        i++;
    }  while(ret != 3);
    return num;
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