# Phase 3 — Server Authentication

Requires a C++17 compiler, OpenSSL, and ncurses. Certificates are generated automatically during the build.

```sh
make
make server          # terminal 1
make client          # terminal 2
```

For proxy tests, use `make proxy` and `make victim-client`; use the `-fake` or `-thief` variants for their respective attacks.
