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
#include <signal.h>

#include "webserver.h"


http_client::http_client()
{
    addr = new struct sockaddr_in;
    //TODO init vars
}

Server::Server()
{


}

int Server::start()
{
	int sock_server;
    //listen on the server port
    if((sock_server = listenOn(1337)) < 0){
        perror("Closing: error binding to port");
        return -1;
    }

    struct http_client * client = new struct http_client;

    //Accept clients
    while((client->sockfd 
        = acceptClient(sock_server, client->addr))) {
        //Create a new thread to assign to the new client
        pthread_t client_thread;
        pthread_create( &client_thread,
                NULL,
                handleClient,
                (void *)client);
        //Detach the client, from this point the thread will live it's own life
        pthread_detach( client_thread );
        //Create a new client container  for the next newcomer
        client = new struct http_client;
    }

    close(sock_server);
    return 0;
}

int Server::listenOn(int port)
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

int Server::acceptClient(int sockfd, struct sockaddr_in* client_addr)
{
    socklen_t sin_size = sizeof(struct sockaddr_in);
    int new_sockfd = accept(sockfd, (struct sockaddr*)client_addr, &sin_size);
    return new_sockfd;
}

void handleClient(void * client)
{
	return void;
}