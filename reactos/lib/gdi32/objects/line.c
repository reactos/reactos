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
BOOL
STDCALL
Arc(
	HDC	a0,
	int	a1,
	int	a2,
	int	a3,
	int	a4,
	int	a5,
	int	a6,
	int	a7,
	int	a8
	)
{
	return NtGdiArc(a0,a1,a2,a3,a4,a5,a6,a7,a8);
}

/*
 * @implemented
 */
int
STDCALL
SetArcDirection(
        HDC     hdc,
        int     a1
        )
{
        return NtGdiSetArcDirection(hdc, a1);
}


/*
 * @implemented
 */
int
STDCALL
GetArcDirection(
        HDC     hdc
        )
{
        return NtGdiGetArcDirection(hdc);
}


/*
 * @implemented
 */
BOOL
STDCALL
LineTo(HDC hDC, int XEnd, int YEnd)
{
   return NtGdiLineTo(hDC, XEnd, YEnd);
}


/*
 * @implemented
 */
BOOL  
STDCALL 
MoveToEx(HDC hDC, int X, int Y, LPPOINT Point)
{
   return NtGdiMoveToEx(hDC, X, Y, Point);
}


/*
 * @implemented
 */
BOOL
STDCALL
Polyline( HDC hdc, CONST POINT *lppt, int cPoints )
{
   return NtGdiPolyline(hdc, (CONST LPPOINT) lppt, cPoints);
}

/*
 * @implemented
 */
BOOL
STDCALL
ArcTo(
	HDC	hdc,
	int	a1,
	int	a2,
	int	a3,
	int	a4,
	int	a5,
	int	a6,
	int	a7,
	int	a8
	)
{
	return NtGdiArcTo(hdc,a1,a2,a3,a4,a5,a6,a7,a8);
}


/*
 * @implemented
 */
BOOL
STDCALL
PolyBezier(
	HDC		a0,
	CONST POINT	*a1,
	DWORD		a2
	)
{
	return PolyBezier(a0,(CONST PPOINT)a1,a2);
}

/*
 * @implemented
 */
BOOL
STDCALL
PolyBezierTo(
	HDC		a0,
	CONST POINT	*a1,
	DWORD		a2
	)
{
	return PolyBezierTo(a0,(CONST PPOINT)a1,a2);
}


/*
 * @implemented
 */
BOOL
STDCALL
PolylineTo(
	HDC		a0,
	CONST POINT	*a1,
	DWORD		a2
	)
{
	return PolyBezierTo(a0,(CONST PPOINT)a1,a2);
}
