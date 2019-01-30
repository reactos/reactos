/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtGdiCreateCompatibleDC
 * PROGRAMMERS:
 */

#include <win32nt.h>

START_TEST(NtGdiCreateCompatibleDC)
{
    HDC hDC;
    HGDIOBJ hObj;

    /* Test if aa NULL DC is accepted */
    hDC = NtGdiCreateCompatibleDC(NULL);
    ok(hDC != NULL, "hDC was NULL.\n");

    /* We select a nwe palette. Note: SelectObject doesn't work with palettes! */
    hObj = SelectPalette(hDC, GetStockObject(DEFAULT_PALETTE), 0);
    /* The old palette should be GetStockObject(DEFAULT_PALETTE) */
    ok_ptr(hObj, GetStockObject(DEFAULT_PALETTE));

    /* The default bitmap should be GetStockObject(21) */
    hObj = SelectObject(hDC, GetStockObject(21));
    ok_ptr(hObj, GetStockObject(21));

    /* The default pen should be GetStockObject(BLACK_PEN) */
    hObj = SelectObject(hDC, GetStockObject(WHITE_PEN));
    ok_ptr(hObj, GetStockObject(BLACK_PEN));

    ok(NtGdiDeleteObjectApp(hDC) != 0, "NtGdiDeleteObjectApp(hDC) was zero.\n");
}
