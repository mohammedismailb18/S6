#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<errno.h>
#include<arpa/inet.h>

void error(const char* msg) {
	error(msg);
	exit(1);
}

int main(int argc, char* argv[]) {
	
	struct sockaddr_in srvr_in;
	struct sockaddr_in clnt_in;
	char* srvr_addr;		//For server IP address
	int s;				//socket file descriptor
	int z;				//For status code
	char buff[512];		//Buffer
	int len_inet;			//For storing struct size
	
	//Initialising server ip address to srvr_addr
	if(argc < 2) 
		srvr_addr = "127.0.0.31";
	else
		srvr_addr = argv[1];
	
	//Socket creation
	s = socket(PF_INET, SOCK_DGRAM, 0);
	if(s == -1)
		error("creating socket failed");
	
	//Initialising srvr_in parameters
	memset(&srvr_in, 0, sizeof(srvr_in));
	srvr_in.sin_family = AF_INET;
	srvr_in.sin_port = htons(9191);	// host order to network order short conversion 
	srvr_in.sin_addr.s_addr = inet_addr(srvr_addr);
	
	len_inet = sizeof(srvr_in);
	
	if(srvr_in.sin_addr.s_addr == INADDR_NONE)
		error("Bad Address");
	
	//Binding socket with address
	z = bind(s, (struct sockaddr *) &srvr_in, len_inet);
	if(z == -1)
		error("Bind failed");
	
	printf("server started running at %s in port no 9191.......\n\n",srvr_addr);

	for(;;) {
		len_inet = sizeof(clnt_in);
		
		z = recvfrom(s, buff, sizeof(buff), 0, (struct sockaddr *) &clnt_in, &len_inet);
		if(z == -1)
			error("recvfrom() failed");
		
		buff[z] = '\0';			//adding null character at end of string
		
		if(!strncmp("QUIT",buff,4))
			break;
		
		printf("Client: %s\n",buff);
		
		memset(buff,0,512);
		fgets(buff,512,stdin);
		
		//sending server input
		z = sendto(s, buff, sizeof(buff), 0, (struct sockaddr *) &clnt_in, sizeof(clnt_in));
		if(z == -1)
			error("sendto() failed");
	}
	
	close(s);
	return 0;
	
			
}
