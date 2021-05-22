#pragma once
#include "ISocket.h"
#include <cstdint>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <thread>

namespace sockets {

using sockets::ISocket;

class UdpMcast {
public:
    UdpMcast(ISocket *callback);

    ~UdpMcast();

    SocketRet start(const char *mcastAddr, uint16_t port);

    SocketRet sendMsg(const unsigned char *msg, size_t size);

    SocketRet finish();

private:
    void publishServerMsg(const unsigned char *msg, size_t msgSize);
    void publishDisconnected(const SocketRet &ret);
    void ReceiveTask();

    struct sockaddr_in m_sockaddr;
    int m_fd;
    bool m_stop = false;

    ISocket *m_callback;
    std::thread m_thread;
};

}  // Namespace sockets