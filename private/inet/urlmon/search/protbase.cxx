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
#include <urlint.h>
#include <stdio.h>
#include <sem.hxx>
#include <wininet.h>
#include "urlcf.hxx"
#include "protbase.hxx"
#include "resprot.hxx"


extern GUID CLSID_ResProtocol;


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

    TransDebugOut((DEB_PROT, "%p _IN CBaseProtocol::QueryInterface\n", this));

    hr = _pUnkOuter->QueryInterface(riid, ppvObj);

    TransDebugOut((DEB_PROT, "%p OUT CBaseProtocol::QueryInterface (hr:%lx\n", this,hr));
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
    TransDebugOut((DEB_PROT, "%p _IN CBaseProtocol::AddRef\n", this));

    LONG lRet = _pUnkOuter->AddRef();

    TransDebugOut((DEB_PROT, "%p OUT CBaseProtocol::AddRef (cRefs:%ld)\n", this,lRet));
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
    TransDebugOut((DEB_PROT, "%p _IN CBaseProtocol::Release\n", this));

    LONG lRet = _pUnkOuter->Release();

    TransDebugOut((DEB_PROT, "%p OUT CBaseProtocol::Release (cRefs:%ld)\n",this,lRet));
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
                          DWORD grfSTI, DWORD dwReserved)
{
    TransDebugOut((DEB_PROT, "%p _IN CBaseProtocol::Start\n", this));
    HRESULT hr = NOERROR;

    TransAssert((!_pProtSink && pOIBindInfo && pTrans));
    TransAssert((_pszUrl == NULL));

    _pProtSink = pTrans;
    _pProtSink->AddRef();

    _BndInfo.cbSize = sizeof(BINDINFO);

    _pOIBindInfo =  pOIBindInfo;
    _pOIBindInfo->AddRef();

    hr = pOIBindInfo->GetBindInfo(&_grfBindF, &_BndInfo);

    // save the URL
    _pszUrl = DupW2A(pwzUrl);

    TransDebugOut((DEB_PROT, "%p OUT CBaseProtocol::Start (hr:%lx)\n",this, hr));
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
    TransDebugOut((DEB_PROT, "%p _IN CBaseProtocol::Continue\n", this));

    HRESULT hr = E_FAIL;

    TransDebugOut((DEB_PROT, "%p OUT CBaseProtocol::Continue (hr:%lx)\n",this, hr));
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
    TransDebugOut((DEB_PROT, "%p _IN CBaseProtocol::Abort\n", this));
    HRESULT hr = NOERROR;

    if (_pProt)
        return _pProt->Abort(hrReason, dwOptions);

    TransAssert((_pProtSink));

    hr = _pProtSink->ReportResult(E_ABORT, 0, 0);

    TransDebugOut((DEB_PROT, "%p OUT CBaseProtocol::Abort (hr:%lx)\n",this, hr));
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
    TransDebugOut((DEB_PROT, "%p _IN CBaseProtocol::Terminate\n", this));
    HRESULT hr = NOERROR;

    if (_pProt)
    {
        hr = _pProt->Terminate(dwOptions);

        if (FAILED(hr))
            return hr;

        _pProt->Release();
        _pProt = NULL;
    }

    TransAssert((_pProtSink));

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
        TransDebugOut((DEB_TRANS, "%p --- CBaseProtocol::Stop ReleaseStgMedium (%p)\n", this,_BndInfo.stgmedData));
#endif

    ReleaseBindInfo(&_BndInfo);

    TransDebugOut((DEB_PROT, "%p OUT CBaseProtocol::Terminate (hr:%lx)\n",this, hr));
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
    TransDebugOut((DEB_PROT, "%p _IN CBaseProtocol::Suspend\n", this));

    HRESULT hr = E_NOTIMPL;

    if (_pProt)
        hr = _pProt->Suspend();

    TransDebugOut((DEB_PROT, "%p OUT CBaseProtocol::Suspend (hr:%lx)\n",this, hr));
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
    TransDebugOut((DEB_PROT, "%p _IN CBaseProtocol::Resume\n", this));

    HRESULT hr = E_NOTIMPL;

    if (_pProt)
        hr = _pProt->Resume();

    TransDebugOut((DEB_PROT, "%p OUT CBaseProtocol::Resume (hr:%lx)\n",this, hr));
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
    TransDebugOut((DEB_PROT, "%p _IN CBaseProtocol::SetPriority\n", this));

    HRESULT hr = E_NOTIMPL;

    TransDebugOut((DEB_PROT, "%p OUT CBaseProtocol::SetPriority (hr:%lx)\n",this, hr));
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
    TransDebugOut((DEB_PROT, "%p _IN CBaseProtocol::GetPriority\n", this));

    HRESULT hr = E_NOTIMPL;

    TransDebugOut((DEB_PROT, "%p OUT CBaseProtocol::GetPriority (hr:%lx)\n",this, hr));
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
    TransDebugOut((DEB_PROT, "%p _IN CBaseProtocol::Read\n", this));

    HRESULT hr = E_FAIL;

    if (_pProt)
        hr = _pProt->Read(pv, cb, pcbRead);

    TransDebugOut((DEB_PROT, "%p OUT CBaseProtocol::Read (hr:%lx)\n",this, hr));
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
    TransDebugOut((DEB_PROT, "%p _IN CBaseProtocol::Seek\n", this));

    HRESULT hr = E_FAIL;

    if (_pProt)
        hr = _pProt->Seek(dlibMove, dwOrigin, plibNewPosition);

    TransDebugOut((DEB_PROT, "%p OUT CBaseProtocol::Seek (hr:%lx)\n",this, hr));
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
    TransDebugOut((DEB_PROT, "%p _IN CBaseProtocol::LockRequest\n", this));

    HRESULT hr = NOERROR;

    if (_pProt)
        hr = _pProt->LockRequest(dwOptions);

    TransDebugOut((DEB_PROT, "%p OUT CBaseProtocol::LockRequest (hr:%lx)\n",this, hr));
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
    TransDebugOut((DEB_PROT, "%p _IN CBaseProtocol::UnlockRequest\n", this));

    HRESULT hr = NOERROR;

    if (_pProt)
        hr = _pProt->UnlockRequest();

    TransDebugOut((DEB_PROT, "%p OUT CBaseProtocol::UnlockRequest (hr:%lx)\n",this, hr));
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
    TransDebugOut((DEB_PROT, "%p _IN CBaseProtocol::Prepare\n", this));
    HRESULT hr = NOERROR;

    TransAssert((  IsApartmentThread() ));


    TransDebugOut((DEB_PROT,"%p OUT CBaseProtocol::Prepare (hr:%lx)\n",this, hr));
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
    TransDebugOut((DEB_PROT, "%p _IN CBaseProtocol::Continue\n", this));
    HRESULT hr = NOERROR;

    TransAssert((  !IsApartmentThread() ));

    _dwThreadID = GetCurrentThreadId();

    TransDebugOut((DEB_PROT,"%p OUT CBaseProtocol::Continue (hr:%lx)\n",this, hr));
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
    TransDebugOut((DEB_PROT, "%p _IN CBaseProtocol::CBaseProtocol \n", this));
    _dwThreadID = GetCurrentThreadId();
    _bscf = BSCF_FIRSTDATANOTIFICATION;
    _pOIBindInfo = 0;

    _pszUrl = 0;
    _pProt = 0;

    if (!pUnkOuter)
    {
        pUnkOuter = &_Unknown;
    }
    else
    {
        TransAssert((ppUnkInner));
        if (ppUnkInner)
        {
            *ppUnkInner =  &_Unknown;
            _CRefs = 0;
        }
    }

    _pUnkOuter = pUnkOuter;

    TransDebugOut((DEB_PROT, "%p OUT CBaseProtocol::CBaseProtocol \n", this));
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
    if (_pszUrl)
        delete _pszUrl;

    TransDebugOut((DEB_PROT, "%p _IN/OUT CBaseProtocol::~CBaseProtocol \n", this));
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

    TransDebugOut((DEB_PROT, "%p _IN CBaseProtocol::CPrivUnknown::QueryInterface\n", this));
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
    else
    {
        hr = E_NOINTERFACE;
    }

    TransDebugOut((DEB_PROT, "%p OUT CBaseProtocol::CPrivUnknown::QueryInterface (hr:%lx\n", this,hr));
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
    TransDebugOut((DEB_PROT, "%p _IN CBaseProtocol::CPrivUnknown::AddRef\n", this));

    LONG lRet = ++_CRefs;

    TransDebugOut((DEB_PROT, "%p OUT CBaseProtocol::CPrivUnknown::AddRef (cRefs:%ld)\n", this,lRet));
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
    TransDebugOut((DEB_PROT, "%p _IN CBaseProtocol::CPrivUnknown::Release\n", this));

    CBaseProtocol *pCBaseProtocol = GETPPARENT(this, CBaseProtocol, _Unknown);

    LONG lRet = --_CRefs;

    if (lRet == 0)
    {
        delete pCBaseProtocol;
    }

    TransDebugOut((DEB_PROT, "%p OUT CBaseProtocol::CPrivUnknown::Release (cRefs:%ld)\n",this,lRet));
    return lRet;
}


