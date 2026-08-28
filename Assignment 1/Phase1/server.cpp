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

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT_NO);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (struct sockaddr*)&serverAddress,
        sizeof(serverAddress));
}