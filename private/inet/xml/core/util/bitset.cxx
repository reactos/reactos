/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
/*
 */

#include "core.hxx"
#pragma hdrstop

#include "core/util/bitset.hxx"

DEFINE_CLASS_MEMBERS_CLONING(BitSet, _T("BitSet"), Base);

// BUGBUG - Consider changing from int to unsigned to remove "> 0" checks and the exceptions.

/**
 */
BitSet::BitSet(unsigned nbits) 
{
    // BUGBUG: should test for overflow, but is it really necessary ???

    /* subscript(nbits + MASK) is the length of the array needed to hold nbits */
    length = subscript(nbits + MASK);
    if (length > (sizeof(lbits)/sizeof(unsigned)))
    {
        bits = new unsigned[length];
        memset(bits, 0, length * sizeof(unsigned));
    }
    else
    {
        length = sizeof(lbits)/sizeof(unsigned);
        bits = lbits;
        // no need to clear it since it was cleared when we allocated this object
    }
}

BitSet::~BitSet()
{
    if (bits != lbits)
        delete bits;
}

/**
 */
void BitSet::ensureLength(unsigned nRequiredLength ) 
{
    /* Doesn't need to be synchronized because it's an internal method. */
    if (nRequiredLength > length) 
    {
        /* Ask for larger of doubled size or required size */
        unsigned request = max(2 * length, nRequiredLength);
        unsigned * newBits = new unsigned[request];
        memcpy(newBits, bits, length * sizeof(unsigned));
        memset(newBits + length, 0, length * sizeof(unsigned));
        length = request;
        if (bits != lbits)
            delete [] bits;
        bits = newBits;
    }
}

/**
 */
void BitSet::reset() 
{
    memset(bits, 0, length * sizeof(unsigned));
}

/**
 */
void BitSet::set(unsigned bit) 
{
    unsigned nBitSlot = subscript( bit );

    ensureLength( nBitSlot + 1 );
    bits[ nBitSlot ] |= (1L << (bit & MASK));
}

/**
 */
void BitSet::clear(unsigned bit) 
{
    unsigned n = subscript(bit);        /* always positive */
    if (n < length) 
    {
        bits[n] &= ~(1L << (bit & MASK));
    }
}

/**
 */
bool BitSet::get(unsigned bit) const
{
    bool result = false;
    unsigned n = subscript(bit);        /* always positive */
    if (n < length) 
    {
        result = ((bits[n] & (1L << (bit & MASK))) != 0);
    }
    return result;
}

/**
 */
void BitSet::and(BitSet * set) 
{
    /*
     * Need to synchronize  both this and set->
     * This might lead to deadlock if one thread grabs them in one order
     * while another thread grabs them the other order.
     * Use a trick from Doug Lea's book on concurrency,
     * somewhat complicated because BitSet overrides hashCode().
     */
    if (this == set) 
    {
        return;
    }

    unsigned bitsLength = length;
    unsigned setLength = set->length;
    unsigned n = min(bitsLength, setLength);
    for (unsigned i = n ; i-- > 0 ; ) 
    {
        bits[i] &= set->bits[i];
    }
    for (; n < bitsLength ; n++) 
    {
        bits[n] = 0;
    }
}

/**
 */
void BitSet::or(BitSet * set) 
{
    if (this == set) 
    {
        return;
    }

    unsigned setLength = set->length;
    ensureLength(setLength);
    for (unsigned i = setLength; i-- > 0 ;) 
    {
        bits[i] |= set->bits[i];
    }
}

/**
 */
void BitSet::xor(BitSet * set) 
{
    unsigned setLength = set->length;
    ensureLength(setLength);
    for (unsigned i = setLength; i-- > 0 ;) 
    {
        bits[i] ^= set->bits[i];
    }
}

/**
 */
int BitSet::hashCode() 
{
    int h = 1234;
    for (int i = length; --i >= 0; ) 
    {
        h ^= bits[i] * (i + 1);
    }
    return (int)((h >> 32) ^ h);
}

/**
 */
bool BitSet::equals(Object * obj) 
{
    if ((obj != null) && (BitSet::_getClass()->isInstance(obj))) 
    {
        if (this == obj) 
        {
            return true;
        }
        BitSet * set = (BitSet *) obj;

        int bitsLength = length;
        int setLength = set->length;
        int n = min(bitsLength, setLength);
        for (int i = n ; i-- > 0 ;) 
        {
            if (bits[i] != set->bits[i]) 
            {
                return false;
            }
        }
        if (bitsLength > n) 
        {
            for (int i = bitsLength ; i-- > n ;) 
            {
                if (bits[i] != 0) 
                {
                    return false;
                }
            }
        } 
        else 
        {
            for (int i = setLength ; i-- > n ;) 
            {
                if (set->bits[i] != 0) 
                {
                    return false;
                }
            }
        }
        return true;
    }
    return false;
}

/**
 */
Object * BitSet::clone() 
{
    BitSet * result = null;
    result = (BitSet *) super::clone();
    if (lbits == bits)
        result->bits = result->lbits;
    else
        result->bits = new unsigned[length];
    result->length = length;
    memcpy(result->bits, bits, length * sizeof(unsigned));
    return result;
}

#if DBG==1
// This is a waste to actually include in a retail build

/**
 */
String * BitSet::toString() 
{
    StringBuffer * buffer = StringBuffer::newStringBuffer();
    bool needSeparator = false;
    buffer->append(_T("{"));
    SYNCHRONIZED(this); 
    {
        int limit = size();
        int    i;

        for( i = 0 ; i < limit ; i++) 
        {
            int n = subscript( i );
            TCHAR tch;
            if ( (bits[ n ] & (1L << (i & MASK))) != 0 )
                tch = L'X';
            else
                tch = L'0';
            buffer->append(tch);
        }
    }

    buffer->append(_T("}"));
    return buffer->toString();

}
#endif
