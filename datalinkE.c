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
#define C_RR_0 0x05
#define C_RR_1 0x85
#define C_REJ_0 0x01
#define C_REJ_1 0x81
#define C_DISC 0x0B
#define C_0 0x00
#define C_1 0x40
#define ESC 0x7d
#define ESCSTF 0x5d
#define FLAGSTF 0x5e
#define pckgsize 64

unsigned char SET[5] = {FLAG, A, C_SET, A^C_SET, FLAG};
unsigned char UA[5] = {FLAG, A, C_UA, A^C_UA, FLAG};
unsigned char RR[5] = {FLAG, A, C_RR, A^C_RR, FLAG};
unsigned char REJ[5] = {FLAG, A, C_REJ, A^C_REJ, FLAG};
//unsigned char DISC[5] = {FLAG, A, C_DISC, A^C_DISC, FLAG};
unsigned char DISC[5] = {FLAG, A_DISC, C_DISC, A_DISC^C_DISC, FLAG};

unsigned char RR_0[5] = {FLAG, A, C_RR_0, A^C_RR_0, FLAG};
unsigned char RR_1[5] = {FLAG, A, C_RR_1, A^C_RR_1, FLAG};
unsigned char REJ_0[5] = {FLAG, A, C_REJ_0, A^C_REJ_0, FLAG};
unsigned char REJ_1[5] = {FLAG, A, C_REJ_1, A^C_REJ_1, FLAG};
volatile int STOP=FALSE;

int Seq=0, alarm_counter=0;
int flag_alarm = 0;
int stateMachine(char *aux, unsigned char value, int *state){
	printf("State: %d\n", *state);
	printf("Value: %x\n", value);
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
			aux[2] = C_SET;
			*state = 6;
		}
		else if(value == C_RR_0){
			aux[2] = C_RR_0;
			*state = 12;
		}
		else if(value == C_RR_1){
			aux[2] = C_RR_1;
			*state = 12;
		}
		else if(value == C_REJ_0){
        	aux[2] = C_REJ_0;
        	*state = 15;
        }
		else if(value == C_REJ_1){
        	aux[2] = C_REJ_1;
        	*state = 15;
        }
		else if(value == C_DISC){
			aux[2] = C_DISC;
			*state = 9;
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

		case 6:
		if(value == FLAG){
			aux[0] = FLAG;
			*state = 1;
		}
		else if(value == A^C_UA){
			aux[3] = A^C_UA;
			*state = 7;
		}
		else
		*state = 0;
		break;

		case 7:
		if(value == FLAG){
			aux[4] = FLAG;
			*state = 8;
			return 1;
		}
		else
		*state = 0;
		break;

		case 9:
		if(value == FLAG){
			aux[0] = FLAG;
			*state = 1;
		}
		else if(value == A^C_DISC){
			aux[3] = A^C_DISC;
			*state = 10;
		}
		else if(value == A_DISC^C_DISC){
			aux[3] = A_DISC^C_DISC;
			*state = 10;
		}
		break;
		case 10:
		if(value == FLAG){
			aux[4] = FLAG;
			*state = 11;
			return 1;
		}
		else
		*state = 0;
		break;

		case 12:
		if(value == FLAG){
			aux[0] = FLAG;
			*state = 1;
		}
		else if(value == A^C_RR_0){
			aux[3] = A^C_RR_0;
			*state = 13;
		}
		else if(value == A^C_RR_1){
			aux[3] = A^C_RR_1;
			*state = 13;
		}
		else
		*state = 0;
		break;

		case 13:
		if(value == FLAG){
			aux[4] = FLAG;
			*state = 14;
			return 1;
		}
		else
		*state = 0;
		break;

		case 15:
		if(value == FLAG){
			aux[0] = FLAG;
			*state = 1;
		}
		else if(value == A^C_REJ_0){
			aux[3] = A^C_REJ_0;
			*state = 16;
		}
		else if(value == A^C_REJ_1){
			aux[3] = A^C_REJ_1;
			*state = 16;
		}
		else
		*state = 0;
		break;

		case 16:
		if(value == FLAG){
			aux[4] = FLAG;
			*state = 17;
			return 1;
		}
		else
		*state = 0;
		break;
	}
	return 0;
}


