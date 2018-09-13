//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       resprot.cxx
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
#include "b4hook.hxx"
#include <tchar.h>

#define MAX_ID 10000


HRESULT LookupProtocolClsIDFromReg(LPCTSTR pszUrl, CLSID *pclsid);

#define SZPROTOCOLROOT  "PROTOCOLS\\Handler\\"
#define SZCLASS         "CLSID"

#define SZMYPROTOCOL    "search"



//+---------------------------------------------------------------------------
//
//  Method:     CB4Hook::Start
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
STDMETHODIMP CB4Hook::Start(LPCWSTR pwzUrl, IOInetProtocolSink *pTrans, IOInetBindInfo *pOIBindInfo,
                          DWORD grfSTI, DWORD dwReserved)
{
    TransDebugOut((DEB_PROT, "%p _IN CB4Hook::Start\n", this));
    HRESULT hr = NOERROR;
    CLSID clsid;

    TransAssert((!_pProtSink && pOIBindInfo && pTrans));
    TransAssert((_pszUrl == NULL));

    // have to start the base class to get the bindinfo and the full URL.

    hr = CBaseProtocol::Start(pwzUrl,pTrans, pOIBindInfo, grfSTI, dwReserved);

    if (hr == NOERROR)
    {
        // first, check if this is hookable URL.
        // return E_USEDEFAULTPROTOCAL if not.

        if ((hr = Bind()) == NOERROR)
        {
            // We need to use the new (cooked) URL if Bind() succeeded.

            if ((hr = LookupProtocolClsIDFromReg(_szNewUrl, &clsid)) == NOERROR)
            {
                IClassFactory *pCF = 0;
                hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER,IID_IClassFactory, (void**)&pCF);
                if (hr == NOERROR)
                {
                    // Perf: 
                    // we might want to move Create/Release of the HTTP protocl handler out of
                    // Start/Terminate to Create/Release.
    
                    hr = pCF->CreateInstance(NULL, IID_IOInetProtocol, (void **)&_pProt);
                    if (hr == NOERROR)
                    {
                        // ???
                        // We also may need to implement our own IOInetBindInfo so that we
                        // can give it the new (cooked) BindInfo.

                        // We need to pass down the new (cooked) URL if Bind() succeeded.

                        LPWSTR pwzNewUrl = DupA2W(_szNewUrl);
            
                        if (pwzNewUrl)
                        {
                            hr = _pProt->Start(pwzNewUrl, pTrans, pOIBindInfo, grfSTI, dwReserved);

                            delete pwzNewUrl;
                        }
                    }
    
                    pCF->Release();
                }
    
            }
        }
    }

    TransDebugOut((DEB_PROT, "%p OUT CB4Hook::Start (hr:%lx)\n",this, hr));
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CB4Hook::Continue
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
STDMETHODIMP CB4Hook::Continue(PROTOCOLDATA *pStateInfoIn)
{
    TransDebugOut((DEB_PROT, "%p _IN CB4Hook::Continue\n", this));
    HRESULT hr = E_FAIL;

    if (_pProt)
        hr = _pProt->Continue(pStateInfoIn);

    TransDebugOut((DEB_PROT, "%p OUT CB4Hook::Continue (hr:%lx)\n",this, hr));
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CB4Hook::Read
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
STDMETHODIMP CB4Hook::Read(void *pv,ULONG cb,ULONG *pcbRead)
{
    TransDebugOut((DEB_PROT, "%p _IN CB4Hook::Read (cb:%ld)\n", this,cb));
    HRESULT hr = NOERROR;

    if (_pProt)
        hr = _pProt->Read(pv, cb, pcbRead);

    TransDebugOut((DEB_PROT, "%p OUT CB4Hook::Read (pcbRead:%ld, hr:%lx)\n",this,*pcbRead, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CB4Hook::Seek
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
//  Notes:      WORK: not done
//
//----------------------------------------------------------------------------
STDMETHODIMP CB4Hook::Seek(LARGE_INTEGER dlibMove,DWORD dwOrigin,ULARGE_INTEGER *plibNewPosition)
{
    TransDebugOut((DEB_PROT, "%p _IN CB4Hook::Seek\n", this));
    HRESULT hr = NOERROR;

    if (_pProt)
        hr = _pProt->Seek(dlibMove, dwOrigin, plibNewPosition);

    TransDebugOut((DEB_PROT, "%p OUT CB4Hook::Seek (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CB4Hook::CB4Hook
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
CB4Hook::CB4Hook(REFCLSID rclsid, IUnknown *pUnkOuter, IUnknown **ppUnkInner) : CBaseProtocol(rclsid, pUnkOuter, ppUnkInner)
{
    TransDebugOut((DEB_PROT, "%p _IN/OUT CB4Hook::CB4Hook \n", this));
}

//+---------------------------------------------------------------------------
//
//  Method:     CB4Hook::~CB4Hook
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
CB4Hook::~CB4Hook()
{
    TransDebugOut((DEB_PROT, "%p _IN/OUT CB4Hook::~CB4Hook \n", this));
}


// SUPER HACK FUNCTION because InternetCrackUrl sucks.
// THIS FUNCTION DOES NOT CHECK THE INPUT BUFFER SIZE.
// AND CALLER MUST PROVIDE lpszUrlPath buffer for us to parse.

BOOL
MyCrackUrl(
    LPCSTR lpszUrl,
    DWORD dwUrlLength,
    DWORD dwFlags,
    LPURL_COMPONENTSA lpUC
)
{
    LPSTR pszBuf = lpUC->lpszUrlPath;
    DWORD dwSize = lpUC->dwUrlPathLength;
    BOOL ret = FALSE;

    if (pszBuf)
    {
        if (InternetCanonicalizeUrl(lpszUrl, pszBuf, &dwSize, ICU_DECODE | ICU_NO_ENCODE))
        {
            // find protocol

            LPSTR pTmp = StrChr(pszBuf, ':');

            if (pTmp)
            {
                *pTmp = '\0';

                if (lpUC->lpszScheme)
                {
                    lstrcpy(lpUC->lpszScheme, pszBuf);
                    lpUC->dwSchemeLength = pTmp - pszBuf;
                }

                pszBuf = ++pTmp;

                // skip '/'s

                while (*pszBuf && (*pszBuf == '/')) 
                    pszBuf++;

                // find host name

                pTmp = StrChr(pszBuf, '/');

                if (lpUC->lpszHostName)
                {
                    if (pTmp)
                        lpUC->dwHostNameLength = pTmp - pszBuf;
                    else
                        lpUC->dwHostNameLength = lstrlen(pszBuf);
    
                    // + 1 for the NULL terminator
    
                    lstrcpyn(lpUC->lpszHostName, pszBuf, lpUC->dwHostNameLength + 1);
                }

                // if a '/' was found, the rest is URL path

                if (pTmp)
                    lpUC->dwUrlPathLength = lstrlen(pTmp);
                else
                    lpUC->dwUrlPathLength = 0;

                lpUC->lpszUrlPath = pTmp;

                ret = TRUE;
            }
        }
    }

    return ret;
}


//+---------------------------------------------------------------------------
//
//  Method:     CB4Hook::Bind
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
STDMETHODIMP CB4Hook::Bind()
{
    TransDebugOut((DEB_PROT, "%p _IN CB4Hook::Bind (szUrl >%s< )\n", this, _pszUrl));

    HRESULT hr = INET_E_USE_DEFAULT_PROTOCOLHANDLER;
    URL_COMPONENTS uc;
    TCHAR szScheme[INTERNET_MAX_SCHEME_LENGTH];
    TCHAR szHost[INTERNET_MAX_HOST_NAME_LENGTH];
    TCHAR szURL[MAX_URL_SIZE];
    DWORD dwNeeded;
    LPSTR lpSearchName;
    LPSTR lpNewName = NULL;

    ZeroMemory(&uc, sizeof(uc));
    uc.dwStructSize = sizeof(uc);
    uc.lpszScheme = szScheme;
    uc.dwSchemeLength = ARRAYSIZE(szScheme);
    uc.lpszHostName = szHost;
    uc.dwHostNameLength = ARRAYSIZE(szHost);
    uc.lpszUrlPath = szURL;
    uc.dwUrlPathLength = ARRAYSIZE(szURL);

    // uc.dwExtraInfoLength ???

    // BUGBUG ???
    // InternetCrackUrl doesn't work with "search:\\..."

    if (MyCrackUrl(_pszUrl, 0, ICU_DECODE, &uc))
    {
        // TODO:
        // process the URL string

        if (!lstrcmpi(szScheme, SZMYPROTOCOL))
        {
            // if this is our "search:" protocol

            // BUGBUG
            // we need to look up the registry for the default protocol to use.

            uc.lpszScheme = NULL;
            uc.nScheme = INTERNET_SCHEME_HTTP;
            uc.nPort = INTERNET_DEFAULT_HTTP_PORT;

            if (uc.dwHostNameLength)
            {
                lpSearchName = uc.lpszHostName;
                uc.dwHostNameLength = 0;

                // ???
                // should we clear the UrlPath and ExtraInfo
            }
            else if (uc.dwUrlPathLength)
            {
                lpSearchName = uc.lpszUrlPath;
                uc.lpszUrlPath = NULL;
            }
            else
                lpSearchName = NULL;

            if (lpSearchName)
            {
                // apply the search.
                // ==========================================================
                if (!lstrcmpi(lpSearchName, "united airline"))
                    lpNewName = "www.ual.com";
                else if (!lstrcmpi(lpSearchName, "foo bar"))
                    lpNewName = "msw";
                // ==========================================================

                if (lpNewName)
                {
                    // this is a searchable string

                    uc.lpszHostName = lpNewName;
    
                    dwNeeded = ARRAYSIZE(_szNewUrl);
        
                    if (InternetCreateUrl(&uc, 0, _szNewUrl, &dwNeeded))
                        hr = NOERROR;
                }
            }
        }
        else if (uc.nScheme == INTERNET_SCHEME_HTTP)
        {
            lpSearchName = uc.lpszHostName;

            // apply search
            // ==========================================================
            if (!lstrcmpi(lpSearchName, "united airline"))
                lpNewName = "www.ual.com";
            else if (!lstrcmpi(lpSearchName, "foo bar"))
                lpNewName = "msw";
            // ==========================================================

            if (lpNewName)
            {
                // if search succeeded
                uc.lpszHostName = lpNewName;
                uc.dwHostNameLength = 0;
    
                dwNeeded = ARRAYSIZE(_szNewUrl);
    
                if (InternetCreateUrl(&uc, 0, _szNewUrl, &dwNeeded))
                    hr = NOERROR;
            }
        }
    }
    else
    {
        DebugBreak();

        DWORD dwError = GetLastError();
    }

    // if (hr == MK_E_SYNTAX)
    if (hr == INET_E_USE_DEFAULT_PROTOCOLHANDLER)
    {
        _pProtSink->ReportResult(hr, 0, 0);
    }

    TransDebugOut((DEB_PROT, "%p OUT CB4Hook::Bind (hr:%lx)\n", this,hr));
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   LookupProtocolClsIDFromReg
//
//  Synopsis:   finds a protocol handler class for a given URL
//
//  Arguments:  [pwzUrl] --
//              [pclsid] --
//
//  Returns:
//
//  History:    11-01-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT LookupProtocolClsIDFromReg(LPCTSTR pszUrl, CLSID *pclsid)
{
    TransDebugOut((DEB_PROT, "API _IN LookupProtocolClsIDFromReg (szUrl >%s< )\n", pszUrl));
    HRESULT hr = INET_E_UNKNOWN_PROTOCOL;
    DWORD dwType;
    TCHAR pszProt[MAX_URL_SIZE + 1];

    TransAssert((pszUrl && pclsid));

    if (pszUrl)
    {
        char szDelimiter = ':';

        lstrcpy(pszProt, pszUrl);

        LPSTR pszDel = StrChr(pszProt, szDelimiter);

        if (pszDel)
        {
            *pszDel = '\0';

            // fail if the protocol is "search" so we don't get call recursively.
            if (lstrcmpi(pszProt, SZMYPROTOCOL))
            {
                HKEY hProtocolKey = NULL;
                DWORD dwLen = 256;
                char szProtocolKey[256];

                lstrcpy(szProtocolKey, SZPROTOCOLROOT);
                lstrcat(szProtocolKey, pszProt);
    
                if (RegOpenKeyEx(HKEY_CLASSES_ROOT, szProtocolKey, 0, KEY_QUERY_VALUE, &hProtocolKey) == ERROR_SUCCESS)
                {
                    if (RegQueryValueEx(hProtocolKey, SZCLASS, NULL, &dwType, (LPBYTE)szProtocolKey, &dwLen) == ERROR_SUCCESS)
                    {
                        LPWSTR pwzClsId = DupA2W(szProtocolKey);
    
                        if (pwzClsId)
                        {
                            hr = CLSIDFromString(pwzClsId, pclsid);
                            TransDebugOut((DEB_PROT, "API FOUND LookupProtocolClsIDFromReg(hr:%lx, ClsId:%ws)\n", hr,pwzClsId));
                            delete pwzClsId;
                        }
                        else
                        {
                            hr = E_OUTOFMEMORY;
                        }
                    }
    
                    RegCloseKey(hProtocolKey);
                }
            }
        }
        else
        {
            // look up the registry
            hr = MK_E_SYNTAX;
        }
    }

    TransDebugOut((DEB_PROT, "API OUT LookupProtocolClsIDFromReg(hr:%lx)\n", hr));
    return hr;
}


