#ifndef EVILNETLIB_HEADER
#define EVILNETLIB_HEADER

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
#define BACKLOG 100
#define PHP_COMMAND "php-cgi html/test.php "

typedef struct { 
    int sockfd;
    struct sockaddr_in * addr;
    char http_version[4];
} http_client_t;

//request_type = 1:GET, 2:POST
typedef struct {
    int request_type;
    char * request_string;
} http_request_t;

int connectTo(struct in_addr *host, int port);
int listenOn(int port);
int acceptClient(int sockfd, struct sockaddr_in* client_addr);
int sendString(int sockfd, unsigned char *buffer);
int sendHeader(int sockfd, unsigned char *message, unsigned char *value);
int sendFile(int sockfd, unsigned char *file_name);
int sendPHP(int sockfd, http_request_t* http_request);
int recvLine(int sockfd, unsigned char *buffer, int max_size);
int get_file_size(int fd);

struct in_addr* lookUpHost(char * host);

#include "evilnetlib.c"

#endif