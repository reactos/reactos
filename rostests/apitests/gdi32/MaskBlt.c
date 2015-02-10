/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for MaskBlt
 * PROGRAMMERS:     Timo Kreuzer
 */

#include <apitest.h>

#include <wingdi.h>
#include <winuser.h>
#include "init.h"

void Test_MaskBlt_1bpp()
{
    HDC hdcDst, hdcSrc;
    struct
    {
        BITMAPINFOHEADER bmiHeader;
        ULONG aulColors[2];
    } bmiData = {{sizeof(BITMAPINFOHEADER), 8, 1, 1, 1, BI_RGB, 0, 10, 10, 2,0}, {0, 0xFFFFFF}};
    PBITMAPINFO pbmi = (PBITMAPINFO)&bmiData;
    HBITMAP hbmDst, hbmSrc, hbmMsk;
    PUCHAR pjBitsDst, pjBitsSrc, pjBitsMsk;
    BOOL ret;

    /* Create a dest dc and bitmap */
    hdcDst = CreateCompatibleDC(NULL);
    hbmDst = CreateDIBSection(hdcDst, pbmi, DIB_RGB_COLORS, (PVOID*)&pjBitsDst, NULL, 0);
    SelectObject(hdcDst, hbmDst);

    /* Create a source dc and bitmap */
    hdcSrc = CreateCompatibleDC(NULL);
    hbmSrc = CreateDIBSection(hdcSrc, pbmi, DIB_RGB_COLORS, (PVOID*)&pjBitsSrc, NULL, 0);
    SelectObject(hdcSrc, hbmSrc);

    /* Create a 1 bpp mask bitmap */
    hbmMsk = CreateDIBSection(hdcDst, pbmi, DIB_RGB_COLORS, (PVOID*)&pjBitsMsk, NULL, 0);

    /* Do the masking (SRCCOPY / NOOP) */
    pjBitsDst[0] = 0xAA;
    pjBitsSrc[0] = 0xCC;
    pjBitsMsk[0] = 0xF0;
    ret = MaskBlt(hdcDst, 0, 0, 8, 1, hdcSrc, 0, 0, hbmMsk, 0, 0, MAKEROP4(SRCCOPY, 0xAA0000));
    ok(ret == 1, "MaskBlt failed (%d)\n", ret);
    ok(pjBitsDst[0] == 0xCA, "pjBitsDst[0] == 0x%x\n", pjBitsDst[0]);

    pjBitsDst[0] = 0x00;
    pjBitsSrc[0] = 0xFF;
    pjBitsMsk[0] = 0xF0;
    ret = MaskBlt(hdcDst, 0, 0, 8, 1, hdcSrc, 0, 0, hbmMsk, 0, 0, MAKEROP4(SRCCOPY, 0xAA0000));
    ok(ret == 1, "MaskBlt failed (%d)\n", ret);
    ok (pjBitsDst[0] == 0xF0, "pjBitsDst[0] == 0x%x\n", pjBitsDst[0]);

    /* Do the masking (NOTSRCERASE / SRCINVERT) */
    pjBitsDst[0] = 0xF0; // 11110000
    pjBitsSrc[0] = 0xCC; // 11001100
    pjBitsMsk[0] = 0xAA; // 10101010

    ret = MaskBlt(hdcDst, 0, 0, 8, 1, hdcSrc, 0, 0, hbmMsk, 0, 0, MAKEROP4(NOTSRCERASE, SRCINVERT)); // 22
    ok(ret == 1, "MaskBlt failed (%d)\n", ret);
    ok (pjBitsDst[0] == 0x16, "pjBitsDst[0] == 0x%x\n", pjBitsDst[0]);

    /* Do the masking (MERGEPAINT / DSxn) */
    pjBitsDst[0] = 0xF0;
    pjBitsSrc[0] = 0xCC;
    pjBitsMsk[0] = 0xAA;
    ret = MaskBlt(hdcDst, 0, 0, 8, 1, hdcSrc, 0, 0, hbmMsk, 0, 0, MAKEROP4(MERGEPAINT, 0x990000));
    ok(ret == 1, "MaskBlt failed (%d)\n", ret);
    ok (pjBitsDst[0] == 0xE3, "pjBitsDst[0] == 0x%x\n", pjBitsDst[0]);

    /* Try a ROP that needs a mask with a NULL mask bitmap handle */
    ret = MaskBlt(hdcDst, 0, 0, 8, 1, hdcSrc, 0, 0, NULL, 0, 0, MAKEROP4(SRCCOPY, 0xAA0000));
    ok(ret == 1, "MaskBlt failed (%d)\n", ret);
    ok(pjBitsDst[0] == 0xCC, "pjBitsDst[0] == 0x%x\n", pjBitsDst[0]);

    /* Try a ROP that needs a mask with an invalid mask bitmap handle */
    ret = MaskBlt(hdcDst, 0, 0, 8, 1, hdcSrc, 0, 0, (HBITMAP)0x123456, 0, 0, MAKEROP4(SRCCOPY, 0xAA0000));
    ok(ret == 0, "MaskBlt should fail, but succeeded (%d)\n", ret);

    /* Try a ROP that needs a mask with an invalid mask bitmap */
    ok(ghbmp24 != NULL, "ghbmp24 is NULL!\n");
    ret = MaskBlt(hdcDst, 0, 0, 8, 1, hdcSrc, 0, 0, ghbmp24, 0, 0, MAKEROP4(SRCCOPY, 0xAA0000));
    ok(ret == 0, "MaskBlt should fail, but succeeded (%d)\n", ret);

    /* Try a ROP that needs no mask with an invalid mask bitmap */
    ret = MaskBlt(hdcDst, 0, 0, 8, 1, hdcSrc, 0, 0, (HBITMAP)0x123456, 0, 0, MAKEROP4(SRCCOPY, SRCCOPY));
    ok(ret == 1, "MaskBlt failed (%d)\n", ret);

    /* Try (PATCOPY / NOOP) with a NULL source mask and bitmap */
    ret = MaskBlt(hdcDst, 0, 0, 8, 1, NULL, 0, 0, NULL, 0, 0, MAKEROP4(PATCOPY, 0xAA0000));
    ok(ret == 0, "MaskBlt should fail, but succeeded (%d)\n", ret);

}

