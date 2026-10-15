#include "e2e_session.hpp"
#include <openssl/evp.h>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace {
constexpr std::size_t IV_SIZE = 12;
constexpr std::size_t TAG_SIZE = 16;
constexpr std::size_t FINGERPRINT_SIZE = 64;
}

E2E::Session::Session(const DHE::DHParameters& sessionParameters)
    : parameters(sessionParameters) {}

std::string E2E::Session::public_key_bytes(const DHE::DHKeyPair& pair) {
    const auto value = DHE::BIGNUMToBytes(pair.get_public_key());
    return std::string(value.begin(), value.end());
}

std::vector<uint8_t> E2E::Session::bytes(const std::string& value) {
    return std::vector<uint8_t>(value.begin(), value.end());
}

std::string E2E::Session::encrypt_bytes(const DHE::AES_KEY& key,
                                        const std::string& value) {
    const auto encrypted = AES::encrypt(key, bytes(value));
    std::string result(reinterpret_cast<const char*>(encrypted.iv.data()), encrypted.iv.size());
    result.append(reinterpret_cast<const char*>(encrypted.ciphertext.data()), encrypted.ciphertext.size());
    result.append(reinterpret_cast<const char*>(encrypted.tag.data()), encrypted.tag.size());
    return result;
}

std::string E2E::Session::decrypt_bytes(const DHE::AES_KEY& key,
                                        const std::string& value) {
    if (value.size() < IV_SIZE + TAG_SIZE) throw std::runtime_error("Invalid rekey ciphertext");
    AES::Ciphertext encrypted;
    encrypted.iv.assign(value.begin(), value.begin() + IV_SIZE);
    encrypted.tag.assign(value.end() - TAG_SIZE, value.end());
    encrypted.ciphertext.assign(value.begin() + IV_SIZE, value.end() - TAG_SIZE);
    const auto plaintext = AES::decrypt(key, encrypted);
    return std::string(plaintext.begin(), plaintext.end());
}

std::string E2E::Session::fingerprint(const DHE::AES_KEY& key) {
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int length = 0;
    EVP_Digest(key.data(), key.size(), digest, &length, EVP_sha256(), nullptr);
    std::ostringstream output;
    output << std::hex << std::setfill('0');
    for (unsigned int index = 0; index < length; ++index)
        output << std::setw(2) << static_cast<unsigned int>(digest[index]);
    return output.str();
}

std::string E2E::Session::begin() {
    keypair = DHE::getNewKeyPair(parameters);
    established = false;
    return std::string(INIT_TAG) + public_key_bytes(keypair);
}

void E2E::Session::derive_key(const std::string& peer_public_key) {
    if (peer_public_key.empty())
        throw std::runtime_error("E2E peer public key is empty");
    DHE::DHKey peer(DHE::BytesToBIGNUM(bytes(peer_public_key)));
    if (!peer.get()) throw std::runtime_error("Invalid E2E peer public key");
    aesKey = DHE::deriveAESKey(
        keypair.get_private_key(), peer.get(), parameters.get_p());
    established = true;
}

void E2E::Session::derive_pending_key(const std::string& peer_public_key) {
    if (peer_public_key.empty()) throw std::runtime_error("E2E rekey public key is empty");
    DHE::DHKey peer(DHE::BytesToBIGNUM(bytes(peer_public_key)));
    if (!peer.get()) throw std::runtime_error("Invalid E2E rekey public key");
    pendingAESKey = DHE::deriveAESKey(
        pendingKeypair.get_private_key(), peer.get(), parameters.get_p());
    rekeyPending = true;
}

void E2E::Session::switch_to_pending_key() {
    if (!rekeyPending) throw std::runtime_error("No pending E2E key");
    aesKey = pendingAESKey;
    pendingAESKey.fill(0);
    keypair = std::move(pendingKeypair);
    rekeyPending = false;
    switchAfterConfirm = false;
}

std::string E2E::Session::accept_init(const std::string& payload) {
    if (payload.rfind(INIT_TAG, 0) != 0)
        throw std::runtime_error("Invalid E2E initialization tag");
    keypair = DHE::getNewKeyPair(parameters);
    derive_key(payload.substr(sizeof(INIT_TAG) - 1));
    return std::string(ACK_TAG) + public_key_bytes(keypair);
}

void E2E::Session::accept_ack(const std::string& payload) {
    if (payload.rfind(ACK_TAG, 0) != 0)
        throw std::runtime_error("Invalid E2E acknowledgment tag");
    if (!keypair.get_private_key())
        throw std::runtime_error("E2E acknowledgment has no pending exchange");
    derive_key(payload.substr(sizeof(ACK_TAG) - 1));
}

