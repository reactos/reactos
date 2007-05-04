#line 2 "GetObject.c"

#include "../gditest.h"

BOOL
Test_General(INT* passed, INT* failed)
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
	TEST(GetObjectA(0, 0, NULL) == 0);
	TEST(GetObjectA((HANDLE)-1, 0, NULL) == 0);
	TEST(GetObjectA((HANDLE)0x00380000, 0, NULL) == 0);
	TEST(GetLastError() == ERROR_SUCCESS);

	SetLastError(ERROR_SUCCESS);
	TEST(GetObjectA((HANDLE)GDI_OBJECT_TYPE_DC, 0, NULL) == 0);
	TEST(GetLastError() == ERROR_INVALID_HANDLE);
	SetLastError(ERROR_SUCCESS);
	TEST(GetObjectW((HANDLE)GDI_OBJECT_TYPE_DC, 0, NULL) == 0);
	TEST(GetLastError() == ERROR_INVALID_HANDLE);
	SetLastError(ERROR_SUCCESS);
	TEST(GetObjectA((HANDLE)GDI_OBJECT_TYPE_REGION, 0, NULL) == 0);
	TEST(GetLastError() == ERROR_INVALID_HANDLE);
	SetLastError(ERROR_SUCCESS);
	TEST(GetObjectW((HANDLE)GDI_OBJECT_TYPE_REGION, 0, NULL) == 0);
	TEST(GetLastError() == ERROR_INVALID_HANDLE);
	SetLastError(ERROR_SUCCESS);
	TEST(GetObjectA((HANDLE)GDI_OBJECT_TYPE_EMF, 0, NULL) == 0);
	TEST(GetLastError() == ERROR_INVALID_HANDLE);
	SetLastError(ERROR_SUCCESS);
	TEST(GetObjectW((HANDLE)GDI_OBJECT_TYPE_EMF, 0, NULL) == 0);
	TEST(GetLastError() == ERROR_INVALID_HANDLE);
	SetLastError(ERROR_SUCCESS);
	TEST(GetObjectA((HANDLE)GDI_OBJECT_TYPE_METAFILE, 0, NULL) == 0);
	TEST(GetLastError() == ERROR_INVALID_HANDLE);
	SetLastError(ERROR_SUCCESS);
	TEST(GetObjectW((HANDLE)GDI_OBJECT_TYPE_METAFILE, 0, NULL) == 0);
	TEST(GetLastError() == ERROR_INVALID_HANDLE);
	SetLastError(ERROR_SUCCESS);
	TEST(GetObjectA((HANDLE)GDI_OBJECT_TYPE_ENHMETAFILE, 0, NULL) == 0);
	TEST(GetLastError() == ERROR_INVALID_HANDLE);
	SetLastError(ERROR_SUCCESS);
	TEST(GetObjectW((HANDLE)GDI_OBJECT_TYPE_ENHMETAFILE, 0, NULL) == 0);
	TEST(GetLastError() == ERROR_INVALID_HANDLE);

	/* Test need of alignment */
	hBrush = GetStockObject(WHITE_BRUSH);
	plogbrush = (PVOID)((ULONG_PTR)&TestStruct.logbrush);
	TEST(GetObject(hBrush, sizeof(LOGBRUSH), plogbrush) == sizeof(LOGBRUSH));
	plogbrush = (PVOID)((ULONG_PTR)&TestStruct.logbrush + 2);
	TEST(GetObject(hBrush, sizeof(LOGBRUSH), plogbrush) == sizeof(LOGBRUSH));
	plogbrush = (PVOID)((ULONG_PTR)&TestStruct.logbrush + 1);
	TEST(GetObject(hBrush, sizeof(LOGBRUSH), plogbrush) == 0);

    return TRUE;
}

