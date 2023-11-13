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
