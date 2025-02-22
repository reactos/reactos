/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for CreateDIBPatternBrush
 * PROGRAMMERS:     Timo Kreuzer
 */

#include "precomp.h"

#include "init.h"

/* New color use parameter. See https://www.betaarchive.com/wiki/index.php?title=Microsoft_KB_Archive/108497 */
#define DIB_PAL_INDICES 2

void Test_CreateDIBPatternBrush()
{

}

void Test_CreateDIBPatternBrushPt()
{
    struct
    {
        BITMAPINFOHEADER bmiHeader;
        WORD wColors[4];
        BYTE ajBuffer[16];
    } PackedDIB =
    {
        {sizeof(BITMAPINFOHEADER), 4, -4, 1, 8, BI_RGB, 0, 1, 1, 4, 0},
        {0, 1, 2, 7},
        {0,1,2,3,  1,2,3,0,  2,3,0,1,  3,0,1,2},
    };
    HBRUSH hbr, hbrOld;
    HPALETTE hpalOld;
    LOGBRUSH logbrush;

    SetLastError(0);
    ok_hdl(CreateDIBPatternBrushPt(NULL, 0), NULL);
    ok_hdl(CreateDIBPatternBrushPt(NULL, DIB_PAL_COLORS), NULL);
    ok_hdl(CreateDIBPatternBrushPt(NULL, 2), NULL);
    ok_hdl(CreateDIBPatternBrushPt(NULL, 3), NULL);
    ok_err(0);

    hbr = CreateDIBPatternBrushPt(&PackedDIB, 0);
    ok(hbr != 0, "Expected success\n");
    DeleteObject(hbr);
    hbr = CreateDIBPatternBrushPt(&PackedDIB, 2);
    ok(hbr != 0, "Expected success\n");
    DeleteObject(hbr);

    SetLastError(0);
    hbr = CreateDIBPatternBrushPt(&PackedDIB, 3);
    ok(hbr == 0, "Expected failure\n");
    ok_err(ERROR_INVALID_PARAMETER);
    SetLastError(0);
    hbr = CreateDIBPatternBrushPt(&PackedDIB, 10);
    ok(hbr == 0, "Expected failure\n");
    ok_err(ERROR_INVALID_PARAMETER);

    /* Create a DIB brush with palette indices */
    hbr = CreateDIBPatternBrushPt(&PackedDIB, DIB_PAL_COLORS);
    ok(hbr != 0, "CreateDIBPatternBrushPt failed, skipping tests.\n");
    if (!hbr) return;

    /* Check the logbrush */
    ok(GetObject(hbr, sizeof(logbrush), &logbrush), "GetObject() failed\n");
    ok_int(logbrush.lbStyle, BS_DIBPATTERN);
    ok_hex(logbrush.lbColor, 0);
    ok(logbrush.lbHatch == (ULONG_PTR)&PackedDIB,
       "invalid lbHatch. Got %p, expected %p\n", (PVOID)logbrush.lbHatch, &PackedDIB);

    /* Select the brush into the dc */
    hbrOld = SelectObject(ghdcDIB32, hbr);

    /* Copy it on the dib section */
    ok_long(PatBlt(ghdcDIB32, 0, 0, 4, 4, PATCOPY), 1);
    ok_long((*gpDIB32)[0][0], 0x000000); // 0
    ok_long((*gpDIB32)[0][1], 0x800000); // 1
    ok_long((*gpDIB32)[0][2], 0x008000); // 2
    ok_long((*gpDIB32)[0][3], 0xc0c0c0); // 7

    /* Select a logical palette into the DC */
    hpalOld = SelectPalette(ghdcDIB32, ghpal, FALSE);
    ok(hpalOld != 0, "Expected success, error %ld\n", GetLastError());

    /* Copy it on the dib section */
    ok_long(PatBlt(ghdcDIB32, 0, 0, 4, 4, PATCOPY), 1);
    ok_long((*gpDIB32)[0][0], 0x102030); // 0
    ok_long((*gpDIB32)[0][1], 0x203040); // 1
    ok_long((*gpDIB32)[0][2], 0x304050); // 2
    ok_long((*gpDIB32)[0][3], 0x8090a0); // 7

    /* Select back old palette and destroy the DIB data */
    SelectPalette(ghdcDIB32, hpalOld, FALSE);
    memset(gpDIB32, 0x77, sizeof(*gpDIB32));

    /* Copy it on the dib section */
    ok_long(PatBlt(ghdcDIB32, 0, 0, 4, 4, PATCOPY), 1);
    ok_long((*gpDIB32)[0][0], 0x000000); // 0
    ok_long((*gpDIB32)[0][1], 0x800000); // 1
    ok_long((*gpDIB32)[0][2], 0x008000); // 2
    ok_long((*gpDIB32)[0][3], 0xc0c0c0); // 7

    SelectObject(ghdcDIB32, hbrOld);
    DeleteObject(hbr);

    /* Set some different values */
    PackedDIB.ajBuffer[0] = 3;
    PackedDIB.ajBuffer[1] = 2;
    PackedDIB.ajBuffer[2] = 1;
    PackedDIB.ajBuffer[3] = 0;

    /* Create a DIB brush with DIB_PAL_INDICES */
    hbr = CreateDIBPatternBrushPt(&PackedDIB, DIB_PAL_INDICES);
    ok(hbr != 0, "CreateSolidBrush failed, skipping tests.\n");
    if (!hbr) return;

    /* Check the logbrush */
    ok(GetObject(hbr, sizeof(logbrush), &logbrush), "GetObject() failed\n");
    ok_int(logbrush.lbStyle, BS_DIBPATTERN);
    ok_hex(logbrush.lbColor, 0);
    ok(logbrush.lbHatch == (ULONG_PTR)&PackedDIB,
       "invalid lbHatch. Got %p, expected %p\n", (PVOID)logbrush.lbHatch, &PackedDIB);

    /* Select the brush into the dc */
    hbrOld = SelectObject(ghdcDIB32, hbr);
    ok(hbrOld != 0, "CreateSolidBrush failed, skipping tests.\n");

    /* Copy it on a dib section */
    memset(gpDIB32, 0x77, sizeof(*gpDIB32));
    ok_long(PatBlt(ghdcDIB32, 0, 0, 4, 4, PATCOPY), 1);
    ok_long((*gpDIB32)[0][0], 0x77777777);
    ok_long((*gpDIB32)[0][1], 0x77777777);
    ok_long((*gpDIB32)[0][2], 0x77777777);
    ok_long((*gpDIB32)[0][3], 0x77777777);

    /* Select a logical palette into the DC */
    hpalOld = SelectPalette(ghdcDIB32, ghpal, FALSE);
    ok(hpalOld != 0, "Expected success, error %ld\n", GetLastError());

    /* Copy it on a dib section */
    ok_long(PatBlt(ghdcDIB32, 0, 0, 4, 4, PATCOPY), 1);
    ok_long((*gpDIB32)[0][0], 0x77777777);
    ok_long((*gpDIB32)[0][1], 0x77777777);
    ok_long((*gpDIB32)[0][2], 0x77777777);
    ok_long((*gpDIB32)[0][3], 0x77777777);

    SelectPalette(ghdcDIB32, hpalOld, FALSE);
    SelectObject(ghdcDIB32, hbrOld);
    DeleteObject(hbr);

}

