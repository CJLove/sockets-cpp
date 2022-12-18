# sockets-cpp

A header-only library with socket classes:
* `TcpClient` - template class for a TCP client socket
* `TcpServer` - template class for a TCP Server supporting multiple client connections
* `UdpSocket` - template class for a UDP Multicast/Unicast 

## Dependencies
* Optional dependency on `fmt` for error string formatting
* C++14 or later
* CMake
* gtest and gmock for unit tests. Enable unit tests by specifying `-DBUILD_TESTS=ON` when running `CMake`

# UdpSocket
The UdpSocket class is templated on the "callback" class which receives data via UDP.

The template argument type must support the following interface:
```c++
   void onReceiveData(const char *data, size_t size) ;
```

The UdpSocket class has the following interface for managing the UDP socket:
```c++
// Constructor
UdpSocket(CallbackImpl &callback, SocketOpt *options = nullptr);

// Start a multicast socket
SocketRet startMcast(const char *mcastAddr, uint16_t port);

// Start a unicast client/server socket
SocketRet startUnicast(const char *remoteAddr, uint16_t localPort, uint16_t port)

// Start a unicast server-only socket
SocketRet startUnicast(uint16_t localPort);

// Send data via UDP
SocketRet sendMsg(const char *msg, size_t size);

// Shutdown the UDP socket
SocketRet finish();
```

# TcpClient
The TcpClient class is templated on the "callback" class which receives data via TCP.

The template argument type must support the following interface:
```c++
    void onReceiveData(const char *data, size_t size);

    void onDisconnect(const sockets::SocketRet &ret);
```

The TcpClient class has the following interface for managing the TCP client connection:
```c++
// Constructor
TcpClient(CallbackImpl &callback, SocketOpt *options = nullptr);

// Connect to a TCP server
SocketRet connectTo(const char *remoteIp, uint16_t remotePort);

// Send data to the TCP server
SocketRet sendMsg(const char *msg, size_t size);

// Shutdown the TCP client socket
SocketRet finish();
```

# TcpServer
The TcpServer class is templated on the "callback" class which manages the TCP server.

The template argument must support the following interface:
```c++
    void onClientConnect(const sockets::ClientHandle &client);

    void onReceiveClientData(const sockets::ClientHandle &client, const char *data, size_t size);

    void onClientDisconnect(const sockets::ClientHandle &client, const sockets::SocketRet &ret);
```

The TcpServer class has the following interfaces:
```c++
// Create a TCP server socket
TcpServer(CallbackImpl &callback, SocketOpt *options = nullptr);

// Start the server listening on the specified port
SocketRet start(uint16_t port);

// Send a message to all connected clients
SocketRet sendBcast(const char *msg, size_t size);

// Send a message to a specific client connection
SocketRet sendClientMessage(ClientHandle &clientId, const char *msg, size_t size);

// Shutdown the TCP server socket
SocketRet finish();
```



# Sample socket apps using these classes:
Enable building sample apps by specifying `-DBUILD_EXAMPLES=ON` when running `CMake`.

## TCP Client
```bash
$ ./clientApp -a <ipAddr> -p <port>
```
## TCP Server
```bash
$ ./serverApp -p <port>
```
## UDP Multicast
```bash
$ ./mcastApp -m <multicastAddr> -p <port>
```
## UDP Unicast
```bash
$ ./unicastApp -a <ipAddr> -l <localPort> -p <remotePort>
```


