#include "../w32knapi.h"

LONG STDCALL
NtGdiGetBitmapBits(
	HBITMAP  hBitmap,
	DWORD  Bytes,
	IN PBYTE Bits)
{
	return (LONG)Syscall(L"NtGdiGetBitmapBits", 3, &hBitmap);
}


INT
Test_NtGdiGetBitmapBits(PTESTINFO pti)
{
	BYTE Bits[50] = {0,1,2,3,4,5,6,7,8,9};
	HBITMAP hBitmap;

	SetLastError(ERROR_SUCCESS);
	RTEST(NtGdiGetBitmapBits(0, 0, 0) == 0);
	RTEST(GetLastError() == ERROR_INVALID_HANDLE);

	/* Test NULL bitmap handle */
	SetLastError(ERROR_SUCCESS);
	RTEST(NtGdiGetBitmapBits(0, 5, Bits) == 0);
	RTEST(GetLastError() == ERROR_INVALID_HANDLE);

	/* Test invalid bitmap handle */
	hBitmap = (HBITMAP)CreatePen(PS_SOLID, 1, RGB(1,2,3));
	SetLastError(ERROR_SUCCESS);
	RTEST(NtGdiGetBitmapBits(hBitmap, 5, Bits) == 0);
	RTEST(GetLastError() == ERROR_INVALID_HANDLE);
	DeleteObject(hBitmap);

	hBitmap = CreateBitmap(3, 3, 1, 8, NULL);
	SetLastError(ERROR_SUCCESS);

	/* test NULL pointer and count buffer size != 0 */
	RTEST(NtGdiGetBitmapBits(hBitmap, 5, NULL) == 12);

	/* test NULL pointer and buffer size == 0*/
	RTEST(NtGdiGetBitmapBits(hBitmap, 0, NULL) == 12);

	/* test bad pointer */
	RTEST(NtGdiGetBitmapBits(hBitmap, 5, (PBYTE)0x500) == 0);

	/* Test if we can set a number of bytes between lines */
	RTEST(NtGdiGetBitmapBits(hBitmap, 5, Bits) == 5);

	/* Test alignment */
	RTEST(NtGdiGetBitmapBits(hBitmap, 4, Bits+1) == 4);

	/* Test 1 byte too much */
	RTEST(NtGdiGetBitmapBits(hBitmap, 10, Bits) == 10);

	/* Test one row too much */
	RTEST(NtGdiGetBitmapBits(hBitmap, 12, Bits) == 12);

	RTEST(NtGdiGetBitmapBits(hBitmap, 13, Bits) == 12);

	RTEST(NtGdiGetBitmapBits(hBitmap, 100, Bits) == 12);

	/* Test huge bytes count */
	RTEST(NtGdiGetBitmapBits(hBitmap, 12345678, Bits) == 12);

	/* Test negative bytes count */
	RTEST(NtGdiGetBitmapBits(hBitmap, -5, Bits) == 12);

	RTEST(GetLastError() == ERROR_SUCCESS);

	DeleteObject(hBitmap);

	return APISTATUS_NORMAL;
}
