/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

// Some code from the WINE project source (www.winehq.com)

// Should use Fx in Point
//
BOOL FASTCALL
IntGdiMoveToEx(DC      *dc,
               int     X,
               int     Y,
               LPPOINT Point)
{
    BOOL  PathIsOpen;
    PDC_ATTR pdcattr = dc->pdcattr;
    if ( Point )
    {
        if ( pdcattr->ulDirty_ & DIRTY_PTLCURRENT ) // Double hit!
        {
            Point->x = pdcattr->ptfxCurrent.x; // ret prev before change.
            Point->y = pdcattr->ptfxCurrent.y;
            IntDPtoLP ( dc, Point, 1);         // reconvert back.
        }
        else
        {
            Point->x = pdcattr->ptlCurrent.x;
            Point->y = pdcattr->ptlCurrent.y;
        }
    }
    pdcattr->ptlCurrent.x = X;
    pdcattr->ptlCurrent.y = Y;
    pdcattr->ptfxCurrent = pdcattr->ptlCurrent;
    CoordLPtoDP(dc, &pdcattr->ptfxCurrent); // Update fx
    pdcattr->ulDirty_ &= ~(DIRTY_PTLCURRENT|DIRTY_PTFXCURRENT|DIRTY_STYLESTATE);

    PathIsOpen = PATH_IsPathOpen(dc->dclevel);

    if ( PathIsOpen )
        return PATH_MoveTo ( dc );

    return TRUE;
}

// Should use Fx in pt
//
VOID FASTCALL
IntGetCurrentPositionEx(PDC dc, LPPOINT pt)
{
    PDC_ATTR pdcattr = dc->pdcattr;

    if ( pt )
    {
        if (pdcattr->ulDirty_ & DIRTY_PTFXCURRENT)
        {
            pdcattr->ptfxCurrent = pdcattr->ptlCurrent;
            CoordLPtoDP(dc, &pdcattr->ptfxCurrent); // Update fx
            pdcattr->ulDirty_ &= ~(DIRTY_PTFXCURRENT|DIRTY_STYLESTATE);
        }
        pt->x = pdcattr->ptlCurrent.x;
        pt->y = pdcattr->ptlCurrent.y;
    }
}

BOOL FASTCALL
IntGdiLineTo(DC  *dc,
             int XEnd,
             int YEnd)
{
    SURFACE *psurf;
    BOOL      Ret = TRUE;
    PBRUSH pbrLine;
    RECTL     Bounds;
    POINT     Points[2];
    PDC_ATTR pdcattr = dc->pdcattr;

    if (PATH_IsPathOpen(dc->dclevel))
    {
        Ret = PATH_LineTo(dc, XEnd, YEnd);
        if (Ret)
        {
            // FIXME - PATH_LineTo should maybe do this? No
            pdcattr->ptlCurrent.x = XEnd;
            pdcattr->ptlCurrent.y = YEnd;
            pdcattr->ptfxCurrent = pdcattr->ptlCurrent;
            CoordLPtoDP(dc, &pdcattr->ptfxCurrent); // Update fx
            pdcattr->ulDirty_ &= ~(DIRTY_PTLCURRENT|DIRTY_PTFXCURRENT|DIRTY_STYLESTATE);
        }
        return Ret;
    }
    else
    {
       if (pdcattr->ulDirty_ & (DIRTY_LINE | DC_PEN_DIRTY))
          DC_vUpdateLineBrush(dc);

        psurf = SURFACE_LockSurface( dc->rosdc.hBitmap );
        if (NULL == psurf)
        {
            SetLastWin32Error(ERROR_INVALID_HANDLE);
            return FALSE;
        }

        Points[0].x = pdcattr->ptlCurrent.x;
        Points[0].y = pdcattr->ptlCurrent.y;
        Points[1].x = XEnd;
        Points[1].y = YEnd;

        IntLPtoDP(dc, Points, 2);

        /* The DCOrg is in device coordinates */
        Points[0].x += dc->ptlDCOrig.x;
        Points[0].y += dc->ptlDCOrig.y;
        Points[1].x += dc->ptlDCOrig.x;
        Points[1].y += dc->ptlDCOrig.y;

        Bounds.left = min(Points[0].x, Points[1].x);
        Bounds.top = min(Points[0].y, Points[1].y);
        Bounds.right = max(Points[0].x, Points[1].x);
        Bounds.bottom = max(Points[0].y, Points[1].y);

        /* get BRUSH from current pen. */
        pbrLine = dc->dclevel.pbrLine;
        ASSERT(pbrLine);

        if (!(pbrLine->flAttrs & GDIBRUSH_IS_NULL))
        {
            Ret = IntEngLineTo(&psurf->SurfObj,
                               dc->rosdc.CombinedClip,
                               &dc->eboLine.BrushObject,
                               Points[0].x, Points[0].y,
                               Points[1].x, Points[1].y,
                               &Bounds,
                               ROP2_TO_MIX(pdcattr->jROP2));
        }

        SURFACE_UnlockSurface(psurf);
    }

    if (Ret)
    {
        pdcattr->ptlCurrent.x = XEnd;
        pdcattr->ptlCurrent.y = YEnd;
        pdcattr->ptfxCurrent = pdcattr->ptlCurrent;
        CoordLPtoDP(dc, &pdcattr->ptfxCurrent); // Update fx
        pdcattr->ulDirty_ &= ~(DIRTY_PTLCURRENT|DIRTY_PTFXCURRENT|DIRTY_STYLESTATE);
    }

    return Ret;
}

