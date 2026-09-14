# Matt_daemon

`Matt_daemon` is a small C++ daemon that listens for TCP connections on port `4242`, records client activity, and writes timestamped events to a log file.

## Features

The program currently provides the following behavior:

- Builds an executable named `Matt_daemon`.
- Requires root privileges to start.
- Runs in the background as a daemon using the traditional double-fork pattern.
- Listens on TCP port `4242` on all IPv4 interfaces.
- Accepts up to three simultaneous clients.
- Uses `poll(2)` to monitor the listening socket and connected clients.
- Logs daemon startup, client connections, client disconnections, and received client messages.
- Creates an exclusive lock so that only one daemon instance can run at a time.
- Stores the daemon PID in a PID file when possible.
- Redirects standard input, output, and error to `/dev/null` after daemonization.

The assignment specification also requires the daemon to stop on the message `quit` and to handle signals cleanly. These behaviors are part of the project requirements; consult the **Current implementation notes** section below when using this version of the source.

## Requirements

- A C++ compiler available as `clang++`.
- GNU Make.
- A free TCP port `4242`.

The Makefile compiles with:

- `-Wall -Wextra -Werror`
- debug symbols via `-g3`
- headers from `includes/`

## Building

From the project root:

```sh
make
```

This creates the `Matt_daemon` executable and intermediate object files in `obj/`.

Other available targets:

```sh
make clean   # Remove object files
make fclean  # Remove object files and the executable
make re      # Clean and rebuild everything
```

## Running

The daemon must be started as root:

```sh
sudo ./Matt_daemon
```

Because the program daemonizes itself, the launching process exits while the daemon continues in the background. After startup, the daemon listens on port `4242`.

To confirm that it is listening, use one of the following commands:

```sh
sudo ss -ltnp | grep ':4242'
```

or, on systems with `net-tools` installed:

```sh
sudo netstat -ltnp | grep 4242
```

To inspect the recorded PID:

```sh
sudo cat /var/run/matt_daemon/matt_daemon.pid
```

## Connecting as a client

A simple TCP client such as `nc` can be used to connect:

```sh
nc 127.0.0.1 4242
```

Type a message and press Enter. The daemon records the message in its log. For example:

```text
Hello from a client
```

Then inspect the log:

```sh
sudo tail -f /var/log/matt_daemon/matt_daemon.log
```

The server accepts at most three clients at once. A further connection is logged as an error and is closed.

## Stopping the daemon

If the running source supports the assignment's `quit` command, send `quit` through a client connection:

```sh
printf 'quit\n' | nc 127.0.0.1 4242
```

The assignment also requires graceful signal handling. A conventional way to request shutdown is:

```sh
sudo kill -TERM "$(sudo cat /var/run/matt_daemon/matt_daemon.pid)"
```

Always verify the resulting behavior in the log before relying on a particular shutdown method in production.

## Runtime files

The daemon uses the following fixed paths, defined in `includes/utils.hpp` and `includes/MattDaemon.hpp`:

| Path | Purpose |
| --- | --- |
| `/var/log/matt_daemon/matt_daemon.log` | Timestamped daemon and client activity log |
| `/var/lock/matt_daemon.lock` | Exclusive lock preventing multiple daemon instances |
| `/var/run/matt_daemon/matt_daemon.pid` | PID of the daemon process, when the PID file can be created |

The required log format is based on the local time and includes the date, time, and log level. A typical entry resembles:

```text
14 / 09 / 2026 - 12 : 34 : 56 [INFO] Server started at 0.0.0.0:4242
```

The exact messages depend on the event and the current implementation.

## Running only one instance

At startup, the daemon opens `/var/lock/matt_daemon.lock` and applies an exclusive, non-blocking `flock(2)`. If another instance already owns the lock, startup fails instead of launching a second daemon.

For troubleshooting, check whether an old process is still running before removing any lock file. The lock is held by the process and is normally released automatically when that process exits; deleting the file while a daemon is still running does not release the underlying lock.

## Project structure

```text
.
├── Makefile
├── includes/
│   ├── Client.hpp
│   ├── MattDaemon.hpp
│   ├── Tintin_reporter.hpp
│   ├── utils.hpp
│   └── exceptions/
├── src/
│   ├── Client.cpp
│   ├── MattDaemon.cpp
│   ├── Tintin_reporter.cpp
│   └── main.cpp
└── en.subject.pdf
```


