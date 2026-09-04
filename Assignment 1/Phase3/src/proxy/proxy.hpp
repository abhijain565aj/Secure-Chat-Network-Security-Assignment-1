#ifndef PROXY_HPP
#define PROXY_HPP

#include "crypto/dhe.hpp"
#include <string>

class Message;
enum class ProxyMode { Normal, Fake, Thief };

class Proxy {
    ProxyMode mode;
    int listenPort;
    std::string serverIp;
    int serverPort;
    std::string certificateDirectory;
    int listener = -1;

public:
        Proxy(ProxyMode mode, int listenPort, std::string serverIp, int serverPort,
            std::string certificateDirectory = "build/proxy-certs");
    ~Proxy();

    void run();

private:
    void acceptConnections();
    void handleConnection(int clientSocket);
    void mitm(int clientSocket, int serverSocket);
    void relay(int clientSocket, int serverSocket);

    static int createListener(int port);
    static int connectToServer(const std::string& ip, int port);
    static void printMessage(const char* direction, const Message& message);
    static void sendEncrypted(int socket, const Message& message,
                              const DHE::AES_KEY& key);
};

#endif // PROXY_HPP
