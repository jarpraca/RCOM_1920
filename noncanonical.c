/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define START 0
#define FLAG_RCV 1
#define A_RCV 2
#define C_RCV 3
#define BCC_OK 4
#define STOPS 5

#define FLAG 0x7E
#define A 0x03
#define SET 0x03
#define UA 0x07
#define DISC 0x0B

volatile int STOP=FALSE;

int open_port(struct termios *oldtio){
	int fd;
	struct termios newtio;

	/*
		Open serial port device for reading and writing and not as controlling tty
		because we don't want to get killed if linenoise sends CTRL-C.
	*/
    
    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd <0) {perror(argv[1]); exit(-1); }

    if ( tcgetattr(fd, oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 1 chars received */

	/* 
		VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
		leitura do(s) pr�ximo(s) caracter(es)
	*/

    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");
}

void  close_port(int fd, struct termios *oldtio){
	sleep(1);
    tcsetattr(fd, TCSANOW, oldtio);
    close(fd);
}

void receive_msg(int fd, char c){
	int state=START;
	int res;
	char buf[255];

    while (state!=STOPS) {       /* loop for input */
	 	printf("state:%d\n", state);
		res = read(fd,buf,1);   /* returns after 5 chars have been input */;
	  	printf(":%04x:%d\n", *buf, res);
		int msg=buf[0];
		printf(":%d:%d\n", msg, res);
		switch(state){
			case START:
				if (msg==FLAG)
					state=FLAG_RCV;
				break;
			case FLAG_RCV:
				if (msg==A)
					state=A_RCV;
				else if (msg!=FLAG)
					state=START;
				break;
			case A_RCV:
				if (msg==c)
					state=C_RCV;
				else
					state=START;
				break;
			case C_RCV:
				if (msg==A^c)
					state=BCC_OK;
				else
					state=START;
				break;
			case BCC_OK:
				if (msg==FLAG)
					state=STOPS;
				else
					state=START;
				break;
			default:
				state=START;
				break;
		}
	}
}

void receive_set(int fd){
	receive_msg(fd, SET);
}

void receive_disc(int fd){
	receive_msg(fd, DISC);
}

void send_resp(int fd, char c){
	int res;
	char buf2[5];
	buf2[0] = FLAG;
	buf2[1] = A;
	buf2[2] = c;
	buf2[3] = A^c;
	buf2[4] = FLAG;

    res = write(fd,buf2,5);
}

void send_ua(int fd){
	send_resp(fd, UA);
}

void send_disc(int fd){
	send_resp(fd, DISC);
}

int main(int argc, char** argv)
{
    int fd;
    struct termios oldtio;

    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }

	fd = open_port(&oldtio);
	
	receive_set(fd);
	send_ua(fd);

	close_port(fd, &oldtio);

    return 0;
}
