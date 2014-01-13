#ifndef CONFIG_HEADER
#define CONFIG_HEADER

int loadConfig();

enum config {
	CONFIG_NONE,
	CONFIG_SERVER,
	CONFIG_MODULE
};

#endif