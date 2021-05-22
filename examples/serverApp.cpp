#include "ISocket.h"
#include "TcpServer.h"
#include <unistd.h>

class ServerApp : public sockets::ISocket {
public:
    // TCP Server
    explicit ServerApp(uint16_t port);

    virtual ~ServerApp();

    void onReceiveData(const unsigned char *data, size_t size) override;

    void onDisconnect(const sockets::SocketRet &ret) override;

    void sendMsg(const unsigned char *data, size_t len);

private:
    void threadEntry();

    bool m_stop;
    sockets::TcpServer *m_server;
    std::thread m_thread;
};

ServerApp::ServerApp(uint16_t port)
    : m_stop(false), m_server(new sockets::TcpServer(this)), m_thread(&ServerApp::threadEntry, this) {
    sockets::SocketRet ret = m_server->start(port);
    if (ret.m_success) {
        std::cout << "Server started on port " << port << "\n";
    } else {
        std::cout << "Error: " << ret.m_msg << "\n";
    }
}

void ServerApp::threadEntry() {
    while (!m_stop) {
        sockets::Client client = m_server->acceptClient(1);
        if (client.isConnected()) {
            std::cout << "Got client with IP: " << client.getIp() << "\n";
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

ServerApp::~ServerApp() {
    m_stop = true;
    m_thread.join();
    if (m_server)
        delete m_server;
}

void ServerApp::sendMsg(const unsigned char *data, size_t len) {

    if (m_server) {
        auto ret = m_server->sendMsg(data, len);
        if (!ret.m_success) {
            std::cout << "Send Error: " << ret.m_msg << "\n";
        }
    }
}

void ServerApp::onReceiveData(const unsigned char *data, size_t) {
    std::string str(reinterpret_cast<const char *>(data));

    std::cout << "Received: " << str << "\n";
}

void ServerApp::onDisconnect(const sockets::SocketRet &ret) {
    std::cout << "Disconnect: " << ret.m_msg << "\n";
}

void usage() {
    std::cout << "ServerApp -p <port>\n";
}

int main(int argc, char **argv) {
    int c = 0;
    uint16_t port = 0;
    while ((c = getopt(argc, argv, "p:?")) != EOF) {
        switch (c) {
        case 'p':
            port = static_cast<uint16_t>(std::stoul(optarg));
            break;
        case '?':
            usage();
            exit(1);
        }
    }

    ServerApp *app = new ServerApp(port);

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