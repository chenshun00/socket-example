### 系统API

* socket
    * `socket(AF_INET, SOCK_STREAM, 0)`
* setsockopt
    * `setsockopt(serverFd, SOL_SOCKET, SO_REUSEPORT, &open, sizeof open);`
    * `setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &open, sizeof open);`
    * `SO_KEEPALIVE`
* htons,ntohs
    * host to network short(大小端子节序)
    * network to host short(大小端子节序)
* bind
    * `bind(serverFd, (const sockaddr *) &addr, sizeof(addr))`
* listen
    * `listen(serverFd, 128)`
* accept
    * `accept(serverFd, nullptr, nullptr)`
* fork
* read
* write
* recv


#### thread

* pthread_create

```c
    int clientFd = accept(serverFd, nullptr, nullptr);
    pthread_t id;
    pthread_create(&id, nullptr, &run, &clientFd);
```
run thread
```c
    void *run(void *argv) {
        int *clientFd = (int *) argv;
        char buf[128];
        //read will block
        ssize_t num = 0;
        while ((num = read(*clientFd, buf, 1024)) > 0) {
            buf[num] = '\0';
            std::cout << "read num : " << num << ",sizeof(buf) = " << sizeof buf << ", value = " << buf << std::endl;
            if (strncmp(buf, "exit", 4) == 0) {
                close(*clientFd);
                stop = 0;
                break;
            }
            char *msg = "yes , yes\r\n";
            send(*clientFd, msg, strlen(msg), 0);
        }
    };
```

### 结构体

* sockaddr_in (套接字地址结构)

> 参考UNIX网络编程卷1：套接字联网API-第三章
> sockaddr_in = socket address; internet (默认是ipv4)
> ? 为什么传递指针的时候, 总是要传一个长度的参数, 书中明确有写的
> 这个数据, 需要从内核拷贝到进程或者是从进程拷贝到内核中，因此需要传递一个size，告诉内核拷贝多少数据

```c
   /*
    * Socket address, internet style.
    */
    struct sockaddr_in {
        __uint8_t       sin_len;
        sa_family_t     sin_family;
        in_port_t       sin_port;
        struct in_addr sin_addr;
        char sin_zero[8];
    };
    
    /*
     * [XSI] Structure used by kernel to store most addresses.
     */
    struct sockaddr {
        __uint8_t       sa_len;         /* total length */
        sa_family_t     sa_family;      /* [XSI] address family */
        char            sa_data[14];    /* [XSI] addr value */
    };
```

### 好的接口设计
#### getaddrinfo 地址转换

接口申明如下

```c
     #include <sys/types.h>
     #include <sys/socket.h>
     #include <netdb.h>

    struct addrinfo {
        int ai_flags;           /* input flags */
        int ai_family;          /* protocol family for socket */
        int ai_socktype;        /* socket type */
        int ai_protocol;        /* protocol for socket */
        socklen_t ai_addrlen;   /* length of socket-address */
        struct sockaddr *ai_addr; /* socket-address for socket */
        char *ai_canonname;     /* canonical name for service location */
        struct addrinfo *ai_next; /* pointer to next in list */
    };

    struct sockaddr {
        __uint8_t       sa_len;         /* total length */
        sa_family_t     sa_family;      /* [XSI] address family */
        char            sa_data[14];    /* [XSI] addr value */
    };

    /*
    * Socket address, internet style.
    */
    struct sockaddr_in {
        __uint8_t       sin_len;
        sa_family_t     sin_family;
        in_port_t       sin_port;
        struct  in_addr sin_addr;
        char            sin_zero[8];
    };

    int getaddrinfo(const char *hostname, const char *servname, const struct addrinfo *hints, struct addrinfo **res);

    void freeaddrinfo(struct addrinfo *ai);
```

> `sockaddr`和`sockaddr_in`的历史关系
> 
> `sockaddr`: 通用的结构体，用于描述套接字地址, 它本身并不包含任何具体的地址信息，而是一个抽象的概念;
> 
> `sockaddr_in`: 是一个具体的结构体, 专门用于 `IPv4` 地址, 它继承了 sockaddr 的一些属性，同时添加了 IPv4 特定的字段.
> 
>  使用: 在实际编程中，当你需要创建或接收套接字时，通常会使用 sockaddr_in 或 sockaddr_in6 来指定具体的地址信息。
> 而在调用某些函数（如 bind, connect, accept 等）时，通常需要将这些具体的结构体转换为 sockaddr 指针。

参数说明
* hostname: www.baidu.com
* hints: 过滤条件
* res: 返回值, 是一个指针的链表

不得不说 `POSIX` 的接口设计其拓展性和兼容性都特别强.

### 概念

#### 应用端缓存

##### 背景
`TCP` 是一个复杂, 可靠的字节流协议, 但是并不提供消息边界。 因此需要业务在上层做消息的编解码器(codec)。

##### 为什么需要应用端缓存

数据从网络写入到内核的队列中, 如果应用不能及时的从队列中取走这些数据, 就会阻塞对端继续发送. 从而形成事实上的 `dead lock`.

应用读取到这部分数据以后(例如写入到 `std::string` 中), 供上层应用解析， 有可能这部分数据并不能表达一个完整的消息. 这种情况下，需要继续持有这部分数据，在下次收到对端发送的数据后，继续进行 `parse`, 从而完成一次业务操作。
