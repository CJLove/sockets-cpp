#include "UdpTester.h"
#include <chrono>
#include <unistd.h>

uint16_t getPort()
{
    static uint16_t idx = 0;
    constexpr uint16_t BASE_PORT = 5000;
    constexpr uint16_t MULTIPLIER = 100;
    // Retrieve the pid of this process and reduce it to the range 0..255
    pid_t myPid = getpid();
    
    uint16_t pidMod = static_cast<uint16_t>(static_cast<uint32_t>(myPid) % 256);

    uint16_t port = BASE_PORT + (pidMod * MULTIPLIER) + idx;
    idx++;
    return port;
}

UdpTester::UdpTester():
    m_socket(this),
    m_ready(false)
{

}

UdpTester::~UdpTester()
{
    m_socket.finish();
}

void UdpTester::onReceiveData(const char *data, size_t size)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    std::string dataStr = std::string(data,size);
    m_receiveData.push_back(dataStr);
    m_ready = true;
    m_cond.notify_one();
}

void UdpTester::clear() {
    std::lock_guard<std::mutex> guard(m_mutex);
    m_receiveData.clear();
}

bool UdpTester::wait(uint32_t waitMs)
{
    std::unique_lock<std::mutex> unique_lock(m_mutex);
    if (m_ready) {
        return true;
    }
    if (m_cond.wait_for(unique_lock, std::chrono::milliseconds(waitMs), [this] { return m_ready; })) {
        return true;
    }
    return false;
}

std::vector<std::string> UdpTester::receiveData() {
    std::lock_guard<std::mutex> guard(m_mutex);
    return m_receiveData;
}