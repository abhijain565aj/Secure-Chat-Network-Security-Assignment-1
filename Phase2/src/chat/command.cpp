#include "command.hpp"
#include <stdexcept>

namespace {
bool is_tamper_command(const std::string& input) {
    return input.rfind("/tamper ", 0) == 0;
}
}

ParsedCommand CommandParser::parse(const std::string& input) {
    if (input == "/quit") return {CommandType::Quit, nullptr, ""};
    if (input == "/who") return {CommandType::Who, std::make_unique<WhoMessage>(), ""};
    if (input == "/clear") return {CommandType::Clear, nullptr, ""};
    if (input == "/help") return {CommandType::Help, nullptr, ""};
    if (is_tamper_command(input))
        throw std::runtime_error("Use /tamper <message> after selecting a recipient");
    if (input.rfind("/chat", 0) == 0) return parse_chat(input);

    const std::string login_prefix = "/login ";
    if (input.rfind(login_prefix, 0) == 0) {
        return parse_login(input);
    }

    if (!input.empty() && input[0] == '@') {
        return parse_user_message(input);
    }

    throw std::runtime_error("Unknown command");
}

ParsedCommand CommandParser::parse(const std::string& input,
                                   const std::string& currentChat) {
    if (is_tamper_command(input)) {
        auto command = parse_tamper_message(input, currentChat);
        command.tamper = true;
        return command;
    }
    if (!input.empty() && input[0] == '@') return parse_user_message(input);
    if (input.empty() || input[0] == '/') return parse(input);
    return parse_current_chat(input, currentChat);
}

ParsedCommand CommandParser::parse_tamper_message(const std::string& input,
                                                  const std::string& currentChat) {
    if (currentChat.empty())
        throw std::runtime_error("Select a recipient before using /tamper");
    const std::string tamper_prefix = "/tamper ";
    const std::string text = input.substr(tamper_prefix.size());
    if (text.empty())
        throw std::runtime_error("Usage: /tamper <message>");
    auto message = std::make_unique<UserMessage>(currentChat, text);
    return {CommandType::CurrentChat, std::move(message), "", false};
}

ParsedCommand CommandParser::parse_login(const std::string& input) {
    const std::string login_prefix = "/login ";
    std::string username = input.substr(login_prefix.size());
    if (username.empty() || username.find(' ') != std::string::npos)
        throw std::runtime_error("Usage: /login username");
    return {CommandType::Login, std::make_unique<LoginMessage>(username), ""};
}

ParsedCommand CommandParser::parse_chat(const std::string& input) {
    const std::string chat_prefix = "/chat ";
    if (input.rfind(chat_prefix, 0) != 0)
        throw std::runtime_error("Usage: /chat username");

    std::string username = input.substr(chat_prefix.size());
    if (username.empty() || username.find(' ') != std::string::npos)
        throw std::runtime_error("Usage: /chat username");
    return {CommandType::Chat, nullptr, username};
}

ParsedCommand CommandParser::parse_user_message(const std::string& input) {
    size_t separator = input.find(' ');
    if (separator <= 1 || separator == std::string::npos || separator + 1 >= input.size())
        throw std::runtime_error("Usage: @username message");
    std::string username = input.substr(1, separator - 1);
    std::string message = input.substr(separator + 1);
    return {CommandType::UserMessage,
            std::make_unique<UserMessage>(username, message),
            ""};
}

ParsedCommand CommandParser::parse_current_chat(const std::string& input,
                                                const std::string& currentChat) {
    if (currentChat.empty())
        throw std::runtime_error("Select a recipient with @username message first");
    return {CommandType::CurrentChat,
            std::make_unique<UserMessage>(currentChat, input),
            ""};
}

ParsedCommand parse_command(const std::string& input) {
    return CommandParser::parse(input);
}
