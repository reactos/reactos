//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       cdlprot.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    02-06-1997   t-alans (Alan Shi)   Created
//
//----------------------------------------------------------------------------

#include <eapp.h>
#include <tchar.h>
#ifdef unix
#include "../download/cdl.h"
#else
#include "..\download\cdl.h"
#endif /* !unix */


// From shlwapip.h
LWSTDAPI_(HRESULT) CLSIDFromStringWrap(LPOLESTR lpsz, LPCLSID pclsid);


#define VALUE_EQUAL_CHAR            '='
#define VALUE_SEPARATOR_CHAR        ';'


//+---------------------------------------------------------------------------
//
//  Method:     CCdlProtocol::CCdlProtocol
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    02-06-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

CCdlProtocol::CCdlProtocol(REFCLSID rclsid, IUnknown *pUnkOuter,
                           IUnknown **ppUnkInner)
: CBaseProtocol(rclsid, pUnkOuter, ppUnkInner)
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CCdlProtocol::CCdlProtocol \n", this));
 
    DllAddRef();

    _clsidReport = CLSID_NULL;
    _pCodeDLBSC = NULL;
    _fDataPending = TRUE;
    _fNotStarted = TRUE;
    _iid = IID_IUnknown;
    _pbc = NULL;
    _fGetClassObject = FALSE;
    EProtDebugOut((DEB_PLUGPROT, "%p OUT CCdlProtocol::CCdlProtocol \n", this));
}

//+---------------------------------------------------------------------------
//
//  Method:     CCdlProtocol::~CCdlProtocol
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    02-06-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

CCdlProtocol::~CCdlProtocol()
{
    if (_pbc)
    {
        _pbc->Release();
    }

    DllRelease();
    EProtDebugOut((DEB_PLUGPROT, "%p _IN/OUT CCdlProtocol::~CCdlProtocol \n", this));
}

