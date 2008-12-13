
INT
Test_NtGdiCreateCompatibleDC(PTESTINFO pti)
{
	HDC hDC;
	HGDIOBJ hObj;

	/* Test if aa NULL DC is accepted */
	hDC = NtGdiCreateCompatibleDC(NULL);
	TEST(hDC != NULL);

	/* We select a nwe palette. Note: SelectObject doesn't work with palettes! */
	hObj = SelectPalette(hDC, GetStockObject(DEFAULT_PALETTE), 0);
	/* The old palette should be GetStockObject(DEFAULT_PALETTE) */
	TEST(hObj == GetStockObject(DEFAULT_PALETTE));

	/* The default bitmap should be GetStockObject(21) */
	hObj = SelectObject(hDC, GetStockObject(21));
	TEST(hObj == GetStockObject(21));

	/* The default pen should be GetStockObject(BLACK_PEN) */
	hObj = SelectObject(hDC, GetStockObject(WHITE_PEN));
	TEST(hObj == GetStockObject(BLACK_PEN));

	return APISTATUS_NORMAL;
}

