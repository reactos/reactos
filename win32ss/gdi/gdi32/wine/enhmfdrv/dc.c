/*
 * Enhanced MetaFile driver dc value functions
 *
 * Copyright 1999 Huw D M Davies
 * Copyright 2016 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <assert.h>
#include "enhmfdrv/enhmetafiledrv.h"

/* get the emf physdev from the path physdev */
static inline PHYSDEV get_emfdev( PHYSDEV path )
{
    return &CONTAINING_RECORD( path, EMFDRV_PDEVICE, pathdev )->dev;
}

static const struct gdi_dc_funcs emfpath_driver;

INT EMFDRV_SaveDC( PHYSDEV dev )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pSaveDC );
    INT ret = next->funcs->pSaveDC( next );

    if (ret)
    {
        EMRSAVEDC emr;
        emr.emr.iType = EMR_SAVEDC;
        emr.emr.nSize = sizeof(emr);
        EMFDRV_WriteRecord( dev, &emr.emr );
    }
    return ret;
}

BOOL EMFDRV_RestoreDC( PHYSDEV dev, INT level )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pRestoreDC );
    EMFDRV_PDEVICE* physDev = get_emf_physdev( dev );
    DC *dc = get_physdev_dc( dev );
    EMRRESTOREDC emr;
    BOOL ret;

    emr.emr.iType = EMR_RESTOREDC;
    emr.emr.nSize = sizeof(emr);

    if (level < 0)
        emr.iRelative = level;
    else
        emr.iRelative = level - dc->saveLevel - 1;

    physDev->restoring++;
    ret = next->funcs->pRestoreDC( next, level );
    physDev->restoring--;

    if (ret) EMFDRV_WriteRecord( dev, &emr.emr );
    return ret;
}

UINT EMFDRV_SetTextAlign( PHYSDEV dev, UINT align )
{
    EMRSETTEXTALIGN emr;
    emr.emr.iType = EMR_SETTEXTALIGN;
    emr.emr.nSize = sizeof(emr);
    emr.iMode = align;
    return EMFDRV_WriteRecord( dev, &emr.emr ) ? align : GDI_ERROR;
}

BOOL EMFDRV_SetTextJustification(PHYSDEV dev, INT nBreakExtra, INT nBreakCount)
{
    EMRSETTEXTJUSTIFICATION emr;
    emr.emr.iType = EMR_SETTEXTJUSTIFICATION;
    emr.emr.nSize = sizeof(emr);
    emr.nBreakExtra = nBreakExtra;
    emr.nBreakCount = nBreakCount;
    return EMFDRV_WriteRecord(dev, &emr.emr);
}

INT EMFDRV_SetBkMode( PHYSDEV dev, INT mode )
{
    EMRSETBKMODE emr;
    emr.emr.iType = EMR_SETBKMODE;
    emr.emr.nSize = sizeof(emr);
    emr.iMode = mode;
    return EMFDRV_WriteRecord( dev, &emr.emr ) ? mode : 0;
}

COLORREF EMFDRV_SetBkColor( PHYSDEV dev, COLORREF color )
{
    EMRSETBKCOLOR emr;
    EMFDRV_PDEVICE *physDev = get_emf_physdev( dev );

    if (physDev->restoring) return color;  /* don't output records during RestoreDC */

    emr.emr.iType = EMR_SETBKCOLOR;
    emr.emr.nSize = sizeof(emr);
    emr.crColor = color;
    return EMFDRV_WriteRecord( dev, &emr.emr ) ? color : CLR_INVALID;
}


COLORREF EMFDRV_SetTextColor( PHYSDEV dev, COLORREF color )
{
    EMRSETTEXTCOLOR emr;
    EMFDRV_PDEVICE *physDev = get_emf_physdev( dev );

    if (physDev->restoring) return color;  /* don't output records during RestoreDC */

    emr.emr.iType = EMR_SETTEXTCOLOR;
    emr.emr.nSize = sizeof(emr);
    emr.crColor = color;
    return EMFDRV_WriteRecord( dev, &emr.emr ) ? color : CLR_INVALID;
}

INT EMFDRV_SetROP2( PHYSDEV dev, INT rop )
{
    EMRSETROP2 emr;
    emr.emr.iType = EMR_SETROP2;
    emr.emr.nSize = sizeof(emr);
    emr.iMode = rop;
    return EMFDRV_WriteRecord( dev, &emr.emr ) ? rop : 0;
}

INT EMFDRV_SetPolyFillMode( PHYSDEV dev, INT mode )
{
    EMRSETPOLYFILLMODE emr;
    emr.emr.iType = EMR_SETPOLYFILLMODE;
    emr.emr.nSize = sizeof(emr);
    emr.iMode = mode;
    return EMFDRV_WriteRecord( dev, &emr.emr ) ? mode : 0;
}

