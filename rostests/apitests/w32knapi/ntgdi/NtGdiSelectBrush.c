INT
Test_NtGdiSelectBrush(PTESTINFO pti)
{
	HDC hDC;
	HBRUSH hBrush, hOldBrush;

	hDC = CreateDCW(L"DISPLAY", NULL, NULL, NULL);

	hBrush = GetStockObject(GRAY_BRUSH);

	/* Test NULL DC */
	SetLastError(ERROR_SUCCESS);
	hOldBrush = NtGdiSelectBrush(NULL, hBrush);
	TEST(hOldBrush == NULL);
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Test invalid DC */
	SetLastError(ERROR_SUCCESS);
	hOldBrush = NtGdiSelectBrush((HDC)((ULONG_PTR)hDC & 0x0000ffff), hBrush);
	TEST(hOldBrush == NULL);
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Test NULL brush */
	SetLastError(ERROR_SUCCESS);
	hOldBrush = NtGdiSelectBrush(hDC, NULL);
	TEST(hOldBrush == NULL);
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Test invalid brush */
	SetLastError(ERROR_SUCCESS);
	hOldBrush = NtGdiSelectBrush(hDC, (HBRUSH)((ULONG_PTR)hBrush & 0x0000ffff));
	TEST(hOldBrush == NULL);
	TEST(GetLastError() == ERROR_SUCCESS);

	SetLastError(ERROR_SUCCESS);
	hOldBrush = NtGdiSelectBrush(hDC, hBrush);
	TEST(hOldBrush != NULL);
	hOldBrush = NtGdiSelectBrush(hDC, hOldBrush);
	TEST(hOldBrush == hBrush);
	TEST(GetLastError() == ERROR_SUCCESS);


	DeleteDC(hDC);

	return APISTATUS_NORMAL;
}

