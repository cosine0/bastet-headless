//
// Created by cos on 17. 8. 29.
//

#ifndef BASTET_SOCKET_H
#define BASTET_SOCKET_H


#include <sys/socket.h>
#include <netinet/in.h>
#include <memory.h>
#include <iostream>
#include <unistd.h>

class JsonSocket
{
private:
    int _socket_fd;
    int _connection_fd;
public:
    explicit JsonSocket(uint16_t port=13737);
    void close();
    ~JsonSocket();
    void send(const std::string &buffer) const;
    std::string recv(size_t length=4096) const;
};


#endif //BASTET_SOCKET_H
