/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for GetObject
 * PROGRAMMERS:     Timo Kreuzer
 */

#include <stdio.h>
#include <wine/test.h>
#include <windows.h>
#include <winddi.h>
#include <include/ntgdityp.h>
#include <include/ntgdihdl.h>

void
Test_General(void)
{
	struct
	{
		LOGBRUSH logbrush;
		BYTE additional[5];
	} TestStruct;
	PLOGBRUSH plogbrush;
	HBRUSH hBrush;

	/* Test null pointer and invalid handles */
	SetLastError(ERROR_SUCCESS);
	ok(GetObjectA(0, 0, NULL) == 0, "\n");
	ok(GetObjectA((HANDLE)-1, 0, NULL) == 0, "\n");
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
	hBrush = GetStockObject(WHITE_BRUSH);
	plogbrush = (PVOID)((ULONG_PTR)&TestStruct.logbrush);
	ok(GetObject(hBrush, sizeof(LOGBRUSH), plogbrush) == sizeof(LOGBRUSH), "\n");
	plogbrush = (PVOID)((ULONG_PTR)&TestStruct.logbrush + 2);
	ok(GetObject(hBrush, sizeof(LOGBRUSH), plogbrush) == sizeof(LOGBRUSH), "\n");
	plogbrush = (PVOID)((ULONG_PTR)&TestStruct.logbrush + 1);
	//ok(GetObject(hBrush, sizeof(LOGBRUSH), plogbrush) == 0, "\n"); // fails on win7
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
	BITMAPINFO bmi = {{sizeof(BITMAPINFOHEADER), 10, 9, 1, 8, BI_RGB, 0, 10, 10, 0,0}};
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
	ok(GetObject(hBitmap, sizeof(DIBSECTION), NULL) == sizeof(BITMAP), "\n");
	ok(GetObject(hBitmap, 0, NULL) == sizeof(BITMAP), "\n");
	ok(GetObject(hBitmap, 5, NULL) == sizeof(BITMAP), "\n");
	ok(GetObject(hBitmap, -5, NULL) == sizeof(BITMAP), "\n");
	ok(GetObject(hBitmap, 0, &dibsection) == 0, "\n");
	ok(GetObject(hBitmap, 5, &dibsection) == 0, "\n");
	ok(GetObject(hBitmap, sizeof(BITMAP), &bitmap) == sizeof(BITMAP), "\n");
	ok(GetObject(hBitmap, sizeof(BITMAP)+2, &bitmap) == sizeof(BITMAP), "\n");
	ok(bitmap.bmType == 0, "\n");
	ok(bitmap.bmWidth == 10, "\n");
	ok(bitmap.bmHeight == 9, "\n");
	ok(bitmap.bmWidthBytes == 12, "\n");
	ok(bitmap.bmPlanes == 1, "\n");
	ok(bitmap.bmBitsPixel == 8, "\n");
	ok(bitmap.bmBits == pData, "\n");
	ok(GetObject(hBitmap, sizeof(DIBSECTION), &dibsection) == sizeof(DIBSECTION), "\n");
	ok(GetObject(hBitmap, sizeof(DIBSECTION)+2, &dibsection) == sizeof(DIBSECTION), "\n");
	ok(GetObject(hBitmap, -5, &dibsection) == sizeof(DIBSECTION), "\n");
	ok(GetLastError() == ERROR_SUCCESS, "\n");
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
    SetLastError(ERROR_SUCCESS);
	ok(GetObject(hPen, 0, &extlogpen) == 0, "\n");
	ok(GetLastError() == ERROR_NOACCESS, "got %ld\n", GetLastError());
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
	ok(((EXTLOGPEN*)&elpUserStyle)->elpStyleEntry[0] == 0, "\n");
	ok(((EXTLOGPEN*)&elpUserStyle)->elpStyleEntry[1] == 1, "\n");
	ok(((EXTLOGPEN*)&elpUserStyle)->elpStyleEntry[15] == 15, "\n");
	DeleteObject(hPen);
}

