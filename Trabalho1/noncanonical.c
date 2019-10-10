/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdbool.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define START 0
#define FLAG_RCV 1
#define A_RCV 2
#define C_RCV 3
#define BCC_OK 4
#define RCV_DATA 5
#define BCC2_OK 6
#define STOPS 7

#define FLAG 0x7E
#define A_RSP 0x03
#define A_CMD 0x01
#define SET 0x03
#define UA 0x07
#define DISC 0x0B
#define ESC 0x7D
#define ESC1 0x5E
#define ESC2 0x0D
#define RR_R1 0x85
#define RR_R0 0x05
#define REJ_R1 0x81
#define REJ_R2 0x01
#define C_DATA_S0 0x00
#define C_DATA_S1 0x40

volatile int STOP=FALSE;

int open_port(char **argv, struct termios *oldtio)
{
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

	return fd;
}

void  close_port(int fd, struct termios *oldtio){
	sleep(1);
    tcsetattr(fd, TCSANOW, oldtio);
    close(fd);
}

void receive_msg(int fd, char c, char a, bool data){
	int state=START;
	int res;
	char buf[255];
	bool escape=false;

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
				if (msg==a)
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
				if (msg==a^c)
					state=BCC_OK;
				else
					state=START;
				break;
			case BCC_OK:
				if (msg==FLAG){
					state=STOPS;
				} 
				else if (data){
					state=RCV_DATA;
					if (msg==ESC)
						escape=true;
					else
						escape=false;//save data
				}
				else
					state=START;
				break;
			case RCV_DATA:
				if (msg==FLAG){
					state=STOPS;
					break;
				} 
				if (escape)
				{
					if (msg==ESC1)
						escape=false; //save data (0x7E)
					else if (msg==ESC2)
						escape=false; //save data (0x7D)
					else
						escape=false; //ERRO
					escape=false;
					break;
				}
				//save data (msg)
				break;
			default:
				state=START;
				break;
		}
	}
}

void receive_set(int fd){
	receive_msg(fd, SET, A_RSP);
}

void receive_disc(int fd){
	receive_msg(fd, DISC, A_RSP);
}

void receive_ua(int fd){
	receive_msg(fd, SET, A_CMD);
}

void send_resp(int fd, char c, char a){
	int res;
	char buf2[5];
	buf2[0] = FLAG;
	buf2[1] = a;
	buf2[2] = c;
	buf2[3] = a^c;
	buf2[4] = FLAG;

    res = write(fd,buf2,5);
}

void send_ua(int fd){
	send_resp(fd, UA, A_RSP);
}

void send_disc(int fd){
	send_resp(fd, DISC, A_CMD);
}

int main(int argc, char** argv)
{
    int fd;
    struct termios oldtio;

	if ((argc < 2) ||
		((strcmp("/dev/ttyS0", argv[1]) != 0) &&
		 (strcmp("/dev/ttyS1", argv[1]) != 0) &&
		 (strcmp("/dev/ttyS2", argv[1]) != 0))) {
		printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
		exit(1);
    }

	fd = open_port(argv, &oldtio);
	
	receive_set(fd);
	send_ua(fd);

	close_port(fd, &oldtio);

    return 0;
}
