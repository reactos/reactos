static INT
Test_General(PTESTINFO pti)
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
	RTEST(GetObjectA(0, 0, NULL) == 0);
	RTEST(GetObjectA((HANDLE)-1, 0, NULL) == 0);
	RTEST(GetObjectA((HANDLE)0x00380000, 0, NULL) == 0);
	RTEST(GetLastError() == ERROR_SUCCESS);

	SetLastError(ERROR_SUCCESS);
	RTEST(GetObjectA((HANDLE)GDI_OBJECT_TYPE_DC, 0, NULL) == 0);
	RTEST(GetLastError() == ERROR_INVALID_HANDLE);
	SetLastError(ERROR_SUCCESS);
	RTEST(GetObjectW((HANDLE)GDI_OBJECT_TYPE_DC, 0, NULL) == 0);
	RTEST(GetLastError() == ERROR_INVALID_HANDLE);
	SetLastError(ERROR_SUCCESS);
	RTEST(GetObjectA((HANDLE)GDI_OBJECT_TYPE_REGION, 0, NULL) == 0);
	RTEST(GetLastError() == ERROR_INVALID_HANDLE);
	SetLastError(ERROR_SUCCESS);
	RTEST(GetObjectW((HANDLE)GDI_OBJECT_TYPE_REGION, 0, NULL) == 0);
	RTEST(GetLastError() == ERROR_INVALID_HANDLE);
	SetLastError(ERROR_SUCCESS);
	RTEST(GetObjectA((HANDLE)GDI_OBJECT_TYPE_EMF, 0, NULL) == 0);
	RTEST(GetLastError() == ERROR_INVALID_HANDLE);
	SetLastError(ERROR_SUCCESS);
	RTEST(GetObjectW((HANDLE)GDI_OBJECT_TYPE_EMF, 0, NULL) == 0);
	RTEST(GetLastError() == ERROR_INVALID_HANDLE);
	SetLastError(ERROR_SUCCESS);
	RTEST(GetObjectA((HANDLE)GDI_OBJECT_TYPE_METAFILE, 0, NULL) == 0);
	RTEST(GetLastError() == ERROR_INVALID_HANDLE);
	SetLastError(ERROR_SUCCESS);
	RTEST(GetObjectW((HANDLE)GDI_OBJECT_TYPE_METAFILE, 0, NULL) == 0);
	RTEST(GetLastError() == ERROR_INVALID_HANDLE);
	SetLastError(ERROR_SUCCESS);
	RTEST(GetObjectA((HANDLE)GDI_OBJECT_TYPE_ENHMETAFILE, 0, NULL) == 0);
	RTEST(GetLastError() == ERROR_INVALID_HANDLE);
	SetLastError(ERROR_SUCCESS);
	RTEST(GetObjectW((HANDLE)GDI_OBJECT_TYPE_ENHMETAFILE, 0, NULL) == 0);
	RTEST(GetLastError() == ERROR_INVALID_HANDLE);

	/* Test need of alignment */
	hBrush = GetStockObject(WHITE_BRUSH);
	plogbrush = (PVOID)((ULONG_PTR)&TestStruct.logbrush);
	RTEST(GetObject(hBrush, sizeof(LOGBRUSH), plogbrush) == sizeof(LOGBRUSH));
	plogbrush = (PVOID)((ULONG_PTR)&TestStruct.logbrush + 2);
	RTEST(GetObject(hBrush, sizeof(LOGBRUSH), plogbrush) == sizeof(LOGBRUSH));
	plogbrush = (PVOID)((ULONG_PTR)&TestStruct.logbrush + 1);
	RTEST(GetObject(hBrush, sizeof(LOGBRUSH), plogbrush) == 0);

    return TRUE;
}

