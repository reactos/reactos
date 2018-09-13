//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       protbase.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    11-07-1996   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <eapp.h>

//+---------------------------------------------------------------------------
//
//  Method:     CBaseProtocol::QueryInterface
//
//  Synopsis:
//
//  Arguments:  [riid] --
//              [ppvObj] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBaseProtocol::QueryInterface(REFIID riid, void **ppvObj)
{
    VDATEPTROUT(ppvObj, void *);
    VDATETHIS(this);
    HRESULT hr = NOERROR;

    EProtDebugOut((DEB_PLUGPROT, "%p _IN CBaseProtocol::QueryInterface\n", this));

    hr = _pUnkOuter->QueryInterface(riid, ppvObj);

    EProtDebugOut((DEB_PLUGPROT, "%p OUT CBaseProtocol::QueryInterface (hr:%lx\n", this,hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CBaseProtocol::AddRef
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CBaseProtocol::AddRef(void)
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CBaseProtocol::AddRef\n", this));

    LONG lRet = _pUnkOuter->AddRef();

    EProtDebugOut((DEB_PLUGPROT, "%p OUT CBaseProtocol::AddRef (cRefs:%ld)\n", this,lRet));
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   CBaseProtocol::Release
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CBaseProtocol::Release(void)
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CBaseProtocol::Release\n", this));

    LONG lRet = _pUnkOuter->Release();

    EProtDebugOut((DEB_PLUGPROT, "%p OUT CBaseProtocol::Release (cRefs:%ld)\n",this,lRet));
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBaseProtocol::Start
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
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBaseProtocol::Start(LPCWSTR pwzUrl, IOInetProtocolSink *pTrans, IOInetBindInfo *pOIBindInfo,
                          DWORD grfSTI, DWORD_PTR dwReserved)
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CBaseProtocol::Start\n", this));
    HRESULT hr = NOERROR;

    EProtAssert((!_pProtSink && pOIBindInfo && pTrans));
    EProtAssert((_pwzUrl == NULL));


    if ( !(grfSTI & PI_PARSE_URL))
    {
        _pProtSink = pTrans;
        _pProtSink->AddRef();

        _pOIBindInfo =  pOIBindInfo;
        _pOIBindInfo->AddRef();
    }

    _BndInfo.cbSize = sizeof(BINDINFO);
    hr = pOIBindInfo->GetBindInfo(&_grfBindF, &_BndInfo);

    // Do we need to append the extra data to the url?
    if (_BndInfo.szExtraInfo)
    {
        // append extra info at the end of the url
        // Make sure we don't overflow the URL
        if (wcslen(_BndInfo.szExtraInfo) + wcslen(pwzUrl) >= MAX_URL_SIZE)
        {
            hr = E_INVALIDARG;
            goto End;
        }

        wcscpy(_wzFullURL, pwzUrl);

        // Append the extra data to the url.  Note that we have already
        // checked for overflow, so we need not worry about it here.
        wcscat(_wzFullURL + wcslen(_wzFullURL), _BndInfo.szExtraInfo);
    }
    else
    {
        // Make sure we don't overflow the URL
        if (wcslen(pwzUrl) + 1 > MAX_URL_SIZE)
        {
            hr = E_INVALIDARG;
            goto End;
        }
        wcscpy(_wzFullURL, pwzUrl);
    }

    if ( !(grfSTI & PI_PARSE_URL))
    {
        // save the URL
        _pwzUrl = OLESTRDuplicate((LPCWSTR)pwzUrl);
    }

    _grfSTI = grfSTI;

End:

    EProtDebugOut((DEB_PLUGPROT, "%p OUT CBaseProtocol::Start (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBaseProtocol::Continue
//
//  Synopsis:
//
//  Arguments:  [pStateInfoIn] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBaseProtocol::Continue(PROTOCOLDATA *pStateInfoIn)
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CBaseProtocol::Continue\n", this));

    HRESULT hr = E_FAIL;

    EProtDebugOut((DEB_PLUGPROT, "%p OUT CBaseProtocol::Continue (hr:%lx)\n",this, hr));
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CBaseProtocol::Abort
//
//  Synopsis:
//
//  Arguments:  [hrReason] --
//              [dwOptions] --
//
//  Returns:
//
//  History:    11-09-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBaseProtocol::Abort(HRESULT hrReason, DWORD dwOptions)
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CBaseProtocol::Abort\n", this));
    HRESULT hr = NOERROR;

    EProtAssert((_pProtSink));

    hr = _pProtSink->ReportResult(E_ABORT, 0, 0);

    EProtDebugOut((DEB_PLUGPROT, "%p OUT CBaseProtocol::Abort (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBaseProtocol::Terminate
//
//  Synopsis:
//
//  Arguments:  [dwOptions] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBaseProtocol::Terminate(DWORD dwOptions)
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CBaseProtocol::Terminate\n", this));
    HRESULT hr = NOERROR;

    EProtAssert((_pProtSink));

    if (_pProt)
    {
        _pProt->Terminate(dwOptions);
        _pProt->Release();
        _pProt = NULL;
    }

    if (_pOIBindInfo)
    {
        _pOIBindInfo->Release();
        _pOIBindInfo = NULL;
    }
    if (_pProtSink)
    {
        _pProtSink->Release();
        _pProtSink = NULL;
    }

#if DBG == 1
    if ( _BndInfo.stgmedData.tymed != TYMED_NULL )
        EProtDebugOut((DEB_PLUGPROT, "%p --- CBaseProtocol::Terminate ReleaseStgMedium (%p)\n", this,_BndInfo.stgmedData));
#endif

    ReleaseBindInfo(&_BndInfo);

    EProtDebugOut((DEB_PLUGPROT, "%p OUT CBaseProtocol::Terminate (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBaseProtocol::Suspend
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBaseProtocol::Suspend()
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CBaseProtocol::Suspend\n", this));

    HRESULT hr = E_NOTIMPL;

    EProtDebugOut((DEB_PLUGPROT, "%p OUT CBaseProtocol::Suspend (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBaseProtocol::Resume
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBaseProtocol::Resume()
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CBaseProtocol::Resume\n", this));

    HRESULT hr = E_NOTIMPL;

    EProtDebugOut((DEB_PLUGPROT, "%p OUT CBaseProtocol::Resume (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBaseProtocol::SetPriority
//
//  Synopsis:
//
//  Arguments:  [nPriority] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBaseProtocol::SetPriority(LONG nPriority)
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CBaseProtocol::SetPriority\n", this));

    HRESULT hr = E_NOTIMPL;

    EProtDebugOut((DEB_PLUGPROT, "%p OUT CBaseProtocol::SetPriority (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBaseProtocol::GetPriority
//
//  Synopsis:
//
//  Arguments:  [pnPriority] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBaseProtocol::GetPriority(LONG * pnPriority)
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CBaseProtocol::GetPriority\n", this));

    HRESULT hr = E_NOTIMPL;

    EProtDebugOut((DEB_PLUGPROT, "%p OUT CBaseProtocol::GetPriority (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBaseProtocol::Read
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//              [ULONG] --
//              [pcbRead] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBaseProtocol::Read(void *pv,ULONG cb,ULONG *pcbRead)
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CBaseProtocol::Read\n", this));

    HRESULT hr = E_FAIL;

    EProtDebugOut((DEB_PLUGPROT, "%p OUT CBaseProtocol::Read (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBaseProtocol::Seek
//
//  Synopsis:
//
//  Arguments:  [DWORD] --
//              [ULARGE_INTEGER] --
//              [plibNewPosition] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBaseProtocol::Seek(LARGE_INTEGER dlibMove,DWORD dwOrigin,ULARGE_INTEGER *plibNewPosition)
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CBaseProtocol::Seek\n", this));

    HRESULT hr = E_FAIL;

    EProtDebugOut((DEB_PLUGPROT, "%p OUT CBaseProtocol::Seek (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBaseProtocol::LockRequest
//
//  Synopsis:
//
//  Arguments:  [dwOptions] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBaseProtocol::LockRequest(DWORD dwOptions)
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CBaseProtocol::LockRequest\n", this));

    HRESULT hr = NOERROR;

    EProtDebugOut((DEB_PLUGPROT, "%p OUT CBaseProtocol::LockRequest (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBaseProtocol::UnlockRequest
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBaseProtocol::UnlockRequest()
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CBaseProtocol::UnlockRequest\n", this));
    HRESULT hr = NOERROR;

    CloseTempFile();

    EProtDebugOut((DEB_PLUGPROT, "%p OUT CBaseProtocol::UnlockRequest (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBaseProtocol::Prepare
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    11-07-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBaseProtocol::Prepare()
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CBaseProtocol::Prepare\n", this));
    HRESULT hr = NOERROR;

    EProtAssert((  IsApartmentThread() ));


    EProtDebugOut((DEB_PLUGPROT,"%p OUT CBaseProtocol::Prepare (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBaseProtocol::Continue
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    11-09-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBaseProtocol::Continue()
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CBaseProtocol::Continue\n", this));
    HRESULT hr = NOERROR;

    EProtAssert((  !IsApartmentThread() ));

    _dwThreadID = GetCurrentThreadId();

    EProtDebugOut((DEB_PLUGPROT,"%p OUT CBaseProtocol::Continue (hr:%lx)\n",this, hr));
    return hr;
}

STDMETHODIMP CBaseProtocol::QueryService(REFGUID rsid, REFIID riid, void ** ppvObj)
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CBaseProtocol::QueryService \n", this));
    HRESULT     hr = NOERROR;
    VDATETHIS(this);
    EProtAssert((ppvObj));

    *ppvObj = 0;

    if (riid == IID_IHttpNegotiate)
    {
        IServiceProvider *pServProv = 0;

        if ((hr = _pUnkInner->QueryInterface(IID_IServiceProvider, (void **)&pServProv)) == NOERROR)
        {
            // hand back oo
            if ((hr = pServProv->QueryService(rsid, riid, ppvObj)) == NOERROR)
            {
                _pHttpNeg = new CHttpNegotiate((IHttpNegotiate *)*ppvObj);
                if (_pHttpNeg)
                {
                    *ppvObj = _pHttpNeg;
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }

            pServProv->Release();
        }

    }
    else
    {
        IServiceProvider *pServProv = 0;

        if ((hr = _pUnkInner->QueryInterface(IID_IServiceProvider, (void **)&pServProv)) == NOERROR)
        {
            hr = pServProv->QueryService(rsid, riid, ppvObj);

            pServProv->Release();
        }
    }

    EProtAssert(( (hr == E_NOINTERFACE) || ((hr == NOERROR) && *ppvObj) ));

    EProtDebugOut((DEB_PLUGPROT, "%p OUT CBaseProtocol::QueryService (hr:%lx) \n", this, hr));
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CBaseProtocol::CBaseProtocol
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CBaseProtocol::CBaseProtocol(REFCLSID rclsid, IUnknown *pUnkOuter, IUnknown **ppUnkInner) : _CRefs(), _pclsidProtocol(rclsid), _Unknown()
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CBaseProtocol::CBaseProtocol \n", this));
    _dwThreadID = GetCurrentThreadId();
    _bscf = BSCF_FIRSTDATANOTIFICATION;
    _pOIBindInfo = 0;


    if (!pUnkOuter)
    {
        pUnkOuter = &_Unknown;
    }
    else
    {
        EProtAssert((ppUnkInner));
        if (ppUnkInner)
        {
            *ppUnkInner =  &_Unknown;
            _CRefs = 0;
        }
    }

    _pUnkOuter = pUnkOuter;

    EProtDebugOut((DEB_PLUGPROT, "%p OUT CBaseProtocol::CBaseProtocol \n", this));
}

//+---------------------------------------------------------------------------
//
//  Method:     CBaseProtocol::~CBaseProtocol
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    11-09-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CBaseProtocol::~CBaseProtocol()
{

    if (_pUnkInner)
    {
        _pUnkInner->Release();
    }
    if (_pwzUrl)
    {
        delete _pwzUrl;
    }

    EProtAssert(( _hFile == NULL));

    if (_szTempFile[0] != '\0')
    {
        DeleteFile(_szTempFile);
    }

    EProtDebugOut((DEB_PLUGPROT, "%p _IN/OUT CBaseProtocol::~CBaseProtocol \n", this));
}

//+---------------------------------------------------------------------------
//
//  Method:     CBaseProtocol::CPrivUnknown::QueryInterface
//
//  Synopsis:
//
//  Arguments:  [riid] --
//              [ppvObj] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBaseProtocol::CPrivUnknown::QueryInterface(REFIID riid, void **ppvObj)
{
    VDATEPTROUT(ppvObj, void *);
    VDATETHIS(this);
    HRESULT hr = NOERROR;

    EProtDebugOut((DEB_PLUGPROT, "%p _IN CBaseProtocol::CPrivUnknown::QueryInterface\n", this));
    CBaseProtocol *pCBaseProtocol = GETPPARENT(this, CBaseProtocol, _Unknown);

    *ppvObj = NULL;

    if ((riid == IID_IUnknown) || (riid == IID_IOInetProtocol) || (riid == IID_IOInetProtocolRoot) )
    {
        *ppvObj = (IOInetProtocol *) pCBaseProtocol;
        pCBaseProtocol->AddRef();
    }
    else if (riid == IID_IOInetThreadSwitch)
    {
        *ppvObj = (IOInetThreadSwitch *)pCBaseProtocol;
        pCBaseProtocol->AddRef();
    }
    else if (riid == IID_IServiceProvider)
    {
        *ppvObj = (IServiceProvider *)pCBaseProtocol;
        pCBaseProtocol->AddRef();
    }
    else if (pCBaseProtocol->_pUnkInner)
    {
        hr = pCBaseProtocol->_pUnkInner->QueryInterface(riid, ppvObj);
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    EProtDebugOut((DEB_PLUGPROT, "%p OUT CBaseProtocol::CPrivUnknown::QueryInterface (hr:%lx\n", this,hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CBaseProtocol::CPrivUnknown::AddRef
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CBaseProtocol::CPrivUnknown::AddRef(void)
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CBaseProtocol::CPrivUnknown::AddRef\n", this));

    LONG lRet = ++_CRefs;

    EProtDebugOut((DEB_PLUGPROT, "%p OUT CBaseProtocol::CPrivUnknown::AddRef (cRefs:%ld)\n", this,lRet));
    return lRet;
}
//+---------------------------------------------------------------------------
//
//  Function:   CBaseProtocol::Release
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CBaseProtocol::CPrivUnknown::Release(void)
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CBaseProtocol::CPrivUnknown::Release\n", this));

    CBaseProtocol *pCBaseProtocol = GETPPARENT(this, CBaseProtocol, _Unknown);

    LONG lRet = --_CRefs;

    if (lRet == 0)
    {
        delete pCBaseProtocol;
    }

    EProtDebugOut((DEB_PLUGPROT, "%p OUT CBaseProtocol::CPrivUnknown::Release (cRefs:%ld)\n",this,lRet));
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBaseProtocol::OpenTempFile
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    11-09-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL CBaseProtocol::OpenTempFile()
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CBaseProtocol::OpenTempFile\n", this));

    HANDLE hTempFile;
    BOOL fRet = FALSE;

    static char szTempPath[MAX_PATH+32] = {0};


    if (!szTempPath[0])
    {
        GetTempPath(MAX_PATH, szTempPath);
    }

    if (GetTempFileName(szTempPath, "Res", 0, _szTempFile))
    {
        // the file should be delete
        DWORD dwFileAtr = FILE_ATTRIBUTE_TEMPORARY;

        EProtDebugOut((DEB_PLUGPROT, "%p +++ CBaseProtocol::OpenTempFile (szFile:%s)\n",this, _szTempFile));
        hTempFile = CreateFile(_szTempFile, GENERIC_WRITE,FILE_SHARE_READ, NULL,CREATE_ALWAYS,dwFileAtr, NULL);

        if (hTempFile == INVALID_HANDLE_VALUE)
        {
            _hFile = NULL;
        }
        else
        {
            WCHAR    wzTempFile[MAX_PATH];
            A2W(_szTempFile,wzTempFile,MAX_PATH);

            _pProtSink->ReportProgress(BINDSTATUS_CACHEFILENAMEAVAILABLE, wzTempFile);
            _hFile = hTempFile;
            fRet = TRUE;
        }
    }

    EProtDebugOut((DEB_PLUGPROT, "%p OUT CBaseProtocol::OpenTempFile (_szTempFile:%s, fRet:%d)\n",this, _szTempFile, fRet));
    return fRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBaseProtocol::CloseTempFile
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL CBaseProtocol::CloseTempFile()
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CBaseProtocol::CloseTempFile\n", this));

    BOOL fRet = FALSE;

    if (_hFile)
    {
        CloseHandle(_hFile);
        _hFile = 0;
        fRet = TRUE;
    }

    EProtDebugOut((DEB_PLUGPROT, "%p OUT CBaseProtocol::CloseTempFile (fRet:%d)\n",this, fRet));
    return fRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   CreateAPP
//
//  Synopsis:
//
//  Arguments:  [rclsid] --
//              [pUnkOuter] --
//              [riid] --
//              [ppUnk] --
//
//  Returns:
//
//  History:    11-09-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CreateAPP(REFCLSID rclsid, IUnknown *pUnkOuter, REFIID riid, IUnknown **ppUnk)
{
    EProtDebugOut((DEB_PLUGPROT, "API _IN CreateKnownProtocolInstance\n"));
    HRESULT hr = NOERROR;

    EProtAssert((ppUnk));

    if (!ppUnk || (pUnkOuter && (riid != IID_IUnknown)) )
    {
        // Note: aggregation only works if asked for IUnknown
        EProtAssert((FALSE && "Dude, look up aggregation rules - need to ask for IUnknown"));
        hr = E_INVALIDARG;
    }
    else
    {
        CBaseProtocol *pCBaseProtocol = NULL;

        if (rclsid == CLSID_CdlProtocol)
        {
            pCBaseProtocol = new CCdlProtocol(CLSID_CdlProtocol,pUnkOuter, ppUnk);
        }

#ifdef TEST_EAPP
        else if (rclsid == CLSID_OhServNameSp)
        {
            pCBaseProtocol = new COhServNameSp(CLSID_OhServNameSp,pUnkOuter, ppUnk);
        }
        else if (rclsid == CLSID_MimeHandlerTest1)
        {
            pCBaseProtocol = new CMimeHandlerTest1(CLSID_MimeHandlerTest1,pUnkOuter, ppUnk);
        }
        else if (rclsid == CLSID_ResProtocol)
        {
            pCBaseProtocol = new CResProtocol(CLSID_ResProtocol,pUnkOuter, ppUnk);
        }
#endif

        if (pCBaseProtocol)
        {
            if (riid == IID_IUnknown)
            {

            }
            else if (riid == IID_IOInetProtocol)
            {
                // ok, got the right interface already
                *ppUnk = (IOInetProtocol *)pCBaseProtocol;
            }
            else
            {
                hr = pCBaseProtocol->QueryInterface(riid, (void **)ppUnk);
                // remove extra refcount
                pCBaseProtocol->Release();
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    EProtDebugOut((DEB_PLUGPROT, "API OUT CreateKnownProtocolInstance(hr:%lx)\n", hr));
    return hr;
}

HRESULT CBaseProtocol::ObtainService(REFGUID rsid, REFIID riid, void ** ppvObj)
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CBaseProtocol::ObtainService \n", this));
    HRESULT     hr = NOERROR;
    VDATETHIS(this);
    LPVOID pvLocal = NULL;

#ifdef unusedXXX
    CBSCNode   *pNode;
    pNode = _pCBSCNode;

    if (riid == IID_IHttpNegotiate)
    {
        *ppvObj = (void*)(IHttpNegotiate *) this;
        AddRef();

        // loop once to get all interfaces
        if (!_fHttpNegotiate)
        {
            while (pNode)
            {
                if (   (pNode->GetBSCB()->QueryInterface(riid, &pvLocal) == NOERROR)
                    || (   pNode->GetServiceProvider()
                        && (pNode->GetHttpNegotiate() == NULL)
                        && (pNode->GetServiceProvider()->QueryService(riid, riid, &pvLocal)) == NOERROR)
                    )
                {
                    // Note: the interface is addref'd by QI or QS
                    pNode->SetHttpNegotiate((IHttpNegotiate *)pvLocal);
                }

                pNode = pNode->GetNextNode();
            }

            _fHttpNegotiate = TRUE;
        }
    }
    else if (riid == IID_IAuthenticate)
    {
        *ppvObj = (void*)(IAuthenticate *) this;
        AddRef();

        if (!_fAuthenticate)
        {
            while (pNode)
            {
                if (   (pNode->GetBSCB()->QueryInterface(riid, &pvLocal) == NOERROR)
                    || (   pNode->GetServiceProvider()
                        && (pNode->GetAuthenticate() == NULL)
                        && (pNode->GetServiceProvider()->QueryService(riid, riid, &pvLocal)) == NOERROR)
                    )
                {
                    // Note: the interface is addref'd by QI or QS
                    pNode->SetAuthenticate((IAuthenticate *)pvLocal);
                }

                pNode = pNode->GetNextNode();
            }

            _fAuthenticate = TRUE;
        }

    }
    else
    {
        *ppvObj = NULL;
        hr = E_NOINTERFACE;

        while (pNode)
        {
            if (   (pNode->GetBSCB()->QueryInterface(riid, &pvLocal) == NOERROR)
                || (   pNode->GetServiceProvider()
                    && (pNode->GetServiceProvider()->QueryService(riid, riid, &pvLocal)) == NOERROR)
                )
            {
                *ppvObj = pvLocal;
                hr = NOERROR;
                // Note: the interface is addref'd by QI or QS
                // stop looking at other nodes for this service
                pNode = NULL;
            }

            if (pNode)
            {
                pNode = pNode->GetNextNode();
            }
        }
    }
#endif //unused

    EProtDebugOut((DEB_PLUGPROT, "%p OUT CBaseProtocol::ObtainService (hr:%lx) \n", this, hr));
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CHttpNegotiate::QueryInterface
//
//  Synopsis:
//
//  Arguments:  [riid] --
//              [ppvObj] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CHttpNegotiate::QueryInterface(REFIID riid, void **ppvObj)
{
    VDATEPTROUT(ppvObj, void *);
    VDATETHIS(this);
    HRESULT hr = NOERROR;

    EProtDebugOut((DEB_PLUGPROT, "%p _IN CHttpNegotiate::QueryInterface\n", this));

    if ((riid == IID_IUnknown) || (riid == IID_IHttpNegotiate))
    {
        *ppvObj = (IHttpNegotiate *) this;
        AddRef();
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    EProtDebugOut((DEB_PLUGPROT, "%p OUT CHttpNegotiate::QueryInterface (hr:%lx\n", this,hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CHttpNegotiate::AddRef
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CHttpNegotiate::AddRef(void)
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CHttpNegotiate::AddRef\n", this));

    LONG lRet = ++_CRefs;

    EProtDebugOut((DEB_PLUGPROT, "%p OUT CHttpNegotiate::AddRef (cRefs:%ld)\n", this,lRet));
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   CHttpNegotiate::Release
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CHttpNegotiate::Release(void)
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CHttpNegotiate::Release\n", this));

    LONG lRet = --_CRefs;

    if (lRet == 0)
    {
        delete this;
    }

    EProtDebugOut((DEB_PLUGPROT, "%p OUT CHttpNegotiate::Release (cRefs:%ld)\n",this,lRet));
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     CHttpNegotiate::BeginningTransaction
//
//  Synopsis:
//
//  Arguments:  [szURL] --
//              [szHeaders] --
//              [dwReserved] --
//              [pszAdditionalHeaders] --
//
//  Returns:
//
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CHttpNegotiate::BeginningTransaction(LPCWSTR szURL, LPCWSTR szHeaders,
            DWORD dwReserved, LPWSTR *pszAdditionalHeaders)
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CHttpNegotiate::BeginningTransaction (szURL:%ws, szHeaders:%ws)\n", this, szURL, szHeaders));
    VDATETHIS(this);
    HRESULT    hr = NOERROR;
    LPWSTR     szTmp = NULL, szNew = NULL, szRunning = NULL;

    EProtAssert((szURL));


    EProtDebugOut((DEB_PLUGPROT, "%p OUT CHttpNegotiate::BeginningTransaction (pszAdditionalHeaders:%ws )\n", this, *pszAdditionalHeaders));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CHttpNegotiate::OnResponse
//
//  Synopsis:
//
//  Arguments:  [LPCWSTR] --
//              [szResponseHeaders] --
//              [LPWSTR] --
//              [pszAdditionalRequestHeaders] --
//
//  Returns:
//
//  History:    4-05-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CHttpNegotiate::OnResponse(DWORD dwResponseCode,LPCWSTR wzResponseHeaders,
                        LPCWSTR wzRequestHeaders,LPWSTR *pszAdditionalRequestHeaders)
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CHttpNegotiate::OnResponse\n", this));
    VDATETHIS(this);
    HRESULT    hr;
    LPWSTR     szTmp = NULL, szNew = NULL, szRunning = NULL;

    hr = IsStatusOk(dwResponseCode) ? S_OK : S_FALSE;

    EProtDebugOut((DEB_PLUGPROT, "%p OUT CHttpNegotiate::OnResponse\n", this));
    return hr;
}
