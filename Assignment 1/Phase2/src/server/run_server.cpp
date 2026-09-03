#include "server.hpp"
#include "utils/helpers.hpp"
#include <iostream>

int main(int argc, char* argv[])
{
    if (argc > 2) {
        std::cerr << "Usage: run_server [port]\n";
        return 1;
    }

    try {
        int port = argc == 2 ? parse_port(argv[1]) : 12345;
        Server server(port);
        server.startServer();
        server.eventLoop();
        server.stopServer();
    } catch (const std::exception& error) {
        std::cerr << "Server error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}