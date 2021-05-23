#include "ISocket.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

namespace sockets {

void ISocket::onReceiveData(const unsigned char *data, size_t size)
{
}

void ISocket::onReceiveClientData(const Client &client, const unsigned char *data, size_t size)
{
}

void ISocket::onDisconnect(const SocketRet &ret)
{
}

void ISocket::onClientDisconnect(const Client &client, const SocketRet &ret)
{
}

}   // Namespace sockets