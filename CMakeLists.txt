cmake_minimum_required(VERSION 3.19)
project(socket_example)

set(CMAKE_CXX_STANDARD 17)

add_executable(socket_example main.cpp
        base/Node.cpp
        block/Server.h block/Server.cpp
        domain/Domain.h domain/Domain.cpp
        client/Client.h client/Client.cpp
        fork/Fork.h fork/Fork.cpp
        multiplexing/poll/Poll.cpp
        multiplexing/select/Select.cpp
        )