#include "precomp.h"


/*
 * @implemented
 */
BOOL
STDCALL
LineTo( HDC hDC, INT x, INT y )
{
#if 0
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_MetaParam2( hDC, META_LINETO, x, y);
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return MFDRV_LineTo( hDC, x, y )
      }
      return FALSE;
    }
 }
#endif
 return NtGdiLineTo( hDC, x, y);
}


BOOL
STDCALL
MoveToEx( HDC hDC, INT x, INT y, LPPOINT Point )
{
 PDC_ATTR Dc_Attr;
#if 0
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_MetaParam2( hDC, META_MOVETO, x, y);
    else
    {
      PLDC pLDC = Dc_Attr->pvLDC;
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        if (!EMFDRV_MoveTo( hDC, x, y)) return FALSE;
      }
    }
 }
#endif
 if (!GdiGetHandleUserData((HGDIOBJ) hDC, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr)) return FALSE;

 if ( Point )
 {
    if ( Dc_Attr->ulDirty_ & DIRTY_PTLCURRENT ) // Double hit!
    {
       Point->x = Dc_Attr->ptfxCurrent.x; // ret prev before change.
       Point->y = Dc_Attr->ptfxCurrent.y;
       DPtoLP ( hDC, Point, 1);          // reconvert back.
    }
    else
    {
       Point->x = Dc_Attr->ptlCurrent.x;
       Point->y = Dc_Attr->ptlCurrent.y;
    }
 }

 Dc_Attr->ptlCurrent.x = x;
 Dc_Attr->ptlCurrent.y = y;

 Dc_Attr->ulDirty_ &= ~DIRTY_PTLCURRENT;
 Dc_Attr->ulDirty_ |= ( DIRTY_PTFXCURRENT|DIRTY_STYLESTATE); // Set dirty
 return TRUE;
}


/*
 * @implemented
 */
BOOL
STDCALL
Ellipse(HDC hDC, INT Left, INT Top, INT Right, INT Bottom)
{
#if 0
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_MetaParam4(hDC, META_ELLIPSE, Left, Top, Right, Bottom );
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return EMFDRV_Ellipse( hDC, Left, Top, Right, Bottom );
      }
      return FALSE;
    }
 }
#endif
 return NtGdiEllipse( hDC, Left, Top, Right, Bottom);
}


/*
 * @implemented
 */
BOOL
STDCALL
Rectangle(HDC hDC, INT Left, INT Top, INT Right, INT Bottom)
{
#if 0
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_MetaParam4(hDC, META_RECTANGLE, Left, Top, Right, Bottom );
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return EMFDRV_Rectangle( hDC, Left, Top, Right, Bottom );
      }
      return FALSE;
    }
 }
#endif
 return NtGdiRectangle( hDC, Left, Top, Right, Bottom);
}


/*
 * @implemented
 */
BOOL
STDCALL
RoundRect(HDC hDC, INT Left, INT Top, INT Right, INT Bottom,
                                                INT ell_Width, INT ell_Height)
{
#if 0
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_MetaParam6( hDC, META_ROUNDRECT, Left, Top, Right, Bottom,
                                                      ell_Width, ell_Height  );
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return EMFDRV_RoundRect( hDC, Left, Top, Right, Bottom,
                                                      ell_Width, ell_Height );
      }
      return FALSE;
    }
 }
#endif
 return NtGdiRoundRect( hDC, Left, Top, Right, Bottom, ell_Width, ell_Height);
}


/*
 * @implemented
 */
COLORREF
STDCALL
GetPixel( HDC hDC, INT x, INT y )
{
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC) return CLR_INVALID;
 if (!GdiIsHandleValid((HGDIOBJ) hDC)) return CLR_INVALID;
 return NtGdiGetPixel( hDC, x, y);
}


/*
 * @implemented
 */
COLORREF
STDCALL
SetPixel( HDC hDC, INT x, INT y, COLORREF Color )
{
#if 0
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_MetaParam4(hDC, META_SETPIXEL, x, y, HIWORD(Color),
                                                              LOWORD(Color));
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return 0;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return EMFDRV_SetPixel( hDC, x, y, Color );
      }
      return 0;
    }
 }
#endif
 return NtGdiSetPixel( hDC, x, y, Color);
}


