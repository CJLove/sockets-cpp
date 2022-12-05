#pragma once
#include "SocketCommon.h"
#include <cstdint>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <thread>

namespace sockets {

/**
 * @brief Interface class for receiving data or disconnection notifications from
 * UdpSocket
 *
 */
class IUdpSocket {
public:
    /**
     * @brief Receive data from a TCP client or UDP socket connection
     *
     * @param data - pointer to received data
     * @param size - length of received data
     */
    virtual void onReceiveData(const char *data, size_t size) = 0;

};


/**
 * @brief The UdpSocket class represents a UDP unicast or multicast socket connection
 *
 */
class UdpSocket {
public:
    /**
     * @brief Construct a new UDP Socket object
     *
     * @param callback - the callback recipient
     * @param options - optional socket options
     */
    UdpSocket(IUdpSocket *callback, SocketOpt *options = nullptr);

    UdpSocket(const UdpSocket &) = delete;
    UdpSocket(UdpSocket &&) = delete;

    /**
     * @brief Destroy the UDP Socket object
     *
     */
    ~UdpSocket();

    UdpSocket &operator=(const UdpSocket &) = delete;
    UdpSocket &operator=(UdpSocket &&) = delete;

    /**
     * @brief Start a UDP multicast socket by binding to the server address and joining the
     *          multicast group.
     *
     * @param mcastAddr - multicast group address to join
     * @param port - port number to listen/connect to
     * @return SocketRet - indication that multicast setup was successful
     */
    SocketRet startMcast(const char *mcastAddr, uint16_t port);

    /**
     * @brief Start a UDP unicast socket by binding to the server address and storing the
     *          IP address and port number for the peer.
     *
     * @param remoteAddr - remote IP address
     * @param localPort - local port to listen on
     * @param port - remote port to connect to when sending messages
     * @return SocketRet - Indication that unicast setup was successful
     */
    SocketRet startUnicast(const char *remoteAddr, uint16_t localPort, uint16_t port);

    /**
     * @brief Start a UDP unicast socket by binding to the server address
     * 
     * @param localPort - local port to listen on
     * @return SocketRet - Indication that unicast setup was successful
     */
    SocketRet startUnicast(uint16_t localPort);
    
    /**
     * @brief Send a message over UDP
     *
     * @param msg - pointer to the message data
     * @param size - length of the message data
     * @return SocketRet - indication that the message was sent successfully
     */
    SocketRet sendMsg(const char *msg, size_t size);

    /**
     * @brief Shutdown the UDP socket
     *
     * @return SocketRet - indication that the UDP socket was shut down successfully
     */
    SocketRet finish();

private:
    /**
     * @brief Publish a UDP message received from a peer
     *
     * @param msg - pointer to the message data
     * @param msgSize - length of the message data
     */
    void publishUdpMsg(const char *msg, size_t msgSize);

    /**
     * @brief The receive thread for receiving data from UDP peer(s).
     */
    void ReceiveTask();

    /**
     * @brief The remote or multicast socket address
     */
    struct sockaddr_in m_sockaddr;

    /**
     * @brief The socket file descriptor
     */
    int m_fd = -1;

    /**
     * @brief Indicator that the receive thread should exit
     */
    bool m_stop = false;

    /**
     * @brief Pointer to the callback recipient
     */
    IUdpSocket *m_callback;

    /**
     * @brief Handle of the receive thread
     */
    std::thread m_thread;

    /**
     * @brief Socket options for SO_SNDBUF and SO_RCVBUF
     */
    SocketOpt m_sockOptions;

};

}  // Namespace sockets