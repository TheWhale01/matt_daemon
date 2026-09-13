#pragma once
#include <fstream>
#include <string>

enum class LOG_LEVEL {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL,
};

class Tintin_reporter {
    public:
        Tintin_reporter(void);
        ~Tintin_reporter(void);
        Tintin_reporter(const Tintin_reporter&) = delete;

        void print_log(std::string const &msg, LOG_LEVEL level);
        void print_log(std::string const &msg, int fd, LOG_LEVEL level);

        Tintin_reporter& operator=(const Tintin_reporter&) = delete;

    private:
        std::ofstream _s_logfile;

        std::string _get_logformat_str(LOG_LEVEL level) const ;
};
