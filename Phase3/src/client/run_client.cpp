#include "client.hpp"
#include "utils/helpers.hpp"
#include <iostream>

int main(int argc, char* argv[])
{
    if (argc > 4) {
        std::cerr << "Usage: run_client [ip] [port] [certificate-directory]\n";
        return 1;
    }

    try {
        std::string ip = argc >= 2 ? argv[1] : "127.0.0.1";
        int port = argc >= 3 ? parse_port(argv[2]) : 12345;
        std::string certificateDirectory = argc == 4 ? argv[3] : "certs";

        Client client(certificateDirectory);
        client.connectToServer(ip, port);
        client.eventLoop();
        client.closeConnection();
    } catch (const std::exception& error) {
        std::cerr << "Client error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}