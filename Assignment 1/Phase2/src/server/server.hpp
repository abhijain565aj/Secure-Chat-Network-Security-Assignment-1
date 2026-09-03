#ifndef SERVER_HPP
#define SERVER_HPP

#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <vector>
#include <string>
#include "chat/message.hpp"
#include "user_registry.hpp"
#include "crypto/dhe.hpp"

class Server{
    const static int MAX_EVENTS = 1024;
    const static int BUFFER_SIZE = 1024;

    int PORT_NO;
    sockaddr_in serverAddress;
    int serverSocket = -1;

    std::vector<pollfd> fds;
    UserRegistry users;

    DHE::DHParameters dh_params;
    DHE::DHKeyPair dh_keypair;

public:
    Server(int _port_no): PORT_NO(_port_no) {}
    ~Server();
    void startServer();
    void stopServer();
    void eventLoop();

private:
    void accept_new_clients();
    void handle_client(int client_fd);
    void close_client(int client_fd);
    void handle_login(int client_fd, User& user, const LoginMessage& message);
    void handle_user_message(int client_fd, User& user, const UserMessage& message);
    void handle_who(int client_fd, const User& user);
    void log_request(int client_fd, const Message& message) const;
    void addClientPublicKey(int client_fd, DHE::DHKey public_key);

};

#endif // SERVER_HPP