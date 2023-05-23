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
#define C_RR0 0x01
#define C_RR1 0x21
#define C_REJ0 0x05
#define C_REJ1 0x25
#define C_S0 0x00
#define C_S1 0x02
#define C_DISC 0x0b
#define ESC 0x5d
#define XOR 0x20

static int numTries_Receiver, timeOut_Receiver;
static int numTries_Transmiter, timeOut_Transmiter;
static int TYPE; // TYPE = 0 (TX) | TYPE = 1 (RX)
volatile int STOP=FALSE;
unsigned char SET[] = {FLAG, A_TRANSMITTER, C_SET, FLAG^A_TRANSMITTER, FLAG};
unsigned char UA[] = {FLAG, A_TRANSMITTER, C_UA, FLAG^A_TRANSMITTER, FLAG};

typedef enum{
    STATE0,
    STATE1,
    STATE2,
    STATE3,
    STATE4,
    STATE5,
    STATE6,
    STATE7,
    STATE8
} CONTROLO;

CONTROLO STATE = STATE0;


int llwrite(char* buf, int bufSize){
    // controlo dos parametros
    if((!buf)||(bufSize<0)||(bufSize>MAX_PAYLOAD_SIZE)) return -1;

    conta = 1;  //  PARA AS ESTATISTICAS
        //sleep(1);
    /* Reeniciar timeouts */
    timeouts = 0, flag_ = 1;

    char frame[7+2+MAX_PAYLOAD_SIZE*2];  /*frame auxiliar*/
    int STOP = FALSE, count = 0, i = 0, flag_encontrada = 0, bufSize_aux, STATE = 0, Ns = 0;
    char buffer, estados[5];
    char REJ0[] = {FLAG, A_TRANSMITTER, C_REJ0, (A_TRANSMITTER^C_REJ0), FLAG};  
    char REJ1[] = {FLAG, A_TRANSMITTER, C_REJ1, (A_TRANSMITTER^C_REJ1), FLAG};  
    char RR0[] = {FLAG, A_TRANSMITTER, C_RR0, (A_TRANSMITTER^C_RR0), FLAG}; //MAL OS COISOS
    char RR1[] = {FLAG, A_TRANSMITTER, C_RR1, (A_TRANSMITTER^C_RR1), FLAG};


    char BCC2 = 0x00;
    for(int k = 0; k < frameSize; k++){
    BCC2 ^= frame[k];
    
    }

    frame[0] = FLAG;
    frame[1] = A_TRANSMITTER;
    frame[2] = C_I0;
    frame[3] = frame[1]^frame[2];
    for(int i = 0; i < bufSize; i++){
        frame[i+4] = buf[i];
    }
      
    frame[bufSize + 4] = BCC2;
    frame[bufSize + 5] = FLAG;

    //stuffing   

    if((!buf)||(bufSize<0)) return -1;
    
    char aux1, aux2;

    for(int i = 3; i < bufSize-1; i++){
        if(buf[i] == ESC){
            bufSize++;
            aux1 = buf[i+1];
            buf[i+1] = (ESC^XOR);    
            for(int j = i+2; j <= bufSize ;j++){
                aux2 = buf[j];
                buf[j]=aux1;
                aux1 = aux2;
            }
        }
    }

    for(int i = 3; i<bufSize-1; i++){
        if(buf[i] == FLAG){
            buf[i]=ESC;
            bufSize++;
            aux1=buf[i+1];
            buf[i+1]=(FLAG^XOR); 
            for(int j = i+2; j<=bufSize; j++){
                aux2 = buf[j];
                buf[j]=aux1;
                aux1=aux2;
            }
        }
    }

    //escreve
    int res = write(fd, frame, bufSize); 

    //timeouts
    (void) signal(SIGALRM, timeout_);
    int ciclo = 1;
    alarm(0);
    while (STOP==FALSE) /* loop for input */ 
    {   
        if(timeouts < 4/*numTries_Transmiter*/){
            if(flag_){
                alarm(3/*timeOut_Transmiter*/);
                flag_ = 0;
                if(timeouts!=0){           // after 1st timeout
                    //printf("Sending again due to timeout. \n");
                    res = write(fd, frame, bufSize);    // send all the frame again   
                    if(res == -1) return -1;

                }
            }
        }
        else{
            alarm(0);
            STOP = TRUE;
        }
            
        res = read(fd, buffer, 1);//printf("read: %02x\n", buffer);
        if(res == -1) return -1;
        if(res == 0){
            //printf("Timeout\n");
        }
        
        estados[STATE] = buffer;
        switch (STATE)
        {
        case STATE0:
            if(buffer == FLAG) 
            STATE = STATE1;
            break;
            else STATE = STATE0;
            break;
        case STATE1:
            if(buffer == FLAG) STATE = STATE1;
            else if(buffer == A_TRANSMITTER) STATE = STATE2;
            else STATE = STATE0; 
            break;
        case STATE2:
            if(buffer == FLAG) 
            STATE = STATE1;
            break;
            else if((buffer == C_RR0)||(buffer == C_RR1)){    // "vai ler um RR"
                if((Ns == C_S0)&&(buffer == C_RR1)) 
                STATE = STATE3;
                break;
                else if((Ns == C_S1)&&(buffer == C_RR0)) 
                STATE = STATE3;
                break;
                else 
                STATE = STATE0;
                break;
            }
            else if((buffer == C_REJ0)||(buffer == C_REJ1)){  // "vai ler um REJ
                if((Ns == C_S0)&&(buffer == C_REJ0)) 
                STATE = STATE6;
                break;
                else if((Ns == C_S1)&&(buffer == C_REJ1)) 
                STATE = STATE6;
                break;
                else 
                STATE = STATE0;
                break;
            }   
            else STATE = STATE0; 
            break;
        case STATE3:
            if(buffer == FLAG) 
            STATE = STATE1;
            break;
            else if((buffer == (C_RR0^A_TRANSMITTER))||(buffer == (C_RR1^A_TRANSMITTER))) 
            STATE = STATE4;
            break;
            else STATE = STATE0; 
            break;
        case STATE4:
            if(buffer == FLAG) 
            STATE = STATE5;
            break;
            else 
            STATE = STATE0; 
            break;
        case STATE5:
            STOP = TRUE;
            Ns = 1;
            break;
        case STATE6:
            if(buffer == FLAG) 
            STATE = STATE1;
            break;
            else if((buffer == (C_REJ0^A_TRANSMITTER))||(buffer == (C_REJ1^A_TRANSMITTER))) 
            STATE = STATE7;
            break;
            else STATE = STATE0; 
            break;
        case STATE7:
            if(buffer == FLAG) STATE = STATE8;
            else STATE = STATE0; 
            break;
        case STATE8:
            //Recebeu um REJ. Reevia e volta ao estado 
            alarm(0);
            res = write(fd, frame, bufSize);    // send all the frame again   
            if(res == -1) return -1;
            printf("REJ Recebido! A reenviar pacote \n");
            timeouts++;
            STATE = STATE0;
            break;
        }
        
        //printf("STATE: %d . AUX: %02x\n", STATE, buffer);
        if(STATE == STATE5) {
            printf("RR Recebido\n");
            Ns = 0;
            break;
        }
        
        
    }
    alarm(0);

}

llread(char* packet)
{

}
