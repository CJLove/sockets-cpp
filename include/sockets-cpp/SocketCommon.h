#pragma once
#include <memory>
#include <string>

namespace sockets {
    /**
     * @brief Default values for SO_SNDBUF and SO_RCVBUF
     * 
     */
    constexpr int TX_BUFFER_SIZE = 10240;
    constexpr int RX_BUFFER_SIZE = 10240;

/**
 * @brief Status structure returned by socket class methods.
 *
 */
struct SocketRet {
    /**
     * @brief Error message text
     */
    std::string m_msg;

    /**
     * @brief Indication of whether the operation succeeded or failed
     */
    bool m_success = false;
};


/**
 * @brief Socket options to be used.
 * 
 */
struct SocketOpt {


    /**
     * @brief Value passed to setsockopt(SO_SNDBUF)
     * 
     */
    int m_txBufSize = TX_BUFFER_SIZE;

    /**
     * @brief Value passed to setsockopt(SO_RCVBUF)
     * 
     */
    int m_rxBufSize = RX_BUFFER_SIZE;

    /**
     * @brief Socket listen address
     * 
     */
    std::string m_listenAddr = "0.0.0.0";

};

}  // Namespace sockets