#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include<errno.h>
#include<pthread.h>
#include<time.h>

#include "packet.h"

typedef struct account{
    char username[50];
    char password[50];
}Accounts;

typedef struct client{
    int sock_fd;
    Accounts* acc;
    int total;
}Client;

void error(const char* msg) {
    perror(msg);
    exit(1);    
}

Client* new_client(int sock_fd, Accounts* acc, int user_count) {
    Client* node = (Client*)malloc(sizeof(Client));
    node->sock_fd = sock_fd;
    node->acc = acc;
    node->total = user_count;
    return node;
}

Accounts* store_credential(int* user_count){
    
    FILE* fp = fopen("logincred.txt","r");
    int count = 0;
    char fusername[50],fpassword[50];

    while(fscanf(fp,"%[^,]%*c %[^\n]%*c",fusername,fpassword) != EOF)
        count++;
    
    *user_count = count;

    Accounts* acc = (Accounts*)malloc(count*sizeof(Accounts));
    fseek(fp, 0, SEEK_SET);

    int i=0;
    while(fscanf(fp,"%[^,]%*c %[^\n]%*c",fusername,fpassword) != EOF){
        strcpy(acc[i].username, fusername);
        strcpy(acc[i].password, fpassword);
        i++;
    }

    return acc;
}

int verify_username_password(char* username, char* password, Accounts* acc, int total) {
    for(int i=0;i<total;i++) {
        if(strcmp(username, acc[i].username) == 0) {
            if(strcmp(password, acc[i].password) == 0)
                return 1;
        }
    }
    return 0;
}

