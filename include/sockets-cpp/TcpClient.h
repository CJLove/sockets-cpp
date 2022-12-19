#pragma once

#include "AddrLookup.h"
#include "SocketCommon.h"
#include "SocketCore.h"
#include <array>
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sys/types.h>
#include <thread>
#include <vector>
#if defined(FMT_SUPPORT)
#include <fmt/core.h>
#endif

namespace sockets {

constexpr size_t MAX_PACKET_SIZE = 65536;

constexpr uint32_t MSG_SIZE = 100;

/**
 * @brief TcpClient encapsulates a TCP client socket connection to a server
 *
 */
template <class CallbackImpl, class SocketImpl = sockets::SocketCore>
class TcpClient {
public:
    /**
     * @brief Construct a new Tcp Client object
     *
     * @param callback - reference to the callback object which will handle notifications
     * @param options - optional socket options to configure SO_SNDBUF and SO_RCVBUF
     */
    explicit TcpClient(CallbackImpl &callback, SocketOpt *options = nullptr)
        : m_stop(false), m_callback(callback), m_addrLookup(m_socketCore) {
        if (options != nullptr) {
            m_sockOptions = *options;
        }
    }

    TcpClient(const TcpClient &) = delete;
    TcpClient(TcpClient &&) = delete;

    /**
     * @brief Destroy the Tcp Client object
     */
    ~TcpClient() {
        finish();
    }

    TcpClient &operator=(const TcpClient &) = delete;
    TcpClient &operator=(TcpClient &&) = delete;

#if defined(TEST_CORE_ACCESS)
    SocketImpl &getCore() {
        return m_socketCore;
    }
#endif

    /**
     * @brief Establish the TCP client connection
     *
     * @param remoteIp - remote IP address to connect to
     * @param remotePort - remote port number to connect to
     * @return SocketRet - indication of whether the client connection was established
     */
    SocketRet connectTo(const char *remoteIp, uint16_t remotePort) {
        m_sockfd = INVALID_SOCKET;
        SocketRet ret;

        int result = m_socketCore.Initialize();
        if (result != 0) {
#if defined(FMT_SUPPORT)
            ret.m_msg = fmt::format("Error: Socket initializatio failed: {}", result);
#else
            std::array<char,MSG_SIZE> msg;
            (void)snprintf(msg.data(),msg.size(),"Error: Socket initialization failed: %d",result);
            ret.m_msg = msg.data();
#endif
            ret.m_success = false;
            return ret;
        }

        m_sockfd = m_socketCore.Socket(AF_INET, SOCK_STREAM, 0);
        if (m_sockfd == INVALID_SOCKET) {  // socket failed
            ret.m_success = false;
#if defined(FMT_SUPPORT)
            ret.m_msg = fmt::format("Error: Failed to create socket: errno {}", errno);
#else
            std::array<char,MSG_SIZE> msg;
            (void)snprintf(msg.data(),msg.size(),"Error: Failed to create socket: %d",errno);
            ret.m_msg = msg.data();
#endif
            return ret;
        }

        // set TX and RX buffer sizes
        if (m_socketCore.SetSockOpt(m_sockfd, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<char *>(&m_sockOptions.m_rxBufSize),
                sizeof(m_sockOptions.m_rxBufSize)) < 0) {
            ret.m_success = false;
#if defined(FMT_SUPPORT)
            ret.m_msg = fmt::format("Error: setsockopt(SO_RCVBUF) failed: errno {}", errno);
#else
            std::array<char,MSG_SIZE> msg;
            (void)snprintf(msg.data(),msg.size(),"Error: setsockopt(SO_REUSEADDR) failed: %d", errno);
            ret.m_msg = msg.data();
#endif
            return ret;
        }

        if (m_socketCore.SetSockOpt(m_sockfd, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<char *>(&m_sockOptions.m_txBufSize),
                sizeof(m_sockOptions.m_txBufSize)) < 0) {
            ret.m_success = false;
#if defined(FMT_SUPPORT)
            ret.m_msg = fmt::format("Error: setsockopt(SO_SNDBUF) failed: errno {}", errno);
#else
            std::array<char,MSG_SIZE> msg;
            (void)snprintf(msg.data(),msg.size(),"Error: setsockopt(SO_SNDBUF) failed: %d", errno);
            ret.m_msg = msg.data();
#endif
            return ret;
        }

        if (m_addrLookup.lookupHost(remoteIp, m_server.sin_addr.s_addr) != 0) {
#if defined(FMT_SUPPORT)
            ret.m_msg = fmt::format("Failed to resolve hostname {}", remoteIp);
#else
            std::array<char,MSG_SIZE> msg;
            (void)snprintf(msg.data(),msg.size(),"Failed to resolve hostname %s",remoteIp);
            ret.m_msg = msg.data();
#endif
            return ret;
            //        }
        }
        m_server.sin_family = AF_INET;
        m_server.sin_port = htons(remotePort);

        int connectRet = m_socketCore.Connect(m_sockfd, reinterpret_cast<struct sockaddr *>(&m_server), sizeof(m_server));
        if (connectRet == -1) {
            ret.m_success = false;
#if defined(FMT_SUPPORT)
            ret.m_msg = fmt::format("Error: connect() failed errno {}", errno);
#else
            std::array<char,MSG_SIZE> msg;
            (void)snprintf(msg.data(),msg.size(),"Error: connect() failed errno %d",errno);
            ret.m_msg = msg.data();
#endif
            return ret;
        }
        m_thread = std::thread(&TcpClient::ReceiveTask, this);
        ret.m_success = true;
        return ret;
    }

