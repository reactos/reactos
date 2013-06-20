/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for SetDIBitsToDevice
 * PROGRAMMERS:     Timo Kreuzer
 */

#include <wine/test.h>
#include <wingdi.h>

#include "init.h"

static void
Test_SetDIBitsToDevice_Params()
{
    UCHAR ajBmiBuffer[sizeof(BITMAPINFO) + 4];
    PBITMAPINFO pbmi = (PBITMAPINFO)ajBmiBuffer;
    ULONG aulBits[16];
    INT ret;

    /* Setup the bitmap info */
    pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biWidth = 2;
    pbmi->bmiHeader.biHeight = -2;
    pbmi->bmiHeader.biPlanes = 1;
    pbmi->bmiHeader.biBitCount = 32;
    pbmi->bmiHeader.biCompression = BI_RGB;
    pbmi->bmiHeader.biSizeImage = 0;
    pbmi->bmiHeader.biXPelsPerMeter = 0;
    pbmi->bmiHeader.biYPelsPerMeter = 0;
    pbmi->bmiHeader.biClrUsed = 0;
    pbmi->bmiHeader.biClrImportant = 0;

    SetLastError(0xdeadc0de);
    ret = SetDIBitsToDevice(ghdcDIB32,
                            0, // XDest,
                            0, // YDest,
                            2, // dwWidth,
                            2, // dwHeight,
                            0, // XSrc,
                            0, // YSrc,
                            0, // uStartScan,
                            2, // cScanLines,
                            aulBits, // lpvBits,
                            pbmi,
                            DIB_RGB_COLORS);
    ok_dec(ret, 2);
    ok_err(0xdeadc0de);

    SetLastError(0xdeadc0de);
    ret = SetDIBitsToDevice(ghdcDIB32,
                            0, // XDest,
                            0, // YDest,
                            2, // dwWidth,
                            2, // dwHeight,
                            0, // XSrc,
                            0, // YSrc,
                            0, // uStartScan,
                            2, // cScanLines,
                            NULL, // lpvBits,
                            pbmi,
                            DIB_RGB_COLORS);
    ok_dec(ret, 0);
    ok_err(0xdeadc0de);

    /* test unaligned buffer */
    SetLastError(0xdeadc0de);
    ret = SetDIBitsToDevice(ghdcDIB32,
                            0, // XDest,
                            0, // YDest,
                            2, // dwWidth,
                            2, // dwHeight,
                            0, // XSrc,
                            0, // YSrc,
                            0, // uStartScan,
                            2, // cScanLines,
                            (BYTE*)aulBits + 1, // lpvBits,
                            pbmi,
                            DIB_RGB_COLORS);
    ok_dec(ret, 2);
    ok_err(0xdeadc0de);

    /* test unaligned and huge scanline buffer */
    SetLastError(0xdeadc0de);
    ret = SetDIBitsToDevice(ghdcDIB32,
                            0, // XDest,
                            0, // YDest,
                            2, // dwWidth,
                            2, // dwHeight,
                            0, // XSrc,
                            0, // YSrc,
                            0, // uStartScan,
                            2000000, // cScanLines,
                            (BYTE*)aulBits + 1, // lpvBits,
                            pbmi,
                            DIB_RGB_COLORS);
    ok_dec(ret, 0);
    ok_err(0xdeadc0de);

    /* test unaligned illegal buffer */
    SetLastError(0xdeadc0de);
    ret = SetDIBitsToDevice(ghdcDIB32,
                            0, // XDest,
                            0, // YDest,
                            2, // dwWidth,
                            2, // dwHeight,
                            0, // XSrc,
                            0, // YSrc,
                            0, // uStartScan,
                            2, // cScanLines,
                            (BYTE*)0x7fffffff, // lpvBits,
                            pbmi,
                            DIB_RGB_COLORS);
    ok_dec(ret, 0);
    ok_err(0xdeadc0de);

    SetLastError(0xdeadc0de);
    ret = SetDIBitsToDevice(ghdcDIB32,
                            -100, // XDest,
                            0, // YDest,
                            2, // dwWidth,
                            2, // dwHeight,
                            0, // XSrc,
                            0, // YSrc,
                            0, // uStartScan,
                            2, // cScanLines,
                            aulBits, // lpvBits,
                            pbmi,
                            DIB_RGB_COLORS);
    ok_dec(ret, 2);
    ok_err(0xdeadc0de);

    SetLastError(0xdeadc0de);
    ret = SetDIBitsToDevice(ghdcDIB32,
                            LONG_MAX, // XDest,
                            0, // YDest,
                            2, // dwWidth,
                            2, // dwHeight,
                            0, // XSrc,
                            0, // YSrc,
                            0, // uStartScan,
                            2, // cScanLines,
                            aulBits, // lpvBits,
                            pbmi,
                            DIB_RGB_COLORS);
    ok_dec(ret, 2);
    ok_err(0xdeadc0de);

    SetLastError(0xdeadc0de);
    ret = SetDIBitsToDevice(ghdcDIB32,
                            0, // XDest,
                            0, // YDest,
                            2, // dwWidth,
                            2, // dwHeight,
                            100, // XSrc,
                            0, // YSrc,
                            0, // uStartScan,
                            2, // cScanLines,
                            aulBits, // lpvBits,
                            pbmi,
                            DIB_RGB_COLORS);
    ok_dec(ret, 2);
    ok_err(0xdeadc0de);

    SetLastError(0xdeadc0de);
    ret = SetDIBitsToDevice(ghdcDIB32,
                            0, // XDest,
                            0, // YDest,
                            2, // dwWidth,
                            2, // dwHeight,
                            0, // XSrc,
                            100, // YSrc,
                            0, // uStartScan,
                            2, // cScanLines,
                            aulBits, // lpvBits,
                            pbmi,
                            DIB_RGB_COLORS);
    ok_dec(ret, 2);
    ok_err(0xdeadc0de);

    SetLastError(0xdeadc0de);
    ret = SetDIBitsToDevice(ghdcDIB32,
                            0, // XDest,
                            0, // YDest,
                            2, // dwWidth,
                            2, // dwHeight,
                            0, // XSrc,
                            0, // YSrc,
                            0, // uStartScan,
                            66, // cScanLines,
                            aulBits, // lpvBits,
                            pbmi,
                            DIB_RGB_COLORS);
    ok_dec(ret, 66);
    ok_err(0xdeadc0de);

    SetLastError(0xdeadc0de);
    ret = SetDIBitsToDevice(ghdcDIB32,
                            0, // XDest,
                            0, // YDest,
                            2, // dwWidth,
                            2, // dwHeight,
                            0, // XSrc,
                            0, // YSrc,
                            0, // uStartScan,
                            200, // cScanLines,
                            aulBits, // lpvBits,
                            pbmi,
                            DIB_RGB_COLORS);
    ok_dec(ret, 0);
    ok_err(0xdeadc0de);

    SetLastError(0xdeadc0de);
    ret = SetDIBitsToDevice(ghdcDIB32,
                            0, // XDest,
                            0, // YDest,
                            2, // dwWidth,
                            2, // dwHeight,
                            0, // XSrc,
                            0, // YSrc,
                            2000, // uStartScan,
                            66, // cScanLines,
                            aulBits, // lpvBits,
                            pbmi,
                            DIB_RGB_COLORS);
    ok_dec(ret, 66);
    ok_err(0xdeadc0de);

    /* Test pbmi == NULL */
    SetLastError(0xdeadc0de);
    ret = SetDIBitsToDevice(ghdcDIB32,
                            0, // XDest,
                            0, // YDest,
                            2, // dwWidth,
                            2, // dwHeight,
                            0, // XSrc,
                            0, // YSrc,
                            0, // uStartScan,
                            2, // cScanLines,
                            aulBits, // lpvBits,
                            NULL,
                            DIB_RGB_COLORS);
    ok_dec(ret, 0);
    ok_err(0xdeadc0de);

    /* Test illegal bitmap info */
    pbmi->bmiHeader.biSize = 0;
    SetLastError(0xdeadc0de);
    ret = SetDIBitsToDevice(ghdcDIB32,
                            0, // XDest,
                            0, // YDest,
                            2, // dwWidth,
                            2, // dwHeight,
                            0, // XSrc,
                            0, // YSrc,
                            0, // uStartScan,
                            2, // cScanLines,
                            aulBits, // lpvBits,
                            pbmi,
                            DIB_RGB_COLORS);
    ok_dec(ret, 0);
    ok_err(0xdeadc0de);

}


