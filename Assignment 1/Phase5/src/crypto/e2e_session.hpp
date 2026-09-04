#ifndef E2E_SESSION_HPP
#define E2E_SESSION_HPP

#include "dhe.hpp"
#include "aes_gcm.hpp"
#include <string>
#include <vector>

namespace E2E {

inline constexpr char INIT_TAG[] = "__E2E_INIT__";
inline constexpr char ACK_TAG[] = "__E2E_ACK__";
inline constexpr char MESSAGE_TAG[] = "__E2E_MSG__";
inline constexpr char REKEY_REQUEST_TAG[] = "__E2E_REKEY_REQUEST__";
inline constexpr char REKEY_RESPONSE_TAG[] = "__E2E_REKEY_RESPONSE__";
inline constexpr char REKEY_CONFIRM_TAG[] = "__E2E_REKEY_CONFIRM__";

class Session {
    DHE::DHParameters parameters;
    DHE::DHKeyPair keypair;
    DHE::AES_KEY aesKey{};
    DHE::DHKeyPair pendingKeypair;
    DHE::AES_KEY pendingAESKey{};
    bool established = false;
    bool rekeyPending = false;
    bool switchAfterConfirm = false;

public:
    explicit Session(const DHE::DHParameters& parameters);

    std::string begin();
    std::string accept_init(const std::string& payload);
    void accept_ack(const std::string& payload);

    std::string encrypt(const std::string& plaintext) const;
    std::string decrypt(const std::string& payload) const;
    std::string begin_rekey();
    std::string handle_rekey(const std::string& payload);
    void confirm_rekey();
    std::string key_fingerprint() const;
    bool is_rekey_payload(const std::string& payload) const;
    bool is_rekey_pending() const { return rekeyPending; }
    bool is_established() const { return established; }

private:
    void derive_key(const std::string& peer_public_key);
    void derive_pending_key(const std::string& peer_public_key);
    void switch_to_pending_key();
    static std::string public_key_bytes(const DHE::DHKeyPair& keypair);
    static std::vector<uint8_t> bytes(const std::string& value);
    static std::string encrypt_bytes(const DHE::AES_KEY& key, const std::string& value);
    static std::string decrypt_bytes(const DHE::AES_KEY& key, const std::string& value);
    static std::string fingerprint(const DHE::AES_KEY& key);
};

}

#endif