/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests for InternalExtractIconListW
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "shelltest.h"
#include <undocshell.h>
#include <versionhelpers.h>

START_TEST(InternalExtractIconListW)
{
    WCHAR szPath[MAX_PATH];
    GetModuleFileNameW(NULL, szPath, _countof(szPath));

    HGLOBAL hPairs = InternalExtractIconListW(GetModuleHandleW(NULL), szPath, 0);
    if (IsWindowsVistaOrGreater())
    {
        ok(hPairs == NULL, "hPairs was %p\n", hPairs);
        skip("InternalExtractIconListW on Vista+ is a stub (returns NULL)\n");
        return;
    }
    else
    {
        ok(hPairs != NULL, "hPairs was NULL\n");
    }

    UINT nIcons = (UINT)(GlobalSize(hPairs) / sizeof(ICON_AND_ID));
    ok(nIcons != 0, "nIcons was zero\n");

    PICON_AND_ID pPairs = (PICON_AND_ID)GlobalLock(hPairs);
    ok(pPairs != NULL, "pPairs was NULL\n");

    ok((pPairs && pPairs[0].hIcon != NULL), "pPairs[0].hIcon was NULL\n");
    ok((pPairs && pPairs[0].nIconID != 0), "pPairs[0].nIconID was zero\n");

    if (pPairs)
    {
        for (UINT iIcon = 0; iIcon < nIcons; ++iIcon)
        {
            DestroyIcon(pPairs[iIcon].hIcon);
        }
    }

    GlobalUnlock(hPairs);
    GlobalFree(hPairs);
}
