//
// Created by chenshun on 2024/8/25.
//

#ifndef SOCKET_EXAMPLE_POLL_H
#define SOCKET_EXAMPLE_POLL_H

#include "../../base/Node.h"

extern const char *EXIT;

class Poll {
public:

    explicit Poll(short port) : port_(port) {
    };


    void start();

    static void write_handler(void *param);

    static void read_handler(void *param);

    static void accept_handler(void *param);

private:
    short port_;

    static void active(int n);
};


#endif //SOCKET_EXAMPLE_POLL_H