BOOL FASTCALL
IntGdiPolyBezier(DC      *dc,
                 LPPOINT pt,
                 DWORD   Count)
{
    BOOL ret = FALSE; // default to FAILURE

    if ( PATH_IsPathOpen(dc->dclevel) )
    {
        return PATH_PolyBezier ( dc, pt, Count );
    }

    /* We'll convert it into line segments and draw them using Polyline */
    {
        POINT *Pts;
        INT nOut;

        Pts = GDI_Bezier ( pt, Count, &nOut );
        if ( Pts )
        {
            ret = IntGdiPolyline(dc, Pts, nOut);
            ExFreePoolWithTag(Pts, TAG_BEZIER);
        }
    }

    return ret;
}

BOOL FASTCALL
IntGdiPolyBezierTo(DC      *dc,
                   LPPOINT pt,
                   DWORD  Count)
{
    BOOL ret = FALSE; // default to failure
    PDC_ATTR pdcattr = dc->pdcattr;

    if ( PATH_IsPathOpen(dc->dclevel) )
        ret = PATH_PolyBezierTo ( dc, pt, Count );
    else /* We'll do it using PolyBezier */
    {
        POINT *npt;
        npt = ExAllocatePoolWithTag(PagedPool,
                                    sizeof(POINT) * (Count + 1),
                                    TAG_BEZIER);
        if ( npt )
        {
            npt[0].x = pdcattr->ptlCurrent.x;
            npt[0].y = pdcattr->ptlCurrent.y;
            memcpy(npt + 1, pt, sizeof(POINT) * Count);
            ret = IntGdiPolyBezier(dc, npt, Count+1);
            ExFreePoolWithTag(npt, TAG_BEZIER);
        }
    }
    if ( ret )
    {
        pdcattr->ptlCurrent.x = pt[Count-1].x;
        pdcattr->ptlCurrent.y = pt[Count-1].y;
        pdcattr->ptfxCurrent = pdcattr->ptlCurrent;
        CoordLPtoDP(dc, &pdcattr->ptfxCurrent); // Update fx
        pdcattr->ulDirty_ &= ~(DIRTY_PTLCURRENT|DIRTY_PTFXCURRENT|DIRTY_STYLESTATE);
    }

    return ret;
}

