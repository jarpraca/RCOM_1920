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

    unsigned char *buffer;
    buffer= malloc(sizeof(char)*4096);
   // buffer[0]=' ';
    unsigned char *filename;
    filename= malloc(sizeof(char)*128);
    int size,res, i=0;
    size = llreadFile(fd, buffer, filename);

    llclose(fd);
    //if(filename[strlen(filename)-1] == '?')
        filename[strlen(filename)-1]='\0';
    printf("filename: %s\n",filename);

    FILE* file;
    file = fopen(filename, "w");
    printf("file: %s\n",file);
  //  do{
    //    printf("res: %d\n", res);
        res= fwrite(buffer, sizeof(char), strlen(buffer)+1, file);
   // }while(res==strlen(buffer)+1);
    printf("Final result: %s \n", buffer);
   
    free(buffer);
    free(filename);

    return 0;
}