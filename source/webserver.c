/** 
 *  @file   webserver.c
 *  @brief  HTTP webserver based on evilnetlib library
 *  @author Factionwars@evilzone.org
 *  @co-authors You and you
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

#include "evilnetlib.h"
#include "webserver.h"

long long requests = 0;
pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;

int main()
{
    return server();
}

int server()
{
    int sock_server;
    //listen on the server port
    sock_server = listenOn(SERVER_PORT);
    //Create a pointer to keep the client in
    http_client_t *client_container;
    //Init the first client container
    client_container = initClientContainer();
    //Accept clients
    while((client_container->sockfd 
        = acceptClient(sock_server, client_container->addr))) {
        //Create a new thread to assign to the new client
        pthread_t client_thread;
        pthread_create( &client_thread,
                NULL,
                &handleClient,
                (void *)client_container);
        //Detach the client, from this point the thread will live it's own life
        pthread_detach( client_thread );
        //Create a new client container  for the next newcomer
        client_container = initClientContainer();
    }

    close(sock_server);
    return 0;
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
    http_request->request_uri = NULL;
    http_request->request_type = 0;
    http_request->client = client;
    printf("Got a connection from %s on port %d\n", inet_ntoa(client->addr->sin_addr), ntohs(client->addr->sin_port));

    int first = 0;

    while(recvLine(client->sockfd, buffer, 4000))
    {
        if(first == 0) {
            if(strncasecmp(buffer, "GET", 3) == 0) {
                http_request->request_type = 1;
                http_request->request_uri = strdup(buffer+4);            
            } else if(strncasecmp(buffer, "POST", 4) == 0) {
                http_request->request_type = 2;
                http_request->request_uri = strdup(buffer+5); 
            } 
            first++;
        } else {
            if(http_request->user_agent != NULL 
                && strncasecmp(buffer, "User-Agent", 10) == 0) {
                http_request->user_agent = strdup(buffer+10);            
            } else if(strncasecmp(buffer, "Accept", 6) == 0) {
                http_request->request_uri = strdup(buffer+6); 
            } 
        }
         printf("received: %s\n", buffer);

         if(buffer[0] == '\xd')
            break;
    }    

    char buf[2000];
    time_t now = time(0);
    struct tm tm = *gmtime(&now);
    strftime(buf, sizeof buf, "%a, %d %b %Y %H:%M:%S %Z", &tm);

    if(http_request->request_uri != NULL){
        unsigned int i;
        for(i = 0; i < strlen(http_request->request_uri); i++){
            if(http_request->request_uri[i] == ' '){
                if(strncasecmp(http_request->request_uri + i + 1, "HTTP", 4) == 0){
                    strncpy(client->http_version, http_request->request_uri + i + 6, 3); 
                    client->http_version[3] = '\0';
                }                      
                http_request->request_uri[i] = '\0';
            }
        }
    }

    if(http_request->request_type == 1 || http_request->request_type == 2)
    {        
        if(http_request->request_uri != NULL) {
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

