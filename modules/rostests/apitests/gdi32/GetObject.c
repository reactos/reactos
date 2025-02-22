/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for GetObject
 * PROGRAMMERS:     Timo Kreuzer
 */

#include "precomp.h"

#include <pseh/pseh2.h>

#define INVALID_POINTER ((PVOID)(ULONG_PTR)0xdeadbeefdeadbeefULL)

UCHAR src_mask[] = {
	  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	  0,0,0,0,0,0,255,0,0,0,0,0,0,0,0,0,
	  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,

	  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};


void
Test_General(void)
{
    struct
    {
        LOGBRUSH logbrush;
        BYTE additional[600];
    } TestStruct;
    PLOGBRUSH plogbrush;
    HBRUSH hBrush;
    HPEN hpen;
    INT ret;

    /* Test null pointer and invalid handles */
    SetLastError(ERROR_SUCCESS);
    ok(GetObjectA(0, 0, NULL) == 0, "\n");
    ok(GetObjectA((HANDLE)-1, 0, NULL) == 0, "\n");

    /* Test invalid habdles of different types */
    ok(GetObjectA((HANDLE)0x00380000, 0, NULL) == 0, "\n");
    ok(GetLastError() == ERROR_SUCCESS, "\n");
    ok(GetObjectA((HANDLE)0x00380000, 10, &TestStruct) == 0, "\n");
    ok(GetLastError() == ERROR_SUCCESS, "\n");
    SetLastError(ERROR_SUCCESS);
    ok(GetObjectA((HANDLE)0x00010000, 0, NULL) == 0, "\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "\n");
    SetLastError(ERROR_SUCCESS);
    ok(GetObjectA((HANDLE)0x00020000, 0, NULL) == 0, "\n");
    ok(GetLastError() == ERROR_SUCCESS, "\n");
    SetLastError(ERROR_SUCCESS);
    ok(GetObjectA((HANDLE)0x00030000, 0, NULL) == 0, "\n");
    ok(GetLastError() == ERROR_SUCCESS, "\n");
    SetLastError(ERROR_SUCCESS);
    ok(GetObjectA((HANDLE)0x00040000, 0, NULL) == 0, "\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "\n");
    SetLastError(ERROR_SUCCESS);
    ok(GetObjectA((HANDLE)0x00060000, 0, NULL) == 0, "\n");
    ok(GetLastError() == ERROR_SUCCESS, "\n");
    SetLastError(ERROR_SUCCESS);
    ok(GetObjectA((HANDLE)0x00070000, 0, NULL) == 0, "\n");
    ok(GetLastError() == ERROR_SUCCESS, "\n");
    SetLastError(ERROR_SUCCESS);
    ok(GetObjectA((HANDLE)0x000B0000, 0, NULL) == 0, "\n");
    ok(GetLastError() == ERROR_SUCCESS, "\n");
    SetLastError(ERROR_SUCCESS);
    ok(GetObjectA((HANDLE)0x000C0000, 0, NULL) == 0, "\n");
    ok(GetLastError() == ERROR_SUCCESS, "\n");
    SetLastError(ERROR_SUCCESS);
    ok(GetObjectA((HANDLE)0x000D0000, 0, NULL) == 0, "\n");
    ok(GetLastError() == ERROR_SUCCESS, "\n");
    SetLastError(ERROR_SUCCESS);
    ok(GetObjectA((HANDLE)0x000E0000, 0, NULL) == 0, "\n");
    ok(GetLastError() == ERROR_SUCCESS, "\n");
    SetLastError(ERROR_SUCCESS);
    ok(GetObjectA((HANDLE)0x000F0000, 0, NULL) == 0, "\n");
    ok(GetLastError() == ERROR_SUCCESS, "\n");
    SetLastError(ERROR_SUCCESS);
    ok(GetObjectA((HANDLE)0x00110000, 0, NULL) == 0, "\n");
    ok(GetLastError() == ERROR_SUCCESS, "\n");
    SetLastError(ERROR_SUCCESS);
    ok(GetObjectA((HANDLE)0x00120000, 0, NULL) == 0, "\n");
    ok(GetLastError() == ERROR_SUCCESS, "\n");
    SetLastError(ERROR_SUCCESS);
    ok(GetObjectA((HANDLE)0x00130000, 0, NULL) == 0, "\n");
    ok(GetLastError() == ERROR_SUCCESS, "\n");
    SetLastError(ERROR_SUCCESS);
    ok(GetObjectA((HANDLE)0x00140000, 0, NULL) == 0, "\n");
    ok(GetLastError() == ERROR_SUCCESS, "\n");
    SetLastError(ERROR_SUCCESS);
    ok(GetObjectA((HANDLE)0x00150000, 0, NULL) == 0, "\n");
    ok(GetLastError() == ERROR_SUCCESS, "\n");
    SetLastError(ERROR_SUCCESS);
    ok(GetObjectA((HANDLE)0x00160000, 0, NULL) == 0, "\n");
    ok(GetLastError() == ERROR_SUCCESS, "\n");
    SetLastError(ERROR_SUCCESS);
    ok(GetObjectA((HANDLE)GDI_OBJECT_TYPE_DC, 0, NULL) == 0, "\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "\n");
    SetLastError(ERROR_SUCCESS);
    ok(GetObjectW((HANDLE)GDI_OBJECT_TYPE_DC, 0, NULL) == 0, "\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "\n");
    SetLastError(ERROR_SUCCESS);
    ok(GetObjectA((HANDLE)GDI_OBJECT_TYPE_REGION, 0, NULL) == 0, "\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "\n");
    SetLastError(ERROR_SUCCESS);
    ok(GetObjectW((HANDLE)GDI_OBJECT_TYPE_REGION, 0, NULL) == 0, "\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "\n");
    SetLastError(ERROR_SUCCESS);
    ok(GetObjectA((HANDLE)GDI_OBJECT_TYPE_EMF, 0, NULL) == 0, "\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "\n");
    SetLastError(ERROR_SUCCESS);
    ok(GetObjectW((HANDLE)GDI_OBJECT_TYPE_EMF, 0, NULL) == 0, "\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "\n");
    SetLastError(ERROR_SUCCESS);
    ok(GetObjectA((HANDLE)GDI_OBJECT_TYPE_METAFILE, 0, NULL) == 0, "\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "\n");
    SetLastError(ERROR_SUCCESS);
    ok(GetObjectW((HANDLE)GDI_OBJECT_TYPE_METAFILE, 0, NULL) == 0, "\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "\n");
    SetLastError(ERROR_SUCCESS);
    ok(GetObjectA((HANDLE)GDI_OBJECT_TYPE_ENHMETAFILE, 0, NULL) == 0, "\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "\n");
    SetLastError(ERROR_SUCCESS);
    ok(GetObjectW((HANDLE)GDI_OBJECT_TYPE_ENHMETAFILE, 0, NULL) == 0, "\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "\n");

    /* Test need of alignment */
    SetLastError(ERROR_SUCCESS);
    hBrush = GetStockObject(WHITE_BRUSH);
    plogbrush = (PVOID)((ULONG_PTR)&TestStruct.logbrush);
    ok(GetObject(hBrush, sizeof(LOGBRUSH), plogbrush) == sizeof(LOGBRUSH), "\n");
    plogbrush = (PVOID)((ULONG_PTR)&TestStruct.logbrush + 2);
    ok(GetObject(hBrush, sizeof(LOGBRUSH), plogbrush) == sizeof(LOGBRUSH), "\n");
    plogbrush = (PVOID)((ULONG_PTR)&TestStruct.logbrush + 1);
    //ok(GetObject(hBrush, sizeof(LOGBRUSH), plogbrush) == 0, "\n"); // fails on win7

    /* Test invalid buffer */
    SetLastError(0xbadbad00);
    ok(GetObjectA(GetStockObject(WHITE_BRUSH), sizeof(LOGBRUSH), INVALID_POINTER) == 0, "\n");
    ok((GetLastError() == 0xbadbad00) || (GetLastError() == ERROR_NOACCESS), "wrong error: %ld\n", GetLastError());
    SetLastError(0xbadbad00);
    ok(GetObjectW(GetStockObject(BLACK_PEN), sizeof(LOGPEN), INVALID_POINTER) == 0, "\n");
    ok((GetLastError() == 0xbadbad00) || (GetLastError() == ERROR_NOACCESS), "wrong error: %ld\n", GetLastError());
    SetLastError(0xbadbad00);
    ok(GetObjectW(GetStockObject(21), sizeof(BITMAP), INVALID_POINTER) == 0, "\n");
    ok((GetLastError() == 0xbadbad00) || (GetLastError() == ERROR_NOACCESS), "wrong error: %ld\n", GetLastError());
    SetLastError(0xbadbad00);
    ok(GetObjectW(GetStockObject(SYSTEM_FONT), sizeof(LOGFONT), INVALID_POINTER) == 0, "\n");
    ok(GetLastError() == 0xbadbad00, "wrong error: %ld\n", GetLastError());
    SetLastError(ERROR_SUCCESS);
    _SEH2_TRY
    {
        ret = GetObjectA(GetStockObject(SYSTEM_FONT), sizeof(LOGFONT), INVALID_POINTER);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = -1;
    }
    _SEH2_END
    ok(ret == -1, "should have got an exception\n");

    ok(GetObjectW(GetStockObject(SYSTEM_FONT), 0x50000000, &TestStruct) == 356, "\n");
    ok(GetObjectW(GetStockObject(WHITE_BRUSH), 0x50000000, &TestStruct) == sizeof(LOGBRUSH), "\n");


    /* Test buffer size of 0 */
    SetLastError(0xbadbad00);
    hBrush = CreateSolidBrush(123);
    ok(hBrush != NULL, "Failed to create brush\n");
    ok_long(GetObjectA(hBrush, 0, &TestStruct), 0);
    ok_err(0xbadbad00);
    DeleteObject(hBrush);
    SetLastError(0xbadbad00);
    hpen = CreatePen(PS_SOLID, 1, 123);
    ok(hpen != NULL, "Failed to create pen\n");
    ok_long(GetObjectA(hpen, 0, &TestStruct), 0);
    ok((GetLastError() == 0xbadbad00) || (GetLastError() == ERROR_NOACCESS), "wrong error: %ld\n", GetLastError());
    SetLastError(0xbadbad00);
    TestStruct.logbrush.lbStyle = BS_SOLID;
    TestStruct.logbrush.lbColor = RGB(1,2,3);
    TestStruct.logbrush.lbHatch = 0;
    hpen = ExtCreatePen(PS_GEOMETRIC | PS_SOLID, 1, &TestStruct.logbrush, 0, NULL);
    ok(hpen != NULL, "Failed to create pen\n");
    ok_long(GetObjectA(hpen, 0, &TestStruct), 0);
    ok((GetLastError() == 0xbadbad00) || (GetLastError() == ERROR_NOACCESS), "wrong error: %ld\n", GetLastError());
    SetLastError(0xbadbad00);
    ok(GetObjectW(GetStockObject(SYSTEM_FONT), 0, &TestStruct) == 0, "\n");
    ok_err(0xbadbad00);
    SetLastError(0xbadbad00);
    ok(GetObjectW(GetStockObject(21), 0, &TestStruct) == 0, "\n");
    ok_err(0xbadbad00);

}

void
Test_Bitmap(void)
{
    HBITMAP hBitmap;
    BITMAP bitmap;
    DIBSECTION dibsection;
    BYTE bData[100] = {0};
    BYTE Buffer[100] = {48,48,48,48,48,48,48,48,48,48,48,48,48,48,48,0};

    FillMemory(&bitmap, sizeof(BITMAP), 0x77);
    hBitmap = CreateBitmap(10,10,1,8,bData);
    ok(hBitmap != 0, "CreateBitmap failed, skipping tests.\n");
    if (!hBitmap) return;

    SetLastError(ERROR_SUCCESS);
    ok(GetObjectA((HANDLE)GDI_OBJECT_TYPE_BITMAP, 0, NULL) == sizeof(BITMAP), "\n");
    ok(GetObjectW((HANDLE)GDI_OBJECT_TYPE_BITMAP, 0, NULL) == sizeof(BITMAP), "\n");
    ok(GetObjectA((HANDLE)GDI_OBJECT_TYPE_BITMAP, sizeof(BITMAP), NULL) == sizeof(BITMAP), "\n");
    ok(GetLastError() == ERROR_SUCCESS, "\n");
    ok(GetObjectA(hBitmap, sizeof(DIBSECTION), NULL) == sizeof(BITMAP), "\n");
    ok(GetObjectA(hBitmap, 0, NULL) == sizeof(BITMAP), "\n");
    ok(GetObjectA((HANDLE)((UINT_PTR)hBitmap & 0x0000ffff), 0, NULL) == sizeof(BITMAP), "\n");
    ok(GetObjectW((HANDLE)((UINT_PTR)hBitmap & 0x0000ffff), 0, NULL) == sizeof(BITMAP), "\n");
    ok(GetLastError() == ERROR_SUCCESS, "\n");
    ok(GetObjectA(hBitmap, 5, NULL) == sizeof(BITMAP), "\n");
    ok(GetObjectA(hBitmap, -5, NULL) == sizeof(BITMAP), "\n");
    ok(GetObjectA(hBitmap, 0, Buffer) == 0, "\n");
    ok(GetObjectA(hBitmap, 5, Buffer) == 0, "\n");
    ok(GetLastError() == ERROR_SUCCESS, "\n");
    ok(GetObjectA(hBitmap, sizeof(BITMAP), &bitmap) == sizeof(BITMAP), "\n");
    ok(GetObjectA(hBitmap, sizeof(BITMAP)+2, &bitmap) == sizeof(BITMAP), "\n");
    ok(GetObjectA(hBitmap, sizeof(DIBSECTION), &dibsection) == sizeof(BITMAP), "\n");
    ok(GetObjectA(hBitmap, -5, &bitmap) == sizeof(BITMAP), "\n");
    ok(GetLastError() == ERROR_SUCCESS, "\n");
    ok(GetObjectA((HANDLE)GDI_OBJECT_TYPE_BITMAP, sizeof(BITMAP), &bitmap) == 0, "\n");
    ok(GetLastError() == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", GetLastError());

    // todo: test invalid handle + buffer

    DeleteObject(hBitmap);
}

void
Test_Dibsection(void)
{
    BITMAPINFO bmi = {{sizeof(BITMAPINFOHEADER), 10, 9, 1, 16, BI_RGB, 0, 10, 10, 0,0}};
    HBITMAP hBitmap;
    BITMAP bitmap;
    DIBSECTION dibsection;
    PVOID pData;
    HDC hDC;

    FillMemory(&dibsection, sizeof(DIBSECTION), 0x77);
    hDC = GetDC(0);
    hBitmap = CreateDIBSection(hDC, &bmi, DIB_RGB_COLORS, &pData, NULL, 0);
    ok(hBitmap != 0, "CreateDIBSection failed with %ld, skipping tests.\n", GetLastError());
    if (!hBitmap) return;

    ok(GetObjectA((HANDLE)((UINT_PTR)hBitmap & 0x0000ffff), 0, NULL) == sizeof(BITMAP), "\n");
    ok(GetObjectW((HANDLE)((UINT_PTR)hBitmap & 0x0000ffff), 0, NULL) == sizeof(BITMAP), "\n");

    SetLastError(ERROR_SUCCESS);
    ok_long(GetObject(hBitmap, sizeof(DIBSECTION), NULL), sizeof(BITMAP));
    ok_long(GetObject(hBitmap, 0, NULL), sizeof(BITMAP));
    ok_long(GetObject(hBitmap, 5, NULL), sizeof(BITMAP));
    ok_long(GetObject(hBitmap, -5, NULL), sizeof(BITMAP));
    ok_long(GetObject(hBitmap, 0, &dibsection), 0);
    ok_long(GetObject(hBitmap, 5, &dibsection), 0);
    ok_long(GetObject(hBitmap, sizeof(BITMAP), &bitmap), sizeof(BITMAP));
    ok_long(GetObject(hBitmap, sizeof(BITMAP)+2, &bitmap), sizeof(BITMAP));
    ok_long(bitmap.bmType, 0);
    ok_long(bitmap.bmWidth, 10);
    ok_long(bitmap.bmHeight, 9);
    ok_long(bitmap.bmWidthBytes, 20);
    ok_long(bitmap.bmPlanes, 1);
    ok_long(bitmap.bmBitsPixel, 16);
    ok(bitmap.bmBits == pData, "\n");
    ok_long(GetObject(hBitmap, sizeof(DIBSECTION), &dibsection), sizeof(DIBSECTION));
    ok_long(GetObject(hBitmap, sizeof(DIBSECTION)+2, &dibsection), sizeof(DIBSECTION));
    ok_long(GetObject(hBitmap, -5, &dibsection), sizeof(DIBSECTION));
    ok_err(ERROR_SUCCESS);
    DeleteObject(hBitmap);
    ReleaseDC(0, hDC);
}

void
Test_Palette(void)
{
    LOGPALETTE logpal;
    HPALETTE hPalette;
    WORD wPalette;

    FillMemory(&wPalette, sizeof(WORD), 0x77);
    logpal.palVersion = 0x0300;
    logpal.palNumEntries = 1;
    logpal.palPalEntry[0].peRed = 0;
    logpal.palPalEntry[0].peGreen = 0;
    logpal.palPalEntry[0].peBlue = 0;
    logpal.palPalEntry[0].peFlags = PC_EXPLICIT;
    hPalette = CreatePalette(&logpal);
    ok(hPalette != 0, "CreatePalette failed, skipping tests.\n");
    if (!hPalette) return;

    ok(GetObjectA((HANDLE)((UINT_PTR)hPalette & 0x0000ffff), 0, NULL) == sizeof(WORD), "\n");
    ok(GetObjectW((HANDLE)((UINT_PTR)hPalette & 0x0000ffff), 0, NULL) == sizeof(WORD), "\n");

    SetLastError(ERROR_SUCCESS);
    ok(GetObjectA((HANDLE)GDI_OBJECT_TYPE_PALETTE, 0, NULL) == sizeof(WORD), "\n");
    ok(GetObjectW((HANDLE)GDI_OBJECT_TYPE_PALETTE, 0, NULL) == sizeof(WORD), "\n");
    ok(GetObject(hPalette, sizeof(WORD), NULL) == sizeof(WORD), "\n");
    ok(GetObject(hPalette, 0, NULL) == sizeof(WORD), "\n");
    ok(GetObject(hPalette, 5, NULL) == sizeof(WORD), "\n");
    ok(GetObject(hPalette, -5, NULL) == sizeof(WORD), "\n");
    ok(GetObject(hPalette, sizeof(WORD), &wPalette) == sizeof(WORD), "\n");
    ok(GetObject(hPalette, sizeof(WORD)+2, &wPalette) == sizeof(WORD), "\n");
    ok(GetObject(hPalette, 0, &wPalette) == 0, "\n");
    ok(GetObject(hPalette, 1, &wPalette) == 0, "\n");
    ok(GetObject(hPalette, -1, &wPalette) == sizeof(WORD), "\n");
    ok(GetLastError() == ERROR_SUCCESS, "\n");
    DeleteObject(hPalette);
    ok(GetObjectW((HANDLE)GDI_OBJECT_TYPE_PALETTE, sizeof(WORD), &wPalette) == 0, "\n");
    ok(GetLastError() == ERROR_SUCCESS, "\n");

}

void
Test_Brush(void)
{
    LOGBRUSH logbrush;
    HBRUSH hBrush;

    FillMemory(&logbrush, sizeof(LOGBRUSH), 0x77);
    hBrush = CreateSolidBrush(RGB(1,2,3));
    ok(hBrush != 0, "CreateSolidBrush failed, skipping tests.\n");
    if (!hBrush) return;

    SetLastError(ERROR_SUCCESS);
    ok(GetObjectA((HANDLE)GDI_OBJECT_TYPE_BRUSH, 0, NULL) == sizeof(LOGBRUSH), "\n");
    ok(GetObjectW((HANDLE)GDI_OBJECT_TYPE_BRUSH, 0, NULL) == sizeof(LOGBRUSH), "\n");
    ok(GetObject(hBrush, sizeof(WORD), NULL) == sizeof(LOGBRUSH), "\n");
    ok(GetObject(hBrush, 0, NULL) == sizeof(LOGBRUSH), "\n");
    ok(GetObject(hBrush, 5, NULL) == sizeof(LOGBRUSH), "\n");
    ok(GetObject(hBrush, -5, NULL) == sizeof(LOGBRUSH), "\n");

    //ok(GetObject(hBrush, 0, &logbrush) == 0, "\n"); fails on win7
    ok(logbrush.lbStyle == 0x77777777, "\n");
    ok(GetObject(hBrush, 5, &logbrush) == sizeof(LOGBRUSH), "\n");
    ok(logbrush.lbStyle == 0, "\n");
    ok(logbrush.lbColor == 0x77777701, "\n");

    ok(GetObject(hBrush, sizeof(LOGBRUSH), &logbrush) == sizeof(LOGBRUSH), "\n");
    ok(GetObject(hBrush, sizeof(LOGBRUSH) - 1, &logbrush) == sizeof(LOGBRUSH), "\n");
    ok(GetObject(hBrush, 1, &logbrush) == sizeof(LOGBRUSH), "\n");
    ok(GetObject(hBrush, sizeof(LOGBRUSH)+2, &logbrush) == sizeof(LOGBRUSH), "\n");
    ok(GetObject(hBrush, -1, &logbrush) == sizeof(LOGBRUSH), "\n");
    // TODO: test all members

    ok(GetLastError() == ERROR_SUCCESS, "\n");
    DeleteObject(hBrush);

    ok(GetObjectW((HANDLE)GDI_OBJECT_TYPE_BRUSH, sizeof(LOGBRUSH), &logbrush) == 0, "\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    /* Test handle fixup */
    hBrush = (HBRUSH)((ULONG_PTR)GetStockObject(WHITE_BRUSH) & 0xFFFF);
    ok(GetObjectW(hBrush, sizeof(LOGBRUSH), &logbrush) == sizeof(LOGBRUSH),
       "GetObject(0x%p, ...) failed.\n", hBrush);

#ifdef _WIN64
    /* Test upper 32 bits */
    hBrush = (HBRUSH)((ULONG64)GetStockObject(WHITE_BRUSH) | 0xFFFFFFFF00000000ULL);
    ok(GetObjectW(hBrush, sizeof(LOGBRUSH), &logbrush) == sizeof(LOGBRUSH),
       "GetObject(0x%p, ...) failed.\n", hBrush);
    hBrush = (HBRUSH)((ULONG64)GetStockObject(WHITE_BRUSH) | 0x537F9F2F00000000ULL);
    ok(GetObjectW(hBrush, sizeof(LOGBRUSH), &logbrush) == sizeof(LOGBRUSH),
       "GetObject(0x%p, ...) failed.\n", hBrush);
#endif
}

void
Test_DIBBrush(void)
{
    struct
    {
        BITMAPINFOHEADER bmiHeader;
        WORD wColors[4];
        BYTE jBuffer[16];
    } PackedDIB =
    {
        {sizeof(BITMAPINFOHEADER), 4, 4, 1, 8, BI_RGB, 0, 1, 1, 4, 0},
        {1, 7, 3, 1},
        {0,1,2,3,  1,2,3,0,  2,3,0,1,  3,0,1,2},
    };
    LOGBRUSH logbrush;
    HBRUSH hBrush;

    /* Create a DIB brush */
    hBrush = CreateDIBPatternBrushPt(&PackedDIB, DIB_PAL_COLORS);
    ok(hBrush != 0, "CreateSolidBrush failed, skipping tests.\n");
    if (!hBrush) return;

    FillMemory(&logbrush, sizeof(LOGBRUSH), 0x77);
    SetLastError(ERROR_SUCCESS);

    ok_long(GetObject(hBrush, sizeof(LOGBRUSH), &logbrush), sizeof(LOGBRUSH));
    ok_long(logbrush.lbStyle, BS_DIBPATTERN);
    ok_long(logbrush.lbColor, 0);
    ok_long(logbrush.lbHatch, (ULONG_PTR)&PackedDIB);

    ok_err(ERROR_SUCCESS);
    DeleteObject(hBrush);


    /* Create a DIB brush with undocumented iUsage 2 */
    hBrush = CreateDIBPatternBrushPt(&PackedDIB, 2);
    ok(hBrush != 0, "CreateSolidBrush failed, skipping tests.\n");
    if (!hBrush) return;

    FillMemory(&logbrush, sizeof(LOGBRUSH), 0x77);
    SetLastError(ERROR_SUCCESS);

    ok_long(GetObject(hBrush, sizeof(LOGBRUSH), &logbrush), sizeof(LOGBRUSH));
    ok_long(logbrush.lbStyle, BS_DIBPATTERN);
    ok_long(logbrush.lbColor, 0);
    ok_long(logbrush.lbHatch, (ULONG_PTR)&PackedDIB);

    ok_err(ERROR_SUCCESS);
    DeleteObject(hBrush);

}

void
Test_Pen(void)
{
    LOGPEN logpen;
    HPEN hPen;

    FillMemory(&logpen, sizeof(LOGPEN), 0x77);
    hPen = CreatePen(PS_SOLID, 3, RGB(4,5,6));
    ok(hPen != 0, "CreatePen failed, skipping tests.\n");
    if (!hPen) return;
    SetLastError(ERROR_SUCCESS);
    ok(GetObjectA((HANDLE)GDI_OBJECT_TYPE_PEN, 0, NULL) == sizeof(LOGPEN), "\n");
    ok(GetObjectW((HANDLE)GDI_OBJECT_TYPE_PEN, 0, NULL) == sizeof(LOGPEN), "\n");
    ok(GetObject(hPen, sizeof(BITMAP), NULL) == sizeof(LOGPEN), "\n");
    ok(GetObject(hPen, 0, NULL) == sizeof(LOGPEN), "\n");
    ok(GetObject(hPen, 5, NULL) == sizeof(LOGPEN), "\n");
    ok(GetObject(hPen, -5, NULL) == sizeof(LOGPEN), "\n");
    ok(GetObject(hPen, sizeof(LOGPEN), &logpen) == sizeof(LOGPEN), "\n");
    ok(GetObject(hPen, sizeof(LOGPEN)-1, &logpen) == 0, "\n");
    ok(GetObject(hPen, sizeof(LOGPEN)+2, &logpen) == sizeof(LOGPEN), "\n");
    ok(GetObject(hPen, 0, &logpen) == 0, "\n");
    ok(GetObject(hPen, -5, &logpen) == sizeof(LOGPEN), "\n");
    //ok(GetLastError() == ERROR_SUCCESS, "\n"); fails on win7

    /* test if the fields are filled correctly */
    ok(logpen.lopnStyle == PS_SOLID, "\n");

    ok(GetObjectW((HANDLE)GDI_OBJECT_TYPE_PEN, sizeof(LOGPEN), &logpen) == 0, "\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    DeleteObject(hPen);
}

void
Test_ExtPen(void)
{
    HPEN hPen;
    EXTLOGPEN extlogpen;
    LOGBRUSH logbrush;
    DWORD dwStyles[17] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
    struct
    {
        EXTLOGPEN extlogpen;
        DWORD dwStyles[50];
    } elpUserStyle;
    int i;

    SetLastError(ERROR_SUCCESS);
    ok(GetObjectA((HANDLE)GDI_OBJECT_TYPE_EXTPEN, 0, NULL) == 0, "\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "\n");
    SetLastError(ERROR_SUCCESS);
    ok(GetObjectW((HANDLE)GDI_OBJECT_TYPE_EXTPEN, 0, NULL) == 0, "\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "\n");

    FillMemory(&extlogpen, sizeof(EXTLOGPEN), 0x77);
    logbrush.lbStyle = BS_SOLID;
    logbrush.lbColor = RGB(1,2,3);
    logbrush.lbHatch = 22;
    hPen = ExtCreatePen(PS_GEOMETRIC | PS_DASH, 5, &logbrush, 0, NULL);

    SetLastError(ERROR_SUCCESS);
    ok(GDI_HANDLE_GET_TYPE(hPen) == GDI_OBJECT_TYPE_EXTPEN, "\n");
    ok(GetObject(hPen, sizeof(EXTLOGPEN), NULL) == sizeof(EXTLOGPEN)-sizeof(DWORD), "\n");
    ok(GetObject(hPen, 0, NULL) == sizeof(EXTLOGPEN)-sizeof(DWORD), "\n");
    ok(GetObject((HANDLE)GDI_HANDLE_GET_INDEX(hPen), 0, NULL) == sizeof(EXTLOGPEN)-sizeof(DWORD), "\n");
    ok(GetObject(hPen, 5, NULL) == sizeof(EXTLOGPEN)-sizeof(DWORD), "\n");
    ok(GetObject(hPen, -5, NULL) == sizeof(EXTLOGPEN)-sizeof(DWORD), "\n");
    ok(GetLastError() == ERROR_SUCCESS, "\n");
    ok(GetObject((HANDLE)GDI_OBJECT_TYPE_EXTPEN, 0, NULL) == 0, "\n");
    ok(GetLastError() ==  ERROR_INVALID_PARAMETER, "got %ld\n", GetLastError());
    SetLastError(0xbadbad00);
    ok(GetObject(hPen, 0, &extlogpen) == 0, "\n");
    ok((GetLastError() == 0xbadbad00) || (GetLastError() == ERROR_NOACCESS), "wrong error: %ld\n", GetLastError());
    SetLastError(ERROR_SUCCESS);
    ok(GetObject(hPen, 4, &extlogpen) == 0, "\n");
    ok(GetLastError() == ERROR_SUCCESS, "got %ld\n", GetLastError());
    SetLastError(ERROR_SUCCESS);
    ok(GetObject((HANDLE)GDI_OBJECT_TYPE_EXTPEN, 0, &extlogpen) == 0, "\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "got %ld\n", GetLastError());
    SetLastError(ERROR_SUCCESS);
    ok(GetObject((HANDLE)GDI_OBJECT_TYPE_EXTPEN, 4, &extlogpen) == 0, "\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "got %ld\n", GetLastError());
    SetLastError(ERROR_SUCCESS);
    ok(GetObject(hPen,  sizeof(EXTLOGPEN) - 5, &extlogpen) == 0, "\n");
    ok(GetLastError() == ERROR_SUCCESS, "got %ld\n", GetLastError());

    /* Nothing should be filled */
    ok(extlogpen.elpPenStyle == 0x77777777, "\n");
    ok(extlogpen.elpWidth == 0x77777777, "\n");

    ok(GetObject(hPen, sizeof(EXTLOGPEN), &extlogpen) == sizeof(EXTLOGPEN)-sizeof(DWORD), "\n");
    ok(GetObject(hPen, sizeof(EXTLOGPEN)-sizeof(DWORD), &extlogpen) == sizeof(EXTLOGPEN)-sizeof(DWORD), "\n");
    ok(GetObject(hPen, sizeof(EXTLOGPEN)-sizeof(DWORD)-1, &extlogpen) == 0, "\n");
    ok(GetObject(hPen, sizeof(EXTLOGPEN)+2, &extlogpen) == sizeof(EXTLOGPEN)-sizeof(DWORD), "\n");
    ok(GetObject(hPen, -5, &extlogpen) == sizeof(EXTLOGPEN)-sizeof(DWORD), "\n");

    /* test if the fields are filled correctly */
    ok(extlogpen.elpPenStyle == (PS_GEOMETRIC | PS_DASH), "\n");
    ok(extlogpen.elpWidth == 5, "\n");
    ok(extlogpen.elpBrushStyle == 0, "\n");
    ok(extlogpen.elpColor == RGB(1,2,3), "\n");
    ok(extlogpen.elpHatch == 22, "\n");
    ok(extlogpen.elpNumEntries == 0, "\n");
    DeleteObject(hPen);

    /* A maximum of 16 Styles is allowed */
    hPen = ExtCreatePen(PS_GEOMETRIC | PS_USERSTYLE, 5, &logbrush, 16, (CONST DWORD*)&dwStyles);
    ok(GetObject(hPen, 0, NULL) == sizeof(EXTLOGPEN) + 15*sizeof(DWORD), "\n");
    ok(GetObject(hPen, sizeof(EXTLOGPEN) + 15*sizeof(DWORD), &elpUserStyle) == sizeof(EXTLOGPEN) + 15*sizeof(DWORD), "\n");
    for (i = 0; i <= 15; i++)
        ok(((EXTLOGPEN*)&elpUserStyle)->elpStyleEntry[i] == i, "%d\n", i);
    DeleteObject(hPen);
}

void
Test_Font(void)
{
    HFONT hFont;
    union
    {
        LOGFONTA logfonta;
        LOGFONTW logfontw;
        EXTLOGFONTA extlogfonta;
        EXTLOGFONTW extlogfontw;
        ENUMLOGFONTEXA enumlogfontexa;
        ENUMLOGFONTEXW enumlogfontexw;
        ENUMLOGFONTEXDVA enumlogfontexdva;
        ENUMLOGFONTEXDVW enumlogfontexdvw;
        ENUMLOGFONTA enumlogfonta;
        ENUMLOGFONTW enumlogfontw;
        BYTE bData[270];
    } u;
    int ret;

    FillMemory(&u, sizeof(u), 0x77);
    hFont = CreateFontA(8, 8, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS,
        ANTIALIASED_QUALITY, DEFAULT_PITCH, "testfont");
    ok(hFont != 0, "\n");

    SetLastError(ERROR_SUCCESS);
    ok(GetObjectA((HANDLE)GDI_OBJECT_TYPE_FONT, 0, NULL) == sizeof(LOGFONTA), "\n");
    ok(GetObjectW((HANDLE)GDI_OBJECT_TYPE_FONT, 0, NULL) == sizeof(LOGFONTW), "\n");
    ok(GetObjectA((HANDLE)GDI_OBJECT_TYPE_FONT, sizeof(LOGFONTA), NULL) == sizeof(LOGFONTA), "\n"); // 60
    ok(GetObjectA((HANDLE)GDI_OBJECT_TYPE_FONT, sizeof(ENUMLOGFONTA), NULL) == sizeof(LOGFONTA), "\n"); // 156
    ok(GetObjectA((HANDLE)GDI_OBJECT_TYPE_FONT, sizeof(ENUMLOGFONTEXA), NULL) == sizeof(LOGFONTA), "\n"); // 188
    ok(GetObjectA((HANDLE)GDI_OBJECT_TYPE_FONT, sizeof(EXTLOGFONTA), NULL) == sizeof(LOGFONTA), "\n"); // 192
    ok(GetObjectA((HANDLE)GDI_OBJECT_TYPE_FONT, sizeof(ENUMLOGFONTEXDVA), NULL) == sizeof(LOGFONTA), "\n"); // 260
    ok(GetObjectA((HANDLE)GDI_OBJECT_TYPE_FONT, sizeof(ENUMLOGFONTEXDVA)+1, NULL) == sizeof(LOGFONTA), "\n"); // 260
    ok(GetObjectW((HANDLE)GDI_OBJECT_TYPE_FONT, sizeof(LOGFONTW), NULL) == sizeof(LOGFONTW), "\n"); // 92
    ok(GetObjectW((HANDLE)GDI_OBJECT_TYPE_FONT, sizeof(ENUMLOGFONTW), NULL) == sizeof(LOGFONTW), "\n"); // 284
    ok(GetObjectW((HANDLE)GDI_OBJECT_TYPE_FONT, sizeof(EXTLOGFONTW), NULL) == sizeof(LOGFONTW), "\n"); // 320
    ok(GetObjectW((HANDLE)GDI_OBJECT_TYPE_FONT, sizeof(ENUMLOGFONTEXW), NULL) == sizeof(LOGFONTW), "\n"); // 348
    ok(GetObjectW((HANDLE)GDI_OBJECT_TYPE_FONT, sizeof(ENUMLOGFONTEXDVW), NULL) == sizeof(LOGFONTW), "\n"); // 420
    ok(GetObjectW((HANDLE)GDI_OBJECT_TYPE_FONT, sizeof(ENUMLOGFONTEXDVW)+1, NULL) == sizeof(LOGFONTW), "\n"); // 356!
    ok(GetLastError() == ERROR_SUCCESS, "got %ld\n", GetLastError());

    ok(GetObjectA(hFont, sizeof(LOGFONTA), NULL) == sizeof(LOGFONTA), "\n");
    ok(GetObjectA(hFont, 0, NULL) == sizeof(LOGFONTA), "\n");
    ok(GetObjectA(hFont, 5, NULL) == sizeof(LOGFONTA), "\n");
    ok(GetObjectA(hFont, -5, NULL) == sizeof(LOGFONTA), "\n");
    ok(GetObjectA(hFont, 0, &u.logfonta) == 0, "\n");
    ok(u.logfonta.lfHeight == 0x77777777, "\n");

    ok(GetObjectA(hFont, 5, &u.logfonta) == 5, "\n");
    ok(u.logfonta.lfHeight == 8, "\n");
    ok(u.logfonta.lfWidth == 0x77777708, "\n");

    ok(GetObjectA(hFont, 0, &u.logfonta) == 0, "\n");
    ok(GetObjectA(hFont, -1, &u.logfonta) == sizeof(ENUMLOGFONTEXDVA), "\n");
    ok(GetObjectA(hFont, 1, &u.logfonta) == 1, "\n"); // 1 -> 1
    ok(GetObjectA(hFont, sizeof(LOGFONTA) - 1, &u.logfonta) == sizeof(LOGFONTA) - 1, "\n"); // 59 -> 59
    ok(GetObjectA(hFont, sizeof(LOGFONTA), &u.logfonta) == sizeof(LOGFONTA), "\n"); // 60 -> 60
    ok(GetObjectA(hFont, sizeof(LOGFONTA) + 1, &u.logfonta) == sizeof(LOGFONTA) + 1, "\n"); // 61 -> 61
    ok(GetObjectA(hFont, sizeof(LOGFONTW) - 1, &u.logfontw) == sizeof(LOGFONTW) - 1, "\n"); // 91 -> 91
    ok(GetObjectA(hFont, sizeof(LOGFONTW), &u.logfontw) == sizeof(LOGFONTA), "\n"); // 92 -> 60
    ok(GetObjectA(hFont, sizeof(LOGFONTW) + 1, &u.logfontw) == sizeof(LOGFONTW) + 1, "\n"); // 93 -> 93
    ok(GetObjectA(hFont, sizeof(EXTLOGFONTA), &u.extlogfonta) == sizeof(EXTLOGFONTA), "\n"); // 192 -> 192
    ok(GetObjectA(hFont, sizeof(EXTLOGFONTA)+1, &u.extlogfonta) == sizeof(EXTLOGFONTA)+1, "\n"); // 192+1 -> 192+1
    ok(GetObjectA(hFont, sizeof(EXTLOGFONTA)+16*4, &u.extlogfonta) == sizeof(EXTLOGFONTA)+16*4, "\n"); // 192+1 -> 192+1
    ok(GetObjectA(hFont, sizeof(EXTLOGFONTW), &u.extlogfontw) == sizeof(ENUMLOGFONTEXDVA), "\n"); // 320 -> 260
    ok(GetObjectA(hFont, 261, &u.bData) == 260, "\n"); // no

    ok(GetObjectA(hFont, sizeof(ENUMLOGFONTEXDVA) - 1, &u.enumlogfontexdvw) == sizeof(ENUMLOGFONTEXDVA) - 1, "\n"); // 419
    ok(GetObjectA(hFont, sizeof(ENUMLOGFONTEXDVA), &u.enumlogfontexdvw) == sizeof(ENUMLOGFONTEXDVA), "\n"); // 420
    ok(GetObjectA(hFont, sizeof(ENUMLOGFONTEXDVA)+1, &u.enumlogfontexdvw) == sizeof(ENUMLOGFONTEXDVA), "\n"); // 356!

    /* LOGFONT / GetObjectW */
    FillMemory(&u.logfontw, sizeof(LOGFONTW), 0x77);

    ok(GetObjectW(hFont, sizeof(LOGFONTW), NULL) == sizeof(LOGFONTW), "1\n");
    ok(GetObjectW(hFont, 0, NULL) == sizeof(LOGFONTW), "\n");
    ok(GetObjectW(hFont, 5, NULL) == sizeof(LOGFONTW), "\n");
    ok(GetObjectW(hFont, sizeof(ENUMLOGFONTEXA), NULL) == sizeof(LOGFONTW), "\n");
    ok(GetObjectW(hFont, -5, NULL) == sizeof(LOGFONTW), "\n");
    ok(GetObjectW(hFont, 0, &u.logfontw) == 0, "\n");
    ok(u.logfontw.lfHeight == 0x77777777, "\n");

    ok(GetObjectW(hFont, 5, &u.logfontw) == 5, "\n");
    ok(u.logfontw.lfHeight == 8, "\n");
    ok(u.logfontw.lfWidth == 0x77777708, "\n");

    ok(GetObjectA(hFont, sizeof(LOGFONTA), &u.logfonta) == sizeof(LOGFONTA), "\n"); // 60
    ok(u.logfonta.lfHeight == 8, "\n");
    ok(GetObjectA(hFont, sizeof(ENUMLOGFONTA), &u.enumlogfonta) == sizeof(ENUMLOGFONTA), "\n"); // 156
    ok(GetObjectA(hFont, sizeof(ENUMLOGFONTEXA), &u.enumlogfontexa) == sizeof(ENUMLOGFONTEXA), "\n"); // 188
    ok(GetObjectA(hFont, sizeof(EXTLOGFONTA), &u.extlogfonta) == sizeof(EXTLOGFONTA), "\n"); // 192
    ok(GetObjectA(hFont, sizeof(ENUMLOGFONTEXDVA), &u.enumlogfontexdva) == sizeof(ENUMLOGFONTEXDVA), "\n"); // 260
    ok(GetObjectA(hFont, sizeof(ENUMLOGFONTEXDVA)+1, &u.enumlogfontexdva) == sizeof(ENUMLOGFONTEXDVA), "\n"); // 260

    ok(GetObjectW(hFont, sizeof(LOGFONTW), &u.logfontw) == sizeof(LOGFONTW), "\n"); // 92
    ok(GetObjectW(hFont, sizeof(LOGFONTW) + 1, &u.logfontw) == sizeof(LOGFONTW) + 1, "\n"); // 92
    ok(GetObjectW(hFont, sizeof(LOGFONTW) - 1, &u.logfontw) == sizeof(LOGFONTW) - 1, "\n"); // 92
    ok(GetObjectW(hFont, sizeof(ENUMLOGFONTW), &u.enumlogfontw) == sizeof(ENUMLOGFONTW), "\n"); // 284
    ok(GetObjectW(hFont, sizeof(EXTLOGFONTW), &u.extlogfontw) == sizeof(EXTLOGFONTW), "\n"); // 320
    ok(GetObjectW(hFont, sizeof(ENUMLOGFONTEXW), &u.enumlogfontexw) == sizeof(ENUMLOGFONTEXW), "\n"); // 348
    ok(GetObjectW(hFont, sizeof(ENUMLOGFONTEXW) + 1, &u.enumlogfontexw) == sizeof(ENUMLOGFONTEXW) + 1, "\n"); // 348
    ok(GetObjectW(hFont, 355, &u.enumlogfontexdvw) == 355, "\n"); // 419

    ok(GetObjectW(hFont, 356, &u.enumlogfontexdvw) == sizeof(ENUMLOGFONTEXW) + 2*sizeof(DWORD), "\n"); // 419
    ret = sizeof(ENUMLOGFONTEXW) + 2*sizeof(DWORD);
    ret = GetObjectW(hFont, 357, &u.enumlogfontexdvw);
    ok(ret == sizeof(ENUMLOGFONTEXW) + 2*sizeof(DWORD), "\n");
    ok(GetObjectW(hFont, 357, &u.enumlogfontexdvw) == sizeof(ENUMLOGFONTEXW) + 2*sizeof(DWORD), "\n"); // 419
    ok(GetObjectW(hFont, sizeof(ENUMLOGFONTEXDVW) - 1, &u.enumlogfontexdvw) == sizeof(ENUMLOGFONTEXW) + 2*sizeof(DWORD), "\n"); // 419
    ok(GetObjectW(hFont, sizeof(ENUMLOGFONTEXDVW), &u.enumlogfontexdvw) == sizeof(ENUMLOGFONTEXW) + 2*sizeof(DWORD), "\n"); // 420
    ok(GetObjectW(hFont, sizeof(ENUMLOGFONTEXDVW)+1, &u.enumlogfontexdvw) == sizeof(ENUMLOGFONTEXW) + 2*sizeof(DWORD), "\n"); // 356!
    ok(GetLastError() == ERROR_SUCCESS, "got %ld\n", GetLastError());

    DeleteObject(hFont);
}

void
Test_Colorspace(void)
{
    UCHAR buffer[1000];

    SetLastError(ERROR_SUCCESS);
    GetObjectA((HANDLE)GDI_OBJECT_TYPE_COLORSPACE, 0, NULL);
    //ok(GetObjectA((HANDLE)GDI_OBJECT_TYPE_COLORSPACE, 0, NULL) == 60, "\n");// FIXME: what structure? fails on win7
    ok_err(ERROR_INSUFFICIENT_BUFFER);
    SetLastError(ERROR_SUCCESS);
    ok(GetObjectW((HANDLE)GDI_OBJECT_TYPE_COLORSPACE, 0, NULL) == 0, "\n");
    ok_err(ERROR_INSUFFICIENT_BUFFER);
    SetLastError(ERROR_SUCCESS);
    ok(GetObjectA((HANDLE)GDI_OBJECT_TYPE_COLORSPACE, 327, buffer) == 0, "\n");
    ok_err(ERROR_INSUFFICIENT_BUFFER);
    ok(GetObjectA((HANDLE)GDI_OBJECT_TYPE_COLORSPACE, 328, buffer) == 0, "\n");
    ok_err(ERROR_INVALID_PARAMETER);

    //ok_long(GetObjectA((HANDLE)GDI_OBJECT_TYPE_COLORSPACE, 328, NULL), 0); // FIXME: fails on WHS
    //ok_err(ERROR_INSUFFICIENT_BUFFER);
}

void
Test_MetaDC(void)
{
    /* Windows does not SetLastError() on a metadc, but it doesn't seem to do anything with it */
    HDC hMetaDC;
    BYTE buffer[1000];

    hMetaDC = CreateMetaFile(NULL);
    ok(hMetaDC != 0, "CreateMetaFile failed, skipping tests.\n");
    if(!hMetaDC) return;

    ok(((UINT_PTR)hMetaDC & GDI_HANDLE_TYPE_MASK) == GDI_OBJECT_TYPE_METADC, "\n");

    SetLastError(ERROR_SUCCESS);
    ok(GetObjectA((HANDLE)GDI_OBJECT_TYPE_METADC, 0, NULL) == 0, "\n");
    ok(GetObjectA((HANDLE)GDI_OBJECT_TYPE_METADC, 100, &buffer) == 0, "\n");
    ok(GetObjectA(hMetaDC, 0, NULL) == 0, "\n");
    ok(GetObjectA(hMetaDC, 1000, &buffer) == 0, "\n");
    ok(GetLastError() == ERROR_SUCCESS, "got %ld\n", GetLastError());
}

void
Test_Region(void)
{
    HRGN hRgn;
    hRgn = CreateRectRgn(0,0,5,5);
    SetLastError(ERROR_SUCCESS);
    ok(GetObjectW(hRgn, 0, NULL) == 0, "\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "\n");
    DeleteObject(hRgn);
}

void Test_CursorIcon()
{
    BITMAP bmp;
    HBITMAP hbmMask;
    CURSORINFO CursorInfo;

    /* On XP sp3 GetObject reports a 32x32 bitmap. */
    hbmMask = CreateBitmap(32, 64, 1, 1, src_mask);
    GetObjectW(hbmMask, sizeof(BITMAP), &bmp);
    ok(bmp.bmWidth == (bmp.bmHeight / 2), "ERR UNICODE CursorIcon RECT got %ldx%ld\n", bmp.bmWidth, bmp.bmHeight);
    ok(bmp.bmHeight == 64, "ERR UNICODE CursorIcon Height got %ld\n", bmp.bmHeight);
    DeleteObject(hbmMask);

    CursorInfo.cbSize = sizeof(CURSORINFO);
    GetCursorInfo(&CursorInfo);
    ok(CursorInfo.hCursor != NULL, "Invalid HCURSOR Handler\n");
    ok(CursorInfo.flags != 0, "Mouse cursor is hidden\n");
    GetObject(CursorInfo.hCursor, sizeof(BITMAP), &bmp);
    ok(bmp.bmWidth == bmp.bmHeight / 2, "ERR CursorIcon RECT got %ldx%ld\n", bmp.bmWidth, bmp.bmHeight);
    ok(bmp.bmHeight == 64, "ERR CursorIcon Height got %ld\n", bmp.bmHeight);
}

START_TEST(GetObject)
{

    Test_Font();
    Test_Colorspace();
    Test_General();
    Test_Bitmap();
    Test_Dibsection();
    Test_Palette();
    Test_Brush();
    Test_DIBBrush();
    Test_Pen();
    Test_ExtPen(); // not implemented yet in ROS
    Test_MetaDC();
    Test_Region();
    Test_CursorIcon();
}

