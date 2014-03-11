#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include <netdb.h>
 
#include "evilnetlib.h"

int sendCGI(int sockfd, http_request_t* http_request, char * command, char * script) 
{
    //Set environment variables
    char ** envp = NULL;

    int envp_length = 0;
 
    envp = addEnv(envp, "REDIRECT_STATUS", "200", &envp_length);

    if(http_request->request_type == 1){
        envp = addEnv(envp, "REQUEST_METHOD", "GET", &envp_length);
    } else if(http_request->request_type == 2){
       envp = addEnv(envp, "REQUEST_METHOD", "POST", &envp_length);
        if(http_request->content_body != NULL)
            envp = addEnv(envp, "BODY", http_request->content_body, &envp_length); 
        //Render content Length       
        char clength[4];
        snprintf(clength, 4 ,"%d", http_request->content_length);
        envp = addEnv(envp, "CONTENT_LENGTH", clength, &envp_length);
    }
    if(http_request->request_uri != NULL)
        envp = addEnv(envp, "PATH_INFO", http_request->request_uri, &envp_length);
    if(http_request->request_query != NULL)
        envp = addEnv(envp, "QUERY_STRING", http_request->request_query, &envp_length);

    envp = addEnv(envp, "REMOTE_ADDR", inet_ntoa(http_request->client->addr->sin_addr), &envp_length);    
    envp = addEnv(envp, "SCRIPT_FILENAME", script, &envp_length);
    envp = addEnv(envp, "HTTP_ACCEPT", "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\0", &envp_length);
    envp = addEnv(envp, "CONTENT_TYPE", "application/x-www-form-urlencoded\0", &envp_length);
    envp = addEnv(envp, "GATEWAY_INTERFACE","CGI/1.1\0", &envp_length);
    envp = addEnv(envp, "SERVER_NAME", SERVER_NAME, &envp_length);
    envp = addEnv(envp, "SERVER_PROTOCOL", "HTTP/1.1\0", &envp_length);
    envp = addEnv(envp, "SERVER_PORT", SERVER_PORT_CGI, &envp_length);
    envp = addEnv(envp, "SERVER_SOFTWARE", SERVER_SOFTWARE, &envp_length);

    char *argv[] = { command , script , 0 }; /* Arg value array */
    //Close environment list
    envp = realloc(envp, (++envp_length) * sizeof(envp[0]));
    envp[envp_length - 1] = 0;

    int pipes[4];

    if(pipe(&pipes[0]) < 0)/* Parent read/child write pipe */
        return -1;
    if(pipe(&pipes[2]) < 0) /* Child read/parent write pipe */
        return -1;
    pid_t pid1;
    pid_t pid2;
    int status;

    if ((pid1 = fork())) {
         //we won't use the childs end
        close(pipes[1]);
        close(pipes[2]);

        char buffer[1024];
        int ret = 0;

        //If request is post write the body to the child
        if(http_request->request_type == 2){
            int to_write = http_request->content_length;
            int written = 0;
            char * bptr; /* body pointer*/
            bptr = http_request->content_body;
            while(to_write > 0){
                written = write(pipes[3], bptr, to_write);
                to_write -= written;
                bptr += written;
            }
        }

        //Read the CGI output from the pipe and send it to the client
        int received = 0;
        while ((received = read(pipes[0], buffer, 1023))) {
            buffer[received] = '\0';
            if(sendString(sockfd, (char *)buffer) < 0){
                ret = -1;
                break;
            }
        }

        close(pipes[0]);
        close(pipes[3]);
        
        //Cleanup enviroment
        while(envp_length--){
            free(envp[envp_length]);
        }
                             
        free(envp);
       
        waitpid(pid1, &status, 0);
        return ret;
    } else if(!pid1) {
        if((pid2 = fork())) {
            exit(0);
        } else if(!pid2) {
            close(pipes[0]);
            close(pipes[3]);
            //Duplicate our pipes against STDIN/STDOUT
            dup2(pipes[1], fileno(stdout));
            dup2(pipes[2], fileno(stdin));
            //If enabled also pipe against STDERR
            if(CGI_ERRORS)
                dup2(pipes[1], fileno(stderr));

            execve(command, &argv[0], envp);     
            printf("Failed to launch CGI application\n");
            exit(0);
        }
    }

    return -1;

}