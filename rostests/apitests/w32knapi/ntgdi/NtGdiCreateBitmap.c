#include "../w32knapi.h"

W32KAPI
HBITMAP
APIENTRY
NtGdiCreateBitmap(
    IN INT cx,
    IN INT cy,
    IN UINT cPlanes,
    IN UINT cBPP,
    IN OPTIONAL LPBYTE pjInit
)
{
	return (HBITMAP)Syscall(L"NtGdiCreateBitmap", 5, &cx);
}

INT
Test_NtGdiCreateBitmap_Params(PTESTINFO pti)
{
	HBITMAP hBmp;
	BITMAP bitmap;
	BYTE BitmapData[10] = {0x11, 0x22, 0x33};

	/* Test simple params */
	SetLastError(ERROR_SUCCESS);
	TEST((hBmp = NtGdiCreateBitmap(1, 1, 1, 1, NULL)) != NULL);
	TEST(GetLastError() == ERROR_SUCCESS);
	DeleteObject(hBmp);

	/* Test all zero */
	SetLastError(ERROR_SUCCESS);
	TEST(NtGdiCreateBitmap(0, 0, 0, 0, NULL) == NULL);
	TEST(GetLastError() == ERROR_INVALID_PARAMETER);

	/* Test cx == 0 */
	SetLastError(ERROR_SUCCESS);
	TEST(NtGdiCreateBitmap(0, 1, 1, 1, NULL) == NULL);
	TEST(GetLastError() == ERROR_INVALID_PARAMETER);

	/* Test negative cx */
	SetLastError(ERROR_SUCCESS);
	TEST(NtGdiCreateBitmap(-10, 1, 1, 1, NULL) == NULL);
	TEST(GetLastError() == ERROR_INVALID_PARAMETER);

	/* Test cy == 0 */
	SetLastError(ERROR_SUCCESS);
	TEST(NtGdiCreateBitmap(1, 0, 1, 1, NULL) == NULL);
	TEST(GetLastError() == ERROR_INVALID_PARAMETER);

	/* Test negative cy */
	SetLastError(ERROR_SUCCESS);
	TEST(NtGdiCreateBitmap(1, -2, 1, 1, NULL) == NULL);
	TEST(GetLastError() == ERROR_INVALID_PARAMETER);

	/* Test negative cy */
	SetLastError(ERROR_SUCCESS);
	TEST(NtGdiCreateBitmap(1, -2, 1, 1, BitmapData) == NULL);
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Test huge size */
	SetLastError(ERROR_SUCCESS);
	TEST(NtGdiCreateBitmap(100000, 100000, 1, 1, NULL) == NULL);
	TEST(GetLastError() == ERROR_NOT_ENOUGH_MEMORY);

	/* Test cPlanes == 0 */
	SetLastError(ERROR_SUCCESS);
	TEST((hBmp = NtGdiCreateBitmap(1, 1, 0, 1, NULL)) != NULL);
	TEST(GetLastError() == ERROR_SUCCESS);
	ASSERT1(GetObject(hBmp, sizeof(BITMAP), &bitmap) == sizeof(BITMAP));
	TEST(bitmap.bmType == 0);
	TEST(bitmap.bmWidth == 1);
	TEST(bitmap.bmHeight == 1);
	TEST(bitmap.bmWidthBytes == 2);
	TEST(bitmap.bmPlanes == 1);
	TEST(bitmap.bmBitsPixel == 1);
	DeleteObject(hBmp);

	/* Test big cPlanes */
	SetLastError(ERROR_SUCCESS);
	TEST((hBmp = NtGdiCreateBitmap(1, 1, 32, 1, NULL)) != NULL);
	TEST(GetLastError() == ERROR_SUCCESS);
	DeleteObject(hBmp);

	TEST(NtGdiCreateBitmap(1, 1, 33, 1, NULL) == NULL);
	TEST(GetLastError() == ERROR_INVALID_PARAMETER);

	/* Test cBPP == 0 */
	SetLastError(ERROR_SUCCESS);
	TEST((hBmp = NtGdiCreateBitmap(1, 1, 1, 0, NULL)) != NULL);
	TEST(GetLastError() == ERROR_SUCCESS);
	ASSERT1(GetObject(hBmp, sizeof(BITMAP), &bitmap) == sizeof(BITMAP));
	TEST(bitmap.bmType == 0);
	TEST(bitmap.bmWidth == 1);
	TEST(bitmap.bmHeight == 1);
	TEST(bitmap.bmWidthBytes == 2);
	TEST(bitmap.bmPlanes == 1);
	TEST(bitmap.bmBitsPixel == 1);
	DeleteObject(hBmp);

	/* Test negative cBPP */
	SetLastError(ERROR_SUCCESS);
	TEST(NtGdiCreateBitmap(1, 1, 1, -1, NULL) == NULL);
	TEST(GetLastError() == ERROR_INVALID_PARAMETER);

	/* Test bad cBPP */
	SetLastError(ERROR_SUCCESS);
	TEST((hBmp = NtGdiCreateBitmap(1, 1, 1, 3, NULL)) != NULL);
	ASSERT1(GetObject(hBmp, sizeof(BITMAP), &bitmap) == sizeof(BITMAP));
	TEST(bitmap.bmBitsPixel == 4);
	DeleteObject(hBmp);

	TEST((hBmp = NtGdiCreateBitmap(1, 1, 1, 6, NULL)) != NULL);
	ASSERT1(GetObject(hBmp, sizeof(BITMAP), &bitmap) == sizeof(BITMAP));
	TEST(bitmap.bmBitsPixel == 8);
	DeleteObject(hBmp);

	TEST((hBmp = NtGdiCreateBitmap(1, 1, 1, 15, NULL)) != NULL);
	ASSERT1(GetObject(hBmp, sizeof(BITMAP), &bitmap) == sizeof(BITMAP));
	TEST(bitmap.bmBitsPixel == 16);
	DeleteObject(hBmp);

	TEST((hBmp = NtGdiCreateBitmap(1, 1, 1, 17, NULL)) != NULL);
	ASSERT1(GetObject(hBmp, sizeof(BITMAP), &bitmap) == sizeof(BITMAP));
	TEST(bitmap.bmBitsPixel == 24);
	DeleteObject(hBmp);

	TEST((hBmp = NtGdiCreateBitmap(1, 1, 3, 7, NULL)) != NULL);
	ASSERT1(GetObject(hBmp, sizeof(BITMAP), &bitmap) == sizeof(BITMAP));
	TEST(bitmap.bmBitsPixel == 24);
	DeleteObject(hBmp);

	TEST((hBmp = NtGdiCreateBitmap(1, 1, 1, 25, NULL)) != NULL);
	ASSERT1(GetObject(hBmp, sizeof(BITMAP), &bitmap) == sizeof(BITMAP));
	TEST(bitmap.bmBitsPixel == 32);
	DeleteObject(hBmp);

	TEST(GetLastError() == ERROR_SUCCESS);

	TEST(NtGdiCreateBitmap(1, 1, 1, 33, NULL) == NULL);
	TEST(GetLastError() == ERROR_INVALID_PARAMETER);

	/* Test bad pointer */
	SetLastError(ERROR_SUCCESS);
	TEST(NtGdiCreateBitmap(1, 1, 1, 1, (BYTE*)0x80001234) == NULL);
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Test pointer alignment */
	SetLastError(ERROR_SUCCESS);
	TEST((hBmp = NtGdiCreateBitmap(1, 1, 1, 1, &BitmapData[1])) != NULL);
	TEST(GetLastError() == ERROR_SUCCESS);
	DeleteObject(hBmp);

	/* Test normal params */
	SetLastError(ERROR_SUCCESS);
	TEST((hBmp = NtGdiCreateBitmap(5, 7, 2, 4, NULL)) != NULL);
	TEST(GetLastError() == ERROR_SUCCESS);
	ASSERT1(GetObject(hBmp, sizeof(BITMAP), &bitmap) == sizeof(BITMAP));
	TEST(bitmap.bmType == 0);
	TEST(bitmap.bmWidth == 5);
	TEST(bitmap.bmHeight == 7);
	TEST(bitmap.bmWidthBytes == 6);
	TEST(bitmap.bmPlanes == 1);
	TEST(bitmap.bmBitsPixel == 8);
	DeleteObject(hBmp);

	return APISTATUS_NORMAL;
}

INT
Test_NtGdiCreateBitmap(PTESTINFO pti)
{
	INT ret;

	ret = Test_NtGdiCreateBitmap_Params(pti);
	if (ret != APISTATUS_NORMAL)
		return ret;

//	ret = Test_NtGdiCreateBitmap_Pixel(pti);
//	if (ret != APISTATUS_NORMAL)
//		return ret;

	return APISTATUS_NORMAL;

}
