FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    build-essential g++ make libssl-dev libncurses-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . /app

# Build only; `make client` would also try to run the client during image build.
RUN make build/run_client
