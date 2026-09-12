#pragma once
#include "exceptions/MattDaemonException.hpp"

class RunWithNonRootUserException: public MattDaemonException {
    public:
        RunWithNonRootUserException(std::string const &msg): MattDaemonException(msg) {}
};
