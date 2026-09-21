#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace x32nx {

using ChaChaKey = std::array<unsigned char, 32>;

struct EncryptedPacket {
    std::vector<unsigned char> ciphertext;
    std::array<unsigned char, 12> nonce{};
    std::array<unsigned char, 16> tag{};
};

EncryptedPacket encrypt_protobuf(
    const std::string& serialized_proto,
    const ChaChaKey& key,
    const std::vector<uint8_t>& aad = {}
);

bool decrypt_and_verify(
    const EncryptedPacket& packet,
    const ChaChaKey& key,
    std::string& plaintext,
    const std::vector<uint8_t>& aad = {}
);

}
