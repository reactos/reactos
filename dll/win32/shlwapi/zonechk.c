/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Implementing ZoneCheck* functions (Internet Zone Manager)
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <windef.h>
#include <urlmon.h>
#include <shlobj.h>
#define NO_SHLWAPI_REG
#include <shlwapi_undoc.h>
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(zonechk);

static IClassFactory *g_pZoneMgrCF = NULL; /* Internet Zone Manager's Class Factory (cached) */
CRITICAL_SECTION g_csZoneMgrLock; /* Guards g_pZoneMgrCF (ReactOS only) */
static HINSTANCE g_hinstZoneMgr = NULL; /* The module of Zone Manager */

static HRESULT
SHLWAPI_GetCachedZonesManagerInner(
    _In_ REFIID riid,
    _Out_ PVOID *ppv)
{
    HRESULT hr;
    IClassFactory *pCF;

    if (!g_pZoneMgrCF)
    {
        hr = CoGetClassObject(&CLSID_InternetSecurityManager, CLSCTX_INPROC_SERVER, NULL,
                              &IID_IClassFactory, (PVOID *)&pCF);
        if (FAILED(hr))
        {
            *ppv = NULL;
            return hr;
        }

        g_pZoneMgrCF = pCF;
        g_hinstZoneMgr = SHPinDllOfCLSID(&CLSID_InternetSecurityManager);
    }

    return g_pZoneMgrCF->lpVtbl->CreateInstance(g_pZoneMgrCF, NULL, riid, ppv);
}

/*************************************************************************
 * SHLWAPI_GetCachedZonesManager
 *
 * An internal helper that caches the InternetSecurityManager's IClassFactory and
 * returns an instance of the specified interface.
 */
static HRESULT
SHLWAPI_GetCachedZonesManager(
    _In_ REFIID riid,
    _Out_ PVOID *ppv)
{
    HRESULT hr;
    EnterCriticalSection(&g_csZoneMgrLock);
    hr = SHLWAPI_GetCachedZonesManagerInner(riid, ppv);
    LeaveCriticalSection(&g_csZoneMgrLock);
    return hr;
}

EXTERN_C VOID SHLWAPI_DeleteCachedZonesManager(VOID)
{
    EnterCriticalSection(&g_csZoneMgrLock);
    if (g_pZoneMgrCF)
    {
        g_pZoneMgrCF->lpVtbl->Release(g_pZoneMgrCF);
        g_pZoneMgrCF = NULL;
    }
    if (g_hinstZoneMgr)
    {
        FreeLibrary(g_hinstZoneMgr);
        g_hinstZoneMgr = NULL;
    }
    LeaveCriticalSection(&g_csZoneMgrLock);
}

/*************************************************************************
 * SuperPrivate_ZoneCheckPath
 *
 * An internal helper, used in SHRegisterValidateTemplate
 */
HRESULT SuperPrivate_ZoneCheckPath(PCWSTR pszPath, DWORD dwExpectedZone)
{
    IInternetSecurityManager *pISM;
    HRESULT hr = SHLWAPI_GetCachedZonesManager(&IID_IInternetSecurityManager, (PVOID *)&pISM);
    if (FAILED(hr))
        return E_ACCESSDENIED;

    DWORD dwRealZone = URLZONE_UNTRUSTED;
    hr = pISM->lpVtbl->MapUrlToZone(pISM, pszPath, &dwRealZone, 0);
    if (SUCCEEDED(hr) && dwRealZone == dwExpectedZone)
        hr = S_OK;
    else
        hr = E_ACCESSDENIED;

    pISM->lpVtbl->Release(pISM);
    return hr;
}

/*************************************************************************
 * ZoneCheckUrlExCacheA [SHLWAPI.232]
 */
HRESULT WINAPI
ZoneCheckUrlExCacheA(
    _In_                             PCSTR                     pszUrl,
    _Out_writes_bytes_opt_(cbPolicy) PBYTE                     pbPolicy,
    _In_                             DWORD                     cbPolicy,
    _In_reads_bytes_opt_(cbContext)  PBYTE                     pbContext,
    _In_                             DWORD                     cbContext,
    _In_                             DWORD                     dwAction,
    _In_                             DWORD                     dwFlags,
    _In_opt_                         IInternetSecurityMgrSite *pSecuritySite,
    _In_opt_                         IInternetSecurityManager *pISM)
{
    WCHAR szUrl[2048];
    if (!pszUrl)
    {
        ERR("pszUrl was NULL\n");
        return E_INVALIDARG;
    }
    SHAnsiToUnicode(pszUrl, szUrl, _countof(szUrl));
    return ZoneCheckUrlExCacheW(szUrl, pbPolicy, cbPolicy, pbContext, cbContext,
                                dwAction, dwFlags, pSecuritySite, pISM);
}

