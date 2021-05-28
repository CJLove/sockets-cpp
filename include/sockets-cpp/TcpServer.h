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
#include <thread>
#include <unistd.h>
#include <vector>
#include <unordered_map>

namespace sockets {

using sockets::ISocket;
class TcpServer;

class ClientRef {
public:
    ClientRef(TcpServer &server, uint32_t id);

    ~ClientRef() = default;

    std::string getIp() const
    {
        return m_server.getIp(m_clientId);
    }

    bool isConnected() const
    {
        return m_server.isConnected(m_clientId);
    }

    std::string getInfoMsg() const
    {
        return m_server.getInfoMsg(m_clientId);
    }

private:
    TcpServer &m_server;
    uint32_t m_clientId;
};

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
    TcpServer(ISocket *callback);

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
     * @brief Accept a client connection
     *
     * @param timeout - flag indicating to time out after 1 second
     * @return ClientRef - handle to the TCP client if a connection was accepted
     */
    ClientRef acceptClient(uint32_t timeout);

    /**
     * @brief Remove a TCP client connection
     *
     * @param client - the client to be dropped
     * @return true
     * @return false
     */
    bool deleteClient(ClientRef &client);

    /**
     * @brief Send a broadcast message to all connected TCP clients
     *
     * @param msg - pointer to the message data
     * @param size - length of the message data
     * @return SocketRet - indication that the message was sent to all clients
     */
    SocketRet sendBcast(const unsigned char *msg, size_t size);

    /**
     * @brief Shut down the TCP server
     *
     * @return SocketRet - indication that the server was stopped successfully
     */
    SocketRet finish();

    std::string getIp(const uint32_t clientId) const;

    bool isConnected(const uint32_t clientId) const;

    std::string getInfoMsg(const uint32_t clientId) const;

private:
    /**
     * @brief Client represents a connection to a TCP client
     */
    class Client {
    public:
        Client(int fd, const char *ipAddr);

        /**
         * @brief Destroy the Client object
         *
         */
        ~Client();

        /**
         * @brief Return two Client objects are equal
         *
         * @param other - the other Client object
         * @return true
         * @return false
         */
        bool operator==(const Client &other);

        /**
         * @brief Set the File Descriptor object
         *
         * @param sockfd - the file descriptor for this socket connection
         *
         */
        void setFileDescriptor(int sockfd) {
            m_sockfd = sockfd;
        }

        /**
         * @brief Get the File Descriptor object
         *
         * @return int - the file descriptor
         */
        int getFileDescriptor() const {
            return m_sockfd;
        }

        /**
         * @brief Set the IP address of the TCP client
         *
         * @param ip - the client IP address
         */
        void setIp(const std::string &ip) {
            m_ip = ip;
        }

        /**
         * @brief Get the IP address of the TCP client
         *
         * @return std::string
         */
        std::string getIp() const {
            return m_ip;
        }

        /**
         * @brief Set the error message associated with the TCP client connection
         *
         * @param msg
         */
        void setErrorMessage(const std::string &msg) {
            m_errorMsg = msg;
        }

        /**
         * @brief Get the Error/info message associated with the TCP client connection
         *
         * @return std::string - the error message
         */
        std::string getInfoMessage() const {
            return m_errorMsg;
        }

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
         * @brief Set/start the thread handling data received from the TCP client
         *
         * @param func - function pointer
         */
        void setThreadHandler(std::function<void(void)> func) {
            m_thread = std::thread(func);
        }

        void joinThreadHandler() {
            m_thread.join();
        }

        /**
         * @brief Send a message to this TCP client
         *
         * @param msg - pointer to the message data
         * @param size - length of the message data
         * @return SocketRet - indication of whether the message was sent successfully
         */
        SocketRet sendMsg(const unsigned char *msg, size_t size);
        
    private:
        /**
         * @brief The socket file descriptor for the TCP client connection
         */
        int m_sockfd = -1;

        /**
         * @brief The TCP client's IP address
         */
        std::string m_ip = "";

        /**
         * @brief The error/information message associated with this TCP client
         */
        std::string m_errorMsg = "";

        /**
         * @brief Indicator whether TCP client is connected
         */
        bool m_isConnected = false;

        /**
         * @brief The handle to the receive thread for this TCP client connection
         *
         */
        std::thread m_thread;
    };

    /**
     * @brief Publish data received from a TCP client
     *
     * @param client - handle of the TCP client which sent the data
     * @param msg - pointer to the message data
     * @param msgSize - length of the message data
     */
    void publishClientMsg(const ClientRef &client, const unsigned char *msg, size_t msgSize);

    /**
     * @brief Publish notification that a TCP client has disconnected
     *
     * @param client - handle of the TCP client which has disconnected
     */
    void publishDisconnected(const ClientRef &client);

    /**
     * @brief The method used to receive data from a TCP client.  This method executes in a separate
     *          thread for each TCP client connection
     */
    void receiveTask();

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
     * @brief The set of file descriptor(s) for accepting connections
     */
    fd_set m_fds;

    /**
     * @brief The collection of connected TCP clients
     */
    //std::vector<Client> m_clients;
    std::unordered_map<uint32_t,Client> m_clients;

    /**
     * @brief The registered callback recipient
     */
    ISocket *m_callback;
};

}  // Namespace sockets