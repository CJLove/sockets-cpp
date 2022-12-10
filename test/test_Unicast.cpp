#include "gtest/gtest.h"
#include "UdpTester.h"
#include "UdpSocket.h"
#include <string>
#include <random>

TEST(UdpSocket,unicast) 
{
    UdpTester test1;
    UdpTester test2;

    auto ret1 = test1.m_socket.startUnicast("127.0.0.1",UDP_PORT1,UDP_PORT2);
    auto ret2 = test2.m_socket.startUnicast(UDP_PORT2);

    EXPECT_TRUE(ret1.m_success);
    EXPECT_TRUE(ret2.m_success);

    EXPECT_TRUE(test1.receiveData().empty());
    EXPECT_TRUE(test2.receiveData().empty());

    std::string testMessage {"testMessage"};
    test1.m_socket.sendMsg(testMessage.data(),testMessage.size());

    EXPECT_TRUE(test2.wait(5));

    auto test2Data = test2.receiveData();
    EXPECT_FALSE(test2Data.empty());
    if (!test2Data.empty()) {
        EXPECT_EQ(testMessage,test2Data[0]);
    }

}

#if 0
TEST(UdpSocket,unicast6000) 
{
    UdpTester test1;
    UdpTester test2;

    auto ret1 = test1.m_socket.startUnicast("127.0.0.1",UDP_PORT1+2,UDP_PORT2+3);
    auto ret2 = test2.m_socket.startUnicast(UDP_PORT2+3);

    EXPECT_TRUE(ret1.m_success);
    EXPECT_TRUE(ret2.m_success);

    EXPECT_TRUE(test1.receiveData().empty());
    EXPECT_TRUE(test2.receiveData().empty());

    std::string testMessage(UDP_MSG_SIZE,'A');
    test1.m_socket.sendMsg(testMessage.data(),testMessage.size());

    EXPECT_TRUE(test2.wait(5));
    auto test2Data = test2.receiveData();
    EXPECT_FALSE(test2Data.empty());
    if (!test2Data.empty()) {
        EXPECT_EQ(testMessage,test2Data[0]);
    }
}

TEST(UdpSocket,unicast65507) 
{
    UdpTester test1;
    UdpTester test2;

    auto ret1 = test1.m_socket.startUnicast("127.0.0.1",UDP_PORT1+4,UDP_PORT2+5);
    auto ret2 = test2.m_socket.startUnicast(UDP_PORT2+5);

    EXPECT_TRUE(ret1.m_success);
    EXPECT_TRUE(ret2.m_success);

    EXPECT_TRUE(test1.receiveData().empty());
    EXPECT_TRUE(test2.receiveData().empty());

    // Maximum UDP datagram size
    std::string testMessage(UDP_MAX_MSG_SIZE,'A');
    test1.m_socket.sendMsg(testMessage.data(),testMessage.size());

    EXPECT_TRUE(test2.wait(5));
    auto test2Data = test2.receiveData();
    EXPECT_FALSE(test2Data.empty());
    if (!test2Data.empty()) {
        EXPECT_EQ(testMessage,test2Data[0]);
    }
}
#endif
TEST(UdpSocket,unicastFail) 
{
    UdpTester test1;
    UdpTester test2;

    auto ret1 = test1.m_socket.startUnicast("127.0.0.1",UDP_PORT1+6,UDP_PORT2+7);
    auto ret2 = test2.m_socket.startUnicast(UDP_PORT1+7);

    EXPECT_TRUE(ret1.m_success);
    EXPECT_TRUE(ret2.m_success);

    EXPECT_TRUE(test1.receiveData().empty());
    EXPECT_TRUE(test2.receiveData().empty());

    std::string testMessage {"testMessage"};
    test1.m_socket.sendMsg(testMessage.data(),testMessage.size());

    EXPECT_FALSE(test2.wait(5));
    auto test2Data = test2.receiveData();
    EXPECT_TRUE(test2Data.empty());

}
