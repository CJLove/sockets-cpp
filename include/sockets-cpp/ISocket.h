#pragma once
#include <memory>
#include <string>

namespace sockets {

class Client;

struct SocketRet {
    bool m_success;
    std::string m_msg;
    SocketRet() {
        m_success = false;
        m_msg = "";
    }
};

// Interface class for receiving data or disconnection notification from socket
// classes
class ISocket {
public:
    virtual ~ISocket() = default;

    virtual void onReceiveData(const unsigned char *data, size_t size)
    {
    }

    virtual void onReceiveClientData(const Client &client, const unsigned char *data, size_t size)
    {
    }

    virtual void onDisconnect(const SocketRet &ret)
    {
    }

    virtual void onClientDisconnect(const Client &client, const SocketRet &ret)
    {
    }

};

}  // Namespace sockets