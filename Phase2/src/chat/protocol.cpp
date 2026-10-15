#include "protocol.hpp"
#include "packet_buffer.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <sys/socket.h>
#include <unordered_map>

namespace {
const uint32_t MAX_FRAME_SIZE = 1024;
using Decoder = std::unique_ptr<Message> (*)(PacketBuffer&);
const std::unordered_map<MessageType, Decoder> DECODERS = {
    {MessageType::User, &UserMessage::deserialize},
    {MessageType::Login, &LoginMessage::deserialize},
    {MessageType::Who, &WhoMessage::deserialize},
    {MessageType::Server, &ServerMessage::deserialize},
    {MessageType::DHParametersMsg, &DHParametersMessage::deserialize},
    {MessageType::DHKeyMsg, &DHKeyMessage::deserialize},
    {MessageType::Ciphertext, &CiphertextMsg::deserialize}
};

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
    PacketBuffer packet(message.type());
    packet << message;
    return packet.bytes();
}

std::unique_ptr<Message> deserialize_message(const std::vector<uint8_t>& frame) {
    PacketBuffer packet(frame);
    auto decoder = DECODERS.find(packet.type());
    if (decoder == DECODERS.end()) throw std::runtime_error("Unknown message type");
    std::unique_ptr<Message> message = decoder->second(packet);
    if (!packet.empty()) throw std::runtime_error("Unexpected message data");
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
