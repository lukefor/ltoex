#include "SLDC.h"

bool SLDC::Extract(const uint8_t* pCompressed, size_t length, std::vector<uint8_t>& result)
{
    m_bitset = decltype(m_bitset)(pCompressed, length);
   
    for (size_t i = 0; i < m_bitset.size();)
    {
        const uint8_t byte = m_bitset.GetByte(i);
        const bool bit = m_bitset.test(i);
        if (byte == 0xFF && m_bitset.test(i + 8))
        {
            SetControl(static_cast<ControlSymbol>(m_bitset.GetNibble(i + 9)));
            i += 13;
        }
        else
        {
            if (m_scheme == Scheme::ONE)
            {
                if (bit == 0) // Raw byte
                {
                    AddByte(m_bitset.GetByte(i + 1), result);
                    i += 9;
                }
                else // Compressed reference to history buffer
                {

                }
            }
            else if (m_scheme == Scheme::TWO)
            {
                if (byte == 0xFF && !m_bitset.test(i + 8))
                {
                    AddByte(0xFF, result);
                    i += 9;
                }
                else
                {
                    AddByte(byte, result);
                    i += 8;
                }
            }
            else
            {
                assert(false);
            }
        }
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


        case ControlSymbol::RESET1:
        {
            m_history.Reset();
            m_scheme = Scheme::ONE;
        } break;

        case ControlSymbol::RESET2:
        {
            m_history.Reset();
            m_scheme = Scheme::TWO;
        } break;
    }
}