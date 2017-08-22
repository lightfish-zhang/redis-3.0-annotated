# 事件与网络通信

## 概念

- Redis服务是一个事件驱动程序，主要处理的事件类型:
    + 文件事件`file event`,也就是网络通信，通过`socket`套接字，多服务端之间，服务端与客户端之间。
    + 时间事件`time event`，redis服务的一些操作(eg, serverCron函数)，需要在给定时间点执行


## 文件事件

### 事件的API

Redis封装的事件的各种函数，由于不同操作系统提供的文件事件托管通知的接口不一样，所以Redis的预编译命令如下

```c
#ifdef HAVE_EVPORT
#include "ae_evport.c"
#else
    #ifdef HAVE_EPOLL
    #include "ae_epoll.c"
    #else
        #ifdef HAVE_KQUEUE
        #include "ae_kqueue.c"
        #else
        #include "ae_select.c"
        #endif
    #endif
#endif
```

#### 事件循环的数据结构

- Redis以Reactor模式开发了网络事件处理器

```c
typedef struct aeEventLoop {

    // 目前已注册的最大描述符
    int maxfd;   /* highest file descriptor currently registered */

    // 目前已追踪的最大描述符
    int setsize; /* max number of file descriptors tracked */

    // 用于生成时间事件 id
    long long timeEventNextId;

    // 最后一次执行时间事件的时间
    time_t lastTime;     /* Used to detect system clock skew */

    // 已注册的文件事件
    aeFileEvent *events; /* Registered events */

    // 已就绪的文件事件
    aeFiredEvent *fired; /* Fired events */

    // 时间事件
    aeTimeEvent *timeEventHead;

    // 事件处理器的开关
    int stop;

    // 多路复用库的私有数据
    void *apidata; /* This is used for polling API specific data */

    // 在处理事件前要执行的函数
    aeBeforeSleepProc *beforesleep;

} aeEventLoop;

```

#### 以Linux的epoll为例子

- 相关代码在`src/ae_epoll.c`
- 创建`epfd`描述符

```c
state->epfd = epoll_create(1024);
```

- 把需要监听的`fd`关联到`epfd`，由于Redis只需要做网络事件监听，所以只需要监听`EPOLLIN`可读事件与`EPOLLOUT`可写事件(如果是非阻塞，表示缓存空间可以继续接收数据)
- 以下代码，添加事件，epoll使用默认的水平触发的方式，因为`ee.events`没有`EPOLLET`

```c
/*
 * 文件事件状态
 */
// 未设置
#define AE_NONE 0
// 可读
#define AE_READABLE 1
// 可写
#define AE_WRITABLE 2


//...

static int aeApiAddEvent(aeEventLoop *eventLoop, int fd, int mask) {
    //...
    int op = eventLoop->events[fd].mask == AE_NONE ?
        EPOLL_CTL_ADD : EPOLL_CTL_MOD;

    // 注册事件到 epoll
    ee.events = 0;
    mask |= eventLoop->events[fd].mask; /* Merge old events */
    if (mask & AE_READABLE) ee.events |= EPOLLIN;
    if (mask & AE_WRITABLE) ee.events |= EPOLLOUT;
    // ...

    if (epoll_ctl(state->epfd,op,fd,&ee) == -1) return -1;
    // ...
}
```

- 其他的，删除给定事件，释放 epoll 实例和事件槽，获取可执行事件的函数就不展开说了


#### 网络通信的函数

- 相关代码在`src/networking.c`,`src/anet.c`文件
- `anet.c`封装了对`socket`的相关处理函数，如下，都是socket的tcp的服务端套路

```
/*
 * 创建并返回 socket
 */
static int anetCreateSocket(char *err, int domain) {
    int s;
    if ((s = socket(domain, SOCK_STREAM, 0)) == -1) {
        anetSetError(err, "creating socket: %s", strerror(errno));
        return ANET_ERR;
    }

    /* Make sure connection-intensive things like the redis benchmark
     * will be able to close/open sockets a zillion of times */
    if (anetSetReuseAddr(err,s) == ANET_ERR) {
        close(s);
        return ANET_ERR;
    }
    return s;
}

/*
 * 将 fd 设置为非阻塞模式（O_NONBLOCK）
 */
int anetNonBlock(char *err, int fd)
{
    int flags;

    /* Set the socket non-blocking.
     * Note that fcntl(2) for F_GETFL and F_SETFL can't be
     * interrupted by a signal. */
    if ((flags = fcntl(fd, F_GETFL)) == -1) {
        anetSetError(err, "fcntl(F_GETFL): %s", strerror(errno));
        return ANET_ERR;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        anetSetError(err, "fcntl(F_SETFL,O_NONBLOCK): %s", strerror(errno));
        return ANET_ERR;
    }
    return ANET_OK;
}

/*
 * accept
 */
static int anetGenericAccept(char *err, int s, struct sockaddr *sa, socklen_t *len) {
    int fd;
    while(1) {
        fd = accept(s,sa,len);
        if (fd == -1) {
            if (errno == EINTR)
                continue;
            else {
                anetSetError(err, "accept: %s", strerror(errno));
                return ANET_ERR;
            }
        }
        break;
    }
    return fd;
}

```

- `networking.c`封装了与客户端通信的数据的处理函数
- 网络通信有一点需要注意的是大小端数据的问题，大小端也就是字节顺序类型
    + Redis的规范是，所有键值对的数据都以小端表示，网络通信的字节也是约定为小端格式
    + 通过预编译命令，判断主机的环境时大端还是小端
    + 当主机的数据格式为大端时，才会使用到大小端转换的函数`src/endianconv.c`
    + 由上面３点可知，当主机为大端格式，且在写入字典类型的数据才会用到大小端转化函数，eg, intset, ziplist

```c
// src/config.c 
/* Byte ordering detection */
#include <sys/types.h> /* This will likely define BYTE_ORDER */

#ifndef BYTE_ORDER
#if defined(linux) || defined(__linux__)
# include <endian.h>
#else

// .....
```

```c
// src/endianconv.h
/* variants of the function doing the actual convertion only if the target
 * host is big endian */
#if (BYTE_ORDER == LITTLE_ENDIAN)
#define memrev16ifbe(p)
#define memrev32ifbe(p)
#define memrev64ifbe(p)
#define intrev16ifbe(v) (v)
#define intrev32ifbe(v) (v)
#define intrev64ifbe(v) (v)
#else
#define memrev16ifbe(p) memrev16(p)
#define memrev32ifbe(p) memrev32(p)
#define memrev64ifbe(p) memrev64(p)
#define intrev16ifbe(v) intrev16(v)
#define intrev32ifbe(v) intrev32(v)
#define intrev64ifbe(v) intrev64(v)
#endif
```


## 时间事件

