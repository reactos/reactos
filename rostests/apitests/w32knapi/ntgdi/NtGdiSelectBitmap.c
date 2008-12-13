INT
Test_NtGdiSelectBitmap(PTESTINFO pti)
{
	HDC hDC;
	HBITMAP hBmp, hOldBmp;
	HPALETTE hOldPalette, hPalette;
	LOGPALETTE logpal = {0x300, 1, {{12,13,14,15}}};

	hBmp = CreateBitmap(2,2,1,1,NULL);
	ASSERT(hBmp);

	/* We cannot select a bitmap into a display DC */
	hDC = GetDC(NULL);
	ASSERT(hDC);
	hOldBmp = NtGdiSelectBitmap(hDC, hBmp);
	TEST(hOldBmp == NULL);

	hDC = CreateCompatibleDC(GetDC(NULL));
	ASSERT(hDC);

	/* Check the palette before we mess it up*/
	hPalette = CreatePalette(&logpal);
	hOldPalette = SelectPalette(hDC, hPalette, 0);
	TEST(hOldPalette == GetStockObject(DEFAULT_PALETTE));

	/* Test NULL DC */
	SetLastError(ERROR_SUCCESS);
	hOldBmp = NtGdiSelectBitmap(NULL, hBmp);
	TEST(hOldBmp == NULL);
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Test invalid DC */
	SetLastError(ERROR_SUCCESS);
	hOldBmp = NtGdiSelectBitmap((HDC)((ULONG_PTR)hDC & 0x0000ffff), hBmp);
	TEST(hOldBmp == NULL);
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Test NULL bitmap */
	SetLastError(ERROR_SUCCESS);
	hOldBmp = NtGdiSelectBitmap(hDC, NULL);
	TEST(hOldBmp == NULL);
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Test bitmap with only index */
	SetLastError(ERROR_SUCCESS);
	hOldBmp = NtGdiSelectBitmap(hDC, (HBITMAP)((ULONG_PTR)hBmp & 0x0000ffff));
	TEST(hOldBmp == NULL);
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Test valid bitmap */
	SetLastError(ERROR_SUCCESS);
	hOldBmp = NtGdiSelectBitmap(hDC, hBmp);
	TEST(hOldBmp != NULL);
	/* The default bitmap should be GetStockObject(21) */
	TEST(hOldBmp == GetStockObject(21));

	/* Check the palette */
	hOldPalette = SelectPalette(hDC, hOldPalette, 0);
	TEST(hOldPalette == hPalette);
	DeleteObject(hPalette);

	/* Select the old one again and check */
	hOldBmp = NtGdiSelectBitmap(hDC, hOldBmp);
	TEST(hOldBmp == hBmp);
	TEST(GetLastError() == ERROR_SUCCESS);

	/* cleanup */
	DeleteObject(hBmp);
	DeleteDC(hDC);

	return APISTATUS_NORMAL;
}

