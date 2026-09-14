#pragma once
#include "Tintin_reporter.hpp"
#include "exceptions/MattDaemonException.hpp"

class UnableToOpenFileException: public MattDaemonException {
    public:
        UnableToOpenFileException(std::string const &msg, std::string const &filename): MattDaemonException(msg + filename) {}
        UnableToOpenFileException(std::string const &msg, std::string const &filename, Tintin_reporter &logger, LOG_LEVEL level): MattDaemonException(msg + filename, logger, level) {}
};
