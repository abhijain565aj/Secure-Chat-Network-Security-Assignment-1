#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <string>
#include <cstdint>
#include <memory>
#include <memory>
#include "utils/helpers.hpp"
#include "crypto/dhe.hpp"

class PacketBuffer;

enum class MessageType : uint8_t {
    User = 1,
    Login = 2,
    Who = 3,
    Server = 4, 
    DHParametersMsg = 5,
    DHKeyMsg = 6
};

class Message{
    std::time_t timestamp;
public:
    Message();
    virtual ~Message() = default;
    virtual MessageType type() const = 0;
    virtual void serialize(PacketBuffer& packet) const = 0;
};

class UserMessage : public Message{
    std::string sender;
    std::string username;
    std::string msg;
public:
    UserMessage(std::string _username, std::string _msg);
    UserMessage(std::string _sender, std::string _username, std::string _msg);
    MessageType type() const override { return MessageType::User; }
    void serialize(PacketBuffer& packet) const override;
    static std::unique_ptr<Message> deserialize(PacketBuffer& packet);
    const std::string& get_sender() const { return sender; }
    const std::string& get_username() const { return username; }
    const std::string& get_message() const { return msg; }
};

class LoginMessage : public Message{
    std::string username;
public:
    LoginMessage(std::string _username);
    MessageType type() const override { return MessageType::Login; }
    void serialize(PacketBuffer& packet) const override;
    static std::unique_ptr<Message> deserialize(PacketBuffer& packet);
    const std::string& get_username() const { return username; }
};

class WhoMessage : public Message{
public:
    WhoMessage();
    MessageType type() const override { return MessageType::Who; }
    void serialize(PacketBuffer& packet) const override;
    static std::unique_ptr<Message> deserialize(PacketBuffer& packet);
};

class ServerMessage : public Message{
    std::string text;
public:
    explicit ServerMessage(std::string text);
    MessageType type() const override { return MessageType::Server; }
    void serialize(PacketBuffer& packet) const override;
    static std::unique_ptr<Message> deserialize(PacketBuffer& packet);
    const std::string& get_text() const { return text; }
};

class DHParametersMessage : public Message {
    std::vector<uint8_t> b;
    std::vector<uint8_t> g;
public:
    explicit DHParametersMessage(DHE::DHParameters params);
    MessageType type() const override { return MessageType::DHParametersMsg; }
    void serialize(PacketBuffer& packet) const override;
    static std::unique_ptr<Message> deserialize(PacketBuffer& packet);
    BIGNUM* get_b() const { return DHE::BytesToBIGNUM(b); }
    BIGNUM* get_g() const { return DHE::BytesToBIGNUM(g); }
};

class DHKeyMessage : public Message {
    std::vector<uint8_t> public_key;
public:
    explicit DHKeyMessage(BIGNUM* public_key);
    MessageType type() const override { return MessageType::DHKeyMsg; }
    void serialize(PacketBuffer& packet) const override;
    static std::unique_ptr<Message> deserialize(PacketBuffer& packet);
    BIGNUM* get_public_key() const { return DHE::BytesToBIGNUM(public_key); }
};

#endif