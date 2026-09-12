#pragma once
#include <fstream>
#include <string>

class Tintin_reporter {
    public:
        Tintin_reporter(void);
        ~Tintin_reporter(void);
        Tintin_reporter(const Tintin_reporter&) = delete;

        void print_log(std::string const &msg);
        void print_log(std::string const &msg, int fd);

        Tintin_reporter& operator=(const Tintin_reporter&) = delete;

    private:
        std::ofstream _s_logfile;
};
