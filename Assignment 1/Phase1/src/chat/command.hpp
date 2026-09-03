#ifndef COMMAND_HPP
#define COMMAND_HPP

#include "message.hpp"
#include <memory>
#include <string>

enum class CommandType {
	Login,
	Who,
	UserMessage,
	CurrentChat,
	Chat,
	Clear,
	Help,
	Quit
};

struct ParsedCommand {
	CommandType type;
	std::unique_ptr<Message> message;
	std::string chatUsername;
};

class CommandParser {
public:
	static ParsedCommand parse(const std::string& input);
	static ParsedCommand parse(const std::string& input,
	                          const std::string& currentChat);

private:
	static ParsedCommand parse_login(const std::string& input);
	static ParsedCommand parse_user_message(const std::string& input);
	static ParsedCommand parse_chat(const std::string& input);
	static ParsedCommand parse_current_chat(const std::string& input,
	                                       const std::string& currentChat);
};

ParsedCommand parse_command(const std::string& input);

#endif // COMMAND_HPP