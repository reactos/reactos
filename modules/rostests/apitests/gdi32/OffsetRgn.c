/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for FrameRgn
 * PROGRAMMERS:     Timo Kreuzer
 */

#include <apitest.h>
#include <windows.h>
#include <stdio.h>
#include "init.h"

void Test_OffsetRgn()
{
    HRGN hrgn1, hrgn2;
    HDC hdc;

    hrgn1 = CreateRectRgn(0, 0, 0, 0);
    ok(hrgn1 != NULL, "CreateRectRgn failed\n");
    ok_int(OffsetRgn(hrgn1, INT_MIN + 10, 10), NULLREGION);
    ok_int(OffsetRgn(hrgn1, 0xF000000, 0xF000000), NULLREGION);
    DeleteObject(hrgn1);

    hrgn1 = CreateRectRgn(0, 0, 100, 100);
    ok(hrgn1 != NULL, "CreateRectRgn failed\n");
    ok_int(OffsetRgn(hrgn1, 10, 10), SIMPLEREGION);
    ok_int(OffsetRgn(hrgn1, 0x8000000 - 110, 10), ERROR);
    ok_int(OffsetRgn(hrgn1, 0x8000000 - 111, 10), SIMPLEREGION);
    DeleteObject(hrgn1);

    hrgn1 = CreateRectRgn(0, 0, 100, 100);
    ok(hrgn1 != NULL, "CreateRectRgn failed\n");
    ok_int(OffsetRgn(hrgn1, -10, 10), SIMPLEREGION);
    ok_int(OffsetRgn(hrgn1, -(0x8000000 - 9), 10), ERROR);
    ok_int(OffsetRgn(hrgn1, -(0x8000000 - 10), 10), SIMPLEREGION);
    DeleteObject(hrgn1);

    hrgn1 = CreateRectRgn(0, 0, 10, 10);
    hrgn2 = CreateRectRgn(1000, 20, 1010, 30);
    ok_int(CombineRgn(hrgn1, hrgn1, hrgn2, RGN_OR), COMPLEXREGION);
    ok_int(OffsetRgn(hrgn1, 0x8000000 - 100, 10), ERROR);
    ok_int(CombineRgn(hrgn1, hrgn1, hrgn2, RGN_XOR), SIMPLEREGION);
    DeleteObject(hrgn2);
    hrgn2 = CreateRectRgn(0, 0, 10, 10);
    ok_int(CombineRgn(hrgn1, hrgn1, hrgn2, RGN_XOR), NULLREGION);

    hrgn1 = CreateRectRgn(0, 0, 0, 0);
    hdc = CreateCompatibleDC(NULL);
    ok_int(GetClipRgn(hdc, hrgn1), 0);
    ok_int(OffsetRgn(hrgn1, 10, 10), NULLREGION);

}

START_TEST(OffsetRgn)
{
    Test_OffsetRgn();
}

