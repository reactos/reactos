/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Implementing ZoneCheck* functions
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <windef.h>
#include <winbase.h>
#include <urlmon.h>
#include <shlobj.h>
#define NO_SHLWAPI_REG
#include <shlwapi.h>
#include <shlwapi_undoc.h>
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(zonechk);

static IClassFactory *g_pcf = NULL;

/* FIXME: CLSID_InternetSecurityManager */
/* FIXME: ZoneCheckHost, ZoneCheckHostEx */

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
    if (g_pcf)
        return g_pcf->lpVtbl->CreateInstance(g_pcf, NULL, riid, ppv);

    CoGetClassObject(&CLSID_InternetSecurityManager,
                     CLSCTX_INPROC_SERVER,
                     NULL,
                     &IID_IClassFactory,
                     (PVOID *)&g_pcf);

    SHPinDllOfCLSID(&CLSID_InternetSecurityManager);

    if (!g_pcf)
    {
        *ppv = NULL;
        return E_FAIL;
    }

    return g_pcf->lpVtbl->CreateInstance(g_pcf, NULL, riid, ppv);
}

/*************************************************************************
 * SuperPrivate_ZoneCheckPath
 *
 * An internal helper
 */
HRESULT SuperPrivate_ZoneCheckPath(PCWSTR pwszUrl, DWORD dwZone)
{
    IInternetSecurityManager *pISM;
    HRESULT hr = SHLWAPI_GetCachedZonesManager(&IID_IInternetSecurityManager, (PVOID *)&pISM);
    if (FAILED(hr))
        return E_ACCESSDENIED;

    DWORD dwRealZone = URLZONE_UNTRUSTED;
    hr = pISM->lpVtbl->MapUrlToZone(pISM, pwszUrl, &dwRealZone, 0);
    if (SUCCEEDED(hr) && dwRealZone == dwZone)
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
    _In_z_       PCSTR                     pszUrl,
    _In_opt_     PBYTE                     pbPolicy,
    _In_         DWORD                     cbPolicy,
    _In_opt_     PBYTE                     pbContext,
    _In_         DWORD                     cbContext,
    _In_         DWORD                     dwAction,
    _In_         DWORD                     dwFlags,
    _In_opt_     IInternetSecurityMgrSite  *pSecuritySite,
    _In_opt_     IInternetSecurityManager  *pISM)
{
    WCHAR szUrl[2048];
    SHAnsiToUnicode(pszUrl, szUrl, _countof(szUrl));
    return ZoneCheckUrlExCacheW(szUrl, pbPolicy, cbPolicy, pbContext, cbContext,
                                dwAction, dwFlags, pSecuritySite, pISM);
}

/*************************************************************************
 * ZoneCheckUrlExCacheW [SHLWAPI.233]
 */
HRESULT WINAPI
ZoneCheckUrlExCacheW(
    _In_z_       PCWSTR                    pszUrl,
    _Out_writes_bytes_opt_(cbPolicy)
                 PBYTE                     pbPolicy,
    _In_         DWORD                     cbPolicy,
    _In_reads_bytes_opt_(cbContext)
                 PBYTE                     pbContext,
    _In_         DWORD                     cbContext,
    _In_         DWORD                     dwAction,
    _In_         DWORD                     dwFlags,
    _In_opt_     IInternetSecurityMgrSite  *pSecuritySite,
    _In_opt_     IInternetSecurityManager  *pISM)
{
    HRESULT hr;
    IInternetSecurityManager *pNewISM;
    DWORD dwPolicyBuf, dwContextBuf, cbPolicyEffect;
    PBYTE pbPolicyEffect, pbContextEffect;

    if (!pszUrl)
        return E_INVALIDARG;

    if (pISM && pISM->lpVtbl)
    {
        hr = pISM->lpVtbl->QueryInterface(pISM, &IID_IInternetSecurityManager,
                                          (PVOID *)&pNewISM);
    }
    else
    {
        hr = SHLWAPI_GetCachedZonesManager(&IID_IInternetSecurityManager,
                                           (PVOID *)&pNewISM);
        if (FAILED(hr))
            return hr;

        if (pISM)
            hr = pISM->lpVtbl->QueryInterface(pISM, &IID_IInternetSecurityManager,
                                              (PVOID *)&pNewISM);
    }

    if (FAILED(hr))
        return hr;

    dwPolicyBuf = dwContextBuf = 0;

    if (pSecuritySite)
        pNewISM->lpVtbl->SetSecuritySite(pNewISM, pSecuritySite);

    if (pbContext)
    {
        pbContextEffect = pbContext;
    }
    else
    {
        pbContextEffect = (PBYTE)&dwContextBuf;
        cbContext = sizeof(dwContextBuf);
    }

    if (pbPolicy)
    {
        pbPolicyEffect = pbPolicy;
        cbPolicyEffect = cbPolicy;
    }
    else
    {
        pbPolicyEffect = (PBYTE)&dwPolicyBuf;
        cbPolicyEffect = sizeof(dwPolicyBuf);
    }

    hr = pNewISM->lpVtbl->ProcessUrlAction(pNewISM, pszUrl, dwAction,
                                           pbPolicyEffect, cbPolicyEffect,
                                           pbContextEffect, cbContext,
                                           dwFlags, 0);

    if (pSecuritySite)
        pNewISM->lpVtbl->SetSecuritySite(pNewISM, NULL);

    pNewISM->lpVtbl->Release(pNewISM);
    return hr;
}

