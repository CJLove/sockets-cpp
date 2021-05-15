#pragma once
#include <string>
#include <memory>

struct SocketRet {
    bool m_success;
    std::string m_msg;
    SocketRet() {
        m_success = false;
        m_msg = "";
    }
};

// Interface class for receiving data or disconnection notification from a socket
class ISocket {
public:
    virtual void onReceiveData(const unsigned char *data, size_t size) = 0;

    virtual void onDisconnect(const SocketRet &ret) = 0;
};

