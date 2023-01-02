#pragma once
#include "AddrLookup.h"
#include "SocketCommon.h"
#include "SocketCore.h"
#include <array>
#include <atomic>
#include <cstdint>
#include <string>
#include <thread>
#if defined(FMT_SUPPORT)
#include <fmt/core.h>
#endif

namespace sockets {
/**
 * @brief Max UDP datagram size for UDP protocol
 *
 */
constexpr size_t MAX_PACKET_SIZE = 65507;

constexpr uint32_t MSG_SIZE = 100;

/**
 * @brief The UdpSocket class represents a UDP unicast or multicast socket connection
 *
 */
template <class CallbackImpl, class SocketImpl = sockets::SocketCore>
class UdpSocket {
public:
    /**
     * @brief Construct a new UDP Socket object
     *
     * @param callback - the callback recipient
     * @param options - optional socket options
     */
    explicit UdpSocket(CallbackImpl &callback, SocketOpt *options = nullptr)
        : m_sockaddr({}), m_stop(false), m_callback(callback), m_addrLookup(m_socketCore) {
        if (options != nullptr) {
            m_sockOptions = *options;
        }
    }

    UdpSocket(const UdpSocket &) = delete;
    UdpSocket(UdpSocket &&) = delete;

    /**
     * @brief Destroy the UDP Socket object
     *
     */
    ~UdpSocket() {
        finish();
    }

    UdpSocket &operator=(const UdpSocket &) = delete;
    UdpSocket &operator=(UdpSocket &&) = delete;

#if defined(TEST_CORE_ACCESS)
    SocketImpl &getCore() {
        return m_socketCore;
    }
#endif

