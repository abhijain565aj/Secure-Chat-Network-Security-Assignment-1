#ifndef DHE_HPP
#define DHE_HPP

#include <openssl/bn.h>
#include <array>            
#include <cstdint>
#include <vector>

namespace DHE{

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

DHKeyPair getNewKeyPair(const DHParameters& params);

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
    


} //namespace DHE

#endif
