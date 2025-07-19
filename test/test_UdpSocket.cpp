#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "MockSocketCore.h"
#define TEST_CORE_ACCESS
#include "UdpSocket.h"
#include <string>
#include <thread>
#include <iostream>

using ::testing::AtLeast;
using ::testing::Return;
using ::testing::NotNull;
using ::testing::_;
using ::testing::SetArgPointee;
using ::testing::SetArrayArgument;
using ::testing::DoAll;

class UdpTestApp {
public:
    UdpTestApp(): m_socket(*this)
    {};

    UdpTestApp(sockets::SocketOpt *opts): m_socket(*this,opts)
    {};

    void onReceiveData(const char *data, size_t size) ;

    sockets::UdpSocket<UdpTestApp,MockSocketCore> m_socket;

    std::string m_receiveData;

};

void UdpTestApp::onReceiveData(const char *data, size_t size)
{
    m_receiveData = std::string(data,size);
}

TEST(UdpSocket,unicast_socket_fail) 
{
    UdpTestApp app;
    MockSocketCore &core = app.m_socket.getCore();

    EXPECT_CALL(core, Socket(_,_,_)).WillOnce(Return(-1));
    auto ret = app.m_socket.startUnicast(5000);
    EXPECT_EQ(false,ret.m_success);
}

TEST(UdpSocket,mcast_socket_fail) 
{
    UdpTestApp app;
    MockSocketCore &core = app.m_socket.getCore();

    EXPECT_CALL(core, Socket(_,_,_)).WillOnce(Return(-1));
    auto ret = app.m_socket.startMcast("224.0.0.1",5000);
    EXPECT_EQ(false,ret.m_success);
}

TEST(UdpSocket,unicast_sockopt_fail1)
{
    UdpTestApp app;
    MockSocketCore &core = app.m_socket.getCore();

    EXPECT_CALL(core, Socket(_,_,_)).WillOnce(Return(4));
    EXPECT_CALL(core, SetSockOpt(_,_,_,_,_)).WillOnce(Return(-1));
    EXPECT_CALL(core, Close(_)).WillOnce(Return(0));
    auto ret = app.m_socket.startUnicast(5000);
    EXPECT_EQ(false,ret.m_success);
}

TEST(UdpSocket,mcast_sockopt_fail1)
{
    UdpTestApp app;
    MockSocketCore &core = app.m_socket.getCore();

    EXPECT_CALL(core, Socket(_,_,_)).WillOnce(Return(4));
    EXPECT_CALL(core, SetSockOpt(_,_,_,_,_)).WillOnce(Return(-1));
    EXPECT_CALL(core, Close(_)).WillOnce(Return(0));
    auto ret = app.m_socket.startMcast("224.0.0.1",5000);
    EXPECT_EQ(false,ret.m_success);
}

TEST(UdpSocket,unicast_sockopt_fail2)
{
    UdpTestApp app;
    MockSocketCore &core = app.m_socket.getCore();

    EXPECT_CALL(core, Socket(_,_,_)).WillOnce(Return(4));
    EXPECT_CALL(core, SetSockOpt(_,_,_,_,_)).WillOnce(Return(0)).WillOnce(Return(-1));
    EXPECT_CALL(core, Close(_)).WillOnce(Return(0));
    auto ret = app.m_socket.startUnicast(5000);
    EXPECT_EQ(false,ret.m_success);
}

TEST(UdpSocket,mcast_sockopt_fail2)
{
    UdpTestApp app;
    MockSocketCore &core = app.m_socket.getCore();

    EXPECT_CALL(core, Socket(_,_,_)).WillOnce(Return(4));
    EXPECT_CALL(core, SetSockOpt(_,_,_,_,_)).WillOnce(Return(0)).WillOnce(Return(-1));
    EXPECT_CALL(core, Close(_)).WillOnce(Return(0));
    auto ret = app.m_socket.startMcast("224.0.0.1",5000);
    EXPECT_EQ(false,ret.m_success);
}

TEST(UdpSocket,unicast_sockopt_fail3)
{
    UdpTestApp app;
    MockSocketCore &core = app.m_socket.getCore();

    EXPECT_CALL(core, Socket(_,_,_)).WillOnce(Return(4));
    EXPECT_CALL(core, SetSockOpt(_,_,_,_,_)).WillOnce(Return(0)).WillOnce(Return(0)).WillOnce(Return(-1));
    EXPECT_CALL(core, Close(_)).WillOnce(Return(0));
    auto ret = app.m_socket.startUnicast(5000);
    EXPECT_EQ(false,ret.m_success);
}