    /**
     * @brief Start a UDP multicast socket by binding to the server address and joining the
     *          multicast group.
     *
     * @param mcastAddr - multicast group address to join
     * @param port - port number to listen/connect to
     * @return SocketRet - indication that multicast setup was successful
     */
    SocketRet startMcast(const char *mcastAddr, uint16_t port) {
        SocketRet ret;

        int result = m_socketCore.Initialize();
        if (result != 0) {
#if defined(FMT_SUPPORT)
            ret.m_msg = fmt::format("Error: Socket initialization failed: {}", result);
#else
            std::array<char,MSG_SIZE> msg;
            (void)snprintf(msg.data(),msg.size(),"Error: Socket initialization failed: %d",result);
            ret.m_msg = msg.data();
#endif
            ret.m_success = false;
            return ret;
        }

        m_fd = m_socketCore.Socket(AF_INET, SOCK_DGRAM, 0);
        if (m_fd == INVALID_SOCKET) {
            ret.m_success = false;
#if defined(FMT_SUPPORT)
            ret.m_msg = fmt::format("Error: socket() failed: errno {}", errno);
#else
            std::array<char,MSG_SIZE> msg;
            (void)snprintf(msg.data(),msg.size(),"Error: socket() failed: %d",errno);
            ret.m_msg = msg.data();
#endif
            return ret;
        }
        // Allow multiple sockets to use the same port
        unsigned yes = 1;
        if (m_socketCore.SetSockOpt(m_fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char *>(&yes), sizeof(yes)) < 0) {
            ret.m_success = false;
#if defined(FMT_SUPPORT)
            ret.m_msg = fmt::format("Error: setsockopt(SO_REUSEADDR) failed: errno {}", errno);
#else
            std::array<char,MSG_SIZE> msg;
            (void)snprintf(msg.data(),msg.size(),"Error: setsockopt(SO_REULSEADDR) failed: %d",errno);
            ret.m_msg = msg.data();
#endif
            return ret;
        }
        // Set TX and RX buffer sizes
        if (m_socketCore.SetSockOpt(m_fd, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<char *>(&m_sockOptions.m_rxBufSize),
                sizeof(m_sockOptions.m_rxBufSize)) < 0) {
            ret.m_success = false;
#if defined(FMT_SUPPORT)
            ret.m_msg = fmt::format("Error: setsockopt(SO_RCVBUF) failed: errno {}", errno);
#else
            std::array<char,MSG_SIZE> msg;
            (void)snprintf(msg.data(),msg.size(),"Error: setsockopt(SO_RCVBUF) failed: %d",errno);
            ret.m_msg = msg.data();
#endif
            return ret;
        }

        if (m_socketCore.SetSockOpt(m_fd, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<char *>(&m_sockOptions.m_txBufSize),
                sizeof(m_sockOptions.m_txBufSize)) < 0) {
            ret.m_success = false;
#if defined(FMT_SUPPORT)
            ret.m_msg = fmt::format("Error: setsockopt(SO_SNDBUF) failed: errno {}", errno);
#else
            std::array<char,MSG_SIZE> msg;
            (void)snprintf(msg.data(),msg.size(),"Error: setsockopt(SO_SNDBUF) failed: %d",errno);
            ret.m_msg = msg.data();
#endif
            return ret;
        }

        sockaddr_in localAddr = {};
        localAddr.sin_family = AF_INET;
        localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        localAddr.sin_port = htons(port);

        if (m_socketCore.Bind(m_fd, reinterpret_cast<struct sockaddr *>(&localAddr), sizeof(localAddr)) < 0) {
            ret.m_success = false;
#if defined(FMT_SUPPORT)
            ret.m_msg = fmt::format("Error: bind() failed: errno {}", errno);
#else
            std::array<char,MSG_SIZE> msg;
            (void)snprintf(msg.data(),msg.size(),"Error: bind() failed: %d",errno);
            ret.m_msg = msg.data();
#endif
            return ret;
        }

        // store the multicast group address for use by send()
        memset(&m_sockaddr, 0, sizeof(sockaddr));
        m_sockaddr.sin_family = AF_INET;
        inet_pton(AF_INET,mcastAddr,&m_sockaddr.sin_addr);
        //m_sockaddr.sin_addr.s_addr = inet_addr(mcastAddr);
        m_sockaddr.sin_port = htons(port);

        // use setsockopt() to request that the kernel join a multicast group
        //
        struct ip_mreq mreq { };
        inet_pton(AF_INET,mcastAddr,&mreq.imr_multiaddr);
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        if (m_socketCore.SetSockOpt(m_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, reinterpret_cast<char *>(&mreq), sizeof(mreq)) < 0) {
            ret.m_success = false;
#if defined(FMT_SUPPORT)
            ret.m_msg = fmt::format("Error: setsockopt(IP_ADD_MEMBERSHIP) failed: errno {}", errno);
#else
            std::array<char,MSG_SIZE> msg;
            (void)snprintf(msg.data(),msg.size(),"Error: setsockopt(IP_ADD_MEMBERSHIP) failed: %d",errno);
            ret.m_msg = msg.data();
#endif
            return ret;
        }

        m_thread = std::thread(&UdpSocket::ReceiveTask, this);
        ret.m_success = true;
        return ret;
    }

    /**
     * @brief Start a UDP unicast socket by binding to the server address and storing the
     *          IP address and port number for the peer.
     *
     * @param remoteAddr - remote IP address
     * @param localPort - local port to listen on
     * @param port - remote port to connect to when sending messages
     * @return SocketRet - Indication that unicast setup was successful
     */
    SocketRet startUnicast(const char *remoteAddr, uint16_t localPort, uint16_t port) {
        SocketRet ret;

        int result = m_socketCore.Initialize();
        if (result != 0) {
#if defined(FMT_SUPPORT)
            ret.m_msg = fmt::format("Error: Socket initialization failed: {}", result);
#else
            std::array<char,MSG_SIZE> msg;
            (void)snprintf(msg.data(),msg.size(),"Error: Socket initialization failed: %d",result);
            ret.m_msg = msg.data();
#endif
            ret.m_success = false;
            return ret;
        }

        // store the remoteaddress for use by sendto()
        memset(&m_sockaddr, 0, sizeof(sockaddr));
        m_sockaddr.sin_family = AF_INET;
        if (m_addrLookup.lookupHost(remoteAddr, m_sockaddr.sin_addr.s_addr) != 0) {
#if defined(FMT_SUPPORT)
            ret.m_msg = fmt::format("Failed to resolve hostname {}", remoteAddr);
#else
            std::array<char,MSG_SIZE> msg;
            (void)snprintf(msg.data(),msg.size(),"Failed to resolve hostname %s",remoteAddr);
            ret.m_msg = msg.data();
#endif
            return ret;
        }

        m_sockaddr.sin_port = htons(port);

        // return the result of setting up the local server
        return startUnicast(localPort);
    }

