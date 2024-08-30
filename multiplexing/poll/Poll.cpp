//
// Created by chenshun on 2024/8/25.
//

#include "Poll.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <cerrno>
#include <fcntl.h>

#include <sys/poll.h>

extern const int read_event;
extern const int write_event;

int MAX_SIZE = 1024;

struct ApiState {
    int maxFds{0};
    struct pollfd *event{};
};
ApiState apiState;
EventLoop eventLoop;

void add_event(struct pollfd event) {

    for (int i = 0; i < MAX_SIZE; ++i) {
        if (apiState.event[i].fd == -1) {
            apiState.event[i] = event;
            apiState.maxFds++;
            break;
        }
    }
}

void refresh() {
    for (int i = 1; i < apiState.maxFds; ++i) {
        if (apiState.event[i].fd == -1) {
            if (i < MAX_SIZE - 1) {
                std::swap(apiState.event[i], apiState.event[i + 1]);
            }
        }
    }
}

void del_event(struct pollfd event) {
    for (int i = 1; i < MAX_SIZE; ++i) {
        if (apiState.event[i].fd == event.fd) {
            apiState.maxFds--;
            apiState.event[i].fd = -1;
            apiState.event[i].events = 0;
            apiState.event[i].revents = 0;
            break;
        }
    }
}

void Poll::read_handler(void *param) {
    Pair *pair = (Pair *) param;

    char cache_data[128];
    memset(cache_data, 0, sizeof cache_data);
    size_t num = 0;
    do {
        num = read(pair->firedEvent->fd, cache_data, sizeof cache_data);
    } while (num < 0 && errno == EINTR);
    if (num > 0) {
        std::cout << "read msg:" << cache_data;
        int _exit = strncmp(cache_data, EXIT, 4);
        if (_exit == 0) {
            close(pair->firedEvent->fd);
            del_event(pair->_poll);
            return;
        }
        pair->_poll.revents = 0;
        pair->_poll.events = POLLOUT;
    } else if (num == 0) {
        close(pair->firedEvent->fd);
        del_event(pair->_poll);
    }
}

void Poll::write_handler(void *param) {
    Pair *pair = (Pair *) param;

    char *msg = "good\r\n";
    write(pair->firedEvent->fd, msg, strlen(msg));
    write(pair->firedEvent->fd, msg, strlen(msg));

    pair->_poll.revents = 0;
    pair->_poll.events = POLLIN;
}

void Poll::accept_handler(void *param) {
    Pair *pair = (Pair *) param;
    int servFd = pair->firedEvent->fd;
    //未处理中断等信息
    int clientFd = accept(servFd, nullptr, nullptr);
    if (clientFd > 0) {
        struct pollfd event{};
        event.fd = clientFd;
        event.events = POLLIN;
        add_event(event);

        eventLoop.fileEvent[clientFd].rfileProc = read_handler;
        eventLoop.fileEvent[clientFd].wfileProc = write_handler;
    }
}

void Poll::active(int n) {

    for (int i = 0; i < MAX_SIZE; ++i) {
        struct pollfd _pollfd = apiState.event[i];
        if (_pollfd.fd == -1) {
            break;
        }
        int mask = 0;
        if (_pollfd.revents & POLLIN) {
            mask |= read_event;
        }
        if (_pollfd.revents & POLLOUT) {
            mask |= write_event;
        }
        eventLoop.firedEvent[i].fd = _pollfd.fd;
        eventLoop.firedEvent[i].mask = mask;
    }

    for (int i = 0; i < MAX_SIZE; ++i) {
        //如果不使用引用的话, 注意这里的拷贝构造，会导致raw data不更新.
        struct pollfd &_pollfd = apiState.event[i];
        if (_pollfd.fd == -1) {
            break;
        }
        struct FiredEvent firedEvent = eventLoop.firedEvent[i];
        struct FileEvent event = eventLoop.fileEvent[firedEvent.fd];

        Pair *_pair = new Pair(_pollfd);
        _pair->firedEvent = &firedEvent;
        _pair->fileEvent = &event;

        if ((firedEvent.mask & read_event) == read_event) {
            event.rfileProc(_pair);
        }
        if ((firedEvent.mask & write_event) == write_event) {
            event.wfileProc(_pair);
        }

        delete _pair;
    }
}

void Poll::start() {
    int servFd = socket(AF_INET, SOCK_STREAM, 0);

    int enable = 1;
    setsockopt(servFd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof enable);
    setsockopt(servFd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof enable);

    int flags = fcntl(servFd, F_GETFD);
    fcntl(servFd, F_SETFD, flags | O_NONBLOCK);

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(this->port_);

    bind(servFd, (struct sockaddr *) &addr, sizeof addr);

    apiState.event = new pollfd[MAX_SIZE];

    for (int i = 0; i < MAX_SIZE; ++i) {
        apiState.event[i].fd = -1;
    }

    struct pollfd event{};
    event.fd = servFd;
    event.events = POLLIN;
    add_event(event);

    eventLoop.fileEvent = new FileEvent[MAX_SIZE];
    eventLoop.firedEvent = new FiredEvent[MAX_SIZE];

    struct FileEvent *evnet = &eventLoop.fileEvent[servFd];
    evnet->mask = 0 | read_event;
    evnet->rfileProc = accept_handler;

    listen(servFd, 8);

    while (1) {
        refresh();
        int n = poll(apiState.event, apiState.maxFds + 1, 3000);
        if (n < 0 && errno == EINTR) {
            continue;
        }
        if (n > 0) {
            this->active(n);
        }
    }
    close(servFd);
}