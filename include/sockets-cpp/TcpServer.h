#pragma once
#include "ISocket.h"
#include <arpa/inet.h>
#include <cstring>
#include <errno.h>
#include <functional>
#include <iostream>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>
#include <thread>

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
    TcpServer(IServerSocket *callback);

    /**
     * @brief Destroy the TCP Server object
     */
    ~TcpServer();

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
     * @param client - handle of the TCP client to be dropped
     * @return true
     * @return false
     */
    bool deleteClient(ClientHandle &client);

    /**
     * @brief Send a broadcast message to all connected TCP clients
     *
     * @param msg - pointer to the message data
     * @param size - length of the message data
     * @return SocketRet - indication that the message was sent to all clients
     */
    SocketRet sendBcast(const unsigned char *msg, size_t size);

    SocketRet sendClientMessage(ClientHandle &client, const unsigned char *msg, size_t size);

    /**
     * @brief Shut down the TCP server
     *
     * @return SocketRet - indication that the server was stopped successfully
     */
    SocketRet finish();

    std::string getIp(ClientHandle clientId) const;

    uint16_t getPort(ClientHandle clientId) const;

    bool isConnected(ClientHandle clientId) const;

private:
    /**
     * @brief Client represents a connection to a TCP client
     */
    struct Client {
    
        /**
         * @brief The TCP client's IP address
         */
        std::string m_ip = "";

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

        Client(const char *ipAddr, int fd, uint16_t port);

        Client()
        {}

        /**
         * @brief Set the indicator to indicate that this TCP client is connected
         */
        void setConnected() {
            m_isConnected = true;
        }

        /**
         * @brief Set the indicator to indicate that this TCP client is not connected
         */
        void setDisconnected() {
            m_isConnected = false;
        }

        /**
         * @brief Get the indicator of whether the TCP client is connected or not
         *
         * @return true
         * @return false
         */
        bool isConnected() {
            return m_isConnected;
        }

        /**
         * @brief Send a message to this TCP client
         *
         * @param msg - pointer to the message data
         * @param size - length of the message data
         * @return SocketRet - indication of whether the message was sent successfully
         */
        SocketRet sendMsg(const unsigned char *msg, size_t size);
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

    void publishClientConnect(const ClientHandle &client);

    int findMaxFd();

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
    fd_set m_fds;

    /**
     * @brief Flag to stop the server thread
     * 
     */
    bool m_stop = false;

    /**
     * @brief The collection of connected TCP clients
     */
    std::unordered_map<ClientHandle,Client> m_clients;

    /**
     * @brief The registered callback recipient
     */
    IServerSocket *m_callback;

    std::thread m_thread;
};

}  // Namespace sockets