/*
 * GDI drawing functions.
 *
 * Copyright 1993, 1994 Alexandre Julliard
 * Copyright 1997 Bertho A. Stultiens
 *           1999 Huw D M Davies
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
#include <string.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winerror.h"
#include "gdi_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdi);


/***********************************************************************
 *           LineTo    (GDI32.@)
 */
BOOL WINAPI LineTo( HDC hdc, INT x, INT y )
{
    DC * dc = get_dc_ptr( hdc );
    BOOL ret;

    if(!dc) return FALSE;

    update_dc( dc );
    if(PATH_IsPathOpen(dc->path))
        ret = PATH_LineTo(dc, x, y);
    else
        ret = dc->funcs->pLineTo && dc->funcs->pLineTo(dc->physDev,x,y);
    if(ret) {
        dc->CursPosX = x;
        dc->CursPosY = y;
    }
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           MoveToEx    (GDI32.@)
 */
BOOL WINAPI MoveToEx( HDC hdc, INT x, INT y, LPPOINT pt )
{
    BOOL ret = TRUE;
    DC * dc = get_dc_ptr( hdc );

    if(!dc) return FALSE;

    if(pt) {
        pt->x = dc->CursPosX;
        pt->y = dc->CursPosY;
    }
    dc->CursPosX = x;
    dc->CursPosY = y;

    if(PATH_IsPathOpen(dc->path)) ret = PATH_MoveTo(dc);
    else if (dc->funcs->pMoveTo) ret = dc->funcs->pMoveTo(dc->physDev,x,y);
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           Arc    (GDI32.@)
 */
BOOL WINAPI Arc( HDC hdc, INT left, INT top, INT right,
                     INT bottom, INT xstart, INT ystart,
                     INT xend, INT yend )
{
    BOOL ret = FALSE;
    DC * dc = get_dc_ptr( hdc );

    if (dc)
    {
        update_dc( dc );
        if(PATH_IsPathOpen(dc->path))
            ret = PATH_Arc(dc, left, top, right, bottom, xstart, ystart, xend, yend,0);
        else if (dc->funcs->pArc)
            ret = dc->funcs->pArc(dc->physDev,left,top,right,bottom,xstart,ystart,xend,yend);
        release_dc_ptr( dc );
    }
    return ret;
}

/***********************************************************************
 *           ArcTo    (GDI32.@)
 */
BOOL WINAPI ArcTo( HDC hdc,
                     INT left,   INT top,
                     INT right,  INT bottom,
                     INT xstart, INT ystart,
                     INT xend,   INT yend )
{
    double width = fabs(right-left),
        height = fabs(bottom-top),
        xradius = width/2,
        yradius = height/2,
        xcenter = right > left ? left+xradius : right+xradius,
        ycenter = bottom > top ? top+yradius : bottom+yradius,
        angle;
    BOOL result;
    DC * dc = get_dc_ptr( hdc );
    if(!dc) return FALSE;

    update_dc( dc );
    if(PATH_IsPathOpen(dc->path))
        result = PATH_Arc(dc,left,top,right,bottom,xstart,ystart,xend,yend,-1);
    else if(dc->funcs->pArcTo)
        result = dc->funcs->pArcTo( dc->physDev, left, top, right, bottom,
				  xstart, ystart, xend, yend );
    else /* We'll draw a line from the current position to the starting point of the arc, then draw the arc */
    {
        angle = atan2(((ystart-ycenter)/height),
                      ((xstart-xcenter)/width));
        LineTo(hdc, GDI_ROUND(xcenter+(cos(angle)*xradius)),
               GDI_ROUND(ycenter+(sin(angle)*yradius)));
        result = Arc(hdc, left, top, right, bottom, xstart, ystart, xend, yend);
    }
    if (result) {
        angle = atan2(((yend-ycenter)/height),
                      ((xend-xcenter)/width));
        dc->CursPosX = GDI_ROUND(xcenter+(cos(angle)*xradius));
        dc->CursPosY = GDI_ROUND(ycenter+(sin(angle)*yradius));
    }
    release_dc_ptr( dc );
    return result;
}


/***********************************************************************
 *           Pie   (GDI32.@)
 */
BOOL WINAPI Pie( HDC hdc, INT left, INT top,
                     INT right, INT bottom, INT xstart, INT ystart,
                     INT xend, INT yend )
{
    BOOL ret = FALSE;
    DC * dc = get_dc_ptr( hdc );
    if (!dc) return FALSE;

    update_dc( dc );
    if(PATH_IsPathOpen(dc->path))
        ret = PATH_Arc(dc,left,top,right,bottom,xstart,ystart,xend,yend,2);
    else if(dc->funcs->pPie)
        ret = dc->funcs->pPie(dc->physDev,left,top,right,bottom,xstart,ystart,xend,yend);

    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           Chord    (GDI32.@)
 */
BOOL WINAPI Chord( HDC hdc, INT left, INT top,
                       INT right, INT bottom, INT xstart, INT ystart,
                       INT xend, INT yend )
{
    BOOL ret = FALSE;
    DC * dc = get_dc_ptr( hdc );
    if (!dc) return FALSE;

    update_dc( dc );
    if(PATH_IsPathOpen(dc->path))
	ret = PATH_Arc(dc,left,top,right,bottom,xstart,ystart,xend,yend,1);
    else if(dc->funcs->pChord)
        ret = dc->funcs->pChord(dc->physDev,left,top,right,bottom,xstart,ystart,xend,yend);

    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           Ellipse    (GDI32.@)
 */
BOOL WINAPI Ellipse( HDC hdc, INT left, INT top,
                         INT right, INT bottom )
{
    BOOL ret = FALSE;
    DC * dc = get_dc_ptr( hdc );
    if (!dc) return FALSE;

    update_dc( dc );
    if(PATH_IsPathOpen(dc->path))
	ret = PATH_Ellipse(dc,left,top,right,bottom);
    else if (dc->funcs->pEllipse)
        ret = dc->funcs->pEllipse(dc->physDev,left,top,right,bottom);

    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           Rectangle    (GDI32.@)
 */
BOOL WINAPI Rectangle( HDC hdc, INT left, INT top,
                           INT right, INT bottom )
{
    BOOL ret = FALSE;
    DC * dc = get_dc_ptr( hdc );

    if (dc)
    {
        update_dc( dc );
        if(PATH_IsPathOpen(dc->path))
            ret = PATH_Rectangle(dc, left, top, right, bottom);
        else if (dc->funcs->pRectangle)
            ret = dc->funcs->pRectangle(dc->physDev,left,top,right,bottom);
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *           RoundRect    (GDI32.@)
 */
BOOL WINAPI RoundRect( HDC hdc, INT left, INT top, INT right,
                           INT bottom, INT ell_width, INT ell_height )
{
    BOOL ret = FALSE;
    DC *dc = get_dc_ptr( hdc );

    if (dc)
    {
        update_dc( dc );
        if(PATH_IsPathOpen(dc->path))
	    ret = PATH_RoundRect(dc,left,top,right,bottom,ell_width,ell_height);
        else if (dc->funcs->pRoundRect)
            ret = dc->funcs->pRoundRect(dc->physDev,left,top,right,bottom,ell_width,ell_height);
        release_dc_ptr( dc );
    }
    return ret;
}

/***********************************************************************
 *           SetPixel    (GDI32.@)
 */
COLORREF WINAPI SetPixel( HDC hdc, INT x, INT y, COLORREF color )
{
    COLORREF ret = 0;
    DC * dc = get_dc_ptr( hdc );

    if (dc)
    {
        update_dc( dc );
        if (dc->funcs->pSetPixel) ret = dc->funcs->pSetPixel(dc->physDev,x,y,color);
        release_dc_ptr( dc );
    }
    return ret;
}

/***********************************************************************
 *           SetPixelV    (GDI32.@)
 */
BOOL WINAPI SetPixelV( HDC hdc, INT x, INT y, COLORREF color )
{
    BOOL ret = FALSE;
    DC * dc = get_dc_ptr( hdc );

    if (dc)
    {
        update_dc( dc );
        if (dc->funcs->pSetPixel)
        {
            dc->funcs->pSetPixel(dc->physDev,x,y,color);
            ret = TRUE;
        }
        release_dc_ptr( dc );
    }
    return ret;
}

/***********************************************************************
 *           GetPixel    (GDI32.@)
 */
COLORREF WINAPI GetPixel( HDC hdc, INT x, INT y )
{
    COLORREF ret = CLR_INVALID;
    DC * dc = get_dc_ptr( hdc );

    if (dc)
    {
        update_dc( dc );
        /* FIXME: should this be in the graphics driver? */
        if (PtVisible( hdc, x, y ))
        {
            if (dc->funcs->pGetPixel) ret = dc->funcs->pGetPixel(dc->physDev,x,y);
        }
        release_dc_ptr( dc );
    }
    return ret;
}


/******************************************************************************
 * ChoosePixelFormat [GDI32.@]
 * Matches a pixel format to given format
 *
 * PARAMS
 *    hdc  [I] Device context to search for best pixel match
 *    ppfd [I] Pixel format for which a match is sought
 *
 * RETURNS
 *    Success: Pixel format index closest to given format
 *    Failure: 0
 */
INT WINAPI ChoosePixelFormat( HDC hdc, const PIXELFORMATDESCRIPTOR* ppfd )
{
    INT ret = 0;
    DC * dc = get_dc_ptr( hdc );

    TRACE("(%p,%p)\n",hdc,ppfd);

    if (!dc) return 0;

    if (!dc->funcs->pChoosePixelFormat) FIXME(" :stub\n");
    else ret = dc->funcs->pChoosePixelFormat(dc->physDev,ppfd);

    release_dc_ptr( dc );
    return ret;
}


/******************************************************************************
 * SetPixelFormat [GDI32.@]
 * Sets pixel format of device context
 *
 * PARAMS
 *    hdc          [I] Device context to search for best pixel match
 *    iPixelFormat [I] Pixel format index
 *    ppfd         [I] Pixel format for which a match is sought
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI SetPixelFormat( HDC hdc, INT iPixelFormat,
                            const PIXELFORMATDESCRIPTOR *ppfd)
{
    INT bRet = FALSE;
    DC * dc = get_dc_ptr( hdc );

    TRACE("(%p,%d,%p)\n",hdc,iPixelFormat,ppfd);

    if (!dc) return 0;

    update_dc( dc );
    if (!dc->funcs->pSetPixelFormat) FIXME(" :stub\n");
    else bRet = dc->funcs->pSetPixelFormat(dc->physDev,iPixelFormat,ppfd);

    release_dc_ptr( dc );
    return bRet;
}


/******************************************************************************
 * GetPixelFormat [GDI32.@]
 * Gets index of pixel format of DC
 *
 * PARAMETERS
 *    hdc [I] Device context whose pixel format index is sought
 *
 * RETURNS
 *    Success: Currently selected pixel format
 *    Failure: 0
 */
INT WINAPI GetPixelFormat( HDC hdc )
{
    INT ret = 0;
    DC * dc = get_dc_ptr( hdc );

    TRACE("(%p)\n",hdc);

    if (!dc) return 0;

    update_dc( dc );
    if (!dc->funcs->pGetPixelFormat) FIXME(" :stub\n");
    else ret = dc->funcs->pGetPixelFormat(dc->physDev);

    release_dc_ptr( dc );
    return ret;
}


/******************************************************************************
 * DescribePixelFormat [GDI32.@]
 * Gets info about pixel format from DC
 *
 * PARAMS
 *    hdc          [I] Device context
 *    iPixelFormat [I] Pixel format selector
 *    nBytes       [I] Size of buffer
 *    ppfd         [O] Pointer to structure to receive pixel format data
 *
 * RETURNS
 *    Success: Maximum pixel format index of the device context
 *    Failure: 0
 */
INT WINAPI DescribePixelFormat( HDC hdc, INT iPixelFormat, UINT nBytes,
                                LPPIXELFORMATDESCRIPTOR ppfd )
{
    INT ret = 0;
    DC * dc = get_dc_ptr( hdc );

    TRACE("(%p,%d,%d,%p): stub\n",hdc,iPixelFormat,nBytes,ppfd);

    if (!dc) return 0;

    update_dc( dc );
    if (!dc->funcs->pDescribePixelFormat)
    {
        FIXME(" :stub\n");
        ppfd->nSize = nBytes;
        ppfd->nVersion = 1;
	ret = 3;
    }
    else ret = dc->funcs->pDescribePixelFormat(dc->physDev,iPixelFormat,nBytes,ppfd);

    release_dc_ptr( dc );
    return ret;
}


/******************************************************************************
 * SwapBuffers [GDI32.@]
 * Exchanges front and back buffers of window
 *
 * PARAMS
 *    hdc [I] Device context whose buffers get swapped
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI SwapBuffers( HDC hdc )
{
    INT bRet = FALSE;
    DC * dc = get_dc_ptr( hdc );

    TRACE("(%p)\n",hdc);

    if (!dc) return TRUE;

    update_dc( dc );
    if (!dc->funcs->pSwapBuffers)
    {
        FIXME(" :stub\n");
	bRet = TRUE;
    }
    else bRet = dc->funcs->pSwapBuffers(dc->physDev);

    release_dc_ptr( dc );
    return bRet;
}


/***********************************************************************
 *           PaintRgn    (GDI32.@)
 */
BOOL WINAPI PaintRgn( HDC hdc, HRGN hrgn )
{
    BOOL ret = FALSE;
    DC * dc = get_dc_ptr( hdc );

    if (dc)
    {
        update_dc( dc );
        if (dc->funcs->pPaintRgn) ret = dc->funcs->pPaintRgn(dc->physDev,hrgn);
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *           FillRgn    (GDI32.@)
 */
BOOL WINAPI FillRgn( HDC hdc, HRGN hrgn, HBRUSH hbrush )
{
    BOOL retval = FALSE;
    HBRUSH prevBrush;
    DC * dc = get_dc_ptr( hdc );

    if (!dc) return FALSE;
    if(dc->funcs->pFillRgn)
    {
        update_dc( dc );
        retval = dc->funcs->pFillRgn(dc->physDev, hrgn, hbrush);
    }
    else if ((prevBrush = SelectObject( hdc, hbrush )))
    {
        retval = PaintRgn( hdc, hrgn );
        SelectObject( hdc, prevBrush );
    }
    release_dc_ptr( dc );
    return retval;
}


/***********************************************************************
 *           FrameRgn     (GDI32.@)
 */
BOOL WINAPI FrameRgn( HDC hdc, HRGN hrgn, HBRUSH hbrush,
                          INT nWidth, INT nHeight )
{
    BOOL ret = FALSE;
    DC *dc = get_dc_ptr( hdc );

    if (!dc) return FALSE;

    if(dc->funcs->pFrameRgn)
    {
        update_dc( dc );
        ret = dc->funcs->pFrameRgn( dc->physDev, hrgn, hbrush, nWidth, nHeight );
    }
    else
    {
        HRGN tmp = CreateRectRgn( 0, 0, 0, 0 );
        if (tmp)
        {
            if (REGION_FrameRgn( tmp, hrgn, nWidth, nHeight ))
            {
                FillRgn( hdc, tmp, hbrush );
                ret = TRUE;
            }
            DeleteObject( tmp );
        }
    }
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           InvertRgn    (GDI32.@)
 */
BOOL WINAPI InvertRgn( HDC hdc, HRGN hrgn )
{
    HBRUSH prevBrush;
    INT prevROP;
    BOOL retval;
    DC *dc = get_dc_ptr( hdc );
    if (!dc) return FALSE;

    if(dc->funcs->pInvertRgn)
    {
        update_dc( dc );
        retval = dc->funcs->pInvertRgn( dc->physDev, hrgn );
    }
    else
    {
        prevBrush = SelectObject( hdc, GetStockObject(BLACK_BRUSH) );
        prevROP = SetROP2( hdc, R2_NOT );
        retval = PaintRgn( hdc, hrgn );
        SelectObject( hdc, prevBrush );
        SetROP2( hdc, prevROP );
    }
    release_dc_ptr( dc );
    return retval;
}


/**********************************************************************
 *          Polyline   (GDI32.@)
 */
BOOL WINAPI Polyline( HDC hdc, const POINT* pt, INT count )
{
    BOOL ret = FALSE;
    DC * dc = get_dc_ptr( hdc );

    if (dc)
    {
        update_dc( dc );
        if (PATH_IsPathOpen(dc->path)) ret = PATH_Polyline(dc, pt, count);
        else if (dc->funcs->pPolyline) ret = dc->funcs->pPolyline(dc->physDev,pt,count);
        release_dc_ptr( dc );
    }
    return ret;
}

/**********************************************************************
 *          PolylineTo   (GDI32.@)
 */
BOOL WINAPI PolylineTo( HDC hdc, const POINT* pt, DWORD cCount )
{
    DC * dc = get_dc_ptr( hdc );
    BOOL ret = FALSE;

    if(!dc) return FALSE;

    if(PATH_IsPathOpen(dc->path))
    {
        update_dc( dc );
        ret = PATH_PolylineTo(dc, pt, cCount);
    }
    else if(dc->funcs->pPolylineTo)
    {
        update_dc( dc );
        ret = dc->funcs->pPolylineTo(dc->physDev, pt, cCount);
    }
    else /* do it using Polyline */
    {
        POINT *pts = HeapAlloc( GetProcessHeap(), 0,
				sizeof(POINT) * (cCount + 1) );
	if (pts)
        {
            pts[0].x = dc->CursPosX;
            pts[0].y = dc->CursPosY;
            memcpy( pts + 1, pt, sizeof(POINT) * cCount );
            ret = Polyline( hdc, pts, cCount + 1 );
            HeapFree( GetProcessHeap(), 0, pts );
        }
    }
    if(ret) {
        dc->CursPosX = pt[cCount-1].x;
	dc->CursPosY = pt[cCount-1].y;
    }
    release_dc_ptr( dc );
    return ret;
}


/**********************************************************************
 *          Polygon  (GDI32.@)
 */
BOOL WINAPI Polygon( HDC hdc, const POINT* pt, INT count )
{
    BOOL ret = FALSE;
    DC * dc = get_dc_ptr( hdc );

    if (dc)
    {
        update_dc( dc );
        if (PATH_IsPathOpen(dc->path)) ret = PATH_Polygon(dc, pt, count);
        else if (dc->funcs->pPolygon) ret = dc->funcs->pPolygon(dc->physDev,pt,count);
        release_dc_ptr( dc );
    }
    return ret;
}


/**********************************************************************
 *          PolyPolygon  (GDI32.@)
 */
BOOL WINAPI PolyPolygon( HDC hdc, const POINT* pt, const INT* counts,
                             UINT polygons )
{
    BOOL ret = FALSE;
    DC * dc = get_dc_ptr( hdc );

    if (dc)
    {
        update_dc( dc );
        if (PATH_IsPathOpen(dc->path)) ret = PATH_PolyPolygon(dc, pt, counts, polygons);
        else if (dc->funcs->pPolyPolygon) ret = dc->funcs->pPolyPolygon(dc->physDev,pt,counts,polygons);
        release_dc_ptr( dc );
    }
    return ret;
}

/**********************************************************************
 *          PolyPolyline  (GDI32.@)
 */
BOOL WINAPI PolyPolyline( HDC hdc, const POINT* pt, const DWORD* counts,
                            DWORD polylines )
{
    BOOL ret = FALSE;
    DC * dc = get_dc_ptr( hdc );

    if (dc)
    {
        update_dc( dc );
        if (PATH_IsPathOpen(dc->path)) ret = PATH_PolyPolyline(dc, pt, counts, polylines);
        else if (dc->funcs->pPolyPolyline) ret = dc->funcs->pPolyPolyline(dc->physDev,pt,counts,polylines);
        release_dc_ptr( dc );
    }
    return ret;
}

/**********************************************************************
 *          ExtFloodFill   (GDI32.@)
 */
BOOL WINAPI ExtFloodFill( HDC hdc, INT x, INT y, COLORREF color,
                              UINT fillType )
{
    BOOL ret = FALSE;
    DC * dc = get_dc_ptr( hdc );

    if (dc)
    {
        update_dc( dc );
        if (dc->funcs->pExtFloodFill) ret = dc->funcs->pExtFloodFill(dc->physDev,x,y,color,fillType);
        release_dc_ptr( dc );
    }
    return ret;
}


/**********************************************************************
 *          FloodFill   (GDI32.@)
 */
BOOL WINAPI FloodFill( HDC hdc, INT x, INT y, COLORREF color )
{
    return ExtFloodFill( hdc, x, y, color, FLOODFILLBORDER );
}


/******************************************************************************
 * PolyBezier [GDI32.@]
 * Draws one or more Bezier curves
 *
 * PARAMS
 *    hDc     [I] Handle to device context
 *    lppt    [I] Pointer to endpoints and control points
 *    cPoints [I] Count of endpoints and control points
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI PolyBezier( HDC hdc, const POINT* lppt, DWORD cPoints )
{
    BOOL ret = FALSE;
    DC * dc;

    /* cPoints must be 3 * n + 1 (where n>=1) */
    if (cPoints == 1 || (cPoints % 3) != 1) return FALSE;

    dc = get_dc_ptr( hdc );
    if(!dc) return FALSE;

    if(PATH_IsPathOpen(dc->path))
    {
        update_dc( dc );
	ret = PATH_PolyBezier(dc, lppt, cPoints);
    }
    else if (dc->funcs->pPolyBezier)
    {
        update_dc( dc );
        ret = dc->funcs->pPolyBezier(dc->physDev, lppt, cPoints);
    }
    else  /* We'll convert it into line segments and draw them using Polyline */
    {
        POINT *Pts;
	INT nOut;

	if ((Pts = GDI_Bezier( lppt, cPoints, &nOut )))
        {
	    TRACE("Pts = %p, no = %d\n", Pts, nOut);
	    ret = Polyline( hdc, Pts, nOut );
	    HeapFree( GetProcessHeap(), 0, Pts );
	}
    }

    release_dc_ptr( dc );
    return ret;
}

/******************************************************************************
 * PolyBezierTo [GDI32.@]
 * Draws one or more Bezier curves
 *
 * PARAMS
 *    hDc     [I] Handle to device context
 *    lppt    [I] Pointer to endpoints and control points
 *    cPoints [I] Count of endpoints and control points
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI PolyBezierTo( HDC hdc, const POINT* lppt, DWORD cPoints )
{
    DC * dc;
    BOOL ret = FALSE;

    /* cbPoints must be 3 * n (where n>=1) */
    if (!cPoints || (cPoints % 3) != 0) return FALSE;

    dc = get_dc_ptr( hdc );
    if(!dc) return FALSE;

    if(PATH_IsPathOpen(dc->path))
    {
        update_dc( dc );
        ret = PATH_PolyBezierTo(dc, lppt, cPoints);
    }
    else if(dc->funcs->pPolyBezierTo)
    {
        update_dc( dc );
        ret = dc->funcs->pPolyBezierTo(dc->physDev, lppt, cPoints);
    }
    else  /* We'll do it using PolyBezier */
    {
        POINT *pt = HeapAlloc( GetProcessHeap(), 0, sizeof(POINT) * (cPoints + 1) );
	if(pt)
        {
            pt[0].x = dc->CursPosX;
            pt[0].y = dc->CursPosY;
            memcpy(pt + 1, lppt, sizeof(POINT) * cPoints);
            ret = PolyBezier(hdc, pt, cPoints+1);
            HeapFree( GetProcessHeap(), 0, pt );
        }
    }
    if(ret) {
        dc->CursPosX = lppt[cPoints-1].x;
        dc->CursPosY = lppt[cPoints-1].y;
    }
    release_dc_ptr( dc );
    return ret;
}

/***********************************************************************
 *      AngleArc (GDI32.@)
 */
BOOL WINAPI AngleArc(HDC hdc, INT x, INT y, DWORD dwRadius, FLOAT eStartAngle, FLOAT eSweepAngle)
{
    INT x1,y1,x2,y2, arcdir;
    BOOL result;
    DC *dc;

    if( (signed int)dwRadius < 0 )
	return FALSE;

    dc = get_dc_ptr( hdc );
    if(!dc) return FALSE;

    /* Calculate the end point */
    x2 = GDI_ROUND( x + cos((eStartAngle+eSweepAngle)*M_PI/180) * dwRadius );
    y2 = GDI_ROUND( y - sin((eStartAngle+eSweepAngle)*M_PI/180) * dwRadius );

    if(!PATH_IsPathOpen(dc->path) && dc->funcs->pAngleArc)
    {
        update_dc( dc );
        result = dc->funcs->pAngleArc( dc->physDev, x, y, dwRadius, eStartAngle, eSweepAngle );
    }
    else { /* do it using ArcTo */
        x1 = GDI_ROUND( x + cos(eStartAngle*M_PI/180) * dwRadius );
        y1 = GDI_ROUND( y - sin(eStartAngle*M_PI/180) * dwRadius );

        arcdir = SetArcDirection( hdc, eSweepAngle >= 0 ? AD_COUNTERCLOCKWISE : AD_CLOCKWISE);
        result = ArcTo( hdc, x-dwRadius, y-dwRadius, x+dwRadius, y+dwRadius,
                        x1, y1, x2, y2 );
        SetArcDirection( hdc, arcdir );
    }
    if (result) {
        dc->CursPosX = x2;
        dc->CursPosY = y2;
    }
    release_dc_ptr( dc );
    return result;
}

/***********************************************************************
 *      PolyDraw (GDI32.@)
 */
BOOL WINAPI PolyDraw(HDC hdc, const POINT *lppt, const BYTE *lpbTypes,
                       DWORD cCount)
{
    DC *dc;
    BOOL result = FALSE;
    POINT * line_pts = NULL, * bzr_pts = NULL, bzr[4];
    INT i, num_pts, num_bzr_pts, space, size;

    dc = get_dc_ptr( hdc );
    if(!dc) return FALSE;

    if( PATH_IsPathOpen( dc->path ) )
    {
        update_dc( dc );
        result = PATH_PolyDraw(dc, lppt, lpbTypes, cCount);
    }
    else if(dc->funcs->pPolyDraw)
    {
        update_dc( dc );
        result = dc->funcs->pPolyDraw( dc->physDev, lppt, lpbTypes, cCount );
    }
    else {
        /* check for valid point types */
        for(i = 0; i < cCount; i++) {
            switch(lpbTypes[i]) {
                case PT_MOVETO:
                case PT_LINETO | PT_CLOSEFIGURE:
                case PT_LINETO:
                    break;
                case PT_BEZIERTO:
                    if((i + 2 < cCount) && (lpbTypes[i + 1] == PT_BEZIERTO) &&
                        ((lpbTypes[i + 2] & ~PT_CLOSEFIGURE) == PT_BEZIERTO)){
                        i += 2;
                        break;
                    }
                default:
                    goto end;
            }
        }

        space = cCount + 300;
        line_pts = HeapAlloc(GetProcessHeap(), 0, space * sizeof(POINT));
        num_pts = 1;

        line_pts[0].x = dc->CursPosX;
        line_pts[0].y = dc->CursPosY;

        for(i = 0; i < cCount; i++) {
            switch(lpbTypes[i]) {
                case PT_MOVETO:
                    if(num_pts >= 2)
                        Polyline(hdc, line_pts, num_pts);
                    num_pts = 0;
                    line_pts[num_pts++] = lppt[i];
                    break;
                case PT_LINETO:
                case (PT_LINETO | PT_CLOSEFIGURE):
                    line_pts[num_pts++] = lppt[i];
                    break;
                case PT_BEZIERTO:
                    bzr[0].x = line_pts[num_pts - 1].x;
                    bzr[0].y = line_pts[num_pts - 1].y;
                    memcpy(&bzr[1], &lppt[i], 3 * sizeof(POINT));

                    bzr_pts = GDI_Bezier(bzr, 4, &num_bzr_pts);

                    size = num_pts + (cCount - i) + num_bzr_pts;
                    if(space < size){
                        space = size * 2;
                        line_pts = HeapReAlloc(GetProcessHeap(), 0, line_pts,
                            space * sizeof(POINT));
                    }
                    memcpy(&line_pts[num_pts], &bzr_pts[1],
                        (num_bzr_pts - 1) * sizeof(POINT));
                    num_pts += num_bzr_pts - 1;
                    HeapFree(GetProcessHeap(), 0, bzr_pts);
                    i += 2;
                    break;
                default:
                    goto end;
            }

            if(lpbTypes[i] & PT_CLOSEFIGURE)
                line_pts[num_pts++] = line_pts[0];
        }

        if(num_pts >= 2)
            Polyline(hdc, line_pts, num_pts);

        MoveToEx(hdc, line_pts[num_pts - 1].x, line_pts[num_pts - 1].y, NULL);
        HeapFree(GetProcessHeap(), 0, line_pts);
        result = TRUE;
    }

end:
    release_dc_ptr( dc );
    return result;
}


/**********************************************************************
 *           LineDDA   (GDI32.@)
 */
BOOL WINAPI LineDDA(INT nXStart, INT nYStart, INT nXEnd, INT nYEnd,
                    LINEDDAPROC callback, LPARAM lParam )
{
    INT xadd = 1, yadd = 1;
    INT err,erradd;
    INT cnt;
    INT dx = nXEnd - nXStart;
    INT dy = nYEnd - nYStart;

    if (dx < 0)
    {
        dx = -dx;
        xadd = -1;
    }
    if (dy < 0)
    {
        dy = -dy;
        yadd = -1;
    }
    if (dx > dy)  /* line is "more horizontal" */
    {
        err = 2*dy - dx; erradd = 2*dy - 2*dx;
        for(cnt = 0;cnt < dx; cnt++)
        {
            callback(nXStart,nYStart,lParam);
            if (err > 0)
            {
                nYStart += yadd;
                err += erradd;
            }
            else err += 2*dy;
            nXStart += xadd;
        }
    }
    else   /* line is "more vertical" */
    {
        err = 2*dx - dy; erradd = 2*dx - 2*dy;
        for(cnt = 0;cnt < dy; cnt++)
        {
            callback(nXStart,nYStart,lParam);
            if (err > 0)
            {
                nXStart += xadd;
                err += erradd;
            }
            else err += 2*dx;
            nYStart += yadd;
        }
    }
    return TRUE;
}


/******************************************************************
 *
 *   *Very* simple bezier drawing code,
 *
 *   It uses a recursive algorithm to divide the curve in a series
 *   of straight line segments. Not ideal but sufficient for me.
 *   If you are in need for something better look for some incremental
 *   algorithm.
 *
 *   7 July 1998 Rein Klazes
 */

 /*
  * some macro definitions for bezier drawing
  *
  * to avoid truncation errors the coordinates are
  * shifted upwards. When used in drawing they are
  * shifted down again, including correct rounding
  * and avoiding floating point arithmetic
  * 4 bits should allow 27 bits coordinates which I saw
  * somewhere in the win32 doc's
  *
  */

#define BEZIERSHIFTBITS 4
#define BEZIERSHIFTUP(x)    ((x)<<BEZIERSHIFTBITS)
#define BEZIERPIXEL        BEZIERSHIFTUP(1)
#define BEZIERSHIFTDOWN(x)  (((x)+(1<<(BEZIERSHIFTBITS-1)))>>BEZIERSHIFTBITS)
/* maximum depth of recursion */
#define BEZIERMAXDEPTH  8

/* size of array to store points on */
/* enough for one curve */
#define BEZIER_INITBUFSIZE    (150)

/* calculate Bezier average, in this case the middle
 * correctly rounded...
 * */

#define BEZIERMIDDLE(Mid, P1, P2) \
    (Mid).x=((P1).x+(P2).x + 1)/2;\
    (Mid).y=((P1).y+(P2).y + 1)/2;

/**********************************************************
* BezierCheck helper function to check
* that recursion can be terminated
*       Points[0] and Points[3] are begin and endpoint
*       Points[1] and Points[2] are control points
*       level is the recursion depth
*       returns true if the recursion can be terminated
*/
static BOOL BezierCheck( int level, POINT *Points)
{
    INT dx, dy;
    dx=Points[3].x-Points[0].x;
    dy=Points[3].y-Points[0].y;
    if(abs(dy)<=abs(dx)){/* shallow line */
        /* check that control points are between begin and end */
        if(Points[1].x < Points[0].x){
            if(Points[1].x < Points[3].x)
                return FALSE;
        }else
            if(Points[1].x > Points[3].x)
                return FALSE;
        if(Points[2].x < Points[0].x){
            if(Points[2].x < Points[3].x)
                return FALSE;
        }else
            if(Points[2].x > Points[3].x)
                return FALSE;
        dx=BEZIERSHIFTDOWN(dx);
        if(!dx) return TRUE;
        if(abs(Points[1].y-Points[0].y-(dy/dx)*
                BEZIERSHIFTDOWN(Points[1].x-Points[0].x)) > BEZIERPIXEL ||
           abs(Points[2].y-Points[0].y-(dy/dx)*
                   BEZIERSHIFTDOWN(Points[2].x-Points[0].x)) > BEZIERPIXEL )
            return FALSE;
        else
            return TRUE;
    }else{ /* steep line */
        /* check that control points are between begin and end */
        if(Points[1].y < Points[0].y){
            if(Points[1].y < Points[3].y)
                return FALSE;
        }else
            if(Points[1].y > Points[3].y)
                return FALSE;
        if(Points[2].y < Points[0].y){
            if(Points[2].y < Points[3].y)
                return FALSE;
        }else
            if(Points[2].y > Points[3].y)
                return FALSE;
        dy=BEZIERSHIFTDOWN(dy);
        if(!dy) return TRUE;
        if(abs(Points[1].x-Points[0].x-(dx/dy)*
                BEZIERSHIFTDOWN(Points[1].y-Points[0].y)) > BEZIERPIXEL ||
           abs(Points[2].x-Points[0].x-(dx/dy)*
                   BEZIERSHIFTDOWN(Points[2].y-Points[0].y)) > BEZIERPIXEL )
            return FALSE;
        else
            return TRUE;
    }
}

/* Helper for GDI_Bezier.
 * Just handles one Bezier, so Points should point to four POINTs
 */
static void GDI_InternalBezier( POINT *Points, POINT **PtsOut, INT *dwOut,
				INT *nPtsOut, INT level )
{
    if(*nPtsOut == *dwOut) {
        *dwOut *= 2;
	*PtsOut = HeapReAlloc( GetProcessHeap(), 0, *PtsOut,
			       *dwOut * sizeof(POINT) );
    }

    if(!level || BezierCheck(level, Points)) {
        if(*nPtsOut == 0) {
            (*PtsOut)[0].x = BEZIERSHIFTDOWN(Points[0].x);
            (*PtsOut)[0].y = BEZIERSHIFTDOWN(Points[0].y);
            *nPtsOut = 1;
        }
	(*PtsOut)[*nPtsOut].x = BEZIERSHIFTDOWN(Points[3].x);
        (*PtsOut)[*nPtsOut].y = BEZIERSHIFTDOWN(Points[3].y);
        (*nPtsOut) ++;
    } else {
        POINT Points2[4]; /* for the second recursive call */
        Points2[3]=Points[3];
        BEZIERMIDDLE(Points2[2], Points[2], Points[3]);
        BEZIERMIDDLE(Points2[0], Points[1], Points[2]);
        BEZIERMIDDLE(Points2[1],Points2[0],Points2[2]);

        BEZIERMIDDLE(Points[1], Points[0],  Points[1]);
        BEZIERMIDDLE(Points[2], Points[1], Points2[0]);
        BEZIERMIDDLE(Points[3], Points[2], Points2[1]);

        Points2[0]=Points[3];

        /* do the two halves */
        GDI_InternalBezier(Points, PtsOut, dwOut, nPtsOut, level-1);
        GDI_InternalBezier(Points2, PtsOut, dwOut, nPtsOut, level-1);
    }
}



/***********************************************************************
 *           GDI_Bezier   [INTERNAL]
 *   Calculate line segments that approximate -what microsoft calls- a bezier
 *   curve.
 *   The routine recursively divides the curve in two parts until a straight
 *   line can be drawn
 *
 *  PARAMS
 *
 *  Points  [I] Ptr to count POINTs which are the end and control points
 *              of the set of Bezier curves to flatten.
 *  count   [I] Number of Points.  Must be 3n+1.
 *  nPtsOut [O] Will contain no of points that have been produced (i.e. no. of
 *              lines+1).
 *
 *  RETURNS
 *
 *  Ptr to an array of POINTs that contain the lines that approximate the
 *  Beziers.  The array is allocated on the process heap and it is the caller's
 *  responsibility to HeapFree it. [this is not a particularly nice interface
 *  but since we can't know in advance how many points we will generate, the
 *  alternative would be to call the function twice, once to determine the size
 *  and a second time to do the work - I decided this was too much of a pain].
 */
POINT *GDI_Bezier( const POINT *Points, INT count, INT *nPtsOut )
{
    POINT *out;
    INT Bezier, dwOut = BEZIER_INITBUFSIZE, i;

    if (count == 1 || (count - 1) % 3 != 0) {
        ERR("Invalid no. of points %d\n", count);
	return NULL;
    }
    *nPtsOut = 0;
    out = HeapAlloc( GetProcessHeap(), 0, dwOut * sizeof(POINT));
    for(Bezier = 0; Bezier < (count-1)/3; Bezier++) {
	POINT ptBuf[4];
	memcpy(ptBuf, Points + Bezier * 3, sizeof(POINT) * 4);
	for(i = 0; i < 4; i++) {
	    ptBuf[i].x = BEZIERSHIFTUP(ptBuf[i].x);
	    ptBuf[i].y = BEZIERSHIFTUP(ptBuf[i].y);
	}
        GDI_InternalBezier( ptBuf, &out, &dwOut, nPtsOut, BEZIERMAXDEPTH );
    }
    TRACE("Produced %d points\n", *nPtsOut);
    return out;
}

/******************************************************************************
 *           GdiGradientFill   (GDI32.@)
 *
 *  FIXME: we don't support the Alpha channel properly
 */
BOOL WINAPI GdiGradientFill( HDC hdc, TRIVERTEX *vert_array, ULONG nvert,
                          void * grad_array, ULONG ngrad, ULONG mode )
{
  unsigned int i;

  TRACE("vert_array:%p nvert:%d grad_array:%p ngrad:%d\n",
        vert_array, nvert, grad_array, ngrad);

  switch(mode) 
    {
    case GRADIENT_FILL_RECT_H:
      for(i = 0; i < ngrad; i++) 
        {
          GRADIENT_RECT *rect = ((GRADIENT_RECT *)grad_array) + i;
          TRIVERTEX *v1 = vert_array + rect->UpperLeft;
          TRIVERTEX *v2 = vert_array + rect->LowerRight;
          int y1 = v1->y < v2->y ? v1->y : v2->y;
          int y2 = v2->y > v1->y ? v2->y : v1->y;
          int x, dx;
          if (v1->x > v2->x)
            {
              TRIVERTEX *t = v2;
              v2 = v1;
              v1 = t;
            }
          dx = v2->x - v1->x;
          for (x = 0; x < dx; x++)
            {
              POINT pts[2];
              HPEN hPen, hOldPen;
              
              hPen = CreatePen( PS_SOLID, 1, RGB(
                  (v1->Red   * (dx - x) + v2->Red   * x) / dx >> 8,
                  (v1->Green * (dx - x) + v2->Green * x) / dx >> 8,
                  (v1->Blue  * (dx - x) + v2->Blue  * x) / dx >> 8));
              hOldPen = SelectObject( hdc, hPen );
              pts[0].x = v1->x + x;
              pts[0].y = y1;
              pts[1].x = v1->x + x;
              pts[1].y = y2;
              Polyline( hdc, &pts[0], 2 );
              DeleteObject( SelectObject(hdc, hOldPen ) );
            }
        }
      break;
    case GRADIENT_FILL_RECT_V:
      for(i = 0; i < ngrad; i++) 
        {
          GRADIENT_RECT *rect = ((GRADIENT_RECT *)grad_array) + i;
          TRIVERTEX *v1 = vert_array + rect->UpperLeft;
          TRIVERTEX *v2 = vert_array + rect->LowerRight;
          int x1 = v1->x < v2->x ? v1->x : v2->x;
          int x2 = v2->x > v1->x ? v2->x : v1->x;
          int y, dy;
          if (v1->y > v2->y)
            {
              TRIVERTEX *t = v2;
              v2 = v1;
              v1 = t;
            }
          dy = v2->y - v1->y;
          for (y = 0; y < dy; y++)
            {
              POINT pts[2];
              HPEN hPen, hOldPen;
              
              hPen = CreatePen( PS_SOLID, 1, RGB(
                  (v1->Red   * (dy - y) + v2->Red   * y) / dy >> 8,
                  (v1->Green * (dy - y) + v2->Green * y) / dy >> 8,
                  (v1->Blue  * (dy - y) + v2->Blue  * y) / dy >> 8));
              hOldPen = SelectObject( hdc, hPen );
              pts[0].x = x1;
              pts[0].y = v1->y + y;
              pts[1].x = x2;
              pts[1].y = v1->y + y;
              Polyline( hdc, &pts[0], 2 );
              DeleteObject( SelectObject(hdc, hOldPen ) );
            }
        }
      break;
    case GRADIENT_FILL_TRIANGLE:
      for (i = 0; i < ngrad; i++)  
        {
          GRADIENT_TRIANGLE *tri = ((GRADIENT_TRIANGLE *)grad_array) + i;
          TRIVERTEX *v1 = vert_array + tri->Vertex1;
          TRIVERTEX *v2 = vert_array + tri->Vertex2;
          TRIVERTEX *v3 = vert_array + tri->Vertex3;
          int y, dy;
          
          if (v1->y > v2->y)
            { TRIVERTEX *t = v1; v1 = v2; v2 = t; }
          if (v2->y > v3->y)
            {
              TRIVERTEX *t = v2; v2 = v3; v3 = t;
              if (v1->y > v2->y)
                { t = v1; v1 = v2; v2 = t; }
            }
          /* v1->y <= v2->y <= v3->y */

          dy = v3->y - v1->y;
          for (y = 0; y < dy; y++)
            {
              /* v1->y <= y < v3->y */
              TRIVERTEX *v = y < (v2->y - v1->y) ? v1 : v3;
              /* (v->y <= y < v2->y) || (v2->y <= y < v->y) */
              int dy2 = v2->y - v->y;
              int y2 = y + v1->y - v->y;

              int x1 = (v3->x     * y  + v1->x     * (dy  - y )) / dy;
              int x2 = (v2->x     * y2 + v-> x     * (dy2 - y2)) / dy2;
              int r1 = (v3->Red   * y  + v1->Red   * (dy  - y )) / dy;
              int r2 = (v2->Red   * y2 + v-> Red   * (dy2 - y2)) / dy2;
              int g1 = (v3->Green * y  + v1->Green * (dy  - y )) / dy;
              int g2 = (v2->Green * y2 + v-> Green * (dy2 - y2)) / dy2;
              int b1 = (v3->Blue  * y  + v1->Blue  * (dy  - y )) / dy;
              int b2 = (v2->Blue  * y2 + v-> Blue  * (dy2 - y2)) / dy2;
               
              int x;
	      if (x1 < x2)
                {
                  int dx = x2 - x1;
                  for (x = 0; x < dx; x++)
                    SetPixel (hdc, x + x1, y + v1->y, RGB(
                      (r1 * (dx - x) + r2 * x) / dx >> 8,
                      (g1 * (dx - x) + g2 * x) / dx >> 8,
                      (b1 * (dx - x) + b2 * x) / dx >> 8));
                }
              else
                {
                  int dx = x1 - x2;
                  for (x = 0; x < dx; x++)
                    SetPixel (hdc, x + x2, y + v1->y, RGB(
                      (r2 * (dx - x) + r1 * x) / dx >> 8,
                      (g2 * (dx - x) + g1 * x) / dx >> 8,
                      (b2 * (dx - x) + b1 * x) / dx >> 8));
                }
            }
        }
      break;
    default:
      return FALSE;
  }

  return TRUE;
}
