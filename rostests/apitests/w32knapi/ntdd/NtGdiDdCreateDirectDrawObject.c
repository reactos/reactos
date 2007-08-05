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
	HDC hdc = CreateDCW(L"Display",NULL,NULL,NULL);
	ASSERT1(hdc != NULL);

	RTEST(NtGdiDdCreateDirectDrawObject(NULL) == NULL);

	TEST(NtGdiDdCreateDirectDrawObject(hdc) != NULL);
    
	DeleteDC(hdc);

	return APISTATUS_NORMAL;
}
