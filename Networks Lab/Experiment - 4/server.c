#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<errno.h>
#include<signal.h>

void error(char* msg) {
    perror(msg);
    exit(1);
}

void reverse(char* buffer) {
    int end = strlen(buffer)-2,start =0;
    while(start<end) {
        char temp = buffer[start];
        buffer[start] = buffer[end];
        buffer[end] = temp;
        start++;
        end--;
    }
}

int main(int argc, char** argv) {

    int serverSocket, ret, newSocket;
    struct sockaddr_in serverAddr, clientAddr;
    char buffer[1024];
    pid_t childPid;
    socklen_t addrSize;

    if(argc < 2) 
        error("Please provide port no");

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(serverSocket < 0)
        error("Server Socket creation failed");

    printf("Server socket created successfully\n");

    memset(&serverAddr, '\0', sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(atoi(argv[1]));
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    ret = bind(serverSocket, (struct sockaddr*) &serverAddr, sizeof(serverAddr));
    if(ret < 0) 
        error("Bind error");
    
    printf("Bind to port %s\n", argv[1]);

    if(listen(serverSocket, 5) == 0)
        printf("Listening .....\n");
    else
        printf("Listening failed\n");

    while(1) {
        
        newSocket = accept(serverSocket, (struct sockaddr*) &clientAddr, &addrSize);
        if(newSocket < 0) 
            error("Accept failed");
        printf("New Connection accepted from %s:%d\n",inet_ntoa(clientAddr.sin_addr),ntohs(clientAddr.sin_port));

        if((childPid = fork()) == 0) {
            
            //Inside server child process
            while(1) {

                close(serverSocket);

                memset(buffer, '\0', 1024);
                recv(newSocket, buffer, 1024, 0);
                
                if(strncmp("Bye", buffer, 3) == 0) {
                    printf("%s:%d disconnected\n",inet_ntoa(clientAddr.sin_addr),ntohs(clientAddr.sin_port));
                    close(newSocket);
                    exit(1);
                }
                else {
                    printf("%s:%d : %s\n", inet_ntoa(clientAddr.sin_addr),ntohs(clientAddr.sin_port),buffer);
                    reverse(buffer);
                    send(newSocket, buffer, strlen(buffer), 0);
                }

            }
        }
    }

    close(serverSocket);
    return 0;
}