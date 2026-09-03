// #include <cstring>
// #include <iostream>
// #include <netinet/in.h>
// #include <sys/socket.h>
// #include <unistd.h>

// using namespace std;

// int main()
// {
//     // listening to the assigned socket
//     listen(serverSocket, 5);

//     // accepting connection request
//     int clientSocket
//         = accept(serverSocket, nullptr, nullptr);

//     // recieving data
//     char buffer[1024] = { 0 };
//     recv(clientSocket, buffer, sizeof(buffer), 0);
//     cout << "Message from client: " << buffer
//               << endl;

//     // closing the socket.
//     close(serverSocket);

//     return 0;
// }

#include "server.hpp"

void Server::startServer(){
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT_NO);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (struct sockaddr*)&serverAddress,
        sizeof(serverAddress));
}

void Server::stopServer(){
}

void Server::eventLoop(){

    fds.push_back({
        serverSocket,
        POLLIN,
        0
    });

    while (true) {

        [[maybe_unused]] int n = poll(
            fds.data(),
            fds.size(),
            -1
        );

        for (auto &pfd : fds) {

            if (pfd.revents & POLLIN) {
                if (pfd.fd == serverSocket) {
                    accept_new_clients();
                } else {
                    handle_client(pfd.fd);
                }
            }
        }
    }
}

void Server::accept_new_clients(){
    int clientSocket = accept(serverSocket, nullptr, nullptr);

    pollfd pfd;
    pfd.fd = clientSocket;
    pfd.events = POLLIN;
    pfd.revents = 0;

    fds.push_back(pfd);
}

void Server::handle_client(int client_fd){
    char buffer[1024] = { 0 };
    recv(client_fd, buffer, sizeof(buffer), 0);
    std::cout << "Message from client: " << buffer << std::endl;
}