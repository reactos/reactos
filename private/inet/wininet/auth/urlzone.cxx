/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    urlzone.cxx

Abstract:

    Glue layer to zone security manager, now residing in urlmon.dll

Author:

    Rajeev Dujari (rajeevd)  02-Aug-1997

Contents:

    UrlZonesAttach
    UrlZonesDetach
    GetCredPolicy

--*/


#include <wininetp.h>
#include "urlmon.h"

//
// prototypes
//
typedef HRESULT (*PFNCREATESECMGR)(IServiceProvider * pSP, IInternetSecurityManager **ppSM, DWORD dwReserved);
typedef HRESULT (*PFNCREATEZONEMGR)(IServiceProvider * pSP, IInternetZoneManager **ppZM, DWORD dwReserved);

//
// globals
//
static IInternetSecurityManager* g_pSecMgr  = NULL;
static IInternetZoneManager*     g_pZoneMgr = NULL;
static BOOL g_bAttemptedInit = FALSE;
static HINSTANCE g_hInstUrlMon = NULL;
static PFNCREATESECMGR  g_pfnCreateSecMgr  = NULL;
static PFNCREATEZONEMGR g_pfnCreateZoneMgr = NULL;

//
// Handy class which uses a stack buffer if possible, otherwise allocates.
//
struct FlexBuf
{
    BYTE Buf[512];
    LPBYTE pbBuf;

    FlexBuf()
    {
        pbBuf = NULL;
    }

    ~FlexBuf()
    {
        if (pbBuf != NULL) {
            delete [] pbBuf;
        }
    }

    LPBYTE GetPtr (DWORD cbBuf)
    {
        if (cbBuf < sizeof(Buf))
            return Buf;
        pbBuf = new BYTE [cbBuf];
        return pbBuf;
    }
};

struct UnicodeBuf : public FlexBuf
{
    LPWSTR Convert (LPCSTR pszUrl)
    {
        DWORD cbUrl = lstrlenA (pszUrl) + 1;
        LPWSTR pwszUrl = (LPWSTR) GetPtr (sizeof(WCHAR) * cbUrl);
        if (!pwszUrl)
            return NULL;
        MultiByteToWideChar
            (CP_ACP, 0, pszUrl, cbUrl, pwszUrl, cbUrl);
        return pwszUrl;
    }
};

//
// Dynaload urlmon and create security manager object on demand.
//

BOOL UrlZonesAttach (void)
{
    EnterCriticalSection(&ZoneMgrCritSec);
    BOOL bRet = FALSE;
    HRESULT hr;

    if (g_bAttemptedInit)
    {
        bRet = (g_pSecMgr != NULL);
        goto End;
    }

    g_bAttemptedInit = TRUE;

    g_hInstUrlMon = LoadLibraryA("urlmon.dll");
    if (!g_hInstUrlMon)
    {
        bRet = FALSE;
        goto End;
    }

    g_pfnCreateSecMgr = (PFNCREATESECMGR)
        GetProcAddress(g_hInstUrlMon, "CoInternetCreateSecurityManager");
    if (!g_pfnCreateSecMgr)
    {
        bRet = FALSE;
        goto End;
    }

    g_pfnCreateZoneMgr = (PFNCREATEZONEMGR)
        GetProcAddress(g_hInstUrlMon, "CoInternetCreateZoneManager");
    if (!g_pfnCreateZoneMgr)
    {
        bRet = FALSE;
        goto End;
    }

    hr = (*g_pfnCreateSecMgr)(NULL, &g_pSecMgr, NULL);
    if( hr != S_OK )
    {
        bRet = FALSE;
        goto End;
    }

    hr = (*g_pfnCreateZoneMgr)(NULL, &g_pZoneMgr, NULL);
    bRet = (hr == S_OK);

End:
    LeaveCriticalSection(&ZoneMgrCritSec);
    return bRet;
}

//
// Clean up upon process detach.
//

#ifdef UNIX
extern "C"
#endif /* UNIX */
VOID UrlZonesDetach (void)
{
    EnterCriticalSection(&ZoneMgrCritSec);

    if (g_pSecMgr)
    {
        g_pSecMgr->Release();
        g_pSecMgr = NULL;
    }
    if (g_pZoneMgr)
    {
        g_pZoneMgr->Release();
        g_pZoneMgr = NULL;
    }
    if (g_hInstUrlMon)
    {
        FreeLibrary(g_hInstUrlMon);
        g_hInstUrlMon = NULL;
    }
    LeaveCriticalSection(&ZoneMgrCritSec);
}

//
// Routine to get the Routine to indicate whether ntlm logon credential is allowed.
//

