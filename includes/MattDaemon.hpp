#pragma once

#include <vector>
#include "Client.hpp"
#include "Tintin_reporter.hpp"
#include "utils.hpp"

class MattDaemon {
    public:
        MattDaemon(void);
        MattDaemon(Tintin_reporter const &logger);
        ~MattDaemon(void);

        void run(void);

    private:
        int _server_fd;
        int _lockfile_fd;
        bool _running;
    	static const int _server_port = 4242;
        const std::string _pid_filepath = "/var/run/matt_daemon/matt_daemon.pid";
        std::vector<Client> _clients;
        std::vector<t_pollfd> _pollfds;
        Tintin_reporter _logger;

        void _daemonize(void);
        void _lock_file(void);
        void _init_socket(void);
        void _handle_new_connection(void);
        bool _handle_client(int client_index);
        void _signal_init(void);
        void _signal_handler(int signum);
};
