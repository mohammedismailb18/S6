#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include<sys/time.h>

#define BUFF_SIZE 500
#define LENGTH_MSG 100

int main(int argc, char *argv[])
{
    int sockfd = 0;
    int bytesReceived = 0;
    char recvBuff[BUFF_SIZE];
    memset(recvBuff, '0', sizeof(recvBuff));
    struct sockaddr_in serv_addr;

    /* Creating a socket */
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0))< 0)
    {
        printf("\n [-]Error : Could not create socket \n");
        return 1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5001); // port
    char ip[50];
    
    if (argc < 2) 
    {
        printf("Enter IP address to connect: ");
        gets(ip);
    }
    else
        strcpy(ip,argv[1]);

    serv_addr.sin_addr.s_addr = inet_addr(ip);

    /* connecting to server */
    if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0)
    {
        printf("\n [-]Error : Connect Failed \n");
        return 1;
    }

    printf("[+]Connected to ip: %s : %d\n",inet_ntoa(serv_addr.sin_addr),ntohs(serv_addr.sin_port));

   	 /* Create file to store data */
    FILE *fp;
	char fname[100],buffer[255];
    int n;

    while(1) {
		bzero(buffer, 255);
		fgets(buffer, 255, stdin);
		n = write(sockfd, buffer, strlen(buffer));
		if(n < 0)
			error("[-]ERROR in writing");
        if(strncmp("Bye", buffer, 3) == 0) {
			printf("Connection closing\n");
            fclose(sockfd);
            exit(0);
        }
        else if(strncmp("GivemeyourVedio", buffer, 15) == 0) {
			break;
        }
		bzero(buffer, 255);
		n = read(sockfd, buffer, 255);
		if(n < 0)
			error("[-]ERROR on reading");
        fflush(stdout);
		printf("%s\n",buffer);
	}

	 read(sockfd, fname, 256);

	printf("File Name: %s\n",fname);
	printf("[+]Receiving file...");
   	fp = fopen(fname, "ab"); 
    FILE* data_fp = fopen("data.txt","w+");
    if(fp == NULL || data_fp == NULL)
    {
       	 printf("[-]Error opening file");
         return 1;
    }
    /* Receiving data from server */
    printf("\n");
    struct timeval start,t1,t2;
    gettimeofday(&start, NULL);
    t1.tv_sec = start.tv_sec;
    t1.tv_usec = start.tv_usec;
    unsigned long long diff,bytesCount=0,bytesCount1 = 0,bytesCount2,bytesDiff;
    float time_from_start;
    while((bytesReceived = read(sockfd, recvBuff, BUFF_SIZE)) > 0)
    { 
        bytesCount += bytesReceived;
        
        printf("Received: %.4f Mb\r",((float)bytesCount/(1024*1024)));
	    fflush(stdout);
        fwrite(recvBuff, 1,bytesReceived,fp);
        gettimeofday(&t2, NULL);
        diff = ((t2.tv_sec*1000000 + t2.tv_usec) - (t1.tv_sec*1000000 + t1.tv_usec));
        
        if(diff>=100000) {
            
            bytesCount2 = bytesCount - bytesCount1;
            bytesCount1 = bytesCount;

            t1.tv_sec = t2.tv_sec;
            t1.tv_usec = t2.tv_usec;
            time_from_start = (float)((t2.tv_sec*1000000 + t2.tv_usec) - (start.tv_sec*1000000 + start.tv_usec))/1000000;

            fprintf(data_fp,"%.6f \t %.4f\n",time_from_start,(float)bytesCount2/(1024*1024));
        }
    }
    fclose(data_fp);
    printf("\n");
    if(bytesReceived < 0)
    {
        printf("\n Read Error \n");
        exit(1);
    }
    printf("\nFile OK....Completed\nBye\n");
    return 0;
}