int tramastf (char* trama, int step, char stf) {
	char tramabuff[255];
	int i=0;
	for (i=0;i<255;i++)
	{
	tramabuff[i]=trama[i];
	}
	for (i=step;i<(255);i++)
	{
	trama[i+1]=tramabuff[i];
	}

	trama[step] = stf;
	printf("Sucesso stuffing\n");
	return 0;
}

int trama_append(char *trama, int step, char *buf, int buffer_length) {
	int i=0;	

	for (i=0; i < buffer_length; i++) {
		trama[step+i] = buf[i];
	}

	printf("Appended trama\n\n");

	return 0;
}
int llwrite(int fd, unsigned char* buffer, int length){
    int i, j=4, FLAGPOS=0;
	for(i=0; i < length; i++) {
		//printf("Buffer Write %d: %x\n", i, buffer[i]);
	}
    unsigned char trama[255];
    trama[0] = FLAG;
    trama[1] = A;
	if(Seq==0){
		trama[2] = C_0;
    	trama[3] = A^C_0;
	}
	else{ 
		trama[2] = C_1;
    	trama[3] = A^C_0;
	}
    unsigned char BCC2=0;
    for(i=0;i<length-1;i++)
    {
   	 BCC2^=buffer[i];
    }
    trama[length+4] = BCC2;
    trama[length+5] = FLAG;

	/*for(i=0; i < length+6; i++) {
		printf("Trama_check: %x\n", trama[i]);
	}*/

	trama_append(trama, 4, buffer, length);

	for(i=0; i < length+6; i++) {
		//printf("New Trama_check: %x\n", trama[i]);
	}

    FLAGPOS = length+5;
    while(1)
    {
        if(j==FLAGPOS-1) break;
        else if(trama[j]==FLAG) {
	        trama[j]=ESC;
			tramastf(trama, j+1, FLAGSTF);
    	    FLAGPOS++;
        }
        else if(trama[j]==ESC) {
	        trama[j]=ESC;
			tramastf(trama, j+1, ESCSTF);
    	    FLAGPOS++;
        }
        j++;
    }

	//printf("TRAMA SIZE: %d\n", sizeof(trama)*sizeof(char));

    int lli = write(fd, trama, FLAGPOS+1);

    int inc=0;
    while(inc < FLAGPOS+1) {
    		//printf("%x\n", trama[inc]);
    		inc++;
    	}
    
	printf("(%d bytes written)\n\n", lli);
    
	return 0;
}

int sendCtrlPckg(int fd, int ctrl_field, char* filepath, int filesize) {
	int i, j=3;	
	unsigned char file_size[16] = {};
	snprintf(file_size, 20000, "%d", filesize);
	
	printf("Filepath: %s\nFilepath size: %d\nFilesize: %d\n", filepath, strlen(filepath), strlen(file_size));
	int pckg_size = 6 + strlen(filepath) + strlen(file_size);
	unsigned char ctrl_pckg[pckg_size];

	ctrl_pckg[0] = 0x02;
	ctrl_pckg[1] = 0x00; // FILE SIZE PARAM
	ctrl_pckg[2] = (char)strlen(file_size);

	for(i = 0; i < strlen(file_size); i++){
		ctrl_pckg[j] = file_size[i];
		j++;
	}
	ctrl_pckg[j] = '#';
	j++;
	ctrl_pckg[j] = 0x01; // FILE NAME PARAM
	j++;	
	ctrl_pckg[j] = (char)strlen(filepath);
	j++;

	for(i = 0; i < strlen(filepath); i++) {
		ctrl_pckg[j] = filepath[i];
		j++;
	}

	if (llwrite(fd, ctrl_pckg, pckg_size) < 0) {
		printf("Failed to write control package\n");
		return -1;
	}

	return 0;
}


