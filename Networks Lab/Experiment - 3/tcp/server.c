#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>

void error(const char* msg) {
	perror(msg);
	exit(1);
}

int main(int argc, char* argv[]) {
	
	//incase of port no is not given
	if(argc < 2) {
		fprintf(stderr, "Port No not provided, Program Terminated");
		exit(1);
	}
	
	int sockfd, newsockfd, portno, n;
	char buffer[255];

	struct sockaddr_in serv_addr, cli_addr;
	socklen_t clilen;
	
	//creating socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
	//if socket creation failed
	if(sockfd < 0) {
		error("Error opening socket");
	}

	//clearing serv_addr if it contains any data
	bzero((char *) &serv_addr, sizeof(serv_addr));
	portno = atoi(argv[1]);		//Port no is the second argument in terminal command

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);
	
	//Binding socket to the address and port in serv_addr
	if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		error("Error in socket binding");
	}

	listen(sockfd, 5);	//backlog queue size is 5
	clilen = sizeof(cli_addr);
	
	printf("Server Started running on prot no %d\n", portno);
	
	//Accepting new client connection on newsockfd socket
	newsockfd = accept(sockfd, (struct sockaddr*) &cli_addr, &clilen);
	if(newsockfd < 0) 
		error("Accept error");

	while(1) {
		bzero(buffer, 255);
		if(read(newsockfd, buffer, 255) < 0)
			error("error in reading");
		
		printf("Client : %s\n",buffer);

		bzero(buffer, 255);
		fgets(buffer, 255, stdin);
		if(write(newsockfd, buffer, strlen(buffer)) < 0)
			error("Error in writing");

		if(strncmp("Bye", buffer, 3) == 0) 
			break;
	}
	
	close(newsockfd);
	close(sockfd);
	return 0;	
}
