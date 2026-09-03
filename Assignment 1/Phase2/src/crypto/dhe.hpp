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

struct DHParameters {
    BIGNUM* p;
    BIGNUM* g;

    DHParameters();
    ~DHParameters();

    DHParameters(BIGNUM* _p, BIGNUM* _g) : p(_p), g(_g) {}
    DHParameters(const DHParameters&);
    DHParameters& operator=(const DHParameters&);
};

DHParameters getNewDHParameters();

struct DHKeyPair {
    BIGNUM* private_key;
    BIGNUM* public_key;
    
    DHKeyPair();
    ~DHKeyPair();
    
    DHKeyPair(const DHKeyPair&);
    DHKeyPair& operator=(const DHKeyPair&);
};

struct DHKey {
    BIGNUM* public_key;

    DHKey();
    ~DHKey();

    DHKey(BIGNUM* _public_key) : public_key(_public_key) {}
    DHKey(const DHKey&);
    DHKey& operator=(const DHKey&);
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
