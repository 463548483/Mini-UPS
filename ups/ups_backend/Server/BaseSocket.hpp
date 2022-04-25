#ifndef BASESOCKET_H
#define BASESOCKET_H

#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <exception>
#include <string>

class SocketCreateFailure : public std::exception
{
public:    
    virtual const char* what() const noexcept {
       return "Fail to create socket.";
    }
};

class SocketConnectFailure : public std::exception
{
public:    
    virtual const char* what() const noexcept {
       return "Fail to connect socket.";
    }
};

class SocketBindFailure : public std::exception
{
public:
    virtual const char* what() const noexcept {
       return "Fail to bind the socket to certain port.";
    }
};

class SendResponseFailure : public std::exception
{
public:
    virtual const char* what() const noexcept {
       return "Fail to back the response.";
    }
};

class ReceiveDataFailure : public std::exception
{
public:
    virtual const char* what() const noexcept {
       return "Fail to receive data from client socket.";
    }
};

class GetIpFailure : public std::exception
{
public:
    virtual const char* what() const noexcept {
       return "Fail to get the ip address from socket.";
    }
};

class SocketListenFailure : public std::exception {
public:
    virtual const char* what() const noexcept {
       return "Fail to listening to the socket to.";
    }
};

class AcceptConnectFailure : public std::exception
{
public:
    virtual const char* what() const noexcept {
        return "Fail to accept client connections.";
    }
};

class GetAddrInfoFailure : public std::exception
{
public:
    virtual const char* what() const noexcept {
        return "Fail to get address info for host.";
    }
};

class MySocket {
private:
    // socket file descripter
    int socket_fd;
    const char *hostname;
    const char *port;
    struct addrinfo host_info;

    int createSocket(int domain, int type, int protocol);
public:
    MySocket(int _socket_fd, const char *_hostname, const char *_port, struct addrinfo host_info);
    MySocket(struct addrinfo host_info, const char *hostname, const char *port);
    ~MySocket();

    // bind to this socket
    // on failure, throw SocketBindFailure();
    void bindSocket();
    // listen to this socket
    // on failure, throw SocketListenFailure();
    void listenSocket(int backlog);
    // connect to this socket
    // on failure, throw SocketConnectFailure();
    void connectSocket();
    // accept connection to the socket, and create a new socket to handle that
    MySocket *acceptConnection();
    // receive data from this socket
    std::string receiveData();
    // send data to this socket
    void sendData(std::string);

    // getter function
    const char *getHostName() const;
    const char *getPort() const;
    int getSocketFd() const;

    // setter funciton
    void setSocketFd(int);
    void setHostName(const char *);
    void setPort(const char *);
};

#endif /* BASESOCKET_H */