INT EMFDRV_SetStretchBltMode( PHYSDEV dev, INT mode )
{
    EMRSETSTRETCHBLTMODE emr;
    emr.emr.iType = EMR_SETSTRETCHBLTMODE;
    emr.emr.nSize = sizeof(emr);
    emr.iMode = mode;
    return EMFDRV_WriteRecord( dev, &emr.emr ) ? mode : 0;
}

INT EMFDRV_SetArcDirection(PHYSDEV dev, INT arcDirection)
{
    EMRSETARCDIRECTION emr;

    emr.emr.iType = EMR_SETARCDIRECTION;
    emr.emr.nSize = sizeof(emr);
    emr.iArcDirection = arcDirection;
    return EMFDRV_WriteRecord(dev, &emr.emr) ? arcDirection : 0;
}

INT EMFDRV_ExcludeClipRect( PHYSDEV dev, INT left, INT top, INT right, INT bottom )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pExcludeClipRect );
    EMREXCLUDECLIPRECT emr;

    emr.emr.iType      = EMR_EXCLUDECLIPRECT;
    emr.emr.nSize      = sizeof(emr);
    emr.rclClip.left   = left;
    emr.rclClip.top    = top;
    emr.rclClip.right  = right;
    emr.rclClip.bottom = bottom;
    if (!EMFDRV_WriteRecord( dev, &emr.emr )) return ERROR;
    return next->funcs->pExcludeClipRect( next, left, top, right, bottom );
}

INT EMFDRV_IntersectClipRect( PHYSDEV dev, INT left, INT top, INT right, INT bottom)
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pIntersectClipRect );
    EMRINTERSECTCLIPRECT emr;

    emr.emr.iType      = EMR_INTERSECTCLIPRECT;
    emr.emr.nSize      = sizeof(emr);
    emr.rclClip.left   = left;
    emr.rclClip.top    = top;
    emr.rclClip.right  = right;
    emr.rclClip.bottom = bottom;
    if (!EMFDRV_WriteRecord( dev, &emr.emr )) return ERROR;
    return next->funcs->pIntersectClipRect( next, left, top, right, bottom );
}

INT EMFDRV_OffsetClipRgn( PHYSDEV dev, INT x, INT y )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pOffsetClipRgn );
    EMROFFSETCLIPRGN emr;

    emr.emr.iType   = EMR_OFFSETCLIPRGN;
    emr.emr.nSize   = sizeof(emr);
    emr.ptlOffset.x = x;
    emr.ptlOffset.y = y;
    if (!EMFDRV_WriteRecord( dev, &emr.emr )) return ERROR;
    return next->funcs->pOffsetClipRgn( next, x, y );
}

INT EMFDRV_ExtSelectClipRgn( PHYSDEV dev, HRGN hrgn, INT mode )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pExtSelectClipRgn );
    EMREXTSELECTCLIPRGN *emr;
    DWORD size, rgnsize;
    BOOL ret;

    if (!hrgn)
    {
        if (mode != RGN_COPY) return ERROR;
        rgnsize = 0;
    }
    else rgnsize = GetRegionData( hrgn, 0, NULL );

    size = rgnsize + offsetof(EMREXTSELECTCLIPRGN,RgnData);
    emr = HeapAlloc( GetProcessHeap(), 0, size );
    if (rgnsize) GetRegionData( hrgn, rgnsize, (RGNDATA *)&emr->RgnData );

    emr->emr.iType = EMR_EXTSELECTCLIPRGN;
    emr->emr.nSize = size;
    emr->cbRgnData = rgnsize;
    emr->iMode     = mode;

    ret = EMFDRV_WriteRecord( dev, &emr->emr );
    HeapFree( GetProcessHeap(), 0, emr );
    return ret ? next->funcs->pExtSelectClipRgn( next, hrgn, mode ) : ERROR;
}

INT EMFDRV_SetMapMode( PHYSDEV dev, INT mode )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pSetMapMode );
    EMRSETMAPMODE emr;
    emr.emr.iType = EMR_SETMAPMODE;
    emr.emr.nSize = sizeof(emr);
    emr.iMode = mode;

    if (!EMFDRV_WriteRecord( dev, &emr.emr )) return 0;
    return next->funcs->pSetMapMode( next, mode );
}

BOOL EMFDRV_SetViewportExtEx( PHYSDEV dev, INT cx, INT cy, SIZE *size )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pSetViewportExtEx );
    EMRSETVIEWPORTEXTEX emr;

    emr.emr.iType = EMR_SETVIEWPORTEXTEX;
    emr.emr.nSize = sizeof(emr);
    emr.szlExtent.cx = cx;
    emr.szlExtent.cy = cy;

    if (!EMFDRV_WriteRecord( dev, &emr.emr )) return FALSE;
    return next->funcs->pSetViewportExtEx( next, cx, cy, size );
}

