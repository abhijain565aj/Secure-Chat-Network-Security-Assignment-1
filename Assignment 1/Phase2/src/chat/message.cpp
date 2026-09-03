#include "chat/message.hpp"
#include "chat/packet_buffer.hpp"
#include "utils/password.hpp"
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
    }

WhoMessage::WhoMessage() : Message() {}

ServerMessage::ServerMessage(std::string _text)
    : Message(), text(std::move(_text)) {
    if (text.length() > MESSAGE_MAX_LENGTH)
        throw std::runtime_error("Server message exceeds maximum length");
}

void UserMessage::serialize(PacketBuffer& packet) const { packet << sender << username << msg; }

std::unique_ptr<Message> UserMessage::deserialize(PacketBuffer& packet) {
    std::string sender, username, message;
    packet >> sender >> username >> message;
    return std::make_unique<UserMessage>(std::move(sender), std::move(username), std::move(message));
}

void LoginMessage::serialize(PacketBuffer& packet) const { packet << username; }

std::unique_ptr<Message> LoginMessage::deserialize(PacketBuffer& packet) {
    std::string username;
    packet >> username;
    return std::make_unique<LoginMessage>(std::move(username));
}

void WhoMessage::serialize(PacketBuffer&) const {}

std::unique_ptr<Message> WhoMessage::deserialize(PacketBuffer&) {
    return std::make_unique<WhoMessage>();
}

void ServerMessage::serialize(PacketBuffer& packet) const { packet << text; }

std::unique_ptr<Message> ServerMessage::deserialize(PacketBuffer& packet) {
    std::string text;
    packet >> text;
    return std::make_unique<ServerMessage>(std::move(text));
}

DHParametersMessage::DHParametersMessage(BIGNUM* b_bn, BIGNUM* g_bn)
    : Message(), b(DHE::BIGNUMToBytes(b_bn)), g(DHE::BIGNUMToBytes(g_bn)) {}

void DHParametersMessage::serialize(PacketBuffer& packet) const {
    packet << b << g;
}

std::unique_ptr<Message> DHParametersMessage::deserialize(PacketBuffer& packet) {
    std::vector<uint8_t> b, g;
    packet >> b >> g;
    return std::make_unique<DHParametersMessage>(std::move(b), std::move(g));
}

DHKeyMessage::DHKeyMessage(BIGNUM* public_key_bn)
    : Message(), public_key(DHE::BIGNUMToBytes(public_key_bn)) {}

void DHKeyMessage::serialize(PacketBuffer& packet) const {
    packet << public_key;
}

std::unique_ptr<Message> DHKeyMessage::deserialize(PacketBuffer& packet) {
    std::vector<uint8_t> public_key;
    packet >> public_key;
    return std::make_unique<DHKeyMessage>(std::move(public_key));
}