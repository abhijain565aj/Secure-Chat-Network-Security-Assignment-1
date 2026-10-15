#include "chat/message.hpp"
#include <stdexcept>
#include <utility>

Message::Message() : timestamp(get_timestamp()) {}

UserMessage::UserMessage(std::string _username, std::string _msg)
    : UserMessage("", std::move(_username), std::move(_msg)) {}

UserMessage::UserMessage(std::string _sender, std::string _username, std::string _msg)
    : Message(), sender(std::move(_sender)), username(std::move(_username)), msg(std::move(_msg)) {
        check_username(username);
        if (msg.length() > MESSAGE_MAX_LENGTH) {
            throw std::runtime_error("Message exceeds maximum length");
        }
    }

LoginMessage::LoginMessage(std::string _username)
    : Message(), username(_username) {
        check_username(username);
        // password = get_password("Enter password: ");
    }

WhoMessage::WhoMessage() : Message() {}

ServerMessage::ServerMessage(std::string _text)
    : Message(), text(std::move(_text)) {
    if (text.length() > MESSAGE_MAX_LENGTH)
        throw std::runtime_error("Server message exceeds maximum length");
}
