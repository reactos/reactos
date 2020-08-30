/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Tests for the StretchBlt API
 * COPYRIGHT:       Copyright 2020 Doug Lyons <douglyons at douglyons dot com>
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
#include "mmsystem.h"
#include "wine/winternl.h"
#include "wine/test.h"

static void test_StretchBlt(void)
{
    HBITMAP bmpDst, bmpSrc;
    HBITMAP oldDst, oldSrc;
    HDC hdcScreen, hdcDst, hdcSrc;
    UINT32 *dstBuffer, *srcBuffer;
    BITMAPINFO biDst, biSrc;
    UINT32 expected[256];

    memset(&biDst, 0, sizeof(BITMAPINFO));
    biDst.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    biDst.bmiHeader.biWidth = 16;
    biDst.bmiHeader.biHeight = -16;
    biDst.bmiHeader.biPlanes = 1;
    biDst.bmiHeader.biBitCount = 32;
    biDst.bmiHeader.biCompression = BI_RGB;
    memcpy(&biSrc, &biDst, sizeof(BITMAPINFO));

    hdcScreen = CreateCompatibleDC(0);
    hdcDst = CreateCompatibleDC(hdcScreen);
    hdcSrc = CreateCompatibleDC(hdcDst);

    bmpDst = CreateDIBSection(hdcScreen, &biDst, DIB_RGB_COLORS, (void**)&dstBuffer,
        NULL, 0);
    oldDst = SelectObject(hdcDst, bmpDst);

    bmpSrc = CreateDIBSection(hdcScreen, &biSrc, DIB_RGB_COLORS, (void**)&srcBuffer,
        NULL, 0);
    oldSrc = SelectObject(hdcSrc, bmpSrc);

    SelectObject(hdcSrc, oldSrc);
    DeleteObject(bmpSrc);
    SelectObject(hdcDst, oldDst);
    DeleteObject(bmpDst);

    memset(&biDst, 0, sizeof(BITMAPINFO));              // Clear our Bitmap Into to all zeroes

    biDst.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);  // Set our Size to the header size
    biDst.bmiHeader.biWidth = 2;                        // Set our Width to 2
    biDst.bmiHeader.biHeight = 2;                       // Set our Height to 2 that's positive (bottom-up)
    biDst.bmiHeader.biPlanes = 1;                       // Set out planes to 1 (1 required by Windows)
    biDst.bmiHeader.biBitCount = 32;                    // Set out BitCount to 32 (Full Color)
    biDst.bmiHeader.biCompression = BI_RGB;             // Set our Compression to BI_RBG (uncompressed)

    memcpy(&biSrc, &biDst, sizeof(BITMAPINFO));         // Put same Destination params into the Source

    bmpSrc = CreateDIBSection(hdcSrc, &biSrc, DIB_RGB_COLORS, (void**)&srcBuffer, NULL, 0);
    oldSrc = SelectObject(hdcSrc, bmpSrc);
    bmpDst = CreateDIBSection(hdcDst, &biDst, DIB_RGB_COLORS, (void**)&dstBuffer, NULL, 0);
    oldDst = SelectObject(hdcDst, bmpDst);

    srcBuffer[0] = 255;             // BLUE// stored beginning bottom left
    srcBuffer[1] = 255*256;         // GREEN
    srcBuffer[2] = 255*256*256;     // RED
    srcBuffer[3] = 127*256*256*256; // BLACK

    /* Flip Left to Right on Source */
    StretchBlt(hdcDst, 0, 0, 2, 2, hdcSrc, 1, 0, -2, 2, SRCCOPY );
    expected[0] = 255*256;
    expected[1] = 255;
    expected[2] = 127*256*256*256;
    expected[3] = 255*256*256;

    ok(expected[1] == dstBuffer[1], "StretchBlt expected { %08X } got { %08X }\n",
        expected[1], dstBuffer[1] );

    ok(expected[3] == dstBuffer[3], "StretchBlt expected { %08X } got { %08X }\n",
        expected[3], dstBuffer[3] );

    /* Flip Top to Bottom  on Source */
    StretchBlt(hdcDst, 0, 0, 2, 2, hdcSrc, 0, 1, 2, -2, SRCCOPY );
    expected[0] = 255*256*256;
    expected[1] = 127*256*256*256;
    expected[2] = 255;
    expected[3] = 255*256;

    ok(expected[0] == dstBuffer[0], "StretchBlt expected { %08X } got { %08X }\n",
        expected[0], dstBuffer[0] );

    ok(expected[1] == dstBuffer[1], "StretchBlt expected { %08X } got { %08X }\n",
        expected[1], dstBuffer[1] );

    /* Flip Left to Right and Top To Bottom on Destination */
    StretchBlt(hdcDst, 1, 1, -2, -2, hdcSrc, 0, 0, 2, 2, SRCCOPY );
    expected[0] = 127*256*256*256;
    expected[1] = 255*256*256;
    expected[2] = 255*256;
    expected[3] = 255;

    StretchBlt(hdcDst, 0, 0, 2, 2, hdcSrc, 1, 1, -2, -2, SRCCOPY );
    expected[0] = 127*256*256*256;
    expected[1] = 255*256*256;
    expected[2] = 255*256;
    expected[3] = 255;

    ok(expected[1] == dstBuffer[1], "StretchBlt expected { %08X } got { %08X }\n",
        expected[1], dstBuffer[1] );

    /* Flip Left to Right and Top To Bottom on both Source and Destination  */
    StretchBlt(hdcDst, 1, 1, -2, -2, hdcSrc, 1, 1, -2, -2, SRCCOPY );
    expected[0] = 127*256*256*256;
    expected[1] = 255*256*256;
    expected[2] = 255*256*256;
    expected[3] = 255;

    ok(expected[1] == dstBuffer[1], "StretchBlt expected { %08X } got { %08X }\n",
        expected[1], dstBuffer[1] );

    DeleteDC(hdcSrc);

    SelectObject(hdcDst, oldDst);
    DeleteObject(bmpDst);
    DeleteDC(hdcDst);

    DeleteDC(hdcScreen);
}


START_TEST(StretchBlt)
{
    test_StretchBlt();
}
