typedef struct { 
    int sockfd;
    struct sockaddr_in * addr;
} http_client_t;

typedef struct {
    int is_get;
    char * string_get;
} http_request_t;

int server();
void *handleClient(void *client_void);
http_client_t * initClientContainer();