BOOL
Test_Bitmap(INT* passed, INT* failed)
{
	HBITMAP hBitmap;
	BITMAP bitmap;
	DIBSECTION dibsection;
	BYTE bData[100] = {0};
	BYTE Buffer[100] = {48,48,48,48,48,48,48,48,48,48,48,48,48,48,48,0};

	FillMemory(&bitmap, sizeof(BITMAP), 0x77);
	hBitmap = CreateBitmap(10,10,1,8,bData);
	if (!hBitmap) return FALSE;

	SetLastError(ERROR_SUCCESS);
	TEST(GetObjectA((HANDLE)GDI_OBJECT_TYPE_BITMAP, 0, NULL) == sizeof(BITMAP));
	TEST(GetObjectW((HANDLE)GDI_OBJECT_TYPE_BITMAP, 0, NULL) == sizeof(BITMAP));
	TEST(GetObjectA((HANDLE)GDI_OBJECT_TYPE_BITMAP, sizeof(BITMAP), NULL) == sizeof(BITMAP));
	TEST(GetObjectA(hBitmap, sizeof(DIBSECTION), NULL) == sizeof(BITMAP));
	TEST(GetObjectA(hBitmap, 0, NULL) == sizeof(BITMAP));
	TEST(GetObjectA((HANDLE)((UINT)hBitmap & 0x0000ffff), 0, NULL) == sizeof(BITMAP));
	TEST(GetObjectA(hBitmap, 5, NULL) == sizeof(BITMAP));
	TEST(GetObjectA(hBitmap, -5, NULL) == sizeof(BITMAP));
	TEST(GetObjectA(hBitmap, 0, Buffer) == 0);
	TEST(GetObjectA(hBitmap, 5, Buffer) == 0);
	TEST(GetObjectA(hBitmap, sizeof(BITMAP), &bitmap) == sizeof(BITMAP));
	TEST(GetObjectA(hBitmap, sizeof(BITMAP)+2, &bitmap) == sizeof(BITMAP));
	TEST(GetObjectA(hBitmap, sizeof(DIBSECTION), &dibsection) == sizeof(BITMAP));
	TEST(GetObjectA(hBitmap, -5, &bitmap) == sizeof(BITMAP));
	TEST(GetLastError() == ERROR_SUCCESS);

	// todo: test invalid handle + buffer

	DeleteObject(hBitmap);
	return TRUE;
}

BOOL
Test_Dibsection(INT* passed, INT* failed)
{
	BITMAPINFO bmi = {{sizeof(BITMAPINFOHEADER), 10, 10, 1, 8, BI_RGB, 0, 10, 10, 0,0}};
	HBITMAP hBitmap;
	DIBSECTION dibsection;
	PVOID pData;

	FillMemory(&dibsection, sizeof(DIBSECTION), 0x77);
	HDC hDC = GetDC(0);
	hBitmap = CreateDIBSection(hDC, &bmi, DIB_RGB_COLORS, &pData, NULL, 0);
	if(!hBitmap) return FALSE;

	SetLastError(ERROR_SUCCESS);
	TEST(GetObject(hBitmap, sizeof(DIBSECTION), NULL) == sizeof(BITMAP));
	TEST(GetObject(hBitmap, 0, NULL) == sizeof(BITMAP));
	TEST(GetObject(hBitmap, 5, NULL) == sizeof(BITMAP));
	TEST(GetObject(hBitmap, -5, NULL) == sizeof(BITMAP));
	TEST(GetObject(hBitmap, 0, &dibsection) == 0);
	TEST(GetObject(hBitmap, 5, &dibsection) == 0);
	TEST(GetObject(hBitmap, sizeof(BITMAP), &dibsection) == sizeof(BITMAP));
	TEST(GetObject(hBitmap, sizeof(BITMAP)+2, &dibsection) == sizeof(BITMAP));
	TEST(GetObject(hBitmap, sizeof(DIBSECTION), &dibsection) == sizeof(DIBSECTION));
	TEST(GetObject(hBitmap, sizeof(DIBSECTION)+2, &dibsection) == sizeof(DIBSECTION));
	TEST(GetObject(hBitmap, -5, &dibsection) == sizeof(DIBSECTION));
	TEST(GetLastError() == ERROR_SUCCESS);
	DeleteObject(hBitmap);
	ReleaseDC(0, hDC);

	return TRUE;
}

