#pragma once

#include <array>
#include <vector>
#include "dynamic_bitset.hpp"
#include "HistoryBuffer.h"


enum class ControlSymbol : uint8_t
{
    FLUSH = 0,
    SCHEME1,
    SCHEME2,
    FILEMARK,
    EOR,
    RESET1,
    RESET2,
    END = 0b1111
};

enum class State : uint8_t
{
    UNKNOWN,
    SKIP,
    SCHEME1,
    SCHEME2,
    END
};

class SLDC
{
public:
    SLDC()
    {}

    bool Extract(const uint8_t* pCompressed, size_t length, std::vector<uint8_t>& result);

private:
    void AddByte(uint8_t byte, std::vector<uint8_t>& result);
    void SetControl(ControlSymbol control);

private:
    dynamic_bitset<uint8_t> m_bitset;
    HistoryBuffer m_history;
    State m_state = State::UNKNOWN;

};
