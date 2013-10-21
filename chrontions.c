#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include <netdb.h>

#define EOL "\n"
#define EOL_LENGTH 1
#define BACKLOG 3
#define initPre "chron:"

int connectTo(struct in_addr *host, int port)
{
    int sockfd;
    struct sockaddr_in target_addr;
    unsigned char buffer[4096];

    if((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
        return -1;    

    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(port);
    target_addr.sin_addr = *host; 
    memset(&(target_addr.sin_zero), '\0', 8);

    if(connect(sockfd, (struct sockaddr *)&target_addr, sizeof(struct sockaddr)) == -1)
        return -1;
    
    return sockfd;
}

int listenOn(int port)
{
    int sockfd;

    int new_sockfd, yes = 1;

    struct sockaddr_in host_addr;


    if((sockfd = socket(PF_INET, SOCK_STREAM , 0)) == -1)
        return -1;

    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        return -1;

    host_addr.sin_family = AF_INET;
    host_addr.sin_port = htons(port);
    host_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(host_addr.sin_zero), '\0', 8);

    if(bind(sockfd, (struct sockaddr*)&host_addr, sizeof(struct sockaddr)) == -1)
        return -1;

    if(listen(sockfd, BACKLOG) == -1)
        return -1;

    return sockfd;

}

int acceptClient(int sockfd, struct sockaddr_in* client_addr)
{
    int new_sockfd;
    socklen_t sin_size;
    sin_size = sizeof(struct sockaddr_in);
    new_sockfd = accept(sockfd, (struct sockaddr*)client_addr, &sin_size);

    return new_sockfd;
}

int sendInit(int sockfd, int type, unsigned char *value)
{
    unsigned char * init_string;
    int preLength = strlen(initPre);
    if((init_string = (unsigned char *)malloc(preLength + 2 + EOL_LENGTH + strlen(value))) == NULL)
        return -1;
    strcpy(init_string, initPre);
    init_string[preLength + 0] = '0' + type;
    init_string[preLength + 1] = ',';

    strcpy(init_string + preLength +  2, value);
    
    strcpy(init_string + preLength + 2 + strlen(value), EOL);

    int status = sendString(sockfd, init_string);
    free(init_string);
    return status;
}

int readInit(int sockfd)
{
    char *initString;

    if(recvLine(sockfd, initString) == -1)
        return -1;

    if(strncmp(initString, initPre, strlen(initPre)) != 0)
        return 0;

    int type;
    char ** endPtr;
    type = strtol( (initString + strlen(initPre)), endPtr, 10);
    printf("The type read is %d", type);
    return 1;
}

int sendString(int sockfd, unsigned char *buffer) 
{
    int send_bytes = 0;
    int bytes_to_send = strlen(buffer);
    while(bytes_to_send > 0){
        send_bytes = send(sockfd, buffer, bytes_to_send, 0);
        if(send_bytes == -1)
            return -1;
        bytes_to_send -= send_bytes;
        buffer += send_bytes;
    }
    return 1;
}

int sendFile(int sockfd, unsigned char *file_name)
{
    int file, length;
    unsigned char * ptr;
    unsigned char * bPtr;
    if((file = open(file_name, O_RDONLY, 0)) == -1)
        return -1;
    length = get_file_size(file);
    if((ptr = (unsigned char *)malloc(length)) == NULL)
        return -1;
    bPtr = ptr;
    if(read(file, ptr, length) == -1)
        return -1;

    int send_bytes = 0;
    int bytes_to_send = length;

    while(bytes_to_send > 0){
        send_bytes = send(sockfd, ptr, bytes_to_send, 0);
        if(send_bytes == -1)
            return -1;
        bytes_to_send -= send_bytes;
        ptr += send_bytes;
    }
    free(bPtr);
    return send_bytes;

}

int recvLine(int sockfd, unsigned char *buffer) 
{
    unsigned char *ptr = buffer;
    while(recv(sockfd, ptr, 1, 0) == 1){
        if(*ptr == '\n'){ 
            *(ptr) = '\0';
            return strlen(buffer);
        } 
        ptr++;
    }
    return 0;

}

struct in_addr* lookUpHost(char * host)
{
    struct hostent *host_info;
    struct in_addr *address;

    host_info = gethostbyname(host);
    if(host_info == NULL){
            address = NULL;
    } else {
            address = (struct in_addr *)(host_info->h_addr);
    }
    return address;
}

int get_file_size(int fd) {
    struct stat stat_struct;
    if(fstat(fd, &stat_struct) == -1)
        return -1;
    return (int) stat_struct.st_size;
}