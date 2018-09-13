//+-----------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994, 1995
//  All rights Reserved.
//  Information contained herein is Proprietary and Confidential.
//
//  File:      src\ddoc\datadoc\bitary.cxx
//
//
//  Contents: The bitmap array implementation code
//
//  Classes:   CBitAry
//
//  Maintained by:  LaszloG
//
//  This is a bitmap array with a subscript [] operator and (slightly) optimized
//  bitmap storage. The unnamed union stores the bits if they're fewer than 32
//  or a pointer to them if they are more numerous.
//
//-------------------------------------------------------------------------

#include "headers.hxx"
#include "dfrm.hxx"


#ifndef _BITARY_HXX_
#   include "bitary.hxx"
#endif





CBitAry::~CBitAry()
{
    if ( _cWords )
	{
		delete [] _pdwBits;
	}
}





HRESULT
CBitAry::Copy(const CBitAry& x)
{
    _cWords = x._cWords;

    if ( _cWords )
    {
        _pdwBits = new DWORD[_cWords];
        if ( ! _pdwBits )
            goto MemoryError;

        memcpy(_pdwBits, x._pdwBits, _cWords * sizeof(DWORD));
    }
    else
    {
        _dwBits = x._dwBits;
    }

    return S_OK;

MemoryError:
    return E_OUTOFMEMORY;
    
}


//+---------------------------------------------------------------------------
//  
//  Member:     CBitAry::Clear
//
//  Synopsis:   Clear all bits
//
//----------------------------------------------------------------------------
void
CBitAry::Clear()
{
    if (_cWords)
    {
        memset(&_pdwBits, 0, _cWords * sizeof(DWORD));
    }
    else
    {
        _dwBits = 0;
    }
}



//+---------------------------------------------------------------------------
//  
//  Member:     CBitAry::IsEmpty
//
//  Synopsis:   Checks if every bit is cleared.
//
//  Returns:    TRUE if all bits clear.
//
//----------------------------------------------------------------------------
BOOL
CBitAry::IsEmpty(void)
{
    BOOL f;         //  Fool the compiler

    if ( _cWords )
    {
        unsigned i;
        for ( f = FALSE, i = 0;
              !f && i < _cWords;
              i++ )
        {
            f |= _pdwBits[i];
        }
    }
    else
    {
        f = _dwBits;
    }

    return !f;
}




//+---------------------------------------------------------------------------
//  
//  Member:     CBitAry::SetSize
//
//  Synopsis:   Sets the bitarrays size to hold the requested number of bits.
//
//  Arguments:  uNewSize    the requested new size in bits.
//
//  Returns:    S_OK if everything is fine
//              E_OUTOFMEMORY if not enough memory to enlarge.
//
//  Comments:   For speed's sake it can only grow the bitmap. If the new size specified is
//              not bigger than the present size it just returns (no-op).
//
//----------------------------------------------------------------------------

HRESULT
CBitAry::SetSize(unsigned int uNewSize)
{
    DWORD * pBits;

    //  transform bit count into DWORD count
    uNewSize = (uNewSize + (DWORD_BITS - 1)) / DWORD_BITS;

	if ( _cWords < uNewSize )
	{
		DWORD * pNew;

		pNew = new DWORD[uNewSize];
		if ( ! pNew )
			RRETURN(E_OUTOFMEMORY);

		memset(pNew,0,uNewSize * sizeof(DWORD));
        
        pBits = _cWords ? _pdwBits : &_dwBits;
		memmove(_pdwBits, pNew, (_cWords * sizeof(DWORD)) );

		if ( _cWords )
		    delete [] _pdwBits;

		_pdwBits = pNew;
        _cWords = uNewSize;
	}

    RRETURN(S_OK);
}





//+---------------------------------------------------------------------------
//  
//  Member:     CBitAry::Merge 
//
//  Synopsis:   There is a bit set in both arrays, merge them and set the
//              bits between them to 1
//
//  Arguments:  other   the other bitarray
//
//----------------------------------------------------------------------------

void
CBitAry::Merge(const CBitAry& other)
{
    DWORD * pdwBits;
    const DWORD * pdwOtherBits;
    UINT i;
    DWORD dwFirst;
    DWORD dwLast;
    BOOL fFirst = TRUE;

    Assert(GetSize() == other.GetSize());
    if ( _cWords )
    {
        pdwBits = _pdwBits;
        pdwOtherBits = other._pdwBits;
        i = _cWords;
    }
    else
    {
        pdwBits = &_dwBits;
        pdwOtherBits = &other._dwBits;
        i = 1;
    }

    for (; i; i--, pdwBits++, pdwOtherBits++)
    {
        dwFirst = *pdwBits;
        dwLast = *pdwOtherBits;
        Assert( 0 == ((dwFirst << 1) & dwFirst) && 0 == ((dwFirst >> 1) & dwFirst) );
        Assert( 0 == ((dwLast << 1) & dwLast) && 0 == ((dwLast >> 1) & dwLast) );
        if (fFirst)
        {
            if (!(dwFirst ^ dwLast))
            {
                // both empty or the two bits happen to collide in which
                // case we just go thru all the dwords...
                continue;
            }
            if (!dwFirst || (dwLast && dwFirst > dwLast))
            {
                dwFirst = dwLast;
                dwLast = *pdwBits;
            }
            Assert(dwFirst && (0 == dwLast || dwFirst < dwLast));
            while (dwFirst != dwLast)
            {
                *pdwBits |= dwFirst;
                dwFirst <<= 1;
            }
            *pdwBits |= dwLast;
            fFirst = FALSE;
        }
        else
        {
            if (!(dwFirst |= dwLast))
            {
                // both empty, we haven't reached the second bit
                *pdwBits = (DWORD)~0;
            }
            else
            {
                while (dwFirst)
                {
                    *pdwBits |= dwFirst;
                    dwFirst >>= 1;
                }
                break;
            }
        }
    }
}




//+---------------------------------------------------------------------------
//  
//  Member:     CBitAry::IsEmpty
//
//  Synopsis:   Checks if every bit is cleared.
//
//  Returns:    TRUE if all bits clear.
//
//----------------------------------------------------------------------------
BOOL
CDDSelBitAry::IsEmpty(void)
{
    DWORD dw;

    // here we assume that s_cReservedBits cannot be more then 32 !
    if ( _cWords )
    {
        unsigned i;

        dw = _pdwBits[0];
        dw >>= s_cReservedBits;
        for ( i = 1; !dw && i < _cWords; i++ )
        {
            dw |= _pdwBits[i];
        }
    }
    else
    {
        dw = _dwBits >> s_cReservedBits;
    }

    return !dw;
}





//
//
//  end of file
//
//-------------------------------------------------------------------------

