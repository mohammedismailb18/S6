#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include<errno.h>

#include "packet.h"

#define LINE_SIZE 80
#define NO_OF_LINE 50
#define MESSAGE_SIZE LINE_SIZE*NO_OF_LINE

void error(const char* msg) {
    perror(msg);
    exit(1);    
}

void error_close(int sock_fd, const char* msg) {
    close(sock_fd);
    printf("%s\n",msg);
    exit(1);
}

void horizontal() {
    for(int i=0;i<40;i++)
        printf("_ ");
    printf("\n\n");
}

int verify_email(char* email) {
    int x=0,y=0,i=0;
    while(email[i]!='\0'){
        if(email[i] == '@'){
            i++;
            break;
        }
        x++;
        i++;
    }
    while(email[i]!='\0'){
        y++;
        i++;
    }
    return x>0 && y>0;
}

int handle_response(int sock_fd){
    char* response;
    if(recv_packet(sock_fd, &response) < 0)
        error_close(sock_fd, "response packet recieving failed");
    
    if(!strcmp(response, "250 OK")){
        return 1;
    }
    else if(!strcmp(response, "500 Syntax error")){
        printf("[-] From Email not verified");
        return 0;
    }
    else if(!strcmp(response, "354 send the mail data, end with .")){
        return 1;
    }
    else if(!strcmp(response, "550 No such user")){
        return 0;
    }
    return 0;
}

int send_mail(int sock_fd){
    char from[80],to[80],message[MESSAGE_SIZE], data[1024];
    int length = 0;

    //From address
    printf("\nFrom : ");
    scanf("%s",from);
    if(!verify_email(from)){
        printf("[-] Email is not proper format");
        return 0;
    }
    length = sprintf(message, "MAIL FROM:<%s>\r\n", from);
    send_packet(sock_fd, message, length);
    if(!handle_response(sock_fd)){
        printf("from response not OK");
        return 0;
    }

    //To address
    printf("To : ");
    scanf(" %s",to);
    if(!verify_email(to)){
        printf("[-] Email is not proper format");
        return 0;
    }
    length = sprintf(message, "RCPT TO:<%s>\r\n", to);
    send_packet(sock_fd, message, length);
    if(!handle_response(sock_fd)){
        printf("to response not OK");
        return 0;
    }

    //DATA
    strcpy(message, "DATA");
    send_packet(sock_fd, message, strlen(message));
    if(!handle_response(sock_fd)){
        printf("DATA response not OK");
        return 0;
    }
    
    //Subject
    printf("Subject : ");
    scanf(" %[^\n]%*c",message);
    
    // //Message
    bzero(data,1024);
    strcat(data, "Subject: ");
    strcat(data,message);
    strcat(data,"\n");

    while(1) {
        scanf("\n");
        scanf("%[^\n]s",message);
        if(!strcmp(message,".")){
            strcat(data,".\n");
            break;
        }
        strcat(data, message);
        strcat(data, "\n");
    }

    send_packet(sock_fd, data, strlen(data));
    if(!handle_response(sock_fd)){
        printf("[-] Message packet error\n");
        return 0;
    }
    return 1;
}

int main(int argc, char** argv) {

    if(argc < 2)
        error("format : ./filename port_no");
    
    char* address = "127.0.0.1";
    int port_no = atoi(argv[1]);

    int sock_fd = 0, recv_len;
    struct sockaddr_in server_address;
    char username[50], password[50];

    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("Error creating socket!!\n");
        return -1;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port_no);

    if (inet_pton(AF_INET, address, &server_address.sin_addr) <= 0)
    {
        printf("Invalid address\\Address not supported\n");
        return -1;
    }

    printf("Connecting...\n");

    if (connect(sock_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        printf("Connection failed!\n");
        return -1;
    }

    printf("[+] Successfully connected to %s:%d\n", address, port_no);

    printf("\nPlease enter username and password\n");
    printf("username: ");
    scanf("%s",username);
    printf("password : ");
    scanf("%s",password);

    if(send_packet(sock_fd, username, strlen(username)) < 0) {
        close(sock_fd);
        error("[-] username sending error");
    }

    if(send_packet(sock_fd, password, strlen(password)) < 0) {
        close(sock_fd);
        error("[-] password sending error");
    }
    
    char* buffer;
    if(recv_packet(sock_fd, &buffer) < 0) {
        close(sock_fd);
        error("authentication message recieving failed");
    }

    if(strcmp(buffer, "Authenticated successfully") == 0)
        printf("\n[+] Authenticated successfully\n");
    else{
        close(sock_fd);
        printf("Authentication failed\n");
        exit(1);
    }

    while(1){
        horizontal();

        int option;
        printf("Choose an option\n\n");
        printf("1. Send mail\n2. Quit\nOption : ");
        scanf("%d",&option);

        if(option < 1 || option > 2){
            printf("Invalid option\n");
            continue;
        }
        else if(option == 2) {
            char* response = "QUIT";
            send_packet(sock_fd, response, strlen(response));
            break;
        }
        else if(option == 1){
            if(send_mail(sock_fd))
                printf("[+] Email send successfully\n");
            else
                printf("[-] Email sending failed\n");
        }
    }

    printf("\nSocket Closing ....\nBye\n");
    close(sock_fd);
    return 0;
}