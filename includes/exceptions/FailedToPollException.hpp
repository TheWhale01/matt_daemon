#pragma once
#include "exceptions/MattDaemonException.hpp"

class FailedToPollException: public MattDaemonException {
    public:
        FailedToPollException(std::string const &msg): MattDaemonException(msg) {}
        FailedToPollException(std::string const &msg, Tintin_reporter &logger, LOG_LEVEL level): MattDaemonException(msg, logger, level) {}
};
