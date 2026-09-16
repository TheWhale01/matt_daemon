#pragma once
#include "exceptions/MattDaemonException.hpp"

class FailedToInitSignalsException: public MattDaemonException {
    public:
        FailedToInitSignalsException(std::string const &msg): MattDaemonException(msg) {}
        FailedToInitSignalsException(std::string const &msg, Tintin_reporter &logger, LOG_LEVEL level): MattDaemonException(msg, logger, level) {}
};