static INT
Test_Bitmap(PTESTINFO pti)
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
	RTEST(GetObjectA((HANDLE)GDI_OBJECT_TYPE_BITMAP, 0, NULL) == sizeof(BITMAP));
	RTEST(GetObjectW((HANDLE)GDI_OBJECT_TYPE_BITMAP, 0, NULL) == sizeof(BITMAP));
	RTEST(GetObjectA((HANDLE)GDI_OBJECT_TYPE_BITMAP, sizeof(BITMAP), NULL) == sizeof(BITMAP));
	RTEST(GetObjectA(hBitmap, sizeof(DIBSECTION), NULL) == sizeof(BITMAP));
	RTEST(GetObjectA(hBitmap, 0, NULL) == sizeof(BITMAP));
	RTEST(GetObjectA((HANDLE)((UINT)hBitmap & 0x0000ffff), 0, NULL) == sizeof(BITMAP));
	RTEST(GetObjectA(hBitmap, 5, NULL) == sizeof(BITMAP));
	RTEST(GetObjectA(hBitmap, -5, NULL) == sizeof(BITMAP));
	RTEST(GetObjectA(hBitmap, 0, Buffer) == 0);
	RTEST(GetObjectA(hBitmap, 5, Buffer) == 0);
	RTEST(GetObjectA(hBitmap, sizeof(BITMAP), &bitmap) == sizeof(BITMAP));
	RTEST(GetObjectA(hBitmap, sizeof(BITMAP)+2, &bitmap) == sizeof(BITMAP));
	RTEST(GetObjectA(hBitmap, sizeof(DIBSECTION), &dibsection) == sizeof(BITMAP));
	RTEST(GetObjectA(hBitmap, -5, &bitmap) == sizeof(BITMAP));
	RTEST(GetLastError() == ERROR_SUCCESS);

	// todo: test invalid handle + buffer

	DeleteObject(hBitmap);
	return TRUE;
}

static INT
Test_Dibsection(PTESTINFO pti)
{
	BITMAPINFO bmi = {{sizeof(BITMAPINFOHEADER), 10, 9, 1, 8, BI_RGB, 0, 10, 10, 0,0}};
	HBITMAP hBitmap;
	BITMAP bitmap;
	DIBSECTION dibsection;
	PVOID pData;

	FillMemory(&dibsection, sizeof(DIBSECTION), 0x77);
	HDC hDC = GetDC(0);
	hBitmap = CreateDIBSection(hDC, &bmi, DIB_RGB_COLORS, &pData, NULL, 0);
	ASSERT(hBitmap);

	SetLastError(ERROR_SUCCESS);
	RTEST(GetObject(hBitmap, sizeof(DIBSECTION), NULL) == sizeof(BITMAP));
	RTEST(GetObject(hBitmap, 0, NULL) == sizeof(BITMAP));
	RTEST(GetObject(hBitmap, 5, NULL) == sizeof(BITMAP));
	RTEST(GetObject(hBitmap, -5, NULL) == sizeof(BITMAP));
	RTEST(GetObject(hBitmap, 0, &dibsection) == 0);
	RTEST(GetObject(hBitmap, 5, &dibsection) == 0);
	RTEST(GetObject(hBitmap, sizeof(BITMAP), &bitmap) == sizeof(BITMAP));
	RTEST(GetObject(hBitmap, sizeof(BITMAP)+2, &bitmap) == sizeof(BITMAP));
	TEST(bitmap.bmType == 0);
	TEST(bitmap.bmWidth == 10);
	TEST(bitmap.bmHeight == 9);
	TEST(bitmap.bmWidthBytes == 12);
	TEST(bitmap.bmPlanes == 1);
	TEST(bitmap.bmBitsPixel == 8);
	TEST(bitmap.bmBits == pData);
	RTEST(GetObject(hBitmap, sizeof(DIBSECTION), &dibsection) == sizeof(DIBSECTION));
	RTEST(GetObject(hBitmap, sizeof(DIBSECTION)+2, &dibsection) == sizeof(DIBSECTION));
	RTEST(GetObject(hBitmap, -5, &dibsection) == sizeof(DIBSECTION));
	RTEST(GetLastError() == ERROR_SUCCESS);
	DeleteObject(hBitmap);
	ReleaseDC(0, hDC);

	return TRUE;
}

