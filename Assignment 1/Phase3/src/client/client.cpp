#include "client.hpp"
#include "chat/command.hpp"
#include "chat/protocol.hpp"
#include "chat/presentation.hpp"
#include "crypto/certificate.hpp"
#include "crypto/signature.hpp"
#include <openssl/x509.h>
#include <arpa/inet.h>
#include <stdexcept>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <filesystem>
#include <iostream>

Client::Client(std::string certificate_directory)
        : clientSocket(-1), certificateDirectory(std::move(certificate_directory)),
            terminalUi(std::make_shared<TerminalUI>()), running(true) {}

Client::~Client(){
    closeConnection();
}

void Client::connectToServer(std::string IP, int port){
    log_handshake("Connecting to " + IP + ":" + std::to_string(port));
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        log_handshake(std::string("Socket creation failed: ") + std::strerror(errno));
        throw std::runtime_error(std::strerror(errno));
    }

    serverAddress = {};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    if (inet_pton(AF_INET, IP.c_str(), &serverAddress.sin_addr) != 1) {
        log_handshake("Connection failed: invalid IP address");
        closeConnection();
        throw std::runtime_error("Invalid IP address");
    }

    if (connect(clientSocket, (struct sockaddr*)&serverAddress,
                sizeof(serverAddress)) < 0) {
        log_handshake(std::string("Connection failed: ") + std::strerror(errno));
        closeConnection();
        throw std::runtime_error(std::strerror(errno));
    }
    log_handshake("TCP connection established");
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
            log_handshake(std::string("Poll failed: ") + std::strerror(errno));
            closeConnection();
            throw std::runtime_error(std::strerror(errno));
        }

        if (fds[0].revents & (POLLIN | POLLHUP | POLLERR)) {
            handleInput();
        }

        if (fds[1].revents & (POLLHUP | POLLERR | POLLNVAL)) {
            log_handshake("Server connection closed unexpectedly");
            running = false;
            closeConnection();
            return;
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
    fds.clear();
    if (terminalUi != nullptr) terminalUi.reset();
}