void
Test_Font(void)
{
	HFONT hFont;
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

	FillMemory(&logfonta, sizeof(LOGFONTA), 0x77);
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
	ok(GetObjectA(hFont, 0, &logfonta) == 0, "\n");
	ok(logfonta.lfHeight == 0x77777777, "\n");

	ok(GetObjectA(hFont, 5, &logfonta) == 5, "\n");
	ok(logfonta.lfHeight == 8, "\n");
	ok(logfonta.lfWidth == 0x77777708, "\n");

	ok(GetObjectA(hFont, 0, &logfonta) == 0, "\n");
	ok(GetObjectA(hFont, -1, &logfonta) == sizeof(ENUMLOGFONTEXDVA), "\n");
	ok(GetObjectA(hFont, 1, &logfonta) == 1, "\n"); // 1 -> 1
	ok(GetObjectA(hFont, sizeof(LOGFONTA) - 1, &logfonta) == sizeof(LOGFONTA) - 1, "\n"); // 59 -> 59
	ok(GetObjectA(hFont, sizeof(LOGFONTA), &logfonta) == sizeof(LOGFONTA), "\n"); // 60 -> 60
	ok(GetObjectA(hFont, sizeof(LOGFONTA) + 1, &logfonta) == sizeof(LOGFONTA) + 1, "\n"); // 61 -> 61
	ok(GetObjectA(hFont, sizeof(LOGFONTW) - 1, &logfontw) == sizeof(LOGFONTW) - 1, "\n"); // 91 -> 91
	ok(GetObjectA(hFont, sizeof(LOGFONTW), &logfontw) == sizeof(LOGFONTA), "\n"); // 92 -> 60
	ok(GetObjectA(hFont, sizeof(LOGFONTW) + 1, &logfontw) == sizeof(LOGFONTW) + 1, "\n"); // 93 -> 93
	ok(GetObjectA(hFont, sizeof(EXTLOGFONTA), &extlogfonta) == sizeof(EXTLOGFONTA), "\n"); // 192 -> 192
	ok(GetObjectA(hFont, sizeof(EXTLOGFONTA)+1, &extlogfonta) == sizeof(EXTLOGFONTA)+1, "\n"); // 192+1 -> 192+1
	ok(GetObjectA(hFont, sizeof(EXTLOGFONTA)+16*4, &extlogfonta) == sizeof(EXTLOGFONTA)+16*4, "\n"); // 192+1 -> 192+1
	ok(GetObjectA(hFont, sizeof(EXTLOGFONTW), &extlogfontw) == sizeof(ENUMLOGFONTEXDVA), "\n"); // 320 -> 260
	ok(GetObjectA(hFont, 261, &bData) == 260, "\n"); // no

	ok(GetObjectA(hFont, sizeof(ENUMLOGFONTEXDVA) - 1, &enumlogfontexdvw) == sizeof(ENUMLOGFONTEXDVA) - 1, "\n"); // 419
	ok(GetObjectA(hFont, sizeof(ENUMLOGFONTEXDVA), &enumlogfontexdvw) == sizeof(ENUMLOGFONTEXDVA), "\n"); // 420
	ok(GetObjectA(hFont, sizeof(ENUMLOGFONTEXDVA)+1, &enumlogfontexdvw) == sizeof(ENUMLOGFONTEXDVA), "\n"); // 356!

	/* LOGFONT / GetObjectW */
	FillMemory(&logfontw, sizeof(LOGFONTW), 0x77);

	ok(GetObjectW(hFont, sizeof(LOGFONTW), NULL) == sizeof(LOGFONTW), "1\n");
	ok(GetObjectW(hFont, 0, NULL) == sizeof(LOGFONTW), "\n");
	ok(GetObjectW(hFont, 5, NULL) == sizeof(LOGFONTW), "\n");
	ok(GetObjectW(hFont, sizeof(ENUMLOGFONTEXA), NULL) == sizeof(LOGFONTW), "\n");
	ok(GetObjectW(hFont, -5, NULL) == sizeof(LOGFONTW), "\n");
	ok(GetObjectW(hFont, 0, &logfontw) == 0, "\n");
	ok(logfontw.lfHeight == 0x77777777, "\n");

	ok(GetObjectW(hFont, 5, &logfontw) == 5, "\n");
	ok(logfontw.lfHeight == 8, "\n");
	ok(logfontw.lfWidth == 0x77777708, "\n");

	ok(GetObjectA(hFont, sizeof(LOGFONTA), &logfonta) == sizeof(LOGFONTA), "\n"); // 60
	ok(logfonta.lfHeight == 8, "\n");
	ok(GetObjectA(hFont, sizeof(ENUMLOGFONTA), &enumlogfonta) == sizeof(ENUMLOGFONTA), "\n"); // 156
	ok(GetObjectA(hFont, sizeof(ENUMLOGFONTEXA), &enumlogfontexa) == sizeof(ENUMLOGFONTEXA), "\n"); // 188
	ok(GetObjectA(hFont, sizeof(EXTLOGFONTA), &extlogfonta) == sizeof(EXTLOGFONTA), "\n"); // 192
	ok(GetObjectA(hFont, sizeof(ENUMLOGFONTEXDVA), &enumlogfontexdva) == sizeof(ENUMLOGFONTEXDVA), "\n"); // 260
	ok(GetObjectA(hFont, sizeof(ENUMLOGFONTEXDVA)+1, &enumlogfontexdva) == sizeof(ENUMLOGFONTEXDVA), "\n"); // 260

	ok(GetObjectW(hFont, sizeof(LOGFONTW), &logfontw) == sizeof(LOGFONTW), "\n"); // 92
	ok(GetObjectW(hFont, sizeof(LOGFONTW) + 1, &logfontw) == sizeof(LOGFONTW) + 1, "\n"); // 92
	ok(GetObjectW(hFont, sizeof(LOGFONTW) - 1, &logfontw) == sizeof(LOGFONTW) - 1, "\n"); // 92
	ok(GetObjectW(hFont, sizeof(ENUMLOGFONTW), &enumlogfontw) == sizeof(ENUMLOGFONTW), "\n"); // 284
	ok(GetObjectW(hFont, sizeof(EXTLOGFONTW), &extlogfontw) == sizeof(EXTLOGFONTW), "\n"); // 320
	ok(GetObjectW(hFont, sizeof(ENUMLOGFONTEXW), &enumlogfontexw) == sizeof(ENUMLOGFONTEXW), "\n"); // 348
	ok(GetObjectW(hFont, sizeof(ENUMLOGFONTEXW) + 1, &enumlogfontexw) == sizeof(ENUMLOGFONTEXW) + 1, "\n"); // 348
	ok(GetObjectW(hFont, 355, &enumlogfontexdvw) == 355, "\n"); // 419
	ok(GetObjectW(hFont, 356, &enumlogfontexdvw) == sizeof(ENUMLOGFONTEXW) + 2*sizeof(DWORD), "\n"); // 419
	ok(GetObjectW(hFont, sizeof(ENUMLOGFONTEXDVW) - 1, &enumlogfontexdvw) == sizeof(ENUMLOGFONTEXW) + 2*sizeof(DWORD), "\n"); // 419
	ok(GetObjectW(hFont, sizeof(ENUMLOGFONTEXDVW), &enumlogfontexdvw) == sizeof(ENUMLOGFONTEXW) + 2*sizeof(DWORD), "\n"); // 420
	ok(GetObjectW(hFont, sizeof(ENUMLOGFONTEXDVW)+1, &enumlogfontexdvw) == sizeof(ENUMLOGFONTEXW) + 2*sizeof(DWORD), "\n"); // 356!

	ok(GetObjectW(hFont, 356, &bData) == 356, "\n");
	ok(GetObjectW(hFont, 357, &bData) == 356, "\n");
	ok(GetLastError() == ERROR_SUCCESS, "got %ld\n", GetLastError());

	DeleteObject(hFont);
}

