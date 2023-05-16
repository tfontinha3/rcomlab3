/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define FLAG 0x5c
#define A_TRANSMITTER 0x01
#define A_RECEIVER 0x03
#define C_UA 0x07

typedef enum{
    INICIAL,
    WAIT_SET,
    SEND_UA,
    UA_SENT,
} CONTROLO;

CONTROLO CONT = INICIAL;

volatile int STOP=FALSE;

int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    unsigned int buf[5];

    if ( (argc < 2) ||
         ((strcmp("/dev/ttyS0", argv[1])!=0) &&
          (strcmp("/dev/ttyS1", argv[1])!=0) )) {
        printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
        exit(1);
    }

    /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
    */

    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd < 0) { perror(argv[1]); exit(-1); }

    if (tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
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
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 1 char received */

    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd,TCSANOW,&newtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");

    // Preparing the UA frame
    buf[0] = FLAG;
    buf[1] = A_TRANSMITTER;
    buf[2] = C_UA;
    buf[3] = buf[0] ^ buf[1];
    buf[4] = FLAG;

    res = read(fd,buf,5);   /* returns after 1 char has been input */                                  
    printf("Received SET package: %X %X %X %X %X\n", buf[0], buf[1], buf[2], buf[3], buf[4]);

    res = write(fd,buf,5); 
    printf("Sent UA package: %X %X %X %X %X\n", buf[0], buf[1], buf[2], buf[3], buf[4]);
   

    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}