TEST(UdpSocket,mcastsockopt_fail3)
{
    UdpTestApp app;
    MockSocketCore &core = app.m_socket.getCore();

    EXPECT_CALL(core, Socket(_,_,_)).WillOnce(Return(4));
    EXPECT_CALL(core, SetSockOpt(_,_,_,_,_)).WillOnce(Return(0)).WillOnce(Return(0)).WillOnce(Return(-1));
    EXPECT_CALL(core, Close(_)).WillOnce(Return(0));
    auto ret = app.m_socket.startMcast("224.0.0.1",5000);
    EXPECT_EQ(false,ret.m_success);
}


TEST(UdpSocket,unicast_bind_fail3)
{
    UdpTestApp app;
    MockSocketCore &core = app.m_socket.getCore();

    EXPECT_CALL(core, Socket(_,_,_)).WillOnce(Return(4));
    EXPECT_CALL(core, SetSockOpt(_,_,_,_,_)).WillOnce(Return(0)).WillOnce(Return(0)).WillOnce(Return(0));
    EXPECT_CALL(core,Bind(_,_,_)).WillOnce(Return(-1));
    EXPECT_CALL(core, Close(_)).WillOnce(Return(0));
    auto ret = app.m_socket.startUnicast(5000);
    EXPECT_EQ(false,ret.m_success);
}

TEST(UdpSocket,mcast_bind_fail3)
{
    UdpTestApp app;
    MockSocketCore &core = app.m_socket.getCore();

    EXPECT_CALL(core, Socket(_,_,_)).WillOnce(Return(4));
    EXPECT_CALL(core, SetSockOpt(_,_,_,_,_)).WillOnce(Return(0)).WillOnce(Return(0)).WillOnce(Return(0));
    EXPECT_CALL(core,Bind(_,_,_)).WillOnce(Return(-1));
    EXPECT_CALL(core, Close(_)).WillOnce(Return(0));
    auto ret = app.m_socket.startMcast("224.0.0.1",5000);
    EXPECT_EQ(false,ret.m_success);
}

TEST(UdpSocket,mcast_join_fail)
{
    // Exercise passing socket options in
    sockets::SocketOpt opts;
    opts.m_rxBufSize = 8192;
    opts.m_txBufSize = 8192;
    UdpTestApp app(&opts);
    MockSocketCore &core = app.m_socket.getCore();

    EXPECT_CALL(core, Socket(_,_,_)).WillOnce(Return(4));
    EXPECT_CALL(core, SetSockOpt(_,_,_,_,_)).WillOnce(Return(0)).WillOnce(Return(0)).WillOnce(Return(0)).WillOnce(Return(-1));
    EXPECT_CALL(core,Bind(_,_,_)).WillOnce(Return(0));
    EXPECT_CALL(core, Close(_)).WillOnce(Return(0));
    EXPECT_CALL(core, Select(_,_,_,_,_)).WillRepeatedly(Return(0));
    auto ret = app.m_socket.startMcast("224.0.0.1",5000);
    EXPECT_EQ(false,ret.m_success);
}

TEST(UdpSocket,unicast_start_stop)
{
    UdpTestApp app;
    MockSocketCore &core = app.m_socket.getCore();

    EXPECT_CALL(core, Socket(_,_,_)).WillOnce(Return(4));
    EXPECT_CALL(core, SetSockOpt(_,_,_,_,_)).WillOnce(Return(0)).WillOnce(Return(0)).WillOnce(Return(0));
    EXPECT_CALL(core,Bind(_,_,_)).WillOnce(Return(0));
    EXPECT_CALL(core, Close(_)).WillOnce(Return(0));
    EXPECT_CALL(core, Select(_,_,_,_,_)).WillRepeatedly(Return(0));
    auto ret = app.m_socket.startUnicast(5000);
    EXPECT_EQ(true,ret.m_success);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    app.m_socket.finish();
}

TEST(UdpSocket,mcast_start_stop)
{
    UdpTestApp app;
    MockSocketCore &core = app.m_socket.getCore();

    EXPECT_CALL(core, Socket(_,_,_)).WillOnce(Return(4));
    EXPECT_CALL(core, SetSockOpt(_,_,_,_,_)).WillOnce(Return(0)).WillOnce(Return(0)).WillOnce(Return(0)).WillOnce(Return(0)).WillOnce(Return(0));
    EXPECT_CALL(core,Bind(_,_,_)).WillOnce(Return(0));
    EXPECT_CALL(core, Close(_)).WillOnce(Return(0));
    EXPECT_CALL(core, Select(_,_,_,_,_)).WillRepeatedly(Return(0));
    auto ret = app.m_socket.startMcast("224.0.0.1",5000);
    EXPECT_EQ(true,ret.m_success);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    app.m_socket.finish();
}