//+---------------------------------------------------------------------------
//
//  Function:   OLESTRDuplicate
//
//  Synopsis:
//
//  Arguments:  [ws] --
//
//  Returns:
//
//  History:    11-09-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
LPWSTR OLESTRDuplicate(LPWSTR ws)
{
    LPWSTR wsNew = NULL;

    if (ws)
    {
        wsNew = (LPWSTR) new  WCHAR [wcslen(ws) + 1];
        if (wsNew)
        {
            wcscpy(wsNew, ws);
        }
    }

    return wsNew;
}

//+---------------------------------------------------------------------------
//
//  Function:   DupW2A
//
//  Synopsis:   duplicates a wide string to an ansi string
//
//  Arguments:  [pwz] --
//
//  History:    7-20-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
LPSTR DupW2A(const WCHAR *pwz)
{
    LPSTR szNew = NULL;

    if (pwz)
    {
        DWORD dwlen = wcslen(pwz) + 1;
        szNew = (LPSTR) new char [dwlen];
        if (szNew)
        {
            W2A(pwz, szNew, dwlen);
        }
    }

    return szNew;
}


//+---------------------------------------------------------------------------
//
//  Function:   DupA2W
//
//  Synopsis:   duplicates an ansi string to a wide string
//
//  Arguments:  [lpszAnsi] --
//
//  History:    7-20-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
LPWSTR DupA2W(const LPSTR psz)
{
    LPWSTR wzNew = NULL;

    if (psz)
    {
        DWORD dwlen = strlen(psz) + 1;
        wzNew = (LPWSTR) new WCHAR [dwlen];
        if (wzNew)
        {
            A2W(psz, wzNew, dwlen);
        }
    }

    return wzNew;
}


