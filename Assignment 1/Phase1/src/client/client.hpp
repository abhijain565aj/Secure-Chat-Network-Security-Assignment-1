#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <vector>
#include <string>

class Client{
    const static int BUFFER_SIZE = 1024;

    int clientSocket;
    std::vector<pollfd> fds;
    sockaddr_in serverAddress;

public:
    Client();
    void connectToServer(std::string IP, int port);
    void eventLoop();
    void closeConnection();
    
private:
    void handle_recv();
    void handle_send();
};

#endif // CLIENT_HPP