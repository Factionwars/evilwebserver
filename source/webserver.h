#ifndef SERVER_HEADER
#define SERVER_HEADER

#define DEBUG true

struct config_server {
	int port;
	char * name;
};

typedef struct config_module {
	char * command;
	char * method
}config_module;

int server();
void *handleClient(void *client_void);

#endif