#include "UdpTester.h"

UdpTester::UdpTester():
    m_socket(this)
{

}

void UdpTester::onReceiveData(const char *data, size_t size)
{
    m_lastReceivedData = std::string(data,size);
}

void UdpTester::clear() {
    m_lastReceivedData.clear();
}