TEST(UdpSocket,mcast_start_stop_local)
{
    UdpTestApp app;
    MockSocketCore &core = app.m_socket.getCore();

    EXPECT_CALL(core, Socket(_,_,_)).WillOnce(Return(4));
    EXPECT_CALL(core, SetSockOpt(_,_,_,_,_)).WillOnce(Return(0)).WillOnce(Return(0)).WillOnce(Return(0)).WillOnce(Return(0)).WillOnce(Return(0));
    EXPECT_CALL(core,Bind(_,_,_)).WillOnce(Return(0));
    EXPECT_CALL(core, Close(_)).WillOnce(Return(0));
    EXPECT_CALL(core, Select(_,_,_,_,_)).WillRepeatedly(Return(0));
    auto ret = app.m_socket.startMcast("224.0.0.1",5000,"127.0.0.1");
    EXPECT_EQ(true,ret.m_success);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    app.m_socket.finish();
}

TEST(UdpSocket,finish_close_failure)
{
    UdpTestApp app;
    MockSocketCore &core = app.m_socket.getCore();

    EXPECT_CALL(core, Socket(_,_,_)).WillOnce(Return(4));
    EXPECT_CALL(core, SetSockOpt(_,_,_,_,_)).WillOnce(Return(0)).WillOnce(Return(0)).WillOnce(Return(0));
    EXPECT_CALL(core,Bind(_,_,_)).WillOnce(Return(0));
    EXPECT_CALL(core, Close(_)).WillOnce(Return(-1));
    EXPECT_CALL(core, Select(_,_,_,_,_)).WillRepeatedly(Return(0));
    auto ret = app.m_socket.startUnicast(5000);
    EXPECT_EQ(true,ret.m_success);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    app.m_socket.finish();
}

TEST(UdpSocket,unicast_lookup_fail)
{
    UdpTestApp app;
    MockSocketCore &core = app.m_socket.getCore();

    EXPECT_CALL(core, GetAddrInfo(_,_, NotNull(),NotNull())).WillOnce(Return(-1));
    auto ret = app.m_socket.startUnicast("badhost",5000,5001);
    EXPECT_EQ(false,ret.m_success);
}

TEST(UdpSocket,unicast_lookup_success)
{
    UdpTestApp app;
    MockSocketCore &core = app.m_socket.getCore();

    struct addrinfo res;
    struct sockaddr theAddr = { 0,
#ifdef __APPLE__
    0,
#endif
    "\000\000\177\000\000\001" };
    res.ai_addr = &theAddr;
    sockets::AddrLookup<MockSocketCore> lookup(core);
    EXPECT_CALL(core, GetAddrInfo(_,_, NotNull(),_)).WillOnce(DoAll(SetArgPointee<3>(&res), Return(0)));
    EXPECT_CALL(core, FreeAddrInfo(_));
    EXPECT_CALL(core, Socket(_,_,_)).WillOnce(Return(4));
    EXPECT_CALL(core, SetSockOpt(_,_,_,_,_)).WillOnce(Return(0)).WillOnce(Return(0)).WillOnce(Return(0));
    EXPECT_CALL(core, Bind(_,_,_)).WillOnce(Return(0));
    EXPECT_CALL(core, Close(_)).WillOnce(Return(0));
    EXPECT_CALL(core, Select(_,_,_,_,_)).WillRepeatedly(Return(0));
    auto ret = app.m_socket.startUnicast("localhost",5000,5001);
    EXPECT_EQ(true,ret.m_success);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    app.m_socket.finish();
}

TEST(UdpSocket,unicast_receive_data)
{
    UdpTestApp app;
    MockSocketCore &core = app.m_socket.getCore();
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(4,&fds);
    char receiveData[] = { "Received Data" };
    char *ptr = receiveData;

    EXPECT_CALL(core, Socket(_,_,_)).WillOnce(Return(4));
    EXPECT_CALL(core, SetSockOpt(_,_,_,_,_)).WillOnce(Return(0)).WillOnce(Return(0)).WillOnce(Return(0));
    EXPECT_CALL(core,Bind(_,_,_)).WillOnce(Return(0));
    EXPECT_CALL(core, Close(_)).WillOnce(Return(0));
    EXPECT_CALL(core, Select(_,_,_,_,_)).WillOnce(DoAll(SetArgPointee<1>(fds),Return(1))).WillRepeatedly(Return(0));
    EXPECT_CALL(core, Recv(_,_,_,_)).WillOnce(DoAll(SetArrayArgument<1>(ptr,ptr+13), Return(13)));
    auto ret = app.m_socket.startUnicast(5000);
    EXPECT_EQ(true,ret.m_success);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    app.m_socket.finish();

    EXPECT_EQ(app.m_receiveData,"Received Data");
}

