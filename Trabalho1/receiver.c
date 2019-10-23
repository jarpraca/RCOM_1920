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

    int size;
    size = llreadFile(fd);

    llclose(fd);

    return 0;
}