/*************************************************************************
 * ZoneCheckUrlExCacheW [SHLWAPI.233]
 */
HRESULT WINAPI
ZoneCheckUrlExCacheW(
    _In_                             PCWSTR                    pszUrl,
    _Out_writes_bytes_opt_(cbPolicy) PBYTE                     pbPolicy,
    _In_                             DWORD                     cbPolicy,
    _In_reads_bytes_opt_(cbContext)  PBYTE                     pbContext,
    _In_                             DWORD                     cbContext,
    _In_                             DWORD                     dwAction,
    _In_                             DWORD                     dwFlags,
    _In_opt_                         IInternetSecurityMgrSite *pSecuritySite,
    _In_opt_                         IInternetSecurityManager *pISM)
{
    HRESULT hr;
    IInternetSecurityManager *pWorkISM;
    DWORD dwPolicyBuf, dwContextBuf;

    if (!pszUrl)
    {
        ERR("pszUrl was NULL\n");
        return E_INVALIDARG;
    }

    if (pISM && pISM->lpVtbl)
        hr = pISM->lpVtbl->QueryInterface(pISM, &IID_IInternetSecurityManager, (PVOID *)&pWorkISM);
    else
        hr = SHLWAPI_GetCachedZonesManager(&IID_IInternetSecurityManager, (PVOID *)&pWorkISM);

    if (FAILED(hr))
    {
        ERR("hr: 0x%lX\n", hr);
        return hr;
    }

    if (pSecuritySite)
        pWorkISM->lpVtbl->SetSecuritySite(pWorkISM, pSecuritySite);

    if (!pbContext)
    {
        dwContextBuf = 0;
        pbContext = (PBYTE)&dwContextBuf;
        cbContext = sizeof(dwContextBuf);
    }

    if (!pbPolicy)
    {
        dwPolicyBuf = 0;
        pbPolicy = (PBYTE)&dwPolicyBuf;
        cbPolicy = sizeof(dwPolicyBuf);
    }

    hr = pWorkISM->lpVtbl->ProcessUrlAction(pWorkISM, pszUrl, dwAction, pbPolicy, cbPolicy,
                                            pbContext, cbContext, dwFlags, 0);

    if (pSecuritySite)
        pWorkISM->lpVtbl->SetSecuritySite(pWorkISM, NULL);

    pWorkISM->lpVtbl->Release(pWorkISM);
    return hr;
}

/*************************************************************************
 * ZoneCheckPathA [SHLWAPI.226]
 */
HRESULT WINAPI
ZoneCheckPathA(
    _In_     PCSTR pszPath,
    _In_     DWORD dwAction,
    _In_     DWORD dwFlags,
    _In_opt_ IInternetSecurityMgrSite *pSecuritySite)
{
    WCHAR szPath[2048];
    if (!pszPath)
    {
        ERR("pszPath was NULL\n");
        return E_INVALIDARG;
    }
    SHAnsiToUnicode(pszPath, szPath, _countof(szPath));
    return ZoneCheckPathW(szPath, dwAction, dwFlags, pSecuritySite);
}

/*************************************************************************
 * ZoneCheckPathW [SHLWAPI.227]
 */
HRESULT WINAPI
ZoneCheckPathW(
    _In_     PCWSTR  pszPath,
    _In_     DWORD   dwAction,
    _In_     DWORD   dwFlags,
    _In_opt_ IInternetSecurityMgrSite *pSecuritySite)
{
    return ZoneCheckUrlW(pszPath, dwAction, dwFlags | PUAF_ISFILE, pSecuritySite);
}

/*************************************************************************
 * ZoneCheckUrlA [SHLWAPI.228]
 */
HRESULT WINAPI
ZoneCheckUrlA(
    _In_     PCSTR   pszUrl,
    _In_     DWORD   dwAction,
    _In_     DWORD   dwFlags,
    _In_opt_ IInternetSecurityMgrSite *pSecuritySite)
{
    WCHAR szUrl[2048];
    if (!pszUrl)
    {
        ERR("pszUrl was NULL\n");
        return E_INVALIDARG;
    }
    SHAnsiToUnicode(pszUrl, szUrl, _countof(szUrl));
    return ZoneCheckUrlW(szUrl, dwAction, dwFlags, pSecuritySite);
}