BOOL EMFDRV_SetWindowExtEx( PHYSDEV dev, INT cx, INT cy, SIZE *size )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pSetWindowExtEx );
    EMRSETWINDOWEXTEX emr;

    emr.emr.iType = EMR_SETWINDOWEXTEX;
    emr.emr.nSize = sizeof(emr);
    emr.szlExtent.cx = cx;
    emr.szlExtent.cy = cy;

    if (!EMFDRV_WriteRecord( dev, &emr.emr )) return FALSE;
    return next->funcs->pSetWindowExtEx( next, cx, cy, size );
}

BOOL EMFDRV_SetViewportOrgEx( PHYSDEV dev, INT x, INT y, POINT *pt )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pSetViewportOrgEx );
    EMRSETVIEWPORTORGEX emr;

    emr.emr.iType = EMR_SETVIEWPORTORGEX;
    emr.emr.nSize = sizeof(emr);
    emr.ptlOrigin.x = x;
    emr.ptlOrigin.y = y;

    if (!EMFDRV_WriteRecord( dev, &emr.emr )) return FALSE;
    return next->funcs->pSetViewportOrgEx( next, x, y, pt );
}

BOOL EMFDRV_SetWindowOrgEx( PHYSDEV dev, INT x, INT y, POINT *pt )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pSetWindowOrgEx );
    EMRSETWINDOWORGEX emr;

    emr.emr.iType = EMR_SETWINDOWORGEX;
    emr.emr.nSize = sizeof(emr);
    emr.ptlOrigin.x = x;
    emr.ptlOrigin.y = y;

    if (!EMFDRV_WriteRecord( dev, &emr.emr )) return FALSE;
    return next->funcs->pSetWindowOrgEx( next, x, y, pt );
}

BOOL EMFDRV_ScaleViewportExtEx( PHYSDEV dev, INT xNum, INT xDenom, INT yNum, INT yDenom, SIZE *size )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pScaleViewportExtEx );
    EMRSCALEVIEWPORTEXTEX emr;

    emr.emr.iType = EMR_SCALEVIEWPORTEXTEX;
    emr.emr.nSize = sizeof(emr);
    emr.xNum      = xNum;
    emr.xDenom    = xDenom;
    emr.yNum      = yNum;
    emr.yDenom    = yDenom;

    if (!EMFDRV_WriteRecord( dev, &emr.emr )) return FALSE;
    return next->funcs->pScaleViewportExtEx( next, xNum, xDenom, yNum, yDenom, size );
}

BOOL EMFDRV_ScaleWindowExtEx( PHYSDEV dev, INT xNum, INT xDenom, INT yNum, INT yDenom, SIZE *size )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pScaleWindowExtEx );
    EMRSCALEWINDOWEXTEX emr;

    emr.emr.iType = EMR_SCALEWINDOWEXTEX;
    emr.emr.nSize = sizeof(emr);
    emr.xNum      = xNum;
    emr.xDenom    = xDenom;
    emr.yNum      = yNum;
    emr.yDenom    = yDenom;

    if (!EMFDRV_WriteRecord( dev, &emr.emr )) return FALSE;
    return next->funcs->pScaleWindowExtEx( next, xNum, xDenom, yNum, yDenom, size );
}

DWORD EMFDRV_SetLayout( PHYSDEV dev, DWORD layout )
{
    EMRSETLAYOUT emr;

    emr.emr.iType = EMR_SETLAYOUT;
    emr.emr.nSize = sizeof(emr);
    emr.iMode = layout;
    return EMFDRV_WriteRecord( dev, &emr.emr ) ? layout : GDI_ERROR;
}

BOOL EMFDRV_SetWorldTransform( PHYSDEV dev, const XFORM *xform)
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pSetWorldTransform );
    EMRSETWORLDTRANSFORM emr;

    emr.emr.iType = EMR_SETWORLDTRANSFORM;
    emr.emr.nSize = sizeof(emr);
    emr.xform = *xform;

    if (!EMFDRV_WriteRecord( dev, &emr.emr )) return FALSE;
    return next->funcs->pSetWorldTransform( next, xform );
}

BOOL EMFDRV_ModifyWorldTransform( PHYSDEV dev, const XFORM *xform, DWORD mode)
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pModifyWorldTransform );
    EMRMODIFYWORLDTRANSFORM emr;

    emr.emr.iType = EMR_MODIFYWORLDTRANSFORM;
    emr.emr.nSize = sizeof(emr);
    emr.xform = *xform;
    emr.iMode = mode;

    if (!EMFDRV_WriteRecord( dev, &emr.emr )) return FALSE;
    return next->funcs->pModifyWorldTransform( next, xform, mode );
}

