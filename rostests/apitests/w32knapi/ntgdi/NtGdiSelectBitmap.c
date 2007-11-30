INT
Test_NtGdiSelectBitmap(PTESTINFO pti)
{
	HDC hDC;
	HBITMAP hBmp, hOldBmp;

	hDC = CreateCompatibleDC(GetDC(NULL));
	ASSERT(hDC);

	hBmp = CreateBitmap(2,2,1,1,NULL);

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

	/* Test NULL bitmap */
	SetLastError(ERROR_SUCCESS);
	hOldBmp = NtGdiSelectBitmap(hDC, (HBITMAP)((ULONG_PTR)hBmp & 0x0000ffff));
	TEST(hOldBmp == NULL);
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Test invalid bitmap */
	SetLastError(ERROR_SUCCESS);
	hOldBmp = NtGdiSelectBitmap(hDC, hBmp);
	TEST(hOldBmp != NULL);
	hOldBmp = NtGdiSelectBitmap(hDC, hOldBmp);
	TEST(hOldBmp == hBmp);
	TEST(GetLastError() == ERROR_SUCCESS);

	DeleteObject(hBmp);
	DeleteDC(hDC);

	return APISTATUS_NORMAL;
}