TEST(UdpSocket,send_msg_fail)
{
    UdpTestApp app;
    MockSocketCore &core = app.m_socket.getCore();

    struct addrinfo res;
    struct sockaddr theAddr = { 0, 
#ifdef __APPLE__
    0,
#endif   
    "\000\000\177\000\000\001" };
    res.ai_addr = &theAddr;
    sockets::AddrLookup<MockSocketCore> lookup(core);
    EXPECT_CALL(core, GetAddrInfo(_,_, NotNull(),_)).WillOnce(DoAll(SetArgPointee<3>(&res), Return(0)));
    EXPECT_CALL(core, FreeAddrInfo(_));
    EXPECT_CALL(core, Socket(_,_,_)).WillOnce(Return(4));
    EXPECT_CALL(core, SetSockOpt(_,_,_,_,_)).WillOnce(Return(0)).WillOnce(Return(0)).WillOnce(Return(0));
    EXPECT_CALL(core, Bind(_,_,_)).WillOnce(Return(0));
    EXPECT_CALL(core, Close(_)).WillOnce(Return(0));
    EXPECT_CALL(core, SendTo(_,_,_,_,_,_)).WillOnce(Return(-1));
    EXPECT_CALL(core, Select(_,_,_,_,_)).WillRepeatedly(Return(0));
    auto ret = app.m_socket.startUnicast("localhost",5000,5001);
    EXPECT_EQ(true,ret.m_success);

    ret = app.m_socket.sendMsg("test message",11);
    EXPECT_EQ(false,ret.m_success);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    app.m_socket.finish();
}

TEST(UdpSocket,send_msg_partial)
{
    UdpTestApp app;
    MockSocketCore &core = app.m_socket.getCore();

    struct addrinfo res;
    struct sockaddr theAddr = { 0,
#ifdef __APPLE__
    0,
#endif
    "\000\000\177\000\000\001" };
    res.ai_addr = &theAddr;
    sockets::AddrLookup<MockSocketCore> lookup(core);
    EXPECT_CALL(core, GetAddrInfo(_,_, NotNull(),_)).WillOnce(DoAll(SetArgPointee<3>(&res), Return(0)));
    EXPECT_CALL(core, FreeAddrInfo(_));
    EXPECT_CALL(core, Socket(_,_,_)).WillOnce(Return(4));
    EXPECT_CALL(core, SetSockOpt(_,_,_,_,_)).WillOnce(Return(0)).WillOnce(Return(0)).WillOnce(Return(0));
    EXPECT_CALL(core, Bind(_,_,_)).WillOnce(Return(0));
    EXPECT_CALL(core, Close(_)).WillOnce(Return(0));
    EXPECT_CALL(core, SendTo(_,_,_,_,_,_)).WillOnce(Return(5));
    EXPECT_CALL(core, Select(_,_,_,_,_)).WillRepeatedly(Return(0));
    auto ret = app.m_socket.startUnicast("localhost",5000,5001);
    EXPECT_EQ(true,ret.m_success);

    ret = app.m_socket.sendMsg("test message",11);
    EXPECT_EQ(false,ret.m_success);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    app.m_socket.finish();
}

TEST(UdpSocket,send_msg)
{
    UdpTestApp app;
    MockSocketCore &core = app.m_socket.getCore();

    struct addrinfo res;
    struct sockaddr theAddr = { 0,
#ifdef __APPLE__
    0,
#endif
    "\000\000\177\000\000\001" };
    res.ai_addr = &theAddr;
    sockets::AddrLookup<MockSocketCore> lookup(core);
    EXPECT_CALL(core, GetAddrInfo(_,_, NotNull(),_)).WillOnce(DoAll(SetArgPointee<3>(&res), Return(0)));
    EXPECT_CALL(core, FreeAddrInfo(_));
    EXPECT_CALL(core, Socket(_,_,_)).WillOnce(Return(4));
    EXPECT_CALL(core, SetSockOpt(_,_,_,_,_)).WillOnce(Return(0)).WillOnce(Return(0)).WillOnce(Return(0));
    EXPECT_CALL(core, Bind(_,_,_)).WillOnce(Return(0));
    EXPECT_CALL(core, Close(_)).WillOnce(Return(0));
    EXPECT_CALL(core, SendTo(_,_,_,_,_,_)).WillOnce(Return(11));
    EXPECT_CALL(core, Select(_,_,_,_,_)).WillRepeatedly(Return(0));
    auto ret = app.m_socket.startUnicast("localhost",5000,5001);
    EXPECT_EQ(true,ret.m_success);

    ret = app.m_socket.sendMsg("test message",11);
    EXPECT_EQ(true,ret.m_success);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    app.m_socket.finish();
}