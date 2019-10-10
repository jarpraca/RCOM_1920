/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define SIZE 255

#define START 0
#define FLAG_RCV 1
#define A_RCV 2
#define C_RCV 3
#define UA_RCV 3
#define BCC_OK 4
#define STOPS 5

#define FLAG 0x7E
#define A 0x03
#define C_SET 0x03
#define C_UA 0x07
#define BCC A ^ C_SET
#define BCC2 A ^ C_UA

#define MAX_RETR 3
#define TIMEOUT 3

volatile int STOP = FALSE;
int fd;
int state = START;
static int num_retr = 0;

void send_set()
{
  char buf[255];
  buf[0] = FLAG;
  buf[1] = A;
  buf[2] = C_SET;
  buf[3] = BCC;
  buf[4] = FLAG;

  write(fd, buf, 5);
}

void alarmHandler()
{
  if (state >= UA_RCV)
    return;
  if (num_retr < MAX_RETR)
  {
    printf("alarm %d \n", num_retr);
    send_set();
    alarm(TIMEOUT);
    num_retr++;
  }
  else
  {
    exit(1);
  }
}

int main(int argc, char **argv)
{
  signal(SIGALRM, alarmHandler);

  int c, res;
  struct termios oldtio, newtio;
  int i, sum = 0, speed = 0;

  if ((argc < 2) ||
      ((strcmp("/dev/ttyS0", argv[1]) != 0) &&
       (strcmp("/dev/ttyS1", argv[1]) != 0) &&
       (strcmp("/dev/ttyS2", argv[1]) != 0)))
  {
    printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
    exit(1);
  }

  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */

  fd = open(argv[1], O_RDWR | O_NOCTTY);
  if (fd < 0)
  {
    perror(argv[1]);
    exit(-1);
  }

  if (tcgetattr(fd, &oldtio) == -1)
  { /* save current port settings */
    perror("tcgetattr");
    exit(-1);
  }

  bzero(&newtio, sizeof(newtio));
  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;

  /* set input mode (non-canonical, no echo,...) */
  newtio.c_lflag = 0;

  newtio.c_cc[VTIME] = 0; /* inter-character timer unused */
  newtio.c_cc[VMIN] = 1;  /* blocking read until 5 chars received */

  /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) pr�ximo(s) caracter(es)
  */

  tcflush(fd, TCIOFLUSH);

  if (tcsetattr(fd, TCSANOW, &newtio) == -1)
  {
    perror("tcsetattr");
    exit(-1);
  }

  printf("New termios structure set\n");

  send_set();
  alarm(TIMEOUT);

  /* 
    O ciclo FOR e as instru��es seguintes devem ser alterados de modo a respeitar 
    o indicado no gui�o 
  */

  char buf[255];

  while (state != STOPS)
  { /* loop for input */
    printf("state:%d\n", state);
    res = read(fd, buf, 1); /* returns after 5 chars have been input */
    ;
    printf(":%04x:%d\n", *buf, res);
    int msg = buf[0];
    printf(":%d:%d\n", msg, res);
    switch (state)
    {
    case START:
      if (msg == FLAG)
        state = FLAG_RCV;
      break;
    case FLAG_RCV:
      if (msg == A)
        state = A_RCV;
      else if (msg != FLAG)
        state = START;
      break;
    case A_RCV:
      if (msg == C_UA)
        state = C_RCV;
      else
        state = START;
      break;
    case C_RCV:
      if (msg == BCC)
        state = BCC_OK;
      else
        state = START;
      break;
    case BCC_OK:
      if (msg == FLAG)
        state = STOPS;
      else
        state = START;
      break;
    default:
      state = START;
      break;
    }
  }

  sleep(1);

  if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
  {
    perror("tcsetattr");
    exit(-1);
  }

  close(fd);
  return 0;
}