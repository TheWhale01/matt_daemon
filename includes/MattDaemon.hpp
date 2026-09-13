#pragma once
#include <vector>
#include "Tintin_reporter.hpp"

class MattDaemon {
    public:
        MattDaemon(void);
        MattDaemon(Tintin_reporter const &logger);
        ~MattDaemon(void);

        void init(void);
        void daemonize(void);
        void run(void);

    private:
        int _lockfile_fd;
        const std::string _pid_filepath = "./Matt_daemon.pid";
        std::vector<int> _clients;
        Tintin_reporter _logger;
};
