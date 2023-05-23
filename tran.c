#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#define BAUDRATE B38400
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

static int numTries_Receiver, timeOut_Receiver;
static int numTries_Transmiter, timeOut_Transmiter;
static int TYPE; // TYPE = 0 (TX) | TYPE = 1 (RX)
volatile int STOP=FALSE;
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
    int fd, res;
    int n_timeouts=0;
    int flag=1, conta=1;

    void atende()                   // atende alarme
    {
        printf("alarme # %d\n", conta);
        flag=1;
        conta++;
    }

    struct termios oldtio, newtio;
    unsigned char buf[5];

    if ((argc < 2) ||
        ((strcmp("/dev/ttyS0", argv[1]) != 0) &&
        (strcmp("/dev/ttyS1", argv[1]) != 0))) {
        printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
        exit(1);
    }

    /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
    */

    fd = open(argv[1], O_RDWR | O_NOCTTY);
    if (fd < 0) { perror(argv[1]); exit(-1); }

    if (tcgetattr(fd, &oldtio) == -1) { /* save current port settings */
        perror("tcgetattr");
        exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME] = 0;   /* Disable the inter-character timer */
    newtio.c_cc[VMIN] = 1;    /* Set blocking read until 1 char received */

    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");
   
    
    (void) signal(SIGALRM, atende);  // instala rotina que atende interrupcao

    res = write(fd, SET, 5);
    printf("Sent SET package: %X %X %X %X %X\n", SET[0], SET[1], SET[2], SET[3], SET[4]);

    
    while (STOP==FALSE) /* loop for input */
    {       
      if((n_timeouts<3/*connectionParameters.numTries*/)&&(CONT!=STOP_RCV)){          
        if(flag){
          alarm(/*connectionParameters.timeOut*/3);
          flag = 0;
          
          if(n_timeouts!=0){
          // after 1st timeout
            res = write(fd,SET,5);
            printf("Resent SET package: %X %X %X %X %X\n", SET[0], SET[1], SET[2], SET[3], SET[4]);    // send SET again 
          }
        }
      }
      else{
        STOP=TRUE;
        alarm(0);
      }
    



     // Preparing the SET frame
    while (CONT != STOP_RCV) {
        memset(buf, 0, sizeof(buf)); 
        switch (CONT) {
            case START:
                CONT = FLAG_RCV;
                // Start the timeout
                alarm(3);
                break;
            case FLAG_RCV:
                if (buf[0] == FLAG){
                CONT = A_RCV;
                break;
                } 
                else{ 
                CONT = A_RCV;
                break;
                } 
            case A_RCV:
                if (buf[1] == A_TRANSMITTER){ 
                CONT = C_RCV;
                break;
                } 
                else{ 
                CONT = C_RCV;
                break;
                } 
            case C_RCV:
                if (buf[2] == C_UA){ 
                CONT = BCC_OK;
                break;
                } 
                else{
                CONT = BCC_OK;
                break;
                } 
            case BCC_OK:
                if (buf[3] == buf[0] ^ buf[1]){
                CONT = STOP_RCV;
                break;
                }
                else{
                CONT = START;
                break;
                } 

        }
    }


    if (CONT == STOP_RCV)
    { 
        STOP = TRUE;
        res = read(fd, buf, 5);
        if (res < 0) {
            perror("Error reading from serial port");
            exit(-1);
        }
        printf("Received UA package: %X %X %X %X %X\n", buf[0], buf[1], buf[2], buf[3], buf[4]);
        alarm(0);
    }

    if(n_timeouts>=3/*connectionParameters.numTries*/)
    {
        printf("Failed to start conection...\n");
        return -1;
    }         


   
    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
    }
}
