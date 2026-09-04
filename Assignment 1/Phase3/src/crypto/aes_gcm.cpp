#include "aes_gcm.hpp"

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <stdexcept>

AES::Ciphertext AES::encrypt(
    const DHE::AES_KEY& aes_key,
    const std::vector<uint8_t>& plaintext
)
{
    AES::Ciphertext result;

    constexpr size_t IV_SIZE = 12;
    constexpr size_t TAG_SIZE = 16;

    result.iv.resize(IV_SIZE);
    result.ciphertext.resize(plaintext.size());
    result.tag.resize(TAG_SIZE);

    // Generate fresh random IV
    if (RAND_bytes(
            result.iv.data(),
            IV_SIZE
        ) != 1)
    {
        throw std::runtime_error("RAND_bytes failed");
    }

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();

    if (!ctx) {
        throw std::runtime_error("EVP_CIPHER_CTX_new failed");
    }

    int len = 0;
    int ciphertext_len = 0;

    if (EVP_EncryptInit_ex(
            ctx,
            EVP_aes_256_gcm(),
            nullptr,
            nullptr,
            nullptr
        ) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("EncryptInit failed");
    }

    // Set IV length
    if (EVP_CIPHER_CTX_ctrl(
            ctx,
            EVP_CTRL_GCM_SET_IVLEN,
            IV_SIZE,
            nullptr
        ) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to set IV length");
    }

    // Set key and IV
    if (EVP_EncryptInit_ex(
            ctx,
            nullptr,
            nullptr,
            aes_key.data(),
            result.iv.data()
        ) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to set key/IV");
    }

    // Encrypt plaintext
    if (EVP_EncryptUpdate(
            ctx,
            result.ciphertext.data(),
            &len,
            plaintext.data(),
            static_cast<int>(plaintext.size())
        ) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Encryption failed");
    }

    ciphertext_len = len;

    // Finalize
    if (EVP_EncryptFinal_ex(
            ctx,
            result.ciphertext.data() + ciphertext_len,
            &len
        ) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Encryption finalization failed");
    }

    ciphertext_len += len;

    result.ciphertext.resize(ciphertext_len);

    // Get authentication tag
    if (EVP_CIPHER_CTX_ctrl(
            ctx,
            EVP_CTRL_GCM_GET_TAG,
            TAG_SIZE,
            result.tag.data()
        ) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to obtain GCM tag");
    }

    EVP_CIPHER_CTX_free(ctx);

    return result;
}

std::vector<uint8_t>AES::decrypt(
    const DHE::AES_KEY& aes_key,
    const Ciphertext& encrypted
)
{
    constexpr size_t TAG_SIZE = 16;

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();

    if (!ctx) {
        throw std::runtime_error("EVP_CIPHER_CTX_new failed");
    }

    std::vector<uint8_t> plaintext(
        encrypted.ciphertext.size()
    );

    int len = 0;
    int plaintext_len = 0;

    if (EVP_DecryptInit_ex(
            ctx,
            EVP_aes_256_gcm(),
            nullptr,
            nullptr,
            nullptr
        ) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("DecryptInit failed");
    }

    if (EVP_CIPHER_CTX_ctrl(
            ctx,
            EVP_CTRL_GCM_SET_IVLEN,
            encrypted.iv.size(),
            nullptr
        ) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to set IV length");
    }

    if (EVP_DecryptInit_ex(
            ctx,
            nullptr,
            nullptr,
            aes_key.data(),
            encrypted.iv.data()
        ) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to set key/IV");
    }

    // Decrypt
    if (EVP_DecryptUpdate(
            ctx,
            plaintext.data(),
            &len,
            encrypted.ciphertext.data(),
            encrypted.ciphertext.size()
        ) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Decryption failed");
    }

    plaintext_len = len;

    // IMPORTANT:
    // Set authentication tag BEFORE Final.
    if (EVP_CIPHER_CTX_ctrl(
            ctx,
            EVP_CTRL_GCM_SET_TAG,
            TAG_SIZE,
            const_cast<uint8_t*>(encrypted.tag.data())
        ) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to set GCM tag");
    }

    // Returns <= 0 if authentication fails.
    int ret = EVP_DecryptFinal_ex(
        ctx,
        plaintext.data() + plaintext_len,
        &len
    );

    EVP_CIPHER_CTX_free(ctx);

    if (ret <= 0) {
        throw std::runtime_error(
            "Authentication failed: invalid GCM tag"
        );
    }

    plaintext_len += len;
    plaintext.resize(plaintext_len);

    return plaintext;
}