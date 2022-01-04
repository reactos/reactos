/*
 * Enhanced MetaFile driver
 *
 * Copyright 1999 Huw D M Davies
 * Copyright 2021 Jacek Caban for CodeWeavers
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

//#include "ntgdi_private.h"
#include "wine/config.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winnls.h"
#include "winerror.h"
#include "gdi_private.h"
#ifdef __REACTOS__
#include "wine/winternl.h"
#else
#include "winternl.h"
#endif
#include "wine/wingdi16.h"
#include "wine/debug.h"

#define M_PI 3.14159265358979323846
#define M_PI_2 1.570796326794896619

static void emfdrv_update_bounds( WINEDC *dc, RECTL *rect )
{
    RECTL *bounds = &dc->emf_bounds;
    RECTL vport_rect = *rect;

    //lp_to_dp( dc, (POINT *)&vport_rect, 2 );
    LPtoDP(dc->hdc, (POINT *)&vport_rect, 2 );

    /* The coordinate systems may be mirrored
       (LPtoDP handles points, not rectangles) */
    if (vport_rect.left > vport_rect.right)
    {
        LONG temp = vport_rect.right;
        vport_rect.right = vport_rect.left;
        vport_rect.left = temp;
    }
    if (vport_rect.top > vport_rect.bottom)
    {
        LONG temp = vport_rect.bottom;
        vport_rect.bottom = vport_rect.top;
        vport_rect.top = temp;
    }

    if (bounds->left > bounds->right)
    {
        /* first bounding rectangle */
        *bounds = vport_rect;
    }
    else
    {
        bounds->left   = min( bounds->left,   vport_rect.left );
        bounds->top    = min( bounds->top,    vport_rect.top );
        bounds->right  = max( bounds->right,  vport_rect.right );
        bounds->bottom = max( bounds->bottom, vport_rect.bottom );
    }
}

BOOL EMFDRV_LineTo( WINEDC *dc, INT x, INT y )
{
    RECTL bounds;
    POINT pt;

    //pt = dc->attr->cur_pos;
    GetCurrentPositionEx(dc->hdc, &pt);

    bounds.left   = min( x, pt.x );
    bounds.top    = min( y, pt.y );
    bounds.right  = max( x, pt.x );
    bounds.bottom = max( y, pt.y );
    emfdrv_update_bounds( dc, &bounds );
    return TRUE;
}

BOOL EMFDRV_RoundRect( WINEDC *dc, INT left, INT top, INT right,
                                    INT bottom, INT ell_width, INT ell_height )
{
    RECTL bounds;

    if (left == right || top == bottom) return FALSE;

    bounds.left   = min( left, right );
    bounds.top    = min( top, bottom );
    bounds.right  = max( left, right );
    bounds.bottom = max( top, bottom );
    if (GetGraphicsMode(dc->hdc) == GM_COMPATIBLE)//dc->attr->graphics_mode == GM_COMPATIBLE)
    {
        bounds.right--;
        bounds.bottom--;
    }

    emfdrv_update_bounds( dc, &bounds );
    return TRUE;
}

