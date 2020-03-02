#include "SLDC.h"



bool SLDC::Extract(const uint8_t* pCompressed, size_t length, std::vector<uint8_t>& result)
{
    static constexpr std::array<uint32_t, 5> matchDigits{ { 1, 2, 3, 4, 8} };
    static constexpr std::array<uint32_t, 5> matchSkip{ { 1, 1, 1, 1, 0} };

    m_bitset = decltype(m_bitset)(pCompressed, length);

    for (size_t i = 0; i < m_bitset.size();)
    {
        if (i >= m_bitset.size() - 8 || m_state == State::END)
        {
            if (m_state != State::END && m_state != State::SKIP)
            {
                fprintf(stderr, "Went too far\n");
                return false;
            }
            break;
        }

        const uint8_t byte = m_bitset.GetByte(i);
        if (byte == 0xFF && m_bitset.test(i + 8))
        {
            SetControl(static_cast<ControlSymbol>(m_bitset.GetNibble(i + 9)));
            i += 13;
        }
        else
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
                    else // Compressed reference to history buffer
                    {
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
                        if (matchCount < 2 || matchCount > 271)
                        {
                            fprintf(stderr, "matchCount (%i) out of range at pos %i\n", matchCount, i);
                        }

                        // displacement is a simple 10 bit value
                        uint32_t displacement = (m_bitset.GetByte(i) << 2) |
                            ((m_bitset.test(i + 8) ? 1 : 0) << 1) |
                            (m_bitset.test(i + 9) ? 1 : 0);
                        i += 10;

                        for (uint32_t j = 0; j < matchCount; ++j)
                        {
                            AddByte(m_history.Get(displacement + j), result);
                        }
                    }
                } break;

                case State::SCHEME2:
                {
                    AddByte(byte, result);
                    i += (byte == 0xFF ? 9 : 8);
                } break;

                case State::SKIP:
                {
                    ++i;
                } break;

                default:
                {
                    fprintf(stderr, "Unknown SLDC state at pos %i\n", i);
                    return false;
                } break;
            }
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

void SLDC::SetControl(ControlSymbol control)
{
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
            fprintf(stderr, "Unknown SLDC control symbol %i\n", control);
        } break;
    }
}
