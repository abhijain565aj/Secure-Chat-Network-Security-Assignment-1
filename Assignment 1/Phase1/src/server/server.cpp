
#include "server.hpp"
#include "chat/message.hpp"
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <unistd.h>
#include <iostream>
#include <arpa/inet.h>

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
    users.clear();
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
    sockaddr_in clientAddress{};
    socklen_t addressLength = sizeof(clientAddress);
    int clientSocket = accept(serverSocket,
                              reinterpret_cast<sockaddr*>(&clientAddress),
                              &addressLength);
    if (clientSocket < 0) {
        std::cerr << "accept: " << std::strerror(errno) << std::endl;
        return;
    }

    pollfd pfd;
    pfd.fd = clientSocket;
    pfd.events = POLLIN;
    pfd.revents = 0;

    fds.push_back(pfd);

    char ipAddress[INET_ADDRSTRLEN]{};
    if (inet_ntop(AF_INET, &clientAddress.sin_addr, ipAddress,
                  sizeof(ipAddress)) == nullptr) {
        close_client(clientSocket);
        return;
    }
    users.emplace_back(clientSocket, ipAddress, ntohs(clientAddress.sin_port));
    std::cout << "Client connected: " << ipAddress << ":"
              << ntohs(clientAddress.sin_port) << " (fd " << clientSocket
              << ")" << std::endl;
}

void Server::handle_client(int client_fd){
    try {
        std::unique_ptr<Message> message = receive_message(client_fd);
        User* user = find_user(client_fd);
        if (user == nullptr) throw std::runtime_error("Unknown client");

        if (auto* login = dynamic_cast<LoginMessage*>(message.get())) {
            user->login(login->get_username());
            std::cout << "Login request: " << user->get_username()
                      << " from " << user->get_ip_address() << ":"
                      << user->get_port() << std::endl;
            ServerMessage response("Login successful.");
            send_message(client_fd, response);
        } else if (!user->is_logged_in()) {
            ServerMessage response("Please login first.");
            send_message(client_fd, response);
        } else if (auto* userMessage = dynamic_cast<UserMessage*>(message.get())) {
            std::cout << "Message from " << user->get_username() << ": @"
                      << userMessage->get_username() << " "
                      << userMessage->get_message() << std::endl;
            User* recipient = find_user(userMessage->get_username());
            if (recipient == nullptr || !recipient->is_logged_in()) {
                ServerMessage response("User is not online.");
                send_message(client_fd, response);
            } else {
                send_message(recipient->get_file_descriptor(), *userMessage);
            }
        } else {
            std::cout << "Who request from " << user->get_username() << std::endl;
            send_message(client_fd, *message);
        }
    } catch (const std::exception& error) {
        std::cerr << "message: " << error.what() << std::endl;
        close_client(client_fd);
    }
}

void Server::close_client(int client_fd){
    close(client_fd);
    users.erase(std::remove_if(users.begin(), users.end(),
                               [client_fd](const User& user) {
                                   return user.get_file_descriptor() == client_fd;
                               }),
                users.end());
    for (auto& pfd : fds)
        if (pfd.fd == client_fd) pfd.fd = -1;
    fds.erase(std::remove_if(fds.begin(), fds.end(),
                             [](const pollfd& pfd) { return pfd.fd < 0; }),
              fds.end());
}

User* Server::find_user(int client_fd){
    for (auto& user : users) {
        if (user.get_file_descriptor() == client_fd) return &user;
    }
    return nullptr;
}

User* Server::find_user(const std::string& username){
    for (auto& user : users) {
        if (user.is_logged_in() && user.get_username() == username) return &user;
    }
    return nullptr;
}