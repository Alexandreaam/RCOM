/*Non-Canonical Input Processing*/

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
		printf("New:\n\n");
		res = read(fd,receive,1);
		printf("Receive - %x\n", receive[0]);
		STOP = stateMachineRe(aux, receive[0], &state);
		printf("state %d\n", state);
		printf("Length - %d\n", length);
		aux[length]=receive[0];
		for(j=0;j<length+1;j++){
			printf("%x", aux[j]);
		}
		printf("\n");	
		length++;
	}
	printf("state %d\n", state);
	for(j=0;j<length;j++){
		printf("value %x\n", aux[j]);
	}	

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

int readControlPkg(int fd, char* fileName, int* fileSize, int* pathSize){
	printf("Receiving control pkg....\n");
	int sizeControlPkg=0,i=0,j=0;
	unsigned char controlPkg[255];
	int llreadVal = llread(fd,&controlPkg,&sizeControlPkg);
	if(llreadVal==-1){
	  printf("Error no read");
	  return -1;
	}

	unsigned char sizeFile[255];
	for(i=3;i<sizeControlPkg;i++){
		if(controlPkg[i]=='#')
			break;
		else
			sizeFile[i-3]=controlPkg[i];	
	}
	
	*fileSize = atoi(sizeFile);

	int sizePath = (int)(controlPkg[i+2]);
	printf("%d\n",(int)(controlPkg[i+2]));
	*pathSize = sizePath;

	printf("File size: %d\n", *fileSize);
	j=0;
	for(i=i+3;i<sizeControlPkg;i++){
		fileName[j]=controlPkg[i];
		j++;
	}

	printf("File path: ");
	for(i=0;i<j;i++){
		printf("%c",fileName[i]);
	}
	printf("\n");
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



int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    char buf[255];

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
    if (fd <0) {perror(argv[1]); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
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
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */



  /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) proximo(s) caracter(es)
  */



    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");


    //-----------------------------------// 

    if(llopen(fd,res)==-1){
      printf("Error establishing connection...");
    }

	//Read control package
	char aux[255];
    int fileSize, pathSize=0;
    int readControlVal = readControlPkg(fd,&aux,&fileSize,&pathSize);
	int j=0;
	unsigned char *filePath;
	filePath = (char *) malloc(pathSize * sizeof(char));
	for(j=0;j<pathSize;j++){
		filePath[j]=aux[j];
	}
	
	
	//create file
	char data[255],temp[255];
	int num_read = 0,i = 0,size=0;
	FILE *f = fopen(filePath, "wb");
	if (f == NULL)
	{
		printf("Error opening file!\n");
		exit(1);
	}
	int k=0,nseq=1;
	//RECEIVE DATA
	while(1){
		printf("\nTrama n %d\n", ++k);
		int llreadVal = llread(fd,&temp,&size);
		if(llreadVal==0){
			for(i=4;i<size;i++){
				//data[num_read]=temp[i];
				/*for(j=0;j<size;j++)
					printf("%x",temp[j]);
				printf("\n");*/

				fprintf(f,"%c",temp[i]);
				num_read++;
			}
		}
		else if(llreadVal==-1){
		  //tcflush(fd, TCIOFLUSH);
		  printf("Error no read");
		} else if(llreadVal==2){
			printf("The file was sucessfully transfered!\n");
			break;
		}
	}
	if(llclose(fd)==-1){
  		printf("Error no close");
	}
	fclose(f);	



	//----------------------------------//



    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}