BOOL EMFDRV_OffsetViewportOrgEx( PHYSDEV dev, INT x, INT y, POINT *pt )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pOffsetViewportOrgEx );
    EMRSETVIEWPORTORGEX emr;
    POINT prev;

    GetViewportOrgEx( dev->hdc, &prev );

    emr.emr.iType = EMR_SETVIEWPORTORGEX;
    emr.emr.nSize = sizeof(emr);
    emr.ptlOrigin.x = prev.x + x;
    emr.ptlOrigin.y = prev.y + y;

    if (!EMFDRV_WriteRecord( dev, &emr.emr )) return FALSE;
    return next->funcs->pOffsetViewportOrgEx( next, x, y, pt );
}

BOOL EMFDRV_OffsetWindowOrgEx( PHYSDEV dev, INT x, INT y, POINT *pt )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pOffsetWindowOrgEx );
    EMRSETWINDOWORGEX emr;
    POINT prev;

    GetWindowOrgEx( dev->hdc, &prev );

    emr.emr.iType = EMR_SETWINDOWORGEX;
    emr.emr.nSize = sizeof(emr);
    emr.ptlOrigin.x = prev.x + x;
    emr.ptlOrigin.y = prev.y + y;

    if (!EMFDRV_WriteRecord( dev, &emr.emr )) return FALSE;
    return next->funcs->pOffsetWindowOrgEx( next, x, y, pt );
}

DWORD EMFDRV_SetMapperFlags( PHYSDEV dev, DWORD flags )
{
    EMRSETMAPPERFLAGS emr;

    emr.emr.iType = EMR_SETMAPPERFLAGS;
    emr.emr.nSize = sizeof(emr);
    emr.dwFlags   = flags;

    return EMFDRV_WriteRecord( dev, &emr.emr ) ? flags : GDI_ERROR;
}

BOOL EMFDRV_AbortPath( PHYSDEV dev )
{
    EMRABORTPATH emr;

    emr.emr.iType = EMR_ABORTPATH;
    emr.emr.nSize = sizeof(emr);

    return EMFDRV_WriteRecord( dev, &emr.emr );
}

BOOL EMFDRV_BeginPath( PHYSDEV dev )
{
    EMFDRV_PDEVICE *physDev = get_emf_physdev( dev );
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pBeginPath );
    EMRBEGINPATH emr;
    DC *dc = get_physdev_dc( dev );

    emr.emr.iType = EMR_BEGINPATH;
    emr.emr.nSize = sizeof(emr);

    if (!EMFDRV_WriteRecord( dev, &emr.emr )) return FALSE;
    if (physDev->path) return TRUE;  /* already open */

    if (!next->funcs->pBeginPath( next )) return FALSE;
    push_dc_driver( &dc->physDev, &physDev->pathdev, &emfpath_driver );
    physDev->path = TRUE;
    return TRUE;
}

BOOL EMFDRV_CloseFigure( PHYSDEV dev )
{
    EMRCLOSEFIGURE emr;

    emr.emr.iType = EMR_CLOSEFIGURE;
    emr.emr.nSize = sizeof(emr);

    EMFDRV_WriteRecord( dev, &emr.emr );
    return FALSE;  /* always fails without a path */
}

BOOL EMFDRV_EndPath( PHYSDEV dev )
{
    EMRENDPATH emr;

    emr.emr.iType = EMR_ENDPATH;
    emr.emr.nSize = sizeof(emr);

    EMFDRV_WriteRecord( dev, &emr.emr );
    return FALSE;  /* always fails without a path */
}

BOOL EMFDRV_FlattenPath( PHYSDEV dev )
{
    EMRFLATTENPATH emr;

    emr.emr.iType = EMR_FLATTENPATH;
    emr.emr.nSize = sizeof(emr);

    return EMFDRV_WriteRecord( dev, &emr.emr );
}

BOOL EMFDRV_SelectClipPath( PHYSDEV dev, INT iMode )
{
 //   PHYSDEV next = GET_NEXT_PHYSDEV( dev, pSelectClipPath ); This HACK breaks test_emf_clipping
    EMRSELECTCLIPPATH emr;
 //   BOOL ret = FALSE;
 //   HRGN hrgn;

    emr.emr.iType = EMR_SELECTCLIPPATH;
    emr.emr.nSize = sizeof(emr);
    emr.iMode = iMode;

    if (!EMFDRV_WriteRecord( dev, &emr.emr )) return FALSE;
/*    hrgn = PathToRegion( dev->hdc );
    if (hrgn)
    {
        ret = next->funcs->pExtSelectClipRgn( next, hrgn, iMode );
        DeleteObject( hrgn );
    } ERR("EMFDRV_SelectClipPath ret %d\n",ret);
    return ret;*/
    return TRUE;
}

