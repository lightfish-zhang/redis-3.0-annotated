#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netdb.h>
#include "../src/zmalloc.h"
#include "../src/zmalloc.c"
#include "../src/ae.h"
#include "../src/ae.c"
#include "../src/anet.h"
#include "../src/anet.c"

#ifndef HAVE_EPOLL
    #define HAVE_EPOLL
#endif

char * err;


long long current_timestamp() {
    struct timeval te; 
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000 + te.tv_usec/1000;
    return milliseconds;
}

int func1()
{
    printf("func1 run, us is %lld\n", current_timestamp());
    return 5000;
}

int func2()
{
    printf("func2 run, us is %lld\n", current_timestamp());
    return AE_NOMORE;
}

void err_print(char * tag)
{
    sprintf(stderr, "tag is %s error is %s\n", tag, err);
    err = "";
}

void acceptFunc(aeEventLoop * loop, int s, void * clientData, int mask){
    int fd;
    struct sockaddr in_addr;
    socklen_t in_len = sizeof in_addr;
    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
    while(1) {
        fd = accept(s, &in_addr, &in_len);
        if (fd == -1) {
            if (errno == EINTR)
                continue;
            else {
                err = strerror(errno);
                err_print( "accept");
                return;
            }
        }
        break;
    }
    if (getnameinfo(&in_addr, in_len, hbuf, sizeof hbuf, sbuf, sizeof sbuf, NI_NUMERICHOST | NI_NUMERICSERV) == 0) {
        printf("Accepted connection on descriptor %d (host=%s, port=%s)\n", fd, hbuf, sbuf);
    }
}

int
main()
{
    printf("main run, us is %lld\n", current_timestamp());

    aeEventLoop * loop = aeCreateEventLoop(1024);
    long long timerId1 = aeCreateTimeEvent(loop, 1000, func1, NULL, NULL); 
    long long timerId2 = aeCreateTimeEvent(loop, 0, func2, NULL, NULL);

    int socketFd = anetTcpServer(err, 1234, NULL, SOMAXCONN);
    if(socketFd < 0){
        err_print("anetTcpServer");
    }
    int ret = aeCreateFileEvent(loop, socketFd, AE_READABLE, acceptFunc, NULL);
    if(ret < 0){
        err_print("aeCreateFileEvent");
    }

    aeMain(loop);
    exit(0);
}