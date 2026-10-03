/*
 * PROJECT:     ReactOS shdocvw
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Utility routines
 * COPYRIGHT:   Copyright 2024 Whindmar Saksit <whindsaks@proton.me>
 *              Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "objects.h"
#include <strsafe.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

static inline INT_PTR
GetMenuItemIdByPos(HMENU hMenu, UINT Pos)
{
    MENUITEMINFOW mii;
    mii.cbSize = FIELD_OFFSET(MENUITEMINFOW, hbmpItem); /* USER32 version agnostic */
    mii.fMask = MIIM_ID; /* GetMenuItemID does not handle sub-menus, this does */
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

static VOID
SHDOCVW_PathDeleteInvalidChars(LPWSTR pszDisplayName)
{
#define PATH_VALID_ELEMENT ( \
    PATH_CHAR_CLASS_DOT | PATH_CHAR_CLASS_SEMICOLON | PATH_CHAR_CLASS_COMMA | \
    PATH_CHAR_CLASS_SPACE | PATH_CHAR_CLASS_OTHER_VALID \
)
    PWCHAR pch, pchSrc;
    for (pch = pchSrc = pszDisplayName; *pchSrc; ++pchSrc)
    {
        if (PathIsValidCharW(*pchSrc, PATH_VALID_ELEMENT))
            *pch++ = *pchSrc;
    }
    *pch = UNICODE_NULL;
}

static HRESULT
SHDOCVW_CreateShortcut(
    _In_ LPCWSTR pszLnkFileName, 
    _In_ PCIDLIST_ABSOLUTE pidlTarget,
    _In_opt_ LPCWSTR pszDescription)
{
    HRESULT hr;

    CComPtr<IShellLink> psl;
    hr = CoCreateInstance(CLSID_ShellLink, NULL,  CLSCTX_INPROC_SERVER,
                          IID_PPV_ARG(IShellLink, &psl));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    psl->SetIDList(pidlTarget);

    if (pszDescription)
        psl->SetDescription(pszDescription);

    CComPtr<IPersistFile> ppf;
    hr = psl->QueryInterface(IID_PPV_ARG(IPersistFile, &ppf));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return ppf->Save(pszLnkFileName, TRUE);
}

struct ADDFAVORITEDIALOGDATA
{
    PWSTR pszName;
    PCWSTR pszCaption;
    WNDPROC pOrgProc;
    HWND hEdit;
    BOOL IgnoreChanges;
};

static LRESULT AddFavoriteDialogProc(
   _In_ HWND hwnd,
   _In_ UINT uMsg,
   _In_ WPARAM wParam,
   _In_ LPARAM lParam)
{
    ADDFAVORITEDIALOGDATA &data = *(ADDFAVORITEDIALOGDATA*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    NMHDR *pHdr = (NMHDR*)lParam;
    if (uMsg == WM_NOTIFY)
        data.IgnoreChanges |= pHdr->code == NM_CLICK || pHdr->code == NM_SETFOCUS; // Ignore changes from the tree
    if (uMsg == WM_COMMAND && HIWORD(wParam) == EN_SETFOCUS)
        data.IgnoreChanges = (HWND)lParam != data.hEdit; // Ignore changes in the tree rename edit
    if (uMsg == WM_COMMAND && HIWORD(wParam) == EN_UPDATE && (HWND)lParam == data.hEdit && !data.IgnoreChanges)
    {
        SendMessageW(data.hEdit, WM_GETTEXT, MAX_PATH, (LPARAM)data.pszName); // Save user input
        BOOL bad = FALSE;
        for (PWSTR p = data.pszName; *p; ++p)
            bad |= !PathIsValidCharW(*p, PATH_VALID_ELEMENT);
        SendMessageW(hwnd, BFFM_ENABLEOK, 0, !bad);
    }
    return CallWindowProcW(data.pOrgProc, hwnd, uMsg, wParam, lParam);
}

static int CALLBACK AddFavoriteDialogCallback(
   _In_ HWND hwnd,
   _In_ UINT uMsg,
   _In_ LPARAM lParam,
   _In_ LPARAM lpData)
{
    ADDFAVORITEDIALOGDATA &data = *(ADDFAVORITEDIALOGDATA*)lpData;
    switch (uMsg)
    {
    case BFFM_INITIALIZED:
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)&data);
        data.pOrgProc = (WNDPROC)SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)AddFavoriteDialogProc);
        data.hEdit = FindWindowExW(hwnd, NULL, L"EDIT", NULL);
        SendMessageW(hwnd, WM_SETTEXT, 0, (LPARAM)data.pszCaption);
        SendMessageW(hwnd, WM_NEXTDLGCTL, (WPARAM)data.hEdit, TRUE);
        SendMessageW(data.hEdit, WM_SETTEXT, 0, (LPARAM)data.pszName); // Initialize
        break;
    case BFFM_SELCHANGED:
        SendMessageW(data.hEdit, WM_SETTEXT, 0, (LPARAM)data.pszName); // Restore
        break;
    case BFFM_VALIDATEFAILED:
        return TRUE;
    }
    return 0;
}

