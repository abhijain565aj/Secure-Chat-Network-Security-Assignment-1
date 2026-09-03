#ifndef DHE_HPP
#define DHE_HPP

#include <openssl/bn.h>
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

std::vector<uint8_t> deriveAESKey(
    const BIGNUM* private_key,
    const BIGNUM* peer_public_key,
    const BIGNUM* p
);
    
} //namespace DHE

#endif
