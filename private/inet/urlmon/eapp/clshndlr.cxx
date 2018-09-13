

//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       cldhndlr.cxx
//
//  Contents:   Performs download of class object and its handler concurrently
//
//  Classes:
//
//  Functions:
//
//  History:    06-27-97   EricV (Eric VandenBerg)   Created
//
//----------------------------------------------------------------------------
#include <eapp.h>

PerfDbgTag(tagClassInstallFilter,    "Urlmon", "Log ClassInstallFilter",        DEB_TRANS)
    DbgTag(tagClassInstallFilterErr, "Urlmon", "Log ClassInstallFilter Errors", DEB_TRANS|DEB_ERROR)
   
//+---------------------------------------------------------------------------
//
//  Method:     CClassInstallFilterSink::CClassInstallFilterSink
//
//  Synopsis:
//
//  Arguments:  [CClassInstallFilter] --
//
//  Returns:
//
//  History:    06-27-97   EricV (Eric VandenBerg)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CClassInstallFilterSink::CClassInstallFilterSink(CClassInstallFilter *pInstallFilter)
{
    PerfDbgLog(tagClassInstallFilter, this, "+CClassInstallFilterSink::CClassInstallFilterSink\n");
    
    TransAssert(pInstallFilter);

    _pInstallFilter = pInstallFilter;

    _dwRef = 1;
    _bDone = FALSE;

    PerfDbgLog(tagClassInstallFilter, this, "+CClassInstallFilterSink::CClassInstallFilterSink (end)\n");
}

//+---------------------------------------------------------------------------
//
//  Method:     CClassInstallFilterSink::~CClassInstallFilterSink
//
//  Synopsis:
//
//  Arguments:  
//
//  Returns:
//
//  History:    06-27-97   EricV (Eric VandenBerg)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CClassInstallFilterSink::~CClassInstallFilterSink()
{
    PerfDbgLog(tagClassInstallFilter, this, "+CClassInstallFilterSink::~CClassInstallFilterSink\n");
    PerfDbgLog(tagClassInstallFilter, this, "+CClassInstallFilterSink::~CClassInstallFilterSink (end)\n");
}

//+---------------------------------------------------------------------------
//
//  Method:     CClassInstallFilterSink::QueryInterface
//
//  Synopsis:
//
//  Arguments:  [riid] --
//              [ppvObj] --
//
//  Returns:
//
//  History:    06-27-97   EricV (Eric VandenBerg)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CClassInstallFilterSink::QueryInterface(REFIID iid, void **ppvObj)
{
    PerfDbgLog(tagClassInstallFilter, this, "+CClassInstallFilterSink::QueryInterface\n");
    HRESULT hr = E_FAIL;

    if (!ppvObj)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        *ppvObj = NULL;
    
        if ((iid == IID_IOInetProtocolSink)
            || (iid == IID_IUnknown)) 
        {
            *ppvObj = (IOInetProtocolSink *) this;
            AddRef();
            hr = S_OK;   
        }
        else if (iid == IID_IServiceProvider)
        {
            *ppvObj = (IServiceProvider *) this;
            AddRef();
            hr = S_OK;
        }
        else
        {
            hr = E_NOINTERFACE;
        }
    }

    PerfDbgLog1(tagClassInstallFilter, this, "-CClassInstallFilterSink::QueryInterface (hr:%lx)\n", hr);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CClassInstallFilterSink::AddRef
//
//  Synopsis:
//
//  Arguments:  [riid] --
//              [ppvObj] --
//
//  Returns:
//
//  History:    06-27-97   EricV (Eric VandenBerg)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
ULONG CClassInstallFilterSink::AddRef(void)
{
    PerfDbgLog(tagClassInstallFilter, this, "-CClassInstallFilterSink::AddRef\n");

    {
        _dwRef++;
    }
    
    PerfDbgLog1(tagClassInstallFilter, this, "-CClassInstallFilterSink::AddRef (_dwRef=%ld)\n", _dwRef);
    return _dwRef;
}

//+---------------------------------------------------------------------------
//
//  Method:     CClassInstallFilterSink::Release
//
//  Synopsis:
//
//  Arguments:  [riid] --
//              [ppvObj] --
//
//  Returns:
//
//  History:    06-27-97   EricV (Eric VandenBerg)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
ULONG CClassInstallFilterSink::Release(void)
{
    PerfDbgLog(tagClassInstallFilter, this, "-CClassInstallFilterSink::Release\n");

    {
        _dwRef--;
        if (!_dwRef)
        {
            delete this;
        }
    }

  
    PerfDbgLog1(tagClassInstallFilter, this, "-CClassInstallFilterSink::Release (_dwRef:%ld)\n", _dwRef);
    return _dwRef;
}


