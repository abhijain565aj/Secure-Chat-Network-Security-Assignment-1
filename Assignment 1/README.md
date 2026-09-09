# Secure Chat — Network Security Assignment

**Collaborators:** Abhi Jain (23b0903), Sameer Arvind Patil (23b1035)

A five-phase C++ secure-chat implementation for CS-6008.

| Phase | Focus |
| --- | --- |
| 1 | TCP chat with framing and routing |
| 2 | Client-server Diffie-Hellman and AES-GCM; MITM demonstration |
| 3 | Server authentication with a private CA and certificates |
| 4 | Peer-to-peer end-to-end encryption over the relay server |
| 5 | Automatic E2E key rotation for forward secrecy |

## Prerequisites

Local execution requires a C++17 compiler, OpenSSL, and ncurses. Docker execution requires Docker Desktop (or Docker Engine) with Docker Compose v2.

All commands below are run from this `Assignment 1` directory.

```sh
cd "Assignment 1"
```

## Run locally

Use one iTerm2 pane (or terminal window) per running program. The assignment-level Makefile deliberately keeps each program in the foreground so that the ncurses client receives keyboard input.

### Phase 1

```sh
# Pane 1
make phase1-server

# Pane 2
make phase1-client       # alias for phase1-client1

# Pane 3
make phase1-client2
```

### Phase 2

```sh
# Pane 1
make phase2-server

# Pane 2
make phase2-client

# Pane 3
make phase2-client2
```

Optional MITM demonstration:

```sh
# Additional pane
make phase2-proxy

# Additional client, connected to the proxy at port 12346
make phase2-victim-client1
```

### Phase 3

```sh
# Panes 1–3
make phase3-server
make phase3-client
make phase3-client2
```

The certificate files are generated automatically. Optional proxy targets are `phase3-proxy`, `phase3-proxy-fake`, and `phase3-proxy-thief`; use the matching `phase3-victim-client1`, `phase3-victim-client1-fake`, or `phase3-victim-client1-thief` target.

### Phase 4

```sh
# Panes 1–3
make phase4-server
make phase4-client
make phase4-client2
```

### Phase 5

```sh
# Panes 1–3
make phase5-server
make phase5-client
make phase5-client2
```

To stop a local service from another pane, use a corresponding target such as `make stop-phase4-client1`. Stop an entire phase with `make stop-phase4`, or all services with `make stop-all`.

## Using the ncurses client

At the `Command >` prompt, log in before chatting:

```text
/login alice
```

Log in as another user in the other client (for example, `/login bob`). Useful commands include:

```text
/who             List connected users
/chat bob        Select bob as the current chat recipient
hello            Send to the selected recipient
@bob hello       Send directly to bob
/help            Show supported commands
/quit            Exit the client
```

For Phases 4 and 5, establish end-to-end encryption after both users log in:

```text
/e2e bob
```

Both clients show a fingerprint; verify that the fingerprints match. In Phase 5, leave both clients running to observe the automatic key rotation.

## Run with Docker

The Docker client must be started from an attached terminal. Do not open a separate `docker exec ... bash` shell for an already-running client: its keystrokes go to the shell instead of the ncurses client.

For any phase, start the server in the background, then run each client in a separate iTerm2 pane. Replace `N` below with the desired phase number.

```sh
# Pane 1: start the server
docker compose -f docker-compose.phaseN.yml up --build -d server

# Pane 2: interactive ncurses client
docker compose -f docker-compose.phaseN.yml run --rm client1

# Pane 3: interactive ncurses client
docker compose -f docker-compose.phaseN.yml run --rm client2
```

For example, Phase 1 is:

```sh
docker compose -f docker-compose.phase1.yml up --build -d server
docker compose -f docker-compose.phase1.yml run --rm client1
docker compose -f docker-compose.phase1.yml run --rm client2
```

The unqualified `docker-compose.yml` is the Phase 2 configuration, so its equivalent commands are:

```sh
docker compose up --build -d server
docker compose run --rm client1
docker compose run --rm client2
```

To stop and remove a phase's Docker containers and network:

```sh
docker compose -f docker-compose.phaseN.yml down
```

If all services were started detached, attach a real terminal directly to a client instead:

```sh
docker attach chat-client1
```

Detach without stopping the client with `Ctrl-P`, then `Ctrl-Q`.

### Docker MITM and legacy-proxy scenarios

The Phase 2 proxy can be started in the Mallory container, then a client can be run against it:

```sh
docker compose -f docker-compose.phase2.yml exec -d mallory \
  ./build/run_proxy 12346 172.28.0.10 12345
docker compose -f docker-compose.phase2.yml run --rm client1 \
  ./build/run_client 172.28.0.13 12346
```

Phase 3's `docker-compose.phase3.yml` contains commands in its header for normal, fake-certificate, stolen-certificate, and legacy-proxy scenarios.

## Build and clean

Build every local phase without running it:

```sh
make build
```

Remove local build outputs:

```sh
make clean
```
