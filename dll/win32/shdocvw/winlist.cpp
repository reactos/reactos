/*
 * PROJECT:     ReactOS shdocvw
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     CLSID_ShellWindows and WinList_* functions
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "objects.h"

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

static VARIANT s_vaEmpty = { VT_EMPTY };

static HRESULT
InitVariantFromBuffer(
    _Out_ LPVARIANTARG pvarg,
    _In_ LPCVOID pv,
    _In_ SIZE_T cb)
{
    VariantInit(pvarg);

    LPSAFEARRAY pArray = SafeArrayCreateVector(VT_UI1, 0, cb);
    if (!pArray)
    {
        ERR("!pArray\n");
        return E_OUTOFMEMORY;
    }

    V_ARRAY(pvarg) = pArray;
    V_VT(pvarg) = VT_ARRAY | VT_UI1;
    CopyMemory(pArray->pvData, pv, cb);
    return S_OK;
}

static HRESULT
InitVariantFromIDList(
    _Out_ LPVARIANTARG pvarg,
    _In_ LPCITEMIDLIST pidl)
{
    return InitVariantFromBuffer(pvarg, pidl, ILGetSize(pidl));
}

static HRESULT
VariantClearLazy(_Inout_ LPVARIANTARG pvarg)
{
    switch (V_VT(pvarg))
    {
        case VT_EMPTY:
        case VT_BOOL:
        case VT_I4:
        case VT_UI4:
            break;
        case VT_UNKNOWN:
            if (V_UNKNOWN(pvarg))
                V_UNKNOWN(pvarg)->Release();
            break;
        case VT_DISPATCH:
            if (V_DISPATCH(pvarg))
                V_DISPATCH(pvarg)->Release();
            break;
        case VT_SAFEARRAY:
            SafeArrayDestroy(V_ARRAY(pvarg));
            break;
        default:
            return VariantClear(pvarg);
    }
    V_VT(pvarg) = VT_EMPTY;
    return S_OK;
}

/*************************************************************************
 *    WinList_Init (SHDOCVW.110)
 *
 * Retired in NT 6.1.
 */
EXTERN_C
BOOL WINAPI
WinList_Init(VOID)
{
    FIXME("()\n");
    return FALSE;
}

/*************************************************************************
 *    WinList_Terminate (SHDOCVW.111)
 *
 * NT 4.71 and higher. Retired in NT 6.1.
 */
EXTERN_C
VOID WINAPI
WinList_Terminate(VOID)
{
    FIXME("()\n");
}

/*************************************************************************
 *    WinList_GetShellWindows (SHDOCVW.179)
 *
 * NT 5.0 and higher.
 * @see https://learn.microsoft.com/en-us/windows/win32/api/exdisp/nn-exdisp-ishellwindows
 */
EXTERN_C
IShellWindows* WINAPI
WinList_GetShellWindows(
    _In_ BOOL bCreate)
{
    FIXME("(%d)\n", bCreate);
    return NULL;
}

/*************************************************************************
 *    WinList_NotifyNewLocation (SHDOCVW.177)
 *
 * NT 5.0 and higher.
 * @see https://learn.microsoft.com/en-us/windows/win32/api/exdisp/nf-exdisp-ishellwindows-onnavigate
 */
EXTERN_C
HRESULT WINAPI
WinList_NotifyNewLocation(
    _In_ IShellWindows *pShellWindows,
    _In_ LONG lCookie,
    _In_ LPCITEMIDLIST pidl)
{
    TRACE("(%p, %ld, %p)\n", pShellWindows, lCookie, pidl);

    if (!pidl)
    {
        ERR("!pidl\n");
        return E_UNEXPECTED;
    }

    VARIANTARG varg;
    HRESULT hr = InitVariantFromIDList(&varg, pidl);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = pShellWindows->OnNavigate(lCookie, &varg);
    VariantClearLazy(&varg);
    return hr;
}

