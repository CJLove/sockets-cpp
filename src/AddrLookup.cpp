#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
namespace sockets {

int lookupHost(const char *host, in_addr_t &addr) {
    struct addrinfo hints, *res, *result;
    int errcode = 0;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;

    errcode = getaddrinfo(host, NULL, &hints, &result);
    if (errcode != 0) {
        return -1;
    }

    res = result;

    if (res != nullptr) {
        addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr.s_addr;
    } else {
        errcode = -1;
    }

    freeaddrinfo(result);

    return errcode;
}

}