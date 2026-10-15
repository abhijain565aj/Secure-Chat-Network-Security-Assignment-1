
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
#include <filesystem>
#include <arpa/inet.h>
#include "crypto/certificate.hpp"
#include "crypto/signature.hpp"
#include <cctype>
#include <string>
#include <iomanip>
#include <sstream>

Server::Server(int _port_no, std::string certificate_directory)
    : PORT_NO(_port_no), certificateDirectory(std::move(certificate_directory)) {
    const std::filesystem::path certificatePath(certificateDirectory);
    serverCertificate =
        Certificate::loadCertificate(
            (certificatePath / "server.crt").string()
        );

    serverPrivateKey =
        Signature::loadPrivateKey(
            (certificatePath / "server.key").string()
        );

    if (!serverCertificate || !serverPrivateKey) {
        throw std::runtime_error("Failed to load server credentials");
    }
}

Server::~Server(){
    stopServer();
}

void Server::startServer(){
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        throw std::runtime_error(std::strerror(errno));
    }

    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        stopServer();
        throw std::runtime_error(std::strerror(errno));
    }

    serverAddress = {};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT_NO);
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(serverSocket, (struct sockaddr*)&serverAddress,
             sizeof(serverAddress)) < 0) {
        stopServer();
        throw std::runtime_error(std::strerror(errno));
    }

    if (listen(serverSocket, SOMAXCONN) < 0) {
        stopServer();
        throw std::runtime_error(std::strerror(errno));
    }

    dh_params = DHE::getNewDHParameters();
    dh_keypair = DHE::getNewKeyPair(dh_params);
}

void Server::stopServer(){
    for (const auto& pfd : fds) {
        if (pfd.fd >= 0 && pfd.fd != serverSocket) close(pfd.fd);
    }
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
            stopServer();
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
    
    //send the DH parameters to the client
    User* user = users.find_by_file_descriptor(clientSocket);
    if (user == nullptr) {
        close_client(clientSocket);
        throw std::runtime_error("Unknown client");
    }

    auto certificate =
        Certificate::readCertificateBytes(
            (std::filesystem::path(certificateDirectory) / "server.crt").string()
        );
    if (certificate.empty()) {
        throw std::runtime_error("Failed to read server certificate");
    }
    std::cout << Presentation::server_log(
        "Loaded server certificate (" + std::to_string(certificate.size()) +
        " bytes)") << std::endl;

    // send the DH parameters and server certificate to the client
    DHParametersMessage dh_params_message(dh_params);
    handle_send(user, dh_params_message, false);
    CertificateMessage certificateMessage(certificate);
    handle_send(user, certificateMessage, false);
    std::cout << Presentation::server_log(
        "Sent DH parameters and certificate to client (fd " +
        std::to_string(clientSocket) + ")") << std::endl;

    std::cout << Presentation::server_log(
        "Sent DH parameters to client (fd " +
        std::to_string(clientSocket) + ")") << std::endl;
}

void Server::handle_client(int client_fd){
    try {
        std::unique_ptr<Message> message = receive_message(client_fd);
        User* user = users.find_by_file_descriptor(client_fd);
        if (user == nullptr) throw std::runtime_error("Unknown client");

        if(message->type() == MessageType::Ciphertext) {
            CiphertextMsg& ciphertext_msg = static_cast<CiphertextMsg&>(*message);
            message = ciphertext_msg.getMsg(user->get_aes_key());
        }
        log_request(client_fd, *message);

        switch (message->type()){
        case MessageType::RAMsg: {
            auto& ra_msg = static_cast<RAMessage&>(*message);
            handle_RAMessage(user, ra_msg);
            break;
        }
        case MessageType::Login:{
            handle_login(*user, static_cast<LoginMessage&>(*message));
            break;
        }
        default:
            if (!user->is_logged_in()) {
            ServerMessage response("Please login first.");
            handle_send(user, response);
            std::cout << Presentation::server_log(
                "Rejected request from fd " + std::to_string(client_fd) +
                ": login required") << std::endl;
                break;
            }
            switch (message->type()) {
            case MessageType::User:
                handle_user_message(client_fd, *user, static_cast<UserMessage&>(*message));
                break;
            case MessageType::Who:
                handle_who(client_fd, *user);
                break;
            default: {
                ServerMessage response("Unsupported message.");
                handle_send(user, response);
                break;
            }
            }
        }
    } catch (const std::exception& error) {
            std::cerr << Presentation::server_log(std::string("message: ") + error.what())
                      << std::endl;
        close_client(client_fd);
    }
}

