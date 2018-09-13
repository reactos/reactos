//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994-1996               **
//*********************************************************************
//
// DLYLDDLL.C - uses macros for delay loading of DLLs
//

#include "inetcplp.h"
#include <cryptui.h>

// coded copied from SHDOCVW's dllload.c file

#pragma warning(disable:4229)  // No warnings when modifiers used on data


// Exporting by ordinal is not available on UNIX.
// But we have all these symbols exported because it's UNIX default.
#ifdef UNIX
#define GET_PRIVATE_PROC_ADDRESS(_hinst, _fname, _ord) GetProcAddress(_hinst, #_fname)
#else
#define GET_PRIVATE_PROC_ADDRESS(_hinst, _fname, _ord) GetProcAddress(_hinst, (LPSTR) _ord)
#endif


#define ENSURE_LOADED(_hinst, _dll)   ( _hinst ? TRUE : NULL != (_hinst=LoadLibraryA(#_dll)) )
#define DELAY_LOAD_ERR(_hinst, _dll, _ret, _fn, _args, _nargs, _err) \
_ret __stdcall _fn _args                \
{                                       \
    static _ret (* __stdcall _pfn##_fn) _args = NULL;   \
    if (!ENSURE_LOADED(_hinst, _dll))   \
    {                                   \
        ASSERT(_hinst); \
        return (_ret)_err;                      \
    }                                   \
    if (_pfn##_fn == NULL)              \
    {                                   \
        *(FARPROC*)&(_pfn##_fn) = GetProcAddress(_hinst, #_fn); \
        ASSERT(_pfn##_fn); \
        if (_pfn##_fn == NULL)      \
            return (_ret)_err;          \
    }                                   \
    return _pfn##_fn _nargs;            \
 }

#define DELAY_LOAD(_hinst, _dll, _ret, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, _ret, _fn, _args, _nargs, 0)
#define DELAY_LOAD_HRESULT(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, HRESULT, _fn, _args, _nargs, E_FAIL)
#define DELAY_LOAD_SAFEARRAY(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, SAFEARRAY *, _fn, _args, _nargs, NULL)
#define DELAY_LOAD_UINT(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, UINT, _fn, _args, _nargs, 0)

#define DELAY_LOAD_VOID(_hinst, _dll, _fn, _args, _nargs) \
void __stdcall _fn _args                \
{                                       \
    static void (* __stdcall _pfn##_fn) _args = NULL;   \
    if (!ENSURE_LOADED(_hinst, _dll))   \
    {                                   \
        ASSERT((BOOL)_hinst); \
        return;                         \
    }                                   \
    if (_pfn##_fn == NULL)              \
    {                                   \
        *(FARPROC*)&(_pfn##_fn) = GetProcAddress(_hinst, #_fn); \
        ASSERT((BOOL)_pfn##_fn); \
        if (_pfn##_fn == NULL)      \
            return;                     \
    }                                   \
    _pfn##_fn _nargs;                   \
 }



// For private entrypoints exported by ordinal.
#define DELAY_LOAD_ORD_ERR(_hinst, _dll, _ret, _fn, _ord, _args, _nargs, _err) \
_ret __stdcall _fn _args                \
{                                       \
    static _ret (* __stdcall _pfn##_fn) _args = NULL;   \
    if (!ENSURE_LOADED(_hinst, _dll))   \
    {                                   \
        ASSERT(_hinst); \
        return (_ret)_err;                      \
    }                                   \
    if (_pfn##_fn == NULL)              \
    {                                   \
        *(FARPROC*)&(_pfn##_fn) = GET_PRIVATE_PROC_ADDRESS(_hinst, _fn, _ord); \
        ASSERT(_pfn##_fn); \
        if (_pfn##_fn == NULL)      \
            return (_ret)_err;          \
    }                                   \
    return _pfn##_fn _nargs;            \
 }

#define DELAY_LOAD_ORD_HRESULT(_hinst, _dll, _fn, _ord, _args, _nargs) DELAY_LOAD_ORD_ERR(_hinst, _dll, HRESULT, _fn, _ord, _args, _nargs, E_FAIL)

#define DELAY_LOAD_ORD(_hinst, _dll, _ret, _fn, _ord, _args, _nargs) DELAY_LOAD_ORD_ERR(_hinst, _dll, _ret, _fn, _ord, _args, _nargs, 0)


#define DELAY_LOAD_VOID_ORD(_hinst, _dll, _fn, _ord, _args, _nargs) \
void __stdcall _fn _args                \
{                                       \
    static void (* __stdcall _pfn##_fn) _args = NULL;   \
    if (!ENSURE_LOADED(_hinst, _dll))   \
    {                                   \
        ASSERT((BOOL)_hinst); \
        return;                         \
    }                                   \
    if (_pfn##_fn == NULL)              \
    {                                   \
        *(FARPROC*)&(_pfn##_fn) =  GET_PRIVATE_PROC_ADDRESS(_hinst, _fn, _ord); \
        ASSERT((BOOL)_pfn##_fn); \
        if (_pfn##_fn == NULL)      \
            return;                     \
    }                                   \
    _pfn##_fn _nargs;                   \
 }

//--------- wininet.dll -----------------

HINSTANCE g_hinstWinInet = NULL;

DELAY_LOAD(g_hinstWinInet, WININET.DLL, BOOL, InternetSetOption, 
           (IN HINTERNET hInternet OPTIONAL,IN DWORD dwOption,IN LPVOID lpBuffer,
            IN DWORD dwBufferLength),
           (hInternet,dwOption,lpBuffer,dwBufferLength));

DELAY_LOAD(g_hinstWinInet, WININET.DLL, BOOL, InternetQueryOption, 
           (IN HINTERNET hInternet OPTIONAL,IN DWORD dwOption,IN LPVOID lpBuffer,
            IN OUT LPDWORD lpdwBufferLength),
           (hInternet,dwOption,lpBuffer,lpdwBufferLength));

DELAY_LOAD(g_hinstWinInet, WININET.DLL, DWORD, ShowX509EncodedCertificate, 
           (IN HWND    hWndParent,IN LPBYTE  lpCert,IN DWORD   cbCert),
           (hWndParent,lpCert,cbCert));

DELAY_LOAD(g_hinstWinInet, WININET.DLL, DWORD, ParseX509EncodedCertificateForListBoxEntry,
           (IN LPBYTE  lpCert,IN DWORD  cbCert,OUT LPSTR lpszListBoxEntry,IN LPDWORD lpdwListBoxEntry),
           (lpCert,cbCert,lpszListBoxEntry,lpdwListBoxEntry));

DELAY_LOAD(g_hinstWinInet, WININET.DLL, BOOL, GetUrlCacheConfigInfoA,
           (
            OUT LPINTERNET_CACHE_CONFIG_INFOA lpCacheConfigInfo,
            IN OUT LPDWORD lpdwCacheConfigInfoBufferSize,
            IN DWORD dwFieldControl
           ),
           (lpCacheConfigInfo,lpdwCacheConfigInfoBufferSize,dwFieldControl));

DELAY_LOAD(g_hinstWinInet, WININET.DLL, BOOL, SetUrlCacheConfigInfoA,
           (
            IN LPINTERNET_CACHE_CONFIG_INFOA lpCacheConfigInfo,
            IN DWORD dwFieldControl
           ),
           (lpCacheConfigInfo,dwFieldControl));

DELAY_LOAD(g_hinstWinInet, WININET.DLL, BOOL, FreeUrlCacheSpaceA,
           (
            IN LPCSTR lpszCachePath,
            IN DWORD dwSize,
            IN DWORD dwReserved
           ),
           (lpszCachePath,dwSize,dwReserved));

DELAY_LOAD(g_hinstWinInet, WININET.DLL, BOOL, UpdateUrlCacheContentPath,
           (
            IN LPSTR lpszCachePath
           ),
           (lpszCachePath));

DELAY_LOAD(g_hinstWinInet, WININET.DLL, BOOL, FindCloseUrlCache,
           (
            IN HANDLE hEnumHandle
           ),
           (hEnumHandle));

DELAY_LOAD(g_hinstWinInet, WININET.DLL, HANDLE, FindFirstUrlCacheEntryExA,
           (
            IN     LPCSTR    lpszUrlSearchPattern,
            IN     DWORD     dwFlags,
            IN     DWORD     dwFilter,
            IN     GROUPID   GroupId,
            OUT    LPINTERNET_CACHE_ENTRY_INFOA lpFirstCacheEntryInfo,
            IN OUT LPDWORD   lpdwFirstCacheEntryInfoBufferSize,
            OUT    LPVOID    lpReserved,     // must pass NULL
            IN OUT LPDWORD   pcbReserved2,   // must pass NULL
            IN     LPVOID    lpReserved3     // must pass NULL
           ),
           (lpszUrlSearchPattern, dwFlags, dwFilter, GroupId, lpFirstCacheEntryInfo, 
            lpdwFirstCacheEntryInfoBufferSize, lpReserved, pcbReserved2, lpReserved3));

DELAY_LOAD(g_hinstWinInet, WININET.DLL, BOOL, FindNextUrlCacheEntryExA,
           (
            IN     HANDLE    hEnumHandle,
            OUT    LPINTERNET_CACHE_ENTRY_INFOA lpFirstCacheEntryInfo,
            IN OUT LPDWORD   lpdwFirstCacheEntryInfoBufferSize,
            OUT    LPVOID    lpReserved,     // must pass NULL
            IN OUT LPDWORD   pcbReserved2,   // must pass NULL
            IN     LPVOID    lpReserved3     // must pass NULL
           ),
           (hEnumHandle, lpFirstCacheEntryInfo, lpdwFirstCacheEntryInfoBufferSize, lpReserved, pcbReserved2, lpReserved3));

DELAY_LOAD(g_hinstWinInet, WININET.DLL, BOOL, InternetGetConnectedStateExA,
            (
             OUT LPDWORD lpdwFlags,
             IN LPSTR lpszConnectionName,
             IN DWORD dwNameSize,
             IN DWORD dwReserved
            ),
            (lpdwFlags, lpszConnectionName, dwNameSize, dwReserved));

DELAY_LOAD_ORD(g_hinstWinInet, WININET.DLL, BOOL, GetDiskInfoA, 102,
            (
             IN PSTR pszPath, 
             IN OUT PDWORD pdwClusterSize, 
             IN OUT PDWORDLONG pdlAvail, 
             IN OUT PDWORDLONG pdlTotal
            ),
            (pszPath, pdwClusterSize, pdlAvail, pdlTotal));

//--------- urlmon.dll ------------------

HINSTANCE g_hinstUrlMon = NULL;

DELAY_LOAD(g_hinstUrlMon, URLMON.DLL, HRESULT, UrlMkSetSessionOption,
           (DWORD dwOption, LPVOID pBuffer, DWORD dwBufferLength, DWORD dwReserved),
           (dwOption, pBuffer, dwBufferLength, dwReserved));


DELAY_LOAD(g_hinstUrlMon, URLMON.DLL, HRESULT, CoInternetCreateZoneManager,
           (IServiceProvider *pSP, IInternetZoneManager **ppZM, DWORD dwReserved),
           (pSP, ppZM, dwReserved));

DELAY_LOAD(g_hinstUrlMon, URLMON.DLL, HRESULT, CoInternetCreateSecurityManager,
           (IServiceProvider *pSP, IInternetSecurityManager **ppSM, DWORD dwReserved),
           (pSP, ppSM, dwReserved));

DELAY_LOAD(g_hinstUrlMon, URLMON.DLL, HRESULT, CreateURLMoniker,
           (LPMONIKER pMkCtx, LPCWSTR szURL, LPMONIKER FAR * ppmk),
           (pMkCtx,szURL,ppmk));

DELAY_LOAD(g_hinstUrlMon, URLMON.DLL, HRESULT, FaultInIEFeature,
            (HWND hWnd, uCLSSPEC *pClassSpec, QUERYCONTEXT *pQuery, DWORD dwFlags),
            (hWnd, pClassSpec, pQuery, dwFlags));

// -------- crypt32.dll ----------------------------


HINSTANCE g_hinstCrypt32 = NULL;

DELAY_LOAD(g_hinstCrypt32, CRYPT32.DLL, HCERTSTORE, CertOpenSystemStoreA,
           (HCRYPTPROV hProv, LPCSTR szSubSystemProtocol),
           (hProv, szSubSystemProtocol));

DELAY_LOAD(g_hinstCrypt32, CRYPT32.DLL, BOOL , CertCloseStore,
           (IN HCERTSTORE hCertStore, DWORD dwFlags),
           (hCertStore, dwFlags));

DELAY_LOAD(g_hinstCrypt32, CRYPT32.DLL, BOOL, CertGetEnhancedKeyUsage,
           (IN PCCERT_CONTEXT pCertContext, IN DWORD dwFlags, OUT PCERT_ENHKEY_USAGE pUsage, IN OUT DWORD *pcbUsage),
           (pCertContext, dwFlags, pUsage, pcbUsage));

DELAY_LOAD(g_hinstCrypt32, CRYPT32.DLL, BOOL, CertSetEnhancedKeyUsage,
           (IN PCCERT_CONTEXT pCertContext, IN PCERT_ENHKEY_USAGE pUsage),
           (pCertContext, pUsage));

DELAY_LOAD(g_hinstCrypt32, CRYPT32.DLL, BOOL, CertDeleteCertificateFromStore,
           (IN PCCERT_CONTEXT pCertContext),
           (pCertContext));

DELAY_LOAD(g_hinstCrypt32, CRYPT32.DLL, BOOL, CertAddEnhancedKeyUsageIdentifier,
           (IN PCCERT_CONTEXT pCertContext, IN LPCSTR pszUsageIdentifier),
           (pCertContext, pszUsageIdentifier));

DELAY_LOAD(g_hinstCrypt32, CRYPT32.DLL, BOOL, CertRemoveEnhancedKeyUsageIdentifier,
           (IN PCCERT_CONTEXT pCertContext,IN LPCSTR pszUsageIdentifier),
           (pCertContext, pszUsageIdentifier));

DELAY_LOAD(g_hinstCrypt32, CRYPT32.DLL,  PCCERT_CONTEXT, CertFindCertificateInStore,
           (IN HCERTSTORE hCertStore, IN DWORD dwCertEncodingType, IN DWORD dwFindFlags,IN DWORD dwFindType,
            IN const void *pvFindPara, IN PCCERT_CONTEXT pPrevCertContext),
           (hCertStore, dwCertEncodingType, dwFindFlags, dwFindType, pvFindPara, pPrevCertContext));

DELAY_LOAD(g_hinstCrypt32, CRYPT32.DLL, BOOL, CertAddEncodedCertificateToStore,
           (IN HCERTSTORE hCertStore,IN DWORD dwCertEncodingType,IN const BYTE *pbCertEncoded,
            IN DWORD cbCertEncoded,IN DWORD dwAddDisposition, OUT OPTIONAL PCCERT_CONTEXT *ppCertContext),
           (hCertStore, dwCertEncodingType, pbCertEncoded, cbCertEncoded, dwAddDisposition, ppCertContext));

DELAY_LOAD(g_hinstCrypt32, CRYPT32.DLL,  BOOL, CertFreeCertificateContext,
           (IN PCCERT_CONTEXT pCertContext),
           (pCertContext));

DELAY_LOAD(g_hinstCrypt32, CRYPT32.DLL, BOOL, CertCompareCertificateName,
           (IN DWORD dwCertEncodingType,IN PCERT_NAME_BLOB pCertName1,IN PCERT_NAME_BLOB pCertName2),
           (dwCertEncodingType,pCertName1,pCertName2));

DELAY_LOAD(g_hinstCrypt32, CRYPT32.DLL, PCCERT_CONTEXT, CertCreateCertificateContext,
           (IN DWORD dwCertEncodingType, IN const BYTE *pbCertEncoded, IN DWORD cbCertEncoded),
           (dwCertEncodingType,pbCertEncoded,cbCertEncoded));

DELAY_LOAD(g_hinstCrypt32, CRYPT32.DLL, BOOL, CertAddCertificateContextToStore,
           (IN HCERTSTORE hCertStore,
            IN PCCERT_CONTEXT pCertContext,
            IN DWORD dwAddDisposition,
            OUT OPTIONAL PCCERT_CONTEXT *ppStoreContext),
           (hCertStore,pCertContext,dwAddDisposition,ppStoreContext));

DELAY_LOAD(g_hinstCrypt32, CRYPT32.DLL, PCCERT_CONTEXT, CertEnumCertificatesInStore,
           (IN HCERTSTORE hCertStore,
            IN PCCERT_CONTEXT pPrevCertContext),
           (hCertStore,pPrevCertContext));

DELAY_LOAD(g_hinstCrypt32, CRYPT32.DLL, BOOL, PFXExportCertStore,
           (HCERTSTORE hStore,
            CRYPT_DATA_BLOB* pPFX,
            LPCWSTR szPassword,
            DWORD   dwFlags),
           (hStore,pPFX, szPassword, dwFlags));


DELAY_LOAD(g_hinstCrypt32, CRYPT32.DLL, HCERTSTORE, PFXImportCertStore,
           (CRYPT_DATA_BLOB* pPFX,
            LPCWSTR szPassword,
            DWORD   dwFlags),
           (pPFX,szPassword,dwFlags));


DELAY_LOAD(g_hinstCrypt32, CRYPT32.DLL, HCERTSTORE, CertOpenStore,
           (IN LPCSTR lpszStoreProvider,
            IN DWORD dwEncodingType,
            IN HCRYPTPROV hCryptProv,
            IN DWORD dwFlags,
            IN const void *pvPara),
           (lpszStoreProvider,
            dwEncodingType,
            hCryptProv,
            dwFlags,
            pvPara));

DELAY_LOAD(g_hinstCrypt32, CRYPT32.DLL, BOOL, CryptHashCertificate,
           (IN HCRYPTPROV hCryptProv,
            IN ALG_ID Algid,
            IN DWORD dwFlags,
            IN const BYTE *pbEncoded,
            IN DWORD cbEncoded,
            OUT BYTE *pbComputedHash,
            IN OUT DWORD *pcbComputedHash
           ),
           (hCryptProv,Algid,dwFlags,pbEncoded,cbEncoded,pbComputedHash,pcbComputedHash));

DELAY_LOAD(g_hinstCrypt32, CRYPT32.DLL, PCCERT_CONTEXT, CertDuplicateCertificateContext,
           (IN PCCERT_CONTEXT pCertContext),
           (pCertContext));

DELAY_LOAD(g_hinstCrypt32, CRYPT32.DLL, BOOL, CertGetCertificateContextProperty,
            (IN PCCERT_CONTEXT pCertContext,
             IN DWORD dwPropId,
             OUT void *pvData,
             IN OUT DWORD *pcbData
            ),
            (pCertContext, dwPropId, pvData, pcbData));

DELAY_LOAD(g_hinstCrypt32, CRYPT32.DLL, BOOL, CryptDecodeObject,
            (IN DWORD       dwCertEncodingType,
             IN LPCSTR      lpszStructType,
             IN const BYTE  *pbEncoded,
             IN DWORD       cbEncoded,
             IN DWORD       dwFlags,
             OUT void       *pvStructInfo,
             IN OUT DWORD   *pcbStructInfo
             ),
             (dwCertEncodingType, lpszStructType, pbEncoded, cbEncoded, dwFlags, pvStructInfo, pcbStructInfo));

DELAY_LOAD(g_hinstCrypt32, CRYPT32.DLL, PCERT_EXTENSION, CertFindExtension,
           (IN LPCSTR           pszObjId,
            IN DWORD            cExtensions,
            IN CERT_EXTENSION   rgExtensions[]
            ),
            (pszObjId, cExtensions, rgExtensions));

// -------- cryptui.dll ----------------------------

HINSTANCE g_hinstCryptui = NULL;

DELAY_LOAD(g_hinstCryptui, CRYPTUI.DLL, BOOL, CryptUIDlgCertMgr,
           (PCCRYPTUI_CERT_MGR_STRUCT pCryptUICertMgr),
           (pCryptUICertMgr));

//--------- shdocvw.dll ------------------

HINSTANCE g_hinstShdocvw = NULL;
const TCHAR c_tszShdocvw[] = TEXT("SHDOCVW.DLL");

// HRESULT URLSubRegQueryA(LPCSTR pszKey, LPCSTR pszValue, BOOL fUseHKCU, LPSTR pszUrlOut, DWORD cchSize, DWORD dwSubstitutions);
DELAY_LOAD_ORD_HRESULT(g_hinstShdocvw, SHDOCVW.DLL, URLSubRegQueryA, 151,
                       (LPCSTR pszKey, LPCSTR pszValue, BOOL fUseHKCU, LPSTR pszUrlOut, DWORD cchSize, DWORD dwSubstitutions),
                       (pszKey, pszValue, fUseHKCU, pszUrlOut, cchSize, dwSubstitutions));

// HRESULT ResetProfileSharing(HWND hwin);
DELAY_LOAD_ORD_HRESULT(g_hinstShdocvw, SHDOCVW.DLL, ResetProfileSharing, 164,
                        (HWND hwnd),
                        (hwnd));

// HRESULT ClearAutoSuggestForForms(DWORD dwClear);
DELAY_LOAD_ORD_HRESULT(g_hinstShdocvw, SHDOCVW.DLL, ClearAutoSuggestForForms, 211,
                        (DWORD dwClear),
                        (dwClear));

// HRESULT ResetWebSettings(HWND hwnd)
DELAY_LOAD_ORD_HRESULT(g_hinstShdocvw, SHDOCVW.DLL, ResetWebSettings, 223,
                        (HWND hwnd, BOOL *pfChangedHomePage),
                        (hwnd,pfChangedHomePage));

#ifdef UNIX_FEATURE_ALIAS
DELAY_LOAD_ORD_HRESULT(g_hinstShdocvw, SHDOCVW.DLL, RefreshGlobalAliasList, 164,
                        (),
                        ());
#endif /* UNIX_FEATURE_ALIAS */

//--------- msrating.dll ----------------

HINSTANCE g_hinstRatings = NULL;
const TCHAR c_tszRatingsDLL[] = TEXT("MSRATING.DLL");

DELAY_LOAD(g_hinstRatings, MSRATING.DLL, HRESULT, RatingEnable,
           (HWND hwndParent, LPCSTR pszUsername, BOOL fEnable),
           (hwndParent,pszUsername,fEnable));


DELAY_LOAD(g_hinstRatings, MSRATING.DLL, HRESULT, RatingSetupUI,
           (HWND hDlg, LPCSTR pszUsername),
           (hDlg, pszUsername));

DELAY_LOAD(g_hinstRatings, MSRATING.DLL, HRESULT, RatingEnabledQuery,
           (), ());

// --------- mshtml.dll --------------------

HINSTANCE g_hinstMSHTML = NULL;
const TCHAR c_tszMSHTMLDLL[] = TEXT("MSHTML.DLL");

DELAY_LOAD(g_hinstMSHTML, MSHTML.DLL, HRESULT, ShowModalDialog,
           (HWND hwndParent, IMoniker *pmk, VARIANT *pvarArgIn, TCHAR* pchOptions, VARIANT *pvarArgOut),
           (hwndParent,pmk,pvarArgIn,pchOptions,pvarArgOut));

//
// BUGBUG: We don't need to delay load anything from MSHTML, 
//         but we are using this still to determine if 
//         MSHTML.DLL is around.
//


HINSTANCE g_hinstOCCache = NULL;

DELAY_LOAD_HRESULT(g_hinstOCCache, OCCache.DLL, RemoveExpiredControls,
                   (DWORD dwFlags, DWORD dwReserved),
                   ( dwFlags, dwReserved));

// --------- mpr.dll --------------------

HINSTANCE g_hinstMPR = NULL;
const TCHAR c_tszMPRDLL[] = TEXT("MPR.DLL");

#ifndef UNICODE
DELAY_LOAD(g_hinstMPR, MPR.DLL, DWORD, WNetGetConnectionA,
           (LPCSTR pszLocalDevice, LPSTR pszUNC, LPDWORD pcbUNC),
           (pszLocalDevice, pszUNC, pcbUNC));
#else
DELAY_LOAD(g_hinstMPR, MPR.DLL, DWORD, WNetGetConnectionW,
           (LPCTSTR pszLocalDevice, LPTSTR pszUNC, LPDWORD pcbUNC),
           (pszLocalDevice, pszUNC, pcbUNC));
#endif
// ---------- end of DLL definitions --------

#pragma warning(default:4229)