//BUG-WORK remove this and link to lib
const GUID IID_IOInet                = { 0x79eac9e0, 0xbaf9, 0x11ce, {0x8c, 0x82, 0x00, 0xaa, 0x00, 0x4b, 0xa9, 0x0b} };
const GUID IID_IOInetBindInfo        = { 0x79eac9e1, 0xbaf9, 0x11ce, {0x8c, 0x82, 0x00, 0xaa, 0x00, 0x4b, 0xa9, 0x0b} };
const GUID IID_IOInetBindClient      = { 0x79eac9e2, 0xbaf9, 0x11ce, {0x8c, 0x82, 0x00, 0xaa, 0x00, 0x4b, 0xa9, 0x0b} };
const GUID IID_IOInetProtocolRoot    = { 0x79eac9e3, 0xbaf9, 0x11ce, {0x8c, 0x82, 0x00, 0xaa, 0x00, 0x4b, 0xa9, 0x0b} };
const GUID IID_IOInetProtocol        = { 0x79eac9e4, 0xbaf9, 0x11ce, {0x8c, 0x82, 0x00, 0xaa, 0x00, 0x4b, 0xa9, 0x0b} };
const GUID IID_IOInetProtocolSink    = { 0x79eac9e5, 0xbaf9, 0x11ce, {0x8c, 0x82, 0x00, 0xaa, 0x00, 0x4b, 0xa9, 0x0b} };
const GUID IID_IOInetBinding         = { 0x79eac9e6, 0xbaf9, 0x11ce, {0x8c, 0x82, 0x00, 0xaa, 0x00, 0x4b, 0xa9, 0x0b} };
const GUID IID_IOInetSession         = { 0x79eac967, 0xbaf9, 0x11ce, {0x8c, 0x82, 0x00, 0xaa, 0x00, 0x4b, 0xa9, 0x0b} };
const GUID IID_IOInetThreadSwitch    = { 0x79eac968, 0xbaf9, 0x11ce, {0x8c, 0x82, 0x00, 0xaa, 0x00, 0x4b, 0xa9, 0x0b} };
const GUID IID_IOInetBindSink        = { 0x79eac9e9, 0xbaf9, 0x11ce, {0x8c, 0x82, 0x00, 0xaa, 0x00, 0x4b, 0xa9, 0x0b} };
const GUID IID_IOInetCache           = { 0x79eac9ea, 0xbaf9, 0x11ce, {0x8c, 0x82, 0x00, 0xaa, 0x00, 0x4b, 0xa9, 0x0b} };
const GUID IID_IOInetPriority        = { 0x79eac9eb, 0xbaf9, 0x11ce, {0x8c, 0x82, 0x00, 0xaa, 0x00, 0x4b, 0xa9, 0x0b} };


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
    TransDebugOut((DEB_PROT, "API _IN CreateKnownProtocolInstance\n"));
    HRESULT hr = NOERROR;

    TransAssert((ppUnk));

    if (!ppUnk || (pUnkOuter && (riid != IID_IUnknown)) )
    {
        // Note: aggregation only works if asked for IUnknown
        TransAssert((FALSE && "Dude, look up aggregation rules - need to ask for IUnknown"));
        hr = E_INVALIDARG;
    }
    else
    {
        CBaseProtocol *pCBaseProtocol = NULL;

        if (rclsid == CLSID_ResProtocol)
        {
            pCBaseProtocol = new CResProtocol(CLSID_ResProtocol,pUnkOuter, ppUnk);
        }

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

    TransDebugOut((DEB_PROT, "API OUT CreateKnownProtocolInstance(hr:%lx)\n", hr));
    return hr;

}


