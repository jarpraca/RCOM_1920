#include "protocol.h"

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

    fd = open_port(argv, &oldtio);

    // send_disc_snd(fd);
    // receive_disc_snd(fd);
    // send_ua_snd(fd); 

    send_msg(fd, "o", 2);
    printf("AQUI1\n");
    receive_data_rsp(fd);
    printf("AQUI2\n");

    close_port(fd, &oldtio);
    printf("AQUI3\n");

    return 0;
}