/*************************************************************************
 *    WinList_FindFolderWindow (SHDOCVW.178)
 *
 * NT 5.0 and higher.
 * @see https://learn.microsoft.com/en-us/windows/win32/api/exdisp/nf-exdisp-ishellwindows-findwindowsw
 */
EXTERN_C
HRESULT WINAPI
WinList_FindFolderWindow(
    _In_ LPCITEMIDLIST pidl,
    _In_ DWORD dwUnused,
    _Out_opt_ PLONG phwnd, // Stores a window handle but LONG type
    _Out_opt_ IWebBrowserApp **ppWebBrowserApp)
{
    UNREFERENCED_PARAMETER(dwUnused);

    TRACE("(%p, %ld, %p, %p)\n", pidl, dwUnused, phwnd, ppWebBrowserApp);

    if (ppWebBrowserApp)
        *ppWebBrowserApp = NULL;

    if (phwnd)
        *phwnd = 0;

    if (!pidl)
    {
        ERR("!pidl\n");
        return E_UNEXPECTED;
    }

    CComPtr<IShellWindows> pShellWindows(WinList_GetShellWindows(ppWebBrowserApp != NULL));
    if (!pShellWindows)
    {
        ERR("!pShellWindows\n");
        return E_UNEXPECTED;
    }

    VARIANTARG varg;
    HRESULT hr = InitVariantFromIDList(&varg, pidl);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    CComPtr<IDispatch> pDispatch;
    const INT options = SWFO_INCLUDEPENDING | (ppWebBrowserApp ? SWFO_NEEDDISPATCH : 0);
    hr = pShellWindows->FindWindowSW(&varg, &s_vaEmpty, SWC_BROWSER, phwnd, options, &pDispatch);
    if (pDispatch && ppWebBrowserApp)
        hr = pDispatch->QueryInterface(IID_PPV_ARG(IWebBrowserApp, ppWebBrowserApp));

    VariantClearLazy(&varg);
    return hr;
}

/*************************************************************************
 *    WinList_RegisterPending (SHDOCVW.180)
 *
 * NT 5.0 and higher.
 * @see https://learn.microsoft.com/en-us/windows/win32/api/exdisp/nf-exdisp-ishellwindows-registerpending
 */
EXTERN_C
HRESULT WINAPI
WinList_RegisterPending(
    _In_ DWORD dwThreadId,
    _In_ LPCITEMIDLIST pidl,
    _In_ DWORD dwUnused,
    _Out_ PLONG plCookie)
{
    TRACE("(%ld, %p, %ld, %p)\n", dwThreadId, pidl, dwUnused, plCookie);

    if (!pidl)
    {
        ERR("!pidl\n");
        return E_UNEXPECTED;
    }

    CComPtr<IShellWindows> pShellWindows(WinList_GetShellWindows(FALSE));
    if (!pShellWindows)
    {
        ERR("!pShellWindows\n");
        return E_UNEXPECTED;
    }

    VARIANTARG varg;
    HRESULT hr = InitVariantFromIDList(&varg, pidl);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = pShellWindows->RegisterPending(dwThreadId, &varg, &s_vaEmpty, SWC_BROWSER, plCookie);
    VariantClearLazy(&varg);
    return hr;
}

/*************************************************************************
 *    WinList_Revoke (SHDOCVW.181)
 *
 * NT 5.0 and higher.
 * @see https://learn.microsoft.com/en-us/windows/win32/api/exdisp/nf-exdisp-ishellwindows-revoke
 */
EXTERN_C
HRESULT WINAPI
WinList_Revoke(
    _In_ LONG lCookie)
{
    TRACE("(%ld)\n", lCookie);

    CComPtr<IShellWindows> pShellWindows(WinList_GetShellWindows(TRUE));
    if (!pShellWindows)
    {
        ERR("!pShellWindows\n");
        return E_FAIL;
    }

    return pShellWindows->Revoke(lCookie);
}
