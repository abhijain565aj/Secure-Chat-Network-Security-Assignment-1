#include "presentation.hpp"
#include <sstream>

namespace Presentation {

Output user_message(const UserMessage& message,
                    const std::string& ownUsername,
                    const std::string& currentChat) {
    const std::string speaker = message.get_sender() == ownUsername
        ? "you" : message.get_sender();
    const Color color = message.get_sender() == ownUsername
        ? Color::Own
        : message.get_sender() == currentChat ? Color::Incoming : Color::OtherUser;
        return {"[" + speaker + "] " + message.get_message() + "\n",
            color, color == Color::OtherUser};
}

Output server_message(const std::string& message) {
    std::ostringstream formatted;
    std::istringstream lines(message);
    std::string line;
    bool first = true;
    while (std::getline(lines, line)) {
        if (!first && message.rfind("Online users:", 0) == 0)
            formatted << "         - ";
        else
            formatted << (first ? "[Server] " : "         ");
        formatted << line << "\n";
        first = false;
    }
    if (first) formatted << "[Server]\n";
    return {formatted.str(), Color::Server};
}

Output error_message(const std::string& message) {
    return {"[Error] " + message + "\n", Color::Error};
}

Output login_success(const std::string& username) {
    return server_message("Hi @" + username + ", Login successful.");
}

Output help_message() {
    return server_message(
        "Commands:\n"
        "  /login username   Log in to the server\n"
        "  /chat username    Switch the local chat target\n"
        "  @username message Send a message and switch target\n"
        "  /who              Show online users\n"
        "  /clear            Clear the terminal output\n"
        "  /help             Show this help\n"
        "  /quit             Disconnect and exit");
}

Output chat_switched(const std::string& username) {
    return {"Now chatting with @" + username, Color::Divider, true};
}

std::string server_log(const std::string& message) {
    return "\033[36m[Server]\033[0m " + message;
}

}
