#include <cerrno>
#include <csignal>
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
#include <sys/signalfd.h>
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
#include "exceptions/FailedToInitSignalsException.hpp"
#include "exceptions/FailedToPollException.hpp"
#include "exceptions/RunWithNonRootUserException.hpp"
#include "exceptions/UnableToOpenFileException.hpp"
#include "utils.hpp"

MattDaemon::MattDaemon(void): _server_fd(-1), _lockfile_fd(-1) {
    if (geteuid() != 0)
        throw RunWithNonRootUserException("Could not initialize deamon. Ensure it's running as root.");
    try {
        std::filesystem::create_directories(std::filesystem::path(LOCKFILE_PATH).parent_path().string());
    }
    catch (const std::filesystem::filesystem_error &e) {
        throw UnableToOpenFileException("Can't open: ", LOCKFILE_PATH);
    }
    _lock_file();
    _logger.init();
    try {
        std::filesystem::create_directories(std::filesystem::path(_pid_filepath).parent_path().string());
    }
    catch (const std::filesystem::filesystem_error &e) {
        _logger.print_log("Could not create path: " + _pid_filepath + " Will not be able to store daemon pid.", LOG_LEVEL::WARNING);
    }
    _running = _daemonize();
    if (!_running)
        return ;
    _init_socket();
    _init_signal();
}

MattDaemon::~MattDaemon(void) {
    close(_lockfile_fd);
    remove(_pid_filepath.c_str());
}

void MattDaemon::_init_signal(void) {
    sigset_t mask;

    sigemptyset(&mask);
    sigaddset(&mask, SIGTERM);
    if (sigprocmask(SIG_BLOCK, &mask, nullptr) < 0)
        throw FailedToInitSignalsException("Could not initialize signals.");
    _signal_fd = signalfd(-1, &mask, SFD_CLOEXEC);
    if (_signal_fd < 0)
        throw FailedToInitSignalsException("Could not initialize signals.");
    Client client(_signal_fd, {});
    _pollfds.push_back(client.get_pollfd());
    _clients.push_back(std::move(client));
}

void MattDaemon::_lock_file(void) {
    _lockfile_fd = open(LOCKFILE_PATH, O_RDWR | O_CREAT, 0644);
    if (_lockfile_fd < 0 || flock(_lockfile_fd, LOCK_EX | LOCK_NB) == -1)
        throw UnableToOpenFileException("Could not open: ", LOCKFILE_PATH);
    _logger.print_log("Process successfully initialized !", STDOUT_FILENO, LOG_LEVEL::INFO);
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
    Client server_client(_server_fd, addr);
    _pollfds.push_back(server_client.get_pollfd());
    _clients.push_back(std::move(server_client));
    _logger.print_log("Server started at " + Client::get_str_ip(addr), LOG_LEVEL::INFO);
}

bool MattDaemon::_daemonize(void) {
    pid_t current_pid;
    pid_t first_child;
    pid_t second_child;
    FILE *current_pid_fp;

    first_child = fork();
    if (first_child == -1)
        throw FailedToDaemonizeException("Could not call first fork()", _logger, LOG_LEVEL::CRITICAL);
    if (first_child != 0)
        return false;
    if (setsid() < 0)
        throw FailedToDaemonizeException("Could not detach process to session", _logger, LOG_LEVEL::CRITICAL);
    second_child = fork();
    if (second_child == -1)
        throw FailedToDaemonizeException("Could not call second fork()", _logger, LOG_LEVEL::CRITICAL);
    if (second_child != 0)
        return false;
    _logger.print_log("Process successfully daemonized", LOG_LEVEL::INFO);
    current_pid = getpid();
    current_pid_fp = std::fopen(_pid_filepath.c_str(), "w");
    if (!current_pid_fp) {
        _logger.print_log("Could not store pid in " + _pid_filepath + " file. Continuing daemon initialization.", LOG_LEVEL::WARNING);
        return true;
    }
    fprintf(current_pid_fp, "%d", current_pid);
    fclose(current_pid_fp);
    if (chdir("/") < 0)
        throw FailedToDaemonizeException("Could not change home directory of current process.", _logger, LOG_LEVEL::CRITICAL);
    umask(0);
    int new_stdio = open("/dev/null", O_RDWR);
    dup2(new_stdio, STDIN_FILENO);
    dup2(new_stdio, STDOUT_FILENO);
    dup2(new_stdio, STDERR_FILENO);
    if (new_stdio > STDERR_FILENO)
        close(new_stdio);
    return true;
}

