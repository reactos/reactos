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
	HDC	hdc,
	int	nLeftRect,
	int	nTopRect,
	int	nRightRect,
	int	nBottomRect,
	int	nXStartArc,
	int	nYStartArc,
	int	nXEndArc,
	int	nYEndArc
	)
{
  return NtGdiArc(hdc, nLeftRect, nTopRect, nRightRect, nBottomRect,
                  nXStartArc, nYStartArc, nXEndArc, nYEndArc);
}

/*
 * @implemented
 */
int
STDCALL
SetArcDirection(
        HDC     hdc,
        int     ArcDirection
        )
{
  return NtGdiSetArcDirection(hdc, ArcDirection);
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
LineTo(HDC hdc, int nXEnd, int nYEnd)
{
  return NtGdiLineTo(hdc, nXEnd, nYEnd);
}


/*
 * @implemented
 */
BOOL  
STDCALL 
MoveToEx(HDC hdc, int X, int Y, LPPOINT lpPoint)
{
  return NtGdiMoveToEx(hdc, X, Y, lpPoint);
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
	int	nLeftRect,
	int	nTopRect,
	int	nRightRect,
	int	nBottomRect,
	int	nXRadial1,
	int	nYRadial1,
	int	nXRadial2,
	int	nYRadial2
	)
{
  return NtGdiArcTo(hdc, nLeftRect, nTopRect, nRightRect, nBottomRect,
                    nXRadial1, nYRadial1, nXRadial2, nYRadial2);
}


/*
 * @implemented
 */
BOOL
STDCALL
PolyBezier(
	HDC		hdc,
	CONST POINT	*lppt,
	DWORD		cPoints
	)
{
  return NtGdiPolyBezier(hdc, (CONST PPOINT)lppt, cPoints);
}

/*
 * @implemented
 */
BOOL
STDCALL
PolyBezierTo(
	HDC		hdc,
	CONST POINT	*lppt,
	DWORD		cCount
	)
{
  return NtGdiPolyBezierTo(hdc, (CONST PPOINT)lppt, cCount);
}


/*
 * @implemented
 */
BOOL
STDCALL
PolylineTo(
	HDC		hdc,
	CONST POINT	*lppt,
	DWORD		cCount
	)
{
  return NtGdiPolylineTo(hdc, (CONST PPOINT)lppt, cCount);
}

/*
 * @implemented
 */
BOOL
STDCALL
PolyPolyline(
	HDC		hdc,
	CONST POINT	*lppt,
	CONST DWORD	*lpdwPolyPoints,
	DWORD		cCount
	)
{
  return NtGdiPolyPolyline(hdc, (LPPOINT)lppt, (LPDWORD)lpdwPolyPoints, cCount);
}
