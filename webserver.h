#ifndef SERVER_HEADER
#define SERVER_HEADER

#include <time.h>
#include <stdio.h>
#include <pthread.h>


int server();
void *handleClient(void *client_void);

#endif