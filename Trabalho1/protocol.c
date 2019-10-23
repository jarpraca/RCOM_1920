/*Non-Canonical Input Processing*/

#include "protocol.h"

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

#define MAX_RETR 6
#define TIMEOUT 3

volatile int STOP = FALSE;
bool even_bit = 0;
bool previous_s = 0;
static int num_retr_set = 0;
static int num_retr_disc = 0;
static int num_retr_data = 0;
int fdG;
unsigned char msgG[1024];
int lengthG;
int state;

bool ua_received=false;
bool data_received=false;
bool disc_received=false;

void alarmSet()
{
  if (ua_received)
    return;
  if (num_retr_set < MAX_RETR)
  {
    printf("alarmSet %d \n", num_retr_set);
    send_resp(fdG, SET, A_SND_CMD);
    num_retr_set++;
  }
  else
  {
    exit(1);
  }
}

void alarmDisc()
{
 if (disc_received)
    return;
  if (num_retr_disc < MAX_RETR)
  {
    printf("alarmDisc %d \n", num_retr_disc);
    send_resp(fdG, DISC, A_SND_CMD);
    num_retr_disc++;
  }
  else
  {
    exit(1);
  }}

void alarmData()
{
  if (data_received)
    return;
  if (num_retr_data < MAX_RETR)
  {
    printf("alarmData %d \n", num_retr_data);
    send_msg(fdG, msgG, lengthG);
    num_retr_data++;
  }
  else
  {
    exit(1);
  }
}

int open_port(char* porta, struct termios *oldtio)
{
    int fd;
    struct termios newtio;

    /*
		Open serial port device for reading and writing and not as controlling tty
		because we don't want to get killed if linenoise sends CTRL-C.
	*/
    
    fd = open(porta, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        perror(porta);
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
    unsigned char buf2[5];
    buf2[0] = FLAG;
    buf2[1] = a;
    buf2[2] = c;
    buf2[3] = a ^ c;
    buf2[4] = FLAG;
    write(fd, buf2, 5);
}

void send_set(int fd)
{
    signal(SIGALRM, alarmSet);
    alarm(TIMEOUT);
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
    signal(SIGALRM, alarmDisc);
    alarm(TIMEOUT);
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

int send_msg(int fd,unsigned char* msg, int length)
{
    if(num_retr_data==0){
        signal(SIGALRM, alarmData);
        alarm(TIMEOUT);
        fdG=fd;
        for(int i=0; i<length;i++){
            msgG[i]=msg[i];
        }
        lengthG=length;
    }

    char buf2[7 + length*2];
    buf2[0] = FLAG;
    buf2[1] = A_SND_CMD;
    if (even_bit)
        buf2[2] = C_DATA_S0;
    else
        buf2[2] = C_DATA_S1;
    buf2[3] = (A_SND_CMD ^ buf2[2]);
    
    int cnt=0;
    for (int i = 0; i < length; i++)
    {
        if (msg[i] == 0x7E)
        {
            printf("message 7e");
            cnt++;
            buf2[4+i+cnt]=0x7D;
            buf2[4 + i+cnt+1] = 0x5E;
        }
        else if (msg[i] == 0x7D)
        {
            printf("message 7d");
            cnt++;
            buf2[4 + i + cnt] = 0x7D;
            buf2[4 + i + cnt + 1] = 0x5D;
        }
        else
            buf2[4 + i + cnt] = msg[i];
    }

    char bcc2 = msg[0];
    for (int i = 1; i < length; i++)
        bcc2 = (bcc2 ^ msg[i]);
        
    buf2[4 + length +cnt] = bcc2;
    buf2[5 + length +cnt] = FLAG;
    buf2[6 + length +cnt] = '\0';
    return write(fd, buf2, 6 + length+cnt);
}

int receive_msg(int fd, unsigned char c, unsigned char a, bool data, unsigned char* data_buf, bool data_resp)
{
    state = START;
    int res;
    bool escape = false;
    int cnt = 0;
    while (state != STOPS)
    { /* loop for input */
        unsigned char buf[2];
        res = read(fd, buf, 1); /* returns after 1 char have been input */
        buf[1]='\0';
        unsigned char msg = buf[0];
        switch (state)
        {
        case START:
            if (msg == FLAG){
                state = FLAG_RCV;
            }
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
                if ((int)msg == RR_R1 || (int)msg == RR_R0 || (int)msg == REJ_R1 || (int)msg == REJ_R0)
                {
                    data_buf[cnt] = msg;
                    state = C_RCV;
                    c=msg;
                    cnt++;
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
                    c=msg;
                }
                else
                {
                    even_bit = (bool)msg;
                    send_data_response(fd, true, (((bool)msg) == previous_s));
                    state = START;
                }
            }
            else
            {
                state=START;
            }
            
            break;
        case C_RCV:
            if (msg == (a^c))
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
                state = RCV_DATA;
                if (msg == ESC)
                    escape = true;
                // else if((char)msg == '\0'){
                //     data_buf[cnt] = '\0';
                //     cnt++; 
                //  }
                else {
                    char msg_string[255];
                    sprintf(msg_string, "%c", msg);
                    data_buf[cnt] = '\0';
                    strcat(data_buf, msg_string); //save data (msg)
                    cnt++;
                }
            }
            else{
                state = START;
            }
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
                    previous_s = even_bit;
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
                    strcat(data_buf, "0x7E"); //save data (0x7E)
                    cnt++;
                }
                else if (msg == ESC2)
                {
                    strcat(data_buf, "0x7D"); //save data (0x7D)
                    cnt++;
                }
                else
                    state = START; //ERRO
                escape = false;
                break;
            }
            
            char msg_string[255];
            sprintf(msg_string, "%c", msg);
            data_buf[cnt] = '\0';
            strcat(data_buf, msg_string); //save data (msg)
            cnt++;
            break;
        default:
            state = START;
            break;
        }
    }
    return cnt;
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
    disc_received = true;
}

void receive_ua_rcv(int fd)
{
    receive_msg(fd, UA, A_SND_RSP, false, NULL, false);
}

void receive_ua_snd(int fd)
{
    receive_msg(fd, UA, A_RCV_RSP, false, NULL, false);
    ua_received=true;
}

int receive_data(int fd, unsigned char* data_buf)
{
    int size;
    if (previous_s == 0)
        size= receive_msg(fd, C_DATA_S1, A_SND_CMD, true, data_buf, false);
    else
        size = receive_msg(fd, C_DATA_S0, A_SND_CMD, true, data_buf, false);
    return size;
}

void receive_data_rsp(int fd)
{
    char buf[1024];
    receive_msg(fd, RR_R0, A_RCV_RSP, false, buf, true);
    data_received = true;
}