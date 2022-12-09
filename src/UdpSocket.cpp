#include "UdpSocket.h"
#include "AddrLookup.h"
#include <array>
#include <arpa/inet.h>
#include <cstring>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#if defined(FMT_SUPPORT)
#include <fmt/core.h>
#endif

/**
 * @brief Max UDP datagram size for UDP protocol
 * 
 */
constexpr size_t MAX_PACKET_SIZE = 65507;

namespace sockets {

UdpSocket::UdpSocket(IUdpSocket *callback, SocketOpt *options)
    : m_sockaddr({}), m_callback(callback) {
    if (options != nullptr) {
        m_sockOptions = *options;
    }
}

UdpSocket::~UdpSocket() {
    finish();
}

SocketRet UdpSocket::startMcast(const char *mcastAddr, uint16_t port) {
    SocketRet ret;
    m_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_fd < 0) {
        ret.m_success = false;
#if defined(FMT_SUPPORT)
        ret.m_msg = fmt::format("Error: socket() failed: errno {}", errno);
#else
        ret.m_msg = "socket() failed";
#endif
        return ret;
    }
    // Allow multiple sockets to use the same port
    unsigned yes = 1;
    if (setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char *>(&yes), sizeof(yes)) < 0) {
        ret.m_success = false;
#if defined(FMT_SUPPORT)
        ret.m_msg = fmt::format("Error: setsockopt(SO_REUSEADDR) failed: errno {}", errno);
#else
        ret.m_msg = "setsockopt(SO_REUSEADDR) failed";
#endif
        return ret;
    }
    // Set TX and RX buffer sizes
    if (setsockopt(m_fd, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<char*>(&m_sockOptions.m_rxBufSize), sizeof(m_sockOptions.m_rxBufSize)) < 0) {
        ret.m_success = false;
#if defined(FMT_SUPPORT)
        ret.m_msg = fmt::format("Error: setsockopt(SO_RCVBUF) failed: errno {}", errno);
#else
        ret.m_msg = "setsockopt(SO_RCVBUF) failed";
#endif
        return ret;
    }

    if (setsockopt(m_fd, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<char*>(&m_sockOptions.m_txBufSize), sizeof(m_sockOptions.m_txBufSize)) < 0) {
        ret.m_success = false;
#if defined(FMT_SUPPORT)
        ret.m_msg = fmt::format("Error: setsockopt(SO_SNDBUF) failed: errno {}", errno);
#else
        ret.m_msg = "setsockopt(SO_SNDBUF) failed";
#endif
        return ret;
    }       

    sockaddr_in localAddr = {};
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    localAddr.sin_port = htons(port);

    if (::bind(m_fd, reinterpret_cast<struct sockaddr *>(&localAddr), sizeof(localAddr)) < 0) {
        ret.m_success = false;
#if defined(FMT_SUPPORT)
        ret.m_msg = fmt::format("Error: bind() failed: errno {}", errno);
#else
        ret.m_msg = "bind() failed";
#endif
        return ret;
    }

    // store the multicast group address for use by send()
    memset(&m_sockaddr, 0, sizeof(sockaddr));
    m_sockaddr.sin_family = AF_INET;
    m_sockaddr.sin_addr.s_addr = inet_addr(mcastAddr);
    m_sockaddr.sin_port = htons(port);

    // use setsockopt() to request that the kernel join a multicast group
    //
    struct ip_mreq mreq { };
    mreq.imr_multiaddr.s_addr = inet_addr(mcastAddr);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(m_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, reinterpret_cast<char *>(&mreq), sizeof(mreq)) < 0) {
        ret.m_success = false;
#if defined(FMT_SUPPORT)
        ret.m_msg = fmt::format("Error: setsockopt(IP_ADD_MEMBERSHIP) failed: errno {}", errno);
#else
        ret.m_msg = "setsockopt(IP_ADD_MEMBERSHIP) failed";
#endif
        return ret;
    }

    m_thread = std::thread(&UdpSocket::ReceiveTask, this);
    ret.m_success = true;
    return ret;
}

SocketRet UdpSocket::startUnicast(uint16_t localPort) {
    SocketRet ret;
    m_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_fd < 0) {
        ret.m_success = false;
#if defined(FMT_SUPPORT)
        ret.m_msg = fmt::format("Error: socket() failed: errno {}", errno);
#else
        ret.m_msg = "socket() failed";
#endif
        return ret;
    }
    // Allow multiple sockets to use the same port
    unsigned yes = 1;
    if (setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char *>(&yes), sizeof(yes)) < 0) {
        ret.m_success = false;
#if defined(FMT_SUPPORT)
        ret.m_msg = fmt::format("Error: setsockopt(SO_REUSEADDR) failed: errno {}", errno);
#else
        ret.m_msg = "setsockopt(SO_REUSEADDR) failed";
#endif
        return ret;
    }

    // Set TX and RX buffer sizes
    if (setsockopt(m_fd, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<char*>(&m_sockOptions.m_rxBufSize), sizeof(m_sockOptions.m_rxBufSize)) < 0) {
        ret.m_success = false;
#if defined(FMT_SUPPORT)
        ret.m_msg = fmt::format("Error: setsockopt(SO_RCVBUF) failed: errno {}", errno);
#else
        ret.m_msg = "setsockopt(SO_RCVBUF) failed";
#endif
        return ret;
    }

    if (setsockopt(m_fd, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<char*>(&m_sockOptions.m_txBufSize), sizeof(m_sockOptions.m_txBufSize)) < 0) {
        ret.m_success = false;
#if defined(FMT_SUPPORT)
        ret.m_msg = fmt::format("Error: setsockopt(SO_SNDBUF) failed: errno {}", errno);
#else
        ret.m_msg = "setsockopt(SO_SNDBUF) failed";
#endif
        return ret;
    }      

    sockaddr_in localAddr {};
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    localAddr.sin_port = htons(localPort);

    if (::bind(m_fd, reinterpret_cast<struct sockaddr *>(&localAddr), sizeof(localAddr)) < 0) {
        ret.m_success = false;
#if defined(FMT_SUPPORT)
        ret.m_msg = fmt::format("Error: bind() failed: errno {}", errno);
#else
        ret.m_msg = "bind() failed";
#endif
        return ret;
    }

    m_thread = std::thread(&UdpSocket::ReceiveTask, this);
    ret.m_success = true;
    return ret;
}

