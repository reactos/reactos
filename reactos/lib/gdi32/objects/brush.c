#ifdef UNICODE
#undef UNICODE
#endif

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/kapi.h>


/*
 * @implemented
 */
HBRUSH
STDCALL
CreateSolidBrush(
	COLORREF	a0
	)
{
	return W32kCreateSolidBrush(a0);
}
