#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include "../src/zmalloc.h"
#include "../src/zmalloc.c"
#include "../src/ae.h"
#include "../src/ae.c"

#ifndef HAVE_EPOLL
    #define HAVE_EPOLL
#endif


long long current_timestamp() {
    struct timeval te; 
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000 + te.tv_usec/1000;
    return milliseconds;
}

int func1()
{
    printf("func1 run, us is %lld\n", current_timestamp());
    return 1000;
}

int func2()
{
    printf("func2 run, us is %lld\n", current_timestamp());
    return AE_NOMORE;
}

int
main()
{
    printf("main run, us is %lld\n", current_timestamp());

    aeEventLoop * el = aeCreateEventLoop(1024);
    long long timerId1 = aeCreateTimeEvent(el, 1000, func1, NULL, NULL); 
    long long timerId2 = aeCreateTimeEvent(el, 0, func2, NULL, NULL);

    aeMain(el);

    exit(0);
}