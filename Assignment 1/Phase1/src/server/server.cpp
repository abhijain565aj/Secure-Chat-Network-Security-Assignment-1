
#include "server.hpp"
#include "chat/message.hpp"
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <unistd.h>
#include <iostream>

void Server::startServer(){
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        throw std::runtime_error(std::strerror(errno));
    }

    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error(std::strerror(errno));
    }

    serverAddress = {};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT_NO);
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(serverSocket, (struct sockaddr*)&serverAddress,
             sizeof(serverAddress)) < 0) {
        throw std::runtime_error(std::strerror(errno));
    }

    if (listen(serverSocket, SOMAXCONN) < 0) {
        throw std::runtime_error(std::strerror(errno));
    }
}

void Server::stopServer(){
    for (const auto& pfd : fds) close(pfd.fd);
    fds.clear();
    if (serverSocket >= 0) {
        close(serverSocket);
        serverSocket = -1;
    }
}

void Server::eventLoop(){

    fds.push_back({
        serverSocket,
        POLLIN,
        0
    });

    while (true) {

        int n = poll(
            fds.data(),
            fds.size(),
            -1
        );
        if (n < 0) {
            if (errno == EINTR) continue;
            throw std::runtime_error(std::strerror(errno));
        }

        for (std::size_t i = 0; i < fds.size(); ++i) {
            auto& pfd = fds[i];

            if (pfd.revents & (POLLIN | POLLHUP | POLLERR)) {
                if (pfd.fd == serverSocket) {
                    accept_new_clients();
                } else {
                    handle_client(pfd.fd);
                }
            }
        }
    }
}

void Server::accept_new_clients(){
    int clientSocket = accept(serverSocket, nullptr, nullptr);
    if (clientSocket < 0) {
        std::cerr << "accept: " << std::strerror(errno) << std::endl;
        return;
    }

    pollfd pfd;
    pfd.fd = clientSocket;
    pfd.events = POLLIN;
    pfd.revents = 0;

    fds.push_back(pfd);
}

void Server::handle_client(int client_fd){
    try {
        std::unique_ptr<Message> message = receive_message(client_fd);
        if (auto* user = dynamic_cast<UserMessage*>(message.get())) {
            std::cout << "Message from client: @" << user->get_username()
                      << " " << user->get_message() << std::endl;
        } else if (auto* login = dynamic_cast<LoginMessage*>(message.get())) {
            std::cout << "Login request: " << login->get_username() << std::endl;
        } else {
            std::cout << "Who request received" << std::endl;
        }
        send_message(client_fd, *message);
    } catch (const std::exception& error) {
        std::cerr << "message: " << error.what() << std::endl;
        close_client(client_fd);
    }
}

void Server::close_client(int client_fd){
    close(client_fd);
    for (auto& pfd : fds)
        if (pfd.fd == client_fd) pfd.fd = -1;
    fds.erase(std::remove_if(fds.begin(), fds.end(),
                             [](const pollfd& pfd) { return pfd.fd < 0; }),
              fds.end());
}