SocketRet UdpSocket::startUnicast(const char *remoteAddr, uint16_t localPort, uint16_t port) {
    SocketRet ret;

    // store the remoteaddress for use by sendto()
    memset(&m_sockaddr, 0, sizeof(sockaddr));
    m_sockaddr.sin_family = AF_INET;
    int inetSuccess = inet_aton(remoteAddr, &m_sockaddr.sin_addr);
    if (inetSuccess == 0) {// inet_addr failed to parse address
        if (lookupHost(remoteAddr,m_sockaddr.sin_addr.s_addr) != 0) {
#if defined(FMT_SUPPORT)
            ret.m_msg = fmt::format("Failed to resolve hostname {}",remoteAddr);
#else
            ret.m_msg = "Failed to resolve hostname";
#endif
            return ret;
        }
    }


    m_sockaddr.sin_port = htons(port);

    // return the result of setting up the local server
    return startUnicast(localPort);
}

void UdpSocket::publishUdpMsg(const char *msg, size_t msgSize) {
    if (m_callback != nullptr) {
        m_callback->onReceiveData(msg, msgSize);
    }
}

void UdpSocket::ReceiveTask() {
    constexpr int64_t USEC_DELAY = 500000;
    m_stop = false;
    while (!m_stop.load()) {
        if (m_fd != -1) {
            fd_set fds;
            struct timeval delay {
                0, USEC_DELAY
            };
            FD_ZERO(&fds);
            FD_SET(m_fd, &fds);
            int selectRet = select(m_fd + 1, &fds, nullptr, nullptr, &delay);
            if (selectRet <= 0) {  // select failed or timeout
                if (m_stop) {
                    break;
                }
            } else if (FD_ISSET(m_fd, &fds)) {

                std::array<char, MAX_PACKET_SIZE> msg;
                ssize_t numOfBytesReceived = recv(m_fd, msg.data(), MAX_PACKET_SIZE, 0);
                if (numOfBytesReceived < 0) {
                    SocketRet ret;
                    ret.m_success = false;
                    m_stop = true;
                    if (numOfBytesReceived == 0) {
#if defined(FMT_SUPPORT)
                        ret.m_msg = fmt::format("Closed connection");
#else
                        ret.m_msg = "Closed connection";
#endif
                    } else {
#if defined(FMT_SUPPORT)
                        ret.m_msg = fmt::format("Error: errno {}", errno);
#else
                        ret.m_msg = strerror(errno);
#endif
                    }
                    break;
                }
                publishUdpMsg(msg.data(), static_cast<size_t>(numOfBytesReceived));
            }
        }
    }

}

SocketRet UdpSocket::sendMsg(const char *msg, size_t size) {
    SocketRet ret;
    // If destination addr/port specified
    if (m_sockaddr.sin_port != 0) {
        ssize_t numBytesSent = sendto(m_fd, &msg[0], size, 0, reinterpret_cast<struct sockaddr *>(&m_sockaddr), sizeof(m_sockaddr));
        if (numBytesSent < 0) {  // send failed
            ret.m_success = false;
#if defined(FMT_SUPPORT)
            ret.m_msg = fmt::format("Error: errno {}", errno);
#else
            ret.m_msg = strerror(errno);
#endif
            return ret;
        }
        if (static_cast<size_t>(numBytesSent) < size) {  // not all bytes were sent
            ret.m_success = false;
#if defined(FMT_SUPPORT)
            ret.m_msg = fmt::format("Only {} bytes of {} was sent to client", numBytesSent, size);
#else
            char msg[100];
            snprintf(msg, sizeof(msg), "Only %ld bytes out of %lu was sent to client", numBytesSent, size);
            ret.m_msg = msg;
#endif
            return ret;
        }
    }
    ret.m_success = true;
    return ret;
}

SocketRet UdpSocket::finish() {
    m_stop = true;
    if (m_thread.joinable()) {
        m_thread.join();
    }
    SocketRet ret;
    if (close(m_fd) == -1) {  // close failed
        ret.m_success = false;
#if defined(FMT_SUPPORT)
        ret.m_msg = fmt::format("Error: errno {}", errno);
#else
        ret.m_msg = strerror(errno);
#endif
        return ret;
    }
    ret.m_success = true;
    return ret;
}

}  // Namespace sockets