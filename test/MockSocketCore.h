#pragma once
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif // WIN32_LEAN_AND_MEAN
    #include <winsock2.h>
    #include <windows.h>
    #include <ws2tcpip.h>
    #include <inaddr.h>
    // Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
    #pragma comment (lib, "Ws2_32.lib")
    #pragma comment (lib, "Mswsock.lib")
    #pragma comment (lib, "AdvApi32.lib")
#else
    #include <sys/select.h>
    #include <sys/socket.h>
    #include <netdb.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif

#ifdef _WIN32
using ssize_t = int;
using in_addr_t = ULONG;
#else
using SOCKET = int;
constexpr SOCKET INVALID_SOCKET = -1;
#endif

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