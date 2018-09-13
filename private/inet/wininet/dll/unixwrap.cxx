/*
 * unixwrap.cxx
 *
 * Purpose:
 *          Implementation of Functions, to overcome following problems
 *          On Unix:
 *
 *          1. Unix map files do not support statments such as
 *              InternetTimeFromSystemTimeA
 *              InternetTimeFromSystemTimeW
 *              InternetTimeFromSystemTime=InternetTimeFromSystemTimeA
 *          2. Delay loading of functions.
 *
 * Functions:
 *          InternetTimeFromSystemTime
 *          InternetTimeToSystemTime
 *          InternetSetStatusCallback
 *          InternetConfirmZoneCrossing
 *          UnlockUrlCacheEntryFile
 *          DeleteUrlCacheEntry
 *          SetUrlCacheEntryGroup
 *          InternetShowSecurityInfoByURL
 *          InternetDial
 *          InternetSetDialState
 *          InternetGoOnline
 *          InternetGetConnectedStateEx
 *          InternetGetCertByURL
 *
 * ----- CRYPT32 Wrappers -------------------
 *          CryptDecodeObject
 *          CertFindRDNAttr
 *          CertFreeCertificateContext
 *          CryptGetProvParam
 *          CertCreateCertificateContext
 *          CertSetCertificateContextProperty
 *          CertGetCertificateContextProperty
 *          CertNameToStrA
 *          CryptSetProvParam
 *          CertRDNValueToStrA
 *          CryptReleaseContext
 *          CertDuplicateCertificateContext
 *          CertCloseStore
 *          CertControlStore
 *          CertOpenSystemStoreA
 *          CryptAcquireContextA
 *          CertFindCertificateInStore
 */ 

#include <wininetp.h>

/*****************************************************************************/
/*================= Begin of the delay loading definitions ==================*/
/*****************************************************************************/

/*
 * Delay loading mechanism.  This allows you to write code as if you are
 * calling implicitly linked APIs, and yet have these APIs really be
 * explicitly linked.  You can reduce the initial number of DLLs that 
 * are loaded (load on demand) using this technique.
 *
 * Use the following macros to indicate which APIs/DLLs are delay-linked
 * and -loaded.
 *
 *      DELAY_LOAD
 *      DELAY_LOAD_HRESULT
 *      DELAY_LOAD_SAFEARRAY
 *      DELAY_LOAD_UINT
 *      DELAY_LOAD_INT
 *      DELAY_LOAD_VOID
 *
 */

