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

class TcpClient {
public:
    TcpClient(ISocket *callback);

    ~TcpClient();

    SocketRet connectTo(const char *remoteIp, uint16_t remotePort);

    SocketRet sendMsg(const unsigned char *msg, size_t size);

    SocketRet finish();

private:
    void publishServerMsg(const unsigned char *msg, size_t msgSize);
    void publishDisconnected(const SocketRet &ret);
    void ReceiveTask();
    void terminateReceiveThread();

    int m_sockfd = 0;
    bool stop = false;
    struct sockaddr_in m_server;
    std::thread *m_receiveTask = nullptr;
    ISocket *m_callback = nullptr;
};

}  // Namespace sockets