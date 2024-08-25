//
// Created by chenshun on 2024/8/19.
//

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <cstring>

#include <arpa/inet.h> // For inet_ntop()

#include <iostream>

#include "Domain.h"

void Domain::printHost(char *domain) {
    struct addrinfo hints{}, *res0;

    memset(&hints, 0, sizeof(hints));
    //协议镞
    hints.ai_family = AF_INET;
    //socket type
    hints.ai_socktype = SOCK_STREAM;
    //tcp协议
    hints.ai_protocol = IPPROTO_TCP;
    //输入输出
    char ipstr[INET6_ADDRSTRLEN];
    //domain 是域名信息
    //hints过滤参数
    //res0返回结果
    int error = getaddrinfo(domain, nullptr, &hints, &res0);
    if (error) {
        std::cout << "error, " << gai_strerror(error) << std::endl;
        return;
    }
    struct addrinfo *res;
    for (res = res0; res; res = res->ai_next) {
        auto *ipv4 = (struct sockaddr_in *) res->ai_addr;
        inet_ntop(res->ai_family, &(ipv4->sin_addr), ipstr, sizeof ipstr);
        printf("%s: %s\n", "IPv4", ipstr);
    }
    freeaddrinfo(res0);
}