BOOL FASTCALL
IntGdiPolyline(DC      *dc,
               LPPOINT pt,
               int     Count)
{
    SURFACE *psurf;
    BRUSH *pbrLine;
    LPPOINT Points;
    BOOL Ret = TRUE;
    LONG i;
    PDC_ATTR pdcattr = dc->pdcattr;

    if (PATH_IsPathOpen(dc->dclevel))
        return PATH_Polyline(dc, pt, Count);

    if (pdcattr->ulDirty_ & (DIRTY_FILL | DC_BRUSH_DIRTY))
        DC_vUpdateFillBrush(dc);

    if (pdcattr->ulDirty_ & (DIRTY_LINE | DC_PEN_DIRTY))
        DC_vUpdateLineBrush(dc);

    /* Get BRUSHOBJ from current pen. */
    pbrLine = dc->dclevel.pbrLine;
    ASSERT(pbrLine);

    if (!(pbrLine->flAttrs & GDIBRUSH_IS_NULL))
    {
        Points = EngAllocMem(0, Count * sizeof(POINT), TAG_COORD);
        if (Points != NULL)
        {
            psurf = SURFACE_LockSurface(dc->rosdc.hBitmap);
            /* FIXME - psurf can be NULL!!!!
               Don't assert but handle this case gracefully! */
            ASSERT(psurf);

            RtlCopyMemory(Points, pt, Count * sizeof(POINT));
            IntLPtoDP(dc, Points, Count);

            /* Offset the array of point by the dc->rosdc.DCOrg */
            for (i = 0; i < Count; i++)
            {
                Points[i].x += dc->ptlDCOrig.x;
                Points[i].y += dc->ptlDCOrig.y;
            }

            Ret = IntEngPolyline(&psurf->SurfObj,
                                 dc->rosdc.CombinedClip,
                                 &dc->eboLine.BrushObject,
                                 Points,
                                 Count,
                                 ROP2_TO_MIX(pdcattr->jROP2));

            SURFACE_UnlockSurface(psurf);
            EngFreeMem(Points);
        }
        else
        {
            Ret = FALSE;
        }
    }

    return Ret;
}

BOOL FASTCALL
IntGdiPolylineTo(DC      *dc,
                 LPPOINT pt,
                 DWORD   Count)
{
    BOOL ret = FALSE; // default to failure
    PDC_ATTR pdcattr = dc->pdcattr;

    if (PATH_IsPathOpen(dc->dclevel))
    {
        ret = PATH_PolylineTo(dc, pt, Count);
    }
    else /* do it using Polyline */
    {
        POINT *pts = ExAllocatePoolWithTag(PagedPool,
                                           sizeof(POINT) * (Count + 1),
                                           TAG_SHAPE);
        if ( pts )
        {
            pts[0].x = pdcattr->ptlCurrent.x;
            pts[0].y = pdcattr->ptlCurrent.y;
            memcpy( pts + 1, pt, sizeof(POINT) * Count);
            ret = IntGdiPolyline(dc, pts, Count + 1);
            ExFreePoolWithTag(pts, TAG_SHAPE);
        }
    }
    if ( ret )
    {
        pdcattr->ptlCurrent.x = pt[Count-1].x;
        pdcattr->ptlCurrent.y = pt[Count-1].y;
        pdcattr->ptfxCurrent = pdcattr->ptlCurrent;
        CoordLPtoDP(dc, &pdcattr->ptfxCurrent); // Update fx
        pdcattr->ulDirty_ &= ~(DIRTY_PTLCURRENT|DIRTY_PTFXCURRENT|DIRTY_STYLESTATE);
    }

    return ret;
}


BOOL FASTCALL
IntGdiPolyPolyline(DC      *dc,
                   LPPOINT pt,
                   PULONG  PolyPoints,
                   DWORD   Count)
{
    int i;
    LPPOINT pts;
    PULONG pc;
    BOOL ret = FALSE; // default to failure
    pts = pt;
    pc = PolyPoints;

    if (PATH_IsPathOpen(dc->dclevel))
        return PATH_PolyPolyline( dc, pt, PolyPoints, Count );

    for (i = 0; i < Count; i++)
    {
        ret = IntGdiPolyline ( dc, pts, *pc );
        if (ret == FALSE)
        {
            return ret;
        }
        pts+=*pc++;
    }

    return ret;
}

