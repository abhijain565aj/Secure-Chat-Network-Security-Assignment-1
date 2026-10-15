#ifndef PROXY_HPP
#define PROXY_HPP

#include "crypto/dhe.hpp"
#include <string>

class Message;

class Proxy {
    int listenPort;
    std::string serverIp;
    int serverPort;
    int listener = -1;

public:
    Proxy(int listenPort, std::string serverIp, int serverPort);
    ~Proxy();

    void run();

private:
    void acceptConnections();
    void handleConnection(int clientSocket);
    void mitm(int clientSocket, int serverSocket);

    static int createListener(int port);
    static int connectToServer(const std::string& ip, int port);
    static void printMessage(const char* direction, const Message& message);
    static void sendEncrypted(int socket, const Message& message,
                              const DHE::AES_KEY& key);
};

#endif // PROXY_HPP
