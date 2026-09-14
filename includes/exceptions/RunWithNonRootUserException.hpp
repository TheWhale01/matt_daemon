#pragma once
#include "exceptions/MattDaemonException.hpp"

class RunWithNonRootUserException: public MattDaemonException {
    public:
        RunWithNonRootUserException(std::string const &msg): MattDaemonException(msg) {}
        RunWithNonRootUserException(std::string const &msg, Tintin_reporter &logger, LOG_LEVEL level): MattDaemonException(msg, logger, level) {}
};