/*************************************************************************
 * ZoneCheckPathA [SHLWAPI.226]
 */
HRESULT WINAPI
ZoneCheckPathA(
    _In_z_   PCSTR   pszPath,
    _In_     DWORD   dwAction,
    _In_     DWORD   dwFlags,
    _In_opt_ IInternetSecurityMgrSite *pSecuritySite)
{
    WCHAR szPath[2048];
    SHAnsiToUnicode(pszPath, szPath, _countof(szPath));
    return ZoneCheckPathW(szPath, dwAction, dwFlags, pSecuritySite);
}

/*************************************************************************
 * ZoneCheckPathW [SHLWAPI.227]
 */
HRESULT WINAPI
ZoneCheckPathW(
    _In_z_   PCWSTR  pszPath,
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
    _In_z_   PCSTR   pszUrl,
    _In_     DWORD   dwAction,
    _In_     DWORD   dwFlags,
    _In_opt_ IInternetSecurityMgrSite *pSecuritySite)
{
    WCHAR szUrl[2048];
    SHAnsiToUnicode(pszUrl, szUrl, _countof(szUrl));
    return ZoneCheckUrlW(szUrl, dwAction, dwFlags, pSecuritySite);
}

/*************************************************************************
 * ZoneCheckUrlW [SHLWAPI.229]
 */
HRESULT WINAPI
ZoneCheckUrlW(
    _In_z_   PCWSTR  pszUrl,
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
    _In_z_       PCSTR   pszUrl,
    _In_opt_     PBYTE   pbPolicy,
    _In_         DWORD   cbPolicy,
    _In_opt_     PBYTE   pbContext,
    _In_         DWORD   cbContext,
    _In_         DWORD   dwAction,
    _In_         DWORD   dwFlags,
    _In_opt_     IInternetSecurityMgrSite *pSecuritySite)
{
    WCHAR szUrl[2048];
    SHAnsiToUnicode(pszUrl, szUrl, _countof(szUrl));
    return ZoneCheckUrlExW(szUrl, pbPolicy, cbPolicy, pbContext, cbContext,
                           dwAction, dwFlags, pSecuritySite);
}

/*************************************************************************
 * ZoneCheckUrlExW [SHLWAPI.231]
 */
HRESULT WINAPI
ZoneCheckUrlExW(
    _In_z_       PCWSTR  pszUrl,
    _In_opt_     PBYTE   pbPolicy,
    _In_         DWORD   cbPolicy,
    _In_opt_     PBYTE   pbContext,
    _In_         DWORD   cbContext,
    _In_         DWORD   dwAction,
    _In_         DWORD   dwFlags,
    _In_opt_     IInternetSecurityMgrSite *pSecuritySite)
{
    return ZoneCheckUrlExCacheW(pszUrl, pbPolicy, cbPolicy, pbContext, cbContext,
                                dwAction, dwFlags, pSecuritySite, NULL);
}
