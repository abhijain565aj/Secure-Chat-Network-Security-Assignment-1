
#include "server.hpp"
#include "chat/message.hpp"
#include "chat/protocol.hpp"
#include "chat/presentation.hpp"
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
        std::cerr << Presentation::server_log(
            std::string("accept: ") + std::strerror(errno)) << std::endl;
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
    users.add(User(clientSocket, ipAddress, ntohs(clientAddress.sin_port)));
    std::cout << Presentation::server_log(
        "Client connected: " + std::string(ipAddress) + ":" +
        std::to_string(ntohs(clientAddress.sin_port)) + " (fd " +
        std::to_string(clientSocket) + ")") << std::endl;
}

void Server::handle_client(int client_fd){
    try {
        std::unique_ptr<Message> message = receive_message(client_fd);
        User* user = users.find_by_file_descriptor(client_fd);
        if (user == nullptr) throw std::runtime_error("Unknown client");
        log_request(client_fd, *message);

        if (auto* login = dynamic_cast<LoginMessage*>(message.get())) {
            handle_login(client_fd, *user, *login);
        } else if (!user->is_logged_in()) {
            ServerMessage response("Please login first.");
            send_message(client_fd, response);
            std::cout << Presentation::server_log(
                "Rejected request from fd " + std::to_string(client_fd) +
                ": login required") << std::endl;
        } else if (auto* userMessage = dynamic_cast<UserMessage*>(message.get())) {
            handle_user_message(client_fd, *user, *userMessage);
        } else if (dynamic_cast<WhoMessage*>(message.get())) {
            handle_who(client_fd, *user);
        } else {
            ServerMessage response("Unsupported message.");
            send_message(client_fd, response);
        }
    } catch (const std::exception& error) {
            std::cerr << Presentation::server_log(std::string("message: ") + error.what())
                      << std::endl;
        close_client(client_fd);
    }
}

void Server::close_client(int client_fd){
    close(client_fd);
    users.remove(client_fd);
    for (auto& pfd : fds)
        if (pfd.fd == client_fd) pfd.fd = -1;
    fds.erase(std::remove_if(fds.begin(), fds.end(),
                             [](const pollfd& pfd) { return pfd.fd < 0; }),
              fds.end());
}

void Server::handle_login(int client_fd, User& user, const LoginMessage& message) {
    if (users.find_logged_in(message.get_username()) != nullptr) {
        ServerMessage response("Username is already in use.");
        send_message(client_fd, response);
        return;
    }
    user.login(message.get_username());
    std::cout << Presentation::server_log(
        "User logged in: " + user.get_username()) << std::endl;
    ServerMessage response("Hi @" + user.get_username() + ", Login successful.");
    send_message(client_fd, response);
}

void Server::handle_user_message(int client_fd, User& user, const UserMessage& message) {
    User* recipient = users.find_logged_in(message.get_username());
    if (recipient == nullptr) {
        ServerMessage response("Message failed: user is not online.");
        send_message(client_fd, response);
        std::cout << Presentation::server_log(
            "Delivery failed: " + user.get_username() + " -> @" +
            message.get_username() + " (user offline)") << std::endl;
        return;
    }
    UserMessage delivered(user.get_username(), message.get_username(), message.get_message());
    try {
        send_message(recipient->get_file_descriptor(), delivered);
        if (recipient->get_file_descriptor() != client_fd)
            send_message(client_fd, delivered);
        std::cout << Presentation::server_log(
            "Relayed message: " + user.get_username() + " -> @" +
            message.get_username() + ": " + message.get_message()) << std::endl;
    } catch (const std::exception&) {
        ServerMessage response("Message failed: delivery error.");
        send_message(client_fd, response);
        std::cout << Presentation::server_log(
            "Delivery failed: " + user.get_username() + " -> @" +
            message.get_username() + " (send error)") << std::endl;
    }
}

void Server::handle_who(int client_fd, const User&) {
    ServerMessage response("Online users:\n" + users.logged_in_usernames());
    send_message(client_fd, response);
    std::cout << Presentation::server_log("Processed /who request") << std::endl;
}

void Server::log_request(int client_fd, const Message& message) const {
    std::string request;
    if (const auto* login = dynamic_cast<const LoginMessage*>(&message)) {
        request = "/login " + login->get_username();
    } else if (const auto* userMessage = dynamic_cast<const UserMessage*>(&message)) {
        request = "message to @" + userMessage->get_username() + ": " +
                  userMessage->get_message();
    } else if (dynamic_cast<const WhoMessage*>(&message)) {
        request = "/who";
    } else {
        request = "unknown message";
    }
    std::cout << Presentation::server_log(
        "Request from fd " + std::to_string(client_fd) + ": " + request)
              << std::endl;
}