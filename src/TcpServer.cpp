#include "TcpServer.h"

#define MAX_PACKET_SIZE 65536

namespace sockets {

Client::~Client() {
    if (m_thread != nullptr) {
        if (m_thread->joinable()) {
            m_isConnected = false;
            m_thread->join();
        }
        delete m_thread;
        m_thread = nullptr;
    }
}

bool Client::operator==(const Client &other) {
    if ((this->m_sockfd == other.m_sockfd) && (this->m_ip == other.m_ip)) {
        return true;
    }
    return false;
}

SocketRet Client::sendMsg(const unsigned char *msg, size_t size) {
    SocketRet ret;
    if (m_sockfd) {
        ssize_t numBytesSent = send(m_sockfd, (char *)msg, size, 0);
        if (numBytesSent < 0) {  // send failed
            ret.m_success = false;
            ret.m_msg = strerror(errno);
            return ret;
        }
        if (static_cast<size_t>(numBytesSent) < size) {  // not all bytes were sent
            ret.m_success = false;
            char msg[100];
            sprintf(msg, "Only %ld bytes out of %lu was sent to client", numBytesSent, size);
            ret.m_msg = msg;
            return ret;
        }
    }
    ret.m_success = true;
    return ret;
}

TcpServer::TcpServer(ISocket *callback) : m_callback(callback) {
}

TcpServer::~TcpServer() {
    finish();
}

void TcpServer::receiveTask() {
    // Note: new client has just been added to the end of m_clients at the time
    // this new thread has been started
    Client *client = &m_clients.back();

    while (client->isConnected()) {
        fd_set fds;
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 500000;
        FD_ZERO(&fds);
        FD_SET(client->getFileDescriptor(), &fds);
        int selectRet = select(client->getFileDescriptor() + 1, &fds, NULL, NULL, &tv);
        if (selectRet == -1) {  // select failed
            if (!client->isConnected()) {
                break;
            }
        } else if (selectRet == 0) {  // timeout
            if (!client->isConnected()) {
                break;
            }
        } else if (FD_ISSET(client->getFileDescriptor(), &fds)) {
            unsigned char msg[MAX_PACKET_SIZE];
            ssize_t numOfBytesReceived = recv(client->getFileDescriptor(), msg, MAX_PACKET_SIZE, 0);
            if (numOfBytesReceived < 1) {
                client->setDisconnected();
                if (numOfBytesReceived == 0) {  // client closed connection
                    client->setErrorMessage("Client closed connection");
                } else {
                    client->setErrorMessage(strerror(errno));
                }
                close(client->getFileDescriptor());
                publishDisconnected(*client);
                deleteClient(*client);
                break;
            } else {
                publishClientMsg(*client, msg, numOfBytesReceived);
            }
        }
    }
}

bool TcpServer::deleteClient(Client &client) {
    size_t clientIndex = 0;
    bool found = false;
    for (size_t i = 0; i < m_clients.size(); i++) {
        if (m_clients[i] == client) {
            clientIndex = i;
            found = true;
            break;
        }
    }
    if (found) {
        m_clients.erase(m_clients.begin() + clientIndex);
        return true;
    }
    return false;
}

void TcpServer::publishClientMsg(const Client &client, const unsigned char *msg, size_t msgSize) {
    if (m_callback) {
        m_callback->onReceiveClientData(client, msg, msgSize);
    }
}

void TcpServer::publishDisconnected(const Client &client) {
    if (m_callback) {
        SocketRet ret;
        ret.m_msg = client.getInfoMessage();

        m_callback->onClientDisconnect(client, ret);
    }
}

SocketRet TcpServer::start(uint16_t port) {
    m_sockfd = 0;
    m_clients.reserve(10);
    SocketRet ret;

    m_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_sockfd == -1) {  // socket failed
        ret.m_success = false;
        ret.m_msg = strerror(errno);
        return ret;
    }
    // set socket for reuse (otherwise might have to wait 4 minutes every time socket is closed)
    int option = 1;
    setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    memset(&m_serverAddress, 0, sizeof(m_serverAddress));
    m_serverAddress.sin_family = AF_INET;
    m_serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    m_serverAddress.sin_port = htons(port);

    int bindSuccess = bind(m_sockfd, (struct sockaddr *)&m_serverAddress, sizeof(m_serverAddress));
    if (bindSuccess == -1) {  // bind failed
        ret.m_success = false;
        ret.m_msg = strerror(errno);
        return ret;
    }
    const int clientsQueueSize = 5;
    int listenSuccess = listen(m_sockfd, clientsQueueSize);
    if (listenSuccess == -1) {  // listen failed
        ret.m_success = false;
        ret.m_msg = strerror(errno);
        return ret;
    }
    ret.m_success = true;
    return ret;
}

Client TcpServer::acceptClient(uint timeout) {
    socklen_t sosize = sizeof(m_clientAddress);
    Client newClient;

    if (timeout > 0) {
        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        FD_ZERO(&m_fds);
        FD_SET(m_sockfd, &m_fds);
        int selectRet = select(m_sockfd + 1, &m_fds, NULL, NULL, &tv);
        if (selectRet == -1) {  // select failed
            newClient.setErrorMessage(strerror(errno));
            return newClient;
        } else if (selectRet == 0) {  // timeout
            newClient.setErrorMessage("Timeout waiting for client");
            return newClient;
        } else if (!FD_ISSET(m_sockfd, &m_fds)) {  // no new client
            newClient.setErrorMessage("File descriptor is not set");
            return newClient;
        }
    }

    int file_descriptor = accept(m_sockfd, (struct sockaddr *)&m_clientAddress, &sosize);
    if (file_descriptor == -1) {  // accept failed
        newClient.setErrorMessage(strerror(errno));
        return newClient;
    }

    newClient.setFileDescriptor(file_descriptor);
    newClient.setConnected();
    newClient.setIp(inet_ntoa(m_clientAddress.sin_addr));
    m_clients.push_back(newClient);
    m_clients.back().setThreadHandler(std::bind(&TcpServer::receiveTask, this));

    return newClient;
}

SocketRet TcpServer::sendBcast(const unsigned char *msg, size_t size) {
    SocketRet ret;
    ret.m_success = true;
    for (auto &client : m_clients) {
        auto clientRet = client.sendMsg(msg, size);
        ret.m_success &= clientRet.m_success;
        if (!clientRet.m_success) {
            ret.m_msg = clientRet.m_msg;
            break;
        }
    }
    return ret;
}

SocketRet TcpServer::finish() {
    SocketRet ret;
    for (uint i = 0; i < m_clients.size(); i++) {
        m_clients[i].setDisconnected();
        m_clients[i].joinThreadHandler();
        if (close(m_clients[i].getFileDescriptor()) == -1) {  // close failed
            ret.m_success = false;
            ret.m_msg = strerror(errno);
            return ret;
        }
    }
    if (close(m_sockfd) == -1) {  // close failed
        ret.m_success = false;
        ret.m_msg = strerror(errno);
        return ret;
    }
    m_clients.clear();
    ret.m_success = true;
    return ret;
}

}  // Namespace sockets
