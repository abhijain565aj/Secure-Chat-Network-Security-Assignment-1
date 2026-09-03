#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <string>
#include <cstdint>
#include <memory>
#include <vector>
#include "utils/time.hpp"
#include "utils/password.hpp"

enum class MessageType : uint8_t {
    User = 1,
    Login = 2,
    Who = 3,
    Server = 4
};

class Message{
    std::time_t timestamp;
public:
    Message();
    virtual ~Message() = default;
    virtual MessageType type() const = 0;
};

class UserMessage : public Message{
    std::string username;
    std::string msg;
public:
    UserMessage(std::string _username, std::string _msg);
    MessageType type() const override { return MessageType::User; }
    const std::string& get_username() const { return username; }
    const std::string& get_message() const { return msg; }
};

class LoginMessage : public Message{
    std::string username;
    std::string password; // FUTURE TODO
public:
    LoginMessage(std::string _username);
    MessageType type() const override { return MessageType::Login; }
    const std::string& get_username() const { return username; }
};

class WhoMessage : public Message{
public:
    WhoMessage();
    MessageType type() const override { return MessageType::Who; }
};

class ServerMessage : public Message{
    std::string text;
public:
    explicit ServerMessage(std::string text);
    MessageType type() const override { return MessageType::Server; }
    const std::string& get_text() const { return text; }
};

std::vector<uint8_t> serialize_message(const Message& message);
std::unique_ptr<Message> deserialize_message(const std::vector<uint8_t>& frame);
void send_message(int socket, const Message& message);
std::unique_ptr<Message> receive_message(int socket);

#endif