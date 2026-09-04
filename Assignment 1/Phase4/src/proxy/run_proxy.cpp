#include "proxy.hpp"
#include <iostream>
#include <stdexcept>

int main(int argc, char* argv[]) {
    if (argc != 4 && argc != 5 && argc != 6) {
        std::cerr << "Usage: run_proxy [normal] <listen-port> <server-ip> "
                     "<server-port> [certificate-directory]\n";
        return 1;
    }

    try {
        ProxyMode mode = ProxyMode::Normal;
        int argumentOffset = 0;
        if (argc >= 5) {
            if (std::string(argv[1]) == "fake") mode = ProxyMode::Fake;
            else if (std::string(argv[1]) == "thief") mode = ProxyMode::Thief;
            else if (std::string(argv[1]) != "normal")
                throw std::runtime_error("Proxy mode must be 'normal', 'fake', or 'thief'");
            argumentOffset = 1;
        }
        std::string certificateDirectory = argc == 6
            ? argv[5] : "build/proxy-certs";
        Proxy proxy(mode, std::stoi(argv[1 + argumentOffset]),
                    argv[2 + argumentOffset],
                    std::stoi(argv[3 + argumentOffset]),
                    certificateDirectory);
        proxy.run();
    } catch (const std::exception& error) {
        std::cerr << "Proxy error: " << error.what() << '\n';
        return 1;
    }
}
