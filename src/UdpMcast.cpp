#include <stdexcept>
#include <cstring>
#include "sockets-cpp/UdpMcast.h"

namespace sockets {

UdpMcast::UdpMcast(const char* mcastAddr, uint16_t port, ISocket *callback):
    m_addr(mcastAddr),
    m_port(port),
    m_callback(callback)
{
    if ((m_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        throw std::runtime_error("socket() failed");
    }
    // Allow multiple sockets to use the same port
    unsigned yes = 1;
    if (setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR,(char*)&yes,sizeof(yes)) < 0) {
        throw std::runtime_error("setsockopt(SO_REUSEADDR)");
    }

    sockaddr_in localAddr;
    memset(&localAddr,0,sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    localAddr.sin_port = htons(port);

    if (::bind(m_fd, (struct sockaddr*)&localAddr, sizeof(localAddr)) < 0) {
        throw std::runtime_error("bind()");
    }
}

UdpMcast::~UdpMcast()
{
    
}

}   // Namespace sockets