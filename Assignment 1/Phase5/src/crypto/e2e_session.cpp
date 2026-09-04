#include "e2e_session.hpp"
#include <stdexcept>

namespace {
constexpr std::size_t IV_SIZE = 12;
constexpr std::size_t TAG_SIZE = 16;
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