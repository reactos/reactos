/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests for SHAreIconsEqual
 * COPYRIGHT:   Copyright 2018 Mark Jansen (mark.jansen@reactos.org)
 */

#include <apitest.h>
#include <shlwapi.h>
#include "resource.h"

static BOOL (WINAPI *pSHAreIconsEqual)(HICON hIcon1, HICON hIcon2);

static const char* names[] =
{
    "16_8_black",
    "16_8_red",
    "16_32_black",
    "16_32_red",
    "32_8",
    "32_32",
};

void compare_icons_imp(int id1, int id2, BOOL expected)
{
    HICON icon1 = LoadImageA(GetModuleHandle(NULL), MAKEINTRESOURCEA(id1), IMAGE_ICON, 0, 0, 0);
    HICON icon2 = LoadImageA(GetModuleHandle(NULL), MAKEINTRESOURCEA(id2), IMAGE_ICON, 0, 0, 0);

    BOOL result = pSHAreIconsEqual(icon1, icon2);

    winetest_ok(icon1 != icon2, "Expected two different handles for %s==%s\n", names[id1-1], names[id2-1]);
    winetest_ok(result == expected, "Expected %d, got %d for %s==%s\n", expected, result, names[id1-1], names[id2-1]);

    DestroyIcon(icon1);
    DestroyIcon(icon2);
}

#define compare_icons  (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : compare_icons_imp



START_TEST(SHAreIconsEqual)
{
    HMODULE module = LoadLibraryA("shlwapi.dll");
    BOOL Continue = FALSE;
    pSHAreIconsEqual = (void*)GetProcAddress(module, MAKEINTRESOURCEA(548));
    if (!pSHAreIconsEqual)
    {
        skip("SHAreIconsEqual not exported\n");
        return;
    }

    _SEH2_TRY
    {
        pSHAreIconsEqual((HICON)IDC_APPSTARTING, (HICON)IDC_APPSTARTING);
        Continue = TRUE;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Continue = FALSE;
        trace("SHAreIconsEqual not implemented?\n");
    }
    _SEH2_END;

    if (!Continue)
    {
        return;
    }

    ok(pSHAreIconsEqual((HICON)NULL, (HICON)NULL) == FALSE, "NULL\n");
    ok(pSHAreIconsEqual((HICON)IDC_APPSTARTING, (HICON)IDC_APPSTARTING) == FALSE, "IDC_APPSTARTING\n");
    ok(pSHAreIconsEqual((HICON)IDC_ARROW, (HICON)IDC_ARROW) == FALSE, "IDC_ARROW\n");
    ok(pSHAreIconsEqual((HICON)IDC_SIZENESW, (HICON)IDC_SIZENESW) == FALSE, "IDC_SIZENESW\n");

    compare_icons(ICON_16_8_BLACK, ICON_16_8_BLACK, TRUE);
    compare_icons(ICON_16_8_BLACK, ICON_16_8_RED, FALSE);
    compare_icons(ICON_16_8_BLACK, ICON_16_32_BLACK, FALSE);
    compare_icons(ICON_16_8_BLACK, ICON_16_32_RED, FALSE);
    compare_icons(ICON_16_8_BLACK, ICON_32_8, FALSE);
    compare_icons(ICON_16_8_BLACK, ICON_32_32, FALSE);
}
