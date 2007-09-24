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
	BITMAPINFO bi;
	INT ScreenBpp;

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
	RTEST(NtGdiGetDIBitsInternal((HDC)2345, hBitmap, 0, 15, NULL, &bi, 0, 0, 0) == 0);
	RTEST(GetLastError() == ERROR_SUCCESS);

	SetLastError(ERROR_SUCCESS);
    ZeroMemory(&bi, sizeof(BITMAPINFO));
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	RTEST(NtGdiGetDIBitsInternal(hDCScreen, hBitmap, 0, 15, NULL, &bi, DIB_RGB_COLORS,
								 DIB_BitmapMaxBitsSize(&bi, 15), 0) > 0);
	RTEST(GetLastError() == ERROR_SUCCESS);

	ScreenBpp = GetDeviceCaps(hDCScreen, BITSPIXEL);
	RTEST(bi.bmiHeader.biWidth == 16);
	RTEST(bi.bmiHeader.biHeight == 16);
	RTEST(bi.bmiHeader.biBitCount == ScreenBpp);
	RTEST(bi.bmiHeader.biSizeImage == (16 * 16) * (ScreenBpp / 8));

	ReleaseDC(NULL, hDCScreen);
	DeleteObject(hBitmap);

	return APISTATUS_NORMAL;
}
