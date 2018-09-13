
//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       baseprot.cxx
//
//  Contents:   Implementation of a base class for pluggable protocols
//
//  History:    02-12-97    AnandRa     Created
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_UWININET_H_
#define X_UWININET_H_
#include "uwininet.h"
#endif

#ifndef X_BASEPROT_HXX_
#define X_BASEPROT_HXX_
#include "baseprot.hxx"
#endif

//+---------------------------------------------------------------------------
//
//  Method:     CBaseProtocolCF::QueryInterface
//
//  Synopsis:   per IPrivateUnknown
//
//----------------------------------------------------------------------------

HRESULT
CBaseProtocolCF::QueryInterface(REFIID riid, void **ppv)
{
    *ppv = NULL;

    if (riid == IID_IInternetProtocolInfo)
    {
        *ppv = (IInternetProtocolInfo *)this;
    }
    else
    {
        RRETURN(super::QueryInterface(riid, ppv));
    }

    Assert(*ppv);
    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}



//+---------------------------------------------------------------------------
//
//  Method:     CBaseProtocolCF::ParseUrl
//
//  Synopsis:   per IInternetProtocolInfo
//
//----------------------------------------------------------------------------

HRESULT
CBaseProtocolCF::ParseUrl(
    LPCWSTR     pwzUrl,
    PARSEACTION ParseAction,
    DWORD       dwFlags,
    LPWSTR      pwzResult,
    DWORD       cchResult,
    DWORD *     pcchResult,
    DWORD       dwReserved)
{
    CStr    cstr;
    HRESULT hr = INET_E_DEFAULT_ACTION;

    if (!pcchResult || !pwzResult)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    if (ParseAction == PARSE_DOMAIN)
    {
        BSTR bstrTemp;

        hr = THR(UnwrapSpecialUrl(pwzUrl, cstr));
        if (hr)
            goto Cleanup;

        *pcchResult = cstr.Length() + 1;
        
        if (cstr.Length() + 1 > cchResult)
        {
            // Not enough room
            hr = S_FALSE;
            goto Cleanup;
        }

        cstr.AllocBSTR(&bstrTemp);

#ifdef WIN16
        Assert(0);
#else
        UrlGetPartW(bstrTemp, pwzResult, pcchResult, URL_PART_HOSTNAME, 0);  
#endif
        SysFreeString(bstrTemp);
    }

Cleanup:
    RRETURN2(hr, INET_E_DEFAULT_ACTION, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Method:     CBaseProtocolCF::CombineUrl
//
//  Synopsis:   per IInternetProtocolInfo
//
//----------------------------------------------------------------------------

HRESULT
CBaseProtocolCF::CombineUrl(
    LPCWSTR     pwzBaseUrl,
    LPCWSTR     pwzRelativeUrl,
    DWORD       dwFlags,
    LPWSTR      pwzResult,
    DWORD       cchResult,
    DWORD *     pcchResult,
    DWORD       dwReserved)
{
    HRESULT hr = INET_E_DEFAULT_ACTION;
    // get the correct base url for navigating to a non-pluggable protocol from a
    // pluggable protocol. We get here from CoInternetCombineUrl if the base url is
    // a pluggable protocol. So, search for the last embedded \1 to extract the true
    // base url and if present, call the API again to combine the non-pluggable
    // protocol url properly. if not prest, just tell urlmon to do the default thing.
    if (pwzBaseUrl)
    {
        TCHAR *pchSrc = _tcsrchr(pwzBaseUrl, _T('\1'));
        if (pchSrc)
        {
            hr = THR(CoInternetCombineUrl(
                    ++pchSrc, 
                    pwzRelativeUrl, 
                    URL_ESCAPE_SPACES_ONLY, 
                    pwzResult, 
                    cchResult, 
                    pcchResult, 
                    0));
        }
    }

    RRETURN1(hr, INET_E_DEFAULT_ACTION);
}


//+---------------------------------------------------------------------------
//
//  Method:     CBaseProtocolCF::CompareUrl
//
//  Synopsis:   per IInternetProtocolInfo
//
//----------------------------------------------------------------------------

HRESULT
CBaseProtocolCF::CompareUrl(
    LPCWSTR     pwzUrl1,
    LPCWSTR     pwzUrl2,
    DWORD       dwFlags)
{
    RRETURN(E_NOTIMPL);
}


//+---------------------------------------------------------------------------
//
//  Method:     CBaseProtocolCF::QueryInfo
//
//  Synopsis:   per IInternetProtocolInfo
//
//----------------------------------------------------------------------------

HRESULT
CBaseProtocolCF::QueryInfo(
    LPCWSTR         pwzUrl,
    QUERYOPTION     QueryOption,
    DWORD           dwQueryFlags,
    LPVOID          pvBuffer,
    DWORD           cbBuffer,
    DWORD *         pcbBuffer,
    DWORD           dwReserved)
{
    HRESULT hr = INET_E_DEFAULT_ACTION;

    switch (QueryOption)
    {
    case QUERY_USES_NETWORK:
        {
            if (!pvBuffer || cbBuffer < sizeof(DWORD))
                return E_FAIL;

            if (pcbBuffer)
            {
                *pcbBuffer = sizeof(DWORD);
            }

            *(DWORD *)pvBuffer = FALSE;
            hr = S_OK;
        }
        break;

    case QUERY_IS_SECURE:
        {
            if (!pvBuffer || cbBuffer < sizeof(DWORD))
                return E_FAIL;

            if (pcbBuffer)
            {
                *pcbBuffer = sizeof(DWORD);
            }

            *(DWORD *)pvBuffer = HasSecureContext(pwzUrl);
            hr = S_OK;
        }
        break;
    }

    RRETURN1(hr, INET_E_DEFAULT_ACTION);
}


//+---------------------------------------------------------------------------
//
//  Method:     CBaseProtocolCF::UnwrapSpecialUrl
//
//  Synopsis:   Helper to unwrap a url by lopping off any stuff after \1
//
//----------------------------------------------------------------------------

HRESULT
CBaseProtocolCF::UnwrapSpecialUrl(LPCWSTR pchUrl, CStr &cstrUnwrappedUrl)
{
    TCHAR * pchSpecial = NULL;
    HRESULT hr = S_OK;
    TCHAR   ach[pdlUrlLen];
    DWORD   dwSize;
        
    hr = THR(CoInternetParseUrl(
            pchUrl, 
            PARSE_ENCODE, 
            0, 
            ach, 
            ARRAY_SIZE(ach), 
            &dwSize, 
            0));
    if (hr)
        goto Cleanup;

    pchSpecial = _tcsrchr(ach, _T('\1'));
    if (pchSpecial)
    {
        hr = THR(cstrUnwrappedUrl.Set(pchSpecial + 1));
        if (hr)
            goto Cleanup;
    }
    else
    {
        hr = THR(cstrUnwrappedUrl.Set(ach));
        if (hr)
            goto Cleanup;
    }
    
Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Method:     CBaseProtocol::CBaseProtocol
//
//  Synopsis:   ctor
//
//----------------------------------------------------------------------------

CBaseProtocol::CBaseProtocol(IUnknown *pUnkOuter) : super()
{
    _pUnkOuter = pUnkOuter ? pUnkOuter : PunkInner();
    _bscf = BSCF_FIRSTDATANOTIFICATION;
}


//+---------------------------------------------------------------------------
//
//  Method:     CBaseProtocol::~CBaseProtocol
//
//  Synopsis:   dtor
//
//----------------------------------------------------------------------------

CBaseProtocol::~CBaseProtocol()
{
}


//+---------------------------------------------------------------------------
//
//  Method:     CBaseProtocol::Passivate
//
//  Synopsis:   1st stage dtor
//
//----------------------------------------------------------------------------

void
CBaseProtocol::Passivate()
{
    ClearInterface(&_pStm);
    super::Passivate();
}


//+---------------------------------------------------------------------------
//
//  Method:     CBaseProtocol::PrivateQueryInterface
//
//  Synopsis:   per IPrivateUnknown
//
//----------------------------------------------------------------------------

HRESULT
CBaseProtocol::PrivateQueryInterface(REFIID riid, void **ppv)
{
    *ppv = NULL;

    if (riid == IID_IUnknown)
    {
        *ppv = PunkInner();
    }
    else if (riid == IID_IInternetProtocol || riid == IID_IInternetProtocolRoot)
    {
        *ppv = (IInternetProtocol *)this;
    }
    else if (riid == IID_IServiceProvider)
    {
        *ppv = (IServiceProvider *)this;
    }
    else
    {
        RRETURN(E_NOINTERFACE);
    }

    Assert(*ppv);
    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Method:     CBaseProtocol::Start
//
//  Synopsis:   per IInternetProtocol
//
//----------------------------------------------------------------------------

HRESULT
CBaseProtocol::Start(
    LPCWSTR pchUrl, 
    IInternetProtocolSink *pTrans, 
    IInternetBindInfo *pOIBindInfo,
    DWORD grfSTI, 
    HANDLE_PTR dwReserved)
{
    HRESULT         hr = NOERROR;
    PROTOCOLDATA    protdata;
    TCHAR           ach[pdlUrlLen];
    DWORD           dwSize;

    Assert(!_pProtSink && pOIBindInfo && pTrans && !_cstrURL);

    if ( !(grfSTI & PI_PARSE_URL))
    {
        ReplaceInterface(&_pProtSink, pTrans);
        ReplaceInterface(&_pOIBindInfo, pOIBindInfo);
    }

    _bindinfo.cbSize = sizeof(BINDINFO);
    hr = THR(pOIBindInfo->GetBindInfo(&_grfBindF, &_bindinfo));

    //
    // First get the basic url.  Unescape it first.
    //

    hr = THR(CoInternetParseUrl(pchUrl, PARSE_ENCODE, 0, ach, ARRAY_SIZE(ach), &dwSize, 0));
    if (hr)
        goto Cleanup;
    
    hr = THR(_cstrURL.Set(ach));
    if (hr)
        goto Cleanup;

    //
    // Now append any extra data if needed.
    //
    
    if (_bindinfo.szExtraInfo)
    {
        hr = THR(_cstrURL.Append(_bindinfo.szExtraInfo));
        if (hr)
            goto Cleanup;
    }

    _grfSTI = grfSTI;

    //
    // Always go async and return E_PENDING now.
    // Perform script execution when we get the Continue.
    //

    hr = E_PENDING;
    protdata.grfFlags = PI_FORCE_ASYNC;
    protdata.dwState = BIND_ASYNC;
    protdata.pData = NULL;
    protdata.cbData = 0;

    _pProtSink->Switch(&protdata);

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Method:     CBaseProtocol::Continue
//
//  Synopsis:   per IInternetProtocol
//
//----------------------------------------------------------------------------

HRESULT
CBaseProtocol::Continue(PROTOCOLDATA *pStateInfoIn)
{
    HRESULT hr = E_FAIL;

    Assert(!pStateInfoIn->pData && !pStateInfoIn->cbData && 
            pStateInfoIn->dwState == BIND_ASYNC);

    if (pStateInfoIn->dwState == BIND_ASYNC)
    {
        hr =  THR(ParseAndBind());
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Method:     CBaseProtocol::Abort
//
//  Synopsis:   per IInternetProtocol
//
//----------------------------------------------------------------------------

HRESULT
CBaseProtocol::Abort(HRESULT hrReason, DWORD dwOptions)
{
    _fAborted = TRUE;
    RRETURN(_pProtSink->ReportResult(E_ABORT, 0, 0));
}


//+---------------------------------------------------------------------------
//
//  Method:     CBaseProtocol::Terminate
//
//  Synopsis:   per IInternetProtocol
//
//----------------------------------------------------------------------------

HRESULT
CBaseProtocol::Terminate(DWORD dwOptions)
{
    ClearInterface(&_pOIBindInfo);
    ClearInterface(&_pProtSink);
    ReleaseBindInfo(&_bindinfo);
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Method:     CBaseProtocol::Suspend
//
//  Synopsis:   per IInternetProtocol
//
//----------------------------------------------------------------------------

HRESULT
CBaseProtocol::Suspend()
{
    RRETURN(E_NOTIMPL);
}


//+---------------------------------------------------------------------------
//
//  Method:     CBaseProtocol::Resume
//
//  Synopsis:   per IInternetProtocol
//
//----------------------------------------------------------------------------

HRESULT
CBaseProtocol::Resume()
{
    RRETURN(E_NOTIMPL);
}


//+---------------------------------------------------------------------------
//
//  Method:     CBaseProtocol::Read
//
//  Synopsis:   per IInternetProtocol
//
//----------------------------------------------------------------------------

HRESULT
CBaseProtocol::Read(void *pv,ULONG cb,ULONG *pcbRead)
{
    HRESULT hr = S_OK;
    
    if (!_pStm)
    {
        hr = E_FAIL;
    }
    else
    {
        hr = THR(_pStm->Read(pv, cb, pcbRead));
    }
    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Method:     CBaseProtocol::Seek
//
//  Synopsis:   per IInternetProtocol
//
//----------------------------------------------------------------------------

HRESULT
CBaseProtocol::Seek(
    LARGE_INTEGER dlibMove,
    DWORD dwOrigin,
    ULARGE_INTEGER *plibNewPosition)
{
    HRESULT hr = S_OK;
    
    if (!_pStm)
    {
        hr = E_FAIL;
    }
    else
    {
        hr = THR(_pStm->Seek(dlibMove, dwOrigin, plibNewPosition));
    }
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Method:     CBaseProtocol::LockRequest
//
//  Synopsis:   per IInternetProtocol
//
//----------------------------------------------------------------------------

HRESULT
CBaseProtocol::LockRequest(DWORD dwOptions)
{
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Method:     CBaseProtocol::UnlockRequest
//
//  Synopsis:   per IInternetProtocol
//
//----------------------------------------------------------------------------

HRESULT
CBaseProtocol::UnlockRequest()
{
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Method:     CBaseProtocol::QueryService
//
//  Synopsis:   per IServiceProvider
//
//----------------------------------------------------------------------------

HRESULT
CBaseProtocol::QueryService(REFGUID rsid, REFIID riid, void ** ppv)
{
    HRESULT             hr = S_OK;
    IServiceProvider *  pSP = NULL;

    *ppv = NULL;

    hr = THR_NOTRACE(_pProtSink->QueryInterface(
            IID_IServiceProvider, (void **)&pSP));
    if (hr)
        goto Cleanup;

    hr = THR_NOTRACE(pSP->QueryService(rsid, riid, ppv));
    if (hr)
        goto Cleanup;

Cleanup:
    ReleaseInterface(pSP);
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Method:     CBaseProtocol::ParseAndBind
//
//  Synopsis:   Actually perform the binding.
//              Derived classes should just implement this one method
//
//----------------------------------------------------------------------------

HRESULT
CBaseProtocol::ParseAndBind()
{
    if (_pProtSink)
    {
        _pProtSink->ReportResult(E_UNEXPECTED, 0, 0);
    }
    
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:     HasSecureContext
//
//  Synopsis:   Scans a \1-style URL to see if it should be treated as secure
//
//----------------------------------------------------------------------------
BOOL
HasSecureContext(const TCHAR *pchUrl)
{
    TCHAR *pchSpecial; 
    TCHAR *pchNext;
    TCHAR ach[pdlUrlLen]; 
    DWORD dwSize; 

    if (! THR(CoInternetParseUrl( 
        pchUrl, 
        PARSE_ENCODE, 
        0, 
        ach, 
        ARRAY_SIZE(ach), 
        &dwSize, 
        0))) 
    {
        // 1. scan for \1\1 - if present, we have mixed security somewhere along the way
        pchSpecial = ach;

        for (;;)
        {
            pchNext = _tcschr(pchSpecial, _T('\1'));
            
            if (!pchNext)
                break;
                
            pchSpecial = pchNext + 1;

            // Mixed security or missing context: not secure
            
            if (*pchSpecial == _T('\1') || *pchSpecial == _T('\0'))
                return FALSE;
        }
        
        // Last context is https: secure
        
        if (pchSpecial && URL_SCHEME_HTTPS == GetUrlScheme(pchSpecial)) 
            return TRUE; 
    }

    return FALSE;
}


