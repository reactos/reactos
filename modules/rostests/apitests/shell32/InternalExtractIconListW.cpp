/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test for SHShouldShowWizards
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "shelltest.h"
#include <undocshell.h>
#include <versionhelpers.h>

struct ICON_AND_ID
{
    HICON hIcon;
    UINT nIconID;
};

START_TEST(InternalExtractIconListW)
{
    if (IsWindowsVistaOrGreater())
    {
        skip("Vista+\n"); // InternalExtractIconListW of Vista+ is useless
        return;
    }

    WCHAR szPath[MAX_PATH];
    GetModuleFileNameW(NULL, szPath, _countof(szPath));

    HGLOBAL hPairs = InternalExtractIconListW(GetModuleHandleW(NULL), szPath, 0);
    ok(hPairs != NULL, "hPairs was NULL\n");

    SIZE_T cIcons = GlobalSize(hPairs) / sizeof(ICON_AND_ID);
    ok(cIcons != 0, "cIcons was zero\n");

    ICON_AND_ID *pPairs = (ICON_AND_ID *)GlobalLock(hPairs);
    ok(pPairs != NULL, "pPairs was NULL\n");

    ok((pPairs && pPairs[0].hIcon != NULL), "pPairs[0].hIcon was NULL\n");
    ok((pPairs && pPairs[0].nIconID != 0), "pPairs[0].nIconID was zero\n");

    if (pPairs)
    {
        for (SIZE_T iIcon = 0; iIcon < cIcons; ++iIcon)
        {
            DestroyIcon(pPairs[iIcon].hIcon);
        }
    }

    GlobalUnlock(hPairs);
    GlobalFree(hPairs);
}
