#include "server.hpp"
#include "utils/helpers.hpp"
#include <iostream>

int main(int argc, char* argv[])
{
    if (argc > 3) {
        std::cerr << "Usage: run_server [port] [certificate-directory]\n";
        return 1;
    }

    try {
        int port = argc >= 2 ? parse_port(argv[1]) : 12345;
        std::string certificateDirectory = argc == 3 ? argv[2] : "certs";
        Server server(port, certificateDirectory);
        server.startServer();
        server.eventLoop();
        server.stopServer();
    } catch (const std::exception& error) {
        std::cerr << "Server error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}