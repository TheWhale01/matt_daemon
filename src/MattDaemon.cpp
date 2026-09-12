#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#include <filesystem>
#include <sys/file.h>
#include "MattDaemon.hpp"
#include "Tintin_reporter.hpp"
#include "exceptions/FailedToDeamonize.hpp"
#include "exceptions/RunWithNonRootUserException.hpp"
#include "exceptions/UnableToOpenFile.hpp"
#include "utils.hpp"

MattDaemon::MattDaemon(void) {
    try {
        std::filesystem::create_directories(std::filesystem::path(LOCKFILE_PATH).parent_path().string());
    }
    catch (const std::filesystem::filesystem_error &e) {
        throw UnableToOpenFile("Can't open: ", LOCKFILE_PATH);
    }
}

MattDaemon::~MattDaemon(void) {
    close(_lockfile_fd);
}

void MattDaemon::init(void) {
    if (geteuid() != 0)
        throw RunWithNonRootUserException("Could not initialize deamon. Ensure it's running as root.");
    _lockfile_fd = open(LOCKFILE_PATH, O_RDWR | O_CREAT, 0644);
    if (_lockfile_fd < 0 || flock(_lockfile_fd, LOCK_EX | LOCK_NB) == -1)
        throw UnableToOpenFile("Could not open: ", LOCKFILE_PATH);
    _logger.print_log("Process successfully initialized !");
}

void MattDaemon::daemonize(void) {
    pid_t first_child;
    pid_t second_child;

    first_child = fork();
    if (first_child == -1)
        throw FailedToDaemonize("Could not call first fork()");
    if (first_child != 0)
        exit(EXIT_SUCCESS);
    second_child = fork();
    if (second_child == -1)
        throw FailedToDaemonize("Could not call second fork()");
    if (second_child != 0)
        exit(EXIT_SUCCESS);
    _logger.print_log("Process successfully daemonized");
}

void MattDaemon::run(void) {
    while (true);
}
