#pragma once
#include <fstream>
#include <string>

class Tintin_reporter {
    public:
        Tintin_reporter(void);
        ~Tintin_reporter(void);

        void print_log(std::string const &msg);
        void print_log(std::string const &msg, int fd);

    private:
        std::ofstream _s_logfile;
};
