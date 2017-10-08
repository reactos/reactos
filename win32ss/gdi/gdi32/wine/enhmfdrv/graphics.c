/*
 * Enhanced MetaFile driver graphics functions
 *
 * Copyright 1999 Huw D M Davies
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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "enhmfdrv/enhmetafiledrv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(enhmetafile);

static const RECTL empty_bounds = { 0, 0, -1, -1 };

/* determine if we can use 16-bit points to store all the input points */
static BOOL can_use_short_points( const POINT *pts, UINT count )
{
    UINT i;

    for (i = 0; i < count; i++)
        if (((pts[i].x + 0x8000) & ~0xffff) || ((pts[i].y + 0x8000) & ~0xffff))
            return FALSE;
    return TRUE;
}

/* store points in either long or short format; return a pointer to the end of the stored data */
static void *store_points( POINTL *dest, const POINT *pts, UINT count, BOOL short_points )
{
    if (short_points)
    {
        UINT i;
        POINTS *dest_short = (POINTS *)dest;

        for (i = 0; i < count; i++)
        {
            dest_short[i].x = pts[i].x;
            dest_short[i].y = pts[i].y;
        }
        return dest_short + count;
    }
    else
    {
        memcpy( dest, pts, count * sizeof(*dest) );
        return dest + count;
    }
}

/* compute the bounds of an array of points, optionally including the current position */
#ifdef __REACTOS__
static void get_points_bounds( RECTL *bounds, const POINT *pts, UINT count, HDC hdc )
#else
static void get_points_bounds( RECTL *bounds, const POINT *pts, UINT count, DC *dc )
#endif
{
    UINT i;
#ifdef __REACTOS__
    if (hdc)
    {
        POINT cur_pt;
        GetCurrentPositionEx( hdc, &cur_pt );
        bounds->left = bounds->right = cur_pt.x;
        bounds->top = bounds->bottom = cur_pt.y;
    }
#else
    if (dc)
    {
        bounds->left = bounds->right = dc->cur_pos.x;
        bounds->top = bounds->bottom = dc->cur_pos.y;
    }
#endif
    else if (count)
    {
        bounds->left = bounds->right = pts[0].x;
        bounds->top = bounds->bottom = pts[0].y;
    }
    else *bounds = empty_bounds;

    for (i = 0; i < count; i++)
    {
        bounds->left   = min( bounds->left, pts[i].x );
        bounds->right  = max( bounds->right, pts[i].x );
        bounds->top    = min( bounds->top, pts[i].y );
        bounds->bottom = max( bounds->bottom, pts[i].y );
    }
}

/* helper for path stroke and fill functions */
#ifdef __REACTOS__
static BOOL emfdrv_stroke_and_fill_path( PHYSDEV dev, INT type )
{
    EMRSTROKEANDFILLPATH emr;
    LPPOINT Points;
    LPBYTE Types;
    INT nSize;

    emr.emr.iType = type;
    emr.emr.nSize = sizeof(emr);

    nSize = GetPath(dev->hdc, NULL, NULL, 0);
    if (nSize != -1)
    {
       Points = HeapAlloc( GetProcessHeap(), 0, nSize*sizeof(POINT) );
       Types  = HeapAlloc( GetProcessHeap(), 0, nSize*sizeof(BYTE) );

       GetPath(dev->hdc, Points, Types, nSize);
       get_points_bounds( &emr.rclBounds, Points, nSize, 0 );

       HeapFree( GetProcessHeap(), 0, Points );
       HeapFree( GetProcessHeap(), 0, Types );

       TRACE("GetBounds l %d t %d r %d b %d\n",emr.rclBounds.left, emr.rclBounds.top, emr.rclBounds.right, emr.rclBounds.bottom);
    }
    else emr.rclBounds = empty_bounds;

    if (!EMFDRV_WriteRecord( dev, &emr.emr )) return FALSE;
    if (nSize == -1 ) return FALSE;
    EMFDRV_UpdateBBox( dev, &emr.rclBounds );
    return TRUE;
}
#else
static BOOL emfdrv_stroke_and_fill_path( PHYSDEV dev, INT type )
{
    DC *dc = get_physdev_dc( dev );
    EMRSTROKEANDFILLPATH emr;
    struct gdi_path *path;
    POINT *points;
    BYTE *flags;

    emr.emr.iType = type;
    emr.emr.nSize = sizeof(emr);

    if ((path = get_gdi_flat_path( dc, NULL )))
    {
        int count = get_gdi_path_data( path, &points, &flags );
        get_points_bounds( &emr.rclBounds, points, count, 0 );
        free_gdi_path( path );
    }
    else emr.rclBounds = empty_bounds;

    if (!EMFDRV_WriteRecord( dev, &emr.emr )) return FALSE;
    if (!path) return FALSE;
    EMFDRV_UpdateBBox( dev, &emr.rclBounds );
    return TRUE;
}
#endif

/**********************************************************************
 *	     EMFDRV_MoveTo
 */
