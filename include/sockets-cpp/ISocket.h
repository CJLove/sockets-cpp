#pragma once
#include <memory>
#include <string>

namespace sockets {

class Client;

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
 * @brief Interface class for receiving data or disconnection notifications from
 * socket classes
 * 
 */
class ISocket {
public:
    /**
     * @brief Destroy the ISocket-derived object
     */
    virtual ~ISocket() = default;

    /**
     * @brief Receive data from a TCP client or UDP socket connection
     * 
     * @param data - pointer to received data
     * @param size - length of received data
     */
    virtual void onReceiveData(const unsigned char *data, size_t size);

    /**
     * @brief Receive data from a TCP server connection
     * 
     * @param client - TCP client which sent the data
     * @param data - pointer to received data
     * @param size - length of received data
     */
    virtual void onReceiveClientData(const Client &client, const unsigned char *data, size_t size);

    /**
     * @brief Receive notification that UDP or TCP server connection has disconnected
     * 
     * @param ret - Error information
     */
    virtual void onDisconnect(const SocketRet &ret);

    /**
     * @brief Receive notification that a TCP client connection has disconnected
     * 
     * @param client - TCP client whose connection dropped
     * @param ret - Error information
     */
    virtual void onClientDisconnect(const Client &client, const SocketRet &ret);
};

}  // Namespace sockets