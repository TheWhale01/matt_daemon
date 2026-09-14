#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <filesystem>
#include <sys/file.h>
#include "MattDaemon.hpp"
#include "Tintin_reporter.hpp"
#include "exceptions/FailedToDeamonizeException.hpp"
#include "exceptions/RunWithNonRootUserException.hpp"
#include "exceptions/UnableToOpenFileException.hpp"
#include "utils.hpp"

MattDaemon::MattDaemon(void) {
    try {
        std::filesystem::create_directories(std::filesystem::path(LOCKFILE_PATH).parent_path().string());
    }
    catch (const std::filesystem::filesystem_error &e) {
        throw UnableToOpenFileException("Can't open: ", LOCKFILE_PATH);
    }
}

MattDaemon::~MattDaemon(void) {
    close(_lockfile_fd);
    int _ = remove(_pid_filepath.c_str());
}

void MattDaemon::init(void) {
    if (geteuid() != 0)
        throw RunWithNonRootUserException("Could not initialize deamon. Ensure it's running as root.");
    _lockfile_fd = open(LOCKFILE_PATH, O_RDWR | O_CREAT, 0644);
    if (_lockfile_fd < 0 || flock(_lockfile_fd, LOCK_EX | LOCK_NB) == -1)
        throw UnableToOpenFileException("Could not open: ", LOCKFILE_PATH);
    _logger.print_log("Process successfully initialized !", LOG_LEVEL::INFO);
}

void MattDaemon::daemonize(void) {
    pid_t first_child;
    pid_t second_child;

    first_child = fork();
    if (first_child == -1)
        throw FailedToDaemonizeException("Could not call first fork()");
    if (first_child != 0)
        exit(EXIT_SUCCESS);
    if (setsid() < 0)
        throw FailedToDaemonizeException("Could not detach process to session");
    second_child = fork();
    if (second_child == -1)
        throw FailedToDaemonizeException("Could not call second fork()");
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
        throw FailedToDaemonizeException("Could not change home directory of current process.");
    umask(0);
    for (int fd = 0; fd < sysconf(_SC_OPEN_MAX); fd++)
        close(fd);
    int new_stdio = open("/dev/null", O_RDWR);
    dup2(new_stdio, STDIN_FILENO);
    dup2(new_stdio, STDOUT_FILENO);
    dup2(new_stdio, STDERR_FILENO);
    if (new_stdio > STDERR_FILENO)
        close(new_stdio);
}

void MattDaemon::run(void) {
    while (true);
}
