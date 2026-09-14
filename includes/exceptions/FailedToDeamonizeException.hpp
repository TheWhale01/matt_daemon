#pragma once
#include "exceptions/MattDaemonException.hpp"

class FailedToDaemonizeException: public MattDaemonException {
    public:
        FailedToDaemonizeException(std::string const &msg): MattDaemonException(msg) {}
        FailedToDaemonizeException(std::string const &msg, Tintin_reporter &logger, LOG_LEVEL level): MattDaemonException(msg, logger, level) {}
};
