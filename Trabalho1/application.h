#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

void send_file();

int llopen(int porta, bool transmitter);

int llread(int fd, unsigned char * buffer);

int llwrite(int fd, unsigned char * buffer, int length);

int llclose(int fd);