void Test_CreateDIBPatternBrushPt_RLE8()
{
    struct
    {
        BITMAPINFOHEADER bmiHeader;
        WORD wColors[4];
        BYTE ajBuffer[20];
    } PackedDIB =
    {
        {sizeof(BITMAPINFOHEADER), 4, 4, 1, 8, BI_RLE8, 20, 1, 1, 4, 0},
        {0, 1, 2, 7},
        {4,0,   0,2,0,1,0,2,3,1,   2,1, 2,2,   1,3,1,0,1,2, },
    };
    HBRUSH hbr;

    HDC hdc = CreateCompatibleDC(0);
    HBITMAP hbmp = CreateDIBitmap(hdc, &PackedDIB.bmiHeader, CBM_INIT, &PackedDIB.ajBuffer, (PVOID)&PackedDIB, DIB_PAL_COLORS);
    ok(hbmp != 0, "CreateDIBitmap failed, skipping tests.\n");

    /* Create a DIB brush with palette indices */
    hbr = CreateDIBPatternBrushPt(&PackedDIB, DIB_PAL_COLORS);
    ok(hbr == 0, "CreateDIBPatternBrushPt should fail.\n");


}


START_TEST(CreateDIBPatternBrush)
{
    InitStuff();

    Test_CreateDIBPatternBrush();
    Test_CreateDIBPatternBrushPt();
    //Test_CreateDIBPatternBrushPt_RLE8(); broken
}

