/* taken from gdi32, bitmap.c */
UINT
FASTCALL
DIB_BitmapMaxBitsSize( PBITMAPINFO Info, UINT ScanLines )
{
    UINT MaxBits = 0;

    if (Info->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
    {
       PBITMAPCOREHEADER Core = (PBITMAPCOREHEADER)Info;
       MaxBits = Core->bcBitCount * Core->bcPlanes * Core->bcWidth;
    }
    else  /* assume BITMAPINFOHEADER */
    {
       if ((Info->bmiHeader.biCompression) && (Info->bmiHeader.biCompression != BI_BITFIELDS))
           return Info->bmiHeader.biSizeImage;
    // Planes are over looked by Yuan. I guess assumed always 1.
       MaxBits = Info->bmiHeader.biBitCount * Info->bmiHeader.biPlanes * Info->bmiHeader.biWidth;
    }
    MaxBits = ((MaxBits + 31) & ~31 ) / 8; // From Yuan, ScanLineSize = (Width * bitcount + 31)/32
    return (MaxBits * ScanLines);  // ret the full Size.
}


INT
Test_NtGdiGetDIBitsInternal(PTESTINFO pti)
{
	HBITMAP hBitmap;
	struct
	{
		BITMAPINFO bi;
		RGBQUAD Colors[20];
	} bmp;
//	BITMAPINFO bi;
	INT ScreenBpp;
	BITMAPCOREINFO bic;
	DWORD data[20*16];

	HDC hDCScreen = GetDC(NULL);
	ASSERT(hDCScreen != NULL);

	hBitmap = CreateCompatibleBitmap(hDCScreen, 16, 16);
	ASSERT(hBitmap != NULL);

	SetLastError(ERROR_SUCCESS);
	RTEST(NtGdiGetDIBitsInternal(0, 0, 0, 0, NULL, NULL, 0, 0, 0) == 0);
	RTEST(GetLastError() == ERROR_SUCCESS);

	SetLastError(ERROR_SUCCESS);
	RTEST(NtGdiGetDIBitsInternal((HDC)2345, 0, 0, 0, NULL, NULL, 0, 0, 0) == 0);
	RTEST(GetLastError() == ERROR_SUCCESS);

	SetLastError(ERROR_SUCCESS);
	RTEST(NtGdiGetDIBitsInternal((HDC)2345, hBitmap, 0, 0, NULL, NULL, 0, 0, 0) == 0);
	RTEST(GetLastError() == ERROR_SUCCESS);

	SetLastError(ERROR_SUCCESS);
	RTEST(NtGdiGetDIBitsInternal(hDCScreen, hBitmap, 0, 0, NULL, NULL, 0, 0, 0) == 0);
	RTEST(GetLastError() == ERROR_SUCCESS);

	SetLastError(ERROR_SUCCESS);
	RTEST(NtGdiGetDIBitsInternal(hDCScreen, hBitmap, 0, 15, NULL, NULL, 0, 0, 0) == 0);
	RTEST(GetLastError() == ERROR_SUCCESS);

	ZeroMemory(&bmp, sizeof(bmp));
	bmp.bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	FillMemory(&bmp.Colors, sizeof(bmp.Colors), 0x44);

	SetLastError(ERROR_SUCCESS);
	RTEST(NtGdiGetDIBitsInternal((HDC)0, hBitmap, 0, 15, NULL, &bmp.bi, 0, 0, 0) > 0);
	RTEST(GetLastError() == ERROR_SUCCESS);
	TEST(bmp.Colors[0].rgbRed == 0x44);

	ZeroMemory(&bmp, sizeof(bmp));
	bmp.bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	FillMemory(&bmp.Colors, sizeof(bmp.Colors), 0x44);

	SetLastError(ERROR_SUCCESS);
	RTEST(NtGdiGetDIBitsInternal((HDC)2345, hBitmap, 0, 15, NULL, &bmp.bi, 0, 0, 0) > 0);
	RTEST(GetLastError() == ERROR_SUCCESS);
	TEST(bmp.Colors[0].rgbRed == 0x44);

	ZeroMemory(&bmp, sizeof(bmp));
	bmp.bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	FillMemory(&bmp.Colors, sizeof(bmp.Colors), 0x44);

	SetLastError(ERROR_SUCCESS);
	RTEST(NtGdiGetDIBitsInternal(hDCScreen, hBitmap, 0, 15, NULL, &bmp.bi, DIB_RGB_COLORS,
								 DIB_BitmapMaxBitsSize(&bmp.bi, 15), 0) > 0);
	RTEST(GetLastError() == ERROR_SUCCESS);

	ScreenBpp = GetDeviceCaps(hDCScreen, BITSPIXEL);

	RTEST(bmp.bi.bmiHeader.biWidth == 16);
	RTEST(bmp.bi.bmiHeader.biHeight == 16);
	RTEST(bmp.bi.bmiHeader.biBitCount == ScreenBpp);
	RTEST(bmp.bi.bmiHeader.biSizeImage == (16 * 16) * (ScreenBpp / 8));

	TEST(bmp.Colors[0].rgbRed == 0x44);

	/* Test with pointer */
//	ZeroMemory(&bmp.bi, sizeof(BITMAPINFO));
	bmp.bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
//	FillMemory(&bmp.Colors, sizeof(bmp.Colors), 0x11223344);

	SetLastError(ERROR_SUCCESS);
	TEST(NtGdiGetDIBitsInternal(hDCScreen, hBitmap, 0, 15, (void*)data, &bmp.bi, DIB_RGB_COLORS,
								 DIB_BitmapMaxBitsSize(&bmp.bi, 15), 0) > 0);
	RTEST(GetLastError() == ERROR_SUCCESS);

	RTEST(bmp.bi.bmiHeader.biWidth == 16);
	RTEST(bmp.bi.bmiHeader.biHeight == 16);
	RTEST(bmp.bi.bmiHeader.biBitCount == ScreenBpp);
	RTEST(bmp.bi.bmiHeader.biSizeImage == (16 * 16) * (ScreenBpp / 8));

	TEST(bmp.Colors[0].rgbRed != 0x44);

	/* Test a BITMAPCOREINFO structure */
	SetLastError(ERROR_SUCCESS);
	ZeroMemory(&bic, sizeof(BITMAPCOREINFO));
	bic.bmciHeader.bcSize = sizeof(BITMAPCOREHEADER);
	TEST(NtGdiGetDIBitsInternal(hDCScreen, hBitmap, 0, 15, NULL, (PBITMAPINFO)&bic, DIB_RGB_COLORS,
								 DIB_BitmapMaxBitsSize((PBITMAPINFO)&bic, 15), 0) > 0);
	RTEST(GetLastError() == ERROR_SUCCESS);


	ReleaseDC(NULL, hDCScreen);
	DeleteObject(hBitmap);

	return APISTATUS_NORMAL;
}
