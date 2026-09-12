#pragma once
#include <stdexcept>
#include <string>

class MattDaemonException : public std::runtime_error {
    public:
        MattDaemonException(std::string const &msg): std::runtime_error("Matt_daemon Error: " + msg) {}
};
