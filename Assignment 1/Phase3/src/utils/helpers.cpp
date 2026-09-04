#include "utils/helpers.hpp"
#include <stdexcept>

int parse_port(const std::string& value) {
    try {
        std::size_t parsedLength = 0;
        int port = std::stoi(value, &parsedLength);
        if (parsedLength != value.size())
            throw std::invalid_argument("trailing characters");
        if (port < 1 || port > 65535)
            throw std::out_of_range("port");
        return port;
    } catch (...) {
        throw std::runtime_error("Port must be a number between 1 and 65535");
    }
}

std::time_t get_timestamp() {
    return std::time(nullptr);
}
