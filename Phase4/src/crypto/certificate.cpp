#include "certificate.hpp"

#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#include <openssl/x509v3.h>

#include <cstdio>
#include <fstream>
#include <iostream>

X509* Certificate::loadCertificate(const std::string& path)
{
    BIO* bio = BIO_new_file(path.c_str(), "r");

    if (!bio) {
        std::cerr << "Failed to open certificate: "
                  << path << '\n';
        return nullptr;
    }

    X509* cert = PEM_read_bio_X509(
        bio,
        nullptr,
        nullptr,
        nullptr
    );

    BIO_free(bio);

    if (!cert) {
        std::cerr << "Failed to parse certificate: "
                  << path << '\n';
    }

    return cert;
}

std::vector<unsigned char> Certificate::readCertificateBytes(
    const std::string& path)
{
    std::ifstream file(
        path,
        std::ios::binary
    );

    if (!file) {
        std::cerr << "Failed to open certificate: "
                  << path << '\n';
        return {};
    }

    file.seekg(0, std::ios::end);

    std::streamsize size = file.tellg();

    if (size <= 0) {
        return {};
    }

    file.seekg(0, std::ios::beg);

    std::vector<unsigned char> data(
        static_cast<size_t>(size)
    );

    if (!file.read(
            reinterpret_cast<char*>(data.data()),
            size))
    {
        return {};
    }

    return data;
}

// Parse PEM certificate received from network
X509* Certificate::parseCertificate(
    const std::vector<unsigned char>& pem)
{
    if (pem.empty()) {
        return nullptr;
    }

    BIO* bio = BIO_new_mem_buf(
        pem.data(),
        static_cast<int>(pem.size())
    );

    if (!bio) {
        return nullptr;
    }

    X509* cert = PEM_read_bio_X509(
        bio,
        nullptr,
        nullptr,
        nullptr
    );

    BIO_free(bio);

    return cert;
}

bool Certificate::validateCertificate(
    X509* certificate,
    const std::string& caCertificatePath,
    const std::string& expectedIdentity)
{
    if (!certificate) {
        std::cerr << "Certificate is null\n";
        return false;
    }

    X509* caCertificate =
        loadCertificate(caCertificatePath);

    if (!caCertificate) {
        std::cerr << "Failed to load trusted CA\n";
        return false;
    }

    // Create trust store
    X509_STORE* store = X509_STORE_new();

    if (!store) {
        X509_free(caCertificate);
        return false;
    }

    // Only trust our CA.
    if (X509_STORE_add_cert(
            store,
            caCertificate) != 1)
    {
        std::cerr << "Failed to add CA to trust store\n";

        X509_STORE_free(store);
        X509_free(caCertificate);

        return false;
    }

    // Create verification context
    X509_STORE_CTX* ctx = X509_STORE_CTX_new();

    if (!ctx) {
        X509_STORE_free(store);
        X509_free(caCertificate);

        return false;
    }

    if (X509_STORE_CTX_init(
            ctx,
            store,
            certificate,
            nullptr) != 1)
    {
        std::cerr << "Failed to initialize certificate "
                     "verification context\n";

        X509_STORE_CTX_free(ctx);
        X509_STORE_free(store);
        X509_free(caCertificate);

        return false;
    }

    if (X509_verify_cert(ctx) != 1) {

        int error =
            X509_STORE_CTX_get_error(ctx);

        std::cerr
            << "Certificate verification failed: "
            << X509_verify_cert_error_string(error)
            << '\n';

        X509_STORE_CTX_free(ctx);
        X509_STORE_free(store);
        X509_free(caCertificate);

        return false;
    }

    // Verify server identity
    if (X509_check_host(
            certificate,
            expectedIdentity.c_str(),
            expectedIdentity.size(),
            0,
            nullptr) != 1)
    {
        std::cerr
            << "Certificate identity verification failed\n"
            << "Expected: "
            << expectedIdentity << '\n';

        X509_STORE_CTX_free(ctx);
        X509_STORE_free(store);
        X509_free(caCertificate);

        return false;
    }

    // Everything passed
    X509_STORE_CTX_free(ctx);
    X509_STORE_free(store);
    X509_free(caCertificate);

    return true;
}

EVP_PKEY* Certificate::getPublicKey(X509* certificate)
{
    if (!certificate) {
        return nullptr;
    }

    return X509_get_pubkey(certificate);
}