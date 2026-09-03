#ifndef SERVER_HPP
#define SERVER_HPP

#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <vector>
class Server{
    const static int MAX_EVENTS = 1024;
    const static int BUFFER_SIZE = 1024;

    int PORT_NO;
    sockaddr_in serverAddress;
    int serverSocket = -1;

    std::vector<pollfd> fds;

public:
    Server(int _port_no): PORT_NO(_port_no) {}
    void startServer();
    void stopServer();
    void eventLoop();

private:
    void accept_new_clients();
    void handle_client(int client_fd);
    void close_client(int client_fd);
};

#endif // SERVER_HPP