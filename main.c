#include <stdio.h>
#include <pthread.h>
#include "server.h"
#include "chrontions.c"

long long open_connections = 0;
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

    while(client_container->sockfd = acceptClient(sock_server, client_container->addr)) {

        pthread_t client_thread;
        pthread_create( &client_thread, NULL, &handleClient, (void *)client_container);
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

void cleanUpClient(http_client_t * client)
{
    close(client->sockfd);
    free(client->addr);
    free(client);
}

void logError(int level)
{
    printf("Error level %d occured\n", level);
    pthread_exit(0);
}

void *handleClient(void *client_void)
{

    http_client_t *client = (http_client_t*)client_void;
    http_request_t *http_request;
    char buffer[4000];

    if( ( http_request = (http_request_t *)malloc(sizeof(http_request_t)) ) == NULL)
        logError(3);

    printf("Got a connection from %s on port %d\n", inet_ntoa(client->addr->sin_addr), ntohs(client->addr->sin_port));

    while(recvLine(client->sockfd, buffer))
    {
         printf("received: %s\n", buffer);
         if(strncasecmp(buffer, "GET", 3) == 0) {
            http_request->is_get = 1;
            http_request->string_get = strdup(buffer);            
         }

         if(buffer[0] == '\xd')
            break;
    }    

    printf("received: %s\n", buffer);

    if(http_request->is_get == 1)
    {        
        if(strncasecmp(http_request->string_get, "GET / ", 6) == 0) {
            sendString(client->sockfd, "HTTP/1.1 200 OK\r\n");
            sendString(client->sockfd, "Content-Type: text/html; charset=UTF-8\r\n");
            sendString(client->sockfd, "\r\n");
            sendFile(client->sockfd, "html/index.html");
        } else {
            sendString(client->sockfd, "HTTP/1.1 404\r\n");
            sendString(client->sockfd, "\r\n");
            sendFile(client->sockfd, "html/404.html");
        }
    } else {
        sendString(client->sockfd, "HTTP/1.1 502\r\n");
    }

    printf("Closing connection to %s on port %d\n", inet_ntoa(client->addr->sin_addr), ntohs(client->addr->sin_port));

    free(http_request);
    cleanUpClient(client);


    pthread_mutex_lock(&count_mutex);
    requests++;
    pthread_mutex_unlock(&count_mutex);
    pthread_exit(0);
}