/******************************************************************************/

BOOL
APIENTRY
NtGdiLineTo(HDC  hDC,
            int  XEnd,
            int  YEnd)
{
    DC *dc;
    BOOL Ret;

    dc = DC_LockDc(hDC);
    if (!dc)
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (dc->dctype == DC_TYPE_INFO)
    {
        DC_UnlockDc(dc);
        /* Yes, Windows really returns TRUE in this case */
        return TRUE;
    }

    Ret = IntGdiLineTo(dc, XEnd, YEnd);

    DC_UnlockDc(dc);
    return Ret;
}

BOOL
APIENTRY
NtGdiPolyDraw(
    IN HDC hdc,
    IN LPPOINT lppt,
    IN LPBYTE lpbTypes,
    IN ULONG cCount)
{
    PDC dc;
    PPATH pPath;
    BOOL result = FALSE;
    POINT lastmove;
    unsigned int i;
    PDC_ATTR pdcattr;

    dc = DC_LockDc(hdc);
    if (!dc) return FALSE;
    pdcattr = dc->pdcattr;

    _SEH2_TRY
    {
        ProbeArrayForRead(lppt, sizeof(POINT), cCount, sizeof(LONG));
        ProbeArrayForRead(lpbTypes, sizeof(BYTE), cCount, sizeof(BYTE));

        /* check for each bezierto if there are two more points */
        for ( i = 0; i < cCount; i++ )
        {
            if ( lpbTypes[i] != PT_MOVETO &&
                 lpbTypes[i] & PT_BEZIERTO )
            {
                if ( cCount < i+3 ) _SEH2_LEAVE;
                else i += 2;
            }
        }

        /* if no moveto occurs, we will close the figure here */
        lastmove.x = pdcattr->ptlCurrent.x;
        lastmove.y = pdcattr->ptlCurrent.y;

        /* now let's draw */
        for ( i = 0; i < cCount; i++ )
        {
            if ( lpbTypes[i] == PT_MOVETO )
            {
                IntGdiMoveToEx( dc, lppt[i].x, lppt[i].y, NULL );
                lastmove.x = pdcattr->ptlCurrent.x;
                lastmove.y = pdcattr->ptlCurrent.y;
            }
            else if ( lpbTypes[i] & PT_LINETO )
                IntGdiLineTo( dc, lppt[i].x, lppt[i].y );
            else if ( lpbTypes[i] & PT_BEZIERTO )
            {
                POINT pts[4];
                pts[0].x = pdcattr->ptlCurrent.x;
                pts[0].y = pdcattr->ptlCurrent.y;
                RtlCopyMemory(pts + 1, &lppt[i], sizeof(POINT) * 3);
                IntGdiPolyBezier(dc, pts, 4);
                i += 2;
            }
            else _SEH2_LEAVE;

            if ( lpbTypes[i] & PT_CLOSEFIGURE )
            {
                if ( PATH_IsPathOpen(dc->dclevel) )
                {
                    pPath = PATH_LockPath( dc->dclevel.hPath );
                    if (pPath)
                    {
                       IntGdiCloseFigure( pPath );
                       PATH_UnlockPath( pPath );
                    }
                }
                else IntGdiLineTo( dc, lastmove.x, lastmove.y );
            }
        }

        result = TRUE;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastNtError(_SEH2_GetExceptionCode());
    }
    _SEH2_END;

    DC_UnlockDc(dc);

    return result;
}

 /*
 * @unimplemented
 */
BOOL
APIENTRY
NtGdiMoveTo(
    IN HDC hdc,
    IN INT x,
    IN INT y,
    OUT OPTIONAL LPPOINT pptOut)
{
    UNIMPLEMENTED;
    return FALSE;
}

/* EOF */
