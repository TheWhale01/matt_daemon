#include <arpa/inet.h>
#include <cstring>
#include <netinet/in.h>
#include <string>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include "Client.hpp"
#include "MattDaemon.hpp"
#include "exceptions/FailedToCreateSocketException.hpp"

Client::Client(int server_fd): _addr_len(sizeof(_addr)) {
    _pollfd.fd = accept(server_fd, reinterpret_cast<sockaddr*>(&_addr), &_addr_len);
    if (_pollfd.fd < 0)
        throw FailedToCreateSocketException("Could not get client fd.");
    _pollfd.events = POLLIN;
    _pollfd.revents = 0;
}

Client::Client(int server_fd, sockaddr_in addr): _addr(addr), _addr_len(sizeof(addr)) {
    _pollfd.fd = server_fd;
    _pollfd.events = POLLIN;
    _pollfd.revents = 0;
}

Client::~Client(void) {
    close(_pollfd.fd);
}

const t_pollfd &Client::get_pollfd(void) const {
    return _pollfd;
}

const sockaddr_in &Client::get_sockaddr_in(void) const {
    return _addr;
}

const socklen_t &Client::get_socklen(void) const {
    return _addr_len;
}

std::string Client::get_str_ip(sockaddr_in addr) {
    char buff[INET_ADDRSTRLEN];

    std::memset(buff, 0, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &addr.sin_addr, buff, sizeof(buff));
    return std::string(buff) + ":" + std::to_string(htons(addr.sin_port));
}

std::string Client::get_str_ip(void) const {
    return Client::get_str_ip(_addr);
}
