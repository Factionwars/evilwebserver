#ifndef SERVER_HEADER
#define SERVER_HEADER

/* Basic webserver configuration */

#define DIR_CONFIG "config/"

/* File defines*/
typedef struct {
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

config_server_t ** config_servers;
config_module_t ** config_modules;
route_node_t * config_routes;

int nservers; 
int nmodules;

int server();
int loadConfig();
void *handleClient(void *client_void);

#endif