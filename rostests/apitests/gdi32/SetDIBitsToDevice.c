/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for SetDIBitsToDevice
 * PROGRAMMERS:     Timo Kreuzer
 */

#include <apitest.h>

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
    pbmi->bmiHeader.biHeight = -4;
    pbmi->bmiHeader.biPlanes = 1;
    pbmi->bmiHeader.biBitCount = 32;
    pbmi->bmiHeader.biCompression = BI_RGB;
    pbmi->bmiHeader.biSizeImage = 0;
    pbmi->bmiHeader.biXPelsPerMeter = 0;
    pbmi->bmiHeader.biYPelsPerMeter = 0;
    pbmi->bmiHeader.biClrUsed = 0;
    pbmi->bmiHeader.biClrImportant = 0;

    /* Test a normal operation */
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

    /* Test hdc == NULL */
    SetLastError(0xdeadc0de);
    ret = SetDIBitsToDevice(NULL,
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
    ok_err(ERROR_INVALID_HANDLE);

    /* Test truncated hdc */
    SetLastError(0xdeadc0de);
    ret = SetDIBitsToDevice((HDC)((ULONG_PTR)ghdcDIB32 & 0xFFFF),
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
    ok_err(ERROR_INVALID_HANDLE);

    /* Test invalid ColorUse */
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
                            7);
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
                            20000000, // cScanLines,
                            (BYTE*)aulBits + 1, // lpvBits,
                            pbmi,
                            DIB_RGB_COLORS);
    todo_ros ok_dec(ret, 0);
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
    todo_ros ok_dec(ret, 0);
    ok_err(0xdeadc0de);

    /* Test negative XDest */
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

    /* Test huge XDest */
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

    /* Test XSrc outside of the DIB */
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

    /* Test YSrc outside of the DIB */
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

    /* Test uStartScan outside of the DIB */
    SetLastError(0xdeadc0de);
    ret = SetDIBitsToDevice(ghdcDIB32,
                            0, // XDest,
                            0, // YDest,
                            2, // dwWidth,
                            2, // dwHeight,
                            0, // XSrc,
                            0, // YSrc,
                            100, // uStartScan,
                            5, // cScanLines,
                            aulBits, // lpvBits,
                            pbmi,
                            DIB_RGB_COLORS);
    ok_dec(ret, 5);
    ok_err(0xdeadc0de);

    /* Test cScanLines larger than the DIB */
    SetLastError(0xdeadc0de);
    ret = SetDIBitsToDevice(ghdcDIB32,
                            0, // XDest,
                            0, // YDest,
                            2, // dwWidth,
                            2, // dwHeight,
                            0, // XSrc,
                            0, // YSrc,
                            0, // uStartScan,
                            7, // cScanLines,
                            aulBits, // lpvBits,
                            pbmi,
                            DIB_RGB_COLORS);
    todo_ros ok_dec(ret, 7);
    ok_err(0xdeadc0de);

    /* Test large cScanlines */
    SetLastError(0xdeadc0de);
    ret = SetDIBitsToDevice(ghdcDIB32,
                            0, // XDest,
                            0, // YDest,
                            2, // dwWidth,
                            2, // dwHeight,
                            0, // XSrc,
                            0, // YSrc,
                            0, // uStartScan,
                            2000, // cScanLines,
                            aulBits, // lpvBits,
                            pbmi,
                            DIB_RGB_COLORS);
    todo_ros ok_dec(ret, 0);
    ok_err(0xdeadc0de);

    /* Test uStartScan and cScanLines larger than the DIB */
    SetLastError(0xdeadc0de);
    ret = SetDIBitsToDevice(ghdcDIB32,
                            0, // XDest,
                            0, // YDest,
                            2, // dwWidth,
                            2, // dwHeight,
                            0, // XSrc,
                            0, // YSrc,
                            100, // uStartScan,
                            7, // cScanLines,
                            aulBits, // lpvBits,
                            pbmi,
                            DIB_RGB_COLORS);
    ok_dec(ret, 7);
    ok_err(0xdeadc0de);

    /* Test lpvBits == NULL */
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

    /* Test huge positive DIB height, result is limited to dwHeight */
    pbmi->bmiHeader.biHeight = 10000;
    SetLastError(0xdeadc0de);
    ret = SetDIBitsToDevice(ghdcDIB32,
                            0, // XDest,
                            1, // YDest,
                            2, // dwWidth,
                            3, // dwHeight,
                            0, // XSrc,
                            1, // YSrc,
                            0, // uStartScan,
                            7, // cScanLines,
                            aulBits, // lpvBits,
                            pbmi,
                            DIB_RGB_COLORS);
    ok_dec(ret, 4);
    ok_err(0xdeadc0de);

    /* Test huge negative DIB height */
    pbmi->bmiHeader.biHeight = -10000;
    SetLastError(0xdeadc0de);
    ret = SetDIBitsToDevice(ghdcDIB32,
                            0, // XDest,
                            0, // YDest,
                            2, // dwWidth,
                            2, // dwHeight,
                            0, // XSrc,
                            0, // YSrc,
                            0, // uStartScan,
                            7, // cScanLines,
                            aulBits, // lpvBits,
                            pbmi,
                            DIB_RGB_COLORS);
    ok_dec(ret, 7);
    ok_err(0xdeadc0de);

    /* Test what happens when we cause an integer overflow */
    pbmi->bmiHeader.biHeight = LONG_MIN;
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

    /* Now also test a huge value of uStartScan */
    SetLastError(0xdeadc0de);
    ret = SetDIBitsToDevice(ghdcDIB32,
                            0, // XDest,
                            0, // YDest,
                            2, // dwWidth,
                            2, // dwHeight,
                            0, // XSrc,
                            0, // YSrc,
                            abs(pbmi->bmiHeader.biHeight) - 3, // uStartScan,
                            9, // cScanLines,
                            aulBits, // lpvBits,
                            pbmi,
                            DIB_RGB_COLORS);
    ok_dec(ret, 3);
    ok_err(0xdeadc0de);

    /* Now also test a huge value of uStartScan */
    pbmi->bmiHeader.biHeight = LONG_MIN + 1;
    SetLastError(0xdeadc0de);
    ret = SetDIBitsToDevice(ghdcDIB32,
                            0, // XDest,
                            0, // YDest,
                            2, // dwWidth,
                            2, // dwHeight,
                            0, // XSrc,
                            0, // YSrc,
                            abs(pbmi->bmiHeader.biHeight) - 3, // uStartScan,
                            9, // cScanLines,
                            aulBits, // lpvBits,
                            pbmi,
                            DIB_RGB_COLORS);
    ok_dec(ret, 5);
    ok_err(0xdeadc0de);

    /* Now also test a huge value of uStartScan */
    pbmi->bmiHeader.biHeight = LONG_MIN + 7;
    SetLastError(0xdeadc0de);
    ret = SetDIBitsToDevice(ghdcDIB32,
                            0, // XDest,
                            0, // YDest,
                            2, // dwWidth,
                            2, // dwHeight,
                            0, // XSrc,
                            0, // YSrc,
                            abs(pbmi->bmiHeader.biHeight) - 3, // uStartScan,
                            32, // cScanLines,
                            aulBits, // lpvBits,
                            pbmi,
                            DIB_RGB_COLORS);
    ok_dec(ret, 17);
    ok_err(0xdeadc0de);

    /* Test invalid bitmap info header */
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


    memset(gpDIB32, 0, sizeof(*gpDIB32));
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
    ok_hex((*gpDIB32)[0][0], 0x11000000);
    ok_hex((*gpDIB32)[0][1], 0x00000011);
    ok_hex((*gpDIB32)[0][2], 0x00000000);
    ok_hex((*gpDIB32)[0][3], 0x00000000);
    ok_hex((*gpDIB32)[1][0], 0xFF000000);
    ok_hex((*gpDIB32)[1][1], 0x000000FF);
    ok_hex((*gpDIB32)[1][2], 0x00000000);
    ok_hex((*gpDIB32)[1][3], 0x00000000);

    memset(gpDIB32, 0, sizeof(*gpDIB32));
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
    ok_hex((*gpDIB32)[0][0], 0x00000000);
    ok_hex((*gpDIB32)[0][1], 0x00000000);
    ok_hex((*gpDIB32)[0][2], 0x00000000);
    ok_hex((*gpDIB32)[0][3], 0x00000000);
    ok_hex((*gpDIB32)[1][0], 0x11000000);
    ok_hex((*gpDIB32)[1][1], 0x00000011);
    ok_hex((*gpDIB32)[1][2], 0x00000000);
    ok_hex((*gpDIB32)[1][3], 0x00000000);
    ok_hex((*gpDIB32)[2][0], 0xFF000000);
    ok_hex((*gpDIB32)[2][1], 0x000000FF);
    ok_hex((*gpDIB32)[2][2], 0x00000000);
    ok_hex((*gpDIB32)[2][3], 0x00000000);

    memset(gpDIB32, 0, sizeof(*gpDIB32));
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
    todo_ros ok_hex((*gpDIB32)[0][0], 0x00000000);
    todo_ros ok_hex((*gpDIB32)[0][1], 0x00000000);
    ok_hex((*gpDIB32)[0][2], 0x00000000);
    ok_hex((*gpDIB32)[0][3], 0x00000000);
    todo_ros ok_hex((*gpDIB32)[1][0], 0x11000000);
    todo_ros ok_hex((*gpDIB32)[1][1], 0x00000011);
    ok_hex((*gpDIB32)[1][2], 0x00000000);
    ok_hex((*gpDIB32)[1][3], 0x00000000);
