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
#include <signal.h>

//Net library
#include "evilnetlib.h"
 //JSON parser library
#include "jsmn/jsmn.h"
#include "webserver.h"

long long requests = 0;
pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;

int main()
{
    return server();
}

int server()
{
    //load Json configurations
    if(loadConfig() < 0){
        perror("Closing: error reading config files.");
        return EXIT_FAILURE;
    }
    return 0;
    int sock_server;
    //listen on the server port
    if((sock_server = listenOn(SERVER_PORT)) < 0){
        perror("Closing: error binding to port");
        return EXIT_FAILURE;
    }
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

int loadConfig(){

    //json = "{\"server\":{\"port\" : \"1337\",\"name\" : \"EvilTinyHTTPD\" }}";

    int file, length;
    char * json;
    if((file = open(DIR_CONFIG"config.json", O_RDONLY, 0)) == -1){
        perror("Error opening config file");
        return -1;
    }

    length = get_file_size(file);

    if((json = malloc(length * sizeof(char) )) == NULL){
        perror("Error allocating memory");
        return -1;
    }
        
    if(read(file, json, length) == -1){
        perror("Error reading config file");
        return -1;
    }
    int r;
    jsmn_parser parser;
    jsmntok_t tokens[256];

    jsmn_init(&parser);  
    r = jsmn_parse(&parser, json, tokens, 256);
    if(r != JSMN_SUCCESS){
        printf("%s\n", json);
        return -1;
    }

    int cparent = -1;   /* current parent node */
    int nservers = 0;   /* number of server configs */
    int i = 1;          /* array iterator */
    jsmntok_t ctoken;   /* current token holder*/

    //The first token MUST be a object
    if(tokens[0].type != JSMN_OBJECT)
        return -1;

    //Servers config parser
    while((ctoken = tokens[i]).end != 0){
        if(ctoken.type == JSMN_OBJECT){
            if(cparent == -1 || ctoken.start > tokens[cparent].end){
                if(tokens[i - 1].type != JSMN_STRING){
                    i++;
                    continue;
                }
                cparent = i;

                config_servers = realloc(config_servers,
                 (nservers + 1 * sizeof(config_server_t *)));
                config_servers[nservers] = malloc(sizeof(config_server_t)); 
                config_servers[nservers]->port = 0;
                config_servers[nservers]->name = NULL;
                nservers++;
            }
        } else if(cparent == -1){
            i++;
            continue;
        }
        
        if(ctoken.type != JSMN_STRING && tokens[i+1].type != JSMN_STRING){
            i++;
            continue;
        }
        //Current server to be configured
        config_server_t *cserver = config_servers[nservers - 1];

        //Process key(ctoken):val(vtoken) pair
        jsmntok_t vtoken = tokens[i+1];

        unsigned int length = vtoken.end - vtoken.start;
        char value[length + 10];
        memcpy(value, &json[vtoken.start], length);
        value[length] = '\0';

        if(strncasecmp(&json[ctoken.start], "port", 4) == 0){       
            if((cserver->port = atoi(value)) > 65535 || cserver->port < 0){
                perror("Invalid port number");
                return -1;
            }
        } else if(strncasecmp(&json[ctoken.start], "name", 4) == 0){
            cserver->name = strdup(value);
        }
        i++;
    }

    //check(t[0].type == JSMN_OBJECT);
    //check(t[0].start == 0 && t[0].end == 2);
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
    //Make sure we do not get killed by a sigpipe
    //TODO: Make sure everything is cleaned up after a sigpipe
    signal(SIGPIPE, SIG_IGN);
    http_client_t *client = (http_client_t*)client_void;
    http_request_t *http_request;

    //Init Request struct
    if( ( http_request = malloc(sizeof(http_request_t))) == NULL)
        logError(3, client, http_request);
    //Initialize pointers to NULL
    // so we know if we can free them or not
    http_request->request_uri   = NULL;
    http_request->content_body  = NULL;
    http_request->request_query = NULL; 
    http_request->request_type  = 0;
    http_request->client = client;

#if DEBUG
    printf("Got a connection from %s on port %d\n", inet_ntoa(client->addr->sin_addr), ntohs(client->addr->sin_port));
#endif

    //Init linked list struct
    http_request->headers = malloc(sizeof(struct http_header));
    struct http_header * theader;
    //temp header for looping through the list
    theader = http_request->headers;
    theader->name = NULL;
    theader->value = NULL;
    theader->next = NULL;

    //Allowed headers to parse and pass-on
    char *alheader[] = {"Content-Length", "User-Agent", "Accept", "Connection", "Host"};
    //alheader array length
    int allength = sizeof(alheader) / sizeof(alheader[0]);
    //Create the linked list for storing the headers
    int i = 0;
    for(i = 0; i < allength; i++){
        theader->name = strdup(alheader[i]);
        theader->next = malloc(sizeof(struct http_header));
        theader = theader->next;
        theader->name = NULL;
        theader->value = NULL;
        theader->next = NULL;
    }
    
    int first = 0;
    http_request->content_length = 0;
    char buffer[8192];
    while(recvLine(client->sockfd, buffer, 8192))
    {
        if(first == 0) {
            int uri_start = 0;
            if(strncasecmp(buffer, "GET", 3) == 0) {
                http_request->request_type = 1;
                uri_start = 4;
            } else if(strncasecmp(buffer, "POST", 4) == 0) {
                http_request->request_type = 2;
                uri_start = 5;
            }

            char * query;
            query = strchr(buffer, '?');

            if(query == NULL){
                http_request->request_uri = strdup(buffer + uri_start);
                http_request->request_query = NULL;
            } else {
                int uri_length = query - buffer - uri_start;
                http_request->request_uri = strndup(buffer + uri_start, uri_length);
                char * query_end = strchr(query, ' ') - 1;
                if(query == NULL)
                    http_request->request_query = strdup(query + 1);
                else {
                    int query_length = query_end - query;
                    http_request->request_query = strndup(query + 1, query_length);
                }                        
            }
            first++;
        } else {
            char * seperator;
            seperator = strchr(buffer, ':');
            //Name Length
            int nLength = seperator - buffer;
            //Temporary header
            theader = http_request->headers;
            //Parse header into the headers linked list
            while(theader != NULL){
                if(nLength < 0 || nLength > 4064){
                    theader = theader->next;
                    continue;
                }
                char * end;
                end = strchr(buffer + nLength, '\xd');
                //value length
                int vLength = end - buffer - nLength + 1;
                //break;        
                if(theader->value == NULL && theader->name != NULL 
                    && strncasecmp(buffer, theader->name, nLength) == 0){
                    //TODO: parse whitespace
                    theader->value = strndup(buffer + nLength + 1, vLength);
                    //Check for the content-length header
                    if(strcasecmp(theader->name, "Content-Length") == 0){
                        http_request->content_length = atoi(theader->value);
                        if(http_request->content_length > 8192)
                            http_request->content_length = 8192;
                        if(http_request->content_length < 0)
                            http_request->content_length = 0;
                    }
                }                            
                theader = theader->next;
            }
        }
        printf("received: %s\n", buffer);

        if(buffer[0] == '\xd')
            break;
    }

    if(http_request->request_type == 2){
        if(http_request->content_length < 8192){
            int received = 0;            
            http_request->content_body = malloc(sizeof(char) * http_request->content_length);
            while(received < http_request->content_length){
                received += recv(client->sockfd,
                    (http_request->content_body + received),
                    (http_request->content_length - received),
                    0);

            }
        }
    }

    //Create creation date
    char buf[2000];
    time_t now = time(0);
    struct tm tm = *gmtime(&now);
    strftime(buf, sizeof buf, "%a, %d %b %Y %H:%M:%S %Z", &tm);

    //Parse first request string
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
            //sendPython(client->sockfd, http_request);
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

