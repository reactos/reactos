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

/*
 * @implemented
 */
HBRUSH
STDCALL
CreateBrushIndirect(
	CONST LOGBRUSH	*a0
	)
{
	return W32kCreateBrushIndirect(a0);
}

/*
 * @implemented
 */
HBRUSH
STDCALL
CreateDIBPatternBrushPt(
	CONST VOID		*a0,
	UINT			a1
	)
{
	return W32kCreateDIBPatternBrushPt(a0,a1);
}

/*
 * @implemented
 */
HBRUSH
STDCALL
CreateHatchBrush(
	int		a0,
	COLORREF	a1
	)
{
	return W32kCreateHatchBrush(a0,a1);
}
