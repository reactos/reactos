/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Implement shell light-weight utility functions
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#define _ATL_NO_EXCEPTIONS
#include "precomp.h"
#include <shellapi.h>
#include <shlwapi.h>
#include <shlwapi_undoc.h>
#include <shlobj_undoc.h>
#include <shlguid_undoc.h>
#include <atlstr.h>
#include <strsafe.h>

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/*************************************************************************
 * IContextMenu_Invoke [SHLWAPI.207]
 *
 * Used by Win:SHELL32!CISFBand::_TrySimpleInvoke.
 */
EXTERN_C
BOOL WINAPI
IContextMenu_Invoke(
    _In_ IContextMenu *pContextMenu,
    _In_ HWND hwnd,
    _In_ LPCSTR lpVerb,
    _In_ UINT uFlags)
{
    CMINVOKECOMMANDINFO info;
    BOOL ret = FALSE;
    INT iDefItem = 0;
    HMENU hMenu = NULL;
    HCURSOR hOldCursor;

    TRACE("(%p, %p, %s, %u)\n", pContextMenu, hwnd, debugstr_a(lpVerb), uFlags);

    if (!pContextMenu)
        return FALSE;

    hOldCursor = SetCursor(LoadCursorW(NULL, (LPCWSTR)IDC_WAIT));

    ZeroMemory(&info, sizeof(info));
    info.cbSize = sizeof(info);
    info.hwnd = hwnd;
    info.nShow = SW_NORMAL;
    info.lpVerb = lpVerb;

    if (IS_INTRESOURCE(lpVerb))
    {
        hMenu = CreatePopupMenu();
        if (hMenu)
        {
            pContextMenu->QueryContextMenu(hMenu, 0, 1, MAXSHORT, uFlags | CMF_DEFAULTONLY);
            iDefItem = GetMenuDefaultItem(hMenu, 0, 0);
            if (iDefItem != -1)
                info.lpVerb = MAKEINTRESOURCEA(iDefItem - 1);
        }
    }

    if (iDefItem != -1 || info.lpVerb)
    {
        if (!hwnd)
            info.fMask |= CMIC_MASK_FLAG_NO_UI;
        ret = SUCCEEDED(pContextMenu->InvokeCommand(&info));
    }

    /* Invoking itself doesn't need the menu object, but getting the command info
       needs the menu. */
    if (hMenu)
        DestroyMenu(hMenu);

    SetCursor(hOldCursor);

    return ret;
}

/*************************************************************************
 * PathAddDefExtW [Internal]
 *
 * @param pszPath The path string.
 * @param dwFlags The PADE_... flags.
 * @param pdwAttrs A pointer to the file attributes. Optional.
 * @return TRUE if successful.
 */
static BOOL
PathAddDefExtW(
    _Inout_ LPWSTR pszPath,
    _In_ DWORD dwFlags,
    _Out_opt_ LPDWORD pdwAttrs)
{
    INT cchPath = lstrlenW(pszPath);
    if (cchPath + 4 + 1 > MAX_PATH) // ".ext" is 4 letters, and then a NUL
        return FALSE;

    LPWSTR pch = &pszPath[cchPath];
    INT_PTR cchFileTitle = pch - PathFindFileNameW(pszPath);

    WIN32_FIND_DATAW FindData;
    StringCchCatW(pszPath, MAX_PATH, L".*");
    HANDLE hFind = FindFirstFileW(pszPath, &FindData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        *pch = UNICODE_NULL;
        return FALSE;
    }

    DWORD dwAttrs = INVALID_FILE_ATTRIBUTES;
    static const LPCWSTR s_DotExts[] =
    {
        L".pif", L".com", L".exe", L".bat", L".lnk", L".cmd", L"", NULL
    };

    BOOL ret = FALSE;
    do
    {
        for (SIZE_T iExt = 0, nBits = dwFlags; s_DotExts[iExt]; ++iExt)
        {
            if ((nBits & 1) || iExt == 6) // 6 --> L""
            {
                if (lstrcmpiW(&FindData.cFileName[cchFileTitle], s_DotExts[iExt]) == 0)
                {
                    dwAttrs = FindData.dwFileAttributes;
                    *pch = UNICODE_NULL;
                    StringCchCatW(pszPath, MAX_PATH, s_DotExts[iExt]);
                    ret = TRUE;
                    break;
                }
            }
            nBits >>= 1;
        }
    } while (!ret && FindNextFileW(hFind, &FindData));

    FindClose(hFind);

    if (pdwAttrs)
        *pdwAttrs = dwAttrs;

    if (!ret)
        *pch = UNICODE_NULL;

    return ret;
}

/*************************************************************************
 * PathFileExistsDefExtAndAttributesW [SHLWAPI.511]
 *
 * @param pszPath The path string.
 * @param dwFlags The PADE_... flags.
 * @param pdwFileAttributes A pointer to the file attributes. Optional.
 * @return TRUE if successful.
 */
BOOL WINAPI
PathFileExistsDefExtAndAttributesW(
    _Inout_ LPWSTR pszPath,
    _In_ DWORD dwFlags,
    _Out_opt_ LPDWORD pdwFileAttributes)
{
    TRACE("(%s, 0x%lX, %p)\n", debugstr_w(pszPath), dwFlags, pdwFileAttributes);

    if (pdwFileAttributes)
        *pdwFileAttributes = INVALID_FILE_ATTRIBUTES;

    if (!pszPath)
        return FALSE;

    if (!dwFlags || (*PathFindExtensionW(pszPath) && (dwFlags & PADE_OPTIONAL)))
        return PathFileExistsAndAttributesW(pszPath, pdwFileAttributes);

    return PathAddDefExtW(pszPath, dwFlags, pdwFileAttributes);
}