BOOL EMFDRV_MoveTo(PHYSDEV dev, INT x, INT y)
{
    EMRMOVETOEX emr;

    emr.emr.iType = EMR_MOVETOEX;
    emr.emr.nSize = sizeof(emr);
    emr.ptl.x = x;
    emr.ptl.y = y;

    return EMFDRV_WriteRecord( dev, &emr.emr );
}

/***********************************************************************
 *           EMFDRV_LineTo
 */
BOOL EMFDRV_LineTo( PHYSDEV dev, INT x, INT y )
{
    EMFDRV_PDEVICE *physDev = get_emf_physdev( dev );
#ifndef __REACTOS__
    DC *dc = get_physdev_dc( dev );
#endif
    POINT pt;
    EMRLINETO emr;
    RECTL bounds;

    emr.emr.iType = EMR_LINETO;
    emr.emr.nSize = sizeof(emr);
    emr.ptl.x = x;
    emr.ptl.y = y;

    if(!EMFDRV_WriteRecord( dev, &emr.emr ))
    	return FALSE;
#ifdef __REACTOS__
    GetCurrentPositionEx( dev->hdc, &pt );
#else
    pt = dc->cur_pos;
#endif
    bounds.left   = min(x, pt.x);
    bounds.top    = min(y, pt.y);
    bounds.right  = max(x, pt.x);
    bounds.bottom = max(y, pt.y);

    if(!physDev->path)
        EMFDRV_UpdateBBox( dev, &bounds );

    return TRUE;
}


/***********************************************************************
 *           EMFDRV_ArcChordPie
 */