std::string E2E::Session::begin_rekey() {
    if (!established || rekeyPending) throw std::runtime_error("E2E rekey is not available");
    pendingKeypair = DHE::getNewKeyPair(parameters);
    rekeyPending = true;
    return std::string(REKEY_REQUEST_TAG) + encrypt_bytes(
        aesKey, public_key_bytes(pendingKeypair));
}

std::string E2E::Session::handle_rekey(const std::string& payload) {
    if (!established) throw std::runtime_error("E2E session is not established");
    if (payload.rfind(REKEY_REQUEST_TAG, 0) == 0) {
        if (rekeyPending) throw std::runtime_error("E2E rekey already in progress");
        pendingKeypair = DHE::getNewKeyPair(parameters);
        const auto peerPublic = decrypt_bytes(
            aesKey, payload.substr(sizeof(REKEY_REQUEST_TAG) - 1));
        derive_pending_key(peerPublic);
        return std::string(REKEY_RESPONSE_TAG) + encrypt_bytes(
            aesKey, public_key_bytes(pendingKeypair) + fingerprint(pendingAESKey));
    }
    if (payload.rfind(REKEY_RESPONSE_TAG, 0) == 0) {
        if (!rekeyPending) throw std::runtime_error("Unexpected E2E rekey response");
        const auto response = decrypt_bytes(
            aesKey, payload.substr(sizeof(REKEY_RESPONSE_TAG) - 1));
        if (response.size() <= FINGERPRINT_SIZE)
            throw std::runtime_error("Invalid E2E rekey response");
        derive_pending_key(response.substr(0, response.size() - FINGERPRINT_SIZE));
        const auto expected = response.substr(response.size() - FINGERPRINT_SIZE);
        if (fingerprint(pendingAESKey) != expected) {
            pendingAESKey.fill(0);
            rekeyPending = false;
            throw std::runtime_error("E2E rekey fingerprint mismatch");
        }
        switchAfterConfirm = true;
        return std::string(REKEY_CONFIRM_TAG) + encrypt_bytes(aesKey, expected);
    }
    if (payload.rfind(REKEY_CONFIRM_TAG, 0) == 0) {
        if (!rekeyPending) throw std::runtime_error("Unexpected E2E rekey confirmation");
        const auto expected = decrypt_bytes(
            aesKey, payload.substr(sizeof(REKEY_CONFIRM_TAG) - 1));
        if (expected != fingerprint(pendingAESKey))
            throw std::runtime_error("E2E rekey confirmation mismatch");
        switch_to_pending_key();
        return {};
    }
    throw std::runtime_error("Unknown E2E rekey tag");
}

void E2E::Session::confirm_rekey() {
    if (!switchAfterConfirm) throw std::runtime_error("No E2E rekey confirmation to apply");
    switch_to_pending_key();
}

std::string E2E::Session::key_fingerprint() const { return fingerprint(aesKey); }

bool E2E::Session::is_rekey_payload(const std::string& payload) const {
    return payload.rfind(REKEY_REQUEST_TAG, 0) == 0 ||
           payload.rfind(REKEY_RESPONSE_TAG, 0) == 0 ||
           payload.rfind(REKEY_CONFIRM_TAG, 0) == 0;
}

std::string E2E::Session::encrypt(const std::string& plaintext) const {
    if (!established) throw std::runtime_error("E2E session is not established");
    const auto encrypted = AES::encrypt(aesKey, bytes(plaintext));
    std::string payload(MESSAGE_TAG);
    payload.append(reinterpret_cast<const char*>(encrypted.iv.data()), encrypted.iv.size());
    payload.append(reinterpret_cast<const char*>(encrypted.ciphertext.data()),
                   encrypted.ciphertext.size());
    payload.append(reinterpret_cast<const char*>(encrypted.tag.data()), encrypted.tag.size());
    return payload;
}

std::string E2E::Session::decrypt(const std::string& payload) const {
    if (!established) throw std::runtime_error("E2E session is not established");
    if (payload.rfind(MESSAGE_TAG, 0) != 0)
        throw std::runtime_error("Invalid E2E message tag");
    const std::string encryptedBytes = payload.substr(sizeof(MESSAGE_TAG) - 1);
    if (encryptedBytes.size() < IV_SIZE + TAG_SIZE)
        throw std::runtime_error("Invalid E2E ciphertext");

    AES::Ciphertext encrypted;
    encrypted.iv.assign(encryptedBytes.begin(), encryptedBytes.begin() + IV_SIZE);
    encrypted.tag.assign(encryptedBytes.end() - TAG_SIZE, encryptedBytes.end());
    encrypted.ciphertext.assign(encryptedBytes.begin() + IV_SIZE,
                                encryptedBytes.end() - TAG_SIZE);
    const auto plaintext = AES::decrypt(aesKey, encrypted);
    return std::string(plaintext.begin(), plaintext.end());
}