BOOL Test_Palette(INT* passed, INT* failed)
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
	if (!hPalette) return FALSE;

	SetLastError(ERROR_SUCCESS);
	TEST(GetObjectA((HANDLE)GDI_OBJECT_TYPE_PALETTE, 0, NULL) == sizeof(WORD));
	TEST(GetObjectW((HANDLE)GDI_OBJECT_TYPE_PALETTE, 0, NULL) == sizeof(WORD));
	TEST(GetObject(hPalette, sizeof(WORD), NULL) == sizeof(WORD));
	TEST(GetObject(hPalette, 0, NULL) == sizeof(WORD));
	TEST(GetObject(hPalette, 5, NULL) == sizeof(WORD));
	TEST(GetObject(hPalette, -5, NULL) == sizeof(WORD));
	TEST(GetObject(hPalette, sizeof(WORD), &wPalette) == sizeof(WORD));
	TEST(GetObject(hPalette, sizeof(WORD)+2, &wPalette) == sizeof(WORD));
	TEST(GetObject(hPalette, 0, &wPalette) == 0);
	TEST(GetObject(hPalette, 1, &wPalette) == 0);
	TEST(GetObject(hPalette, -1, &wPalette) == sizeof(WORD));
	TEST(GetLastError() == ERROR_SUCCESS);
	DeleteObject(hPalette);
	return TRUE;
}

BOOL Test_Brush(INT* passed, INT* failed)
{
	LOGBRUSH logbrush;
	HBRUSH hBrush;

	FillMemory(&logbrush, sizeof(LOGBRUSH), 0x77);
	hBrush = CreateSolidBrush(RGB(1,2,3));
	if (!hBrush) return FALSE;

	SetLastError(ERROR_SUCCESS);
	TEST(GetObjectA((HANDLE)GDI_OBJECT_TYPE_BRUSH, 0, NULL) == sizeof(LOGBRUSH));
	TEST(GetObjectW((HANDLE)GDI_OBJECT_TYPE_BRUSH, 0, NULL) == sizeof(LOGBRUSH));
	TEST(GetObject(hBrush, sizeof(WORD), NULL) == sizeof(LOGBRUSH));
	TEST(GetObject(hBrush, 0, NULL) == sizeof(LOGBRUSH));
	TEST(GetObject(hBrush, 5, NULL) == sizeof(LOGBRUSH));
	TEST(GetObject(hBrush, -5, NULL) == sizeof(LOGBRUSH));

	TEST(GetObject(hBrush, 0, &logbrush) == 0);
	TEST(logbrush.lbStyle == 0x77777777);
	TEST(GetObject(hBrush, 5, &logbrush) == sizeof(LOGBRUSH));
	TEST(logbrush.lbStyle == 0);
	TEST(logbrush.lbColor == 0x77777701);

	TEST(GetObject(hBrush, sizeof(LOGBRUSH), &logbrush) == sizeof(LOGBRUSH));
	TEST(GetObject(hBrush, sizeof(LOGBRUSH)+2, &logbrush) == sizeof(LOGBRUSH));
	TEST(GetObject(hBrush, -1, &logbrush) == sizeof(LOGBRUSH));
	// TODO: test all members

	TEST(GetLastError() == ERROR_SUCCESS);
	DeleteObject(hBrush);
	return TRUE;
}

