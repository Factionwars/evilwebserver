#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include <netdb.h>

#include "server.h"

#define EOL "\n"
#define EOL_LENGTH 1
#define BACKLOG 100
#define initPre "chron:"
#define PHP_COMMAND "php-cgi html/test.php "

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

    //Create a TCP socket
    if((sockfd = socket(PF_INET, SOCK_STREAM , 0)) == -1)
        return -1;
    //Set socket options for quick reuse (Good for debugging)
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        return -1;
    //Fill in server addr struct
    host_addr.sin_family = AF_INET;
    host_addr.sin_port = htons(port);
    host_addr.sin_addr.s_addr = INADDR_ANY;
    //Fill the struct padding with zero's
    memset(&(host_addr.sin_zero), '\0', 8);

    //Bind the socket to the addr struct
    if(bind(sockfd, (struct sockaddr*)&host_addr, sizeof(struct sockaddr)) == -1)
        return -1;

    //Turn on listen mode for the socket and give it a backlog (Max unhandled connections)
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

int sendString(int sockfd, unsigned char *buffer) 
{
    //Pretty default function for sending a buffer to socket
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

int sendHeader(int sockfd, unsigned char *message, unsigned char *value)
{
    int header_length = strlen(message) + strlen(value);
    char * header = (char*)malloc(header_length);
    //Render header
    sprintf(header,"%s: %s\r\n", message, value);
    //Send and free the header ptr
    sendString(sockfd, header);
    free(header);
}

int sendFile(int sockfd, unsigned char *file_name)
{

    int file, length;
    //Pointer to play with
    unsigned char * ptr;
    //Backup pointer to ptr
    unsigned char * bPtr;
    //Open file and get the filesize
    if((file = open(file_name, O_RDONLY, 0)) == -1)
        return -1;
    length = get_file_size(file);

    //Start sending the HTTP file headers
    int header_length = strlen("Content-Length: ") + 5;
    char length_header[header_length];
    snprintf(length_header, header_length ,"Content-Length: %d\r\n", length);
    sendString(sockfd, length_header);
    //TODO: Different file support
    sendString(sockfd, "Content-Type: text/html; charset=UTF-8\r\n");

    sendString(sockfd, "\r\n");

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
    //Free the original ptr
    free(bPtr);
    //Return the send_bytes 
    return send_bytes;

}

int sendPHP(int sockfd, http_request_t* http_request) 
{
    //Render the PHP command 
    char * command;
    command = (char *)malloc(strlen(PHP_COMMAND) + strlen(http_request->request_string) + 1);
    strcpy(command, PHP_COMMAND);
    strcpy(command + strlen(PHP_COMMAND), http_request->request_string);

    command[strlen(command) - 1] = '\0';
    
    FILE *child = popen(command, "r");

    // error checking omitted.
    char * buffer = (char *)malloc(1024);

    //Read the PHP-CGI output from the FILE pipe and send it to the client
    while (fgets(buffer, 1024 - 1, child)) {
        buffer[1024] = '\0';
        sendString(sockfd, buffer);
    }
    //Cleanup 
    free(command);
    free(buffer);
}

int recvLine(int sockfd, unsigned char *buffer, int max_size) 
{
    unsigned char *ptr = buffer;
    while(recv(sockfd, ptr, 1, 0) == 1 && (ptr - buffer) <= (max_size / 2) ){
        if(*ptr == '\n'){ 
            break;
        }
        ptr++;
    }
    *(ptr) = '\0';
    return strlen(buffer);

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


/* Some test functions for a custom protocol
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
}*/
