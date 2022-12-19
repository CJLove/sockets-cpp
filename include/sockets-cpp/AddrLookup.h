#pragma once
#include "SocketCore.h"
#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif // WIN32_LEAN_AND_MEAN
    #include <winsock2.h>
    #include <windows.h>
    #include <ws2tcpip.h>
    #include <inaddr.h>
#else
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <sys/socket.h>
#endif
#include <cstring>
#include <sys/types.h>

namespace sockets {

template <class SocketImpl = sockets::SocketCore>
class AddrLookup {
public:
    AddrLookup() = default;

    explicit AddrLookup(SocketImpl &impl): m_socketCore(impl)
    {}

    ~AddrLookup() = default;

    /**
     * @brief Return the first IPV4 hostname for a hostname
     *
     * @param host - hostname to resolve
     * @param addr - IPV4 address
     * @return int - 0 indicates success, non-zero indicates failure
     */
    int lookupHost(const char *host, in_addr_t &addr) {
        struct addrinfo hints;
        struct addrinfo *res = nullptr;
        struct addrinfo *result = nullptr;
        int errcode = 0;

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;

        errcode = m_socketCore.GetAddrInfo(host, nullptr, &hints, &result);
        if (errcode != 0) {
            return -1;
        }

        res = result;

        if (res != nullptr) {
            addr = reinterpret_cast<struct sockaddr_in *>(res->ai_addr)->sin_addr.s_addr;
        } else {
            errcode = -1;
        }

        m_socketCore.FreeAddrInfo(result);

        return errcode;
    }

private:
    SocketImpl &m_socketCore;
};

}  // namespace sockets