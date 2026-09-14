#pragma once
#include "Tintin_reporter.hpp"
#include <stdexcept>
#include <string>

class MattDaemonException : public std::runtime_error {
    public:
        MattDaemonException(std::string const &msg, Tintin_reporter &logger, LOG_LEVEL level): std::runtime_error("Matt_daemon Error: " + msg) {
            logger.print_log("Matt_daemon Error: " + msg, level);
        }
        MattDaemonException(std::string const &msg): std::runtime_error("Matt_daemon Error: " + msg) {}
};