static INT
Test_Palette(PTESTINFO pti)
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
	RTEST(GetObjectA((HANDLE)GDI_OBJECT_TYPE_PALETTE, 0, NULL) == sizeof(WORD));
	RTEST(GetObjectW((HANDLE)GDI_OBJECT_TYPE_PALETTE, 0, NULL) == sizeof(WORD));
	RTEST(GetObject(hPalette, sizeof(WORD), NULL) == sizeof(WORD));
	RTEST(GetObject(hPalette, 0, NULL) == sizeof(WORD));
	RTEST(GetObject(hPalette, 5, NULL) == sizeof(WORD));
	RTEST(GetObject(hPalette, -5, NULL) == sizeof(WORD));
	RTEST(GetObject(hPalette, sizeof(WORD), &wPalette) == sizeof(WORD));
	RTEST(GetObject(hPalette, sizeof(WORD)+2, &wPalette) == sizeof(WORD));
	RTEST(GetObject(hPalette, 0, &wPalette) == 0);
	RTEST(GetObject(hPalette, 1, &wPalette) == 0);
	RTEST(GetObject(hPalette, -1, &wPalette) == sizeof(WORD));
	RTEST(GetLastError() == ERROR_SUCCESS);
	DeleteObject(hPalette);
	return TRUE;
}

static INT
Test_Brush(PTESTINFO pti)
{
	LOGBRUSH logbrush;
	HBRUSH hBrush;

	FillMemory(&logbrush, sizeof(LOGBRUSH), 0x77);
	hBrush = CreateSolidBrush(RGB(1,2,3));
	if (!hBrush) return FALSE;

	SetLastError(ERROR_SUCCESS);
	RTEST(GetObjectA((HANDLE)GDI_OBJECT_TYPE_BRUSH, 0, NULL) == sizeof(LOGBRUSH));
	RTEST(GetObjectW((HANDLE)GDI_OBJECT_TYPE_BRUSH, 0, NULL) == sizeof(LOGBRUSH));
	RTEST(GetObject(hBrush, sizeof(WORD), NULL) == sizeof(LOGBRUSH));
	RTEST(GetObject(hBrush, 0, NULL) == sizeof(LOGBRUSH));
	RTEST(GetObject(hBrush, 5, NULL) == sizeof(LOGBRUSH));
	RTEST(GetObject(hBrush, -5, NULL) == sizeof(LOGBRUSH));

	RTEST(GetObject(hBrush, 0, &logbrush) == 0);
	RTEST(logbrush.lbStyle == 0x77777777);
	RTEST(GetObject(hBrush, 5, &logbrush) == sizeof(LOGBRUSH));
	RTEST(logbrush.lbStyle == 0);
	RTEST(logbrush.lbColor == 0x77777701);

	RTEST(GetObject(hBrush, sizeof(LOGBRUSH), &logbrush) == sizeof(LOGBRUSH));
	RTEST(GetObject(hBrush, sizeof(LOGBRUSH)+2, &logbrush) == sizeof(LOGBRUSH));
	RTEST(GetObject(hBrush, -1, &logbrush) == sizeof(LOGBRUSH));
	// TODO: test all members

	RTEST(GetLastError() == ERROR_SUCCESS);
	DeleteObject(hBrush);
	return TRUE;
}

