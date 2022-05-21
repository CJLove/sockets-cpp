#pragma once
#include "ISocket.h"
#include <arpa/inet.h>
#include <cstring>
#include <cerrno>
#include <functional>
#include <iostream>
#include <netinet/in.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>

namespace sockets {

using sockets::IServerSocket;
class TcpServer;

/**
 * @brief The TcpServer class encapsulates a TCP server supporting one or more TCP client connections
 *
 */
class TcpServer {
public:
    /**
     * @brief Construct a new TCP Server object
     *
     * @param callback - pointer to the callback recipient
     */
    explicit TcpServer(IServerSocket *callback);

    TcpServer(const TcpServer &) = delete;
    TcpServer(TcpServer &&) = delete;

    /**
     * @brief Shutdown and destroy the TCP Server object
     */
    ~TcpServer();

    TcpServer &operator=(const TcpServer &) = delete;
    TcpServer &operator=(TcpServer &&) = delete;

    /**
     * @brief Start the TCP server listening on the specified port number
     *
     * @param port - port to listen on for connections
     * @return SocketRet - indicator of whether the server was started successfully
     */
    SocketRet start(uint16_t port);

    /**
     * @brief Remove a TCP client connection
     *
     * @param handle - handle of the TCP client to be dropped
     * @return true
     * @return false
     */
    bool deleteClient(ClientHandle &handle);

    /**
     * @brief Send a broadcast message to all connected TCP clients
     *
     * @param msg - pointer to the message data
     * @param size - length of the message data
     * @return SocketRet - indication that the message was sent to all clients
     */
    SocketRet sendBcast(const unsigned char *msg, size_t size);

    /**
     * @brief Send a message to a specific connected client
     *
     * @param client - handle of the TCP client
     * @param msg - pointer to the message data
     * @param size - length of the message data
     * @return SocketRet - indication that the message was sent to the client
     */
    SocketRet sendClientMessage(ClientHandle &client, const unsigned char *msg, size_t size);

    /**
     * @brief Shut down the TCP server
     *
     * @return SocketRet - indication that the server was stopped successfully
     */
    SocketRet finish();

    /**
     * @brief Get the IP address for a specific client
     *
     * @param clientId - handle of the TCP client
     * @return std::string - IP address
     */
    std::string getIp(ClientHandle clientId) const;

    /**
     * @brief Get the port number for a specific client
     *
     * @param clientId - handle of the TCP client
     * @return uint16_t - port number
     */
    uint16_t getPort(ClientHandle clientId) const;

    /**
     * @brief Return whether a specific client is connected or not
     *
     * @param clientId - handle of the TCP client
     * @return true - client is connected
     * @return false - client is not connected
     */
    bool isConnected(ClientHandle clientId) const;

private:
    /**
     * @brief Client represents a connection to a TCP client
     */
    struct Client {

        /**
         * @brief The TCP client's IP address
         */
        std::string m_ip;

        /**
         * @brief The socket file descriptor for the TCP client connection
         */
        int m_sockfd = -1;

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
        Client(const char *ipAddr, int clientFd, uint16_t port);

        /**
         * @brief Construct a new Client object
         */
        Client() = default;

        /**
         * @brief Send a message to this TCP client
         *
         * @param msg - pointer to the message data
         * @param size - length of the message data
         * @return SocketRet - indication of whether the message was sent successfully
         */
        SocketRet sendMsg(const unsigned char *msg, size_t size) const;
    };

    /**
     * @brief Publish data received from a TCP client
     *
     * @param client - handle of the TCP client which sent the data
     * @param msg - pointer to the message data
     * @param msgSize - length of the message data
     */
    void publishClientMsg(const ClientHandle &client, const unsigned char *msg, size_t msgSize);

    /**
     * @brief Publish notification that a TCP client has disconnected
     *
     * @param client - handle of the TCP client which has disconnected
     */
    void publishDisconnected(const ClientHandle &client);

    /**
     * @brief Publish notification of a new TCP client connection
     *
     * @param client - handle of the new TCP client
     */
    void publishClientConnect(const ClientHandle &client);

    /**
     * @brief Find the maximum file descriptor among listen socket and all client sockets for
     *          use by select()
     * @return int
     */
    int findMaxFd();

    /**
     * @brief Thread handling all accept requests and reception of data from connected clients
     */
    void serverTask();

    /**
     * @brief The socket file descriptor used for accepting connections
     */
    int m_sockfd = -1;

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
    bool m_stop = false;

    /**
     * @brief The collection of connected TCP clients
     */
    std::unordered_map<ClientHandle, Client> m_clients;

    /**
     * @brief The registered callback recipient
     */
    IServerSocket *m_callback;

    /**
     * @brief Server thread
     */
    std::thread m_thread;

    const int TX_BUFFER_SIZE = 10240;
    const int RX_BUFFER_SIZE = 10240;    
};

}  // Namespace sockets