int sendDataPckg(int fd, int seq_nr, char* buffer, int buffer_size) {
	unsigned char package[buffer_size + 4];
	package[0] = 0x01;
	package[1] = seq_nr;
	package[2] = (buffer_size / 256) + '0';
	package[3] = (buffer_size % 256) + '0';
	int i;

	int m, nm=0;
	for(m=0; m < buffer_size; m++) {
		package[m+4] = buffer[nm];
		nm++;
	}  
	int write_value = llwrite(fd, package, buffer_size + 4);
	if(write_value != 0) {
		printf("Failed to write data package\n");
		return -1;
	}
	
	return 0;
}

void atende() {
	flag_alarm = 1;
	alarm_counter++;
}

int llopen(int fd,int res){
	printf("A enviar pedido de ligacao...\n");
	
	int i = write(fd, SET, 5);
	printf("(%d bytes written)\n\n", i);
	if(i<0)
		return -1;
	
	int state=0, increm=0;
	unsigned char aux[5];
    unsigned char receive[5];

	printf("Waiting for UA...\n");
	STOP = FALSE;
	alarm(3);
	while(STOP == FALSE) {
		res = read(fd,receive,1);
		//printf("\nRead n%d\nreceive = %x\n", increm, receive[0]);
		STOP = stateMachine(aux, receive[0], &state);
		if(flag_alarm == 1){
			flag_alarm = 0;
			i = write(fd, SET, 5);
			printf("Disparou alarme e re-enviei\n");
			alarm(3);
		}
		increm++;
	}

	printf("Acknowledge received.\n");

	alarm(0);	

	int j=0;
	if(state==8) {
	printf("RECEIVED UA: ");
	while(j < 5) {
		printf("%x", aux[j]);
		j++;
	}
	printf("\n");
}

	return 0;
}

int llread(int fd, char * buffer)
{
	int llstate=0, llincrem=0, state=0, res=0;
	unsigned char aux[5];
	unsigned char receive[5];
	printf("testar received\n");
	STOP = FALSE;
	while(STOP == FALSE) {
		res = read(fd,receive,1);

		//printf("\nLLRead n%d\nreceive = %x\n", llincrem, receive[0]);
		
		STOP = stateMachine(aux, receive[0], &state);
		if(flag_alarm == 1){
			return -3;
		}
		llincrem++;
	}
	printf("done\n\n");

    if(state == 14) {
		if(aux[2]==C_RR_0){
		int j=0;
	  /*  printf("RECEIVED RR_0: ");
	    while(j < 5) {
		    printf("%x", aux[j]);
		    j++;
	    }*/
		return 1;
}
		else if(aux[2]==C_RR_1){
		int j=0;
	   /* printf("RECEIVED RR_1: ");
	    while(j < 5) {
		    printf("%x", aux[j]);
		    j++;
	    }*/
		return 2;
}
	    
    }
    else if(state == 17) {
		if(aux[2]==C_REJ_0){
		int j=0;
      /*  printf("RECEIVED REJ_0: ");
        while(j < 5) {
            printf("%x", aux[j]);
            j++;
        }*/
		return -1;
}
		if(aux[2]==C_REJ_1){
        int j=0;
       /* printf("RECEIVED REJ_1: ");
        while(j < 5) {
            printf("%x", aux[j]);
            j++;
        }*/
		return -2;
		}
    }
    else printf("FAILED TO RECEIVE PROPER TRAMA\n");

	printf("\n");

	return 0;
}

int llclose(int fd) {
	int state=0, increm=0;
	int res=0;
	int i = write(fd, DISC, 5);
	printf("\nSent DISC: %d bytes\n", i);

	unsigned char aux[5];
	unsigned char receive[5];
	STOP = FALSE;
	
	while(STOP == FALSE) {
		res = read(fd,receive,1);

		//printf("\nRead DISC n%d\nreceive = %x\n", increm, receive[0]);

		STOP = stateMachine(aux, receive[0], &state);
		increm++;
	}

	i = write(fd, UA, 5);
	printf("\nSent UA: %d bytes\n", i);

	return 0;
}

