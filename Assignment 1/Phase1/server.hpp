#ifndef SERVER_HPP
#define SERVER_HPP

#include <sys/socket.h>
#include <netinet/in.h>
#include "client.hpp"

class Server{
    const static int MAX_WAITING_CLIENTS = 5;
    
    int PORT_NO;
    sockaddr_in serverAddress;
    int serverSocket;
public:
    Server(int _port_no): PORT_NO(_port_no) {}
    void startServer();
    void stopServer();
};

#endif // SERVER_HPP