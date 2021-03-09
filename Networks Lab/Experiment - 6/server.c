#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include<pthread.h>

#define BUFF_SIZE 500

struct sockaddr_in c_addr;
char fname[100];

void sendFile() {
    
}

void* clientProcess(int *arg)
{
    int connfd=(int)*arg;
    printf("Connection accepted and id: %d\n",connfd);
    printf("Connected to Clent: %s:%d\n",inet_ntoa(c_addr.sin_addr),ntohs(c_addr.sin_port));
    char buffer[255],f_size[20];
    int n;

    while(1) {
        bzero(buffer, 255);
		n = read(connfd, buffer, 255);
		if(n < 0)
			perror("[-]ERROR on reading");

        if(strncmp("Bye", buffer, 3) == 0) {
            printf("Closing Connection for id: %d\n",connfd);
            close(connfd);
            shutdown(connfd,SHUT_WR);
        }
        else if(strncmp("GivemeyourVedio",buffer,15) == 0) {
            break;
        }
		printf("Client : %s\n",buffer);

		bzero(buffer, 255);
        strcpy(buffer,"[server : Your message recieved here]");
		// fgets(buffer, 255, stdin);
		n = write(connfd, buffer, strlen(buffer));
		if(n < 0)
			error("[-]ERROR in writing");
	}

    FILE *fp = fopen(fname,"rb");

    if(fp==NULL)
    {
        printf("File opern error");
        return;   
    }   
    write(connfd, fname,256);

    /* Read data from file and send it */
    while(1)
    {
        unsigned char buff[BUFF_SIZE]={0};
        int nread = fread(buff,1,BUFF_SIZE,fp);        

        if(nread > 0)
        {
            write(connfd, buff, nread);
        }
        if (nread < BUFF_SIZE)
        {
            if (feof(fp))
	        {
                printf("End of file\n");
	            printf("File transfer completed for id: %d\n",connfd);
	        }
            if (ferror(fp))
                printf("Error reading\n");
            break;
        }
    }
    printf("Client : Bye\nClosing Connection for id: %d\n",connfd);
    close(connfd);
    shutdown(connfd,SHUT_WR);
    sleep(2);
}

int main(int argc, char *argv[])
{
    int connfd = 0,err;
    pthread_t tid; 
    struct sockaddr_in serv_addr;
    int listenfd = 0,ret;
    char sendBuff[BUFF_SIZE+1];
    int numrv;
    size_t clen=0;

    //creating server socket for listening client connections
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd<0)
	{
	  printf("[-]Error in socket creation\n");
	  exit(2);
	}

    printf("[+]server socket created success\n");

    //Initialize server address structure
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(5001);

    //binding socket with address
    ret=bind(listenfd, (struct sockaddr*)&serv_addr,sizeof(serv_addr));
    if(ret<0)
    {
      printf("Error in bind\n");
      exit(2);
    }

    //listening for incoming connections
    if(listen(listenfd, 10) == -1)
    {
        printf("Failed to listen\n");
        return -1;
    }

    if (argc < 2) 
    {
	    printf("Enter file name to send: ");
        scanf("%[^\n]%*c",fname);
    }
    else
        strcpy(fname,argv[1]);

    while(1)
    {
        clen=sizeof(c_addr);
        printf("Waiting...\n");
        connfd = accept(listenfd, (struct sockaddr*)&c_addr,&clen);
        if(connfd<0)
        {
	        printf("Error in accept\n");
	        continue;	
	    }
        err = pthread_create(&tid, NULL, &clientProcess, &connfd);
        if (err != 0)
            printf("\ncan't create thread :[%s]", strerror(err));
    }
    close(connfd);
    return 0;
}
