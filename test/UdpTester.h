#pragma once

#include "UdpSocket.h"
#include <string>

class UdpTester: public sockets::IUdpSocket {
public:
    UdpTester();

    void onReceiveData(const char *data, size_t size) override;

    void clear();

    std::string m_lastReceivedData;
    sockets::UdpSocket m_socket;
};