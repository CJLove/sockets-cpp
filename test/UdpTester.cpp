#include "UdpTester.h"
#include <chrono>


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