//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       cbinding.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    11-11-95   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <trans.h>
#include "oinet.hxx"
#include "cdl.h"    // defined in urlmon\download\

// from helpers.cxx
HRESULT IsMimeHandled(LPCWSTR pwszMimeType);

// From shlwapip.h
LWSTDAPI_(HRESULT) CLSIDFromStringWrap(LPOLESTR lpsz, LPCLSID pclsid);


PerfDbgTag(tagCBinding,     "Urlmon", "Log CBinding",        DEB_BINDING);
DbgTag(tagCBindingErr,  "Urlmon", "Log CBinding Errors", DEB_BINDING|DEB_ERROR);
extern DWORD g_dwSettings;

#define MAX_PROTOCOL_LEN    32  // protocl string length in ASCII BUGBUG is there a standard?
#define PROTOCOL_DELIMITER  ':'
#define REG_PROTOCOL_HANDLER    L"ProtocolHandler"

WCHAR *rglpProto[] =
{
    L"https",
    L"http",
    L"ftp",
    L"gopher",
    L"file",
    L"local",
    L"mk",
    NULL,
};

HRESULT GetTransactionObjects(LPBC pBndCtx, LPCWSTR wzUrl, IUnknown *pUnkOuter, IUnknown **ppUnk, IOInetProtocol **ppCTrans, DWORD dwOption, CTransData **pCTransData);
BOOL PDFNeedProgressiveDownload();

EXTERN_C const GUID CLSID_MsHtml;
EXTERN_C const GUID IID_ITransactionData;


