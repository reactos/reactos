/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for the StretchBlt API
 * COPYRIGHT:   Copyright 2020 Doug Lyons (douglyons at douglyons dot com)
 *              Most Code copied and modified from Wine gdi32:bitmap test.
 */

#include <stdarg.h>
#include <assert.h>
#include <string.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/test.h"

static void test_StretchBlt(void)
{
    HBITMAP bmpDst, bmpSrc;
    HDC hdcDst, hdcSrc;
    UINT32 *dstBuffer, *srcBuffer;
    BITMAPINFO biDst, biSrc;
    UINT32 expected[256];

    memset(&biDst, 0, sizeof(BITMAPINFO));

    biDst.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    biDst.bmiHeader.biWidth = 2;
    biDst.bmiHeader.biHeight = 2;    // Set our Height to positive so we are bottom-up
    biDst.bmiHeader.biPlanes = 1;
    biDst.bmiHeader.biBitCount = 32; // Set our BitCount to 32 which is Full Color
    biDst.bmiHeader.biCompression = BI_RGB;

    memcpy(&biSrc, &biDst, sizeof(BITMAPINFO)); // Put same Destination params into the Source

    hdcDst = CreateCompatibleDC(0);
    hdcSrc = CreateCompatibleDC(0);

    bmpSrc = CreateDIBSection(hdcSrc, &biSrc, DIB_RGB_COLORS, (void**)&srcBuffer, NULL, 0);
    SelectObject(hdcSrc, bmpSrc);
    bmpDst = CreateDIBSection(hdcDst, &biDst, DIB_RGB_COLORS, (void**)&dstBuffer, NULL, 0);
    SelectObject(hdcDst, bmpDst);

    srcBuffer[0] = 0x000000FF; // BLUE - stored beginning bottom left
    srcBuffer[1] = 0x0000FF00; // GREEN
    srcBuffer[2] = 0x00FF0000; // RED
    srcBuffer[3] = 0xFF000000; // BLACK - 0xFF for alpha channel is easy to recognize when printed in hex format

    /* Flip Left to Right on Source */
    StretchBlt(hdcDst, 0, 0, 2, 2, hdcSrc, 1, 0, -2, 2, SRCCOPY);
    expected[0] = 0x0000FF00;
    expected[1] = 0x000000FF;
    expected[2] = 0xFF000000;
    expected[3] = 0x00FF0000;

    ok(expected[1] == dstBuffer[1], "StretchBlt expected { %08X } got { %08X }\n",
        expected[1], dstBuffer[1]);

    ok(expected[3] == dstBuffer[3], "StretchBlt expected { %08X } got { %08X }\n",
        expected[3], dstBuffer[3]);

    /* Flip Top to Bottom on Source */
    StretchBlt(hdcDst, 0, 0, 2, 2, hdcSrc, 0, 1, 2, -2, SRCCOPY);
    expected[0] = 0x00FF0000;
    expected[1] = 0xFF000000;
    expected[2] = 0x000000FF;
    expected[3] = 0x0000FF00;

    ok(expected[0] == dstBuffer[0], "StretchBlt expected { %08X } got { %08X }\n",
        expected[0], dstBuffer[0]);

    ok(expected[1] == dstBuffer[1], "StretchBlt expected { %08X } got { %08X }\n",
        expected[1], dstBuffer[1]);

    /* Flip Left to Right and Top to Bottom on Source */
    StretchBlt(hdcDst, 0, 0, 2, 2, hdcSrc, 1, 1, -2, -2, SRCCOPY);
    expected[0] = 0xFF000000;
    expected[1] = 0x00FF0000;
    expected[2] = 0x0000FF00;
    expected[3] = 0x000000FF;

    ok(expected[1] == dstBuffer[1], "StretchBlt expected { %08X } got { %08X }\n",
        expected[1], dstBuffer[1]);

    /* Flip Left to Right and Top to Bottom on both Source and Destination */
    StretchBlt(hdcDst, 1, 1, -2, -2, hdcSrc, 1, 1, -2, -2, SRCCOPY);
    expected[0] = 0xFF000000;
    expected[1] = 0x00FF0000;
    expected[2] = 0x00FF0000;
    expected[3] = 0x000000FF;

    ok(expected[1] == dstBuffer[1], "StretchBlt expected { %08X } got { %08X }\n",
        expected[1], dstBuffer[1]);

    DeleteDC(hdcSrc);
    DeleteDC(hdcDst);
}


START_TEST(StretchBlt)
{
    test_StretchBlt();
}
