#pragma once

#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

namespace sockets {

/**
 * @brief SocketCore is a pass-through interface to standard Socket API calls.
 *        This class exists to facilitate unit testing using an alternate MockSocketCore
 *        implementation.
 */
class SocketCore {
public:
    SocketCore() = default;

    ~SocketCore() = default;

    int Socket(int domain, int type, int protocol) {
        return ::socket(domain, type, protocol);
    }

    int SetSockOpt(int sockfd, int level, int optname, void * optval, socklen_t optlen) {
        return ::setsockopt(sockfd, level, optname, optval, optlen);
    }

    int Bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
        return ::bind(sockfd, addr, addrlen);
    }

    int Accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
        return ::accept(sockfd, addr, addrlen);
    }

    int Listen(int sockfd, int backlog) {
        return ::listen(sockfd, backlog);
    }

    int Connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
        return ::connect(sockfd, addr, addrlen);
    }

    int Close(int sockfd) {
        return ::close(sockfd);
    }

    int Select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval * timeout) {
        return ::select(nfds, readfds, writefds, exceptfds, timeout);
    }

    ssize_t Recv(int sockfd, void *buf, size_t len, int flags) {
        return ::recv(sockfd, buf, len, flags);
    }

    ssize_t SendTo(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen) {
        return ::sendto(sockfd, buf, len, flags, dest_addr, addrlen);
    }

    ssize_t Send(int sockfd, const void *buf, size_t len, int flags) {
        return ::send(sockfd, buf, len, flags);
    }

    int GetAddrInfo(const char *node, const char *service, const addrinfo *hints, addrinfo **res) {
        return ::getaddrinfo(node, service, hints, res);
    }

    void FreeAddrInfo(struct addrinfo *res) {
        return ::freeaddrinfo(res);
    }
};

}  // namespace sockets