/*
 * @implemented
 */
BOOL
STDCALL
SetPixelV( HDC hDC, INT x, INT y, COLORREF Color )
{
   COLORREF Cr = SetPixel( hDC, x, y, Color );
   if (Cr != CLR_INVALID) return TRUE;
   return FALSE;
}


/*
 * @implemented
 */
BOOL
STDCALL
FillRgn( HDC hDC, HRGN hRgn, HBRUSH hBrush )
{

 if ( (!hRgn) || (!hBrush) ) return FALSE;
#if 0
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_FillRgn( hDC, hRgn, hBrush);
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return EMFDRV_FillRgn(( hDC, hRgn, hBrush);
      }
      return FALSE;
    }
 }
#endif
 return NtGdiFillRgn( hDC, hRgn, hBrush);
}


/*
 * @implemented
 */
BOOL
STDCALL
FrameRgn( HDC hDC, HRGN hRgn, HBRUSH hBrush, INT nWidth, INT nHeight )
{

 if ( (!hRgn) || (!hBrush) ) return FALSE;
#if 0
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_FrameRgn( hDC, hRgn, hBrush, nWidth, nHeight );
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return EMFDRV_FrameRgn( hDC, hRgn, hBrush, nWidth, nHeight );
      }
      return FALSE;
    }
 }
#endif
 return NtGdiFrameRgn( hDC, hRgn, hBrush, nWidth, nHeight);
}


/*
 * @implemented
 */
BOOL
STDCALL
InvertRgn( HDC hDC, HRGN hRgn )
{

 if ( !hRgn ) return FALSE;
#if 0
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_InvertRgn( hDC, HRGN hRgn ); // Use this instead of MFDRV_MetaParam.
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return EMFDRV_PaintInvertRgn( hDC, hRgn, EMR_INVERTRGN );
      }
      return FALSE;
    }
 }
#endif
 return NtGdiInvertRgn( hDC, hRgn);
}


/*
 * @implemented
 */
BOOL
STDCALL
PaintRgn( HDC hDC, HRGN hRgn )
{
#if 0
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_PaintRgn( hDC, HRGN hRgn ); // Use this instead of MFDRV_MetaParam.
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return EMFDRV_PaintInvertRgn( hDC, hRgn, EMR_PAINTRGN );
      }
      return FALSE;
    }
 }
#endif
 // Could just use Dc_Attr->hbrush? No.
 HBRUSH hBrush = (HBRUSH) GetDCObject( hDC, GDI_OBJECT_TYPE_BRUSH);

 return NtGdiFillRgn( hDC, hRgn, hBrush);
}


/*
 * @implemented
 */
BOOL
STDCALL
PolyBezier(HDC hDC ,const POINT* Point, DWORD cPoints)
{
#if 0
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
 /*
  * Since MetaFiles don't record Beziers and they don't even record
  * approximations to them using lines.
  */
      return FALSE;
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return FALSE; // Not supported yet.
      }
      return FALSE;
    }
 }
#endif
 return NtGdiPolyPolyDraw( hDC ,(PPOINT) Point, &cPoints, 1, GdiPolyBezier );
}


/*
 * @implemented
 */
BOOL
STDCALL
PolyBezierTo(HDC hDC, const POINT* Point ,DWORD cPoints)
{
#if 0
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return FALSE;
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return FALSE; // Not supported yet.
      }
      return FALSE;
    }
 }
#endif
 return NtGdiPolyPolyDraw( hDC , (PPOINT) Point, &cPoints, 1, GdiPolyBezierTo );
}


/*
 * @implemented
 */
BOOL
STDCALL
PolyDraw(HDC hDC, const POINT* Point, const BYTE *lpbTypes, int cCount )
{
#if 0
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return FALSE;
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return FALSE; // Not supported yet.
      }
      return FALSE;
    }
 }
#endif
 return NtGdiPolyDraw( hDC , (PPOINT) Point, (PBYTE)lpbTypes, cCount );
}


/*
 * @implemented
 */
BOOL
STDCALL
Polygon(HDC hDC, const POINT *Point, int Count)
{
#if 0
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_Polygon( hDC, Point, Count );
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return EMFDRV_Polygon( hDC, Point, Count );
      }
      return FALSE;
    }
 }