static INT
Test_Pen(PTESTINFO pti)
{
	LOGPEN logpen;
	HPEN hPen;

	FillMemory(&logpen, sizeof(LOGPEN), 0x77);
	hPen = CreatePen(PS_SOLID, 3, RGB(4,5,6));
	if (!hPen) return FALSE;
	SetLastError(ERROR_SUCCESS);
	RTEST(GetObjectA((HANDLE)GDI_OBJECT_TYPE_PEN, 0, NULL) == sizeof(LOGPEN));
	RTEST(GetObjectW((HANDLE)GDI_OBJECT_TYPE_PEN, 0, NULL) == sizeof(LOGPEN));
	RTEST(GetObject(hPen, sizeof(BITMAP), NULL) == sizeof(LOGPEN));
	RTEST(GetObject(hPen, 0, NULL) == sizeof(LOGPEN));
	RTEST(GetObject(hPen, 5, NULL) == sizeof(LOGPEN));
	RTEST(GetObject(hPen, -5, NULL) == sizeof(LOGPEN));
	RTEST(GetObject(hPen, sizeof(LOGPEN), &logpen) == sizeof(LOGPEN));
	RTEST(GetObject(hPen, sizeof(LOGPEN)-1, &logpen) == 0);
	RTEST(GetObject(hPen, sizeof(LOGPEN)+2, &logpen) == sizeof(LOGPEN));
	RTEST(GetObject(hPen, 0, &logpen) == 0);
	RTEST(GetObject(hPen, -5, &logpen) == sizeof(LOGPEN));
	RTEST(GetLastError() == ERROR_SUCCESS);

	/* test if the fields are filled correctly */
	RTEST(logpen.lopnStyle == PS_SOLID);


	DeleteObject(hPen);
	return TRUE;
}

static INT
Test_ExtPen(PTESTINFO pti)
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
	RTEST(GetObjectA((HANDLE)GDI_OBJECT_TYPE_EXTPEN, 0, NULL) == 0);
	RTEST(GetObjectW((HANDLE)GDI_OBJECT_TYPE_EXTPEN, 0, NULL) == 0);
	TEST(GetLastError() == ERROR_INVALID_PARAMETER);

	FillMemory(&extlogpen, sizeof(EXTLOGPEN), 0x77);
	logbrush.lbStyle = BS_SOLID;
	logbrush.lbColor = RGB(1,2,3);
	logbrush.lbHatch = 22;
	hPen = ExtCreatePen(PS_GEOMETRIC | PS_DASH, 5, &logbrush, 0, NULL);

	RTEST(GDI_HANDLE_GET_TYPE(hPen) == GDI_OBJECT_TYPE_EXTPEN);
	RTEST(GetObject((HANDLE)GDI_OBJECT_TYPE_EXTPEN, 0, NULL) == 0);
	RTEST(GetObject(hPen, sizeof(EXTLOGPEN), NULL) == sizeof(EXTLOGPEN)-sizeof(DWORD));
	RTEST(GetObject(hPen, 0, NULL) == sizeof(EXTLOGPEN)-sizeof(DWORD));
	RTEST(GetObject((HANDLE)GDI_HANDLE_GET_INDEX(hPen), 0, NULL) == sizeof(EXTLOGPEN)-sizeof(DWORD));
	RTEST(GetObject(hPen, 5, NULL) == sizeof(EXTLOGPEN)-sizeof(DWORD));
	RTEST(GetObject(hPen, -5, NULL) == sizeof(EXTLOGPEN)-sizeof(DWORD));
	RTEST(GetObject(hPen, 0, &extlogpen) == 0);
	RTEST(GetObject(hPen, 4, &extlogpen) == 0);

	/* Nothing should be filled */
	RTEST(extlogpen.elpPenStyle == 0x77777777);
	RTEST(extlogpen.elpWidth == 0x77777777);

	RTEST(GetObject(hPen, sizeof(EXTLOGPEN), &extlogpen) == sizeof(EXTLOGPEN)-sizeof(DWORD));
	RTEST(GetObject(hPen, sizeof(EXTLOGPEN)-sizeof(DWORD), &extlogpen) == sizeof(EXTLOGPEN)-sizeof(DWORD));
	RTEST(GetObject(hPen, sizeof(EXTLOGPEN)-sizeof(DWORD)-1, &extlogpen) == 0);
	RTEST(GetObject(hPen, sizeof(EXTLOGPEN)+2, &extlogpen) == sizeof(EXTLOGPEN)-sizeof(DWORD));
	RTEST(GetObject(hPen, -5, &extlogpen) == sizeof(EXTLOGPEN)-sizeof(DWORD));

	/* test if the fields are filled correctly */
	RTEST(extlogpen.elpPenStyle == (PS_GEOMETRIC | PS_DASH));
	RTEST(extlogpen.elpWidth == 5);
	RTEST(extlogpen.elpBrushStyle == 0);
	RTEST(extlogpen.elpColor == RGB(1,2,3));
	RTEST(extlogpen.elpHatch == 22);
	RTEST(extlogpen.elpNumEntries == 0);
	DeleteObject(hPen);

	/* A maximum of 16 Styles is allowed */
	hPen = ExtCreatePen(PS_GEOMETRIC | PS_USERSTYLE, 5, &logbrush, 16, (CONST DWORD*)&dwStyles);
	RTEST(GetObject(hPen, 0, NULL) == sizeof(EXTLOGPEN) + 15*sizeof(DWORD));
	RTEST(GetObject(hPen, sizeof(EXTLOGPEN) + 15*sizeof(DWORD), &elpUserStyle) == sizeof(EXTLOGPEN) + 15*sizeof(DWORD));
	RTEST(((EXTLOGPEN*)&elpUserStyle)->elpStyleEntry[0] == 0);
	RTEST(((EXTLOGPEN*)&elpUserStyle)->elpStyleEntry[1] == 1);
	RTEST(((EXTLOGPEN*)&elpUserStyle)->elpStyleEntry[15] == 15);
	DeleteObject(hPen);

	return TRUE;
}

