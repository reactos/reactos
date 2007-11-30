INT
Test_NtGdiSelectPen(PTESTINFO pti)
{
	HDC hDC;
	HPEN hPen, hOldPen;

	hDC = CreateDCW(L"DISPLAY", NULL, NULL, NULL);

	hPen = GetStockObject(WHITE_PEN);

	/* Test NULL DC */
	SetLastError(ERROR_SUCCESS);
	hOldPen = NtGdiSelectPen(NULL, hPen);
	TEST(hOldPen == NULL);
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Test invalid DC */
	SetLastError(ERROR_SUCCESS);
	hOldPen = NtGdiSelectPen((HDC)((ULONG_PTR)hDC & 0x0000ffff), hPen);
	TEST(hOldPen == NULL);
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Test NULL pen */
	SetLastError(ERROR_SUCCESS);
	hOldPen = NtGdiSelectPen(hDC, NULL);
	TEST(hOldPen == NULL);
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Test NULL pen */
	SetLastError(ERROR_SUCCESS);
	hOldPen = NtGdiSelectPen(hDC, (HPEN)((ULONG_PTR)hPen & 0x0000ffff));
	TEST(hOldPen == NULL);
	TEST(GetLastError() == ERROR_SUCCESS);

	SetLastError(ERROR_SUCCESS);
	hOldPen = NtGdiSelectPen(hDC, hPen);
	TEST(hOldPen != NULL);
	hOldPen = NtGdiSelectPen(hDC, hOldPen);
	TEST(hOldPen == hPen);
	TEST(GetLastError() == ERROR_SUCCESS);


	DeleteDC(hDC);

	return APISTATUS_NORMAL;
}
