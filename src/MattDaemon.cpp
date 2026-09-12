#include <fcntl.h>
#include <unistd.h>
#include <filesystem>
#include "MattDaemon.hpp"
#include "Tintin_reporter.hpp"
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

MattDaemon::~MattDaemon(void) {}

void MattDaemon::init(void) {
    if (geteuid() != 0)
        throw RunWithNonRootUserException("Could not initialize deamon. Ensure it's running as root.");
    int lockfile_fd = open(LOCKFILE_PATH, O_RDWR | O_CREAT, 0644);
    if (lockfile_fd < 0)
        throw UnableToOpenFile("Can't open", LOCKFILE_PATH);
    _logger.print_log("Daemon successfully initialized !");
}
