//+---------------------------------------------------------------------------
//
//  Function:   CEnumFmtEtc
//
//  Synopsis:   Implements the IEnumFormatEtc.
//              Used by the urlmon by dataobject and CreateEnumFormatEtc
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
#include <mon.h>

PerfDbgTag(tagCEnumFmtEtc, "Urlmon", "Log CEnumFmtEtc", DEB_FORMAT);

//+---------------------------------------------------------------------------
//
//  Method:     CEnumFmtEtc::Create
//
//  Synopsis:
//
//  Arguments:  [cfmtetc] --
//              [rgfmtetc] --
//
//  Returns:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CEnumFmtEtc * CEnumFmtEtc::Create(UINT cfmtetc, FORMATETC* rgfmtetc)
{
    PerfDbgLog(tagCEnumFmtEtc, NULL, "+CEnumFmtEtc::Create");
    CEnumFmtEtc * pCEnumFEtc = NULL;

    if (cfmtetc >= 1)
    {
        // only create an enumerator if at least one element
        pCEnumFEtc = new CEnumFmtEtc();
        if (pCEnumFEtc)
        {
            if (pCEnumFEtc->Initialize(cfmtetc, rgfmtetc, 0) == FALSE)
            {
                delete pCEnumFEtc;
                pCEnumFEtc = NULL;
            }
        }
    }

    PerfDbgLog1(tagCEnumFmtEtc, NULL, "-CEnumFmtEtc::Create pEnum->(%lx)", pCEnumFEtc);
    return pCEnumFEtc;
}

//+---------------------------------------------------------------------------
//
//  Method:     CEnumFmtEtc::Initialize
//
//  Synopsis:   set the size and position
//
//  Arguments:  [cfmtetc] -- number of elements
//              [iPos] --    position of enumerator
//
//  Returns:    true on success
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL CEnumFmtEtc::Initialize(UINT cfmtetc, FORMATETC* rgfmtetc,  UINT iPos)
{
    LONG cbSize =  sizeof(FORMATETC) * cfmtetc;
    _iNext = iPos;
    _cElements = cfmtetc;

    _pFmtEtc = (FORMATETC *) new FORMATETC [cfmtetc];
    if (_pFmtEtc)
    {
        memcpy(_pFmtEtc, rgfmtetc, cbSize);
        _cElements = cfmtetc;
    }
    else
    {
        _cElements = 0;
    }

    return _cElements != 0;
}
//+---------------------------------------------------------------------------
//
//  Method:     CEnumFmtEtc::QueryInterface
//
//  Synopsis:
//
//  Arguments:  [riid] --
//              [ppvObj] --
//
//  Returns:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CEnumFmtEtc::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    PerfDbgLog(tagCEnumFmtEtc, this, "+CEnumFmtEtc::QueryInterface");
    HRESULT hr = NOERROR;

    if (   (riid == IID_IUnknown)
        || (riid == IID_IEnumFORMATETC))
    {
        *ppvObj = this;
        AddRef();
    }
    else
    {
        *ppvObj = NULL;
        hr = E_NOINTERFACE;
    }

    PerfDbgLog(tagCEnumFmtEtc, this, "-CEnumFmtEtc::QueryInterface");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CEnumFmtEtc::AddRef
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CEnumFmtEtc::AddRef(void)
{
    GEN_VDATEPTRIN(this,ULONG,0L);
    LONG lRet = ++_CRefs;
    PerfDbgLog1(tagCEnumFmtEtc, this, "CEnumFmtEtc::AddRef(%ld)", lRet);
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   CEnumFmtEtc::Release
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CEnumFmtEtc::Release(void)
{
    GEN_VDATEPTRIN(this,ULONG,0L);
    PerfDbgLog(tagCEnumFmtEtc, this, "+CEnumFmtEtc::Release");

    LONG lRet = --_CRefs;
    if (_CRefs == 0)
    {
        delete this;
    }

    PerfDbgLog1(tagCEnumFmtEtc, this, "-CEnumFmtEtc::Release(%ld)", lRet);
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     CEnumFmtEtc::Next
//
//  Synopsis:
//
//  Arguments:  [celt] --
//              [rgelt] --
//              [pceltFetched] --
//
//  Returns:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CEnumFmtEtc::Next(ULONG celt, FORMATETC * rgelt, ULONG * pceltFetched)
{
    PerfDbgLog(tagCEnumFmtEtc, this, "+CEnumFmtEtc::Next");
    HRESULT hr;

    if (!rgelt)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        for (ULONG i = 0; (i < celt) && (_iNext < _cElements); i++)
        {
            rgelt[i] = *(_pFmtEtc + _iNext++);
        }

        if (pceltFetched)
        {
            *pceltFetched = i;
        }

        hr = ((i == celt) ? NOERROR : S_FALSE);

    }

    PerfDbgLog1(tagCEnumFmtEtc, this, "-CEnumFmtEtc::Next (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CEnumFmtEtc::Skip
//
//  Synopsis:
//
//  Arguments:  [celt] --
//
//  Returns:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CEnumFmtEtc::Skip(ULONG celt)
{
    PerfDbgLog(tagCEnumFmtEtc, this, "+CEnumFmtEtc::Skip");
    HRESULT hr;

    _iNext += celt;

    if (_iNext <= _cElements)
    {
        hr = NOERROR;
    }
    else
    {
        _iNext = _cElements;
        hr = S_FALSE;
    }
    PerfDbgLog1(tagCEnumFmtEtc, this, "-CEnumFmtEtc::Skip (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CEnumFmtEtc::Reset
//
//  Synopsis:
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CEnumFmtEtc::Reset(void)
{
    PerfDbgLog(tagCEnumFmtEtc, this, "+CEnumFmtEtc::Reset");
    _iNext = 0;
    PerfDbgLog1(tagCEnumFmtEtc, this, "-CEnumFmtEtc::Reset (hr:%lx)", S_OK);
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:     CEnumFmtEtc::Clone
//
//  Synopsis:
//
//  Arguments:  [ppenum] --
//
//  Returns:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CEnumFmtEtc::Clone(IEnumFORMATETC ** ppenum)
{
    PerfDbgLog(tagCEnumFmtEtc, this, "+CEnumFmtEtc::Clone");
    HRESULT hr;
    if (ppenum)
    {
        CEnumFmtEtc * pCEnumFEtc;
        TransAssert((_cElements > 0));

        pCEnumFEtc = new CEnumFmtEtc();
        if (pCEnumFEtc)
        {
            if (pCEnumFEtc->Initialize(_cElements, _pFmtEtc, _iNext) == FALSE)
            {
                delete pCEnumFEtc;
                pCEnumFEtc = NULL;
            }
            else
            {
                *ppenum = pCEnumFEtc;
            }
        }
        hr = ((*ppenum != NULL) ? NOERROR : E_OUTOFMEMORY);

    }
    else
    {
        hr = E_INVALIDARG;
    }

    PerfDbgLog1(tagCEnumFmtEtc, this, "-CEnumFmtEtc::Clone (hr:%lx)", hr);
    return hr;
}
