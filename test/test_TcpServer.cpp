#include "gtest/gtest.h"
#include "gmock/gmock.h"
#define TEST_CORE_ACCESS
#include "TcpServer.h"
#include "MockSocketCore.h"
#include <map>

using ::testing::AtLeast;
using ::testing::Return;
using ::testing::NotNull;
using ::testing::_;
using ::testing::SetArgPointee;
using ::testing::SetArrayArgument;
using ::testing::DoAll;

class TcpServerTestApp {
public:
    TcpServerTestApp(): m_socket(*this)
    {}

    TcpServerTestApp(sockets::SocketOpt *opts): m_socket(*this,opts)
    {}

    ~TcpServerTestApp() = default;

    void onClientConnect(const sockets::ClientHandle &client);

    void onReceiveClientData(const sockets::ClientHandle &client, const char *data, size_t size);

    void onClientDisconnect(const sockets::ClientHandle &client, const sockets::SocketRet &ret);

    sockets::TcpServer<TcpServerTestApp,MockSocketCore> m_socket;

    std::map<sockets::ClientHandle, std::string> m_receiveData;

    std::set<sockets::ClientHandle> m_clients;

};

void TcpServerTestApp::onClientConnect(const sockets::ClientHandle &client) {
    m_clients.insert(client);
}

void TcpServerTestApp::onReceiveClientData(const sockets::ClientHandle &client, const char *data, size_t size) {
    m_receiveData[client] = std::string(data,size);
}

void TcpServerTestApp::onClientDisconnect(const sockets::ClientHandle &client, const sockets::SocketRet &) {
    m_clients.erase(client);
}

TEST(TcpServerSocket,start_socket_fail)
{
    TcpServerTestApp app;
    MockSocketCore &core = app.m_socket.getCore();

    EXPECT_CALL(core, Socket(_,_,_)).WillOnce(Return(-1));
    auto ret = app.m_socket.start(5000);
    EXPECT_EQ(false,ret.m_success);
}

TEST(TcpServerSocket,start_setsockopt_fail1)
{
    TcpServerTestApp app;
    MockSocketCore &core = app.m_socket.getCore();

    EXPECT_CALL(core, Socket(_,_,_)).WillOnce(Return(4));
    EXPECT_CALL(core, SetSockOpt(_,_,_,_,_)).WillOnce(Return(-1));
    EXPECT_CALL(core, Close(_)).WillOnce(Return(0));
    auto ret = app.m_socket.start(5000);
    EXPECT_EQ(false,ret.m_success);
}

TEST(TcpServerSocket,start_setsockopt_fail2)
{
    TcpServerTestApp app;
    MockSocketCore &core = app.m_socket.getCore();

    EXPECT_CALL(core, Socket(_,_,_)).WillOnce(Return(4));
    EXPECT_CALL(core, SetSockOpt(_,_,_,_,_)).WillOnce(Return(0)).WillOnce(Return(-1));
    EXPECT_CALL(core, Close(_)).WillOnce(Return(0));
    auto ret = app.m_socket.start(5000);
    EXPECT_EQ(false,ret.m_success);
}

TEST(TcpServerSocket,start_setsockopt_fail3)
{
    TcpServerTestApp app;
    MockSocketCore &core = app.m_socket.getCore();

    EXPECT_CALL(core, Socket(_,_,_)).WillOnce(Return(4));
    EXPECT_CALL(core, SetSockOpt(_,_,_,_,_)).WillOnce(Return(0)).WillOnce(Return(0)).WillOnce(Return(-1));
    EXPECT_CALL(core, Close(_)).WillOnce(Return(0));
    auto ret = app.m_socket.start(5000);
    EXPECT_EQ(false,ret.m_success);
}

TEST(TcpServerSocket,start_bind_fail)
{
    TcpServerTestApp app;
    MockSocketCore &core = app.m_socket.getCore();

    EXPECT_CALL(core, Socket(_,_,_)).WillOnce(Return(4));
    EXPECT_CALL(core, SetSockOpt(_,_,_,_,_)).WillOnce(Return(0)).WillOnce(Return(0)).WillOnce(Return(0));
    EXPECT_CALL(core, Bind(_,_,_)).WillOnce(Return(-1));
    EXPECT_CALL(core, Close(_)).WillOnce(Return(0));
    auto ret = app.m_socket.start(5000);
    EXPECT_EQ(false,ret.m_success);
}

TEST(TcpServerSocket,start_listen_fail)
{
    TcpServerTestApp app;
    MockSocketCore &core = app.m_socket.getCore();

    EXPECT_CALL(core, Socket(_,_,_)).WillOnce(Return(4));
    EXPECT_CALL(core, SetSockOpt(_,_,_,_,_)).WillOnce(Return(0)).WillOnce(Return(0)).WillOnce(Return(0));
    EXPECT_CALL(core, Bind(_,_,_)).WillOnce(Return(0));
    EXPECT_CALL(core, Listen(_,_)).WillOnce(Return(-1));
    EXPECT_CALL(core, Close(_)).WillOnce(Return(0));
    auto ret = app.m_socket.start(5000);
    EXPECT_EQ(false,ret.m_success);
}

