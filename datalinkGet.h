#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define FLAG 0x7e
#define A 0x03
#define A_DISC 0x01
#define C_SET 0x03
#define C_UA 0x07
#define C_RR 0x04
#define C_REJ 0x05
#define ESC 0x7d
#define ESCSTF 0x5d
#define FLAGSTF 0x5e


#define C_RR_0 0x05
#define C_RR_1  0x85
#define C_REJ_0  0x01
#define C_REJ_1  0x81
#define C_DISC 0x0B
#define C_0 0x00
#define C_1 0x40
 

unsigned char UA[5] = {FLAG, A, C_UA, A^C_UA, FLAG};;
unsigned char RR[5] = {FLAG, A, C_RR, A^C_RR, FLAG};
unsigned char REJ[5] = {FLAG, A, C_REJ, A^C_REJ, FLAG};
unsigned char DISC[5] = {FLAG, A_DISC, C_DISC, A_DISC^C_DISC, FLAG};
unsigned char RR_0[5] = {FLAG, A, C_RR_0, A^C_RR_0, FLAG};
unsigned char RR_1[5] = {FLAG, A, C_RR_1, A^C_RR_1, FLAG};
unsigned char REJ_0[5] = {FLAG, A, C_REJ_0, A^C_REJ_0, FLAG};
unsigned char REJ_1[5] = {FLAG, A, C_REJ_1, A^C_REJ_1, FLAG};

volatile int STOP=FALSE;
int Seq = 0;



int stateMachine(char *aux, unsigned char value, int *state){
//printf("state %d\n", *state);
	switch(*state){
			case 0:
				if(value == FLAG){
					aux[0] = FLAG;
					*state = 1;	
				}
				break;
			case 1:
				if(value == FLAG){
					aux[0] = FLAG;
					*state = 1;
				}
				else if(value == A){
					aux[1] = A;
					*state = 2;
				}
				else
					*state = 0;	
				break;
			case 2:
				if(value == FLAG){
					aux[0] = FLAG;
					*state = 1;
				}
				else if(value == C_SET){
					aux[2] = C_SET;
					*state = 3;
				}
				else if(value == C_UA){
					aux[2] = C_UA;
					*state = 7;
				}
				else
					*state = 0;	
				break;
			case 3:
				if(value == FLAG){
					aux[0] = FLAG;
					*state = 1;
				}
				else if(value == A^C_SET){
					aux[3] = A^C_SET;
					*state = 4;
				}
				else
					*state = 0;	
				break;
			case 4:
				if(value == FLAG){
					aux[4] = FLAG;
					*state = 5;
					return 1;
				}
				else
					*state = 0;
				break;
			case 7:
				if(value == FLAG){
					aux[0] = FLAG;
					*state = 1;
				}
				else if(value == A^C_UA){
					aux[3] = A^C_UA;
					*state = 8;
				}
				else
					*state = 0;	
				break;
			case 8:
				if(value == FLAG){
					aux[4] = FLAG;
					*state = 8;
					return 1;
				}
				else
					*state = 0;
				break;
	}
	return 0;
}

int stateMachineRe(char *aux, char value, int *state){
//printf("state %d\n", *state);
	switch(*state){
			case 0:
				if(value == FLAG){
					aux[0] = FLAG;
					*state = 1;	
				}
				break;
			case 1:
				if(value == FLAG){
					aux[0] = FLAG;
					*state = 1;
				}
				else if(value == A){
					aux[1] = A;
					*state = 2;
				}
				else if(value == A_DISC){
					aux[1] = A_DISC;
					*state = 2;
				}
				else
					*state = 0;	
				break;
			case 2:
				if(value == FLAG){
					aux[0] = FLAG;
					*state = 1;
				}
				else if(value == C_SET){
					aux[2] = C_SET;
					*state = 3;
				}
				else if(value == C_0){
					aux[2] = C_0;
					*state = 3;
				}
				else if(value == C_1){
					aux[2] = C_1;
					*state = 3;
				}
				else if(value == C_DISC){
					aux[2] = C_DISC;
					*state = 5;
				}
				else
					*state = 0;	
				break;
			case 3:
				if(value == FLAG){
					aux[0] = FLAG;
					*state = 1;
				}
				else if(value == A^C_SET){
					aux[3] = A^C_SET;
					*state = 4;
				}
				else if(value == A^C_0){
					aux[3] = A^C_0;
					*state = 4;
				}
				else if(value == A^C_1){
					aux[3] = A^C_1;
					*state = 4;
				}
				else
					*state = 0;	
				break;
			case 4:
				if(value == FLAG){
					*state = 5;
					return 1;
				}
				break;
			case 5:
				if(value == FLAG){
					aux[0] = FLAG;
					*state = 1;
				}
				else if(value == A^C_DISC){
					aux[3] = A^C_DISC;
					*state = 6;
				}
				else if(value == A_DISC^C_DISC){
					aux[3] = A_DISC^C_DISC;
					*state = 6;
				}
				else
					*state = 0;	
				break;
			case 6: //estado DISC
				if(value == FLAG){
					*state = 6;
					return 1;
				}
				
				break;
				
	}
	return 0;
}

