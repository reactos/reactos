

FORCEINLINE
PALETTEENTRY
PALENTRY(BYTE r, BYTE g, BYTE b)
{
	PALETTEENTRY ret;

	ret.peRed = r;
	ret.peGreen = g;
	ret.peBlue = b;
	ret.peFlags = 0;
	return ret;
}

INT
Test_NtUserSelectPalette(PTESTINFO pti)
{
	HPALETTE hPal, hOldPal;
	HWND hWnd;
	HDC hDC, hCompDC;
	struct
	{
		LOGPALETTE logpal;
		PALETTEENTRY entry[20];
	} pal;

	ZeroMemory(&pal, sizeof(pal));

	pal.logpal.palVersion = 0x300;
	pal.logpal.palNumEntries = 6;
	pal.entry[0] = PALENTRY(0,0,0);
	pal.entry[1] = PALENTRY(255,255,255);
	pal.entry[2] = PALENTRY(128,128,128);
	pal.entry[3] = PALENTRY(128,0,0);
	pal.entry[4] = PALENTRY(0,128,0);
	pal.entry[5] = PALENTRY(0,0,128);

	hPal = CreatePalette(&pal.logpal);
	ASSERT(hPal);
	TEST(DeletePalette(hPal) == 1);
	hPal = CreatePalette(&pal.logpal);
	ASSERT(hPal);

	/* Create a window */
	hWnd = CreateWindowW(L"BUTTON", L"TestWindow", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
	                    CW_USEDEFAULT, CW_USEDEFAULT, 100, 100,
	                    NULL, NULL, g_hInstance, 0);
	hDC = GetDC(hWnd);
	ASSERT(hDC);
	hCompDC = CreateCompatibleDC(hDC);
	ASSERT(hCompDC);

	/* Test NULL DC */
	SetLastError(ERROR_SUCCESS);
	hOldPal = NtUserSelectPalette(NULL, hPal, 0);
	TEST(hOldPal == 0);
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Test invalid DC */
	SetLastError(ERROR_SUCCESS);
	hOldPal = NtUserSelectPalette((HDC)-1, hPal, 0);
	TEST(hOldPal == 0);
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Test NULL palette */
	SetLastError(ERROR_SUCCESS);
	hOldPal = NtUserSelectPalette(hDC, NULL, 0);
	TEST(hOldPal == 0);
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Test invalid palette */
	SetLastError(ERROR_SUCCESS);
	hOldPal = NtUserSelectPalette(hDC, (HPALETTE)-1, 0);
	TEST(hOldPal == 0);
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Test valid palette */
	hOldPal = NtUserSelectPalette(hDC, hPal, 0);
	TEST(hOldPal != 0);
	TEST(hOldPal == GetStockObject(DEFAULT_PALETTE));

	/* We cannot Delete the palette */
	TEST(DeletePalette(hPal) == 0);

	/* We can still select the Palette into a compatible DC */
	hOldPal = NtUserSelectPalette(hCompDC, hPal, 0);
	TEST(hOldPal != 0);


	return APISTATUS_NORMAL;
}
