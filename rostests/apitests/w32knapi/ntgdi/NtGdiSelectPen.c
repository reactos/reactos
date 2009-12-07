INT
Test_NtGdiSelectPen(PTESTINFO pti)
{
	HDC hDC;
	HPEN hPen, hOldPen;
	LOGBRUSH logbrush;

	hDC = GetDC(NULL);
	ASSERT(hDC);

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

	/* Test invalid pen */
	SetLastError(ERROR_SUCCESS);
	hOldPen = NtGdiSelectPen(hDC, (HPEN)((ULONG_PTR)hPen & 0x0000ffff));
	TEST(hOldPen == NULL);
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Test valid pen */
	SelectObject(hDC, GetStockObject(BLACK_PEN));
	SetLastError(ERROR_SUCCESS);
	hOldPen = NtGdiSelectPen(hDC, hPen);
	TEST(hOldPen == GetStockObject(BLACK_PEN));
	hOldPen = NtGdiSelectPen(hDC, hOldPen);
	TEST(hOldPen == hPen);
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Test extpen */
	SetLastError(ERROR_SUCCESS);
	logbrush.lbStyle = BS_SOLID;
	logbrush.lbColor = RGB(0x12,0x34,0x56);
	hPen = ExtCreatePen(PS_COSMETIC|PS_ALTERNATE, 1, &logbrush, 0, NULL);
	ASSERT(hPen);
	hOldPen = NtGdiSelectPen(hDC, hPen);
	TEST(hOldPen != NULL);
	hOldPen = NtGdiSelectPen(hDC, hOldPen);
	TEST(hOldPen == hPen);
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Test deleting pen */
	SetLastError(ERROR_SUCCESS);
	hOldPen = NtGdiSelectPen(hDC, hPen);
	TEST(DeleteObject(hPen) == 1);
	hOldPen = NtGdiSelectPen(hDC, hOldPen);
	TEST(hOldPen == hPen);
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Test that fallback pen is BLACK_PEN */

	DeleteDC(hDC);

	return APISTATUS_NORMAL;
}