BOOL EMFDRV_WidenPath( PHYSDEV dev )
{
    EMRWIDENPATH emr;

    emr.emr.iType = EMR_WIDENPATH;
    emr.emr.nSize = sizeof(emr);

    return EMFDRV_WriteRecord( dev, &emr.emr );
}

INT EMFDRV_GetDeviceCaps(PHYSDEV dev, INT cap)
{
    EMFDRV_PDEVICE *physDev = get_emf_physdev( dev );

    return GetDeviceCaps( physDev->ref_dc, cap );
}


/***********************************************************************
 *           emfpathdrv_AbortPath
 */
static BOOL emfpathdrv_AbortPath( PHYSDEV dev )
{
    PHYSDEV emfdev = get_emfdev( dev );
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pAbortPath );
    DC *dc = get_physdev_dc( dev );

    emfpath_driver.pDeleteDC( pop_dc_driver( dc, &emfpath_driver ));
    emfdev->funcs->pAbortPath( emfdev );
    return next->funcs->pAbortPath( next );
}

/***********************************************************************
 *           emfpathdrv_AngleArc
 */
static BOOL emfpathdrv_AngleArc( PHYSDEV dev, INT x, INT y, DWORD radius, FLOAT start, FLOAT sweep )
{
    PHYSDEV emfdev = get_emfdev( dev );
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pAngleArc );

    return (emfdev->funcs->pAngleArc( emfdev, x, y, radius, start, sweep ) &&
            next->funcs->pAngleArc( next, x, y, radius, start, sweep ));
}

/***********************************************************************
 *           emfpathdrv_Arc
 */
static BOOL emfpathdrv_Arc( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                            INT xstart, INT ystart, INT xend, INT yend )
{
    PHYSDEV emfdev = get_emfdev( dev );
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pArc );

    return (emfdev->funcs->pArc( emfdev, left, top, right, bottom, xstart, ystart, xend, yend ) &&
            next->funcs->pArc( next, left, top, right, bottom, xstart, ystart, xend, yend ));
}

/***********************************************************************
 *           emfpathdrv_ArcTo
 */
static BOOL emfpathdrv_ArcTo( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                              INT xstart, INT ystart, INT xend, INT yend )
{
    PHYSDEV emfdev = get_emfdev( dev );
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pArcTo );

    return (emfdev->funcs->pArcTo( emfdev, left, top, right, bottom, xstart, ystart, xend, yend ) &&
            next->funcs->pArcTo( next, left, top, right, bottom, xstart, ystart, xend, yend ));
}

/***********************************************************************
 *           emfpathdrv_BeginPath
 */
static BOOL emfpathdrv_BeginPath( PHYSDEV dev )
{
    PHYSDEV emfdev = get_emfdev( dev );
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pBeginPath );

    return (emfdev->funcs->pBeginPath( emfdev ) && next->funcs->pBeginPath( next ));
}

/***********************************************************************
 *           emfpathdrv_Chord
 */
static BOOL emfpathdrv_Chord( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                              INT xstart, INT ystart, INT xend, INT yend )
{
    PHYSDEV emfdev = get_emfdev( dev );
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pChord );

    return (emfdev->funcs->pChord( emfdev, left, top, right, bottom, xstart, ystart, xend, yend ) &&
            next->funcs->pChord( next, left, top, right, bottom, xstart, ystart, xend, yend ));
}

/***********************************************************************
 *           emfpathdrv_CloseFigure
 */
static BOOL emfpathdrv_CloseFigure( PHYSDEV dev )
{
    PHYSDEV emfdev = get_emfdev( dev );
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pCloseFigure );

    emfdev->funcs->pCloseFigure( emfdev );
    return next->funcs->pCloseFigure( next );
}

/***********************************************************************
 *           emfpathdrv_CreateDC
 */
static BOOL emfpathdrv_CreateDC( PHYSDEV *dev, LPCWSTR driver, LPCWSTR device,
                                 LPCWSTR output, const DEVMODEW *devmode )
{
    assert( 0 );  /* should never be called */
    return TRUE;
}

/*************************************************************
 *           emfpathdrv_DeleteDC
 */
static BOOL emfpathdrv_DeleteDC( PHYSDEV dev )
{
    EMFDRV_PDEVICE *physdev = (EMFDRV_PDEVICE *)get_emfdev( dev );

    physdev->path = FALSE;
    return TRUE;
}

/***********************************************************************
 *           emfpathdrv_Ellipse
 */