void Test_MaskBlt_16bpp()
{
    HDC hdcDst, hdcSrc;
    BITMAPINFO bmi1 = {{sizeof(BITMAPINFOHEADER), 8, 1, 1, 1, BI_RGB, 0, 10, 10, 0,0}};
    BITMAPINFO bmi32 = {{sizeof(BITMAPINFOHEADER), 8, 1, 1, 16, BI_RGB, 0, 10, 10, 0,0}};
    HBITMAP hbmDst, hbmSrc, hbmMsk;
    PUCHAR pjBitsMsk;
    PUSHORT pusBitsDst, pusBitsSrc;
    BOOL ret;

    /* Create a dest dc and bitmap */
    hdcDst = CreateCompatibleDC(NULL);
    hbmDst = CreateDIBSection(hdcDst, &bmi32, DIB_RGB_COLORS, (PVOID*)&pusBitsDst, NULL, 0);
    SelectObject(hdcDst, hbmDst);

    /* Create a source dc and bitmap */
    hdcSrc = CreateCompatibleDC(NULL);
    hbmSrc = CreateDIBSection(hdcSrc, &bmi32, DIB_RGB_COLORS, (PVOID*)&pusBitsSrc, NULL, 0);
    SelectObject(hdcSrc, hbmSrc);
    ok(hdcSrc && hbmSrc, "\n");

    /* Create a 1 bpp mask bitmap */
    hbmMsk = CreateDIBSection(hdcDst, &bmi1, DIB_RGB_COLORS, (PVOID*)&pjBitsMsk, NULL, 0);
    ok(hbmMsk != 0, "CreateDIBSection failed\n");

    /* Do the masking */
    pusBitsDst[0] = 0x1234;
    pusBitsDst[1] = 0x5678;
    pusBitsSrc[0] = 0x4321;
    pusBitsSrc[1] = 0x8765;
    pjBitsMsk[0] = 0x80;
    ret = MaskBlt(hdcDst, 0, 0, 8, 1, hdcSrc, 0, 0, hbmMsk, 0, 0, MAKEROP4(SRCCOPY, 0xAA0000));
    ok(ret == 1, "MaskBlt failed (%d)\n", ret);
    ok (pusBitsDst[0] == 0x4321, "pusBitsDst[0] == 0x%x\n", pusBitsDst[0]);
    ok (pusBitsDst[1] == 0x5678, "pusBitsDst[0] == 0x%x\n", pusBitsDst[1]);

    pusBitsDst[0] = 0x1234;
    pusBitsDst[1] = 0x5678;
    ret = MaskBlt(hdcDst, 0, 0, 8, 1, hdcSrc, 0, 0, hbmMsk, 0, 0, MAKEROP4(SRCPAINT, MERGEPAINT));
    ok(ret == 1, "MaskBlt failed (%d)\n", ret);
    ok (pusBitsDst[0] == 0x5335, "pusBitsDst[0] == 0x%x\n", pusBitsDst[0]);
    ok (pusBitsDst[1] == 0x7efa, "pusBitsDst[0] == 0x%x\n", pusBitsDst[1]);
}

