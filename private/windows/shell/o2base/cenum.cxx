//+------------------------------------------------------------------------
//
//  File:       cenum.cxx
//
//  Contents:   Generic enumerator class.
//
//  Classes:    CEnumGeneric
//
//-------------------------------------------------------------------------

#include <headers.hxx>
#pragma hdrstop

//+------------------------------------------------------------------------
//
//  Member:     CEnumGeneric::Create
//
//  Synopsis:   Helper method that creates a generic enumerator.
//
//  Arguments:  [iid]       Interface implemented by the enumerator
//              [cb]        Size of each element
//              [c]         Number of elements
//              [pv]        Pointer to first element
//
//  Returns:    CEnumGeneric *; NULL if OOM
//
//-------------------------------------------------------------------------
CEnumGeneric *
CEnumGeneric::Create(REFIID iid, int cb, int c, void * pv, BOOL fAddRef)
{
    CEnumGeneric *  pEnum;

    //pEnum = new (NullOnFail) CEnumGeneric(iid, cb, c, pv, fAddRef);
    pEnum = new CEnumGeneric(iid, cb, c, pv, fAddRef);
    if (!pEnum)
        return NULL;

    pEnum->AddRef();
    return pEnum;
}



//+------------------------------------------------------------------------
//
//  Member:     CEnumGeneric::CEnumGeneric
//
//  Synopsis:   Generic array-based enumerator constructor.
//
//  Arguments:  [iid]       Interface implemented by the enumerator
//              [cb]        Size of each element
//              [c]         Number of elements
//              [pv]        Pointer to first element
//
//-------------------------------------------------------------------------
CEnumGeneric::CEnumGeneric( REFIID iid, int cb, int c, void * pv, BOOL fAddRef )
{
    _refs = 0;

    _iid = iid;

    _fAddRef = fAddRef;
    _cb = cb;
    _c = c;
    _i = 0;
    _pv = pv;
}



//+------------------------------------------------------------------------
//
//  Member:     CEnumGeneric::CEnumGeneric
//
//  Synopsis:   Generic enumerator constructor. Creates a new copy of an
//              existing enumerator.
//
//  Arguments:  [pEnum]     Enumerator to copy.
//
//-------------------------------------------------------------------------
CEnumGeneric::CEnumGeneric( CEnumGeneric * pEnum )
{
    _refs = 0;
    _iid = pEnum->_iid;
    _cb = pEnum->_cb;
    _c = pEnum->_c;
    _i = pEnum->_i;
    _pv = pEnum->_pv;
    _fAddRef = pEnum->_fAddRef;
}



//+------------------------------------------------------------------------
//
//  Member:     CEnumGeneric::QueryInterface
//
//  Synopsis:   The enumerator implements two interfaces, IUnknown and
//              whatever interface IID is passed to its constructor.
//
//  Arguments:  [iid]
//              [ppv]
//
//  Returns:    HRESULT (STDMETHOD)
//
//-------------------------------------------------------------------------
STDMETHODIMP
CEnumGeneric::QueryInterface(REFIID iid, LPVOID * ppv)
{
    if (IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, _iid))
    {
        AddRef();
        *ppv = this;
        return NOERROR;
    }
#if DBG
    DOUT(L"CEnumGeneric::QueryInterface E_NOINTERFACE\r\n");
#endif
    return E_NOINTERFACE;
}



//+------------------------------------------------------------------------
//
//  Member:     CEnumGeneric::AddRef
//
//  Synopsis:   Normal implementation.
//
//  Returns:    ULONG (STDMETHOD)
//
//-------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CEnumGeneric::AddRef( )
{
    return ++_refs;
}



//+------------------------------------------------------------------------
//
//  Member:     CEnumGeneric::Release
//
//  Synopsis:   Normal implementation.
//
//  Returns:    ULONG (STDMETHOD)
//
//-------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CEnumGeneric::Release( )
{
    if (!--_refs)
    {
        delete this;
        return 0;
    }
    return _refs;
}



//+------------------------------------------------------------------------
//
//  Member:     CEnumGeneric::Next
//
//  Synopsis:   Returns the next celt members in the enumeration. If less
//              than celt members remain, then the remaining members are
//              returned and S_FALSE is reported. In all cases, the number
//              of elements actually returned in placed in *pceltFetched.
//
//  Arguments:  [celt]          Number of elements to fetch
//              [reelt]         The elements are returned in reelt[]
//              [pceltFetched]  Number of elements actually fetched
//
//  Returns:    HRESULT (STDMETHOD)
//
//-------------------------------------------------------------------------
STDMETHODIMP
CEnumGeneric::Next(ULONG celt, void * reelt, ULONG * pceltFetched)
{
    int c = min((int) celt, _c - _i);
    if(_fAddRef)
    {
        for(int i = 0; i < c; i++)
        {
            LPUNKNOWN *ppUnk = (LPUNKNOWN*)(((BYTE *) _pv) + (_i + i) * _cb);
            (*ppUnk)->AddRef();
        }
    }
    memcpy(reelt, ((BYTE *) _pv) + _i * _cb, c * _cb);
    if (pceltFetched)
    {
        *pceltFetched = c;
    }
    _i += c;

    return ((c == (int) celt) ? NOERROR : S_FALSE);
}



//+------------------------------------------------------------------------
//
//  Member:     CEnumGeneric::Skip
//
//  Synopsis:   Skips the next celt elements. If less than celt elements
//              remain, the enumerator skips to the end and returns S_FALSE.
//
//  Arguments:  [celt]      Number of elements to skip.
//
//  Returns:    HRESULT (STDMETHOD)
//
//-------------------------------------------------------------------------
STDMETHODIMP
CEnumGeneric::Skip( ULONG celt )
{
    int c = min((int) celt, _c - _i);
    _i += c;

    return ((c == (int) celt) ? NOERROR : S_FALSE);
}


//+------------------------------------------------------------------------
//
//  Member:     CEnumGeneric::Reset
//
//  Synopsis:   Resets the enumerator to the beginning of the enumeration.
//
//  Returns:    HRESULT (STDMETHOD)
//
//-------------------------------------------------------------------------
STDMETHODIMP
CEnumGeneric::Reset(  )
{
    _i = 0;

    return NOERROR;
}


//+------------------------------------------------------------------------
//
//  Member:     CEnumGeneric::Clone
//
//  Synopsis:   Creates a copy of this enumerator; the copy should have the
//              same state as this enumerator.
//
//  Arguments:  [ppEnum]    New enumerator is returned in *ppEnum
//
//  Returns:    HRESULT (STDMETHOD)
//
//-------------------------------------------------------------------------
STDMETHODIMP
CEnumGeneric::Clone( CEnumGeneric * * ppEnum )
{
    //if (!(*ppEnum = new (NullOnFail) CEnumGeneric(this)))
    if (!(*ppEnum = new CEnumGeneric(this)))
    {
        DOUT(L"o2base/CEnumGeneric::Clone failed\r\n");
        return E_OUTOFMEMORY;
    }

    (*ppEnum)->AddRef();
    return NOERROR;
}
