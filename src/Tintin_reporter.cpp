#include <filesystem>
#include <string>
#include <unistd.h>
#include "Tintin_reporter.hpp"
#include "exceptions/UnableToOpenFile.hpp"
#include "utils.hpp"

Tintin_reporter::Tintin_reporter(void) {
    try {
        std::filesystem::create_directories(std::filesystem::path(LOGFILE_PATH).parent_path().string());
    }
    catch (const std::filesystem::filesystem_error &e) {
        throw UnableToOpenFile("Can't open: ", LOGFILE_PATH);
    }
    _s_logfile.open(LOGFILE_PATH);
    if (!_s_logfile.is_open())
        throw UnableToOpenFile("Can't open: ", LOGFILE_PATH);
}

Tintin_reporter::~Tintin_reporter(void) {
    _s_logfile.close();
}

void Tintin_reporter::print_log(std::string const &msg) {
    _s_logfile << msg << std::endl;
};

void Tintin_reporter::print_log(std::string const &msg, int fd) {
    size_t _ = write(fd, msg.c_str(), msg.length());
}
