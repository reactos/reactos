//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       uwininet.h
//
//  Contents:   Redefinitions of unicode versions of some Wininet APIs
//              which are declared incorrectly in wininet.h and unimplemented
//              in wininet.dll.
//
//              Note that this workaround doesn't solve the whole problem
//              since INTERNET_CACHE_ENTRY_INFO contains a non-unicode
//              lpszSourceUrlName which too much trouble to convert in our
//              wrapper (at any rate, we don't use it).
//
//----------------------------------------------------------------------------

#ifndef I_UWININET_H_
#define I_UWININET_H_
#pragma INCMSG("--- Beg 'uwininet.h'")

#ifndef X_WININET_H_
#define X_WININET_H_
#pragma INCMSG("--- Beg <wininet.h>")
#define _WINX32_
#include <wininet.h>
#pragma INCMSG("--- End <wininet.h>")
#endif

// to get all the defs, winineti.h requires wincrypt.h be included before it!
#ifndef WIN16
#ifndef X_WINCRYPT_H
#define X_WINCRYPT_H
#pragma INCMSG("--- Beg <wincrypt.h>")
#include "wincrypt.h"
#pragma INCMSG("--- End <wincrypt.h>")
#endif
#endif

#ifndef WIN16
#ifndef X_WININETI_H_
#define X_WININETI_H_
#pragma INCMSG("--- Beg <winineti.h>")
#define _WINX32_
#include <winineti.h>
#pragma INCMSG("--- End <winineti.h>")
#endif
#endif

#include <w4warn.h>

#define DATE_STR_LENGTH 30

#define LPCBYTE         const BYTE *

#ifndef WIN16

// urlmon unicode wrapper for ansi api
STDAPI ObtainUserAgentStringW(DWORD dwOption, LPWSTR lpszUAOut, DWORD* cbSize);

//
//  Wrap CreateUrlCacheEntry
//--------------------------------------------------
URLCACHEAPI BOOL WINAPI
CreateUrlCacheEntryBugW ( IN LPCWSTR lpszUrlName, 
                       IN DWORD dwFileSize, 
                       IN LPCWSTR lpszExtension, 
                       OUT LPWSTR lpszFileName, 
                       IN DWORD dwRes);

#undef CreateUrlCacheEntry  
#ifdef UNICODE
#define CreateUrlCacheEntry CreateUrlCacheEntryBugW
#else
#define CreateUrlCacheEntry CreateUrlCacheEntryA
#endif

//
// Wrap CommitUrlCachEntry
//--------------------------------------------------

URLCACHEAPI BOOL WINAPI
CommitUrlCacheEntryBugW ( 
                       IN LPCWSTR  lpszUrlName,
                       IN LPCWSTR  lpszLocalFileName,
                       IN FILETIME ExpireTime,
                       IN FILETIME LastModifiedTime,
                       IN DWORD    dwCachEntryType,
                       IN LPCBYTE  lpHeaderInfo,
                       IN DWORD    dwHeaderSize,
                       IN LPCWSTR  lpszFileExtension,
                       IN DWORD    dwReserved);

#undef CommitUrlCacheEntry
#ifdef UNICODE  
#define CommitUrlCacheEntry CommitUrlCacheEntryBugW
#else
#define CommitUrlCacheEntry CommitUrlCacheEntryA
#endif




URLCACHEAPI
BOOL
WINAPI
GetUrlCacheEntryInfoBugW(
    IN LPCWSTR lpszUrlName,
    OUT LPINTERNET_CACHE_ENTRY_INFO lpCacheEntryInfo,
    IN OUT LPDWORD lpdwCacheEntryInfoBufferSize
    );

#undef GetUrlCacheEntryInfo // remove the buggy version

#ifdef UNICODE
#define GetUrlCacheEntryInfo GetUrlCacheEntryInfoBugW
#else
#define GetUrlCacheEntryInfo GetUrlCacheEntryInfoA
#endif // !UNICODE


// Same Problem here

BOOLAPI
GetUrlCacheEntryInfoExBugW(
	IN LPCWSTR lpszUrl,
	OUT LPINTERNET_CACHE_ENTRY_INFOW lpCacheEntryInfo,
	IN OUT LPDWORD lpdwCacheEntryInfoBufSize,
	OUT LPSTR lpszRedirectUrl,
	IN OUT LPDWORD lpdwRedirectUrlBufSize,
	LPVOID lpReserved,
	DWORD dwReserved
);

#undef GetUrlCacheEntryInfoEx // remove the buggy version

#ifdef UNICODE
#define GetUrlCacheEntryInfoEx GetUrlCacheEntryInfoExBugW
#else
#define GetUrlCacheEntryInfoEx GetUrlCacheEntryInfoExA
#endif // !UNICODE

// Same problem here
URLCACHEAPI
BOOL
WINAPI
SetUrlCacheEntryInfoBugW(
    IN LPCWSTR lpszUrlName,
    IN LPINTERNET_CACHE_ENTRY_INFO lpCacheEntryInfo,
    IN DWORD dwFieldControl
    );

#undef SetUrlCacheEntryInfo 

#ifdef UNICODE
#define SetUrlCacheEntryInfo  SetUrlCacheEntryInfoBugW
#else
#define SetUrlCacheEntryInfo  SetUrlCacheEntryInfoA
#endif // !UNICODE

