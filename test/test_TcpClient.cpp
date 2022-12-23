#include "gtest/gtest.h"
#include "gmock/gmock.h"
#define TEST_CORE_ACCESS
#include "TcpClient.h"
#include "MockSocketCore.h"
#include <thread>

using ::testing::AtLeast;
using ::testing::Return;
using ::testing::NotNull;
using ::testing::_;
using ::testing::SetArgPointee;
using ::testing::SetArrayArgument;
using ::testing::DoAll;

class TcpClientTestApp {
public:
    TcpClientTestApp(): m_socket(*this)
    {};

    TcpClientTestApp(sockets::SocketOpt *opts): m_socket(*this,opts)
    {};

    virtual ~TcpClientTestApp() = default;

    void onReceiveData(const char *data, size_t size);

    void onDisconnect(const sockets::SocketRet &ret);   

    sockets::TcpClient<TcpClientTestApp,MockSocketCore> m_socket;

    std::string m_receiveData;

    bool m_disconnected;
};

void
TcpClientTestApp::onDisconnect(const sockets::SocketRet &) {
    m_disconnected = true;
}

void
TcpClientTestApp::onReceiveData(const char *data, size_t size) {
    m_receiveData = std::string(data,size);
}

TEST(TcpClientSocket,tcp_socket_fail) 
{
    TcpClientTestApp app;
    MockSocketCore &core = app.m_socket.getCore();

    EXPECT_CALL(core, Socket(_,_,_)).WillOnce(Return(-1));
    auto ret = app.m_socket.connectTo("localhost",5000);
    EXPECT_EQ(false,ret.m_success);
}

TEST(TcpClientSocket,tcp_sockopt_fail1)
{
    TcpClientTestApp app;
    MockSocketCore &core = app.m_socket.getCore();

    EXPECT_CALL(core, Socket(_,_,_)).WillOnce(Return(4));
    EXPECT_CALL(core, SetSockOpt(_,_,_,_,_)).WillOnce(Return(-1));
    EXPECT_CALL(core, Close(_)).WillOnce(Return(0));
    auto ret = app.m_socket.connectTo("localhost",5000);
    EXPECT_EQ(false,ret.m_success);
}

TEST(TcpClientSocket,tcp_sockopt_fail2)
{
    TcpClientTestApp app;
    MockSocketCore &core = app.m_socket.getCore();

    EXPECT_CALL(core, Socket(_,_,_)).WillOnce(Return(4));
    EXPECT_CALL(core, SetSockOpt(_,_,_,_,_)).WillOnce(Return(0)).WillOnce(Return(-1));
    EXPECT_CALL(core, Close(_)).WillOnce(Return(0));
    auto ret = app.m_socket.connectTo("localhost",5000);
    EXPECT_EQ(false,ret.m_success);
}

TEST(TcpClientSocket,unicast_lookup_fail)
{
    TcpClientTestApp app;
    MockSocketCore &core = app.m_socket.getCore();

    EXPECT_CALL(core, Socket(_,_,_)).WillOnce(Return(4));
    EXPECT_CALL(core, SetSockOpt(_,_,_,_,_)).WillOnce(Return(0)).WillOnce(Return(0));
    EXPECT_CALL(core, Close(_)).WillOnce(Return(0));
    EXPECT_CALL(core, GetAddrInfo(_,_, NotNull(),NotNull())).WillOnce(Return(-1));
    auto ret = app.m_socket.connectTo("localhost",5000);

    EXPECT_EQ(false,ret.m_success);
}

TEST(TcpClientSocket,tcp_connect_fail)
{
    // Exercise passing socket options in
    sockets::SocketOpt opts;
    opts.m_rxBufSize = 8192;
    opts.m_txBufSize = 8192;

    TcpClientTestApp app(&opts);
    MockSocketCore &core = app.m_socket.getCore();
    struct addrinfo res;
    struct sockaddr theAddr = { 0, 
#ifdef __APPLE__
    0,
#endif   
    "\000\000\177\000\000\001" };
    res.ai_addr = &theAddr;
    EXPECT_CALL(core, Socket(_,_,_)).WillOnce(Return(4));
    EXPECT_CALL(core, SetSockOpt(_,_,_,_,_)).WillOnce(Return(0)).WillOnce(Return(0));
    EXPECT_CALL(core, GetAddrInfo(_,_, NotNull(),_)).WillOnce(DoAll(SetArgPointee<3>(&res), Return(0)));
    EXPECT_CALL(core, FreeAddrInfo(_));
    EXPECT_CALL(core, Connect(_,_,_)).WillOnce(Return(-1));
    EXPECT_CALL(core, Close(_)).WillOnce(Return(0));
    auto ret = app.m_socket.connectTo("localhost",5000);
    EXPECT_EQ(false,ret.m_success);
}

