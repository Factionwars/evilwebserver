#ifndef CONFIG_HEADER
#define CONFIG_HEADER

enum config {
	CONFIG_NONE,
	CONFIG_SERVER,
	CONFIG_MODULE,
	CONFIG_ROUTE
};

int loadConfig();
void cleanConfig();
int parseConfig(char * filename);


#endif