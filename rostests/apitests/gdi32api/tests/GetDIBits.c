INT
Test_GetDIBits(PTESTINFO pti)
{
	HDC hDCScreen;
	HBITMAP hBitmap;
	BITMAPINFO bi;
	INT ScreenBpp;

	hDCScreen = GetDC(NULL);
	if (hDCScreen == NULL)
	{
		return FALSE;
	}

	hBitmap = CreateCompatibleBitmap(hDCScreen, 16, 16);
	RTEST(hBitmap != NULL);

	SetLastError(ERROR_SUCCESS);
	RTEST(GetDIBits(0, 0, 0, 0, NULL, NULL, 0) == 0);
	RTEST(GetLastError() == ERROR_INVALID_PARAMETER);

	SetLastError(ERROR_SUCCESS);
	RTEST(GetDIBits((HDC)2345, 0, 0, 0, NULL, NULL, 0) == 0);
	RTEST(GetLastError() == ERROR_INVALID_PARAMETER);

	SetLastError(ERROR_SUCCESS);
	RTEST(GetDIBits((HDC)2345, hBitmap, 0, 0, NULL, NULL, 0) == 0);
	RTEST(GetLastError() == ERROR_INVALID_PARAMETER);

	SetLastError(ERROR_SUCCESS);
	RTEST(GetDIBits((HDC)2345, hBitmap, 0, 15, NULL, &bi, 0) == 0);
	RTEST(GetLastError() == ERROR_INVALID_PARAMETER);

	SetLastError(ERROR_SUCCESS);
    ZeroMemory(&bi, sizeof(BITMAPINFO));
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	RTEST(GetDIBits(hDCScreen, hBitmap, 0, 15, NULL, &bi, DIB_RGB_COLORS) > 0);
	RTEST(GetLastError() == ERROR_SUCCESS);

	ScreenBpp = GetDeviceCaps(hDCScreen, BITSPIXEL);
	RTEST(bi.bmiHeader.biWidth == 16);
	RTEST(bi.bmiHeader.biHeight == 16);
	RTEST(bi.bmiHeader.biBitCount == ScreenBpp);
	RTEST(bi.bmiHeader.biSizeImage == (16 * 16) * (ScreenBpp / 8));

	DeleteObject(hBitmap);
	ReleaseDC(NULL, hDCScreen);
	return APISTATUS_NORMAL;
}
