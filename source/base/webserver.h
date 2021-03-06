#ifndef SERVER_HEADER
#define SERVER_HEADER
#define DEBUG true

#define DIR_CONFIG "config/"

// Config struct defines
typedef struct {
    int sockfd;
    int num_server;
	int port;
	char * name;
	char * hostname;
}config_server_t;

typedef struct {
	char * name;
	char * command;
	char * method;
}config_module_t;

typedef struct  route_node{
	char * path;
	int module;
	char * option;
	struct route_node * next;
}route_node_t;


int server();
int loadConfig();
void *serverLoop(void *config_void);
void *handleClient(void *client_void);

#endif