char* trim_email(char* buffer){
    char* token1 = strtok(buffer,"<");
    char* token2 = strtok(NULL,">");
    char* email = (char*)malloc(strlen(token2));
    email = token2;
    return email;
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

int check_recipient(char* user, Accounts* account, int total){
    for(int i=0;i<total;i++){
        if(!strcmp(user, account[i].username))
            return 1;
    }
    return 0;
}

void get_time_string(char *time_string){
    time_t current_time;
    struct tm *timeinfo;

    current_time = time(NULL);
    timeinfo = gmtime(&current_time);
    strftime(time_string, 100, "%a, %d %b %Y %I:%M:%S GMT", timeinfo);
}

void* handle_client(void* arg){

    Client* client = (Client*)arg;

    char *username,*password,*response,*buffer;

    size_t len;
    //Recieving username
    if(recv_packet(client->sock_fd, &username) < 0){
        close(client->sock_fd);
        printf("[-] username message error\n");
        return NULL;
    }

    //Recieving password
    if(recv_packet(client->sock_fd, &password) < 0){
        close(client->sock_fd);
        printf("[-] password message error\n");
        return NULL;
    }

    if(verify_username_password(username,password,client->acc, client->total)){
        response = "Authenticated successfully";
        send_packet(client->sock_fd, response, strlen(response));
    }
    else{
        printf("[-] %s Authentication failed\n",username);
        response = "Authentication failed";
        send_packet(client->sock_fd, response, strlen(response));
        close(client->sock_fd);
        return NULL;
    }

    printf("[+] %s logged in successfully\n",username);

    char* from_email,*to_email;
    char to_username[50];

    while(1){
        if(recv_packet(client->sock_fd, &buffer) < 0){
            printf("[-] option message recieving failed\n");
            break;
        }
        
        printf("%s : %s\n",username,buffer);

        if(!strcmp(buffer,"QUIT"))
            break;
        
        //From message
        if(!strncmp(buffer, "MAIL", 4)){
            from_email = trim_email(buffer);
            if(verify_email(from_email)){
                response = "250 OK";
                printf("Server : %s\n",response);
                send_packet(client->sock_fd, response, strlen(response));
            }
            else{
                response = "500 Syntax error";
                printf("Server : %s",response);
                send_packet(client->sock_fd, response, strlen(response));
                continue;
            }
            
            //To message
            if(recv_packet(client->sock_fd, &buffer) < 0){
                printf("[-] option message recieving failed\n");
                break;
            }

            printf("%s : %s\n",username,buffer);
            
            if(!strncmp(buffer, "RCPT", 4)){
                to_email = trim_email(buffer);
                if(verify_email(to_email)){
                    //check Recipient
                    strcpy(to_username, to_email);
                    strcpy(to_username, strtok(to_username,"@"));
                    if(!check_recipient(to_username, client->acc, client->total)){
                        response = "550 No such user";
                        printf("Server : %s\n",response);
                        send_packet(client->sock_fd, response, strlen(response));
                        continue;
                    }
                    
                    response = "250 OK";
                    printf("Server : %s\n",response);
                    send_packet(client->sock_fd, response, strlen(response));
                }
                else{
                    response = "500 Syntax error";
                    send_packet(client->sock_fd, response, strlen(response));
                    continue;
                }
            }
            
            //DATA command
            if(recv_packet(client->sock_fd, &buffer) < 0){
                printf("[-] DATA message recieving failed\n");
                break;
            }

            printf("%s : %s\n",username,buffer);

            if(!strncmp(buffer, "DATA", 4)){
                response = "354 send the mail data, end with .";
                printf("Server : %s\n",response);
                send_packet(client->sock_fd, response, strlen(response)); 
            }

            //Body
            if(recv_packet(client->sock_fd, &buffer) < 0){
                printf("[-] Message body recieving failed\n");
                break;
            }

            printf("%s\n",buffer);

            if(!strncmp(buffer, "Subject", 4)){
                    response = "250 OK";
                    printf("Server : %s\n",response);
                    send_packet(client->sock_fd, response, strlen(response));
            }
            else{
                response = "500 Syntax error";
                send_packet(client->sock_fd, response, strlen(response));
                continue;
            }

        char mailbox_fname[100];
        sprintf(mailbox_fname, "%s/mymailbox.mail", to_username);
        FILE *mailbox = fopen(mailbox_fname, "a");

        char time_str[50];
        get_time_string(time_str);

        fprintf(mailbox, "From: %s\n", from_email);
        fprintf(mailbox, "To: %s\n", to_email);
        char* subject_line = strtok(buffer,"\n");
        fprintf(mailbox, "%s\n", subject_line);
        fprintf(mailbox, "Received: %s\n", time_str);
        
        char* remaining_portion;
        while(remaining_portion = strtok(NULL,"\n"))
            fprintf(mailbox, "%s\n", remaining_portion);

        fclose(mailbox);

        }
        
    }
    
    printf("[+] %s logged out\n",username);
    close(client->sock_fd);
    return NULL;
}

int main(int argc, char** argv) {

    if(argc < 2)
        error("format : ./filename port_no");
    
    int port_no = atoi(argv[1]);

    struct sockaddr_in server_addr,client_addr;
    socklen_t client_len = sizeof(client_addr);
    int fd_server, fd_client;
    int on = 1;

    //creating server socket
    fd_server = socket(AF_INET, SOCK_STREAM, 0);
    if(fd_server < 0)
        perror("[-]server socket creating failed");

    printf("[+] SMTP Server socket created ...\n");

    setsockopt(fd_server, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));

    //Server info
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(port_no);

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

    printf("[+] Server listening on port no %d\n",port_no);

    //Storing login credentials of all users
    int user_count = 0;
    Accounts* credentials = store_credential(&user_count);

    while(1) {
        
        fd_client = accept(fd_server, (struct sockaddr*)&client_addr, &client_len);
        if(fd_client == -1) {
            perror("Connection failed....\n");
            continue;
        }

        printf("[+] New Connection accepted from [%s : %d]\n",inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
    
        Client* client_ptr = new_client(fd_client, credentials, user_count);

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handle_client, client_ptr);
    }
    return 0;
}