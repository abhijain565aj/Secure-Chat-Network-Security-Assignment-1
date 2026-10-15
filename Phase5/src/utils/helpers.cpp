#include "utils/helpers.hpp"
#include <stdexcept>
#include <cctype>
#include <string>
#include <iomanip>
#include <sstream>

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

std::string to_hex(const std::string& s) {
    std::ostringstream oss;
    for (unsigned char c : s) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)c << " ";
    }
    std::string result = oss.str();
    if (!result.empty()) {
        result.pop_back();
    }
    return result;
}

std::string describe_payload(const std::string& payload) {
    bool readable = true;
    for (unsigned char value : payload) {
        if (!std::isprint(value) && value != '\n' && value != '\r' && value != '\t') {
            readable = false;
            break;
        }
    }
    std::string content;
    if (readable) {
        content = payload;
    } else {
        const size_t byte_count = 8;
        if (payload.size() <= byte_count * 2) {
            content = "<opaque/binary data: " + to_hex(payload) + ">";
        } else {
            std::string head = payload.substr(0, byte_count);
            std::string tail = payload.substr(payload.size() - byte_count);
            content = "<opaque/binary data: " + to_hex(head) + " ... " + to_hex(tail) + ">";
        }
    }
    return std::to_string(payload.size()) + " bytes: " + content;
}
