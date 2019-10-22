#include "application.h"

int main(int argc, char **argv)
{
    int fd;
    struct termios oldtio;

    if ((argc != 3) ||
        ((strcmp("/dev/ttyS0", argv[1]) != 0) &&
         (strcmp("/dev/ttyS1", argv[1]) != 0) &&
         (strcmp("/dev/ttyS2", argv[1]) != 0)))
    {
        printf("Usage:\tnserial SerialPort \t imagePath \n\tex: nserial /dev/ttyS1 C:\\image \n");
        exit(1);
    }

   fd = llopen(argv[1], true);


   // unsigned char buffer[] = "ola tudo bem?ola tudo bem?ola tudo bem?ola tudo bem?ola tudo bem?ola tudo bem?ola tudo bem?ola tudo bem?ola tudo bem?ola tudo bem?ola tudo bem?";
    llopen_image(argv[2], fd);
    llclose(fd);

    return 0;
}