int main(int argc, char** argv)
{
    int fd, res;
	int c;
    struct termios oldtio,newtio;
    char buf[255];
    int i, sum = 0, speed = 0;
    
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

    newtio.c_cc[VTIME]    = 1;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 0;   /* blocking read until 5 chars received */



  /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) próximo(s) caracter(es)
  */



    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");

	//-----------------------------------// 
	signal(SIGALRM, atende);

    if(llopen(fd,res)==-1){
      printf("Error establishing connection...");
    }

	/*
	 * SEND (START) CTRL PACKAGE
	 */



	char* filepath = argv[2];
	struct stat st;
	int filesize;
	if (stat(filepath, &st) == 0)
		filesize = st.st_size;
	else {
		printf("Failed to get file path/size\n");
		return -1;
	}
	printf("GIF Filesize: %d\n", filesize);

	int ctrlpckg_value = sendCtrlPckg(fd, 2, filepath, filesize);
	if(ctrlpckg_value != 0) {
		printf("Failed sending START control package\n");
		return -1;
	}

	/*
	 * CTRL PACKAGE SENT && SEND DATA PACKAGE
	 */

	FILE* file = fopen(filepath, "r");

	int bytes_read, bytes_total, seq_nr = 0;
	unsigned char* buffer = malloc(pckgsize * sizeof(char));
	int read_value = 0;
	int k=1, offset=0, exitc=0, n=0, n_counter=0;
	printf("\nSeq antes While: %d\n", Seq);
	while(1) {
		printf("Trama n: %d \n", k);
		if(read_value>0){
			bytes_read = fread(buffer, sizeof(char), pckgsize, file);
			offset += bytes_read;
			fseek(file, offset, SEEK_SET);
			for(n=0; n < bytes_read; n++) {
				//printf("Buffer: %x\n", buffer[n]);
			}	
			printf("Bytes read: %d\n", bytes_read);
			if(bytes_read <= 0) break;
		}
		
		alarm(3);

		if(n_counter==255){
			n_counter=0;
		}
		int datapckg_value = sendDataPckg(fd, 0, buffer, bytes_read);
		n_counter++; 
		int dp;		
		/*printf("Trama enviada: ");
		for(dp=0; dp < bytes_read; dp++) {
			printf("%c", buffer[dp]);
		}*/
		printf("\n");

		char* temp_buf;
		read_value = llread(fd, temp_buf);
		if(read_value == 1 && Seq == 1) { //recebeu RR_0
			printf("Received RR_0\n");
			k++;
			alarm_counter=0;		
			Seq = 0;		
		}
		else if(read_value == 2 && Seq == 0) { //recebeu RR_1
			printf("Received RR_1\n");
			k++;	
			alarm_counter=0;	
			Seq = 1;	
		}
		else if(read_value < 0 && read_value !=-3) { //recebeu REJ_0 ou REJ_1	
			printf("Received REJ_0 ou REJ_1\n");
			k++;
			alarm_counter=0;				
		}
		/*else if(read_value == -1 && Seq == 1) { //recebeu REJ_0	
			printf("Received REJ_0\n");
			k++;
			alarm_counter=0;
			Seq = 0;	
			read_value = 1;			
		}
		else if(read_value == -2 && Seq == 0) { //recebeu REJ_1
			printf("Received REJ_1\n");
			k++;
			alarm_counter=0;	
			Seq = 1;	
			read_value = 1;					
		}*/
		else if(read_value == -3){
			flag_alarm = 0;
			printf("Disparou alarme e re-enviei (%d)\n", alarm_counter);
		}
		printf("\nOffset: %d\n", offset);
		if(offset > filesize) break;
		if(alarm_counter == 4) return 0;
	}


	//----------------------------------//
   
    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }


	llclose(fd);

    close(fd);
    return 0;
}