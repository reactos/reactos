//+---------------------------------------------------------------------
//
//  File:       stdenum.cxx
//
//  Contents:   Standard implementations of common enumerators
//
//----------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop
#include <limits.h>         // for UINT_MAX below

//
//  forward declarations
//

class StdEnumOLEVERB;
typedef StdEnumOLEVERB FAR* LPSTDENUMOLEVERB;

class StdEnumFORMATETC;
typedef StdEnumFORMATETC FAR* LPSTDENUMFORMATETC;

#if 1
class StdStaticEnum;
typedef StdStaticEnum FAR* LPSTDSTATICENUM;
#endif  // 0


//+---------------------------------------------------------------
//
//  Class:      StdEnumOLEVERB
//
//  Purpose:    Standard enumerator of OLEVERB tables
//
//---------------------------------------------------------------

class StdEnumOLEVERB: public IEnumOLEVERB
{
    friend HRESULT CreateOLEVERBEnum(LPOLEVERB, ULONG, LPENUMOLEVERB FAR*);

public:
    DECLARE_STANDARD_IUNKNOWN(StdEnumOLEVERB);

    // *** IEnumOLEVERB methods ***
    STDMETHOD(Next) (ULONG celt, LPOLEVERB rgelt, ULONG FAR* pceltFetched);
    STDMETHOD(Skip) (ULONG celt);
    STDMETHOD(Reset) (void);
    STDMETHOD(Clone) (IEnumOLEVERB FAR* FAR* ppenm);

private:
    StdEnumOLEVERB(LPOLEVERB pStart, ULONG cCount);
    ~StdEnumOLEVERB(void);

    LPOLEVERB _pStart;
    ULONG _cCount;
    ULONG _cCurrent;
};

//+---------------------------------------------------------------
//
//  Member:     StdEnumOLEVERB::StdEnumOLEVERB, private
//
//  Synopsis:   Constructor for StdEnumOLEVERB objects
//
//  Arguments:  [pStart] -- pointer to the beginning of the OLEVERB array
//              [cCount] -- the number of elements in the array
//
//  Notes:      OLEVERB enumerators should be constructed using the
//              CreateOLEVERBEnum function.
//
//----------------------------------------------------------------

StdEnumOLEVERB::StdEnumOLEVERB(LPOLEVERB pStart, ULONG cCount)
{
    DOUT(L"StdEnumOLEVERB constructed.\r\n");

    _ulRefs = 1;
    _pStart = pStart;
    _cCount = cCount;
    _cCurrent = 0;
}

//+---------------------------------------------------------------
//
//  Member:     StdEnumOLEVERB::~StdEnumOLEVERB, private
//
//  Synopsis:   Destructor for StdEnumOLEVERB objects
//
//  Notes:      Static enumerators should never be `deleted' but
//              instead IUnknown::Release'd.
//
//----------------------------------------------------------------

StdEnumOLEVERB::~StdEnumOLEVERB(void)
{
    DOUT(L"StdEnumOLEVERB destructed.\r\n");
}

IMPLEMENT_STANDARD_IUNKNOWN(StdEnumOLEVERB)

//+---------------------------------------------------------------
//
//  Member:     StdEnumOLEVERB::QueryInterface, public
//
//  Synopsis:   Method of IUnknown interface
//
//----------------------------------------------------------------

STDMETHODIMP
StdEnumOLEVERB::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
#ifdef VERBOSE_DBG
    OLECHAR achBuffer[256];
    wsprintf(achBuffer,
            L"StdEnumOLEVERB::QueryInterface  (%lx)\r\n",
            riid.Data1);
    DOUT(achBuffer);
#endif //VERBOSE_DBG
    if (IsEqualIID(riid,IID_IUnknown) || IsEqualIID(riid,IID_IEnumOLEVERB))
    {
        *ppvObj = this;
        AddRef();
        return NOERROR;
    }
#ifdef VERBOSE_DBG
    DOUT(L"StdEnumOLEVERB::QueryInterface returning E_NOINTERFACE\r\n");
