#pragma once
#include "exceptions/MattDaemonException.hpp"

class FailedToDaemonizeException: public MattDaemonException {
    public:
        FailedToDaemonizeException(std::string const &msg): MattDaemonException(msg) {}
};