TEST(TcpClientSocket,tcp_connect)
{
    TcpClientTestApp app;
    MockSocketCore &core = app.m_socket.getCore();
    struct addrinfo res;
    struct sockaddr theAddr = { 0,
#ifdef __APPLE__
    0,
#endif   
    "\000\000\177\000\000\001" };
    res.ai_addr = &theAddr;
    EXPECT_CALL(core, Socket(_,_,_)).WillOnce(Return(4));
    EXPECT_CALL(core, SetSockOpt(_,_,_,_,_)).WillOnce(Return(0)).WillOnce(Return(0));
    EXPECT_CALL(core, GetAddrInfo(_,_, NotNull(),_)).WillOnce(DoAll(SetArgPointee<3>(&res), Return(0)));
    EXPECT_CALL(core, FreeAddrInfo(_));
    EXPECT_CALL(core, Connect(_,_,_)).WillOnce(Return(0));
    EXPECT_CALL(core, Select(_,_,_,_,_)).WillRepeatedly(Return(0));
    EXPECT_CALL(core, Close(_)).WillOnce(Return(0));
    auto ret = app.m_socket.connectTo("localhost",5000);
    EXPECT_EQ(true,ret.m_success);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    app.m_socket.finish();
}

TEST(TcpClientSocket,tcp_finish_fail)
{
    TcpClientTestApp app;
    MockSocketCore &core = app.m_socket.getCore();
    struct addrinfo res;
    struct sockaddr theAddr = { 0, 
#ifdef __APPLE__
    0,
#endif   
    "\000\000\177\000\000\001" };
    res.ai_addr = &theAddr;
    EXPECT_CALL(core, Socket(_,_,_)).WillOnce(Return(4));
    EXPECT_CALL(core, SetSockOpt(_,_,_,_,_)).WillOnce(Return(0)).WillOnce(Return(0));
    EXPECT_CALL(core, GetAddrInfo(_,_, NotNull(),_)).WillOnce(DoAll(SetArgPointee<3>(&res), Return(0)));
    EXPECT_CALL(core, FreeAddrInfo(_));
    EXPECT_CALL(core, Connect(_,_,_)).WillOnce(Return(0));
    EXPECT_CALL(core, Select(_,_,_,_,_)).WillRepeatedly(Return(0));
    EXPECT_CALL(core, Close(_)).WillOnce(Return(-1));
    auto ret = app.m_socket.connectTo("localhost",5000);
    EXPECT_EQ(true,ret.m_success);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    app.m_socket.finish();
}

TEST(TcpClientSocket,tcp_send_fail)
{
    TcpClientTestApp app;
    MockSocketCore &core = app.m_socket.getCore();
    struct addrinfo res;
    struct sockaddr theAddr = { 0, 
#ifdef __APPLE__
    0,
#endif   
    "\000\000\177\000\000\001" };
    res.ai_addr = &theAddr;
    EXPECT_CALL(core, Socket(_,_,_)).WillOnce(Return(4));
    EXPECT_CALL(core, SetSockOpt(_,_,_,_,_)).WillOnce(Return(0)).WillOnce(Return(0));
    EXPECT_CALL(core, GetAddrInfo(_,_, NotNull(),_)).WillOnce(DoAll(SetArgPointee<3>(&res), Return(0)));
    EXPECT_CALL(core, FreeAddrInfo(_));
    EXPECT_CALL(core, Connect(_,_,_)).WillOnce(Return(0));
    EXPECT_CALL(core, Select(_,_,_,_,_)).WillRepeatedly(Return(0));
    EXPECT_CALL(core, Send(_,_,_,_)).WillOnce(Return(-1));
    EXPECT_CALL(core, Close(_)).WillOnce(Return(0));
    auto ret = app.m_socket.connectTo("localhost",5000);
    EXPECT_EQ(true,ret.m_success);

    ret = app.m_socket.sendMsg("Message Data",12);
    EXPECT_EQ(false,ret.m_success);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    app.m_socket.finish();
}

TEST(TcpClientSocket,tcp_send_partial)
{
    TcpClientTestApp app;
    MockSocketCore &core = app.m_socket.getCore();
    struct addrinfo res;
    struct sockaddr theAddr = { 0, 
#ifdef __APPLE__
    0,
#endif   
    "\000\000\177\000\000\001" };
    res.ai_addr = &theAddr;
    EXPECT_CALL(core, Socket(_,_,_)).WillOnce(Return(4));
    EXPECT_CALL(core, SetSockOpt(_,_,_,_,_)).WillOnce(Return(0)).WillOnce(Return(0));
    EXPECT_CALL(core, GetAddrInfo(_,_, NotNull(),_)).WillOnce(DoAll(SetArgPointee<3>(&res), Return(0)));
    EXPECT_CALL(core, FreeAddrInfo(_));
    EXPECT_CALL(core, Connect(_,_,_)).WillOnce(Return(0));
    EXPECT_CALL(core, Select(_,_,_,_,_)).WillRepeatedly(Return(0));
    EXPECT_CALL(core, Send(_,_,_,_)).WillOnce(Return(5));
    EXPECT_CALL(core, Close(_)).WillOnce(Return(0));
    auto ret = app.m_socket.connectTo("localhost",5000);
    EXPECT_EQ(true,ret.m_success);

    ret = app.m_socket.sendMsg("Message Data",12);
    EXPECT_EQ(false,ret.m_success);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    app.m_socket.finish();
}

