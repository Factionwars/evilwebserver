#ifndef EVILNETLIB_HEADER
#define EVILNETLIB_HEADER

#define EOL "\n"
#define EOL_LENGTH 1
#define BACKLOG 100
#define PHP_COMMAND "/usr/bin/php-cgi"
#define PHP_FILE "scripts/test.php"

#define PYTHON_COMMAND "/usr/bin/python2"
#define PYTHON_FILE "scripts/cgi_test.py"

#define CGI_ERRORS 1
#define DROP_UID 33
#define DROP_GID 33

#define SERVER_NAME "EvilTinyHTTPD"
#define SERVER_PORT 1337
#define SERVER_PORT_CGI "1337"
#define SERVER_SOFTWARE "EvilWebserver v0.2"

typedef struct { 
    int sockfd;
    struct sockaddr_in * addr;
    char http_version[4];
} http_client_t;

struct http_header{
    char * name;
    char * value;
    struct http_header * next;
};

//request_type = 1:GET, 2:POST
typedef struct {
    int request_type;
    struct http_header * headers;
    char * request_uri;
    char * request_query;    
    int content_length;
    char * content_body;
    http_client_t * client;
} http_request_t;

http_client_t * initClientContainer();
void cleanUpClient(http_client_t * client, http_request_t * http_request);
void initCGI();

int connectTo(struct in_addr *host, int port);
int listenOn(int port);
int acceptClient(int sockfd, struct sockaddr_in* client_addr);
int sendString(int sockfd, char *buffer);
int sendHeader(int sockfd, char *message, char *value);
int sendFile(int sockfd, char *file_name);

char ** addEnv(char ** envp, char * name, char * value, int * length);
int sendCGI(int sockfd, http_request_t* http_request, char * command, char * script);
int sendPHP(int sockfd, http_request_t* http_request);
int sendPython(int sockfd, http_request_t* http_request);
int recvLine(int sockfd, char *buffer, int max_size);
int get_file_size(int fd);

struct in_addr* lookUpHost(char * host);
#endif