/*
 * PROJECT:     ReactOS header
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     SHLWAPI IShellFolder helpers
 * COPYRIGHT:   Copyright 2009 Andrew Hill <ash77 at domain reactos.org>
 *              Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* HACK! These function names are conflicting with <shobjidl.h> functions. */
#undef IShellFolder_GetDisplayNameOf
#undef IShellFolder_ParseDisplayName
#undef IShellFolder_CompareIDs

HRESULT WINAPI
IShellFolder_GetDisplayNameOf(
    _In_ IShellFolder *psf,
    _In_ LPCITEMIDLIST pidl,
    _In_ SHGDNF uFlags,
    _Out_ LPSTRRET lpName,
    _In_ DWORD dwRetryFlags);

/* Flags for IShellFolder_GetDisplayNameOf */
#define SFGDNO_RETRYWITHFORPARSING  0x00000001
#define SFGDNO_RETRYALWAYS          0x80000000

HRESULT WINAPI
IShellFolder_ParseDisplayName(
    _In_ IShellFolder *psf,
    _In_ HWND hwndOwner,
    _In_ LPBC pbcReserved,
    _In_ LPOLESTR lpszDisplayName,
    _Out_ ULONG *pchEaten,
    _Out_ PIDLIST_RELATIVE *ppidl,
    _Out_ ULONG *pdwAttributes);

EXTERN_C HRESULT WINAPI
IShellFolder_CompareIDs(
    _In_ IShellFolder *psf,
    _In_ LPARAM lParam,
    _In_ PCUIDLIST_RELATIVE pidl1,
    _In_ PCUIDLIST_RELATIVE pidl2);

#ifdef __cplusplus
} /* extern "C" */
#endif