BOOL Test_Pen(INT* passed, INT* failed)
{
	LOGPEN logpen;
	HPEN hPen;

	FillMemory(&logpen, sizeof(LOGPEN), 0x77);
	hPen = CreatePen(PS_SOLID, 3, RGB(4,5,6));
	if (!hPen) return FALSE;
	SetLastError(ERROR_SUCCESS);
	TEST(GetObjectA((HANDLE)GDI_OBJECT_TYPE_PEN, 0, NULL) == sizeof(LOGPEN));
	TEST(GetObjectW((HANDLE)GDI_OBJECT_TYPE_PEN, 0, NULL) == sizeof(LOGPEN));
	TEST(GetObject(hPen, sizeof(BITMAP), NULL) == sizeof(LOGPEN));
	TEST(GetObject(hPen, 0, NULL) == sizeof(LOGPEN));
	TEST(GetObject(hPen, 5, NULL) == sizeof(LOGPEN));
	TEST(GetObject(hPen, -5, NULL) == sizeof(LOGPEN));
	TEST(GetObject(hPen, sizeof(LOGPEN), &logpen) == sizeof(LOGPEN));
	TEST(GetObject(hPen, sizeof(LOGPEN)-1, &logpen) == 0);
	TEST(GetObject(hPen, sizeof(LOGPEN)+2, &logpen) == sizeof(LOGPEN));
	TEST(GetObject(hPen, 0, &logpen) == 0);
	TEST(GetObject(hPen, -5, &logpen) == sizeof(LOGPEN));
	TEST(GetLastError() == ERROR_SUCCESS);

	/* test if the fields are filled correctly */
	TEST(logpen.lopnStyle == PS_SOLID);
	

	DeleteObject(hPen);
	return TRUE;
}

BOOL Test_ExtPen(INT* passed, INT* failed)
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
	TEST(GetObjectA((HANDLE)GDI_OBJECT_TYPE_EXTPEN, 0, NULL) == 0);
	TEST(GetObjectW((HANDLE)GDI_OBJECT_TYPE_EXTPEN, 0, NULL) == 0);
	TEST(GetLastError() == ERROR_INVALID_PARAMETER);

	FillMemory(&extlogpen, sizeof(EXTLOGPEN), 0x77);
	logbrush.lbStyle = BS_SOLID;
	logbrush.lbColor = RGB(1,2,3);
	logbrush.lbHatch = 22;
	hPen = ExtCreatePen(PS_GEOMETRIC | PS_DASH, 5, &logbrush, 0, NULL);

	TEST(GDI_HANDLE_GET_TYPE(hPen) == GDI_OBJECT_TYPE_EXTPEN);
	TEST(GetObject((HANDLE)GDI_OBJECT_TYPE_EXTPEN, 0, NULL) == 0);
	TEST(GetObject(hPen, sizeof(EXTLOGPEN), NULL) == sizeof(EXTLOGPEN)-sizeof(DWORD));
	TEST(GetObject(hPen, 0, NULL) == sizeof(EXTLOGPEN)-sizeof(DWORD));
	TEST(GetObject((HANDLE)GDI_HANDLE_GET_INDEX(hPen), 0, NULL) == sizeof(EXTLOGPEN)-sizeof(DWORD));
	TEST(GetObject(hPen, 5, NULL) == sizeof(EXTLOGPEN)-sizeof(DWORD));
	TEST(GetObject(hPen, -5, NULL) == sizeof(EXTLOGPEN)-sizeof(DWORD));
	TEST(GetObject(hPen, 0, &extlogpen) == 0);
	TEST(GetObject(hPen, 4, &extlogpen) == 0);

	/* Nothing should be filled */
	TEST(extlogpen.elpPenStyle == 0x77777777);
	TEST(extlogpen.elpWidth == 0x77777777);

	TEST(GetObject(hPen, sizeof(EXTLOGPEN), &extlogpen) == sizeof(EXTLOGPEN)-sizeof(DWORD));
	TEST(GetObject(hPen, sizeof(EXTLOGPEN)-sizeof(DWORD), &extlogpen) == sizeof(EXTLOGPEN)-sizeof(DWORD));
	TEST(GetObject(hPen, sizeof(EXTLOGPEN)-sizeof(DWORD)-1, &extlogpen) == 0);
	TEST(GetObject(hPen, sizeof(EXTLOGPEN)+2, &extlogpen) == sizeof(EXTLOGPEN)-sizeof(DWORD));
	TEST(GetObject(hPen, -5, &extlogpen) == sizeof(EXTLOGPEN)-sizeof(DWORD));

	/* test if the fields are filled correctly */
	TEST(extlogpen.elpPenStyle == (PS_GEOMETRIC | PS_DASH));
	TEST(extlogpen.elpWidth == 5);
	TEST(extlogpen.elpBrushStyle == 0);
	TEST(extlogpen.elpColor == RGB(1,2,3));
	TEST(extlogpen.elpHatch == 22);
	TEST(extlogpen.elpNumEntries == 0);
	DeleteObject(hPen);

	/* A maximum of 16 Styles is allowed */
	hPen = ExtCreatePen(PS_GEOMETRIC | PS_USERSTYLE, 5, &logbrush, 16, (CONST DWORD*)&dwStyles);
	TEST(GetObject(hPen, 0, NULL) == sizeof(EXTLOGPEN) + 15*sizeof(DWORD));
	TEST(GetObject(hPen, sizeof(EXTLOGPEN) + 15*sizeof(DWORD), &elpUserStyle) == sizeof(EXTLOGPEN) + 15*sizeof(DWORD));
	TEST(((EXTLOGPEN*)&elpUserStyle)->elpStyleEntry[0] == 0);
	TEST(((EXTLOGPEN*)&elpUserStyle)->elpStyleEntry[1] == 1);
	TEST(((EXTLOGPEN*)&elpUserStyle)->elpStyleEntry[15] == 15);
	DeleteObject(hPen);

	return TRUE;
}

