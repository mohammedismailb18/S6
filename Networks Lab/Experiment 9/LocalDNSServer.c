#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <pthread.h>

#define SIZE 4096
#define QSIZE 100

typedef struct DNSPacketHeader DNSHeader;
typedef struct DNSPacketQuestion DNSQuestion;
typedef struct DNSPacketAnsAddition DNSAns;
// typedef struct DNSRecord DNSRecord;

typedef struct handleLookupArg HLArg;

// struct DNSRecord
// {
//     char NAME[QSIZE];
//     short unsigned size;
//     char TYPE[2];
//     char CLASS[2];
//     unsigned short TTL;
//     char RDLENGTH[2];
//     char RDATA[100];

//     struct DNSRecord *next;
//     struct DNSRecord *prev;
// };

struct DNSPacketHeader
{
    char id[2];
    short unsigned QR;
    short unsigned Opcode;
    short unsigned AA;
    short unsigned TC;
    short unsigned RD;
    short unsigned RA;
    short unsigned Z;
    short unsigned AD;
    short unsigned CD;
    short unsigned RCODE;
    short unsigned QDCOUNT;
    short unsigned ANCOUNT;
    short unsigned NSCOUNT;
    short unsigned ARCOUNT;
};

struct DNSPacketQuestion
{
    char QNAME[QSIZE];
    short unsigned qsize;
    char QTYPE[2];
    char QCLASS[2];
};

struct DNSPacketAnsAddition
{
    unsigned TTL;
    unsigned short RDLENGTH;
    char RDATA[QSIZE];
};

struct handleLookupArg
{
    int sock;
    char Buf[SIZE];
    struct sockaddr_in clientAddr;
};

void getTime(char *t_str)
{
    time_t t;
    struct tm tm;

    t = time(NULL);
    tm = *localtime(&t);
    strcpy(t_str, asctime(&tm));
}

void horizontal(){
    for(int i=0;i<20;i++)
        printf("- ");
    printf("\n\n");
}
void change_to_dot_format(unsigned char *str) {
	int i, j;
	for(i = 0; i < strlen((const char*)str); ++i) {
		unsigned int len = str[i];
		for(j = 0; j < len; ++j) {
			str[i] = str[i + 1];
			++i;
		}
		str[i] = '.';
	}
	str[i - 1] = '\0';
}

void parseHeader(char *recvBuf, DNSHeader *req)
{

    char octect;

    req->id[0] = recvBuf[0];
    req->id[1] = recvBuf[1];

    octect = recvBuf[2];
    req->QR = (octect & 128) >> 7;
    req->Opcode = (octect & 120) >> 3;
    req->AA = (octect & 4) >> 2;
    req->TC = (octect & 2) >> 1;
    req->RD = octect & 1;

    octect = recvBuf[3];
    req->RA = (octect & 128) >> 7;
    req->Z = (octect & 64) >> 6;
    req->AD = (octect & 32) >> 5;
    req->CD = (octect & 16) >> 4;
    req->RCODE = octect & 15;

    req->QDCOUNT = recvBuf[4];
    req->QDCOUNT = (req->QDCOUNT) << 8;
    req->QDCOUNT = (req->QDCOUNT) + recvBuf[5];

    req->ANCOUNT = recvBuf[6];
    req->ANCOUNT = (req->ANCOUNT) << 8;
    req->ANCOUNT = (req->ANCOUNT) + recvBuf[7];

    req->NSCOUNT = recvBuf[8];
    req->NSCOUNT = (req->NSCOUNT) << 8;
    req->NSCOUNT = (req->NSCOUNT) + recvBuf[9];

    req->ARCOUNT = recvBuf[10];
    req->ARCOUNT = (req->ARCOUNT) << 8;
    req->ARCOUNT = (req->ARCOUNT) + recvBuf[11];
}

void fetchQuestion(char *qstart, DNSQuestion *qstn)
{
    int i = 0;
    while (qstart[i])
    {
        i = i + (qstart[i] + 1);
    }

    qstn->qsize = i + 1;

    for (i = 0; i < qstn->qsize; ++i)
    {
        qstn->QNAME[i] = qstart[i];
        //printf("%c ", qstn->QNAME[i]);
    }

    // printf("\nQSIZE : %hu\n", qstn->qsize);
}

void parseQuestion(char *qStart, DNSQuestion *qstn)
{
    fetchQuestion(qStart, qstn);

    qstn->QTYPE[0] = qStart[qstn->qsize];
    qstn->QTYPE[1] = qStart[qstn->qsize + 1];

    qstn->QCLASS[0] = qStart[qstn->qsize + 2];
    qstn->QCLASS[1] = qStart[qstn->qsize + 3];
}

