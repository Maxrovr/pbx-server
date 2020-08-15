#include "pbx.h"
#include "csapp.h"
#include "debug.h"

struct tu
{
    int connfd;
    int curr_state;
};

struct pbx
{
    TU *tu[PBX_MAX_EXTENSIONS];
    int dialled[PBX_MAX_EXTENSIONS];
};

int notify_async(TU *tu);
int notify_chat_async(TU *tu, char * msg);


PBX *pbx_init()
{
    PBX *p = malloc(sizeof(PBX));
    for (int i = 0; i < PBX_MAX_EXTENSIONS; ++i)
    {
        p->tu[i] = NULL;
        p->dialled[i] = -1;
    }
    return p;
}

void pbx_shutdown(PBX *pbx)
{
    for (int i = 0; i < PBX_MAX_EXTENSIONS; ++i)
    {
        if(pbx->tu[i] != NULL)
        {
            shutdown(pbx->tu[i]->connfd, SHUT_RDWR);
            // free(pbx->tu[i]);
        }
    }
    free(pbx);
}


TU *pbx_register(PBX *pbx, int fd)
{
    debug("Registering %d and updating state to TU_ON_HOOK", fd);
    TU *tu = malloc(sizeof(TU));
    if(tu!=NULL)
    {
        tu->connfd = fd;
        tu->curr_state = TU_ON_HOOK;
        pbx->tu[fd] = tu;
        notify_async(tu);
        return tu;
    }
    return NULL;
}

int pbx_unregister(PBX *pbx, TU *tu)
{
    debug("Unregistering %d and updating state to TU_ON_HOOK", tu->connfd);
    if(tu!=NULL)
    {
        if(tu->curr_state==TU_CONNECTED)
        {
            TU *caller = tu;
            TU *callee = pbx->tu[pbx->dialled[caller->connfd]];
            callee->curr_state = TU_DIAL_TONE;
            if(notify_async(callee)<0) return -1;
        }
        pbx->tu[tu->connfd] = NULL;
        pbx->dialled[tu->connfd] = -1;
        free(tu);
    }
    return 0;
}

int tu_fileno(TU *tu)
{
    if(tu==NULL) return -1;
    return tu->connfd;
}

int tu_extension(TU *tu)
{
    return tu_fileno(tu);
}

int tu_pickup(TU *tu)
{
    TU *caller;
    TU *callee;
    switch(tu->curr_state)
    {
        case TU_ON_HOOK:
            tu->curr_state = TU_DIAL_TONE;
            if(notify_async(tu)<0) return -1;
            break;
        case TU_RINGING:
            caller = pbx->tu[pbx->dialled[tu->connfd]];
            callee = tu;
            caller->curr_state = TU_CONNECTED;
            if(notify_async(caller)<0) return -1;
            callee->curr_state = TU_CONNECTED;
            if(notify_async(callee)<0) return -1;
            break;
        default:
            if(notify_async(tu)<0) return -1;
            break;
    }
    return 0;
}

int tu_hangup(TU *tu)
{
    TU *caller = tu;
    int rxfd = pbx->dialled[caller->connfd];

    if(tu->curr_state == TU_CONNECTED)
    {
        if(rxfd==-1)
        {
            if(notify_async(caller)<0) return -1;
            return 0;
        }
        TU *callee = pbx->tu[rxfd];
        caller->curr_state = TU_ON_HOOK;
        if(notify_async(caller)<0) return -1;
        callee->curr_state = TU_DIAL_TONE;
        if(notify_async(callee)<0) return -1;

    }
    else if(tu->curr_state == TU_RING_BACK)
    {
        TU *callee = pbx->tu[pbx->dialled[caller->connfd]];
        caller->curr_state = TU_ON_HOOK;
        if(notify_async(caller)<0) return -1;
        callee->curr_state = TU_ON_HOOK;
        if(notify_async(callee)<0) return -1;
    }
    else if(tu->curr_state == TU_RINGING)
    {
        TU *callee = pbx->tu[pbx->dialled[caller->connfd]];
        caller->curr_state = TU_ON_HOOK;
        if(notify_async(caller)<0) return -1;
        callee->curr_state = TU_DIAL_TONE;
        if(notify_async(callee)<0) return -1;
    }
    else if(tu->curr_state == TU_DIAL_TONE || tu->curr_state == TU_BUSY_SIGNAL || tu->curr_state == TU_ERROR)
    {
        caller->curr_state = TU_ON_HOOK;
        if(notify_async(caller)<0) return -1;
    }
    else
    {
        if(notify_async(caller)<0) return -1;
    }
    return 0;
}

