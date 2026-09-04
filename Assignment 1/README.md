# Collaborators

- Abhi Jain (23b0903)
- Sameer Arvind Patil (23b1035)

# Secure Chat — Network Security Assignment

A five-phase C++ secure-chat implementation for CS-6008. Each phase is self-contained; see its directory README for its exact commands.

| Phase | Focus |
| --- | --- |
| 1 | TCP chat with framing and routing |
| 2 | Client--server Diffie--Hellman and AES-GCM; MITM demonstration |
| 3 | Server authentication with a private CA and certificates |
| 4 | Peer-to-peer end-to-end encryption over the relay server |
| 5 | Automatic E2E key rotation for forward secrecy |

## Run locally

Each phase requires a C++17 compiler, OpenSSL, and ncurses. From a phase directory, build and launch the server and clients in separate terminals:

```sh
make
make server
make client
```

## Docker (Phase 2)

The included Compose setup builds and runs the Phase 2 server, two clients, and Mallory on an isolated `172.28.0.0/24` network. Start the server in one terminal:

```sh
docker compose up --build server
```

Then open a client in each of two other terminals:

```sh
docker compose run --rm client1
docker compose run --rm client2
```

For the MITM container, start `docker compose up --build mallory`. Stop the stack with `docker compose down`.