static INT
Test_Font(PTESTINFO pti)
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
	RTEST(hFont);

	SetLastError(ERROR_SUCCESS);
	RTEST(GetObjectA((HANDLE)GDI_OBJECT_TYPE_FONT, 0, NULL) == sizeof(LOGFONTA));
	RTEST(GetObjectW((HANDLE)GDI_OBJECT_TYPE_FONT, 0, NULL) == sizeof(LOGFONTW));
	RTEST(GetObjectA((HANDLE)GDI_OBJECT_TYPE_FONT, sizeof(LOGFONTA), NULL) == sizeof(LOGFONTA)); // 60
	RTEST(GetObjectA((HANDLE)GDI_OBJECT_TYPE_FONT, sizeof(ENUMLOGFONTA), NULL) == sizeof(LOGFONTA)); // 156
	RTEST(GetObjectA((HANDLE)GDI_OBJECT_TYPE_FONT, sizeof(ENUMLOGFONTEXA), NULL) == sizeof(LOGFONTA)); // 188
	RTEST(GetObjectA((HANDLE)GDI_OBJECT_TYPE_FONT, sizeof(EXTLOGFONTA), NULL) == sizeof(LOGFONTA)); // 192
	RTEST(GetObjectA((HANDLE)GDI_OBJECT_TYPE_FONT, sizeof(ENUMLOGFONTEXDVA), NULL) == sizeof(LOGFONTA)); // 260
	RTEST(GetObjectA((HANDLE)GDI_OBJECT_TYPE_FONT, sizeof(ENUMLOGFONTEXDVA)+1, NULL) == sizeof(LOGFONTA)); // 260
	RTEST(GetObjectW((HANDLE)GDI_OBJECT_TYPE_FONT, sizeof(LOGFONTW), NULL) == sizeof(LOGFONTW)); // 92
	RTEST(GetObjectW((HANDLE)GDI_OBJECT_TYPE_FONT, sizeof(ENUMLOGFONTW), NULL) == sizeof(LOGFONTW)); // 284
	RTEST(GetObjectW((HANDLE)GDI_OBJECT_TYPE_FONT, sizeof(EXTLOGFONTW), NULL) == sizeof(LOGFONTW)); // 320
	RTEST(GetObjectW((HANDLE)GDI_OBJECT_TYPE_FONT, sizeof(ENUMLOGFONTEXW), NULL) == sizeof(LOGFONTW)); // 348
	RTEST(GetObjectW((HANDLE)GDI_OBJECT_TYPE_FONT, sizeof(ENUMLOGFONTEXDVW), NULL) == sizeof(LOGFONTW)); // 420
	RTEST(GetObjectW((HANDLE)GDI_OBJECT_TYPE_FONT, sizeof(ENUMLOGFONTEXDVW)+1, NULL) == sizeof(LOGFONTW)); // 356!

	RTEST(GetObjectA(hFont, sizeof(LOGFONTA), NULL) == sizeof(LOGFONTA));
	RTEST(GetObjectA(hFont, 0, NULL) == sizeof(LOGFONTA));
	RTEST(GetObjectA(hFont, 5, NULL) == sizeof(LOGFONTA));
	RTEST(GetObjectA(hFont, -5, NULL) == sizeof(LOGFONTA));
	RTEST(GetObjectA(hFont, 0, &logfonta) == 0);
	RTEST(logfonta.lfHeight == 0x77777777);

	TEST(GetObjectA(hFont, 5, &logfonta) == 5);
	TEST(logfonta.lfHeight == 8);
	TEST(logfonta.lfWidth == 0x77777708);

	RTEST(GetObjectA(hFont, sizeof(LOGFONTA), &logfonta) == sizeof(LOGFONTA)); // 60
	TEST(GetObjectA(hFont, sizeof(LOGFONTW), &logfontw) == sizeof(LOGFONTA)); // 92
	RTEST(GetObjectA(hFont, sizeof(EXTLOGFONTA), &extlogfonta) == sizeof(EXTLOGFONTA)); // 192
	TEST(GetObjectA(hFont, sizeof(EXTLOGFONTA)+1, &extlogfonta) == sizeof(EXTLOGFONTA)+1); // 192
	TEST(GetObjectA(hFont, sizeof(EXTLOGFONTW), &extlogfontw) == sizeof(ENUMLOGFONTEXDVA)); // 320

	TEST(GetObjectA(hFont, 261, &bData) == 260); // no

	/* LOGFONT / GetObjectW */
	FillMemory(&logfontw, sizeof(LOGFONTW), 0x77);

	RTEST(GetObjectW(hFont, sizeof(LOGFONTW), NULL) == sizeof(LOGFONTW));
	RTEST(GetObjectW(hFont, 0, NULL) == sizeof(LOGFONTW));
	RTEST(GetObjectW(hFont, 5, NULL) == sizeof(LOGFONTW));
	RTEST(GetObjectW(hFont, -5, NULL) == sizeof(LOGFONTW));
	RTEST(GetObjectW(hFont, 0, &logfontw) == 0);
	RTEST(logfontw.lfHeight == 0x77777777);

	TEST(GetObjectW(hFont, 5, &logfontw) == 5);
	TEST(logfontw.lfHeight == 8);
	TEST(logfontw.lfWidth == 0x77777708);

	RTEST(GetObjectA(hFont, sizeof(LOGFONTA), &logfonta) == sizeof(LOGFONTA)); // 60
	RTEST(logfonta.lfHeight == 8);
	RTEST(GetObjectA(hFont, sizeof(ENUMLOGFONTA), &enumlogfonta) == sizeof(ENUMLOGFONTA)); // 156
	RTEST(GetObjectA(hFont, sizeof(ENUMLOGFONTEXA), &enumlogfontexa) == sizeof(ENUMLOGFONTEXA)); // 188
	RTEST(GetObjectA(hFont, sizeof(EXTLOGFONTA), &extlogfonta) == sizeof(EXTLOGFONTA)); // 192
	RTEST(GetObjectA(hFont, sizeof(ENUMLOGFONTEXDVA), &enumlogfontexdva) == sizeof(ENUMLOGFONTEXDVA)); // 260
	TEST(GetObjectA(hFont, sizeof(ENUMLOGFONTEXDVA)+1, &enumlogfontexdva) == sizeof(ENUMLOGFONTEXDVA)); // 260

	RTEST(GetObjectW(hFont, sizeof(LOGFONTW), &logfontw) == sizeof(LOGFONTW)); // 92
	RTEST(GetObjectW(hFont, sizeof(ENUMLOGFONTW), &enumlogfontw) == sizeof(ENUMLOGFONTW)); // 284
	RTEST(GetObjectW(hFont, sizeof(EXTLOGFONTW), &extlogfontw) == sizeof(EXTLOGFONTW)); // 320
	RTEST(GetObjectW(hFont, sizeof(ENUMLOGFONTEXW), &enumlogfontexw) == sizeof(ENUMLOGFONTEXW)); // 348
	TEST(GetObjectW(hFont, sizeof(ENUMLOGFONTEXDVW), &enumlogfontexdvw) == sizeof(ENUMLOGFONTEXW) + 2*sizeof(DWORD)); // 420
	TEST(GetObjectW(hFont, sizeof(ENUMLOGFONTEXDVW)+1, &enumlogfontexdvw) == sizeof(ENUMLOGFONTEXW) + 2*sizeof(DWORD)); // 356!

	TEST(GetObjectW(hFont, 356, &bData) == 356);
	TEST(GetLastError() == ERROR_SUCCESS);

	DeleteObject(hFont);

	return TRUE;
}

