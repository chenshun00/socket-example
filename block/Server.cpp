//
// Created by chenshun on 2024/8/18.
//

#include "Server.h"

#include <stdexcept>

#include <sys/socket.h>
#include <sys/types.h>

#include <unistd.h>
#include <cstring>

#include <netinet/in.h>

#include <iostream>

#include <pthread.h>

bool stop = 1;

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

int Server::start(int port) {
    auto serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd == -1) {
        throw std::logic_error("open socket error");
    }
    int open = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEPORT, &open, sizeof open);
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &open, sizeof open);
    struct sockaddr_in addr{};
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);


    int res = bind(serverFd, (const sockaddr *) &addr, sizeof(addr));
    if (res != 0) {
        close(serverFd);
        throw std::logic_error("bind socket error");
    }
    auto listenRes = listen(serverFd, 128);
    if (listenRes != 0) {
        close(serverFd);
        throw std::logic_error("bind socket error");
    }
    std::cout << "listen at localhost:" << port << std::endl;
    char buf[1024];
    while (stop) {
        //accept will block
        int clientFd = accept(serverFd, nullptr, nullptr);
        pthread_t id;
        pthread_create(&id, nullptr, &run, &clientFd);
    }
}