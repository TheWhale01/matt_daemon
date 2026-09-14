#include "MattDaemon.hpp"
#include "exceptions/MattDaemonException.hpp"
#include <cstdlib>
#include <iostream>
#include <unistd.h>

int main(void) {
    try {
        MattDaemon daemon;

        daemon.run();
    }
    catch (MattDaemonException const &e) {
        std::cerr << e.what() << std::endl;
    }
    return EXIT_SUCCESS;
}
