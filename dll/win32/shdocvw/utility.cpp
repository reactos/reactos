/*
 * PROJECT:     ReactOS shdocvw
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Utility routines
 * COPYRIGHT:   Copyright 2024 Whindmar Saksit <whindsaks@proton.me>
 */

#include "objects.h"

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

#ifndef SHCIDS_CANONICALONLY
#define SHCIDS_CANONICALONLY 0x10000000L
#endif

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
