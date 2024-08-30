//
// Created by chenshun on 2024/8/25.
//

#ifndef SOCKET_EXAMPLE_NODE_H
#define SOCKET_EXAMPLE_NODE_H

#include "iostream"
#include "string"
#include "functional"
#include "unistd.h"

typedef void (*Callback)(void *);

const int POST = 9987;

const int read_event = 2;
const int write_event = 4;

class ListNode;

class List;

struct FdNode {
    int fd;
    int mask;
    FdNode *next;
    FdNode *prev;
    Callback callback;
};

/* A fired event, copy from redis */
struct FiredEvent {
    int fd;
    int mask;
};

/* File event structure */
struct FileEvent {
    int mask; /* one of AE_(READABLE|WRITABLE|BARRIER) */
    void *clientData;
    Callback rfileProc;
    Callback wfileProc;
};

struct EventLoop {
    struct FileEvent *fileEvent;
    struct FiredEvent *firedEvent;
};

class ListNode {
public:
    int fd{};
    int index = -1;
    std::string buf;
    int mask{};
    ListNode *prev{};
    ListNode *next{};
    Callback callback{};

    ~ListNode() {
        prev = nullptr;
        next = nullptr;
        buf.clear();
        close(fd);
    }
};

struct Pair {
    Pair(struct pollfd &poll);

    FiredEvent *firedEvent;
    FileEvent *fileEvent;
    struct pollfd &_poll;
};

class List {
public:
    ListNode *head;

    List() : head(new ListNode()) {
        this->init();
    }

    ~List() {
        delete head;
    }

private:
    void init() const {
        this->head->next = this->head;
        this->head->prev = this->head;
    }

};

#endif //SOCKET_EXAMPLE_NODE_H
