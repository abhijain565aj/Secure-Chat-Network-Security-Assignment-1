#ifndef PRESENTATION_HPP
#define PRESENTATION_HPP

#include "message.hpp"
#include <string>

namespace Presentation {

enum class Color {
    Normal,
    Server,
    Incoming,
    Own,
    Error
};

struct Output {
    std::string text;
    Color color;
};

Output user_message(const UserMessage& message, const std::string& ownUsername);
Output server_message(const std::string& message);
Output error_message(const std::string& message);
Output login_success(const std::string& username);
Output help_message();
Output chat_switched(const std::string& username);
std::string server_log(const std::string& message);

}

#endif // PRESENTATION_HPP
