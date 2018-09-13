/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
/*
 */

#ifndef _CORE_UTIL_BITSET
#define _CORE_UTIL_BITSET


DEFINE_CLASS(BitSet);

const int BITS_PER_UNIT = 5;
const int MASK = (1 << BITS_PER_UNIT) - 1;

/**
 */
class BitSet : public Base
{
    DECLARE_CLASS_MEMBERS(BitSet, Base);
    DECLARE_CLASS_CLONING(BitSet, Base);

    // cloning constructor, shouldn't do anything with data members...
    protected:  BitSet(CloningEnum e) : super(e) {}

    /**
     */
    private: void ensureLength(unsigned nRequiredLength);

    /**
     */
    protected: unsigned subscript(unsigned bitIndex) const
    {
        return bitIndex >> BITS_PER_UNIT;
    }


public: 
    /**
     */
    BitSet(unsigned nbits = 1 << BITS_PER_UNIT);

    /**
     */
    ~BitSet();

    /**
     */
    void reset();

    /**
     */
    void set(unsigned bit);

    /**
     */
    void clear(unsigned bit);

    /**
     */
    bool get(unsigned bit) const;

    /**
     */
    void and(BitSet * set);

    /**
     */
    void or(BitSet * set);

    /**
     */
    void xor(BitSet * set);

    /**
     */
    virtual int hashCode();

    /**
     */
	inline ULONG_PTR identityHashCode(Object * x)
    {
        return (ULONG_PTR)x;
    }         

    /**
     */
    int size() const
    {
        /* This doesn't need to be synchronized, since it just reads a field. */
        return length << BITS_PER_UNIT;
    }

    /**
     */
    virtual bool equals(Object * obj);

    /**
     */
    virtual Object * clone();

#if DBG==1
    /**
     */
    virtual String * toString();
#endif

protected: 
    virtual void finalize()
    {
        super::finalize();
    }

    unsigned length;
    unsigned * bits;
    unsigned lbits[4]; // for small bitsets
};


#endif _CORE_UTIL_BITSET

