//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       wininet.cxx
//
//  Contents:   Dynamic wrappers for InternetCombineUrl
//
//----------------------------------------------------------------------------

#include "precomp.hxx"

#ifndef X_UWININET_H_
#define X_UWININET_H_
#include "uwininet.h"
#endif

#ifndef WIN16 // most of this file is not used for win16.

#ifndef X_UNICWRAP_HXX_
#define X_UNICWRAP_HXX_
#include "unicwrap.hxx"
#endif

DYNLIB g_dynlibWININET = { NULL, NULL, "WININET.DLL" };

#ifdef _MAC // temporarily redefine macros to warn about missing code.

#define WRAPIT_(type, fn, a1, a2)\
type WINAPI fn a1\
{\
    DEBUGSTR("\pWinInet Missing Implementation");\
    return (type) 0;\
}
#define WRAPIT(fn, a1, a2) WRAPIT_(BOOL, fn, a1, a2)

#define WRAPIT2_(type, fn, fn2, a1, a2)\
type WINAPI fn a1\
{\
    DEBUGSTR("\pWinInet Missing Implementation");\
    return (type) 0;\
}
#define WRAPIT2(fn, fn2, a1, a2) WRAPIT2_(BOOL, fn, fn2, a1, a2)

#else

#define WRAPIT_(type, fn, a1, a2)\
type WINAPI fn a1\
{\
    static DYNPROC s_dynproc##fn = { NULL, &g_dynlibWININET, #fn };\
    if(THR(LoadProcedure(&s_dynproc##fn)))\
        return FALSE;\
    return ((*(type (APIENTRY *) a1)s_dynproc##fn.pfn) a2);\
}
#define WRAPIT(fn, a1, a2) WRAPIT_(BOOL, fn, a1, a2)

#define WRAPIT2_(type, fn, fn2, a1, a2)\
type WINAPI fn a1\
{\
    static DYNPROC s_dynproc##fn = { NULL, &g_dynlibWININET, fn2 };\
    if(THR(LoadProcedure(&s_dynproc##fn)))\
        return FALSE;\
    return ((*(type (APIENTRY *) a1)s_dynproc##fn.pfn) a2);\
}
#define WRAPIT2(fn, fn2, a1, a2) WRAPIT2_(BOOL, fn, fn2, a1, a2)

#endif // _MAC

WRAPIT(InternetCanonicalizeUrlA,
    (LPCSTR lpszUrl, LPSTR lpszBuffer, LPDWORD lpdwBufferLength, DWORD dwFlags),
    (lpszUrl, lpszBuffer, lpdwBufferLength, dwFlags));

BOOL WINAPI InternetCanonicalizeUrlW(
    LPCWSTR lpszUrl,
    LPWSTR lpszBuffer,
    LPDWORD lpdwBufferLength,
    DWORD dwFlags)
{
    CStrIn  strInUrl(lpszUrl);
    CStrOut strOutBuffer(lpszBuffer, *lpdwBufferLength);
    BOOL fRet;

    fRet = InternetCanonicalizeUrlA(strInUrl, strOutBuffer, lpdwBufferLength, dwFlags);

    if (fRet)
    {
        *lpdwBufferLength = strOutBuffer.ConvertExcludingNul();
    }

    return fRet;
}

//
// Wrapper for CreateUrlCacheEntry
//-----------------------------------------------------------------------
WRAPIT(CreateUrlCacheEntryA, 
       (LPCSTR lpszUrlName, DWORD dwFileSize, LPCSTR lpszExt, LPSTR lpszFileName, DWORD dwRes),
       (lpszUrlName, dwFileSize, lpszExt, lpszFileName, dwRes));

BOOL WINAPI CreateUrlCacheEntryBugW( IN  LPCWSTR lpszUrlName, 
                                  IN  DWORD dwFileSize, 
                                  IN  LPCWSTR lpszExtension, 
                                  OUT LPWSTR lpszFileName, 
                                  IN  DWORD dwRes)
{
    CStrIn strInUrl(lpszUrlName);
    CStrIn strInExt(lpszExtension);
    CStrOut strOutName(lpszFileName, MAX_PATH);

    return CreateUrlCacheEntryA(strInUrl, dwFileSize, strInExt, strOutName, dwRes);
}


//
//  Wrapper for CommitUrlCacheEntry
//----------------------------------------------------------------------------
WRAPIT(CommitUrlCacheEntryA,
       (LPCSTR lpszUrl, LPCSTR lpszLocalName, FILETIME Expires, FILETIME lastMod, 
            DWORD dwType, LPCBYTE lpHeaderInfo, DWORD dwHeaderSize, LPCSTR lpszFileExtension, DWORD dwRes),
       (lpszUrl, lpszLocalName, Expires, lastMod, dwType, lpHeaderInfo, dwHeaderSize, lpszFileExtension, dwRes));

BOOL WINAPI CommitUrlCacheEntryBugW ( 
                                   IN LPCWSTR   lpszUrlName,
                                   IN LPCWSTR   lpszLocalFileName,
                                   IN FILETIME  ExpireTime,
                                   IN FILETIME  LastModifiedTime,
                                   IN DWORD     dwCachEntryType,
                                   IN LPCBYTE   lpHeaderInfo,
                                   IN DWORD     dwHeaderSize,
                                   IN LPCWSTR   lpszFileExtension,
                                   IN DWORD     dwReserved)
{
    CStrIn strInUrl(lpszUrlName);
    CStrIn strInFile(lpszLocalFileName);
    CStrIn strInExt(lpszFileExtension);

    return CommitUrlCacheEntryA(strInUrl, 
                                strInFile, 
                                ExpireTime, 
                                LastModifiedTime, 
                                dwCachEntryType, 
                                lpHeaderInfo, 
                                dwHeaderSize, 
                                strInExt,
                                dwReserved);
}


//
// Wrapper for GetUrlCacheEntryInfo
//--------------------------------------------------------------------------
WRAPIT(GetUrlCacheEntryInfoA,
    (LPCSTR lpszUrl, LPINTERNET_CACHE_ENTRY_INFO pcai, LPDWORD pcb),
    (lpszUrl, pcai, pcb));

BOOL WINAPI GetUrlCacheEntryInfoBugW(
    LPCWSTR lpszUrl,
    LPINTERNET_CACHE_ENTRY_INFO pcai,
    LPDWORD pcb)
{
    CStrIn strInUrl(lpszUrl);

    return GetUrlCacheEntryInfoA(strInUrl, pcai, pcb);
}


WRAPIT(GetUrlCacheEntryInfoExA,
    (IN LPCSTR lpszUrl,
    OUT LPINTERNET_CACHE_ENTRY_INFOW lpCacheEntryInfo,
    IN OUT LPDWORD lpdwCacheEntryInfoBufSize,
    OUT LPSTR lpszRedirectUrl,
    IN OUT LPDWORD lpdwRedirectUrlBufSize,
    LPVOID lpReserved,
    DWORD dwReserved),
    (lpszUrl, lpCacheEntryInfo,lpdwCacheEntryInfoBufSize,lpszRedirectUrl,lpdwRedirectUrlBufSize,lpReserved,dwReserved));
    


BOOL WINAPI
GetUrlCacheEntryInfoExBugW(
    IN LPCWSTR lpszUrl,
    OUT LPINTERNET_CACHE_ENTRY_INFOW lpCacheEntryInfo,
    IN OUT LPDWORD lpdwCacheEntryInfoBufSize,
    OUT LPSTR lpszRedirectUrl,
    IN OUT LPDWORD lpdwRedirectUrlBufSize,
    LPVOID lpReserved,
    DWORD dwReserved)
{
    CStrIn strInUrl(lpszUrl);
    
    return GetUrlCacheEntryInfoExA(strInUrl, lpCacheEntryInfo,
                                    lpdwCacheEntryInfoBufSize, lpszRedirectUrl,
                                    lpdwRedirectUrlBufSize, lpReserved,
                                    dwReserved);
}

WRAPIT2(DeleteUrlCacheEntryA, "DeleteUrlCacheEntry",
       (LPCSTR lpszUrlName),
       (lpszUrlName));

BOOL WINAPI
DeleteUrlCacheEntryBugW(LPCWSTR lpszUrlName)
{
    CStrIn strInUrl(lpszUrlName);

    return DeleteUrlCacheEntryA(strInUrl);
}


WRAPIT(SetUrlCacheEntryInfoA,
       (LPCSTR lpszUrlName, LPINTERNET_CACHE_ENTRY_INFO lpCacheEntryInfo,
        DWORD dwFieldControl),
       (lpszUrlName, lpCacheEntryInfo, dwFieldControl));

BOOL WINAPI SetUrlCacheEntryInfoBugW(
    LPCWSTR lpszUrlName,                                     
    LPINTERNET_CACHE_ENTRY_INFO lpCacheEntryInfo,
    DWORD dwFieldControl)
{
    CStrIn strInUrl(lpszUrlName);

    return SetUrlCacheEntryInfoA(strInUrl, lpCacheEntryInfo, dwFieldControl);
}




WRAPIT(InternetQueryOptionA,
    (IN HINTERNET hInternet,
    IN DWORD dwOption,
    OUT LPVOID lpBuffer,
    IN OUT LPDWORD lpdwBufferLength),
    (hInternet, dwOption, lpBuffer, lpdwBufferLength));






WRAPIT(RetrieveUrlCacheEntryFileA,
    (LPCSTR lpszUrl, LPINTERNET_CACHE_ENTRY_INFO pcai, LPDWORD pcb, DWORD res),
    (lpszUrl, pcai, pcb, res));

BOOL WINAPI RetrieveUrlCacheEntryFileBugW(
    LPCWSTR lpszUrl,
    LPINTERNET_CACHE_ENTRY_INFO pcai,
    LPDWORD pcb,
    DWORD res)
{
#ifndef UNIX
    BYTE                        buf[MAX_CACHE_ENTRY_INFO_SIZE];
    INTERNET_CACHE_ENTRY_INFO * pInfo = (INTERNET_CACHE_ENTRY_INFO *) buf;
#else
    union
    {
        double alignOn8ByteBoundary;
        BYTE   alignedBuf[MAX_CACHE_ENTRY_INFO_SIZE];
    } buf;
    INTERNET_CACHE_ENTRY_INFO * pInfo = (INTERNET_CACHE_ENTRY_INFO *)&buf;
#endif // UNIX

    DWORD                       cInfo = sizeof(buf);
    CStrIn                      strInUrl(lpszUrl);

    if (!RetrieveUrlCacheEntryFileA(strInUrl, pInfo, &cInfo, res))
    {
        // BUGBUG#### (rodc) This test is here because sometimes an error is
        // reported even if the local file can be returned. In this case
        // ERROR_INVALID_DATA is the last error. Since all we want is the
        // file name pretend all is well.
        if (GetLastError() != ERROR_INVALID_DATA)
            return FALSE;
    }

    cInfo = *pcb - sizeof(INTERNET_CACHE_ENTRY_INFO);
    pcai->lpszLocalFileName = (TCHAR *) (((BYTE *) pcai) + sizeof(INTERNET_CACHE_ENTRY_INFO));
    MultiByteToWideChar(
            CP_ACP,
            0,
            (char *) pInfo->lpszLocalFileName,
            -1,
            pcai->lpszLocalFileName,
            cInfo);
    pcai->LastModifiedTime.dwHighDateTime = pInfo->LastModifiedTime.dwHighDateTime;
    pcai->LastModifiedTime.dwLowDateTime = pInfo->LastModifiedTime.dwLowDateTime;
    pcai->ExpireTime.dwHighDateTime = pInfo->LastModifiedTime.dwHighDateTime;
    pcai->ExpireTime.dwLowDateTime = pInfo->LastModifiedTime.dwLowDateTime;

    return TRUE;
}

WRAPIT(InternetCrackUrlA,
    (LPCSTR lpszUrl, DWORD dwUrlLength, DWORD dwFlags, LPURL_COMPONENTS lpUrlComponents),
    (lpszUrl, dwUrlLength, dwFlags, lpUrlComponents));

#ifndef WINCE
BOOL WINAPI InternetCrackUrlW(
    IN LPCWSTR lpszUrl,
    IN DWORD dwUrlLength,
    IN DWORD dwFlags,
    IN OUT LPURL_COMPONENTS puc
    )
{
    CStrIn  strInUrl(lpszUrl);
    CStrOut strOutScheme    (puc->lpszScheme,   puc->dwSchemeLength);
    CStrOut strOutHostName  (puc->lpszHostName, puc->dwHostNameLength);
    CStrOut strOutUserName  (puc->lpszUserName, puc->dwUserNameLength);
    CStrOut strOutPassword  (puc->lpszPassword, puc->dwPasswordLength);
    CStrOut strOutUrlPath   (puc->lpszUrlPath,  puc->dwUrlPathLength);
    CStrOut strOutExtraInfo (puc->lpszExtraInfo,puc->dwExtraInfoLength);
    BOOL    fRet;

 #ifdef _MAC
    URL_COMPONENTSA ucA;

    Assert(puc->dwStructSize==sizeof(URL_COMPONENTS));

    memmove(&ucA, puc, sizeof(ucA));

    ucA.lpszScheme       = ((LPSTR)strOutScheme);
    ucA.lpszHostName     = ((LPSTR)strOutHostName);
    ucA.lpszUserName     = ((LPSTR)strOutUserName);
    ucA.lpszPassword     = ((LPSTR)strOutPassword);
    ucA.lpszUrlPath      = ((LPSTR)strOutUrlPath);
    ucA.lpszExtraInfo    = ((LPSTR)strOutExtraInfo);

    fRet = InternetCrackUrlA((char*)strInUrl, dwUrlLength, dwFlags, &ucA);

#else

   URL_COMPONENTS ucA;

    Assert(puc->dwStructSize==sizeof(URL_COMPONENTS));

    memmove(&ucA, puc, sizeof(ucA));

    ucA.lpszScheme       = (LPTSTR)((LPSTR)strOutScheme);
    ucA.lpszHostName     = (LPTSTR)((LPSTR)strOutHostName);
    ucA.lpszUserName     = (LPTSTR)((LPSTR)strOutUserName);
    ucA.lpszPassword     = (LPTSTR)((LPSTR)strOutPassword);
    ucA.lpszUrlPath      = (LPTSTR)((LPSTR)strOutUrlPath);
    ucA.lpszExtraInfo    = (LPTSTR)((LPSTR)strOutExtraInfo);

    fRet = InternetCrackUrlA(strInUrl, dwUrlLength, dwFlags, &ucA);

#endif // _MAC

    if (fRet)
    {
        puc->dwStructSize       = ucA.dwStructSize;
        puc->dwSchemeLength     = strOutScheme.ConvertExcludingNul();
        puc->nScheme            = ucA.nScheme;
        puc->dwHostNameLength   = strOutHostName.ConvertExcludingNul();
        puc->nPort              = ucA.nPort;
        puc->dwUserNameLength   = strOutUserName.ConvertExcludingNul();
        puc->dwPasswordLength   = strOutPassword.ConvertExcludingNul();
        puc->dwUrlPathLength    = strOutUrlPath.ConvertExcludingNul();
        puc->dwExtraInfoLength  = strOutExtraInfo.ConvertExcludingNul();
    }
    else
    {
        puc->dwStructSize       = ucA.dwStructSize;
        puc->dwSchemeLength     = ucA.dwSchemeLength;
        puc->nScheme            = ucA.nScheme;
        puc->dwHostNameLength   = ucA.dwHostNameLength;
        puc->nPort              = ucA.nPort;
        puc->dwUserNameLength   = ucA.dwUserNameLength;
        puc->dwPasswordLength   = ucA.dwPasswordLength;
        puc->dwUrlPathLength    = ucA.dwUrlPathLength;
        puc->dwExtraInfoLength  = ucA.dwExtraInfoLength;
    }

    return fRet;
}
#else //WINCE
WRAPIT(InternetCrackUrlW,
    (LPCWSTR lpszUrl, DWORD dwUrlLength, DWORD dwFlags, LPURL_COMPONENTS lpUrlComponents),
    (lpszUrl, dwUrlLength, dwFlags, lpUrlComponents));
#endif //WINCE

WRAPIT(InternetCreateUrlA,
    (LPURL_COMPONENTS lpUrlComponents, DWORD dwFlags, LPSTR lpszUrl, LPDWORD lpdwUrlLength),
    (lpUrlComponents, dwFlags, lpszUrl, lpdwUrlLength));

BOOL WINAPI InternetCreateUrlW(
    IN LPURL_COMPONENTS puc,
    IN DWORD dwFlags,
    OUT LPWSTR lpszUrl,
    IN OUT LPDWORD lpdwUrlLength
    )
{
    CStrOut  strOutUrl(lpszUrl, *lpdwUrlLength);

    CStrIn strInScheme    (puc->lpszScheme,   puc->dwSchemeLength);
    CStrIn strInHostName  (puc->lpszHostName, puc->dwHostNameLength);
    CStrIn strInUserName  (puc->lpszUserName, puc->dwUserNameLength);
    CStrIn strInPassword  (puc->lpszPassword, puc->dwPasswordLength);
    CStrIn strInUrlPath   (puc->lpszUrlPath,  puc->dwUrlPathLength);
    CStrIn strInExtraInfo (puc->lpszExtraInfo,puc->dwExtraInfoLength);
    BOOL    fRet;

    URL_COMPONENTS ucA;
    
    Assert(puc->dwStructSize==sizeof(URL_COMPONENTS));

    // Only the pointer members need to be changed, so copy all the
    // data and then set the pointer members to their correct values.
    memmove(&ucA, puc, sizeof(ucA));

    ucA.lpszScheme       = (LPTSTR)((LPSTR)strInScheme);
    ucA.lpszHostName     = (LPTSTR)((LPSTR)strInHostName);
    ucA.lpszUserName     = (LPTSTR)((LPSTR)strInUserName);
    ucA.lpszPassword     = (LPTSTR)((LPSTR)strInPassword);
    ucA.lpszUrlPath      = (LPTSTR)((LPSTR)strInUrlPath);
    ucA.lpszExtraInfo    = (LPTSTR)((LPSTR)strInExtraInfo);

    fRet = InternetCreateUrlA(&ucA, dwFlags, strOutUrl, lpdwUrlLength);

    if (fRet)
    {
        *lpdwUrlLength = strOutUrl.ConvertExcludingNul();
    }
    return fRet;
}
  
WRAPIT(InternetGetCertByURL,
        (LPCTSTR lpszURL, LPTSTR lpszCertText, DWORD dwcbCertText),
        (lpszURL, lpszCertText, dwcbCertText));

WRAPIT(InternetShowSecurityInfoByURL,
       (LPTSTR lpszURL, HWND hwndParent),
       (lpszURL, hwndParent));

WRAPIT(InternetAlgIdToStringA,
        (ALG_ID algID, LPSTR lpsz, LPDWORD lpdw, DWORD dwReserved),
        (algID, lpsz, lpdw, dwReserved));

WRAPIT(InternetAlgIdToStringW,
        (ALG_ID algID, LPWSTR lpwz, LPDWORD lpdw, DWORD dwReserved),
        (algID, lpwz, lpdw, dwReserved));

WRAPIT(InternetSecurityProtocolToStringA,
        (DWORD dwProtocol, LPSTR lpsz, LPDWORD lpdw, DWORD dwReserved),
        (dwProtocol, lpsz, lpdw, dwReserved));

WRAPIT(InternetSecurityProtocolToStringW,
        (DWORD dwProtocol, LPWSTR lpwz, LPDWORD lpdw, DWORD dwReserved),
        (dwProtocol, lpwz, lpdw, dwReserved));

WRAPIT_(HANDLE, FindFirstUrlCacheEntryA,
    (LPCSTR lpszUrlSearchPattern, LPINTERNET_CACHE_ENTRY_INFOA lpFirstCacheEntryInfo, LPDWORD lpdwFirstCacheEntryInfoBufferSize),
    (lpszUrlSearchPattern, lpFirstCacheEntryInfo, lpdwFirstCacheEntryInfoBufferSize));
WRAPIT(FindNextUrlCacheEntryA,
    (HANDLE hEnumHandle, LPINTERNET_CACHE_ENTRY_INFOA lpNextCacheEntryInfo, LPDWORD lpdwNextCacheEntryInfoBufferSize),
    (hEnumHandle, lpNextCacheEntryInfo, lpdwNextCacheEntryInfoBufferSize));
WRAPIT(DeleteUrlCacheEntry,
    (LPCSTR lpszUrlName),
    (lpszUrlName));
WRAPIT(FindCloseUrlCache,
    (HANDLE hEnumHandle),
    (hEnumHandle));

WRAPIT_(DWORD, InternetErrorDlg,
     (HWND hWnd, HINTERNET hRequest, DWORD dwError, DWORD dwFlags, LPVOID * lppvData),
     (hWnd, hRequest, dwError, dwFlags, lppvData));


#endif // ndef WIN16
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//      Misc
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

BOOL
IsURLSchemeCacheable(UINT uScheme)
{
    switch(uScheme)
    {
    case URL_SCHEME_HTTP:
    case URL_SCHEME_HTTPS:
    case URL_SCHEME_GOPHER:
    case URL_SCHEME_FTP:
        return TRUE;
    }
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   IsSecureUrl
//
//  Synopsis:   Returns TRUE for https, FALSE for anything else
//
//              Starting with an IE 4.01 QFE in 1/98, IsUrlSecure returns TRUE 
//              for javascript:, vbscript:, and about: if the source 
//              document in the wrapped URL is secure. 
// 
//----------------------------------------------------------------------------
BOOL
IsUrlSecure(const TCHAR *pchUrl)
{
    BOOL fSecure;
    ULONG cb;
    
    if (!pchUrl)
        return FALSE;

    switch(GetUrlScheme(pchUrl))
    {
    case URL_SCHEME_HTTPS:
        return TRUE;

    case URL_SCHEME_HTTP:
    case URL_SCHEME_FILE:
    case URL_SCHEME_RES:
    case URL_SCHEME_FTP:
        return FALSE;

    default:
        if (!CoInternetQueryInfo(pchUrl, QUERY_IS_SECURE, 0, &fSecure, sizeof(fSecure), &cb, 0) && cb == sizeof(fSecure))
            return fSecure;
        return FALSE;
    }
}

UINT
GetUrlScheme(const TCHAR * pchUrlIn)
{
    PARSEDURL      puw = {0};

    if (!pchUrlIn)
        return (UINT)URL_SCHEME_INVALID;

    puw.cbSize = sizeof(PARSEDURL);

    return (SUCCEEDED(ParseURL(pchUrlIn, &puw))) ?
                puw.nScheme : URL_SCHEME_INVALID;
}

HRESULT
GetUrlComponentHelper(const TCHAR * pchUrlIn,
                      CStr *        pstrComp,
                      DWORD         dwFlags,
                      URLCOMP_ID    ucid)
{
    HRESULT         hr = E_INVALIDARG;
    URL_COMPONENTS  uc;
    TCHAR           achUrl[pdlUrlLen];
    TCHAR           achComp[pdlUrlLen];
    DWORD           dw, dwLength = ARRAY_SIZE(achUrl);
    TCHAR           chTarget, chDelimit;

    if (!pstrComp || !pchUrlIn)
        goto Cleanup;

    if (!InternetCanonicalizeUrl(pchUrlIn, achUrl, &dwLength, dwFlags))
        goto Cleanup;

    hr = S_OK;

    // Clear everything and set only those fields that we are interested in
    memset(&uc, 0, sizeof(uc));
    uc.dwStructSize = sizeof(URL_COMPONENTS);
    switch(ucid)
    {
    case URLCOMP_HOST:
    case URLCOMP_HOSTNAME:
        uc.lpszHostName = achComp;
        uc.dwHostNameLength = ARRAY_SIZE(achComp);
        break;
    case URLCOMP_PATHNAME:
        uc.lpszUrlPath = achComp;
        uc.dwUrlPathLength = ARRAY_SIZE(achComp);
        break;
    case URLCOMP_PROTOCOL:
        uc.lpszScheme = achComp;
        uc.dwSchemeLength = ARRAY_SIZE(achComp);
        break;
    case URLCOMP_HASH:
    case URLCOMP_SEARCH:
        uc.lpszExtraInfo = achComp;
        uc.dwExtraInfoLength = ARRAY_SIZE(achComp);
        break;
    }

    if (!InternetCrackUrl(achUrl, 0, 0, &uc))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    switch(ucid)
    {
    case URLCOMP_HOST:
        hr = THR(pstrComp->Set(uc.lpszHostName));
        if (hr)
            goto Cleanup;
        if (uc.nPort)
        {
            hr = THR(pstrComp->Append(_T(":")));
            if (hr)
                goto Cleanup;
            _itot(uc.nPort, achComp, 10);
            hr = THR(pstrComp->Append(achComp));
        }
        break;
    case URLCOMP_HOSTNAME:
        hr = THR(pstrComp->Set(uc.lpszHostName));
        break;
    case URLCOMP_PATHNAME:
        // get rid of the leading '/' if any
        if (_T('/') == uc.lpszUrlPath[0])
        {
            uc.lpszUrlPath++;
            uc.dwUrlPathLength--;
        }

        // ignore the 'search' portion starting from the first
        // '?' or '#'
        for (dw = 0; dw < uc.dwUrlPathLength; dw++)
        {
            if (_T('?') == uc.lpszUrlPath[dw] ||
                _T('#') == uc.lpszUrlPath[dw])
            {
                uc.dwUrlPathLength = dw;
                break;
            }
        }
        hr = THR(pstrComp->Set(uc.lpszUrlPath, uc.dwUrlPathLength));
        break;
    case URLCOMP_PORT:
        _itot(uc.nPort, achComp, 10);
        hr = THR(pstrComp->Set(achComp));
        break;
    case URLCOMP_PROTOCOL:
        hr = THR(pstrComp->Set(uc.lpszScheme));
        if (hr)
            goto Cleanup;
        hr = THR(pstrComp->Append(_T(":")));
        break;

    case URLCOMP_HASH:
        chTarget = _T('#');
        chDelimit = _T('?');
        goto ExtractExtraInfo;
        break;

    case URLCOMP_SEARCH:
        chTarget = _T('?');
        chDelimit = _T('#');
ExtractExtraInfo:
        // extra info returns both hash/search in the same slot. Find the part
        // that has ?abc
        {
            TCHAR * pchSearch = _tcschr(uc.lpszExtraInfo, chTarget); 
            if (pchSearch)
            {
                TCHAR * pchHashPart = NULL;
                DWORD   cch = uc.dwExtraInfoLength;

                // for N.S. compatability, leave the chTarget on the front,
                // unless its empty
                if (cch==1)
                {
                    cch--;
                    pchSearch++;
                }

                pchHashPart = _tcschr(pchSearch, chDelimit);

                cch = (pchHashPart) ? pchHashPart - pchSearch : _tcslen(pchSearch);

                hr = THR(pstrComp->Set(pchSearch, cch));
            }
        }
        break;

    }

Cleanup:
    RRETURN(hr);
}


HRESULT
SetUrlComponentHelper(const TCHAR * pchUrlIn,
                      TCHAR       * pchUrlOut,
                      DWORD         dwBufLen,
                      const BSTR  * pstrOriginal,
                      URLCOMP_ID    ucid)
{
    HRESULT         hr = E_INVALIDARG;
    URL_COMPONENTS  uc;
    TCHAR           achHostName[INTERNET_MAX_HOST_NAME_LENGTH];
    TCHAR           achScheme[INTERNET_MAX_SCHEME_LENGTH];
    TCHAR           achUrlPath[INTERNET_MAX_PATH_LENGTH];
    TCHAR           achExtraInfo[INTERNET_MAX_PATH_LENGTH];
    TCHAR           achUserName[INTERNET_MAX_USER_NAME_LENGTH];
    TCHAR           achPassword[INTERNET_MAX_PASSWORD_LENGTH];
    DWORD           dwLength = dwBufLen;
    TCHAR *         pchPort = NULL;
    BOOL            fPrefixExists;
    TCHAR *         pstrComp = NULL;
    int             iCompLen;
    
 
    if (!pstrOriginal || dwLength < pdlUrlLen)
        goto Cleanup;
    if (!pchUrlIn || _tcslen(pchUrlIn) >= dwLength)
        goto Cleanup;
    if (!InternetCanonicalizeUrl(pchUrlIn, pchUrlOut, &dwLength, 0))
        goto Cleanup;

    hr = S_OK;

    uc.dwStructSize = sizeof(URL_COMPONENTS);
    uc.lpszScheme = achScheme;
    uc.dwSchemeLength = ARRAY_SIZE(achScheme);
    uc.lpszHostName = achHostName;
    uc.dwHostNameLength = ARRAY_SIZE(achHostName);
    uc.lpszUserName = achUserName;
    uc.dwUserNameLength = ARRAY_SIZE(achUserName);
    uc.lpszPassword = achPassword;
    uc.dwPasswordLength = ARRAY_SIZE(achPassword);
    uc.lpszUrlPath = achUrlPath;
    uc.dwUrlPathLength = ARRAY_SIZE(achUrlPath);
    uc.lpszExtraInfo = achExtraInfo;
    uc.dwExtraInfoLength = ARRAY_SIZE(achExtraInfo);
    uc.nPort = 0;

    if (!_tcslen(pchUrlOut))
    {
        // there is no current url, so set all fields to 0, we will
        // only set the one ...
        uc.dwSchemeLength = 0;
        uc.dwHostNameLength = 0;
        uc.dwUserNameLength = 0;
        uc.dwPasswordLength = 0;
        uc.dwUrlPathLength = 0;
        uc.dwExtraInfoLength = 0;
    }
    else if (!InternetCrackUrl(pchUrlOut, 0, 0, &uc))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }


    iCompLen = SysStringLen(*pstrOriginal);
    pstrComp = *pstrOriginal;

    // pull off leading and trailing /'s for NS compatibility
    while (iCompLen && (*pstrComp == _T('/') || *pstrComp == _T('\\')))
    {
        pstrComp++;
        iCompLen--;
    }

    while (iCompLen && (*(pstrComp+iCompLen-1) == _T('/') || 
                        *(pstrComp+iCompLen-1) ==_T('\\')))
    {
        iCompLen--;
    }

    switch(ucid)
    {
    case URLCOMP_HOST:
        // set port first
        pchPort = _tcsrchr(pstrComp, _T(':'));
        if (pchPort)
            uc.nPort = (USHORT)StrToInt(pchPort+1);
        // fall through
    case URLCOMP_HOSTNAME:
        uc.dwHostNameLength = min( (LONG)((pchPort) ? pchPort - pstrComp : (UINT)iCompLen),
                                   (LONG)(ARRAY_SIZE(achHostName) -1));
        _tcsncpy(uc.lpszHostName, pstrComp, uc.dwHostNameLength);
        break;

    case URLCOMP_PATHNAME:
        uc.dwUrlPathLength = min( (UINT)iCompLen, 
                                  (UINT) ARRAY_SIZE(achUrlPath)-1 );
        _tcsncpy(uc.lpszUrlPath, pstrComp, uc.dwUrlPathLength);
        break;
    case URLCOMP_PORT:
        uc.nPort = (USHORT)StrToInt(pstrComp);
        break;
    case URLCOMP_PROTOCOL:
        uc.dwSchemeLength = min( (UINT)iCompLen,
                                 (UINT) ARRAY_SIZE(achScheme));
        _tcsncpy(uc.lpszScheme, pstrComp, uc.dwSchemeLength);

        // Remove trailing ':' if any.
        if (uc.dwSchemeLength > 0 &&
            _T(':') == uc.lpszScheme[uc.dwSchemeLength - 1])
        {
             uc.lpszScheme[--uc.dwSchemeLength] = 0;
        }
        break;
    case URLCOMP_SEARCH:
        // Must prefix this with '?'
        fPrefixExists = (uc.dwExtraInfoLength > 0);
        uc.dwExtraInfoLength = iCompLen + 1;
        if (uc.dwExtraInfoLength >= ARRAY_SIZE(achExtraInfo))
            goto Cleanup;
        uc.lpszExtraInfo[0] = _T('?');
         _tcsncpy(uc.lpszExtraInfo + 1, pstrComp, iCompLen);
        break;
    case URLCOMP_HASH:
        // Must prefix this with '#'
        fPrefixExists = (uc.dwExtraInfoLength > 0);
        uc.dwExtraInfoLength = iCompLen + 1;
        if (uc.dwExtraInfoLength >= ARRAY_SIZE(achExtraInfo))
            goto Cleanup;
        uc.lpszExtraInfo[0] = _T('#');
         _tcsncpy(uc.lpszExtraInfo + 1, pstrComp, iCompLen);
        break;
    }

    dwLength = dwBufLen;
    hr = THR(ComposeUrl(&uc, 0, pchUrlOut, & dwLength));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------
//
//  method  :   ComposeUrl()
//
//  Synopsis : The core of this was stolen from ie, and modified to be 
//      more general and robust.  it creates a single url string from
//      the componenet pieces.
//
//----------------------------------------------------------------

HRESULT
ComposeUrl( URL_COMPONENTS *puc,
            DWORD           dwFlags,
            TCHAR         * pchUrlOut,
            DWORD         * pdwSize)
{
	HRESULT   hr = S_OK;
    ULONG     lRequiredSize;
    TCHAR     achSchemeSep[]  = _T("://");
    TCHAR     achPort[18];
    TCHAR   * pchCopyHere = NULL;
    const int iSchemeSepLen = _tcslen(achSchemeSep);
    int       nPortLength;
    int       iTch =  sizeof(TCHAR);

    // set up for the port component
    _itot(puc->nPort, achPort, 10);
    nPortLength = _tcslen(achPort);

    // is there enough space in the buffer?
    lRequiredSize = puc->dwSchemeLength    +     // http
                   iSchemeSepLen          +     // ://
                   puc->dwHostNameLength  +     // www.myserver.org
                   nPortLength  + 1       +     // :##
                   puc->dwUserNameLength  +
                   ((puc->dwUserNameLength != 0) ? 1 : 0) +  // +1 for '@'
                   puc->dwPasswordLength  +
                   ((puc->dwPasswordLength != 0) ? 1 : 0) +  // +1 for ':'
                   1                      +     // /
                   puc->dwUrlPathLength   +     // the rest
                   1                      +     // '#' or '?'
                   puc->dwExtraInfoLength +     // search string, hash-name
                   1;                           // +1 for '\0'

    if (lRequiredSize > *pdwSize)
    {
        hr = STG_E_INSUFFICIENTMEMORY;
        goto Cleanup;
    }

    // start building the string...
    pchCopyHere = pchUrlOut;

    if (puc->dwSchemeLength)
    {
	    _tcsncpy(pchCopyHere, puc->lpszScheme, puc->dwSchemeLength);
        pchCopyHere += puc->dwSchemeLength;

        _tcsncpy(pchCopyHere, achSchemeSep, iSchemeSepLen);
        pchCopyHere += iSchemeSepLen;
    }

    if (puc->dwUserNameLength)
    {
        _tcsncpy(pchCopyHere, puc->lpszUserName, puc->dwUserNameLength);
        pchCopyHere += puc->dwUserNameLength;

        if (puc->dwPasswordLength)
        {
            _tcsncpy(pchCopyHere, _T(":"), iTch);
            pchCopyHere ++;

            _tcsncpy(pchCopyHere, puc->lpszPassword, puc->dwPasswordLength);
            pchCopyHere += puc->dwPasswordLength;
        }
        _tcsncpy(pchCopyHere, _T("@"), iTch);
        pchCopyHere ++;
    }

    _tcsncpy(pchCopyHere, puc->lpszHostName, puc->dwHostNameLength);
    pchCopyHere += puc->dwHostNameLength;

    if (nPortLength)
    {
        _tcsncpy(pchCopyHere, _T(":"), iTch);
        pchCopyHere ++;

        _tcsncpy(pchCopyHere, achPort, nPortLength);
        pchCopyHere += nPortLength;
    }

    if (puc->dwUrlPathLength)
    {
        if (puc->dwHostNameLength && *puc->lpszUrlPath != _T('/'))
        {
            _tcsncpy(pchCopyHere, _T("/"), iTch);
            pchCopyHere ++;
        }

        _tcsncpy(pchCopyHere, puc->lpszUrlPath, puc->dwUrlPathLength);
        pchCopyHere += puc->dwUrlPathLength;
    }

    if (puc->dwExtraInfoLength)
    {
        _tcsncpy(pchCopyHere, puc->lpszExtraInfo, puc->dwExtraInfoLength);
        pchCopyHere += puc->dwExtraInfoLength;
    }

    *pchCopyHere= _T('\0');

Cleanup:
    RRETURN(hr);
}


#ifndef WIN16

WRAPIT(InternetGetCookie,
       (LPCTSTR lpszUrl, LPCTSTR lpszCookieName, LPTSTR lpCookieData,
        LPDWORD lpdwSize),
       (lpszUrl, lpszCookieName, lpCookieData, lpdwSize));

WRAPIT(InternetSetCookie,
       (LPCTSTR lpszUrl, LPCTSTR lpszCookieName, LPCTSTR lpszCookieData),
       (lpszUrl, lpszCookieName, lpszCookieData));

//+-----------------------------------------------------------------------
//
// GetDateFormat() and GetTimeFormat() only have unicode version on NT
//
//-----------------------------------------------------------------------

WRAPIT(GetDateFormatA,
       (LCID locale, DWORD dwFlags, CONST SYSTEMTIME * lpDate, LPCTSTR lpFormat, 
            LPTSTR lpDateStr, int cchDate),
       (locale, dwFlags, lpDate, lpFormat, lpDateStr, cchDate));

int WINAPI GetDateFormat_BugW( IN LCID Locale, 
                               IN DWORD dwFlags, 
                               IN CONST SYSTEMTIME * lpDate, 
                               IN LPCTSTR lpFormat,
                               OUT LPTSTR lpDateStr, 
                               IN int cchDate)
{
#ifndef WINCE
    int    iValue;
    CStrIn strInFormat(lpFormat);
    CStrOut strOutDateStr(lpDateStr, cchDate);

    iValue = GetDateFormatA(Locale, dwFlags, lpDate, strInFormat, strOutDateStr, cchDate);

    if (iValue)
    {
        iValue = strOutDateStr.ConvertIncludingNul();
    }

    return iValue;
#else //WINCE
	return GetDateFormat(Locale, dwFlags, lpDate, lpFormat, lpDateStr, cchDate);
#endif //WINCE
}

WRAPIT(GetTimeFormatA,
       (LCID Locale, DWORD dwFlags, CONST SYSTEMTIME * lpTime, LPCTSTR lpFormat, 
            LPTSTR lpTimeStr, int cchTime),
       (Locale, dwFlags, lpTime, lpFormat, lpTimeStr, cchTime));

int WINAPI GetTimeFormat_BugW( IN LCID Locale, 
                               IN DWORD dwFlags, 
                               IN CONST SYSTEMTIME * lpTime, 
                               IN LPCTSTR lpFormat,
                               OUT LPTSTR lpTimeStr, 
                               IN int cchTime)
{
#ifndef WINCE
    int   iValue;
    CStrIn strInFormat(lpFormat);
    CStrOut strOutTimeStr(lpTimeStr, cchTime);

    iValue = GetTimeFormatA(Locale, dwFlags, lpTime, strInFormat, strOutTimeStr, cchTime);

    if (iValue)
    {
        iValue = strOutTimeStr.ConvertIncludingNul();
    }
    return iValue;
#else //WINCE
    return GetTimeFormat(Locale, dwFlags, lpTime, lpFormat, lpTimeStr, cchTime);
#endif //WINCE
}
#endif // ndef WIN16


//+---------------------------------------------------------------------------
//
//  method : ConvertDateTimeToString
//
//  Synopsis:
//          Converts given date and time if requested to string using the local time
//          in a fixed format mm/dd/yyyy or mm/dd/yyy hh:mm:ss
//          
//----------------------------------------------------------------------------

// NB (cthrash) This function produces a standard(?) format date, of the form
// MM/DD/YY HH:MM:SS (in military time.) The date format *will not* be tailored
// for the locale.  This is for Netscape compatibility, and is a departure from
// how IE3 worked.  If you want the date in the format of the document's locale,
// you should use the Java Date object.

HRESULT 
ConvertDateTimeToString(FILETIME Time, BSTR * pBstr, BOOL fReturnTime)
{
    HRESULT    hr;
    SYSTEMTIME SystemTime;
    TCHAR pchDateStr[DATE_STR_LENGTH];
#ifndef WIN16
    FILETIME     ft;
   
    Assert(pBstr);

    // We want to return local time as Nav not GMT 
    if (!FileTimeToLocalFileTime(&Time, &ft))
    {
        hr  = GetLastWin32Error();
        goto Cleanup;
    }

    if (!FileTimeToSystemTime( &ft, &SystemTime ))
    {
        hr  = GetLastWin32Error();
        goto Cleanup;
    }

    // We want Gregorian dates and 24-hour time.
    if(fReturnTime)
    {
        hr = THR(Format( 0, pchDateStr, ARRAY_SIZE(pchDateStr),
                     _T("<0d2>/<1d2>/<2d4> <3d2>:<4d2>:<5d2>"),
                     SystemTime.wMonth,
                     SystemTime.wDay,
                     SystemTime.wYear,
                     SystemTime.wHour,
                     SystemTime.wMinute,
                     SystemTime.wSecond ));
    }
    else
    {
        hr = THR(Format( 0, pchDateStr, ARRAY_SIZE(pchDateStr),
                     _T("<0d2>/<1d2>/<2d4>"),
                     SystemTime.wMonth,
                     SystemTime.wDay,
                     SystemTime.wYear));
    }
#else
    struct tm *LocalTm;
    int cchResult;

    FileTimeToSystemTime(&Time, &SystemTime);

    LocalTm = localtime(&SystemTime);
    
    if(fReturnTime)
    {
        cchResult = wsprintf(pchDateStr, "%2d/%2d/%4d %2d:%2d:%2d",
            LocalTm->tm_mon +1,
            LocalTm->tm_mday,
            LocalTm->tm_year,
            LocalTm->tm_hour,
            LocalTm->tm_min,
            LocalTm->tm_sec );
    }
    else
    {
        cchResult = wsprintf(pchDateStr, "%2d/%2d/%4d",
            LocalTm->tm_mon +1,
            LocalTm->tm_mday,
            LocalTm->tm_year);
    }
    // hopefully we didn't overwrite our buffer.
    Assert(cchResult <= ARRAY_SIZE(pchDateStr);

    hr = ERROR_SUCCESS;

#endif // ndef WIN16 else

    if (hr)
        goto Cleanup;

    hr = THR(FormsAllocString(pchDateStr, pBstr));
    
Cleanup:
    RRETURN(hr);
}


#ifndef WIN16

WRAPIT(InternetTimeToSystemTime,
       (LPCTSTR lpszTime, SYSTEMTIME *pst, DWORD dwReserved),
       (lpszTime, pst, dwReserved));

WRAPIT(UnlockUrlCacheEntryFileA,
    (LPCSTR lpszUrl, DWORD dwReserved),
    (lpszUrl, dwReserved));

BOOL WINAPI UnlockUrlCacheEntryFileBugW(
    LPCWSTR lpszUrl,
    DWORD dwReserved)
{
    CStrIn strInUrl(lpszUrl);

    return UnlockUrlCacheEntryFileA(strInUrl, dwReserved);
}

// These calls are used by pluginst.cxx to track down the mime
// type.  These are all specifically ANSI calls because wonderful
// wininet.dll does not support the wide-char versions, even though
// it happily exports them.

WRAPIT_( HINTERNET, InternetOpenA,
    (LPCSTR lpszAgent,DWORD dwAccessType,LPCSTR lpszProxy,LPCSTR lpszProxyBypass,DWORD dwFlags),
    (lpszAgent,dwAccessType,lpszProxy,lpszProxyBypass,dwFlags));

INTERNETAPI
HINTERNET
WINAPI
InternetOpenW(
    IN LPCWSTR lpszAgent,
    IN DWORD dwAccessType,
    IN LPCWSTR lpszProxy OPTIONAL,
    IN LPCWSTR lpszProxyBypass OPTIONAL,
    IN DWORD dwFlags
    )
{
    CStrIn szAgent( lpszAgent );
    CStrIn szProxy( lpszProxy );
    CStrIn szProxyBypass( lpszProxyBypass );

    return( InternetOpenA( szAgent, dwAccessType, szProxy, szProxyBypass, dwFlags ) );
}


WRAPIT_( HINTERNET, InternetOpenUrlA,
    (HINTERNET hInternet,LPCSTR lpszUrl,LPCSTR lpszHeaders,DWORD dwHeadersLength,DWORD dwFlags,DWORD_PTR dwContext),
    (hInternet,lpszUrl,lpszHeaders,dwHeadersLength,dwFlags,dwContext));

INTERNETAPI
HINTERNET
WINAPI
InternetOpenUrlW(
    IN HINTERNET hInternet,
    IN LPCWSTR lpszUrl,
    IN LPCWSTR lpszHeaders OPTIONAL,
    IN DWORD dwHeadersLength,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )
{
    CStrIn szUrl( lpszUrl );
    CStrIn szHeaders( lpszHeaders );

    return( InternetOpenUrlA(hInternet,szUrl,szHeaders,dwHeadersLength,dwFlags,dwContext) );
}


WRAPIT_( BOOL, InternetSetOptionA, 
    (HINTERNET hInternet,DWORD dwOption,LPVOID lpBuffer,DWORD dwBufferLength),
    (hInternet,dwOption,lpBuffer,dwBufferLength)
);

//
// W A R N I N G:  
//
//   This wrapper does not handle all types of calls to this
//   routine.  It does no Wide-char to ascii conversion, and
//   so only handles the calls such as INTERNET_SET_CONNECT_TIMETOUT
//   which do NOT take strings in the lpBuffer parameter.
//
// W A R N I N G
//

BOOLAPI
InternetSetOptionW(
    IN HINTERNET hInternet OPTIONAL,
    IN DWORD dwOption,
    IN LPVOID lpBuffer,
    IN DWORD dwBufferLength
    )
{
    return( InternetSetOptionA( hInternet, dwOption, lpBuffer, dwBufferLength ) );
}

WRAPIT_( BOOL, HttpQueryInfoA, 
    (HINTERNET hRequest,DWORD dwInfoLevel,LPVOID lpBuffer,LPDWORD lpdwBufferLength,LPDWORD lpdwIndex),
    (hRequest,dwInfoLevel,lpBuffer,lpdwBufferLength,lpdwIndex)
);


BOOLAPI
HttpQueryInfoW(
    IN HINTERNET hRequest,
    IN DWORD dwInfoLevel,
    IN OUT LPVOID lpBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength,
    IN OUT LPDWORD lpdwIndex OPTIONAL
    )
{
    CStrOut     strOutMime((WCHAR*)lpBuffer, MAX_PATH);
    BOOL        fRet;

    fRet = HttpQueryInfoA(hRequest,dwInfoLevel,strOutMime,lpdwBufferLength,lpdwIndex);

    strOutMime.ConvertIncludingNul();

    return( fRet );
}

WRAPIT( InternetCloseHandle,
    (HINTERNET hInternet),
    (hInternet));

WRAPIT(InternetUnlockRequestFile,
    (HANDLE hLock),
    (hLock));

WRAPIT(InternetGetConnectedState,
    (LPDWORD lpdwFlags, DWORD dwReserved),
    (lpdwFlags, dwReserved));

WRAPIT(IsUrlCacheEntryExpiredW,
    (LPCWSTR lpszUrlName, DWORD dwFlags, FILETIME * pftLastModifiedTime),
    (lpszUrlName, dwFlags, pftLastModifiedTime));

#endif // ndef WIN16

//+--------------------------------------------------------------------------
//
//  Function : ShortCutUrlHelper
//
//  Synopsis : the helper's helper. in this cas (HASH and SEARCH setting we
//          do not need the over head of the full SetUrlCode. i.e. we are 
//          either appending or replacing existing parts
//
//---------------------------------------------------------------------------

HRESULT
ShortCutSetUrlHelper(const TCHAR * pchUrlIn,
                     TCHAR       * pchUrlOut,
                     DWORD         dwBufLen,
                     const BSTR  * pstrComp,
                     URLCOMP_ID    ucid)
{
    HRESULT        hr = S_OK;
    DWORD          cch = 0;
    const TCHAR    chTarget = (ucid==URLCOMP_HASH) ? _T('#') : _T('?');
    long           lLength = SysStringLen(*pstrComp)+1;  // +1 for \0

    Assert((ucid==URLCOMP_HASH) || (ucid==URLCOMP_SEARCH));
 
    // copy over the base url
    if (pchUrlIn)
    {
        while (*pchUrlIn && (*pchUrlIn != chTarget) && (cch++ < (dwBufLen-1)))
        {
            *pchUrlOut++ = * pchUrlIn++;
        }
    }

    // deal with the hash/search character
    // NS always appends a '#' but never a '?'
    if (ucid==URLCOMP_HASH)
    {
        (*pchUrlOut++) = chTarget;
        cch++;
    }

    // add the search/hash property. Truncate if too long
    if (cch+lLength > dwBufLen)
        lLength = dwBufLen - cch;

    _tcsncpy(pchUrlOut, *pstrComp, lLength);
    *(pchUrlOut+lLength-1) = _T('\0');
    hr = S_OK;

    RRETURN(hr);
}


