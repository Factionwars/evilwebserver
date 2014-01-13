#ifndef SERVER_HEADER
#define SERVER_HEADER

/* Basic webserver configuration */
#define DEBUG true
#define DIR_CONFIG "config/"

/* File defines*/
typedef struct {
	int port;
	char * name;
	char * hostname;
}config_server_t;

typedef struct {
	char * command;
	char * method;
}config_module_t;

config_server_t ** config_servers;
config_server_t ** config_modules;

int server();
int loadConfig();
void *handleClient(void *client_void);

#endif