#pragma once
#include "exceptions/MattDaemonException.hpp"

class FailedToDaemonize: public MattDaemonException {
    public:
        FailedToDaemonize(std::string const &msg): MattDaemonException(msg) {}
};