static BOOL emfpathdrv_Ellipse( PHYSDEV dev, INT x1, INT y1, INT x2, INT y2 )
{
    PHYSDEV emfdev = get_emfdev( dev );
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pEllipse );

    return (emfdev->funcs->pEllipse( emfdev, x1, y1, x2, y2 ) &&
            next->funcs->pEllipse( next, x1, y1, x2, y2 ));
}

/***********************************************************************
 *           emfpathdrv_EndPath
 */
static BOOL emfpathdrv_EndPath( PHYSDEV dev )
{
    PHYSDEV emfdev = get_emfdev( dev );
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pEndPath );
    DC *dc = get_physdev_dc( dev );

    emfpath_driver.pDeleteDC( pop_dc_driver( dc, &emfpath_driver ));
    emfdev->funcs->pEndPath( emfdev );
    return next->funcs->pEndPath( next );
}

/***********************************************************************
 *           emfpathdrv_ExtTextOut
 */
static BOOL emfpathdrv_ExtTextOut( PHYSDEV dev, INT x, INT y, UINT flags, const RECT *rect,
                                   LPCWSTR str, UINT count, const INT *dx )
{
    PHYSDEV emfdev = get_emfdev( dev );
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pExtTextOut );

    return (emfdev->funcs->pExtTextOut( emfdev, x, y, flags, rect, str, count, dx ) &&
            next->funcs->pExtTextOut( next, x, y, flags, rect, str, count, dx ));
}

/***********************************************************************
 *           emfpathdrv_LineTo
 */
static BOOL emfpathdrv_LineTo( PHYSDEV dev, INT x, INT y )
{
    PHYSDEV emfdev = get_emfdev( dev );
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pLineTo );

    return (emfdev->funcs->pLineTo( emfdev, x, y ) && next->funcs->pLineTo( next, x, y ));
}

/***********************************************************************
 *           emfpathdrv_MoveTo
 */
static BOOL emfpathdrv_MoveTo( PHYSDEV dev, INT x, INT y )
{
    PHYSDEV emfdev = get_emfdev( dev );
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pMoveTo );

    return (emfdev->funcs->pMoveTo( emfdev, x, y ) && next->funcs->pMoveTo( next, x, y ));
}

/***********************************************************************
 *           emfpathdrv_Pie
 */
static BOOL emfpathdrv_Pie( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                            INT xstart, INT ystart, INT xend, INT yend )
{
    PHYSDEV emfdev = get_emfdev( dev );
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pPie );

    return (emfdev->funcs->pPie( emfdev, left, top, right, bottom, xstart, ystart, xend, yend ) &&
            next->funcs->pPie( next, left, top, right, bottom, xstart, ystart, xend, yend ));
}

/***********************************************************************
 *           emfpathdrv_PolyBezier
 */
static BOOL emfpathdrv_PolyBezier( PHYSDEV dev, const POINT *pts, DWORD count )
{
    PHYSDEV emfdev = get_emfdev( dev );
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pPolyBezier );

    return (emfdev->funcs->pPolyBezier( emfdev, pts, count ) &&
            next->funcs->pPolyBezier( next, pts, count ));
}

/***********************************************************************
 *           emfpathdrv_PolyBezierTo
 */
static BOOL emfpathdrv_PolyBezierTo( PHYSDEV dev, const POINT *pts, DWORD count )
{
    PHYSDEV emfdev = get_emfdev( dev );
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pPolyBezierTo );

    return (emfdev->funcs->pPolyBezierTo( emfdev, pts, count ) &&
            next->funcs->pPolyBezierTo( next, pts, count ));
}

/***********************************************************************
 *           emfpathdrv_PolyDraw
 */
static BOOL emfpathdrv_PolyDraw( PHYSDEV dev, const POINT *pts, const BYTE *types, DWORD count )
{
    PHYSDEV emfdev = get_emfdev( dev );
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pPolyDraw );

    return (emfdev->funcs->pPolyDraw( emfdev, pts, types, count ) &&
            next->funcs->pPolyDraw( next, pts, types, count ));
}

/***********************************************************************
 *           emfpathdrv_PolyPolygon
 */
static BOOL emfpathdrv_PolyPolygon( PHYSDEV dev, const POINT *pts, const INT *counts, UINT polygons )
{
    PHYSDEV emfdev = get_emfdev( dev );
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pPolyPolygon );

    return (emfdev->funcs->pPolyPolygon( emfdev, pts, counts, polygons ) &&
            next->funcs->pPolyPolygon( next, pts, counts, polygons ));
}

/***********************************************************************
 *           emfpathdrv_PolyPolyline
 */
