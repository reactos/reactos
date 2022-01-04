/*
 * PROJECT:         ReactOS win32 kernel mode subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            win32ss/gdi/ntgdi/fillshap.c
 * PURPOSE:         fillshap
 * PROGRAMMER:
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

#define Rsin(d) ((d) == 0.0 ? 0.0 : ((d) == 90.0 ? 1.0 : sin(d*M_PI/180.0)))
#define Rcos(d) ((d) == 0.0 ? 1.0 : ((d) == 90.0 ? 0.0 : cos(d*M_PI/180.0)))

BOOL FASTCALL
IntGdiPolygon(PDC    dc,
              PPOINT Points,
              int    Count)
{
    SURFACE *psurf;
    PBRUSH pbrLine, pbrFill;
    BOOL ret = FALSE; // Default to failure
    RECTL DestRect;
    INT i, CurrentPoint;
    PDC_ATTR pdcattr;
    POINTL BrushOrigin;
    PPATH pPath;
//    int Left;
//    int Top;

    ASSERT(dc); // Caller's responsibility to pass a valid dc

    if (!Points || Count < 2 )
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

/*
    // Find start x, y
    Left = Points[0].x;
    Top  = Points[0].y;
    for (CurrentPoint = 1; CurrentPoint < Count; ++CurrentPoint) {
      Left = min(Left, Points[CurrentPoint].x);
      Top  = min(Top, Points[CurrentPoint].y);
    }
*/

    pdcattr = dc->pdcattr;

    /* Convert to screen coordinates */
    IntLPtoDP(dc, Points, Count);
    for (CurrentPoint = 0; CurrentPoint < Count; CurrentPoint++)
    {
        Points[CurrentPoint].x += dc->ptlDCOrig.x;
        Points[CurrentPoint].y += dc->ptlDCOrig.y;
    }
    // No need to have path here.
    {
        DestRect.left   = Points[0].x;
        DestRect.right  = Points[0].x;
        DestRect.top    = Points[0].y;
        DestRect.bottom = Points[0].y;

        for (CurrentPoint = 1; CurrentPoint < Count; ++CurrentPoint)
        {
            DestRect.left     = min(DestRect.left, Points[CurrentPoint].x);
            DestRect.right    = max(DestRect.right, Points[CurrentPoint].x);
            DestRect.top      = min(DestRect.top, Points[CurrentPoint].y);
            DestRect.bottom   = max(DestRect.bottom, Points[CurrentPoint].y);
        }

        if (pdcattr->ulDirty_ & (DIRTY_FILL | DC_BRUSH_DIRTY))
            DC_vUpdateFillBrush(dc);

        if (pdcattr->ulDirty_ & (DIRTY_LINE | DC_PEN_DIRTY))
            DC_vUpdateLineBrush(dc);

        /* Special locking order to avoid lock-ups */
        pbrFill = dc->dclevel.pbrFill;
        pbrLine = dc->dclevel.pbrLine;
        psurf = dc->dclevel.pSurface;
        if (psurf == NULL)
        {
            /* Memory DC without a bitmap selected, nothing to do. */
            return TRUE;
        }

        /* Now fill the polygon with the current fill brush. */
        if (!(pbrFill->flAttrs & BR_IS_NULL))
        {
            BrushOrigin = *((PPOINTL)&pbrFill->ptOrigin);
            BrushOrigin.x += dc->ptlDCOrig.x;
            BrushOrigin.y += dc->ptlDCOrig.y;
            ret = IntFillPolygon (dc,
                                  psurf,
                                  &dc->eboFill.BrushObject,
                                  Points,
                                  Count,
                                  DestRect,
                                  &BrushOrigin);
        }

        // Draw the Polygon Edges with the current pen ( if not a NULL pen )
        if (!(pbrLine->flAttrs & BR_IS_NULL))
        {
            if (IntIsEffectiveWidePen(pbrLine))
            {
                /* Clear the path */
                PATH_Delete(dc->dclevel.hPath);
                dc->dclevel.hPath = NULL;

                /* Begin a path */
                pPath = PATH_CreatePath(Count + 1);
                dc->dclevel.flPath |= DCPATH_ACTIVE;
                dc->dclevel.hPath = pPath->BaseObject.hHmgr;
                pPath->pos = Points[0];
                IntLPtoDP(dc, &pPath->pos, 1);

                PATH_MoveTo(dc, pPath);
                for (i = 1; i < Count; ++i)
                {
                    PATH_LineTo(dc, Points[i].x, Points[i].y);
                }
                PATH_LineTo(dc, Points[0].x, Points[0].y);

                /* Close the path */
                pPath->state = PATH_Closed;
                dc->dclevel.flPath &= ~DCPATH_ACTIVE;

                /* Actually stroke a path */
                ret = PATH_StrokePath(dc, pPath);

                /* Clear the path */
                PATH_UnlockPath(pPath);
                PATH_Delete(dc->dclevel.hPath);
                dc->dclevel.hPath = NULL;
            }
            else
            {
                for (i = 0; i < Count-1; i++)
                {
// DPRINT1("Polygon Making line from (%d,%d) to (%d,%d)\n",
//                                 Points[0].x, Points[0].y,
//                                 Points[1].x, Points[1].y );

                    ret = IntEngLineTo(&psurf->SurfObj,
                                       (CLIPOBJ *)&dc->co,
                                       &dc->eboLine.BrushObject,
                                       Points[i].x,          /* From */
                                       Points[i].y,
                                       Points[i+1].x,        /* To */
                                       Points[i+1].y,
                                       &DestRect,
                                       ROP2_TO_MIX(pdcattr->jROP2)); /* MIX */
                    if (!ret) break;
                }
                /* Close the polygon */
                if (ret)
                {
                    ret = IntEngLineTo(&psurf->SurfObj,
                                       (CLIPOBJ *)&dc->co,
                                       &dc->eboLine.BrushObject,
                                       Points[Count-1].x, /* From */
                                       Points[Count-1].y,
                                       Points[0].x,       /* To */
                                       Points[0].y,
                                       &DestRect,
                                       ROP2_TO_MIX(pdcattr->jROP2)); /* MIX */
                }
            }
        }
    }

    return ret;
}

