#ifndef SERVER_HPP
#define SERVER_HPP

#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <vector>
#include <string>
#include "user.hpp"

class Server{
    const static int MAX_EVENTS = 1024;
    const static int BUFFER_SIZE = 1024;

    int PORT_NO;
    sockaddr_in serverAddress;
    int serverSocket = -1;

    std::vector<pollfd> fds;
    std::vector<User> users;

public:
    Server(int _port_no): PORT_NO(_port_no) {}
    void startServer();
    void stopServer();
    void eventLoop();

private:
    void accept_new_clients();
    void handle_client(int client_fd);
    void close_client(int client_fd);
    User* find_user(int client_fd);
    User* find_user(const std::string& username);
};

#endif // SERVER_HPP