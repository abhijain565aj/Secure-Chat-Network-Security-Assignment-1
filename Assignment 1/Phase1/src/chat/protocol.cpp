#include "protocol.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <sys/socket.h>

namespace {
const uint32_t MAX_FRAME_SIZE = 1024;

void append_uint32(std::vector<uint8_t>& frame, uint32_t value) {
    uint32_t networkValue = htonl(value);
    const auto* bytes = reinterpret_cast<const uint8_t*>(&networkValue);
    frame.insert(frame.end(), bytes, bytes + sizeof(networkValue));
}

uint32_t read_uint32(const std::vector<uint8_t>& frame, size_t& offset) {
    if (offset + sizeof(uint32_t) > frame.size()) throw std::runtime_error("Invalid message frame");
    uint32_t networkValue;
    std::memcpy(&networkValue, frame.data() + offset, sizeof(networkValue));
    offset += sizeof(networkValue);
    return ntohl(networkValue);
}

void append_string(std::vector<uint8_t>& frame, const std::string& value) {
    append_uint32(frame, static_cast<uint32_t>(value.size()));
    frame.insert(frame.end(), value.begin(), value.end());
}

std::string read_string(const std::vector<uint8_t>& frame, size_t& offset) {
    uint32_t length = read_uint32(frame, offset);
    if (length > MESSAGE_MAX_LENGTH || offset + length > frame.size())
        throw std::runtime_error("Invalid message field length");
    std::string value(reinterpret_cast<const char*>(frame.data() + offset), length);
    offset += length;
    return value;
}

void send_all(int socket, const uint8_t* data, size_t size) {
    size_t sent = 0;
    while (sent < size) {
        ssize_t result = send(socket, data + sent, size - sent, 0);
        if (result <= 0) throw std::runtime_error(std::strerror(errno));
        sent += static_cast<size_t>(result);
    }
}

void receive_all(int socket, uint8_t* data, size_t size) {
    size_t received = 0;
    while (received < size) {
        ssize_t result = recv(socket, data + received, size - received, 0);
        if (result <= 0) throw std::runtime_error("Connection closed while reading message");
        received += static_cast<size_t>(result);
    }
}
}

std::vector<uint8_t> serialize_message(const Message& message) {
    std::vector<uint8_t> frame{static_cast<uint8_t>(message.type())};
    if (const auto* user = dynamic_cast<const UserMessage*>(&message)) {
        append_string(frame, user->get_sender());
        append_string(frame, user->get_username());
        append_string(frame, user->get_message());
    } else if (const auto* login = dynamic_cast<const LoginMessage*>(&message)) {
        append_string(frame, login->get_username());
    } else if (const auto* server = dynamic_cast<const ServerMessage*>(&message)) {
        append_string(frame, server->get_text());
    } else if (!dynamic_cast<const WhoMessage*>(&message)) {
        throw std::runtime_error("Unknown message type");
    }
    if (frame.size() > MAX_FRAME_SIZE) throw std::runtime_error("Message frame is too large");
    return frame;
}

std::unique_ptr<Message> deserialize_message(const std::vector<uint8_t>& frame) {
    if (frame.empty() || frame.size() > MAX_FRAME_SIZE) throw std::runtime_error("Invalid message frame");
    size_t offset = 1;
    auto type = static_cast<MessageType>(frame[0]);
    std::unique_ptr<Message> message;
    if (type == MessageType::User) {
        std::string sender = read_string(frame, offset);
        std::string recipient = read_string(frame, offset);
        std::string text = read_string(frame, offset);
        message = std::make_unique<UserMessage>(sender, recipient, text);
    } else if (type == MessageType::Login) {
        message = std::make_unique<LoginMessage>(read_string(frame, offset));
    } else if (type == MessageType::Who) {
        message = std::make_unique<WhoMessage>();
    } else if (type == MessageType::Server) {
        message = std::make_unique<ServerMessage>(read_string(frame, offset));
    } else {
        throw std::runtime_error("Unknown message type");
    }
    if (offset != frame.size()) throw std::runtime_error("Unexpected message data");
    return message;
}

void send_message(int socket, const Message& message) {
    auto frame = serialize_message(message);
    uint32_t size = htonl(static_cast<uint32_t>(frame.size()));
    send_all(socket, reinterpret_cast<const uint8_t*>(&size), sizeof(size));
    send_all(socket, frame.data(), frame.size());
}

std::unique_ptr<Message> receive_message(int socket) {
    uint32_t networkSize;
    receive_all(socket, reinterpret_cast<uint8_t*>(&networkSize), sizeof(networkSize));
    uint32_t size = ntohl(networkSize);
    if (size == 0 || size > MAX_FRAME_SIZE) throw std::runtime_error("Invalid message size");
    std::vector<uint8_t> frame(size);
    receive_all(socket, frame.data(), frame.size());
    return deserialize_message(frame);
}