static INT
Test_Colorspace(PTESTINFO pti)
{
	SetLastError(ERROR_SUCCESS);
	TEST(GetObjectA((HANDLE)GDI_OBJECT_TYPE_COLORSPACE, 0, NULL) == 60);// FIXME: what structure?
	TEST(GetLastError() == ERROR_INSUFFICIENT_BUFFER);
	SetLastError(ERROR_SUCCESS);
	RTEST(GetObjectW((HANDLE)GDI_OBJECT_TYPE_COLORSPACE, 0, NULL) == 0);
	TEST(GetLastError() == ERROR_INSUFFICIENT_BUFFER);

	return TRUE;
}

static INT
Test_MetaDC(PTESTINFO pti)
{
	/* Windows does not SetLastError() on a metadc, but it doesn't seem to do anything with it */
	HDC hMetaDC;
	BYTE buffer[100];

	hMetaDC = CreateMetaFile(NULL);
	if(!hMetaDC) return FALSE;
	if(((UINT)hMetaDC & GDI_HANDLE_TYPE_MASK) != GDI_OBJECT_TYPE_METADC) return FALSE;

	SetLastError(ERROR_SUCCESS);
	RTEST(GetObjectA((HANDLE)GDI_OBJECT_TYPE_METADC, 0, NULL) == 0);
	RTEST(GetObjectA((HANDLE)GDI_OBJECT_TYPE_METADC, 100, &buffer) == 0);
	RTEST(GetObjectA(hMetaDC, 0, NULL) == 0);
	RTEST(GetObjectA(hMetaDC, 100, &buffer) == 0);
	RTEST(GetLastError() == ERROR_SUCCESS);
	return TRUE;
}

INT
Test_GetObject(PTESTINFO pti)
{

    HRGN hRgn;
	hRgn = CreateRectRgn(0,0,5,5);
	SetLastError(ERROR_SUCCESS);
	RTEST(GetObjectW(hRgn, 0, NULL) == 0);
	RTEST(GetLastError() == ERROR_INVALID_HANDLE);
	DeleteObject(hRgn);

	Test_Font(pti);
	Test_Colorspace(pti);
	Test_General(pti);
	Test_Bitmap(pti);
	Test_Dibsection(pti);
	Test_Palette(pti);
	Test_Brush(pti);
	Test_Pen(pti);
	Test_ExtPen(pti); // not implemented yet in ROS
	Test_MetaDC(pti);

	return APISTATUS_NORMAL;
}
