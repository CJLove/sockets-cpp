#pragma once
#include "SocketCommon.h"
#include "SocketCore.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <sys/types.h>
#include <thread>
#include <unordered_map>
#if defined(FMT_SUPPORT)
#include <fmt/core.h>
#endif
namespace sockets {

constexpr size_t MAX_PACKET_SIZE = 65536;

constexpr uint32_t MSG_SIZE = 100;

/**
 * @brief ClientHandle is an identifier which refers to a TCP client connection
 *      established with this server.
 *
 */
using ClientHandle = int32_t;

/**
 * @brief The TcpServer class encapsulates a TCP server supporting one or more TCP client connections
 *
 */
template <class CallbackImpl, class SocketImpl = sockets::SocketCore>
class TcpServer {
public:
    /**
     * @brief Construct a new TCP Server object
     *
     * @param callback - pointer to the callback recipient
     * @param options - optional socket options to specify SO_SNDBUF and SO_RCVBUF
     */
    explicit TcpServer(CallbackImpl &callback, SocketOpt *options = nullptr)
        : m_serverAddress({}), m_clientAddress({}), m_fds({}), m_stop(false), m_callback(callback) {
        if (options != nullptr) {
            m_sockOptions = *options;
        }
    }

    TcpServer(const TcpServer &) = delete;
    TcpServer(TcpServer &&) = delete;

    /**
     * @brief Shutdown and destroy the TCP Server object
     */
    ~TcpServer() {
        finish();
    }

    TcpServer &operator=(const TcpServer &) = delete;
    TcpServer &operator=(TcpServer &&) = delete;

#if defined(TEST_CORE_ACCESS)
    SocketImpl &getCore() {
        return m_socketCore;
    }
#endif

    /**
     * @brief Start the TCP server listening on the specified port number
     *
     * @param port - port to listen on for connections
     * @return SocketRet - indicator of whether the server was started successfully
     */
    SocketRet start(uint16_t port) {
        SocketRet ret;

        int result = m_socketCore.Initialize();
        if (result != 0) {
#if defined(FMT_SUPPORT)
            ret.m_msg = fmt::format("Error: Socket initialization failed: {}", result);
#else
            std::array<char,MSG_SIZE> msg;
            (void)snprintf(msg.data(),msg.size(),"Error: Socket initialization failed: %d",result);
            ret.m_msg = msg.data();
#endif
            ret.m_success = false;
            return ret;
        }

        m_sockfd = m_socketCore.Socket(AF_INET, SOCK_STREAM, 0);
        if (m_sockfd == INVALID_SOCKET) {  // socket failed
            ret.m_success = false;
#if defined(FMT_SUPPORT)
            ret.m_msg = fmt::format("Error: Socket creation failed errno{}", errno);
#else
            std::array<char,MSG_SIZE> msg;
            (void)snprintf(msg.data(),msg.size(),"Error: Socket creation failed: %d",errno);
            ret.m_msg = msg.data();
#endif
            return ret;
        }
        // set socket for reuse (otherwise might have to wait 4 minutes every time socket is closed)
        int option = 1;
        if (m_socketCore.SetSockOpt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0) {
            ret.m_success = false;
#if defined(FMT_SUPPORT)
            ret.m_msg = fmt::format("Error: SetSockOpt(SO_REUSEADDR) failed: errno {}", errno);
#else
            std::array<char,MSG_SIZE> msg;
            (void)snprintf(msg.data(),msg.size(),"Error: SetSockOpt(SO_REUSEADDR) failed: %d",errno);
            ret.m_msg = msg.data();
#endif
            return ret;            
        }

        // set TX and RX buffer sizes
        if (m_socketCore.SetSockOpt(m_sockfd, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<char *>(&m_sockOptions.m_rxBufSize),
                sizeof(m_sockOptions.m_rxBufSize)) < 0) {
            ret.m_success = false;
#if defined(FMT_SUPPORT)
            ret.m_msg = fmt::format("Error: SetSockOpt(SO_RCVBUF) failed: errno {}", errno);
#else
            std::array<char,MSG_SIZE> msg;
            (void)snprintf(msg.data(),msg.size(),"Error: SetSockOpt(SO_RCVBUF) failed: %d",errno);
            ret.m_msg = msg.data();
#endif
            return ret;
        }

        if (m_socketCore.SetSockOpt(m_sockfd, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<char *>(&m_sockOptions.m_txBufSize),
                sizeof(m_sockOptions.m_txBufSize)) < 0) {
            ret.m_success = false;
#if defined(FMT_SUPPORT)
            ret.m_msg = fmt::format("Error: SetSockOpt(SO_SNDBUF) failed: errno {}", errno);
#else
            std::array<char,MSG_SIZE> msg;
            (void)snprintf(msg.data(),msg.size(),"Error: SetSockOpt(SO_SNDBUF) failed: %d",errno);
            ret.m_msg = msg.data();
#endif
            return ret;
        }

        memset(&m_serverAddress, 0, sizeof(m_serverAddress));
        m_serverAddress.sin_family = AF_INET;
        inet_pton(AF_INET, m_sockOptions.m_listenAddr.c_str(),&m_serverAddress.sin_addr.s_addr);
//        m_serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
        m_serverAddress.sin_port = htons(port);

        int bindSuccess =
            m_socketCore.Bind(m_sockfd, reinterpret_cast<struct sockaddr *>(&m_serverAddress), sizeof(m_serverAddress));
        if (bindSuccess == -1) {  // bind failed
            ret.m_success = false;
#if defined(FMT_SUPPORT)
            ret.m_msg = fmt::format("Error: errno {}", errno);
#else
            std::array<char,MSG_SIZE> msg;
            (void)snprintf(msg.data(),msg.size(),"BInd error: %d",errno);
            ret.m_msg = msg.data();
#endif
            return ret;
        }
        const int clientsQueueSize = 5;
        int listenSuccess = m_socketCore.Listen(m_sockfd, clientsQueueSize);
        if (listenSuccess == -1) {  // listen failed
            ret.m_success = false;
#if defined(FMT_SUPPORT)
            ret.m_msg = fmt::format("Error: listen() failed errno {}", errno);
#else
            std::array<char,MSG_SIZE> msg;
            (void)snprintf(msg.data(),msg.size(),"Error: listen() failed: %d",errno);
            ret.m_msg = msg.data();
#endif
            return ret;
        }
        ret.m_success = true;

        // Add the accept socket to m_fds
        FD_ZERO(&m_fds);
        FD_SET(m_sockfd, &m_fds);

        m_thread = std::thread(&TcpServer::serverTask, this);

        return ret;
    }

