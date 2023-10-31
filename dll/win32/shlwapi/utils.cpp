/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Implement shell property bags
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#define _ATL_NO_EXCEPTIONS
#include "precomp.h"
#include <shellapi.h>
#include <shlwapi.h>
#include <shlwapi_undoc.h>
#include <shlobj_undoc.h>
#include <shlguid_undoc.h>
#include <atlstr.h>         // for CStringW
#include <strsafe.h>        // for StringC... functions

WINE_DEFAULT_DEBUG_CHANNEL(shell);

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

    if (!pContextMenu)
        return FALSE;

    hOldCursor = SetCursor(LoadCursorW(0, (LPCWSTR)IDC_WAIT));

    ZeroMemory(&info, sizeof(info));
    info.cbSize = sizeof(CMINVOKECOMMANDINFO);
    info.hwnd = hwnd;
    info.nShow = SW_NORMAL;
    info.lpVerb = lpVerb;

    if (IS_INTRESOURCE(lpVerb))
    {
        hMenu = CreatePopupMenu();
        if (hMenu)
        {
            pContextMenu->QueryContextMenu(hMenu, 0, 1, 0x7FFF, uFlags | CMF_DEFAULTONLY);
            iDefItem = GetMenuDefaultItem(hMenu, 0, 0);
            if (iDefItem != -1)
                info.lpVerb = MAKEINTRESOURCEA(iDefItem - 1);
        }
    }

    SetCursor(hOldCursor);

    if (iDefItem != -1 || info.lpVerb)
    {
        if (!hwnd)
            info.fMask |= CMIC_MASK_FLAG_NO_UI;
        pContextMenu->InvokeCommand(&info);
        ret = TRUE;
    }

    if (hMenu)
        DestroyMenu(hMenu);

    return ret;
}
