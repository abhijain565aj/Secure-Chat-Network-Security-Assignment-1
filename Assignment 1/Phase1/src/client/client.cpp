#include "client.hpp"
#include <arpa/inet.h>
#include <stdexcept>
#include <unistd.h>
#include <iostream>
#include <cstring>

Client::Client(){
    fds.resize(2);
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;

    fds[1].fd = clientSocket;
    fds[1].events = POLLIN;
}

void Client::connectToServer(std::string IP, int port){
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        throw std::runtime_error(std::strerror(errno));
    }

    serverAddress = {};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    if (inet_pton(AF_INET, IP.c_str(), &serverAddress.sin_addr) != 1) {
        throw std::runtime_error("Invalid IP address");
    }

    if (connect(clientSocket, (struct sockaddr*)&serverAddress,
                sizeof(serverAddress)) < 0) {
        throw std::runtime_error(std::strerror(errno));
    }
}

void Client::eventLoop(){
    while (true) {
        int n = poll(fds.data(), fds.size(), -1);

        if (n < 0) {
            if (errno == EINTR) continue;
            throw std::runtime_error(std::strerror(errno));
        }

        if (fds[0].revents & POLLIN) {
            handle_send();
        }

        if (fds[1].revents & POLLIN) {
            handle_recv();
        }
    }
}

void Client::closeConnection(){
    close(clientSocket);
}

void Client::handle_recv(){
    char buffer[BUFFER_SIZE] = { 0 };
    ssize_t received = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (received > 0) {
        buffer[received] = '\0';
        std::cout << "Server: " << buffer;
    } else {
        if (received < 0)
            std::cerr << "recv: " << std::strerror(errno) << std::endl;
        closeConnection();
        exit(0);
    }
}

void Client::handle_send(){
    std::string message;
    std::getline(std::cin, message);
    ssize_t sent = send(clientSocket, message.c_str(), message.size(), 0);
    if (sent < 0) {
        std::cerr << "send: " << std::strerror(errno) << std::endl;
        closeConnection();
        exit(1);
    }
}