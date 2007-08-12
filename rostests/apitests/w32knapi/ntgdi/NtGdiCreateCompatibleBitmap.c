#include "../w32knapi.h"

W32KAPI
HBITMAP
APIENTRY
NtGdiCreateCompatibleBitmap(
    IN HDC hdc,
    IN INT cx,
    IN INT cy
)
{
	return (HBITMAP)Syscall(L"NtGdiCreateCompatibleBitmap", 3, &hdc);
}

INT
Test_NtGdiCreateCompatibleBitmap(PTESTINFO pti)
{
	return APISTATUS_NORMAL;
}
