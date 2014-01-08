/** 
 *  @file   evilnetlib.c
 *  @brief  Networking library with a focus on HTTP
 *  @author Factionwars@evilzone.org
 *  @co-authors You and you
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <netdb.h>
 
#include "evilnetlib.h"


/**
 * @brief This function assigns a new client container
 * @return New http_client_t container pointer
 */
http_client_t * initClientContainer()
{
    http_client_t *client_container;
    client_container = malloc(sizeof(http_client_t));
    client_container->addr = malloc(sizeof(struct sockaddr_in));
    client_container->sockfd = 0;
    return client_container;
}

/**
 * @brief Cleans up a client container and a http_request container
 * @return New http_client_t container pointer
 */
void cleanUpClient(http_client_t * client, http_request_t * http_request)
{
    if(client->sockfd != 0) {
        close(client->sockfd);
        client->sockfd = 0;
    }
    if(client->addr != NULL){
        free(client->addr);
        client->addr = NULL;
    }
    if(client != NULL){
        free(client);
        client = NULL;
    }
    if(http_request->request_uri != NULL){
        free(http_request->request_uri);
        http_request->request_uri = NULL;
    }
    if(http_request->request_query != NULL){
        free(http_request->request_query);
        http_request->request_query = NULL;
    }
    if(http_request->content_body != NULL){
        free(http_request->content_body);
        http_request->content_body = NULL;
    }
    if(http_request->headers != NULL){
        struct http_header * previous;
        //Clean linked list
        while(http_request->headers != NULL){            
            if(http_request->headers->name != NULL)
                free(http_request->headers->name);
            if(http_request->headers->value != NULL)
                free(http_request->headers->value);
            previous = http_request->headers;
            http_request->headers = http_request->headers->next;
            free(previous);
        }
        free(http_request->headers);
        http_request->headers = NULL;
    }
    if(http_request != NULL) {
        free(http_request);
        http_request = NULL;
    }
}

/**
 * @brief This function connects to a in_addr struck and port
 * @param *host Location struct
 * @param port Location Port
 * @return Sockfd or negative on error
 */