void MattDaemon::run(void) {
    while (_running) {
        int res = poll(_pollfds.data(), _pollfds.size(), -1);
        if (res < 0) {
            if (errno == EINTR)
                continue;
            throw FailedToPollException("Could not poll. Stopping daemon.", _logger, LOG_LEVEL::CRITICAL);
        }
        for (size_t i = 0; i < _pollfds.size(); i++) {
            if (!(_pollfds[i].revents & POLLIN))
                continue;
            if (_pollfds[i].fd == _server_fd)
                _handle_new_connection();
            else if (_pollfds[i].fd == _signal_fd)
                _handle_signal();
            else
                _running = _handle_client(i);
        }
    }
    for (size_t i = 0; i < _pollfds.size(); i++) {
        if (_pollfds[i].fd != _server_fd) {
            _logger.print_log("Server shutdown.", _pollfds[i].fd, LOG_LEVEL::INFO);
        }
    }
    if (_server_fd != -1)
        _logger.print_log("Server shutdown.", LOG_LEVEL::INFO);
}

bool MattDaemon::_handle_client(int client_index) {
    char buffer[RD_BUFFER_SIZE];

    std::memset(buffer, 0, RD_BUFFER_SIZE);
    ssize_t bytes = recv(_clients[client_index].get_pollfd().fd, buffer, RD_BUFFER_SIZE - 1, 0);
    if (bytes <= 0) {
        _logger.print_log("Client at " + _clients[client_index].get_str_ip() + " disconnected", LOG_LEVEL::INFO);
        _pollfds.erase(_pollfds.begin() + client_index);
        _clients.erase(_clients.begin() + client_index);
        return true;
    }
    while (bytes > 0 && (buffer[bytes - 1] == '\n' || buffer[bytes - 1] == '\r')) {
        buffer[bytes - 1] = '\0';
        bytes--;
    }
    std::string msg(buffer);
    _logger.print_log("Client at " + _clients[client_index].get_str_ip() + " sent `" + msg + "`", LOG_LEVEL::INFO);
    return !(msg.size() == 4 && msg == "quit");
}

void MattDaemon::_handle_new_connection(void) {
    std::string msg;

    try {
        Client client(_server_fd);

        if (_clients.size() >= NB_CLIENTS + 1) {
            msg = "Client tried to connect from: " + client.get_str_ip() + ". Max client number reached. Closing connection.";

            _logger.print_log(msg, client.get_pollfd().fd, LOG_LEVEL::ERROR);
            _logger.print_log(msg, LOG_LEVEL::ERROR);
            return ;
        }
        msg = "New client connected from: " + client.get_str_ip();
        _logger.print_log(msg, LOG_LEVEL::INFO);
        _logger.print_log(msg, client.get_pollfd().fd, LOG_LEVEL::INFO);
        _pollfds.push_back(client.get_pollfd());
        _clients.push_back(std::move(client));
    }
    catch (FailedToCreateSocketException const &e) {
        _logger.print_log("Client failed to connect.", LOG_LEVEL::ERROR);
    }
}

void MattDaemon::_handle_signal(void) {
    signalfd_siginfo siginfo;

    ssize_t size = read(_signal_fd, &siginfo, sizeof(siginfo));
    if (size != sizeof(siginfo)) {
        _logger.print_log("Could not read signal.", LOG_LEVEL::ERROR);
        return ;
    }
   _logger.print_log("Signal: " + std::to_string(siginfo.ssi_signo) + " received !", LOG_LEVEL::INFO);
}
