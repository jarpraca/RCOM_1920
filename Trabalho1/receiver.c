#include "application.h"

int main(int argc, char **argv)
{
    int fd;
    struct termios oldtio;

    if ((argc < 2) ||
        ((strcmp("/dev/ttyS0", argv[1]) != 0) &&
         (strcmp("/dev/ttyS1", argv[1]) != 0) &&
         (strcmp("/dev/ttyS2", argv[1]) != 0)))
    {
        printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
        exit(1);
    }

    fd = llopen(argv[1], false);

    unsigned char *buffer;
    buffer= malloc(sizeof(char)*1024);
    buffer[0]=" ";
    unsigned char *filename;
    filename= malloc(sizeof(char)*128);
    int size, i=0;
    llreadFile(fd, buffer, filename);

    printf("Final result: %s \n", buffer);
    llclose(fd);
    return 0;
}