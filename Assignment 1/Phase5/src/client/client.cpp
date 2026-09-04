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
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace {
bool has_tag(const std::string& value, const char* tag) {
    return value.rfind(tag, 0) == 0;
}
}

Client::Client(std::string certificate_directory)
        : clientSocket(-1), certificateDirectory(std::move(certificate_directory)),
            terminalUi(nullptr), running(true) {}

Client::~Client(){
    closeConnection();
    if (!pendingHandshakeLogs.empty()) {
        std::cerr << "\n--- Client handshake log ---\n";
        for (const auto& message : pendingHandshakeLogs) {
            std::cerr << "[Client] " << message << '\n';
        }
        std::cerr << "--- End client handshake log ---\n";
    }
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
    try {
        perform_handshake();
    } catch (const std::exception& error) {
        log_handshake(std::string("Handshake failed: ") + error.what());
        closeConnection();
        throw;
    }
    terminalUi = std::make_shared<TerminalUI>();
    flush_handshake_logs();
}

void Client::perform_handshake() {
    auto parametersMessage = receive_message(clientSocket);
    if (parametersMessage->type() != MessageType::DHParametersMsg)
        throw std::runtime_error("Expected DH parameters from server");
    auto& parameters = static_cast<DHParametersMessage&>(*parametersMessage);
    dh_params = parameters.get_parameters();
    dh_keypair = DHE::getNewKeyPair(dh_params);
    log_handshake("Received DH parameters and generated key pair");

    auto certificateMessage = receive_message(clientSocket);
    if (certificateMessage->type() != MessageType::CertificateMsg)
        throw std::runtime_error("Expected server certificate");
    auto& certificate = static_cast<CertificateMessage&>(*certificateMessage);
    log_handshake("Received server certificate (" +
                  std::to_string(certificate.get_certificate().size()) +
                  " bytes)");
    validateServerCertificate(certificate.get_certificate());

    auto signatureMessage = receive_message(clientSocket);
    if (signatureMessage->type() != MessageType::BSignatureMsg)
        throw std::runtime_error("Expected signed server handshake response");
    handle_signature_message(
        static_cast<const BSignatureMessage&>(*signatureMessage));
}