    /**
     * @brief Start a UDP unicast socket by binding to the server address
     *
     * @param localPort - local port to listen on
     * @return SocketRet - Indication that unicast setup was successful
     */
    SocketRet startUnicast(uint16_t localPort) {
        SocketRet ret;
        int result = m_socketCore.Initialize();
        if (result != 0) {
#if defined(FMT_SUPPORT)
            ret.m_msg = fmt::format("Error: Socket initialization failed: {}", result);
#else
            std::array<char,MSG_SIZE> msg;
            (void)snprintf(msg.data(),msg.size(),"Error: Socket initialization failed: %d",result);
            ret.m_msg = msg.data();
#endif
            ret.m_success = false;
            return ret;
        }

        m_fd = m_socketCore.Socket(AF_INET, SOCK_DGRAM, 0);
        if (m_fd == INVALID_SOCKET) {
            ret.m_success = false;
#if defined(FMT_SUPPORT)
            ret.m_msg = fmt::format("Error: socket() failed: errno {}", errno);
#else
            ret.m_msg = "socket() failed";
#endif
            return ret;
        }
        // Allow multiple sockets to use the same port
        unsigned yes = 1;
        if (m_socketCore.SetSockOpt(m_fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char *>(&yes), sizeof(yes)) < 0) {
            ret.m_success = false;
#if defined(FMT_SUPPORT)
            ret.m_msg = fmt::format("Error: setsockopt(SO_REUSEADDR) failed: errno {}", errno);
#else
            std::array<char,MSG_SIZE> msg;
            (void)snprintf(msg.data(),msg.size(),"Error: setsockopt(SO_REUSEADDR) failed: errno %d", errno);
            ret.m_msg = msg.data();
#endif
            return ret;
        }

        // Set TX and RX buffer sizes
        if (m_socketCore.SetSockOpt(m_fd, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<char *>(&m_sockOptions.m_rxBufSize),
                sizeof(m_sockOptions.m_rxBufSize)) < 0) {
            ret.m_success = false;
#if defined(FMT_SUPPORT)
            ret.m_msg = fmt::format("Error: setsockopt(SO_RCVBUF) failed: errno {}", errno);
#else
            std::array<char,MSG_SIZE> msg;
            (void)snprintf(msg.data(),msg.size(),"Error: setsockopt(SO_RCVBUF) failed: errno %d", errno);
            ret.m_msg = msg.data();
#endif
            return ret;
        }

        if (m_socketCore.SetSockOpt(m_fd, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<char *>(&m_sockOptions.m_txBufSize),
                sizeof(m_sockOptions.m_txBufSize)) < 0) {
            ret.m_success = false;
#if defined(FMT_SUPPORT)
            ret.m_msg = fmt::format("Error: setsockopt(SO_SNDBUF) failed: errno {}", errno);
#else
            std::array<char,MSG_SIZE> msg;
            (void)snprintf(msg.data(),msg.size(),"Error: setsockopt(SO_SNDBUF) failed: errno %d", errno);
            ret.m_msg = msg.data();
#endif
            return ret;
        }

        sockaddr_in localAddr {};
        localAddr.sin_family = AF_INET;
        inet_pton(AF_INET,m_sockOptions.m_listenAddr.c_str(),&localAddr.sin_addr.s_addr);
        localAddr.sin_port = htons(localPort);

        if (m_socketCore.Bind(m_fd, reinterpret_cast<struct sockaddr *>(&localAddr), sizeof(localAddr)) < 0) {
            ret.m_success = false;
#if defined(FMT_SUPPORT)
            ret.m_msg = fmt::format("Error: bind() failed: errno {}", errno);
#else
            std::array<char,MSG_SIZE> msg;
            (void)snprintf(msg.data(),msg.size(),"Error: bind() failed: %d", errno);
            ret.m_msg = msg.data();
#endif
            return ret;
        }

        m_thread = std::thread(&UdpSocket::ReceiveTask, this);
        ret.m_success = true;
        return ret;
    }

