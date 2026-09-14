#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <string>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <filesystem>
#include <sys/file.h>
#include <arpa/inet.h>
#include <vector>
#include "Client.hpp"
#include "MattDaemon.hpp"
#include "Tintin_reporter.hpp"
#include "exceptions/FailedToCreateSocketException.hpp"
#include "exceptions/FailedToDeamonizeException.hpp"
#include "exceptions/FailedToPollException.hpp"
#include "exceptions/RunWithNonRootUserException.hpp"
#include "exceptions/UnableToOpenFileException.hpp"
#include "utils.hpp"

MattDaemon::MattDaemon(void): _lockfile_fd(-1), _server_fd(-1) {
    try {
        std::filesystem::create_directories(std::filesystem::path(LOCKFILE_PATH).parent_path().string());
    }
    catch (const std::filesystem::filesystem_error &e) {
        throw UnableToOpenFileException("Can't open: ", LOCKFILE_PATH);
    }
    try {
        std::filesystem::create_directories(std::filesystem::path(_pid_filepath).parent_path().string());
    }
    catch (const std::filesystem::filesystem_error &e) {
        _logger.print_log("Could not create path: " + _pid_filepath + " Will not be able to store daemon pid.", LOG_LEVEL::WARNING);
    }
}

MattDaemon::~MattDaemon(void) {
    close(_lockfile_fd);
    for (size_t i = 0; i < _clients.size(); i++)
        close(_clients[i].get_pollfd().fd);
    int _ = remove(_pid_filepath.c_str());
}

void MattDaemon::init(void) {
    _lock_file();
    _daemonize();
    _init_socket();
}

void MattDaemon::_lock_file(void) {
    if (geteuid() != 0)
        throw RunWithNonRootUserException("Could not initialize deamon. Ensure it's running as root.");
    _lockfile_fd = open(LOCKFILE_PATH, O_RDWR | O_CREAT, 0644);
    if (_lockfile_fd < 0 || flock(_lockfile_fd, LOCK_EX | LOCK_NB) == -1)
        throw UnableToOpenFileException("Could not open: ", LOCKFILE_PATH);
    _logger.print_log("Process successfully initialized !", LOG_LEVEL::INFO);
}

void MattDaemon::_init_socket(void) {
    sockaddr_in addr;

    _server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_server_fd < 0)
        throw FailedToCreateSocketException("Failed to create server socket.", _logger, LOG_LEVEL::CRITICAL);
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(_server_port);
    if (bind(_server_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(_server_fd);
        throw FailedToCreateSocketException("Failed to bind server socket.", _logger, LOG_LEVEL::CRITICAL);
    }
    if (listen(_server_fd, SOMAXCONN) < 0) {
       	close(_server_fd);
       	throw FailedToCreateSocketException("Failed to listen on server socket.", _logger, LOG_LEVEL::CRITICAL);
    }
    _clients.push_back(Client(_server_fd, addr));
    _logger.print_log("Server started at " + Client::get_str_ip(addr), LOG_LEVEL::INFO);
}

void MattDaemon::_daemonize(void) {
    pid_t first_child;
    pid_t second_child;

    first_child = fork();
    if (first_child == -1)
        throw FailedToDaemonizeException("Could not call first fork()", _logger, LOG_LEVEL::CRITICAL);
    if (first_child != 0)
        exit(EXIT_SUCCESS);
    if (setsid() < 0)
        throw FailedToDaemonizeException("Could not detach process to session", _logger, LOG_LEVEL::CRITICAL);
    second_child = fork();
    if (second_child == -1)
        throw FailedToDaemonizeException("Could not call second fork()", _logger, LOG_LEVEL::CRITICAL);
    if (second_child != 0)
        exit(EXIT_SUCCESS);
    _logger.print_log("Process successfully daemonized", LOG_LEVEL::INFO);
    pid_t pid = getpid();
    FILE *pid_fp = std::fopen(_pid_filepath.c_str(), "w");
    if (!pid_fp) {
        _logger.print_log("Could not store pid in " + _pid_filepath + " file. Continuing daemon initialization.", LOG_LEVEL::WARNING);
        return;
    }
    fprintf(pid_fp, "%d", pid);
    fclose(pid_fp);
    if (chdir("/") < 0)
        throw FailedToDaemonizeException("Could not change home directory of current process.", _logger, LOG_LEVEL::CRITICAL);
    umask(0);
    int new_stdio = open("/dev/null", O_RDWR);
    dup2(new_stdio, STDIN_FILENO);
    dup2(new_stdio, STDOUT_FILENO);
    dup2(new_stdio, STDERR_FILENO);
    if (new_stdio > STDERR_FILENO)
        close(new_stdio);
}

void MattDaemon::run(void) {
    while (true) {
        std::vector<pollfd> pollfds;

        pollfds.reserve(_clients.size());
        for (size_t i = 0; i < _clients.size(); i++)
            pollfds.push_back(_clients[i].get_pollfd());
        int res = poll(pollfds.data(), _clients.size(), -1);
        if (res < 0) {
            if (errno == EINTR)
                continue;
            throw FailedToPollException("Could not poll. Stopping daemon.", _logger, LOG_LEVEL::CRITICAL);
        }
        for (size_t i = 0; i < _clients.size(); i++) {
            if (!(pollfds[i].revents & POLLIN))
                continue;
            if (pollfds[i].fd == _server_fd)
                _handle_new_connection();
            else
                _handle_client(i);
        }
    }
}

void MattDaemon::_handle_client(int client_index) {
    char buffer[RD_BUFFER_SIZE];

    std::memset(buffer, 0, RD_BUFFER_SIZE);
    ssize_t bytes = recv(_clients[client_index].get_pollfd().fd, buffer, RD_BUFFER_SIZE - 1, 0);
    if (bytes <= 0) {
        _clients.erase(_clients.begin() + client_index);
        return ;
    }
}

void MattDaemon::_handle_new_connection(void) {
    try {
        Client client(_server_fd);

        if (_clients.size() >= NB_CLIENTS + 1) {
            const std::string msg = "Client tried to connect from: " + client.get_str_ip() + ". Max client number reached. Closing connection.";

            _logger.print_log(msg, client.get_pollfd().fd, LOG_LEVEL::ERROR);
            _logger.print_log(msg, LOG_LEVEL::ERROR);
            return ;
        }
        _clients.push_back(client);
        _logger.print_log("New client connected from: " + client.get_str_ip(), LOG_LEVEL::INFO);
    }
    catch (FailedToCreateSocketException const &e) {
        _logger.print_log("Client failed to connect.", LOG_LEVEL::ERROR);
    }
}
