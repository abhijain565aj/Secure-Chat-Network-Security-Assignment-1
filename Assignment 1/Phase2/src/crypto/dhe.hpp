#ifndef DHE_HPP
#define DHE_HPP

#include <openssl/bn.h>
#include <array>            
#include <cstdint>
#include <vector>

namespace DHE{

BIGNUM* getPrime();

BIGNUM* modularExponentiation(
    const BIGNUM* base,
    const BIGNUM* exponent,
    const BIGNUM* modulus
);

struct DHKeyPair {
    BIGNUM* private_key;
    BIGNUM* public_key;
    
    DHKeyPair();
    ~DHKeyPair();
    
    DHKeyPair& operator=(const DHKeyPair&) = delete;
};

DHKeyPair getBN(const BIGNUM* p, const BIGNUM* g);

BIGNUM* getPublicKey(
    const BIGNUM* server_g,
    const BIGNUM* p,
    const BIGNUM* private_key
);

std::array<uint8_t, 32> deriveAESKey(
    const BIGNUM* private_key,
    const BIGNUM* peer_public_key,
    const BIGNUM* p
);
    
struct DHParameters {
    BIGNUM* p;
    BIGNUM* g;

    DHParameters();
    ~DHParameters();
};

} //namespace DHE

#endif