    /**
     * @brief Send a message over UDP
     *
     * @param msg - pointer to the message data
     * @param size - length of the message data
     * @return SocketRet - indication that the message was sent successfully
     */
    SocketRet sendMsg(const char *msg, size_t size) {
        SocketRet ret;
        // If destination addr/port specified
        if (m_sockaddr.sin_port != 0) {
            ssize_t numBytesSent = m_socketCore.SendTo(
                m_fd, &msg[0], size, 0, reinterpret_cast<struct sockaddr *>(&m_sockaddr), sizeof(m_sockaddr));
            if (numBytesSent < 0) {  // send failed
                ret.m_success = false;
#if defined(FMT_SUPPORT)
                ret.m_msg = fmt::format("Error: sendto() failed: {}", errno);
#else
                std::array<char,MSG_SIZE> msg;
               (void)snprintf(msg.data(),msg.size(),"Error: sendto() failed: %d",errno);
                ret.m_msg = msg.data();
#endif
                return ret;
            }
            if (static_cast<size_t>(numBytesSent) < size) {  // not all bytes were sent
                ret.m_success = false;
#if defined(FMT_SUPPORT)
                ret.m_msg = fmt::format("Only {} bytes of {} was sent to client", numBytesSent, size);
#else
                std::array<char,MSG_SIZE> msg;
                (void)snprintf(msg.data(), msg.size(), "Only %ld bytes out of %lu was sent to client", numBytesSent, size);
                ret.m_msg = msg.data();
#endif
                return ret;
            }
        }
        ret.m_success = true;
        return ret;
    }

    /**
     * @brief Shutdown the UDP socket
     */
    void finish() {
        m_stop.store(true);
        if (m_thread.joinable()) {
            try {
                m_thread.join();
            }
            catch (...) {
            }
        }
        if (m_fd != INVALID_SOCKET) {
            m_socketCore.Close(m_fd);
        }
        m_fd = INVALID_SOCKET;
    }

private:
    /**
     * @brief Publish a UDP message received from a peer
     *
     * @param msg - pointer to the message data
     * @param msgSize - length of the message data
     */
    void publishUdpMsg(const char *msg, size_t msgSize) {
        m_callback.onReceiveData(msg, msgSize);
    }

    /**
     * @brief The receive thread for receiving data from UDP peer(s).
     */
    void ReceiveTask() {
        constexpr int64_t USEC_DELAY = 500000;
        while (!m_stop.load()) {
            if (m_fd != INVALID_SOCKET) {
                fd_set fds;
                struct timeval delay {
                    0, USEC_DELAY
                };
                FD_ZERO(&fds);
                FD_SET(m_fd, &fds);
                int selectRet = m_socketCore.Select(m_fd + 1, &fds, nullptr, nullptr, &delay);
                if (selectRet <= 0) {  // select failed or timeout
                    if (m_stop) {
                        break;
                    }
                } else if (FD_ISSET(m_fd, &fds)) {

                    std::array<char, MAX_PACKET_SIZE> msg;
                    ssize_t numOfBytesReceived = m_socketCore.Recv(m_fd, msg.data(), MAX_PACKET_SIZE, 0);
                    // Note: recv() returning 0 can happen for zero-length datagrams
                    if (numOfBytesReceived >= 0) {
                        publishUdpMsg(msg.data(), static_cast<size_t>(numOfBytesReceived));
                    }
                }
            }
        }
    }

    /**
     * @brief The remote or multicast socket address
     */
    struct sockaddr_in m_sockaddr;

    /**
     * @brief The socket file descriptor
     */
    SOCKET m_fd = INVALID_SOCKET;

    /**
     * @brief Indicator that the receive thread should exit
     */
    std::atomic_bool m_stop;

    /**
     * @brief Pointer to the callback recipient
     */
    CallbackImpl &m_callback;

    /**
     * @brief Handle of the receive thread
     */
    std::thread m_thread;

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