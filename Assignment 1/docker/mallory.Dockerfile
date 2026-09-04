FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    build-essential g++ make libssl-dev libncurses-dev \
    python3 iproute2 iputils-ping net-tools \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . /app

CMD ["sleep", "infinity"]
