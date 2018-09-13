#include "pch.h"
#pragma hdrstop

#include "bitset.h"


BitSet::BitSet(
    int cBits
    ) : m_pBuffer(NULL),
        m_cBits(0),
        m_cSet(0),
        m_cElements(0)
{
    Initialize(cBits);
}


BitSet::BitSet(
    const BitSet& rhs
    ) : m_pBuffer(NULL),
        m_cBits(0),
        m_cSet(0),
        m_cElements(0)
{
    *this = rhs;
}

BitSet& 
BitSet::operator = (
    const BitSet& rhs
    )
{
    if (this != &rhs)
    {
        Initialize(rhs.m_cBits);
        for (int i = 0; i < m_cElements; i++)
        {
            m_pBuffer[i] = rhs.m_pBuffer[i];
        }
    }
    return *this;
}


BitSet::~BitSet(
    void
    ) throw()
{
    delete [] m_pBuffer;
}


void
BitSet::Initialize(
    int cBits
    )
{
    delete [] m_pBuffer;
    m_cSet      = 0;
    m_cBits     = cBits;
    m_cElements = ElementInBuffer(m_cBits) + 1;
    m_pBuffer   = new ELEMENT_TYPE[m_cElements];
    ClrAll();
}


bool 
BitSet::GetBitState(
    int iBit
    ) const
{
    ValidateBitNumber(iBit);

    int iElement   = ElementInBuffer(iBit);
    bool bBitState = false;

    ELEMENT_TYPE mask = MaskFromBit(iBit);
    bBitState = (*(m_pBuffer + iElement) & mask) != 0;

    return bBitState;
}


void 
BitSet::SetBitState(
    int iBit,
    bool bSet
    )
{
    ValidateBitNumber(iBit);

    int iElement = ElementInBuffer(iBit);

    ELEMENT_TYPE mask = MaskFromBit(iBit);
    if (bSet)
    {
        *(m_pBuffer + iElement) |= mask;
        m_cSet++;
    }
    else
    {
        *(m_pBuffer + iElement) &= ~mask;
        m_cSet--;
    }
}

void 
BitSet::Complement(
    void
    ) throw()
{
    for (int i = 0; i < m_cElements; i++)
    {
        m_pBuffer[i] = ~m_pBuffer[i];
    }
    m_cSet = CountClr();
}


void 
BitSet::Dump(
    void
    ) const
{
#define BitSetOUT(s)  *pszOut++ = TEXT('\r'); \
                      *pszOut++ = TEXT('\n'); \
                      *pszOut++ = TEXT('\0'); \
                      OutputDebugString(s)

    TCHAR szOut[80];
    LPTSTR pszOut = szOut;
    const TCHAR szBits[] = TEXT("01");
    for (int i = 0; i < m_cBits; i++)
    {
        if (i % 8 == 0)
        {
            if (i % 32 == 0)
            {
                BitSetOUT(szOut);
                pszOut = szOut;
            }
            else
                *pszOut++ = TEXT(' ');
        }
        *pszOut++ = szBits[int(GetBitState(i))];
    }
    BitSetOUT(szOut);
}
