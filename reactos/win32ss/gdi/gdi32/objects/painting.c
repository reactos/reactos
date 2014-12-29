#include <precomp.h>


/*
 * @implemented
 */
BOOL
WINAPI
LineTo(
    _In_ HDC hdc,
    _In_ INT x,
    _In_ INT y )
{
#if 0
// Handle something other than a normal dc object.
    if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
            return MFDRV_MetaParam2(hdc, META_LINETO, x, y);
        else
        {
            PLDC pLDC = GdiGetLDC(hdc);
            if (!pLDC)
            {
                SetLastError(ERROR_INVALID_HANDLE);
                return FALSE;
            }
            if (pLDC->iType == LDC_EMFLDC)
            {
                return MFDRV_LineTo(hdc, x, y )
                   }
                   return FALSE;
        }
    }
#endif
    return NtGdiLineTo(hdc, x, y);
}


BOOL
WINAPI
MoveToEx(
    _In_ HDC hdc,
    _In_ INT x,
    _In_ INT y,
    _Out_opt_ LPPOINT ppt)
{
    PDC_ATTR pdcattr;
#if 0
    if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
            return MFDRV_MetaParam2(hdc, META_MOVETO, x, y);
        else
        {
            PLDC pLDC = pdcattr->pvLDC;
            if ( !pLDC )
            {
                SetLastError(ERROR_INVALID_HANDLE);
                return FALSE;
            }
            if (pLDC->iType == LDC_EMFLDC)
            {
                if (!EMFDRV_MoveTo(hdc, x, y)) return FALSE;
            }
        }
    }
#endif
    if (!GdiGetHandleUserData((HGDIOBJ) hdc, GDI_OBJECT_TYPE_DC, (PVOID) &pdcattr)) return FALSE;

    if (ppt)
    {
        if ( pdcattr->ulDirty_ & DIRTY_PTLCURRENT ) // Double hit!
        {
            ppt->x = pdcattr->ptfxCurrent.x; // ret prev before change.
            ppt->y = pdcattr->ptfxCurrent.y;
            DPtoLP (hdc, ppt, 1);          // reconvert back.
        }
        else
        {
            ppt->x = pdcattr->ptlCurrent.x;
            ppt->y = pdcattr->ptlCurrent.y;
        }
    }

    pdcattr->ptlCurrent.x = x;
    pdcattr->ptlCurrent.y = y;

    pdcattr->ulDirty_ &= ~DIRTY_PTLCURRENT;
    pdcattr->ulDirty_ |= ( DIRTY_PTFXCURRENT|DIRTY_STYLESTATE); // Set dirty
    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
Ellipse(
    _In_ HDC hdc,
    _In_ INT left,
    _In_ INT top,
    _In_ INT right,
    _In_ INT bottom)
{
#if 0
// Handle something other than a normal dc object.
    if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
            return MFDRV_MetaParam4(hdc, META_ELLIPSE, left, top, right, bottom );
        else
        {
            PLDC pLDC = GdiGetLDC(hdc);
            if ( !pLDC )
            {
                SetLastError(ERROR_INVALID_HANDLE);
                return FALSE;
            }
            if (pLDC->iType == LDC_EMFLDC)
            {
                return EMFDRV_Ellipse(hdc, left, top, right, bottom );
            }
            return FALSE;
        }
    }
#endif
    return NtGdiEllipse(hdc, left, top, right, bottom);
}


/*
 * @implemented
 */
BOOL
WINAPI
Rectangle(
    _In_ HDC hdc,
    _In_ INT left,
    _In_ INT top,
    _In_ INT right,
    _In_ INT bottom)
{
#if 0
// Handle something other than a normal dc object.
    if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
            return MFDRV_MetaParam4(hdc, META_RECTANGLE, left, top, right, bottom);
        else
        {
            PLDC pLDC = GdiGetLDC(hdc);
            if ( !pLDC )
            {
                SetLastError(ERROR_INVALID_HANDLE);
                return FALSE;
            }
            if (pLDC->iType == LDC_EMFLDC)
            {
                return EMFDRV_Rectangle(hdc, left, top, right, bottom);
            }
            return FALSE;
        }
    }
