#include "proxy.hpp"
#include "chat/message.hpp"
#include "chat/protocol.hpp"
#include "crypto/certificate.hpp"
#include "crypto/signature.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <filesystem>
#include <poll.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

Proxy::Proxy(ProxyMode mode, int listenPort, std::string serverIp, int serverPort,
                         std::string certificateDirectory)
        : mode(mode), listenPort(listenPort), serverIp(std::move(serverIp)),
            serverPort(serverPort), certificateDirectory(std::move(certificateDirectory)) {}
constexpr uint32_t MAX_FRAME_SIZE = 4096;


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
    if (mode == ProxyMode::Normal) {
        std::cout << "[Proxy] Normal mode: forwarding raw frames only; "
                     "TLS/DH and application contents are not inspected\n";
        relay(clientSocket, serverSocket);
        return;
    }

    auto serverParameters = receive_message(serverSocket);
    auto serverCertificate = receive_message(serverSocket);
    if (serverParameters->type() != MessageType::DHParametersMsg ||
        serverCertificate->type() != MessageType::CertificateMsg) {
        throw std::runtime_error("Unexpected server handshake");
    }

    const auto& parameters = static_cast<const DHParametersMessage&>(*serverParameters);
    auto clientParameters = parameters.get_parameters();
    if (mode == ProxyMode::Thief) {
        send_message(clientSocket, *serverParameters);
        send_message(clientSocket, *serverCertificate);
        auto clientRequest = receive_message(clientSocket);
        if (clientRequest->type() != MessageType::RAMsg)
            throw std::runtime_error("Unexpected client handshake");
        throw std::runtime_error(
            "Thief mode cannot sign B,signature: stolen certificate has no "
            "server private key");
    }

    const auto certificatePath = std::filesystem::path(certificateDirectory);
    auto proxyCertificateBytes =
        Certificate::readCertificateBytes((certificatePath / "server.crt").string());
    auto proxyPrivateKey =
        Signature::loadPrivateKey((certificatePath / "server.key").string());
    if (proxyCertificateBytes.empty() || proxyPrivateKey == nullptr)
        throw std::runtime_error("Failed to load proxy certificate and private key");

    auto serverKeyPair = DHE::getNewKeyPair(clientParameters);
    auto serverChallenge = DHE::generateChallenge();
    RAMessage serverRequest(serverKeyPair.get_public_key(), serverChallenge);
    send_message(serverSocket, serverRequest);
    auto serverResponse = receive_message(serverSocket);
    if (serverResponse->type() != MessageType::BSignatureMsg)
        throw std::runtime_error("Unexpected server handshake response");
    const auto& serverSignature = static_cast<const BSignatureMessage&>(*serverResponse);
    auto serverPublicKey = DHE::DHKey(
        DHE::BytesToBIGNUM(serverSignature.get_public_key_bytes()));
    DHE::AES_KEY serverAesKey = DHE::deriveAESKey(
        serverKeyPair.get_private_key(), serverPublicKey.get(), clientParameters.get_p());

    send_message(clientSocket, *serverParameters);
    CertificateMessage fakeCertificate(proxyCertificateBytes);
    send_message(clientSocket, fakeCertificate);
    auto clientRequest = receive_message(clientSocket);
    if (clientRequest->type() != MessageType::RAMsg)
        throw std::runtime_error("Unexpected client handshake");
    const auto& clientHandshake = static_cast<const RAMessage&>(*clientRequest);
    auto clientKeyPair = DHE::getNewKeyPair(clientParameters);
    auto clientBBytes = DHE::BIGNUMToBytes(clientKeyPair.get_public_key());
    std::vector<uint8_t> transcript;
    const char* version = "MYCHAT-HANDSHAKE-V1";
    transcript.insert(transcript.end(), version, version + strlen(version));
    transcript.insert(transcript.end(), clientHandshake.get_challenge().begin(),
                      clientHandshake.get_challenge().end());
    transcript.insert(transcript.end(), clientHandshake.get_public_key_bytes().begin(),
                      clientHandshake.get_public_key_bytes().end());
    transcript.insert(transcript.end(), clientBBytes.begin(), clientBBytes.end());
    auto clientSignature = Signature::sign(proxyPrivateKey, transcript);
    BSignatureMessage fakeResponse(clientBBytes, std::move(clientSignature));
    send_message(clientSocket, fakeResponse);
    auto clientPublicKey = clientHandshake.get_public_key();
    DHE::AES_KEY clientAesKey = DHE::deriveAESKey(
        clientKeyPair.get_private_key(), clientPublicKey.get(), clientParameters.get_p());
    EVP_PKEY_free(proxyPrivateKey);
    std::cout << "[Proxy] Fake mode presented proxy-certs/server.crt and "
                 "established separate client/server channels\n";

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
    if (mode == ProxyMode::Normal) {
        std::cout << "  Normal: transparent raw-frame relay; cannot inspect or decrypt content\n";
    } else if (mode == ProxyMode::Fake) {
        std::cout << "  Fake: presents proxy-certs and can decrypt traffic if the client trusts its CA\n";
    } else {
        std::cout << "  Thief: presents the stolen certificate but cannot sign without the server private key\n";
    }
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

void Proxy::relay(int clientSocket, int serverSocket) {
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
            auto message = receive_message(sockets[index].fd);
            send_message(index == 0 ? serverSocket : clientSocket, *message);
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
    const char* modeName = mode == ProxyMode::Normal ? "normal"
        : mode == ProxyMode::Fake ? "fake" : "thief";
    std::cout << "Proxy (" << modeName
              << ") listening on port " << listenPort
              << ", forwarding to " << serverIp << ':' << serverPort << '\n';
    acceptConnections();
}
