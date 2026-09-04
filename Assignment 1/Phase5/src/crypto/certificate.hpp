#ifndef CERTIFICATE_HPP
#define CERTIFICATE_HPP

#include <openssl/evp.h>

#include <cstddef>
#include <string>
#include <vector>

namespace Certificate {

// Certificate handling
// Read a PEM certificate from a file.
// Caller owns the returned X509* and must call X509_free().
struct x509_st;
using X509_ptr = X509*;

X509_ptr loadCertificate(const std::string& path);

// Read the certificate as PEM bytes.
// Useful for sending server.crt over the socket.
std::vector<unsigned char> readCertificateBytes(
    const std::string& path
);

// Parse a PEM certificate received over the network.
// Caller owns returned X509*.
X509_ptr parseCertificate(
    const std::vector<unsigned char>& pem
);

// Certificate verification
// Verify:
//   1. Certificate was signed by our trusted CA
//   2. Certificate is currently valid
//   3. Certificate belongs to expected server identity
// Returns true only if ALL checks pass.
bool validateCertificate(
    X509* certificate,
    const std::string& caCertificatePath,
    const std::string& expectedIdentity
);

// Public key
// Extract public key from a validated certificate.
// Caller owns returned EVP_PKEY* and must call EVP_PKEY_free().
EVP_PKEY* getPublicKey(X509* certificate);

} // namespace Certificate

#endif