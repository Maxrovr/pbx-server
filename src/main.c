#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include "pbx.h"
#include "server.h"
#include "debug.h"
#include "csapp.h"

#include "custom.h"

static void terminate(int status);
void signal_handler(int sig);

/*
 * "PBX" telephone exchange simulation.
 *
 * Usage: pbx -p <port>
 */



int main(int argc, char* argv[]){
    // Option processing should be performed here.
    // Option '-p <port>' is required in order to specify the port number
    // on which the server should listen.
    int opt;
    char *port;
    while ((opt = getopt(argc, argv, "p:")) != -1)
    {
        switch (opt)
        {
            case 'p':
                port = optarg;
                break;
            default:
                fprintf(stderr, "Usage: %s [-p port]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    struct sigaction *handler = malloc(sizeof(struct sigaction));
    handler->sa_handler = signal_handler;
    sigaction(SIGHUP, handler, NULL);

    // Perform required initialization of the PBX module.
    debug("Initializing PBX...");
    pbx = pbx_init();

    int listenfd;
    if((listenfd = open_listenfd(port))<0)
    {
        perror("Error opening listenfd");
        exit(EXIT_FAILURE);
    }

    pthread_t tid;
    socklen_t clientlen=sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;

    while(1)
    {
        int *connfd = Malloc(sizeof(int));
        *connfd = Accept(listenfd,  (SA *) &clientaddr, &clientlen);
        Pthread_create(&tid, NULL, pbx_client_service, connfd);
    }

    terminate(EXIT_FAILURE);
}

/*
 * Function called to cleanly shut down the server.
 */
void terminate(int status) {
    debug("Shutting down PBX...");
    pbx_shutdown(pbx);
    debug("PBX server terminating");
    exit(status);
}

void signal_handler(int sig)
{
    terminate(EXIT_SUCCESS);
}