//+---------------------------------------------------------------------------
//
//  Function:   CreateURLBinding
//
//  Synopsis:
//
//  Arguments:  [lpszUrl] --
//              [pbc] --
//              [ppBdg] --
//
//  Returns:
//
//  History:    12-04-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CreateURLBinding(LPWSTR lpszUrl, IBindCtx *pbc, IBinding **ppBdg)
{
    PerfDbgLog(tagCBinding, NULL, "+CreateURLBinding");
    HRESULT  hr = NOERROR;

    PerfDbgLog1(tagCBinding, NULL, "-CreateURLBinding (IBinding:%lx)", *ppBdg);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBinding::Create
//
//  Synopsis:
//
//  Arguments:  [pUnkOuter] --
//              [LPBC] --
//              [pbc] --
//
//  Returns:
//
//  History:    12-06-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CBinding::Create(IUnknown *pUnkOuter, LPCWSTR szUrl, LPBC pbc, REFIID riid, BOOL fBindToObject, CBinding **ppCBdg)
{
    PerfDbgLog1(tagCBinding, NULL, "+CBinding::Create (szUrl:%ws)", szUrl);
    HRESULT hr = NOERROR;
    CBinding *pCBdg;

    UrlMkAssert((ppCBdg != NULL));

    // Create and initialize the cbinding object
    pCBdg = new CBinding(NULL);
    if (pCBdg == NULL)
    {
        hr = E_OUTOFMEMORY;
    }

    *ppCBdg = pCBdg;

    PerfDbgLog2(tagCBinding, NULL, "-CBinding::Create (hr:%lx,IBinding:%lx)", hr, pCBdg);
    return hr;
}
//+---------------------------------------------------------------------------
//
//  Method:     CBinding::Initialize
//
//  Synopsis:
//
//  Arguments:  [szUrl] --
//              [pbc] --
//
//  Returns:
//
//  History:    12-04-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CBinding::Initialize(LPCWSTR szUrl, IBindCtx *pbc, DWORD grfBindF, REFIID riid, BOOL fBindToObject)
{
    HRESULT hr = NOERROR;
    PerfDbgLog(tagCBinding, this, "+CBinding::Initialize");

    _fBindToObject = fBindToObject;

    if (fBindToObject)
    {

        // Get the bind options from the bind context
        _bindopts.cbStruct = sizeof(BIND_OPTS);
        hr = pbc->GetBindOptions(&_bindopts);
        if (FAILED(hr))
        {
            goto End;
        }
    }

    hr = CBindCtx::Create(&_pBndCtx, pbc);


    if ((hr == NOERROR) && szUrl)
    {
        TransAssert((_pBndCtx));

        int cchWideChar;

        cchWideChar  = wcslen(szUrl) + 2;
        _lpwszUrl = (LPWSTR) new WCHAR [cchWideChar];
        if( !_lpwszUrl )
        {
            hr = E_OUTOFMEMORY;
            goto End;
        }
        wcscpy(_lpwszUrl, szUrl);

        // Try to get an IBindStatusCallback  pointer from the bind context
        hr = GetObjectParam(pbc, REG_BSCB_HOLDER, IID_IBindStatusCallback, (IUnknown**)&_pBSCB);

        UrlMkAssert(( (hr == NOERROR) && _pBSCB ));

        PerfDbgLog2(tagCBinding, this, "=== CBinding::Initialize (pbc:%lx -> _pBSCB:%lx)", pbc, _pBSCB);

        hr = GetTransactionObjects(_pBndCtx, _lpwszUrl, NULL, NULL, &_pOInetBdg,OIBDG_APARTMENTTHREADED, &_pCTransData);

        if (hr == S_OK)
        {
            TransAssert((!_pCTransData));
            // create the transaction data object
            // Note: the transdat object has a refcount
            //       and must be released when done
            hr = CTransData::Create(_lpwszUrl, grfBindF, riid, _pBndCtx, _fBindToObject, &_pCTransData);
            if (FAILED(hr))
            {
                //goto End;
            }
            else
            {
                UrlMkAssert((_pCTransData != NULL && "CTransData invalid"));
                _pBndCtx->SetTransData(_pCTransData);
            }
        }
        else if (hr == S_FALSE)
        {
            UrlMkAssert((_pCTransData != NULL && "CTransData invalid"));
            _grfInternalFlags |= BDGFLAGS_ATTACHED;

            hr = _pCTransData->Initialize(_lpwszUrl, grfBindF, riid, _pBndCtx);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        if (_fBindToObject)
        {
            _piidRes = (IID *) &riid;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
End:

    PerfDbgLog1(tagCBinding, this, "-CBinding::Initialize (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBinding::CBinding
//
//  Synopsis:
//
//  Arguments:  [pUnk] --
//
//  Returns:
//
//  History:    11-11-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CBinding::CBinding(IUnknown *pUnk) : _CRefs()
{
    _pUnk = pUnk;
    if (_pUnk)
    {
        _pUnk->AddRef();
    }
    _dwThreadId   = GetCurrentThreadId();
    _pBSCB        = 0;
    _nPriority    = THREAD_PRIORITY_NORMAL;
    _dwState      = 0;
    _OperationState = OPS_Initialized;
    _hwndNotify   = 0;
    _grfBINDF     = 0;
    _dwLastSize   = 0;
    _lpwszUrl     = 0;
    _pOInetBdg      = 0;
    _fSentLastNotification = 0;
    _fSentFirstNotification = 0;
    _fCreateStgMed = 0;
    _fCompleteDownloadHere = FALSE;
    _fForceBindToObjFail = FALSE;
    _fAcceptRanges = FALSE;
    _fClsidFromProt = FALSE;
    _pMnk = NULL;
    _pBndCtx = NULL;
    _piidRes = (IID*)&IID_IUnknown; // NULL;
    _pUnkObject = NULL;
    _pBasicAuth = NULL;
    _hrBindResult = NOERROR;
    _hrInstantiate = NOERROR;
    _dwBindError = 0;
    _grfInternalFlags = BDGFLAGS_NOTIFICATIONS;
    _pwzRedirectUrl = 0;
    _pwzResult = 0;

    _pIWinInetInfo = 0;
    _pIWinInetHttpInfo = 0;
    _pBindInfo = 0;
    _clsidIn = CLSID_NULL;
}
//+---------------------------------------------------------------------------
//
//  Method:     CBinding::~CBinding
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    11-11-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CBinding::~CBinding()
{
    PerfDbgLog(tagCBinding, this, "+CBinding::~CBinding");

    if (_pIWinInetInfo)
    {
        _pIWinInetInfo->Release();
    }

    if (_pIWinInetHttpInfo)
    {
        _pIWinInetHttpInfo->Release();
    }
    if (_pBindInfo)
    {
        _pBindInfo->Release();
    }

    if (_pUnk)
    {
        DbgLog(tagCBinding, this, "CBinding::~CBinding Release on _pUnk");
        _pUnk->Release();
    }

    if (_pBasicAuth)
    {
        DbgLog1(tagCBinding, this, "CBinding::~CBinding Release on _pBasicAuth (%lx)", _pBasicAuth);
        _pBasicAuth->Release();
    }

    if (_pBSCB)
    {
        DbgLog1(tagCBinding, this, "CBinding::~CBinding Release on IBSCB (%lx)", _pBSCB);
        _pBSCB->Release();
    }

    if (_pOInetBdg)
    {
        _pOInetBdg->Release();
    }

    if (_pMnk)
    {
        _pMnk->Release();
    }
    if (_pBndCtx)
    {
        _pBndCtx->Release();
    }
    if (_pCTransData)
    {
        DbgLog1(tagCBinding, this, "CBinding::~CBinding Release TransData (%lx)", _pCTransData);
        _pCTransData->Release();
    }

    if (_pUnkObject)
    {
        _pUnkObject->Release();
    }

    if (_lpwszUrl)
    {
        delete [] _lpwszUrl;
    }
    if (_pwzRedirectUrl)
    {
        delete [] _pwzRedirectUrl;
    }
    if (_pwzResult)
    {
        delete [] _pwzResult;
    }

    ReleaseBindInfo(&_BndInfo);

    PerfDbgLog(tagCBinding, this, "-CBinding::~CBinding");
}

LPWSTR CBinding::GetFileName()
{
    return _pCTransData->GetFileName();
}

//+---------------------------------------------------------------------------
//
//  Method:     CBinding::QueryInterface
//
//  Synopsis:
//
//  Arguments:  [riid] --
//              [ppv] --
//
//  Returns:
//
//  History:    11-11-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBinding::QueryInterface( REFIID riid, void **ppv )
{
    PerfDbgLog2(tagCBinding, this, "+CBinding::QueryInterface (%lx, %lx)", riid, ppv);
    HRESULT     hr = NOERROR;
    *ppv = NULL;

    //UrlMkAssert(( !IsEqualIID(GetProtocolClassID(),CLSID_NULL) ));

    if (   IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, IID_IBinding) )
    {
        *ppv = (void FAR *)(IBinding *)this;
        AddRef();
    }
    else if (IsEqualIID(riid, IID_IOInetProtocolSink))
    {
        *ppv = (void FAR *)(IOInetProtocolSink *)this;
        AddRef();
    }
    else if (IsEqualIID(riid, IID_IOInetBindInfo))
    {
        *ppv = (void FAR *)(IOInetBindInfo *)this;
        AddRef();
    }
    else if (IsEqualIID(riid, IID_IServiceProvider))
    {
        *ppv = (void FAR *)(IServiceProvider *)this;
        AddRef();
    }
    else if (IsEqualIID(riid, IID_IWinInetInfo))
    {
        if (!_pIWinInetInfo && _pOInetBdg)
        {
            _pOInetBdg->QueryInterface(riid, (void **)&_pIWinInetInfo);
        }

        if (_pIWinInetInfo)
        {
            *ppv = (void FAR *) (IWinInetInfo *)this;
            AddRef();
        }
        else
        {
            hr = E_NOINTERFACE;
        }
    }
    else if (IsEqualIID(riid, IID_IWinInetHttpInfo))
    {
        if (!_pIWinInetHttpInfo && _pOInetBdg)
        {
            _pOInetBdg->QueryInterface(riid, (void **)&_pIWinInetHttpInfo);
        }

        if (_pIWinInetHttpInfo)
        {
            *ppv = (void FAR *) (IWinInetHttpInfo *)this;
            AddRef();
        }
        else
        {
            hr = E_NOINTERFACE;
        }
    }
    else
    {
        *ppv = NULL;
        hr = E_NOINTERFACE;
#if DBG==1
        //LPSTR lpszName = GetInterfaceName(riid);
        //DbgLog3(tagCBinding, this, "CBinding::QI(pUnkObj) >%s< hr:%lx [%lx]", lpszName, hr, *ppv));
#endif // DBG==1

    }

    PerfDbgLog2(tagCBinding, this, "-CBinding::QueryInterface (%lx)[%lx]", hr, *ppv);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBinding::AddRef
//
//  Synopsis:
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    11-11-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CBinding::AddRef( void )
{
    LONG lRet = ++_CRefs;
    PerfDbgLog1(tagCBinding, this, "CBinding::AddRef (%ld)", lRet);
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBinding::Release
//
//  Synopsis:
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    11-11-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CBinding::Release( void )
{
    PerfDbgLog(tagCBinding, this, "+CBinding::Release");
    LONG lRet = --_CRefs;
    if (_CRefs == 0)
    {
        delete this;
    }
    PerfDbgLog1(tagCBinding, this, "-CBinding::Release (%ld)", lRet);
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBinding::Abort
//
//  Synopsis:
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    11-11-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBinding::Abort( void )
{
    PerfDbgLog(tagCBinding, this, "+CBinding::Abort");
    HRESULT     hr = NOERROR;
    
    // AddRef - Release pair to guard this function since it may 
    // call OnStopBinding() and client will re-enter this 
    // Object with a Release() call.

    AddRef();

    if (   (GetOperationState() < OPS_Abort)
        && (GetOperationState() > OPS_Initialized))
    {
        DbgLog(tagCBindingErr, this, ">>> CBinding::Abort");

        // Abort will call OnStopBinding
        TransAssert((_pOInetBdg));
        hr = _pOInetBdg->Abort(E_ABORT, 0);
        if( hr != INET_E_RESULT_DISPATCHED )
        {
            //
            // only set state to OPS_Abort if the the ReportResult
            // has not been dispatched already
            //
            DbgLog(tagCBindingErr, this, ">>> Result already dispatched");
            SetOperationState(OPS_Abort);
        }
    }
    else
    {
        UrlMkAssert(( (   (GetOperationState() < OPS_Stopped)
                       && (GetOperationState() > OPS_Initialized)) ));
        hr = E_FAIL;
    }

    PerfDbgLog1(tagCBinding, this, "-CBinding::Abort (hr:%lx)", hr);

    // release
    Release();
    return hr;
}
//+---------------------------------------------------------------------------
//
//  Method:     CBinding::Suspend
//
//  Synopsis:
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    11-11-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBinding::Suspend( void )
{
    PerfDbgLog(tagCBinding, this, "+CBinding::Suspend");
    HRESULT     hr = E_FAIL;

#ifdef SUSPEND_WORKING
    if (   (GetOperationState() < OPS_Stopped)
        && (GetOperationState() > OPS_Initialized))
    {
        SetOperationState(OPS_Suspend);
    }
    else
    {
        UrlMkAssert(( (   (GetOperationState() < OPS_Stopped)
                       && (GetOperationState() > OPS_Initialized)) ));
    }
#endif //SUSPEND_WORKING

    hr = _pOInetBdg->Suspend();

#ifdef UNUSED
    UrlMkAssert((_dwState == OPS_Downloading));

    _dwState = OPS_Suspend;
    if (_pOInetBdg)
    {
        _pOInetBdg->SetOperationState(OPS_Suspend);
    }
#endif //UNUSED

    PerfDbgLog1(tagCBinding, this, "-CBinding::Suspend (hr:%lx)", hr);
    return hr;
}
//+---------------------------------------------------------------------------
//
//  Method:     CBinding::Resume
//
//  Synopsis:
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    11-11-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBinding::Resume( void )
{
    PerfDbgLog(tagCBinding, this, "+CBinding::Resume");
    HRESULT     hr = NOERROR;


    if (GetOperationState() == OPS_Suspend)
    {
        SetOperationState(OPS_Downloading);
    }
    else
    {
        UrlMkAssert(( GetOperationState() == OPS_Suspend ));
    }
    hr = _pOInetBdg->Resume();

    PerfDbgLog1(tagCBinding, this, "-CBinding::Resume (hr:%lx)", hr);
    return hr;
}
//+---------------------------------------------------------------------------
//
//  Method:     CBinding::SetPriority
//
//  Synopsis:
//
//  Arguments:  [nPriority] --
//
//  Returns:
//
//  History:    11-11-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBinding::SetPriority(LONG nPriority)
{
    PerfDbgLog1(tagCBinding, this, "+CBinding::SetPriority (%ld)", nPriority);
    HRESULT     hr = NOERROR;

    _nPriority = nPriority;

    PerfDbgLog1(tagCBinding, this, "-CBinding::SetPriority (hr:%lx)", hr);
    return hr;
}
//+---------------------------------------------------------------------------
//
//  Method:     CBinding::GetPriority
//
//  Synopsis:
//
//  Arguments:  [pnPriority] --
//
//  Returns:
//
//  History:    11-11-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBinding::GetPriority(LONG *pnPriority)
{
    PerfDbgLog(tagCBinding, this, "+CBinding::GetPriority");
    HRESULT     hr = NOERROR;

    if (!pnPriority)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        *pnPriority = _nPriority;
    }

    PerfDbgLog1(tagCBinding, this, "-CBinding::GetPriority (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBinding::GetBindResult
//
//  Synopsis:
//
//  Arguments:  [pnPriority] --
//
//  Returns:
//
//  History:    11-11-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBinding::GetBindResult(CLSID *pclsidProtocol, DWORD *pdwResult, LPWSTR *pszResult,DWORD *pdwReserved)
{
    PerfDbgLog(tagCBinding, this, "+CBinding::GetBindResult");
    HRESULT     hr = NOERROR;

    if (!pdwResult || !pszResult || pdwReserved)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        HRESULT  hrRet = NOERROR;
        *pdwResult = 0;
        *pszResult = 0;
        *pclsidProtocol = CLSID_NULL;

        if ((hrRet = GetInstantiateHresult()) != NOERROR)
        {
            *pdwResult = (DWORD) hrRet;
            TransAssert ((  (_hrBindResult == INET_E_CANNOT_INSTANTIATE_OBJECT)
                         || (_hrBindResult == INET_E_CANNOT_LOAD_DATA) ));
        }
        else if (_hrBindResult != NOERROR)
        {
            *pclsidProtocol = _clsidProtocol;
            *pszResult = OLESTRDuplicate(_pwzResult);

            UrlMkAssert(( (_dwBindError != 0) || (_hrBindResult != NOERROR) ));

            if (_dwBindError == 0)
            {
                _dwBindError = _hrBindResult;
            }
            *pdwResult = _dwBindError;
        }
    }

    PerfDbgLog3(tagCBinding, this, "-CBinding::GetBindResult (hr:%lx,_hrBindResult;%lx, pdwResult:%lx)", hr, _hrBindResult, *pdwResult);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBinding::QueryOption
//
//  Synopsis:   Calls QueryOptions on
//
//  Arguments:  [dwOption] --
//              [pBuffer] --
//              [pcbBuf] --
//
//  Returns:
//
//  History:    4-10-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBinding::QueryOption(DWORD dwOption, LPVOID pBuffer, DWORD *pcbBuf)
{
    HRESULT hr = NOERROR;
    PerfDbgLog(tagCBinding, this, "+CBinding::QueryOption");
    VDATEPTROUT(pcbBuf, DWORD*);

    if (   (GetOperationState() < OPS_Stopped)
        && (GetOperationState() > OPS_Initialized))
    {
        TransAssert((_pIWinInetInfo || _pIWinInetHttpInfo));
        if (_pIWinInetInfo)
        {
            hr = _pIWinInetInfo->QueryOption(dwOption, pBuffer, pcbBuf);
        }
        else if (_pIWinInetHttpInfo)
        {
            hr = _pIWinInetHttpInfo->QueryOption(dwOption, pBuffer, pcbBuf);
        }
    }
    else
    {
        hr = E_FAIL;
    }

    PerfDbgLog1(tagCBinding, this, "-CBinding::QueryOption (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBinding::QueryInfo
//
//  Synopsis:   Calls QueryInfos on
//
//  Arguments:  [dwOption] --
//              [pBuffer] --
//              [pcbBuf] --
//              [pdwFlags] --
//              [pdwReserved] --
//
//  Returns:
//
//  History:    4-10-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBinding::QueryInfo(DWORD dwOption, LPVOID pBuffer, DWORD *pcbBuf, DWORD *pdwFlags, DWORD *pdwReserved)
{
    HRESULT hr = NOERROR;
    PerfDbgLog(tagCBinding, this, "+CBinding::QueryInfo");
    VDATEPTROUT(pcbBuf, DWORD*);

    if (   (GetOperationState() < OPS_Stopped)
        && (GetOperationState() > OPS_Initialized))
    {
        TransAssert((_pIWinInetHttpInfo));
        hr = _pIWinInetHttpInfo->QueryInfo(dwOption, pBuffer, pcbBuf, pdwFlags, pdwReserved);
    }
    else
    {
        hr = E_FAIL;
    }

    PerfDbgLog1(tagCBinding, this, "-CBinding::QueryInfo (hr:%lx)", hr);
    return hr;
}

// IServiceProvider methods

//+---------------------------------------------------------------------------
//
//  Method:     CBinding::QueryService
//
//  Synopsis:   Calls QueryInfos on
//
//  Arguments:  [rsid] --
//              [riid] --
//              [ppvObj] --
//
//  Returns:
//
//  History:    4-10-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT IUnknown_QueryService(IUnknown* punk, REFGUID rsid, REFIID riid, void ** ppvObj)
{
    HRESULT hr = E_NOINTERFACE;

    *ppvObj = 0;

    if (punk)
    {
        IServiceProvider *pSrvPrv;
        hr = punk->QueryInterface(IID_IServiceProvider, (void **) &pSrvPrv);
        if (hr == NOERROR)
        {
            hr = pSrvPrv->QueryService(rsid,riid, ppvObj);
            pSrvPrv->Release();
        }
    }

    return hr;
}

HRESULT CBinding::QueryService(REFGUID rsid, REFIID riid, void ** ppvObj)
{
    PerfDbgLog(tagCBinding, this, "+CBinding::QueryService");
    HRESULT hr = E_NOINTERFACE;
    VDATETHIS(this);
    UrlMkAssert((ppvObj));

    hr = IUnknown_QueryService(_pBSCB, rsid, riid, ppvObj);

    PerfDbgLog1(tagCBinding, this, "-CBinding::QueryService (hr:%lx)", hr);
    return hr;
}


//IOInetBindInfo methods
//+---------------------------------------------------------------------------
//
//  Method:     CBinding::GetBindInfo
//
//  Synopsis:
//
//  Arguments:  [pdwBINDF] --
//              [pbindinfo] --
//
//  Returns:
//
//  History:    11-07-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CBinding::GetBindInfo(DWORD *pdwBINDF, BINDINFO *pbindinfo)
{
    PerfDbgLog(tagCBinding, this, "+CBinding::GetBindInfo");
    HRESULT hr = NOERROR;

    TransAssert((pdwBINDF && pbindinfo));

    *pdwBINDF = _grfBINDF;
    hr = CopyBindInfo(&_BndInfo, pbindinfo ); // Src->Dest

    PerfDbgLog1(tagCBinding, this, "-CBinding::GetBindInfo (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBinding::GetBindString
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
//  History:    11-07-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CBinding::GetBindString(ULONG ulStringType, LPOLESTR *ppwzStr, ULONG cEl, ULONG *pcElFetched)
{
    PerfDbgLog(tagCBinding, this, "+CTransaction::GetBindString");
    HRESULT hr = INET_E_USE_DEFAULT_SETTING;

    switch (ulStringType)
    {
    case BINDSTRING_HEADERS     :
        break;
    case BINDSTRING_ACCEPT_MIMES:
        hr = _pCTransData->GetAcceptMimes(ppwzStr,cEl, pcElFetched);
        break;
    case BINDSTRING_EXTRA_URL   :
        break;
    case BINDSTRING_LANGUAGE    :
        break;
    case BINDSTRING_USERNAME    :
        break;
    case BINDSTRING_PASSWORD    :
        break;
    case BINDSTRING_ACCEPT_ENCODINGS:
        break;
    case BINDSTRING_URL:
        if( _lpwszUrl )
        {
            LPWSTR pwzURL = NULL;
            pwzURL = OLESTRDuplicate(_lpwszUrl);
            if( pwzURL )
            {
                *ppwzStr = pwzURL,
                *pcElFetched = 1;
                hr = NOERROR;
            }
            else
            {
                hr = E_OUTOFMEMORY;
                *pcElFetched = 0;
            }
        }
        break;
    case BINDSTRING_USER_AGENT  :
    case BINDSTRING_POST_COOKIE :
    case BINDSTRING_POST_DATA_MIME:
        {
        hr = NOERROR;
        // QI on IBSC for interface
        if (_pBindInfo == NULL)
        {
            hr = LocalQueryInterface(IID_IInternetBindInfo, (void **)&_pBindInfo);
        }
        
        if ( (hr == NOERROR) && _pBindInfo)
        {
            hr = _pBindInfo->GetBindString(ulStringType, ppwzStr, cEl, pcElFetched);
        }
        }
        break;

    case BINDSTRING_IID:
        TransAssert(_piidRes);
        if (_piidRes)
        {
            hr = StringFromCLSID(*_piidRes, ppwzStr);
            if (pcElFetched)
            {
                *pcElFetched = (SUCCEEDED(hr)) ? (1) : (0);
            }
        }
        else
        {
            hr = E_UNEXPECTED;
            *pcElFetched = 0;
        }
        break;

    case BINDSTRING_FLAG_BIND_TO_OBJECT:
        *ppwzStr = new WCHAR[FLAG_BTO_STR_LENGTH];

        if (*ppwzStr)
        {
            if (_fBindToObject)
            {
                 StrCpyNW(*ppwzStr, FLAG_BTO_STR_TRUE,
                          lstrlenW(FLAG_BTO_STR_TRUE) + 1);
            }
            else
            {
                StrCpyNW(*ppwzStr, FLAG_BTO_STR_FALSE,
                         lstrlenW(FLAG_BTO_STR_FALSE) + 1);
            }
            *pcElFetched = 1;
            hr = S_OK;
        }
        else
        {
            *pcElFetched = 0;
            hr = E_OUTOFMEMORY;
        }
       
        break;

    case BINDSTRING_PTR_BIND_CONTEXT:
        if (!_pBndCtx) {
            hr = E_UNEXPECTED;
            *pcElFetched = 0;
        }
        else {
            *ppwzStr = new WCHAR[MAX_DWORD_DIGITS + 1];
            
            if (*ppwzStr)
            {
                wnsprintfW(*ppwzStr, MAX_DWORD_DIGITS, L"%ld", (DWORD_PTR)_pBndCtx);
                *pcElFetched = 1;
                _pBndCtx->AddRef();
                hr = S_OK;
            }
            else
            {
                *pcElFetched = 0;
                hr = E_OUTOFMEMORY;
            }
        }

        break;

    default:
        TransAssert((FALSE));
    }

    PerfDbgLog1(tagCBinding, this, "-CBinding::GetBindString (hr:%lx)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CBindProtocol::CBindProtocol
//
//  Synopsis:
//
//  Arguments:  [pUnk] --
//
//  Returns:
//
//  History:    11-11-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CBindProtocol::CBindProtocol() : _CRefs()
{
    _pUnk = NULL;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBindProtocol::~CBindProtocol
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    11-11-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CBindProtocol::~CBindProtocol()
{
}
//+---------------------------------------------------------------------------
//
//  Method:     CBindProtocol::QueryInterface
//
//  Synopsis:
//
//  Arguments:  [riid] --
//              [ppv] --
//
//  Returns:
//
//  History:    11-11-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBindProtocol::QueryInterface( REFIID riid, void **ppv )
{
    HRESULT     hr = NOERROR;

    PerfDbgLog2(tagCBinding, this, "+CBindProtocol::QueryInterface (%lx, %lx)", riid, ppv);

    if (   IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, IID_IBindProtocol) )
    {
        *ppv = (void FAR *)this;
        AddRef();
    }
    else
    {
        *ppv = NULL;
        hr = E_NOINTERFACE;
#if DBG==1
        //LPSTR lpszName = GetInterfaceName(riid);
        //DbgLog3(tagCBinding, this, "CBindProtocol::QI(pUnkObj) >%s< hr:%lx [%lx]", lpszName, hr, *ppv);
#endif // DBG==1

    }

    PerfDbgLog2(tagCBinding, this, "-CBindProtocol::QueryInterface (%lx)[%lx]", hr, *ppv);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBindProtocol::AddRef
//
//  Synopsis:
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    11-11-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CBindProtocol::AddRef( void )
{
    LONG lRet = _CRefs++;
    PerfDbgLog1(tagCBinding, this, "CBindProtocol::AddRef (%ld)", lRet);
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBindProtocol::Release
//
//  Synopsis:
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    11-11-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CBindProtocol::Release( void )
{
    PerfDbgLog(tagCBinding, this, "+CBindProtocol::Release");

    LONG lRet = --_CRefs;
    if (_CRefs == 0)
    {
        if (_pUnk)
        {
            PerfDbgLog(tagCBinding, this, "+CBindProtocol::Release _pUnk");
            _pUnk->Release();
            _pUnk = NULL;
            PerfDbgLog(tagCBinding, this, "-CBindProtocol::Release _pUnk");
        }

        delete this;
    }

    PerfDbgLog1(tagCBinding, this, "-CBindProtocol::Release (%ld)", lRet);
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBindProtocol::CreateBinding
//
//  Synopsis:
//
//  Arguments:  [url] --
//              [pBCtx] --
//              [ppBdg] --
//
//  Returns:
//
//  History:    11-11-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBindProtocol::CreateBinding(LPCWSTR szUrl, IBindCtx *pBCtx, IBinding **ppBdg)
{
    PerfDbgLog(tagCBinding, this, "+CBindProtocol::CreateBinding");
    HRESULT hr = NOERROR;

    BIND_OPTS           bindopts;
    CBinding            *pCBdg = NULL;
    VDATEPTROUT(ppBdg, LPVOID);
    VDATEIFACE(pBCtx);

    *ppBdg = NULL;
    // Get the bind options from the bind context
    bindopts.cbStruct = sizeof(BIND_OPTS);
    hr = pBCtx->GetBindOptions(&bindopts);
    ChkHResult(hr);

    hr = CBinding::Create(NULL, szUrl, pBCtx, IID_IStream, FALSE, &pCBdg );
    if (hr != NOERROR)
    {
        return hr;
    }

    // Start the download transaction
    {
        LPWSTR pwzExtra = NULL;
        hr = pCBdg->StartBinding(szUrl, pBCtx, IID_IStream, FALSE, &pwzExtra, NULL);
    }

    if (FAILED(hr))
    {
        // the transaction could not be started
        goto End;
    }

    // if the caller doesn't support IBindStatusCallback
    if ( pCBdg->IsAsyncBinding() )
    {
        // Async case: interface is passed on in OnDataAvailable
        *ppBdg = pCBdg;
    }

End:
    if (pCBdg)
    {
        pCBdg->Release();
    }

    PerfDbgLog1(tagCBinding, this, "-CBindProtocol::CreateBinding (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetObjectParam
//
//  Synopsis:
//
//  Arguments:  [pbc] --
//              [pszKey] --
//              [riid] --
//              [ppUnk] --
//
//  Returns:
//
//  History:    12-04-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT GetObjectParam(IBindCtx *pbc, LPOLESTR pszKey, REFIID riid, IUnknown **ppUnk)
{
    PerfDbgLog1(tagCBinding, NULL, "+GetObjectParam (IBindCtx:%lx)", pbc);
    HRESULT hr = E_FAIL;
    IUnknown *pUnk;

    // Try to get an IUnknown pointer from the bind context
    if (pbc)
    {
        hr = pbc->GetObjectParam(pszKey, &pUnk);
    }
    if (FAILED(hr))
    {
        *ppUnk = NULL;
    }
    else
    {
        // Query for riid
        hr = pUnk->QueryInterface(riid, (void **)ppUnk);
        pUnk->Release();

        if (FAILED(hr))
        {
            *ppUnk = NULL;
            DumpIID(riid);
        }
    }

    PerfDbgLog2(tagCBinding, NULL, "-GetObjectParam (IBindCtx:%lx, hr:%lx)", pbc, hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBinding::StartBinding
//
//  Synopsis:
//
//  Arguments:  [fBindToObject] --
//
//  Returns:
//
//  History:    12-04-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CBinding::StartBinding(LPCWSTR szUrl, IBindCtx *pbc, REFIID riid, BOOL fBindToObject, LPWSTR *ppwzExtra, LPVOID *ppv )
{
    PerfDbgLog(tagCBinding, this, "+CBinding::StartTransaction");
    //DbgLog1(tagCBindingErr, this, ">>> CBinding::Start(url=%ws)", szUrl);
    HRESULT hr;
    BOOL fBindingStarted = FALSE;

    UrlMkAssert((ppwzExtra));
    UrlMkAssert((_pBSCB == NULL));

    do
    {
        // Try to get an IBindStatusCallback  pointer from the bind context
        hr = GetObjectParam(pbc, REG_BSCB_HOLDER, IID_IBindStatusCallback, (IUnknown**)&_pBSCB);

        if  (FAILED(hr))
        {
            break;
        }
        UrlMkAssert(( (hr == NOERROR) && _pBSCB ));

        if  (_pBSCB == NULL)
        {
            hr = E_INVALIDARG;
            break;
        }
        _fBindToObject = fBindToObject;
        if (_fBindToObject)
        {
            _grfInternalFlags |= BDGFLAGS_PARTIAL;
            _piidRes = (IID *) &riid;

            // Get the bind options from the bind context
            _bindopts.cbStruct = sizeof(BIND_OPTS);
            hr = pbc->GetBindOptions(&_bindopts);
            if (FAILED(hr))
            {
                break;
            }
        }

        hr = CBindCtx::Create(&_pBndCtx, pbc);

        if (FAILED(hr))
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        {
            int cchWideChar;

            cchWideChar  = wcslen(szUrl) + 2;
            _lpwszUrl = (LPWSTR) new WCHAR [cchWideChar];
            if( !_lpwszUrl )
            {
                hr = E_OUTOFMEMORY;
                break;
            }
            wcscpy(_lpwszUrl, szUrl);
        }

        // call GetBindInfo
        _BndInfo.cbSize = sizeof(BINDINFO);

#if DBG==1
        if (_BndInfo.stgmedData.tymed != TYMED_NULL)
        {
            PerfDbgLog1(tagCBinding, this, "CBinding::StartTransaction ReleaseStgMedium (%lx)", _BndInfo.stgmedData);
        }
#endif // DBG==1

        // Make sure the BINDINFO is released and empty
        ReleaseBindInfo(&_BndInfo);

        _grfBINDF = 0;
        // call IBSC::GetBindInfo
        TransAssert((_BndInfo.stgmedData.tymed == TYMED_NULL));

        hr = CallGetBindInfo(&_grfBINDF, &_BndInfo);

        if( hr == NOERROR && !IsAsyncTransaction() )
        {
            // we need to turn off BINDF_ASYNCSTORAGE for sync binding
            _grfBINDF &= ~BINDF_ASYNCSTORAGE;
        }

        // this call should not fail
        if (FAILED(hr))
        {
            break;
        }

        // turn on direct read - not documented yet for APPs
        if (g_dwSettings & 0x20000000)
        {
            _grfBINDF |= BINDF_DIRECT_READ;
        }

        // check for extend binding (rosebud) 

        // get the bind option for extend binding (low bits)
        // the highest 16 bits is used for additional flags (e.g. wininet flag) 
        DWORD dwExtBindOption = _BndInfo.dwOptions & 0x0000ffff;
        if( dwExtBindOption && !fBindToObject)
        {
            // extend binding (rosebud)
            COInetSession* pSession = NULL;
            IOInetProtocol* pProt = NULL;

            hr = GetCOInetSession(0, &pSession, 0);
            if( hr != NOERROR )
            {
                break;
            }

            CLSID clsid = CLSID_NULL;
            hr = pSession->CreateFirstProtocol(
                _lpwszUrl, NULL, NULL, &pProt, &clsid, dwExtBindOption);
            pSession->Release();

            if( hr != NOERROR )
            {
                break;
            }


            StartParam param;
            param.iid =  riid;
            param.pIBindCtx = pbc; 
            param.pItf = NULL;

            // the interface ptr is returned via param.pItf
            hr = pProt->Start(_lpwszUrl, NULL, NULL, 0, (DWORD_PTR) &param );

            // release the pluggable protocol
            pProt->Release();

            if( hr == NOERROR && param.pItf )
            {
                // we are done, return the pointer
                *ppv = param.pItf;
                hr = INET_E_USE_EXTEND_BINDING;
                break; 
            }
            else
            if( hr != INET_E_USE_DEFAULT_PROTOCOLHANDLER )
            {
                break;
            }
                
            // continue with the normal binding process...
        }
        
        {
            // check for iid (only for BindToStorage)
            if( !fBindToObject && (IsRequestedIIDValid(riid) == FALSE) )
            {
                hr = E_INVALIDARG;
                break;
            }

            if (!IsOInetProtocol(pbc, szUrl))
            { 
                hr = INET_E_UNKNOWN_PROTOCOL;
                break;
            }


            DWORD dwObjectsFlags = OIBDG_APARTMENTTHREADED;
            if (_fBindToObject)
            {
                dwObjectsFlags |= BDGFLAGS_PARTIAL;
            }

            hr = GetTransactionObjects(_pBndCtx, _lpwszUrl, NULL, NULL, &_pOInetBdg, dwObjectsFlags, &_pCTransData);
        }

        if (hr == S_OK)
        {
            TransAssert((!_pCTransData));
            // create the transaction data object
            // Note: the transdat object has a refcount
            //       and must be released when done
            hr = CTransData::Create(_lpwszUrl, _grfBINDF, riid, _pBndCtx, _fBindToObject, &_pCTransData);
            if (SUCCEEDED(hr))
            {
                UrlMkAssert((_pCTransData != NULL && "CTransData invalid"));
                _pBndCtx->SetTransData(_pCTransData);
            }
        }
        else if (hr == S_FALSE)
        {
            // found an existing transaction
            UrlMkAssert((_pCTransData != NULL && "CTransData invalid"));
            _grfInternalFlags |= BDGFLAGS_ATTACHED;

            hr = _pCTransData->Initialize(_lpwszUrl, _grfBINDF, riid, _pBndCtx);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        if (FAILED(hr))
        {
            break;
        }

        // hand back to pointer to of extra info
        // to update the url
        *ppwzExtra = (_BndInfo.szExtraInfo) ? _BndInfo.szExtraInfo : NULL;

        if (_pCTransData->SetDataSink(_grfBINDF) == DataSink_Unknown)
        {
            hr = E_INVALIDARG;
        }

        if (   (_pCTransData->IsFileRequired())
            || (_fBindToObject && (IsKnownProtocol(_lpwszUrl) == DLD_PROTOCOL_NONE)) )
        {
            PerfDbgLog(tagCBinding, this, "---TURN ON NEEDFILE!---");
            // turn on flag to request file from protocol
            _grfBINDF |= BINDF_NEEDFILE;
        }

        if (SUCCEEDED(hr))
        {
            PerfDbgLog(tagCBinding, this, "---BINDF_FROMURLMON---");
            // turn on flag indicating the binding is from urlmon
            _grfBINDF |= BINDF_FROMURLMON;

            if( _fBindToObject )
            {
                _BndInfo.dwOptions |= BINDINFO_OPTIONS_BINDTOOBJECT;
            }
        }

        if (FAILED(hr))
        {
            break;
        }

        // send the OnStartBinding notification
        hr = CallOnStartBinding(NULL, this);
        fBindingStarted = TRUE;

        // check if the user did abort
        if (SUCCEEDED(hr))
        {
            OperationState opSt = GetOperationState();
            UrlMkAssert((opSt > OPS_Initialized));

            if (opSt == OPS_Abort)
            {
                hr = E_ABORT;
            }
        }

        BOOL fIsSync = (IsAsyncTransaction() == FALSE);

        if ( SUCCEEDED(hr))
        {
            // Note: check if url got redirected
            //       and report redirection url

            if (_pwzRedirectUrl)
            {
                PerfDbgLog1(tagCBinding, this, "StartTransaction OnProgress REDIRECTING _pBSCP(%lx)", _pBSCB);
                hr = CallOnProgress( 0, 0, BINDSTATUS_REDIRECTING,_pwzRedirectUrl );
                PerfDbgLog1(tagCBinding, this, "StartTransaction OnProgress REDIRECTING _pBSCP(%lx)", _pBSCB);
            }
        }

        if (    SUCCEEDED(hr))
        {
            DWORD dwBindFlags = OIBDG_APARTMENTTHREADED | PI_MIMEVERIFICATION | PI_DOCFILECLSIDLOOKUP;

            if (fIsSync)
            {
                dwBindFlags |= PI_SYNCHRONOUS;
            }

            if (_grfBINDF & BINDF_FREE_THREADED)
            {
                dwBindFlags &= ~OIBDG_APARTMENTTHREADED;
            }
            if (_fBindToObject)
            {
                dwBindFlags |= BDGFLAGS_PARTIAL | PI_CLASSINSTALL;
            }
            if (_grfInternalFlags & BDGFLAGS_ATTACHED)
            {
                dwBindFlags |= BDGFLAGS_ATTACHED;
            }
            
            if (_pOInetBdg)
            {
                // Just before starting the transaction give it the priority.

                IOInetPriority * pOInetPriority = NULL;
                if (_pOInetBdg->QueryInterface(IID_IOInetPriority, (void **) &pOInetPriority) == S_OK)
                {
                    pOInetPriority->SetPriority(_nPriority);
                    pOInetPriority->Release();
                }
            }

            if (   _pCTransData->InProgress() != S_FALSE
                || !_pCTransData->IsRemoteObjectReady() )
            {
                // guard the transaction object
                // the operation might complete synchronous
                _pOInetBdg->AddRef();

                hr = _pCTransData->OnStart(_pOInetBdg);
                TransAssert((hr == NOERROR));
                if (hr == NOERROR)
                {
                    hr = _pOInetBdg->Start(_lpwszUrl, (IOInetProtocolSink *) this, (IOInetBindInfo *) this, dwBindFlags, 0);
                }

                _pOInetBdg->Release();
            }
            else
            {
                // call OnProgress for mime type and filename
                //
                DWORD dwSize = _pCTransData->GetDataSize();
                if (_pCTransData->GetMimeType())
                {
                    OnTransNotification(BINDSTATUS_MIMETYPEAVAILABLE, dwSize, dwSize, (LPWSTR)_pCTransData->GetMimeType(), NOERROR);
                }
                if (_pCTransData->GetFileName())
                {
                    OnTransNotification(BINDSTATUS_CACHEFILENAMEAVAILABLE, dwSize, dwSize, _pCTransData->GetFileName(), NOERROR );
                }
                // report data - will call OnStopBinding
                OnTransNotification(BINDSTATUS_ENDDOWNLOADDATA, dwSize, dwSize,0 , NOERROR);
            }
        }

        break;
    } while (TRUE);

    if ( FAILED(hr))
    {
        // call OnStopBinding in case of error
        HRESULT hr1 = NOERROR;
        if ((_pBSCB != NULL) && fBindingStarted)
        {
            _hrBindResult = hr;
            hr1 = CallOnStopBinding(hr, NULL);
        }
        
        if (_pOInetBdg)
        {
            _pOInetBdg->Terminate(0);
            _pOInetBdg->Release();
            _pOInetBdg = NULL;
        }
    }

    PerfDbgLog(tagCBinding, this, "-CBinding::StartTransaction");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBinding::CompleteTransaction
//
//  Synopsis:
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
STDMETHODIMP CBinding::CompleteTransaction()
{
    HRESULT hr = NOERROR;
    PerfDbgLog(tagCBinding, this, "+CBinding::CompleteTransaction");

    if (_hrBindResult != NOERROR)
    {
        hr = _hrBindResult;
    }

    PerfDbgLog1(tagCBinding, this, "-CBinding::CompleteTransaction (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBinding::OnTransNotification
//
//  Synopsis:
//
//  Arguments:  [pCTP] --
//
//  Returns:
//
//  History:    12-11-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(BOOL) CBinding::OnTransNotification(BINDSTATUS NotMsg, DWORD dwCurrentSize, DWORD dwTotalSize,
                                                  LPWSTR pwzStr, HRESULT hrINet)
{
    PerfDbgLog(tagCBinding, this, "+CBinding::OnTransNotification");
    BOOL fRelease = FALSE;
    HRESULT hr = NOERROR;
    UrlMkAssert((_dwThreadId));

    if (   (   (_dwThreadId == GetCurrentThreadId())
            || (_grfBINDF & BINDF_FREE_THREADED))
        && (_grfInternalFlags & BDGFLAGS_NOTIFICATIONS)
        && (_pBSCB != NULL))
    {
        switch (NotMsg)
        {
        case BINDSTATUS_PROTOCOLCLASSID:
            UrlMkAssert((pwzStr));
            CLSIDFromString(pwzStr, &_clsidProtocol);
            break;

        case BINDSTATUS_MIMETYPEAVAILABLE:
            UrlMkAssert((pwzStr));
            _pCTransData->SetMimeType(pwzStr);
            CallOnProgress(0,0,BINDSTATUS_MIMETYPEAVAILABLE, pwzStr);
            break;

        case BINDSTATUS_CLASSIDAVAILABLE:
            UrlMkAssert((pwzStr));
            CLSIDFromString(pwzStr, &_clsidIn);
            break;

        case BINDSTATUS_IUNKNOWNAVAILABLE:
            if( _fBindToObject )
            {
                IUnknown                  *pUnk = NULL;

                _fClsidFromProt = TRUE;

                // the object should be instantiated now
                if (SUCCEEDED(hr))
                {
                    hr = _pBndCtx->GetObjectParam(SZ_IUNKNOWN_PTR, &pUnk);
                }

                if (SUCCEEDED(hr))
                {
                    hr = CallOnObjectAvailable(*_piidRes, pUnk);
                    pUnk->Release();
                }

                if (hr != NOERROR)
                {
                    SetInstantiateHresult(hr);
                }

                // return the download result in case if no error
                if (hr == NOERROR || GetHResult() != NOERROR)
                {
                    hr = GetHResult();
                }

                _hrBindResult = hr;
                hr = CallOnStopBinding(hr, NULL);
                fRelease = TRUE;
            }
            break;

        case BINDSTATUS_CLSIDCANINSTANTIATE:
            if(IsEqualGUID(_clsidIn, CLSID_NULL) && pwzStr)
            {
                CLSIDFromString(pwzStr, &_clsidIn);
            }
           
            if( _fBindToObject )
            {
                _fClsidFromProt = TRUE;

                // the object should be instantiated now
                hr = OnObjectAvailable(0,dwCurrentSize,dwTotalSize,TRUE);
                if (hr != NOERROR)
                {
                    SetInstantiateHresult(hr);
                }

                // return the download result in case if no error
                if (hr == NOERROR || GetHResult() != NOERROR)
                {
                    hr = GetHResult();
                }

                _hrBindResult = hr;
                hr = CallOnStopBinding(hr, NULL);
                fRelease = TRUE;
            }
            break;


        case BINDSTATUS_PROXYDETECTING :
            // indicate resolving proxyserver
            if (hrINet == NOERROR)
            {
                hr = CallOnProgress(0,0,BINDSTATUS_PROXYDETECTING,NULL );                
            }
            break;

        case BINDSTATUS_CACHEFILENAMEAVAILABLE :
            UrlMkAssert((pwzStr));
            _pCTransData->SetFileName(pwzStr);
            break;

        case BINDSTATUS_FINDINGRESOURCE   :
            // indicate resolving name - pass on server/proxy name
            if (hrINet == NOERROR)
            {
                hr = CallOnProgress(0,0,BINDSTATUS_FINDINGRESOURCE,pwzStr );
                PerfDbgLog1(tagCBinding, this, "OnTransNotification done on OnProgress _pBSCP(%lx)", _pBSCB);
            }
            break;

        case BINDSTATUS_SENDINGREQUEST  :
            // indicate resolving name - pass on server/proxy name
            if (hrINet == NOERROR)
            {
                hr = CallOnProgress(0,0,BINDSTATUS_SENDINGREQUEST,pwzStr );
                PerfDbgLog1(tagCBinding, this, "OnTransNotification done on OnProgress _pBSCP(%lx)", _pBSCB);
            }
            break;


        case BINDSTATUS_CONNECTING      :
            // inidicate progress connecting - pass on address
            if (hrINet == NOERROR)
            {
                hr = CallOnProgress(0,0,BINDSTATUS_CONNECTING, pwzStr);
                PerfDbgLog1(tagCBinding, this, "OnTransNotification done on OnProgress _pBSCP(%lx)", _pBSCB);
            }
            break;

        case BINDSTATUS_REDIRECTING     :
            if (hrINet == NOERROR)
            {
                TransAssert((pwzStr));
                PerfDbgLog1(tagCBinding, this, "OnTransNotification calling OnProgress _pBSCP(%lx)", _pBSCB);
                hr = CallOnProgress(
                            dwCurrentSize,   // ulProgress
                            dwTotalSize,     // ulProgressMax
                            BINDSTATUS_REDIRECTING,
                            pwzStr                  // new url
                            );

                PerfDbgLog1(tagCBinding, this, "OnTransNotification done on OnProgress _pBSCP(%lx)", _pBSCB);

                TransAssert((_lpwszUrl));

                {
                    DWORD dwLenOld = wcslen(_lpwszUrl);
                    DWORD dwLenNew = wcslen(pwzStr);
                    if (dwLenOld < dwLenNew)
                    {
                        delete _lpwszUrl;
                        _lpwszUrl = (LPWSTR) new WCHAR [dwLenNew + 1];

                    }
                    if (_lpwszUrl)
                    {
                        wcscpy(_lpwszUrl, pwzStr);
                    }
                }

            }
            break;

        case BINDSTATUS_ENDDOWNLOADDATA:
            PerfDbgLog(tagCBinding, this, "CBinding::OnTransNotification Notify_Done");
            // more work here for data notification
            //UrlMkAssert((errCode == INLERR_OK && "Notify_Done with Error"));

            if (hrINet == NOERROR)
            {
                PerfDbgLog1(tagCBinding, this, "OnTransNotification calling OnProgress _pBSCP(%lx)", _pBSCB);

                if (!_fSentFirstNotification)
                {
                    hr = CallOnProgress(dwCurrentSize, dwTotalSize,
                                        BINDSTATUS_BEGINDOWNLOADDATA, _lpwszUrl);

                    LPCWSTR pwzFilename = _pCTransData->GetFileName();

                    // a filename is not always available
                    if (pwzFilename)
                    {
                        hr = CallOnProgress(dwCurrentSize, dwTotalSize,
                                            BINDSTATUS_CACHEFILENAMEAVAILABLE, pwzFilename);
                    }
                }

                hr = CallOnProgress(dwCurrentSize,dwTotalSize,
                                        BINDSTATUS_ENDDOWNLOADDATA,_lpwszUrl);

                PerfDbgLog1(tagCBinding, this, "OnTransNotification done on OnProgress _pBSCP(%lx)", _pBSCB);

                // In some cases (e.g. data is in cache or progress notifications
                // are disbled) we may not get progress notifications
                // We might directly get the DONE notitification

                // In anycase we do not want to send data notification if it is a
                if (!_fBindToObject)
                {
                    hr = OnDataNotification(0,dwCurrentSize,dwTotalSize, TRUE);
                }
                else
                {
                    // the object should be instanciate now
                    hr = OnObjectAvailable(0,dwCurrentSize,dwTotalSize,TRUE);
                    if (hr != NOERROR)
                    {
                        SetInstantiateHresult(hr);
                    }
                }
            }

            // return the download result in case if no error
            if (hr == NOERROR || GetHResult() != NOERROR)
            {
                hr = GetHResult();
            }

            PerfDbgLog2(tagCBinding, this, "OnTransNotification calling OnStopBinding _pBSCP(%lx) HR:%lx", _pBSCB, hr);

            _hrBindResult = hr;
            hr = CallOnStopBinding(hr, NULL);
            fRelease = TRUE;

            PerfDbgLog1(tagCBinding, this, "OnTransNotification done on OnStopBinding _pBSCP(%lx)", _pBSCB);
            break;

        case BINDSTATUS_BEGINDOWNLOADDATA:
        case BINDSTATUS_DOWNLOADINGDATA:


            PerfDbgLog(tagCBinding, this, "CBinding::OnTransNotification Notify_Update");

            // Call OnProgress once if data are from cache
            if (!_fSentFirstNotification)
            {
                if (_pCTransData->IsFromCache())
                {
                    hr = CallOnProgress(0,0,BINDSTATUS_USINGCACHEDCOPY, NULL);
                }
            }

            hr = CallOnProgress(dwCurrentSize, dwTotalSize,
                        (!_fSentFirstNotification) ? BINDSTATUS_BEGINDOWNLOADDATA : BINDSTATUS_DOWNLOADINGDATA,
                        _lpwszUrl);

            if (!_fSentFirstNotification)
            {
                LPCWSTR pwzFilename = _pCTransData->GetFileName();

                // a filename is not always available
                if (pwzFilename)
                {
                    hr = CallOnProgress(dwCurrentSize, dwTotalSize,
                                        BINDSTATUS_CACHEFILENAMEAVAILABLE, pwzFilename);
                }
            }


            PerfDbgLog1(tagCBinding, this, "OnTransNotification done on OnProgress _pBSCP(%lx)", _pBSCB);

            if (!_fBindToObject)
            {
                OnDataNotification(0,dwCurrentSize,dwTotalSize, FALSE);
            }
            else
            {

                // 
                // here is the hack for ms-its:
                // if they are sending dwCurrent==dwTotal, we need to flip
                // the FALSE to TRUE in order to make word/excel doc host
                // working under IE5 (IE5 #71203)
                //
                BOOL fFullData = FALSE;
                if( dwCurrentSize == dwTotalSize &&
                    dwCurrentSize &&
                    _lpwszUrl &&
                    wcslen(_lpwszUrl) > 7 &&
                    !StrCmpNIW(_lpwszUrl, L"ms-its:", 7) )
                {
                    fFullData = TRUE;
                }


                // check here if the object can be create already
                //hr = OnObjectAvailable(pCTP, FALSE);
                hr = OnObjectAvailable(0,dwCurrentSize,dwTotalSize, fFullData);

                // mark the transobject for completion
                if (hr == S_OK)
                {
                    hr = CallOnStopBinding(NOERROR, NULL);
                    fRelease = TRUE;
                }
                else if ((hr != S_OK ) && (hr != S_FALSE))
                {
                    _hrBindResult = hr;
                    hr = CallOnStopBinding(hr, NULL);
                    fRelease = TRUE;
                }

            }
            break;

        case BINDSTATUS_ERROR:

            PerfDbgLog2(tagCBinding, this, "CBinding::OnTransNotification Notify_Error[hr%lx, dwResutl;%lx]", _hrBindResult, _dwBindError);

            // call StopBinding witht error code
            UrlMkAssert(( (_hrBindResult != NOERROR) && (_dwBindError != 0) ));
            hr = CallOnStopBinding(_hrBindResult, NULL);
            fRelease = TRUE;
            break;

        case BINDSTATUS_RESULT:
            PerfDbgLog2(tagCBinding, this, "CBinding::OnTransNotification Notify_Error[hr%lx, dwResutl;%lx]", _hrBindResult, _dwBindError);
            if( _hrBindResult == INET_E_REDIRECT_TO_DIR )
            {
                hr = CallOnStopBinding(_hrBindResult, _pwzResult);
            }
            else
            {
                hr = CallOnStopBinding(_hrBindResult, NULL);
            }
            fRelease = TRUE;
            break;

        case BINDSTATUS_DECODING:
            hr = CallOnProgress(0,0,BINDSTATUS_DECODING,pwzStr );
            break;
            
        case BINDSTATUS_LOADINGMIMEHANDLER:
            hr = CallOnProgress(0,0,BINDSTATUS_LOADINGMIMEHANDLER,pwzStr);
            break;

        case BINDSTATUS_INTERNAL:
        case BINDSTATUS_INTERNALASYNC:
            break;

        case BINDSTATUS_CONTENTDISPOSITIONATTACH:
            _fForceBindToObjFail = TRUE;
            hr = CallOnProgress(0,0,BINDSTATUS_CONTENTDISPOSITIONATTACH, NULL);
            break;

        case BINDSTATUS_ACCEPTRANGES:
            _fAcceptRanges= TRUE;
            break;

        default:
            DbgLog1(tagCBindingErr, this, "CBinding::OnTransNotification Unknown (NMsg:%lx)", NotMsg);
            UrlMkAssert((FALSE));
            break;
        }
    }

    PerfDbgLog(tagCBinding, this, "-CBinding::OnTransNotification");
    return fRelease;
}




//+---------------------------------------------------------------------------
//
//  Method:     CBinding::OnDataNotification
//
//  Synopsis:
//
//  Arguments:  [pCTP] --
//              [fLastNotification] --
//
//  Returns:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBinding::OnDataNotification(DWORD grfBSCF, DWORD dwCurrentSize, DWORD dwTotalSize, BOOL fLastNotification)
{
    HRESULT hr = NOERROR;
    PerfDbgLog3(tagCBinding, this, "+CBinding::OnDataNotification (dwLastSize:%ld, dwCurrentSize:%ld, dwTotalSize:%ld)",
        _dwLastSize, dwCurrentSize, dwTotalSize);
    STGMEDIUM *pStgMed = 0;
    FORMATETC *pFmtETC = 0;

    // We shouldn't have less data than last time
    #if 0
    UrlMkAssert((   (dwTotalSize == 0)
                 || (dwTotalSize != 0 && dwCurrentSize > _dwLastSize)
                 || (dwTotalSize == dwCurrentSize && dwCurrentSize == _dwLastSize)
                 ));
    #endif // 0
    
    // must be BindToStorage scenario
    UrlMkAssert((!_fBindToObject));
    // should never end up here after the last data notification
    //UrlMkAssert((!_fSentLastNotification));

    if (   ((dwCurrentSize > 0) || fLastNotification)
        && (!_fSentLastNotification))
    {
        grfBSCF = 0;


        // Check if this will be the first data notification
        if (!_fSentFirstNotification)
        {
            grfBSCF |= BSCF_FIRSTDATANOTIFICATION;
            _fSentFirstNotification = TRUE;
        }

        if (fLastNotification)
        {
            // Check if this will be the last data notification
            grfBSCF |= BSCF_LASTDATANOTIFICATION;
            _fSentLastNotification = TRUE;
        }
        // all other notifications are intermediate
        if (grfBSCF == 0)
        {
            grfBSCF |= BSCF_INTERMEDIATEDATANOTIFICATION;
        }

        // get the stgmed for this
        if (_fCreateStgMed == FALSE)
        {
            hr = _pCTransData->GetData(&pFmtETC, &pStgMed, grfBSCF);
            if (hr == S_OK)
            {
                //_fCreateStgMed = TRUE;
                TransAssert((pStgMed));
            }
        }
        //
        // hold on to the stream or storage
        //
        if (   (hr == S_OK)
            && (_pUnkObject == NULL)
            && (   pStgMed->tymed == TYMED_ISTREAM
                || pStgMed->tymed == TYMED_ISTORAGE))
        {
            _pUnkObject = pStgMed->pstm;
            _pUnkObject->AddRef();
        }

        if ( hr == S_OK && fLastNotification )
        {
            _pCTransData->OnEndofData();
        }

        if (hr == S_OK)
        {
            PerfDbgLog5(tagCBinding, this,
                    ">>> %lx::OnDataNotification (Options:%ld,Size:%ld,FmtEtc:%lx,StgMed:%lx)",
                    _pBSCB, grfBSCF, dwCurrentSize, pFmtETC, pStgMed);

            UrlMkAssert((grfBSCF != 0));

            hr = CallOnDataAvailable(grfBSCF, dwCurrentSize, pFmtETC, pStgMed);

            if (pStgMed)
            {
               ReleaseStgMedium(pStgMed);
            }
        }
     }

    _dwLastSize = dwCurrentSize;

    PerfDbgLog2(tagCBinding, this, "-CBinding::OnDataNotification (hr:%lx, dwLastSize:%ld)", hr, _dwLastSize);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBinding::OnObjectAvailable
//
//  Synopsis:
//
//  Arguments:  [pCTP] --
//              [fLastNotification] --
//
//  Returns:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBinding::OnObjectAvailable(DWORD grfBSCF, DWORD dwCurrentSize, DWORD dwTotalSize, BOOL fLastNotification)
{
    HRESULT hr = NOERROR;
    PerfDbgLog3(tagCBinding, this, "+CBinding::OnObjectAvailable (dwCurrentSize:%ld, dwTotalSize:%ld, LastNotification:%d)",
                                    dwCurrentSize, dwTotalSize, fLastNotification);
    CLSID clsid = CLSID_NULL;

    // We shouldn't have less data than last time
    UrlMkAssert((   (dwTotalSize == 0)
                 || (dwTotalSize != 0 && dwCurrentSize != 0)
                 ));

    // must be BindToObject scenario
    UrlMkAssert((_fBindToObject));

    if (!fLastNotification && !_fSentFirstNotification)
    {
        // check if we've instantiated the object yet
        if (_pUnkObject == NULL)
        {
#ifdef _ZEROIMPACT
            // this code to check for already have clsid is from below (on LastNotification)
            // but didn't exist here previously
            // was there a reason for that?
            // t-mattp 8-7-98
            if( _fClsidFromProt && !IsEqualGUID(_clsidIn, CLSID_NULL) )
            {
                clsid = _clsidIn;
                hr = NOERROR;
            }
            else
#endif
                hr = _pCTransData->GetClassID(_clsidIn, &clsid);

            if (FAILED(hr)) {

                hr = InstallIEFeature();

                if (SUCCEEDED(hr))
                    hr = _pCTransData->GetClassID(_clsidIn, &clsid);
            }

            if( _fForceBindToObjFail )
            {
                hr = REGDB_E_CLASSNOTREG;
            }
            

            if (hr == S_OK)
            {
                // instantiate the object now and call pass it on in OnDataAvailable
                hr = InstantiateObject(&clsid, *_piidRes, &_pUnkObject, FALSE);

                if (hr == S_FALSE)
                {
                    // switch to datasink file
                    _pCTransData->SwitchDataSink(DataSink_File);

                    if( _pCTransData->IsEOFOnSwitchSink() )
                    {
                        hr = ObjectPersistMnkLoad(_pUnkObject, _fLocal, TRUE);

                        if (hr == E_NOINTERFACE)
                        {
                            hr = ObjectPersistFileLoad(_pUnkObject);
                        }
                    }
                }

                // Don't send notifications after the 'last' one.
                if ((hr == S_OK) && (_pUnkObject != NULL))
                {
                    hr = CallOnObjectAvailable(*_piidRes,_pUnkObject);

                    // release the object in case transaction is async
                    // the object is passed back via *ppvObj in sync case
                    if (IsAsyncTransaction())
                    {
                        _pUnkObject->Release();
                        _pUnkObject = NULL;
                    }

                    UrlMkAssert((SUCCEEDED(hr)));
                    hr = S_OK;
                }
                // keep the object until all data here

            }
            else if (hr == S_FALSE)
            {
                //
                // switch the datasink to file
                //
                _pCTransData->SwitchDataSink(DataSink_File);
            }
            else
            {

                hr = REGDB_E_CLASSNOTREG;
            }
        }
        else
        {
            // we got the object loaded but have to wait until all data
            // are available
            hr = S_FALSE;
        }

        if (!_fSentFirstNotification)
        {
            _fSentFirstNotification = TRUE;
        }

    }
    else if (fLastNotification)
    {
        UrlMkAssert((!_fSentLastNotification));
        DWORD grfBSCF = 0;

        // Check if this will be the first data notification

        // Check if this will be the last data notification
        if (fLastNotification)
        {
            grfBSCF |= BSCF_LASTDATANOTIFICATION;
        }

        if (_pUnkObject == NULL)
        {
            if( _fClsidFromProt && !IsEqualGUID(_clsidIn, CLSID_NULL) )
            {
                clsid = _clsidIn;
                hr = NOERROR;
            }
            else
            {
                hr = _pCTransData->GetClassID(_clsidIn, &clsid);
            }

            if (FAILED(hr)) {
                hr = InstallIEFeature();

                if (SUCCEEDED(hr))
                    hr = _pCTransData->GetClassID(_clsidIn, &clsid);
            }

            if( _fForceBindToObjFail )
            {
                hr = REGDB_E_CLASSNOTREG;
            }
            
            if (FAILED(hr))
            {
                hr = REGDB_E_CLASSNOTREG;
            }
            else
            {
                // instanciate the object now and call pass it on in OnDataAvailable
                if( _fClsidFromProt )
                {
                    hr = CreateObject(&clsid, *_piidRes, &_pUnkObject);
                }
                else
                {
                    hr = InstantiateObject(&clsid, *_piidRes, 
                                            &_pUnkObject, TRUE);
                }
            }
        }
        else
        {
            CallOnProgress( 0, 0, BINDSTATUS_BEGINSYNCOPERATION, 0 );

            hr = ObjectPersistMnkLoad(_pUnkObject,_fLocal,TRUE);
            if (hr != NOERROR && hr != E_ABORT)
            {
                // if all bits are available
                hr = ObjectPersistFileLoad(_pUnkObject);
            }

            CallOnProgress( 0, 0, BINDSTATUS_ENDSYNCOPERATION, 0 );

        }

        // Don't send notifications after the 'last' one.
        if (   SUCCEEDED(hr)
            && (_pUnkObject != NULL)
            && !_fSentLastNotification)
        {

            BOOL  bRegisteredTransactionData = FALSE;

            // Note: register the transaction object only
            // in case we are asked to transfer it to the
            // new process/thread where no new download
            // should be started
            if (_grfBINDF & BINDF_COMPLETEDOWNLOAD)
            {
                hr = _pBndCtx->RegisterObjectParam(SZ_TRANSACTIONDATA, (ITransactionData *)_pCTransData);
                if (SUCCEEDED(hr))
                {
                    bRegisteredTransactionData = TRUE;
                }

                PerfDbgLog2(tagCBinding, this, "=== CBinding::OnObjectAvailable RegisterObjectParam SZ_TRANSACTIONDATA: pbndctx:%lx, hr:%lx)", _pBndCtx, hr);
            }

            TransAssert((hr == NOERROR));

            hr = CallOnObjectAvailable(*_piidRes,_pUnkObject);

            if (bRegisteredTransactionData)
            {
                _pBndCtx->RevokeObjectParam(SZ_TRANSACTIONDATA);
            }

            // release the object
            if (IsAsyncTransaction())
            {
                _pUnkObject->Release();
                _pUnkObject = NULL;
            }

            UrlMkAssert((SUCCEEDED(hr)));
        }
        else
        {
            // why did it fail
        }

        if (grfBSCF & BSCF_LASTDATANOTIFICATION)
        {
            _fSentLastNotification = TRUE;
        }
    }
    else
    {
        hr = S_FALSE;
    }

    _dwLastSize = dwCurrentSize;
    PerfDbgLog1(tagCBinding, this, "-CBinding::OnObjectAvailable (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBinding::InstallIEFeature
//
//  Synopsis:
//              called when you can't by comventional registry lookup means
//              find a clsid for a given mime type
//              This code then checks to see if this is an IE feature
//              and if so installs it by calling the IE JIT API.
//
//  Arguments: 
//
//  Returns:
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBinding::InstallIEFeature()
{
    PerfDbgLog(tagCBinding, this, "+CBinding::InstallIEFeature");
    HRESULT     hr  = INET_E_CANNOT_INSTANTIATE_OBJECT;
    uCLSSPEC classpec;
    IWindowForBindingUI *pWindowForBindingUI = NULL;
    HWND hWnd = NULL;
    REFGUID rguidReason = IID_ICodeInstall;
    LPCWSTR pwszMimeType = _pCTransData->GetMimeType();
    DWORD dwJITFlags;

    // BUGBUG: Cannot extern vwzApplicationCDF from datasnif.cxx
    // For some reason, vwzApplicationCDF contains bogus data in some scenarios!
    static WCHAR vwzAppCDF[] = L"application/x-cdf";

    if (!pwszMimeType) {
        return hr;
    }

    if (SUCCEEDED(IsMimeHandled(pwszMimeType))) {
        
        // if the mime has been handled by an EXE out of proc
        // then fail to instantiate the obj. shdocvw will call 
        // shellexecute on this url which will succeed.
        // we assume here that we enter InstallIEfeature only
        // after a conversion MimeToClsid has failed!

        hr  = INET_E_CANNOT_INSTANTIATE_OBJECT;
        return hr;
    }

    CallOnProgress( 0, 0, BINDSTATUS_BEGINSYNCOPERATION, 0 );

    // Get IWindowForBindingUI ptr
    hr = _pBSCB->QueryInterface(IID_IWindowForBindingUI,
            (LPVOID *)&pWindowForBindingUI);

    if (FAILED(hr)) {
        IServiceProvider *pServProv;
        hr = _pBSCB->QueryInterface(IID_IServiceProvider,
            (LPVOID *)&pServProv);

        if (hr == NOERROR) {
            pServProv->QueryService(IID_IWindowForBindingUI,IID_IWindowForBindingUI,
                (LPVOID *)&pWindowForBindingUI);
            pServProv->Release();
        }
    }

    hr = INET_E_CANNOT_INSTANTIATE_OBJECT; // init to fail

    // get hWnd
    if (pWindowForBindingUI) {
        pWindowForBindingUI->GetWindow(rguidReason, &hWnd);
        pWindowForBindingUI->Release();

        QUERYCONTEXT qc;

        memset(&qc, 0, sizeof(qc));

        // fill in the minimum version number of the component you need
        //qc.dwVersionHi = 
        //qc.dwVersionLo = 


        classpec.tyspec=TYSPEC_MIMETYPE;
        classpec.tagged_union.pMimeType=(LPWSTR)pwszMimeType;

        if (pwszMimeType) {
            dwJITFlags = (!StrCmpIW(pwszMimeType, vwzAppCDF)) ?
                         (FIEF_FLAG_FORCE_JITUI) : (0);
        }
        else {
            dwJITFlags = 0;
        }

        hr = FaultInIEFeature(hWnd, &classpec, &qc, dwJITFlags);

    }

    CallOnProgress( 0, 0, BINDSTATUS_ENDSYNCOPERATION, 0 );

    PerfDbgLog1(tagCBinding, this, "-CBinding::InstallIEFeature (hr:%lx)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CBinding::InstantiateObject
//
//  Synopsis:
//
//  Arguments:  [pclsid] --
//              [riidResult] --
//              [ppUnk] --
//
//  Returns:
//
//  History:    1-12-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBinding::InstantiateObject(CLSID *pclsid, REFIID riidResult, IUnknown **ppUnk,BOOL fFullyAvailable)
{
    PerfDbgLog(tagCBinding, this, "+CBinding::InstantiateObject");
    HRESULT     hr;
    IUnknown    *pUnk = NULL;
    LPOLESTR    pszStr = NULL;
    BOOL        fLocal = FALSE;

    //call OnProgress with CLASSID_AVAILABLE
    hr = StringFromCLSID(*pclsid, &pszStr);
    if (hr == NOERROR)
    {
        PerfDbgLog1(tagCBinding, this, "CBinding::InstantiateObject (Class ID:%ws)", pszStr);
        hr = CallOnProgress( 0, 0, BINDSTATUS_CLASSIDAVAILABLE, pszStr );
        if(hr == E_ABORT)
            SetOperationState(OPS_Abort);
        else
            UrlMkAssert((hr == NOERROR));
    }

    if (GetOperationState() == OPS_Abort)
    {
        // stop now - the client aborted the operation
        hr = E_ABORT;
    }
    else
    {

        DumpIID(riidResult);


        CallOnProgress( 0, 0, BINDSTATUS_BEGINSYNCOPERATION, 0 );

        //  If CLSID_MsHtml object had to be created to honor scripting access
        //  before OnObjectAvailable, BINDSTATUS_CLASSIDAVAILABLE OnProgress
        //  message is signal to register such an object in bind context

        hr = _pBndCtx->GetObjectParam(L"__PrecreatedObject", &pUnk);
        if (FAILED(hr))
        {
            // call OleAutoConvert
            {
                CLSID clsidIn = *pclsid;
                CLSID clsidOut;
                hr = OleGetAutoConvert(clsidIn, &clsidOut);
                if (hr == S_OK)
                {
                    *pclsid = clsidOut;
                }
            }

            if (_grfBINDF & BINDF_GETCLASSOBJECT)
            {
                // Just want the class object
                hr = CoGetClassObject(*pclsid, CLSCTX_INPROC_SERVER,
                                      NULL, riidResult, (void **)&pUnk);
                if (FAILED(hr))
                {
                    hr = CoGetClassObject(
                            *pclsid, CLSCTX_LOCAL_SERVER,
                            NULL, riidResult, (void **)&pUnk);

                    if (FAILED(hr))
                    {
                        hr = CoGetClassObject(
                            *pclsid, CLSCTX_INPROC_HANDLER,
                            NULL, riidResult, (void **)&pUnk);
                    }
                }
            }
            else
            {
                hr = CoCreateInstance(*pclsid, NULL, CLSCTX_INPROC_SERVER,
                                    riidResult, (void**)&pUnk);

                if (FAILED(hr))
                {
                    DbgLog1(tagCBindingErr, this, "=== CBinding::InstantiateObject InProcServer (hr:%lx)", hr);
                    hr = CoCreateInstance(*pclsid, NULL, CLSCTX_LOCAL_SERVER,
                                        riidResult, (void**)&pUnk);
                    _fLocal = fLocal = TRUE;

                    if (FAILED(hr))
                    {
                        DumpIID(*pclsid);
                        DbgLog1(tagCBindingErr, this, "=== CBinding::InstantiateObject LocalServer (hr:%lx)", hr);

                        hr = CoCreateInstance(*pclsid, NULL, CLSCTX_INPROC_HANDLER, riidResult, (void**)&pUnk);

                        _fLocal = fLocal = FALSE;
                        if( FAILED(hr))
                        {
                            DbgLog1(tagCBindingErr, this, "=== CBinding::InstantiateObject InProcHandler (hr:%lx)", hr);
                        }
                    }
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            *ppUnk = pUnk;

            if (   _grfBINDF & BINDF_COMPLETEDOWNLOAD
                && IsEqualGUID(*pclsid, CLSID_MsHtml)
                && fFullyAvailable == FALSE)
            {
                hr = S_FALSE;
            }
            else
            {

                // S_FALSE means try later when all data are available
                hr = ObjectPersistMnkLoad(pUnk,_fLocal,fFullyAvailable);

                if (hr == E_NOINTERFACE)
                {
                    if (fFullyAvailable)
                    {
                        hr = ObjectPersistFileLoad(_pUnkObject);
                    }
                    else
                    {
                        // call PersistFile::Load later
                        hr = S_FALSE;
                    }
                }
                else if (hr == S_FALSE)
                {
                    // lock the request
                    _pOInetBdg->LockRequest(0);
                    _fCompleteDownloadHere = TRUE;
                }
                else if (hr != NOERROR && hr != E_ABORT)
                {
                    //
                    // your last chance of being loaded...
                    // this was not needed for IE4 since wininet
                    // ALWAYS return async, start from IE5, wininet
                    // will return SYNC if the file is in cache
                    //
                    if (fFullyAvailable)
                    {
                        HRESULT hr2 = ObjectPersistFileLoad(_pUnkObject);
                        if( hr2 == NOERROR )
                        {
                            // if we succeeded here, needs to return NOERROR
                            hr = hr2;
                        }
                    }
                }
                // else pass back error
            }
        }
        else
        {
            SetInstantiateHresult(hr);
            hr = INET_E_CANNOT_INSTANTIATE_OBJECT;

        }
        CallOnProgress( 0, 0, BINDSTATUS_ENDSYNCOPERATION, 0 );
    }

    if (pszStr)
    {
        delete pszStr;
    }
    PerfDbgLog1(tagCBinding, this, "-CBinding::InstantiateObject (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBinding::ObjectPersistMnkLoad
//
//  Synopsis:
//
//  Arguments:  [pUnk] --
//
//  Returns:
//
//  History:    2-07-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBinding::ObjectPersistMnkLoad(IUnknown *pUnk, BOOL fLocal, BOOL fFullyAvailable)
{
    PerfDbgLog(tagCBinding, this, "+CBinding::ObjectPersistMnkLoad");
    HRESULT          hr;
    IPersistMoniker *pPersistMk = NULL;
    BIND_OPTS        bindopts;
    BOOL             bRegisteredTransactionData = FALSE;

    if (_grfBINDF & BINDF_GETCLASSOBJECT)
    {
        // GetClassObj, which means there is no need to PersistMnkLoad
        PerfDbgLog(tagCBinding, this, " ObjectPersistMnkLoad : GETCLASSOBJ");
        hr = NOERROR;
        goto exit;
    }

    hr = pUnk->QueryInterface(IID_IPersistMoniker, (void**)&pPersistMk);
    if (hr == NOERROR)
    {
        TransAssert((pPersistMk != NULL));
        if (!fLocal)
        {
            hr = _pBndCtx->RegisterObjectParam(SZ_BINDING, (IBinding *)this);
        }

        IUnknown *pUnk = 0;

        // remove the current bindstatuscallback
        hr = _pBndCtx->GetObjectParam(REG_BSCB_HOLDER, &pUnk);
        TransAssert((hr == NOERROR));

        hr = _pBndCtx->RevokeObjectParam(REG_BSCB_HOLDER);
        TransAssert((hr == NOERROR));

        bindopts = _bindopts;

        TransAssert(( bindopts.grfMode & (STGM_READWRITE | STGM_SHARE_EXCLUSIVE) ));

        if ( !(bindopts.grfMode & (STGM_READWRITE | STGM_SHARE_EXCLUSIVE)) )
        {
            bindopts.grfMode |= (STGM_READWRITE | STGM_SHARE_EXCLUSIVE);
        }

        // Note:
        // switch of flag do not receive notifications on this cbinding
        // BindToStorage synchronous might be called
        _grfInternalFlags &= ~BDGFLAGS_NOTIFICATIONS;

        if ( (fFullyAvailable)
             && ( ( !(_grfBINDF & BINDF_COMPLETEDOWNLOAD)
                    && _fCompleteDownloadHere )
                  ||
                  (_fLocal) 
                )
           )
        {
            _grfBINDF |= BINDF_COMPLETEDOWNLOAD;
            hr = _pBndCtx->RegisterObjectParam(SZ_TRANSACTIONDATA, (ITransactionData *)_pCTransData);
            if (SUCCEEDED(hr))
            {
                bRegisteredTransactionData = TRUE;
            }
            PerfDbgLog2(tagCBinding, this, "=== CBinding::OnObjectAvailable RegisterObjectParam SZ_TRANSACTIONDATA: pbndctx:%lx, hr:%lx)", _pBndCtx, hr);

            // swtich the _ds back to StreamOnFile so that the BindToStorage
            // can get the IStream from pbc (inproc server only)
            if( !_fLocal )
            {
                _pCTransData->SetFileAsStmFile();
            } 
        }


        LPCWSTR pwszMimeType = _pCTransData->GetMimeType();
        if( !fFullyAvailable    && 
            pwszMimeType        &&
            !StrCmpNIW( pwszMimeType, L"application/pdf", 15) )
        {
            // let's find out we are dealing with Acrobat 3.02 and above
            if( _fAcceptRanges && PDFNeedProgressiveDownload() )
            {
                // this is 3.02 and above, go ahead call PMK::Load
                hr = pPersistMk->Load(
                    fFullyAvailable, GetMoniker(), _pBndCtx, bindopts.grfMode);
                
            }
            else
            {
                //
                // this is 3.0 and 3.01, 
                // or we are dealing with server does not support Range
                // don't call PMK::Load until fully available 
                //
                hr = S_FALSE;
            }
        }
        else
        {
            hr = pPersistMk->Load(fFullyAvailable, GetMoniker(), _pBndCtx, bindopts.grfMode);
        }

        if (bRegisteredTransactionData)
        {
            _pBndCtx->RevokeObjectParam(SZ_TRANSACTIONDATA);
        }

        if (FAILED(hr))
        {
            if (hr == E_FAIL)
            {
                hr = CO_E_SERVER_EXEC_FAILURE;
            }
        }

        // Note: OnStopBinding is still called even
        // even the download finished due sync BindToStorage
        // turn the flag back on
        _grfInternalFlags |= BDGFLAGS_NOTIFICATIONS;

        if (!fLocal)
        {
            _pBndCtx->RevokeObjectParam(SZ_BINDING);
        }

        if (pUnk)
        {
            _pBndCtx->RegisterObjectParam(REG_BSCB_HOLDER, pUnk);
            pUnk->Release();
        }

        pPersistMk->Release();
    }

exit:
    PerfDbgLog1(tagCBinding, this, "-CBinding::ObjectPersistMnkLoad (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBinding::ObjectPersistFileLoad
//
//  Synopsis:
//
//  Arguments:  [pUnk] --
//
//  Returns:
//
//  History:    2-07-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBinding::ObjectPersistFileLoad(IUnknown *pUnk)
{
    HRESULT hr;
    PerfDbgLog1(tagCBinding, this, "+CBinding::ObjectPersistFileLoad (filename:%ws)",  GetFileName());

    IPersistFile *pPersistFile = NULL;

    hr = pUnk->QueryInterface(IID_IPersistFile, (void**)&pPersistFile);
    if (hr == NOERROR)
    {
        hr = pPersistFile->Load(GetFileName(), 0);
        if (hr != NOERROR)
        {
            SetInstantiateHresult(hr);
            hr = INET_E_CANNOT_LOAD_DATA;
        }
        PerfDbgLog1(tagCBinding, this, "=== CBinding::ObjectPersistFileLoad (Load returned:%lx)", hr);
        pPersistFile->Release();
    }

    PerfDbgLog1(tagCBinding, this, "-CBinding::ObjectPersistFileLoad (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBinding::CallGetBindInfo
//
//  Synopsis:
//
//  Arguments:  [grfBINDINFOF] --
//              [pBdInfo] --
//
//  Returns:
//
//  History:    2-07-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBinding::CallGetBindInfo(DWORD *grfBINDINFOF, BINDINFO *pBdInfo)
{
    HRESULT hr = E_FAIL;
    UrlMkAssert((grfBINDINFOF != NULL));
    PerfDbgLog1(tagCBinding, this, "+CBinding::CallGetBindInfo (grfBINDINFOF:%ld)", *grfBINDINFOF);

    if (GetOperationState() == OPS_Initialized)
    {
        hr = _pBSCB->GetBindInfo(grfBINDINFOF, pBdInfo);
        SetOperationState(OPS_GetBindInfo);
    }
    else
    {
        UrlMkAssert((GetOperationState() == OPS_Initialized));
    }

    if (   (*grfBINDINFOF & BINDF_ASYNCSTORAGE)
        && (*grfBINDINFOF & BINDF_PULLDATA) )
    {
        PerfDbgLog2(tagCBinding, this, "=== grfBINDINFOF:%lx, (%s)", *grfBINDINFOF, "BINDF_ASYNCSTORAGE | BINDF_PULLDATA");
    }

    PerfDbgLog2(tagCBinding, this, "-CBinding::CallGetBindInfo (grfBINDINFOF:%lx, hr:%lx)", *grfBINDINFOF, hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBinding::CallOnStartBinding
//
//  Synopsis:
//
//  Arguments:  [grfBINDINFOF] --
//              [pib] --
//
//  Returns:
//
//  History:    2-07-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBinding::CallOnStartBinding(DWORD grfBINDINFOF, IBinding * pib)
{
    HRESULT hr = E_FAIL;
    PerfDbgLog1(tagCBinding, this, "+CBinding::CallOnStartBinding (grfBINDINFOF:%lx)", grfBINDINFOF);

    if (GetOperationState() == OPS_GetBindInfo)
    {
        hr = _pBSCB->OnStartBinding(NULL, this);
        SetOperationState(OPS_StartBinding);
    }
    else
    {
        UrlMkAssert((GetOperationState() == OPS_GetBindInfo));
    }


    PerfDbgLog1(tagCBinding, this, "-CBinding::CallOnStartBinding (hr:%lx)", hr);
    return hr;
}
//+---------------------------------------------------------------------------
//
//  Method:     CBinding::CallOnProgress
//
//  Synopsis:
//
//  Arguments:  [ulProgress] --
//              [ulProgressMax] --
//              [ulStatusCode] --
//              [szStatusText] --
//
//  Returns:
//
//  History:    2-14-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBinding::CallOnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
{
    HRESULT hr = NOERROR;
    PerfDbgLog(tagCBinding, this, "+CBinding::CallOnProgress");

    if ( GetOperationState() == OPS_StartBinding)
    {
        SetOperationState(OPS_Downloading);
    }

    if (GetOperationState() == OPS_Downloading)
    {
        hr = _pBSCB->OnProgress(ulProgress, ulProgressMax, ulStatusCode, szStatusText);
    }

    PerfDbgLog1(tagCBinding, this, "-CBinding::CallOnProgress hr:%lx", hr);
    return hr;
}
//+---------------------------------------------------------------------------
//
//  Method:     CBinding::CallOnStopBinding
//
//  Synopsis:
//
//  Arguments:  [LPCWSTR] --
//              [szError] --
//
//  Returns:
//
//  History:    2-14-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBinding::CallOnStopBinding(HRESULT hrRet,LPCWSTR szError)
{
    HRESULT hr = E_FAIL;
    PerfDbgLog1(tagCBinding, this, "+CBinding::CallOnStopBinding ->hrRet:%lx", hrRet);

    if (   (GetOperationState() < OPS_Stopped)
        && (GetOperationState() > OPS_Initialized))
    {
        UrlMkAssert((  (hrRet != S_FALSE && hrRet != E_FAIL) ));

        if (hrRet == E_FAIL)
        {
            hrRet = INET_E_DOWNLOAD_FAILURE;
        }
        TransAssert((    ((hr == NOERROR) && _fSentLastNotification)
                      || (hr != NOERROR) ));

        //if( _fBindToObject ) 
        //    DbgLog(tagCBindingErr, this, ">>> OnStopBinding (BindToObject)");
        //else
        //    DbgLog(tagCBindingErr, this, ">>> OnStopBinding (BindToStorage)");
        hr = _pBSCB->OnStopBinding(hrRet, NULL);
        SetOperationState(OPS_Stopped);
    }
    TransAssert((_pBndCtx));
    _pBndCtx->SetTransactionObjects(NULL,NULL);

    PerfDbgLog1(tagCBinding, this, "-CBinding::CallOnStopBinding (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBinding::CallOnLowResource
//
//  Synopsis:
//
//  Arguments:  [reserved] --
//
//  Returns:
//
//  History:    2-14-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBinding::CallOnLowResource (DWORD reserved)
{
    HRESULT hr = E_FAIL;
    PerfDbgLog(tagCBinding, this, "+CBinding::CallOnLowResource");

    PerfDbgLog1(tagCBinding, this, "-CBinding::CallOnLowResource (hr:%lx)", hr);
    return hr;
}
//+---------------------------------------------------------------------------
//
//  Method:     CBinding::CallGetPriority
//
//  Synopsis:
//
//  Arguments:  [pnPriority] --
//
//  Returns:
//
//  History:    2-14-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBinding::CallGetPriority (LONG * pnPriority)
{
    HRESULT hr = E_FAIL;
    PerfDbgLog(tagCBinding, this, "+CBinding::CallGetPriority");

    PerfDbgLog1(tagCBinding, this, "-CBinding::CallGetPriority (hr:%lx)", hr);
    return hr;
}
//+---------------------------------------------------------------------------
//
//  Method:     CBinding::CallOnDataAvailable
//
//  Synopsis:
//
//  Arguments:  [DWORD] --
//              [FORMATETC] --
//              [STGMEDIUM] --
//              [pStgMed] --
//
//  Returns:
//
//  History:    2-14-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBinding::CallOnDataAvailable(DWORD grfBSC,DWORD dwSize,FORMATETC *pFmtETC,STGMEDIUM *pStgMed)
{
    HRESULT hr = E_FAIL;
    PerfDbgLog(tagCBinding, this, "+CBinding::CallOnDataAvailable");

    if (GetOperationState() == OPS_Downloading)
    { 
        //DbgLog2(tagCBindingErr, this, ">>> OnDataAvailable (BSC=%lx size=%d)",
        //        grfBSC, dwSize);
        hr = _pBSCB->OnDataAvailable(grfBSC, dwSize, pFmtETC, pStgMed);
    }
    else if (GetOperationState() == OPS_Abort)
    {
        hr = E_ABORT;
    }

    PerfDbgLog1(tagCBinding, this, "-CBinding::CallOnDataAvailable (hr:%lx)", hr);
    return hr;
}
//+---------------------------------------------------------------------------
//
//  Method:     CBinding::CallOnObjectAvailable
//
//  Synopsis:
//
//  Arguments:  [IUnknown] --
//              [punk] --
//
//  Returns:
//
//  History:    2-14-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBinding::CallOnObjectAvailable (REFIID riid,IUnknown *punk)
{
    HRESULT hr = E_FAIL;
    PerfDbgLog(tagCBinding, this, "+CBinding::CallOnObjectAvailable");

    if (GetOperationState() == OPS_Downloading || _grfBINDF & BINDF_GETCLASSOBJECT)
    {
        hr = _pBSCB->OnObjectAvailable(riid, punk);
        SetOperationState(OPS_ObjectAvailable);
    }
    else
    {
        UrlMkAssert((GetOperationState() == OPS_Downloading));
    }

    PerfDbgLog1(tagCBinding, this, "-CBinding::CallOnObjectAvailable (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBinding::GetRequestedObject
//
//  Synopsis:
//
//  Arguments:  [pbc] --
//              [ppUnk] --
//
//  Returns:
//
//  History:    7-04-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBinding::GetRequestedObject(IBindCtx *pbc, IUnknown **ppUnk)
{
    HRESULT hr = NOERROR;
    PerfDbgLog(tagCBinding, this, "+CBinding::GetRequestedObject");

    UrlMkAssert((ppUnk));
    *ppUnk = NULL;

    if (_pUnkObject)
    {
        *ppUnk = _pUnkObject;
        _pUnkObject->AddRef();
    }
    else if ( IsAsyncTransaction() )
    {
        //object is not available yet
        hr = MK_S_ASYNCHRONOUS;
    }
    else
    {
        // BUGBUG: JohanP - this is bogus for the case of returning a filename in stgmed; either we return a new success code or
        // the punk of the punkforrelease of the stgmed
        hr = NOERROR;
    }

    PerfDbgLog1(tagCBinding, this, "-CBinding::GetRequestedObject (hr:%lx)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CBinding::CallAuthenticate
//
//  Synopsis:
//
//  Arguments:  [phwnd] --
//              [LPWSTR] --
//              [pszPassword] --
//
//  Returns:
//
//  History:    2-08-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CBinding::CallAuthenticate(HWND* phwnd, LPWSTR *pszUsername,LPWSTR *pszPassword)
{
    PerfDbgLog(tagCBinding, this, "+CBinding::CallAuthenticate");
    HRESULT         hr = NOERROR;

    if (_pBasicAuth == NULL)
    {
        hr = LocalQueryInterface(IID_IAuthenticate, (void **) &_pBasicAuth);
    }

    if ((hr == NOERROR) && _pBasicAuth)
    {
         hr = _pBasicAuth->Authenticate(phwnd, pszUsername,pszPassword);
    }
    else
    {
        UrlMkAssert((_pBasicAuth == NULL));
        *phwnd = 0;
        *pszUsername = 0;
        *pszPassword = 0;
    }

    PerfDbgLog4(tagCBinding, this, "-CBinding::CallAuthenticate (hr:%lx, hwnd:%lx, username:%ws, password:%ws)",
        hr, *phwnd, *pszUsername?*pszUsername:L"", *pszPassword?*pszPassword:L"");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBinding::LocalQueryInterface
//
//  Synopsis:
//
//  Arguments:  [iid] --
//              [ppvObj] --
//
//  Returns:
//
//  History:    4-09-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBinding::LocalQueryInterface(REFIID riid, void **ppvObj)
{
    PerfDbgLog2(tagCBinding, this, "+CBinding::LocalQueryInterface (%lx, %lx)", riid, ppvObj);
    HRESULT hr = E_NOINTERFACE;
    *ppvObj = 0;

    if (_pBSCB)
    {
        IServiceProvider *pSrvPrv;
        hr = _pBSCB->QueryInterface(IID_IServiceProvider, (void **) &pSrvPrv);
        if (hr == NOERROR)
        {
            hr = pSrvPrv->QueryService(riid,riid, ppvObj);
            pSrvPrv->Release();
        }
        else
        {
            hr = E_NOINTERFACE;
            *ppvObj = 0;
        }
    }

    PerfDbgLog2(tagCBinding, this, "-CBinding::LocalQueryInterface (%lx)[%lx]", hr, *ppvObj);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBinding::Switch
//
//  Synopsis:
//
//  Arguments:  [pStateInfo] --
//
//  Returns:
//
//  History:    4-10-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBinding::Switch(PROTOCOLDATA *pStateInfo)
{
    PerfDbgLog(tagCBinding, this, "+CBinding::Switch");
    HRESULT hr = E_FAIL;

    TransAssert((FALSE));

    PerfDbgLog1(tagCBinding, this, "-CBinding::Switch (%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBinding::ReportProgress
//
//  Synopsis:
//
//  Arguments:  [ulStatusCode] --
//              [szStatusText] --
//
//  Returns:
//
//  History:    4-10-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBinding::ReportProgress(ULONG ulStatusCode, LPCWSTR szStatusText)
{
    PerfDbgLog(tagCBinding, this, "+CBinding::ReportProgress");
    HRESULT hr = NOERROR;

    if (   (ulStatusCode == BINDSTATUS_BEGINDOWNLOADDATA)
        || (ulStatusCode == BINDSTATUS_DOWNLOADINGDATA)
        || (ulStatusCode == BINDSTATUS_ENDDOWNLOADDATA) )
    {
    }
    else
    {
        BOOL fRet = OnTransNotification(
                    (BINDSTATUS) ulStatusCode  //BINDSTATUS NotMsg,
                    ,0 //ulProgress    //DWORD dwCurrentSize,
                    ,0 //ulProgressMax //DWORD dwTotalSize,
                    ,( LPWSTR)szStatusText  //LPWSTR pwzStr,
                    ,NOERROR       //HRESULT hrINet
                    );
    }


    PerfDbgLog1(tagCBinding, this, "-CBinding::ReportProgress (%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBinding::ReportData
//
//  Synopsis:
//
//  Arguments:  [grfBSCF] --
//              [ulProgress] --
//              [ulProgressMax] --
//
//  Returns:
//
//  History:    4-10-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBinding::ReportData(DWORD grfBSCF, ULONG ulProgress, ULONG ulProgressMax)
{
    PerfDbgLog(tagCBinding, this, "+CBinding::ReportData");
    HRESULT hr = NOERROR;
    BOOL fLastNotification = (grfBSCF & BSCF_LASTDATANOTIFICATION) ? TRUE : FALSE;
    ULONG ulStatusCode = BINDSTATUS_DOWNLOADINGDATA;
    ULONG dwNew;

    AddRef();

    if (grfBSCF & BSCF_LASTDATANOTIFICATION)
    {
        ulStatusCode = BINDSTATUS_ENDDOWNLOADDATA;
    }
    else if (grfBSCF & BSCF_FIRSTDATANOTIFICATION)
    {
        ulStatusCode = BINDSTATUS_BEGINDOWNLOADDATA;
    }

    HRESULT hr1 = _pCTransData->OnDataReceived(grfBSCF, ulProgress, ulProgressMax,&dwNew);
    ulProgress = dwNew;

    if (hr1 == S_FALSE)
    {
        // end of data
        ulStatusCode = BINDSTATUS_ENDDOWNLOADDATA;
    }
    else if (   (hr1 != S_NEEDMOREDATA)
             && (hr1 != E_PENDING)
             && (hr1 != NOERROR))
    {
        ulStatusCode = BINDSTATUS_ERROR;
    }

    if (hr1 != S_NEEDMOREDATA)
    {
        BOOL fRet = OnTransNotification(
                        (BINDSTATUS) ulStatusCode   //BINDSTATUS NotMsg,
                        ,ulProgress                 //DWORD dwCurrentSize,
                        ,ulProgressMax              //DWORD dwTotalSize,
                        ,NULL                       //LPWSTR pwzStr,
                        ,NOERROR                    //HRESULT hrINet
                        );
        if (fRet == TRUE)
        {
            _pCTransData->OnTerminate();
            if (_fBindToObject)
            {
                hr = S_FALSE;
                TransAssert((_grfInternalFlags | ~BDGFLAGS_ATTACHED));
                TransAssert((_grfInternalFlags & BDGFLAGS_PARTIAL));
                
                _pOInetBdg->Terminate(BDGFLAGS_PARTIAL);
            }
        }
    }
    Release();

    PerfDbgLog1(tagCBinding, this, "-CBinding::ReportData (%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBinding::ReportResult
//
//  Synopsis:
//
//  Arguments:  [hrResult] --
//              [dwError] --
//              [pwzResult] --
//
//  Returns:
//
//  History:    4-10-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBinding::ReportResult(HRESULT hrResult, DWORD dwError, LPCWSTR pwzResult)
{
    PerfDbgLog(tagCBinding, this, "+CBinding::ReportResult");
    HRESULT hr = NOERROR;

    AddRef();

    _dwBindError = dwError;
    _hrBindResult = hrResult;

    if (pwzResult)
    {
        _pwzResult = OLESTRDuplicate((LPWSTR)pwzResult);
    }

    BOOL fRet = OnTransNotification(
                    BINDSTATUS_RESULT           //BINDSTATUS NotMsg,
                    ,0 //ulProgress             //DWORD dwCurrentSize,
                    ,0 //ulProgressMax          //DWORD dwTotalSize,
                    ,(LPWSTR)pwzResult           //LPWSTR pwzStr,
                    ,hrResult                   //HRESULT hrINet
                    );
    if (fRet == TRUE)
    {
        hr = S_FALSE;

        DWORD dwFlags = (_fBindToObject) ? BDGFLAGS_PARTIAL : 0;
        _pCTransData->OnTerminate();
        _pOInetBdg->Terminate(dwFlags);
    }
    Release();

    PerfDbgLog1(tagCBinding, this, "-CBinding::ReportResult (%lx)", hr);
    return hr;
}


STDMETHODIMP CBinding::CreateObject(CLSID *pclsid, REFIID riidResult, IUnknown **ppUnk)
{
    PerfDbgLog(tagCBinding, this, "+CBinding::CreateObj");
    HRESULT     hr;
    IUnknown    *pUnk = NULL;

    if (GetOperationState() == OPS_Abort)
    {
        // stop now - the client aborted the operation
        hr = E_ABORT;
    }
    else
    {

        DumpIID(riidResult);

        // call OleAutoConvert
        {
            CLSID clsidIn = *pclsid;
            CLSID clsidOut;
            hr = OleGetAutoConvert(clsidIn, &clsidOut);
            if (hr == S_OK)
            {
                *pclsid = clsidOut;
            }
        }

        if (_grfBINDF & BINDF_GETCLASSOBJECT)
        {
            // Just want the class object
            hr = CoGetClassObject(*pclsid, CLSCTX_INPROC_SERVER,
                                  NULL, riidResult, (void **)&pUnk);
            if (FAILED(hr))
            {
                hr = CoGetClassObject(*pclsid, CLSCTX_LOCAL_SERVER,
                                      NULL, riidResult, (void **)&pUnk);
                if (FAILED(hr))
                {
                    hr = CoGetClassObject(*pclsid, CLSCTX_INPROC_HANDLER,
                                          NULL, riidResult, (void **)&pUnk);

                }
            }
        }
        else
        {

            hr = CoCreateInstance(*pclsid, NULL, CLSCTX_INPROC_SERVER,
                                    riidResult, (void**)&pUnk);

            if (FAILED(hr))
            {
                hr = CoCreateInstance(*pclsid, NULL, CLSCTX_LOCAL_SERVER,
                                   riidResult, (void**)&pUnk);

                if (FAILED(hr))
                {

                    hr = CoCreateInstance(*pclsid, NULL, CLSCTX_INPROC_HANDLER, 
                                       riidResult, (void**)&pUnk);

                }
            }
        }

        if (SUCCEEDED(hr))
        {
            *ppUnk = pUnk;
        }
        else
        {
            SetInstantiateHresult(hr);
            hr = INET_E_CANNOT_INSTANTIATE_OBJECT;
        }
        CallOnProgress( 0, 0, BINDSTATUS_ENDSYNCOPERATION, 0 );
    }

    PerfDbgLog1(tagCBinding, this, "-CBinding::CreateObject (hr:%lx)", hr);
    return hr;
}