#endif
    return NtGdiRectangle(hdc, left, top, right, bottom);
}


/*
 * @implemented
 */
BOOL
WINAPI
RoundRect(
    _In_ HDC hdc,
    _In_ INT left,
    _In_ INT top,
    _In_ INT right,
    _In_ INT bottom,
    _In_ INT width,
    _In_ INT height)
{
#if 0
// Handle something other than a normal dc object.
    if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
            return MFDRV_MetaParam6(hdc, META_ROUNDRECT, left, top, right, bottom,
                                     width, height  );
        else
        {
            PLDC pLDC = GdiGetLDC(hdc);
            if ( !pLDC )
            {
                SetLastError(ERROR_INVALID_HANDLE);
                return FALSE;
            }
            if (pLDC->iType == LDC_EMFLDC)
            {
                return EMFDRV_RoundRect(hdc, left, top, right, bottom,
                                         width, height );
            }
            return FALSE;
        }
    }
#endif
    return NtGdiRoundRect(hdc, left, top, right, bottom, width, height);
}


/*
 * @implemented
 */
COLORREF
WINAPI
GetPixel(
    _In_ HDC hdc,
    _In_ INT x,
    _In_ INT y)
{
    if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC) return CLR_INVALID;
    if (!GdiIsHandleValid((HGDIOBJ) hdc)) return CLR_INVALID;
    return NtGdiGetPixel(hdc, x, y);
}


/*
 * @implemented
 */
COLORREF
WINAPI
SetPixel(
    _In_ HDC hdc,
    _In_ INT x,
    _In_ INT y,
    _In_ COLORREF crColor)
{
#if 0
// Handle something other than a normal dc object.
    if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
            return MFDRV_MetaParam4(hdc, META_SETPIXEL, x, y, HIWORD(crColor),
                                    LOWORD(crColor));
        else
        {
            PLDC pLDC = GdiGetLDC(hdc);
            if ( !pLDC )
            {
                SetLastError(ERROR_INVALID_HANDLE);
                return 0;
            }
            if (pLDC->iType == LDC_EMFLDC)
            {
                return EMFDRV_SetPixel(hdc, x, y, crColor);
            }
            return 0;
        }
    }
#endif
    return NtGdiSetPixel(hdc, x, y, crColor);
}


/*
 * @implemented
 */
BOOL
WINAPI
SetPixelV(
    _In_ HDC hdc,
    _In_ INT x,
    _In_ INT y,
    _In_ COLORREF crColor)
{
    return SetPixel(hdc, x, y, crColor) != CLR_INVALID;
}


/*
 * @implemented
 */
BOOL
WINAPI
FillRgn(
    _In_ HDC hdc,
    _In_ HRGN hrgn,
    _In_ HBRUSH hbr)
{

    if ((hrgn == NULL) || (hbr == NULL))
        return FALSE;
#if 0
// Handle something other than a normal dc object.
    if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
            return MFDRV_FillRgn(hdc, hrgn, hbr);
        else
        {
            PLDC pLDC = GdiGetLDC(hdc);
            if ( !pLDC )
            {
                SetLastError(ERROR_INVALID_HANDLE);
                return FALSE;
            }
            if (pLDC->iType == LDC_EMFLDC)
            {
                return EMFDRV_FillRgn((hdc, hrgn, hbr);
                                  }
                                  return FALSE;
        }
    }
#endif
    return NtGdiFillRgn(hdc, hrgn, hbr);
}


/*
 * @implemented
 */
