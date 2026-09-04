#include "proxy.hpp"
#include <iostream>
#include <stdexcept>

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: run_proxy <listen-port> <server-ip> <server-port>\n";
        return 1;
    }

    try {
        Proxy proxy(std::stoi(argv[1]), argv[2], std::stoi(argv[3]));
        proxy.run();
    } catch (const std::exception& error) {
        std::cerr << "Proxy error: " << error.what() << '\n';
        return 1;
    }
}
