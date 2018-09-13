//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       abtprot.cxx
//
//  Contents:   Implementation of the about protocol
//
//  History:    07-23-97    krisma     Created
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_ABTPROT_HXX_
#define X_ABTPROT_HXX_
#include "abtprot.hxx"
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif

#ifndef X_ROSTM_HXX_
#define X_ROSTM_HXX_
#include "rostm.hxx"
#endif

#ifndef X_UWININET_H_
#define X_UWININET_H_
#include "uwininet.h"
#endif

#ifndef X_SHELLAPI_H_
#define X_SHELLAPI_H_
#include <shellapi.h>  // for the definition of ShellExecuteA (for AXP)
#endif

MtDefine(Protocols, Mem, "Protocols")
MtDefine(CAboutProtocol, Protocols, "CAboutProtocol")

// Some Static strings that we use to read from the registry
static TCHAR szAboutKey[] = 
    _T("SOFTWARE\\Microsoft\\Internet Explorer\\AboutURLs");


//+---------------------------------------------------------------------------
//
//  Function:   CreateAboutProtocol
//
//  Synopsis:   Creates an about Async Pluggable protocol
//
//  Arguments:  pUnkOuter   Controlling IUnknown
//
//----------------------------------------------------------------------------

CBase * 
CreateAboutProtocol(IUnknown *pUnkOuter)
{
    return new CAboutProtocol(pUnkOuter);
}

CAboutProtocolCF   g_cfAboutProtocol(CreateAboutProtocol);


#define GetStdLocationORD   150
typedef HRESULT (APIENTRY* PFNGETSTDLOCATION)(LPTSTR pszPath, DWORD cchPath, UINT id);

HRESULT GetShellStdLocation(LPTSTR lpszBuffer, DWORD cchBuffer, DWORD dwID)
{
    HRESULT hres = E_FAIL;
    HINSTANCE hinst;

    if (0 < cchBuffer)
        *lpszBuffer = 0;
        
    hinst = LoadLibrary(TEXT("shdocvw.dll"));
    if (hinst)
    {
        PFNGETSTDLOCATION pfn = (PFNGETSTDLOCATION)GetProcAddress(hinst, MAKEINTRESOURCEA(GetStdLocationORD));

        if (pfn)
        {
            hres = pfn(lpszBuffer, cchBuffer, dwID);
        }
        FreeLibrary(hinst);
    }

    return hres;
}

HRESULT
CAboutProtocolCF::QueryInfo(
    LPCWSTR       pwzUrl, 
    QUERYOPTION   QueryOption,
    DWORD         dwQueryFlags,
    LPVOID        pvBuffer,
    DWORD         cbBuffer,
    DWORD  *      pcbBuffer,
    DWORD         dwReserved)
{
    HRESULT     hr;
    HKEY        hkey = NULL;
    TCHAR *     szRegValue;
    LONG        lRegErrorValue;
    TCHAR       szBuffer[pdlUrlLen];
    DWORD       dwType;
    DWORD       cbData = sizeof(szBuffer);
    CStr        cstr;
    TCHAR      *pchSourceUrl = NULL;
    DWORD       dwRetval;

    //
    // Before resolving through registry, see if the \1 part forces the URL to be unsecure
    //

    if (QueryOption == QUERY_IS_SECURE && !HasSecureContext(pwzUrl))
    {
        dwRetval = FALSE;
        goto ReturnDword;
    }

    //
    // Search for an embedded \1.  If it's found, then strip it
    // out because that's the source url of this protocol.
    //
    pchSourceUrl = _tcschr(pwzUrl, _T('\1'));
    hr = pchSourceUrl ? 
            THR(cstr.Set(pwzUrl, pchSourceUrl - pwzUrl)) : 
            THR(cstr.Set(pwzUrl));

    szRegValue = _tcschr(cstr, ':');
    szRegValue += 1;
    
    lRegErrorValue = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE, szAboutKey, NULL, KEY_READ,
        &hkey);
    if (lRegErrorValue != ERROR_SUCCESS)
    {
        hr = INET_E_DEFAULT_ACTION;
        goto Cleanup;
    }

    lRegErrorValue = SHQueryValueEx(hkey, szRegValue, NULL, &dwType, (LPBYTE)szBuffer, &cbData);
    if (lRegErrorValue == ERROR_SUCCESS)
    {
        // If the value is in the registry as a DWORD then it must be a special Shell location.  Map the ID to the URL
        // by passing it to GetStdLocation.
        if (dwType == REG_DWORD)
        {
            if (FAILED(GetShellStdLocation(szBuffer, ARRAY_SIZE(szBuffer), *((LPDWORD)szBuffer))))
            {
                hr = INET_E_DEFAULT_ACTION;
                goto Cleanup;
            }
        }
        hr = THR(CoInternetQueryInfo(szBuffer, QueryOption, dwQueryFlags, pvBuffer, cbBuffer, pcbBuffer, dwReserved));
    }
    else
    {
        switch (QueryOption)
        {
        case QUERY_USES_NETWORK:
        case QUERY_IS_CACHED:
            dwRetval = FALSE;
            goto ReturnDword;
            
        case QUERY_IS_SECURE:
            dwRetval = TRUE;
            goto ReturnDword;
            
        default:
            hr = INET_E_DEFAULT_ACTION;
            break;
        }
    }

