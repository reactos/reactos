/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for virtual keys
 * COPYRIGHT:   Copyright 2022 Stanislav Motylkov <x86corez@gmail.com>
 */

#include "precomp.h"

UINT MapTypes[] = {
    MAPVK_VK_TO_VSC,
    MAPVK_VSC_TO_VK,
    MAPVK_VK_TO_CHAR,
    MAPVK_VSC_TO_VK_EX,
#if (NTDDI_VERSION >= NTDDI_VISTA)
    MAPVK_VK_TO_VSC_EX,
#endif
};

struct
{
    UINT VirtKey;
    UINT ScanToVirt;
    UINT ScanCode;
} TestCodes[] = {
    {VK_TAB, 0, 15},
    {VK_RETURN, 0, 28},
    {VK_CONTROL, 0, 29},
    {VK_LCONTROL, VK_CONTROL, 29},
    {VK_RCONTROL, VK_CONTROL, 29},
    {VK_MENU, 0, 56},
    {VK_SPACE, 0, 57},
};

static void testMapVirtualKey()
{
    INT i;
    UINT vCode, vExpect = 0;

    /* Make sure MapVirtualKeyW returns 0 in all cases when uCode == 0 */
    for (i = 0; i < _countof(MapTypes); i++)
    {
        vCode = MapVirtualKeyW(0, MapTypes[i]);
        ok(vCode == vExpect, "[%d] Returned %u, expected %u\n", i, vCode, vExpect);
    }

    /* Test matching between virtual keys and scan codes */
    for (i = 0; i < _countof(TestCodes); i++)
    {
        vCode = MapVirtualKeyW(TestCodes[i].VirtKey, MAPVK_VK_TO_VSC);
        vExpect = TestCodes[i].ScanCode;
        ok(vCode == vExpect, "[%d] ScanCode = %u, expected %u\n", i, vCode, vExpect);

        vCode = MapVirtualKeyW(TestCodes[i].ScanCode, MAPVK_VSC_TO_VK);
        vExpect = (TestCodes[i].ScanToVirt == 0 ? TestCodes[i].VirtKey : TestCodes[i].ScanToVirt);
        ok(vCode == vExpect, "[%d] VirtKey = %u, expected %u\n", i, vCode, vExpect);
    }
}

START_TEST(VirtualKey)
{
    testMapVirtualKey();
}
