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
int
STDCALL
OffsetClipRgn(
        HDC     a0,
        int     a1,
        int     a2
        )
{
        return W32kOffsetClipRgn(a0, a1, a2);
}


/*
 * @implemented
 */
int
STDCALL
GetClipRgn(
        HDC     a0,
        HRGN    a1
        )
{
	HRGN rgn = W32kGetClipRgn(a0);
	if(rgn)
	{
		if(W32kCombineRgn(a1, rgn, 0, RGN_COPY) != ERROR) return 1;
		else
			return -1;
	}
	else	return 0;
}


/*
 * @implemented
 */
HRGN
STDCALL
CreatePolyPolygonRgn(
	CONST POINT	*lppt,
	CONST INT	*lpPolyCounts,
	int		nCount,
	int		fnPolyFillMode
	)
{
  return W32kCreatePolyPolygonRgn ( (CONST PPOINT)lppt,
    (CONST PINT)lpPolyCounts, nCount, fnPolyFillMode );
}


/*
 * @implemented
 */
HBRUSH
STDCALL
CreatePatternBrush(
	HBITMAP		hbmp
	)
{
  return W32kCreatePatternBrush ( hbmp );
}


/*
 * @implemented
 */
HRGN
STDCALL
CreateRectRgn(
	int		a0,
	int		a1,
	int		a2,
	int		a3
	)
{
	return W32kCreateRectRgn(a0,a1,a2,a3);
}


/*
 * @implemented
 */
HRGN
STDCALL
CreateRectRgnIndirect(
	CONST RECT	*a0
	)
{
	return W32kCreateRectRgnIndirect((RECT *)a0);
}


/*
 * @implemented
 */
HRGN
STDCALL
CreateRoundRectRgn(
	int	nLeftRect,
	int	nTopRect,
	int	nRightRect,
	int	nBottomRect,
	int	nWidthEllipse,
	int	nHeightEllipse
	)
{
  return W32kCreateRoundRectRgn (
    nLeftRect, nTopRect, nRightRect, nBottomRect, nWidthEllipse, nHeightEllipse );
}


/*
 * @implemented
 */
BOOL
STDCALL
EqualRgn(
	HRGN		a0,
	HRGN		a1
	)
{
	return W32kEqualRgn(a0,a1);
}


/*
 * @implemented
 */
int
STDCALL
OffsetRgn(
	HRGN	a0,
	int	a1,
	int	a2
	)
{
	return W32kOffsetRgn(a0,a1,a2);
}


/*
 * @implemented
 */
int
STDCALL
GetRgnBox(
	HRGN	a0,
	LPRECT	a1
	)
{
	return W32kGetRgnBox(a0,a1);
}


/*
 * @implemented
 */
BOOL
STDCALL
SetRectRgn(
	HRGN	a0,
	int	a1,
	int	a2,
	int	a3,
	int	a4
	)
{
	return W32kSetRectRgn(a0,a1,a2,a3,a4);
}


/*
 * @implemented
 */
int
STDCALL
CombineRgn(
	HRGN	a0,
	HRGN	a1,
	HRGN	a2,
	int	a3
	)
{
	return W32kCombineRgn(a0,a1,a2,a3);
}


/*
 * @implemented
 */
DWORD
STDCALL
GetRegionData(
	HRGN		a0,
	DWORD		a1,
	LPRGNDATA	a2
	)
{
	return W32kGetRegionData(a0,a1,a2);
}


/*
 * @implemented
 */
BOOL
STDCALL
PaintRgn(
	HDC	a0,
	HRGN	a1
	)
{
	return W32kPaintRgn( a0, a1 );
}


/*
 * @implemented
 */
BOOL
STDCALL
FillRgn(
	HDC	a0,
	HRGN	a1,
	HBRUSH	a2
	)
{
	return W32kFillRgn(a0, a1, a2);
}

/*
 * @implemented
 */
BOOL
STDCALL
PtInRegion(
	HRGN	a0,
	int	a1,
	int	a2
	)
{
	return W32kPtInRegion(a0,a1,a2);
}

/*
 * @implemented
 */
BOOL
STDCALL
RectInRegion(
	HRGN		a0,
	CONST RECT	*a1
	)
{
	return W32kRectInRegion(a0,(CONST PRECT)a1);
}
