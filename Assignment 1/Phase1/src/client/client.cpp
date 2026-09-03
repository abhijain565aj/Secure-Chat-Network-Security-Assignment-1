#include "client.hpp"
#include <arpa/inet.h>
#include <stdexcept>
#include <unistd.h>
#include <cstring>
#include <cerrno>

Client::Client() : clientSocket(-1), terminalUi(std::make_shared<TerminalUI>()),
    running(true) {}

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
    fds = {
        {STDIN_FILENO, POLLIN, 0},
        {clientSocket, POLLIN, 0}
    };

    while (running) {
        int n = poll(fds.data(), fds.size(), 100);

        if (n < 0) {
            if (errno == EINTR) continue;
            throw std::runtime_error(std::strerror(errno));
        }

        if (fds[0].revents & (POLLIN | POLLHUP | POLLERR)) {
            handleInput();
        }

        if (fds[1].revents & POLLIN) {
            handle_recv();
        }
    }
}

void Client::closeConnection(){
    if (clientSocket >= 0) {
        close(clientSocket);
        clientSocket = -1;
    }
}

void Client::handleInput(){
    std::string message;
    while (terminalUi->readInput(message)) {
        handle_send(message);
    }
}

void Client::handle_recv(){
    char buffer[BUFFER_SIZE] = { 0 };
    ssize_t received = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (received > 0) {
        buffer[received] = '\0';
        std::string output = "Server: " + std::string(buffer, received);
        if (output.back() != '\n') output += '\n';
        terminalUi->printOutput(output);
    } else {
        if (received < 0)
            terminalUi->printOutput("recv: " + std::string(std::strerror(errno)) + "\n");
        else
            terminalUi->printOutput("Server disconnected.\n");
        running = false;
        closeConnection();
    }
}

void Client::handle_send(const std::string& message){
    ssize_t sent = send(clientSocket, message.c_str(), message.size(), 0);
    if (sent < 0) {
        terminalUi->printOutput("send: " + std::string(std::strerror(errno)) + "\n");
        running = false;
        closeConnection();
    }
}