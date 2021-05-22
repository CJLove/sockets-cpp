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

namespace sockets {

using sockets::ISocket;

class Client {
public:
    ~Client();
    bool operator==(const Client &other);

    void setFileDescriptor(int sockfd) {
        m_sockfd = sockfd;
    }
    int getFileDescriptor() const {
        return m_sockfd;
    }

    void setIp(const std::string &ip) {
        m_ip = ip;
    }
    std::string getIp() const {
        return m_ip;
    }

    void setErrorMessage(const std::string &msg) {
        m_errorMsg = msg;
    }

    std::string getInfoMessage() const {
        return m_errorMsg;
    }

    void setConnected() {
        m_isConnected = true;
    }

    void setDisconnected() {
        m_isConnected = false;
    }
    bool isConnected() {
        return m_isConnected;
    }

    void setThreadHandler(std::function<void(void)> func) {
        m_threadHandler = new std::thread(func);
    }

    SocketRet sendMsg(const unsigned char *msg, size_t size);

private:
    int m_sockfd = 0;
    std::string m_ip = "";
    std::string m_errorMsg = "";
    bool m_isConnected = false;
    std::thread *m_threadHandler = nullptr;
};

class TcpServer {
public:
    TcpServer(ISocket *callback);

    ~TcpServer();

    SocketRet start(uint16_t port);

    Client acceptClient(uint32_t timeout);

    bool deleteClient(Client &client);

    SocketRet sendBcast(const unsigned char*msg, size_t size);

    SocketRet finish();

private:
    void publishClientMsg(const Client &client, const unsigned char *msg, size_t msgSize);
    void publishDisconnected(const Client &client);
    void receiveTask();

    int m_sockfd;
    struct sockaddr_in m_serverAddress;
    struct sockaddr_in m_clientAddress;
    fd_set m_fds;
    std::vector<Client> m_clients;
    std::thread *threadHandle;
    ISocket *m_callback;
};

}  // Namespace sockets