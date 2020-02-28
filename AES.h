#pragma once

#include <cstddef>
#include <cstdint>


namespace AES
{
    static constexpr size_t EXTRA_BYTES = 48;

    bool Decrypt(const uint8_t* pKey, const uint8_t* pEncrypted, size_t encryptedLength, uint8_t* pOutput, size_t outputLength);
}