BOOL Test_Font(INT* passed, INT* failed)
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
	hFont = CreateFont(8, 8, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, 
		ANSI_CHARSET, OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS,
		ANTIALIASED_QUALITY, DEFAULT_PITCH, "testfont");
	TEST(hFont);

	SetLastError(ERROR_SUCCESS);
	TEST(GetObjectA((HANDLE)GDI_OBJECT_TYPE_FONT, 0, NULL) == sizeof(LOGFONTA));
	TEST(GetObjectW((HANDLE)GDI_OBJECT_TYPE_FONT, 0, NULL) == sizeof(LOGFONTW));
	TEST(GetObjectA((HANDLE)GDI_OBJECT_TYPE_FONT, sizeof(LOGFONTA), NULL) == sizeof(LOGFONTA)); // 60
	TEST(GetObjectA((HANDLE)GDI_OBJECT_TYPE_FONT, sizeof(ENUMLOGFONTA), NULL) == sizeof(LOGFONTA)); // 156
	TEST(GetObjectA((HANDLE)GDI_OBJECT_TYPE_FONT, sizeof(ENUMLOGFONTEXA), NULL) == sizeof(LOGFONTA)); // 188
	TEST(GetObjectA((HANDLE)GDI_OBJECT_TYPE_FONT, sizeof(EXTLOGFONTA), NULL) == sizeof(LOGFONTA)); // 192
	TEST(GetObjectA((HANDLE)GDI_OBJECT_TYPE_FONT, sizeof(ENUMLOGFONTEXDVA), NULL) == sizeof(LOGFONTA)); // 260
	TEST(GetObjectA((HANDLE)GDI_OBJECT_TYPE_FONT, sizeof(ENUMLOGFONTEXDVA)+1, NULL) == sizeof(LOGFONTA)); // 260
	TEST(GetObjectW((HANDLE)GDI_OBJECT_TYPE_FONT, sizeof(LOGFONTW), NULL) == sizeof(LOGFONTW)); // 92
	TEST(GetObjectW((HANDLE)GDI_OBJECT_TYPE_FONT, sizeof(ENUMLOGFONTW), NULL) == sizeof(LOGFONTW)); // 284
	TEST(GetObjectW((HANDLE)GDI_OBJECT_TYPE_FONT, sizeof(EXTLOGFONTW), NULL) == sizeof(LOGFONTW)); // 320
	TEST(GetObjectW((HANDLE)GDI_OBJECT_TYPE_FONT, sizeof(ENUMLOGFONTEXW), NULL) == sizeof(LOGFONTW)); // 348
	TEST(GetObjectW((HANDLE)GDI_OBJECT_TYPE_FONT, sizeof(ENUMLOGFONTEXDVW), NULL) == sizeof(LOGFONTW)); // 420
	TEST(GetObjectW((HANDLE)GDI_OBJECT_TYPE_FONT, sizeof(ENUMLOGFONTEXDVW)+1, NULL) == sizeof(LOGFONTW)); // 356!

	TEST(GetObjectA(hFont, sizeof(LOGFONTA), NULL) == sizeof(LOGFONTA));
	TEST(GetObjectA(hFont, 0, NULL) == sizeof(LOGFONTA));
	TEST(GetObjectA(hFont, 5, NULL) == sizeof(LOGFONTA));
	TEST(GetObjectA(hFont, -5, NULL) == sizeof(LOGFONTA));
	TEST(GetObjectA(hFont, 0, &logfonta) == 0);
	TEST(logfonta.lfHeight == 0x77777777);

	TEST(GetObjectA(hFont, 5, &logfonta) == 5);
	TEST(logfonta.lfHeight == 8);
	TEST(logfonta.lfWidth == 0x77777708);

	TEST(GetObjectA(hFont, sizeof(LOGFONTA), &logfonta) == sizeof(LOGFONTA)); // 60
	TEST(GetObjectA(hFont, sizeof(LOGFONTW), &logfontw) == sizeof(LOGFONTA)); // 92
	TEST(GetObjectA(hFont, sizeof(EXTLOGFONTA), &extlogfonta) == sizeof(EXTLOGFONTA)); // 192
	TEST(GetObjectA(hFont, sizeof(EXTLOGFONTA)+1, &extlogfonta) == sizeof(EXTLOGFONTA)+1); // 192
	TEST(GetObjectA(hFont, sizeof(EXTLOGFONTW), &extlogfontw) == sizeof(ENUMLOGFONTEXDVA)); // 320

	TEST(GetObjectA(hFont, 261, &bData) == 260); // no

	/* LOGFONT / GetObjectW */
	FillMemory(&logfontw, sizeof(LOGFONTW), 0x77);

	TEST(GetObjectW(hFont, sizeof(LOGFONTW), NULL) == sizeof(LOGFONTW));
	TEST(GetObjectW(hFont, 0, NULL) == sizeof(LOGFONTW));
	TEST(GetObjectW(hFont, 5, NULL) == sizeof(LOGFONTW));
	TEST(GetObjectW(hFont, -5, NULL) == sizeof(LOGFONTW));
	TEST(GetObjectW(hFont, 0, &logfontw) == 0);
	TEST(logfontw.lfHeight == 0x77777777);

	TEST(GetObjectW(hFont, 5, &logfontw) == 5);
	TEST(logfontw.lfHeight == 8);
	TEST(logfontw.lfWidth == 0x77777708);

	TEST(GetObjectA(hFont, sizeof(LOGFONTA), &logfonta) == sizeof(LOGFONTA)); // 60
	TEST(logfonta.lfHeight == 8);
	TEST(GetObjectA(hFont, sizeof(ENUMLOGFONTA), &enumlogfonta) == sizeof(ENUMLOGFONTA)); // 156
	TEST(GetObjectA(hFont, sizeof(ENUMLOGFONTEXA), &enumlogfontexa) == sizeof(ENUMLOGFONTEXA)); // 188
	TEST(GetObjectA(hFont, sizeof(EXTLOGFONTA), &extlogfonta) == sizeof(EXTLOGFONTA)); // 192
	TEST(GetObjectA(hFont, sizeof(ENUMLOGFONTEXDVA), &enumlogfontexdva) == sizeof(ENUMLOGFONTEXDVA)); // 260
	TEST(GetObjectA(hFont, sizeof(ENUMLOGFONTEXDVA)+1, &enumlogfontexdva) == sizeof(ENUMLOGFONTEXDVA)); // 260

	TEST(GetObjectW(hFont, sizeof(LOGFONTW), &logfontw) == sizeof(LOGFONTW)); // 92
	TEST(GetObjectW(hFont, sizeof(ENUMLOGFONTW), &enumlogfontw) == sizeof(ENUMLOGFONTW)); // 284
	TEST(GetObjectW(hFont, sizeof(EXTLOGFONTW), &extlogfontw) == sizeof(EXTLOGFONTW)); // 320
	TEST(GetObjectW(hFont, sizeof(ENUMLOGFONTEXW), &enumlogfontexw) == sizeof(ENUMLOGFONTEXW)); // 348
	TEST(GetObjectW(hFont, sizeof(ENUMLOGFONTEXDVW), &enumlogfontexdvw) == sizeof(ENUMLOGFONTEXW) + 2*sizeof(DWORD)); // 420
	TEST(GetObjectW(hFont, sizeof(ENUMLOGFONTEXDVW)+1, &enumlogfontexdvw) == sizeof(ENUMLOGFONTEXW) + 2*sizeof(DWORD)); // 356!

	TEST(GetObjectW(hFont, 356, &bData) == 356);
	TEST(GetLastError() == ERROR_SUCCESS);

	DeleteObject(hFont);

	return TRUE;
}

