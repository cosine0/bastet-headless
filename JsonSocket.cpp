//
// Created by cos on 17. 8. 29.
//

#include "JsonSocket.h"

JsonSocket::JsonSocket(uint16_t port)
{
    _socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_socket_fd < 0)
        throw "ERROR opening socket";

    sockaddr_in server_addr, client_addr;
    bzero((char *) &server_addr, sizeof server_addr);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    if (bind(_socket_fd, (sockaddr *) &server_addr, sizeof server_addr) < 0)
        throw "ERROR on binding";

    listen(_socket_fd, 1);
    socklen_t client = sizeof client_addr;
    _connection_fd = accept(_socket_fd, (sockaddr *) &client_addr, &client);
    if (_connection_fd < 0)
        throw "ERROR on accept";

    std::cout << "connected." << std::endl;
}

void JsonSocket::send(const std::string &buffer) const
{
    const char c = '\n';
    write(_connection_fd, buffer.c_str(), buffer.length());
    write(_connection_fd, &c, 1);
}

std::string JsonSocket::recv(size_t length) const
{
    std::string buffer(length, '\0');
    read(_connection_fd, &buffer[0], length);
    return buffer;
}

JsonSocket::~JsonSocket()
{
    this->close();
}

void JsonSocket::close()
{
    ::close(_connection_fd);
    ::close(_socket_fd);
}

int JsonSocket::getFd() const
{
    return _connection_fd;
}
