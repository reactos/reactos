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
Polygon(HDC	hDC,
	CONST POINT	*lpPoints,
	int	nCount)
{
	return NtGdiPolygon(hDC, (CONST PPOINT)lpPoints, nCount);
}


/*
 * @implemented
 */
BOOL
STDCALL
Rectangle(HDC  hDC,
	int  LeftRect,
	int  TopRect,
	int  RightRect,
	int  BottomRect)
{
   return NtGdiRectangle(hDC, LeftRect, TopRect, RightRect, BottomRect);
}

/*
 * @implemented
 */
BOOL
STDCALL
RoundRect(
	HDC hdc,
	int left,
	int top,
	int right,
	int bottom,
	int width,
	int height
	)
{
  return NtGdiRoundRect ( hdc, left, top, right, bottom, width, height );
}

/*
 * @implemented
 */
BOOL
STDCALL
PolyPolygon(
	HDC		a0,
	CONST POINT	*a1,
	CONST INT	*a2,
	int		a3
	)
{
	return PolyPolygon(a0,(LPPOINT)a1,(LPINT)a2,a3);
}


/*
 * @implemented
 */
BOOL
STDCALL
Ellipse(HDC hDc,
        int Left,
        int Top,
        int Right,
        int Bottom)
{
  return NtGdiEllipse(hDc, Left, Top, Right, Bottom);
}