void change_to_dns_format(char *src, unsigned char *dest) {
	int pos = 0;
	int len = 0;
	int i;
	//strcat(src, ".");   // As each '.' is replaced by a number
	for(i = 0; i < (int)strlen(src); ++i) {
		if(src[i] == '.') {
			dest[pos] = i - len;
			++pos;
			for(; len < i; ++len) {
				dest[pos] = src[len];
				++pos;
			}
			len++;
		}
	}
	dest[pos] = '\0';
}

int checkCache(DNSQuestion *qstn, char* ans)
{
    char buffer[100];
    char type[6], qstnType[6], domain[100], qstnDomain[100], iterativeAnswer[100];
    int foundCacheEntry = 0;

    FILE* fp = fopen("cache.txt","r");

    if (qstn->QTYPE[0] == 0 && qstn->QTYPE[1] == 0x1)           //Type A 
        strcpy(qstnType, "A");
    else if (qstn->QTYPE[0] == 0 && qstn->QTYPE[1] == 0x1c)     //Type AAAA
        strcpy(qstnType, "AAAA");
    else if((qstn->QTYPE[0] == 0 && qstn->QTYPE[1] == 0x2))      //Type NS
        strcpy(qstnType,"NS");
    else if(qstn->QTYPE[0] == 0 && qstn->QTYPE[1] == 0x5)        //Type CNAME
        strcpy(qstnType, "CNAME");
    else
        return -1;
    
    strcpy(qstnDomain, qstn->QNAME);
    change_to_dot_format(qstnDomain);

    memset(buffer, 0, 100);

    while(fgets(buffer, 100, fp) != NULL)
    {
        sscanf(buffer, "%s %s %s", type, domain, iterativeAnswer);
        
        if(!strcmp(type, qstnType) && !strcmp(qstnDomain, domain)){
            foundCacheEntry = 1;
            printf("[+] %s found in cache\n",domain);
            strcpy(ans, iterativeAnswer);
            break;
        }
        memset(buffer, 0, 100);
        printf("%s %s %s\n",type, domain, iterativeAnswer);
    }

    if(foundCacheEntry)
        return 1;
    else{
        printf("%s Not found in cache\n",qstnDomain); 
        return 0;
    }
}

void addCacheEntry(DNSQuestion *qstn, char *ans)
{
    char type[6];
    char cacheEntry[100];
    char domain[100];

    FILE* fp = fopen("cache.txt", "a");

    strcpy(domain, qstn->QNAME);
    change_to_dot_format(domain);

    if (qstn->QTYPE[0] == 0 && qstn->QTYPE[1] == 0x1)           //Type A 
        strcpy(type, "A");
    else if (qstn->QTYPE[0] == 0 && qstn->QTYPE[1] == 0x1c)     //Type AAAA
        strcpy(type, "AAAA");
    else if((qstn->QTYPE[0] == 0 && qstn->QTYPE[1] == 0x2))      //Type NS
        strcpy(type,"NS");
    else if(qstn->QTYPE[0] == 0 && qstn->QTYPE[1] == 0x5)        //Type CNAME
        strcpy(type, "CNAME");
    
    sprintf(cacheEntry, "%s %s %s", type, domain, ans);
    
    fprintf(fp, "%s\n", cacheEntry);

    printf("[+]New Cache Entry : %s %s %s\n",type, domain, ans);

    fclose(fp);
}

void resolveAnswer(DNSQuestion *qstn, DNSAns *ans, char* iterativeAnswer)
{
    int valid = 0;
    ans->TTL = 120;

    printf("resolve Query : Answer = %s\n",iterativeAnswer);

    if (qstn->QTYPE[0] == 0 && qstn->QTYPE[1] == 0x1)   //Type A 
    {
        ans->RDLENGTH = 4;

        if(inet_pton(AF_INET, iterativeAnswer, ans->RDATA) < 0)
            printf("problem in ipv4 conversion\n");
        else
            valid = 1;
    }
    else if (qstn->QTYPE[0] == 0 && qstn->QTYPE[1] == 0x1c) //Type AAAA
    {
        ans->RDLENGTH = 16;

        if(inet_pton(AF_INET6, iterativeAnswer, ans->RDATA) < 0)
            printf("problem in ipv6 conversion\n");
        else
            valid = 1;
    }
    else if((qstn->QTYPE[0] == 0 && qstn->QTYPE[1] == 0x2) || (qstn->QTYPE[0] == 0 && qstn->QTYPE[1] == 0x5)){      //Type NS or CNAME
        
        ans->RDLENGTH = strlen(iterativeAnswer)+1;

        change_to_dns_format(iterativeAnswer, ans->RDATA);

        valid = 1;
    }

    if(valid){
        if(!checkCache(qstn, iterativeAnswer))                    //if entry not present in cache, add this entry
            addCacheEntry(qstn, iterativeAnswer);
    }
}

