#include "UdpSocket.h"
#include <cstring>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_PACKET_SIZE 65535

namespace sockets {

UdpSocket::UdpSocket(ISocket *callback) : m_fd(-1), m_callback(callback), m_thread(&UdpSocket::ReceiveTask, this) {
}

UdpSocket::~UdpSocket() {
    m_stop = true;
    m_thread.detach();
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

    sockaddr_in localAddr;
    memset(&localAddr, 0, sizeof(localAddr));
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
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(mcastAddr);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(m_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*) &mreq, sizeof(mreq)) < 0){
        ret.m_success = false;
        ret.m_msg = "setsockopt(IP_ADD_MEMBERSHIP) failed";
        return ret;
    }

    ret.m_success = true;
    return ret;
}

SocketRet UdpSocket::startUnicast(const char *remoteAddr, uint16_t localPort, uint16_t port)
{
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

    sockaddr_in localAddr;
    memset(&localAddr, 0, sizeof(localAddr));
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

void UdpSocket::publishServerMsg(const unsigned char *msg, size_t msgSize) {
    if (m_callback) {
        m_callback->onReceiveData(msg, msgSize);
    }
}

void UdpSocket::publishDisconnected(const SocketRet &ret) {
    if (m_callback) {
        m_callback->onDisconnect(ret);
    }
}

void UdpSocket::ReceiveTask() {
    while (!m_stop) {
        if (m_fd != -1) {
            unsigned char msg[MAX_PACKET_SIZE];
            int numOfBytesReceived = recv(m_fd, msg, MAX_PACKET_SIZE, 0);
            if (numOfBytesReceived < 1) {
                SocketRet ret;
                ret.m_success = false;
                m_stop = true;
                if (numOfBytesReceived == 0) {
                    ret.m_msg = "Closed connection";
                } else {
                    ret.m_msg = strerror(errno);
                }
                publishDisconnected(ret);
                finish();
                break;
            } else {
                publishServerMsg(msg, numOfBytesReceived);
            }
        }
    }
}

SocketRet UdpSocket::sendMsg(const unsigned char *msg, size_t size)
{
    SocketRet ret;
    int numBytesSent = sendto(m_fd, (void*)msg, size, 0,(struct sockaddr*)&m_sockaddr, sizeof(m_sockaddr));
    if (numBytesSent < 0 ) { // send failed
        ret.m_success = false;
        ret.m_msg = strerror(errno);
        return ret;
    }
    if ((uint)numBytesSent < size) { // not all bytes were sent
        ret.m_success = false;
        char msg[100];
        sprintf(msg, "Only %d bytes out of %lu was sent to client", numBytesSent, size);
        ret.m_msg = msg;
        return ret;
    }
    ret.m_success = true;
    return ret;   
}

SocketRet UdpSocket::finish(){
    m_stop = true;
    m_thread.join();
    SocketRet ret;
    if (close(m_fd) == -1) { // close failed
        ret.m_success = false;
        ret.m_msg = strerror(errno);
        return ret;
    }
    ret.m_success = true;
    return ret;
}

}  // Namespace sockets