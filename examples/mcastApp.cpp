#include "ISocket.h"
#include "UdpMcast.h"
#include <iostream>
#include <unistd.h>

class McastApp : public sockets::ISocket {
public:
    // UDP Multicast
    McastApp(const char *multicastAddr, uint16_t port);

    ~McastApp();

    void onReceiveData(const unsigned char *data, size_t size) override;

    void onDisconnect(const sockets::SocketRet &ret) override;

    void sendMsg(const unsigned char *data, size_t len);

private:
    sockets::UdpMcast *m_mcast;
};

McastApp::McastApp(const char *multicastAddr, uint16_t port) : m_mcast(new sockets::UdpMcast(this)) {
    sockets::SocketRet ret = m_mcast->start(multicastAddr, port);
    if (ret.m_success) {
        std::cout << "Connected to mcast group " << multicastAddr << ":" << port << "\n";
    } else {
        std::cout << "Error: " << ret.m_msg << "\n";
    }
}

McastApp::~McastApp() {
    if (m_mcast)
        delete m_mcast;
}

void McastApp::sendMsg(const unsigned char *data, size_t len) {
    if (m_mcast) {
        auto ret = m_mcast->sendMsg(data, len);
        if (!ret.m_success) {
            std::cout << "Send Error: " << ret.m_msg << "\n";
        }
    }
}

void McastApp::onReceiveData(const unsigned char *data, size_t size) {
    std::string str(reinterpret_cast<const char *>(data));

    std::cout << "Received: " << str << "\n";
}

void McastApp::onDisconnect(const sockets::SocketRet &ret) {
    std::cout << "Disconnect: " << ret.m_msg << "\n";
}

void usage() {
    std::cout << "McastApp -m <mcastAddr> -p <port>\n";
}

int main(int argc, char **argv) {
    int c = 0;
    const char *addr = nullptr;
    uint16_t port = 0;
    while ((c = getopt(argc, argv, "m:p:?")) != EOF) {
        switch (c) {
        case 'm':
            addr = optarg;
            break;
        case 'p':
            port = std::stoi(optarg);
            break;
        case '?':
            usage();
            exit(1);
        }
    }

    McastApp *app = new McastApp(addr, port);

    while (true) {
        std::string data;
        std::cout << "Data >";
        std::cin >> data;
        if (data == "quit")
            break;
        app->sendMsg(reinterpret_cast<const unsigned char *>(data.data()), data.size());
    }

    delete app;
}