int destuffing(unsigned char* trama, int pos){ 

	unsigned char newTrama[255];
	int i=0;
	for(i=0;i<255;i++){
		if(i<pos){
			newTrama[i]=trama[i];
		}
		else 
			newTrama[i]=trama[i+1];
	}
	for(i=0;i<255;i++){
		trama[i] = newTrama[i];
	}

	return 0;
}

int llopen(int fd, int res){
	printf("A receber pedido de ligacao...\n");
	
	unsigned char receive[1], aux[5];
    int value = 0, state=0, j=0;

	while(STOP == FALSE) {
		res = read(fd,receive,1);
		printf("Recebi: %x\n", receive[0]);
		STOP = stateMachine(aux, receive[0], &state);   
	}

	printf("Pedido de ligacao recebido:\n");
	for(j=0; j<5; j++){
		printf("%x\n", aux[j]);
	}	

	int i = write(fd,UA,5);
	printf("Enviados %d bytes de ACK\n", i);
	return value;
}

int llread(int fd, char *data, int* size){

    unsigned char receive[1];

	int state=0;
	unsigned char aux[255];
	int j=0, i=0;
	int res;

    int length = 0;
	STOP = FALSE;
	while(STOP == FALSE) {
		if(state==0)
			length=0;
		//printf("New:\n\n");
		res = read(fd,receive,1);
		//printf("Receive - %x\n", receive[0]);
		STOP = stateMachineRe(aux, receive[0], &state);
		//printf("state %d\n", state);
		//printf("Length - %d\n", length);
		aux[length]=receive[0];
		/*for(j=0;j<length+1;j++){
			printf("%x", aux[j]);
		}*/
		//printf("\n");	
		length++;
	}
	printf("state %d\n", state);
	/*for(j=0;j<length;j++){
		printf("value %x\n", aux[j]);
	}	*/

	//if(state==6) return 2;

	if(aux[2]==C_0 && Seq==1){
		write(fd,RR_1,5);
		printf("Emissor failed to receive previous Ack. ReSent RR_1: \n");
		return 3;
	}
	else if(aux[2]==C_1 && Seq==0){
		write(fd,RR_0,5);
		printf("Emissor failed to receive previous Ack. ReSent RR_0: \n");
		return 3;
	}

	int x;
	j = 0;
	while(1){
		//printf("Este : %x\n", aux[j]);
		if(j>=length-1) break;
		else if(aux[j]==ESC && aux[j+1]==FLAGSTF)
		{
		    x = destuffing(&aux, j);
			aux[j] = FLAG;
			//printf("Meti %x no %d\n", FLAG,j);
		    length--;
		}
		else if(aux[j]==ESC && aux[j+1]==ESCSTF)
		{
		    x = destuffing(&aux, j);
			aux[j] = ESC;
			//printf("Meti %x no %d\n", ESC,j);
		    length--;
		}
		j++;
	//printf("Length: %d\n", length);
	//printf("J: %d\n", j);
    }

    unsigned char BCC2=0;
    for(j=4; j<length-1; j++)
   {
        if(j==length-3)
            break;
        BCC2^=aux[j];
   }
	printf("bcc %x\n",BCC2);
	
		
   if(BCC2 == aux[length-2]) {
		if(aux[4]==0x03 && state==5){
			printf("Receive trama (I) end\n");
			return 2;
		}

		for(j=4;j<length-2;j++){
			data[j-4]=aux[j];	
		}
		*size = j-4;
			printf("Recebi : %x \n", data[0]);
			if(data[0]!=0x01)
				return 0;
			
			printf("Recebi : %d bytes\n", *size-4);
			printf("Recebi C: %x\n", aux[2]);
			if(aux[2]==C_0 && Seq==0){
        			write(fd,RR_1,5);
				printf("Sent RR_1: \n");
			}
			else if(aux[2]==C_1 && Seq==1){
				write(fd,RR_0,5);
				printf("Sent RR_0: \n");
			}
	   		Seq = 1 - Seq;
    }
    else {
        printf("Error BBC2: \n");
		if(Seq==1){
				write(fd,REJ_1,5);
				printf("Sent REJ_1: \n");
		}
		else if(Seq==0){
			write(fd,REJ_0,5);
			printf("Sent REJ_0: \n");
		}
		return -1;
    }
    return 0;
}

int llclose(int fd){
	
	unsigned char receive[1];
	int state=0;
	unsigned char aux[255];
	int j=0;
	int res;
    int length = 0;
	STOP = FALSE;

	while(STOP == FALSE) {
		res = read(fd,receive,1);
		printf("%x\n", receive[0]);
		STOP = stateMachineRe(aux, receive[0], &state);
		aux[length]=receive[0];
		length++;
	}
	
	if(state==6){
		printf("Sent DISC: \n");
		int bytes = write(fd,DISC,5);
		state = 0;
	}
	
	STOP = FALSE;
	while(STOP == FALSE) {
		res = read(fd,receive,1);
		printf("%x\n", receive[0]);
		STOP = stateMachine(aux, receive[0], &state);
		aux[length]=receive[0];
		length++;
	}

	if(state==8){
		printf("Connection terminated.\n");
		return 1;
	}
	return -1;
}