void Test_MaskBlt_32bpp()
{
    HDC hdcDst, hdcSrc;
    BITMAPINFO bmi1 = {{sizeof(BITMAPINFOHEADER), 8, 1, 1, 1, BI_RGB, 0, 10, 10, 0,0}};
    BITMAPINFO bmi32 = {{sizeof(BITMAPINFOHEADER), 8, 1, 1, 32, BI_RGB, 0, 10, 10, 0,0}};
    HBITMAP hbmDst, hbmSrc, hbmMsk;
    PUCHAR pjBitsMsk;
    PULONG pulBitsDst, pulBitsSrc;
    BOOL ret;

    /* Create a dest dc and bitmap */
    hdcDst = CreateCompatibleDC(NULL);
    hbmDst = CreateDIBSection(hdcDst, &bmi32, DIB_RGB_COLORS, (PVOID*)&pulBitsDst, NULL, 0);
    SelectObject(hdcDst, hbmDst);

    /* Create a source dc and bitmap */
    hdcSrc = CreateCompatibleDC(NULL);
    hbmSrc = CreateDIBSection(hdcSrc, &bmi32, DIB_RGB_COLORS, (PVOID*)&pulBitsSrc, NULL, 0);
    SelectObject(hdcSrc, hbmSrc);
    ok(hdcSrc && hbmSrc, "\n");

    /* Create a 1 bpp mask bitmap */
    hbmMsk = CreateDIBSection(hdcDst, &bmi1, DIB_RGB_COLORS, (PVOID*)&pjBitsMsk, NULL, 0);
    ok(hbmMsk != 0, "CreateDIBSection failed\n");

    /* Do the masking */
    pulBitsDst[0] = 0x12345678;
    pulBitsDst[1] = 0x9abcdef0;
    pulBitsSrc[0] = 0x87684321;
    pulBitsSrc[1] = 0x0fedcba9;
    pjBitsMsk[0] = 0x80;
    ret = MaskBlt(hdcDst, 0, 0, 8, 1, hdcSrc, 0, 0, hbmMsk, 0, 0, MAKEROP4(SRCCOPY, 0xAA0000));
    ok(ret == 1, "MaskBlt failed (%d)\n", ret);
    ok (pulBitsDst[0] == 0x87684321, "pulBitsDst[0] == 0x%lx\n", pulBitsDst[0]);
    ok (pulBitsDst[1] == 0x9abcdef0, "pulBitsDst[0] == 0x%lx\n", pulBitsDst[1]);

    pulBitsDst[0] = 0x12345678;
    pulBitsDst[1] = 0x9abcdef0;
    ret = MaskBlt(hdcDst, 0, 0, 8, 1, hdcSrc, 0, 0, hbmMsk, 0, 0, MAKEROP4(SRCPAINT, MERGEPAINT));
    ok(ret == 1, "MaskBlt failed (%d)\n", ret);
    ok (pulBitsDst[0] == 0x977c5779, "pulBitsDst[0] == 0x%lx\n", pulBitsDst[0]);
    ok (pulBitsDst[1] == 0xfabefef6, "pulBitsDst[0] == 0x%lx\n", pulBitsDst[1]);
}

START_TEST(MaskBlt)
{
    InitStuff();
    Test_MaskBlt_1bpp();
    switch (GetDeviceCaps(GetDC(NULL), BITSPIXEL))
    {
        case 16: Test_MaskBlt_16bpp(); break;
        case 32: Test_MaskBlt_32bpp(); break;
    }

}

