#ifndef UTILS_HPP
#define UTILS_HPP

#include "utils/constants.hpp"
#include <string>
#include <ctime>

void check_username(std::string username);
std::string get_password(std::string prompt);

const uint64_t SCRYPT_N = 16384; // CPU/Memory cost
const uint64_t SCRYPT_R = 8;     // Block size
const uint64_t SCRYPT_P = 1;     // Parallelization
const size_t SALT_LEN = 16;      // 128-bit salt
const size_t HASH_LEN = 32;      // 256-bit hash

std::string hash_password(const std::string& password);
bool verify_password(const std::string& stored_data, const std::string& password_attempt);

#endif