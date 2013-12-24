#ifndef SERVER_HEADER
#define SERVER_HEADER
#include <time.h>

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

int server();
void *handleClient(void *client_void);
http_client_t * initClientContainer();

#endif