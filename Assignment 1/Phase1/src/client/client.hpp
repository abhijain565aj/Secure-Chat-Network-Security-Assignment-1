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

    void handleInput();

public:
    Client();
    void connectToServer(std::string IP, int port);
    void eventLoop();
    void closeConnection();
    
private:
    void handle_recv();
    void handle_send(const Message& message);
};

#endif // CLIENT_HPP