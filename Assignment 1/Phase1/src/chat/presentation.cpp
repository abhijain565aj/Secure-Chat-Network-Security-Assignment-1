#include "presentation.hpp"
#include <sstream>

namespace Presentation {

Output user_message(const UserMessage& message, const std::string& ownUsername) {
    const std::string speaker = message.get_sender() == ownUsername
        ? "you" : message.get_sender();
    return {"[" + speaker + "] " + message.get_message() + "\n",
            message.get_sender() == ownUsername ? Color::Own : Color::Incoming};
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

std::string server_log(const std::string& message) {
    return "\033[36m[Server]\033[0m " + message;
}

}
