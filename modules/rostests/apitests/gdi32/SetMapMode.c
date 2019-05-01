/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for SetMapMode
 * PROGRAMMERS:     Timo Kreuzer
 *                  Katayama Hirofumi MZ
 */

#include "precomp.h"

void Test_SetMapMode()
{
    HDC hDC;
    SIZE WindowExt, ViewportExt;
    ULONG ulMapMode;
    POINT pt;

    hDC = CreateCompatibleDC(NULL);
    ok(hDC != 0, "CreateCompatibleDC failed, skipping tests.\n");
    if (!hDC) return;

    GetWindowExtEx(hDC, &WindowExt);
    GetViewportExtEx(hDC, &ViewportExt);

    ulMapMode = SetMapMode(hDC, MM_ISOTROPIC);
    ok_long(ulMapMode, MM_TEXT);
    ok_long(WindowExt.cx, 1);
    ok_long(WindowExt.cy, 1);
    ok_long(ViewportExt.cx, 1);
    ok_long(ViewportExt.cy, 1);

    SetLastError(0);
    ulMapMode = SetMapMode(hDC, 0);
    ok_err(0);
    ok_long(ulMapMode, 0);

    /* Go through all valid values */
    ulMapMode = SetMapMode(hDC, 1);
    ok_long(ulMapMode, MM_ISOTROPIC);
    ulMapMode = SetMapMode(hDC, 2);
    ok_long(ulMapMode, 1);
    ulMapMode = SetMapMode(hDC, 3);
    ok_long(ulMapMode, 2);
    ulMapMode = SetMapMode(hDC, 4);
    ok_long(ulMapMode, 3);
    ulMapMode = SetMapMode(hDC, 5);
    ok_long(ulMapMode, 4);
    ulMapMode = SetMapMode(hDC, 6);
    ok_long(ulMapMode, 5);
    ulMapMode = SetMapMode(hDC, 7);
    ok_long(ulMapMode, 6);
    ulMapMode = SetMapMode(hDC, 8);
    ok_long(ulMapMode, 7);

    /* Test invalid value */
    ulMapMode = SetMapMode(hDC, 9);
    ok_long(ulMapMode, 0);
    ulMapMode = SetMapMode(hDC, 10);
    ok_long(ulMapMode, 0);

    ok_err(0);

    /* Test NULL DC */
    ulMapMode = SetMapMode((HDC)0, 2);
    ok_long(ulMapMode, 0);
    ok_err(ERROR_INVALID_PARAMETER);

    /* Test NULL DC and invalid mode */
    ulMapMode = SetMapMode((HDC)0, 10);
    ok_long(ulMapMode, 0);
    ok_err(ERROR_INVALID_PARAMETER);

    /* Test invalid DC */
    ulMapMode = SetMapMode((HDC)0x12345, 2);
    ok_long(ulMapMode, 0);
    ok_err(ERROR_INVALID_PARAMETER);

    /* Test invalid DC and invalid mode */
    ulMapMode = SetMapMode((HDC)0x12345, 10);
    ok_long(ulMapMode, 0);
    ok_err(ERROR_INVALID_PARAMETER);

    DeleteDC(hDC);

    /* Test a deleted DC */
    ulMapMode = SetMapMode(hDC, 2);
    ok_long(ulMapMode, 0);
    ok_err(ERROR_INVALID_PARAMETER);

    /* Test MM_TEXT */
    hDC = CreateCompatibleDC(NULL);
    SetMapMode(hDC, MM_TEXT);
    GetWindowExtEx(hDC, &WindowExt);
    GetViewportExtEx(hDC, &ViewportExt);
    ok_long(WindowExt.cx, 1);
    ok_long(WindowExt.cy, 1);
    ok_long(ViewportExt.cx, 1);
    ok_long(ViewportExt.cy, 1);
    DeleteDC(hDC);

    /* Test MM_ISOTROPIC */
    hDC = CreateCompatibleDC(NULL);
    SetMapMode(hDC, MM_ISOTROPIC);
    GetWindowExtEx(hDC, &WindowExt);
    GetViewportExtEx(hDC, &ViewportExt);
    //ok_long(WindowExt.cx, 3600);
    //ok_long(WindowExt.cy, 2700);
    //ok_long(ViewportExt.cx, GetDeviceCaps(GetDC(0), HORZRES) - 4);
    ok_long(ViewportExt.cy, -GetDeviceCaps(GetDC(0), VERTRES));
    DeleteDC(hDC);

    /* Test MM_ANISOTROPIC */
    hDC = CreateCompatibleDC(NULL);
    SetMapMode(hDC, MM_ANISOTROPIC);
    GetWindowExtEx(hDC, &WindowExt);
    GetViewportExtEx(hDC, &ViewportExt);
    ok_long(WindowExt.cx, 1);
    ok_long(WindowExt.cy, 1);
    ok_long(ViewportExt.cx, 1);
    ok_long(ViewportExt.cy, 1);

    /* set MM_ISOTROPIC first, the values will be kept */
    SetMapMode(hDC, MM_ISOTROPIC);
    SetMapMode(hDC, MM_ANISOTROPIC);
    GetWindowExtEx(hDC, &WindowExt);
    GetViewportExtEx(hDC, &ViewportExt);
    //ok_long(WindowExt.cx, 3600);
    //ok_long(WindowExt.cy, 2700);
    ok_long(ViewportExt.cx, GetDeviceCaps(GetDC(0), HORZRES));
    ok_long(ViewportExt.cy, -GetDeviceCaps(GetDC(0), VERTRES));
    DeleteDC(hDC);

    /* Test MM_LOMETRIC */
    hDC = CreateCompatibleDC(NULL);
    SetMapMode(hDC, MM_LOMETRIC);
    GetWindowExtEx(hDC, &WindowExt);
    GetViewportExtEx(hDC, &ViewportExt);
    //ok_long(WindowExt.cx, 3600);
    //ok_long(WindowExt.cy, 2700);
    ok_long(ViewportExt.cx, GetDeviceCaps(GetDC(0), HORZRES));
    ok_long(ViewportExt.cy, -GetDeviceCaps(GetDC(0), VERTRES));
    DeleteDC(hDC);

    /* Test MM_HIMETRIC */
    hDC = CreateCompatibleDC(NULL);
    SetMapMode(hDC, MM_HIMETRIC);
    GetWindowExtEx(hDC, &WindowExt);
    GetViewportExtEx(hDC, &ViewportExt);
    //ok_long(WindowExt.cx, 36000);
    //ok_long(WindowExt.cy, 27000);
    ok_long(ViewportExt.cx, GetDeviceCaps(GetDC(0), HORZRES));
    ok_long(ViewportExt.cy, -GetDeviceCaps(GetDC(0), VERTRES));
    DeleteDC(hDC);

    /* Test MM_LOENGLISH */
    hDC = CreateCompatibleDC(NULL);
    SetMapMode(hDC, MM_LOENGLISH);
    GetWindowExtEx(hDC, &WindowExt);
    GetViewportExtEx(hDC, &ViewportExt);
    //ok_long(WindowExt.cx, 1417);
    //ok_long(WindowExt.cy, 1063);
    ok_long(ViewportExt.cx, GetDeviceCaps(GetDC(0), HORZRES));
    ok_long(ViewportExt.cy, -GetDeviceCaps(GetDC(0), VERTRES));
    DeleteDC(hDC);

    /* Test MM_HIENGLISH */
    hDC = CreateCompatibleDC(NULL);
    SetMapMode(hDC, MM_HIENGLISH);
    GetWindowExtEx(hDC, &WindowExt);
    GetViewportExtEx(hDC, &ViewportExt);
    //ok_long(WindowExt.cx, 14173);
    //ok_long(WindowExt.cy, 10630);
    ok_long(ViewportExt.cx, GetDeviceCaps(GetDC(0), HORZRES));
    ok_long(ViewportExt.cy, -GetDeviceCaps(GetDC(0), VERTRES));
    DeleteDC(hDC);

    /* Test MM_TWIPS */
    hDC = CreateCompatibleDC(NULL);
    SetMapMode(hDC, MM_TWIPS);
    GetWindowExtEx(hDC, &WindowExt);
    GetViewportExtEx(hDC, &ViewportExt);
    //ok_long(WindowExt.cx, 20409);
    //ok_long(WindowExt.cy, 15307);
    ok_long(ViewportExt.cx, GetDeviceCaps(GetDC(0), HORZRES));
    ok_long(ViewportExt.cy, -GetDeviceCaps(GetDC(0), VERTRES));
    DeleteDC(hDC);

    //
    // Test mode and extents
    //
    hDC = CreateCompatibleDC(NULL);
    GetViewportExtEx(hDC, &ViewportExt);
    GetWindowExtEx(hDC, &WindowExt);
    ok_long(ViewportExt.cx, 1);
    ok_long(ViewportExt.cy, 1);
    ok_long(WindowExt.cx, 1);
    ok_long(WindowExt.cy, 1);

    SetMapMode(hDC, MM_ANISOTROPIC);
    GetViewportExtEx(hDC, &ViewportExt);
    GetWindowExtEx(hDC, &WindowExt);
    ok_long(ViewportExt.cx, 1);
    ok_long(ViewportExt.cy, 1);
    ok_long(WindowExt.cx, 1);
    ok_long(WindowExt.cy, 1);
    SetWindowExtEx(hDC, 200, 200, NULL);
    SetViewportExtEx(hDC, 100, 100, NULL);

    SetMapMode(hDC, MM_ANISOTROPIC);
    GetViewportExtEx(hDC, &ViewportExt);
    GetWindowExtEx(hDC, &WindowExt);
    ok_long(ViewportExt.cx, 100);
    ok_long(ViewportExt.cy, 100);
    ok_long(WindowExt.cx, 200);
    ok_long(WindowExt.cy, 200);

    SetMapMode(hDC, MM_ANISOTROPIC);
    GetViewportExtEx(hDC, &ViewportExt);
    ok_long(ViewportExt.cx, 100);
    ok_long(ViewportExt.cy, 100);
    ok_long(WindowExt.cx, 200);
    ok_long(WindowExt.cy, 200);

    SetMapMode(hDC, MM_ISOTROPIC);
    GetViewportExtEx(hDC, &ViewportExt);
    //ok_long(ViewportExt.cx, GetDeviceCaps(GetDC(0), HORZRES) - 4);
    ok_long(ViewportExt.cy, -GetDeviceCaps(GetDC(0), VERTRES));
    SetWindowExtEx(hDC, 100, 100, NULL);
    SetViewportExtEx(hDC, 100, 100, NULL);

    SetMapMode(hDC, MM_ISOTROPIC);
    GetViewportExtEx(hDC, &ViewportExt);
    ok_long(ViewportExt.cx, 100);
    ok_long(ViewportExt.cy, 100);

    SetMapMode(hDC, MM_ANISOTROPIC);
    GetViewportExtEx(hDC, &ViewportExt);
    ok_long(ViewportExt.cx, 100);
    ok_long(ViewportExt.cy, 100);

    SetMapMode(hDC, MM_TEXT);
    GetViewportExtEx(hDC, &ViewportExt);
    ok_long(ViewportExt.cx, 1);
    ok_long(ViewportExt.cy, 1);

    SetMapMode(hDC, MM_ANISOTROPIC);
    GetViewportExtEx(hDC, &ViewportExt);
    ok_long(ViewportExt.cx, 1);
    ok_long(ViewportExt.cy, 1);

    SetMapMode(hDC, MM_ISOTROPIC);
    GetViewportExtEx(hDC, &ViewportExt);
    //ok_long(ViewportExt.cx, GetDeviceCaps(GetDC(0), HORZRES) - 4);
    ok_long(ViewportExt.cy, -GetDeviceCaps(GetDC(0), VERTRES));

    SetMapMode(hDC, MM_TEXT);
    GetViewportExtEx(hDC, &ViewportExt);
    GetWindowExtEx(hDC, &WindowExt);
    ok_long(ViewportExt.cx, 1);
    ok_long(ViewportExt.cy, 1);
    ok_long(WindowExt.cx, 1);
    ok_long(WindowExt.cy, 1);
    DeleteDC(hDC);

    //
    // Test mode and GetCurrentPositionEx
    //
    hDC = CreateCompatibleDC(NULL);
    MoveToEx(hDC, 100, 100, NULL);
    SetMapMode(hDC, MM_ANISOTROPIC);
    GetCurrentPositionEx(hDC, &pt);
    ok_long(pt.x, 100);
    ok_long(pt.y, 100);
    SetMapMode(hDC, MM_TEXT);
    GetCurrentPositionEx(hDC, &pt);
    ok_long(pt.x, 100);
    ok_long(pt.y, 100);
    SetMapMode(hDC, MM_ISOTROPIC);
    GetCurrentPositionEx(hDC, &pt);
    ok_long(pt.x, 100);
    ok_long(pt.y, 100);
    DeleteDC(hDC);
}

START_TEST(SetMapMode)
{
    Test_SetMapMode();
}
