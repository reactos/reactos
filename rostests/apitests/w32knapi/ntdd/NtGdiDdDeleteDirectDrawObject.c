#include "../w32knapi.h"

W32KAPI
BOOL
APIENTRY
NtGdiDdDeleteDirectDrawObject(
    HANDLE hDirectDrawLocal
)
{
	return (BOOL)Syscall(L"NtGdiDdDeleteDirectDrawObject", 1, &hDirectDrawLocal);
}

INT
Test_NtGdiDdDeleteDirectDrawObject(PTESTINFO pti)
{
	HANDLE  hDirectDraw;
	HDC hdc = CreateDCW(L"DISPLAY",NULL,NULL,NULL);
	ASSERT1(hdc != NULL);

	/* Test ReactX */
	RTEST(NtGdiDdDeleteDirectDrawObject(NULL) == FALSE);
	RTEST((hDirectDraw=NtGdiDdCreateDirectDrawObject(hdc)) != NULL);
	ASSERT1(hDirectDraw != NULL);
	RTEST(NtGdiDdDeleteDirectDrawObject(hDirectDraw) == TRUE);

	/* Cleanup ReactX setup */
	DeleteDC(hdc);
	Syscall(L"NtGdiDdDeleteDirectDrawObject", 1, &hDirectDraw);

	return APISTATUS_NORMAL;
}
