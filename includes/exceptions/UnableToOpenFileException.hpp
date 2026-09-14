#pragma once
#include "exceptions/MattDaemonException.hpp"

class UnableToOpenFileException: public MattDaemonException {
    public:
        UnableToOpenFileException(std::string const &msg, std::string const &filename): MattDaemonException(msg + filename) {}
};
