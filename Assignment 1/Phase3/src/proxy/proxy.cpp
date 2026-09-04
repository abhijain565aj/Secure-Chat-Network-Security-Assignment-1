#include "proxy.hpp"
#include "chat/message.hpp"
#include "chat/protocol.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <poll.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

Proxy::Proxy(int listenPort, std::string serverIp, int serverPort)
    : listenPort(listenPort), serverIp(std::move(serverIp)), serverPort(serverPort) {}

Proxy::~Proxy() {
    if (listener >= 0) close(listener);
}

int Proxy::createListener(int port) {
    int socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFd < 0) throw std::runtime_error(std::strerror(errno));

    int reuse = 1;
    if (setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        close(socketFd);
        throw std::runtime_error(std::strerror(errno));
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(socketFd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0 ||
        listen(socketFd, SOMAXCONN) < 0) {
        std::string error = std::strerror(errno);
        close(socketFd);
        throw std::runtime_error(error);
    }
    return socketFd;
}

int Proxy::connectToServer(const std::string& ip, int port) {
    int socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFd < 0) throw std::runtime_error(std::strerror(errno));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &address.sin_addr) != 1) {
        close(socketFd);
        throw std::runtime_error("Invalid server IP address");
    }

    if (connect(socketFd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        std::string error = std::strerror(errno);
        close(socketFd);
        throw std::runtime_error(error);
    }
    return socketFd;
}

void Proxy::printMessage(const char* direction, const Message& message) {
    std::cout << "[MITM] " << direction << ' ';
    switch (message.type()) {
    case MessageType::Login:
        std::cout << "login "
                  << static_cast<const LoginMessage&>(message).get_username();
        break;
    case MessageType::User: {
        const auto& user = static_cast<const UserMessage&>(message);
        std::cout << "message to @" << user.get_username() << ": "
                  << user.get_message();
        break;
    }
    case MessageType::Who:
        std::cout << "/who";
        break;
    case MessageType::Server:
        std::cout << "server: "
                  << static_cast<const ServerMessage&>(message).get_text();
        break;
    default:
        std::cout << "message type "
                  << static_cast<int>(message.type());
        break;
    }
    std::cout << '\n';
}

void Proxy::sendEncrypted(int socket, const Message& message,
                          const DHE::AES_KEY& key) {
    CiphertextMsg encrypted(message, key);
    send_message(socket, encrypted);
}

void Proxy::mitm(int clientSocket, int serverSocket) {
    auto serverParameters = receive_message(serverSocket);
    auto serverKeyMessage = receive_message(serverSocket);
    if (serverParameters->type() != MessageType::DHParametersMsg ||
        serverKeyMessage->type() != MessageType::DHKeyMsg) {
        throw std::runtime_error("Unexpected server handshake");
    }

    const auto& parameters = static_cast<const DHParametersMessage&>(*serverParameters);
    auto clientParameters = parameters.get_parameters();
    auto mitmKeyPair = DHE::getNewKeyPair(clientParameters);
    const auto& serverKey = static_cast<const DHKeyMessage&>(*serverKeyMessage);
    auto serverPublicKey = serverKey.get_public_key();
    DHE::AES_KEY serverAesKey = DHE::deriveAESKey(
        mitmKeyPair.get_private_key(), serverPublicKey.get(), clientParameters.get_p());

    send_message(clientSocket, *serverParameters);
    auto clientKeyMessage = receive_message(clientSocket);
    if (clientKeyMessage->type() != MessageType::DHKeyMsg) {
        throw std::runtime_error("Unexpected client handshake");
    }

    const auto& clientKey = static_cast<const DHKeyMessage&>(*clientKeyMessage);
    auto clientPublicKey = clientKey.get_public_key();
    DHE::AES_KEY clientAesKey = DHE::deriveAESKey(
        mitmKeyPair.get_private_key(), clientPublicKey.get(), clientParameters.get_p());

    DHKeyMessage mitmPublicKey(mitmKeyPair.get_public_key());
    send_message(serverSocket, mitmPublicKey);
    send_message(clientSocket, mitmPublicKey);

    pollfd sockets[] = {
        {clientSocket, POLLIN, 0},
        {serverSocket, POLLIN, 0}
    };

    while (true) {
        if (poll(sockets, 2, -1) < 0) {
            if (errno == EINTR) continue;
            throw std::runtime_error(std::strerror(errno));
        }

        for (int index = 0; index < 2; ++index) {
            if (!(sockets[index].revents & (POLLIN | POLLHUP | POLLERR))) continue;

            auto encrypted = receive_message(sockets[index].fd);
            if (encrypted->type() != MessageType::Ciphertext) {
                throw std::runtime_error("Expected encrypted application message");
            }

            const auto& ciphertext = static_cast<const CiphertextMsg&>(*encrypted);
            const bool fromClient = index == 0;
            const DHE::AES_KEY& decryptKey = fromClient ? clientAesKey : serverAesKey;
            const DHE::AES_KEY& encryptKey = fromClient ? serverAesKey : clientAesKey;
            auto message = ciphertext.getMsg(decryptKey);
            printMessage(fromClient ? "client -> server:" : "server -> client:", *message);
            sendEncrypted(fromClient ? serverSocket : clientSocket, *message, encryptKey);
        }
    }
}

void Proxy::handleConnection(int clientSocket) {
    try {
        int serverSocket = connectToServer(serverIp, serverPort);
        std::cout << "Client connected; relaying traffic\n";
        mitm(clientSocket, serverSocket);
        close(serverSocket);
    } catch (const std::exception& error) {
        std::cerr << "Connection error: " << error.what() << '\n';
    }
    close(clientSocket);
}

void Proxy::acceptConnections() {
    while (true) {
        int clientSocket = accept(listener, nullptr, nullptr);
        if (clientSocket < 0) {
            if (errno == EINTR) continue;
            throw std::runtime_error(std::strerror(errno));
        }
        handleConnection(clientSocket);
    }
}

void Proxy::run() {
    listener = createListener(listenPort);
    std::cout << "Proxy listening on port " << listenPort
              << ", forwarding to " << serverIp << ':' << serverPort << '\n';
    acceptConnections();
}
