#pragma once

#include "Global.h"


class BitBuffer
{
public:
    BitBuffer() {}

    BitBuffer(const uint8_t* pBuffer, size_t sizeInBytes)
        : m_buffer(pBuffer)
        , m_size(sizeInBytes * 8)
    {
    }

    FORCEINLINE bool test(size_t offset) const
    {
        assert(offset < m_size);
        return m_buffer[offset / 8] & (1 << (7 - (offset % 8)));
    }

    template <typename T, size_t BITS>
    FORCEINLINE 
    T GetBits(size_t offset) const
    {
        assert(offset + BITS < m_size);
        T result = 0;
        for (size_t i = 0; i < BITS; ++i)
        {
            result |= test(offset + i) << (BITS - 1 - i);
        }
        return result;
    }
    
    FORCEINLINE uint8_t GetByte(size_t offset) const
    {
    #if 1
        assert(offset + 8 < m_size);
        size_t byteOffset = offset / 8;
        uint16_t result = ((uint16_t)(m_buffer[byteOffset]) << 8) | m_buffer[byteOffset + 1];
        result >>= (8 - (offset % 8));
        return (uint8_t)result;
    #else
        return GetBits<uint8_t, 8>(offset);
    #endif
    }

    FORCEINLINE uint8_t GetNibble(size_t offset) const { return GetBits<uint8_t, 4>(offset); }

    size_t size() const { return m_size; }
       
private:
    const uint8_t* m_buffer = nullptr;
    size_t m_size = 0;
};
