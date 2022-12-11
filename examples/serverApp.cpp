#include "TcpServer.h"
#include <set>
#include <unistd.h>

class ServerApp : public sockets::IServerSocket {
public:
    // TCP Server
    explicit ServerApp(uint16_t port);

    virtual ~ServerApp() = default;

    void onClientConnect(const sockets::ClientHandle &client) override;

    void onReceiveClientData(const sockets::ClientHandle &client, const char *data, size_t size) override;

    void onClientDisconnect(const sockets::ClientHandle &client, const sockets::SocketRet &ret) override;

    void sendMsg(int idx, const char *data, size_t len);

private:
    sockets::TcpServer m_server;
    int m_clientIdx = 0;
    std::set<int> m_clients;
};

ServerApp::ServerApp(uint16_t port) : m_server(this) {
    sockets::SocketRet ret = m_server.start(port);
    if (ret.m_success) {
        std::cout << "Server started on port " << port << "\n";
    } else {
        std::cout << "Error: " << ret.m_msg << "\n";
    }
}

void ServerApp::sendMsg(int idx, const char *data, size_t len) {
    if (idx == 0) {
        auto ret = m_server.sendBcast(data, len);
        if (!ret.m_success) {
            std::cout << "Broadcast send Error: " << ret.m_msg << "\n";
        }
    } else if (m_clients.count(idx) > 0) {
        auto ret = m_server.sendClientMessage(idx, data, len);
        if (!ret.m_success) {
            std::cout << "Send Error: " << ret.m_msg << "\n";
        }
    } else {
        std::cout << "Client " << idx << " doesn't exist\n";
    }
}

void ServerApp::onReceiveClientData(const sockets::ClientHandle &client, const char *data, size_t size) {
    std::string str(reinterpret_cast<const char *>(data), size);
    std::cout << "Client " << client << " Rcvd: " << str << "\n";
}

void ServerApp::onClientConnect(const sockets::ClientHandle &client) {
    std::cout << "Client " << client << " connection from " << m_server.getIp(client) << ":" << m_server.getPort(client)
              << "\n";
    m_clients.insert(client);
}

void ServerApp::onClientDisconnect(const sockets::ClientHandle &client, const sockets::SocketRet &ret) {
    std::cout << "Client " << client << " Disconnect: " << ret.m_msg << "\n";
    m_clients.erase(client);
}

void usage() {
    std::cout << "ServerApp -p <port>\n";
}

int main(int argc, char **argv) {
    int arg = 0;
    uint16_t port = 0;
    while ((arg = getopt(argc, argv, "p:?")) != EOF) {    // NOLINT
        switch (arg) {
        case 'p':
            port = static_cast<uint16_t>(std::stoul(optarg));
            break;
        case '?':
            usage();
            exit(1);    // NOLINT
        }
    }

    auto *app = new ServerApp(port);

    while (true) {
        std::string data;
        std::cout << "Data >";
        std::getline(std::cin, data);
        if (data == "quit") {
            break;
        }
        int idx = 0;
        if (data[0] != 'B' && data[0] != 'b') {
            try {
                idx = std::stoi(data);
            } catch (...) { continue; }
        }

        app->sendMsg(idx, data.substr(2, data.size() - 2).c_str(), data.size() - 2);
    }

    delete app;
}