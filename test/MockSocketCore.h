#pragma once
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

/**
 * @brief Mock class for socket api calls
 * 
 */
class MockSocketCore {
public:
    MockSocketCore() = default;

    MockSocketCore(const MockSocketCore &) = default;

    ~MockSocketCore() = default;

    int Initialize() {
        return 0;
    }

    MOCK_METHOD(int, Socket, (int domain, int type, int protocol), ());

    MOCK_METHOD(int, SetSockOpt, (int sockfd, int level, int optname, void *optval, socklen_t optlen), ());

    MOCK_METHOD(int, Bind, (int sockfd, const struct sockaddr *addr, socklen_t addrlen), ());

    MOCK_METHOD(int, Accept, (int sockfd, struct sockaddr *addr, socklen_t *addrlen), ());

    MOCK_METHOD(int, Listen, (int sockfd, int backlog), ());

    MOCK_METHOD(int, Connect, (int sockfd, const struct sockaddr *addr, socklen_t addrlen), ());

    MOCK_METHOD(int, Close, (int sockfd), ());

    MOCK_METHOD(int, Select, (int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout), ());

    MOCK_METHOD(ssize_t, Recv, (int sockfd, char *buf, size_t len, int flags), ());

    MOCK_METHOD(ssize_t, SendTo,
        (int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen), ());

    MOCK_METHOD(ssize_t, Send, (int sockfd, const void *buf, size_t len, int flags), ());

    MOCK_METHOD(int, GetAddrInfo, (const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res), ());

    MOCK_METHOD(void, FreeAddrInfo,(struct addrinfo *res), ());
};