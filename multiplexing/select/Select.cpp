//
// Created by chenshun on 2024/8/25.
//

#include "Select.h"

#include "sys/select.h"
#include "sys/socket.h"
#include <sys/types.h>
#include <arpa/inet.h>
#include <cerrno>

#include <fcntl.h>

#include <sys/uio.h>
#include <unistd.h>

#include "iostream"
#include <cstring>

const int POST = 9987;
const char *EXIT = "exit";

typedef void (*Callback)(void *);

int read_event = 2;
int write_event = 4;

struct FdNode {
    int fd;
    int mask;
    FdNode *next;
    FdNode *prev;
    Callback callback;
};

void _free(FdNode *node);

void write_handler(void *param);

void read_handler(void *param);

void accept_handler(void *param);

struct fd_set readfds, writefds, errorfds;

void _free(FdNode *node) {
    node->next = nullptr;
    node->prev = nullptr;
    node->callback = nullptr;
    delete node;
}

FdNode fdNodes;

void init_headers() {

}

void del(FdNode *node, const int fd) {
    FdNode *dummy = node->next;
    while (dummy != nullptr && dummy->fd != fd) {
        dummy = dummy->next;
    }
    if (dummy == nullptr) {
        return;
    }
    dummy->prev->next = dummy->next;
    if (dummy->next != nullptr) {
        dummy->next->prev = dummy->prev;
    }
    dummy->next = nullptr;
    dummy->prev = nullptr;
    dummy->callback = nullptr;
    delete dummy;
}

void register_read_only(FdNode *node, const int fd, int type, Callback callback) {
    FdNode *dummy = node;
    while (dummy != nullptr && dummy->next != nullptr) {
        dummy = dummy->next;
    }
    auto temp = new FdNode();
    temp->fd = fd;
    temp->callback = callback;
    if (type == read_event) {
        temp->mask = 1 | read_event;
    } else {
        temp->mask = 1 | write_event;
    }
    dummy->next = temp;
    temp->prev = dummy;
}

void change_mask(FdNode *params, const int fd, int type, Callback callback) {
    FdNode *dummy = params->next;
    while (dummy != nullptr && dummy->fd != fd) {
        dummy = dummy->next;
    }
    if (dummy == nullptr) {
        return;
    }
    if (type == read_event) {
        dummy->mask = 1 | read_event;
    } else {
        dummy->mask = 1 | write_event;
    }
    dummy->callback = callback;
}

void write_handler(void *param) {
    int clientFd = *(int *) param;
    char *msg = "good\r\n";
    //注意sizeof(msg)和strlen(msg)的区别
    write(clientFd, msg, strlen(msg));
    write(clientFd, msg, strlen(msg));
    change_mask(&fdNodes, clientFd, read_event, read_handler);
}

void read_handler(void *param) {
    int clientFd = *(int *) param;
    char buf_cache[512];
    memset(buf_cache, 0, sizeof buf_cache);
    //todo 没有处理中断
    ssize_t size = read(clientFd, buf_cache, sizeof(buf_cache));
    if (size == 0) {
        //说明close了
        close(clientFd);
        del(&fdNodes, clientFd);
    } else {
        std::cout << "read message: " << buf_cache;
        int _exit = strncmp(buf_cache, EXIT, 4);
        if (_exit == 0) {
            del(&fdNodes, clientFd);
            close(clientFd);
            return;
        }
        change_mask(&fdNodes, clientFd, write_event, write_handler);
    }
}

void accept_handler(void *param) {
    int servFd = *(int *) param;
    int clientFd = accept(servFd, nullptr, nullptr);
    std::cout << "connection active" << std::endl;
    register_read_only(&fdNodes, clientFd, read_event, read_handler);
}

inline int max(int a, int b) {
    return a > b ? a : b;
}

int max_fd(FdNode *node) {
    int res = 0;
    FdNode *dummy = node->next;
    while (dummy != nullptr) {
        res = max(res, dummy->fd);
        dummy = dummy->next;
    }
    return res + 1;
}

int _Select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *errorfds, struct timeval *timeout) {
    int n;
    do {
        n = select(nfds, readfds, writefds, errorfds, timeout);
    } while (n < 0 && errno == EINTR);
    return n;
}

void fdset(fd_set *set, FdNode *nodes, int type) {
    FD_ZERO(set);
    FdNode *dummy = nodes->next;
    while (dummy != nullptr) {
        if ((dummy->mask & type) == type) {
            FD_SET(dummy->fd, set);
        }
        dummy = dummy->next;
    }
}

void active(FdNode *node, fd_set *read, fd_set *write) {
    auto dummy = node->next;
    while (dummy != nullptr) {
        if ((dummy->mask & read_event) == read_event) {
            if (FD_ISSET(dummy->fd, read)) {
                dummy->callback((void *) &dummy->fd);
            }
        } else if ((dummy->mask & write_event) == write_event) {
            if (FD_ISSET(dummy->fd, write)) {
                dummy->callback((void *) &dummy->fd);
            }
        }
        dummy = dummy->next;
    }
}

void Select::start() {
    auto servFd = socket(AF_INET, SOCK_STREAM, 0);
    int enable = 1;
    size_t size = sizeof enable;
    setsockopt(servFd, SOL_SOCKET, SO_REUSEADDR, &enable, size);
    setsockopt(servFd, SOL_SOCKET, SO_REUSEPORT, &enable, size);

    struct sockaddr_in addr{};
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(POST);

    //设置非阻塞
    int flags = fcntl(servFd, F_GETFD);
    fcntl(servFd, F_SETFD, flags | O_NONBLOCK);

    bind(servFd, (struct sockaddr *) &addr, sizeof(struct sockaddr_in));
    listen(servFd, 8);

    register_read_only(&fdNodes, servFd, read_event, accept_handler);

    while (1) {
        struct timeval timeout{};
        timeout.tv_sec = 1;
        timeout.tv_usec = 10000;

        fdset(&readfds, &fdNodes, read_event);
        fdset(&writefds, &fdNodes, write_event);

        int maxfdl = max_fd(&fdNodes);
        int n = _Select(maxfdl, &readfds, &writefds, nullptr, &timeout);
        if (n > 0) {
            //io就绪
            active(&fdNodes, &readfds, &writefds);
        } else if (n == 0) {
            continue;
        }
    }
    close(servFd);
}
