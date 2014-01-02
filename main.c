/** 
 *  @file   main.c
 *  @brief  HTTP webserver based on chrontions library
 *  @author Factionwars@evilzone.org
 *  @co-authors You and you
 */
#include "evilnetlib.h"
#include "server.h"


#define SERVER_NAME "EvilTinyHTTPD"

long long requests = 0;
pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char * argv[])
{
    return server();
}

int server()
{
    int sock_server;

    sock_server = listenOn(1337);


    http_client_t *client_container;

    client_container = initClientContainer();

    while(client_container->sockfd 
            = acceptClient(sock_server, client_container->addr)) {

        pthread_t client_thread;
        pthread_create( &client_thread,
                NULL,
                &handleClient,
                (void *)client_container);
        pthread_detach( client_thread );

        client_container = initClientContainer();
    }
    close(sock_server);

    return 0;
}

http_client_t * initClientContainer()
{
    http_client_t *client_container;
    client_container = (http_client_t *)malloc(sizeof(http_client_t));
    client_container->addr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
    client_container->sockfd = 0;
    return client_container;
}

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
    if(http_request->request_string != NULL){
        free(http_request->request_string);
        http_request->request_string = NULL;
    }
    if(http_request->request_string != NULL) {
        free(http_request);
        http_request = NULL;
    }
}

void logError(int level, http_client_t * client, http_request_t * http_request)
{
    printf("Error level %d occured\n", level);
    cleanUpClient(client, http_request);
    pthread_exit(0);
}

void *handleClient(void *client_void)
{

    http_client_t *client = (http_client_t*)client_void;
    http_request_t *http_request;
    char buffer[4000];

    if( ( http_request = (http_request_t *)malloc(sizeof(http_request_t)) ) == NULL)
        logError(3, client, http_request);
    http_request->request_string = NULL;
    http_request->request_type = 0;

    printf("Got a connection from %s on port %d\n", inet_ntoa(client->addr->sin_addr), ntohs(client->addr->sin_port));

    int first = 0;

    while(recvLine(client->sockfd, buffer, 4000))
    {
        if(first == 0) {
            if(strncasecmp(buffer, "GET", 3) == 0) {
                http_request->request_type = 1;
                http_request->request_string = strdup(buffer+4);            
            } else if(strncasecmp(buffer, "POST", 4) == 0) {
                http_request->request_type = 2;
                http_request->request_string = strdup(buffer+5); 
            } 
            first++;
        }
         printf("received: %s\n", buffer);

         if(buffer[0] == '\xd')
            break;
    }    

    char buf[2000];
    time_t now = time(0);
    struct tm tm = *gmtime(&now);
    strftime(buf, sizeof buf, "%a, %d %b %Y %H:%M:%S %Z", &tm);

    if(http_request->request_string != NULL){
        unsigned int i;
        for(i = 0; i < strlen(http_request->request_string); i++){
            if(http_request->request_string[i] == ' '){
                if(strncasecmp(http_request->request_string + i + 1, "HTTP", 4) == 0){
                    strncpy(client->http_version, http_request->request_string + i + 6, 3); 
                    client->http_version[3] = '\0';
                }                      
                http_request->request_string[i] = '\0';
            }
        }
    }

    if(http_request->request_type == 1 || http_request->request_type == 2)
    {        
        if(http_request->request_string != NULL) {
            sendString(client->sockfd, "HTTP/1.1 200 OK\r\n");
            sendHeader(client->sockfd, "Server", SERVER_NAME);
            sendHeader(client->sockfd, "Date", buf);

            //sendFile(client->sockfd, "html/index.html");
            sendPHP(client->sockfd, http_request);
        } else {
            sendString(client->sockfd, "HTTP/1.1 404\r\n");
            sendHeader(client->sockfd, "Server", SERVER_NAME);
            sendHeader(client->sockfd, "Date", buf);

            sendFile(client->sockfd, "html/404.html");
        }
    } else {
        sendString(client->sockfd, "HTTP/1.1 502\r\n");
        sendHeader(client->sockfd, "Server", SERVER_NAME);
        sendHeader(client->sockfd, "Date", buf);
    }

    printf("Closing connection to %s on port %d\n", inet_ntoa(client->addr->sin_addr), ntohs(client->addr->sin_port));

    cleanUpClient(client, http_request);


    pthread_mutex_lock(&count_mutex);
    requests++;
    pthread_mutex_unlock(&count_mutex);
    pthread_exit(0);
}