int connectTo(struct in_addr *host, int port)
{
    int sockfd;
    struct sockaddr_in target_addr;
    
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

/**
 * @brief Create a socket and listen on it.
 * @param port Local Port
 * @return Sockfd or negative on error
 */
int listenOn(int port)
{
    int sockfd;
    int yes = 1;

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

/**
 * @brief Accept client and return client socket
 * @param sockfd server socket
 * @param client_addr empty sockaddr_in* to be filled
 * @return new Sockfd or negative on error
 */
int acceptClient(int sockfd, struct sockaddr_in* client_addr)
{
    int new_sockfd;
    socklen_t sin_size;
    sin_size = sizeof(struct sockaddr_in);
    new_sockfd = accept(sockfd, (struct sockaddr*)client_addr, &sin_size);

    return new_sockfd;
}

/**
 * @brief Send a string to a socket
 * @param sockfd remote socket to send to
 * @param buffer string to send
 * @return 1 on success negative on failure
 */
int sendString(int sockfd, char *buffer) 
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

/**
 * @brief Send a HTTP header
 * @param sockfd remote socket to send to
 * @param message For example "Date"
 * @param value For example "Thu, 21 Dec 2012 14:06:53 GMT"
 * @return 0 or negative on failure
 */
int sendHeader(int sockfd, char *message, char *value)
{
    int header_length = strlen(message) + strlen(value);
    char * header = malloc(header_length);
    //Render header
    sprintf(header,"%s: %s\r\n", message, value);
    //Send and free the header ptr
    int ret = 0;
    if(sendString(sockfd, header) < 0)
        ret = -1;

    free(header);
    return ret;
}

/**
 * @brief Send a file
 * @param sockfd remote socket to send to
 * @param *file_name Name of the file to send
 * @return send bytes or negative on failure
 */
int sendFile(int sockfd, char *file_name)
{

    int file, length;
    //Pointer to play with
    char * ptr;
    //Backup pointer to ptr
    char * bPtr;
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

    if((ptr = malloc(length)) == NULL)
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

void initCGI()
{
 
}

char ** addEnv(char ** envp, char * name, char * value, int * length)
{
    envp = realloc(envp, (*length + 1) * sizeof(char *));
    if (envp == NULL) {
        perror("realloc");
        exit(EXIT_FAILURE);
    }
    char * vptr = malloc( (2 + strlen(name) + strlen(value)) * sizeof(char));
    if(vptr == NULL){
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    sprintf(vptr, "%s=%s", name, value);
    envp[*length] = vptr;
    *length += 1;
    return envp;
}

/**
 * @brief Open PHP-CGI session and sends it to the socket
 * @param sockfd remote socket to send to
 * @param http_request http_request object
 * @return 0 or negative on failure
 */
int sendPHP(int sockfd, http_request_t* http_request)
{
    return sendCGI(sockfd, http_request, PHP_COMMAND, PHP_FILE);
}

int sendPython(int sockfd, http_request_t* http_request)
{
    return sendCGI(sockfd, http_request, PYTHON_COMMAND, PYTHON_FILE);
}


int sendCGI(int sockfd, http_request_t* http_request, char * command, char * script) 
{
    //LENGTH INCREMEATION IS FAULTY

    //Set environment variables
    char ** envp = NULL;
    char * raw;
    //envp = malloc(sizeof(char*));

    //envp[0] = malloc(sizeof(char *));

    int envp_length = 0;
 
    envp = addEnv(envp, "REDIRECT_STATUS", "200", &envp_length);

    if(http_request->request_type == 1){
        envp = addEnv(envp, "REQUEST_METHOD", "GET", &envp_length);
    } else if(http_request->request_type == 2){
       envp = addEnv(envp, "REQUEST_METHOD", "POST", &envp_length);
        if(http_request->content_body != NULL)
            envp = addEnv(envp, "BODY", http_request->content_body, &envp_length); 
        //Render content Length       
        char clength[4];
        snprintf(clength, 4 ,"%d", http_request->content_length);
        envp = addEnv(envp, "CONTENT_LENGTH", clength, &envp_length);
    }
    if(http_request->request_uri != NULL)
        envp = addEnv(envp, "PATH_INFO", http_request->request_uri, &envp_length);
    if(http_request->request_query != NULL)
        envp = addEnv(envp, "QUERY_STRING", http_request->request_query, &envp_length);
    //Remote ADDR Bugs
    envp = addEnv(envp, "REMOTE_ADDR", inet_ntoa(http_request->client->addr->sin_addr), &envp_length);
    
    envp = addEnv(envp, "SCRIPT_FILENAME", script, &envp_length);

    //char * content_length = malloc(5);
    //snprintf(content_length, 5, "%d",(int)strlen(http_request->request_uri));
    //TODO: Add remote host
    //setenv("REMOTE_HOST", inet_ntoa(client->addr->sin_addr));
    envp = addEnv(envp, "HTTP_ACCEPT", "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\0", &envp_length);
    envp = addEnv(envp, "CONTENT_TYPE", "application/x-www-form-urlencoded\0", &envp_length);
    envp = addEnv(envp, "GATEWAY_INTERFACE","CGI/1.1\0", &envp_length);
    envp = addEnv(envp, "SERVER_NAME", SERVER_NAME, &envp_length);
    envp = addEnv(envp, "SERVER_PROTOCOL", "HTTP/1.1\0", &envp_length);
    envp = addEnv(envp, "SERVER_PORT", SERVER_PORT_CGI, &envp_length);
    envp = addEnv(envp, "SERVER_SOFTWARE", SERVER_SOFTWARE, &envp_length);

    //Argv list
    char *argv[] = { command , script , 0 };
    //Close environment list
    envp = realloc(envp, (envp_length) * sizeof(envp[0]));
    envp[envp_length] = 0;

    pid_t pid;
    int pipes[4];

    /* Warning: I'm not handling possible errors in pipe/fork */

    pipe(&pipes[0]); /* Parent read/child write pipe */
    pipe(&pipes[2]); /* Child read/parent write pipe */

    if ((pid = fork()) > 0) {
        /* Parent process */
        //pipes[0]; /* Read end */
        //pipes[3]; /* Write end*/

        close(pipes[1]);
        close(pipes[2]);

        char buffer[1024];
        int ret = 0;
        //If request is post write the body 
        if(http_request->request_type == 2){
            int to_write = http_request->content_length;
            int written = 0;
            char * bptr; /* body pointer*/
            bptr = http_request->content_body;
            while(to_write > 0){
                written = write(pipes[3], bptr, to_write);
                to_write -= written;
                bptr += written;
            }
        }
        //Read the CGI output from the pipe and send it to the client
        int received = 0;
        while (received = read(pipes[0], buffer, 1023)) {
            buffer[received] = '\0';
            if(sendString(sockfd, (char *)buffer) < 0){
                ret = -1;
                break;
            }
        }
        close(pipes[0]);
        close(pipes[3]);
        /*  Bugger  TODO: clean this cleanup
        while(envp_length--)
            free(envp[envp_length]);
        free(envp);
        */

    } else {
        close(pipes[0]);
        close(pipes[3]);

        dup2(pipes[1], fileno(stdout));
        dup2(pipes[2], fileno(stdin));

        execve(command, &argv[0], envp);     
        printf("Failed to launch CGI application\n");
        exit(0);
    }

    return -1;

    /*
    
    FILE *child = popen(command, "r");

    // error checking omitted.
    char * buffer[1024];
    int ret = 0;
    //Read the PHP-CGI output from the FILE pipe and send it to the client
    while (fgets((char *)buffer, 1024 - 2, child)) {
        buffer[1024 - 1] = '\0';
        if(sendString(sockfd, (char *)buffer) < 0){
            ret = -1;
            break;
        }
    }

    //Cleanup 
    return ret;
    */
}

/**
 * @brief Receive a line untill EOL(end of line) character
 * @param sockfd remote socket to receive from
 * @param buffer buffer to store message in
 * @param max_size max line size, do not exceed buffer size
 * @return line length or negative on failure
 */
int recvLine(int sockfd, char *buffer, int max_size) 
{
    char *ptr = buffer;
    while(recv(sockfd, ptr, 1, 0) == 1 && (ptr - buffer) <= (max_size / 2) ){
        if(*ptr == '\n'){ 
            break;
        }
        ptr++;
    }
    *(ptr) = '\0';
    return strlen(buffer);

}

/**
 * @brief Lookup host by hostname and return in_addr struct
 * @param *host hostname
 * @return struct in_addr* or NULL on failure
 */
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

/**
 * @brief Get the size of a file
 * @param fd File socket
 * @return Filesize or negative on failure
 */
int get_file_size(int fd) {
    struct stat stat_struct;
    if(fstat(fd, &stat_struct) == -1)
        return -1;
    return (int) stat_struct.st_size;
}


/* Some test functions for a custom protocol
int sendInit(int sockfd, int type, char *value)
{
    char * init_string;
    int preLength = strlen(initPre);
    if((init_string = (char *)malloc(preLength + 2 + EOL_LENGTH + strlen(value))) == NULL)
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
