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

DHE::DHKeyPair DHE::getNewKeyPair(
    const DHParameters& params
)
{
    DHKeyPair keypair;

    keypair.private_key = DHKey(BN_new());

    if (!keypair.private_key.get()) {
        throw std::runtime_error("BN_new failed");
    }

    // Generate random private key. We generate a value in [1, p-2].
    BIGNUM* max = BN_dup(params.get_p());

    if (!max) {
        throw std::runtime_error("BN_dup failed");
    }

    BN_sub_word(max, 2);

    if (BN_rand_range(keypair.private_key.get(), max) != 1) {
        BN_free(max);
        throw std::runtime_error("Failed to generate private key");
    }

    BN_add_word(keypair.private_key.get(), 1);

    BN_free(max);

    // public = g^private mod p
    keypair.public_key = DHKey(modularExponentiation(
        params.get_g(),
        keypair.private_key.get(),
        params.get_p()));

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

DHE::AES_KEY DHE::deriveAESKey(
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

    AES_KEY aes_key{};

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

DHE::DHParameters DHE::getNewDHParameters(){
    DHKey p(getPrime());
    if (!p.get()) {
        throw std::runtime_error("Failed to get DH prime");
    }

    DHKey g(BN_new());

    if (!g.get()) {
        throw std::runtime_error("BN_new failed for g");
    }

    if (BN_set_word(g.get(), 2) != 1) {
        throw std::runtime_error("Failed to set g to 2");
    }

    return DHParameters(std::move(p), std::move(g));
}

DHE::DHKey::DHKey() : key(nullptr) {}

DHE::DHKey::DHKey(BIGNUM* key) : key(key) {}

DHE::DHKey::~DHKey() {
    BN_free(key);
}

DHE::DHKey::DHKey(const DHKey& other) : key(other.key ? BN_dup(other.key) : nullptr) {}

DHE::DHKey::DHKey(DHKey&& other) noexcept : key(other.key) { other.key = nullptr; }

DHE::DHKey& DHE::DHKey::operator=(const DHKey& other) {
    if (this != &other) {
        BIGNUM* new_key = other.key ? BN_dup(other.key) : nullptr;
        BN_free(key);
        key = new_key;
    }
    return *this;
}

DHE::DHKey& DHE::DHKey::operator=(DHKey&& other) noexcept {
    if (this != &other) {
        BN_free(key);
        key = other.key;
        other.key = nullptr;
    }
    return *this;
}

BIGNUM* DHE::DHKey::get() const { return key; }

DHE::DHParameters::DHParameters(DHKey p, DHKey g)
    : p(std::move(p)), g(std::move(g)) {}

BIGNUM* DHE::DHParameters::get_p() const { return p.get(); }
BIGNUM* DHE::DHParameters::get_g() const { return g.get(); }

DHE::DHKeyPair::DHKeyPair(DHKey private_key, DHKey public_key)
    : private_key(std::move(private_key)), public_key(std::move(public_key)) {}

BIGNUM* DHE::DHKeyPair::get_private_key() const { return private_key.get(); }
BIGNUM* DHE::DHKeyPair::get_public_key() const { return public_key.get(); }