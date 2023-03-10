#include "TcpClient.h"
#ifdef _WIN32
    #include "getopt.h"
#else
    #include <unistd.h>
#endif

class ClientApp {
public:
    // TCP Client
    ClientApp(const char *remoteIp, uint16_t port);

    virtual ~ClientApp() = default;

    void onReceiveData(const char *data, size_t size);

    void onDisconnect(const sockets::SocketRet &ret);

    void sendMsg(const char *data, size_t len);

private:
    sockets::TcpClient<ClientApp> m_client;
};

ClientApp::ClientApp(const char *remoteIp, uint16_t port) : m_client(*this) {
    while (true) {
        sockets::SocketRet ret = m_client.connectTo(remoteIp, port);
        if (ret.m_success) {
            std::cout << "Connected to " << remoteIp << ":" << port << "\n";
            break;
        } else {
            std::cout << ret.m_msg << "\n";
            exit(1);   // NOLINT
        }
    }
}

void ClientApp::sendMsg(const char *data, size_t len) {
    auto ret = m_client.sendMsg(data, len);
    if (!ret.m_success) {
        std::cout << "Send Error: " << ret.m_msg << "\n";
    }
}

void ClientApp::onReceiveData(const char *data, size_t size) {
    std::string str(data, size);

    std::cout << "Received: " << str << "\n";
}

void ClientApp::onDisconnect(const sockets::SocketRet &ret) {
    std::cout << "Disconnect: " << ret.m_msg << "\n";
    exit(0);    // NOLINT 
}

void usage() {
    std::cout << "ClientApp [-a <addr>][-p <port>]\n";
}

int main(int argc, char **argv) {
    int arg = 0;
    const char *addr = nullptr;
    uint16_t port = 0;
    while ((arg = getopt(argc, argv, "a:p:?")) != EOF) {  // NOLINT
        switch (arg) {
        case 'a':
            addr = optarg;
            break;
        case 'p':
            port = static_cast<uint16_t>(std::stoul(optarg));
            break;
        case '?':
            usage();
            exit(1);    // NOLINT
        }
    }

    auto *app = new ClientApp(addr, port);

    while (true) {
        std::string data;
        std::cout << "Data >";
        std::getline(std::cin, data);
        if (data == "quit") {
            break;
        }
        app->sendMsg(data.data(), data.size());
    }

    delete app;
}