#include <asm-generic/socket.h>
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
#include "exceptions/FailedToPollException.hpp"
#include "exceptions/RunWithNonRootUserException.hpp"
#include "exceptions/UnableToOpenFileException.hpp"
#include "utils.hpp"

volatile sig_atomic_t g_signum = -1;

MattDaemon::MattDaemon(void): _server_fd(-1), _lockfile_fd(-1), _exit_child(false), _logger("/var/log/matt_daemon/matt_daemon.log") {
    if (geteuid() != 0)
        throw RunWithNonRootUserException("Could not initialize deamon. Ensure it's running as root.");
    try {
        std::filesystem::create_directories(std::filesystem::path(_lockfile_path).parent_path().string());
    }
    catch (const std::filesystem::filesystem_error &e) {
        throw UnableToOpenFileException("Can't open: ", _lockfile_path);
    }
    _lock_file();
    _logger.init();
    try {
        std::filesystem::create_directories(std::filesystem::path(_pid_filepath).parent_path().string());
    }
    catch (const std::filesystem::filesystem_error &e) {
        _logger.print_log("Could not create path: " + _pid_filepath + " Will not be able to store daemon pid.", LOG_LEVEL::WARNING);
    }
    _exit_child = _daemonize();
    if (_exit_child)
        return ;
    _create_pid_file();
    _init_socket();
    _init_signal();
}

MattDaemon::~MattDaemon(void) {
    if (close(_lockfile_fd) >= 0)
        remove(_lockfile_path.c_str());
    remove(_pid_filepath.c_str());
}

void MattDaemon::_init_signal(void) {
    t_sigaction sa;

    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    for (size_t i = 0; i < 32; i++) {
        if (i == SIGKILL || i == SIGSTOP)
            continue;
        sigaction(i, &sa, nullptr);
    }
}

void MattDaemon::_create_pid_file(void) {
    FILE *current_pid_fp;
    pid_t current_pid;

    current_pid = getpid();
    current_pid_fp = std::fopen(_pid_filepath.c_str(), "w");
    if (!current_pid_fp)
        _logger.print_log("Could not store pid in " + _pid_filepath + " file. Continuing daemon initialization.", LOG_LEVEL::WARNING);
    fprintf(current_pid_fp, "%d", current_pid);
    fclose(current_pid_fp);
}

void MattDaemon::_lock_file(void) {
    _lockfile_fd = open(_lockfile_path.c_str(), O_RDWR | O_CREAT, 0644);
    if (_lockfile_fd < 0 || flock(_lockfile_fd, LOCK_EX | LOCK_NB) == -1)
        throw UnableToOpenFileException("Could not open: ", _lockfile_path);
    _logger.print_log("Process successfully initialized !", STDOUT_FILENO, LOG_LEVEL::INFO);
}

void MattDaemon::_init_socket(void) {
    int opt = 1;
    sockaddr_in addr;

    _server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_server_fd < 0)
        throw FailedToCreateSocketException("Failed to create server socket.", _logger, LOG_LEVEL::CRITICAL);
    if (setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        throw FailedToCreateSocketException("Failed to set SO_REUSEADDR to server socket.", _logger, LOG_LEVEL::CRITICAL);
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
    pid_t first_child;
    pid_t second_child;

    first_child = fork();
    if (first_child == -1)
        throw FailedToDaemonizeException("Could not call first fork()", _logger, LOG_LEVEL::CRITICAL);
    if (first_child != 0)
        return true;
    if (setsid() < 0)
        throw FailedToDaemonizeException("Could not detach process to session", _logger, LOG_LEVEL::CRITICAL);
    second_child = fork();
    if (second_child == -1)
        throw FailedToDaemonizeException("Could not call second fork()", _logger, LOG_LEVEL::CRITICAL);
    if (second_child != 0)
        return true;
    _logger.print_log("Process successfully daemonized", LOG_LEVEL::INFO);
    if (chdir("/") < 0)
        throw FailedToDaemonizeException("Could not change home directory of current process.", _logger, LOG_LEVEL::CRITICAL);
    umask(0);
    int new_stdio = open("/dev/null", O_RDWR);
    dup2(new_stdio, STDIN_FILENO);
    dup2(new_stdio, STDOUT_FILENO);
    dup2(new_stdio, STDERR_FILENO);
    if (new_stdio > STDERR_FILENO)
        close(new_stdio);
    return false;
}

void MattDaemon::run(void) {
    while (g_signum == -1 && !_exit_child) {
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
            else
                _handle_client(i);
        }
    }
    if (g_signum != -1)
        _logger.print_log("Signal " + std::to_string(g_signum) + " received.", LOG_LEVEL::INFO);
    for (size_t i = 0; i < _pollfds.size(); i++)
        if (_pollfds[i].fd != _server_fd)
            _logger.print_log("Server shutdown.", _pollfds[i].fd, LOG_LEVEL::INFO);
    if (_server_fd != -1)
        _logger.print_log("Server shutdown.", LOG_LEVEL::INFO);
}

void MattDaemon::_handle_client(int client_index) {
    char buffer[_buffer_read_size];

    std::memset(buffer, 0, _buffer_read_size);
    ssize_t bytes = recv(_clients[client_index].get_pollfd().fd, buffer, _buffer_read_size - 1, 0);
    if (bytes <= 0) {
        _logger.print_log("Client at " + _clients[client_index].get_str_ip() + " disconnected", LOG_LEVEL::INFO);
        _pollfds.erase(_pollfds.begin() + client_index);
        _clients.erase(_clients.begin() + client_index);
        return ;
    }
    while (bytes > 0 && (buffer[bytes - 1] == '\n' || buffer[bytes - 1] == '\r')) {
        buffer[bytes - 1] = '\0';
        bytes--;
    }
    std::string msg(buffer);
    _logger.print_log("Client at " + _clients[client_index].get_str_ip() + " sent `" + msg + "`", LOG_LEVEL::INFO);
    _exit_child = (msg.size() == 4 && msg == "quit");
}

void MattDaemon::_handle_new_connection(void) {
    std::string msg;

    try {
        Client client(_server_fd);

        if (_clients.size() >= _nb_clients) {
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

void signal_handler(int signum) {
    g_signum = signum;
}
