#include "UdpSocket.h"
#include <arpa/inet.h>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

constexpr size_t MAX_PACKET_SIZE = 65535;

namespace sockets {

UdpSocket::UdpSocket(ISocket *callback) : m_fd(-1), m_callback(callback), m_thread(&UdpSocket::ReceiveTask, this) {
}

UdpSocket::~UdpSocket() {
    finish();
}

SocketRet UdpSocket::startMcast(const char *mcastAddr, uint16_t port) {
    SocketRet ret;
    if ((m_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        ret.m_success = false;
        ret.m_msg = "socket() failed";
        return ret;
    }
    // Allow multiple sockets to use the same port
    unsigned yes = 1;
    if (setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&yes, sizeof(yes)) < 0) {
        ret.m_success = false;
        ret.m_msg = "setsockopt(SO_REUSEADDR) failed";
        return ret;
    }

    sockaddr_in localAddr = {};
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    localAddr.sin_port = htons(port);

    if (::bind(m_fd, (struct sockaddr *)&localAddr, sizeof(localAddr)) < 0) {
        ret.m_success = false;
        ret.m_msg = "bind() failed";
        return ret;
    }

    // store the multicast group address for use by send()
    memset(&m_sockaddr, 0, sizeof(sockaddr));
    m_sockaddr.sin_family = AF_INET;
    m_sockaddr.sin_addr.s_addr = inet_addr(mcastAddr);
    m_sockaddr.sin_port = htons(port);

    // use setsockopt() to request that the kernel join a multicast group
    //
    struct ip_mreq mreq {};
    mreq.imr_multiaddr.s_addr = inet_addr(mcastAddr);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(m_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq, sizeof(mreq)) < 0) {
        ret.m_success = false;
        ret.m_msg = "setsockopt(IP_ADD_MEMBERSHIP) failed";
        return ret;
    }

    ret.m_success = true;
    return ret;
}

SocketRet UdpSocket::startUnicast(const char *remoteAddr, uint16_t localPort, uint16_t port) {
    SocketRet ret;
    if ((m_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        ret.m_success = false;
        ret.m_msg = "socket() failed";
        return ret;
    }
    // Allow multiple sockets to use the same port
    unsigned yes = 1;
    if (setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&yes, sizeof(yes)) < 0) {
        ret.m_success = false;
        ret.m_msg = "setsockopt(SO_REUSEADDR) failed";
        return ret;
    }

    sockaddr_in localAddr {};
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    localAddr.sin_port = htons(localPort);

    if (::bind(m_fd, (struct sockaddr *)&localAddr, sizeof(localAddr)) < 0) {
        ret.m_success = false;
        ret.m_msg = "bind() failed";
        return ret;
    }

    // store the remoteaddress for use by sendto()
    memset(&m_sockaddr, 0, sizeof(sockaddr));
    m_sockaddr.sin_family = AF_INET;
    m_sockaddr.sin_addr.s_addr = inet_addr(remoteAddr);
    m_sockaddr.sin_port = htons(port);

    ret.m_success = true;
    return ret;
}

void UdpSocket::publishUdpMsg(const unsigned char *msg, size_t msgSize) {
    if (m_callback != nullptr) {
        m_callback->onReceiveData(msg, msgSize);
    }
}

void UdpSocket::ReceiveTask() {
    constexpr int64_t USEC_DELAY = 500000;
    while (!m_stop) {
        if (m_fd != -1) {
            fd_set fds;
            struct timeval tv { 0, USEC_DELAY };
            FD_ZERO(&fds);
            FD_SET(m_fd, &fds);
            int selectRet = select(m_fd + 1, &fds, nullptr, nullptr, &tv);
            if (selectRet == -1) {  // select failed
                if (m_stop) {
                    break;
                }
            } else if (selectRet == 0) {  // timeout
                if (m_stop) {
                    break;
                }
            } else if (FD_ISSET(m_fd, &fds)) {

                unsigned char msg[MAX_PACKET_SIZE];
                ssize_t numOfBytesReceived = recv(m_fd, msg, MAX_PACKET_SIZE, 0);
                if (numOfBytesReceived < 0) {
                    SocketRet ret;
                    ret.m_success = false;
                    m_stop = true;
                    if (numOfBytesReceived == 0) {
                        ret.m_msg = "Closed connection";
                    } else {
                        ret.m_msg = strerror(errno);
                    }
                    break;
                }     
                publishUdpMsg(msg, static_cast<size_t>(numOfBytesReceived));
            }
        }
    }
}

SocketRet UdpSocket::sendMsg(const unsigned char *msg, size_t size) {
    SocketRet ret;
    ssize_t numBytesSent = sendto(m_fd, (void *)msg, size, 0, (struct sockaddr *)&m_sockaddr, sizeof(m_sockaddr));
    if (numBytesSent < 0) {  // send failed
        ret.m_success = false;
        ret.m_msg = strerror(errno);
        return ret;
    }
    if (static_cast<size_t>(numBytesSent) < size) {  // not all bytes were sent
        ret.m_success = false;
        char msg[100];
        sprintf(msg, "Only %ld bytes out of %lu was sent to client", numBytesSent, size);
        ret.m_msg = msg;
        return ret;
    }
    ret.m_success = true;
    return ret;
}

SocketRet UdpSocket::finish() {
    if (m_thread.joinable()) {
        m_stop = true;
        m_thread.join();
    }
    SocketRet ret;
    if (close(m_fd) == -1) {  // close failed
        ret.m_success = false;
        ret.m_msg = strerror(errno);
        return ret;
    }
    ret.m_success = true;
    return ret;
}

}  // Namespace sockets