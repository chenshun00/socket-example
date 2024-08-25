//
// Created by chenshun on 2024/8/21.
//

#ifndef SOCKET_EXAMPLE_FORK_H
#define SOCKET_EXAMPLE_FORK_H


struct Node {
    int value;
    Node *prev;
    Node *next;
};

class Fork {
public:
    static void init(Node *header);

    static void print(Node *header);

    static void change(Node* header);

    static void Start();
};


#endif //SOCKET_EXAMPLE_FORK_H