static BOOL
EMFDRV_ArcChordPie( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
		    INT xstart, INT ystart, INT xend, INT yend, DWORD iType )
{
    EMFDRV_PDEVICE *physDev = get_emf_physdev( dev );
#ifndef __REACTOS__
    DC *dc = get_physdev_dc( dev );
#endif
    INT temp, xCentre, yCentre, i;
    double angleStart, angleEnd;
    double xinterStart, yinterStart, xinterEnd, yinterEnd;
    EMRARC emr;
    RECTL bounds;

    if(left == right || top == bottom) return FALSE;

    if(left > right) {temp = left; left = right; right = temp;}
    if(top > bottom) {temp = top; top = bottom; bottom = temp;}
#ifdef __REACTOS__
    if(GetGraphicsMode(dev->hdc) == GM_COMPATIBLE) {
#else
    if(dc->GraphicsMode == GM_COMPATIBLE) {
#endif
        right--;
	bottom--;
    }

    emr.emr.iType     = iType;
    emr.emr.nSize     = sizeof(emr);
    emr.rclBox.left   = left;
    emr.rclBox.top    = top;
    emr.rclBox.right  = right;
    emr.rclBox.bottom = bottom;
    emr.ptlStart.x    = xstart;
    emr.ptlStart.y    = ystart;
    emr.ptlEnd.x      = xend;
    emr.ptlEnd.y      = yend;


    /* Now calculate the BBox */
    xCentre = (left + right + 1) / 2;
    yCentre = (top + bottom + 1) / 2;

    xstart -= xCentre;
    ystart -= yCentre;
    xend   -= xCentre;
    yend   -= yCentre;

    /* invert y co-ords to get angle anti-clockwise from x-axis */
    angleStart = atan2( -(double)ystart, (double)xstart);
    angleEnd   = atan2( -(double)yend, (double)xend);

    /* These are the intercepts of the start/end lines with the arc */

    xinterStart = (right - left + 1)/2 * cos(angleStart) + xCentre;
    yinterStart = -(bottom - top + 1)/2 * sin(angleStart) + yCentre;
    xinterEnd   = (right - left + 1)/2 * cos(angleEnd) + xCentre;
    yinterEnd   = -(bottom - top + 1)/2 * sin(angleEnd) + yCentre;

    if(angleStart < 0) angleStart += 2 * M_PI;
    if(angleEnd < 0) angleEnd += 2 * M_PI;
    if(angleEnd < angleStart) angleEnd += 2 * M_PI;

    bounds.left   = min(xinterStart, xinterEnd);
    bounds.top    = min(yinterStart, yinterEnd);
    bounds.right  = max(xinterStart, xinterEnd);
    bounds.bottom = max(yinterStart, yinterEnd);

    for(i = 0; i <= 8; i++) {
        if(i * M_PI / 2 < angleStart) /* loop until we're past start */
	    continue;
	if(i * M_PI / 2 > angleEnd)   /* if we're past end we're finished */
	    break;

	/* the arc touches the rectangle at the start of quadrant i, so adjust
	   BBox to reflect this. */

	switch(i % 4) {
	case 0:
	    bounds.right = right;
	    break;
	case 1:
	    bounds.top = top;
	    break;
	case 2:
	    bounds.left = left;
	    break;
	case 3:
	    bounds.bottom = bottom;
	    break;
	}
    }

    /* If we're drawing a pie then make sure we include the centre */
    if(iType == EMR_PIE) {
        if(bounds.left > xCentre) bounds.left = xCentre;
	else if(bounds.right < xCentre) bounds.right = xCentre;
	if(bounds.top > yCentre) bounds.top = yCentre;
	else if(bounds.bottom < yCentre) bounds.bottom = yCentre;
    }
    if (iType == EMR_ARCTO)
    {
        POINT pt;
#ifdef __REACTOS__
        GetCurrentPositionEx( dev->hdc, &pt );
#else
        pt = dc->cur_pos;
#endif
        bounds.left   = min( bounds.left, pt.x );
        bounds.top    = min( bounds.top, pt.y );
        bounds.right  = max( bounds.right, pt.x );
        bounds.bottom = max( bounds.bottom, pt.y );
    }
    if(!EMFDRV_WriteRecord( dev, &emr.emr ))
        return FALSE;
    if(!physDev->path)
        EMFDRV_UpdateBBox( dev, &bounds );
    return TRUE;
}


/***********************************************************************
 *           EMFDRV_Arc
 */
BOOL EMFDRV_Arc( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                 INT xstart, INT ystart, INT xend, INT yend )
{
    return EMFDRV_ArcChordPie( dev, left, top, right, bottom, xstart, ystart,
			       xend, yend, EMR_ARC );
}

/***********************************************************************
 *           EMFDRV_ArcTo
 */
BOOL EMFDRV_ArcTo( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                   INT xstart, INT ystart, INT xend, INT yend )
{
    return EMFDRV_ArcChordPie( dev, left, top, right, bottom, xstart, ystart,
			       xend, yend, EMR_ARCTO );
}

/***********************************************************************
 *           EMFDRV_Pie
 */
BOOL EMFDRV_Pie( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                 INT xstart, INT ystart, INT xend, INT yend )
{
    return EMFDRV_ArcChordPie( dev, left, top, right, bottom, xstart, ystart,
			       xend, yend, EMR_PIE );
}


/***********************************************************************
 *           EMFDRV_Chord
 */
BOOL EMFDRV_Chord( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                   INT xstart, INT ystart, INT xend, INT yend )
{
    return EMFDRV_ArcChordPie( dev, left, top, right, bottom, xstart, ystart,
			       xend, yend, EMR_CHORD );
}

/***********************************************************************
 *           EMFDRV_AngleArc
 */
BOOL EMFDRV_AngleArc( PHYSDEV dev, INT x, INT y, DWORD radius, FLOAT start, FLOAT sweep )
{
    EMRANGLEARC emr;

    emr.emr.iType   = EMR_ANGLEARC;
    emr.emr.nSize   = sizeof( emr );
    emr.ptlCenter.x = x;
    emr.ptlCenter.y = y;
    emr.nRadius     = radius;
    emr.eStartAngle = start;
    emr.eSweepAngle = sweep;

    return EMFDRV_WriteRecord( dev, &emr.emr );
}

/***********************************************************************
 *           EMFDRV_Ellipse
 */
BOOL EMFDRV_Ellipse( PHYSDEV dev, INT left, INT top, INT right, INT bottom )
{
    EMFDRV_PDEVICE *physDev = get_emf_physdev( dev );
#ifndef __REACTOS__
    DC *dc = get_physdev_dc( dev );
#endif
    EMRELLIPSE emr;
    INT temp;

    TRACE("%d,%d - %d,%d\n", left, top, right, bottom);

    if(left == right || top == bottom) return FALSE;

    if(left > right) {temp = left; left = right; right = temp;}
    if(top > bottom) {temp = top; top = bottom; bottom = temp;}
#ifdef __REACTOS__
    if(GetGraphicsMode(dev->hdc) == GM_COMPATIBLE) {
#else
    if(dc->GraphicsMode == GM_COMPATIBLE) {
#endif
        right--;
	bottom--;
    }

    emr.emr.iType     = EMR_ELLIPSE;
    emr.emr.nSize     = sizeof(emr);
    emr.rclBox.left   = left;
    emr.rclBox.top    = top;
    emr.rclBox.right  = right;
    emr.rclBox.bottom = bottom;

    if(!physDev->path)
        EMFDRV_UpdateBBox( dev, &emr.rclBox );
    return EMFDRV_WriteRecord( dev, &emr.emr );
}

/***********************************************************************
 *           EMFDRV_Rectangle
 */
BOOL EMFDRV_Rectangle(PHYSDEV dev, INT left, INT top, INT right, INT bottom)
{
    EMFDRV_PDEVICE *physDev = get_emf_physdev( dev );
#ifndef __REACTOS__
    DC *dc = get_physdev_dc( dev );
#endif
    EMRRECTANGLE emr;
    INT temp;

    TRACE("%d,%d - %d,%d\n", left, top, right, bottom);

    if(left == right || top == bottom) return FALSE;

    if(left > right) {temp = left; left = right; right = temp;}
    if(top > bottom) {temp = top; top = bottom; bottom = temp;}
#ifdef __REACTOS__
    if(GetGraphicsMode(dev->hdc) == GM_COMPATIBLE) {
#else   
    if(dc->GraphicsMode == GM_COMPATIBLE) {
#endif
        right--;
	bottom--;
    }

    emr.emr.iType     = EMR_RECTANGLE;
    emr.emr.nSize     = sizeof(emr);
    emr.rclBox.left   = left;
    emr.rclBox.top    = top;
    emr.rclBox.right  = right;
    emr.rclBox.bottom = bottom;

    if(!physDev->path)
        EMFDRV_UpdateBBox( dev, &emr.rclBox );
    return EMFDRV_WriteRecord( dev, &emr.emr );
}

/***********************************************************************
 *           EMFDRV_RoundRect
 */
BOOL EMFDRV_RoundRect( PHYSDEV dev, INT left, INT top, INT right,
		  INT bottom, INT ell_width, INT ell_height )
{
    EMFDRV_PDEVICE *physDev = get_emf_physdev( dev );
#ifndef __REACTOS__
    DC *dc = get_physdev_dc( dev );
#endif
    EMRROUNDRECT emr;
    INT temp;

    if(left == right || top == bottom) return FALSE;

    if(left > right) {temp = left; left = right; right = temp;}
    if(top > bottom) {temp = top; top = bottom; bottom = temp;}
#ifdef __REACTOS__
    if(GetGraphicsMode(dev->hdc) == GM_COMPATIBLE) {
#else
    if(dc->GraphicsMode == GM_COMPATIBLE) {
#endif
        right--;
	bottom--;
    }

    emr.emr.iType     = EMR_ROUNDRECT;
    emr.emr.nSize     = sizeof(emr);
    emr.rclBox.left   = left;
    emr.rclBox.top    = top;
    emr.rclBox.right  = right;
    emr.rclBox.bottom = bottom;
    emr.szlCorner.cx  = ell_width;
    emr.szlCorner.cy  = ell_height;

    if(!physDev->path)
        EMFDRV_UpdateBBox( dev, &emr.rclBox );
    return EMFDRV_WriteRecord( dev, &emr.emr );
}

/***********************************************************************
 *           EMFDRV_SetPixel
 */
COLORREF EMFDRV_SetPixel( PHYSDEV dev, INT x, INT y, COLORREF color )
{
    EMRSETPIXELV emr;

    emr.emr.iType  = EMR_SETPIXELV;
    emr.emr.nSize  = sizeof(emr);
    emr.ptlPixel.x = x;
    emr.ptlPixel.y = y;
    emr.crColor = color;

    if (EMFDRV_WriteRecord( dev, &emr.emr )) {
        RECTL bounds;
        bounds.left = bounds.right = x;
        bounds.top = bounds.bottom = y;
        EMFDRV_UpdateBBox( dev, &bounds );
        return color;
    }
    return -1;
}

/**********************************************************************
 *          EMFDRV_Polylinegon
 *
 * Helper for EMFDRV_Poly{line|gon}
 */
static BOOL
EMFDRV_Polylinegon( PHYSDEV dev, const POINT* pt, INT count, DWORD iType )
{
    EMFDRV_PDEVICE *physDev = get_emf_physdev( dev );
#ifndef __REACTOS__
    DC *dc = get_physdev_dc( dev );
#endif
    EMRPOLYLINE *emr;
    DWORD size;
    BOOL ret, use_small_emr = can_use_short_points( pt, count );

    size = use_small_emr ? offsetof( EMRPOLYLINE16, apts[count] ) : offsetof( EMRPOLYLINE, aptl[count] );

    emr = HeapAlloc( GetProcessHeap(), 0, size );
    emr->emr.iType = use_small_emr ? iType + EMR_POLYLINE16 - EMR_POLYLINE : iType;
    emr->emr.nSize = size;
    emr->cptl = count;

    store_points( emr->aptl, pt, count, use_small_emr );

    if (!physDev->path)
        get_points_bounds( &emr->rclBounds, pt, count,
#ifdef __REACTOS__
                           (iType == EMR_POLYBEZIERTO || iType == EMR_POLYLINETO) ? dev->hdc : 0 );
#else
                           (iType == EMR_POLYBEZIERTO || iType == EMR_POLYLINETO) ? dc : 0 );
#endif
    else
        emr->rclBounds = empty_bounds;

    ret = EMFDRV_WriteRecord( dev, &emr->emr );
    if (ret && !physDev->path)
        EMFDRV_UpdateBBox( dev, &emr->rclBounds );
    HeapFree( GetProcessHeap(), 0, emr );
    return ret;
}


/**********************************************************************
 *          EMFDRV_Polyline
 */
BOOL EMFDRV_Polyline( PHYSDEV dev, const POINT* pt, INT count )
{
    return EMFDRV_Polylinegon( dev, pt, count, EMR_POLYLINE );
}

/**********************************************************************
 *          EMFDRV_PolylineTo
 */
BOOL EMFDRV_PolylineTo( PHYSDEV dev, const POINT* pt, INT count )
{
    return EMFDRV_Polylinegon( dev, pt, count, EMR_POLYLINETO );
}

/**********************************************************************
 *          EMFDRV_Polygon
 */
BOOL EMFDRV_Polygon( PHYSDEV dev, const POINT* pt, INT count )
{
    if(count < 2) return FALSE;
    return EMFDRV_Polylinegon( dev, pt, count, EMR_POLYGON );
}

/**********************************************************************
 *          EMFDRV_PolyBezier
 */
BOOL EMFDRV_PolyBezier( PHYSDEV dev, const POINT *pts, DWORD count )
{
    return EMFDRV_Polylinegon( dev, pts, count, EMR_POLYBEZIER );
}

/**********************************************************************
 *          EMFDRV_PolyBezierTo
 */
BOOL EMFDRV_PolyBezierTo( PHYSDEV dev, const POINT *pts, DWORD count )
{
    return EMFDRV_Polylinegon( dev, pts, count, EMR_POLYBEZIERTO );
}


/**********************************************************************
 *          EMFDRV_PolyPolylinegon
 *
 * Helper for EMFDRV_PolyPoly{line|gon}
 */
static BOOL
EMFDRV_PolyPolylinegon( PHYSDEV dev, const POINT* pt, const INT* counts, UINT polys,
			DWORD iType)
{
    EMFDRV_PDEVICE *physDev = get_emf_physdev( dev );
    EMRPOLYPOLYLINE *emr;
    DWORD cptl = 0, poly, size;
    BOOL ret, use_small_emr, bounds_valid = TRUE;

    for(poly = 0; poly < polys; poly++) {
        cptl += counts[poly];
        if(counts[poly] < 2) bounds_valid = FALSE;
    }
    if(!cptl) bounds_valid = FALSE;
    use_small_emr = can_use_short_points( pt, cptl );

    size = FIELD_OFFSET(EMRPOLYPOLYLINE, aPolyCounts[polys]);
    if(use_small_emr)
        size += cptl * sizeof(POINTS);
    else
        size += cptl * sizeof(POINTL);

    emr = HeapAlloc( GetProcessHeap(), 0, size );

    emr->emr.iType = iType;
    if(use_small_emr) emr->emr.iType += EMR_POLYPOLYLINE16 - EMR_POLYPOLYLINE;

    emr->emr.nSize = size;
    if(bounds_valid && !physDev->path)
        get_points_bounds( &emr->rclBounds, pt, cptl, 0 );
    else
        emr->rclBounds = empty_bounds;
    emr->nPolys = polys;
    emr->cptl = cptl;

    if(polys)
    {
        memcpy( emr->aPolyCounts, counts, polys * sizeof(DWORD) );
        store_points( (POINTL *)(emr->aPolyCounts + polys), pt, cptl, use_small_emr );
    }

    ret = EMFDRV_WriteRecord( dev, &emr->emr );
    if(ret && !bounds_valid)
    {
        ret = FALSE;
        SetLastError( ERROR_INVALID_PARAMETER );
    }
    if(ret && !physDev->path)
        EMFDRV_UpdateBBox( dev, &emr->rclBounds );
    HeapFree( GetProcessHeap(), 0, emr );
    return ret;
}

/**********************************************************************
 *          EMFDRV_PolyPolyline
 */
BOOL EMFDRV_PolyPolyline(PHYSDEV dev, const POINT* pt, const DWORD* counts, DWORD polys)
{
    return EMFDRV_PolyPolylinegon( dev, pt, (const INT *)counts, polys,
				   EMR_POLYPOLYLINE );
}

/**********************************************************************
 *          EMFDRV_PolyPolygon
 */
BOOL EMFDRV_PolyPolygon( PHYSDEV dev, const POINT* pt, const INT* counts, UINT polys )
{
    return EMFDRV_PolyPolylinegon( dev, pt, counts, polys, EMR_POLYPOLYGON );
}


/**********************************************************************
 *          EMFDRV_PolyDraw
 */
BOOL EMFDRV_PolyDraw( PHYSDEV dev, const POINT *pts, const BYTE *types, DWORD count )
{
    EMFDRV_PDEVICE *physDev = get_emf_physdev( dev );
    EMRPOLYDRAW *emr;
    BOOL ret;
    BYTE *types_dest;
    BOOL use_small_emr = can_use_short_points( pts, count );
    DWORD size;

    size = use_small_emr ? offsetof( EMRPOLYDRAW16, apts[count] ) : offsetof( EMRPOLYDRAW, aptl[count] );
    size += (count + 3) & ~3;

    if (!(emr = HeapAlloc( GetProcessHeap(), 0, size ))) return FALSE;

    emr->emr.iType = use_small_emr ? EMR_POLYDRAW16 : EMR_POLYDRAW;
    emr->emr.nSize = size;
    emr->cptl = count;

    types_dest = store_points( emr->aptl, pts, count, use_small_emr );
    memcpy( types_dest, types, count );
    if (count & 3) memset( types_dest + count, 0, 4 - (count & 3) );

    if (!physDev->path)
        get_points_bounds( &emr->rclBounds, pts, count, 0 );
    else
        emr->rclBounds = empty_bounds;

    ret = EMFDRV_WriteRecord( dev, &emr->emr );
    if (ret && !physDev->path) EMFDRV_UpdateBBox( dev, &emr->rclBounds );
    HeapFree( GetProcessHeap(), 0, emr );
    return ret;
}


/**********************************************************************
 *          EMFDRV_ExtFloodFill
 */
BOOL EMFDRV_ExtFloodFill( PHYSDEV dev, INT x, INT y, COLORREF color, UINT fillType )
{
    EMREXTFLOODFILL emr;

    emr.emr.iType = EMR_EXTFLOODFILL;
    emr.emr.nSize = sizeof(emr);
    emr.ptlStart.x = x;
    emr.ptlStart.y = y;
    emr.crColor = color;
    emr.iMode = fillType;

    return EMFDRV_WriteRecord( dev, &emr.emr );
}


/*********************************************************************
 *          EMFDRV_FillRgn
 */
BOOL EMFDRV_FillRgn( PHYSDEV dev, HRGN hrgn, HBRUSH hbrush )
{
    EMRFILLRGN *emr;
    DWORD size, rgnsize, index;
    BOOL ret;

    index = EMFDRV_CreateBrushIndirect( dev, hbrush );
    if(!index) return FALSE;

    rgnsize = GetRegionData( hrgn, 0, NULL );
    size = rgnsize + offsetof(EMRFILLRGN,RgnData);
    emr = HeapAlloc( GetProcessHeap(), 0, size );

    GetRegionData( hrgn, rgnsize, (RGNDATA *)&emr->RgnData );

    emr->emr.iType = EMR_FILLRGN;
    emr->emr.nSize = size;
    emr->rclBounds.left   = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.left;
    emr->rclBounds.top    = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.top;
    emr->rclBounds.right  = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.right - 1;
    emr->rclBounds.bottom = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.bottom - 1;
    emr->cbRgnData = rgnsize;
    emr->ihBrush = index;

    ret = EMFDRV_WriteRecord( dev, &emr->emr );
    if(ret)
        EMFDRV_UpdateBBox( dev, &emr->rclBounds );
    HeapFree( GetProcessHeap(), 0, emr );
    return ret;
}
/*********************************************************************
 *          EMFDRV_FrameRgn
 */
BOOL EMFDRV_FrameRgn( PHYSDEV dev, HRGN hrgn, HBRUSH hbrush, INT width, INT height )
{
    EMRFRAMERGN *emr;
    DWORD size, rgnsize, index;
    BOOL ret;

    index = EMFDRV_CreateBrushIndirect( dev, hbrush );
    if(!index) return FALSE;

    rgnsize = GetRegionData( hrgn, 0, NULL );
    size = rgnsize + offsetof(EMRFRAMERGN,RgnData);
    emr = HeapAlloc( GetProcessHeap(), 0, size );

    GetRegionData( hrgn, rgnsize, (RGNDATA *)&emr->RgnData );

    emr->emr.iType = EMR_FRAMERGN;
    emr->emr.nSize = size;
    emr->rclBounds.left   = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.left;
    emr->rclBounds.top    = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.top;
    emr->rclBounds.right  = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.right - 1;
    emr->rclBounds.bottom = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.bottom - 1;
    emr->cbRgnData = rgnsize;
    emr->ihBrush = index;
    emr->szlStroke.cx = width;
    emr->szlStroke.cy = height;

    ret = EMFDRV_WriteRecord( dev, &emr->emr );
    if(ret)
        EMFDRV_UpdateBBox( dev, &emr->rclBounds );
    HeapFree( GetProcessHeap(), 0, emr );
    return ret;
}

/*********************************************************************
 *          EMFDRV_PaintInvertRgn
 *
 * Helper for EMFDRV_{Paint|Invert}Rgn
 */
static BOOL EMFDRV_PaintInvertRgn( PHYSDEV dev, HRGN hrgn, DWORD iType )
{
    EMRINVERTRGN *emr;
    DWORD size, rgnsize;
    BOOL ret;


    rgnsize = GetRegionData( hrgn, 0, NULL );
    size = rgnsize + offsetof(EMRINVERTRGN,RgnData);
    emr = HeapAlloc( GetProcessHeap(), 0, size );

    GetRegionData( hrgn, rgnsize, (RGNDATA *)&emr->RgnData );

    emr->emr.iType = iType;
    emr->emr.nSize = size;
    emr->rclBounds.left   = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.left;
    emr->rclBounds.top    = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.top;
    emr->rclBounds.right  = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.right - 1;
    emr->rclBounds.bottom = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.bottom - 1;
    emr->cbRgnData = rgnsize;

    ret = EMFDRV_WriteRecord( dev, &emr->emr );
    if(ret)
        EMFDRV_UpdateBBox( dev, &emr->rclBounds );
    HeapFree( GetProcessHeap(), 0, emr );
    return ret;
}

/**********************************************************************
 *          EMFDRV_PaintRgn
 */
BOOL EMFDRV_PaintRgn( PHYSDEV dev, HRGN hrgn )
{
    return EMFDRV_PaintInvertRgn( dev, hrgn, EMR_PAINTRGN );
}

/**********************************************************************
 *          EMFDRV_InvertRgn
 */
BOOL EMFDRV_InvertRgn( PHYSDEV dev, HRGN hrgn )
{
    return EMFDRV_PaintInvertRgn( dev, hrgn, EMR_INVERTRGN );
}

/**********************************************************************
 *          EMFDRV_ExtTextOut
 */
BOOL EMFDRV_ExtTextOut( PHYSDEV dev, INT x, INT y, UINT flags, const RECT *lprect,
                        LPCWSTR str, UINT count, const INT *lpDx )
{
    EMFDRV_PDEVICE *physDev = get_emf_physdev( dev );
#ifndef __REACTOS__
    DC *dc = get_physdev_dc( dev );
#endif
    EMREXTTEXTOUTW *pemr;
    DWORD nSize;
    BOOL ret;
    int textHeight = 0;
    int textWidth = 0;
#ifdef __REACTOS__
    const UINT textAlign = GetTextAlign( dev->hdc );
    const INT graphicsMode = GetGraphicsMode( dev->hdc );
#else
    const UINT textAlign = dc->textAlign;
    const INT graphicsMode = dc->GraphicsMode;
#endif
    FLOAT exScale, eyScale;

    nSize = sizeof(*pemr) + ((count+1) & ~1) * sizeof(WCHAR) + count * sizeof(INT);

    TRACE("%s %s count %d nSize = %d\n", debugstr_wn(str, count),
           wine_dbgstr_rect(lprect), count, nSize);
    pemr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nSize);

    if (graphicsMode == GM_COMPATIBLE)
    {
        const INT horzSize = GetDeviceCaps( dev->hdc, HORZSIZE );
        const INT horzRes  = GetDeviceCaps( dev->hdc, HORZRES );
        const INT vertSize = GetDeviceCaps( dev->hdc, VERTSIZE );
        const INT vertRes  = GetDeviceCaps( dev->hdc, VERTRES );
        SIZE wndext, vportext;

        GetViewportExtEx( dev->hdc, &vportext );
        GetWindowExtEx( dev->hdc, &wndext );
        exScale = 100.0 * ((FLOAT)horzSize  / (FLOAT)horzRes) /
                          ((FLOAT)wndext.cx / (FLOAT)vportext.cx);
        eyScale = 100.0 * ((FLOAT)vertSize  / (FLOAT)vertRes) /
                          ((FLOAT)wndext.cy / (FLOAT)vportext.cy);
    }
    else
    {
        exScale = 0.0;
        eyScale = 0.0;
    }

    pemr->emr.iType = EMR_EXTTEXTOUTW;
    pemr->emr.nSize = nSize;
    pemr->iGraphicsMode = graphicsMode;
    pemr->exScale = exScale;
    pemr->eyScale = eyScale;
    pemr->emrtext.ptlReference.x = x;
    pemr->emrtext.ptlReference.y = y;
    pemr->emrtext.nChars = count;
    pemr->emrtext.offString = sizeof(*pemr);
    memcpy((char*)pemr + pemr->emrtext.offString, str, count * sizeof(WCHAR));
    pemr->emrtext.fOptions = flags;
    if(!lprect) {
        pemr->emrtext.rcl.left = pemr->emrtext.rcl.top = 0;
        pemr->emrtext.rcl.right = pemr->emrtext.rcl.bottom = -1;
    } else {
        pemr->emrtext.rcl.left = lprect->left;
        pemr->emrtext.rcl.top = lprect->top;
        pemr->emrtext.rcl.right = lprect->right;
        pemr->emrtext.rcl.bottom = lprect->bottom;
    }

    pemr->emrtext.offDx = pemr->emrtext.offString + ((count+1) & ~1) * sizeof(WCHAR);
    if(lpDx) {
        UINT i;
        SIZE strSize;
        memcpy((char*)pemr + pemr->emrtext.offDx, lpDx, count * sizeof(INT));
        for (i = 0; i < count; i++) {
            textWidth += lpDx[i];
        }
        if (GetTextExtentPoint32W( dev->hdc, str, count, &strSize ))
            textHeight = strSize.cy;
    }
    else {
        UINT i;
        INT *dx = (INT *)((char*)pemr + pemr->emrtext.offDx);
        SIZE charSize;
        for (i = 0; i < count; i++) {
            if (GetTextExtentPoint32W( dev->hdc, str + i, 1, &charSize )) {
                dx[i] = charSize.cx;
                textWidth += charSize.cx;
                textHeight = max(textHeight, charSize.cy);
            }
        }
    }

    if (physDev->path)
    {
        pemr->rclBounds.left = pemr->rclBounds.top = 0;
        pemr->rclBounds.right = pemr->rclBounds.bottom = -1;
        goto no_bounds;
    }

    /* FIXME: handle font escapement */
    switch (textAlign & (TA_LEFT | TA_RIGHT | TA_CENTER)) {
    case TA_CENTER: {
        pemr->rclBounds.left  = x - (textWidth / 2) - 1;
        pemr->rclBounds.right = x + (textWidth / 2) + 1;
        break;
    }
    case TA_RIGHT: {
        pemr->rclBounds.left  = x - textWidth - 1;
        pemr->rclBounds.right = x;
        break;
    }
    default: { /* TA_LEFT */
        pemr->rclBounds.left  = x;
        pemr->rclBounds.right = x + textWidth + 1;
    }
    }

    switch (textAlign & (TA_TOP | TA_BOTTOM | TA_BASELINE)) {
    case TA_BASELINE: {
        TEXTMETRICW tm;
        if (!GetTextMetricsW( dev->hdc, &tm ))
            tm.tmDescent = 0;
        /* Play safe here... it's better to have a bounding box */
        /* that is too big than too small. */
        pemr->rclBounds.top    = y - textHeight - 1;
        pemr->rclBounds.bottom = y + tm.tmDescent + 1;
        break;
    }
    case TA_BOTTOM: {
        pemr->rclBounds.top    = y - textHeight - 1;
        pemr->rclBounds.bottom = y;
        break;
    }
    default: { /* TA_TOP */
        pemr->rclBounds.top    = y;
        pemr->rclBounds.bottom = y + textHeight + 1;
    }
    }
    EMFDRV_UpdateBBox( dev, &pemr->rclBounds );

no_bounds:
    ret = EMFDRV_WriteRecord( dev, &pemr->emr );
    HeapFree( GetProcessHeap(), 0, pemr );
    return ret;
}

/**********************************************************************
 *          EMFDRV_GradientFill
 */
BOOL EMFDRV_GradientFill( PHYSDEV dev, TRIVERTEX *vert_array, ULONG nvert,
                          void *grad_array, ULONG ngrad, ULONG mode )
{
    EMRGRADIENTFILL *emr;
    ULONG i, pt, size, num_pts = ngrad * (mode == GRADIENT_FILL_TRIANGLE ? 3 : 2);
    const ULONG *pts = (const ULONG *)grad_array;
    BOOL ret;

    size = FIELD_OFFSET(EMRGRADIENTFILL, Ver[nvert]) + num_pts * sizeof(pts[0]);

    emr = HeapAlloc( GetProcessHeap(), 0, size );
    if (!emr) return FALSE;

    for (i = 0; i < num_pts; i++)
    {
        pt = pts[i];

        if (i == 0)
        {
            emr->rclBounds.left = emr->rclBounds.right = vert_array[pt].x;
            emr->rclBounds.top = emr->rclBounds.bottom = vert_array[pt].y;
        }
        else
        {
            if (vert_array[pt].x < emr->rclBounds.left)
                emr->rclBounds.left = vert_array[pt].x;
            else if (vert_array[pt].x > emr->rclBounds.right)
                emr->rclBounds.right = vert_array[pt].x;
            if (vert_array[pt].y < emr->rclBounds.top)
                emr->rclBounds.top = vert_array[pt].y;
            else if (vert_array[pt].y > emr->rclBounds.bottom)
                emr->rclBounds.bottom = vert_array[pt].y;
        }
    }
    emr->rclBounds.right--;
    emr->rclBounds.bottom--;

    emr->emr.iType = EMR_GRADIENTFILL;
    emr->emr.nSize = size;
    emr->nVer = nvert;
    emr->nTri = ngrad;
    emr->ulMode = mode;
    memcpy( emr->Ver, vert_array, nvert * sizeof(vert_array[0]) );
    memcpy( emr->Ver + nvert, pts, num_pts * sizeof(pts[0]) );

    EMFDRV_UpdateBBox( dev, &emr->rclBounds );
    ret = EMFDRV_WriteRecord( dev, &emr->emr );
    HeapFree( GetProcessHeap(), 0, emr );
    return ret;
}

/**********************************************************************
 *	     EMFDRV_FillPath
 */
BOOL EMFDRV_FillPath( PHYSDEV dev )
{
    return emfdrv_stroke_and_fill_path( dev, EMR_FILLPATH );
}

/**********************************************************************
 *	     EMFDRV_StrokeAndFillPath
 */
BOOL EMFDRV_StrokeAndFillPath( PHYSDEV dev )
{
    return emfdrv_stroke_and_fill_path( dev, EMR_STROKEANDFILLPATH );
}

/**********************************************************************
 *           EMFDRV_StrokePath
 */
BOOL EMFDRV_StrokePath( PHYSDEV dev )
{
    return emfdrv_stroke_and_fill_path( dev, EMR_STROKEPATH );
}
