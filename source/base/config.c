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

config_server_t ** config_servers;
config_module_t ** config_modules;
route_node_t * config_routes;

int nservers;
int nmodules;

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
    
    enum config current_config = CONFIG_NONE;

    cJSON *mainitem = root->child;
    while(mainitem){
        if(mainitem->string == NULL){
            mainitem = mainitem->next;
            continue;
        } else if(strncasecmp(mainitem->string, "servers", 7) == 0){
            current_config = CONFIG_SERVER;
        } else if(strncasecmp(mainitem->string, "routes", 6) == 0){
            current_config = CONFIG_ROUTE;
        } else if(strncasecmp(mainitem->string, "modules", 7) == 0){
            current_config = CONFIG_MODULE;
        } else {
            mainitem = mainitem->next;
            continue;
        }

        cJSON *subitem = mainitem->child; 
        while (subitem)
        {
            if(current_config == CONFIG_SERVER){
                //cJSON *format = cJSON_GetObjectItem(root,"format");
                config_servers = realloc(config_servers,
                    (nservers + 1 * sizeof(config_server_t *)));
                config_servers[nservers] = object_ninit(sizeof(config_server_t)); 

                config_servers[nservers]->port = cJSON_GetObjectItem(subitem,"port")->valueint;                
                config_servers[nservers]->name = strdup(cJSON_GetObjectItem(subitem,"name")->valuestring);                
                nservers++;
            } else if(current_config == CONFIG_ROUTE) {

            } else if(current_config == CONFIG_MODULE) {
                //set up the modules array
                /*config_modules = realloc(config_modules,
                    (nmodules + 1 * sizeof(config_module_t *)));
                config_modules[nmodules] = object_ninit(sizeof(config_module_t));
                //config_modules[nmodules]->name = strndup(&json[tokens[i - 1].start],
                //tokens[i - 1].end - tokens[i - 1].start);
                config_modules[nmodules]->command = NULL;
                config_modules[nmodules]->method = NULL;
                nmodules++;*/
            }
          
            subitem = subitem->next;
        }
        mainitem = mainitem->next;
    }
    cJSON_Delete(root);
    free(json);
    return 0;
}
