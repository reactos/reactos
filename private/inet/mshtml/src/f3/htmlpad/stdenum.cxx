//+---------------------------------------------------------------------
//
//  File:       stdenum.cxx
//
//  Contents:   Standard implementations of common enumerators
//
//----------------------------------------------------------------------

#include "padhead.hxx"

#ifndef X_LIMITS_H_
#define X_LIMITS_H_
#include <limits.h>         // for UINT_MAX below
#endif

//+---------------------------------------------------------------
//
//  Class:      CEnumXXX
//
//  Synopsis:   Base class for enumerators.
//
//----------------------------------------------------------------

class CEnumXXX : public IUnknown
{
public:
    DECLARE_FORMS_STANDARD_IUNKNOWN(CEnumXXX);

    //*** IEnumX methods ***
    STDMETHOD(Next) (ULONG c, void * pv, ULONG * pcFetched);
    STDMETHOD(Skip) (ULONG c);
    STDMETHOD(Reset) (void);
    STDMETHOD(Clone) (void ** ppEnumXXX) = 0;

protected:
    CEnumXXX(REFIID iid, int c, int i);
    virtual ~CEnumXXX();

    virtual HRESULT FetchElements(int c, void *pv) = 0;

    REFIID _iid;
    int  _c;
    int  _i;
};

//+---------------------------------------------------------------
//
//  Member:     CEnumXXX::CEnumXXX
//
//  Synopsis:   Constructor.
//
//  Arguments:  iid     iid for this enumerator.
//              c       count of elements
//              i       starting index
//
//----------------------------------------------------------------

CEnumXXX::CEnumXXX(REFIID iid, int c, int i)
    : _iid(iid)
{
    _c = c;
    _i = i;
    _ulRefs = 1;
    IncrementObjectCount();
}

CEnumXXX::~CEnumXXX()
{
    DecrementObjectCount();
}

//+---------------------------------------------------------------
//
//  Member:     CEnumXXX::QueryInterface, IUnknown
//
//  Synopsis:   Method of IUnknown interface
//
//----------------------------------------------------------------

STDMETHODIMP
CEnumXXX::QueryInterface(REFIID iid, void ** ppvObj)
{
    if (IsEqualIID(iid,IID_IUnknown) || IsEqualIID(iid,_iid))
    {
        *ppvObj = this;
        AddRef();
        return NOERROR;
    }
    else
    {
        *ppvObj = 0;
        return E_NOINTERFACE;
    }
}

//+---------------------------------------------------------------
//
//  Member:     CEnumXXX::Next
//
//  Synopsis:   Member of IEnumXXX interface
//
//----------------------------------------------------------------

STDMETHODIMP
CEnumXXX::Next(ULONG c, void *pv, ULONG *pcFetched)
{
    int   cFetched;
    HRESULT hr;

    cFetched = c;
    if (cFetched > _c - _i)
        cFetched = _c - _i;

    Assert(cFetched >= 0);

    hr = THR(FetchElements(cFetched, pv));
    if (hr)
        goto Error;

    _i += cFetched;
    hr = cFetched == (int)c ? S_OK : S_FALSE;

Cleanup:
    if (pcFetched)
    {
        *pcFetched = cFetched;
    }

    RRETURN1(hr, S_FALSE);

Error:
    cFetched = 0;
    goto Cleanup;
}

//+---------------------------------------------------------------
//
//  Member:     CEnumXXX::Skip
//
//  Synopsis:   Member of IEnumXXX interface
//
//----------------------------------------------------------------

