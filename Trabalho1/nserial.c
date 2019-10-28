#include "application.h"

int main(int argc, char **argv)
{
    int fd;
    struct termios oldtio;

    if ((argc != 3 && argc != 4) ||
        ((strcmp("/dev/ttyS0", argv[1]) != 0) &&
         (strcmp("/dev/ttyS1", argv[1]) != 0) &&
         (strcmp("/dev/ttyS2", argv[1]) != 0)))
    {
        printf("Usage:\tnserial SerialPort TRANSMITTER(1)|RECEIVER(0)\n\tex: nserial /dev/ttyS1 1\n");
        exit(1);
    }

    if(atoi(argv[2]) == RECEIVER){
        fd = llopen(argv[1], RECEIVER);

        int size;
        size = llreadFile(fd);

        llclose(fd);
    }
    else if (atoi(argv[2]) == TRANSMITTER){
        fd = llopen(argv[1], TRANSMITTER);

        llopen_image(argv[3], fd);
        llclose(fd);
    }
    else{
        printf("Usage:\tnserial SerialPort TRANSMITTER(1)|RECEIVER(0)\n\tex: nserial /dev/ttyS1 1\n");
        exit(1);
    }

    return 0;
}