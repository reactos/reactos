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
	return NtGdiCreateSolidBrush(a0);
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
	return NtGdiCreateBrushIndirect(a0);
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
	return NtGdiCreateDIBPatternBrushPt(a0,a1);
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
	return NtGdiCreateHatchBrush(a0,a1);
}
