#pragma once
#include "exceptions/MattDaemonException.hpp"

class UnableToOpenFile: public MattDaemonException {
    public:
        UnableToOpenFile(std::string const &msg, std::string const &filename): MattDaemonException(msg + filename) {}
};