BOOL
WINAPI
FrameRgn(
    _In_ HDC hdc,
    _In_ HRGN hrgn,
    _In_ HBRUSH hbr,
    _In_ INT nWidth,
    _In_ INT nHeight)
{

    if ((hrgn == NULL) || (hbr == NULL))
        return FALSE;
#if 0
// Handle something other than a normal dc object.
    if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
            return MFDRV_FrameRgn(hdc, hrgn, hbr, nWidth, nHeight );
        else
        {
            PLDC pLDC = GdiGetLDC(hdc);
            if ( !pLDC )
            {
                SetLastError(ERROR_INVALID_HANDLE);
                return FALSE;
            }
            if (pLDC->iType == LDC_EMFLDC)
            {
                return EMFDRV_FrameRgn(hdc, hrgn, hbr, nWidth, nHeight );
            }
            return FALSE;
        }
    }
#endif
    return NtGdiFrameRgn(hdc, hrgn, hbr, nWidth, nHeight);
}


/*
 * @implemented
 */
BOOL
WINAPI
InvertRgn(
    _In_ HDC hdc,
    _In_ HRGN hrgn)
{

    if (hrgn == NULL)
        return FALSE;
#if 0
// Handle something other than a normal dc object.
    if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
            return MFDRV_InvertRgn(hdc, HRGN hrgn ); // Use this instead of MFDRV_MetaParam.
        else
        {
            PLDC pLDC = GdiGetLDC(hdc);
            if ( !pLDC )
            {
                SetLastError(ERROR_INVALID_HANDLE);
                return FALSE;
            }
            if (pLDC->iType == LDC_EMFLDC)
            {
                return EMFDRV_PaintInvertRgn(hdc, hrgn, EMR_INVERTRGN );
            }
            return FALSE;
        }
    }
#endif
    return NtGdiInvertRgn(hdc, hrgn);
}


/*
 * @implemented
 */
BOOL
WINAPI
PaintRgn(
    _In_ HDC hdc,
    _In_ HRGN hrgn)
{
#if 0
// Handle something other than a normal dc object.
    if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
            return MFDRV_PaintRgn(hdc, HRGN hrgn ); // Use this instead of MFDRV_MetaParam.
        else
        {
            PLDC pLDC = GdiGetLDC(hdc);
            if ( !pLDC )
            {
                SetLastError(ERROR_INVALID_HANDLE);
                return FALSE;
            }
            if (pLDC->iType == LDC_EMFLDC)
            {
                return EMFDRV_PaintInvertRgn(hdc, hrgn, EMR_PAINTRGN );
            }
            return FALSE;
        }
    }
#endif
// Could just use pdcattr->hbrush? No.
    HBRUSH hbr = GetCurrentObject(hdc, OBJ_BRUSH);

    return NtGdiFillRgn(hdc, hrgn, hbr);
}


/*
 * @implemented
 */
