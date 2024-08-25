//
// Created by chenshun on 2024/8/19.
//

#include "Client.h"

#include <sys/socket.h>
#include <sys/types.h>

#include <arpa/inet.h> // For inet_ntop()

#include "unistd.h"
#include <netdb.h>

#include <iostream>

void Client::ConnectAndInput(char *value) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in sockaddIn;
    sockaddIn.sin_family = AF_INET;
    sockaddIn.sin_addr.s_addr = inet_addr("127.0.0.1");
    sockaddIn.sin_port = htons(9987);
    int res = connect(fd, (const struct sockaddr *) &sockaddIn, sizeof sockaddIn);
    if (res != 0) {
        int value = errno;
        exit(0);
    }
    write(fd, value, strlen(value));

    char buf[1024];
    read(fd, buf, sizeof buf);
    std::cout << buf << std::endl;
    close(fd);
}