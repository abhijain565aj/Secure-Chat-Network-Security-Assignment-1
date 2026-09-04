#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <vector>
#include <string>
#include <memory>
#include "terminal_ui.hpp"
#include "chat/message.hpp"
#include "crypto/dhe.hpp"

class Client{
    const static int BUFFER_SIZE = 1024;

    int clientSocket;
    std::vector<pollfd> fds;
    sockaddr_in serverAddress;
    std::shared_ptr<TerminalUI> terminalUi;
    std::string username;
    std::string pendingUsername;
    std::string currentChat;
    bool running;
    
    DHE::DHParameters dh_params;
    DHE::DHKeyPair dh_keypair;
    DHE::DHKey server_public_key;
    DHE::AES_KEY server_aes_key;
    std::array<uint8_t, 32> handshake_challenge{};
    bool secureChannelEstablished = false;

    X509* serverCertificate;
    EVP_PKEY* serverPublicKey;

public:
    Client();
    ~Client();
    void connectToServer(std::string IP, int port);
    void eventLoop();
    void closeConnection();
    
private:
    void handleInput();
    void handle_recv();
    void handle_send(const Message& message, bool encrypt = true);
    void handle_signature_message(const BSignatureMessage& message);
    void validateServerCertificate(const std::vector<uint8_t>& certBytes);
};

#endif // CLIENT_HPP