#endif //VERBOSE_DBG
    return E_NOINTERFACE;
}

//+---------------------------------------------------------------
//
//  Member:     StdEnumOLEVERB::Next
//
//  Synopsis:   Member of IEnumXXX interface
//
//----------------------------------------------------------------

STDMETHODIMP
StdEnumOLEVERB::Next(ULONG celt,
                    LPOLEVERB pArrayObjs,
                    ULONG FAR* pceltFetched)
{
#ifdef VERBOSE_DBG
    DOUT(L"StdEnumOLEVERB::Next\r\n");
#endif //VERBOSE_DBG
    ULONG celtFetched = min(celt, _cCount-_cCurrent);
    for (ULONG i = 0; i < celtFetched; i++, _cCurrent++)
    {
        LPOLEVERB pVerb = &_pStart[_cCurrent];

        pArrayObjs[i] = *pVerb;
        if (pVerb->lpszVerbName!=NULL)
        {
            HRESULT r;
            r = TaskAllocString(pVerb->lpszVerbName,
                    &pArrayObjs[i].lpszVerbName);
            if (!OK(r))
                return r;
        }
    }

    if (pceltFetched != NULL)
    {
        *pceltFetched = celtFetched;
    }

    return ((celtFetched == celt) ? NOERROR : S_FALSE);
}

//+---------------------------------------------------------------
//
//  Member:     StdEnumOLEVERB::Skip
//
//  Synopsis:   Member of IEnumXXX interface
//
//----------------------------------------------------------------

