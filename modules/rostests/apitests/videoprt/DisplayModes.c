/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for display mode enumeration and switching through videoprt/Int10
 * COPYRIGHT:   Copyright 2026 Noel <beebix@schmusemail.de>
 */

#include "precomp.h"

START_TEST(DisplayModes)
{
    DEVMODEW origDm, newDm, dm;
    BOOL ret, found = FALSE;
    DWORD i;
    LONG res;
    HDC hdc;
    HBRUSH hbr;
    RECT rc = { 10, 10, 74, 74 };
    COLORREF px;

    RtlZeroMemory(&origDm, sizeof(origDm));
    origDm.dmSize = sizeof(origDm);
    ret = EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &origDm);
    ok(ret, "EnumDisplaySettingsW(ENUM_CURRENT_SETTINGS) failed\n");
    if (!ret)
    {
        skip("No current display mode\n");
        return;
    }

    for (i = 0; ; i++)
    {
        RtlZeroMemory(&dm, sizeof(dm));
        dm.dmSize = sizeof(dm);
        if (!EnumDisplaySettingsW(NULL, i, &dm))
            break;

        if (dm.dmBitsPerPel >= 16 &&
            dm.dmPelsWidth >= 640 && dm.dmPelsWidth <= 1024 &&
            dm.dmPelsHeight >= 480 && dm.dmPelsHeight <= 768 &&
            (dm.dmPelsWidth != origDm.dmPelsWidth ||
             dm.dmPelsHeight != origDm.dmPelsHeight ||
             dm.dmBitsPerPel != origDm.dmBitsPerPel))
        {
            newDm = dm;
            found = TRUE;
            break;
        }
    }

    ok(found, "No alternate display mode found (dummy display driver?)\n");
    if (!found)
    {
        skip("Cannot test mode switch\n");
        return;
    }

    newDm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
    res = ChangeDisplaySettingsW(&newDm, 0);
    ok(res == DISP_CHANGE_SUCCESSFUL,
       "ChangeDisplaySettingsW(%lux%lux%lu) returned %ld\n",
       newDm.dmPelsWidth, newDm.dmPelsHeight, newDm.dmBitsPerPel, res);
    if (res != DISP_CHANGE_SUCCESSFUL)
    {
        skip("Cannot verify switched mode\n");
        return;
    }

    hdc = GetDC(NULL);
    ok((DWORD)GetDeviceCaps(hdc, HORZRES) == newDm.dmPelsWidth &&
       (DWORD)GetDeviceCaps(hdc, VERTRES) == newDm.dmPelsHeight &&
       (DWORD)(GetDeviceCaps(hdc, BITSPIXEL) * GetDeviceCaps(hdc, PLANES)) == newDm.dmBitsPerPel,
       "GetDeviceCaps mismatch after mode switch\n");

    hbr = CreateSolidBrush(RGB(0x20, 0xA0, 0xE0));
    FillRect(hdc, &rc, hbr);
    DeleteObject(hbr);
    GdiFlush();

    px = GetPixel(hdc, 32, 32);
    ok(px != CLR_INVALID &&
       GetRValue(px) >= 0x10 && GetRValue(px) <= 0x38 &&
       GetGValue(px) >= 0x90 && GetGValue(px) <= 0xB8 &&
       GetBValue(px) >= 0xD0 && GetBValue(px) <= 0xF8,
       "Framebuffer readback mismatch: px=0x%06lX\n", (unsigned long)px);
    ReleaseDC(NULL, hdc);

    origDm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
    res = ChangeDisplaySettingsW(&origDm, 0);
    ok(res == DISP_CHANGE_SUCCESSFUL, "Failed to restore original mode: %ld\n", res);
}
