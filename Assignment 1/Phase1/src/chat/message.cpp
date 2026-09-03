#include "chat/message.hpp"

Message::Message() : timestamp(get_timestamp()) {}

UserMessage::UserMessage::UserMessage(std::string _username, std::string _msg)
    : Message(), username(_username), msg(_msg) {
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