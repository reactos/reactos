#include "../w32knapi.h"


W32KAPI
HANDLE
APIENTRY
NtGdiDdCreateDirectDrawObject(
    IN HDC hdc
)
{
	return (HANDLE)Syscall(L"NtGdiDdCreateDirectDrawObject", 1, &hdc);
}

INT
Test_NtGdiDdCreateDirectDrawObject(PTESTINFO pti)
{
	HANDLE  hDirectDraw;
	HDC hdc = CreateDCW(L"DISPLAY",NULL,NULL,NULL);
	ASSERT1(hdc != NULL);

	/* Test ReactX */
	RTEST(NtGdiDdCreateDirectDrawObject(NULL) == NULL);
	RTEST((hDirectDraw=NtGdiDdCreateDirectDrawObject(hdc)) != NULL);

	/* Cleanup ReactX setup */
	DeleteDC(hdc);
	Syscall(L"NtGdiDdDeleteDirectDrawObject", 1, &hDirectDraw);

	return APISTATUS_NORMAL;
}
