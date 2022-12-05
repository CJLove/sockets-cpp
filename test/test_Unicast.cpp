#include "gtest/gtest.h"
#include "UdpTester.h"
#include "UdpSocket.h"
#include <string>

TEST(UdpSocket,unicast) 
{
    UdpTester test1;
    UdpTester test2;

    test1.m_socket.startUnicast("127.0.0.1",5000,5001);
    test2.m_socket.startUnicast(5001);

    EXPECT_EQ("",test1.m_lastReceivedData);
    EXPECT_EQ("",test2.m_lastReceivedData);

    std::string testMessage {"testMessage"};
    test1.m_socket.sendMsg(testMessage.data(),testMessage.size());

    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    EXPECT_EQ(testMessage,test2.m_lastReceivedData);

    // Explicitly stop/close each socket; this otherwise happens in the
    // destructor
    auto ret1 = test1.m_socket.finish();
    EXPECT_EQ(true,ret1.m_success);
    auto ret2 = test2.m_socket.finish();
    EXPECT_EQ(true,ret2.m_success);
}

TEST(UdpSocket,unicast6000) 
{
    UdpTester test1;
    UdpTester test2;

    test1.m_socket.startUnicast("127.0.0.1",5000,5001);
    test2.m_socket.startUnicast(5001);

    EXPECT_EQ("",test1.m_lastReceivedData);
    EXPECT_EQ("",test2.m_lastReceivedData);

    std::string testMessage(6000,'A');
    test1.m_socket.sendMsg(testMessage.data(),testMessage.size());

    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    EXPECT_EQ(testMessage,test2.m_lastReceivedData);
}

TEST(UdpSocket,unicast65507) 
{
    UdpTester test1;
    UdpTester test2;

    test1.m_socket.startUnicast("127.0.0.1",5000,5001);
    test2.m_socket.startUnicast(5001);

    EXPECT_EQ("",test1.m_lastReceivedData);
    EXPECT_EQ("",test2.m_lastReceivedData);

    // Maximum UDP datagram size
    std::string testMessage(65507,'A');
    test1.m_socket.sendMsg(testMessage.data(),testMessage.size());

    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    EXPECT_EQ(testMessage,test2.m_lastReceivedData);
}

TEST(UdpSocket,unicastFail) 
{
    UdpTester test1;
    UdpTester test2;

    test1.m_socket.startUnicast("127.0.0.1",5000,5002);
    test2.m_socket.startUnicast(5001);

    EXPECT_EQ("",test1.m_lastReceivedData);
    EXPECT_EQ("",test2.m_lastReceivedData);

    std::string testMessage {"testMessage"};
    test1.m_socket.sendMsg(testMessage.data(),testMessage.size());

    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    EXPECT_EQ("",test2.m_lastReceivedData);
}