BOOL
WINAPI
PolyBezier(
    _In_ HDC hdc,
    _In_reads_(cpt) const POINT *apt,
    _In_ DWORD cpt)
{
#if 0
// Handle something other than a normal dc object.
    if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
            /*
             * Since MetaFiles don't record Beziers and they don't even record
             * approximations to them using lines.
             */
            return FALSE;
        else
        {
            PLDC pLDC = GdiGetLDC(hdc);
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
    return NtGdiPolyPolyDraw(hdc ,(PPOINT)apt, &cpt, 1, GdiPolyBezier);
}


/*
 * @implemented
 */
BOOL
WINAPI
PolyBezierTo(
    _In_ HDC hdc,
    _In_reads_(cpt) const POINT *apt,
    _In_ DWORD cpt)
{
#if 0
// Handle something other than a normal dc object.
    if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
            return FALSE;
        else
        {
            PLDC pLDC = GdiGetLDC(hdc);
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
    return NtGdiPolyPolyDraw(hdc , (PPOINT)apt, &cpt, 1, GdiPolyBezierTo);
}


/*
 * @implemented
 */
BOOL
WINAPI
PolyDraw(
    _In_ HDC hdc,
    _In_reads_(cpt) const POINT *apt,
    _In_reads_(cpt) const BYTE *aj,
    _In_ INT cpt)
{
#if 0
// Handle something other than a normal dc object.
    if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
            return FALSE;
        else
        {
            PLDC pLDC = GdiGetLDC(hdc);
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
    return NtGdiPolyDraw(hdc, (PPOINT)apt, (PBYTE)aj, cpt);
}


/*
 * @implemented
 */
BOOL
WINAPI
Polygon(
    _In_ HDC hdc,
    _In_reads_(cpt) const POINT *apt,
    _In_ INT cpt)
{
#if 0
// Handle something other than a normal dc object.
    if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
            return MFDRV_Polygon(hdc, apt, cpt );
        else
        {
            PLDC pLDC = GdiGetLDC(hdc);
            if ( !pLDC )
            {
                SetLastError(ERROR_INVALID_HANDLE);
                return FALSE;
            }
            if (pLDC->iType == LDC_EMFLDC)
            {
                return EMFDRV_Polygon(hdc, apt, cpt );
            }
            return FALSE;
        }
    }
#endif
    return NtGdiPolyPolyDraw(hdc , (PPOINT)apt, (PULONG)&cpt, 1, GdiPolyPolygon);
}


/*
 * @implemented
 */
BOOL
WINAPI
Polyline(
    _In_ HDC hdc,
    _In_reads_(cpt) const POINT *apt,
    _In_ INT cpt)
{
#if 0
// Handle something other than a normal dc object.
    if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
            return MFDRV_Polyline(hdc, apt, cpt);
        else
        {
            PLDC pLDC = GdiGetLDC(hdc);
            if ( !pLDC )
            {
                SetLastError(ERROR_INVALID_HANDLE);
                return FALSE;
            }
            if (pLDC->iType == LDC_EMFLDC)
            {
                return EMFDRV_Polyline(hdc, apt, cpt);
            }
            return FALSE;
        }
    }
#endif
    return NtGdiPolyPolyDraw(hdc, (PPOINT)apt, (PULONG)&cpt, 1, GdiPolyPolyLine);
}


/*
 * @implemented
 */
BOOL
WINAPI
PolylineTo(
    _In_ HDC hdc,
    _In_reads_(cpt) const POINT *apt,
    _In_ DWORD cpt)
{
#if 0
// Handle something other than a normal dc object.
    if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
            return FALSE;
        else
        {
            PLDC pLDC = GdiGetLDC(hdc);
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
    return NtGdiPolyPolyDraw(hdc , (PPOINT)apt, &cpt, 1, GdiPolyLineTo);
}


/*
 * @implemented
 */
BOOL
WINAPI
PolyPolygon(
    _In_ HDC hdc,
    _In_ const POINT *apt,
    _In_reads_(csz) const INT *asz,
    _In_ INT csz)
{
#if 0
// Handle something other than a normal dc object.
    if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
            return MFDRV_PolyPolygon(hdc, apt, asz, csz);
        else
        {
            PLDC pLDC = GdiGetLDC(hdc);
            if ( !pLDC )
            {
                SetLastError(ERROR_INVALID_HANDLE);
                return FALSE;
            }
            if (pLDC->iType == LDC_EMFLDC)
            {
                return EMFDRV_PolyPolygon(hdc, apt, asz, csz );
            }
            return FALSE;
        }
    }
#endif
    return NtGdiPolyPolyDraw(hdc, (PPOINT)apt, (PULONG)asz, csz, GdiPolyPolygon);
}


/*
 * @implemented
 */
BOOL
WINAPI
PolyPolyline(
    _In_ HDC hdc,
    _In_ CONST POINT *apt,
    _In_reads_(csz) CONST DWORD *asz,
    _In_ DWORD csz)
{
#if 0
// Handle something other than a normal dc object.
    if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
            return FALSE;
        else
        {
            PLDC pLDC = GdiGetLDC(hdc);
            if ( !pLDC )
            {
                SetLastError(ERROR_INVALID_HANDLE);
                return FALSE;
            }
            if (pLDC->iType == LDC_EMFLDC)
            {
                return EMFDRV_PolyPolyline(hdc, apt, asz, csz);
            }
            return FALSE;
        }
    }
#endif
    return NtGdiPolyPolyDraw(hdc , (PPOINT)apt, (PULONG)asz, csz, GdiPolyPolyLine);
}