#if 0

    memset(gpDIB32, 0, sizeof(*gpDIB32));
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
    todo_ros ok_hex(pulDIB32Bits[0], 0x11000000);
    todo_ros ok_hex(pulDIB32Bits[1], 0x00000011);
    ok_hex(pulDIB32Bits[2], 0x00000000);
    ok_hex(pulDIB32Bits[3], 0x00000000);
    todo_ros ok_hex(pulDIB32Bits[4], 0x00000000);
    todo_ros ok_hex(pulDIB32Bits[5], 0x00000000);
    ok_hex(pulDIB32Bits[6], 0x00000000);
    ok_hex(pulDIB32Bits[7], 0x00000000);

    /*****************/

    /* Use bottom-up bitmap */
    pbmi->bmiHeader.biHeight = 2;

    memset(gpDIB32, 0, sizeof(*gpDIB32));
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

    memset(gpDIB32, 0, sizeof(*gpDIB32));
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

    memset(gpDIB32, 0, sizeof(*gpDIB32));
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
    todo_ros ok_hex(pulDIB32Bits[0], 0x00000000);
    todo_ros ok_hex(pulDIB32Bits[1], 0x00000000);
    ok_hex(pulDIB32Bits[2], 0x00000000);
    ok_hex(pulDIB32Bits[3], 0x00000000);
    todo_ros ok_hex(pulDIB32Bits[4], 0x11000000);
    todo_ros ok_hex(pulDIB32Bits[5], 0x00000011);
    ok_hex(pulDIB32Bits[6], 0x00000000);
    ok_hex(pulDIB32Bits[7], 0x00000000);


    memset(gpDIB32, 0, sizeof(*gpDIB32));
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
    todo_ros ok_hex(pulDIB32Bits[0], 0x11000000);
    todo_ros ok_hex(pulDIB32Bits[1], 0x00000011);
    ok_hex(pulDIB32Bits[2], 0x00000000);
    ok_hex(pulDIB32Bits[3], 0x00000000);
    todo_ros ok_hex(pulDIB32Bits[4], 0x00000000);
    todo_ros ok_hex(pulDIB32Bits[5], 0x00000000);
    ok_hex(pulDIB32Bits[6], 0x00000000);
    ok_hex(pulDIB32Bits[7], 0x00000000);
#endif
}


START_TEST(SetDIBitsToDevice)
{
    InitStuff();

    Test_SetDIBitsToDevice_Params();
    Test_SetDIBitsToDevice();


}