STDMETHODIMP
CEnumXXX::Skip(ULONG c)
{
    _i += c;
    if (_i > _c)
        _i = c;

    return _i == _c ? S_FALSE : S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     StdEnumOLEVERB::Reset
//
//  Synopsis:   Member of IEnumXXX interface
//
//----------------------------------------------------------------

STDMETHODIMP
CEnumXXX::Reset()
{
    _i = 0;
    return NOERROR;
}


//+---------------------------------------------------------------
//
//  Class:      StdEnumOLEVERB
//
//  Purpose:    Standard enumerator of OLEVERB tables
//
//---------------------------------------------------------------

class CEnumOLEVERB: public CEnumXXX
{
    friend HRESULT CreateOLEVERBEnum(LPOLEVERB, ULONG, LPENUMOLEVERB FAR*);

public:
    // IEnumOLEVERB methods
    STDMETHOD(Clone) (void ** ppEnumXXX);

    // CEnumXXX methods
    HRESULT FetchElements(int c, void *pv);

private:
    CEnumOLEVERB(LPOLEVERB pOleVerb, int cOleVerb, int iVerb);

    LPOLEVERB _pOleVerb;
};

//+---------------------------------------------------------------
//
//  Member:     CEnumOLEVERB::CEnumOLEVERB, private
//
//  Synopsis:   Constructor for CEnumOLEVERB objects
//
//  Arguments:  [pOleVerb] -- pointer to the beginning of the OLEVERB array
//              [cOleVerb] -- the number of elements in the array
//              [iOleVerb] -- starting index for enumerator.
//
//  Notes:      OLEVERB enumerators should be constructed using the
//              CreateOLEVERBEnum function.
//
//----------------------------------------------------------------

inline
CEnumOLEVERB::CEnumOLEVERB(LPOLEVERB pOleVerb, int cOleVerb, int iOleVerb)
    : CEnumXXX(IID_IEnumOLEVERB, cOleVerb, iOleVerb)
{
    _pOleVerb = pOleVerb;
}

//+---------------------------------------------------------------
//
//  Member:     StdEnumOLEVERB::FetchElements
//
//  Synopsis:   Fetch elements, called from CEnumXXX::Next
//
//  Arguments:  c   Number of elements to fetch from _i.
//                  Caller insures that this is valid.
//              pv  Where stuff the elements.
//
//----------------------------------------------------------------

HRESULT
CEnumOLEVERB::FetchElements(int c, void *pv)
{
    HRESULT   hr = S_OK;
    int       i;
    LPOLEVERB pOleVerb = (LPOLEVERB)pv;

    for (i = 0; i < c; i++)
    {
        pOleVerb[i] = _pOleVerb[i + _i];
        if (pOleVerb[i].lpszVerbName)
        {
            hr = TaskAllocString(pOleVerb[i].lpszVerbName,
                    &pOleVerb[i].lpszVerbName);
            if (hr)
                goto Error;
        }
    }

Cleanup:
    RRETURN(hr);

Error:
    while (--i >= 0)
    {
        TaskFreeString(pOleVerb[i].lpszVerbName);
    }
    memset(pv, 0, sizeof(OLEVERB) * c);
    goto Cleanup;
}


//+---------------------------------------------------------------
//
//  Member:     StdEnumOLEVERB::Clone
//
//  Synopsis:   Member of IEnumXXX interface
//
//----------------------------------------------------------------

STDMETHODIMP
CEnumOLEVERB::Clone(void **ppEnumXXX)
{
    *ppEnumXXX = new CEnumOLEVERB(_pOleVerb, _c, _i);
    RRETURN(*ppEnumXXX ? S_OK : E_OUTOFMEMORY);
}

//+---------------------------------------------------------------
//
//  Function:   CreateOLEVERBEnum, public
//
//  Synopsis:   Creates a standard enumerator over OLEVERB arrays
//
//  Arguments:  [pOleVerb] -- pointer to the beginning of the OLEVERB array
//              [cOleVerb] -- the number of elements in the array
//              [ppEnum] -- where the enumerator is returned
//
//  Returns:    Success if the enumerator could be successfully created
//
//  Notes:      This function is typically used in the IOleObject::EnumVerbs
//              method implementation.
//
//----------------------------------------------------------------

HRESULT
CreateOLEVERBEnum(LPOLEVERB pOleVerb, ULONG cOleVerb, LPENUMOLEVERB * ppEnum)
{
    *ppEnum = (LPENUMOLEVERB)new CEnumOLEVERB(pOleVerb, cOleVerb, 0);
    RRETURN(*ppEnum ? S_OK : E_OUTOFMEMORY);
}

//+---------------------------------------------------------------
//
//  Class:      StdEnumFORMATETC
//
//  Purpose:    Standard enumerator of FORMATETC tables
//
//---------------------------------------------------------------

class CEnumFORMATETC: public CEnumXXX
{
    friend HRESULT CreateFORMATETCEnum(LPFORMATETC, ULONG, LPENUMFORMATETC FAR*, BOOL fDeleteOnExit);
public:

    // IEnumOLEVERB methods
    STDMETHOD(Clone) (void ** ppEnumXXX);

    // CEnumXXX methods
    HRESULT FetchElements(int c, void *pv);

    CEnumFORMATETC  *_pClonedFrom;

protected:
    virtual ~CEnumFORMATETC();

private:
    CEnumFORMATETC(LPFORMATETC pFmt, int cFmt, int iFmt, BOOL fDeleteOnExit);

    LPFORMATETC     _pFmtEtc;
    BOOL            _fDeleteOnExit;

};

//+---------------------------------------------------------------
//
//  Member:     CEnumFORMATETC::CEnumFORMATETC, private
//
//  Synopsis:   Constructor for CEnumFORMATETC objects
//
//  Arguments:  [pFmtEtc] -- pointer to the beginning of the FORMATETC array
//              [cFmtEtc] -- the number of elements in the array
//              [iFmtEtc] -- starting position for enumerator.
//
//  Notes:      Static enumerators should be constructed using the
//              CreateFORMATETCEnum function.
//
//----------------------------------------------------------------

inline
CEnumFORMATETC::CEnumFORMATETC(LPFORMATETC pFmtEtc, int cFmtEtc, int iFmtEtc, BOOL fDeleteOnExit)
    : CEnumXXX(IID_IEnumFORMATETC, cFmtEtc, iFmtEtc)
{
    _pFmtEtc = pFmtEtc;
    _fDeleteOnExit = fDeleteOnExit;
    _pClonedFrom = 0;
}

CEnumFORMATETC::~CEnumFORMATETC()
{
    if (_fDeleteOnExit)
    {
        delete [] _pFmtEtc;
    }
    ReleaseInterface(_pClonedFrom);
}

//+---------------------------------------------------------------
//
//  Member:     CEnumFORMATETC::FetchElements
//
//  Synopsis:   Fetch elements, called from CEnumXXX::Next
//
//  Arguments:  c   Number of elements to fetch from _i.
//                  Caller insures that this is valid.
//              pv  Where stuff the elements.
//
//----------------------------------------------------------------

HRESULT
CEnumFORMATETC::FetchElements(int c, void *pv)
{
    HRESULT     hr = S_OK;
    int         i;
    LPFORMATETC pFmtEtc = (LPFORMATETC)pv;

    for (i = 0; i < c; i++)
    {
        pFmtEtc[i] = _pFmtEtc[i + _i];
        if (pFmtEtc[i].ptd)
        {
            pFmtEtc[i].ptd = (DVTARGETDEVICE *)CoTaskMemAlloc(sizeof(DVTARGETDEVICE));
            if (!pFmtEtc[i].ptd)
                goto MemoryError;
            *(pFmtEtc[i].ptd) = *(_pFmtEtc[i + _i].ptd);
        }
    }

Cleanup:
    RRETURN(hr);

MemoryError:
    while (--i >= 0)
    {
        CoTaskMemFree(pFmtEtc[i].ptd);
    }
    memset(pv, 0, sizeof(FORMATETC) * c);
    hr = E_OUTOFMEMORY;
    goto Cleanup;
}


//+---------------------------------------------------------------
//
//  Member:     CEnumFORMATETC::Clone
//
//  Synopsis:   Member of IEnumXXX interface
//
//----------------------------------------------------------------

STDMETHODIMP
CEnumFORMATETC::Clone(void **ppEnumXXX)
{
    *ppEnumXXX = new CEnumFORMATETC(_pFmtEtc, _c, _i, FALSE);
    if (*ppEnumXXX && _fDeleteOnExit)
    {
        ((CEnumFORMATETC*)*ppEnumXXX)->_pClonedFrom = this;
        AddRef();
    }
    RRETURN(*ppEnumXXX ? S_OK : E_OUTOFMEMORY);
}

//+---------------------------------------------------------------
//
//  Function:   CreateFORMATETCEnum, public
//
//  Synopsis:   Creates a standard enumerator over FORMATETC arrays
//
//  Arguments:  [pFmtEtc] -- pointer to the beginning of the FORMATETC array
//              [cFmtEtc] -- the number of elements in the array
//              [ppenum] -- where the enumerator is returned
//
//  Returns:    Success if the enumerator could be successfully created
//
//  Notes:      This function is typically used in the IDataObject::EnumFormatetc
//              method implementation.
//
//----------------------------------------------------------------

HRESULT
CreateFORMATETCEnum(LPFORMATETC pFmtEtc, ULONG cFmtEtc, LPENUMFORMATETC * ppEnum, BOOL fDeleteOnExit)
{
    *ppEnum = (IEnumFORMATETC *)new CEnumFORMATETC(pFmtEtc, cFmtEtc, 0, fDeleteOnExit);
    RRETURN(*ppEnum ? S_OK : E_OUTOFMEMORY);
}


#if 0   // this maybe useful later but is not currently used.

//+---------------------------------------------------------------
//
//  Class:      StdStaticEnum
//
//  Purpose:    Enumerates over a static array
//
//  Notes:      This may not be used to enumerate over structures
//              that are "deep".  For instance, it cannot be used
//              to enumerate over an array of FORMATETCs because such
//              an enumerator needs to deep copy the ptd field
//              and the enumerator client frees these allocated ptd.
//              Similarly for the OLEVERB structure where the verb
//              name string must be deep copied.
//
//---------------------------------------------------------------

class StdStaticEnum: public IUnknown
{
    friend HRESULT CreateStaticEnum(REFIID, LPVOID, ULONG, ULONG, LPVOID FAR*);

public:
    DECLARE_FORMS_STANDARD_IUNKNOWN(StdStaticEnum);

    //*** IEnumerator methods ***
    STDMETHOD(Next) (ULONG celt, LPVOID pArrayObjs, ULONG FAR* pceltFetched);
    STDMETHOD(Skip) (ULONG celt);
    STDMETHOD(Reset) (void);
    STDMETHOD(Clone) (LPSTDSTATICENUM FAR* ppenm);

private:
    // constructor/destructor
    StdStaticEnum(REFIID riid, LPVOID pStart, ULONG cSize, ULONG cCount);
    ~StdStaticEnum(void);

    IID _iid;
    LPVOID _pStart;
    ULONG _cSize;
    ULONG _cCount;
    ULONG _cCurrent;
};

//+---------------------------------------------------------------
//
//  Member:     StdStaticEnum::StdStaticEnum, private
//
//  Synopsis:   Constructor for StdStaticEnum objects
//
//  Arguments:  [riid] -- the enumerator interface that this class is
//                          "pretending" to be.
//              [pStart] -- pointer to the beginning of the static array
//              [cSize] -- the size of the elements of the array
//              [cCount] -- the number of elements in the array
//
//  Notes:      Static enumerators should be constructed using the
//              CreateStaticEnum function.
//
//----------------------------------------------------------------

StdStaticEnum::StdStaticEnum(REFIID riid,
        LPVOID pStart,
        ULONG cSize,
        ULONG cCount)
{
    _ulRefs = 1;
    _iid = riid;
    _pStart = pStart;
    _cSize = cSize;
    _cCount = cCount;
    _cCurrent = 0;

    TraceTag((tagStdEnum, "StdStaticEnum constructed."));
}

//+---------------------------------------------------------------
//
//  Member:     StdStaticEnum::~StdStaticEnum, private
//
//  Synopsis:   Destructor for StdStaticEnum objects
//
//  Notes:      Static enumerators should never be `deleted' but
//              instead IUnknown::Release'd.
//
//----------------------------------------------------------------

StdStaticEnum::~StdStaticEnum(void)
{
    TraceTag((tagStdEnum, "StdStaticEnum destructed."));
}

//+---------------------------------------------------------------
//
//  Member:     StdStaticEnum::QueryInterface, public
//
//  Synopsis:   Method of IUnknown interface
//
//----------------------------------------------------------------

STDMETHODIMP
StdStaticEnum::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    if (IsEqualIID(riid,IID_IUnknown) || IsEqualIID(riid,_iid))
    {
        *ppvObj = this;
        AddRef();
        return NOERROR;
    }
    return E_NOINTERFACE;
}

