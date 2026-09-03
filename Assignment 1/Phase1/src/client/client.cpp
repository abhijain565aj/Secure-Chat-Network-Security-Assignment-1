#include "client.hpp"
#include "chat/command.hpp"
#include "chat/protocol.hpp"
#include "chat/presentation.hpp"
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
    std::string promptMessage;
    while (terminalUi->readInput(promptMessage)) {
        try {
            ParsedCommand command = CommandParser::parse(promptMessage);
            switch (command.type) {
            case CommandType::Quit:
                running = false;
                closeConnection();
                return;
			case CommandType::Login:
                pendingUsername = static_cast<LoginMessage*>(command.message.get())->get_username();
                handle_send(*command.message);
                break;
			case CommandType::Who:
			case CommandType::UserMessage:
				handle_send(*command.message);
				break;
            }
        } catch (const std::exception& error) {
            terminalUi->printOutput(Presentation::error_message(error.what()));
        }
    }
}

void Client::handle_recv(){
    try {
        std::unique_ptr<Message> message = receive_message(clientSocket);
        if (auto* user = dynamic_cast<UserMessage*>(message.get())) {
            terminalUi->printOutput(Presentation::user_message(*user, username));
        }
        else if (auto* login = dynamic_cast<LoginMessage*>(message.get()))
            terminalUi->printOutput(Presentation::server_message("login " + login->get_username()));
        else if (auto* server = dynamic_cast<ServerMessage*>(message.get())) {
            if (server->get_text().rfind("Hi @", 0) == 0 &&
                server->get_text().find(", Login successful.") != std::string::npos)
                username = pendingUsername;
            terminalUi->printOutput(Presentation::server_message(server->get_text()));
        }
        else
            terminalUi->printOutput(Presentation::server_message("active users requested"));
    } catch (const std::exception& error) {
        terminalUi->printOutput(Presentation::error_message(std::string("recv: ") + error.what()));
        running = false;
        closeConnection();
    }
}

void Client::handle_send(const Message& message){
    try {
        send_message(clientSocket, message);
    } catch (const std::exception& error) {
        terminalUi->printOutput(Presentation::error_message(std::string("send: ") + error.what()));
        running = false;
        closeConnection();
    }
}