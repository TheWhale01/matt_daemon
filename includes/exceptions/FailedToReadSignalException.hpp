#pragma once
#include "exceptions/MattDaemonException.hpp"

class FailedToReadSignalException: public MattDaemonException {
    public:
        FailedToReadSignalException(std::string const &msg): MattDaemonException(msg) {}
        FailedToReadSignalException(std::string const &msg, Tintin_reporter &logger, LOG_LEVEL level): MattDaemonException(msg, logger, level) {}
};
