#include "gtest/gtest.h"
#include "UdpTester.h"
#include "UdpSocket.h"
#include <string>



TEST(UdpSocket,multicast) 
{
    UdpTester test1;
    UdpTester test2;
    uint16_t port = getPort();
    std::cout << "Using port " << port << std::endl;

    auto ret1 = test1.m_socket.startMcast("224.0.0.1",port);
    auto ret2 = test2.m_socket.startMcast("224.0.0.1",port);

    EXPECT_TRUE(ret1.m_success);
    EXPECT_TRUE(ret2.m_success);

    EXPECT_TRUE(test1.receiveData().empty());
    EXPECT_TRUE(test2.receiveData().empty());

    std::string testMessage {"testMessage"};
    test1.m_socket.sendMsg(testMessage.data(),testMessage.size());

    EXPECT_TRUE(test1.wait(5));
    EXPECT_TRUE(test2.wait(5));

    auto test1Data = test1.receiveData();
    auto test2Data = test2.receiveData();
    EXPECT_FALSE(test1Data.empty());
    EXPECT_FALSE(test2Data.empty());

    EXPECT_EQ(testMessage,test1Data[0]);
    EXPECT_EQ(testMessage,test2Data[0]);

}

TEST(UdpSocket,multicast6000) 
{
    UdpTester test1;
    UdpTester test2;
    uint16_t port = getPort();
    std::cout << "Using port " << port << std::endl;

    auto ret1 = test1.m_socket.startMcast("224.0.0.1",port);
    auto ret2 = test2.m_socket.startMcast("224.0.0.1",port);

    EXPECT_TRUE(ret1.m_success);
    EXPECT_TRUE(ret2.m_success);

    EXPECT_TRUE(test1.receiveData().empty());
    EXPECT_TRUE(test2.receiveData().empty());

    std::string testMessage(UDP_MSG_SIZE,'A');
    test1.m_socket.sendMsg(testMessage.data(),testMessage.size());

    EXPECT_TRUE(test1.wait(5));
    EXPECT_TRUE(test2.wait(5));

    auto test1Data = test1.receiveData();
    auto test2Data = test2.receiveData();
    EXPECT_FALSE(test1Data.empty());
    EXPECT_FALSE(test2Data.empty());

    if (!test1Data.empty()) {
        EXPECT_EQ(testMessage,test1Data[0]);
    }
    if (!test2Data.empty()) {
        EXPECT_EQ(testMessage,test2Data[0]);
    }
}

TEST(UdpSocket,multicastMax) 
{
    UdpTester test1;
    UdpTester test2;
    uint16_t port = getPort();
    std::cout << "Using port " << port << std::endl;

    auto ret1 = test1.m_socket.startMcast("224.0.0.1",port);
    auto ret2 = test2.m_socket.startMcast("224.0.0.1",port);

    EXPECT_TRUE(ret1.m_success);
    EXPECT_TRUE(ret2.m_success);

    EXPECT_TRUE(test1.receiveData().empty());
    EXPECT_TRUE(test2.receiveData().empty());

    // Maximum UDP datagram size
    std::string testMessage(26600,'A');
    test1.m_socket.sendMsg(testMessage.data(),testMessage.size());

    EXPECT_TRUE(test1.wait(5));
    EXPECT_TRUE(test2.wait(5));

    auto test1Data = test1.receiveData();
    auto test2Data = test2.receiveData();
    EXPECT_FALSE(test1Data.empty());
    EXPECT_FALSE(test2Data.empty());

    if (!test1Data.empty()) {
        EXPECT_EQ(testMessage,test1Data[0]);
    }
    if (!test2Data.empty()) {
        EXPECT_EQ(testMessage,test2Data[0]);
    }
}

TEST(UdpSocket,multicastFail) 
{
    UdpTester test1;
    UdpTester test2;
    uint16_t port1 = getPort();
    uint16_t port2 = getPort();
    std::cout << "Using port " << port1 << " & " << port2 << std::endl;

    auto ret1 = test1.m_socket.startMcast("224.0.0.1",port1);
    auto ret2 = test2.m_socket.startMcast("224.1.0.2",port2);

    EXPECT_TRUE(ret1.m_success);
    EXPECT_TRUE(ret2.m_success);

    EXPECT_TRUE(test1.receiveData().empty());
    EXPECT_TRUE(test2.receiveData().empty());

    std::string testMessage {"testMessage"};
    test1.m_socket.sendMsg(testMessage.data(),testMessage.size());

    EXPECT_TRUE(test1.wait(5));
    EXPECT_FALSE(test2.wait(5));

    auto test1Data = test1.receiveData();
    auto test2Data = test2.receiveData();
    EXPECT_FALSE(test1Data.empty());
    EXPECT_TRUE(test2Data.empty());

    EXPECT_EQ(testMessage,test1Data[0]);
}
