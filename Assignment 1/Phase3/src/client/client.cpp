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

Client::Client(std::string certificate_directory)
        : clientSocket(-1), certificateDirectory(std::move(certificate_directory)),
            terminalUi(std::make_shared<TerminalUI>()), running(true) {}

Client::~Client(){
    closeConnection();
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
        closeConnection();
        throw std::runtime_error("Invalid IP address");
    }

    if (connect(clientSocket, (struct sockaddr*)&serverAddress,
                sizeof(serverAddress)) < 0) {
        closeConnection();
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
            closeConnection();
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
            validateServerCertificate(cert_msg.get_certificate());
            break;
        }
        default:
            terminalUi->printOutput(Presentation::server_message("active users requested"));
        }
    } catch (const std::exception& error) {
        terminalUi->printOutput(Presentation::error_message(std::string("recv: ") + error.what()));
        running = false;
        closeConnection();
    }
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

    server_public_key = DHE::DHKey(DHE::BytesToBIGNUM(serverPublicKeyBytes));
    server_aes_key = DHE::deriveAESKey(
        dh_keypair.get_private_key(), server_public_key.get(), dh_params.get_p());
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
        terminalUi->printOutput(Presentation::error_message(std::string("send: ") + error.what()));
        running = false;
        closeConnection();
    }
}