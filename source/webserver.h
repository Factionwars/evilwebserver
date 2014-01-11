#ifndef SERVER_HEADER
#define SERVER_HEADER

#define DEBUG true
int server();
void *handleClient(void *client_void);

#endif