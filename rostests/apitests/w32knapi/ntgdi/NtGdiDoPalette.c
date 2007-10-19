HPALETTE
CreateTestPalette()
{
	struct
	{
		LOGPALETTE logpal;
		PALETTEENTRY entry[5];
	} palstruct =
	{ {0x300,5,
	  { {1,2,3,0} }},
	  { {22,33,44,PC_RESERVED},
	    {11,55,77,PC_EXPLICIT},
	    {00,77,66,PC_RESERVED | PC_NOCOLLAPSE},
	    {12,34,56,78}} };

	return CreatePalette((LOGPALETTE*)&palstruct);
}

INT
Test_NtGdiDoPalette_GdiPalAnimate(PTESTINFO pti)
{
	HPALETTE hPal;
	PALETTEENTRY palEntries[5] = {
		{0,0,0,0},
		{0xff,0xff,0xff,0},
		{0x33,0x66,0x99,0},
		{0x25,0x84,0x14,0},
		{0x12,0x34,0x56,0x11}};
	PALETTEENTRY palEntries2[5];

	/* Test stock palette */
	SetLastError(ERROR_SUCCESS);
	RTEST(NtGdiDoPalette(GetStockObject(DEFAULT_PALETTE), 0, 1, palEntries, GdiPalAnimate, FALSE) == 0);
	RTEST(GetLastError() == ERROR_SUCCESS);


	/* Test pEntries = NULL */
	hPal = CreateTestPalette();
	RTEST(NtGdiDoPalette(hPal, 0, 1, NULL, GdiPalAnimate, TRUE) == 0);
	RTEST(NtGdiDoPalette(hPal, 0, 1, NULL, GdiPalAnimate, FALSE) == 0);
	DeleteObject(hPal);

	/* Test PC_RESERVED */
	hPal = CreateTestPalette();
	RTEST(NtGdiDoPalette(hPal, 0, 5, palEntries, GdiPalAnimate, TRUE) == 2);
	DeleteObject(hPal);

	hPal = CreateTestPalette();
	RTEST(NtGdiDoPalette(hPal, 1, 5, palEntries, GdiPalAnimate, TRUE) == 2);
	DeleteObject(hPal);

	hPal = CreateTestPalette();
	RTEST(NtGdiDoPalette(hPal, 2, 5, palEntries, GdiPalAnimate, TRUE) == 1);
	DeleteObject(hPal);

	hPal = CreateTestPalette();
	RTEST(NtGdiDoPalette(hPal, 3, 5, palEntries, GdiPalAnimate, TRUE) == 1);
	DeleteObject(hPal);

	hPal = CreateTestPalette();
	RTEST(NtGdiDoPalette(hPal, 4, 5, palEntries, GdiPalAnimate, TRUE) == 0);
	DeleteObject(hPal);

	hPal = CreateTestPalette();
	RTEST(NtGdiDoPalette(hPal, 5, 5, palEntries, GdiPalAnimate, TRUE) == 0);
	DeleteObject(hPal);

	hPal = CreateTestPalette();
	RTEST(NtGdiDoPalette(hPal, 0, 5, palEntries, GdiPalAnimate, FALSE) == 2);
	DeleteObject(hPal);

	hPal = CreateTestPalette();
	RTEST(NtGdiDoPalette(hPal, 3, 5, palEntries, GdiPalAnimate, FALSE) == 1);
	DeleteObject(hPal);

	/* Test if entries are set correctly */
	hPal = CreateTestPalette();
	NtGdiDoPalette(hPal, 0, 5, palEntries, GdiPalAnimate, TRUE);
	NtGdiDoPalette(hPal, 0, 5, palEntries2, GdiPalGetEntries, FALSE);
	RTEST(palEntries2[0].peRed == 1);
	RTEST(palEntries2[0].peGreen == 2);
	RTEST(palEntries2[0].peBlue == 3);
	RTEST(palEntries2[0].peFlags == 0);
	RTEST(palEntries2[1].peRed == palEntries[1].peRed);
	RTEST(palEntries2[1].peGreen == palEntries[1].peGreen);
	RTEST(palEntries2[1].peBlue == palEntries[1].peBlue);
	RTEST(palEntries2[1].peFlags == palEntries[1].peFlags);
	RTEST(palEntries2[2].peRed == 11);
	RTEST(palEntries2[2].peGreen == 55);
	RTEST(palEntries2[2].peBlue == 77);
	TEST(palEntries2[2].peFlags == PC_EXPLICIT);
	RTEST(palEntries2[3].peRed == palEntries[3].peRed);
	RTEST(palEntries2[3].peGreen == palEntries[3].peGreen);
	RTEST(palEntries2[3].peBlue == palEntries[3].peBlue);
	RTEST(palEntries2[3].peFlags == palEntries[3].peFlags);
	DeleteObject(hPal);


	return APISTATUS_NORMAL;
}

