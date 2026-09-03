#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <string>
#include "utils/time.hpp"
#include "utils/password.hpp"

class Message{
    std::time_t timestamp;
public:
    Message();
};

class UserMessage : public Message{
    std::string username;
    std::string msg;
public:
    UserMessage(std::string _username, std::string _msg);
};

class LoginMessage : public Message{
    std::string username;
    std::string password; // FUTURE TODO
public:
    LoginMessage(std::string _username);
};

class WhoMessage : public Message{
public:
    WhoMessage();
};

class QuitMessage : public Message{
public:
    QuitMessage();
};

class SwitchMessage : public Message{
    std::string username;
public:
    SwitchMessage(std::string _username);
};

#endif