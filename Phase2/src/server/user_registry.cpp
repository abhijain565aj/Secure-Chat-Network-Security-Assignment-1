#include "user_registry.hpp"
#include <algorithm>
#include <utility>

void UserRegistry::add(User user) {
    users.push_back(std::move(user));
}

void UserRegistry::remove(int fileDescriptor) {
    users.erase(std::remove_if(users.begin(), users.end(),
                               [fileDescriptor](const User& user) {
                                   return user.get_file_descriptor() == fileDescriptor;
                               }),
                users.end());
}

User* UserRegistry::find_by_file_descriptor(int fileDescriptor) {
    for (auto& user : users) {
        if (user.get_file_descriptor() == fileDescriptor) return &user;
    }
    return nullptr;
}

User* UserRegistry::find_logged_in(const std::string& username) {
    for (auto& user : users) {
        if (user.is_logged_in() && user.get_username() == username) return &user;
    }
    return nullptr;
}

std::string UserRegistry::logged_in_usernames() const {
    std::string result;
    for (const auto& user : users) {
        if (!user.is_logged_in()) continue;
        if (!result.empty()) result += "\n";
        result += user.get_username();
    }
    return result.empty() ? "No users online." : result;
}

void UserRegistry::clear() {
    users.clear();
}
