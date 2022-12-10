#pragma once

#include "UdpSocket.h"
#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>

constexpr uint16_t UDP_PORT1 = 5000;
constexpr uint16_t UDP_PORT2 = 5001;

constexpr size_t UDP_MSG_SIZE = 6000;
constexpr size_t UDP_MAX_MSG_SIZE = 60507;

uint16_t getPort();

class UdpTester: public sockets::IUdpSocket {
public:
    UdpTester();

    ~UdpTester();

    void onReceiveData(const char *data, size_t size) override;

    void clear();

    bool wait(uint32_t waitMs);

    std::vector<std::string> receiveData();

    sockets::UdpSocket m_socket;

private:
    std::mutex m_mutex;
    std::condition_variable m_cond;
    std::vector<std::string> m_receiveData;
    bool m_ready = false;
};