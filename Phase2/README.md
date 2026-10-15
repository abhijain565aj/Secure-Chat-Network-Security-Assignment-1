# Phase 2 — Client–Server Encryption

Requires a C++17 compiler, OpenSSL, and ncurses.

```sh
make
make server          # terminal 1
make client          # terminal 2
```

For the MITM demonstration, run `make proxy` in another terminal and connect the intercepted client with `make victim-client`.
