#include <stdlib.h>

#include "server.h"
#include "csapp.h"
#include "debug.h"
#include "pbx.h"
#include "custom.h"

int get_cmd_enum(char *buf);
int get_extension(char *buf, size_t num_bytes);
char *get_chatmsg(char *buf, size_t num_bytes);


void *pbx_client_service(void *arg)
{

    Sem_init(&mutex, 0, 1);
    int connfd = *(int *)arg;
    free(arg);
    Pthread_detach(pthread_self());
    debug("Thread %ld is on connfd %d", Pthread_self(), connfd);
    P(&mutex);
    TU *tu = pbx_register(pbx, connfd);
    V(&mutex);

    size_t num_bytes;
    char buf[MAXLINE];
    rio_t rio;


    Rio_readinitb(&rio, connfd);
    while((num_bytes = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
        int cmd = get_cmd_enum(buf), ext;
        char *msg;

        debug("Locking access");
        P(&mutex);
        switch(cmd)
        {
            case TU_PICKUP_CMD:
                tu_pickup(tu);
                break;
            case TU_HANGUP_CMD:
                tu_hangup(tu);
                break;
            case TU_DIAL_CMD:
                ext = get_extension(buf, num_bytes);
                tu_dial(tu, ext);
                break;
            case TU_CHAT_CMD:
                msg = get_chatmsg(buf, num_bytes);
                tu_chat(tu, msg);
                break;
            default:
                debug("Unknown command");
                break;
        }
        V(&mutex);
        debug("Unlocking access");
    }

    debug("Terminating service thread");
    P(&mutex);
    close(connfd);
    pbx_unregister(pbx, tu);
    V(&mutex);
    return NULL;
}

int get_cmd_enum(char *buf)
{
    for (int i = 0; i < 4; ++i)
    {
        if(!strncmp(buf, tu_command_names[i], 4))
        {
            return i;
        }
    }
    return -1;
}

int get_extension(char *buf, size_t num_bytes)
{
    int i = strlen(tu_command_names[TU_DIAL_CMD]);
    if(!strncmp(buf+i,EOL,sizeof(EOL)))
        return -1;
    char extension[num_bytes-i];
    int j;
    for (j = i; j < num_bytes; ++j)
    {
        extension[j-i] = *(buf+j);
    }
    return(atoi(extension));
}

char *get_chatmsg(char *buf, size_t num_bytes)
{
    int i = strlen(tu_command_names[TU_CHAT_CMD]);
    char *msg = Malloc(num_bytes-i);
    if(!strncmp(buf+i,EOL,sizeof(EOL)))
        return NULL;
    strncpy(msg, buf+i+1, num_bytes-i-sizeof(EOL));
    return msg;
}