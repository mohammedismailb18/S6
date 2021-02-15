#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<errno.h>

void error(char* msg) {
    perror(msg);
    exit(1);
}


int main(int argc, char** argv) {
    
    if(argc <2)
        error("Please provide port no");

    int clientSocket, ret;
    struct sockaddr_in serverAddr;
    char buffer[1024];

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(clientSocket < 0)
        error("Client Socket creation failed");

    printf("Socket created successfully\n");

    memset(&serverAddr, '\0', sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(atoi(argv[1]));
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    ret = connect(clientSocket, (struct sockaddr*) &serverAddr, sizeof(serverAddr));
    if(ret  < 0)
        error("Connecting to server failed");
    printf("Connected to the server\n\n");

    while(1) {
        memset(buffer,'\0',1024);
        fgets(buffer, 1024, stdin);
        if(send(clientSocket, buffer, strlen(buffer), 0) < 0)
            printf("Message sending failed\n");
        
        if(strncmp("Bye", buffer, 3) == 0) {
            close(clientSocket);
            printf("Disconnected from server\n");
            exit(1);
        }
        fflush(stdout);
        memset(buffer, '\0', 1024);
        if(recv(clientSocket, buffer, 1024, 0) < 0)
            printf("Message receiving failed\n");
        else
            printf("Server : %s\n", buffer);
    }

    return 0;
}