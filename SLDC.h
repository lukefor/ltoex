#pragma once

#include <array>
#include <vector>


class SLDC
{
    std::array<uint8_t, 1024> m_history;
    size_t m_historyPtr = 0;

    bool Extract(const uint8_t* pCompressed, size_t length, std::vector<uint8_t>& result);
};
