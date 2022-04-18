#include "BaseSocket.hpp"
#include <iostream>
#include <cstring>
#include <vector>

using namespace std;
#define MAX_LIMIT 65536

MySocket::MySocket(int _socket_fd, const char *_hostname, const char *_port, struct addrinfo _host_info)
    : socket_fd(_socket_fd), hostname(_hostname), port(_port), host_info(_host_info) {
    
}

MySocket::MySocket(struct addrinfo _host_info, const char *_hostname, const char *_port)
    : hostname(_hostname), port(_port), host_info(_host_info) {
    struct addrinfo *host_info_list;
    int status = getaddrinfo(hostname, port,
                    &host_info, &host_info_list);
    if (status != 0) {
        cerr << "Failed: Setup getaddrinfo" << endl;
        throw GetAddrInfoFailure();
    }

    int socketTemp = createSocket(host_info_list->ai_family,
                                host_info_list->ai_socktype,
                                host_info_list->ai_protocol);
    setSocketFd(socketTemp);
    freeaddrinfo(host_info_list);
}

MySocket::~MySocket() {
    close(socket_fd);
}

int MySocket::createSocket(int _domain, int _type, int _protocol) {
    return socket(_domain, _type, _protocol);
}

// bind to this socket
// on failure, throw SocketBindFailure or GetAddrInfoFailure;
void MySocket::bindSocket() {
    // get address information for host
    struct addrinfo *host_info_list;

    int status = getaddrinfo(hostname, port,
                    &host_info, &host_info_list);
    if (status == -1) {
        cout << "Failed: Setup getaddrinfo" << endl;
        throw GetAddrInfoFailure();
    }

    int yes = 1;
    status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    freeaddrinfo(host_info_list);

    if (status == -1) {
        cout << "Failed: Bind to socket" << endl;
        throw SocketBindFailure();
    }
}
// listen to this socket
// on failure, throw SocketListenFailure();
void MySocket::listenSocket(int backlog) {
    int status = listen(socket_fd, backlog);
    if (status == -1) {
        throw SocketListenFailure();
    }
}
// connect to this socket
// on failure, throw SocketConnectFailure();
void MySocket::connectSocket() {
    int status;
    struct addrinfo *host_info_list;

    status = getaddrinfo(hostname, port, &host_info, &host_info_list);
    if (status != 0) {
       throw GetAddrInfoFailure();
    }

    status = connect(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
        throw SocketConnectFailure();
    }
    freeaddrinfo(host_info_list);
}

MySocket *MySocket::acceptConnection() {
    struct sockaddr_storage socket_addr;
    socklen_t socket_addr_len = sizeof(socket_addr);

    // create a new socket for this connection
    int client_connection_fd;
    client_connection_fd = accept(socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
    if (client_connection_fd == -1) {
        cerr << "Accept incoming connection failure\n";
        throw AcceptConnectFailure();
    }

    MySocket *client_sock = new MySocket(client_connection_fd, hostname, port, host_info);
    // cout << "Accept connection!!!\n";
    return client_sock;
}

void MySocket::sendData(std::string message) {
    int status = send(socket_fd, message.c_str(), message.length(), 0);
    if (status == -1) {
        cerr << "Cannot send message\n";
        throw SendResponseFailure();
    }
}

string MySocket::receiveData() {
    vector<char> result(MAX_LIMIT);

	// peek at the HTTP head
	// int size_peek = recv(s , &result.data()[0] , MAX_LIMIT , MSG_PEEK);
	// get the content length if has one

	ssize_t size_recv = recv(socket_fd , &result.data()[0] , MAX_LIMIT , 0);
	// if client close the connection
	if (size_recv == 0) {
		cout << "The other side close connection\n";
		return string();
	} else if (size_recv < 0) {
		cout << "Something went wrong\n";
		return string();
		//exit(EXIT_FAILURE);
	} 
	// cout << "Received data with size: " << size_recv << '\n';
	
	return string(result.begin(), result.begin() + size_recv);
}

int MySocket::getSocketFd() const {
    return socket_fd;
}
const char *MySocket::getHostName() const {
    return hostname;
}
const char *MySocket::getPort() const {
    return port;
}

void MySocket::setSocketFd(int s) {
    if (s == -1) {
        throw SocketCreateFailure();
    }
    socket_fd = s;
}
void MySocket::setHostName(const char *_hostname) {
    hostname = _hostname;
}
void MySocket::setPort(const char *_port) {
    port = _port;
}
