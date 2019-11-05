#include "application.h"

int main(int argc, char **argv)
{
    int fd;
    struct termios oldtio;
    
    clock_t Ticks[2];
    
    
    if ((argc != 3 && argc != 4) ||
        ((strcmp("/dev/ttyS0", argv[1]) != 0) &&
         (strcmp("/dev/ttyS1", argv[1]) != 0) &&
         (strcmp("/dev/ttyS2", argv[1]) != 0)))
    {
        printf("Usage:\tnserial SerialPort TRANSMITTER(1)|RECEIVER(0)\n\tex: nserial /dev/ttyS1 1\n");
        exit(1);
    }
    
    Ticks[0] = clock();
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

    Ticks[1] = clock();
    double Tempo = (Ticks[1] - Ticks[0]) * 1000.0 / CLOCKS_PER_SEC;
    printf("Tempo gasto: %g ms. \n", Tempo);
    return 0;
}
