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

#if 0
BOOL
MyFrameRgn(
    HDC hdc,
    HRGN hrgn,
    HBRUSH hbr,
    INT cx,
    INT cy)
{
    HRGN hrgnTemp;

    hrgnTemp = CreateRectRgn(0, 0, 0, 0);
    if (hrgnTemp == NULL)
    {
        return FALSE;
    }

    if (CombineRgn(hrgnTemp, hrgn, NULL, RGN_COPY) == ERROR)
        goto Failure;

    if (OffsetRgn(hrgn, cx, cy) == ERROR)
        goto Failure;

    if (CombineRgn(hrgnTemp, hrgnTemp, hrgn, RGN_AND) == ERROR)
        goto Failure;

    if (OffsetRgn(hrgn, -2 * cx, 0) == ERROR)
        goto Failure;

    if (CombineRgn(hrgnTemp, hrgnTemp, hrgn, RGN_AND) == ERROR)
        goto Failure;

    if (OffsetRgn(hrgn, 0, -2 * cy) == ERROR)
        goto Failure;

    if (CombineRgn(hrgnTemp, hrgnTemp, hrgn, RGN_AND) == ERROR)
        goto Failure;

    if (OffsetRgn(hrgn, 2 * cx, 0) == ERROR)
        goto Failure;

    if (CombineRgn(hrgnTemp, hrgnTemp, hrgn, RGN_AND) == ERROR)
        goto Failure;

    if (OffsetRgn(hrgn, -cx, cy) == ERROR)
        goto Failure;

    if (CombineRgn(hrgnTemp, hrgn, hrgnTemp, RGN_DIFF) == ERROR)
        goto Failure;

    if (!FillRgn(hdc, hrgnTemp, hbr))
        goto Failure;

    DeleteObject(hrgnTemp);
    return TRUE;

Failure:
    DeleteObject(hrgnTemp);
    return FALSE;
}
#endif // 0

static
void
CheckBitmapBitsWithLine(
    ULONG Line,
    HDC hdc,
    UINT cx,
    UINT cy,
    PUCHAR pjBits,
    COLORREF *pcrColors)
{
    UINT x, y, i;

    for (y = 0; y < cy; y++)
    {
        for (x = 0; x < cy; x++)
        {
            i = y * cx + x;
            ok(GetPixel(hdc, x, y) == pcrColors[pjBits[i]],
               "Wrong pixel at (%u,%u): expected 0x%08lx, got 0x%08lx\n",
               x, y, pcrColors[pjBits[i]], GetPixel(hdc, x, y));
        }
    }
}

#define CheckBitmapBits(hdc,cx,cy,pj,pcr) \
    CheckBitmapBitsWithLine(__LINE__, hdc,cx,cy,pj,pcr)

void Test_FrameRgn()
{
    RECT rc = {0, 0, 8, 8 };
    HRGN hrgn1, hrgn2;
    BOOL bRet;
    UCHAR ajBits[64] = {
        0, 0, 0, 0, 0, 0, 0, 0, // 0000000
        0, 1, 1, 1, 1, 0, 0, 0, // 0****00
        0, 1, 2, 2, 1, 0, 0, 0, // 0*xx**0
        0, 1, 2, 2, 1, 1, 1, 0, // 0*xxx*0
        0, 1, 1, 1, 2, 2, 1, 0, // 0**xx*0
        0, 0, 0, 1, 2, 2, 1, 0, // 00****0
        0, 0, 0, 1, 1, 1, 1, 0, // 0000000
        0, 0, 0, 0, 0, 0, 0, 0  // 0000000
    };
    COLORREF acrColors[16] = {RGB(0,0,0), RGB(255,255,255), RGB(128,128,128), 0};

    FillRect(ghdcDIB32, &rc, GetStockObject(BLACK_BRUSH));

    hrgn1 = CreateRectRgn(1, 1, 5, 5);
    ok(hrgn1 != NULL, "failed to create region\n");

    hrgn2 = CreateRectRgn(3, 3, 7, 7);
    ok(hrgn1 != NULL, "failed to create region\n");

    CombineRgn(hrgn1, hrgn1, hrgn2, RGN_OR);

    bRet = FillRgn(ghdcDIB32, hrgn1, GetStockObject(GRAY_BRUSH));
    ok(bRet != 0, "FrameRgn failed\n");

    bRet = FrameRgn(ghdcDIB32, hrgn1, GetStockObject(WHITE_BRUSH), 1, 1);
    ok(bRet != 0, "FrameRgn failed\n");

    CheckBitmapBits(ghdcDIB32, 8, 8, ajBits, acrColors);

}



START_TEST(FrameRgn)
{
    InitStuff();
    Test_FrameRgn();
}

