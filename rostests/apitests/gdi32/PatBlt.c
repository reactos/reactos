/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for ...
 * PROGRAMMERS:     Timo Kreuzer
 */

#include <stdio.h>
#include <wine/test.h>
#include <windows.h>

HBITMAP ghbmpTarget;
PULONG gpulTargetBits;
HDC hdcTarget;

void Test_BrushOrigin()
{
    ULONG aulBits[2] = {0x5555AAAA, 0};
    HBITMAP hbmp;
    HBRUSH hbr;
    BOOL ret;

    hbmp = CreateBitmap(2, 2, 1, 1, aulBits);
    if (!hbmp)
    {
        printf("Couln not create a bitmap\n");
        return;
    }

    hbr = CreatePatternBrush(hbmp);
    if (!hbr)
    {
        printf("Couln not create a bitmap\n");
        return;
    }

    if (!SelectObject(hdcTarget, hbr))
    {
        printf("failed to select pattern brush\n");
        return;
    }

    ret = PatBlt(hdcTarget, 0, 0, 2, 2, PATCOPY);
    ok_long(ret, 1);
    ok_long(gpulTargetBits[0], 0xffffff);
    ok_long(gpulTargetBits[1], 0);
    ok_long(gpulTargetBits[16], 0);
    ok_long(gpulTargetBits[17], 0xffffff);
    //printf("0x%lx, 0x%lx\n", gpulTargetBits[0], gpulTargetBits[1]);

    ret = PatBlt(hdcTarget, 1, 0, 2, 2, PATCOPY);
    ok_long(ret, 1);
    ok_long(gpulTargetBits[0], 0xffffff);
    ok_long(gpulTargetBits[1], 0);
    ok_long(gpulTargetBits[2], 0xffffff);
    ok_long(gpulTargetBits[16], 0);
    ok_long(gpulTargetBits[17], 0xffffff);
    ok_long(gpulTargetBits[18], 0);

}

START_TEST(PatBlt)
{
    BITMAPINFO bmi;

    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = 16;
    bmi.bmiHeader.biHeight = -16;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = 0;
    bmi.bmiHeader.biXPelsPerMeter = 1;
    bmi.bmiHeader.biYPelsPerMeter = 1;
    bmi.bmiHeader.biClrUsed = 0;
    bmi.bmiHeader.biClrImportant = 0;
    ghbmpTarget = CreateDIBSection(NULL,
                                   &bmi,
                                   DIB_RGB_COLORS,
                                   (PVOID*)&gpulTargetBits,
                                   NULL,
                                   0);
    if (!ghbmpTarget)
    {
        printf("Couln not create target bitmap\n");
        return;
    }

    hdcTarget = CreateCompatibleDC(0);
    if (!hdcTarget)
    {
        printf("Couln not create target dc\n");
        return;
    }


    if (!SelectObject(hdcTarget, ghbmpTarget))
    {
        printf("Failed to select bitmap\n");
        return;
    }

    Test_BrushOrigin();
}

