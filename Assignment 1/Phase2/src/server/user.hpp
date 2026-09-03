#ifndef USER_HPP
#define USER_HPP

#include <string>
#include <crypto/dhe.hpp>

class User {
    int fileDescriptor;
    std::string ipAddress;
    int port;
    std::string username;
    bool loggedIn;
    DHE::DHKey public_key;
    DHE::AES_KEY aes_key;
public:
    User(int fileDescriptor, std::string ipAddress, int port);

    int get_file_descriptor() const;
    const std::string& get_ip_address() const;
    int get_port() const;
    const std::string& get_username() const;
    bool is_logged_in() const;
    void login(const std::string& username);
    void set_public_key(DHE::DHKey& public_key, DHE::AES_KEY& aes_key);
};

#endif // USER_HPP
