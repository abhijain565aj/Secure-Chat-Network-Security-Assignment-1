#include "signature.hpp"

#include <openssl/evp.h>
#include <openssl/pem.h>

#include <cstdio>
#include <iostream>

EVP_PKEY* Signature::loadPrivateKey(
    const std::string& path)
{
    FILE* file = fopen(path.c_str(), "rb");

    if (!file) {
        std::cerr
            << "Failed to open private key: "
            << path << '\n';

        return nullptr;
    }

    EVP_PKEY* key =
        PEM_read_PrivateKey(
            file,
            nullptr,
            nullptr,
            nullptr
        );

    fclose(file);

    if (!key) {
        std::cerr
            << "Failed to read private key: "
            << path << '\n';
    }

    return key;
}

std::vector<unsigned char> Signature::sign(
    EVP_PKEY* privateKey,
    const unsigned char* data,
    size_t dataSize)
{
    if (!privateKey || !data) {
        return {};
    }

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();

    if (!ctx) {
        return {};
    }

    // Initialize SHA-256 + signature
    if (EVP_DigestSignInit(
            ctx,
            nullptr,
            EVP_sha256(),
            nullptr,
            privateKey) != 1)
    {
        EVP_MD_CTX_free(ctx);
        return {};
    }

    // Feed data
    if (EVP_DigestSignUpdate(ctx,data,dataSize) != 1)
    {
        EVP_MD_CTX_free(ctx);
        return {};
    }

    // find req signature size
    size_t signatureSize = 0;
    if (EVP_DigestSignFinal(
            ctx,
            nullptr,
            &signatureSize) != 1)
    {
        EVP_MD_CTX_free(ctx);
        return {};
    }

    std::vector<unsigned char> signature(
        signatureSize
    );

    // Generate signature
    if (EVP_DigestSignFinal(
            ctx,
            signature.data(),
            &signatureSize) != 1)
    {
        EVP_MD_CTX_free(ctx);
        return {};
    }

    signature.resize(signatureSize);

    EVP_MD_CTX_free(ctx);

    return signature;
}

// Sign vector
std::vector<unsigned char> Signature::sign(
    EVP_PKEY* privateKey,
    const std::vector<unsigned char>& data)
{
    return sign(
        privateKey,
        data.data(),
        data.size()
    );
}

// Verify
bool Signature::verify(
    EVP_PKEY* publicKey,
    const unsigned char* data,
    size_t dataSize,
    const unsigned char* signature,
    size_t signatureSize)
{
    if (!publicKey || !data || !signature) {
        return false;
    }

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();

    if (!ctx) {
        return false;
    }

    // Initialize SHA-256 verification
    if (EVP_DigestVerifyInit(
            ctx,
            nullptr,
            EVP_sha256(),
            nullptr,
            publicKey) != 1)
    {
        EVP_MD_CTX_free(ctx);
        return false;
    }

    // Feed original data
    if (EVP_DigestVerifyUpdate(
            ctx,
            data,
            dataSize) != 1)
    {
        EVP_MD_CTX_free(ctx);
        return false;
    }

    // Verify signature
    int result =
        EVP_DigestVerifyFinal(
            ctx,
            signature,
            signatureSize
        );

    EVP_MD_CTX_free(ctx);

    return result == 1;
}

// Verify vector
bool Signature::verify(
    EVP_PKEY* publicKey,
    const std::vector<unsigned char>& data,
    const std::vector<unsigned char>& signature)
{
    return verify(
        publicKey,
        data.data(),
        data.size(),
        signature.data(),
        signature.size()
    );
}