//+---------------------------------------------------------------------------
//
//  Method:     CClassInstallFilterSink::Switch
//
//  Synopsis:
//
//  Arguments:  [riid] --
//              [ppvObj] --
//
//  Returns:
//
//  History:    06-27-97   EricV (Eric VandenBerg)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CClassInstallFilterSink::Switch(PROTOCOLDATA *pStateInfo)
{
    PerfDbgLog(tagClassInstallFilter, this, "+CClassInstallFilterSink::Switch\n");
    HRESULT hr = NOERROR;

    TransAssert(pStateInfo);

    if (_bDone)
    {
        hr = E_FAIL;
    }
 
    PerfDbgLog1(tagClassInstallFilter, this, "-CClassInstallFilter::Switch (hr:%lx)\n", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CClassInstallFilterSink::ReportProgress
//
//  Synopsis:
//
//  Arguments:  
//
//  Returns:
//
//  History:    06-27-97   EricV (Eric VandenBerg)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CClassInstallFilterSink::ReportProgress(ULONG ulStatusCode, LPCWSTR szStatusText)
{
    PerfDbgLog(tagClassInstallFilter, this, "+CClassInstallFilterSink::ReportProgress\n");
    HRESULT hr = NOERROR;
    ULONG ulNewStatus = 0;

    if (ulStatusCode == BINDSTATUS_BEGINDOWNLOADDATA)
    {
        ulNewStatus = BINDSTATUS_BEGINDOWNLOADCOMPONENTS;
    }
    else if (ulStatusCode == BINDSTATUS_DOWNLOADINGDATA)
    {
        ulNewStatus = BINDSTATUS_INSTALLINGCOMPONENTS;
    }
    else if (ulStatusCode == BINDSTATUS_ENDDOWNLOADCOMPONENTS)
    {
        ulNewStatus = BINDSTATUS_ENDDOWNLOADCOMPONENTS;
    }

    // report progress on this

    if (ulNewStatus && _pInstallFilter)
    {
        _pInstallFilter->ReportProgress(ulNewStatus, szStatusText);
    }

    PerfDbgLog1(tagClassInstallFilter, this, "-CClassInstallFilter::ReportProgress (hr:%lx)\n", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CClassInstallFilterSink::ReportData
//
//  Synopsis:
//
//  Arguments:  
//
//  Returns:
//
//  History:    06-27-97   EricV (Eric VandenBerg)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CClassInstallFilterSink::ReportData(DWORD grfBSCF, ULONG ulProgress, ULONG ulProgressMax)
{
    PerfDbgLog(tagClassInstallFilter, this, "+CClassInstallFilterSink::ReportData\n");
    HRESULT hr = NOERROR;

    if (_bDone)
    {
        hr = E_FAIL;
    }

    PerfDbgLog1(tagClassInstallFilter, this, "-CClassInstallFilter::ReportData (hr:%lx)\n", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CClassInstallFilterSink::ReportResult
//
//  Synopsis:
//
//  Arguments:  
//
//  Returns:
//
//  History:    06-27-97   EricV (Eric VandenBerg)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CClassInstallFilterSink::ReportResult(HRESULT hrResult, DWORD dwError, LPCWSTR wzResult)
{
    PerfDbgLog(tagClassInstallFilter, this, "+CClassInstallFilterSink::ReportResult\n");
    HRESULT hr = NOERROR;

    if (_bDone)
    {
        hr = E_FAIL;
    }
    else 
    {
        _bDone = TRUE;
     
        hr = _pInstallFilter->InstallerReportResult(hrResult, dwError, wzResult);
    }
    
    PerfDbgLog1(tagClassInstallFilter, this, "-CClassInstallFilter::ReportResult (hr:%lx)\n", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CClassInstallFilterSink::QueryService
//
//  Synopsis:
//
//  Arguments:  [rsid] --
//              [riid] --
//              [ppvObj] --
//
//  Returns:
//
//  History:    06-27-97   EricV (Eric VandenBerg)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CClassInstallFilterSink::QueryService(REFGUID rsid, REFIID riid, void ** ppvObj)
{
    PerfDbgLog(tagClassInstallFilter, this, "-CClassInstallFilterSink::QueryService\n");
    HRESULT hr = NOERROR;

    if (!_pInstallFilter)
    {
        hr = E_NOINTERFACE;
    }
    else
    {
        hr = _pInstallFilter->QueryService(rsid,riid,ppvObj);
    }

    PerfDbgLog1(tagClassInstallFilter, this, "-CClassInstallFilterSink::QueryService (hr:%xd)\n", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CClassInstallFilter::CClassInstallFilter
//
//  Synopsis:
//
//  Arguments:  
//
//  Returns:
//
//  History:    06-27-97   EricV (Eric VandenBerg)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CClassInstallFilter::CClassInstallFilter() 
{
    PerfDbgLog(tagClassInstallFilter, this, "-CClassInstallFilter::CClassInstallFilter\n");

    DllAddRef();

    _pwzCDLURL = 0;
    _pwzClsId = 0;
    _pwzMime = 0;
    _pInstallSink = 0;
    
    _bAddRef = FALSE;

    _grfBSCF = 0;
    _ulProgress = 0;
    _ulProgressMax = 0;

    _hrResult = 0;
    _dwError = 0;
    _wzResult = NULL;
    _fReportResult = FALSE;
    _pwzDocBase[0] = L'\0';
    _pSecMgr = NULL;

    _CRefs = 1;

    SetInstallState(installingNone);

    PerfDbgLog(tagClassInstallFilter, this, "-CClassInstallFilter::CClassInstallFilter (end)\n");
}

//+---------------------------------------------------------------------------
//
//  Method:     CClassInstallFilter::~CClassInstallFilter
//
//  Synopsis:
//
//  Arguments:  
//
//  Returns:
//
//  History:    06-27-97   EricV (Eric VandenBerg)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CClassInstallFilter::~CClassInstallFilter()
{
    PerfDbgLog(tagClassInstallFilter, this, "-CClassInstallFilter::~CClassInstallFilter\n");

    DllRelease();

    TransAssert((GetInstallState() == installingNone) 
             || (GetInstallState() == installingDone));
 
    if (_wzResult)
    {
        delete [] _wzResult;
    }

    if (_pwzCDLURL)
    {
        delete [] _pwzCDLURL;
    }
    
    if (_pwzClsId)
    {
        delete [] _pwzClsId;
    }
    
    if (_pwzMime)
    {
        delete [] _pwzMime;
    }

    if (_pInstallSink)
    {
        _pInstallSink->Release();
    }

    if (_pCDLnetProtocol)
    {
        _pCDLnetProtocol->Release();
        _pCDLnetProtocol = NULL;
    }

    if (_pSecMgr) {
        _pSecMgr->Release();
    }
 
    PerfDbgLog(tagClassInstallFilter, this, "-CClassInstallFilter::~CClassInstallFilter (end)\n");
}

//+---------------------------------------------------------------------------
//
//  Method:     CClassInstallFilter::QueryInterface
//
//  Synopsis:
//
//  Arguments:  
//
//  Returns:
//
//  History:    06-27-97   EricV (Eric VandenBerg)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CClassInstallFilter::QueryInterface(REFIID riid, void **ppvObj)
{
    PerfDbgLog(tagClassInstallFilter, this, "+CClassInstallFilter::QueryInterface\n");
    VDATEPTROUT(ppvObj, void *);
    VDATETHIS(this);
    HRESULT hr = NOERROR;

    *ppvObj = NULL;

    {
        if (   (riid == IID_IUnknown)
            || (riid == IID_IOInetProtocol))
        {
            *ppvObj = (IOInetProtocol *) this;
            AddRef();
        }
        else if (riid == IID_IOInetProtocolSink)
        {
            *ppvObj = (IOInetProtocolSink *) this;
            AddRef();
        }
        else if (riid == IID_IServiceProvider)
        {
            *ppvObj = (IServiceProvider *) this;
            AddRef();
        }
        else
        {
            hr = E_NOINTERFACE;
        }
    }


    PerfDbgLog1(tagClassInstallFilter, this, "-CClassInstallFilter::QueryInterface (hr:%lx)\n", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CClassInstallFilter::AddRef
//
//  Synopsis:
//
//  Arguments:  
//
//  Returns:
//
//  History:    06-27-97   EricV (Eric VandenBerg)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CClassInstallFilter::AddRef(void)
{
    PerfDbgLog(tagClassInstallFilter, this, "+CClassInstallFilter::AddRef\n");

    LONG lRet;
  
    {
        lRet = ++_CRefs;
    }
    
    PerfDbgLog1(tagClassInstallFilter, this, "-CClassInstallFilter::AddRef (cRefs:%ld)\n", lRet);
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   CClassInstallFilter::Release
//
//  Synopsis:
//
//  Arguments:  
//
//  Returns:
//
//  History:    06-27-97   EricV (Eric VandenBerg)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CClassInstallFilter::Release(void)
{
    PerfDbgLog(tagClassInstallFilter, this, "+CClassInstallFilter::Release\n");

    LONG lRet;
    {
        lRet = --_CRefs;
        if (_CRefs == 0)
        {
            delete this;
        }
    }

    PerfDbgLog1(tagClassInstallFilter, this, "-CClassInstallFilter::Release (cRefs:%ld)\n", lRet);
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     CInstallBindInfo::CInstallBindInfo
//
//  Synopsis:
//
//  Arguments:  
//
//  Returns:
//
//  History:    06-27-97   EricV (Eric VandenBerg)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CInstallBindInfo::CInstallBindInfo() 
{
    PerfDbgLog(tagClassInstallFilter, this, "-CInstallBindInfo::CClassInstallFilter\n");

    _CRefs = 1;

    PerfDbgLog(tagClassInstallFilter, this, "-CInstallBindInfo::ClassInstallFilter (end)\n");
}

//+---------------------------------------------------------------------------
//
//  Method:     CInstallBindInfo::~CInstallBindInfo
//
//  Synopsis:
//
//  Arguments:  
//
//  Returns:
//
//  History:    06-27-97   EricV (Eric VandenBerg)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CInstallBindInfo::~CInstallBindInfo()
{
    PerfDbgLog(tagClassInstallFilter, this, "-CInstallBindInfo::~CInstallBindInfo\n");
    PerfDbgLog(tagClassInstallFilter, this, "-CInstallBindInfo::~CInstallBindInfo (end)\n");
}

//+---------------------------------------------------------------------------
//
//  Method:     CInstallBindInfo::QueryInterface
//
//  Synopsis:
//
//  Arguments:  
//
//  Returns:
//
//  History:    06-27-97   EricV (Eric VandenBerg)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CInstallBindInfo::QueryInterface(REFIID riid, void **ppvObj)
{
    PerfDbgLog(tagClassInstallFilter, this, "+CInstallBindInfo::QueryInterface\n");
    VDATEPTROUT(ppvObj, void *);
    VDATETHIS(this);
    HRESULT hr = NOERROR;

    *ppvObj = NULL;

    {
        if (   (riid == IID_IUnknown)
            || (riid == IID_IOInetBindInfo))
        {
            *ppvObj = (IOInetBindInfo *) this;
            AddRef();
        }
        else
        {
            hr = E_NOINTERFACE;
        }
    }


    PerfDbgLog1(tagClassInstallFilter, this, "-CInstallBindInfo::QueryInterface (hr:%lx)\n", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CInstallBindInfo::AddRef
//
//  Synopsis:
//
//  Arguments:  
//
//  Returns:
//
//  History:    06-27-97   EricV (Eric VandenBerg)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CInstallBindInfo::AddRef(void)
{
    PerfDbgLog(tagClassInstallFilter, this, "+CInstallBindInfo::AddRef\n");

    LONG lRet;
  
    {
        lRet = ++_CRefs;
    }
    
    PerfDbgLog1(tagClassInstallFilter, this, "-CInstallBindInfo::AddRef (cRefs:%ld)\n", lRet);
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   CInstallBindInfo::Release
//
//  Synopsis:
//
//  Arguments:  
//
//  Returns:
//
//  History:    06-27-97   EricV (Eric VandenBerg)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CInstallBindInfo::Release(void)
{
    PerfDbgLog(tagClassInstallFilter, this, "+CInstallBindInfo::Release\n");

    LONG lRet;
    {
        lRet = --_CRefs;
        if (_CRefs == 0)
        {
            delete this;
        }
    }

    PerfDbgLog1(tagClassInstallFilter, this, "-CInstallBindInfo::Release (cRefs:%ld)\n", lRet);
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     CInstallBindInfo::GetBindInfo
//
//  Synopsis:
//
//  Arguments:  [grfBINDF] --
//              [pbindinfo] --
//
//  Returns:
//
//  History:    06-27-97   EricV (Eric VandenBerg)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CInstallBindInfo::GetBindInfo(
                                    DWORD *grfBINDF, BINDINFO * pbindinfo)
{
    PerfDbgLog(tagClassInstallFilter, this, "+CInstallBindInfo::GetBindInfo\n");
    HRESULT hr = NOERROR;

    TransAssert(pbindinfo);
    if (!grfBINDF || !pbindinfo || !pbindinfo->cbSize) 
    {
        hr = E_INVALIDARG;
    }
    else
    {
        DWORD cbSize = pbindinfo->cbSize;
        memset(pbindinfo, 0, cbSize);
        pbindinfo->cbSize = cbSize;
    }

    PerfDbgLog1(tagClassInstallFilter, this, "+CInstallBindInfo::GetBindInfo (hr:%lx)\n",hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CInstallBindInfo::GetBindString
//
//  Synopsis:
//
//  Arguments:  [ulStringType] --
//              [ppwzStr] --
//              [cEl] --
//              [pcElFetched] --
//
//  Returns:
//
//  History:    06-27-97   EricV (Eric VandenBerg)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CInstallBindInfo::GetBindString(
                    ULONG ulStringType, LPOLESTR *ppwzStr, ULONG cEl, ULONG *pcElFetched)
{
    PerfDbgLog(tagClassInstallFilter, this, "+CInstallBindInfo::GetBindString\n");
    HRESULT hr = NOERROR;

    TransAssert(ppwzStr);
    TransAssert(pcElFetched);

    if (ppwzStr && pcElFetched)
    {
        *ppwzStr = NULL;
        *pcElFetched = 0;
    }

    hr = S_FALSE;

    PerfDbgLog1(tagClassInstallFilter, this, "+CInstallBindInfo::GetBindString (hr:%lx)\n",hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CClassInstallFilter::Start
//
//  Synopsis:
//
//  Arguments:  [pwzUrl] --
//              [pTrans] --
//              [pOIBindInfo] --
//              [grfSTI] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    06-27-97   EricV (Eric VandenBerg)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CClassInstallFilter::Start(LPCWSTR pwzUrl, 
                          IOInetProtocolSink *pOInetProtSnk, 
                          IOInetBindInfo *pOIBindInfo,
                          DWORD grfSTI, 
                          DWORD_PTR dwReserved)
{
    PerfDbgLog(tagClassInstallFilter, this, "+CClassInstallFilter::Start\n");
    HRESULT hr = NOERROR;
    IOInetSession *pOInetSession = NULL;
    PROTOCOLFILTERDATA *pFilterData = (PROTOCOLFILTERDATA *)dwReserved;
    static LPCWSTR pwzCDLFormatCDL = L"cdl:";
    static LPCWSTR pwzCDLFormatCodebase = L"codebase=";
    static LPCWSTR pwzCDLFormatClsId = L"clsid=";
    static LPCWSTR pwzCDLFormatMime = L"mimetype=";
    static LPCWSTR pwzCDLFormatVerMS = L";verMS=";
    static LPCWSTR pwzCDLFormatVerLS = L";verLS=";
    static LPCWSTR pwzCDLFormatSeparator = L";";
    IOInetBindInfo *pBindInfo = NULL;
    LPWSTR pwzTmp = NULL;
    LPWSTR pwzVerInfo = NULL, pwzVerMS = NULL, pwzVerLS = NULL;
    DWORD dwSize;
    WCHAR chType = L'\0';

    TransAssert((pOIBindInfo && pOInetProtSnk && pwzUrl));
    TransAssert(!_pCDLnetProtocol);
    
    //BUGBUG:
    // we engage in some trickery to get the URL & class ID.  The URL string
    // passed to us is actually "Class Install Handler", but after the NULL
    // terminator we expect the docbase the show up. After this, the real URL
    // info to occur.  This is because the pwzURL
    // is used by LoadHandler to find us in the registry and then calls us with it.

    // Get the docbase
    while (*pwzUrl)
    {
        pwzUrl++;
    }
    pwzUrl++;
    
    lstrcpyW(_pwzDocBase, pwzUrl);

    // Now get the cdl:// codebase

    while (*pwzUrl)
    {
        pwzUrl++;
    }
    pwzUrl++;

    if (pOInetProtSnk && pwzUrl && *pwzUrl) 
    {
        TransAssert(pwzUrl);
   
        //
        // expect url:   codebase?<type>[clsid|mimetype]?<verMS>,<verLS>
        // example:      http://msw/officehandler.cab?CABCD1234-...?1,2
        //

        delete [] _pwzUrl;
        pwzTmp = _pwzUrl = OLESTRDuplicate((LPCWSTR)pwzUrl);

        while (*pwzTmp && *pwzTmp != L'?')
        {
            pwzTmp++;
        }

        if (*pwzTmp)
        {
            *pwzTmp = L'\0';
            pwzTmp++;

            delete [] _pwzClsId;
            
            // extract version info.
            pwzVerInfo = pwzTmp;
            while (*pwzVerInfo && *pwzVerInfo != L'?')
            {
                pwzVerInfo++;
            }

            if (*pwzVerInfo == L'?')
            {
                pwzVerMS = pwzVerInfo + 1;
                pwzVerLS = pwzVerMS;

                while (*pwzVerLS && *pwzVerLS != L',')
                {
                    pwzVerLS++;
                }
                if (*pwzVerLS == L',')
                {
                    *pwzVerLS = L'\0';
                    pwzVerLS++;        
                }

                *pwzVerInfo = '\0';
            }
            else
            {
                pwzVerInfo = NULL;
            }

            if (*pwzTmp == L'{')
            {
                // handle CLSID string
                _pwzClsId = OLESTRDuplicate((LPCWSTR)pwzTmp);
                _pwzMime = NULL;
            }
            else
            {
                _pwzMime = OLESTRDuplicate((LPCWSTR)pwzTmp);
                _pwzClsId = NULL;
            }

            // compose cdl:// string
            dwSize = lstrlenW(pwzCDLFormatCDL) 
                    + (_pwzUrl ? lstrlenW(pwzCDLFormatCodebase) 
                                + lstrlenW(_pwzUrl)
                                + lstrlenW(pwzCDLFormatSeparator) : 0)
                    + (_pwzClsId ? lstrlenW(pwzCDLFormatClsId)
                                + lstrlenW(_pwzClsId) : 0)
                    + (_pwzMime ? lstrlenW(pwzCDLFormatMime) 
                                + lstrlenW(_pwzMime) : 0)
                    + (pwzVerMS ? lstrlenW(pwzCDLFormatVerMS)
                                + lstrlenW(pwzVerMS) : 0)
                    + (pwzVerLS ? lstrlenW(pwzCDLFormatVerLS)
                                + lstrlenW(pwzVerLS) : 0)
                    + 3;

            delete [] _pwzCDLURL;
            _pwzCDLURL = new WCHAR[dwSize];

            if (_pwzCDLURL)
            {
                StrCpyW(_pwzCDLURL, pwzCDLFormatCDL);
                
                // codebase is optional, but one of clsid or mimetype must exist.
                if (_pwzUrl)
                {
                    StrCatW(_pwzCDLURL, pwzCDLFormatCodebase);
                    StrCatW(_pwzCDLURL, _pwzUrl);
                    StrCatW(_pwzCDLURL, pwzCDLFormatSeparator);
                }
                
                if (_pwzClsId)
                {
                    StrCatW(_pwzCDLURL, pwzCDLFormatClsId);
                    StrCatW(_pwzCDLURL, _pwzClsId);
                }
                else if (_pwzMime)
                {
                    StrCatW(_pwzCDLURL, pwzCDLFormatMime);
                    StrCatW(_pwzCDLURL, _pwzMime);
                }
                else
                {
                    hr = E_UNEXPECTED;
                }

                if (pwzVerMS)
                {
                    StrCatW(_pwzCDLURL, pwzCDLFormatVerMS);
                    StrCatW(_pwzCDLURL, pwzVerMS);
                }
                if (pwzVerLS)
                {
                    StrCatW(_pwzCDLURL, pwzCDLFormatVerLS);
                    StrCatW(_pwzCDLURL, pwzVerLS);
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

        }
        else
        {
            hr = E_INVALIDARG;
        }

        if (SUCCEEDED(hr) && _pwzCDLURL && pFilterData && pFilterData->pProtocol)
        {
            _pProt = pFilterData->pProtocol;
            _pProt->AddRef();

            _pProtSnk = pOInetProtSnk;
            _pProtSnk->AddRef();

            // Pass our IInternetProtocolSink out
            pFilterData->pProtocolSink = (IOInetProtocolSink *)this;

            hr = CoInternetGetSession(0, &pOInetSession, 0);

            if (SUCCEEDED(hr))
            {
                hr = pOInetSession->CreateBinding(NULL, _pwzCDLURL, NULL, NULL, 
                                         (IOInetProtocol **)&_pCDLnetProtocol, 
                                         OIBDG_APARTMENTTHREADED);
                pOInetSession->Release();

                if (SUCCEEDED(hr) && _pCDLnetProtocol)
                {
                    _pInstallSink = new CClassInstallFilterSink(this);
                    
                    if (_pInstallSink)
                    {

                        //hr = _CBindInfo.QueryInterface(IID_IOInetBindInfo, (void **)&pBindInfo);

                        pBindInfo = new CInstallBindInfo();

                        if (!pBindInfo) {
                            hr = E_OUTOFMEMORY;
                        }
        
                        if (SUCCEEDED(hr) && pBindInfo) 
                        {
                            hr = _pCDLnetProtocol->Start(_pwzCDLURL, 
                                    (IOInetProtocolSink *)_pInstallSink, 
                                    (IOInetBindInfo *)pBindInfo,
                                    PI_FORCE_ASYNC | PI_APARTMENTTHREADED, 0);
                            
                            // We add reference ourself so that if we are terminated before
                            // the cdl:// download we don't get deleted.

                            pBindInfo->Release();
                            pBindInfo = NULL;

                            if (hr == E_PENDING) {

                                AddRef();
                                _bAddRef = TRUE;

                                SetInstallState(installingHandler);
                            }
                        }

                    }
                    else 
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
            }
        }
        else
        {
            hr = E_INVALIDARG;
        }

    }
    else
    {
        hr = E_FAIL;
    }

    // BUGBUG: when do we release if E_PENDING?
    if (hr != E_PENDING)
    {
        SetInstallState(installingDone);

        if (_pCDLnetProtocol)
        {
            _pCDLnetProtocol->Release();
            _pCDLnetProtocol = NULL;
        }

    }

    PerfDbgLog1(tagClassInstallFilter, this, "+CClassInstallFilter::Start (hr:%lx)\n",hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CClassInstallFilter::Continue
//
//  Synopsis:
//
//  Arguments:  [pStateInfoIn] --
//
//  Returns:
//
//  History:    06-27-97   EricV (Eric VandenBerg)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CClassInstallFilter::Continue(PROTOCOLDATA *pStateInfoIn)
{
    PerfDbgLog(tagClassInstallFilter, this, "+CClassInstallFilter::Continue\n");

    HRESULT hr = NOERROR;

    if (_pProt)
    {
         hr = _pProt->Continue(pStateInfoIn);
    }

    PerfDbgLog1(tagClassInstallFilter, this, "-CClassInstallFilter::Continue (hr:%lx)\n",hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CClassInstallFilter::Abort
//
//  Synopsis:
//
//  Arguments:  [hrReason] --
//              [dwOptions] --
//
//  Returns:
//
//  History:    06-27-97   EricV (Eric VandenBerg)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CClassInstallFilter::Abort(HRESULT hrReason, DWORD dwOptions)
{
    PerfDbgLog(tagClassInstallFilter, this, "+CClassInstallFilter::Abort\n");
    HRESULT hr = NOERROR;

    if (_pCDLnetProtocol)
    { 
        hr = _pCDLnetProtocol->Abort(hrReason, dwOptions);
    }

    if (_pProt)
    {
        hr = _pProt->Abort(hrReason, dwOptions);        
    }

    // Release sink
    if (_pProtSnk)
    {
        _pProtSnk->Release();
        _pProtSnk = NULL;
    }

    PerfDbgLog1(tagClassInstallFilter, this, "-CClassInstallFilter::Abort (hr:%lx)\n", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CClassInstallFilter::Terminate
//
//  Synopsis:
//
//  Arguments:  [dwOptions] --
//
//  Returns:
//
//  History:    06-27-97   EricV (Eric VandenBerg)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CClassInstallFilter::Terminate(DWORD dwOptions)
{
    PerfDbgLog(tagClassInstallFilter, this, "+CClassInstallFilter::Terminate\n");
    HRESULT hr = NOERROR;

    TransAssert((_pProt));

    if (_pCDLnetProtocol)
    {
        hr = _pCDLnetProtocol->Terminate(dwOptions);
    }

    if (_pProt)
    {
        hr = _pProt->Terminate(dwOptions);
    }

    // Release sink 
    if (_pProtSnk)
    {
        _pProtSnk->Release();
        _pProtSnk = NULL;
    }

    PerfDbgLog1(tagClassInstallFilter, this, "-CClassInstallFilter::Terminate (hr:%lx)\n", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CClassInstallFilter::Suspend
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    06-27-97   EricV (Eric VandenBerg)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CClassInstallFilter::Suspend()
{
    PerfDbgLog(tagClassInstallFilter, this, "+CClassInstallFilter::Suspend\n");
    HRESULT hr = NOERROR;
    
    if (_pProt) 
    {
         hr = _pProt->Suspend();
    }

    PerfDbgLog1(tagClassInstallFilter, this, "-CClassInstallFilter::Suspend (hr:%lx)\n", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CClassInstallFilter::Resume
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    06-27-97   EricV (Eric VandenBerg)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CClassInstallFilter::Resume()
{
    PerfDbgLog(tagClassInstallFilter, this, "+CClassInstallFilter::Resume\n");
    HRESULT hr = NOERROR;

    if (_pProt)
    {
        hr = _pProt->Resume();
    }

    PerfDbgLog1(tagClassInstallFilter, this, "-CClassInstallFilter::Resume (hr:%lx)\n", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CClassInstallFilter::Read
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    06-27-97   EricV (Eric VandenBerg)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CClassInstallFilter::Read(void *pv,ULONG cb,ULONG *pcbRead)
{
    PerfDbgLog(tagClassInstallFilter, this, "+CClassInstallFilter::Read\n");
    HRESULT hr = NOERROR;

    if (_pProt)
    {
        hr = _pProt->Read(pv,cb,pcbRead);
    }
 
    PerfDbgLog1(tagClassInstallFilter, this, "-CClassInstallFilter::Read (hr:%lx)\n", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CClassInstallFilter::Seek
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    06-27-97   EricV (Eric VandenBerg)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

STDMETHODIMP CClassInstallFilter::Seek(LARGE_INTEGER dlibMove,DWORD dwOrigin,
                      ULARGE_INTEGER *plibNewPosition)
{
    PerfDbgLog(tagClassInstallFilter, this, "+CClassInstallFilter::Seek\n");
    HRESULT hr = NOERROR;

    if (_pProt)
    {
        hr = _pProt->Seek(dlibMove, dwOrigin, plibNewPosition);
    } 

    PerfDbgLog1(tagClassInstallFilter, this, "-CClassInstallFilter::Seek (hr:%lx)\n", hr);
    return hr;

}

//+---------------------------------------------------------------------------
//
//  Method:     CClassInstallFilter::LockRequest
//
//  Synopsis:
//
//  Arguments:  [dwOptions] --
//
//  Returns:
//
//  History:    06-27-97   EricV (Eric VandenBerg)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CClassInstallFilter::LockRequest(DWORD dwOptions)
{
    PerfDbgLog(tagClassInstallFilter, this, "+CClassInstallFilter::LockRequest\n");

    HRESULT hr = NOERROR;

    if (_pProt)
    {    
        hr = _pProt->LockRequest(dwOptions);
    }

    PerfDbgLog1(tagClassInstallFilter, this, "-CClassInstallFilter::LockRequest (hr:%lx)\n",hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CClassInstallFilter::UnlockRequest
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    06-27-97   EricV (Eric VandenBerg)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CClassInstallFilter::UnlockRequest()
{
    PerfDbgLog(tagClassInstallFilter, this, "+CClassInstallFilter::UnlockRequest\n");
    HRESULT hr = NOERROR;

    if (_pProt)
    {
         hr = _pProt->UnlockRequest();
    }

    PerfDbgLog1(tagClassInstallFilter, this, "-CClassInstallFilter::UnlockRequest (hr:%lx)\n", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CClassInstallFilter::Switch
//
//  Synopsis:
//
//  Arguments:  [pStateInfo] --
//
//  Returns:
//
//  History:    06-27-97   EricV (Eric VandenBerg)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CClassInstallFilter::Switch(PROTOCOLDATA *pStateInfo)
{
    PerfDbgLog(tagClassInstallFilter, this, "+CClassInstallFilter::Switch\n");
    HRESULT hr = NOERROR;

    if (_pProtSnk)
    {
        hr = _pProtSnk->Switch(pStateInfo);
    }
 
    PerfDbgLog1(tagClassInstallFilter, this, "-CClassInstallFilter::Switch (hr:%lx)\n", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CClassInstallFilter::ReportProgress
//
//  Synopsis:
//
//  Arguments:  [NotMsg] --
//              [szStatusText] --
//
//  Returns:
//
//  History:    06-27-97   EricV (Eric VandenBerg)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CClassInstallFilter::ReportProgress(ULONG NotMsg, LPCWSTR pwzStatusText)
{
    PerfDbgLog(tagClassInstallFilter, this, "+CClassInstallFilter::ReportProgress\n");
    HRESULT hr = NOERROR;

    if (_pProtSnk)
    {
        hr = _pProtSnk->ReportProgress(NotMsg, pwzStatusText);
    }

    PerfDbgLog1(tagClassInstallFilter, this, "-CClassInstallFilter::ReportProgress (hr:%lx)\n", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CClassInstallFilter::ReportData
//
//  Synopsis:
//
//  Arguments:  [grfBSCF] --
//              [ULONG] --
//              [ulProgressMax] --
//
//  Returns:
//
//  History:    06-27-97   EricV (Eric VandenBerg)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CClassInstallFilter::ReportData(DWORD grfBSCF, ULONG ulProgress, ULONG ulProgressMax)
{
    PerfDbgLog3(tagClassInstallFilter, this, "+CClassInstallFilter::ReportData(grfBSCF:%lx, ulProgress:%ld, ulProgressMax:%ld)\n",
                                       grfBSCF, ulProgress, ulProgressMax);
    HRESULT hr = NOERROR;

    _grfBSCF = grfBSCF;
    _ulProgress = ulProgress;
    _ulProgressMax = ulProgressMax;

    if (_pProtSnk)
    {
        hr = _pProtSnk->ReportData( grfBSCF, ulProgress, ulProgressMax);
    }

    PerfDbgLog1(tagClassInstallFilter, this, "-CClassInstallFilter::ReportData (hr:%lx)\n", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CClassInstallFilter::ReportResult
//
//  Synopsis:
//
//  Arguments:  [DWORD] --
//              [dwError] --
//              [wzResult] --
//
//  Returns:
//
//  History:    06-27-97   EricV (Eric VandenBerg)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CClassInstallFilter::ReportResult(HRESULT hrResult, DWORD dwError, LPCWSTR wzResult)
{
    PerfDbgLog(tagClassInstallFilter, this, "+CClassInstallFilter::ReportResult\n");
    HRESULT hr = NOERROR;

    // record any failure, unless overwriting previous failure
    if (FAILED(hrResult) && SUCCEEDED(_hrResult) ) {

        _hrResult = hrResult;
        _dwError = dwError;
        _wzResult = NULL;

        if (_wzResult)
        {
            _wzResult = new WCHAR[lstrlenW(wzResult)+1];
            StrCpyW(_wzResult, wzResult);
        }
    }


    if (GetInstallState() != installingDone)
    {

        // data (docfile) completed download

        _fReportResult = TRUE;
    } 
    else if (_fReportResult)
    {
        // all complete
        if (_pProtSnk)
        {
            // always report recorded results
            hr = _pProtSnk->ReportResult(_hrResult, _dwError, _wzResult);

        }

    } else {

        // data (docfile) completed download
        _fReportResult = TRUE;
    }

    PerfDbgLog1(tagClassInstallFilter, this, "-CClassInstallFilter::ReportResult (hr:%lx)\n", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CClassInstallFilter::CClassInstallFilter
//
//  Synopsis:
//
//  Arguments:  [DWORD] --
//              [dwError] --
//              [wzResult] --
//
//  Returns:
//
//  History:    06-27-97   EricV (Eric VandenBerg)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CClassInstallFilter::InstallerReportResult(HRESULT hrResult, DWORD dwError, LPCWSTR wzResult)
{
    HRESULT hr = NOERROR;

    SetInstallState(installingDone);

    if (_pProtSnk)
    {
        // tell sink to stop waiting on handler to install
        _pProtSnk->ReportProgress(BINDSTATUS_ENDDOWNLOADCOMPONENTS,NULL);

        // repeat last report data to kick start binding operation
        _pProtSnk->ReportData(_grfBSCF, _ulProgress, _ulProgressMax);
    }


    ReportResult(hrResult, dwError, wzResult);

    if (_bAddRef)
    {
        _bAddRef = FALSE;
        Release();
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CClassInstallFilter::QueryService
//
//  Synopsis:
//
//  Arguments:  [rsid] --
//              [riid] --
//              [ppvObj] --
//
//  Returns:
//
//  History:    06-27-97   EricV (Eric VandenBerg)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CClassInstallFilter::QueryService(REFGUID rsid, REFIID riid, void ** ppvObj)
{
    HRESULT     hr = NOERROR;
    IServiceProvider        *pIServiceProvider = NULL;

    EProtAssert(ppvObj);
    if (!ppvObj)
        return E_INVALIDARG;

    *ppvObj = 0;

    if (IsEqualGUID(rsid, IID_IInternetHostSecurityManager) &&
        IsEqualGUID(riid, IID_IInternetHostSecurityManager)) {

        if (_pSecMgr == NULL) {
            hr = CoInternetCreateSecurityManager(NULL, &_pSecMgr, NULL);
        }
        
        if (_pSecMgr) {
            *ppvObj = (IInternetHostSecurityManager *)this;
            AddRef();
        }
        else {
            hr = E_NOINTERFACE;
        }
    }
    else {
        hr = _pProtSnk->QueryInterface(IID_IServiceProvider,
                                                 (LPVOID *)&pIServiceProvider);
    
        if (SUCCEEDED(hr))
        {
            hr = pIServiceProvider->QueryService(rsid, riid, (LPVOID *)ppvObj);
            pIServiceProvider->Release();
        }
    }

    return hr;
}

// IInternetHostSecurityManager
STDMETHODIMP CClassInstallFilter::GetSecurityId(BYTE *pbSecurityId, DWORD *pcbSecurityId,
                                                DWORD_PTR dwReserved)
{
    HRESULT                    hr = E_FAIL;

    if (_pSecMgr) {
        hr = _pSecMgr->GetSecurityId(_pwzDocBase, pbSecurityId,
                                     pcbSecurityId, dwReserved);
    }

    return hr;
}

STDMETHODIMP CClassInstallFilter::ProcessUrlAction(DWORD dwAction, BYTE *pPolicy, DWORD cbPolicy,
                                                   BYTE *pContext, DWORD cbContext, DWORD dwFlags,
                                                   DWORD dwReserved)
{
    HRESULT                    hr = E_FAIL;

    if (_pSecMgr) {
        hr = _pSecMgr->ProcessUrlAction(_pwzDocBase, dwAction, pPolicy,
                                        cbPolicy, pContext, cbContext,
                                        dwFlags, dwReserved);
    }

    return hr;
}

STDMETHODIMP CClassInstallFilter::QueryCustomPolicy(REFGUID guidKey, BYTE **ppPolicy,
                                                    DWORD *pcbPolicy, BYTE *pContext,
                                                    DWORD cbContext, DWORD dwReserved)
{
    HRESULT                    hr = E_FAIL;

    if (_pSecMgr) {
        hr = _pSecMgr->QueryCustomPolicy(_pwzDocBase, guidKey, ppPolicy,
                                         pcbPolicy, pContext, cbContext,
                                         dwReserved);
    }

    return hr;
}
