#include "crypto_chacha.h"

#include <openssl/evp.h>
#include <openssl/rand.h>

#include <memory>
#include <stdexcept>

namespace x32nx {
namespace {

using CipherContext =
    std::unique_ptr<
        EVP_CIPHER_CTX,
        decltype(&EVP_CIPHER_CTX_free)
    >;

CipherContext make_context()
{
    EVP_CIPHER_CTX* raw =
        EVP_CIPHER_CTX_new();

    if (!raw) {
        throw std::runtime_error(
            "EVP_CIPHER_CTX_new failed"
        );
    }

    return CipherContext(
        raw,
        EVP_CIPHER_CTX_free
    );
}

}

EncryptedPacket encrypt_protobuf(
    const std::string& serialized_proto,
    const ChaChaKey& key,
    const std::vector<uint8_t>& aad
)
{
    EncryptedPacket packet;

    if (
        RAND_bytes(
            packet.nonce.data(),
            static_cast<int>(packet.nonce.size())
        ) != 1
    ) {
        throw std::runtime_error(
            "RAND_bytes failed"
        );
    }

    CipherContext context =
        make_context();

    if (
        EVP_EncryptInit_ex(
            context.get(),
            EVP_chacha20_poly1305(),
            nullptr,
            nullptr,
            nullptr
        ) != 1
    ) {
        throw std::runtime_error(
            "ChaCha20-Poly1305 init failed"
        );
    }

    if (
        EVP_CIPHER_CTX_ctrl(
            context.get(),
            EVP_CTRL_AEAD_SET_IVLEN,
            static_cast<int>(packet.nonce.size()),
            nullptr
        ) != 1
    ) {
        throw std::runtime_error(
            "ChaCha20 nonce length rejected"
        );
    }

    if (
        EVP_EncryptInit_ex(
            context.get(),
            nullptr,
            nullptr,
            key.data(),
            packet.nonce.data()
        ) != 1
    ) {
        throw std::runtime_error(
            "ChaCha20 key/nonce init failed"
        );
    }

    int length = 0;

    if (!aad.empty()) {
        if (
            EVP_EncryptUpdate(
                context.get(),
                nullptr,
                &length,
                aad.data(),
                static_cast<int>(aad.size())
            ) != 1
        ) {
            throw std::runtime_error(
                "ChaCha20 AAD failed"
            );
        }
    }

    packet.ciphertext.resize(
        serialized_proto.size() + 16
    );

    int ciphertext_length = 0;

    if (
        EVP_EncryptUpdate(
            context.get(),
            packet.ciphertext.data(),
            &length,
            reinterpret_cast<const unsigned char*>(
                serialized_proto.data()
            ),
            static_cast<int>(
                serialized_proto.size()
            )
        ) != 1
    ) {
        throw std::runtime_error(
            "ChaCha20 encryption failed"
        );
    }

    ciphertext_length = length;

    if (
        EVP_EncryptFinal_ex(
            context.get(),
            packet.ciphertext.data() +
                ciphertext_length,
            &length
        ) != 1
    ) {
        throw std::runtime_error(
            "ChaCha20 finalization failed"
        );
    }

    ciphertext_length += length;

    packet.ciphertext.resize(
        static_cast<size_t>(
            ciphertext_length
        )
    );

    if (
        EVP_CIPHER_CTX_ctrl(
            context.get(),
            EVP_CTRL_AEAD_GET_TAG,
            static_cast<int>(packet.tag.size()),
            packet.tag.data()
        ) != 1
    ) {
        throw std::runtime_error(
            "Poly1305 tag extraction failed"
        );
    }

    return packet;
}

bool decrypt_and_verify(
    const EncryptedPacket& packet,
    const ChaChaKey& key,
    std::string& plaintext,
    const std::vector<uint8_t>& aad
)
{
    CipherContext context =
        make_context();

    if (
        EVP_DecryptInit_ex(
            context.get(),
            EVP_chacha20_poly1305(),
            nullptr,
            nullptr,
            nullptr
        ) != 1
    ) {
        return false;
    }

    if (
        EVP_CIPHER_CTX_ctrl(
            context.get(),
            EVP_CTRL_AEAD_SET_IVLEN,
            static_cast<int>(packet.nonce.size()),
            nullptr
        ) != 1
    ) {
        return false;
    }

    if (
        EVP_DecryptInit_ex(
            context.get(),
            nullptr,
            nullptr,
            key.data(),
            packet.nonce.data()
        ) != 1
    ) {
        return false;
    }

    int length = 0;

    if (!aad.empty()) {
        if (
            EVP_DecryptUpdate(
                context.get(),
                nullptr,
                &length,
                aad.data(),
                static_cast<int>(aad.size())
            ) != 1
        ) {
            return false;
        }
    }

    std::vector<unsigned char> output(
        packet.ciphertext.size() + 16
    );

    int plaintext_length = 0;

    if (
        EVP_DecryptUpdate(
            context.get(),
            output.data(),
            &length,
            packet.ciphertext.data(),
            static_cast<int>(
                packet.ciphertext.size()
            )
        ) != 1
    ) {
        return false;
    }

    plaintext_length = length;

    auto tag = packet.tag;

    if (
        EVP_CIPHER_CTX_ctrl(
            context.get(),
            EVP_CTRL_AEAD_SET_TAG,
            static_cast<int>(tag.size()),
            tag.data()
        ) != 1
    ) {
        return false;
    }

    const int final_result =
        EVP_DecryptFinal_ex(
            context.get(),
            output.data() +
                plaintext_length,
            &length
        );

    if (final_result != 1) {
        plaintext.clear();
        return false;
    }

    plaintext_length += length;

    plaintext.assign(
        reinterpret_cast<const char*>(
            output.data()
        ),
        static_cast<size_t>(
            plaintext_length
        )
    );

    return true;
}

}