Cleanup:
    if (hkey)
        RegCloseKey(hkey);
    RRETURN1(hr, INET_E_DEFAULT_ACTION);

ReturnDword:
    if (!pvBuffer || cbBuffer < sizeof(DWORD))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if (pcbBuffer)
    {
        *pcbBuffer = sizeof(DWORD);
    }
    
    *(DWORD *)pvBuffer = dwRetval;
    hr = S_OK;

    goto Cleanup;
}

//+---------------------------------------------------------------------------
//
//  Method:     CAboutProtocolCF::ParseUrl
//
//  Synopsis:   per IInternetProtocolInfo. Reads new URL from 
//              registry and replaces the about: URL.
//
//----------------------------------------------------------------------------

HRESULT
CAboutProtocolCF::ParseUrl(
    LPCWSTR     pwzUrl,
    PARSEACTION ParseAction,
    DWORD       dwFlags,
    LPWSTR      pwzResult,
    DWORD       cchResult,
    DWORD *     pcchResult,
    DWORD       dwReserved)
{
    CStr        cstr;
    HRESULT     hr = INET_E_DEFAULT_ACTION;
    HKEY        hkey = NULL;
    TCHAR      *szRegValue;
    TCHAR      *pchSourceUrl = NULL;
    LONG        lRegErrorValue;
    TCHAR       szBuffer[pdlUrlLen];
    DWORD       dwType;
    DWORD       cbData = sizeof(szBuffer);

    if (!pcchResult || !pwzResult)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    if (ParseAction == PARSE_CANONICALIZE)
    {
        //
        // Search for an embedded \1.  If it's found, then strip it
        // out because that's the source url of this protocol.
        //
        pchSourceUrl = _tcschr(pwzUrl, _T('\1'));
        hr = pchSourceUrl ? 
                THR(cstr.Set(pwzUrl, pchSourceUrl - pwzUrl)) : 
                THR(cstr.Set(pwzUrl));

        szRegValue = _tcschr(cstr, ':');
        szRegValue += 1;

        //
        //  Special case for about:blank so
        //  we don't lose security info
        //
        if (!_tcsicmp(_T("blank"), szRegValue))
        {
            hr = THR(super::ParseUrl(
                pwzUrl,
                ParseAction,
                dwFlags,
                pwzResult,
                cchResult,
                pcchResult,
                dwReserved));
            goto Cleanup;
        }


        lRegErrorValue = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE, szAboutKey, NULL, KEY_READ,
            &hkey);
        if (lRegErrorValue != ERROR_SUCCESS)
        {
            hr=INET_E_DEFAULT_ACTION;
            goto Cleanup;
        }

        lRegErrorValue = SHQueryValueEx(
            hkey, szRegValue, NULL, &dwType, (LPBYTE)szBuffer, &cbData);
        if (lRegErrorValue == ERROR_SUCCESS)
        {
            // If the value is in the registry as a DWORD then it must be a special Shell location.  Map the ID to the URL
            // by passing it to GetStdLocation.
            if (dwType == REG_DWORD)
            {
                if (FAILED(GetShellStdLocation(szBuffer, ARRAY_SIZE(szBuffer), *((LPDWORD)szBuffer))))
                {
                    hr = INET_E_DEFAULT_ACTION;
                    goto Cleanup;
                }
            }
        }
        else
        {
            //
            //  If the registry value does not exist, procede as 
            //  normal (write the string to the document).
            //
            hr = THR(super::ParseUrl(
                pwzUrl,
                ParseAction,
                dwFlags,
                pwzResult,
                cchResult,
                pcchResult,
                dwReserved));
            goto Cleanup;
        }
        
        *pcchResult = cchResult;
        hr = UrlCanonicalize(szBuffer, pwzResult, pcchResult, dwFlags);

    }
    else if (ParseAction == PARSE_SECURITY_URL)
    {
        hr = THR(UnwrapSpecialUrl(pwzUrl, cstr));
        if (hr)
            goto Cleanup;
        
        *pcchResult = cstr.Length() + 1;
        if (cstr.Length() + 1 > cchResult)
        {
            // Not enough room, so fill in *pcchResult with size we need
            hr = S_FALSE;
            goto Cleanup;
        }

        _tcscpy(pwzResult, cstr);
    }
    else
    {
        hr = THR_NOTRACE(super::ParseUrl(
            pwzUrl,
            ParseAction,
            dwFlags,
            pwzResult,
            cchResult,
            pcchResult,
            dwReserved));
    }
    
Cleanup:
    if (hkey)
        RegCloseKey(hkey);

    RRETURN2(hr, INET_E_DEFAULT_ACTION, S_FALSE);
}


