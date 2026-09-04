#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <string>
#include <cstdint>
#include <memory>
#include <vector>
#include "utils/helpers.hpp"
#include "crypto/dhe.hpp"
#include "crypto/aes_gcm.hpp"

class PacketBuffer;

enum class MessageType : uint8_t {
    User = 1,
    Login = 2,
    Who = 3,
    Server = 4, 
    DHParametersMsg = 5,
    DHKeyMsg = 6,
    RAMsg = 7,
    CertificateMsg = 8,
    BSignatureMsg = 9,
    Ciphertext = 10
};

class Message{
    std::time_t timestamp;
public:
    Message();
    virtual ~Message() = default;
    virtual MessageType type() const = 0;
    virtual void serialize(PacketBuffer& packet) const = 0;
};

class UserMessage : public Message{
    std::string sender;
    std::string username;
    std::string msg;
public:
    UserMessage(std::string _username, std::string _msg);
    UserMessage(std::string _sender, std::string _username, std::string _msg);
    MessageType type() const override { return MessageType::User; }
    void serialize(PacketBuffer& packet) const override;
    static std::unique_ptr<Message> deserialize(PacketBuffer& packet);
    const std::string& get_sender() const { return sender; }
    const std::string& get_username() const { return username; }
    const std::string& get_message() const { return msg; }
};

class LoginMessage : public Message{
    std::string username;
public:
    LoginMessage(std::string _username);
    MessageType type() const override { return MessageType::Login; }
    void serialize(PacketBuffer& packet) const override;
    static std::unique_ptr<Message> deserialize(PacketBuffer& packet);
    const std::string& get_username() const { return username; }
};

class WhoMessage : public Message{
public:
    WhoMessage();
    MessageType type() const override { return MessageType::Who; }
    void serialize(PacketBuffer& packet) const override;
    static std::unique_ptr<Message> deserialize(PacketBuffer& packet);
};

class ServerMessage : public Message{
    std::string text;
public:
    explicit ServerMessage(std::string text);
    MessageType type() const override { return MessageType::Server; }
    void serialize(PacketBuffer& packet) const override;
    static std::unique_ptr<Message> deserialize(PacketBuffer& packet);
    const std::string& get_text() const { return text; }
};

class VectorMessage : public Message {
protected:
    std::vector<uint8_t> values;
    const MessageType messageType;

    VectorMessage(MessageType type, std::vector<uint8_t> values);
    void serialize(PacketBuffer& packet) const override;

public:
    MessageType type() const override { return messageType; }
    const std::vector<uint8_t>& get_values() const { return values; }
};

class TwoVectorMessage : public Message {
protected:
    std::vector<uint8_t> first;
    std::vector<uint8_t> second;
    const MessageType messageType;

    TwoVectorMessage(MessageType type, std::vector<uint8_t> first,
                     std::vector<uint8_t> second);
    void serialize(PacketBuffer& packet) const override;

public:
    MessageType type() const override { return messageType; }
    const std::vector<uint8_t>& get_first() const { return first; }
    const std::vector<uint8_t>& get_second() const { return second; }
};

class DHParametersMessage : public TwoVectorMessage {
public:
    explicit DHParametersMessage(const DHE::DHParameters& params);
    static std::unique_ptr<Message> deserialize(PacketBuffer& packet);
    DHE::DHParameters get_parameters() const {
        return DHE::DHParameters(
            DHE::DHKey(DHE::BytesToBIGNUM(first)),
            DHE::DHKey(DHE::BytesToBIGNUM(second)));
    }
};

class DHKeyMessage : public VectorMessage {
public:
    explicit DHKeyMessage(BIGNUM* public_key);
    explicit DHKeyMessage(std::vector<uint8_t> public_key);
    static std::unique_ptr<Message> deserialize(PacketBuffer& packet);
    DHE::DHKey get_public_key() const { return DHE::DHKey(DHE::BytesToBIGNUM(values)); }
};

class RAMessage : public TwoVectorMessage {
public:
    explicit RAMessage(BIGNUM* public_key, std::array<unsigned char, 32> challenge);
    static std::unique_ptr<Message> deserialize(PacketBuffer& packet);
    DHE::DHKey get_public_key() const { return DHE::DHKey(DHE::BytesToBIGNUM(first)); }
    const std::vector<uint8_t>& get_challenge() const { return second; }
    const std::vector<uint8_t>& get_public_key_bytes() const { return first; }
};
class CertificateMessage : public VectorMessage {
public:
    explicit CertificateMessage(const std::vector<uint8_t>& certificate);
    static std::unique_ptr<Message> deserialize(PacketBuffer& packet);
    const std::vector<uint8_t>& get_certificate() const { return values; }
};

class BSignatureMessage : public TwoVectorMessage {
public:
    BSignatureMessage(std::vector<uint8_t> public_key,
                      std::vector<uint8_t> signature);
    static std::unique_ptr<Message> deserialize(PacketBuffer& packet);
    const std::vector<uint8_t>& get_public_key_bytes() const { return first; }
    const std::vector<uint8_t>& get_signature() const { return second; }
};

class CiphertextMsg : public Message {
    AES::Ciphertext ciphertext;
public:
    explicit CiphertextMsg(const AES::Ciphertext& _ciphertext);
    MessageType type() const override { return MessageType::Ciphertext; }
    void serialize(PacketBuffer& packet) const override;
    static std::unique_ptr<Message> deserialize(PacketBuffer& packet);
    AES::Ciphertext get_ciphertext() const { return ciphertext; }

    // New methods for encryption and decryption
    explicit CiphertextMsg(const Message& message, const DHE::AES_KEY& aes_key);
    std::unique_ptr<Message> getMsg(const DHE::AES_KEY& aes_key) const;
};


#endif