    /**
     * @brief Remove a TCP client connection
     *
     * @param handle - handle of the TCP client to be dropped
     * @return true
     * @return false
     */
    bool deleteClient(ClientHandle &handle) {
        std::lock_guard<std::mutex> guard(m_mutex);
        if (m_clients.count(handle) > 0) {
            Client &client = m_clients[handle];

            // Close socket connection and remove from m_fds
            m_socketCore.Close(client.m_sockfd);
            FD_CLR(client.m_sockfd, &m_fds);

            m_clients.erase(handle);
            return true;
        }
        return false;
    }

    /**
     * @brief Send a broadcast message to all connected TCP clients
     *
     * @param msg - pointer to the message data
     * @param size - length of the message data
     * @return SocketRet - indication that the message was sent to all clients
     */
    SocketRet sendBcast(const char *msg, size_t size) {
        SocketRet ret;
        ret.m_success = true;
        std::lock_guard<std::mutex> guard(m_mutex);
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

    /**
     * @brief Send a message to a specific connected client
     *
     * @param client - handle of the TCP client
     * @param msg - pointer to the message data
     * @param size - length of the message data
     * @return SocketRet - indication that the message was sent to the client
     */
    SocketRet sendClientMessage(ClientHandle &clientId, const char *msg, size_t size) {
        SocketRet ret;
        Client *client = nullptr;
        {
            std::lock_guard<std::mutex> guard(m_mutex);
            if (m_clients.count(clientId) > 0) {
                client = &m_clients[clientId];
            }
        }
        if (client != nullptr) {
            ret = client->sendMsg(msg, size);
            return ret;
        }
#if defined(FMT_SUPPORT)
        ret.m_msg = fmt::format("Error: Client {} not found", clientId);
#else
        std::array<char,MSG_SIZE> errMsg;
        (void)snprintf(errMsg.data(),errMsg.size(),"Error: Client %d not found",clientId);
        ret.m_msg = errMsg.data();
#endif
        ret.m_success = false;
        return ret;
    }

    /**
     * @brief Shut down the TCP server
     */
    void finish() {
        m_stop = true;
        if (m_thread.joinable()) {
            m_stop = true;
            try {
                m_thread.join();
            }
            catch (...) {              
            }
        }

        // Close client sockets
        std::lock_guard<std::mutex> guard(m_mutex);
        for (auto &client : m_clients) {
            m_socketCore.Close(client.second.m_sockfd);
        }

        // Close accept socket
        if (m_sockfd != INVALID_SOCKET) {
            m_socketCore.Close(m_sockfd);
        }
        m_sockfd = INVALID_SOCKET;
        m_clients.clear();
    }

    /**
     * @brief Get current info for a client
     *
     * @param clientId - handle to this client connection
     * @param ipAddr - Client's IP address
     * @param port - Client's port number
     * @param connected - indicates client is connected
     * @return true - clientId is valid and information was returned
     * @return false - clientId is invalid
     */
    bool getClientInfo(ClientHandle clientId, std::string &ipAddr, uint16_t &port, bool &connected) {
        std::lock_guard<std::mutex> guard(m_mutex);
        if (m_clients.count(clientId) > 0) {
            Client &client = m_clients[clientId];
            ipAddr = client.m_ip;
            port = client.m_port;
            connected = client.m_isConnected;
            return true;
        }
        return false;
    }

private:
    /**
     * @brief Client represents a connection to a TCP client
     */
    struct Client {
        SocketImpl *m_socketCore;

        /**
         * @brief The TCP client's IP address
         */
        std::string m_ip;

        /**
         * @brief The socket file descriptor for the TCP client connection
         */
        SOCKET m_sockfd = INVALID_SOCKET;

        /**
         * @brief The peer's port
         */
        uint16_t m_port = 0;

        /**
         * @brief Indicator whether TCP client is connected
         */
        bool m_isConnected = false;

        /**
         * @brief Construct a new Client object
         *
         * @param ipAddr - client's IP address
         * @param clientFd - file descriptor for the client connection
         * @param port - client's port number
         */
        Client(SocketImpl *socketImpl, const char *ipAddr, SOCKET clientFd, uint16_t port)
            : m_socketCore(socketImpl), m_ip(ipAddr), m_sockfd(clientFd), m_port(port), m_isConnected(true) {
        }

        /**
         * @brief Construct a new Client object
         */
        Client() : m_socketCore(nullptr), m_sockfd(INVALID_SOCKET) {
        }

        /**
         * @brief Send a message to this TCP client
         *
         * @param msg - pointer to the message data
         * @param size - length of the message data
         * @return SocketRet - indication of whether the message was sent successfully
         */
        SocketRet sendMsg(const char *msg, size_t size) {
            SocketRet ret;
            if (m_sockfd != INVALID_SOCKET) {
                ssize_t numBytesSent = m_socketCore->Send(m_sockfd, reinterpret_cast<const void *>(msg), size, 0);
                if (numBytesSent < 0) {  // send failed
                    ret.m_success = false;
#if defined(FMT_SUPPORT)
                    ret.m_msg = fmt::format("Error: send() failed errno {}", errno);
#else
                    std::array<char,MSG_SIZE> msg;
                    (void)snprintf(msg.data(),msg.size(),"Error: send() failed: %d",errno);
                    ret.m_msg = msg.data();
#endif
                    return ret;
                }
                if (static_cast<size_t>(numBytesSent) < size) {  // not all bytes were sent
                    ret.m_success = false;
#if defined(FMT_SUPPORT)
                    ret.m_msg = fmt::format("Only {} bytes out of {} was sent to client", numBytesSent, size);
#else
                    std::array<char,MSG_SIZE> msg;
                    (void)snprintf(msg.data(), msg.size(), "Only %ld bytes out of %lu was sent to client", numBytesSent, size);
                    ret.m_msg = msg.data();
#endif
                    return ret;
                }
            }
            ret.m_success = true;
            return ret;
        }
    };

    /**
     * @brief Publish data received from a TCP client
     *
     * @param client - handle of the TCP client which sent the data
     * @param msg - pointer to the message data
     * @param msgSize - length of the message data
     */
    void publishClientMsg(const ClientHandle &client, const char *msg, size_t msgSize) {
        m_callback.onReceiveClientData(client, msg, msgSize);
    }

    /**
     * @brief Publish notification that a TCP client has disconnected
     *
     * @param client - handle of the TCP client which has disconnected
     */
    void publishDisconnected(const ClientHandle &client) {
        SocketRet ret;
#if defined(FMT_SUPPORT)
        ret.m_msg = fmt::format("Client {} disconnected", client);
#else
        std::array<char,MSG_SIZE> msg;
        (void)snprintf(msg.data(),msg.size(),"Client %d disconnected",client);
        ret.m_msg = msg.data();
#endif
        m_callback.onClientDisconnect(client, ret);
    }

    /**
     * @brief Publish notification of a new TCP client connection
     *
     * @param client - handle of the new TCP client
     */
    void publishClientConnect(const ClientHandle &client) {
        m_callback.onClientConnect(client);
    }

    /**
     * @brief Find the maximum file descriptor among listen socket and all client sockets for
     *          use by select()
     * @return int
     */
    int findMaxFd() {
        int maxfd = m_sockfd;
        for (const auto &client : m_clients) {
            maxfd = std::max<SOCKET>(maxfd, client.second.m_sockfd);
        }
        return maxfd + 1;
    }

    /**
     * @brief Thread handling all accept requests and reception of data from connected clients
     */
    void serverTask() {
        constexpr int64_t USEC_DELAY = 500000;
        std::array<char, MAX_PACKET_SIZE> msg;

        while (!m_stop.load()) {
            struct timeval delay {
                0, USEC_DELAY
            };
            fd_set read_set = m_fds;
            int maxfds = findMaxFd();
            int selectRet = m_socketCore.Select(maxfds, &read_set, nullptr, nullptr, &delay);
            if (selectRet <= 0) {
                // select() failed or timed out, so retry after a shutdown check
                continue;
            }
            for (int fd = 0; fd < maxfds; fd++) {
                if (FD_ISSET(fd, &read_set)) {
                    Client *client = nullptr;
                    {
                        std::lock_guard<std::mutex> guard(m_mutex);
                        if (m_clients.count(fd) > 0) {
                            client = &m_clients[fd];
                        }
                    }
                    if (client != nullptr) {
                        // data on client socket
                        ssize_t numOfBytesReceived = m_socketCore.Recv(fd, msg.data(), MAX_PACKET_SIZE, 0);
                        if (numOfBytesReceived < 1) {
                            client->m_isConnected = false;
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
                        int clientfd = m_socketCore.Accept(fd, reinterpret_cast<struct sockaddr *>(&m_clientAddress), &sosize);
                        if (clientfd == -1) {
                            // accept() failed
                        } else {
                            std::array<char, INET_ADDRSTRLEN> addr;
                            inet_ntop(AF_INET, &m_clientAddress.sin_addr, addr.data(), INET_ADDRSTRLEN);
                            {
                                std::lock_guard<std::mutex> guard(m_mutex);
                                m_clients.emplace(clientfd,
                                    Client(&m_socketCore, addr.data(), clientfd,
                                        static_cast<uint16_t>(ntohs(m_clientAddress.sin_port))));
                            }
                            FD_SET(clientfd, &m_fds);
                            publishClientConnect(clientfd);
                        }
                    }
                }
            }
        }
    }

    /**
     * @brief The socket file descriptor used for accepting connections
     */
    SOCKET m_sockfd = INVALID_SOCKET;

    /**
     * @brief The server socket address
     */
    struct sockaddr_in m_serverAddress;

    /**
     * @brief The client socket address when a connection is accepted
     */
    struct sockaddr_in m_clientAddress;

    /**
     * @brief The set of file descriptor(s) for accepting connections and receiving data
     */
    fd_set m_fds = {};

    /**
     * @brief Flag to stop the server thread
     */
    std::atomic_bool m_stop;

    /**
     * @brief The collection of connected TCP clients
     */
    std::unordered_map<ClientHandle, Client> m_clients;

    /**
     * @brief Mutex protecting m_clients
     */
    std::mutex m_mutex;

    /**
     * @brief The registered callback recipient
     */
    CallbackImpl &m_callback;

    /**
     * @brief Server thread
     */
    std::thread m_thread;

    /**
     * @brief Socket options for SO_SNDBUF and SO_RCVBUF
     */
    SocketOpt m_sockOptions;

    /**
     * @brief Interface for socket calls
     */
    SocketImpl m_socketCore;
};

}  // Namespace sockets