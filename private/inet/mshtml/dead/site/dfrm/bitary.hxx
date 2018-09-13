//+-----------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994, 1995
//  All rights Reserved.
//  Information contained herein is Proprietary and Confidential.
//
//  File:      src\ddoc\include\bitary.hxx
//
//
//  Contents: The bitmap array implementation code
//
//  Classes:   CBitAry
//             CDDSelBitAry
//              
//
//  Owner:      LaszloG
//
//  Comments:   This is a bitmap array with a subscript [] operator and (slightly) optimized
//              bitmap storage. The unnamed union stores the bits if they're fewer than 32
//              or a pointer to them if they are more numerous.
//
//-------------------------------------------------------------------------

#ifndef _BITARY_HXX_
#define _BITARY_HXX_


#define DWORD_BITS (8 * sizeof(DWORD))



//+---------------------------------------------------------------------------
//  
//  Class:      CBitAry
//
//  Synopsis:   Bitmap array with test, set, reset, invert methods (inline) and
//              the necessary maintenance members
//
//
//  Comments:   The size is client-defined. If it is less than 32 bits, the bits
//              are stored internally. If more, the same DWORD is used to point
//              to the bit storage memory.
//
//----------------------------------------------------------------------------

class CBitAry
{
public:
    //  constructor
    CBitAry() { _dwBits = _cWords = 0; };

    //  destructor
    ~CBitAry();

public:

    //  Methods

    //  Grow
    HRESULT     SetSize(unsigned int uSize);    //  Set the size in bits
    HRESULT     Copy(const CBitAry &);
    void        Clear(void);


    //  merge two bitarrays (form a union by ORing all the bits)
    //  the resulting bitarray's size is that of the longer one. All bits in the nonexistent
    //  part of the other array are presumed to be 0.
    void Merge(const CBitAry &);

    //  get directly at the bits
    inline unsigned GetSize(void) const { return _cWords ? _cWords * DWORD_BITS : DWORD_BITS; };    //  in bits
    inline DWORD *  GetBits(void) { return _cWords ? _pdwBits : &_dwBits; };

    //+--------------------------------------------
    //  inline bit accessors
    //---------------------------------------------

    //  test the ith bit
    inline BOOL operator[] (unsigned int i)
    {
        BOOL    f;

        if ( _cWords )
        {
            f = _pdwBits[i / DWORD_BITS] & (0x1 << (i % DWORD_BITS));
        }
        else
        {
            f = _dwBits & (0x1 << i);
        }

        return f;
    }


    //  set the ith bit
    inline void Set(unsigned i)
    {
        if ( _cWords )
        {
            _pdwBits[i / DWORD_BITS] |= (0x1 << (i % DWORD_BITS));
        }
        else
        {
            _dwBits |= (0x1 << i);
        }
    }


    //  reset the ith bit
    inline void Reset(unsigned i)
    {
        if ( _cWords )
        {
            _pdwBits[i / DWORD_BITS] &= (~(0x1 << (i % DWORD_BITS)));
        }
        else
        {
            _dwBits &= (~(0x1 << i));
        }
    }


    //  invert the ith bit
    inline void Invert(unsigned i)
    {
        if ( _cWords )
        {
            _pdwBits[i / DWORD_BITS] ^= (0x1 << (i % DWORD_BITS));
        }
        else
        {
            _dwBits ^= (0x1 << i);
        }
    }

    //  Is the whole array empty?
    BOOL IsEmpty(void);

protected:
    union
    {
        DWORD * _pdwBits;
        DWORD   _dwBits;
    };
    unsigned _cWords;
};




//+---------------------------------------------------------------------------
//  
//  Class:      CDDSelBitAry
//
//  Synopsis:   Bitmap array derived from CBitAry. Extra feature is the storage of hidden
//              (or reserved) bits.
//
//  Comments:   This is the bitmap class used by the DataDoc selection code. It features a number
//              of reserved bits. Their number is maintained in a static member whose value
//              comes from an enum defined in the selection code, so
//              *DON'T USE THIS CLASS OUTSIDE THE SELECTION CODE!*
//              This is also enforced with private constructor and friend declaration.
//
//---------------------------------------------------------------------------------------

class CDDSelBitAry : public CBitAry
{
friend struct SelectUnit;

private:
    CDDSelBitAry() : CBitAry() {};     //  enforce tight coupling to selection code

public:

    inline BOOL GetReservedBit(unsigned i)  { return CBitAry::operator[](i); }
    inline void SetReservedBit(unsigned i)  { CBitAry::Set(i);   }
    inline void ResetReservedBit(unsigned i){ CBitAry::Reset(i); }

    inline BOOL operator[] (unsigned i)     { return CBitAry::operator[](i + s_cReservedBits);  }
    inline void Set(unsigned i)             {        CBitAry::Set(i + s_cReservedBits);         }
    inline void Reset(unsigned i)           {        CBitAry::Reset(i + s_cReservedBits);       }
    inline void Invert(unsigned i)          {        CBitAry::Invert(i + s_cReservedBits);      }

    inline HRESULT SetSize(unsigned i)      { return CBitAry::SetSize(i + s_cReservedBits);     }
    inline unsigned GetSize(void)           { return CBitAry::GetSize() - s_cReservedBits;      }

    //  Is the whole array empty?
    BOOL IsEmpty(void);
protected:

    static unsigned int s_cReservedBits;
};



#endif


//
//
//      end of file
//
//-------------------------------------------------------------------------

