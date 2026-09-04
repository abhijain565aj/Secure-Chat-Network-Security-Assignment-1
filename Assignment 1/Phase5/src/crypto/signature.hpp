#ifndef SIGNATURE_HPP
#define SIGNATURE_HPP

#include <openssl/evp.h>

#include <cstddef>
#include <string>
#include <vector>

namespace Signature {

// Load private key from PEM file.
// Caller owns returned EVP_PKEY*.
EVP_PKEY* loadPrivateKey(
    const std::string& path
);

// Signing
// Sign arbitrary data using private key.
// Internally:
//     signature = Sign(SK, SHA256(data))
std::vector<unsigned char> sign(
    EVP_PKEY* privateKey,
    const unsigned char* data,
    size_t dataSize
);

std::vector<unsigned char> sign(
    EVP_PKEY* privateKey,
    const std::vector<unsigned char>& data
);

// Verify:
//     Verify(PK, SHA256(data), signature)
bool verify(
    EVP_PKEY* publicKey,
    const unsigned char* data,
    size_t dataSize,
    const unsigned char* signature,
    size_t signatureSize
);

bool verify(
    EVP_PKEY* publicKey,
    const std::vector<unsigned char>& data,
    const std::vector<unsigned char>& signature
);

} // namespace Signature

#endif