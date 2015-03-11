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
    HANDLE_METADC(BOOL, LineTo, FALSE, hdc, x, y);

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

    HANDLE_METADC(BOOL, MoveTo, FALSE, hdc, x, y, ppt);

    /* Get the DC attribute */
    pdcattr = GdiGetDcAttr(hdc);
    if (pdcattr == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

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
    HANDLE_METADC(BOOL, Ellipse, FALSE, hdc, left, top, right, bottom);

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
    HANDLE_METADC(BOOL, Rectangle, FALSE, hdc, left, top, right, bottom);

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
    HANDLE_METADC(BOOL, RoundRect, FALSE, hdc, left, top, right, bottom, width, height);

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
    HANDLE_METADC(COLORREF, SetPixel, CLR_INVALID, hdc, x, y, crColor);

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

    HANDLE_METADC(BOOL, FillRgn, FALSE, hdc, hrgn, hbr);

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

    HANDLE_METADC(BOOL, FrameRgn, FALSE, hdc, hrgn, hbr, nWidth, nHeight);

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

    HANDLE_METADC(BOOL, InvertRgn, FALSE, hdc, hrgn);

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
    return FillRgn(hdc, hrgn, GetCurrentObject(hdc, OBJ_BRUSH));
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
    HANDLE_METADC(BOOL, PolyBezier, FALSE, hdc, apt, cpt);

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
    HANDLE_METADC(BOOL, PolyBezierTo, FALSE, hdc, apt, cpt);

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
    HANDLE_METADC(BOOL, PolyDraw, FALSE, hdc, apt, aj, cpt);

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
    HANDLE_METADC(BOOL, Polygon, FALSE, hdc, apt, cpt);

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
    HANDLE_METADC(BOOL, Polyline, FALSE, hdc, apt, cpt);

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
    HANDLE_METADC(BOOL, PolylineTo, FALSE, hdc, apt, cpt);

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
    HANDLE_METADC(BOOL, PolyPolygon, FALSE, hdc, apt, asz, csz);

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
    if (GDI_HANDLE_GET_TYPE(hdc) == GDILoObjType_LO_METADC16_TYPE)
        return FALSE;

    HANDLE_METADC(BOOL, PolyPolyline, FALSE, hdc, apt, asz, csz);

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
    HANDLE_METADC(BOOL, ExtFloodFill, FALSE, hdc, xStart, yStart, crFill, fuFillType);

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

    /* For meta DCs we use StretchBlt */
    HANDLE_METADC(BOOL,
                  StretchBlt,
                  FALSE,
                  hdcDest,
                  xDest,
                  yDest,
                  cx,
                  cx,
                  hdcSrc,
                  xSrc,
                  ySrc,
                  cx,
                  cx,
                  dwRop);

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
    HANDLE_METADC(BOOL, PatBlt, FALSE, hdc, nXLeft, nYLeft, nWidth, nHeight, dwRop);

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
    UINT i;
    BOOL bResult;
    HBRUSH hbrOld;

    /* Handle meta DCs */
    if ((GDI_HANDLE_GET_TYPE(hdc) == GDILoObjType_LO_METADC16_TYPE) ||
        (GDI_HANDLE_GET_TYPE(hdc) == GDILoObjType_LO_ALTDC_TYPE))
    {
        if (!GdiIsHandleValid(hdc))
        {
            return FALSE;
        }

        /* Save the current DC brush */
        hbrOld = SelectObject(hdc, GetStockObject(DC_BRUSH));

        /* Assume success */
        bResult = TRUE;

        /* Loop all rect */
        for (i = 0; i < nCount; i++)
        {
            /* Select the brush for this rect */
            SelectObject(hdc, pPoly[i].hBrush);

            /* Do the PatBlt operation for this rect */
            bResult &= PatBlt(hdc,
                               pPoly[i].nXLeft,
                               pPoly[i].nYLeft,
                               pPoly[i].nWidth,
                               pPoly[i].nHeight,
                               dwRop);
        }

        /* Restore the old brush */
        SelectObject(hdc, hbrOld);

        return bResult;
    }

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
    HANDLE_METADC(BOOL,
                  StretchBlt,
                  FALSE,
                  hdcDest,
                  xDest,
                  yDest,
                  cxDest,
                  cyDest,
                  hdcSrc,
                  xSrc,
                  ySrc,
                  cxSrc,
                  cySrc,
                  dwRop);

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
    HANDLE_METADC(BOOL,
                  MaskBlt,
                  FALSE,
                  hdcDest,
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
                  dwRop);

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
    HANDLE_METADC(BOOL,
                  PlgBlt,
                  FALSE,
                  hdcDest,
                  ppt,
                  hdcSrc,
                  xSrc,
                  ySrc,
                  cx,
                  cy,
                  hbmMask,
                  xMask,
                  yMask);

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

    HANDLE_METADC(BOOL,
                  AlphaBlend,
                  FALSE,
                  hdcDst,
                  xDst,
                  yDst,
                  cxDst,
                  cyDst,
                  hdcSrc,
                  xSrc,
                  ySrc,
                  cxSrc,
                  cySrc,
                  blendfn);

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
    HANDLE_METADC(BOOL,
                  TransparentBlt,
                  FALSE,
                  hdcDst,
                  xDst,
                  yDst,
                  cxDst,
                  cyDst,
                  hdcSrc,
                  xSrc,
                  ySrc,
                  cxSrc,
                  cySrc,
                  crTransparent);

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
    HANDLE_METADC(BOOL, GradientFill, FALSE, hdc, pVertex, nVertex, pMesh, nCount, ulMode);

    /* FIXME some part need be done in user mode */
    return NtGdiGradientFill(hdc, pVertex, nVertex, pMesh, nCount, ulMode);
}
