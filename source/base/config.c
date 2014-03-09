#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
 //JSON parser library
#include "../libraries/cJSON/cJSON.h"
#include "../libraries/objectivity/object.h"
#include "webserver.h"
#include "evilnetlib.h"
#include "config.h"

int loadConfig(){
    int ret = 0;
    
    //Clean all the old values out
    cleanConfig();

    ret += parseConfig(DIR_CONFIG"config.json");
    
    parseConfig(DIR_CONFIG"routes.json");

    if(config_servers == NULL)
        return -1;

    return ret;

}

route_node_t * addRouteNode(){
    route_node_t * bptr; /* Backup node */
    route_node_t * nptr; /* Last node */
    bptr = config_routes;
    if(config_routes != NULL){
        while(config_routes != NULL){
            config_routes = config_routes->next;
        }  
    }

    config_routes = object_init(sizeof(route_node_t));

    nptr = config_routes;
    if(bptr != NULL)
        config_routes = bptr;
    return nptr;
    
}

void cleanConfig(){

    if(nservers > 0 && config_servers != NULL){
        int i;       
        for(i = 0; i > nservers; i++){
            if(config_servers[i] != NULL)
                object_delete(config_servers[i]);
        }
        object_delete(config_servers);
    }
    if(nmodules > 0 && config_modules != NULL){
        int i;
        for(i = 0; i > nmodules; i++){
            if(config_modules[i] != NULL)
                object_delete(config_modules[i]);
        }
        object_delete(config_modules);
    }

    config_modules = NULL;
    config_servers = NULL;
    nservers = 0;   /* number of server configs */
    nmodules = 0;   /* number of module configs */

    if(config_routes != NULL){
        while(config_routes != NULL){
            if(config_routes->path != NULL)
                object_delete(config_routes->path);
            if(config_routes->option != NULL)
                object_delete(config_routes->option);
            route_node_t * old = config_routes;
            config_routes = config_routes->next;
            object_delete(old);
        }
        config_routes = NULL;
    }
}

int parseConfig(char * filename){
    //Read config file
    char * json; /* Contents of json file */
    json = readFile(filename);
    if(json == NULL){
        perror("Error reading config file");
        return -1;
    }

    cJSON *root = cJSON_Parse(json);
    
    cJSON *format = cJSON_GetObjectItem(root,"format");
    int framerate = cJSON_GetObjectItem(format,"frame rate")->valueint;
    
    cJSON_Delete(root);
    free(json);
    return 0;
}
