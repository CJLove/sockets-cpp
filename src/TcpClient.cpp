#include "TcpClient.h"

#define MAX_PACKET_SIZE 4096

namespace sockets {

TcpClient::TcpClient(ISocket *callback): m_callback(callback)
{}

TcpClient::~TcpClient()
{
    terminateReceiveThread();
}

SocketRet TcpClient::connectTo(const char *remoteIp, uint16_t remotePort)
{
    m_sockfd = 0;
    SocketRet ret;

    m_sockfd = socket(AF_INET , SOCK_STREAM , 0);
    if (m_sockfd == -1) { //socket failed
        ret.m_success = false;
        ret.m_msg = strerror(errno);
        return ret;
    }

    int inetSuccess = inet_aton(remoteIp, &m_server.sin_addr);

    if(!inetSuccess) { // inet_addr failed to parse address
        // if hostname is not in IP strings and dots format, try resolve it
        struct hostent *host;
        struct in_addr **addrList;
        if ( (host = gethostbyname( remoteIp ) ) == NULL){
            ret.m_success = false;
            ret.m_msg = "Failed to resolve hostname";
            return ret;
        }
        addrList = (struct in_addr **) host->h_addr_list;
        m_server.sin_addr = *addrList[0];
    }
    m_server.sin_family = AF_INET;
    m_server.sin_port = htons( remotePort );

    int connectRet = connect(m_sockfd , (struct sockaddr *)&m_server , sizeof(m_server));
    if (connectRet == -1) {
        ret.m_success = false;
        ret.m_msg = strerror(errno);
        return ret;
    }
    m_receiveTask = new std::thread(&TcpClient::ReceiveTask, this);
    ret.m_success = true;
    return ret;    
}

SocketRet TcpClient::sendMsg(const unsigned char * msg, size_t size) {
    SocketRet ret;
    int numBytesSent = send(m_sockfd, (void*)msg, size, 0);
    if (numBytesSent < 0 ) { // send failed
        ret.m_success = false;
        ret.m_msg = strerror(errno);
        return ret;
    }
    if ((uint)numBytesSent < size) { // not all bytes were sent
        ret.m_success = false;
        char msg[100];
        sprintf(msg, "Only %d bytes out of %lu was sent to client", numBytesSent, size);
        ret.m_msg = msg;
        return ret;
    }
    ret.m_success = true;
    return ret;
}

void TcpClient::publishServerMsg(const unsigned char * msg, size_t msgSize) {
    if (m_callback) {
        m_callback->onReceiveData(msg, msgSize);
    }
}

void TcpClient::publishDisconnected(const SocketRet & ret) {
    if (m_callback) {
        m_callback->onDisconnect(ret);
    }
}

void TcpClient::ReceiveTask() {

    while(!stop) {
        unsigned char msg[MAX_PACKET_SIZE];
        int numOfBytesReceived = recv(m_sockfd, msg, MAX_PACKET_SIZE, 0);
        if(numOfBytesReceived < 1) {
            SocketRet ret;
            ret.m_success = false;
            stop = true;
            if (numOfBytesReceived == 0) { //server closed connection
                ret.m_msg = "Server closed connection";
            } else {
                ret.m_msg = strerror(errno);
            }
            publishDisconnected(ret);
            finish();
            break;
        } else {
            publishServerMsg(msg, numOfBytesReceived);
        }
    }
}

SocketRet TcpClient::finish(){
    stop = true;
    terminateReceiveThread();
    SocketRet ret;
    if (close(m_sockfd) == -1) { // close failed
        ret.m_success = false;
        ret.m_msg = strerror(errno);
        return ret;
    }
    ret.m_success = true;
    return ret;
}

void TcpClient::terminateReceiveThread() {
    if (m_receiveTask != nullptr) {
        m_receiveTask->detach();
        delete m_receiveTask;
        m_receiveTask = nullptr;
    }
}

}   // Namespace sockets