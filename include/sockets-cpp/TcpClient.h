#pragma once

#include "ISocket.h"
#include <arpa/inet.h>
#include <errno.h>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace sockets {

using sockets::ISocket;

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
     */
    TcpClient(ISocket *callback);

    /**
     * @brief Destroy the Tcp Client object
     */
    ~TcpClient();

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
    SocketRet sendMsg(const unsigned char *msg, size_t size);

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
    ISocket *m_callback = nullptr;
};

}  // Namespace sockets