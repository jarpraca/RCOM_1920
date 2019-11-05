#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define TRANSMITTER 1
#define RECEIVER 0

void send_file();

int llopen(unsigned char *porta, bool transmitter);

int llreadFile(int fd);

int llwrite(int fd, unsigned char * buffer, int length);

int llclose(int fd);

void llopen_image(unsigned char *path, int fd);
