#ifndef PACKET_BUFFER_HPP
#define PACKET_BUFFER_HPP

#include <arpa/inet.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <type_traits>
#include <string>
#include <utility>
#include <vector>

class Message;
enum class MessageType : uint8_t;

class PacketBuffer {
public:
    explicit PacketBuffer(MessageType type);
    explicit PacketBuffer(const std::vector<uint8_t>& bytes);

    MessageType type() const;
    const std::vector<uint8_t>& bytes() const;

    template <typename T>
    PacketBuffer& operator<<(const std::vector<T>& values) {
        if (values.size() > MAX_PACKET_SIZE) {
            throw std::runtime_error("Packet vector is too large");
        }
        uint32_t count = htonl(static_cast<uint32_t>(values.size()));
        append(reinterpret_cast<const uint8_t*>(&count), sizeof(count));
        for (const T& value : values) {
            *this << value;
        }
        return *this;
    }

    template <typename T>
    PacketBuffer& operator<<(const T& value) {
        static_assert(std::is_trivially_copyable_v<T>,
                      "Use a PacketBuffer overload for non-trivial types");
        append(reinterpret_cast<const uint8_t*>(&value), sizeof(T));
        return *this;
    }

    PacketBuffer& operator<<(const std::string& value);
    PacketBuffer& operator<<(const Message& message);

    template <typename T>
    PacketBuffer& operator>>(std::vector<T>& values) {
        uint32_t networkCount;
        *this >> networkCount;
        uint32_t count = ntohl(networkCount);
        if (count > MAX_PACKET_SIZE) {
            throw std::runtime_error("Packet vector is too large");
        }

        values.clear();
        values.reserve(count);
        for (uint32_t index = 0; index < count; ++index) {
            T value{};
            *this >> value;
            values.push_back(std::move(value));
        }
        return *this;
    }

    template <typename T>
    PacketBuffer& operator>>(T& value) {
        static_assert(std::is_trivially_copyable_v<T>,
                      "Use a PacketBuffer overload for non-trivial types");
        ensure_readable(sizeof(T));
        std::memcpy(&value, buffer.data() + readOffset, sizeof(T));
        readOffset += sizeof(T);
        return *this;
    }

    PacketBuffer& operator>>(std::string& value);
    bool empty() const;

private:
    static constexpr std::size_t HEADER_SIZE = sizeof(MessageType);
    static constexpr std::size_t MAX_PACKET_SIZE = 4096;

    std::vector<uint8_t> buffer;
    std::size_t readOffset;

    void append(const uint8_t* data, std::size_t size);
    void ensure_readable(std::size_t size) const;
    void ensure_writable(std::size_t size) const;
};

#endif // PACKET_BUFFER_HPP