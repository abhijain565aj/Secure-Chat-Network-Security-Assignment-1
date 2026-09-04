# Phase 1 — Baseline Chat

Requires a C++17 compiler, OpenSSL, and ncurses.

```sh
make
make server          # terminal 1
make client          # terminal 2 (repeat for another client)
```

The default server address is `127.0.0.1:12345`; edit `IP` or `PORT` in the Makefile if needed.
