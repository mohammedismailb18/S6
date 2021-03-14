#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<string.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<errno.h>

#define BUFFER_SIZE 2048

//HTML code to send
char webpage[] = 
"HTTP/1.1 200 OK\r\n"
"Content-Type: text/html\r\n\r\n"
"<!DOCTYPE html>\r\n"
"<html><head><title>VSWS</title></head>\r\n"
"<body style='color: red;'><h1><center>Welcome to Networks Lab!</center></h1></body></html>\r\n";

int main() {

    struct sockaddr_in server_addr,client_addr;
    socklen_t client_len = sizeof(client_addr);
    int fd_server, fd_client;
    char buf[BUFFER_SIZE];
    int on = 1;

    //creating server socket
    fd_server = socket(AF_INET, SOCK_STREAM, 0);
    if(fd_server < 0){
        perror("[-]server socket creating failed");
        exit(1);
    }

    printf("[+] Server socket created ...\n");

    setsockopt(fd_server, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));

    //Server info
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(8080);

    //server socket bind
    if(bind(fd_server, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1){
        perror("[-] server socket bind error");
        close(fd_server);
        exit(1);
    }

    printf("[+] Server socket bind successfully ...\n");

    if(listen(fd_server, 10) == -1) {
        perror("[-] listening failed");
        close(fd_server);
        exit(1);
    }

    while(1) {
        //accepting client connection
        fd_client = accept(fd_server, (struct sockaddr*)&client_addr, &client_len);

        if(fd_client == -1) {
            perror("Connection failed....\n");
            continue;
        }

        printf("New Connection accepted from [%s : %d]\n",inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));

        if(!fork()) {
            //Child Process
            close(fd_server);
            memset(buf,0,2048);
            read(fd_client, buf, 2047);

            printf("\n%s\n",buf);

            if(!strncmp(buf,"GET / HTTP/1.1",14)) {
                //sending html to client
                if(write(fd_client, webpage, sizeof(webpage)-1) == -1) {
                    perror("writing to client socket failed");
                    close(fd_client);
                    exit(1);
                }
            }
            close(fd_client);
            printf("[%s : %d] closed connection \n",inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
            exit(0);
        
        }

        //Parent Process
        close(fd_client);
    }

    return 0;
}