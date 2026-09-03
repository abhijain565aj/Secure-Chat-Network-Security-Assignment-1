#include "user.hpp"
#include <utility>

User::User(int fileDescriptor, std::string ipAddress, int port)
    : fileDescriptor(fileDescriptor), ipAddress(std::move(ipAddress)),
      port(port), loggedIn(false) {}

int User::get_file_descriptor() const {
    return fileDescriptor;
}

const std::string& User::get_ip_address() const {
    return ipAddress;
}

int User::get_port() const {
    return port;
}

const std::string& User::get_username() const {
    return username;
}

bool User::is_logged_in() const {
    return loggedIn;
}

void User::login(const std::string& newUsername) {
    username = newUsername;
    loggedIn = true;
}

void User::set_public_key(DHE::DHKey& new_public_key, DHE::AES_KEY& new_aes_key) {
    public_key = std::move(new_public_key);
    aes_key = std::move(new_aes_key);
}