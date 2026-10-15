#include "utils/password.hpp"
#include <termios.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

// OpenSSL headers
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>

void check_username(std::string username) {
    if (username.length() > USERNAME_MAX_LENGTH) {
        throw std::runtime_error("Username exceeds maximum length");
    }
    else if (username.length() < USERNAME_MIN_LENGTH) {
        throw std::runtime_error("Username is below minimum length");
    }
}

void check_password(std::string password) {
    if (password.length() > PASSWORD_MAX_LENGTH) {
        throw std::runtime_error("Password exceeds maximum length");
    }
    else if (password.length() < PASSWORD_MIN_LENGTH) {
        throw std::runtime_error("Password is below minimum length");
    }
}

std::string get_password(std::string prompt) {
    std::cout << prompt;

    struct termios old_t;
    tcgetattr(STDIN_FILENO, &old_t);
    
    // Create new settings with ECHO disabled
    struct termios new_t = old_t;
    new_t.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_t);

    // Read the password
    std::string password;
    std::getline(std::cin, password);

    // Restore original terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &old_t);
    std::cout << "\n";

    check_password(password);
    return password;
}

std::string to_hex(const std::vector<unsigned char>& data) {
    std::ostringstream oss;
    for (unsigned char byte : data) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
    }
    return oss.str();
}

std::vector<unsigned char> from_hex(const std::string& hex) {
    std::vector<unsigned char> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        bytes.push_back((unsigned char)strtol(hex.substr(i, 2).c_str(), NULL, 16));
    }
    return bytes;
}

std::string hash_password(const std::string& password) {
    // A: Generate a cryptographically secure random salt
    std::vector<unsigned char> salt(SALT_LEN);
    if (RAND_bytes(salt.data(), salt.size()) != 1) {
        throw std::runtime_error("Failed to generate random salt");
    }

    // B: Compute the Scrypt hash
    std::vector<unsigned char> hash(HASH_LEN);
    if (EVP_PBE_scrypt(password.c_str(), password.length(),
                       salt.data(), salt.size(),
                       SCRYPT_N, SCRYPT_R, SCRYPT_P, 0,
                       hash.data(), hash.size()) != 1) {
        throw std::runtime_error("OpenSSL Scrypt hashing failed");
    }

    // C: Combine them into a single string to store in your DB (Format: "SALT_HEX:HASH_HEX")
    return to_hex(salt) + ":" + to_hex(hash);
}

bool verify_password(const std::string& stored_data, const std::string& password_attempt) {
    // A: Split the stored string back into salt and expected hash
    size_t colon_pos = stored_data.find(':');
    if (colon_pos == std::string::npos) return false;

    std::vector<unsigned char> salt = from_hex(stored_data.substr(0, colon_pos));
    std::vector<unsigned char> expected_hash = from_hex(stored_data.substr(colon_pos + 1));

    // B: Re-hash the password attempt using the extracted salt
    std::vector<unsigned char> computed_hash(HASH_LEN);
    if (EVP_PBE_scrypt(password_attempt.c_str(), password_attempt.length(),
                       salt.data(), salt.size(),
                       SCRYPT_N, SCRYPT_R, SCRYPT_P, 0,
                       computed_hash.data(), computed_hash.size()) != 1) {
        return false;
    }

    // C: Compare the hashes
    return CRYPTO_memcmp(expected_hash.data(), computed_hash.data(), HASH_LEN) == 0;
}