#ifndef _INC_CSCVIEW_BITSET_H
#define _INC_CSCVIEW_BITSET_H

//
// Macros & Constants
//
// ELEMENT_TYPE
//      This macro defines the type of each "element" in the bit buffer.
//      This type must be an integral type that can be used as an operand
//      in bitwise expressions.  If it is changed, the macros
//      ELEMENT_BITNUM_MASK and ELEMENT_SHIFT must also be changed as
//      follows:
//
//      ELEMENT_TYPE     ELEMENT_BITNUM_MASK     ELEMENT_SHIFT
//      ---------------- ----------------------- -------------------
//      BYTE             0x00000007              3
//      WORD             0x0000000F              4
//      DWORD            0x0000001F              5
//
// ELEMENT_BITNUM_MASK
//      This macro defines the mask used to extract the number of the
//      bit in the array element from the number of the bit being accessed.
//
// ELEMENT_SHIFT
//      This macro defines the number of bits to right shift the bit number
//      to obtain the number of the array element.
//
//
#define ELEMENT_TYPE            BYTE
const int ELEMENT_BITNUM_MASK = 0x00000007;  
const int ELEMENT_SHIFT       = 3;

//
// Forward declaration.
//
class BitSet;

//
// This is a "helper" class that aids BitSet::operator[].  So that 
// BitSet::operator[] functions properly for both lvalue and rvalue
// conditions, it returns a "Bit" object.  The bit object retains the
// bit number and a pointer to the "owner" BitSet object.  It also 
// overloads the bool conversion operator and assignment operator.
// Knowing the bit number and the owner BitSet, the class can set or 
// obtain the value of the bit in the BitSet's array.
//
class Bit 
{
    public:
        Bit(int iBit, BitSet *pOwnerSet = NULL) throw()
            : m_iBit(iBit),
              m_pOwnerSet(pOwnerSet) { }

        Bit(Bit& bit) throw()
            : m_iBit(bit.m_iBit),
              m_pOwnerSet(bit.m_pOwnerSet) { }

        //
        // Convert value of bit to a bool.
        //
        operator bool () const;

        //
        // Set bit to a bool value.
        //
        bool operator = (bool bState);

    private:
        DWORD m_iBit;        // Number of bit in BitSet [0 to n-1]
        BitSet *m_pOwnerSet; // Ptr to BitSet object.
};


class BitSet 
{
    public:
        BitSet(int cBits = 1);
        ~BitSet(void) throw();

        BitSet(const BitSet& rhs);
        BitSet& operator = (const BitSet& rhs);

        void Initialize(int cBits);

        int Count(void) const throw()
            { return m_cBits; }

        int CountSet(void) const throw()
            { return m_cSet; }

        int CountClr(void) const throw()
            { return m_cBits - m_cSet; }

        bool IsSet(int iBit) const throw()
            { return GetBitState(iBit); }

        bool IsClr(int iBit) const throw()
            { return !GetBitState(iBit); }

        void Complement(void) throw();

        //
        // SetBitState and GetBitState are the fastest
        // ways to alter or retrieve the state of a bit.
        //
        void SetBitState(int iBit, bool bSet);
        bool GetBitState(int iBit) const;

        //
        // Set and Clr are the next fastest ways to alter
        // or retrieve the state of a bit.
        //
        void Set(int iBit)
            { SetBitState(iBit, TRUE); }
        void Clr(int iBit)
            { SetBitState(iBit, FALSE); }

        //
        // Using the subscript operator is the slowest way
        // to alter/retrieve the state of a bit.
        //
        Bit operator [] (int iBit)
            { ValidateBitNumber(iBit); return Bit(iBit, this); }

        void ClrAll(void) throw()
            { ZeroMemory(m_pBuffer, sizeof(ELEMENT_TYPE) * m_cElements); m_cSet = 0; }
        void SetAll(void) throw()
            { FillMemory(m_pBuffer, sizeof(ELEMENT_TYPE) * m_cElements, (BYTE)0xFF); m_cSet = m_cBits; }

        void Dump(void) const;

    private:
        ELEMENT_TYPE *m_pBuffer;    // Array of elements.
        int m_cBits;     // Bits supported in set.
        int m_cSet;      // Number of bits set to '1'.
        int m_cElements; // Number of elements in array.

        //
        // Inline functions for calculating bit/element positions in array.
        //
        DWORD BitInElement(int iBit) const throw()
            { return (iBit & ELEMENT_BITNUM_MASK); }

        DWORD ElementInBuffer(int iBit) const throw()
            { return (iBit >> ELEMENT_SHIFT); }

        ELEMENT_TYPE MaskFromBit(int iBit) const throw()
            { return 1 << BitInElement(iBit); }

        void ValidateBitNumber(int iBit) const
            { 
                if (iBit >= m_cBits)
                    throw CException(ERROR_INVALID_INDEX);
            }

        friend class Bit;
};

inline
Bit::operator bool () const
{
	return m_pOwnerSet->GetBitState(m_iBit); 
}

inline bool 
Bit::operator = (bool bState)
{
    m_pOwnerSet->SetBitState(m_iBit, bState);
	return bState;
}




#endif // _INC_CSCVIEW_BITSET_H
