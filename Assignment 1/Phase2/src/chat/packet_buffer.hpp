#ifndef PACKET_BUFFER_HPP
#define PACKET_BUFFER_HPP

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <string>
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
    PacketBuffer& operator<<(const T& value) {
        static_assert(std::is_trivially_copyable_v<T>,
                      "Use a PacketBuffer overload for non-trivial types");
        append(reinterpret_cast<const uint8_t*>(&value), sizeof(T));
        return *this;
    }

    PacketBuffer& operator<<(const std::string& value);
    PacketBuffer& operator<<(const Message& message);

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
    static constexpr std::size_t MAX_PACKET_SIZE = 1024;

    std::vector<uint8_t> buffer;
    std::size_t readOffset;

    void append(const uint8_t* data, std::size_t size);
    void ensure_readable(std::size_t size) const;
    void ensure_writable(std::size_t size) const;
};

#endif // PACKET_BUFFER_HPP