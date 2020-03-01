#pragma once

#include "Global.h"


class HistoryBuffer
{
public:
    HistoryBuffer() {}

    FORCEINLINE void Add(uint8_t byte)
    {
        m_buffer[m_pos++] = byte;
        m_pos %= m_buffer.size();
    }

    void Reset() { m_pos = 0; }

    FORCEINLINE uint8_t Get(int offset) const
    {
        assert(offset >= 0 && offset < m_buffer.size() * 2);
        return m_buffer[offset % m_buffer.size()];
    }

private:
    std::array<uint8_t, 1024> m_buffer;
    size_t m_pos = 0;
};
