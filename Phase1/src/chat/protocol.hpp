#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include "message.hpp"
#include <memory>
#include <vector>

std::vector<uint8_t> serialize_message(const Message& message);
std::unique_ptr<Message> deserialize_message(const std::vector<uint8_t>& frame);
void send_message(int socket, const Message& message);
std::unique_ptr<Message> receive_message(int socket);

#endif // PROTOCOL_HPP
