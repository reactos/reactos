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
        HDC     hdc,
        int     nXOffset,
        int     nYOffset
        )
{
  return NtGdiOffsetClipRgn(hdc, nXOffset, nYOffset);
}


/*
 * @implemented
 */
int
STDCALL
GetClipRgn(
        HDC     hdc,
        HRGN    hrgn
        )
{
	HRGN rgn = NtGdiGetClipRgn(hdc);
	if(rgn)
	{
		if(NtGdiCombineRgn(hrgn, rgn, 0, RGN_COPY) != ERROR) return 1;
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
CreateEllipticRgn(
	int			nLeftRect,
	int			nTopRect,
	int			nRightRect,
	int			nBottomRect
	)
{
  return NtGdiCreateEllipticRgn(nLeftRect, nTopRect, nRightRect, nBottomRect);
}


/*
 * @implemented
 */
HRGN
STDCALL
CreateEllipticRgnIndirect(
	CONST RECT		*lprc
	)
{
  return NtGdiCreateEllipticRgnIndirect((RECT *)lprc);
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
  return NtGdiCreatePolyPolygonRgn((CONST PPOINT)lppt,
                                   (CONST PINT)lpPolyCounts,
				   nCount,
				   fnPolyFillMode);
}


/*
 * @implemented
 */
HRGN
STDCALL
CreatePolygonRgn(
	CONST POINT	*lppt,
	int		cPoints,
	int		fnPolyFillMode
	)
{
  return NtGdiCreatePolygonRgn((CONST PPOINT)lppt,
                               cPoints,
			       fnPolyFillMode);
}


/*
 * @implemented
 */
HRGN
STDCALL
CreateRectRgn(
	int		nLeftRect,
	int		nTopRect,
	int		nRightRect,
	int		nBottomRect
	)
{
  return NtGdiCreateRectRgn(nLeftRect, nTopRect, nRightRect, nBottomRect);
}


/*
 * @implemented
 */
HRGN
STDCALL
CreateRectRgnIndirect(
	CONST RECT	*lprc
	)
{
  return NtGdiCreateRectRgnIndirect((RECT *)lprc);
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
  return NtGdiCreateRoundRectRgn(nLeftRect, nTopRect, nRightRect, nBottomRect, 
                                 nWidthEllipse, nHeightEllipse);
}


/*
 * @implemented
 */
BOOL
STDCALL
EqualRgn(
	HRGN		hSrcRgn1,
	HRGN		hSrcRgn2
	)
{
  return NtGdiEqualRgn(hSrcRgn1, hSrcRgn2);
}


/*
 * @implemented
 */
int
STDCALL
OffsetRgn(
	HRGN	hrgn,
	int	nXOffset,
	int	nYOffset
	)
{
  return NtGdiOffsetRgn(hrgn, nXOffset, nYOffset);
}


/*
 * @implemented
 */
int
STDCALL
GetRgnBox(
	HRGN	hrgn,
	LPRECT	lprc
	)
{
  return NtGdiGetRgnBox(hrgn, lprc);
}


/*
 * @implemented
 */
BOOL
STDCALL
SetRectRgn(
	HRGN	hrgn,
	int	nLeftRect,
	int	nTopRect,
	int	nRightRect,
	int	nBottomRect
	)
{
  return NtGdiSetRectRgn(hrgn, nLeftRect, nTopRect, nRightRect, nBottomRect);
}


/*
 * @implemented
 */
int
STDCALL
CombineRgn(
	HRGN	hrgnDest,
	HRGN	hrgnSrc1,
	HRGN	hrgnSrc2,
	int	fnCombineMode
	)
{
  return NtGdiCombineRgn(hrgnDest, hrgnSrc1, hrgnSrc2, fnCombineMode);
}


/*
 * @implemented
 */
DWORD
STDCALL
GetRegionData(
	HRGN		hRgn,
	DWORD		dwCount,
	LPRGNDATA	lpRgnData
	)
{
  return NtGdiGetRegionData(hRgn, dwCount, lpRgnData);
}


/*
 * @implemented
 */
BOOL
STDCALL
PaintRgn(
	HDC	hdc,
	HRGN	hrgn
	)
{
  return NtGdiPaintRgn(hdc, hrgn);
}


/*
 * @implemented
 */
BOOL
STDCALL
FillRgn(
	HDC	hdc,
	HRGN	hrgn,
	HBRUSH	hbr
	)
{
  return NtGdiFillRgn(hdc, hrgn, hbr);
}

/*
 * @implemented
 */
BOOL
STDCALL
PtInRegion(
	HRGN	hrgn,
	int	X,
	int	Y
	)
{
  return NtGdiPtInRegion(hrgn, X, Y);
}

/*
 * @implemented
 */
BOOL
STDCALL
RectInRegion(
	HRGN		hrgn,
	CONST RECT	*lprc
	)
{
  return NtGdiRectInRegion(hrgn, (CONST PRECT)lprc);
}

/*
 * @implemented
 */
BOOL
STDCALL
InvertRgn(
	HDC	hdc,
	HRGN	hrgn
	)
{
  return NtGdiInvertRgn(hdc, hrgn);
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

