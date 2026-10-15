# Phase 4 — End-to-End Encryption

Requires a C++17 compiler, OpenSSL, and ncurses. Certificates are generated automatically during the build.

```sh
make
make server          # terminal 1
make client          # terminal 2 (repeat for the peer)
```

Log in two clients and select each other as the chat peer to establish an E2E session.
