#pragma once

#include <sys/types.h>
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

namespace sockets {

#ifdef _WIN32
using ssize_t = int;
using in_addr_t = ULONG;
#else
using SOCKET = int;
constexpr SOCKET INVALID_SOCKET = -1;
#endif

/**
 * @brief SocketCore is a pass-through interface to standard Socket API calls.
 *        This class exists to facilitate unit testing using an alternate MockSocketCore
 *        implementation.
 */
class SocketCore {
public:
    SocketCore() = default;

    ~SocketCore() {
#ifdef _WIN32
        ::WSACleanup();
#endif
    }

    int Initialize() {
#ifdef _WIN32
        return WSAStartup(MAKEWORD(2,2), &m_wsaData);
#else
        return 0;
#endif
    }

    SOCKET Socket(int domain, int type, int protocol) {
        return ::socket(domain, type, protocol);
    }

    int SetSockOpt(SOCKET sockfd, int level, int optname, void * optval, socklen_t optlen) {
#ifdef _WIN32
        return ::setsockopt(sockfd, level, optname, reinterpret_cast<const char*>(optval), optlen);
#else
        return ::setsockopt(sockfd, level, optname, optval, optlen);
#endif
    }

    int Bind(SOCKET sockfd, const struct sockaddr *addr, socklen_t addrlen) {
        return ::bind(sockfd, addr, addrlen);
    }

    SOCKET Accept(SOCKET sockfd, struct sockaddr *addr, socklen_t *addrlen) {
        return ::accept(sockfd, addr, addrlen);
    }

    int Listen(SOCKET sockfd, int backlog) {
        return ::listen(sockfd, backlog);
    }

    int Connect(SOCKET sockfd, const struct sockaddr *addr, socklen_t addrlen) {
        return ::connect(sockfd, addr, addrlen);
    }

    int Close(SOCKET sockfd) {
#ifdef _WIN32
        return ::closesocket(sockfd);
#else
        return ::close(sockfd);
#endif
    }

    int Select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval * timeout) {
        return ::select(nfds, readfds, writefds, exceptfds, timeout);
    }

    ssize_t Recv(int sockfd, void *buf, size_t len, int flags) {
#ifdef _WIN32
        return ::recv(sockfd, reinterpret_cast<char*>(buf), static_cast<int>(len), flags);
#else
        return ::recv(sockfd, buf, len, flags);
#endif
    }

    ssize_t SendTo(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen) {
#ifdef _WIN32
        return ::sendto(sockfd, reinterpret_cast<const char*>(buf), static_cast<int>(len), flags, dest_addr, addrlen);
#else
        return ::sendto(sockfd, buf, len, flags, dest_addr, addrlen);
#endif
    }

    ssize_t Send(int sockfd, const void *buf, size_t len, int flags) {
#ifdef _WIN32
        return ::send(sockfd, reinterpret_cast<const char*>(buf), static_cast<int>(len), flags);
#else        
        return ::send(sockfd, buf, len, flags);
#endif        
    }

    int GetAddrInfo(const char *node, const char *service, const addrinfo *hints, addrinfo **res) {
        return ::getaddrinfo(node, service, hints, res);
    }

    void FreeAddrInfo(struct addrinfo *res) {
        return ::freeaddrinfo(res);
    }

private:
#ifdef _WIN32
    WSADATA  m_wsaData;
#endif
};

}  // namespace sockets