TEST(TcpClientSocket,tcp_send_success)
{
    TcpClientTestApp app;
    MockSocketCore &core = app.m_socket.getCore();
    struct addrinfo res;
    struct sockaddr theAddr = { 0, 
#ifdef __APPLE__
    0,
#endif
    "\000\000\177\000\000\001" };
    res.ai_addr = &theAddr;
    EXPECT_CALL(core, Socket(_,_,_)).WillOnce(Return(4));
    EXPECT_CALL(core, SetSockOpt(_,_,_,_,_)).WillOnce(Return(0)).WillOnce(Return(0));
    EXPECT_CALL(core, GetAddrInfo(_,_, NotNull(),_)).WillOnce(DoAll(SetArgPointee<3>(&res), Return(0)));
    EXPECT_CALL(core, FreeAddrInfo(_));
    EXPECT_CALL(core, Connect(_,_,_)).WillOnce(Return(0));
    EXPECT_CALL(core, Select(_,_,_,_,_)).WillRepeatedly(Return(0));
    EXPECT_CALL(core, Send(_,_,_,_)).WillOnce(Return(12));
    EXPECT_CALL(core, Close(_)).WillOnce(Return(0));
    auto ret = app.m_socket.connectTo("localhost",5000);
    EXPECT_EQ(true,ret.m_success);

    ret = app.m_socket.sendMsg("Message Data",12);
    EXPECT_EQ(true,ret.m_success);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    app.m_socket.finish();
}

TEST(TcpClientSocket,tcp_receive)
{
    TcpClientTestApp app;
    MockSocketCore &core = app.m_socket.getCore();
    struct addrinfo res;
    struct sockaddr theAddr = { 0, 
#ifdef __APPLE__
    0,
#endif
    "\000\000\177\000\000\001" };
    res.ai_addr = &theAddr;
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(4,&fds);
    char receiveData[] = { "Received Data" };
    char *ptr = receiveData;

    EXPECT_CALL(core, Socket(_,_,_)).WillOnce(Return(4));
    EXPECT_CALL(core, SetSockOpt(_,_,_,_,_)).WillOnce(Return(0)).WillOnce(Return(0));
    EXPECT_CALL(core, GetAddrInfo(_,_, NotNull(),_)).WillOnce(DoAll(SetArgPointee<3>(&res), Return(0)));
    EXPECT_CALL(core, FreeAddrInfo(_));
    EXPECT_CALL(core, Connect(_,_,_)).WillOnce(Return(0));
    EXPECT_CALL(core, Select(_,_,_,_,_)).WillOnce(DoAll(SetArgPointee<1>(fds),Return(1))).WillRepeatedly(Return(0));
    EXPECT_CALL(core, Recv(_,_,_,_)).WillOnce(DoAll(SetArrayArgument<1>(ptr,ptr+13), Return(13)));
    EXPECT_CALL(core, Close(_)).WillOnce(Return(0));
    auto ret = app.m_socket.connectTo("localhost",5000);
    EXPECT_EQ(true,ret.m_success);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    app.m_socket.finish();

    EXPECT_EQ(app.m_receiveData,"Received Data");
}

TEST(TcpClientSocket,tcp_server_disconnect)
{
    TcpClientTestApp app;
    MockSocketCore &core = app.m_socket.getCore();
    struct addrinfo res;
    struct sockaddr theAddr = { 0, 
#ifdef __APPLE__
    0,
#endif
    "\000\000\177\000\000\001" };
    res.ai_addr = &theAddr;
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(4,&fds);
    char receiveData[] = { "Received Data" };
    char *ptr = receiveData;

    EXPECT_CALL(core, Socket(_,_,_)).WillOnce(Return(4));
    EXPECT_CALL(core, SetSockOpt(_,_,_,_,_)).WillOnce(Return(0)).WillOnce(Return(0));
    EXPECT_CALL(core, GetAddrInfo(_,_, NotNull(),_)).WillOnce(DoAll(SetArgPointee<3>(&res), Return(0)));
    EXPECT_CALL(core, FreeAddrInfo(_));
    EXPECT_CALL(core, Connect(_,_,_)).WillOnce(Return(0));
    EXPECT_CALL(core, Select(_,_,_,_,_)).WillOnce(DoAll(SetArgPointee<1>(fds),Return(1))).WillRepeatedly(Return(0));
    EXPECT_CALL(core, Recv(_,_,_,_)).WillOnce(DoAll(SetArrayArgument<1>(ptr,ptr+13), Return(0)));
    EXPECT_CALL(core, Close(_)).WillOnce(Return(0));
    auto ret = app.m_socket.connectTo("localhost",5000);
    EXPECT_EQ(true,ret.m_success);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    app.m_socket.finish();

    EXPECT_EQ(true,app.m_disconnected);
}