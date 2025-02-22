/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for ExcludeClipRect
 * PROGRAMMERS:     Timo Kreuzer
 */

#include "precomp.h"

void Test_ExcludeClipRect()
{
    HDC hdc;
    HRGN hrgn, hrgn2;

    hdc = CreateCompatibleDC(NULL);
    ok(hdc != 0, "CreateCompatibleDC failed, skipping tests.\n");
    if (!hdc) return;

    hrgn2 = CreateRectRgn(0, 0, 0, 0);

    /* Test NULL DC */
    SetLastError(0x12345);
    ok_int(ExcludeClipRect(NULL, 0, 0, 0, 0), ERROR);
    ok_int(GetLastError(), ERROR_INVALID_HANDLE);

    /* Test invalid DC */
    SetLastError(0x12345);
    ok_int(ExcludeClipRect((HDC)(ULONG_PTR)0x12345, 0, 0, 0, 0), ERROR);
    ok_int(GetLastError(), ERROR_INVALID_HANDLE);
    SetLastError(0x12345);

    /* Set a clip region */
    hrgn = CreateRectRgn(10, 10, 20, 30);
    ok_int(SelectClipRgn(hdc, hrgn), NULLREGION); // yeah... it's NULLREGION
    ok_int(GetRandomRgn(hdc, hrgn2, CLIPRGN), 1);
    ok_int(EqualRgn(hrgn, hrgn2), TRUE); // but in fact it's the region we set

    /* Exclude something outside of the clip region */
    ok_int(ExcludeClipRect(hdc, 0, 0, 1, 1), COMPLEXREGION); // in reality it's a rect region
    ok_int(GetRandomRgn(hdc, hrgn2, CLIPRGN), 1);
    ok_int(EqualRgn(hrgn, hrgn2), TRUE);

    /* Exclude something on one side of the clip rect */
    ok_int(ExcludeClipRect(hdc, 0, 0, 13, 50), COMPLEXREGION);
    ok_int(GetRandomRgn(hdc, hrgn2, CLIPRGN), 1);
    ok_int(CombineRgn(hrgn, hrgn2, NULL, RGN_COPY), SIMPLEREGION);

    /* Exclude something on the edge of the clip rect */
    ok_int(ExcludeClipRect(hdc, 0, 0, 15, 15), COMPLEXREGION);
    ok_int(GetRandomRgn(hdc, hrgn2, CLIPRGN), 1);
    ok_int(CombineRgn(hrgn, hrgn2, NULL, RGN_COPY), COMPLEXREGION);

    /* Exclude everything left */
    ok_int(ExcludeClipRect(hdc, 0, 0, 100, 100), NULLREGION);
    ok_int(GetRandomRgn(hdc, hrgn2, CLIPRGN), 1);
    ok_int(CombineRgn(hrgn, hrgn2, NULL, RGN_COPY), NULLREGION);

    /* Reset the clip region */
    ok_int(SelectClipRgn(hdc, NULL), SIMPLEREGION); // makes sense, it's actually the whole region
    ok_int(GetRandomRgn(hdc, hrgn2, CLIPRGN), 0); // return value says region is NULL
    ok_int(ExcludeClipRect(hdc, 0, 0, 1, 1), NULLREGION);
    ok_int(GetRandomRgn(hdc, hrgn2, CLIPRGN), 1); // but now we have a region
    ok_int(CombineRgn(hrgn, hrgn2, NULL, RGN_COPY), NULLREGION); // but it's a NULLREGION (aka empty)?

    /* Test negative rect */
    ok_int(SelectClipRgn(hdc, NULL), SIMPLEREGION);
    ok_int(ExcludeClipRect(hdc, -10, -10, 0, 0), COMPLEXREGION); // this time it's a complex region?
    ok_int(GetRandomRgn(hdc, hrgn2, CLIPRGN), 1);
    hrgn = CreateRectRgn(0, 0, 1, 1);
    ok_int(CombineRgn(hrgn2, hrgn2, hrgn, RGN_XOR), NULLREGION);

    /* Test rect with high coordinates */
    ok_int(SelectClipRgn(hdc, NULL), SIMPLEREGION);
    ok_int(ExcludeClipRect(hdc, 100000, 100000, 100010, 100010), COMPLEXREGION); // this time it's a complex region?
    ok_int(GetRandomRgn(hdc, hrgn2, CLIPRGN), 1);
    hrgn = CreateRectRgn(0, 0, 1, 1);
    ok_int(EqualRgn(hrgn, hrgn2), TRUE);
    DeleteObject(hrgn);

    /* Test reversed rect negative, but still above 0 */
    ok_int(SelectClipRgn(hdc, NULL), SIMPLEREGION);
    ok_int(ExcludeClipRect(hdc, 1, 1, -10, -20), NULLREGION);
    ok_int(GetRandomRgn(hdc, hrgn2, CLIPRGN), 1);
    hrgn = CreateRectRgn(0, 0, 0, 0);
    ok_int(CombineRgn(hrgn, hrgn2, NULL, RGN_COPY), NULLREGION);

    ok_int(GetLastError(), 0x12345);

}


START_TEST(ExcludeClipRect)
{
    Test_ExcludeClipRect();
}