#endif
 return NtGdiPolyPolyDraw( hDC , (PPOINT) Point, (PULONG)&Count, 1, GdiPolyPolygon );
}


/*
 * @implemented
 */
BOOL
STDCALL
Polyline(HDC hDC, const POINT *Point, int Count)
{
#if 0
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_Polyline( hDC, Point, Count );
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return EMFDRV_Polyline( hDC, Point, Count );
      }
      return FALSE;
    }
 }
#endif
 return NtGdiPolyPolyDraw( hDC , (PPOINT) Point, (PULONG)&Count, 1, GdiPolyPolyLine );
}


/*
 * @implemented
 */
BOOL
STDCALL
PolylineTo(HDC hDC, const POINT* Point, DWORD Count)
{
#if 0
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return FALSE;
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return FALSE; // Not supported yet.
      }
      return FALSE;
    }
 }
#endif
 return NtGdiPolyPolyDraw( hDC , (PPOINT) Point, &Count, 1, GdiPolyLineTo );
}


/*
 * @implemented
 */
BOOL
STDCALL
PolyPolygon(HDC hDC, const POINT* Point, const INT* Count, int Polys)
{
#if 0
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_PolyPolygon( hDC, Point, Count, Polys);
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return EMFDRV_PolyPolygon( hDC, Point, Count, Polys );
      }
      return FALSE;
    }
 }
#endif
 return NtGdiPolyPolyDraw( hDC , (PPOINT)Point, (PULONG)Count, Polys, GdiPolyPolygon );
}


/*
 * @implemented
 */
BOOL
STDCALL
PolyPolyline(HDC hDC, const POINT* Point, const DWORD* Counts, DWORD Polys)
{
#if 0
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return FALSE;
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return EMFDRV_PolyPolyline(hDC, Point, Counts, Polys);
      }
      return FALSE;
    }
 }
#endif
 return NtGdiPolyPolyDraw( hDC , (PPOINT)Point, (PULONG)Counts, Polys, GdiPolyPolyLine );
}


/*
 * @implemented
 */
BOOL
STDCALL
ExtFloodFill(
       HDC hDC,
       int nXStart,
       int nYStart,
       COLORREF crFill,
       UINT fuFillType
             )
{
#if 0
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_ExtFloodFill( hDC, nXStart, nYStart, crFill, fuFillType );
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return EMFDRV_ExtFloodFill( hDC, nXStart, nYStart, crFill, fuFillType );
      }
      return FALSE;
    }
 }
#endif
    return NtGdiExtFloodFill(hDC, nXStart, nYStart, crFill, fuFillType);
}


/*
 * @implemented
 */
BOOL
WINAPI
FloodFill(
    HDC hDC,
    int nXStart,
    int nYStart,
    COLORREF crFill)
{
    return ExtFloodFill(hDC, nXStart, nYStart, crFill, FLOODFILLBORDER);
}


/*
 * @implemented
 */
BOOL WINAPI
MaskBlt(
	HDC hdcDest,
	INT nXDest,
	INT nYDest,
	INT nWidth,
	INT nHeight,
	HDC hdcSrc,
	INT nXSrc,
	INT nYSrc,
	HBITMAP hbmMask,
	INT xMask,
	INT yMask,
	DWORD dwRop)
{
	return NtGdiMaskBlt(hdcDest,
	                    nXDest,
	                    nYDest,
	                    nWidth,
	                    nHeight,
	                    hdcSrc,
	                    nXSrc,
	                    nYSrc,
	                    hbmMask,
	                    xMask,
	                    yMask,
	                    dwRop,
	                    0);
}


/*
 * @implemented
 */
BOOL
WINAPI
PlgBlt(
	HDC hdcDest,
	const POINT *lpPoint,
	HDC hdcSrc,
	INT nXSrc,
	INT nYSrc,
	INT nWidth,
	INT nHeight,
	HBITMAP hbmMask,
	INT xMask,
	INT yMask)
{
	return NtGdiPlgBlt(hdcDest,
	                   (LPPOINT)lpPoint,
	                   hdcSrc,
	                   nXSrc,
	                   nYSrc,
	                   nWidth,
	                   nHeight,
	                   hbmMask,
	                   xMask,
	                   yMask,
	                   0);
}