static void
Test_SetDIBitsToDevice()
{
    UCHAR ajBmiBuffer[sizeof(BITMAPINFO) + 4];
    PBITMAPINFO pbmi = (PBITMAPINFO)ajBmiBuffer;
    ULONG aulBits[16];
    INT ret;

    /* Setup the bitmap info */
    pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biWidth = 2;
    pbmi->bmiHeader.biHeight = -2;
    pbmi->bmiHeader.biPlanes = 1;
    pbmi->bmiHeader.biBitCount = 32;
    pbmi->bmiHeader.biCompression = BI_RGB;
    pbmi->bmiHeader.biSizeImage = 0;
    pbmi->bmiHeader.biXPelsPerMeter = 0;
    pbmi->bmiHeader.biYPelsPerMeter = 0;
    pbmi->bmiHeader.biClrUsed = 0;
    pbmi->bmiHeader.biClrImportant = 0;

    /* Set pixels */
    aulBits[0] = 0x11000000;
    aulBits[1] = 0x00000011;
    aulBits[2] = 0xFF000000;
    aulBits[3] = 0x000000FF;


    memset(pulDIB32Bits, 0, 4 * 4 * sizeof(ULONG));
    ret = SetDIBitsToDevice(ghdcDIB32,
                            0, // XDest,
                            0, // YDest,
                            2, // dwWidth,
                            2, // dwHeight,
                            0, // XSrc,
                            0, // YSrc,
                            0, // uStartScan,
                            2, // cScanLines,
                            aulBits, // lpvBits,
                            pbmi,
                            DIB_RGB_COLORS);

    ok_dec(ret, 2);
    ok_hex(pulDIB32Bits[0], 0x11000000);
    ok_hex(pulDIB32Bits[1], 0x00000011);
    ok_hex(pulDIB32Bits[2], 0x00000000);
    ok_hex(pulDIB32Bits[3], 0x00000000);
    ok_hex(pulDIB32Bits[4], 0xFF000000);
    ok_hex(pulDIB32Bits[5], 0x000000FF);
    ok_hex(pulDIB32Bits[6], 0x00000000);
    ok_hex(pulDIB32Bits[7], 0x00000000);

    memset(pulDIB32Bits, 0, 4 * 4 * sizeof(ULONG));
    ret = SetDIBitsToDevice(ghdcDIB32,
                            0, // XDest,
                            1, // YDest,
                            2, // dwWidth,
                            2, // dwHeight,
                            0, // XSrc,
                            0, // YSrc,
                            0, // uStartScan,
                            2, // cScanLines,
                            aulBits, // lpvBits,
                            pbmi,
                            DIB_RGB_COLORS);

    ok_dec(ret, 2);
    ok_hex(pulDIB32Bits[0], 0x00000000);
    ok_hex(pulDIB32Bits[1], 0x00000000);
    ok_hex(pulDIB32Bits[2], 0x00000000);
    ok_hex(pulDIB32Bits[3], 0x00000000);
    ok_hex(pulDIB32Bits[4], 0x11000000);
    ok_hex(pulDIB32Bits[5], 0x00000011);
    ok_hex(pulDIB32Bits[6], 0x00000000);
    ok_hex(pulDIB32Bits[7], 0x00000000);
    ok_hex(pulDIB32Bits[8], 0xFF000000);
    ok_hex(pulDIB32Bits[9], 0x000000FF);
    ok_hex(pulDIB32Bits[10], 0x00000000);
    ok_hex(pulDIB32Bits[11], 0x00000000);

    memset(pulDIB32Bits, 0, 4 * 4 * sizeof(ULONG));
    ret = SetDIBitsToDevice(ghdcDIB32,
                            0, // XDest,
                            0, // YDest,
                            2, // dwWidth,
                            2, // dwHeight,
                            0, // XSrc,
                            0, // YSrc,
                            0, // uStartScan,
                            1, // cScanLines,
                            aulBits, // lpvBits,
                            pbmi,
                            DIB_RGB_COLORS);

    ok_dec(ret, 1);
    ok_hex(pulDIB32Bits[0], 0x00000000);
    ok_hex(pulDIB32Bits[1], 0x00000000);
    ok_hex(pulDIB32Bits[2], 0x00000000);
    ok_hex(pulDIB32Bits[3], 0x00000000);
    ok_hex(pulDIB32Bits[4], 0x11000000);
    ok_hex(pulDIB32Bits[5], 0x00000011);
    ok_hex(pulDIB32Bits[6], 0x00000000);
    ok_hex(pulDIB32Bits[7], 0x00000000);


    memset(pulDIB32Bits, 0, 4 * 4 * sizeof(ULONG));
    ret = SetDIBitsToDevice(ghdcDIB32,
                            0, // XDest,
                            0, // YDest,
                            2, // dwWidth,
                            2, // dwHeight,
                            0, // XSrc,
                            0, // YSrc,
                            1, // uStartScan,
                            1, // cScanLines,
                            aulBits, // lpvBits,
                            pbmi,
                            DIB_RGB_COLORS);

    ok_dec(ret, 1);
    ok_hex(pulDIB32Bits[0], 0x11000000);
    ok_hex(pulDIB32Bits[1], 0x00000011);
    ok_hex(pulDIB32Bits[2], 0x00000000);
    ok_hex(pulDIB32Bits[3], 0x00000000);
    ok_hex(pulDIB32Bits[4], 0x00000000);
    ok_hex(pulDIB32Bits[5], 0x00000000);
    ok_hex(pulDIB32Bits[6], 0x00000000);
    ok_hex(pulDIB32Bits[7], 0x00000000);

    /*****************/

    /* Use bottom-up bitmap */
    pbmi->bmiHeader.biHeight = 2;

    memset(pulDIB32Bits, 0, 4 * 4 * sizeof(ULONG));
    ret = SetDIBitsToDevice(ghdcDIB32,
                            0, // XDest,
                            0, // YDest,
                            2, // dwWidth,
                            2, // dwHeight,
                            0, // XSrc,
                            0, // YSrc,
                            0, // uStartScan,
                            2, // cScanLines,
                            aulBits, // lpvBits,
                            pbmi,
                            DIB_RGB_COLORS);

    ok_dec(ret, 2);
    ok_hex(pulDIB32Bits[0], 0xFF000000);
    ok_hex(pulDIB32Bits[1], 0x000000FF);
    ok_hex(pulDIB32Bits[2], 0x00000000);
    ok_hex(pulDIB32Bits[3], 0x00000000);
    ok_hex(pulDIB32Bits[4], 0x11000000);
    ok_hex(pulDIB32Bits[5], 0x00000011);
    ok_hex(pulDIB32Bits[6], 0x00000000);
    ok_hex(pulDIB32Bits[7], 0x00000000);

    memset(pulDIB32Bits, 0, 4 * 4 * sizeof(ULONG));
    ret = SetDIBitsToDevice(ghdcDIB32,
                            0, // XDest,
                            1, // YDest,
                            2, // dwWidth,
                            2, // dwHeight,
                            0, // XSrc,
                            0, // YSrc,
                            0, // uStartScan,
                            2, // cScanLines,
                            aulBits, // lpvBits,
                            pbmi,
                            DIB_RGB_COLORS);

    ok_dec(ret, 2);
    ok_hex(pulDIB32Bits[0], 0x00000000);
    ok_hex(pulDIB32Bits[1], 0x00000000);
    ok_hex(pulDIB32Bits[2], 0x00000000);
    ok_hex(pulDIB32Bits[3], 0x00000000);
    ok_hex(pulDIB32Bits[4], 0xFF000000);
    ok_hex(pulDIB32Bits[5], 0x000000FF);
    ok_hex(pulDIB32Bits[6], 0x00000000);
    ok_hex(pulDIB32Bits[7], 0x00000000);
    ok_hex(pulDIB32Bits[8], 0x11000000);
    ok_hex(pulDIB32Bits[9], 0x00000011);
    ok_hex(pulDIB32Bits[10], 0x00000000);
    ok_hex(pulDIB32Bits[11], 0x00000000);

    memset(pulDIB32Bits, 0, 4 * 4 * sizeof(ULONG));
    ret = SetDIBitsToDevice(ghdcDIB32,
                            0, // XDest,
                            0, // YDest,
                            2, // dwWidth,
                            2, // dwHeight,
                            0, // XSrc,
                            0, // YSrc,
                            0, // uStartScan,
                            1, // cScanLines,
                            aulBits, // lpvBits,
                            pbmi,
                            DIB_RGB_COLORS);

    ok_dec(ret, 1);
    ok_hex(pulDIB32Bits[0], 0x00000000);
    ok_hex(pulDIB32Bits[1], 0x00000000);
    ok_hex(pulDIB32Bits[2], 0x00000000);
    ok_hex(pulDIB32Bits[3], 0x00000000);
    ok_hex(pulDIB32Bits[4], 0x11000000);
    ok_hex(pulDIB32Bits[5], 0x00000011);
    ok_hex(pulDIB32Bits[6], 0x00000000);
    ok_hex(pulDIB32Bits[7], 0x00000000);


    memset(pulDIB32Bits, 0, 4 * 4 * sizeof(ULONG));
    ret = SetDIBitsToDevice(ghdcDIB32,
                            0, // XDest,
                            0, // YDest,
                            2, // dwWidth,
                            2, // dwHeight,
                            0, // XSrc,
                            0, // YSrc,
                            1, // uStartScan,
                            1, // cScanLines,
                            aulBits, // lpvBits,
                            pbmi,
                            DIB_RGB_COLORS);

    ok_dec(ret, 1);
    ok_hex(pulDIB32Bits[0], 0x11000000);
    ok_hex(pulDIB32Bits[1], 0x00000011);
    ok_hex(pulDIB32Bits[2], 0x00000000);
    ok_hex(pulDIB32Bits[3], 0x00000000);
    ok_hex(pulDIB32Bits[4], 0x00000000);
    ok_hex(pulDIB32Bits[5], 0x00000000);
    ok_hex(pulDIB32Bits[6], 0x00000000);
    ok_hex(pulDIB32Bits[7], 0x00000000);

}


START_TEST(SetDIBitsToDevice)
{
    InitStuff();

    Test_SetDIBitsToDevice_Params();
    Test_SetDIBitsToDevice();


}
