#include "../w32knapi.h"

BOOL
STDCALL
NtGdiArcInternal(
        ARCTYPE arctype,
        HDC  hDC,
        int  LeftRect,
        int  TopRect,
        int  RightRect,
        int  BottomRect,
        int  XStartArc,
        int  YStartArc,
        int  XEndArc,
        int  YEndArc)
{
	return (BOOL)Syscall(L"NtGdiArcInternal", 10, &arctype);
}

INT
Test_NtGdiArcInternal(PTESTINFO pti)
{
	HDC hDC = CreateDCW(L"Display",NULL,NULL,NULL);

	SetLastError(ERROR_SUCCESS);
	TEST(NtGdiArcInternal(0, 0, 0, 0, 0, 0, 0, 0, 0, 0) == FALSE);
	TEST(GetLastError() == ERROR_INVALID_HANDLE);

	SetLastError(ERROR_SUCCESS);
	TEST(NtGdiArcInternal(0, hDC, 0, 0, 0, 0, 0, 0, 0, 0) == TRUE);
	TEST(NtGdiArcInternal(1, hDC, 0, 0, 0, 0, 0, 0, 0, 0) == TRUE);
	TEST(NtGdiArcInternal(2, hDC, 0, 0, 0, 0, 0, 0, 0, 0) == TRUE);
	TEST(NtGdiArcInternal(3, hDC, 0, 0, 0, 0, 0, 0, 0, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);

	SetLastError(ERROR_SUCCESS);
	TEST(NtGdiArcInternal(4, hDC, 0, 0, 0, 0, 0, 0, 0, 0) == FALSE);
	TEST(GetLastError() == ERROR_INVALID_PARAMETER);

	SetLastError(ERROR_SUCCESS);
	TEST(NtGdiArcInternal(4, (HDC)10, 0, 0, 0, 0, 0, 0, 0, 0) == FALSE);
	TEST(GetLastError() == ERROR_INVALID_HANDLE);

	SetLastError(ERROR_SUCCESS);
	TEST(NtGdiArcInternal(0, hDC, 10, 10, 0, 0, 0, 0, 0, 0) == TRUE);
	TEST(NtGdiArcInternal(0, hDC, 10, 10, -10, -10, 0, 0, 0, 0) == TRUE);
	TEST(NtGdiArcInternal(0, hDC, 0, 0, 0, 0, 10, 0, -10, 0) == TRUE);

// was passiert, wenn left > right ? einfach tauschen?


	DeleteDC(hDC);

	return APISTATUS_NORMAL;
}