TEST(TcpServerSocket,start_success)
{
    TcpServerTestApp app;
    MockSocketCore &core = app.m_socket.getCore();

    EXPECT_CALL(core, Socket(_,_,_)).WillOnce(Return(4));
    EXPECT_CALL(core, SetSockOpt(_,_,_,_,_)).WillOnce(Return(0)).WillOnce(Return(0)).WillOnce(Return(0));
    EXPECT_CALL(core, Bind(_,_,_)).WillOnce(Return(0));
    EXPECT_CALL(core, Listen(_,_)).WillOnce(Return(0));
    EXPECT_CALL(core, Select(_,_,_,_,_)).WillRepeatedly(Return(0));
    EXPECT_CALL(core, Close(_)).WillOnce(Return(0));
    auto ret = app.m_socket.start(5000);
    EXPECT_EQ(true,ret.m_success);

    sleep(1);
    ret = app.m_socket.finish();
    EXPECT_EQ(true,ret.m_success);
}

TEST(TcpServerSocket,client_connect_send)
{
    TcpServerTestApp app;
    MockSocketCore &core = app.m_socket.getCore();
    struct sockaddr client_addr = { 0, "\000\000\177\000\000\001" };
    struct sockaddr *ptr = &client_addr;
    struct sockaddr *endPtr = ptr + 1;
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(4,&fds);
    EXPECT_CALL(core, Socket(_,_,_)).WillOnce(Return(4));
    EXPECT_CALL(core, SetSockOpt(_,_,_,_,_)).WillOnce(Return(0)).WillOnce(Return(0)).WillOnce(Return(0));
    EXPECT_CALL(core, Bind(_,_,_)).WillOnce(Return(0));
    EXPECT_CALL(core, Listen(_,_)).WillOnce(Return(0));
    EXPECT_CALL(core, Select(_,_,_,_,_)).WillOnce(DoAll(SetArgPointee<1>(fds),Return(1))).WillRepeatedly(Return(0));
    EXPECT_CALL(core, Accept(_,_,_)).WillOnce(DoAll(SetArrayArgument<1>(ptr,endPtr),Return(5)));
    EXPECT_CALL(core, Send(_,_,_,_)).WillOnce(Return(-1)).WillOnce(Return(5)).WillOnce(Return(12));

    EXPECT_CALL(core, Close(_)).WillRepeatedly(Return(0));
    auto ret = app.m_socket.start(5000);
    EXPECT_EQ(true,ret.m_success);

    sleep(1);
    sockets::ClientHandle badHandle = 3;
    sockets::ClientHandle goodHandle = 5;

    // Bad Client handle
    ret = app.m_socket.sendClientMessage(badHandle,"Message Data",12);
    EXPECT_EQ(false,ret.m_success);

    // Good Client handle, send() fails
    ret = app.m_socket.sendClientMessage(goodHandle,"Message Data",12);
    EXPECT_EQ(false,ret.m_success);

    // Good client handle, partial send()
    ret = app.m_socket.sendClientMessage(goodHandle,"Message Data",12);
    EXPECT_EQ(false,ret.m_success);

    ret = app.m_socket.sendClientMessage(goodHandle,"Message Data",12);
    EXPECT_EQ(true,ret.m_success);   

    sleep(1);
    ret = app.m_socket.finish();
    EXPECT_EQ(true,ret.m_success);

    EXPECT_EQ(true,(app.m_clients.find(5) != app.m_clients.end()));
}

TEST(TcpServerSocket,client_connect_receive_disconnect)
{
    TcpServerTestApp app;
    MockSocketCore &core = app.m_socket.getCore();
    struct sockaddr client_addr = { 0, "\000\000\177\000\000\001" };
    struct sockaddr *ptr = &client_addr;
    struct sockaddr *endPtr = ptr + 1;
    fd_set acceptFds;
    FD_ZERO(&acceptFds);
    FD_SET(4,&acceptFds);
    fd_set recvFds;
    FD_ZERO(&recvFds);
    FD_SET(5,&recvFds);
    char receiveData[] = { "Received Data" };
    char *dataPtr = receiveData;
    EXPECT_CALL(core, Socket(_,_,_)).WillOnce(Return(4));
    EXPECT_CALL(core, SetSockOpt(_,_,_,_,_)).WillOnce(Return(0)).WillOnce(Return(0)).WillOnce(Return(0));
    EXPECT_CALL(core, Bind(_,_,_)).WillOnce(Return(0));
    EXPECT_CALL(core, Listen(_,_)).WillOnce(Return(0));
    EXPECT_CALL(core, Select(_,_,_,_,_)).WillOnce(DoAll(SetArgPointee<1>(acceptFds),Return(1))).WillOnce(DoAll(SetArgPointee<1>(recvFds),Return(1))).WillOnce(DoAll(SetArgPointee<1>(recvFds),Return(1))).WillRepeatedly(Return(0));
    EXPECT_CALL(core, Accept(_,_,_)).WillOnce(DoAll(SetArrayArgument<1>(ptr,endPtr),Return(5)));
    EXPECT_CALL(core, Recv(_,_,_,_)).WillOnce(DoAll(SetArrayArgument<1>(dataPtr,dataPtr+13), Return(13))).WillOnce(DoAll(SetArrayArgument<1>(dataPtr,dataPtr+13), Return(0)));
    EXPECT_CALL(core, Close(_)).WillRepeatedly(Return(0));
    auto ret = app.m_socket.start(5000);
    EXPECT_EQ(true,ret.m_success);

    sleep(1);
    ret = app.m_socket.finish();
    EXPECT_EQ(true,ret.m_success);

    EXPECT_EQ(app.m_receiveData[5],"Received Data");
    EXPECT_EQ(true,(app.m_clients.find(5) == app.m_clients.end()));
}