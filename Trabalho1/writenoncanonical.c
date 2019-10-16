/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

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
#define A_RCV_RSP 0x03
#define A_RCV_CMD 0x01
#define A_SND_CMD 0x03
#define A_SND_RSP 0x01
#define SET 0x03
#define UA 0x07
#define DISC 0x0B
#define ESC 0x7D
#define ESC1 0x5E
#define ESC2 0x0D
#define RR_R1 0x85
#define RR_R0 0x05
#define REJ_R1 0x81
#define REJ_R0 0x01
#define C_DATA_S0 0x00
#define C_DATA_S1 0x40

volatile int STOP = FALSE;
bool even_bit = 0;
bool previous_s = 0;

int open_port(char **argv, struct termios *oldtio)
{
  int fd;
  struct termios newtio;

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

  if (tcgetattr(fd, oldtio) == -1)
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
  newtio.c_cc[VMIN] = 1;  /* blocking read until 1 chars received */

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

  return fd;
}

void close_port(int fd, struct termios *oldtio)
{
  sleep(1);
  tcsetattr(fd, TCSANOW, oldtio);
  close(fd);
}

void send_resp(int fd, char c, char a)
{
  char buf2[5];
  buf2[0] = FLAG;
  buf2[1] = a;
  buf2[2] = c;
  buf2[3] = a ^ c;
  buf2[4] = FLAG;

  write(fd, buf2, 5);
}

void send_set(int fd)
{
  send_resp(fd, SET, A_SND_CMD);
}

void send_ua_rcv(int fd)
{
  send_resp(fd, UA, A_RCV_RSP);
}

void send_ua_snd(int fd)
{
  send_resp(fd, UA, A_SND_RSP);
}

void send_disc_rcv(int fd)
{
  send_resp(fd, DISC, A_RCV_CMD);
}

void send_disc_snd(int fd)
{
  send_resp(fd, DISC, A_SND_CMD);
}

void send_data_response(int fd, bool reject, bool duplicated)
{
  if (reject && duplicated && even_bit)
    send_resp(fd, RR_R0, A_RCV_RSP);
  else if (reject && duplicated && !even_bit)
    send_resp(fd, RR_R0, A_RCV_RSP);
  else if (reject && even_bit)
    send_resp(fd, REJ_R1, A_RCV_RSP);
  else if (reject && !even_bit)
    send_resp(fd, REJ_R1, A_RCV_RSP);
  else if (!reject && even_bit)
    send_resp(fd, RR_R0, A_RCV_RSP);
  else
    send_resp(fd, RR_R1, A_RCV_RSP);
}

void send_msg(int fd, char *msg, int length)
{
  char buf2[7 + length];
  buf2[0] = FLAG;
  buf2[1] = A_SND_CMD;
  if (even_bit)
    buf2[2] = C_DATA_S0;
  else
    buf2[2] = C_DATA_S1;
  buf2[3] = (A_SND_CMD ^ buf2[2]);
  for (int i = 0; i < length; i++)
  {
    buf2[4 + i] = msg[i];
  }
  buf2[4 + length] = 0; //bcc2
  buf2[5 + length] = FLAG;
  buf2[6 + length] = 0;
  write(fd, buf2, 7 + length);
}

int receive_msg(int fd, char c, char a, bool data, char *data_buf, bool data_resp)
{
  int state = START;
  int res;
  char buf[255];
  bool escape = false;
  int cnt = 0;

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
      if (msg == a)
        state = A_RCV;
      else if (msg != FLAG)
        state = START;
      break;
    case A_RCV:
      if (!data && !data_resp && msg == c)
        state = C_RCV;
      else if (!data && !data_resp)
        state = START;
      else if (data_resp)
      {
        printf("C: %x\n", msg);
        if (msg == RR_R1 || msg == RR_R0 || msg == REJ_R1 || msg == REJ_R0)
        {
          data_buf = msg;
          state = C_RCV;
        }
        else
          state = START;
      }
      else if (msg == C_DATA_S0 || msg == C_DATA_S1)
      {
        if (((bool)msg) != previous_s)
        {
          even_bit = !(bool)msg;
          state = C_RCV;
        }
        else
        {
          even_bit = (bool)msg;
          send_data_response(fd, true, (((bool)msg) == previous_s));
        }
      }
      break;
    case C_RCV:
      if (msg == (a ^ c))
        state = BCC_OK;
      else
      {
        state = START;
        send_data_response(fd, true, false);
      }
      break;
    case BCC_OK:
      if (msg == FLAG)
      {
        state = STOPS;
      }
      else if (data)
      {
        printf("0\n");
        state = RCV_DATA;
        printf("1\n");

        if (msg == ESC)
          escape = true;
        else
        {
          printf("2\n");

          char msg_string[255];
          sprintf(msg_string, "%d", msg);
          printf("3\n");

          strcat(data_buf, msg_string); //save data
          cnt++;
        }
      }
      else
        state = START;
      break;
    case RCV_DATA:
      if (msg == FLAG)
      {
        cnt--;
        char bcc_rcv = data_buf[cnt];
        char bcc_real = data_buf[0];
        for (int i = 1; i < cnt; i++)
          bcc_real = bcc_real ^ data_buf[i];
        if (bcc_real == bcc_rcv)
        {
          previous_s = !previous_s;
          send_data_response(fd, false, false);
        }
        else
        {
          even_bit = !even_bit;
          send_data_response(fd, true, false);
        }
        state = STOPS;
        break;
      }
      if (escape)
      {
        if (msg == ESC1)
        {
          strcat(data_buf, 0x7E); //save data (0x7E)
          cnt++;
        }
        else if (msg == ESC2)
        {
          strcat(data_buf, 0x7D); //save data (0x7D)
          cnt++;
        }
        else
          state = START; //ERRO
        escape = false;
        break;
      }
      char msg_string[255];
      sprintf(msg_string, "%d", msg);
      strcat(data_buf, msg_string); //save data (msg)
      cnt++;
      break;
    default:
      state = START;
      break;
    }
  }
}

void receive_set(int fd)
{
  receive_msg(fd, SET, A_RCV_RSP, false, NULL, false);
}

void receive_disc_rcv(int fd)
{
  receive_msg(fd, DISC, A_SND_CMD, false, NULL, false);
}

void receive_disc_snd(int fd)
{
  receive_msg(fd, DISC, A_RCV_CMD, false, NULL, false);
}

void receive_ua_rcv(int fd)
{
  receive_msg(fd, UA, A_SND_RSP, false, NULL, false);
}

void receive_ua_snd(int fd)
{
  receive_msg(fd, UA, A_RCV_RSP, false, NULL, false);
}

void receive_data(int fd, char *data_buf)
{
  if (previous_s == 0)
    receive_msg(fd, C_DATA_S1, A_SND_CMD, true, data_buf, false);
  else
    receive_msg(fd, C_DATA_S0, A_SND_CMD, true, data_buf, false);
}

void receive_data_rsp(int fd){
  char buf;
  printf("receive_msg\n");
  receive_msg(fd, RR_R0, A_RCV_RSP, false, &buf, true);
}

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