static HRESULT
AddFavoriteDialog(
    _In_ HWND hwnd,
    _Inout_ PWSTR pszDir,
    _Inout_ PWSTR pszTitle,
    _In_ PCWSTR pszText)
{
    CComHeapPtr<ITEMIDLIST> pidlBaseDir, pidl;
    HRESULT hr = SHParseDisplayName(pszDir, NULL, &pidlBaseDir, 0, NULL);
    if (FAILED(hr))
        return hr;

    ADDFAVORITEDIALOGDATA data = { pszTitle, PathFindFileNameW(pszDir) };
    UINT bif = BIF_RETURNONLYFSDIRS | BIF_EDITBOX | BIF_USENEWUI;
    WCHAR szBuf[MAX_PATH];
    *szBuf = UNICODE_NULL;
    BROWSEINFOW info = { hwnd, pidlBaseDir, szBuf, pszText, bif, AddFavoriteDialogCallback, (LPARAM)&data };
    PIDLIST_ABSOLUTE pidlResult = SHBrowseForFolderW(&info);
    if (!pidlResult)
        return S_FALSE;
    pidl.Attach(pidlResult);
    return SHGetPathFromIDListW(pidl, pszDir) ? S_OK : E_FAIL;
}

/*************************************************************************
 *      AddUrlToFavorites [SHDOCVW.106]
 */
EXTERN_C HRESULT WINAPI
AddUrlToFavorites(
    _In_ HWND hwnd,
    _In_ LPCWSTR pszUrlW,
    _In_opt_ LPCWSTR pszTitleW,
    _In_ BOOL fDisplayUI)
{
    TRACE("%p, %s, %s, %d\n", hwnd, wine_dbgstr_w(pszUrlW), wine_dbgstr_w(pszTitleW), fDisplayUI);

    if (!pszUrlW || !pszTitleW)
        return E_INVALIDARG;

    CCoInit CoInit;
    HRESULT hr = S_OK;
    BOOL IsUrl = PathIsURLW(pszUrlW);
    LPCWSTR pszExt = IsUrl ? L".url" : L".lnk";
    WCHAR szTitle[MAX_PATH];
    StringCchCopyW(szTitle, _countof(szTitle), pszTitleW);

    WCHAR szDir[MAX_PATH];
    if (!SHGetSpecialFolderPathW(hwnd, szDir, CSIDL_FAVORITES, TRUE))
        return E_FAIL;

    if (fDisplayUI)
    {
        PCWSTR pszText = IsUrl ? pszUrlW : PathFindFileNameW(pszUrlW);
        if ((hr = AddFavoriteDialog(hwnd, szDir, szTitle, pszText)) == S_FALSE)
            return hr;
    }
    SHDOCVW_PathDeleteInvalidChars(szTitle);
    
    WCHAR szLnk[MAX_PATH * 2];
    if (SUCCEEDED(hr))
        hr = StringCchPrintfW(szLnk, _countof(szLnk), L"%s\\%s%s", szDir, szTitle, pszExt);

    if (IsUrl && SUCCEEDED(hr))
    {
        hr = WritePrivateProfileStringW(L"InternetShortcut", L"URL", pszUrlW, szLnk) ? S_OK : E_FAIL;
    }
    else if (SUCCEEDED(hr))
    {
        BOOL bMustCopy;
        if (!fDisplayUI)
            hr = SHGetNewLinkInfoW(pszUrlW, szDir, szLnk, &bMustCopy, 0) ? S_OK : E_FAIL;

        CComHeapPtr<ITEMIDLIST> pidl;
        if (!FAILED_UNEXPECTEDLY(hr))
            hr = SHParseDisplayName(pszUrlW, NULL, &pidl, 0, NULL);
        if (!FAILED_UNEXPECTEDLY(hr))
            hr = SHDOCVW_CreateShortcut(szLnk, pidl, NULL);
    }

    if (FAILED(hr) && fDisplayUI)
        SHELL_ErrorBox(hwnd, hr);
    return hr;
}