int tu_dial(TU *tu, int ext)
{
    // debug("%d(%s) dialled %d",tu->connfd, tu_state_names[tu->curr_state], ext, tu_state_names[pbx->tu[ext]->curr_state]);
    TU *caller = tu;
    TU *callee = pbx->tu[ext];

    // Invalid callee
    if(callee == NULL)
    {
        tu->curr_state = TU_ERROR;
        if(notify_async(caller)<0) return -1;
        return 0;
    }
    // Valid Callee
        // Caller has picked up reciever before dialling extension and callee is ready to accept calls
    if(caller->curr_state == TU_DIAL_TONE && callee->curr_state == TU_ON_HOOK)
    {
        caller->curr_state = TU_RING_BACK;
        if(notify_async(caller)<0) return -1;
        callee->curr_state = TU_RINGING;
        if(notify_async(callee)<0) return -1;
        pbx->dialled[callee->connfd] = caller->connfd;
        pbx->dialled[caller->connfd] = callee->connfd;
    }
        // Caller has picked up reciever before dialling extension but callee is busy
    else if(caller->curr_state == TU_DIAL_TONE)
    {
        caller->curr_state = TU_BUSY_SIGNAL;
        if(notify_async(caller)<0) return -1;
    }
    else
    {
        if(notify_async(caller)<0) return -1;
    }
    return 0;
}

int tu_chat(TU *tu, char *msg)
{
    TU *sender = tu;
    int rxfd = pbx->dialled[sender->connfd];
    if(rxfd==-1)
    {
        if(notify_async(sender)<0) return -1;
        return 0;
    }
    TU*receiver = pbx->tu[rxfd];
    if(pbx->dialled[sender->connfd]==receiver->connfd && pbx->dialled[receiver->connfd]==sender->connfd && receiver->curr_state == TU_CONNECTED && sender->curr_state == TU_CONNECTED)
    {
        notify_chat_async(receiver, msg);
        notify_async(sender);
        return 0;
    }
    else
    {
        if(notify_async(sender)<0) return -1;
    }
    return -1;
}

int notify_async(TU *tu)
{
    // int len;
    // char *str = NULL;
    FILE *out  = fdopen(tu->connfd, "w");
    if(tu->curr_state == TU_ON_HOOK)
    {
        // len = snprintf(NULL, 0, "%s %d\n", tu_state_names[tu->curr_state], tu->connfd);
        // if(len < 0) return -1;
        // str = malloc(len+2);
        // if(str == NULL) return -1;
        // len = snprintf(str, len+2, "%s %d\n", tu_state_names[tu->curr_state], tu->connfd);
        // if(len<0) return -1;
        fprintf(out, "%s %d\n", tu_state_names[tu->curr_state], tu->connfd);
    }
    else if(tu->curr_state == TU_DIAL_TONE || tu->curr_state == TU_RING_BACK || tu->curr_state == TU_RINGING || tu->curr_state == TU_BUSY_SIGNAL || tu->curr_state == TU_ERROR)
    {
        // len = snprintf(NULL, 0, "%s\n", tu_state_names[tu->curr_state]);
        // if(len < 0) return -1;
        // str = malloc(len+2);
        // if(str == NULL) return -1;
        // snprintf(str, len+2, "%s\n", tu_state_names[tu->curr_state]);
        // if(len<0) return -1;
        fprintf(out, "%s\n", tu_state_names[tu->curr_state]);
    }
    else if(tu->curr_state == TU_CONNECTED)
    {
        // len = snprintf(NULL, 0, "%s %d\n", tu_state_names[tu->curr_state], pbx->dialled[tu->connfd]);
        // if(len < 0) return -1;
        // str = malloc(len+2);
        // if(str == NULL) return -1;
        // len = snprintf(str, len+2, "%s %d\n", tu_state_names[tu->curr_state], pbx->dialled[tu->connfd]);
        // if(len<0) return -1;
        fprintf(out, "%s %d\n", tu_state_names[tu->curr_state], pbx->dialled[tu->connfd]);
    }

    // fprintf(out, "%s", str);
    // Rio_writen(tu->connfd, str, len+4);
    fflush(out);
    // if(str!=NULL) free(str);

    return 0;
}

int notify_chat_async(TU *tu, char * msg)
{
    // int len;
    // char *str;
    // len = snprintf(NULL, 0, "%s %s\r\n", "CHAT", msg);
    // if(len < 0) return -1;
    // str = malloc(len+1);
    // if(str == NULL) return -1;
    // len = snprintf(str, len+1, "%s %s\r\n", "CHAT", msg);
    // if(len<0) return -1;

    // if(rio_writen(tu->connfd, str, len+1)!=len+1) return -1;
    // if(str!=NULL) free(str);

    FILE *out  = fdopen(tu->connfd, "w");
    fprintf(out, "%s %s\n", "CHAT", msg);
    fflush(out);

    return 0;
}