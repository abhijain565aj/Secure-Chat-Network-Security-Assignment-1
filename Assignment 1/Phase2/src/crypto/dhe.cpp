#include "dhe.hpp"
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <stdexcept>

static constexpr char RFC3526_GROUP14_PRIME[] =
    "FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD1"
    "29024E088A67CC74020BBEA63B139B22514A08798E3404DD"
    "EF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245"
    "E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7ED"
    "EE386BFB5A899FA5AE9F24117C4B1FE649286651ECE65381"
    "FFFFFFFFFFFFFFFF";

std::vector<uint8_t> DHE::BIGNUMToBytes(const BIGNUM* bn) {
    int num_bytes = BN_num_bytes(bn);
    std::vector<uint8_t> bytes(num_bytes);
    BN_bn2bin(bn, bytes.data());
    return bytes;
}

BIGNUM* DHE::BytesToBIGNUM(const std::vector<uint8_t>& bytes) {
    return BN_bin2bn(bytes.data(), bytes.size(), nullptr);
}

BIGNUM* DHE::getPrime(){
    BIGNUM* p = nullptr;

    if (BN_hex2bn(&p, RFC3526_GROUP14_PRIME) == 0) {
        throw std::runtime_error("Failed to create DH prime");
    }

    return p;
}

DHE::DHKeyPair::DHKeyPair()
    : private_key(nullptr),
      public_key(nullptr){
}

DHE::DHKeyPair::~DHKeyPair()
{
    BN_free(private_key);
    BN_free(public_key);
}


DHE::DHKeyPair DHE::getNewKeyPair(
    const DHParameters& params
)
{
    DHKeyPair keypair;

    keypair.private_key = BN_new();

    if (!keypair.private_key) {
        throw std::runtime_error("BN_new failed");
    }

    // Generate random private key. We generate a value in [1, p-2].
    BIGNUM* max = BN_dup(params.p);

    if (!max) {
        throw std::runtime_error("BN_dup failed");
    }

    BN_sub_word(max, 2);

    if (BN_rand_range(keypair.private_key, max) != 1) {
        BN_free(max);
        throw std::runtime_error("Failed to generate private key");
    }

    BN_add_word(keypair.private_key, 1);

    BN_free(max);

    // public = g^private mod p
    keypair.public_key =
        modularExponentiation(
            params.g,
            keypair.private_key,
            params.p
        );

    return keypair;
}

BIGNUM* DHE::modularExponentiation(
    const BIGNUM* base,
    const BIGNUM* exponent,
    const BIGNUM* modulus
)
{
    if (!base || !exponent || !modulus) {
        throw std::invalid_argument("Null BIGNUM argument");
    }

    if (BN_is_zero(modulus) || BN_is_negative(modulus)) {
        throw std::invalid_argument("Invalid modulus");
    }

    BN_CTX* ctx = BN_CTX_new();
    if (!ctx) {
        throw std::runtime_error("BN_CTX_new failed");
    }

    BIGNUM* result = BN_new();
    BIGNUM* current = BN_new();
    BIGNUM* exp = BN_dup(exponent);

    auto cleanup = [&]() {
        BN_free(result);
        BN_free(current);
        BN_free(exp);
        BN_CTX_free(ctx);
    };

    if (!result || !current || !exp) {
        cleanup();
        throw std::runtime_error("BIGNUM allocation failed");
    }

    BN_one(result); //result = 1

    if (BN_nnmod(current, base, modulus, ctx) != 1) {
        cleanup();
        throw std::runtime_error("BN_nnmod failed");
    }

    while (!BN_is_zero(exp)) {
        if (BN_is_odd(exp)) {
            if (BN_mod_mul(result,result,current,modulus,ctx) != 1){
                cleanup();
                throw std::runtime_error("BN_mod_mul failed");
            }
        }
        if (BN_mod_mul(current,current,current,modulus,ctx) != 1){
            cleanup();
            throw std::runtime_error("BN_mod_mul failed");
        }
        BN_rshift1(exp, exp);
    }

    BN_free(current);
    BN_free(exp);
    BN_CTX_free(ctx);

    return result;
}