void
Test_Colorspace(void)
{
    UCHAR buffer[1000];
    int ret = 0;

	SetLastError(ERROR_SUCCESS);
	GetObjectA((HANDLE)GDI_OBJECT_TYPE_COLORSPACE, 0, NULL);
	//ok(GetObjectA((HANDLE)GDI_OBJECT_TYPE_COLORSPACE, 0, NULL) == 60, "\n");// FIXME: what structure? fails on win7
	ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "\n");
	SetLastError(ERROR_SUCCESS);
	ok(GetObjectW((HANDLE)GDI_OBJECT_TYPE_COLORSPACE, 0, NULL) == 0, "\n");
	ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "\n");
	SetLastError(ERROR_SUCCESS);
	ok(GetObjectA((HANDLE)GDI_OBJECT_TYPE_COLORSPACE, 327, buffer) == 0, "\n");
	ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "expected ERROR_INSUFFICIENT_BUFFER, got %ld\n", GetLastError());
	ok(GetObjectA((HANDLE)GDI_OBJECT_TYPE_COLORSPACE, 328, buffer) == 0, "\n");
	ok(GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

	ok(GetObjectA((HANDLE)GDI_OBJECT_TYPE_COLORSPACE, 328, NULL) == 0, "\n");
	//ok(ret == 0, "Expected ... got %d\n", ret);
	ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "expected ERROR_INSUFFICIENT_BUFFER, got %ld\n", GetLastError());
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

START_TEST(GetObject)
{

	Test_Font();
	Test_Colorspace();
	Test_General();
	Test_Bitmap();
	Test_Dibsection();
	Test_Palette();
	Test_Brush();
	Test_Pen();
	Test_ExtPen(); // not implemented yet in ROS
	Test_MetaDC();
	Test_Region();
}

