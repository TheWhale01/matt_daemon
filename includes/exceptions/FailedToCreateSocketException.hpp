#pragma once
#include "exceptions/MattDaemonException.hpp"

class FailedToCreateSocketException: public MattDaemonException {
    public:
        FailedToCreateSocketException(std::string const &msg): MattDaemonException(msg) {}
        FailedToCreateSocketException(std::string const &msg, Tintin_reporter &logger, LOG_LEVEL level): MattDaemonException(msg, logger, level) {}
};
