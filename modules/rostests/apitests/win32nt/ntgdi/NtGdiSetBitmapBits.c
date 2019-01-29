/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtGdiSetBitmapBits
 * PROGRAMMERS:     Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <win32nt.h>

START_TEST(NtGdiSetBitmapBits)
{
    BYTE Bits[100] = {0,1,2,3,4,5,6,7,8,9};
    HBITMAP hBitmap;
    HDC hDC;
    BITMAPINFO bmi;
    LPVOID pvBits;
    LPBYTE LargeBits;

    SetLastError(0xDEADFACE);
    ok_long(NtGdiSetBitmapBits(0, 0, 0), 0);
    ok_long(GetLastError(), 0xDEADFACE);

    /* Test NULL bitnap handle */
    SetLastError(0xDEADFACE);
    ok_long(NtGdiSetBitmapBits(0, 5, Bits), 0);
    ok_long(GetLastError(), ERROR_INVALID_HANDLE);

    /* Test invalid bitmap handle */
    hBitmap = (HBITMAP)CreatePen(PS_SOLID, 1, RGB(1,2,3));
    SetLastError(0xDEADFACE);
    ok_long(NtGdiSetBitmapBits(hBitmap, 5, Bits), 0);
    ok_long(GetLastError(), ERROR_INVALID_HANDLE);
    DeleteObject(hBitmap);

    hBitmap = CreateBitmap(3, 3, 1, 8, NULL);
    ok(hBitmap != NULL, "hBitmap was NULL.\n");

    /* test NULL pointer and count buffer size != 0 */
    SetLastError(0xDEADFACE);
    ok_long(NtGdiSetBitmapBits(hBitmap, 5, NULL), 0);
    ok_long(GetLastError(), 0xDEADFACE);

    /* test NULL pointer and buffer size == 0*/
    SetLastError(0xDEADFACE);
    ok_long(NtGdiSetBitmapBits(hBitmap, 0, NULL), 0);
    ok_long(GetLastError(), 0xDEADFACE);

    /* test bad pointer */
    SetLastError(0xDEADFACE);
    ok_long(NtGdiSetBitmapBits(hBitmap, 5, (PBYTE)0x500), 0);
    ok_long(GetLastError(), 0xDEADFACE);

    /* Test if we can set a number of bytes between lines */
    SetLastError(0xDEADFACE);
    ok_long(NtGdiSetBitmapBits(hBitmap, 5, Bits), 5);
    ok_long(GetLastError(), 0xDEADFACE);

    /* Test alignment */
    SetLastError(0xDEADFACE);
    ok_long(NtGdiSetBitmapBits(hBitmap, 4, Bits+1), 4);
    ok_long(GetLastError(), 0xDEADFACE);

    /* Test 1 byte too much */
    SetLastError(0xDEADFACE);
    ok_long(NtGdiSetBitmapBits(hBitmap, 10, Bits), 10);
    ok_long(GetLastError(), 0xDEADFACE);

    /* Test one row too much */
    SetLastError(0xDEADFACE);
    ok_long(NtGdiSetBitmapBits(hBitmap, 12, Bits), 12);
    ok_long(GetLastError(), 0xDEADFACE);

    SetLastError(0xDEADFACE);
    ok_long(NtGdiSetBitmapBits(hBitmap, 13, Bits), 12);
    ok_long(GetLastError(), 0xDEADFACE);

    SetLastError(0xDEADFACE);
    ok_long(NtGdiSetBitmapBits(hBitmap, 100, Bits), 12);
    ok_long(GetLastError(), 0xDEADFACE);

    /* Test large byte counts (with PAGE_NOACCESS) */
    LargeBits = VirtualAlloc(NULL, 0x100000 + PAGE_SIZE, MEM_RESERVE, PAGE_NOACCESS);
    VirtualAlloc(LargeBits, 0x100000, MEM_COMMIT, PAGE_READWRITE);
    CopyMemory(LargeBits, Bits, sizeof(Bits));

    SetLastError(0xDEADFACE);
    ok_long(NtGdiSetBitmapBits(hBitmap, 0x100, LargeBits), 0xC);
    ok_long(GetLastError(), 0xDEADFACE);

    SetLastError(0xDEADFACE);
    ok_long(NtGdiSetBitmapBits(hBitmap, 0x10000, LargeBits), 0xC);
    ok_long(GetLastError(), 0xDEADFACE);

    SetLastError(0xDEADFACE);
    ok_long(NtGdiSetBitmapBits(hBitmap, 0x100000, LargeBits), 0xC);
    ok_long(GetLastError(), 0xDEADFACE);

    SetLastError(0xDEADFACE);
    ok_long(NtGdiSetBitmapBits(hBitmap, 0x100001, LargeBits), 0x0);
    ok_long(GetLastError(), 0xDEADFACE);

    VirtualFree(LargeBits, 0, MEM_RELEASE);

    /* Test large byte counts (with PAGE_READONLY) */
    LargeBits = VirtualAlloc(NULL, 0x100000 + PAGE_SIZE, MEM_RESERVE, PAGE_READONLY);
    VirtualAlloc(LargeBits, 0x100000, MEM_COMMIT, PAGE_READWRITE);
    CopyMemory(LargeBits, Bits, sizeof(Bits));

    SetLastError(0xDEADFACE);
    ok_long(NtGdiSetBitmapBits(hBitmap, 0x100, LargeBits), 0xC);
    ok_long(GetLastError(), 0xDEADFACE);

    SetLastError(0xDEADFACE);
    ok_long(NtGdiSetBitmapBits(hBitmap, 0x10000, LargeBits), 0xC);
    ok_long(GetLastError(), 0xDEADFACE);

    SetLastError(0xDEADFACE);
    ok_long(NtGdiSetBitmapBits(hBitmap, 0x100000, LargeBits), 0xC);
    ok_long(GetLastError(), 0xDEADFACE);

    SetLastError(0xDEADFACE);
    ok_long(NtGdiSetBitmapBits(hBitmap, 0x100001, LargeBits), 0x0);
    ok_long(GetLastError(), 0xDEADFACE);

    VirtualFree(LargeBits, 0, MEM_RELEASE);

    DeleteObject(hBitmap);

    /* ------------------------- */

    hBitmap = CreateBitmap(16, 16, 1, 1, NULL);
    ok(hBitmap != NULL, "hBitmap was NULL.\n");

    FillMemory(Bits, sizeof(Bits), 0x55);
    SetLastError(0xDEADFACE);
    ok_long(NtGdiGetBitmapBits(hBitmap, 0, Bits), 0);
    ok_long(GetLastError(), 0xDEADFACE);
    ok_int(Bits[0], 0x55);

    FillMemory(Bits, sizeof(Bits), 0x55);

    SetLastError(0xDEADFACE);
    ok_long(NtGdiGetBitmapBits(hBitmap, 1, Bits), 1);
    ok_long(GetLastError(), 0xDEADFACE);
    ok_int(Bits[0], 0);
    ok_int(Bits[1], 0x55);

    FillMemory(Bits, sizeof(Bits), 0x55);
    SetLastError(0xDEADFACE);
    ok_long(NtGdiGetBitmapBits(hBitmap, 2, Bits), 2);
    ok_long(GetLastError(), 0xDEADFACE);
    ok_int(Bits[0], 0);
    ok_int(Bits[1], 0);
    ok_int(Bits[2], 0x55);

    FillMemory(Bits, sizeof(Bits), 0x33);
    SetLastError(0xDEADFACE);
    ok_long(NtGdiSetBitmapBits(hBitmap, 10, Bits), 10);
    ok_long(GetLastError(), 0xDEADFACE);

    FillMemory(Bits, sizeof(Bits), 0xAA);
    SetLastError(0xDEADFACE);
    ok_long(NtGdiSetBitmapBits(hBitmap, 1, Bits), 1);
    ok_long(GetLastError(), 0xDEADFACE);
    FillMemory(Bits, sizeof(Bits), 0x55);
    SetLastError(0xDEADFACE);
    ok_long(NtGdiGetBitmapBits(hBitmap, 1, Bits), 1);
    ok_long(GetLastError(), 0xDEADFACE);
    ok_int(Bits[0], 0xAA);
    ok_int(Bits[1], 0x55);
    SetLastError(0xDEADFACE);
    ok_long(NtGdiGetBitmapBits(hBitmap, 2, Bits), 2);
    ok_long(GetLastError(), 0xDEADFACE);
    ok_int(Bits[0], 0xAA);
    ok_int(Bits[1], 0x33);
    ok_int(Bits[2], 0x55);

    FillMemory(Bits, sizeof(Bits), 0xAA);
    SetLastError(0xDEADFACE);
    ok_long(NtGdiSetBitmapBits(hBitmap, 2, Bits), 2);
    ok_long(GetLastError(), 0xDEADFACE);
    FillMemory(Bits, sizeof(Bits), 0x55);
    SetLastError(0xDEADFACE);
    ok_long(NtGdiGetBitmapBits(hBitmap, 2, Bits), 2);
    ok_long(GetLastError(), 0xDEADFACE);
    ok_int(Bits[0], 0xAA);
    ok_int(Bits[1], 0xAA);
    ok_int(Bits[2], 0x55);
    SetLastError(0xDEADFACE);
    ok_long(NtGdiGetBitmapBits(hBitmap, 3, Bits), 3);
    ok_long(GetLastError(), 0xDEADFACE);
    ok_int(Bits[0], 0xAA);
    ok_int(Bits[1], 0xAA);
    ok_int(Bits[2], 0x33);
    ok_int(Bits[3], 0x55);

    FillMemory(Bits, sizeof(Bits), 0xAA);
    SetLastError(0xDEADFACE);
    ok_long(NtGdiSetBitmapBits(hBitmap, 3, Bits), 3);
    ok_long(GetLastError(), 0xDEADFACE);
    FillMemory(Bits, sizeof(Bits), 0x55);
    SetLastError(0xDEADFACE);
    ok_long(NtGdiGetBitmapBits(hBitmap, 3, Bits), 3);
    ok_long(GetLastError(), 0xDEADFACE);
    ok_int(Bits[0], 0xAA);
    ok_int(Bits[1], 0xAA);
    ok_int(Bits[2], 0xAA);
    ok_int(Bits[3], 0x55);
    SetLastError(0xDEADFACE);
    ok_long(NtGdiGetBitmapBits(hBitmap, 4, Bits), 4);
    ok_long(GetLastError(), 0xDEADFACE);
    ok_int(Bits[0], 0xAA);
    ok_int(Bits[1], 0xAA);
    ok_int(Bits[2], 0xAA);
    ok_int(Bits[3], 0x33);
    ok_int(Bits[4], 0x55);

    DeleteObject(hBitmap);

    /* ------------------------- */

    hDC = CreateCompatibleDC(NULL);

    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = 15;
    bmi.bmiHeader.biHeight = 15;    /* positive */
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    hBitmap = CreateDIBSection(hDC, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0);
    ok(hBitmap != NULL, "hBitmap was NULL.\n");

    FillMemory(Bits, sizeof(Bits), 0x55);
    SetLastError(0xDEADFACE);
    ok_long(NtGdiGetBitmapBits(hBitmap, 0, Bits), 0);
    ok_long(GetLastError(), 0xDEADFACE);
    ok_int(Bits[0], 0x55);

    FillMemory(Bits, sizeof(Bits), 0x55);
    SetLastError(0xDEADFACE);
    ok_long(NtGdiGetBitmapBits(hBitmap, 1, Bits), 1);
    ok_long(GetLastError(), 0xDEADFACE);
    ok_int(Bits[0], 0);
    ok_int(Bits[1], 0x55);

    FillMemory(Bits, sizeof(Bits), 0x55);
    SetLastError(0xDEADFACE);
    ok_long(NtGdiGetBitmapBits(hBitmap, 2, Bits), 2);
    ok_long(GetLastError(), 0xDEADFACE);
    ok_int(Bits[0], 0);
    ok_int(Bits[1], 0);
    ok_int(Bits[2], 0x55);

    FillMemory(Bits, sizeof(Bits), 0x33);
    SetLastError(0xDEADFACE);
    ok_long(NtGdiSetBitmapBits(hBitmap, 10, Bits), 10);
    ok_long(GetLastError(), 0xDEADFACE);

    FillMemory(Bits, sizeof(Bits), 0xAA);
    SetLastError(0xDEADFACE);
    ok_long(NtGdiSetBitmapBits(hBitmap, 1, Bits), 1);
    ok_long(GetLastError(), 0xDEADFACE);
    FillMemory(Bits, sizeof(Bits), 0x55);
    SetLastError(0xDEADFACE);
    ok_long(NtGdiGetBitmapBits(hBitmap, 1, Bits), 1);
    ok_long(GetLastError(), 0xDEADFACE);
    ok_int(Bits[0], 0xAA);
    ok_int(Bits[1], 0x55);
    SetLastError(0xDEADFACE);
    ok_long(NtGdiGetBitmapBits(hBitmap, 2, Bits), 2);
    ok_long(GetLastError(), 0xDEADFACE);
    ok_int(Bits[0], 0xAA);
    ok_int(Bits[1], 0x33);
    ok_int(Bits[2], 0x55);

    FillMemory(Bits, sizeof(Bits), 0xAA);
    SetLastError(0xDEADFACE);
    ok_long(NtGdiSetBitmapBits(hBitmap, 2, Bits), 2);
    ok_long(GetLastError(), 0xDEADFACE);
    FillMemory(Bits, sizeof(Bits), 0x55);
    SetLastError(0xDEADFACE);
    ok_long(NtGdiGetBitmapBits(hBitmap, 2, Bits), 2);
    ok_long(GetLastError(), 0xDEADFACE);
    ok_int(Bits[0], 0xAA);
    ok_int(Bits[1], 0xAA);
    ok_int(Bits[2], 0x55);
    SetLastError(0xDEADFACE);
    ok_long(NtGdiGetBitmapBits(hBitmap, 3, Bits), 3);
    ok_long(GetLastError(), 0xDEADFACE);
    ok_int(Bits[0], 0xAA);
    ok_int(Bits[1], 0xAA);
    ok_int(Bits[2], 0x33);
    ok_int(Bits[3], 0x55);

    FillMemory(Bits, sizeof(Bits), 0xAA);
    SetLastError(0xDEADFACE);
    ok_long(NtGdiSetBitmapBits(hBitmap, 3, Bits), 3);
    ok_long(GetLastError(), 0xDEADFACE);
    FillMemory(Bits, sizeof(Bits), 0x55);
    SetLastError(0xDEADFACE);
    ok_long(NtGdiGetBitmapBits(hBitmap, 3, Bits), 3);
    ok_long(GetLastError(), 0xDEADFACE);
    ok_int(Bits[0], 0xAA);
    ok_int(Bits[1], 0xAA);
    ok_int(Bits[2], 0xAA);
    ok_int(Bits[3], 0x55);
    SetLastError(0xDEADFACE);
    ok_long(NtGdiGetBitmapBits(hBitmap, 4, Bits), 4);
    ok_long(GetLastError(), 0xDEADFACE);
    ok_int(Bits[0], 0xAA);
    ok_int(Bits[1], 0xAA);
    ok_int(Bits[2], 0xAA);
    ok_int(Bits[3], 0x33);
    ok_int(Bits[4], 0x55);

    ok_long(*(LPDWORD)pvBits, 0);
    ok_long(((LPDWORD)pvBits)[1], 0);

    DeleteObject(hBitmap);
    DeleteDC(hDC);

    /* ------------------------- */

    hDC = CreateCompatibleDC(NULL);

    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = 15;
    bmi.bmiHeader.biHeight = -15;   /* negative */
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    hBitmap = CreateDIBSection(hDC, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0);
    ok(hBitmap != NULL, "hBitmap was NULL.\n");

    FillMemory(Bits, sizeof(Bits), 0x55);
    SetLastError(0xDEADFACE);
    ok_long(NtGdiGetBitmapBits(hBitmap, 0, Bits), 0);
    ok_long(GetLastError(), 0xDEADFACE);
    ok_int(Bits[0], 0x55);

    FillMemory(Bits, sizeof(Bits), 0x55);
    SetLastError(0xDEADFACE);
    ok_long(NtGdiGetBitmapBits(hBitmap, 1, Bits), 1);
    ok_long(GetLastError(), 0xDEADFACE);
    ok_int(Bits[0], 0);
    ok_int(Bits[1], 0x55);

    FillMemory(Bits, sizeof(Bits), 0x55);
    SetLastError(0xDEADFACE);
    ok_long(NtGdiGetBitmapBits(hBitmap, 2, Bits), 2);
    ok_long(GetLastError(), 0xDEADFACE);
    ok_int(Bits[0], 0);
    ok_int(Bits[1], 0);
    ok_int(Bits[2], 0x55);

    FillMemory(Bits, sizeof(Bits), 0x33);
    SetLastError(0xDEADFACE);
    ok_long(NtGdiSetBitmapBits(hBitmap, 10, Bits), 10);
    ok_long(GetLastError(), 0xDEADFACE);

    FillMemory(Bits, sizeof(Bits), 0xAA);
    SetLastError(0xDEADFACE);
    ok_long(NtGdiSetBitmapBits(hBitmap, 1, Bits), 1);
    ok_long(GetLastError(), 0xDEADFACE);
    FillMemory(Bits, sizeof(Bits), 0x55);
    SetLastError(0xDEADFACE);
    ok_long(NtGdiGetBitmapBits(hBitmap, 1, Bits), 1);
    ok_long(GetLastError(), 0xDEADFACE);
    ok_int(Bits[0], 0xAA);
    ok_int(Bits[1], 0x55);
    SetLastError(0xDEADFACE);
    ok_long(NtGdiGetBitmapBits(hBitmap, 2, Bits), 2);
    ok_long(GetLastError(), 0xDEADFACE);
    ok_int(Bits[0], 0xAA);
    ok_int(Bits[1], 0x33);
    ok_int(Bits[2], 0x55);

    FillMemory(Bits, sizeof(Bits), 0xAA);
    SetLastError(0xDEADFACE);
    ok_long(NtGdiSetBitmapBits(hBitmap, 2, Bits), 2);
    ok_long(GetLastError(), 0xDEADFACE);
    FillMemory(Bits, sizeof(Bits), 0x55);
    SetLastError(0xDEADFACE);
    ok_long(NtGdiGetBitmapBits(hBitmap, 2, Bits), 2);
    ok_long(GetLastError(), 0xDEADFACE);
    ok_int(Bits[0], 0xAA);
    ok_int(Bits[1], 0xAA);
    ok_int(Bits[2], 0x55);
    SetLastError(0xDEADFACE);
    ok_long(NtGdiGetBitmapBits(hBitmap, 3, Bits), 3);
    ok_long(GetLastError(), 0xDEADFACE);
    ok_int(Bits[0], 0xAA);
    ok_int(Bits[1], 0xAA);
    ok_int(Bits[2], 0x33);
    ok_int(Bits[3], 0x55);

    FillMemory(Bits, sizeof(Bits), 0xAA);
    SetLastError(0xDEADFACE);
    ok_long(NtGdiSetBitmapBits(hBitmap, 3, Bits), 3);
    ok_long(GetLastError(), 0xDEADFACE);
    FillMemory(Bits, sizeof(Bits), 0x55);
    SetLastError(0xDEADFACE);
    ok_long(NtGdiGetBitmapBits(hBitmap, 3, Bits), 3);
    ok_long(GetLastError(), 0xDEADFACE);
    ok_int(Bits[0], 0xAA);
    ok_int(Bits[1], 0xAA);
    ok_int(Bits[2], 0xAA);
    ok_int(Bits[3], 0x55);
    SetLastError(0xDEADFACE);
    ok_long(NtGdiGetBitmapBits(hBitmap, 4, Bits), 4);
    ok_long(GetLastError(), 0xDEADFACE);
    ok_int(Bits[0], 0xAA);
    ok_int(Bits[1], 0xAA);
    ok_int(Bits[2], 0xAA);
    ok_int(Bits[3], 0x33);
    ok_int(Bits[4], 0x55);

    ok_long(*(LPDWORD)pvBits, 0x33AAAAAA);
    ok_long(((LPDWORD)pvBits)[1], 0x33333333);

    DeleteObject(hBitmap);
    DeleteDC(hDC);
}