static BOOL emfpathdrv_PolyPolyline( PHYSDEV dev, const POINT *pts, const DWORD *counts, DWORD polylines )
{
    PHYSDEV emfdev = get_emfdev( dev );
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pPolyPolyline );

    return (emfdev->funcs->pPolyPolyline( emfdev, pts, counts, polylines ) &&
            next->funcs->pPolyPolyline( next, pts, counts, polylines ));
}

/***********************************************************************
 *           emfpathdrv_Polygon
 */
static BOOL emfpathdrv_Polygon( PHYSDEV dev, const POINT *pts, INT count )
{
    PHYSDEV emfdev = get_emfdev( dev );
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pPolygon );

    return (emfdev->funcs->pPolygon( emfdev, pts, count ) &&
            next->funcs->pPolygon( next, pts, count ));
}

/***********************************************************************
 *           emfpathdrv_Polyline
 */
static BOOL emfpathdrv_Polyline( PHYSDEV dev, const POINT *pts, INT count )
{
    PHYSDEV emfdev = get_emfdev( dev );
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pPolyline );

    return (emfdev->funcs->pPolyline( emfdev, pts, count ) &&
            next->funcs->pPolyline( next, pts, count ));
}

/***********************************************************************
 *           emfpathdrv_PolylineTo
 */
static BOOL emfpathdrv_PolylineTo( PHYSDEV dev, const POINT *pts, INT count )
{
    PHYSDEV emfdev = get_emfdev( dev );
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pPolylineTo );

    return (emfdev->funcs->pPolylineTo( emfdev, pts, count ) &&
            next->funcs->pPolylineTo( next, pts, count ));
}

/***********************************************************************
 *           emfpathdrv_Rectangle
 */
static BOOL emfpathdrv_Rectangle( PHYSDEV dev, INT x1, INT y1, INT x2, INT y2 )
{
    PHYSDEV emfdev = get_emfdev( dev );
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pRectangle );

    return (emfdev->funcs->pRectangle( emfdev, x1, y1, x2, y2 ) &&
            next->funcs->pRectangle( next, x1, y1, x2, y2 ));
}

/***********************************************************************
 *           emfpathdrv_RoundRect
 */
static BOOL emfpathdrv_RoundRect( PHYSDEV dev, INT x1, INT y1, INT x2, INT y2,
                                  INT ell_width, INT ell_height )
{
    PHYSDEV emfdev = get_emfdev( dev );
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pRoundRect );

    return (emfdev->funcs->pRoundRect( emfdev, x1, y1, x2, y2, ell_width, ell_height ) &&
            next->funcs->pRoundRect( next, x1, y1, x2, y2, ell_width, ell_height ));
}