void Server::handle_RAMessage(User* user, const RAMessage& ra_msg) {
    add_client_public_key(user->get_file_descriptor(), ra_msg.get_public_key());
    std::cout << Presentation::server_log(
        "Received client's public key and challenge (fd " +
        std::to_string(user->get_file_descriptor()) + ")") << std::endl;
    
    auto&& R = ra_msg.get_challenge();
    auto&& A_bytes = ra_msg.get_public_key_bytes();
    auto&& B_bytes = DHE::BIGNUMToBytes(dh_keypair.get_public_key());

    std::vector<uint8_t> transcript;
    const char* version = "MYCHAT-HANDSHAKE-V1";

    transcript.insert(transcript.end(),version,version + strlen(version));

    transcript.insert(transcript.end(),R.begin(),R.end());
    transcript.insert(transcript.end(),A_bytes.begin(),A_bytes.end());
    transcript.insert(transcript.end(),B_bytes.begin(),B_bytes.end());

    auto signature = Signature::sign(
        serverPrivateKey,
        transcript
    );
    std::cout << Presentation::server_log(
        "Signed handshake transcript and sending B,signature (fd " +
        std::to_string(user->get_file_descriptor()) + ")") << std::endl;

    BSignatureMessage response(B_bytes, std::move(signature));
    handle_send(user, response, false);
    std::cout << Presentation::server_log(
        "Derived shared key and sent handshake response for fd " +
        std::to_string(user->get_file_descriptor())) << std::endl;
}

void Server::handle_send(User* user, Message& message, bool encrypt) {
    if (user == nullptr) throw std::runtime_error("Unknown client");
    if (encrypt) {
        CiphertextMsg ciphertext_msg(message, user->get_aes_key());
        send_message(user->get_file_descriptor(), ciphertext_msg);
    } else {
        send_message(user->get_file_descriptor(), message);
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

void Server::add_client_public_key(int client_fd, DHE::DHKey public_key) {
    User* user = users.find_by_file_descriptor(client_fd);
    if (user == nullptr) throw std::runtime_error("Unknown client");
    DHE::AES_KEY aes_key = DHE::deriveAESKey(
        dh_keypair.get_private_key(), public_key.get(), dh_params.get_p());
    user->set_public_key(public_key,aes_key);
}

void Server::handle_login(User& user, const LoginMessage& message) {
    if (users.find_logged_in(message.get_username()) != nullptr) {
        ServerMessage response("Username is already in use.");
        handle_send(&user, response);
        return;
    }
    user.login(message.get_username());
    std::cout << Presentation::server_log(
        "User logged in: " + user.get_username()) << std::endl;
    ServerMessage response("Hi @" + user.get_username() + ", Login successful.");
    handle_send(&user, response);
}

void Server::handle_user_message(int client_fd, User& user, const UserMessage& message) {
    User* recipient = users.find_logged_in(message.get_username());
    if (recipient == nullptr) {
        ServerMessage response("Message failed: user is not online.");
        handle_send(&user, response);
        std::cout << Presentation::server_log(
            "Delivery failed: " + user.get_username() + " -> @" +
            message.get_username() + " (user offline)") << std::endl;
        return;
    }
    UserMessage delivered(user.get_username(), message.get_username(), message.get_message());
    try {
        handle_send(recipient, delivered);
        if (recipient->get_file_descriptor() != client_fd)
            handle_send(&user, delivered);
        std::cout << Presentation::server_log(
            "Relayed message: " + user.get_username() + " -> @" +
            message.get_username() + " - " + describe_payload(message.get_message()))
                  << std::endl;
    } catch (const std::exception&) {
        ServerMessage response("Message failed: delivery error.");
        handle_send(&user, response);
        std::cout << Presentation::server_log(
            "Delivery failed: " + user.get_username() + " -> @" +
            message.get_username() + " (send error)") << std::endl;
    }
}

void Server::handle_who(int client_fd, const User&) {
    ServerMessage response("Online users:\n" + users.logged_in_usernames());
    User* user = users.find_by_file_descriptor(client_fd);
    handle_send(user, response);
    std::cout << Presentation::server_log("Processed /who request") << std::endl;
}

void Server::log_request(int client_fd, const Message& message) const {
    std::string request;
    switch (message.type()) {
    case MessageType::Login: {
        const auto& login = static_cast<const LoginMessage&>(message);
        request = "/login " + login.get_username();
        break;
    }
    case MessageType::User: {
        const auto& userMessage = static_cast<const UserMessage&>(message);
        request = "message to @" + userMessage.get_username() + " - " +
                  describe_payload(userMessage.get_message());
        break;
    }
    case MessageType::Who:
        request = "/who";
        break;
    case MessageType::RAMsg:
        request = "handshake R,A";
        break;
    default:
        request = "unknown message";
    }
    std::cout << Presentation::server_log(
        "Request from fd " + std::to_string(client_fd) + ": " + request)
              << std::endl;
}