void assignHeader(char *sendBuf, DNSHeader *head)
{
    sendBuf[0] = head->id[0];
    sendBuf[1] = head->id[1];

    char byte = 1; //QR = 1 for response

    byte = byte << 4;
    byte = byte | (head->Opcode);

    byte = byte << 1;
    byte = byte | (head->AA); //[Modify AA]

    byte = byte << 1;
    byte = byte | (head->TC);

    byte = byte << 1;
    byte = byte | (head->RD);

    sendBuf[2] = byte;

    byte = 0; //RA

    byte = byte << 7;
    byte = byte | (head->RCODE);

    sendBuf[3] = byte;

    sendBuf[5] = head->QDCOUNT;
    sendBuf[7] = 1; //[Modify ANCOUNT]
    sendBuf[9] = head->NSCOUNT;
    sendBuf[11] = head->ARCOUNT;
}

void assignQuestion(char *qstField, DNSQuestion *qstn)
{
    unsigned i = 0;
    while (i < qstn->qsize)
    {
        qstField[i] = qstn->QNAME[i];
        ++i;
    }
    qstField[i++] = qstn->QTYPE[0];
    qstField[i++] = qstn->QTYPE[1];

    qstField[i++] = qstn->QCLASS[0];
    qstField[i++] = qstn->QCLASS[1];
}

void assignAnswer(char *ansField, DNSQuestion *qstn, DNSAns *ans)
{
    unsigned i = 0;

    ansField[i++] = 0xc0;
    ansField[i++] = 0x0c; //[NAME field]

    ansField[i++] = qstn->QTYPE[0];
    ansField[i++] = qstn->QTYPE[1];

    ansField[i++] = qstn->QCLASS[0];
    ansField[i++] = qstn->QCLASS[1];

    ansField[i++] = 0;
    ansField[i++] = 0;
    ansField[i++] = 0;
    ansField[i++] = ans->TTL; //[Modify TTL]

    ansField[i++] = 0; //[Modify RDLENGTH]
    ansField[i++] = ans->RDLENGTH;

    for (unsigned j = 0; j < ans->RDLENGTH; ++j)
    {
        ansField[i++] = ans->RDATA[j];
    }
}

unsigned createResponse(DNSHeader *head, DNSQuestion *qstn, DNSAns *ans, char *sendBuf)
{
    memset(sendBuf, 0, SIZE);

    unsigned pos = 0;

    assignHeader(sendBuf, head);
    pos = 12;

    assignQuestion(sendBuf + pos, qstn);
    pos += (qstn->qsize) + 4;

    assignAnswer(sendBuf + pos, qstn, ans);
    pos += 12 + (ans->RDLENGTH);

    return pos;
}

void getServer(char* type, char* domain, char* nameServer){
    char buffer[SIZE], cmd[100];
    char *rp = NULL, *feild;
    FILE* fp;

    sprintf(cmd, "nslookup -type=%s %s", type, domain);
    printf("%s\n",cmd);

    fp = popen(cmd,"r");

    memset(buffer,0,SIZE);
    
    if(!strcmp(type, "NS")){
        while(fscanf(fp, " %[^\n]%*c", buffer)){
            if(rp = strstr(buffer, "nameserver")){
                    strcpy(nameServer, rp+13);
                    break;
            }
            memset(buffer,0,SIZE);
        }
    }
    else if(!strcmp(type, "A") || !strcmp(type, "AAAA")){
         while(fscanf(fp, " %[^\n]%*c", buffer)){
            if(rp = strstr(buffer, "Address: ")){
                if(rp[9]!=' '){
                    strcpy(nameServer, rp+9);
                    break;
                }
            }
            memset(buffer,0,SIZE);
        }
    }
    else if(!strcmp(type, "CNAME")){
         while(fscanf(fp, " %[^\n]%*c", buffer)){
            if(rp = strstr(buffer, "canonical name = ")){
                if(rp[17]!=' '){
                    strcpy(nameServer, rp+17);
                    break;
                }
            }
            memset(buffer,0,SIZE);
        }
    }

    printf("%s\n",nameServer);
    pclose(fp);
}

