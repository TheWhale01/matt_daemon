#pragma once
#include <vector>
#include "Tintin_reporter.hpp"

class MattDaemon {
    public:
        MattDaemon(void);
        MattDaemon(Tintin_reporter const &logger);
        ~MattDaemon(void);

        void init(void);

    private:
        int _logfile_fd;
        std::vector<int> _clients;
        Tintin_reporter _logger;
};