BOOL EMFDRV_ArcChordPie( WINEDC *dc, INT left, INT top, INT right, INT bottom,
                                INT xstart, INT ystart, INT xend, INT yend, DWORD type )
{
    INT temp, x_centre, y_centre, i;
    double angle_start, angle_end;
    double xinter_start, yinter_start, xinter_end, yinter_end;
    EMRARC emr;
    RECTL bounds;

    if (left == right || top == bottom) return FALSE;

    if (left > right) { temp = left; left = right; right = temp; }
    if (top > bottom) { temp = top; top = bottom; bottom = temp; }

    if (GetGraphicsMode(dc->hdc) == GM_COMPATIBLE)//dc->attr->graphics_mode == GM_COMPATIBLE)
    {
        right--;
        bottom--;
    }

    emr.emr.iType     = type;
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
    x_centre = (left + right + 1) / 2;
    y_centre = (top + bottom + 1) / 2;

    xstart -= x_centre;
    ystart -= y_centre;
    xend   -= x_centre;
    yend   -= y_centre;

    /* invert y co-ords to get angle anti-clockwise from x-axis */
    angle_start = atan2( -(double)ystart, (double)xstart );
    angle_end   = atan2( -(double)yend, (double)xend );

    /* These are the intercepts of the start/end lines with the arc */
    xinter_start = (right - left + 1)/2 * cos(angle_start) + x_centre;
    yinter_start = -(bottom - top + 1)/2 * sin(angle_start) + y_centre;
    xinter_end   = (right - left + 1)/2 * cos(angle_end) + x_centre;
    yinter_end   = -(bottom - top + 1)/2 * sin(angle_end) + y_centre;

    if (angle_start < 0) angle_start += 2 * M_PI;
    if (angle_end < 0) angle_end += 2 * M_PI;
    if (angle_end < angle_start) angle_end += 2 * M_PI;

    bounds.left   = min( xinter_start, xinter_end );
    bounds.top    = min( yinter_start, yinter_end );
    bounds.right  = max( xinter_start, xinter_end );
    bounds.bottom = max( yinter_start, yinter_end );

    for (i = 0; i <= 8; i++)
    {
        if(i * M_PI / 2 < angle_start) /* loop until we're past start */
	    continue;
	if(i * M_PI / 2 > angle_end)   /* if we're past end we're finished */
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
    if (type == EMR_PIE)
    {
        if (bounds.left > x_centre) bounds.left = x_centre;
	else if (bounds.right < x_centre) bounds.right = x_centre;
	if (bounds.top > y_centre) bounds.top = y_centre;
	else if (bounds.bottom < y_centre) bounds.bottom = y_centre;
    }
    else if (type == EMR_ARCTO)
    {
        POINT pt;
        //pt = dc->attr->cur_pos;
        GetCurrentPositionEx(dc->hdc, &pt);
        bounds.left   = min( bounds.left, pt.x );
        bounds.top    = min( bounds.top, pt.y );
        bounds.right  = max( bounds.right, pt.x );
        bounds.bottom = max( bounds.bottom, pt.y );
    }
    emfdrv_update_bounds( dc, &bounds );
    return TRUE;
}

BOOL EMFDRV_Arc( WINEDC *dc, INT left, INT top, INT right, INT bottom,
                              INT xstart, INT ystart, INT xend, INT yend )
{
    return EMFDRV_ArcChordPie( dc, left, top, right, bottom, xstart, ystart,
                               xend, yend, EMR_ARC );
}

BOOL EMFDRV_ArcTo( WINEDC *dc, INT left, INT top, INT right, INT bottom,
                                INT xstart, INT ystart, INT xend, INT yend )
{
    return EMFDRV_ArcChordPie( dc, left, top, right, bottom, xstart, ystart,
                               xend, yend, EMR_ARCTO );
}

BOOL EMFDRV_Pie( WINEDC *dc, INT left, INT top, INT right, INT bottom,
                              INT xstart, INT ystart, INT xend, INT yend )
{
    return EMFDRV_ArcChordPie( dc, left, top, right, bottom, xstart, ystart,
                               xend, yend, EMR_PIE );
}

BOOL EMFDRV_Chord( WINEDC *dc, INT left, INT top, INT right, INT bottom,
                                INT xstart, INT ystart, INT xend, INT yend )
{
    return EMFDRV_ArcChordPie( dc, left, top, right, bottom, xstart, ystart,
                               xend, yend, EMR_CHORD );
}

BOOL EMFDRV_Ellipse( WINEDC *dc, INT left, INT top, INT right, INT bottom )
{
    RECTL bounds;

    if (left == right || top == bottom) return FALSE;

    bounds.left   = min( left, right );
    bounds.top    = min( top, bottom );
    bounds.right  = max( left, right );
    bounds.bottom = max( top, bottom );
    if (GetGraphicsMode(dc->hdc) == GM_COMPATIBLE)//dc->attr->graphics_mode == GM_COMPATIBLE)
    {
        bounds.right--;
        bounds.bottom--;
    }

    emfdrv_update_bounds( dc, &bounds );
    return TRUE;
}

BOOL EMFDRV_Rectangle( WINEDC *dc, INT left, INT top, INT right, INT bottom )
{
    RECTL bounds;

    if (left == right || top == bottom) return FALSE;

    bounds.left   = min( left, right );
    bounds.top    = min( top, bottom );
    bounds.right  = max( left, right );
    bounds.bottom = max( top, bottom );
    if (GetGraphicsMode(dc->hdc) == GM_COMPATIBLE)//dc->attr->graphics_mode == GM_COMPATIBLE)
    {
        bounds.right--;
        bounds.bottom--;
    }

    emfdrv_update_bounds( dc, &bounds );
    return TRUE;
}

COLORREF EMFDRV_SetPixel( WINEDC *dc, INT x, INT y, COLORREF color )
{
    RECTL bounds;

    bounds.left = bounds.right = x;
    bounds.top = bounds.bottom = y;
    emfdrv_update_bounds( dc, &bounds );
    return CLR_INVALID;
}

BOOL EMFDRV_PolylineTo( WINEDC *dc, const POINT *pt, INT count )
{
    /* FIXME: update bounding rect */
    return TRUE;
}

BOOL EMFDRV_PolyBezier( WINEDC *dc, const POINT *pts, DWORD count )
{
    /* FIXME: update bounding rect */
    return TRUE;
}

BOOL EMFDRV_PolyBezierTo( WINEDC *dc, const POINT *pts, DWORD count )
{
    /* FIXME: update bounding rect */
    return TRUE;
}

BOOL EMFDRV_PolyPolyline( WINEDC *dc, const POINT *pt,
                                       const DWORD *counts, UINT polys )
{
    /* FIXME: update bounding rect */
    return TRUE;
}

BOOL EMFDRV_PolyPolygon( WINEDC *dc, const POINT *pt,
                                      const INT *counts, UINT polys )
{
    /* FIXME: update bounding rect */
    return TRUE;
}

BOOL EMFDRV_PolyDraw( WINEDC *dc, const POINT *pts,
                                   const BYTE *types, DWORD count )
{
    /* FIXME: update bounding rect */
    return TRUE;
}

BOOL EMFDRV_FillRgn( WINEDC *dc, HRGN hrgn, HBRUSH hbrush )
{
    /* FIXME: update bounding rect */
    return TRUE;
}

BOOL EMFDRV_FrameRgn( WINEDC *dc, HRGN hrgn, HBRUSH hbrush, INT width, INT height )
{
    /* FIXME: update bounding rect */
    return TRUE;
}

BOOL EMFDRV_InvertRgn( WINEDC *dc, HRGN hrgn )
{
    /* FIXME: update bounding rect */
    return TRUE;
}

BOOL EMFDRV_ExtTextOut( WINEDC *dc, INT x, INT y, UINT flags, const RECT *lprect,
                                     LPCWSTR str, UINT count, const INT *lpDx )
{
    /* FIXME: update bounding rect */
    return TRUE;
}

BOOL EMFDRV_GradientFill( WINEDC *dc, TRIVERTEX *vert_array, ULONG nvert,
                                       void *grad_array, ULONG ngrad, ULONG mode )
{
    /* FIXME: update bounding rect */
    return TRUE;
}

BOOL EMFDRV_FillPath( WINEDC *dc )
{
    /* FIXME: update bound rect */
    return TRUE;
}

BOOL EMFDRV_StrokeAndFillPath( WINEDC *dc )
{
    /* FIXME: update bound rect */
    return TRUE;
}

BOOL EMFDRV_StrokePath( WINEDC *dc )
{
    /* FIXME: update bound rect */
    return TRUE;
}

BOOL EMFDRV_AlphaBlend( WINEDC *dc_dst, INT x_dst, INT y_dst, INT width_dst, INT height_dst,
                                     HDC dc_src, INT x_src, INT y_src, INT width_src, INT height_src,
                                     BLENDFUNCTION func )
{
    /* FIXME: update bound rect */
    return TRUE;
}

BOOL EMFDRV_PatBlt( WINEDC *dc, INT left, INT top, INT width, INT height, DWORD rop )
{
    /* FIXME: update bound rect */
    return TRUE;
}

INT EMFDRV_StretchDIBits( WINEDC *dc, INT x_dst, INT y_dst, INT width_dst,
                                       INT height_dst, INT x_src, INT y_src, INT width_src,
                                       INT height_src, const void *bits, BITMAPINFO *info,
                                       UINT wUsage, DWORD dwRop )
{
    /* FIXME: Update bound rect */
    return height_src;
}

INT EMFDRV_SetDIBitsToDevice( WINEDC *dc, INT x_dst, INT y_dst, DWORD width,
                                           DWORD height, INT x_src, INT y_src, UINT startscan,
                                           UINT lines, const void *bits, BITMAPINFO *info,
                                           UINT usage )
{
    /* FIXME: Update bound rect */
    return lines;
}

HBITMAP EMFDRV_SelectBitmap( WINEDC *dc, HBITMAP hbitmap )
{
    return 0;
}

