#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define FLAG 0x5c
#define A_TRANSMITTER 0x01
#define A_RECEIVER 0x03
#define C_UA 0x07
#define C_SET 0x03
#define C_RR 0x33
#define C_I0 0x00
#define C_DISC 0

volatile int STOP=FALSE;

static int numTries_Receiver, timeOut_Receiver;
static int numTries_Transmiter, timeOut_Transmiter;
static int TYPE; // TYPE = 0 (TX) | TYPE = 1 (RX)
unsigned char SET[] = {FLAG, A_TRANSMITTER, C_SET, FLAG^A_TRANSMITTER, FLAG};
unsigned char UA[] = {FLAG, A_TRANSMITTER, C_UA, FLAG^A_TRANSMITTER, FLAG};

typedef enum{
    START,
    FLAG_RCV,
    A_RCV,
    C_RCV,
    BCC_OK,
    STOP_RCV
} CONTROLO;

CONTROLO CONT = START;


    
int main(int argc, char** argv){    
    
    int fd,c, res;
    struct termios oldtio,newtio;
    unsigned char buf[5];

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
   
    res = read(fd,buf,5);
    
    while(CONT != STOP_RCV){
      switch(CONT){
        case START:
        CONT = FLAG_RCV;
        break;
       
        
        case FLAG_RCV:
        
            if(buf[0] !=FLAG)
            {
                CONT = START;
                break;
            }
            
            
            else
            {
                CONT = A_RCV;
                break;
            }
        
        
        case  A_RCV:
        
            if(buf[1] != A_TRANSMITTER)
            {
                CONT = START;
                break;
            }
            
            else
            {
                CONT = C_RCV;
                break;
            }
        
        case C_RCV : 
        
            if(buf[2] != C_SET)
            {
                CONT = START;
                break;
            }

            
            else
            {
                CONT = BCC_OK;
                break;
            }
        
       
        case BCC_OK:
        
            if(buf[3] != buf[0]^buf[1])
            {
                CONT = START;
                break;
            }
            
            else
            {
                CONT = STOP_RCV;
                break;
            }

        }
    }


    if(CONT == STOP_RCV)
    {
        printf("Received SET package: %X %X %X %X %X\n", buf[0], buf[1], buf[2], buf[3], buf[4]); 
        STOP = TRUE;
        res = write(fd,UA,5);
        printf("Sent UA package: %X %X %X %X %X\n", UA[0], UA[1], UA[2], UA[3], UA[4]); 
    }

   
    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}