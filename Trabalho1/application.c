#include "application.h"
#include "protocol.h"

struct termios oldtio; //vai dar problemas!!!

int llopen_transmitter(int porta){
    int fd=open_port(porta, &oldtio);
    send_set(int fd);
    receive_ua_snd(int fd);
    return fd;
}

int llopen_receiver(int porta){
    int fd=open_port(porta, &oldtio);  
    //receive_set(int fd);    //necessário ???
    //send_ua_rcv(int fd);    //necessário ???
    return fd;
}

int llopen(int porta, bool transmitter){
    if (transmitter)
        llopen_transmitter(porta);
    else
        llopen_receiver(porta);
}

int llwrite(int fd, char * buffer, int length){
    int num=send_msg(fd, buffer, length);
    receive_data_rsp(fd);
    return num;
}

int llread(int fd, char * buffer){
    return receive_data(fd, buffer);
}

int llclose(int fd){
    close_port(fd, &oldtio);
}