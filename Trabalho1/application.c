#include "application.h"
#include "protocol.h"

struct termios oldtio; //vai dar problemas!!!
bool trans;
int llopen_transmitter(int porta){
    int fd=open_port(porta, &oldtio);
    send_set(fd);
    receive_ua_snd(fd);
    return fd;
}

int llopen_receiver(int porta){
    int fd=open_port(porta, &oldtio);  
    receive_set(fd);    //necessário ???
    send_ua_rcv(fd);    //necessário ???
    return fd;
}

int llopen(int porta, bool transmitter){
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