void iterativeQuery(DNSQuestion question, char* output){
    char path[100], qstnDomainName[100], domainName[100], nameServer[100];
    char *type, *ans;

    //Checking Query type
    if(question.QTYPE[0] == 0 && question.QTYPE[1] == 0x1)
        type = "A";
    else if (question.QTYPE[0] == 0 && question.QTYPE[1] == 0x1c)
        type = "AAAA";
    else if(question.QTYPE[0] == 0 && question.QTYPE[1] == 0x2)
        type = "NS";
    else if(question.QTYPE[0] == 0 && question.QTYPE[1] == 0x5)
        type = "CNAME";
    

    strcpy(qstnDomainName, question.QNAME);
    change_to_dot_format(qstnDomainName);

    if(!strncmp(qstnDomainName, "www", 3))              //excluding www if present
        strcpy(domainName, qstnDomainName + 4);     
    else 
        strcpy(domainName, qstnDomainName);

    printf("domain name = %s\n",domainName);
    
    getServer("NS",".",nameServer);

    int i = strlen(domainName)-1;

    //Nameserver resolving
    while(i--){
        if(domainName[i] == '.' || i==0){
            memset(path, 0, 100);
            if(i!=0)
                sprintf(path, "%s %s",domainName+i+1, nameServer);
            else
                sprintf(path, "%s %s",domainName+i, nameServer);
            getServer("NS", path, nameServer);
        }
    }

    //Final Query to Authoritative server
    memset(path, 0, 100);

    if(!strcmp(type,"CNAME"))
        sprintf(path, "%s %s", qstnDomainName, nameServer);
    else
        sprintf(path, "%s %s", domainName, nameServer);
    getServer(type, path, nameServer);
    printf("Address : %s\n",nameServer);

    strcpy(output, nameServer);         //answer saved to output
}

void *handleLookup(void *Arg)
{
    horizontal();

    HLArg *arg = (HLArg *)Arg;
    char iterativeAnswer[100];

    DNSHeader reqHeader;
    DNSQuestion reqQstn;
    DNSAns ans;

    parseHeader(arg->Buf, &reqHeader);
    parseQuestion(arg->Buf + 12, &reqQstn);

    if(!checkCache(&reqQstn, iterativeAnswer))
        iterativeQuery(reqQstn, iterativeAnswer);

    resolveAnswer(&reqQstn, &ans, iterativeAnswer);

    char sendBuf[SIZE];
    unsigned packetSize;
    packetSize = createResponse(&reqHeader, &reqQstn, &ans, sendBuf);

    if (sendto(arg->sock, sendBuf, packetSize, 0, (struct sockaddr *)&(arg->clientAddr), sizeof(arg->clientAddr)) < 0)
        perror("[-]sendto() failed\n");
    else
        printf("[+]Response Sent\n");
    printf("END\n");

    horizontal();

    free(arg);
}

int main(int argc, char *argv[])
{

    unsigned short my_port;

    //Confirm PORT no is given
    if (argc < 2)
    {
        printf("\nPORT : ");
        scanf("%hu", &my_port);
    }
    else
    {
        my_port = atoi(argv[1]);
    }

    //Creating Socket

    int sock;
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket() failed");
        return 1;
    }

    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0)
        perror("setsockopt() failed");

    //bind socket to address

    struct sockaddr_in server_addr;

    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(my_port);

    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind() failed");
        return 1;
    }

    printf("[+]Local DNS Server Listening on port %hu.....\n\n", my_port);

    //Cache
    FILE* Cache = fopen("cache.txt", "w+");

    if(Cache == NULL)
        printf("Cache file not opened");

    struct sockaddr_in clientAddr;
    int client_addrLen = sizeof(clientAddr);
    int temp_sock;

    char recvBuf[SIZE];
    int recvLen;

    HLArg *arg;
    while (1)
    {
        if ((recvLen = recvfrom(sock, recvBuf, SIZE - 1, 0, (struct sockaddr *)&clientAddr, &client_addrLen)) < 0)
        {
            perror("recvfrom() failed");
        }
        else
        {
            arg = (HLArg *)malloc(sizeof(HLArg));
            for (int i = 0; i < SIZE; ++i)
            {
                arg->Buf[i] = recvBuf[i];
            }
            arg->sock = sock;
            arg->clientAddr = clientAddr;
            pthread_t tId;
            pthread_create(&tId, NULL, handleLookup, (void *)arg);
            printf("[+]RequestHandler assigned\n");
        }
    }

    fclose(Cache);
    return 0;
}