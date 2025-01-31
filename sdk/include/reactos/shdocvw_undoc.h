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

#ifdef __cplusplus
} /* extern "C" */
#endif
