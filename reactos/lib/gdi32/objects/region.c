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
        return NtGdiOffsetClipRgn(a0, a1, a2);
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
	HRGN rgn = NtGdiGetClipRgn(a0);
	if(rgn)
	{
		if(NtGdiCombineRgn(a1, rgn, 0, RGN_COPY) != ERROR) return 1;
		else
			return -1;
	}
	else	return 0;
}


/*
 * @unimplemented
 */
HRGN
STDCALL
CreateEllipticRgn(
	int			a0,
	int			a1,
	int			a2,
	int			a3
	)
{
	return NtGdiCreateEllipticRgn(a0,a1,a2,a3);
}


/*
 * @unimplemented
 */
HRGN
STDCALL
CreateEllipticRgnIndirect(
	CONST RECT		*a0
	)
{
	return NtGdiCreateEllipticRgnIndirect((RECT *)a0);
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
  return NtGdiCreatePolyPolygonRgn ( (CONST PPOINT)lppt,
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
  return NtGdiCreatePatternBrush ( hbmp );
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
	return NtGdiCreateRectRgn(a0,a1,a2,a3);
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
	return NtGdiCreateRectRgnIndirect((RECT *)a0);
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
  return NtGdiCreateRoundRectRgn (
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
	return NtGdiEqualRgn(a0,a1);
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
	return NtGdiOffsetRgn(a0,a1,a2);
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
	return NtGdiGetRgnBox(a0,a1);
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
	return NtGdiSetRectRgn(a0,a1,a2,a3,a4);
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
	return NtGdiCombineRgn(a0,a1,a2,a3);
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
	return NtGdiGetRegionData(a0,a1,a2);
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
	return NtGdiPaintRgn( a0, a1 );
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
	return NtGdiFillRgn(a0, a1, a2);
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
	return NtGdiPtInRegion(a0,a1,a2);
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
	return NtGdiRectInRegion(a0,(CONST PRECT)a1);
}

/*
 * @implemented
 */
BOOL
STDCALL
InvertRgn(
	HDC	hDc,
	HRGN	hRgn
	)
{
	return NtGdiInvertRgn(hDc, hRgn);
}

/*
 * @implemented
 */
BOOL
STDCALL
FrameRgn(
	HDC	hdc,
	HRGN	hrgn,
	HBRUSH	hbr,
	int	nWidth,
	int	nHeight
	)
{
	return NtGdiFrameRgn(hdc, hrgn, hbr, nWidth, nHeight);
}