//+---------------------------------------------------------------
//
//  Member:     StdStaticEnum::Next
//
//  Synopsis:   Member of IEnumXXX interface
//
//----------------------------------------------------------------

STDMETHODIMP
StdStaticEnum::Next(ULONG celt,
        LPVOID pArrayObjs,
        ULONG FAR* pceltFetched)
{
    ULONG celtFetched = min(celt, _cCount-_cCurrent);

    // calculate the number of bytes to copy
    if (celtFetched != 0 && _cSize > (UINT_MAX/celtFetched))
    {
        return E_FAIL;         // overflow!
    }

    UINT count = (UINT) (celtFetched*_cSize);
    _fmemcpy(pArrayObjs, (LPBYTE)_pStart+_cCurrent*_cSize, count);
    _cCurrent += celtFetched;
    if (pceltFetched != NULL)
    {
        *pceltFetched = celtFetched;
    }
    return ((celtFetched == celt) ? NOERROR : S_FALSE);
}

//+---------------------------------------------------------------
//
//  Member:     StdStaticEnum::Skip
//
//  Synopsis:   Member of IEnumXXX interface
//
//----------------------------------------------------------------

STDMETHODIMP
StdStaticEnum::Skip(ULONG celt)
{
    _cCurrent += celt;
    if (_cCurrent >= _cCount)
    {
        _cCurrent = _cCount;
        return S_FALSE;
    }
    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     StdStaticEnum::Reset
//
//  Synopsis:   Member of IEnumXXX interface
//
//----------------------------------------------------------------

STDMETHODIMP
StdStaticEnum::Reset(void)
{
    _cCurrent = 0;
    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     StdStaticEnum::Clone
//
//  Synopsis:   Member of IEnumXXX interface
//
//----------------------------------------------------------------

STDMETHODIMP
StdStaticEnum::Clone(LPSTDSTATICENUM FAR* ppenm)
{
    HRESULT r;
    LPSTDSTATICENUM penum = new StdStaticEnum(
                                                    _iid,
                                                    _pStart,
                                                    _cSize,
                                                    _cCount);
    if (penum == NULL)
    {
        r = E_OUTOFMEMORY;
    }
    else
    {
        penum->_cCurrent = _cCurrent;
        *ppenm = penum;
        r = NOERROR;
    }
    return r;
}

//+---------------------------------------------------------------
//
//  Function:   CreateStaticEnum, public
//
//  Synopsis:   Creates a standard enumerator over static arrays
//
//  Arguments:  [riid] -- the enumerator interface that this class is
//                          "pretending" to be.
//              [pStart] -- pointer to the beginning of the static array
//              [cSize] -- the size of the elements of the array
//              [cCount] -- the number of elements in the array
//              [ppenum] -- where the enumerator is returned
//
//  Returns:    Success if the enumerator could be successfully created
//
//----------------------------------------------------------------

HRESULT
CreateStaticEnum(REFIID riid,
        LPVOID pStart,
        ULONG cSize,
        ULONG cCount,
        LPVOID FAR* ppenum)
{
    HRESULT r;
    LPSTDSTATICENUM penum = new StdStaticEnum(
                                                    riid,
                                                    pStart,
                                                    cSize,
                                                    cCount);
    if (penum == NULL)
    {
        r = E_OUTOFMEMORY;
    }
    else
    {
        *ppenum = penum;
        r = NOERROR;
    }
    return r;
}

#endif // 0
