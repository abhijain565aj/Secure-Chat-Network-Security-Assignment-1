#include "packet_buffer.hpp"
#include "message.hpp"
#include "utils/constants.hpp"
#include <arpa/inet.h>
#include <stdexcept>

namespace {
bool is_message_type(uint8_t value) {
    return value >= static_cast<uint8_t>(MessageType::User) &&
           value <= static_cast<uint8_t>(MessageType::Ciphertext);
}
}

PacketBuffer::PacketBuffer(MessageType type) : buffer{static_cast<uint8_t>(type)}, readOffset(HEADER_SIZE) {}

PacketBuffer::PacketBuffer(const std::vector<uint8_t>& bytes)
    : buffer(bytes), readOffset(HEADER_SIZE) {
    if (buffer.size() < HEADER_SIZE || buffer.size() > MAX_PACKET_SIZE ||
        !is_message_type(buffer.front())) {
        throw std::runtime_error("Invalid packet buffer");
    }
}

MessageType PacketBuffer::type() const {
    return static_cast<MessageType>(buffer.front());
}

const std::vector<uint8_t>& PacketBuffer::bytes() const {
    return buffer;
}

void PacketBuffer::append(const uint8_t* data, std::size_t size) {
    ensure_writable(size);
    buffer.insert(buffer.end(), data, data + size);
}

PacketBuffer& PacketBuffer::operator<<(const std::string& value) {
    if (value.size() > MESSAGE_MAX_LENGTH) throw std::runtime_error("Packet field is too large");
    uint32_t length = htonl(static_cast<uint32_t>(value.size()));
    append(reinterpret_cast<const uint8_t*>(&length), sizeof(length));
    append(reinterpret_cast<const uint8_t*>(value.data()), value.size());
    return *this;
}

PacketBuffer& PacketBuffer::operator<<(const Message& message) {
    message.serialize(*this);
    return *this;
}

PacketBuffer& PacketBuffer::operator>>(std::string& value) {
    uint32_t networkLength;
    *this >> networkLength;
    uint32_t length = ntohl(networkLength);
    if (length > MESSAGE_MAX_LENGTH) {
        throw std::runtime_error("Invalid packet field length");
    }
    ensure_readable(length);
    value.assign(reinterpret_cast<const char*>(buffer.data() + readOffset), length);
    readOffset += length;
    return *this;
}

bool PacketBuffer::empty() const {
    return readOffset == buffer.size();
}

void PacketBuffer::ensure_readable(std::size_t size) const {
    if (size > buffer.size() - readOffset) {
        throw std::runtime_error("Invalid packet data");
    }
}

void PacketBuffer::ensure_writable(std::size_t size) const {
    if (size > MAX_PACKET_SIZE - buffer.size()) {
        throw std::runtime_error("Packet buffer is too large");
    }
}