//
// Created by chenshun on 2024/8/21.
//

#include "Fork.h"

#include <sys/socket.h>
#include <arpa/inet.h>

#include "unistd.h"
#include "signal.h"

#include "iostream"


const int max_length = 512;
char buf[max_length];

void Fork::init(Node *header) {
    Node *dummy = header;
    int i = 0;
    while (i < 10) {
        auto node = new Node();
        node->value = i + 100;
        dummy->next = node;
        dummy = dummy->next;
        i++;
    }
}

void Fork::print(Node *header) {
    Node *dummy = header->next;
    while (dummy != nullptr) {
        std::cout << dummy->value << " ";
        dummy = dummy->next;
    }
    std::cout << std::endl;
}

void Fork::change(Node *header) {
    Node *dummy = header->next;
    while (dummy != nullptr) {
        dummy->value = dummy->value + 1000;
        dummy = dummy->next;
    }
}


void handle(int clientFd, int age) {
    int n;
    again:
    while ((n = read(clientFd, buf, sizeof buf)) > 0) {

        std::cout << "read num:" << n << ", value=" << buf << std::endl;
        write(clientFd, "good\r\n", sizeof "good\r\n");
    }
    if (n < 0 && errno == EINTR) {
        goto again;
    } else if (n < 0) {
        std::cout << "read error" << std::endl;
    }
    close(clientFd);
}


void child_process(int signalsignal) {
    std::cout << "receive a child_process" << std::endl;
}


//先不管错误处理
void Fork::Start() {
    signal(SIGCHLD, child_process);
    int servFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    int enable = 1;
    setsockopt(servFd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof enable);
    setsockopt(servFd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof enable);

    struct sockaddr_in addr{};
    addr.sin_port = htons(9987);
    addr.sin_family = AF_INET;

    bind(servFd, (const sockaddr *) &addr, sizeof(struct sockaddr_in));

    Node node{};
    init(&node);

    listen(servFd, 8);

    while (1) {
        int clientFd = accept(servFd, nullptr, nullptr);
        int pid;
        int age = 28;
        if ((pid = fork()) == 0) {
            close(servFd);
            //copy on write(COW)
            std::cout << "before update , child age = " << age << std::endl;
            sleep(5);
            std::cout << "after update , child age = " << age << std::endl;
            std::cout << "start print child node :";
            print(&node);
            handle(clientFd, age);
            _exit(0);
        }
        sleep(1);
        age = 2;
        change(&node);
        std::cout << "start print parent node :";
        print(&node);
        std::cout << "parent age = " << age << std::endl;
        close(clientFd);
    }

}