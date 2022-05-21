#include "TcpServer.h"
#include <algorithm>
#if defined(FMT_SUPPORT)
    #include <fmt/core.h>
#endif    

constexpr size_t MAX_PACKET_SIZE = 65536;

namespace sockets {

TcpServer::Client::Client(const char *ipAddr, int clientFd, uint16_t port)
    : m_ip(ipAddr), m_sockfd(clientFd), m_port(port), m_isConnected(true) {
}

SocketRet TcpServer::Client::sendMsg(const unsigned char *msg, size_t size) const {
    SocketRet ret;
    if (m_sockfd != 0) {
        ssize_t numBytesSent = send(m_sockfd, reinterpret_cast<const void *>(msg), size, 0);
        if (numBytesSent < 0) {  // send failed
            ret.m_success = false;
#if defined(FMT_SUPPORT)            
            ret.m_msg = fmt::format("Error: send() failed errno {}", errno);
#else
            ret.m_msg = "Error: send() failed";
#endif
            return ret;
        }
        if (static_cast<size_t>(numBytesSent) < size) {  // not all bytes were sent
            ret.m_success = false;
#if defined(FMT_SUPPORT)
            ret.m_msg = fmt::format("Only {} bytes out of {} was sent to client",numBytesSent,size);
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

TcpServer::TcpServer(IServerSocket *callback) : m_serverAddress({}), m_clientAddress({}), m_fds({}), m_callback(callback) {
}

TcpServer::~TcpServer() {
    finish();
}

SocketRet TcpServer::start(uint16_t port) {
    m_sockfd = 0;
    SocketRet ret;

    m_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_sockfd == -1) {  // socket failed
        ret.m_success = false;
#if defined(FMT_SUPPORT)
        ret.m_msg = fmt::format("Error: Socket creation failed errno{}",errno);
#else                
        ret.m_msg = "Error: Socket creation failed";
#endif        
        return ret;
    }
    // set socket for reuse (otherwise might have to wait 4 minutes every time socket is closed)
    int option = 1;
    setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    // set TX and RX buffer sizes
    int option_value = RX_BUFFER_SIZE;
    if (setsockopt(m_sockfd, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<char*>(&option_value), sizeof(option_value)) < 0) {
        ret.m_success = false;
#if defined(FMT_SUPPORT)
        ret.m_msg = fmt::format("Error: setsockopt(SO_RCVBUF) failed: errno {}", errno);
#else
        ret.m_msg = "setsockopt(SO_REUSEADDR) failed";
#endif
        return ret;
    }

    option_value = TX_BUFFER_SIZE;
    if (setsockopt(m_sockfd, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<char*>(&option_value), sizeof(option_value)) < 0) {
        ret.m_success = false;
#if defined(FMT_SUPPORT)
        ret.m_msg = fmt::format("Error: setsockopt(SO_SNDBUF) failed: errno {}", errno);
#else
        ret.m_msg = "setsockopt(SO_REUSEADDR) failed";
#endif
        return ret;
    }      

    memset(&m_serverAddress, 0, sizeof(m_serverAddress));
    m_serverAddress.sin_family = AF_INET;
    m_serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    m_serverAddress.sin_port = htons(port);

    int bindSuccess = bind(m_sockfd, reinterpret_cast<struct sockaddr *>(&m_serverAddress), sizeof(m_serverAddress));
    if (bindSuccess == -1) {  // bind failed
        ret.m_success = false;
#if defined(FMT_SUPPORT)
        ret.m_msg = fmt::format("Error: errno {}",errno);
#else                
        ret.m_msg = strerror(errno);
#endif        
        return ret;
    }
    const int clientsQueueSize = 5;
    int listenSuccess = listen(m_sockfd, clientsQueueSize);
    if (listenSuccess == -1) {  // listen failed
        ret.m_success = false;
#if defined(FMT_SUPPORT)
        ret.m_msg = fmt::format("Error: listen() failed errno {}", errno);
#else                
        ret.m_msg = "Error: listen() failed";
#endif        
        return ret;
    }
    ret.m_success = true;

    // Add the accept socket to m_fds
    // std::cout << "accept socket " << m_sockfd << "\n";
    FD_ZERO(&m_fds);
    FD_SET(m_sockfd, &m_fds);

    m_thread = std::thread(&TcpServer::serverTask, this);

    return ret;
}

int TcpServer::findMaxFd() {
    int maxfd = m_sockfd;
    for (const auto &client : m_clients) {
        maxfd = std::max(maxfd, client.second.m_sockfd);
    }
    return maxfd + 1;
}

void TcpServer::serverTask() {
    constexpr int64_t USEC_DELAY = 500000;

    while (!m_stop) {
        struct timeval delay {
            0, USEC_DELAY
        };
        fd_set read_set = m_fds;
        int maxfds = findMaxFd();
        int selectRet = select(maxfds, &read_set, nullptr, nullptr, &delay);
        if (selectRet <= 0) {
            // select() failed or timed out, so retry after a shutdown check
            continue;
        }
        for (int fd = 0; fd < maxfds; fd++) {
            if (FD_ISSET(fd, &read_set)) {
                if (m_clients.count(fd) > 0) {
                    // data on client socket
                    Client &client = m_clients[fd];
                    std::array<unsigned char, MAX_PACKET_SIZE> msg;
                    ssize_t numOfBytesReceived = recv(fd, msg.data(), MAX_PACKET_SIZE, 0);
                    if (numOfBytesReceived < 1) {
                        client.m_isConnected = false;
                        if (numOfBytesReceived == 0) {  // client closed connection
                            deleteClient(fd);
                            publishDisconnected(fd);
                        }
                    } else {
                        publishClientMsg(fd, msg.data(), static_cast<size_t>(numOfBytesReceived));
                    }
                } else {
                    // data on accept socket
                    socklen_t sosize = sizeof(m_clientAddress);
                    int clientfd = accept(fd, reinterpret_cast<struct sockaddr *>(&m_clientAddress), &sosize);
                    if (clientfd == -1) {
                        // accept() failed
                        //std::cout << "accept returned " << strerror(errno) << "\n";
                    } else {
                        std::array<char,INET_ADDRSTRLEN> addr;
                        inet_ntop(AF_INET,&m_clientAddress.sin_addr,addr.data(),INET_ADDRSTRLEN);
                        m_clients.emplace(clientfd,
                            Client(addr.data(), clientfd,
                                static_cast<uint16_t>(ntohs(m_clientAddress.sin_port))));
                        FD_SET(clientfd, &m_fds);
                        publishClientConnect(clientfd);
                    }
                }
            }
        }
    }
}

bool TcpServer::deleteClient(ClientHandle &handle) {
    if (m_clients.count(handle) > 0) {
        Client &client = m_clients[handle];

        // Close socket connection and remove from m_fds
        close(client.m_sockfd);
        FD_CLR(client.m_sockfd, &m_fds);

        m_clients.erase(handle);
        return true;
    }
    return false;
}

void TcpServer::publishClientMsg(const ClientHandle &client, const unsigned char *msg, size_t msgSize) {
    if (m_callback != nullptr) {
        m_callback->onReceiveClientData(client, msg, msgSize);
    }
}

void TcpServer::publishDisconnected(const ClientHandle &client) {
    if (m_callback != nullptr) {
        SocketRet ret;
#if defined(FMT_SUPPORT)
        ret.m_msg = fmt::format("Client {} disconnected",client);
#else                
        ret.m_msg = "Client disconnected.";
#endif
        m_callback->onClientDisconnect(client, ret);
    }
}

void TcpServer::publishClientConnect(const ClientHandle &client) {
    if (m_callback != nullptr) {
        m_callback->onClientConnect(client);
    }
}

SocketRet TcpServer::sendBcast(const unsigned char *msg, size_t size) {
    SocketRet ret;
    ret.m_success = true;
    for (auto &client : m_clients) {
        auto clientRet = client.second.sendMsg(msg, size);
        ret.m_success &= clientRet.m_success;
        if (!clientRet.m_success) {
            ret.m_msg = clientRet.m_msg;
            break;
        }
    }
    return ret;
}

SocketRet TcpServer::sendClientMessage(ClientHandle &clientId, const unsigned char *msg, size_t size) {
    SocketRet ret;
    if (m_clients.count(clientId) > 0) {
        Client &client = m_clients[clientId];
        client.sendMsg(msg, size);
        ret.m_success = true;
        return ret;
    }
#if defined(FMT_SUPPORT)
    ret.m_msg = fmt::format("Client {} not found",clientId);
#else        
    ret.m_msg = "Client not found";
#endif    
    ret.m_success = false;
    return ret;
}

SocketRet TcpServer::finish() {
    SocketRet ret;
    m_stop = true;
    m_thread.join();

    // Close client sockets
    for (auto &client : m_clients) {
        if (close(client.second.m_sockfd) == -1) {
            // close() failed
            ret.m_success = false;
#if defined(FMT_SUPPORT)
            ret.m_msg = fmt::format("Error: close() failed errno {}",errno);
#else                        
            ret.m_msg = "Error: close() failed";
#endif            
            return ret;
        }
    }

    // Close accept socket
    if (close(m_sockfd) == -1) {  // close failed
        ret.m_success = false;
#if defined(FMT_SUPPORT)
        ret.m_msg = fmt::format("Error: close() failed errno {}",errno);
#else                
        ret.m_msg = "Error: close() failed";
#endif        
        return ret;
    }
    m_clients.clear();
    ret.m_success = true;
    return ret;
}

std::string TcpServer::getIp(ClientHandle client) const {
    if (m_clients.count(client) > 0) {
        return m_clients.find(client)->second.m_ip;
    }
    return "Client not found";
}

uint16_t TcpServer::getPort(ClientHandle client) const {
    if (m_clients.count(client) > 0) {
        return m_clients.find(client)->second.m_port;
    }
    return 0;
}

bool TcpServer::isConnected(ClientHandle client) const {
    if (m_clients.count(client) > 0) {
        return m_clients.find(client)->second.m_isConnected;
    }
    return false;
}

}  // Namespace sockets
