/*
 * PROJECT:     ReactOS shdocvw
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Utility routines
 * COPYRIGHT:   Copyright 2024 Whindmar Saksit <whindsaks@proton.me>
 */

#include "objects.h"

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

static inline INT_PTR
GetMenuItemIdByPos(HMENU hMenu, UINT Pos)
{
    MENUITEMINFOW mii;
    mii.cbSize = FIELD_OFFSET(MENUITEMINFOW, hbmpItem); /* USER32 version agnostic */
    mii.fMask = MIIM_ID;
    mii.cch = 0;
    return GetMenuItemInfoW(hMenu, Pos, TRUE, &mii) ? mii.wID : -1;
}

static inline BOOL
IsMenuSeparator(HMENU hMenu, UINT Pos)
{
    MENUITEMINFOW mii;
    mii.cbSize = FIELD_OFFSET(MENUITEMINFOW, hbmpItem); /* USER32 version agnostic */
    mii.fMask = MIIM_FTYPE;
    mii.cch = 0;
    return GetMenuItemInfoW(hMenu, Pos, TRUE, &mii) && (mii.fType & MFT_SEPARATOR);
}

EXTERN_C HRESULT
SHELL_GetIDListFromObject(IUnknown *punk, PIDLIST_ABSOLUTE *ppidl)
{
#if DLL_EXPORT_VERSION >= _WIN32_WINNT_VISTA && 0 // FIXME: SHELL32 not ready yet
    return SHGetIDListFromObject(punk, ppidl);
#else
    HRESULT hr;
    IPersistFolder2 *pf2;
    if (SUCCEEDED(hr = punk->QueryInterface(IID_PPV_ARG(IPersistFolder2, &pf2))))
    {
        hr = pf2->GetCurFolder(ppidl);
        pf2->Release();
    }
    IPersistIDList *pil;
    if (FAILED(hr) && SUCCEEDED(hr = punk->QueryInterface(IID_PPV_ARG(IPersistIDList, &pil))))
    {
        hr = pil->GetIDList(ppidl);
        pil->Release();
    }
    return hr;
#endif
}

static HRESULT SHELL_CompareAbsoluteIDs(LPARAM lParam, PCIDLIST_ABSOLUTE a, PCIDLIST_ABSOLUTE b)
{
    IShellFolder *psf;
    HRESULT hr = SHGetDesktopFolder(&psf);
    if (FAILED(hr))
        return hr;
    hr = psf->CompareIDs(lParam, a, b);
    psf->Release();
    return hr;
}

EXTERN_C BOOL
SHELL_IsEqualAbsoluteID(PCIDLIST_ABSOLUTE a, PCIDLIST_ABSOLUTE b)
{
    return !SHELL_CompareAbsoluteIDs(SHCIDS_CANONICALONLY, a, b);
}

EXTERN_C BOOL
SHELL_IsVerb(IContextMenu *pcm, UINT_PTR idCmd, LPCWSTR Verb)
{
    HRESULT hr;
    WCHAR wide[MAX_PATH];
    if (SUCCEEDED(hr = pcm->GetCommandString(idCmd, GCS_VERBW, NULL, (LPSTR)wide, _countof(wide))))
        return !lstrcmpiW(wide, Verb);

    CHAR ansi[_countof(wide)], buf[MAX_PATH];
    if (SHUnicodeToAnsi(Verb, buf, _countof(buf)))
    {
        if (SUCCEEDED(hr = pcm->GetCommandString(idCmd, GCS_VERBA, NULL, ansi, _countof(ansi))))
            return !lstrcmpiA(ansi, buf);
    }
    return FALSE;
}

static int
SHELL_FindVerbPos(IContextMenu *pcm, UINT idCmdFirst, HMENU hMenu, LPCWSTR Verb)
{
    for (UINT i = 0, c = GetMenuItemCount(hMenu); i < c; ++i)
    {
        INT_PTR id = GetMenuItemIdByPos(hMenu, i);
        if (id != -1 && SHELL_IsVerb(pcm, id - idCmdFirst, Verb))
            return i;
    }
    return -1;
}

EXTERN_C VOID
SHELL_RemoveVerb(IContextMenu *pcm, UINT idCmdFirst, HMENU hMenu, LPCWSTR Verb)
{
    int nPos = SHELL_FindVerbPos(pcm, idCmdFirst, hMenu, Verb);
    if (nPos < 0)
        return;
    int nCount = GetMenuItemCount(hMenu);
    BOOL bSepBefore = nPos && IsMenuSeparator(hMenu, nPos - 1);
    BOOL bSepAfter = IsMenuSeparator(hMenu, nPos + 1);
    if (DeleteMenu(hMenu, nPos, MF_BYPOSITION))
    {
        if ((bSepBefore && bSepAfter) || (bSepAfter && nPos == 0))
            DeleteMenu(hMenu, nPos, MF_BYPOSITION);
        else if (bSepBefore && nPos + 1 == nCount)
            DeleteMenu(hMenu, nPos - 1, MF_BYPOSITION);
    }
}

EXTERN_C BOOL
_ILIsDesktop(LPCITEMIDLIST pidl)
{
    return (pidl == NULL || pidl->mkid.cb == 0);
}

/*************************************************************************
 *      IEILIsEqual [SHDOCVW.219]
 */
EXTERN_C BOOL WINAPI
IEILIsEqual(
    _In_ LPCITEMIDLIST pidl1,
    _In_ LPCITEMIDLIST pidl2,
    _In_ BOOL bUnknown)
{
    UINT cb1 = ILGetSize(pidl1), cb2 = ILGetSize(pidl2);
    if (cb1 == cb2 && memcmp(pidl1, pidl2, cb1) == 0)
        return TRUE;

    FIXME("%p, %p\n", pidl1, pidl2);
    return FALSE;
}
