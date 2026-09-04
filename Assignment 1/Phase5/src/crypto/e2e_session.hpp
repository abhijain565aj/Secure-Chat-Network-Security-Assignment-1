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

class Session {
    DHE::DHParameters parameters;
    DHE::DHKeyPair keypair;
    DHE::AES_KEY aesKey{};
    bool established = false;

public:
    explicit Session(const DHE::DHParameters& parameters);

    std::string begin();
    std::string accept_init(const std::string& payload);
    void accept_ack(const std::string& payload);

    std::string encrypt(const std::string& plaintext) const;
    std::string decrypt(const std::string& payload) const;
    bool is_established() const { return established; }

private:
    void derive_key(const std::string& peer_public_key);
    static std::string public_key_bytes(const DHE::DHKeyPair& keypair);
    static std::vector<uint8_t> bytes(const std::string& value);
};

}

#endif