#ifndef DHE_HPP
#define DHE_HPP

#include <openssl/bn.h>
#include <array>            
#include <cstdint>
#include <vector>

namespace DHE{

using AES_KEY = std::array<uint8_t, 32>;

std::vector<uint8_t> BIGNUMToBytes(const BIGNUM* bn);
BIGNUM* BytesToBIGNUM(const std::vector<uint8_t>& bytes);

BIGNUM* getPrime();

BIGNUM* modularExponentiation(
    const BIGNUM* base,
    const BIGNUM* exponent,
    const BIGNUM* modulus
);

struct DHKey {
    BIGNUM* key;

    DHKey();
    explicit DHKey(BIGNUM* key);
    ~DHKey();
    DHKey(const DHKey&);
    DHKey(DHKey&& other) noexcept;
    DHKey& operator=(const DHKey&);
    DHKey& operator=(DHKey&& other) noexcept;

    BIGNUM* get() const;
};

struct DHParameters {
    DHKey p;
    DHKey g;

    DHParameters() = default;
    DHParameters(DHKey p, DHKey g);

    BIGNUM* get_p() const;
    BIGNUM* get_g() const;
};

DHParameters getNewDHParameters();

struct DHKeyPair {
    DHKey private_key;
    DHKey public_key;

    DHKeyPair() = default;
    DHKeyPair(DHKey private_key, DHKey public_key);

    BIGNUM* get_private_key() const;
    BIGNUM* get_public_key() const;
};

DHKeyPair getNewKeyPair(const DHParameters& params);

BIGNUM* getPublicKey(
    const BIGNUM* server_g,
    const BIGNUM* p,
    const BIGNUM* private_key
);

AES_KEY deriveAESKey(
    const BIGNUM* private_key,
    const BIGNUM* peer_public_key,
    const BIGNUM* p
);
    


} //namespace DHE

#endif