BOOL FASTCALL
IntGdiPolyPolygon(DC      *dc,
                  LPPOINT Points,
                  PULONG  PolyCounts,
                  int     Count)
{
    if (PATH_IsPathOpen(dc->dclevel))
        return PATH_PolyPolygon ( dc, Points, (PINT)PolyCounts, Count);

    while (--Count >=0)
    {
        if (!IntGdiPolygon ( dc, Points, *PolyCounts ))
            return FALSE;
        Points+=*PolyCounts++;
    }
    return TRUE;
}

BOOL FASTCALL
IntPolygon(HDC hdc, POINT *Point, int Count)
{
    BOOL bResult;
    PDC pdc;

    pdc = DC_LockDc(hdc);
    if (pdc == NULL)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    bResult = IntGdiPolygon(pdc, Point, Count);

    DC_UnlockDc(pdc);
    return bResult;
}


/******************************************************************************/

/*
 * NtGdiEllipse
 *
 * Author
 *    Filip Navara
 *
 * Remarks
 *    This function uses optimized Bresenham's ellipse algorithm. It draws
 *    four lines of the ellipse in one pass.
 *
 */

BOOL APIENTRY
NtGdiEllipse(
    HDC hDC,
    int Left,
    int Top,
    int Right,
    int Bottom)
{
    PDC dc;
    PDC_ATTR pdcattr;
    RECTL RectBounds;
    PBRUSH pbrush;
    BOOL ret = TRUE;
    LONG PenWidth, PenOrigWidth;
    LONG RadiusX, RadiusY, CenterX, CenterY;
    PBRUSH pFillBrushObj;
    BRUSH tmpFillBrushObj;

    dc = DC_LockDc(hDC);
    if (dc == NULL)
    {
       EngSetLastError(ERROR_INVALID_HANDLE);
       return FALSE;
    }

    if (PATH_IsPathOpen(dc->dclevel))
    {
        ret = PATH_Ellipse(dc, Left, Top, Right, Bottom);
        DC_UnlockDc(dc);
        return ret;
    }

    ////
    //// Could this use PATH_CheckCorners ?
    ////
    if ((Left == Right) || (Top == Bottom))
    {
       DC_UnlockDc(dc);
       return TRUE;
    }

    if (Right < Left)
    {
       INT tmp = Right; Right = Left; Left = tmp;
    }
    if (Bottom < Top)
    {
       INT tmp = Bottom; Bottom = Top; Top = tmp;
    }
    ////

    pdcattr = dc->pdcattr;

    if (pdcattr->ulDirty_ & (DIRTY_FILL | DC_BRUSH_DIRTY))
        DC_vUpdateFillBrush(dc);

    if (pdcattr->ulDirty_ & (DIRTY_LINE | DC_PEN_DIRTY))
        DC_vUpdateLineBrush(dc);

    pbrush = PEN_ShareLockPen(pdcattr->hpen);
    if (!pbrush)
    {
        DPRINT1("Ellipse Fail 1\n");
        DC_UnlockDc(dc);
        EngSetLastError(ERROR_INTERNAL_ERROR);
        return FALSE;
    }

    PenOrigWidth = PenWidth = pbrush->lWidth;
    if (pbrush->ulPenStyle == PS_NULL) PenWidth = 0;

    if (pbrush->ulPenStyle == PS_INSIDEFRAME)
    {
       if (2*PenWidth > (Right - Left)) PenWidth = (Right -Left + 1)/2;
       if (2*PenWidth > (Bottom - Top)) PenWidth = (Bottom -Top + 1)/2;
       Left   += PenWidth / 2;
       Right  -= (PenWidth - 1) / 2;
       Top    += PenWidth / 2;
       Bottom -= (PenWidth - 1) / 2;
    }

    if (!PenWidth) PenWidth = 1;
    pbrush->lWidth = PenWidth;

    RectBounds.left   = Left;
    RectBounds.right  = Right;
    RectBounds.top    = Top;
    RectBounds.bottom = Bottom;

    IntLPtoDP(dc, (LPPOINT)&RectBounds, 2);

    RectBounds.left += dc->ptlDCOrig.x;
    RectBounds.right += dc->ptlDCOrig.x;
    RectBounds.top += dc->ptlDCOrig.y;
    RectBounds.bottom += dc->ptlDCOrig.y;

    // Setup for dynamic width and height.
    RadiusX = max((RectBounds.right - RectBounds.left) / 2, 2); // Needs room
    RadiusY = max((RectBounds.bottom - RectBounds.top) / 2, 2);
    CenterX = (RectBounds.right + RectBounds.left) / 2;
    CenterY = (RectBounds.bottom + RectBounds.top) / 2;

    DPRINT("Ellipse 1: Left: %d, Top: %d, Right: %d, Bottom: %d\n",
               RectBounds.left,RectBounds.top,RectBounds.right,RectBounds.bottom);

    DPRINT("Ellipse 2: XLeft: %d, YLeft: %d, Width: %d, Height: %d\n",
               CenterX - RadiusX, CenterY + RadiusY, RadiusX*2, RadiusY*2);

    pFillBrushObj = BRUSH_ShareLockBrush(pdcattr->hbrush);
    if (NULL == pFillBrushObj)
    {
        DPRINT1("FillEllipse Fail\n");
        EngSetLastError(ERROR_INTERNAL_ERROR);
        ret = FALSE;
    }
    else
    {
        RtlCopyMemory(&tmpFillBrushObj, pFillBrushObj, sizeof(tmpFillBrushObj));
        //tmpFillBrushObj.ptOrigin.x += RectBounds.left - Left;
        //tmpFillBrushObj.ptOrigin.y += RectBounds.top - Top;
        tmpFillBrushObj.ptOrigin.x += dc->ptlDCOrig.x;
        tmpFillBrushObj.ptOrigin.y += dc->ptlDCOrig.y;

        DC_vPrepareDCsForBlit(dc, &RectBounds, NULL, NULL);

        ret = IntFillEllipse( dc,
                              CenterX - RadiusX,
                              CenterY - RadiusY,
                              RadiusX*2, // Width
                              RadiusY*2, // Height
                              &tmpFillBrushObj);
        BRUSH_ShareUnlockBrush(pFillBrushObj);

        if (ret)
        {
           ret = IntDrawEllipse( dc,
                                 CenterX - RadiusX,
                                 CenterY - RadiusY,
                                 RadiusX*2, // Width
                                 RadiusY*2, // Height
                                 pbrush);
        }

        DC_vFinishBlit(dc, NULL);
    }

    pbrush->lWidth = PenOrigWidth;
    PEN_ShareUnlockPen(pbrush);
    DC_UnlockDc(dc);
    DPRINT("Ellipse Exit.\n");
    return ret;
}