const CBase::CLASSDESC CAboutProtocol::s_classdesc =
{
    &CLSID_AboutProtocol,             // _pclsid
};


//+---------------------------------------------------------------------------
//
//  Method:     CAboutProtocol::CAboutProtocol
//
//  Synopsis:   ctor
//
//----------------------------------------------------------------------------

CAboutProtocol::CAboutProtocol(IUnknown *pUnkOuter) : super(pUnkOuter)
{
}


//+---------------------------------------------------------------------------
//
//  Method:     CAboutProtocol::~CAboutProtocol
//
//  Synopsis:   dtor
//
//----------------------------------------------------------------------------

CAboutProtocol::~CAboutProtocol()
{
}


//+---------------------------------------------------------------------------
//
//  Method:     CAboutProtocol::Start
//
//  Synopsis:   per IInternetProtocol
//
//----------------------------------------------------------------------------

HRESULT
CAboutProtocol::Start(
    LPCWSTR pchUrl, 
    IInternetProtocolSink *pTrans, 
    IInternetBindInfo *pOIBindInfo,
    DWORD grfSTI, 
    HANDLE_PTR dwReserved)
{
    HRESULT         hr = NOERROR;
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
    // If forced to go async, return E_PENDING now, and
    // perform binding when we get the Continue.
    //
    
    if (grfSTI & PI_FORCE_ASYNC)
    {
        PROTOCOLDATA    protdata;

        hr = E_PENDING;
        protdata.grfFlags = PI_FORCE_ASYNC;
        protdata.dwState = BIND_ASYNC;
        protdata.pData = NULL;
        protdata.cbData = 0;

        _pProtSink->Switch(&protdata);
    }
    else
    {
        hr = THR(ParseAndBind());
    }


Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Method:     CAboutProtocol::ParseAndBind
//
//  Synopsis:   Actually perform the binding & execution of script.
//
//----------------------------------------------------------------------------

void
CAboutProtocol::_ReportData(ULONG cb)
{
    _bscf |= BSCF_LASTDATANOTIFICATION | BSCF_DATAFULLYAVAILABLE;
    _pProtSink->ReportData(_bscf, cb, cb);
}

HRESULT
CAboutProtocol::ParseAndBind()
{
    HRESULT         hr = S_OK;
    TCHAR *         pch = NULL;
    TCHAR *         pchBuf = NULL;
    TCHAR *         pchSourceUrl = NULL;
    CStr            cstr;
    CStr            cstrBuf;
    ULONG           cb = NULL;
    CROStmOnBuffer *prostm = NULL;
    HINSTANCE       hInst = NULL;

    //
    // The url is of the following syntax:
    // about:<name>
    //
    
    Assert(_tcsnipre(_T("about:"), 6, _cstrURL, -1));

    //
    // Search for an embedded \1.  If it's found, then strip it
    // out because that's the source url of this protocol.
    //

    pchSourceUrl = _tcschr(_cstrURL, _T('\1'));
    hr = pchSourceUrl ? 
            THR(cstr.Set(_cstrURL, pchSourceUrl - _cstrURL)) : 
            THR(cstr.Set(_cstrURL));

    //
    // Do the binding.
    //

    pch = _tcschr(cstr, ':');

    if (!pch)
    {
        hr = MK_E_SYNTAX;
        goto Cleanup;
    }

    pch++;

    {
        TCHAR szBuf[] = _T(" <HTML>");
        szBuf[0] = NATIVE_UNICODE_SIGNATURE;

        hr = THR(cstrBuf.Set(szBuf));
    }

    if (hr)
        goto Cleanup;

    //
    //  Special case for about:blank. Just show a blank page.
    //
    if (_tcsicmp(_T("blank"), pch))
    {
        hr = THR(cstrBuf.Append(pch));
        if (hr)
            goto Cleanup;
    }

    hr = THR(cstrBuf.Append(_T("</HTML>")));
    if (hr)
        goto Cleanup;

    pchBuf = cstrBuf;
    cb = (cstrBuf.Length() + 1) * sizeof(TCHAR);

    hr = THR(_pProtSink->ReportProgress(
            BINDSTATUS_MIMETYPEAVAILABLE, 
            _T("text/html")));
    if (hr)
        goto Cleanup;

    Assert(pchBuf);
    
    // cb includes the null terminator
    
    prostm = new CROStmOnBuffer;
    if (!prostm)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(prostm->Init((BYTE *)pchBuf, cb));
    if (hr)
        goto Cleanup;

    _pStm = (IStream *)prostm;
    _pStm->AddRef();
        
Cleanup:
    if (!_fAborted)
    {
        if (!hr)
        {
            _ReportData(cb);
        }
        if (_pProtSink)
        {
            _pProtSink->ReportResult(hr, 0, 0);
        }
    }
    
    if (hInst)
    {
        FreeLibrary(hInst);
    }
    if (prostm)
    {
        prostm->Release();
    }
    RRETURN(hr);
}
