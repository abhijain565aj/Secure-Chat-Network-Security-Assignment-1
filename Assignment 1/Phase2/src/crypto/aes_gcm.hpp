#pragma once

#include <array>
#include <cstdint>
#include <vector>
#include "crypto/dhe.hpp"
struct Ciphertext {
    std::vector<uint8_t> iv;
    std::vector<uint8_t> ciphertext;
    std::vector<uint8_t> tag;
};

Ciphertext encrypt(
    const DHE::AES_KEY& aes_key,
    const std::vector<uint8_t>& plaintext
);

std::vector<uint8_t> decrypt(
    const DHE::AES_KEY& aes_key,
    const Ciphertext& encrypted
);