#include <iostream>

#include "block/Server.h"
#include "domain/Domain.h"
#include "fork/Fork.h"
#include "multiplexing/select/Select.h"
#include "multiplexing/poll/Poll.h"

void start() {
    Server::start(9987);
}

void printDomain(char *domain) {
    Domain::printHost(domain);
}

void fork_start() {
    Fork::Start();
}

void fork_node() {
    Node node{};
    Fork::init(&node);
    Fork::print(&node);
}

void select_start() {
    Select::start();
}

void poll_start(){
    Poll poll(9987);
    poll.start();
}

int main() {
    std::cout << "Hello, World!" << std::endl;
    printDomain("183.2.172.42");
    poll_start();
    return 0;
}
