# sockets-cpp

## Library with socket classes:
* `TcpClient` 
* `TcpServer`
* `UdpSocket` - UDP Multicast/Unicast

## Dependencies
* Optional dependency on `fmt` for error string formatting

## Sample socket apps using these classes:
### TCP Client
```bash
$ ./clientApp -a <ipAddr> -p <port>
```
### TCP Server
```bash
$ ./serverApp -p <port>
```
### UDP Multicast
```bash
$ ./mcastApp -m <multicastAddr> -p <port>
```
### UDP Unicast
```bash
$ ./unicastApp -a <ipAddr> -l <localPort> -p <remotePort>
```