/*
 * @implemented
 */
BOOL
WINAPI
ExtFloodFill(
    _In_ HDC hdc,
    _In_ INT xStart,
    _In_ INT yStart,
    _In_ COLORREF crFill,
    _In_ UINT fuFillType)
{
#if 0
// Handle something other than a normal dc object.
    if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
            return MFDRV_ExtFloodFill(hdc, xStart, yStart, crFill, fuFillType );
        else
        {
            PLDC pLDC = GdiGetLDC(hdc);
            if ( !pLDC )
            {
                SetLastError(ERROR_INVALID_HANDLE);
                return FALSE;
            }
            if (pLDC->iType == LDC_EMFLDC)
            {
                return EMFDRV_ExtFloodFill(hdc, xStart, yStart, crFill, fuFillType );
            }
            return FALSE;
        }
    }
#endif
    return NtGdiExtFloodFill(hdc, xStart, yStart, crFill, fuFillType);
}


/*
 * @implemented
 */
BOOL
WINAPI
FloodFill(
    _In_ HDC hdc,
    _In_ INT xStart,
    _In_ INT yStart,
    _In_ COLORREF crFill)
{
    return ExtFloodFill(hdc, xStart, yStart, crFill, FLOODFILLBORDER);
}

/*
 * @implemented
 */
BOOL
WINAPI
BitBlt(
    _In_ HDC hdcDest,
    _In_ INT xDest,
    _In_ INT yDest,
    _In_ INT cx,
    _In_ INT cy,
    _In_opt_ HDC hdcSrc,
    _In_ INT xSrc,
    _In_ INT ySrc,
    _In_ DWORD dwRop)
{
    /* Use PatBlt for no source blt, like windows does */
    if (!ROP_USES_SOURCE(dwRop))
    {
        return PatBlt(hdcDest, xDest, yDest, cx, cy, dwRop);
    }

    return NtGdiBitBlt(hdcDest, xDest, yDest, cx, cy, hdcSrc, xSrc, ySrc, dwRop, 0, 0);
}

BOOL
WINAPI
PatBlt(
    _In_ HDC hdc,
    _In_ INT nXLeft,
    _In_ INT nYLeft,
    _In_ INT nWidth,
    _In_ INT nHeight,
    _In_ DWORD dwRop)
{
    /* FIXME some part need be done in user mode */
    return NtGdiPatBlt( hdc,  nXLeft,  nYLeft,  nWidth,  nHeight,  dwRop);
}

BOOL
WINAPI
PolyPatBlt(
    _In_ HDC hdc,
    _In_ DWORD dwRop,
    _In_ PPOLYPATBLT pPoly,
    _In_ DWORD nCount,
    _In_ DWORD dwMode)
{
    /* FIXME some part need be done in user mode */
    return NtGdiPolyPatBlt(hdc, dwRop, pPoly, nCount, dwMode);
}

/*
 * @implemented
 */
BOOL
WINAPI
StretchBlt(
    _In_ HDC hdcDest,
    _In_ INT xDest,
    _In_ INT yDest,
    _In_ INT cxDest,
    _In_ INT cyDest,
    _In_opt_ HDC hdcSrc,
    _In_ INT xSrc,
    _In_ INT ySrc,
    _In_ INT cxSrc,
    _In_ INT cySrc,
    _In_ DWORD dwRop)
{
    if ((cxDest != cxSrc) || (cyDest != cySrc))
    {
        return NtGdiStretchBlt(hdcDest,
                               xDest,
                               yDest,
                               cxDest,
                               cyDest,
                               hdcSrc,
                               xSrc,
                               ySrc,
                               cxSrc,
                               cySrc,
                               dwRop,
                               0);
    }

    return NtGdiBitBlt(hdcDest,
                       xDest,
                       yDest,
                       cxDest,
                       cyDest,
                       hdcSrc,
                       xSrc,
                       ySrc,
                       dwRop,
                       0,
                       0);
}


