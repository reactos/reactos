/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtGdiGetBitmapBits
 * PROGRAMMERS:
 */

#include <win32nt.h>

START_TEST(NtGdiGetBitmapBits)
{
    BYTE Bits[50] = {0,1,2,3,4,5,6,7,8,9};
    HBITMAP hBitmap;

    SetLastError(ERROR_SUCCESS);
    ok_long(NtGdiGetBitmapBits(0, 0, 0), 0);
    ok_long(GetLastError(), ERROR_INVALID_HANDLE);

    /* Test NULL bitmap handle */
    SetLastError(ERROR_SUCCESS);
    ok_long(NtGdiGetBitmapBits(0, 5, Bits), 0);
    ok_long(GetLastError(), ERROR_INVALID_HANDLE);

    /* Test invalid bitmap handle */
    hBitmap = (HBITMAP)CreatePen(PS_SOLID, 1, RGB(1,2,3));
    SetLastError(ERROR_SUCCESS);
    ok_long(NtGdiGetBitmapBits(hBitmap, 5, Bits), 0);
    ok_long(GetLastError(), ERROR_INVALID_HANDLE);
    DeleteObject(hBitmap);

    hBitmap = CreateBitmap(3, 3, 1, 8, NULL);
    SetLastError(ERROR_SUCCESS);

    /* test NULL pointer and count buffer size != 0 */
    ok_long(NtGdiGetBitmapBits(hBitmap, 5, NULL), 12);

    /* test NULL pointer and buffer size == 0*/
    ok_long(NtGdiGetBitmapBits(hBitmap, 0, NULL), 12);

    /* test bad pointer */
    ok_long(NtGdiGetBitmapBits(hBitmap, 5, (PBYTE)0x500), 0);

    /* Test if we can set a number of bytes between lines */
    ok_long(NtGdiGetBitmapBits(hBitmap, 5, Bits), 5);

    /* Test alignment */
    ok_long(NtGdiGetBitmapBits(hBitmap, 4, Bits+1), 4);

    /* Test 1 byte too much */
    ok_long(NtGdiGetBitmapBits(hBitmap, 10, Bits), 10);

    /* Test one row too much */
    ok_long(NtGdiGetBitmapBits(hBitmap, 12, Bits), 12);

    ok_long(NtGdiGetBitmapBits(hBitmap, 13, Bits), 12);

    ok_long(NtGdiGetBitmapBits(hBitmap, 100, Bits), 12);

    /* Test huge bytes count */
    ok_long(NtGdiGetBitmapBits(hBitmap, 12345678, Bits), 12);

    /* Test negative bytes count */
    ok_long(NtGdiGetBitmapBits(hBitmap, -5, Bits), 12);

    ok_long(GetLastError(), ERROR_SUCCESS);

    DeleteObject(hBitmap);
}
