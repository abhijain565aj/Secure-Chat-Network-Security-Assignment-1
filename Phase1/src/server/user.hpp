#ifndef USER_HPP
#define USER_HPP

#include <string>

class User {
    int fileDescriptor;
    std::string ipAddress;
    int port;
    std::string username;
    bool loggedIn;

public:
    User(int fileDescriptor, std::string ipAddress, int port);

    int get_file_descriptor() const;
    const std::string& get_ip_address() const;
    int get_port() const;
    const std::string& get_username() const;
    bool is_logged_in() const;
    void login(const std::string& username);
};

#endif // USER_HPP
