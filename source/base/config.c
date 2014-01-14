#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
 //JSON parser library
#include "../libraries/jsmn/jsmn.h"
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
    if(config_modules == NULL)
        return -1;
    return 0;

}

route_node_t * addRouteNode(){
    route_node_t * bptr; /* Backup node */
    route_node_t * nptr; /* Last node */
    bptr = config_routes;
    if(config_routes != NULL){
        while(config_routes != NULL)
            config_routes = config_routes->next;    
    }

    config_routes = malloc(sizeof(route_node_t));
    config_routes->path = NULL;
    config_routes->option = NULL;
    nptr = config_routes;
    if(bptr != NULL)
        config_routes = bptr;
    return nptr;
    
}

void cleanConfig(){
    config_modules = NULL;
    config_servers = NULL;
    nservers = 0;   /* number of server configs */
    nmodules = 0;   /* number of module configs */

    if(config_routes != NULL){
        while(config_routes != NULL){
            if(config_routes->path != NULL)
                free(config_routes->path);
            if(config_routes->option != NULL)
                free(config_routes->option);
            route_node_t * old = config_routes;
            config_routes = config_routes->next;
            free(old);
        }
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

    int r; /* Return value */
    jsmn_parser parser; /* Parser */
    jsmntok_t tokens[256]; /* Objects parsed from the json file */

    //initialise the parser
    jsmn_init(&parser);

    //Parse the json string, returns jsmnerr_t
    r = jsmn_parse(&parser, json, tokens, 256);
    if(r != JSMN_SUCCESS){
        printf("%s\n", json);
        return -1;
    }

    int cparent = -1;   /* current parent node */

    int i = 1;          /* array iterator */
    int config = CONFIG_NONE; /* Current config array */
    jsmntok_t ctoken;   /* current token holder*/
    route_node_t * route = NULL; /* Current node */
    //The first token MUST be a object
    if(tokens[0].type != JSMN_OBJECT)
        return -1;

    //Servers config parser
    while((ctoken = tokens[i]).end != 0){
        if(ctoken.type == JSMN_OBJECT){
            if(cparent == -1 || ctoken.start > tokens[cparent].end){
                //Seek for a new parent node
                cparent = -1;
                //If the array is unamed continue
                if(tokens[i - 1].type != JSMN_STRING){
                    i++;
                    continue;
                }
                cparent = i;
                //Check for a main config array, if found we set
                // the current mode to that array
                if(strncasecmp(&json[tokens[i - 1].start], "servers", 6) == 0){
                    config = CONFIG_SERVER;
                } else if(strncasecmp(&json[tokens[i - 1].start], "modules", 7) == 0){
                    config = CONFIG_MODULE;
                    i++;
                } else if(strncasecmp(&json[tokens[i - 1].start], "routes", 6) == 0){
                    config = CONFIG_ROUTE;
                    i++;
                } else {
                    config = CONFIG_NONE;
                    cparent = -1;
                    i++;
                }
                continue;
            } else {
                //Check for a sub-array, this can be for example a server or a module
                if(config == CONFIG_SERVER){
                    //Set up the server array
                    config_servers = realloc(config_servers,
                        (nservers + 1 * sizeof(config_server_t *)));
                    config_servers[nservers] = malloc(sizeof(config_server_t)); 
                    config_servers[nservers]->port = 0;
                    config_servers[nservers]->name = NULL;
                    nservers++;
                } else if(config == CONFIG_MODULE){
                    //set up the modules array
                    config_modules = realloc(config_modules,
                        (nmodules + 1 * sizeof(config_module_t *)));
                    config_modules[nmodules] = malloc(sizeof(config_module_t));
                        config_modules[nmodules]->name = strndup(&json[tokens[i - 1].start],
                    tokens[i - 1].end - tokens[i - 1].start);
                    config_modules[nmodules]->command = NULL;
                    config_modules[nmodules]->method = NULL;
                    nmodules++;
                } else if(config == CONFIG_ROUTE) {
                    route = addRouteNode();
                }
            }
        } else if(cparent == -1){
            i++;
            continue;
        }

        //If it's not a pair of strings "string":"string" we do not want it
        //unless we want even more array depth
        if(config == CONFIG_NONE || (ctoken.type != JSMN_STRING && tokens[i+1].type != JSMN_STRING)){
            i++;
            continue;
        }

        //Here we process the values in a sub-array

        //Process key(ctoken):val(vtoken) pair
        jsmntok_t vtoken = tokens[i+1];

        //Abstract the value
        unsigned int length = vtoken.end - vtoken.start;
        char value[length + 10];
        memcpy(value, &json[vtoken.start], length);
        value[length] = '\0';

        if(config == CONFIG_SERVER){
            //Current server to be configured
            config_server_t *cserver = config_servers[nservers - 1];    
            if(strncasecmp(&json[ctoken.start], "port", 4) == 0){    
                if((cserver->port = atoi(value)) > 65535 || cserver->port < 1){
                    perror("Invalid port number");
                    return -1;
                }
            } else if(strncasecmp(&json[ctoken.start], "name", 4) == 0){
                cserver->name = strdup(value);                
            }
        } else if(config == CONFIG_MODULE) {
            //Current module to be configured
            if(strncasecmp(&json[ctoken.start], "command", 7) == 0){    
                config_modules[nmodules - 1]->command = strdup(value);

            } else if(strncasecmp(&json[ctoken.start], "method", 6) == 0){
                config_modules[nmodules - 1]->method = strdup(value);               
            }
        } else if(config == CONFIG_ROUTE) {
            if(strncasecmp(&json[ctoken.start], "path", 4) == 0){
                if(route != NULL)
                    route->path = strdup(value);
            } else if(strncasecmp(&json[ctoken.start], "option", 6) == 0){
                if(route != NULL)
                    route->option = strdup(value);
            }
        }
        i++;
    }

    return 0;
}