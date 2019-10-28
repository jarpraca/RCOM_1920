#include "application.h"
#include "protocol.h"

#define PACKAGE_SIZE 256
struct termios oldtio;
bool trans;

void createDataPackage(unsigned char *package, int indice, int num)
{
    unsigned char* buffer;
    buffer = malloc(sizeof(unsigned char)*(num + 5));
    unsigned char aux[2];
    buffer[0]='1';
    sprintf(aux, "%1x", indice%255);
    buffer[1]=aux[0];
    sprintf(aux, "%d", num/8);
    buffer[2]=aux[0];
    sprintf(aux, "%d", num%8);
    buffer[3]=aux[0];
    for (int i=0; i<num; i++)
        buffer[4+i]=package[i];
    for (int i=0; i<num+4; i++)
        package[i]=buffer[i];
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

    printf("control: %s \n", buffer);
}

void llopen_image(unsigned char *path, int fd)
{
    FILE* file;
    file = fopen(path, "r");

    if(file==NULL){
        perror("File opening failed ");
        exit(1);
    }
    fseek(file, 0L, SEEK_END);
    int size = ftell(file);
    fseek(file,0L, SEEK_SET);
    unsigned char packageControl[5 + strlen(path)];
    createControlPackage(packageControl, 0x02, size, path);
    llwrite(fd,packageControl, strlen(packageControl));
    int num, i=0;
    do{
        unsigned char *packageBuf;
        packageBuf = malloc(sizeof(unsigned char) * (PACKAGE_SIZE + 4));
        num = fread(packageBuf, sizeof(unsigned char), PACKAGE_SIZE, file);
        packageBuf[num] = '\0';
        createDataPackage(packageBuf, i, num);
        llwrite(fd, packageBuf, num+4);
        if (i % 50 == 0)
            printf("%d\n", i);
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
    int num;
    do{
        num=send_msg(fd, buffer, length);
    } while(!receive_data_rsp(fd));
    
    return num;
}

int processPackage(unsigned char *buffer, unsigned char *filename, int sequenceNumber)
{
    switch(buffer[0]){
        case '1':{
            unsigned char aux[2];
            sprintf(aux, "%x", sequenceNumber);
            if (buffer[1] == aux[0])
            {
                int k = (int)buffer[2] + (int)buffer[3] * 8;
                unsigned char aux2[PACKAGE_SIZE+4];
                for (int i=0; i < PACKAGE_SIZE+4; i++)
                    aux2[i] = buffer[i];
                //strcpy(aux2, buffer);
                //strncpy(buffer, &aux2[4], k);
                for (int i=0; i<k; i++)
                    buffer[i] = aux2[i+4];
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
    int num = receive_data(fd, buffer);
    return num;
}

int llreadFile(int fd ){
    unsigned char filename[128];
    unsigned char buf[1024];
    llread(fd, buf);
    processPackage(buf, filename, -1);
    filename[0]='z';
    FILE *file;
    file = fopen(filename, "w");

    unsigned char *aux;
    int num, i = 0, sequenceNumber = 0;
    int ret;
    do{
        unsigned char buf[1024];
        num = llread(fd, buf);
        ret = processPackage(buf, filename, sequenceNumber);
        if(ret==1){
            fwrite(buf, sizeof(unsigned char), num-4, file);
            sequenceNumber++;
            sequenceNumber %= 255;
        }
        if(i%50==0)
            printf("%d\n",i);
        i++;
    }  while(ret != 3);

    fclose(file);

    return num;
}

int llclose_transmitter(int fd){
    send_disc_snd(fd);
    receive_disc_snd(fd);
    send_ua_snd(fd);
    close_port(fd, &oldtio);
    return 0;
}

int llclose_receiver(int fd){
    receive_disc_rcv(fd);
    send_disc_rcv(fd);
    receive_ua_rcv(fd);
    close_port(fd, &oldtio);
    return 0;
}

int llclose(int fd){
    if (trans)
        return llclose_transmitter(fd);
    else
        return llclose_receiver(fd);
}