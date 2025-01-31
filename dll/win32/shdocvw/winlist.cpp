/*
 * PROJECT:     ReactOS shdocvw
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     WinList_* functions
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "objects.h"

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

/*************************************************************************
 *    WinList_Init (SHDOCVW.110)
 *
 * Retired in NT 6.1.
 */
EXTERN_C
BOOL WINAPI
WinList_Init(VOID)
{
    FIXME("\n");
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
    FIXME("\n");
}

/*************************************************************************
 *    WinList_GetShellWindows (SHDOCVW.179)
 *
 * NT 5.0 and higher.
 */
EXTERN_C
IShellWindows* WINAPI
WinList_GetShellWindows(
    _In_ BOOL bCreate)
{
    FIXME("\n");
    return NULL;
}

/*************************************************************************
 *    WinList_NotifyNewLocation (SHDOCVW.177)
 *
 * NT 5.0 and higher.
 */
EXTERN_C
HRESULT WINAPI
WinList_NotifyNewLocation(
    _In_ IShellWindows *pShellWindows,
    _In_ LONG lCookie,
    _In_ LPCITEMIDLIST pidl)
{
    FIXME("\n");
    return E_NOTIMPL;
}

/*************************************************************************
 *    WinList_FindFolderWindow (SHDOCVW.178)
 *
 * NT 5.0 and higher.
 */
EXTERN_C
HRESULT WINAPI
WinList_FindFolderWindow(
    _In_ LPCITEMIDLIST pidl,
    _In_ DWORD dwUnused,
    _Out_ PINT pnClass,
    _Out_ PVOID *ppvObj)
{
    UNREFERENCED_PARAMETER(dwUnused);
    FIXME("\n");
    return E_NOTIMPL;
}

/*************************************************************************
 *    WinList_RegisterPending (SHDOCVW.180)
 *
 * NT 5.0 and higher.
 */
EXTERN_C
HRESULT WINAPI
WinList_RegisterPending(
    _In_ DWORD dwThreadId,
    _In_ LPCITEMIDLIST pidl,
    _In_ DWORD dwUnused,
    _Out_ PLONG plCookie)
{
    FIXME("\n");
    return E_NOTIMPL;
}

/*************************************************************************
 *    WinList_Revoke (SHDOCVW.181)
 *
 * NT 5.0 and higher.
 */
EXTERN_C
HRESULT WINAPI
WinList_Revoke(
    _In_ LONG lCookie)
{
    FIXME("\n");
    return E_NOTIMPL;
}