// again, but worse...
BOOL
WINAPI
DeleteUrlCacheEntryBugW(LPCWSTR lpszUrlName);

BOOL
WINAPI
DeleteUrlCacheEntryA(LPCSTR lpszUrlName);

#undef DeleteUrlCacheEntry

#ifdef UNICODE
#define DeleteUrlCacheEntry  DeleteUrlCacheEntryBugW
#else
#define DeleteUrlCacheEntry  DeleteUrlCacheEntryA
#endif // !UNICODE


#endif // ndef WIN16

// Get/Set helpers for URL components.
enum URLCOMP_ID
{
    URLCOMP_HOST,
    URLCOMP_HOSTNAME,
    URLCOMP_PATHNAME,
    URLCOMP_PORT,
    URLCOMP_PROTOCOL,
    URLCOMP_SEARCH,
    URLCOMP_HASH,
    URLCOMP_WHOLE
};

UINT GetUrlScheme(const TCHAR * pchUrlIn);

BOOL IsURLSchemeCacheable(UINT uScheme);

BOOL IsUrlSecure(const TCHAR * pchUrl);

HRESULT GetUrlComponentHelper(const TCHAR * pchUrlIn,
                      CStr *        pstrComp,
                      DWORD         dwFlags,
                      URLCOMP_ID    ucid);
HRESULT SetUrlComponentHelper(const TCHAR * pchUrlIn,
                      TCHAR *       pchUrlOut,
                      DWORD         dwBufLen,
                      const BSTR *  pstrComp,
                      URLCOMP_ID    ucid);
HRESULT ComposeUrl(URL_COMPONENTS *puc,
                     DWORD           dwFlags,
                     TCHAR         * pchUrlOut,
                     DWORD         * pdwSize);

//Other helper routines and wininetapi wrappers
HRESULT ConvertDateTimeToString(FILETIME Time, 
                               BSTR * pchDateStr, 
                               BOOL   fReturnTime);


#ifndef WIN16

//GetDateFormat is UNICODE on NT ONLY
int 
WINAPI
GetDateFormat_BugW(LCID Locale, 
              DWORD dwFlags, 
              CONST SYSTEMTIME * lpDate, 
              LPCTSTR lpFormat,
              LPTSTR lpDateStr, 
              int cchDate);
#undef GetDateFormat
#ifdef UNICODE
#define GetDateFormat GetDateFormat_BugW
#else
#define GetDateFormat GetDateFormatA
#endif // !UNICODE


//GetTimeFormat is UNICODE on NT ONLY
int 
WINAPI
GetTimeFormat_BugW(LCID Locale, 
              DWORD dwFlags, 
              CONST SYSTEMTIME * lpTime, 
              LPCTSTR lpFormat,
              LPTSTR lpTimeStr, 
              int cchDate);
#undef GetTimeFormat
#ifdef UNICODE
#define GetTimeFormat GetTimeFormat_BugW
#else
#define GetTimeFormat GetTimeFormatA
#endif // !UNICODE

URLCACHEAPI
BOOL
WINAPI
RetrieveUrlCacheEntryFileBugW(
    IN LPCWSTR lpszUrlName,
    OUT LPINTERNET_CACHE_ENTRY_INFO lpCacheEntryInfo,
    IN OUT LPDWORD lpdwCacheEntryInfoBufferSize,
    IN DWORD dwReserved
    );

#undef RetrieveUrlCacheEntryFile // remove the buggy version

#ifdef UNICODE
#define RetrieveUrlCacheEntryFile RetrieveUrlCacheEntryFileBugW
#else
#define RetrieveUrlCacheEntryFile RetrieveUrlCacheEntryFileA
#endif // !UNICODE

// These are exported but not defined in any header file. (cthrash)
INTERNETAPI
BOOLAPI
InternetGetCertByURLW(LPCWSTR lpszURL,
                      LPWSTR lpszCertText,
                      DWORD dwcbCertText);
INTERNETAPI
BOOLAPI
InternetGetCertByURLA(LPCSTR lpszURL,
                      LPSTR lpszCertText,
                      DWORD dwcbCertText);

#ifdef UNICODE
#define InternetGetCertByURL InternetGetCertByURLW
#else
#define InternetGetCertByURL InternetGetCertByURLA
#endif //UNICODE

#else
// WIN16: We only have one version of this.
extern "C"
BOOL
WINAPI
_loadds
InternetGetCertByURL(LPCSTR lpszURL,
                     LPSTR lpszCertText,
                     DWORD dwcbCertText);
#endif

URLCACHEAPI
BOOL
WINAPI
UnlockUrlCacheEntryFileBugW(
    IN LPCWSTR lpszUrlName,
    IN DWORD dwReserved
    );

#undef UnlockUrlCacheEntryFile // remove the buggy version

#ifdef UNICODE
#define DoUnlockUrlCacheEntryFile UnlockUrlCacheEntryFileBugW
#else
#define DoUnlockUrlCacheEntryFile UnlockUrlCacheEntryFileA
#endif

HRESULT ShortCutSetUrlHelper(const TCHAR * pchUrlIn,
                             TCHAR       * pchUrlOut,
                             DWORD         dwBufLen,
                             const BSTR  * pstrComp,
                             URLCOMP_ID    ucid);

#pragma INCMSG("--- End 'uwininet.h'")
#else
#pragma INCMSG("*** Dup 'uwininet.h'")
#endif
