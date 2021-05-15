#pragma once
#include <thread>
#include <string>
#include <netinet/in.h>
#include <sys/socket.h>
#include <cstdint>
#include "ISocket.h"

class UdpMcast: public ISocket {
public:
    UdpMcast(const char* mcastAddr, uint16_t port, ISocket *callback);

    ~UdpMcast();

    SocketRet sendMsg(const unsigned char * msg, size_t size);

    SocketRet finish();

private:
    std::string m_addr;
    uint16_t m_port;
    struct sockaddr_in m_sockaddr;
    int m_fd;
    bool m_stop = false; 

    ISocket *m_callback;
    std::thread m_thread;
};