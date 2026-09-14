#include <ctime>
#include <filesystem>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include "Tintin_reporter.hpp"
#include "exceptions/UnableToOpenFileException.hpp"
#include "utils.hpp"

Tintin_reporter::Tintin_reporter(void) {
    try {
        std::filesystem::create_directories(std::filesystem::path(LOGFILE_PATH).parent_path().string());
    }
    catch (const std::filesystem::filesystem_error &e) {
        throw UnableToOpenFileException("Can't open: ", LOGFILE_PATH);
    }
    _s_logfile.open(LOGFILE_PATH);
    if (!_s_logfile.is_open())
        throw UnableToOpenFileException("Can't open: ", LOGFILE_PATH);
}

Tintin_reporter::~Tintin_reporter(void) {
    _s_logfile.close();
}

std::string Tintin_reporter::_get_logformat_str(LOG_LEVEL level) const {
    std::ostringstream oss;
    time_t time = std::time(nullptr);
    struct tm datetime = *std::localtime(&time);

    oss << std::put_time(&datetime, "%d / %m / %Y - %H : %M : %S") << " [";
    switch (level) {
        case LOG_LEVEL::DEBUG:
            oss << "DEBUG";
            break;
        case LOG_LEVEL::INFO:
            oss << "INFO";
            break;
        case LOG_LEVEL::WARNING:
            oss << "WARNING";
            break;
        case LOG_LEVEL::ERROR:
            oss << "ERROR";
            break;
        case LOG_LEVEL::CRITICAL:
            oss << "CRITICAL";
            break;
        default:
            break;
    }
    oss << "] ";
    return oss.str();
}

void Tintin_reporter::print_log(std::string const &msg, LOG_LEVEL level) {
    _s_logfile << _get_logformat_str(level) << msg << std::endl;
};

void Tintin_reporter::print_log(std::string const &msg, int fd, LOG_LEVEL level) {
    std::string format = _get_logformat_str(level);

    write(fd, format.c_str(), format.length());
    write(fd, (msg + "\n").c_str(), msg.length());
}