static const struct gdi_dc_funcs emfpath_driver =
{
    NULL,                               /* pAbortDoc */
    emfpathdrv_AbortPath,               /* pAbortPath */
    NULL,                               /* pAlphaBlend */
    emfpathdrv_AngleArc,                /* pAngleArc */
    emfpathdrv_Arc,                     /* pArc */
    emfpathdrv_ArcTo,                   /* pArcTo */
    emfpathdrv_BeginPath,               /* pBeginPath */
    NULL,                               /* pBlendImage */
    emfpathdrv_Chord,                   /* pChord */
    emfpathdrv_CloseFigure,             /* pCloseFigure */
    NULL,                               /* pCreateCompatibleDC */
    emfpathdrv_CreateDC,                /* pCreateDC */
    emfpathdrv_DeleteDC,                /* pDeleteDC */
    NULL,                               /* pDeleteObject */
    NULL,                               /* pDeviceCapabilities */
    emfpathdrv_Ellipse,                 /* pEllipse */
    NULL,                               /* pEndDoc */
    NULL,                               /* pEndPage */
    emfpathdrv_EndPath,                 /* pEndPath */
    NULL,                               /* pEnumFonts */
    NULL,                               /* pEnumICMProfiles */
    NULL,                               /* pExcludeClipRect */
    NULL,                               /* pExtDeviceMode */
    NULL,                               /* pExtEscape */
    NULL,                               /* pExtFloodFill */
    NULL,                               /* pExtSelectClipRgn */
    emfpathdrv_ExtTextOut,              /* pExtTextOut */
    NULL,                               /* pFillPath */
    NULL,                               /* pFillRgn */
    NULL,                               /* pFlattenPath */
    NULL,                               /* pFontIsLinked */
    NULL,                               /* pFrameRgn */
    NULL,                               /* pGdiComment */
    NULL,                               /* pGetBoundsRect */
    NULL,                               /* pGetCharABCWidths */
    NULL,                               /* pGetCharABCWidthsI */
    NULL,                               /* pGetCharWidth */
#ifdef __REACTOS__
    EMFDRV_GetDeviceCaps, //// Work around HACK.
#else
    NULL,                               /* pGetDeviceCaps */
#endif
    NULL,                               /* pGetDeviceGammaRamp */
    NULL,                               /* pGetFontData */
    NULL,                               /* pGetFontRealizationInfo */
    NULL,                               /* pGetFontUnicodeRanges */
    NULL,                               /* pGetGlyphIndices */
    NULL,                               /* pGetGlyphOutline */
    NULL,                               /* pGetICMProfile */
    NULL,                               /* pGetImage */
    NULL,                               /* pGetKerningPairs */
    NULL,                               /* pGetNearestColor */
    NULL,                               /* pGetOutlineTextMetrics */
    NULL,                               /* pGetPixel */
    NULL,                               /* pGetSystemPaletteEntries */
    NULL,                               /* pGetTextCharsetInfo */
    NULL,                               /* pGetTextExtentExPoint */
    NULL,                               /* pGetTextExtentExPointI */
    NULL,                               /* pGetTextFace */
    NULL,                               /* pGetTextMetrics */
    NULL,                               /* pGradientFill */
    NULL,                               /* pIntersectClipRect */
    NULL,                               /* pInvertRgn */
    emfpathdrv_LineTo,                  /* pLineTo */
    NULL,                               /* pModifyWorldTransform */
    emfpathdrv_MoveTo,                  /* pMoveTo */
    NULL,                               /* pOffsetClipRgn */
    NULL,                               /* pOffsetViewportOrg */
    NULL,                               /* pOffsetWindowOrg */
    NULL,                               /* pPaintRgn */
    NULL,                               /* pPatBlt */
    emfpathdrv_Pie,                     /* pPie */
    emfpathdrv_PolyBezier,              /* pPolyBezier */
    emfpathdrv_PolyBezierTo,            /* pPolyBezierTo */
    emfpathdrv_PolyDraw,                /* pPolyDraw */
    emfpathdrv_PolyPolygon,             /* pPolyPolygon */
    emfpathdrv_PolyPolyline,            /* pPolyPolyline */
    emfpathdrv_Polygon,                 /* pPolygon */
    emfpathdrv_Polyline,                /* pPolyline */
    emfpathdrv_PolylineTo,              /* pPolylineTo */
    NULL,                               /* pPutImage */
    NULL,                               /* pRealizeDefaultPalette */
    NULL,                               /* pRealizePalette */
    emfpathdrv_Rectangle,               /* pRectangle */
    NULL,                               /* pResetDC */
    NULL,                               /* pRestoreDC */
    emfpathdrv_RoundRect,               /* pRoundRect */
    NULL,                               /* pSaveDC */
    NULL,                               /* pScaleViewportExt */
    NULL,                               /* pScaleWindowExt */
    NULL,                               /* pSelectBitmap */
    NULL,                               /* pSelectBrush */
    NULL,                               /* pSelectClipPath */
    NULL,                               /* pSelectFont */
    NULL,                               /* pSelectPalette */
    NULL,                               /* pSelectPen */
    NULL,                               /* pSetArcDirection */
    NULL,                               /* pSetBkColor */
    NULL,                               /* pSetBkMode */
    NULL,                               /* pSetDCBrushColor */
    NULL,                               /* pSetDCPenColor */
    NULL,                               /* pSetDIBColorTable */
    NULL,                               /* pSetDIBitsToDevice */
    NULL,                               /* pSetDeviceClipping */
    NULL,                               /* pSetDeviceGammaRamp */
    NULL,                               /* pSetLayout */
    NULL,                               /* pSetMapMode */
    NULL,                               /* pSetMapperFlags */
    NULL,                               /* pSetPixel */
    NULL,                               /* pSetPolyFillMode */
    NULL,                               /* pSetROP2 */
    NULL,                               /* pSetRelAbs */
    NULL,                               /* pSetStretchBltMode */
    NULL,                               /* pSetTextAlign */
    NULL,                               /* pSetTextCharacterExtra */
    NULL,                               /* pSetTextColor */
    NULL,                               /* pSetTextJustification */
    NULL,                               /* pSetViewportExt */
    NULL,                               /* pSetViewportOrg */
    NULL,                               /* pSetWindowExt */
    NULL,                               /* pSetWindowOrg */
    NULL,                               /* pSetWorldTransform */
    NULL,                               /* pStartDoc */
    NULL,                               /* pStartPage */
    NULL,                               /* pStretchBlt */
    NULL,                               /* pStretchDIBits */
    NULL,                               /* pStrokeAndFillPath */
    NULL,                               /* pStrokePath */
    NULL,                               /* pUnrealizePalette */
    NULL,                               /* pWidenPath */
    NULL,                               /* wine_get_wgl_driver */
    GDI_PRIORITY_PATH_DRV + 1           /* priority */
};
