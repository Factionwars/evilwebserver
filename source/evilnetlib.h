#ifndef EVILNETLIB_HEADER
#define EVILNETLIB_HEADER

#define EOL "\n"
#define EOL_LENGTH 1
#define BACKLOG 100
#define PHP_COMMAND "php-cgi"
#define PHP_FILE "html/test.php"
#define DROP_UID 33
#define DROP_GID 33

typedef struct { 
    int sockfd;
    struct sockaddr_in * addr;
    char http_version[4];
} http_client_t;

//request_type = 1:GET, 2:POST
typedef struct {
    int request_type;
    char * request_uri;
    char * request_host;
    char * user_agent;
    char * accept;
    char * accept_language;
    char * accept_encoding;
    char * connection;
    char * content_type;
    char * content_length;
} http_request_t;

http_client_t * initClientContainer();
void cleanUpClient(http_client_t * client, http_request_t * http_request);

int connectTo(struct in_addr *host, int port);
int listenOn(int port);
int acceptClient(int sockfd, struct sockaddr_in* client_addr);
int sendString(int sockfd, char *buffer);
int sendHeader(int sockfd, char *message, char *value);
int sendFile(int sockfd, char *file_name);
int sendPHP(int sockfd, http_request_t* http_request);
int recvLine(int sockfd, char *buffer, int max_size);
int get_file_size(int fd);

struct in_addr* lookUpHost(char * host);
#endif