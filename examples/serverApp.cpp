#include "ISocket.h"
#include "TcpServer.h"
#include <map>
#include <unistd.h>

class ServerApp : public sockets::ISocket {
public:
    // TCP Server
    explicit ServerApp(uint16_t port);

    virtual ~ServerApp();

    void onReceiveClientData(const sockets::Client &client, const unsigned char *data, size_t size) override;

    void onClientDisconnect(const sockets::Client &client, const sockets::SocketRet &ret) override;

    void sendMsg(int idx, const unsigned char *data, size_t len);

private:
    void threadEntry();
    int  findClientIdx(const sockets::Client &client);

    bool m_stop;
    sockets::TcpServer m_server;
    std::thread m_thread;
    int m_clientIdx;
    std::map<int, sockets::Client> m_clients;
};

ServerApp::ServerApp(uint16_t port) : m_stop(false), m_server(this), m_thread(&ServerApp::threadEntry, this), m_clientIdx(0) {
    sockets::SocketRet ret = m_server.start(port);
    if (ret.m_success) {
        std::cout << "Server started on port " << port << "\n";
    } else {
        std::cout << "Error: " << ret.m_msg << "\n";
    }
}

ServerApp::~ServerApp() {
    m_stop = true;
    m_thread.join();
}

int ServerApp::findClientIdx(const sockets::Client &client)
{
    std::map<int,sockets::Client>::iterator i = m_clients.begin();
    for (; i != m_clients.end(); ++i) {
        if (i->second == client) {
            return i->first;
        }
    }
    return 0;
}

void ServerApp::threadEntry() {
    while (!m_stop) {
        sockets::Client client = m_server.acceptClient(1);
        if (client.isConnected()) {
            m_clientIdx++;
            m_clients[m_clientIdx] = client;
            std::cout << "Client " << m_clientIdx << " with IP: " << client.getIp() << "\n";
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

void ServerApp::sendMsg(int idx, const unsigned char *data, size_t len) {
    if (idx == 0) {
        auto ret = m_server.sendBcast(data, len);
        if (!ret.m_success) {
            std::cout << "Broadcast send Error: " << ret.m_msg << "\n";
        }
    } else if (m_clients.count(idx) > 0) {
        sockets::Client &client = m_clients[idx];
        auto ret = client.sendMsg(data, len);
        if (!ret.m_success) {
            std::cout << "Broadcast send Error: " << ret.m_msg << "\n";
        }
    } else {
        std::cout << "Client " << idx << " doesn't exist\n";
    }
}


void ServerApp::onReceiveClientData(const sockets::Client &client, const unsigned char *data, size_t size) {
    std::string str(reinterpret_cast<const char *>(data),size);
    int idx = findClientIdx(client);
    if (idx != 0) {
        std::cout << "Client " << idx  << " Rcvd: " << str << "\n";
    }
}

void ServerApp::onClientDisconnect(const sockets::Client &client, const sockets::SocketRet &ret) {
    
    int idx = findClientIdx(client);
    if (idx != 0) {
        std::cout << "Client " << idx << " Disconnect: " << ret.m_msg << "\n";
        m_clients.erase(idx);
    }
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
        std::getline(std::cin, data);
        if (data == "quit") {
            break;
        }
        int idx = 0;
        if (data[0] != 'B' && data[0] != 'b') {
            idx = std::stoi(data);
        }

        app->sendMsg(idx, reinterpret_cast<const unsigned char *>(data.substr(2,data.size()-2).c_str()), data.size() - 2);
    }

    delete app;
}