void Client::flush_handshake_logs() {
    for (const auto& message : pendingHandshakeLogs) {
        terminalUi->printOutput("[Client] " + message + "\n");
    }
    pendingHandshakeLogs.clear();
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
        handle_rekey_timer();
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
            case CommandType::E2E:
                currentChat = command.chatUsername;
                begin_e2e(currentChat);
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
                if (command.message->type() == MessageType::User) {
                    auto& userMessage = static_cast<UserMessage&>(*command.message);
                    auto session = e2eSessions.find(userMessage.get_username());
                    if (session != e2eSessions.end() && session->second.is_established()) {
                        command.message = std::make_unique<UserMessage>(
                            userMessage.get_username(),
                            session->second.encrypt(userMessage.get_message()));
                    }
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
                if (has_tag(user.get_message(), E2E::INIT_TAG) ||
                    has_tag(user.get_message(), E2E::ACK_TAG) ||
                    has_tag(user.get_message(), E2E::MESSAGE_TAG) ||
                    has_tag(user.get_message(), E2E::REKEY_REQUEST_TAG) ||
                    has_tag(user.get_message(), E2E::REKEY_RESPONSE_TAG) ||
                    has_tag(user.get_message(), E2E::REKEY_CONFIRM_TAG)) {
                    handle_e2e_message(user);
                    break;
                }
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

void Client::begin_e2e(const std::string& peerUsername) {
    if (!secureChannelEstablished)
        throw std::runtime_error("Secure channel is not established");
    E2E::Session session(dh_params);
    const std::string payload = session.begin();
    e2eSessions.insert_or_assign(peerUsername, std::move(session));
    rekeyManagers.insert_or_assign(peerUsername, RekeyManager{});
    UserMessage init(peerUsername, payload);
    handle_send(init);
    terminalUi->printOutput(Presentation::server_message(
        "E2E key exchange started with @" + peerUsername));
}

void Client::handle_e2e_message(const UserMessage& message) {
    const std::string& payload = message.get_message();
    const bool isOwnEcho = message.get_sender() == username;
    const std::string& peerUsername = isOwnEcho
        ? message.get_username() : message.get_sender();
    if (isOwnEcho &&
        (has_tag(payload, E2E::INIT_TAG) || has_tag(payload, E2E::ACK_TAG) ||
         has_tag(payload, E2E::REKEY_REQUEST_TAG) ||
         has_tag(payload, E2E::REKEY_RESPONSE_TAG) ||
         has_tag(payload, E2E::REKEY_CONFIRM_TAG)))
        return;
    if (has_tag(payload, E2E::INIT_TAG)) {
        E2E::Session session(dh_params);
        const std::string ack = session.accept_init(payload);
        e2eSessions.insert_or_assign(peerUsername, std::move(session));
        rekeyManagers.insert_or_assign(peerUsername, RekeyManager{});
        UserMessage response(peerUsername, ack);
        handle_send(response);
        terminalUi->printOutput(Presentation::server_message(
            "E2E session established with @" + peerUsername));
        return;
    }
    auto session = e2eSessions.find(peerUsername);
    if (session == e2eSessions.end())
        throw std::runtime_error("Received E2E data without a session");
    if (session->second.is_rekey_payload(payload)) {
        const std::string previousFingerprint = session->second.key_fingerprint();
        const std::string response = session->second.handle_rekey(payload);
        if (!response.empty()) {
            UserMessage rekeyMessage(peerUsername, response);
            handle_send(rekeyMessage);
            if (has_tag(payload, E2E::REKEY_RESPONSE_TAG)) {
                session->second.confirm_rekey();
                log_key_rotation(peerUsername, session->second);
                rekeyManagers[peerUsername].schedule_next();
            }
        } else if (previousFingerprint != session->second.key_fingerprint()) {
            log_key_rotation(peerUsername, session->second);
            rekeyManagers[peerUsername].schedule_next();
        }
        return;
    }
    if (has_tag(payload, E2E::ACK_TAG)) {
        session->second.accept_ack(payload);
        terminalUi->printOutput(Presentation::server_message(
            "E2E session established with @" + peerUsername));
        return;
    }
    if (has_tag(payload, E2E::MESSAGE_TAG)) {
        UserMessage plaintext(message.get_sender(), message.get_username(),
                               session->second.decrypt(payload));
        terminalUi->printOutput(Presentation::user_message(
            plaintext, username, currentChat));
        return;
    }
    throw std::runtime_error("Unknown E2E wire tag");
}

void Client::handle_rekey_timer() {
    if (!secureChannelEstablished || username.empty()) return;
    for (auto& [peerUsername, session] : e2eSessions) {
        if (!session.is_established() || session.is_rekey_pending() ||
            username >= peerUsername) continue;
        auto manager = rekeyManagers.find(peerUsername);
        if (manager == rekeyManagers.end() || !manager->second.due()) continue;
        const std::string request = session.begin_rekey();
        handle_send(UserMessage(peerUsername, request));
        manager->second.schedule_next();
        log_handshake("E2E rekey started with @" + peerUsername);
    }
}

void Client::log_key_rotation(const std::string& peerUsername,
                              const E2E::Session& session) {
    const std::time_t now = std::time(nullptr);
    std::tm localTime{};
    localtime_r(&now, &localTime);
    std::ostringstream timestamp;
    timestamp << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
    log_handshake("E2E key rotation with @" + peerUsername + " at " +
                  timestamp.str() + " fingerprint=" + session.key_fingerprint());
}

void Client::log_handshake(const std::string& message) {
    if (terminalUi != nullptr) {
        terminalUi->printOutput("[Client] " + message + "\n");
    } else {
        pendingHandshakeLogs.push_back(message);
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
        if (terminalUi != nullptr) {
            terminalUi->printOutput(Presentation::error_message(
                std::string("Send failed: ") + error.what()));
        }
        log_handshake("Closing connection after send failure");
        running = false;
        closeConnection();
        if (terminalUi == nullptr) throw;
    }
}