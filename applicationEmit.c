#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include "datalinkEmit.h"


int sendCtrlPckg(int fd, int ctrl_field, char* filepath, int filesize) {
	int i, j=3;	
	unsigned char file_size[16] = {}, buffer[255];
	snprintf(file_size, 20000, "%d", filesize);
	
	printf("Filepath: %s\nFilepath size: %d\nFilesize: %d\n", filepath, strlen(filepath), strlen(file_size));
	int pckg_size = 6 + strlen(filepath) + strlen(file_size);
	unsigned char ctrl_pckg[pckg_size];
	unsigned char hex;

	ctrl_pckg[0] = ctrl_field;
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
	printf("Sent Control Package \n");

	int llreadval=llread(fd, buffer);
	if(llreadval<0){
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

	while(alarm_counter<4){
		int llopenval = llopen(fd,res);
		if(llopenval==-1){
			printf("Error establishing connection...");
		} 
		else if(llopenval==-3 && alarm_counter!=4){
			flag_alarm = 0;
			printf("Disparou alarme e re-enviei (%d)\n", alarm_counter);
			alarm(3);
		} 
		else if(llopenval==0){
			flag_alarm = 0;
			alarm_counter = 0;
			alarm(0);
			break;
		}	
	}

	if(alarm_counter==4){
		printf("Error establishing connection (timeout)\n");
		return -1;
	}

	/*
	 * SEND (START) CTRL PACKAGE
	 */



	char* filepath = argv[2];
	struct stat st;
	int filesize;
	int ctrlpckg_value = 0;
	if (stat(filepath, &st) == 0)
		filesize = st.st_size;
	else {
		printf("Failed to get file path/size\n");
		return -1;
	}
	printf("GIF Filesize: %d\n", filesize);
	alarm(3);
	while(1){
		ctrlpckg_value = sendCtrlPckg(fd, 2, filepath, filesize);
		if(ctrlpckg_value < 0) {
			flag_alarm = 0;
			printf("Disparou alarme e re-enviei control package (%d)\n", alarm_counter);
			alarm(3);
		}
		else{
			flag_alarm = 0;
			alarm_counter = 0;
			alarm(0);
			break;
		}
		if(alarm_counter==4){
		printf("Error sending control package\n");
		return -1;
		}		
	}
	Seq = 1 -Seq;
	/*
	 * CTRL PACKAGE SENT && SEND DATA PACKAGE
	 */

	FILE* file = fopen(filepath, "r");

	int bytes_read, bytes_total, seq_nr = 0;
	unsigned char* buffer = malloc(pckgsize * sizeof(char));
	int read_value = 1;
	int k=1, offset=0, exitc=0, n=0, n_counter=1;
	while(1) {
		printf("Trama n: %d \n", k);
		if(read_value>0){
			bytes_read = fread(buffer, sizeof(char), pckgsize, file);
			offset += bytes_read;
			fseek(file, offset, SEEK_SET);
			printf("Bytes of data read from file: %d\n", bytes_read);
			if(bytes_read <= 0) break;
		}
		
		alarm(3);

		if(n_counter==255){
			n_counter=1;
		}
		int datapckg_value = sendDataPckg(fd, n_counter, buffer, bytes_read);
		int dp;		
		printf("\n");

		char* temp_buf;
		read_value = llread(fd, temp_buf);
		if(read_value == 1 && Seq == 1) { //recebeu RR_0
			printf("Received RR_0\n");
			k++;
			alarm_counter=0;		
			Seq = 0;
			n_counter++; 		
		}
		else if(read_value == 2 && Seq == 0) { //recebeu RR_1
			printf("Received RR_1\n");
			k++;	
			alarm_counter=0;	
			Seq = 1;
			n_counter++; 	
		}
		else if(read_value < 0 && read_value !=-3) { //recebeu REJ_0 ou REJ_1	
			printf("Received REJ_0 ou REJ_1\n");
			k++;
			alarm_counter=0;				
		}
		else if(read_value == -3){
			flag_alarm = 0;
			printf("Disparou alarme e re-enviei (%d)\n", alarm_counter);
		}
		printf("\nOffset: %d\n", offset);
		if(offset > filesize) break;
		if(alarm_counter == 4) return 0;
	}
	
	alarm(3);
	while(1){
		ctrlpckg_value = sendCtrlPckg(fd, 3, filepath, filesize);
		if(ctrlpckg_value < 0) {
			flag_alarm = 0;
			printf("Disparou alarme e re-enviei control package (%d)\n", alarm_counter);
			alarm(3);
		}
		else{
			flag_alarm = 0;
			alarm_counter = 0;
			alarm(0);
			break;
		}
		if(alarm_counter==4){
		printf("Error sending control package\n");
		return -1;
		}		
	}


	//----------------------------------//
   
    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }
	while(alarm_counter<4){
		int llcloseval=llclose(fd);
		if(llcloseval==-3){
			printf("Error closing connection...");
		} 
		else if(llcloseval==-3 && alarm_counter!=4){
			flag_alarm = 0;
			printf("Disparou alarme e re-enviei DISC (%d)\n", alarm_counter);
			alarm(3);
		} 
		else if(llcloseval==0){
			flag_alarm = 0;
			alarm_counter = 0;
			alarm(0);
			break;
		}	
	}
    close(fd);
    return 0;
}