STDMETHODIMP
StdEnumOLEVERB::Skip(ULONG celt)
{
    DOUT(L"StdEnumOLEVERB::Skip\r\n");

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
//  Member:     StdEnumOLEVERB::Reset
//
//  Synopsis:   Member of IEnumXXX interface
//
//----------------------------------------------------------------

STDMETHODIMP
StdEnumOLEVERB::Reset(void)
{
    DOUT(L"StdEnumOLEVERB::Reset\r\n");

    _cCurrent = 0;
    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     StdEnumOLEVERB::Clone
//
//  Synopsis:   Member of IEnumXXX interface
//
//----------------------------------------------------------------

STDMETHODIMP
StdEnumOLEVERB::Clone(LPENUMOLEVERB FAR* ppenm)
{
    DOUT(L"StdEnumOLEVERB::Clone\r\n");

    HRESULT r = E_OUTOFMEMORY;
    LPSTDENUMOLEVERB penum = new StdEnumOLEVERB(_pStart, _cCount);
    if (penum != NULL)
    {
        r = NOERROR;
        penum->_cCurrent = _cCurrent;
        *ppenm = penum;
    }
    else
    {
        DOUT(L"o2base/StdEnumOLEVERB::Clone failed\r\n");
    }
    return r;
}

//+---------------------------------------------------------------
//
//  Function:   CreateOLEVERBEnum, public
//
//  Synopsis:   Creates a standard enumerator over OLEVERB arrays
//
//  Arguments:  [pVerbs] -- pointer to the beginning of the OLEVERB array
//              [cVerbs] -- the number of elements in the array
//              [ppenum] -- where the enumerator is returned
//
//  Returns:    Success if the enumerator could be successfully created
//
//  Notes:      This function is typically used in the IOleObject::EnumVerbs
//              method implementation.
//
//----------------------------------------------------------------

HRESULT
CreateOLEVERBEnum(LPOLEVERB pVerbs, ULONG cVerbs, LPENUMOLEVERB FAR* ppenum)
{
    HRESULT r = E_OUTOFMEMORY;
    LPSTDENUMOLEVERB penum = new StdEnumOLEVERB(pVerbs, cVerbs);
    if (penum != NULL)
    {
        r = NOERROR;
        *ppenum = penum;
    }
    else
    {
        DOUT(L"o2base/stdenum/CreateOLEVERBEnum failed\r\n");
    }
    return r;
}

//+---------------------------------------------------------------
//
//  Class:      StdEnumFORMATETC
//
//  Purpose:    Standard enumerator of FORMATETC tables
//
//---------------------------------------------------------------

class StdEnumFORMATETC: public IEnumFORMATETC
{
    friend HRESULT CreateFORMATETCEnum(LPFORMATETC, ULONG, LPENUMFORMATETC FAR*);
public:
    DECLARE_STANDARD_IUNKNOWN(StdEnumFORMATETC);

    // *** IEnumFORMATETC methods ***
    STDMETHOD(Next) (ULONG celt, LPFORMATETC rgelt, ULONG FAR* pceltFetched);
    STDMETHOD(Skip) (ULONG celt);
    STDMETHOD(Reset) (void);
    STDMETHOD(Clone) (IEnumFORMATETC FAR* FAR* ppenm);

private:
    StdEnumFORMATETC(LPFORMATETC pStart, ULONG cCount);
    ~StdEnumFORMATETC(void);

    LPFORMATETC _pStart;
    ULONG _cCount;
    ULONG _cCurrent;
};

//+---------------------------------------------------------------
//
//  Member:     StdEnumFORMATETC::StdEnumFORMATETC, private
//
//  Synopsis:   Constructor for StdEnumFORMATETC objects
//
//  Arguments:  [pStart] -- pointer to the beginning of the FORMATETC array
//              [cCount] -- the number of elements in the array
//
//  Notes:      Static enumerators should be constructed using the
//              CreateFORMATETCEnum function.
//
//----------------------------------------------------------------

StdEnumFORMATETC::StdEnumFORMATETC(LPFORMATETC pStart, ULONG cCount)
{
    _ulRefs = 1;
    _pStart = pStart;
    _cCount = cCount;
    _cCurrent = 0;

    DOUT(L"StdEnumFORMATETC constructed.\r\n");
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

StdEnumFORMATETC::~StdEnumFORMATETC(void)
{
    DOUT(L"StdEnumFORMATETC destructed.\r\n");
}

IMPLEMENT_STANDARD_IUNKNOWN(StdEnumFORMATETC)

//+---------------------------------------------------------------
//
//  Member:     StdEnumFORMATETC::QueryInterface, public
//
//  Synopsis:   Method of IUnknown interface
//
//----------------------------------------------------------------

STDMETHODIMP
StdEnumFORMATETC::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
#ifdef VERBOSE_DBG
    OLECHAR achBuffer[256];
    wsprintf(achBuffer,
            L"StdEnumFORMATETC::QueryInterface (%lx)\r\n",
            riid.Data1);
    DOUT(achBuffer);
#endif //VERBOSE_DBG

    if (IsEqualIID(riid,IID_IUnknown) || IsEqualIID(riid,IID_IEnumFORMATETC))
    {
        *ppvObj = this;
        AddRef();
        return NOERROR;
    }

#if VERBOSE_DBG
    wsprintf(achBuffer,
            L"StdEnumFORMATETC::QueryInterface returning E_NOINTERFACE for %lx\r\n",
            riid.Data1);
    DOUT(achBuffer);
#endif //VERBOSE_DBG

    return E_NOINTERFACE;
}

//+---------------------------------------------------------------
//
//  Member:     StdEnumFORMATETC::Next
//
//  Synopsis:   Member of IEnumXXX interface
//
//----------------------------------------------------------------

STDMETHODIMP
StdEnumFORMATETC::Next(ULONG celt,
                        LPFORMATETC pArrayObjs,
                        ULONG FAR* pceltFetched)
{
    ULONG celtFetched = min(celt, _cCount - _cCurrent);
    for (ULONG i = 0; i < celtFetched; i++, _cCurrent++)
    {
        LPFORMATETC pFormat = &_pStart[_cCurrent];

        // deep copy the FORMATETC structure
        pArrayObjs[i] = *pFormat;

#ifdef VERBOSE_DBG
    OLECHAR achBuffer[256];
    wsprintf(achBuffer,
            L"StdEnumFORMATETC::Next (cfFormat = %d)\r\n",
            pFormat->cfFormat);
    DOUT(achBuffer);
#endif //VERBOSE_DBG


        if (pFormat->ptd != NULL)
        {
            HRESULT r;
            r = TaskAllocMem(sizeof(DVTARGETDEVICE),
                    (LPVOID FAR*)&pArrayObjs[i].ptd);
            if (OK(r))
            {
                *(pArrayObjs[i].ptd) = *(pFormat->ptd);
            }
            else
            {
                return r;
            }
        }
    }

    if (pceltFetched != NULL)
    {
        *pceltFetched = celtFetched;
    }

    return ((celtFetched == celt) ? NOERROR : S_FALSE);
}

//+---------------------------------------------------------------
//
//  Member:     StdEnumFORMATETC::Skip
//
//  Synopsis:   Member of IEnumXXX interface
//
//----------------------------------------------------------------

STDMETHODIMP
StdEnumFORMATETC::Skip(ULONG celt)
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
//  Member:     StdEnumFORMATETC::Reset
//
//  Synopsis:   Member of IEnumXXX interface
//
//----------------------------------------------------------------

STDMETHODIMP
StdEnumFORMATETC::Reset(void)
{
    _cCurrent = 0;
    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     StdEnumFORMATETC::Clone
//
//  Synopsis:   Member of IEnumXXX interface
//
//----------------------------------------------------------------

STDMETHODIMP
StdEnumFORMATETC::Clone(LPENUMFORMATETC FAR* ppenm)
{
    HRESULT r = E_OUTOFMEMORY;;
    LPSTDENUMFORMATETC penum = new StdEnumFORMATETC(_pStart, _cCount);
    if (penum != NULL)
    {
        r = NOERROR;
        penum->_cCurrent = _cCurrent;
        *ppenm = penum;
    }
    else
    {
        DOUT(L"o2base/StdEnumFORMATETC::Clone failed\r\n");
    }
    return r;
}

//+---------------------------------------------------------------
//
//  Function:   CreateFORMATETCEnum, public
//
//  Synopsis:   Creates a standard enumerator over FORMATETC arrays
//
//  Arguments:  [pFormats] -- pointer to the beginning of the FORMATETC array
//              [cFormats] -- the number of elements in the array
//              [ppenum] -- where the enumerator is returned
//
//  Returns:    Success if the enumerator could be successfully created
//
//  Notes:      This function is typically used in the IDataObject::EnumFormatetc
//              method implementation.
//
//----------------------------------------------------------------

HRESULT
CreateFORMATETCEnum(LPFORMATETC pFormats,
                    ULONG cFormats,
                    LPENUMFORMATETC FAR* ppenum)
{
    HRESULT r;
    LPSTDENUMFORMATETC penum = new StdEnumFORMATETC(pFormats, cFormats);
    if (penum == NULL)
    {
#if DBG
    DOUT(L"o2base/stdenum/CreateFORMATETCEnum E_OUTOFMEMORY\r\n");
#endif
        r = E_OUTOFMEMORY;
    }
    else
    {
        *ppenum = penum;
        r = NOERROR;
    }
    return r;
}

#if 1   // this maybe useful later but is not currently used.

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
    DECLARE_STANDARD_IUNKNOWN(StdStaticEnum);

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

    DOUT(L"StdStaticEnum constructed.\r\n");
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
    DOUT(L"StdStaticEnum destructed.\r\n");
}

IMPLEMENT_STANDARD_IUNKNOWN(StdStaticEnum);

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
#if VERBOSE_DBG
    DOUT(L"StdStaticEnum::QueryInterface E_NOINTERFACE\r\n");
#endif //VERBOSE_DBG
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
#if DBG
    DOUT(L"StdStaticEnum::Next E_FAIL\r\n");
#endif
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
    LPSTDSTATICENUM penum = new StdStaticEnum(_iid,
                                            _pStart,
                                            _cSize,
                                            _cCount);
    if (penum == NULL)
    {
        DOUT(L"o2base/StdStaticEnum::Clone failed\r\n");
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
        DOUT(L"o2base/stdenum/CreateStaticEnum failed\r\n");
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

