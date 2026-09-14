#pragma once

#include "utils.hpp"
#include <netinet/in.h>
#include <sys/socket.h>
#include <string>

class Client {
    public:
        Client(int server_fd);
        Client(int server_fd, sockaddr_in addr);
        ~Client(void);

        const t_pollfd &get_pollfd(void) const;
        const sockaddr_in &get_sockaddr_in(void) const;
        const socklen_t &get_socklen(void) const;
        std::string get_str_ip(void) const;
        static std::string get_str_ip(sockaddr_in addr);

    private:
        t_pollfd _pollfd;
        sockaddr_in _addr;
        socklen_t _addr_len;
};
