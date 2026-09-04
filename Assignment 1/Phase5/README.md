# Phase 5 — Forward Secrecy

Requires a C++17 compiler, OpenSSL, and ncurses. Certificates are generated automatically during the build.

```sh
make
make server          # terminal 1
make client          # terminal 2 (repeat for the peer)
```

After an E2E chat is established, keep both clients running to observe the automatic 60-second key rotations.
