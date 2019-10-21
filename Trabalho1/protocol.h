#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int open_port(char **argv, struct termios *oldtio);

void close_port(int fd, struct termios *oldtio);

void send_resp(int fd, char c, char a);

void send_set(int fd);

void send_ua_rcv(int fd);

void send_ua_snd(int fd);

void send_disc_rcv(int fd);

void send_disc_snd(int fd);

void send_data_response(int fd, bool reject, bool duplicated);

void send_msg(int fd, char *msg, int length);

int receive_msg(int fd, unsigned char c, unsigned char a, bool data, unsigned char data_buf[], bool data_resp);

void receive_set(int fd);

void receive_disc_rcv(int fd);

void receive_disc_snd(int fd);

void receive_ua_rcv(int fd);

void receive_ua_snd(int fd);

void receive_data(int fd, unsigned char data_buf[]);

void receive_data_rsp(int fd);