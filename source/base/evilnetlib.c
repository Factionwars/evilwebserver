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

//Cgi function
#include "object.h"

/** 
 *  @file   evilnetlib.c
 *  @brief  Networking library with a focus on HTTP
 *  @author Factionwars@evilzone.org
 *  @co-authors You and you
 */

/**
 * @brief This function assigns a new client container
 * @return New http_client_t container pointer
 */
http_client_t * initClientContainer()
{
    http_client_t *client_container;
    client_container = object_init(sizeof(http_client_t));
    client_container->addr = object_ninit(sizeof(struct sockaddr_in));
    client_container->sockfd = 0;
    return client_container;
}

/**
 * @brief Cleans up a client container and a http_request container
 * @return void
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
    unsigned int header_length = strlen(message) + strlen(value);

    char * header = malloc(header_length + 5);
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
    char filetype_format[] = "Content-Type: %s; charset=UTF-8\r\n";
    char filetype[] = "text/html";
    char * filetype_header =
        object_ninit((strlen(filetype_format) + strlen(filetype) - 1) * sizeof(char));
    sprintf(filetype_header, filetype_format, filetype);

    sendString(sockfd, filetype_header);
    free(filetype_header);

    sendString(sockfd, "\r\n");

    if((ptr = malloc(length * sizeof(char))) == NULL)
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

char ** addEnv(char ** envp, char * name, char * value, int * length)
{
    envp = realloc(envp,  sizeof(char *) * (*length + 2));
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


/**
 * @brief Receive a line untill EOL(end of line) character
 * @param sockfd remote socket to receive from
 * @param buffer buffer to store message in
 * @param max_size max line size, do not exceed buffer size
 * @return line length or negative on failure
 */

static __thread char  line_buffer[MAX_HEADER_LENGTH] = {0};
static __thread unsigned int buffered = 0;
static __thread unsigned int line_pos = 0;
const int max = MAX_HEADER_LENGTH;

int getLine(int sockfd, char *buffer) 
{


    int buffer_pos = 0;
    char * line_end;
    line_end = memchr(&line_buffer[line_pos], '\n', max - line_pos);
    if(line_end == NULL){
        if(buffered > 0){  
            strncpy(buffer + buffer_pos, &line_buffer[line_pos], buffered);
            buffer_pos += buffered;
            buffered = 0;        
        }
        buffered = recv(sockfd, line_buffer, max, 0);
        if(buffered == 0){
            return 0;
        }
        //error checking on recv
        line_pos = 0;
        line_end = memchr(&line_buffer[line_pos], '\n', max);

        if(line_end == NULL || line_end > (line_buffer + max) ){
            //Howdoe!
            buffered = 0;
            line_pos = 0;
            strncpy(buffer + buffer_pos, &line_buffer[0], max - buffer_pos);
            printf("fuck %d\n", max - line_pos);
            return 0;
        }

    } 

    int line_length = line_end - &line_buffer[line_pos];
    strncpy(&buffer[buffer_pos], &line_buffer[line_pos], line_length);
    buffered -= line_length;

    line_pos += line_length + 1;
    buffer[line_length] = '\0';

    return strlen(buffer);

}

int flushBuffer(char *buffer, unsigned int max_buffer)
{
    //Copy remaining bytes from buffer
    buffered = strlen(&line_buffer[line_pos]);
    if(buffered > max_buffer)
        buffered = max_buffer;
    strncpy(buffer, &line_buffer[line_pos], buffered) ;
    buffer[buffered] = '\0';
    int copied = buffered;
    //Set buffer stats to zero
    line_pos = 0;
    buffered = 0;
    return copied;
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
 * @brief Read a file into memory and returns a pointer to it
 * @param char * filename
 * @return char * ptr
 */
char * readFile(char * filename)
{
    int file, length;
    char * ptr; /* Contents of json file */
    if((file = open(filename, O_RDONLY, 0)) == -1){
        return NULL;
    }

    length = get_file_size(file);

    if((ptr = malloc(length * sizeof(char) )) == NULL){
        return NULL;
    }
    //TODO: Make sure everything is read?
    if(read(file, ptr, length) == -1){
        return NULL;
    }
    close(file);
    return ptr;
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