    /**
     * @brief Send a message to the TCP server
     *
     * @param msg - pointer to the message data
     * @param size - length of the message data
     * @return SocketRet - indication of whether the message was sent successfully
     */
    SocketRet sendMsg(const char *msg, size_t size) {
        SocketRet ret;
        ssize_t numBytesSent = m_socketCore.Send(m_sockfd, reinterpret_cast<const void *>(msg), size, 0);
        if (numBytesSent < 0) {  // send failed
            ret.m_success = false;
#if defined(FMT_SUPPORT)
            ret.m_msg = fmt::format("Error: send() failed errno {}", errno);
#else
            std::array<char,MSG_SIZE> msg;
            (void)snprintf(msg.data(),msg.size(),"Error: send() failed: %d",errno);
            ret.m_msg = msg.data();
#endif
            return ret;
        }
        if (static_cast<size_t>(numBytesSent) < size) {  // not all bytes were sent
            ret.m_success = false;
#if defined(FMT_SUPPORT)
            ret.m_msg = fmt::format("Error: Only {} bytes out of {} sent to client", numBytesSent, size);
#else
            std::array<char,MSG_SIZE> msg;
            (void)snprintf(msg.data(), msg.size(), "Only %ld bytes out of %lu was sent to client", numBytesSent, size);
            ret.m_msg = msg.data();
#endif
            return ret;
        }
        ret.m_success = true;
        return ret;
    }

    /**
     * @brief Shut down the TCP client
     */
    void finish() {
        m_stop.store(true);
        if (m_thread.joinable()) {
            try {
                m_thread.join();
            } catch (...) { }
        }
        if (m_sockfd != INVALID_SOCKET) {
            m_socketCore.Close(m_sockfd);
        }
        m_sockfd = INVALID_SOCKET;
    }

private:
    /**
     * @brief Publish message received from TCP server
     *
     * @param msg - pointer to the message data
     * @param msgSize - length of the message data
     */
    void publishServerMsg(const char *msg, size_t msgSize) {
        m_callback.onReceiveData(msg, msgSize);
    }

    /**
     * @brief Publish notification of disconnection
     *
     * @param ret - error information
     */
    void publishDisconnected(const SocketRet &ret) {
        m_callback.onDisconnect(ret);
    }

    /**
     * @brief Thread which receives data from the TCP server
     */
    void ReceiveTask() {
        constexpr int64_t USEC_DELAY = 500000;
        while (!m_stop.load()) {
            fd_set fds;
            struct timeval delay {
                0, USEC_DELAY
            };
            FD_ZERO(&fds);
            FD_SET(m_sockfd, &fds);
            int selectRet = m_socketCore.Select(m_sockfd + 1, &fds, nullptr, nullptr, &delay);
            if (selectRet <= 0) {  // select failed or timeout
                if (m_stop) {
                    break;
                }
            } else if (FD_ISSET(m_sockfd, &fds)) {
                std::array<char, MAX_PACKET_SIZE> msg;
                ssize_t numOfBytesReceived = m_socketCore.Recv(m_sockfd, msg.data(), MAX_PACKET_SIZE, 0);
                if (numOfBytesReceived < 1) {
                    SocketRet ret;
                    ret.m_success = false;
                    m_stop = true;
                    if (numOfBytesReceived == 0) {  // server closed connection
#if defined(FMT_SUPPORT)
                        ret.m_msg = fmt::format("Server closed connection");
#else
                        ret.m_msg = "Server closed connection";
#endif
                    } else {
#if defined(FMT_SUPPORT)
                        ret.m_msg = fmt::format("Error: recv() failed errno {}", errno);
#else
                        std::array<char,MSG_SIZE> errMsg;
                        (void)snprintf(errMsg.data(),errMsg.size(),"Error: recv() failed: %d",errno);
                        ret.m_msg = errMsg.data();
#endif
                    }
                    publishDisconnected(ret);
                    break;
                }
                publishServerMsg(msg.data(), static_cast<size_t>(numOfBytesReceived));
            }
        }
    }

    /**
     * @brief The socket file descriptor
     */
    SOCKET m_sockfd = INVALID_SOCKET;

    /**
     * @brief Indicator that the receive thread should stop
     */
    std::atomic_bool m_stop;

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
    CallbackImpl &m_callback;

    /**
     * @brief Socket options for SO_SNDBUF and SO_RCVBUF
     */
    SocketOpt m_sockOptions;

    /**
     * @brief Interface for socket calls
     */
    SocketImpl m_socketCore;

    /**
     * @brief Helper for hostname resolution
     */
    AddrLookup<SocketImpl> m_addrLookup;
};

}  // Namespace sockets