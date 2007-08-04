#include "../w32knapi.h"

LONG STDCALL
NtGdiSetBitmapBits(
	HBITMAP  hBitmap,
	DWORD  Bytes,
	IN PBYTE Bits)
{
	return (LONG)Syscall(L"NtGdiSetBitmapBits", 3, &hBitmap);
}


INT
Test_NtGdiSetBitmapBits(PTESTINFO pti)
{
	BYTE Bits[50] = {0,1,2,3,4,5,6,7,8,9};
	HBITMAP hBitmap;

	SetLastError(ERROR_SUCCESS);
	RTEST(NtGdiSetBitmapBits(0, 0, 0) == 0);
	RTEST(GetLastError() == ERROR_SUCCESS);

	/* Test NULL bitnap handle */
	SetLastError(ERROR_SUCCESS);
	RTEST(NtGdiSetBitmapBits(0, 5, Bits) == 0);
	RTEST(GetLastError() == ERROR_INVALID_HANDLE);

	/* Test invalid bitmap handle */
	hBitmap = (HBITMAP)CreatePen(PS_SOLID, 1, RGB(1,2,3));
	SetLastError(ERROR_SUCCESS);
	RTEST(NtGdiSetBitmapBits(hBitmap, 5, Bits) == 0);
	RTEST(GetLastError() == ERROR_INVALID_HANDLE);
	DeleteObject(hBitmap);

	hBitmap = CreateBitmap(3, 3, 1, 8, NULL);
	SetLastError(ERROR_SUCCESS);

	/* test NULL pointer and count buffer size != 0 */
	RTEST(NtGdiSetBitmapBits(hBitmap, 5, NULL) == 0);

	/* test NULL pointer and buffer size == 0*/
	RTEST(NtGdiSetBitmapBits(hBitmap, 0, NULL) == 0);

	/* test bad pointer */
	RTEST(NtGdiSetBitmapBits(hBitmap, 5, (PBYTE)0x500) == 0);

	/* Test if we can set a number of bytes between lines */
	RTEST(NtGdiSetBitmapBits(hBitmap, 5, Bits) == 5);

	/* Test alignment */
	RTEST(NtGdiSetBitmapBits(hBitmap, 4, Bits+1) == 4);

	/* Test 1 byte too much */
	RTEST(NtGdiSetBitmapBits(hBitmap, 10, Bits) == 10);

	/* Test one row too much */
	RTEST(NtGdiSetBitmapBits(hBitmap, 12, Bits) == 12);

	RTEST(NtGdiSetBitmapBits(hBitmap, 13, Bits) == 12);

	RTEST(NtGdiSetBitmapBits(hBitmap, 100, Bits) == 12);

	/* Test huge bytes count */
	TEST(NtGdiSetBitmapBits(hBitmap, 12345678, Bits) == 0);

	/* Test negative bytes count */
	RTEST(NtGdiSetBitmapBits(hBitmap, -5, Bits) == 0);

	RTEST(GetLastError() == ERROR_SUCCESS);

	DeleteObject(hBitmap);

	return APISTATUS_NORMAL;
}