#define ENSURE_LOADED(_hmod, _dll, _ext, pszfn)         \
    (_hmod ? (_hmod) : (_hmod = LoadLibraryA(#_dll "." #_ext)))

void _GetProcFromDLL(HMODULE* phmod, LPCSTR pszDLL, FARPROC* ppfn, LPCSTR pszProc)
{
    /*
     * If it's already loaded, return.
     */
    if (*ppfn) {
        return;
    }

    if (*phmod == NULL)
    {
       *phmod = LoadLibraryA(pszDLL);
       if (*phmod == NULL)
       {
          return;
       }
    }

    *ppfn = GetProcAddress(*phmod, pszProc);
}

/*
 * NOTE: this takes two parameters that are the function name.
 * the First (_fn) is the name that the function will be called in 
 * this DLL and the other (_fni) is the name of the function we 
 * will GetProcAddress. This helps get around functions that
 * are defined in the header files with _declspec...
 * 
 *  HMODULE _hmod - where we cache the HMODULE (aka HINSTANCE)
 *           _dll - Basename of the target DLL, not quoted
 *           _ext - Extension of the target DLL, not quoted (usually DLL)
 *           _ret - Data type of return value
 *        _fnpriv - Local name for the function
 *            _fn - Exported name for the function
 *          _args - Argument list in the form (TYPE1 arg1, TYPE2 arg2, ...)
 *         _nargs - Argument list in the form (arg1, arg2, ...)
 *           _err - Return value if we can't call the actual function
 */
#define DELAY_LOAD_NAME_EXT_ERR(_hmod, _dll, _ext, _ret, _fnpriv, _fn, _args, _nargs, _err) \
_ret __stdcall _fnpriv _args                \
{                                       \
    static _ret (__stdcall *_pfn##_fn) _args = NULL;   \
    _GetProcFromDLL(&_hmod, #_dll "." #_ext, (FARPROC*)&_pfn##_fn, #_fn); \
    if (_pfn##_fn)               \
        return _pfn##_fn _nargs; \
    return (_ret)_err;           \
}

#define DELAY_LOAD_NAME_ERR(_hmod, _dll,       _ret, _fnpriv, _fn, _args, _nargs, _err) \
        DELAY_LOAD_NAME_EXT_ERR(_hmod, _dll,  DLL, _ret, _fnpriv, _fn, _args, _nargs, _err)

#define DELAY_LOAD_ERR(_hmod, _dll, _ret, _fn,      _args, _nargs, _err) \
        DELAY_LOAD_NAME_ERR(_hmod, _dll, _ret, _fn, _fn, _args, _nargs, _err)

#define DELAY_LOAD(_hmod, _dll, _ret, _fn, _args, _nargs) \
        DELAY_LOAD_ERR(_hmod, _dll, _ret, _fn, _args, _nargs, 0)

#define DELAY_LOAD_HRESULT(_hmod, _dll, _fn, _args, _nargs) \
        DELAY_LOAD_ERR(_hmod, _dll, HRESULT, _fn, _args, _nargs, E_FAIL)

#define DELAY_LOAD_SAFEARRAY(_hmod, _dll, _fn, _args, _nargs) \
        DELAY_LOAD_ERR(_hmod, _dll, SAFEARRAY *, _fn, _args, _nargs, NULL)

#define DELAY_LOAD_UINT(_hmod, _dll, _fn, _args, _nargs) \
        DELAY_LOAD_ERR(_hmod, _dll, UINT, _fn, _args, _nargs, 0)

#define DELAY_LOAD_INT(_hmod, _dll, _fn, _args, _nargs) \
        DELAY_LOAD_ERR(_hmod, _dll, INT, _fn, _args, _nargs, 0)

#define DELAY_LOAD_BOOL(_hmod, _dll, _fn, _args, _nargs) \
        DELAY_LOAD_ERR(_hmod, _dll, BOOL, _fn, _args, _nargs, FALSE)

#define DELAY_LOAD_BOOLEAN(_hmod, _dll, _fn, _args, _nargs) \
        DELAY_LOAD_ERR(_hmod, _dll, BOOLEAN, _fn, _args, _nargs, FALSE)

#define DELAY_LOAD_DWORD(_hmod, _dll, _fn, _args, _nargs) \
        DELAY_LOAD_ERR(_hmod, _dll, DWORD, _fn, _args, _nargs, FALSE)

#define DELAY_LOAD_WNET(_hmod, _dll, _fn, _args, _nargs) \
        DELAY_LOAD_ERR(_hmod, _dll, DWORD, _fn, _args, _nargs, WN_NOT_SUPPORTED)

/*
 * the NAME variants allow the local function to be called something different from the imported
 * function to avoid dll linkage problems.
 */
#define DELAY_LOAD_NAME(_hmod, _dll, _ret, _fn, _fni, _args, _nargs) \
        DELAY_LOAD_NAME_ERR(_hmod, _dll, _ret, _fn, _fni, _args, _nargs, 0)

#define DELAY_LOAD_NAME_HRESULT(_hmod, _dll, _fn, _fni, _args, _nargs) \
        DELAY_LOAD_NAME_ERR(_hmod, _dll, HRESULT, _fn, _fni, _args, _nargs, E_FAIL)

#define DELAY_LOAD_NAME_SAFEARRAY(_hmod, _dll, _fn, _fni, _args, _nargs) \
        DELAY_LOAD_NAME_ERR(_hmod, _dll, SAFEARRAY *, _fn, _fni, _args, _nargs, NULL)

#define DELAY_LOAD_NAME_UINT(_hmod, _dll, _fn, _fni, _args, _nargs) \
        DELAY_LOAD_NAME_ERR(_hmod, _dll, UINT, _fn, _fni, _args, _nargs, 0)

#define DELAY_LOAD_NAME_BOOL(_hmod, _dll, _fn, _fni, _args, _nargs) \
        DELAY_LOAD_NAME_ERR(_hmod, _dll, BOOL, _fn, _fni, _args, _nargs, FALSE)

#define DELAY_LOAD_NAME_DWORD(_hmod, _dll, _fn, _fni, _args, _nargs) \
        DELAY_LOAD_NAME_ERR(_hmod, _dll, DWORD, _fn, _fni, _args, _nargs, 0)

#define DELAY_LOAD_NAME_VOID(_hmod, _dll, _fn, _fni, _args, _nargs)                             \
void __stdcall _fn _args                                                                \
{                                                                                       \
    static void (__stdcall *_pfn##_fni) _args = NULL;                                   \
    if (!ENSURE_LOADED(_hmod, _dll, DLL, TEXT(#_fni)))                                       \
    {                                                                                   \
        AssertMsg(BOOLFROMPTR(_hmod), TEXT("LoadLibrary failed on ") ## TEXT(#_dll));         \
        return;                                                                         \
    }                                                                                   \
    if (_pfn##_fni == NULL)                                                              \
    {                                                                                   \
        *(FARPROC*)&(_pfn##_fni) = GetProcAddress(_hmod, #_fni);                         \
        AssertMsg(BOOLFROMPTR(_pfn##_fni), TEXT("GetProcAddress failed on ") ## TEXT(#_fni));    \
        if (_pfn##_fni == NULL)                                                          \
            return;                                                                     \
    }                                                                                   \
    _pfn##_fni _nargs;                                                                   \
}

#define DELAY_LOAD_VOID(_hmod, _dll, _fn, _args, _nargs) \
        DELAY_LOAD_NAME_VOID(_hmod, _dll, _fn, _fn, _args, _nargs)

#define DELAY_LOAD_EXT(_hmod, _dll, _ext, _ret, _fn, _args, _nargs) \
        DELAY_LOAD_NAME_EXT_ERR(_hmod, _dll, _ext, _ret, _fn, _fn, _args, _nargs, 0)

/*****************************************************************************/
/*================= End   of the delay loading definitions ==================*/
/*****************************************************************************/

/*************************************/
/*=== Begin: Wrappers for CRYPT32 ===*/
/*************************************/

HINSTANCE g_hinstCRYPT32 = NULL;

DELAY_LOAD(g_hinstCRYPT32, CRYPT32, BOOL, CryptDecodeObject,
    (DWORD dwCertEncodingType, LPCSTR lpszStructType, const BYTE* pbEncoded, DWORD cbEncoded, DWORD dwFlags, void* pvStructInfo, DWORD *pcbStructInfo),
    (dwCertEncodingType, lpszStructType, pbEncoded, cbEncoded, dwFlags, pvStructInfo, pcbStructInfo ))

DELAY_LOAD(g_hinstCRYPT32, CRYPT32, PCERT_RDN_ATTR, CertFindRDNAttr,
    (LPCSTR pszObjId, PCERT_NAME_INFO pName), (pszObjId, pName))

DELAY_LOAD(g_hinstCRYPT32, CRYPT32, BOOL, CertFreeCertificateContext,
    (PCCERT_CONTEXT pCertContext), (pCertContext))

DELAY_LOAD(g_hinstCRYPT32, CRYPT32, BOOL, CryptGetProvParam,
    (HCRYPTPROV hProv, DWORD dwParam, BYTE* pbData, DWORD* pdwDataLen, DWORD dwFlags), (hProv, dwParam, pbData, pdwDataLen, dwFlags))

DELAY_LOAD(g_hinstCRYPT32, CRYPT32, PCCERT_CONTEXT, CertCreateCertificateContext,
    (DWORD dwCertEncodingType, const BYTE* pbCertEncoded, DWORD cbCertEncoded),
    (dwCertEncodingType, pbCertEncoded, cbCertEncoded))

DELAY_LOAD(g_hinstCRYPT32, CRYPT32, BOOL, CertSetCertificateContextProperty,
    (PCCERT_CONTEXT pCertContext, DWORD dwPropId, DWORD dwFlags, const void* pvData), (pCertContext, dwPropId, dwFlags, pvData))

DELAY_LOAD(g_hinstCRYPT32, CRYPT32, BOOL, CertGetCertificateContextProperty,
    (PCCERT_CONTEXT pCertContext, DWORD dwPropId, void* pvData, DWORD* pcbData),
    (pCertContext, dwPropId, pvData, pcbData))

#ifdef UNICODE
DELAY_LOAD(g_hinstCRYPT32, CRYPT32, DWORD, CertNameToStrW,
    (DWORD dwCertEncodingType, PCERT_NAME_BLOB pName, DWORD dwStrType, LPWSTR psz, DWORD csz),
    (dwCertEncodingType, pName, dwStrType, psz, csz))
#else
DELAY_LOAD(g_hinstCRYPT32, CRYPT32, DWORD, CertNameToStrA,
    (DWORD dwCertEncodingType, PCERT_NAME_BLOB pName, DWORD dwStrType, LPSTR psz, DWORD csz),
    (dwCertEncodingType, pName, dwStrType, psz, csz))
#endif /* UNICODE */

DELAY_LOAD(g_hinstCRYPT32, CRYPT32, BOOL, CryptSetProvParam,
    (HCRYPTPROV hProv, DWORD dwParam, BYTE* pbData, DWORD dwFlags),
    (hProv, dwParam, pbData, dwFlags))

#ifdef UNICODE
DELAY_LOAD(g_hinstCRYPT32, CRYPT32, DWORD, CertRDNValueToStrW,
    (DWORD dwValueType, PCERT_RDN_VALUE_BLOB pValue, LPWSTR psz, DWORD csz),
    (dwValueType, pValue, psz, csz))
#else
DELAY_LOAD(g_hinstCRYPT32, CRYPT32, DWORD, CertRDNValueToStrA,
    (DWORD dwValueType, PCERT_RDN_VALUE_BLOB pValue, LPSTR psz, DWORD csz),
    (dwValueType, pValue, psz, csz))
#endif /* UNICODE */

DELAY_LOAD(g_hinstCRYPT32, CRYPT32, BOOL, CryptReleaseContext,
    (HCRYPTPROV hProv, DWORD dwFlags), (hProv, dwFlags))

DELAY_LOAD(g_hinstCRYPT32, CRYPT32, PCCERT_CONTEXT, CertDuplicateCertificateContext,
    (PCCERT_CONTEXT pCertContext), (pCertContext))

DELAY_LOAD(g_hinstCRYPT32, CRYPT32, BOOL, CertCloseStore,
    (HCERTSTORE hCertStore, DWORD dwFlags), (hCertStore, dwFlags))

DELAY_LOAD(g_hinstCRYPT32, CRYPT32, BOOL, CertControlStore,
    (HCERTSTORE hCertStore, DWORD dwFlags, DWORD dwCtrlType, void const* pvCtrlPara),
    (hCertStore, dwFlags, dwCtrlType, pvCtrlPara))

#ifdef UNICODE
DELAY_LOAD(g_hinstCRYPT32, CRYPT32, HCERTSTORE, CertOpenSystemStoreW,
    (HCRYPTPROV hProv, LPCWSTR szSubsystemProtocol),
    (hProv, szSubsystemProtocol))
#else
DELAY_LOAD(g_hinstCRYPT32, CRYPT32, HCERTSTORE, CertOpenSystemStoreA,
    (HCRYPTPROV hProv, LPCSTR szSubsystemProtocol),
    (hProv, szSubsystemProtocol))
#endif /* UNICODE */

#ifdef UNICODE
DELAY_LOAD(g_hinstCRYPT32, CRYPT32, BOOL, CryptAcquireContextW,
    (HCRYPTPROV* phProv, LPCWSTR pszContainer, LPCWSTR pszProvider, DWORD dwProvType, DWORD dwFlags),
    (phProv, pszContainer, pszProvider, dwProvType, dwFlags))
#else
DELAY_LOAD(g_hinstCRYPT32, CRYPT32, BOOL, CryptAcquireContextA,
    (HCRYPTPROV* phProv, LPCSTR pszContainer, LPCSTR pszProvider, DWORD dwProvType, DWORD dwFlags),
    (phProv, pszContainer, pszProvider, dwProvType, dwFlags))
#endif /* UNICODE */

DELAY_LOAD(g_hinstCRYPT32, CRYPT32, PCCERT_CONTEXT, CertFindCertificateInStore,
    (HCERTSTORE hCertStore, DWORD dwCertEncodingType, DWORD dwFindFlags, DWORD dwFindType, const void* pvFindPara, PCCERT_CONTEXT pPrevCertContext),
    (hCertStore, dwCertEncodingType, dwFindFlags, dwFindType, pvFindPara, pPrevCertContext))

/*************************************/
/*=== End:   Wrappers for CRYPT32 ===*/
/*************************************/

/* In wininet.def, we don't support statements like
 * InternetTimeFromSystemTime=InternetTimeFromSystemTimeA
 * So, on Unix, we create a wrapper for InternetTimeFromSystemTime which will
 * call the corresponding ANSI function.
 *
 * The following are all such wrappers ----
 */

extern "C" {

#ifdef InternetTimeFromSystemTime
#undef InternetTimeFromSystemTime
#endif

INTERNETAPI
BOOL
WINAPI
InternetTimeFromSystemTime(
    IN  CONST SYSTEMTIME *pst,  // input GMT time
    IN  DWORD dwRFC,            // RFC format: must be FORMAT_RFC1123
    OUT LPSTR lpszTime,         // output string buffer
    IN  DWORD cbTime            // output buffer size
    ) {
    return InternetTimeFromSystemTimeA(pst, dwRFC, lpszTime, cbTime);
}

#ifdef InternetTimeToSystemTime
#undef InternetTimeToSystemTime
#endif

INTERNETAPI
BOOL
WINAPI
InternetTimeToSystemTime(
    IN  LPCSTR lpcszTimeString,
    OUT SYSTEMTIME *lpSysTime,
    IN  DWORD dwReserved )
{
    return InternetTimeToSystemTimeA(lpcszTimeString, lpSysTime, dwReserved);
}

#ifdef InternetSetStatusCallback
#undef InternetSetStatusCallback
#endif

INTERNETAPI
INTERNET_STATUS_CALLBACK
WINAPI
InternetSetStatusCallback(
    IN HINTERNET hInternet,
    IN INTERNET_STATUS_CALLBACK lpfnInternetCallback
    )
{
    return InternetSetStatusCallbackA(hInternet, lpfnInternetCallback);
}

#ifdef InternetConfirmZoneCrossing
#undef InternetConfirmZoneCrossing
#endif

INTERNETAPI
DWORD
WINAPI
InternetConfirmZoneCrossing(
    IN HWND hWnd,
    IN LPSTR szUrlPrev,
    IN LPSTR szUrlNew,
    BOOL bPost
    )
{
    return InternetConfirmZoneCrossingA(hWnd, szUrlPrev, szUrlNew, bPost);
}

#ifdef UnlockUrlCacheEntryFile
#undef UnlockUrlCacheEntryFile
#endif

URLCACHEAPI
BOOL
WINAPI
UnlockUrlCacheEntryFile(
    LPCSTR lpszUrlName,
    IN DWORD dwReserved
    )
{
    return UnlockUrlCacheEntryFileA(lpszUrlName,dwReserved);
}

#ifdef DeleteUrlCacheEntry
#undef DeleteUrlCacheEntry
#endif

URLCACHEAPI
BOOL
WINAPI
DeleteUrlCacheEntry(
    IN LPCSTR lpszUrlName
    )
{
    return DeleteUrlCacheEntryA(lpszUrlName);
}

#ifdef SetUrlCacheEntryGroup
#undef SetUrlCacheEntryGroup
#endif

BOOLAPI
SetUrlCacheEntryGroup(
    IN LPCSTR   lpszUrlName,
    IN DWORD    dwFlags,
    IN GROUPID  GroupId,
    IN LPBYTE   pbGroupAttributes, // must pass NULL
    IN DWORD    cbGroupAttributes, // must pass 0
    IN LPVOID   lpReserved         // must pass NULL
    )
{
    return SetUrlCacheEntryGroupA(lpszUrlName, dwFlags, GroupId, pbGroupAttributes,
                                  cbGroupAttributes, lpReserved);
}

#ifdef InternetShowSecurityInfoByURL
#undef InternetShowSecurityInfoByURL
#endif

INTERNETAPI
BOOL
WINAPI
InternetShowSecurityInfoByURL(
    IN LPSTR     lpszURL,
    IN HWND      hwndRootWindow
    )
{
    return InternetShowSecurityInfoByURLA(lpszURL, hwndRootWindow);
}

#ifdef InternetDial
#undef InternetDial
#endif

DWORD InternetDial(HWND hwndParent, LPSTR pszConnectoid, DWORD dwFlags,
    LPDWORD lpdwConnection, DWORD dwReserved)
{
    return InternetDialA(hwndParent, pszConnectoid, dwFlags, lpdwConnection, dwReserved);
}

#ifdef InternetSetDialState
#undef InternetSetDialState
#endif

BOOLAPI InternetSetDialState(LPCSTR lpszConnectoid, DWORD dwState, DWORD dwReserved)
{
        return InternetSetDialStateA(lpszConnectoid, dwState, dwReserved);
}

#ifdef InternetGoOnline
#undef InternetGoOnline
#endif

BOOLAPI InternetGoOnline(LPSTR lpszURL, HWND hwndParent, DWORD dwFlags)
{
        return InternetGoOnlineA(lpszURL, hwndParent, dwFlags);
}

#ifdef InternetGetConnectedStateEx
#undef InternetGetConnectedStateEx
#endif

BOOL InternetGetConnectedStateEx(
    LPDWORD lpdwFlags,
    LPSTR lpszConnectionName,
    DWORD dwBufLen,
    DWORD dwReserved)
{
    return InternetGetConnectedStateExA(lpdwFlags, lpszConnectionName, dwBufLen, dwReserved);
}

#ifdef InternetGetCertByURL
#undef InternetGetCertByURL
#endif

INTERNETAPI
BOOL
WINAPI
InternetGetCertByURLA(
    IN LPSTR     lpszURL,
    IN OUT LPSTR lpszCertText,
    OUT DWORD    dwcbCertText
    );

INTERNETAPI
BOOL
WINAPI
InternetGetCertByURL(
    IN LPSTR     lpszURL,
    IN OUT LPSTR lpszCertText,
    OUT DWORD    dwcbCertText
    )
{
    return InternetGetCertByURLA(lpszURL, lpszCertText, dwcbCertText);
}


#ifdef HttpCheckDavCompliance
#undef HttpCheckDavCompliance
#endif

INTERNETAPI
BOOL
WINAPI
HttpCheckDavComplianceA(
    IN LPCSTR lpszUrl,
    IN LPCSTR lpszComplianceToken,
    IN OUT LPBOOL lpfFound,
    IN HWND hWnd,
    IN LPVOID lpvReserved
    );

INTERNETAPI
BOOL
WINAPI
HttpCheckDavCompliance(
    IN LPCSTR lpszUrl,
    IN LPCSTR lpszComplianceToken,
    IN OUT LPBOOL lpfFound,
    IN HWND hWnd,
    IN LPVOID lpvReserved
    )
{
    return HttpCheckDavComplianceA(lpszUrl,
                                   lpszComplianceToken,
                                   lpfFound,
                                   hWnd,
                                   lpvReserved);
}

#ifdef DAVCACHING // commenting out as per bug 15696

#ifdef HttpCheckCachedDavStatus
#undef HttpCheckCachedDavStatus
#endif

INTERNETAPI
BOOL
WINAPI
HttpCheckCachedDavStatusA(
    IN LPCSTR lpszUrl,
    IN OUT LPDWORD lpdwStatus
    );

INTERNETAPI
BOOL
WINAPI
HttpCheckCachedDavStatus(
    IN LPCSTR lpszUrl,
    IN OUT LPDWORD lpdwStatus
    )
{
    return HttpCheckCachedDavStatusA(lpszUrl, lpdwStatus);
}

#endif /* DAVCACHING */

#ifndef UNIX_BUILDS_AUTOPROXY_DETECT
/* Stub function if apdetect is not included in build */
BOOL
WINAPI
DetectAutoProxyUrl(
    IN OUT LPSTR lpszAutoProxyUrl,
    IN DWORD dwAutoProxyUrlLength,
    IN DWORD dwDetectFlags
    )
{
    return FALSE;
}
#endif /* UNIX_BUIDS_AUTOPROXY_DETECT */


}
