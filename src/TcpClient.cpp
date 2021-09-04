#include "TcpClient.h"

#if defined(FMT_SUPPORT)
    #include <fmt/core.h>
#endif

constexpr size_t MAX_PACKET_SIZE = 4096;

namespace sockets {

TcpClient::TcpClient(ISocket *callback) : m_server({}), m_callback(callback) {
}

TcpClient::~TcpClient() {
    finish();
}

SocketRet TcpClient::connectTo(const char *remoteIp, uint16_t remotePort) {
    m_sockfd = 0;
    SocketRet ret;

    m_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_sockfd == -1) {  // socket failed
        ret.m_success = false;
        ret.m_msg = strerror(errno);
        return ret;
    }

    // set TX and RX buffer sizes
    int option_value = RX_BUFFER_SIZE;
    if (setsockopt(m_sockfd, SOL_SOCKET, SO_RCVBUF, (char*)&option_value, sizeof(option_value)) < 0) {
        ret.m_success = false;
#if defined(FMT_SUPPORT)
        ret.m_msg = fmt::format("Error: setsockopt(SO_RCVBUF) failed: {}", strerror(errno));
#else
        ret.m_msg = "setsockopt(SO_REUSEADDR) failed";
#endif
        return ret;
    }

    option_value = TX_BUFFER_SIZE;
    if (setsockopt(m_sockfd, SOL_SOCKET, SO_SNDBUF, (char*)&option_value, sizeof(option_value)) < 0) {
        ret.m_success = false;
#if defined(FMT_SUPPORT)
        ret.m_msg = fmt::format("Error: setsockopt(SO_SNDBUF) failed: {}", strerror(errno));
#else
        ret.m_msg = "setsockopt(SO_REUSEADDR) failed";
#endif
        return ret;
    }      

    int inetSuccess = inet_aton(remoteIp, &m_server.sin_addr);

    if (inetSuccess == 0) {  // inet_addr failed to parse address
        // if hostname is not in IP strings and dots format, try resolve it
        struct hostent *host = nullptr;
        struct in_addr **addrList = nullptr;
        if ((host = gethostbyname(remoteIp)) == nullptr) {
            close(m_sockfd);
            m_sockfd = 0;
            ret.m_success = false;
#if defined(FMT_SUPPORT)
            ret.m_msg = fmt::format("Failed to resolve hostname {}",remoteIp);
#else
            ret.m_msg = "Failed to resolve hostname";
#endif
            return ret;
        }
        addrList = (struct in_addr **)host->h_addr_list;
        m_server.sin_addr = *addrList[0];
    }
    m_server.sin_family = AF_INET;
    m_server.sin_port = htons(remotePort);

    int connectRet = connect(m_sockfd, (struct sockaddr *)&m_server, sizeof(m_server));
    if (connectRet == -1) {
        close(m_sockfd);
        m_sockfd = 0;
        ret.m_success = false;
#if defined(FMT_SUPPORT)
        ret.m_msg = fmt::format("Error: {}",strerror(errno));
#else
        ret.m_msg = strerror(errno);
#endif                
        return ret;
    }
    m_thread = std::thread(&TcpClient::ReceiveTask, this);
    ret.m_success = true;
    return ret;
}

SocketRet TcpClient::sendMsg(const unsigned char *msg, size_t size) const {
    SocketRet ret;
    ssize_t numBytesSent = send(m_sockfd, (void *)msg, size, 0);
    if (numBytesSent < 0) {  // send failed
        ret.m_success = false;
#if defined(FMT_SUPPORT)        
        ret.m_msg = fmt::format("Error: {}",strerror(errno));
#else
        ret.m_msg = strerror(errno);
#endif                
        return ret;
    }
    if (static_cast<size_t>(numBytesSent) < size) {  // not all bytes were sent
        ret.m_success = false;
        ret.m_msg = fmt::format("Error: Only {} bytes out of {} sent to client",numBytesSent,size);
        return ret;
    }
    ret.m_success = true;
    return ret;
}

void TcpClient::publishServerMsg(const unsigned char *msg, size_t msgSize) {
    if (m_callback != nullptr) {
        m_callback->onReceiveData(msg, msgSize);
    }
}

void TcpClient::publishDisconnected(const SocketRet &ret) {
    if (m_callback != nullptr) {
        m_callback->onDisconnect(ret);
    }
}

void TcpClient::ReceiveTask() {
    constexpr int64_t USEC_DELAY = 500000;
    while (!m_stop) {
        fd_set fds;
        struct timeval tv { 0, USEC_DELAY };
        FD_ZERO(&fds);
        FD_SET(m_sockfd, &fds);
        int selectRet = select(m_sockfd + 1, &fds, nullptr, nullptr, &tv);
        if (selectRet <= 0) {  // select failed or timeout
            if (m_stop) {
                break;
            }
        } else if (FD_ISSET(m_sockfd, &fds)) {
            unsigned char msg[MAX_PACKET_SIZE];
            ssize_t numOfBytesReceived = recv(m_sockfd, msg, MAX_PACKET_SIZE, 0);
            if (numOfBytesReceived < 1) {
                SocketRet ret;
                ret.m_success = false;
                m_stop = true;
                if (numOfBytesReceived == 0) {  // server closed connection
#if defined(FMT_SUPPORT)                
                    ret.m_msg = fmt::format("Server closed connection");
#else
                    ret.m_msg = "Server closed connection";
#endif                                        
                } else {
#if defined(FMT_SUPPORT)                    
                    ret.m_msg = fmt::format("Error: {}",strerror(errno));
#else
                    ret.m_msg = strerror(errno);
#endif                                        
                }
                publishDisconnected(ret);
                break;
            } 
            publishServerMsg(msg, static_cast<size_t>(numOfBytesReceived));
        }
    }
}

SocketRet TcpClient::finish() {
    if (m_thread.joinable()) {
        m_stop = true;
        m_thread.join();
    }
    SocketRet ret;
    if (close(m_sockfd) == -1) {  // close failed
        ret.m_success = false;
        ret.m_msg = strerror(errno);
        return ret;
    }
    ret.m_success = true;
    return ret;
}

}  // Namespace sockets