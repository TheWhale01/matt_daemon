#pragma once

#include <sys/poll.h>

#define NB_CLIENTS 3
#define RD_BUFFER_SIZE 1024
#define LOGFILE_PATH "/var/log/matt_daemon/matt_daemon.log"
#define LOCKFILE_PATH "/var/lock/matt_daemon.lock"

typedef struct pollfd t_pollfd;
