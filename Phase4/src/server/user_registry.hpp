#ifndef USER_REGISTRY_HPP
#define USER_REGISTRY_HPP

#include "user.hpp"
#include <string>
#include <vector>

class UserRegistry {
    std::vector<User> users;

public:
    void add(User user);
    void remove(int fileDescriptor);
    User* find_by_file_descriptor(int fileDescriptor);
    User* find_logged_in(const std::string& username);
    std::string logged_in_usernames() const;
    void clear();
};

#endif // USER_REGISTRY_HPP