INT
Test_NtGdiDoPalette_GdiPalSetEntries(PTESTINFO pti)
{
	HPALETTE hPal;
	PALETTEENTRY palEntries[5] = {
		{0,0,0,0},
		{0xff,0xff,0xff,0},
		{0x33,0x66,0x99,0},
		{0x25,0x84,0x14,0},
		{0x12,0x34,0x56,0x11}};
	PALETTEENTRY palEntries2[5];

	hPal = CreateTestPalette();

	/* Test invalid handle */
	SetLastError(ERROR_SUCCESS);
	RTEST(NtGdiDoPalette((HPALETTE)23, 0, 1, palEntries, GdiPalSetEntries, TRUE) == 0);
	RTEST(GetLastError() == ERROR_SUCCESS);

	/* Test system palette */
	RTEST(NtGdiDoPalette(GetStockObject(DEFAULT_PALETTE), 0, 1, palEntries, GdiPalSetEntries, TRUE) == 0);
	RTEST(GetLastError() == ERROR_SUCCESS);

	RTEST(NtGdiDoPalette(hPal, 0, 1, palEntries, GdiPalSetEntries, TRUE) == 1);
	RTEST(NtGdiDoPalette(hPal, 0, 2, palEntries, GdiPalSetEntries, TRUE) == 2);
	RTEST(NtGdiDoPalette(hPal, 0, 3, palEntries, GdiPalSetEntries, TRUE) == 3);
	RTEST(NtGdiDoPalette(hPal, 0, 5, palEntries, GdiPalSetEntries, TRUE) == 5);
	RTEST(NtGdiDoPalette(hPal, 0, 6, palEntries, GdiPalSetEntries, TRUE) == 5);
	RTEST(NtGdiDoPalette(hPal, 3, 6, palEntries, GdiPalSetEntries, TRUE) == 2);
//	TEST(NtGdiDoPalette(hPal, 4, 23247, palEntries, GdiPalSetEntries, TRUE) == 0);

	/* Test bInbound == FALSE */
	NtGdiDoPalette(hPal, 0, 5, palEntries, GdiPalSetEntries, TRUE);
	ZeroMemory(palEntries2, sizeof(palEntries2));
	RTEST(NtGdiDoPalette(hPal, 0, 5, palEntries2, GdiPalSetEntries, FALSE) == 5);
	/* we should get the old values returned in our buffer! */
	TEST(memcmp(palEntries2, palEntries, sizeof(palEntries)) == 0);

	/* check what we have in our palette now */
	ZeroMemory(palEntries2, sizeof(palEntries2));
	RTEST(NtGdiDoPalette(hPal, 0, 5, palEntries2, GdiPalGetEntries, FALSE) == 5);
	TEST(memcmp(palEntries2, palEntries, sizeof(palEntries)) == 0);

	RTEST(NtGdiDoPalette(hPal, 0, 4, palEntries2, GdiPalSetEntries, TRUE) == 4);
	RTEST(GetLastError() == ERROR_SUCCESS);

	/* Test if entries are set correctly */
	hPal = CreateTestPalette();
	NtGdiDoPalette(hPal, 0, 5, palEntries, GdiPalSetEntries, TRUE);
	NtGdiDoPalette(hPal, 0, 5, palEntries2, GdiPalGetEntries, FALSE);
	RTEST(palEntries2[0].peRed == 0);
	RTEST(palEntries2[0].peGreen == 0);
	RTEST(palEntries2[0].peBlue == 0);
	RTEST(palEntries2[0].peFlags == 0);

	/* Test that the buffer was not changed */

	/* Test pEntries = NULL */
	RTEST(NtGdiDoPalette(hPal, 0, 1, NULL, GdiPalGetEntries, TRUE) == 0);

	return APISTATUS_NORMAL;
}

INT
Test_NtGdiDoPalette_GdiPalGetEntries(PTESTINFO pti)
{
	HPALETTE hPal;

	hPal = CreateTestPalette();

	/* Test pEntries = NULL */
	RTEST(NtGdiDoPalette(hPal, 0, 1, NULL, GdiPalGetEntries, TRUE) == 0);
	RTEST(NtGdiDoPalette(hPal, 0, 1, NULL, GdiPalGetEntries, FALSE) == 5);
	RTEST(NtGdiDoPalette(hPal, 2, 1, NULL, GdiPalGetEntries, FALSE) == 5);
	RTEST(NtGdiDoPalette(hPal, 20, 1, NULL, GdiPalGetEntries, FALSE) == 5);
	RTEST(NtGdiDoPalette(hPal, -20, 1, NULL, GdiPalGetEntries, FALSE) == 5);
	RTEST(NtGdiDoPalette(hPal, 2, 0, NULL, GdiPalGetEntries, FALSE) == 5);


// Test flags 0xf0

	return APISTATUS_NORMAL;
}

INT
Test_NtGdiDoPalette_GetSystemPalette(PTESTINFO pti)
{
	return APISTATUS_NORMAL;
}

INT
Test_NtGdiDoPalette_GetBIBColorTable(PTESTINFO pti)
{
	return APISTATUS_NORMAL;
}

INT
Test_NtGdiDoPalette_SetDIBColorTable(PTESTINFO pti)
{
	return APISTATUS_NORMAL;
}


INT
Test_NtGdiDoPalette(PTESTINFO pti)
{
	INT ret;

	ret = Test_NtGdiDoPalette_GdiPalAnimate(pti);
	if (ret != APISTATUS_NORMAL)
	{
		return ret;
	}

	ret = Test_NtGdiDoPalette_GdiPalSetEntries(pti);
	if (ret != APISTATUS_NORMAL)
	{
		return ret;
	}

	ret = Test_NtGdiDoPalette_GdiPalGetEntries(pti);
	if (ret != APISTATUS_NORMAL)
	{
		return ret;
	}

	ret = Test_NtGdiDoPalette_GetSystemPalette(pti);
	if (ret != APISTATUS_NORMAL)
	{
		return ret;
	}

	ret = Test_NtGdiDoPalette_GetBIBColorTable(pti);
	if (ret != APISTATUS_NORMAL)
	{
		return ret;
	}

	ret = Test_NtGdiDoPalette_SetDIBColorTable(pti);
	if (ret != APISTATUS_NORMAL)
	{
		return ret;
	}

	return APISTATUS_NORMAL;
}