/*
 * @implemented
 */
BOOL
WINAPI
MaskBlt(
    _In_ HDC hdcDest,
    _In_ INT xDest,
    _In_ INT yDest,
    _In_ INT cx,
    _In_ INT cy,
    _In_ HDC hdcSrc,
    _In_ INT xSrc,
    _In_ INT ySrc,
    _In_ HBITMAP hbmMask,
    _In_ INT xMask,
    _In_ INT yMask,
    _In_ DWORD dwRop)
{
    return NtGdiMaskBlt(hdcDest,
                        xDest,
                        yDest,
                        cx,
                        cy,
                        hdcSrc,
                        xSrc,
                        ySrc,
                        hbmMask,
                        xMask,
                        yMask,
                        dwRop,
                        GetBkColor(hdcSrc));
}


/*
 * @implemented
 */
BOOL
WINAPI
PlgBlt(
    _In_ HDC hdcDest,
    _In_reads_(3) const POINT * ppt,
    _In_ HDC hdcSrc,
    _In_ INT xSrc,
    _In_ INT ySrc,
    _In_ INT cx,
    _In_ INT cy,
    _In_opt_ HBITMAP hbmMask,
    _In_ INT xMask,
    _In_ INT yMask)
{
    return NtGdiPlgBlt(hdcDest,
                       (LPPOINT)ppt,
                       hdcSrc,
                       xSrc,
                       ySrc,
                       cx,
                       cy,
                       hbmMask,
                       xMask,
                       yMask,
                       GetBkColor(hdcSrc));
}

BOOL
WINAPI
GdiAlphaBlend(
    _In_ HDC hdcDst,
    _In_ INT xDst,
    _In_ INT yDst,
    _In_ INT cxDst,
    _In_ INT cyDst,
    _In_ HDC hdcSrc,
    _In_ INT xSrc,
    _In_ INT ySrc,
    _In_ INT cxSrc,
    _In_ INT cySrc,
    _In_ BLENDFUNCTION blendfn)
{
    if (hdcSrc == NULL ) return FALSE;

    if (GDI_HANDLE_GET_TYPE(hdcSrc) == GDI_OBJECT_TYPE_METADC) return FALSE;

    return NtGdiAlphaBlend(hdcDst,
                           xDst,
                           yDst,
                           cxDst,
                           cyDst,
                           hdcSrc,
                           xSrc,
                           ySrc,
                           cxSrc,
                           cySrc,
                           blendfn,
                           0);
}


/*
 * @implemented
 */
BOOL
WINAPI
GdiTransparentBlt(
    _In_ HDC hdcDst,
    _In_ INT xDst,
    _In_ INT yDst,
    _In_ INT cxDst,
    _In_ INT cyDst,
    _In_ HDC hdcSrc,
    _In_ INT xSrc,
    _In_ INT ySrc,
    _In_ INT cxSrc,
    _In_ INT cySrc,
    _In_ UINT crTransparent)
{
    /* FIXME some part need be done in user mode */
    return NtGdiTransparentBlt(hdcDst, xDst, yDst, cxDst, cyDst, hdcSrc, xSrc, ySrc, cxSrc, cySrc, crTransparent);
}

/*
 * @implemented
 */
BOOL
WINAPI
GdiGradientFill(
    _In_ HDC hdc,
    _In_reads_(nVertex) PTRIVERTEX pVertex,
    _In_ ULONG nVertex,
    _In_ PVOID pMesh,
    _In_ ULONG nCount,
    _In_ ULONG ulMode)
{
    /* FIXME some part need be done in user mode */
    return NtGdiGradientFill(hdc, pVertex, nVertex, pMesh, nCount, ulMode);
}
