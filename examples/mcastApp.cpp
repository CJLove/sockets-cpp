#include "ISocket.h"
#include "UdpSocket.h"
#include <iostream>
#include <unistd.h>

class McastApp : public sockets::ISocket {
public:
    // UDP Multicast
    McastApp(const char *multicastAddr, uint16_t port);

    virtual ~McastApp() = default;

    void onReceiveData(const unsigned char *data, size_t size) override;

    void sendMsg(const unsigned char *data, size_t len);

private:
    sockets::UdpSocket m_mcast;
};

McastApp::McastApp(const char *multicastAddr, uint16_t port) : m_mcast(this) {
    sockets::SocketRet ret = m_mcast.startMcast(multicastAddr, port);
    if (ret.m_success) {
        std::cout << "Connected to mcast group " << multicastAddr << ":" << port << "\n";
    } else {
        std::cout << "Error: " << ret.m_msg << "\n";
    }
}

void McastApp::sendMsg(const unsigned char *data, size_t len) {
    auto ret = m_mcast.sendMsg(data, len);
    if (!ret.m_success) {
        std::cout << "Send Error: " << ret.m_msg << "\n";
    }
}

void McastApp::onReceiveData(const unsigned char *data, size_t size) {
    std::string str(reinterpret_cast<const char *>(data),size);

    std::cout << "Received: " << str << "\n";
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
            port = static_cast<uint16_t>(std::stoul(optarg));
            break;
        case '?':
            usage();
            exit(1);
        }
    }

    auto *app = new McastApp(addr, port);

    while (true) {
        std::string data;
        std::cout << "Data >";
        std::getline(std::cin, data);
        if (data == "quit") {
            break;
        }
        app->sendMsg(reinterpret_cast<const unsigned char *>(data.data()), data.size());
    }

    delete app;
}