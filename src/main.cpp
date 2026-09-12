#include "MattDaemon.hpp"
#include "Tintin_reporter.hpp"
#include "exceptions/MattDaemonException.hpp"
#include <cstdlib>
#include <unistd.h>

int main(void) {
    try {
        MattDaemon daemon;

        daemon.init();
    }
    catch (MattDaemonException const &e) {
        Tintin_reporter logger;

        logger.print_log(e.what(), STDERR_FILENO);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
