#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include "datalinkGet.h"

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
		printf("\nTrama n: %d\n", ++k);
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
			printf("The file was sucessfully transfered!\n\n");
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
