#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include "terminal_ui.hpp"
#include "chat/message.hpp"
#include "crypto/dhe.hpp"
#include "crypto/e2e_session.hpp"
#include "crypto/rekey_manager.hpp"

class Client{
    const static int BUFFER_SIZE = 1024;

    int clientSocket;
    std::string certificateDirectory;
    std::vector<std::string> pendingHandshakeLogs;
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
    std::unordered_map<std::string, E2E::Session> e2eSessions;
    std::unordered_map<std::string, RekeyManager> rekeyManagers;

    X509* serverCertificate;
    EVP_PKEY* serverPublicKey;

public:
    explicit Client(std::string certificate_directory = "certs");
    ~Client();
    void connectToServer(std::string IP, int port);
    void eventLoop();
    void closeConnection();
    
private:
    void handleInput();
    void handle_recv();
    void handle_send(const Message& message, bool encrypt = true);
    void perform_handshake();
    void flush_handshake_logs();
    void handle_signature_message(const BSignatureMessage& message);
    void begin_e2e(const std::string& peerUsername);
    void handle_e2e_message(const UserMessage& message);
    void handle_rekey_timer();
    void log_key_rotation(const std::string& peerUsername,
                          const E2E::Session& session);
    void log_handshake(const std::string& message);
    void validateServerCertificate(const std::vector<uint8_t>& certBytes);
};

#endif // CLIENT_HPP