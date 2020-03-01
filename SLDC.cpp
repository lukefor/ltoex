#include "SLDC.h"



bool SLDC::Extract(const uint8_t* pCompressed, size_t length, std::vector<uint8_t>& result)
{
    static constexpr std::array<uint32_t, 5> matchDigits{ { 1, 2, 3, 4, 8} };
    static constexpr std::array<uint32_t, 5> matchSkip{ { 1, 1, 1, 1, 0} };

    m_bitset = decltype(m_bitset)(pCompressed, length);

    size_t i = 0;
    SetControl(i);

    for (; i < m_bitset.size() - 8;)
    {
        switch (m_state)
        {
            case State::SCHEME1:
            {
                const bool bit = m_bitset.test(i);
                if (bit == 0) // Raw byte
                {
                    AddByte(m_bitset.GetByte(i + 1), result);
                    i += 9;
                }
                // Check for control symbol (we already know [i] is 1)
                else if (m_bitset.GetByte(i + 1) != 0xFF) 
                {
                    // Compressed reference to history buffer
                    ++i;
                    // get number of sequential 1's (0-4)
                    uint32_t pow2;
                    for (pow2 = 0; pow2 < 4; ++pow2, ++i)
                    {
                        if (!m_bitset.test(i))
                        {
                            break;
                        }
                    }
                    // for 0-3, skip a 0. 4 1's has no 0
                    i += matchSkip[pow2];
                    uint32_t base = 0;
                    // read number of bits based on spec
                    for (uint32_t j = 0; j < matchDigits[pow2]; ++j, ++i)
                    {
                        base |= m_bitset.test(i) << (matchDigits[pow2] - (j + 1));
                    }
                    // match count range decided by given power of 2, plus a binary number offset
                    uint32_t matchCount = (1 << (pow2 + 1)) + base;
                    assert(matchCount >= 2 && matchCount <= 271);

                    // displacement is a simple 10 bit value
                #if 1
                    uint32_t displacement = (m_bitset.GetByte(i) << 2) |
                        ((m_bitset.test(i + 8) ? 1 : 0) << 1) | 
                        (m_bitset.test(i + 9) ? 1 : 0);
                #else
                    uint32_t displacement = m_bitset.GetBits<uint32_t, 10>(i);
                #endif
                    i += 10;

                    for (uint32_t j = 0; j < matchCount; ++j)
                    {
                        AddByte(m_history.Get(displacement + j), result);
                    }
                }
                else
                {
                    SetControl(i);
                }
            } break;

            case State::SCHEME2:
            {
                const uint8_t byte = m_bitset.GetByte(i);
                AddByte(byte, result);
                if (byte != 0xFF)
                {
                    i += 8;
                }
                else if (m_bitset.test(i + 8))
                {
                    SetControl(i);
                }
                else
                {
                    i += 9;
                }
            } break;

            case State::SKIP:
            {
                if (m_bitset.GetByte(i) == 0xFF && m_bitset.test(i + 8))
                {
                    SetControl(i);
                }
                else
                {
                    ++i;
                }
            } break;

            case State::END:
            {
                i = m_bitset.size();
                break;
            }

            default:
            {
                fprintf(stderr, "Unknown SLDC state at pos %i\n", i);
                return false;
            } break;
        }
    }

    // We end up with 4 mystery bytes
    for (size_t i = 0; i < 4; ++i)
    {
        result.pop_back();
    }

    return true;
}

void SLDC::AddByte(uint8_t byte, std::vector<uint8_t>& result)
{
    m_history.Add(byte);
    result.push_back(byte);
}

void SLDC::SetControl(size_t& i)
{
    // Sanity check input
    if (m_bitset.GetByte(i) != 0xFF || !m_bitset.test(i + 8))
    {
        fprintf(stderr, "Invalid 9x1 SLDC control symbol marker at pos %i\n", i);
        assert(false);
    }

    ControlSymbol control = static_cast<ControlSymbol>(m_bitset.GetNibble(i + 9));
    i += 13;
    switch (control)
    {
        case ControlSymbol::SCHEME1:
        {
            m_state = State::SCHEME1;
        } break;

        case ControlSymbol::SCHEME2:
        {
            m_state = State::SCHEME2;
        } break;

        case ControlSymbol::RESET1:
        {
            m_history.Reset();
            m_state = State::SCHEME1;
        } break;

        case ControlSymbol::RESET2:
        {
            m_history.Reset();
            m_state = State::SCHEME2;
        } break;

        case ControlSymbol::EOR:
        case ControlSymbol::FILEMARK:
        case ControlSymbol::FLUSH:
        {
            m_state = State::SKIP;
        } break;

        case ControlSymbol::END:
        {
            m_state = State::END;
        } break;

        default:
        {
            m_state = State::SKIP;
            fprintf(stderr, "Unexpected SLDC control symbol %i\n", control);
            assert(false);
        } break;
    }
}
