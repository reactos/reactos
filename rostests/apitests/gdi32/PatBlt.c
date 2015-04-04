/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for ...
 * PROGRAMMERS:     Timo Kreuzer
 */

#include <apitest.h>

#include <stdio.h>
#include <wingdi.h>

HBITMAP ghbmpTarget;
PULONG gpulTargetBits;
HDC hdcTarget;

void Test_PatBlt_Params()
{
    BOOL ret;
    ULONG i, rop;
    HDC hdc;

    /* Test a rop that contains only the operation index */
    ret = PatBlt(hdcTarget, 0, 0, 1, 1, PATCOPY & 0x00FF0000);
    ok_long(ret, 1);

    /* Test a rop that contains arbitrary values outside the operation index */
    ret = PatBlt(hdcTarget, 0, 0, 1, 1, (PATCOPY & 0x00FF0000) | 0xab00cdef);
    ok_long(ret, 1);

    /* Test an invalid rop  */
    SetLastError(0);
    ok_long(PatBlt(hdcTarget, 0, 0, 1, 1, SRCCOPY) , 0);
    ok_err(0);

    /* Test all rops */
    for (i = 0; i < 256; i++)
    {
        rop = i << 16;
        ret = PatBlt(hdcTarget, 0, 0, 1, 1, rop);

        /* Only these should succeed (they use no source) */
        if ((i == 0) || (i == 5) || (i == 10) || (i == 15) || (i == 80) ||
            (i == 85) || (i == 90) || (i == 95) || (i == 160) || (i == 165) ||
            (i == 170) || (i == 175) || (i == 240) || (i == 245) ||
            (i == 250) || (i == 255))
        {
            ok(ret == 1, "index %ld failed, but should succeed\n", i);
        }
        else
        {
            ok(ret == 0, "index %ld succeeded, but should fail\n", i);
        }
    }

    /* Test quaternary rop, the background part is simply ignored */
    ret = PatBlt(hdcTarget, 0, 0, 1, 1, MAKEROP4(PATCOPY, PATINVERT));
    ok_long(ret, 1);
    ret = PatBlt(hdcTarget, 0, 0, 1, 1, MAKEROP4(PATCOPY, SRCCOPY));
    ok_long(ret, 1);
    ret = PatBlt(hdcTarget, 0, 0, 1, 1, MAKEROP4(SRCCOPY, PATCOPY));
    ok_long(ret, 0);

    /* Test an info DC */
    hdc = CreateICA("DISPLAY", NULL, NULL, NULL);
    ok(hdc != 0, "\n");
    SetLastError(0);
    ok_long(PatBlt(hdc, 0, 0, 1, 1, PATCOPY), 1);
    ok_err(0);
    DeleteDC(hdc);

    /* Test a mem DC without selecting a bitmap */
    hdc = CreateCompatibleDC(NULL);
    ok(hdc != 0, "\n");
    ok_long(PatBlt(hdc, 0, 0, 1, 1, PATCOPY), 1);
    ok_err(0);
    DeleteDC(hdc);



}

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

    Test_PatBlt_Params();

    Test_BrushOrigin();


}