#if 0

// When the fill mode is ALTERNATE, GDI fills the area between odd-numbered and
// even-numbered polygon sides on each scan line. That is, GDI fills the area between the
// first and second side, between the third and fourth side, and so on.

// WINDING Selects winding mode (fills any region with a nonzero winding value).
// When the fill mode is WINDING, GDI fills any region that has a nonzero winding value.
// This value is defined as the number of times a pen used to draw the polygon would go around the region.
// The direction of each edge of the polygon is important.

extern BOOL FillPolygon(PDC dc,
                            SURFOBJ *SurfObj,
                            PBRUSHOBJ BrushObj,
                            MIX RopMode,
                            CONST PPOINT Points,
                            int Count,
                            RECTL BoundRect);

#endif


ULONG_PTR
APIENTRY
NtGdiPolyPolyDraw( IN HDC hDC,
                   IN PPOINT UnsafePoints,
                   IN PULONG UnsafeCounts,
                   IN ULONG Count,
                   IN INT iFunc )
{
    DC *dc;
    PVOID pTemp;
    LPPOINT SafePoints;
    PULONG SafeCounts;
    NTSTATUS Status = STATUS_SUCCESS;
    BOOL Ret = TRUE;
    ULONG nPoints = 0, nMaxPoints = 0, nInvalid = 0, i;

    if (!UnsafePoints || !UnsafeCounts ||
        Count == 0 || iFunc == 0 || iFunc > GdiPolyPolyRgn)
    {
        /* Windows doesn't set last error */
        return FALSE;
    }

    _SEH2_TRY
    {
        ProbeForRead(UnsafePoints, Count * sizeof(POINT), 1);
        ProbeForRead(UnsafeCounts, Count * sizeof(ULONG), 1);

        /* Count points and validate poligons */
        for (i = 0; i < Count; i++)
        {
            if (UnsafeCounts[i] < 2)
            {
                nInvalid++;
            }
            nPoints += UnsafeCounts[i];
            nMaxPoints = max(nMaxPoints, UnsafeCounts[i]);
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    if (!NT_SUCCESS(Status))
    {
        /* Windows doesn't set last error */
        return FALSE;
    }

    if (nPoints == 0 || nPoints < nMaxPoints)
    {
        /* If all polygon counts are zero, or we have overflow,
           return without setting a last error code. */
        return FALSE;
    }

    if (nInvalid != 0)
    {
        /* If at least one poly count is 0 or 1, fail */
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Allocate one buffer for both counts and points */
    pTemp = ExAllocatePoolWithTag(PagedPool,
                                  Count * sizeof(ULONG) + nPoints * sizeof(POINT),
                                  TAG_SHAPE);
    if (!pTemp)
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    SafeCounts = pTemp;
    SafePoints = (PVOID)(SafeCounts + Count);

    _SEH2_TRY
    {
        /* Pointers already probed! */
        RtlCopyMemory(SafeCounts, UnsafeCounts, Count * sizeof(ULONG));
        RtlCopyMemory(SafePoints, UnsafePoints, nPoints * sizeof(POINT));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(pTemp, TAG_SHAPE);
        return FALSE;
    }

    /* Special handling for GdiPolyPolyRgn */
    if (iFunc == GdiPolyPolyRgn)
    {
        INT iMode = (INT)(UINT_PTR)hDC;
        HRGN hrgn;

        hrgn = GreCreatePolyPolygonRgn(SafePoints, SafeCounts, Count, iMode);

        ExFreePoolWithTag(pTemp, TAG_SHAPE);
        return (ULONG_PTR)hrgn;
    }

    dc = DC_LockDc(hDC);
    if (!dc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        ExFreePoolWithTag(pTemp, TAG_SHAPE);
        return FALSE;
    }

    DC_vPrepareDCsForBlit(dc, NULL, NULL, NULL);

    if (dc->pdcattr->ulDirty_ & (DIRTY_FILL | DC_BRUSH_DIRTY))
        DC_vUpdateFillBrush(dc);

    if (dc->pdcattr->ulDirty_ & (DIRTY_LINE | DC_PEN_DIRTY))
        DC_vUpdateLineBrush(dc);

    /* Perform the actual work */
    switch (iFunc)
    {
        case GdiPolyPolygon:
            Ret = IntGdiPolyPolygon(dc, SafePoints, SafeCounts, Count);
            break;
        case GdiPolyPolyLine:
            Ret = IntGdiPolyPolyline(dc, SafePoints, SafeCounts, Count);
            break;
        case GdiPolyBezier:
            Ret = IntGdiPolyBezier(dc, SafePoints, *SafeCounts);
            break;
        case GdiPolyLineTo:
            Ret = IntGdiPolylineTo(dc, SafePoints, *SafeCounts);
            break;
        case GdiPolyBezierTo:
            Ret = IntGdiPolyBezierTo(dc, SafePoints, *SafeCounts);
            break;
        default:
            EngSetLastError(ERROR_INVALID_PARAMETER);
            Ret = FALSE;
    }

    /* Cleanup and return */
    DC_vFinishBlit(dc, NULL);
    DC_UnlockDc(dc);
    ExFreePoolWithTag(pTemp, TAG_SHAPE);

    return (ULONG_PTR)Ret;
}


BOOL
FASTCALL
IntRectangle(PDC dc,
             int LeftRect,
             int TopRect,
             int RightRect,
             int BottomRect)
{
    SURFACE *psurf = NULL;
    PBRUSH pbrLine, pbrFill;
    BOOL       ret = FALSE; // Default to failure
    RECTL      DestRect;
    MIX        Mix;
    PDC_ATTR pdcattr;
    POINTL BrushOrigin;
    PPATH pPath;

    ASSERT ( dc ); // Caller's responsibility to set this up

    pdcattr = dc->pdcattr;

    // Rectangle Path only.
    if ( PATH_IsPathOpen(dc->dclevel) )
    {
        return PATH_Rectangle ( dc, LeftRect, TopRect, RightRect, BottomRect );
    }

	/* Make sure rectangle is not inverted */
    DestRect.left   = min(LeftRect, RightRect);
    DestRect.right  = max(LeftRect, RightRect);
    DestRect.top    = min(TopRect,  BottomRect);
    DestRect.bottom = max(TopRect,  BottomRect);

    IntLPtoDP(dc, (LPPOINT)&DestRect, 2);

    DestRect.left   += dc->ptlDCOrig.x;
    DestRect.right  += dc->ptlDCOrig.x;
    DestRect.top    += dc->ptlDCOrig.y;
    DestRect.bottom += dc->ptlDCOrig.y;

    if (dc->fs & (DC_ACCUM_APP|DC_ACCUM_WMGR))
    {
       IntUpdateBoundsRect(dc, &DestRect);
    }

    /* In GM_COMPATIBLE, don't include bottom and right edges */
    if (pdcattr->iGraphicsMode == GM_COMPATIBLE)
    {
        DestRect.right--;
        DestRect.bottom--;
    }

    DC_vPrepareDCsForBlit(dc, &DestRect, NULL, NULL);

    if (pdcattr->ulDirty_ & (DIRTY_FILL | DC_BRUSH_DIRTY))
        DC_vUpdateFillBrush(dc);

    if (pdcattr->ulDirty_ & (DIRTY_LINE | DC_PEN_DIRTY))
        DC_vUpdateLineBrush(dc);

    pbrFill = dc->dclevel.pbrFill;
    pbrLine = dc->dclevel.pbrLine;
    if (!pbrLine)
    {
        ret = FALSE;
        goto cleanup;
    }

    psurf = dc->dclevel.pSurface;
    if (!psurf)
    {
        ret = TRUE;
        goto cleanup;
    }

    if (pbrFill)
    {
        if (!(pbrFill->flAttrs & BR_IS_NULL))
        {
            BrushOrigin = *((PPOINTL)&pbrFill->ptOrigin);
            BrushOrigin.x += dc->ptlDCOrig.x;
            BrushOrigin.y += dc->ptlDCOrig.y;
            ret = IntEngBitBlt(&psurf->SurfObj,
                               NULL,
                               NULL,
                               (CLIPOBJ *)&dc->co,
                               NULL,
                               &DestRect,
                               NULL,
                               NULL,
                               &dc->eboFill.BrushObject,
                               &BrushOrigin,
                               ROP4_FROM_INDEX(R3_OPINDEX_PATCOPY));
        }
    }

    // Draw the rectangle with the current pen

    ret = TRUE; // Change default to success

    if (!(pbrLine->flAttrs & BR_IS_NULL))
    {
        if (IntIsEffectiveWidePen(pbrLine))
        {
            /* Clear the path */
            PATH_Delete(dc->dclevel.hPath);
            dc->dclevel.hPath = NULL;

            /* Begin a path */
            pPath = PATH_CreatePath(5);
            dc->dclevel.flPath |= DCPATH_ACTIVE;
            dc->dclevel.hPath = pPath->BaseObject.hHmgr;
            pPath->pos.x = LeftRect;
            pPath->pos.y = TopRect;
            IntLPtoDP(dc, &pPath->pos, 1);

            PATH_MoveTo(dc, pPath);
            PATH_LineTo(dc, RightRect, TopRect);
            PATH_LineTo(dc, RightRect, BottomRect);
            PATH_LineTo(dc, LeftRect, BottomRect);
            PATH_LineTo(dc, LeftRect, TopRect);

            /* Close the path */
            pPath->state = PATH_Closed;
            dc->dclevel.flPath &= ~DCPATH_ACTIVE;

            /* Actually stroke a path */
            ret = PATH_StrokePath(dc, pPath);

            /* Clear the path */
            PATH_UnlockPath(pPath);
            PATH_Delete(dc->dclevel.hPath);
            dc->dclevel.hPath = NULL;
        }
        else
        {
            Mix = ROP2_TO_MIX(pdcattr->jROP2);
            ret = ret && IntEngLineTo(&psurf->SurfObj,
                                      (CLIPOBJ *)&dc->co,
                                      &dc->eboLine.BrushObject,
                                      DestRect.left, DestRect.top, DestRect.right, DestRect.top,
                                      &DestRect, // Bounding rectangle
                                      Mix);

            ret = ret && IntEngLineTo(&psurf->SurfObj,
                                      (CLIPOBJ *)&dc->co,
                                      &dc->eboLine.BrushObject,
                                      DestRect.right, DestRect.top, DestRect.right, DestRect.bottom,
                                      &DestRect, // Bounding rectangle
                                      Mix);

            ret = ret && IntEngLineTo(&psurf->SurfObj,
                                      (CLIPOBJ *)&dc->co,
                                      &dc->eboLine.BrushObject,
                                      DestRect.right, DestRect.bottom, DestRect.left, DestRect.bottom,
                                      &DestRect, // Bounding rectangle
                                      Mix);

            ret = ret && IntEngLineTo(&psurf->SurfObj,
                                      (CLIPOBJ *)&dc->co,
                                      &dc->eboLine.BrushObject,
                                      DestRect.left, DestRect.bottom, DestRect.left, DestRect.top,
                                      &DestRect, // Bounding rectangle
                                      Mix);
        }
    }

cleanup:
    DC_vFinishBlit(dc, NULL);

    /* Move current position in DC?
       MSDN: The current position is neither used nor updated by Rectangle. */

    return ret;
}

BOOL
APIENTRY
NtGdiRectangle(HDC  hDC,
               int  LeftRect,
               int  TopRect,
               int  RightRect,
               int  BottomRect)
{
    DC   *dc;
    BOOL ret; // Default to failure

    dc = DC_LockDc(hDC);
    if (!dc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    /* Do we rotate or shear? */
    if (!(dc->pdcattr->mxWorldToDevice.flAccel & XFORM_SCALE))
    {
        POINTL DestCoords[4];
        ULONG PolyCounts = 4;

        DestCoords[0].x = DestCoords[3].x = LeftRect;
        DestCoords[0].y = DestCoords[1].y = TopRect;
        DestCoords[1].x = DestCoords[2].x = RightRect;
        DestCoords[2].y = DestCoords[3].y = BottomRect;
        // Use IntGdiPolyPolygon so to support PATH.
        ret = IntGdiPolyPolygon(dc, DestCoords, &PolyCounts, 1);
    }
    else
    {
        ret = IntRectangle(dc, LeftRect, TopRect, RightRect, BottomRect );
    }

    DC_UnlockDc(dc);

    return ret;
}


BOOL
FASTCALL
IntRoundRect(
    PDC  dc,
    int  Left,
    int  Top,
    int  Right,
    int  Bottom,
    int  xCurveDiameter,
    int  yCurveDiameter)
{
    PDC_ATTR pdcattr;
    PBRUSH   pbrLine, pbrFill;
    RECTL RectBounds;
    LONG PenWidth, PenOrigWidth;
    BOOL ret = TRUE; // Default to success
    BRUSH brushTemp;

    ASSERT ( dc ); // Caller's responsibility to set this up

    if ( PATH_IsPathOpen(dc->dclevel) )
        return PATH_RoundRect ( dc, Left, Top, Right, Bottom,
                                xCurveDiameter, yCurveDiameter );

    if ((Left == Right) || (Top == Bottom)) return TRUE;

    xCurveDiameter = max(abs( xCurveDiameter ), 1);
    yCurveDiameter = max(abs( yCurveDiameter ), 1);

    if (Right < Left)
    {
       INT tmp = Right; Right = Left; Left = tmp;
    }
    if (Bottom < Top)
    {
       INT tmp = Bottom; Bottom = Top; Top = tmp;
    }

    pdcattr = dc->pdcattr;

    if (pdcattr->ulDirty_ & (DIRTY_FILL | DC_BRUSH_DIRTY))
        DC_vUpdateFillBrush(dc);

    if (pdcattr->ulDirty_ & (DIRTY_LINE | DC_PEN_DIRTY))
        DC_vUpdateLineBrush(dc);

    pbrLine = PEN_ShareLockPen(pdcattr->hpen);
    if (!pbrLine)
    {
        /* Nothing to do, as we don't have a bitmap */
        EngSetLastError(ERROR_INTERNAL_ERROR);
        return FALSE;
    }

    PenOrigWidth = PenWidth = pbrLine->lWidth;
    if (pbrLine->ulPenStyle == PS_NULL) PenWidth = 0;

    if (pbrLine->ulPenStyle == PS_INSIDEFRAME)
    {
       if (2*PenWidth > (Right - Left)) PenWidth = (Right -Left + 1)/2;
       if (2*PenWidth > (Bottom - Top)) PenWidth = (Bottom -Top + 1)/2;
       Left   += PenWidth / 2;
       Right  -= (PenWidth - 1) / 2;
       Top    += PenWidth / 2;
       Bottom -= (PenWidth - 1) / 2;
    }

    if (!PenWidth) PenWidth = 1;
    pbrLine->lWidth = PenWidth;

    RectBounds.left = Left;
    RectBounds.top = Top;
    RectBounds.right = Right;
    RectBounds.bottom = Bottom;

    IntLPtoDP(dc, (LPPOINT)&RectBounds, 2);

    RectBounds.left   += dc->ptlDCOrig.x;
    RectBounds.top    += dc->ptlDCOrig.y;
    RectBounds.right  += dc->ptlDCOrig.x;
    RectBounds.bottom += dc->ptlDCOrig.y;

    pbrFill = BRUSH_ShareLockBrush(pdcattr->hbrush);
    if (!pbrFill)
    {
        DPRINT1("FillRound Fail\n");
        EngSetLastError(ERROR_INTERNAL_ERROR);
        ret = FALSE;
    }
    else
    {

        DC_vPrepareDCsForBlit(dc, &RectBounds, NULL, NULL);

        RtlCopyMemory(&brushTemp, pbrFill, sizeof(brushTemp));
        brushTemp.ptOrigin.x += RectBounds.left - Left;
        brushTemp.ptOrigin.y += RectBounds.top - Top;
        ret = IntFillRoundRect( dc,
                                RectBounds.left,
                                RectBounds.top,
                                RectBounds.right,
                                RectBounds.bottom,
                                xCurveDiameter,
                                yCurveDiameter,
                                &brushTemp);
        BRUSH_ShareUnlockBrush(pbrFill);

        if (ret)
        {
           ret = IntDrawRoundRect( dc,
                      RectBounds.left,
                       RectBounds.top,
                     RectBounds.right,
                    RectBounds.bottom,
                       xCurveDiameter,
                       yCurveDiameter,
                       pbrLine);
        }

        DC_vFinishBlit(dc, NULL);
    }


    pbrLine->lWidth = PenOrigWidth;
    PEN_ShareUnlockPen(pbrLine);
    return ret;
}

BOOL
APIENTRY
NtGdiRoundRect(
    HDC  hDC,
    int  LeftRect,
    int  TopRect,
    int  RightRect,
    int  BottomRect,
    int  Width,
    int  Height)
{
    DC   *dc = DC_LockDc(hDC);
    BOOL  ret = FALSE; /* Default to failure */

    DPRINT("NtGdiRoundRect(0x%p,%i,%i,%i,%i,%i,%i)\n",hDC,LeftRect,TopRect,RightRect,BottomRect,Width,Height);
    if ( !dc )
    {
        DPRINT1("NtGdiRoundRect() - hDC is invalid\n");
        EngSetLastError(ERROR_INVALID_HANDLE);
    }
    else
    {
        ret = IntRoundRect ( dc, LeftRect, TopRect, RightRect, BottomRect, Width, Height );
        DC_UnlockDc ( dc );
    }

    return ret;
}

BOOL
NTAPI
GreGradientFill(
    HDC hdc,
    PTRIVERTEX pVertex,
    ULONG nVertex,
    PVOID pMesh,
    ULONG nMesh,
    ULONG ulMode)
{
    PDC pdc;
    SURFACE *psurf;
    EXLATEOBJ exlo;
    RECTL rclExtent;
    POINTL ptlDitherOrg;
    ULONG i;
    BOOL bRet;

    /* Check parameters */
    if (ulMode & GRADIENT_FILL_TRIANGLE)
    {
        PGRADIENT_TRIANGLE pTriangle = (PGRADIENT_TRIANGLE)pMesh;

        for (i = 0; i < nMesh; i++, pTriangle++)
        {
            if (pTriangle->Vertex1 >= nVertex ||
                pTriangle->Vertex2 >= nVertex ||
                pTriangle->Vertex3 >= nVertex)
            {
                EngSetLastError(ERROR_INVALID_PARAMETER);
                return FALSE;
            }
        }
    }
    else
    {
        PGRADIENT_RECT pRect = (PGRADIENT_RECT)pMesh;
        for (i = 0; i < nMesh; i++, pRect++)
        {
            if (pRect->UpperLeft >= nVertex || pRect->LowerRight >= nVertex)
            {
                EngSetLastError(ERROR_INVALID_PARAMETER);
                return FALSE;
            }
        }
    }

    /* Lock the output DC */
    pdc = DC_LockDc(hdc);
    if(!pdc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (!pdc->dclevel.pSurface)
    {
        /* Memory DC with no surface selected */
        DC_UnlockDc(pdc);
        return TRUE; // CHECKME
    }

    /* Calculate extent */
    rclExtent.left = rclExtent.right = pVertex->x;
    rclExtent.top = rclExtent.bottom = pVertex->y;
    for (i = 0; i < nVertex; i++)
    {
        rclExtent.left = min(rclExtent.left, (pVertex + i)->x);
        rclExtent.right = max(rclExtent.right, (pVertex + i)->x);
        rclExtent.top = min(rclExtent.top, (pVertex + i)->y);
        rclExtent.bottom = max(rclExtent.bottom, (pVertex + i)->y);
    }
    IntLPtoDP(pdc, (LPPOINT)&rclExtent, 2);

    rclExtent.left   += pdc->ptlDCOrig.x;
    rclExtent.right  += pdc->ptlDCOrig.x;
    rclExtent.top    += pdc->ptlDCOrig.y;
    rclExtent.bottom += pdc->ptlDCOrig.y;

    if (RECTL_bIsEmptyRect(&rclExtent))
    {
        DC_UnlockDc(pdc);
        return TRUE;
    }

    ptlDitherOrg.x = ptlDitherOrg.y = 0;
    IntLPtoDP(pdc, (LPPOINT)&ptlDitherOrg, 1);

    ptlDitherOrg.x += pdc->ptlDCOrig.x;
    ptlDitherOrg.y += pdc->ptlDCOrig.y;

   if (pdc->fs & (DC_ACCUM_APP|DC_ACCUM_WMGR))
   {
      IntUpdateBoundsRect(pdc, &rclExtent);
   }

    DC_vPrepareDCsForBlit(pdc, &rclExtent, NULL, NULL);

    psurf = pdc->dclevel.pSurface;

    EXLATEOBJ_vInitialize(&exlo, &gpalRGB, psurf->ppal, 0, 0, 0);

    bRet = IntEngGradientFill(&psurf->SurfObj,
                             (CLIPOBJ *)&pdc->co,
                             &exlo.xlo,
                             pVertex,
                             nVertex,
                             pMesh,
                             nMesh,
                             &rclExtent,
                             &ptlDitherOrg,
                             ulMode);

    EXLATEOBJ_vCleanup(&exlo);
    DC_vFinishBlit(pdc, NULL);
    DC_UnlockDc(pdc);

    return bRet;
}

BOOL
APIENTRY
NtGdiGradientFill(
    HDC hdc,
    PTRIVERTEX pVertex,
    ULONG nVertex,
    PVOID pMesh,
    ULONG nMesh,
    ULONG ulMode)
{
    BOOL bRet;
    PTRIVERTEX SafeVertex;
    PVOID SafeMesh;
    ULONG cbVertex, cbMesh;

    /* Validate parameters */
    if (!pVertex || !nVertex || !pMesh || !nMesh)
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    switch (ulMode)
    {
        case GRADIENT_FILL_RECT_H:
        case GRADIENT_FILL_RECT_V:
            cbMesh = nMesh * sizeof(GRADIENT_RECT);
            break;
        case GRADIENT_FILL_TRIANGLE:
            cbMesh = nMesh * sizeof(GRADIENT_TRIANGLE);
            break;
        default:
            EngSetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
    }

    cbVertex = nVertex * sizeof(TRIVERTEX) ;
    if(cbVertex + cbMesh <= cbVertex)
    {
        /* Overflow */
        return FALSE ;
    }

    /* Allocate a kernel mode buffer */
    SafeVertex = ExAllocatePoolWithTag(PagedPool, cbVertex + cbMesh, TAG_SHAPE);
    if(!SafeVertex)
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    SafeMesh = (PVOID)((ULONG_PTR)SafeVertex + cbVertex);

    /* Copy the parameters to kernel mode */
    _SEH2_TRY
    {
        ProbeForRead(pVertex, cbVertex, 1);
        ProbeForRead(pMesh, cbMesh, 1);
        RtlCopyMemory(SafeVertex, pVertex, cbVertex);
        RtlCopyMemory(SafeMesh, pMesh, cbMesh);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ExFreePoolWithTag(SafeVertex, TAG_SHAPE);
        SetLastNtError(_SEH2_GetExceptionCode());
        _SEH2_YIELD(return FALSE;)
    }
    _SEH2_END;

    /* Call the internal function */
    bRet = GreGradientFill(hdc, SafeVertex, nVertex, SafeMesh, nMesh, ulMode);

    /* Cleanup and return result */
    ExFreePoolWithTag(SafeVertex, TAG_SHAPE);
    return bRet;
}

BOOL APIENTRY
NtGdiExtFloodFill(
    HDC  hDC,
    INT  XStart,
    INT  YStart,
    COLORREF  Color,
    UINT  FillType)
{
    PDC dc;
#if 0
    PDC_ATTR   pdcattr;
#endif
    SURFACE    *psurf;
    EXLATEOBJ  exlo;
    BOOL       Ret = FALSE;
    RECTL      DestRect;
    POINTL     Pt;
    ULONG      ConvColor;
    PREGION    prgn;

    dc = DC_LockDc(hDC);
    if (!dc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (!dc->dclevel.pSurface)
    {
        Ret = TRUE;
        goto cleanup;
    }

#if 0
    pdcattr = dc->pdcattr;
#endif

    Pt.x = XStart;
    Pt.y = YStart;
    IntLPtoDP(dc, (LPPOINT)&Pt, 1);

    DC_vPrepareDCsForBlit(dc, &DestRect, NULL, NULL);

    psurf = dc->dclevel.pSurface;

    prgn = dc->prgnRao ? dc->prgnRao : dc->prgnVis;
    if (prgn)
    {
        Ret = REGION_PtInRegion(prgn, Pt.x, Pt.y);
        if (Ret)
            REGION_GetRgnBox(prgn, (LPRECT)&DestRect);
        else
        {
            DC_vFinishBlit(dc, NULL);
            goto cleanup;
        }
    }
    else
    {
        RECTL_vSetRect(&DestRect, 0, 0, psurf->SurfObj.sizlBitmap.cx, psurf->SurfObj.sizlBitmap.cy);
    }

    if (dc->fs & (DC_ACCUM_APP|DC_ACCUM_WMGR))
    {
       IntUpdateBoundsRect(dc, &DestRect);
    }

    EXLATEOBJ_vInitialize(&exlo, &gpalRGB, psurf->ppal, 0, 0xffffff, 0);

    /* Only solid fills supported for now
     * How to support pattern brushes and non standard surfaces (not offering dib functions):
     * Version a (most likely slow): call DrvPatBlt for every pixel
     * Version b: create a flood mask and let MaskBlt blit a masked brush */
    ConvColor = XLATEOBJ_iXlate(&exlo.xlo, Color);
    Ret = DIB_XXBPP_FloodFillSolid(&psurf->SurfObj, &dc->eboFill.BrushObject, &DestRect, &Pt, ConvColor, FillType);

    DC_vFinishBlit(dc, NULL);

    EXLATEOBJ_vCleanup(&exlo);

cleanup:
    DC_UnlockDc(dc);
    return Ret;
}

/* EOF */
