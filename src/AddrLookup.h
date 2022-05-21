#pragma once
#include <arpa/inet.h>

namespace sockets {

    /**
     * @brief Return the first IPV4 hostname for a hostname
     * 
     * @param host - hostname to resolve
     * @param addr - IPV4 address
     * @return int - 0 indicates success, non-zero indicates failure
     */
    int lookupHost(const char *host, in_addr_t &addr);


}   // namespace sockets