//+---------------------------------------------------------------------------
//
//  Method:     CCdlProtocol::Start
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    02-06-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCdlProtocol::Start(LPCWSTR pwzUrl,
                                 IOInetProtocolSink *pIOInetProtocolSink,
                                 IOInetBindInfo *pIOInetBindInfo,
                                 DWORD grfSTI,
                                 DWORD_PTR dwReserved)
{
    DWORD                     cElFetched = 0;
    LPOLESTR                  pwzIID = NULL;
    BINDINFO                  bindinfo;
    DWORD                     grfBINDF = 0;

    EProtDebugOut((DEB_PLUGPROT, "%p _IN CCdlProtocol::Start\n", this));
    HRESULT hr = NOERROR;
    WCHAR    wzURL[MAX_URL_SIZE];

    EProtAssert((!_pProtSink && pIOInetBindInfo && pIOInetProtocolSink));
    EProtAssert((_pwzUrl == NULL));

    bindinfo.cbSize = sizeof(BINDINFO);
    pIOInetBindInfo->GetBindInfo(&grfBINDF, &bindinfo);

    if (grfBINDF & BINDF_GETCLASSOBJECT)
    {
        LPWSTR                  pwzBC = NULL;
        DWORD                   cElFetched = 0;

        hr = pIOInetBindInfo->GetBindString(BINDSTRING_PTR_BIND_CONTEXT,
                                            &pwzBC, 0,
                                            &cElFetched);
        if (SUCCEEDED(hr))
        {
            _pbc = (IBindCtx *)StrToIntW(pwzBC);

            EProtAssert(_pbc);

            delete [] pwzBC;
            pwzBC = NULL;
        }
        else
        {
            hr = E_UNEXPECTED;
            goto Exit;
        }

        _fGetClassObject = TRUE;
    }

    grfSTI |= PI_FORCE_ASYNC;
    hr = CBaseProtocol::Start(pwzUrl, pIOInetProtocolSink, pIOInetBindInfo, grfSTI, dwReserved);

    if (SUCCEEDED(hr))
    {
        hr = pIOInetBindInfo->GetBindString(BINDSTRING_IID, &pwzIID, 0,
                                            &cElFetched);
    }

    if (hr == S_OK)
    {
        hr = CLSIDFromString(pwzIID, &_iid);
        delete [] pwzIID;
    }
    
    if (SUCCEEDED(hr))
    {
        hr =  ParseURL();
    }

Exit:

    EProtDebugOut((DEB_PLUGPROT, "%p OUT CCdlProtocol::Start (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCdlProtocol::Continue
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    02-06-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCdlProtocol::Continue(PROTOCOLDATA *pStateInfoIn)
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CCdlProtocol::Continue\n", this));
    HRESULT hr = E_FAIL;

    if (_fNotStarted && pStateInfoIn->dwState == CDL_STATE_BIND)
    {
        _fNotStarted = FALSE;
        hr =  ParseURL();
    }

    EProtDebugOut((DEB_PLUGPROT, "%p OUT CCdlProtocol::Continue (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCdlProtocol::Read
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    02-06-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCdlProtocol::Read(void *pv,ULONG cb,ULONG *pcbRead)
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CCdlProtocol::Read\n", this));
    HRESULT           hr;

    hr = (_fDataPending) ? (E_PENDING) : (S_FALSE);
    *pcbRead = (_fDataPending) ? (0x0) : (0x100);

    EProtDebugOut((DEB_PLUGPROT, "%p OUT CCdlProtocol::Read\n", this));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCdlProtocol::Abort
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    02-06-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCdlProtocol::Abort(HRESULT hrReason, DWORD dwOptions)
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CCdlProtocol::Abort\n", this));
    HRESULT           hr = E_UNEXPECTED;
   
    EProtAssert( _pCodeDLBSC != NULL );
    if (_pCodeDLBSC != NULL)
    {
        hr = _pCodeDLBSC->Abort();
    }

#if 0
    if (_pProtSink)
    {
        hr = CBaseProtocol::Abort(hrReason, dwOptions);
    }
#endif

    EProtDebugOut((DEB_PLUGPROT, "%p OUT CCdlProtocol::Abort\n", this));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCdlProtocol::Seek
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    02-06-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCdlProtocol::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin,
                                ULARGE_INTEGER *plibNewPosition)
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CCdlProtocol::Seek\n", this));
    EProtDebugOut((DEB_PLUGPROT, "%p OUT CCdlProtocol::Seek\n", this));
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCdlProtocol::LockRequest
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    02-06-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCdlProtocol::LockRequest(DWORD dwOptions)
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CCdlProtocol::LockRequest\n", this));
    EProtDebugOut((DEB_PLUGPROT, "%p OUT CCdlProtocol::LockRequest\n", this));
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCdlProtocol::UnlockRequest
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    02-06-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCdlProtocol::UnlockRequest()
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CCdlProtocol::UnlockRequest\n", this));
    EProtDebugOut((DEB_PLUGPROT, "%p OUT CCdlProtocol::UnlockRequest\n", this));
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCdlProtocol::ParseURL
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    02-06-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCdlProtocol::ParseURL()
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CCdlProtocol::ParseURL\n", this));
    HRESULT            hr = MK_E_SYNTAX; 
    WCHAR              wzlURL[MAX_URL_SIZE];
    LPWSTR             pwz = NULL;
    LPWSTR             pwzValue = NULL;
    LPWSTR             pwzTag = NULL;
    LPWSTR             pwzPound = NULL;
    BOOL               fGotRequiredField = FALSE;
    CodeDownloadData   cdldata;
    CHAR               szValue[MAX_PATH];

    cdldata.szDistUnit = NULL;
    cdldata.szClassString = NULL;
    cdldata.szURL = NULL;
    cdldata.szMimeType = NULL;
    cdldata.szExtension = NULL;
    cdldata.szDll = NULL;
    cdldata.dwFileVersionMS = 0;
    cdldata.dwFileVersionLS = 0;
    cdldata.dwFlags = 0;


// URL is of the form:
// (1)  cdl://[unused]?[clsid=xxx|codebase=xxx|mimetype=xxx|extension=xxx];
//                     [clsid=xxx|codebase=xxx|mimetype=xxx|extension=xxx];
//                     [clsid=xxx|codebase=xxx|mimetype=xxx|extension=xxx];...
//
//     OR
//
// (2)  cdl:[clsid=xxx|codebase=xxx|mimetype=xxx|extension=xxx];
//          [clsid=xxx|codebase=xxx|mimetype=xxx|extension=xxx];...
//
//
// NOTE: [distunit=xxx] added to support AsyncInstallDistributionUnit
//       [flags=xxx] added flags (as an integer so that we can dictate
//                   silent mode, async. etc.

    wcscpy(wzlURL, _wzFullURL);

    // ensure correct format: "cdl://[stuff]?"

    pwz = wcschr(wzlURL, ':');
    pwz++;
    if (!(wcsnicmp(pwz, L"//", 2) && wcsnicmp(pwz, L"\\\\", 2)))
    {
        // URL is of form (1)
        pwz = wcschr(pwz, '?');
        if( pwz == NULL ) {
            // error, no '?' found
            hr = MK_E_SYNTAX;
            goto Exit;
        }
        pwz++; // pwz now points to the start of the param list (start boundry)
    }

    pwzValue = pwz + wcslen( pwz ); // points to NULL


    // If there is a pound, NULL terminate there instead

    pwzPound = pwzValue - 1;

    while (pwzPound >= pwz)
    {
        if (*pwzPound == VALUE_POUND_CHAR)
        {
            *pwzPound = NULL;
            pwzValue = pwzPound;
            break;
        }
        pwzPound--;
    }

    for ( ;; )
    {
        while (pwzValue >= pwz && *pwzValue != VALUE_EQUAL_CHAR &&
               *pwzValue != VALUE_SEPARATOR_CHAR)
        {
            pwzValue--;
        } 
        if (pwzValue < pwz || *pwzValue == VALUE_SEPARATOR_CHAR)
        {
            // error, expected '='
            hr = MK_E_SYNTAX;
            goto Exit;
        }
    
        // pwzValue now points to '='
        *pwzValue = NULL;
        pwzTag = pwzValue;
        pwzValue++;
    
        while (pwzTag >= pwz && *pwzTag != VALUE_EQUAL_CHAR &&
                                *pwzTag != VALUE_SEPARATOR_CHAR)
        {
            pwzTag--;
        }
        if (*pwzTag == VALUE_EQUAL_CHAR)
        {
            // error, expected either a separator, or the beginning
            hr = MK_E_SYNTAX;
            goto Exit;
        }
        pwzTag++;
    
        if (!wcsicmp(L"codebase", pwzTag))
        {
            cdldata.szURL = pwzValue;
        }
        else if (!wcsicmp(L"clsid", pwzTag))
        {
            cdldata.szClassString = pwzValue;
            fGotRequiredField = TRUE;
        }
        else if (!wcsicmp(L"mimetype", pwzTag))
        {
            cdldata.szMimeType = pwzValue;
            fGotRequiredField = TRUE;
        }
        else if (!wcsicmp(L"extension", pwzTag))
        {
            cdldata.szExtension = pwzValue;
            fGotRequiredField = TRUE;
        }
        else if (!wcsicmp(L"verMS", pwzTag))
        {
            //cdldata.dwFileVersionMS = _wtol(pwzValue);
            W2A(pwzValue, szValue, MAX_PATH);
            cdldata.dwFileVersionMS = atol(szValue);
            
        }
        else if (!wcsicmp(L"verLS", pwzTag))
        {
            //cdldata.dwFileVersionLS = _wtol(pwzValue);
            W2A(pwzValue, szValue, MAX_PATH);
            cdldata.dwFileVersionLS = atol(szValue);
        }
        else if (!wcsicmp(L"distunit", pwzTag))
        {
            cdldata.szDistUnit = pwzValue;
            fGotRequiredField = TRUE;
        }
        else if (!wcsicmp(L"flags", pwzTag))
        {
            cdldata.dwFlags = StrToIntW(pwzValue);
        }
        else if (!wcsicmp(L"version", pwzTag))
        {
            W2A(pwzValue, szValue, MAX_PATH);
            GetVersionFromString(szValue, &(cdldata.dwFileVersionMS), &(cdldata.dwFileVersionLS));
        }
        else if (!wcsicmp(L"dllname",pwzTag))
        {
            cdldata.szDll = pwzValue;
        }

        if (pwzTag <= pwz)
        {
            break; // we are done
        }
        else
        {
            pwzValue = pwzTag;
            pwzValue--;
            *pwzValue = NULL;
            pwzTag = NULL;
        }
    }

    // backwards compatability with clsid can be dist unit
    if(cdldata.szClassString && ! cdldata.szDistUnit)
    {
        cdldata.szDistUnit = cdldata.szClassString;
    }
    if(cdldata.szDistUnit && ! cdldata.szClassString)
    {
        cdldata.szClassString = cdldata.szDistUnit;
    }

    if (fGotRequiredField)
    {
        // The client must provide a host security manager for
        // CDL:// protocol bindings. Otherwise, a file:// URL codebase
        // will be executed without WVT UI.

        IInternetHostSecurityManager *phsm = NULL;
        IServiceProvider *psp = NULL;

        hr = _pProtSink->QueryInterface(IID_IServiceProvider,
                                        (void **)&psp);
        if (FAILED(hr)) {
            hr = TRUST_E_FAIL;
            goto Exit;
        }

        hr = psp->QueryService(IID_IInternetHostSecurityManager,
                               IID_IInternetHostSecurityManager, (void **)&phsm);

        if (FAILED(hr)) {
            hr = TRUST_E_FAIL;
            goto Exit;
        }

        psp->Release();
        phsm->Release();
                                 
        if (IsEqualGUID(_clsidReport , CLSID_NULL))
            CLSIDFromString((LPOLESTR)cdldata.szClassString, &_clsidReport);
        hr = StartDownload(cdldata);
    }
    else
    {
        hr = MK_E_SYNTAX;
    }

Exit:

    // if we error for any reason here, then CodeDL BSC was never initiated and we will
    // never get an BSC::OSB to shut up sink.
    
    if (hr != E_PENDING)
    {
        _fDataPending = FALSE;

        if (_pProtSink)
        {
            if (!IsEqualGUID(_clsidReport, CLSID_NULL))
            {
                LPOLESTR pwzStrClsId;
                StringFromCLSID(_clsidReport, &pwzStrClsId);
                _pProtSink->ReportProgress(BINDSTATUS_CLSIDCANINSTANTIATE, pwzStrClsId);
            
                delete [] pwzStrClsId;
            }

            _pProtSink->ReportResult(hr, 0, 0); 
        }
    }

    EProtDebugOut((DEB_PLUGPROT, "%p OUT CCdlProtocol::ParseURL\n", this));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCdlProtocol::StartDownload
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    02-06-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------
STDMETHODIMP CCdlProtocol::StartDownload(CodeDownloadData &cdldata)
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CCdlProtocol::StartDownload\n", this));
    HRESULT               hr  = S_OK;
    IBindCtx             *pbc = NULL;
    IUnknown             *pUnk = NULL;

// Kick off the download
    
    _pCodeDLBSC = new CCodeDLBSC(_pProtSink, _pOIBindInfo, this, _fGetClassObject);
    if (_pCodeDLBSC == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }
    hr = CreateBindCtx(0, &pbc);
    if (SUCCEEDED(hr))
    {
        hr = RegisterBindStatusCallback(pbc, _pCodeDLBSC, NULL, 0);
        if (_pCodeDLBSC != NULL)
        {
            _pCodeDLBSC->Release();
        }
        if (FAILED(hr))
        {
            goto Exit;
        }
    }

    cdldata.dwFlags = (_fGetClassObject) ? (CD_FLAGS_NEED_CLASSFACTORY) : (0);

    hr = AsyncInstallDistributionUnitEx(&cdldata, pbc, _iid, &pUnk, NULL);

    if (hr == MK_S_ASYNCHRONOUS)
    {
        hr = E_PENDING;
    }
    else
    {
        if (_fGetClassObject)
        {
            if (pUnk && SUCCEEDED(hr))
            {
                hr = RegisterIUnknown(pUnk);
                pUnk->Release();
        
                if (SUCCEEDED(hr))
                {
                    hr = _pProtSink->ReportProgress(BINDSTATUS_IUNKNOWNAVAILABLE, NULL);
                }
            }
            _pProtSink->ReportResult(hr, 0, 0);
        }
    }
   
Exit:
    if (pbc != NULL)
    {
        // NOTE: This instruction can cause deletion of this object,
        // referencing "this" afterwords may be a bad idea.
        pbc->Release();
    }

    EProtDebugOut((DEB_PLUGPROT, "%p OUT CCdlProtocol::StartDownload\n", this));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCdlProtocol::SetDataPending
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    02-06-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

void CCdlProtocol::SetDataPending(BOOL fPending)
{
    _fDataPending = fPending;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCdlProtocol::RegisterIUnknown
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    11-12-1998   AlanShi (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

HRESULT CCdlProtocol::RegisterIUnknown(IUnknown *pUnk)
{
    EProtAssert(_pbc);

    return _pbc->RegisterObjectParam(SZ_IUNKNOWN_PTR, pUnk);
}

