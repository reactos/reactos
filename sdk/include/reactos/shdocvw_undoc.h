/*
 * PROJECT:     ReactOS Headers
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     shdocvw.dll undocumented APIs
 * COPYRIGHT:   Copyright 2024-2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#include <exdisp.h> // For IShellWindows

#ifdef __cplusplus
extern "C" {
#endif

BOOL WINAPI
IEILIsEqual(
    _In_ LPCITEMIDLIST pidl1,
    _In_ LPCITEMIDLIST pidl2,
    _In_ BOOL bUnknown);

BOOL WINAPI WinList_Init(VOID);
VOID WINAPI WinList_Terminate(VOID);
HRESULT WINAPI WinList_Revoke(_In_ LONG lCookie);
IShellWindows* WINAPI WinList_GetShellWindows(_In_ BOOL bCreate);

HRESULT WINAPI
WinList_NotifyNewLocation(
    _In_ IShellWindows *pShellWindows,
    _In_ LONG lCookie,
    _In_ LPCITEMIDLIST pidl);

HRESULT WINAPI
WinList_FindFolderWindow(
    _In_ LPCITEMIDLIST pidl,
    _In_ DWORD dwUnused,
    _Out_opt_ PLONG phwnd,
    _Out_opt_ IWebBrowserApp **ppWebBrowserApp);

HRESULT WINAPI
WinList_RegisterPending(
    _In_ DWORD dwThreadId,
    _In_ LPCITEMIDLIST pidl,
    _In_ DWORD dwUnused,
    _Out_ PLONG plCookie);

/*****************************************************************************
 * IConnectionPointCB interface
 *
 * The method names are unconfirmed.
 */
#define INTERFACE IConnectionPointCB
DECLARE_INTERFACE_(IConnectionPointCB, IUnknown)
{
     /*** IUnknown ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
     /*** IConnectionPointCB ***/
    STDMETHOD(OnAdvise)(THIS_ REFIID riid, UINT nNewItemCount, DWORD dwCookie) PURE;
    STDMETHOD(OnUnadvise)(THIS_ REFIID riid, UINT nNewItemCount, DWORD dwCookie) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IConnectionPointCB_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IConnectionPointCB_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IConnectionPointCB_Release(T) (T)->lpVtbl->Release(T)
#define IConnectionPointCB_OnAdvise(T,a,b,c) (T)->lpVtbl->OnAdvise(T,a,b,c)
#define IConnectionPointCB_OnUnadvise(T,a,b,c) (T)->lpVtbl->OnUnadvise(T,a,b,c)
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif
