#pragma once
#include <memory>
#include <string>

namespace sockets {

/**
 * @brief Status structure returned by socket class methods.
 *
 */
struct SocketRet {
    /**
     * @brief Indication of whether the operation succeeded or failed
     */
    bool m_success = false;

    /**
     * @brief Error message text
     */
    std::string m_msg;
};

/**
 * @brief ClientHandle is an identifier which refers to a TCP client connection
 *      established with this server.
 *
 */
using ClientHandle = int32_t;

/**
 * @brief Interface class for receiving data or disconnection notifications from
 * UdpSocket and TcpClient socket classes
 *
 */
class ISocket {
public:
    /**
     * @brief Receive data from a TCP client or UDP socket connection
     *
     * @param data - pointer to received data
     * @param size - length of received data
     */
    virtual void onReceiveData(const unsigned char *data, size_t size) = 0;

    /**
     * @brief Receive notification that UDP or TCP server connection has disconnected
     *
     * @param ret - Error information
     */
    virtual void onDisconnect(const SocketRet &ret) = 0;
};

class IServerSocket {
public:
    /**
     * @brief Receive notification of a new client connection
     *
     * @param client - Handle of the new TCP client
     */
    virtual void onClientConnect(const ClientHandle &client) = 0;

    /**
     * @brief Receive notification that a TCP client connection has disconnected
     *
     * @param client - Handle of the TCP client whose connection dropped
     * @param ret - Error information
     */
    virtual void onClientDisconnect(const ClientHandle &client, const SocketRet &ret) = 0;

    /**
     * @brief Receive data from a TCP server connection
     *
     * @param client - Handle of the TCP client which sent the data
     * @param data - pointer to received data
     * @param size - length of received data
     */
    virtual void onReceiveClientData(const ClientHandle &client, const unsigned char *data, size_t size) = 0;
};

}  // Namespace sockets