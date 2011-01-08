/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for GdiReleaseLocalDC
 * PROGRAMMERS:     Timo Kreuzer
 */

#include <stdio.h>
#include <wine/test.h>
#include <windows.h>

void Test_MaskBlt_1bpp()
{
    HDC hdcDst, hdcSrc;
	BITMAPINFO bmi = {{sizeof(BITMAPINFOHEADER), 8, 1, 1, 1, BI_RGB, 0, 10, 10, 0,0}};
    HBITMAP hbmDst, hbmSrc, hbmMsk;
    PUCHAR pjBitsDst, pjBitsSrc, pjBitsMsk;
    BOOL ret;

    /* Create a dest dc and bitmap */
    hdcDst = CreateCompatibleDC(NULL);
	hbmDst = CreateDIBSection(hdcDst, &bmi, DIB_RGB_COLORS, (PVOID*)&pjBitsDst, NULL, 0);
    SelectObject(hdcDst, hbmDst);

    /* Create a source dc and bitmap */
    hdcSrc = CreateCompatibleDC(NULL);
	hbmSrc = CreateDIBSection(hdcSrc, &bmi, DIB_RGB_COLORS, (PVOID*)&pjBitsSrc, NULL, 0);
    SelectObject(hdcSrc, hbmSrc);

    /* Create a 1 bpp mask bitmap */
	hbmMsk = CreateDIBSection(hdcDst, &bmi, DIB_RGB_COLORS, (PVOID*)&pjBitsMsk, NULL, 0);

    /* Do the masking */
    pjBitsDst[0] = 0xAA;
    pjBitsSrc[0] = 0xCC;
    pjBitsMsk[0] = 0xF0;
    ret = MaskBlt(hdcDst, 0, 0, 8, 1, hdcSrc, 0, 0, hbmMsk, 0, 0, MAKEROP4(SRCCOPY, 0xAA0000));
    ok(ret == 1, "MaskBlt failed (%d)\n", ret);
    ok (pjBitsDst[0] == 0xCA, "pjBitsDst[0] == 0x%x\n", pjBitsDst[0]);

    pjBitsDst[0] = 0x00;
    pjBitsSrc[0] = 0xFF;
    pjBitsMsk[0] = 0xF0;
    ret = MaskBlt(hdcDst, 0, 0, 8, 1, hdcSrc, 0, 0, hbmMsk, 0, 0, MAKEROP4(SRCCOPY, 0xAA0000));
    ok(ret == 1, "MaskBlt failed (%d)\n", ret);
    ok (pjBitsDst[0] == 0xF0, "pjBitsDst[0] == 0x%x\n", pjBitsDst[0]);



}

void Test_MaskBlt_RGBA()
{
    HDC hdcDst, hdcSrc;
	BITMAPINFO bmi1 = {{sizeof(BITMAPINFOHEADER), 8, 1, 1, 1, BI_RGB, 0, 10, 10, 0,0}};
	BITMAPINFO bmi32 = {{sizeof(BITMAPINFOHEADER), 8, 1, 1, 32, BI_RGB, 0, 10, 10, 0,0}};
    HBITMAP hbmDst, hbmSrc, hbmMsk;
    PUCHAR pjBitsDst, pjBitsSrc, pjBitsMsk;
    BOOL ret;

    /* Create a dest dc and bitmap */
    hdcDst = CreateCompatibleDC(NULL);
	hbmDst = CreateDIBSection(hdcDst, &bmi32, DIB_RGB_COLORS, (PVOID*)&pjBitsDst, NULL, 0);
    SelectObject(hdcDst, hbmDst);

    /* Create a source dc and bitmap */
    hdcSrc = CreateCompatibleDC(NULL);
	hbmSrc = CreateDIBSection(hdcSrc, &bmi32, DIB_RGB_COLORS, (PVOID*)&pjBitsSrc, NULL, 0);
    SelectObject(hdcSrc, hbmSrc);

    /* Create a 1 bpp mask bitmap */
	hbmMsk = CreateDIBSection(hdcDst, &bmi1, DIB_RGB_COLORS, (PVOID*)&pjBitsMsk, NULL, 0);

    /* Do the masking */
    pjBitsDst[0] = 0xAA;
    pjBitsSrc[0] = 0xCC;
    pjBitsMsk[0] = 0xF0;
    ret = MaskBlt(hdcDst, 0, 0, 8, 1, hdcSrc, 0, 0, hbmMsk, 0, 0, MAKEROP4(SRCCOPY, 0xAA0000));
    ok(ret == 1, "MaskBlt failed (%d)\n", ret);
    ok (pjBitsDst[0] == 0xCA, "pjBitsDst[0] == 0x%x\n", pjBitsDst[0]);

    pjBitsDst[0] = 0x00;
    pjBitsSrc[0] = 0xFF;
    pjBitsMsk[0] = 0xF0;
    ret = MaskBlt(hdcDst, 0, 0, 8, 1, hdcSrc, 0, 0, hbmMsk, 0, 0, MAKEROP4(SRCCOPY, 0xAA0000));
    ok(ret == 1, "MaskBlt failed (%d)\n", ret);
    ok (pjBitsDst[0] == 0xF0, "pjBitsDst[0] == 0x%x\n", pjBitsDst[0]);

}

START_TEST(MaskBlt)
{
    Test_MaskBlt_1bpp();
}

