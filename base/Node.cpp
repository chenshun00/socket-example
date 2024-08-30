//
// Created by chenshun on 2024/8/26.
//

#include "Node.h"

char *EXIT = "exit";

List global_list;

Pair::Pair(pollfd &poll) : _poll(poll) {

}