BOOL Test_Colorspace(INT* passed, INT* failed)
{
	SetLastError(ERROR_SUCCESS);
	TEST(GetObjectA((HANDLE)GDI_OBJECT_TYPE_COLORSPACE, 0, NULL) == 60);// FIXME: what structure?
	TEST(GetLastError() == ERROR_INSUFFICIENT_BUFFER);
	SetLastError(ERROR_SUCCESS);
	TEST(GetObjectW((HANDLE)GDI_OBJECT_TYPE_COLORSPACE, 0, NULL) == 0);
	TEST(GetLastError() == ERROR_INSUFFICIENT_BUFFER);

	return TRUE;
}

BOOL Test_MetaDC(INT* passed, INT* failed)
{
	/* Windows does not SetLastError() on a metadc, but it doesn't seem to do anything with it */
	HDC hMetaDC;
	BYTE buffer[100];

	hMetaDC = CreateMetaFile(NULL);
	if(!hMetaDC) return FALSE;
	if(((UINT)hMetaDC & GDI_HANDLE_TYPE_MASK) != GDI_OBJECT_TYPE_METADC) return FALSE;

	SetLastError(ERROR_SUCCESS);
	TEST(GetObjectA((HANDLE)GDI_OBJECT_TYPE_METADC, 0, NULL) == 0);
	TEST(GetObjectA((HANDLE)GDI_OBJECT_TYPE_METADC, 100, &buffer) == 0);
	TEST(GetObjectA(hMetaDC, 0, NULL) == 0);
	TEST(GetObjectA(hMetaDC, 100, &buffer) == 0);
	TEST(GetLastError() == ERROR_SUCCESS);
	return TRUE;
}

BOOL Test_GetObject(INT* passed, INT* failed)
{

    HRGN hRgn;
	hRgn = CreateRectRgn(0,0,5,5);
	SetLastError(ERROR_SUCCESS);
	TEST(GetObjectW(hRgn, 0, NULL) == 0);
	TEST(GetLastError() == ERROR_INVALID_HANDLE);
	DeleteObject(hRgn);

	Test_Font(passed, failed);
	Test_Colorspace(passed, failed);
	Test_General(passed, failed);
	Test_Bitmap(passed, failed);
	Test_Dibsection(passed, failed);
	Test_Palette(passed, failed);
	Test_Brush(passed, failed);
	Test_Pen(passed, failed);
	Test_ExtPen(passed, failed); // not implemented yet in ROS
	Test_MetaDC(passed, failed);

	return TRUE;
}