/*************************************************************************
 * ZoneCheckUrlW [SHLWAPI.229]
 */
HRESULT WINAPI
ZoneCheckUrlW(
    _In_     PCWSTR  pszUrl,
    _In_     DWORD   dwAction,
    _In_     DWORD   dwFlags,
    _In_opt_ IInternetSecurityMgrSite *pSecuritySite)
{
    return ZoneCheckUrlExW(pszUrl, NULL, 0, NULL, 0, dwAction, dwFlags, pSecuritySite);
}

/*************************************************************************
 * ZoneCheckUrlExA [SHLWAPI.230]
 */
HRESULT WINAPI
ZoneCheckUrlExA(
    _In_                             PCSTR   pszUrl,
    _Out_writes_bytes_opt_(cbPolicy) PBYTE   pbPolicy,
    _In_                             DWORD   cbPolicy,
    _In_reads_bytes_opt_(cbContext)  PBYTE   pbContext,
    _In_                             DWORD   cbContext,
    _In_                             DWORD   dwAction,
    _In_                             DWORD   dwFlags,
    _In_opt_                         IInternetSecurityMgrSite *pSecuritySite)
{
    WCHAR szUrl[2048];
    if (!pszUrl)
    {
        ERR("pszUrl was NULL\n");
        return E_INVALIDARG;
    }
    SHAnsiToUnicode(pszUrl, szUrl, _countof(szUrl));
    return ZoneCheckUrlExW(szUrl, pbPolicy, cbPolicy, pbContext, cbContext,
                           dwAction, dwFlags, pSecuritySite);
}

/*************************************************************************
 * ZoneCheckUrlExW [SHLWAPI.231]
 */
HRESULT WINAPI
ZoneCheckUrlExW(
    _In_                             PCWSTR  pszUrl,
    _Out_writes_bytes_opt_(cbPolicy) PBYTE   pbPolicy,
    _In_                             DWORD   cbPolicy,
    _In_reads_bytes_opt_(cbContext)  PBYTE   pbContext,
    _In_                             DWORD   cbContext,
    _In_                             DWORD   dwAction,
    _In_                             DWORD   dwFlags,
    _In_opt_                         IInternetSecurityMgrSite *pSecuritySite)
{
    return ZoneCheckUrlExCacheW(pszUrl, pbPolicy, cbPolicy, pbContext, cbContext,
                                dwAction, dwFlags, pSecuritySite, NULL);
}

/*************************************************************************
 * ZoneCheckHost [SHLWAPI.234]
 */
HRESULT WINAPI
ZoneCheckHost(
    _In_   IInternetSecurityManager  *pISM,
    _In_   PCWSTR                     pszUrl,
    _In_   DWORD                      dwAction)
{
    return ZoneCheckHostEx(pISM, NULL, 0, NULL, 0, pszUrl, dwAction);
}

/*************************************************************************
 * ZoneCheckHostEx [SHLWAPI.235]
 */
HRESULT WINAPI
ZoneCheckHostEx(
    _In_                             IInternetSecurityManager *pISM,
    _Out_writes_bytes_opt_(cbPolicy) PBYTE                     pbPolicy,
    _In_                             DWORD                     cbPolicy,
    _In_reads_bytes_opt_(cbContext)  PBYTE                     pbContext,
    _In_                             DWORD                     cbContext,
    _In_                             PCWSTR                    pszUrl,
    _In_                             DWORD                     dwAction)
{
    DWORD dwPolicyBuf, dwContextBuf;

    if (!pISM || !pszUrl)
        return E_INVALIDARG;

    if (!pbPolicy)
    {
        dwPolicyBuf = 0;
        pbPolicy = (PBYTE)&dwPolicyBuf;
        cbPolicy = sizeof(dwPolicyBuf);
    }

    if (!pbContext)
    {
        dwContextBuf = 0;
        pbContext = (PBYTE)&dwContextBuf;
        cbContext = sizeof(dwContextBuf);
    }

    return pISM->lpVtbl->ProcessUrlAction(pISM, pszUrl, dwAction, pbPolicy, cbPolicy,
                                          pbContext, cbContext, 0, 0);
}