DWORD GetCredPolicy (LPSTR pszUrl)
{
   UnicodeBuf ub;
   EnterCriticalSection(&ZoneMgrCritSec);

   if (!UrlZonesAttach())
        goto err;

    // Convert to unicode.
    LPWSTR pwszUrl;
    pwszUrl = ub.Convert (pszUrl);
    if (!pwszUrl)
        goto err;

    DWORD dwPolicy;

    HRESULT hr;
    hr = g_pSecMgr->ProcessUrlAction (pwszUrl, URLACTION_CREDENTIALS_USE,
        (LPBYTE) &dwPolicy, sizeof(dwPolicy), NULL, 0, PUAF_NOUI, 0);
    if (hr != S_OK)
        goto err;

    if (dwPolicy == URLPOLICY_CREDENTIALS_CONDITIONAL_PROMPT)
    {
        // Map the policy into silent or prompt depending on the zone.

        DWORD dwZone;

        hr = g_pSecMgr->MapUrlToZone (pwszUrl, &dwZone, 0);
        if (hr != S_OK)
            goto err;
        if (dwZone == URLZONE_INTRANET)
            dwPolicy = URLPOLICY_CREDENTIALS_SILENT_LOGON_OK;
        else
            dwPolicy = URLPOLICY_CREDENTIALS_MUST_PROMPT_USER;
    }

    LeaveCriticalSection(&ZoneMgrCritSec);
    return dwPolicy;

err:
    INET_ASSERT (FALSE);
    LeaveCriticalSection(&ZoneMgrCritSec);
    return URLPOLICY_CREDENTIALS_MUST_PROMPT_USER;
}

DWORD GetCookiePolicy (LPCSTR pszUrl, BOOL fIsSessionCookie)
{
    EnterCriticalSection(&ZoneMgrCritSec);
    UnicodeBuf ub;
    DWORD dwCP;
    DWORD dwUrlAction = URLACTION_COOKIES;

    if (!UrlZonesAttach())
        goto err;

    // Convert to unicode.
    LPWSTR pwszUrl;
    pwszUrl = ub.Convert (pszUrl);
    if (!pwszUrl)
        goto err;

    DWORD dwPolicy;

    HRESULT hr;

    if( fIsSessionCookie )
        dwUrlAction = URLACTION_COOKIES_SESSION;

    hr = g_pSecMgr->ProcessUrlAction (pwszUrl, dwUrlAction,
            (LPBYTE) &dwPolicy, sizeof(dwPolicy), NULL, 0, PUAF_NOUI, 0);

    if (!SUCCEEDED(hr) )
        goto err;

    dwCP = GetUrlPolicyPermissions(dwPolicy);
    LeaveCriticalSection(&ZoneMgrCritSec);
    return dwCP;

err:
    LeaveCriticalSection(&ZoneMgrCritSec);
    INET_ASSERT (FALSE);
    return URLPOLICY_QUERY;
}

VOID SetStopWarning( LPCSTR pszUrl, DWORD dwPolicy, BOOL fIsSessionCookie)
{
    EnterCriticalSection(&ZoneMgrCritSec);
    UnicodeBuf ub;
    DWORD dwUrlAction = URLACTION_COOKIES;

    if (!UrlZonesAttach())
        goto err;

    // Convert to unicode.
    LPWSTR pwszUrl;
    pwszUrl = ub.Convert (pszUrl);
    if (!pwszUrl)
        goto err;

    HRESULT hr;
    DWORD  dwZone;
    hr = g_pSecMgr->MapUrlToZone(pwszUrl, &dwZone, 0);
    if (!SUCCEEDED(hr) )
        goto err;

    DWORD dwZonePolicy;

    if( fIsSessionCookie )
        dwUrlAction = URLACTION_COOKIES_SESSION;

    hr = g_pZoneMgr->GetZoneActionPolicy(
        dwZone,
        dwUrlAction,
        (LPBYTE)&dwZonePolicy,
        sizeof(dwZonePolicy),
        URLZONEREG_DEFAULT );

    if (!SUCCEEDED(hr) )
        goto err;

    // set the policy back with passed value
    SetUrlPolicyPermissions(dwZonePolicy, dwPolicy);

    hr = g_pZoneMgr->SetZoneActionPolicy(
        dwZone,
        dwUrlAction,
        (LPBYTE) &dwZonePolicy,
        sizeof(dwZonePolicy),
        URLZONEREG_DEFAULT );

    if (!SUCCEEDED(hr) )
        goto err;

err:
    LeaveCriticalSection(&ZoneMgrCritSec);
    return;
}