void Client::handleInput(){
    std::string promptMessage;
    while (terminalUi->readInput(promptMessage)) {
        try {
            ParsedCommand command = CommandParser::parse(promptMessage, currentChat);
            switch (command.type) {
            case CommandType::Quit:
                running = false;
                closeConnection();
                return;
			case CommandType::Chat:
                currentChat = command.chatUsername;
                terminalUi->printOutput(Presentation::chat_switched(currentChat));
                break;
			case CommandType::Clear:
                terminalUi->clearOutput();
                break;
			case CommandType::Help:
                terminalUi->printOutput(Presentation::help_message());
                break;
			case CommandType::Login:
                pendingUsername = static_cast<LoginMessage*>(command.message.get())->get_username();
                handle_send(*command.message);
                break;
			case CommandType::Who:
			case CommandType::UserMessage:
            case CommandType::CurrentChat:
				if (command.type == CommandType::UserMessage) {
                    currentChat = static_cast<UserMessage*>(command.message.get())->get_username();
                    terminalUi->printOutput(Presentation::chat_switched(currentChat));
                }
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

        if(message->type() == MessageType::Ciphertext) {
            CiphertextMsg& ciphertext_msg = static_cast<CiphertextMsg&>(*message);
            message = ciphertext_msg.getMsg(server_aes_key);
        }

        switch (message->type()) {
        case MessageType::User: {
            auto& user = static_cast<UserMessage&>(*message);
            terminalUi->printOutput(Presentation::user_message(user, username, currentChat));
            break;
        }
        case MessageType::Login: {
            auto& login = static_cast<LoginMessage&>(*message);
            terminalUi->printOutput(Presentation::server_message("login " + login.get_username()));
            break;
        }
        case MessageType::Server: {
            auto& server = static_cast<ServerMessage&>(*message);
            if (server.get_text().rfind("Hi @", 0) == 0 &&
                server.get_text().find(", Login successful.") != std::string::npos)
                username = pendingUsername;
            terminalUi->printOutput(Presentation::server_message(server.get_text()));
            break;
        }
        case MessageType::DHParametersMsg: {
            auto& dh_params_msg = static_cast<DHParametersMessage&>(*message);
            dh_params = dh_params_msg.get_parameters();
            dh_keypair = DHE::getNewKeyPair(dh_params);
            log_handshake("Received DH parameters and generated key pair");
            break;
        }
        case MessageType::DHKeyMsg:{
            auto& dh_key_msg = static_cast<DHKeyMessage&>(*message);
            server_public_key = dh_key_msg.get_public_key();
            server_aes_key = DHE::deriveAESKey(
                dh_keypair.get_private_key(), server_public_key.get(), dh_params.get_p());
            secureChannelEstablished = true;
            break;
        }
        case MessageType::BSignatureMsg: {
            handle_signature_message(static_cast<const BSignatureMessage&>(*message));
            break;
        }
        case MessageType::CertificateMsg: {
            auto& cert_msg = static_cast<CertificateMessage&>(*message);
            log_handshake("Received server certificate (" +
                          std::to_string(cert_msg.get_certificate().size()) +
                          " bytes)");
            validateServerCertificate(cert_msg.get_certificate());
            break;
        }
        default:
            terminalUi->printOutput(Presentation::server_message("active users requested"));
        }
    } catch (const std::exception& error) {
        terminalUi->printOutput(Presentation::error_message(
            std::string("Receive failed: ") + error.what()));
        log_handshake("Closing connection after receive failure");
        running = false;
        closeConnection();
    }
}

void Client::log_handshake(const std::string& message) {
    terminalUi->printOutput("[Client] " + message + "\n");
}

void Client::handle_signature_message(const BSignatureMessage& message) {
    const auto& serverPublicKeyBytes = message.get_public_key_bytes();
    const char* version = "MYCHAT-HANDSHAKE-V1";
    std::vector<uint8_t> transcript(version, version + strlen(version));
    transcript.insert(transcript.end(), handshake_challenge.begin(),
                      handshake_challenge.end());
    auto clientPublicKeyBytes = DHE::BIGNUMToBytes(dh_keypair.get_public_key());
    transcript.insert(transcript.end(), clientPublicKeyBytes.begin(),
                      clientPublicKeyBytes.end());
    transcript.insert(transcript.end(), serverPublicKeyBytes.begin(),
                      serverPublicKeyBytes.end());

    if (!Signature::verify(serverPublicKey, transcript, message.get_signature()))
        throw std::runtime_error("Server handshake signature verification failed");
    log_handshake("Server signature verified successfully");

    server_public_key = DHE::DHKey(DHE::BytesToBIGNUM(serverPublicKeyBytes));
    server_aes_key = DHE::deriveAESKey(
        dh_keypair.get_private_key(), server_public_key.get(), dh_params.get_p());
    secureChannelEstablished = true;
    log_handshake("Secure channel established");
}

void Client::validateServerCertificate(const std::vector<uint8_t>& certBytes) {
    serverCertificate = Certificate::parseCertificate(certBytes);
    if (!serverCertificate) {
        close(clientSocket);
        throw std::runtime_error("Failed to parse server certificate");
    }
    if (!Certificate::validateCertificate(
        serverCertificate,
        (std::filesystem::path(certificateDirectory) / "ca.crt").string(),
        "chat-server"))
    {
        X509_free(serverCertificate);
        close(clientSocket);

        throw std::runtime_error("Server certificate validation failed");
    }
    log_handshake("Server certificate validated successfully");

    serverPublicKey = Certificate::getPublicKey(
        serverCertificate
    );

    if (!serverPublicKey) {
        X509_free(serverCertificate);
        close(clientSocket);
        throw std::runtime_error("Failed to extract public key from server certificate");
    }

    auto R = DHE::generateChallenge();
    handshake_challenge = R;
    RAMessage ra_msg(dh_keypair.get_public_key(), R);
    log_handshake("Sending R,A handshake message");
    handle_send(ra_msg, false);
}

void Client::handle_send(const Message& message, bool encrypt){
    if (encrypt && !secureChannelEstablished)
        throw std::runtime_error("Secure channel is not established");
    try {
        if(encrypt){
            CiphertextMsg ciphertext_msg(message, server_aes_key);
            send_message(clientSocket, ciphertext_msg);
        } else{
            send_message(clientSocket, message);
        }
    } catch (const std::exception& error) {
        terminalUi->printOutput(Presentation::error_message(
            std::string("Send failed: ") + error.what()));
        log_handshake("Closing connection after send failure");
        running = false;
        closeConnection();
    }
}