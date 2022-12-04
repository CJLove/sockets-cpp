#pragma once

#include "SocketCommon.h"
#include <arpa/inet.h>
#include <cerrno>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace sockets {

/**
 * @brief Interface class for receiving data or disconnection notifications from
 * TcpClient socket classes
 *
 */
class IClientSocket {
public:
    /**
     * @brief Receive data from a TCP client or UDP socket connection
     *
     * @param data - pointer to received data
     * @param size - length of received data
     */
    virtual void onReceiveData(const unsigned char *data, size_t size) = 0;

    /**
     * @brief Receive notification that TCP server connection has disconnected
     *
     * @param ret - Error information
     */
    virtual void onDisconnect(const SocketRet &ret) = 0;
};

/**
 * @brief TcpClient encapsulates a TCP client socket connection to a server
 *
 */
class TcpClient {
public:
    /**
     * @brief Construct a new Tcp Client object
     *
     * @param callback - pointer to the callback object which will handle notifications
     * @param options - optional socket options to configure SO_SNDBUF and SO_RCVBUF
     */
    TcpClient(IClientSocket *callback, SocketOpt *options = nullptr);

    TcpClient(const TcpClient &) = delete;
    TcpClient(TcpClient &&) = delete;

    /**
     * @brief Destroy the Tcp Client object
     */
    ~TcpClient();

    TcpClient& operator=(const TcpClient&) = delete;
    TcpClient& operator=(TcpClient&&) = delete;

    /**
     * @brief Establish the TCP client connection
     *
     * @param remoteIp - remote IP address to connect to
     * @param remotePort - remote port number to connect to
     * @return SocketRet - indication of whether the client connection was established
     */
    SocketRet connectTo(const char *remoteIp, uint16_t remotePort);

    /**
     * @brief Send a message to the TCP server
     *
     * @param msg - pointer to the message data
     * @param size - length of the message data
     * @return SocketRet - indication of whether the message was sent successfully
     */
    SocketRet sendMsg(const unsigned char *msg, size_t size) const;

    /**
     * @brief Shut down the TCP client
     *
     * @return SocketRet - indication of whether the client was shut down successfully
     */
    SocketRet finish();

private:
    /**
     * @brief Publish message received from TCP server
     *
     * @param msg - pointer to the message data
     * @param msgSize - length of the message data
     */
    void publishServerMsg(const unsigned char *msg, size_t msgSize);

    /**
     * @brief Publish notification of disconnection
     *
     * @param ret - error information
     */
    void publishDisconnected(const SocketRet &ret);

    /**
     * @brief Thread which receives data from the TCP server
     */
    void ReceiveTask();

    /**
     * @brief The socket file descriptor
     */
    int m_sockfd = -1;

    /**
     * @brief Indicator that the receive thread should stop
     */
    bool m_stop = false;

    /**
     * @brief Socket address of the server
     */
    struct sockaddr_in m_server;

    /**
     * @brief Receive thread
     */
    std::thread m_thread;

    /**
     * @brief Pointer to the registered callback receipient
     */
    IClientSocket *m_callback = nullptr;

    /**
     * @brief Socket options for SO_SNDBUF and SO_RCVBUF
     */
    SocketOpt m_sockOptions;
};

}  // Namespace sockets