BIGNUM* DHE::getPublicKey(
    const BIGNUM* server_g,
    const BIGNUM* p,
    const BIGNUM* private_key
)
{
    return modularExponentiation(
        server_g,
        private_key,
        p
    );
}

std::array<uint8_t, 32> DHE::deriveAESKey(
    const BIGNUM* private_key,
    const BIGNUM* peer_public_key,
    const BIGNUM* p
)
{
    BIGNUM* shared_secret =
        modularExponentiation(
            peer_public_key,
            private_key,
            p
        );

    std::vector<uint8_t> secret = BIGNUMToBytes(shared_secret);

    std::array<uint8_t, 32> aes_key{};

    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();

    if (!mdctx) {
        BN_free(shared_secret);
        throw std::runtime_error("EVP_MD_CTX_new failed");
    }

    if (EVP_DigestInit_ex(mdctx, EVP_sha256(), nullptr) != 1 ||
        EVP_DigestUpdate(
            mdctx,
            secret.data(),
            secret.size()
        ) != 1)
    {
        EVP_MD_CTX_free(mdctx);
        BN_free(shared_secret);
        throw std::runtime_error("SHA-256 initialization failed");
    }

    unsigned int digest_len = 0;

    if (EVP_DigestFinal_ex(
            mdctx,
            aes_key.data(),
            &digest_len
        ) != 1)
    {
        EVP_MD_CTX_free(mdctx);
        BN_free(shared_secret);
        throw std::runtime_error("SHA-256 failed");
    }

    EVP_MD_CTX_free(mdctx);
    BN_free(shared_secret);

    if (digest_len != 32) {
        throw std::runtime_error("Unexpected SHA-256 length");
    }

    return aes_key;
}

DHE::DHParameters::DHParameters() : p(nullptr),
      g(nullptr){
}

DHE::DHParameters::~DHParameters()
{
    BN_free(p);
    BN_free(g);
}

DHE::DHParameters DHE::getNewDHParameters(){
    DHParameters params;

    params.p = getPrime();

    if (!params.p) {
        throw std::runtime_error("Failed to get DH prime");
    }

    params.g = BN_new();

    if (!params.g) {
        BN_free(params.p);
        throw std::runtime_error("BN_new failed for g");
    }

    if (BN_set_word(params.g, 2) != 1) {
        BN_free(params.p);
        BN_free(params.g);
        throw std::runtime_error("Failed to set g to 2");
    }

    return params;
}

DHE::DHKeyPair::DHKeyPair(const DHKeyPair& other) : private_key(BN_dup(other.private_key)), public_key(BN_dup(other.public_key)) {
    if (!private_key || !public_key) {
        BN_free(private_key);
        BN_free(public_key);
        throw std::runtime_error("Failed to copy DHKeyPair");
    }
}

DHE::DHKeyPair& DHE::DHKeyPair::operator=(const DHKeyPair& other) {
    if (this != &other) {
        BIGNUM* new_private_key = BN_dup(other.private_key);
        BIGNUM* new_public_key = BN_dup(other.public_key);
        if (!new_private_key || !new_public_key) {
            BN_free(new_private_key);
            BN_free(new_public_key);
            throw std::runtime_error("Failed to copy DHKeyPair");
        }
        BN_free(private_key);
        BN_free(public_key);
        private_key = new_private_key;
        public_key = new_public_key;
    }
    return *this;
}

DHE::DHParameters::DHParameters(const DHParameters& other) : p(BN_dup(other.p)), g(BN_dup(other.g)) {
    if (!p || !g) {
        BN_free(p);
        BN_free(g);
        throw std::runtime_error("Failed to copy DHParameters");
    }
}

DHE::DHParameters& DHE::DHParameters::operator=(const DHParameters& other){
    if(this != &other){
        BIGNUM* new_p = BN_dup(other.g);
        BIGNUM* new_public_key = BN_dup(other.p);
        if (!new_p || !new_public_key) {
            BN_free(new_p);
            BN_free(new_public_key);
            throw std::runtime_error("Failed to copy DHParameters");
        }
        BN_free(p);
        BN_free(g);
        p = new_p;
        g = new_public_key;
    }
    return *this;
}   