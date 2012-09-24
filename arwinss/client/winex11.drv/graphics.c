/*
 * X11 graphics driver graphics functions
 *
 * Copyright 1993,1994 Alexandre Julliard
 * Copyright 1998 Huw Davies
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

/*
 * FIXME: only some of these functions obey the GM_ADVANCED
 * graphics mode
 */

#include "config.h"

#include <stdarg.h>
#include <math.h>
#ifdef HAVE_FLOAT_H
# include <float.h>
#endif
#include <stdlib.h>
#ifndef PI
#define PI M_PI
#endif
#include <string.h>
#include <limits.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"

#include "x11drv.h"
#include "x11font.h"
#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(graphics);

#define ABS(x)    ((x)<0?(-(x)):(x))

  /* ROP code to GC function conversion */
const int X11DRV_XROPfunction[16] =
{
    GXclear,        /* R2_BLACK */
    GXnor,          /* R2_NOTMERGEPEN */
    GXandInverted,  /* R2_MASKNOTPEN */
    GXcopyInverted, /* R2_NOTCOPYPEN */
    GXandReverse,   /* R2_MASKPENNOT */
    GXinvert,       /* R2_NOT */
    GXxor,          /* R2_XORPEN */
    GXnand,         /* R2_NOTMASKPEN */
    GXand,          /* R2_MASKPEN */
    GXequiv,        /* R2_NOTXORPEN */
    GXnoop,         /* R2_NOP */
    GXorInverted,   /* R2_MERGENOTPEN */
    GXcopy,         /* R2_COPYPEN */
    GXorReverse,    /* R2_MERGEPENNOT */
    GXor,           /* R2_MERGEPEN */
    GXset           /* R2_WHITE */
};


/* get the rectangle in device coordinates, with optional mirroring */
static RECT get_device_rect( HDC hdc, int left, int top, int right, int bottom )
{
    RECT rect;

    rect.left   = left;
    rect.top    = top;
    rect.right  = right;
    rect.bottom = bottom;
    if (GetLayout( hdc ) & LAYOUT_RTL)
    {
        /* shift the rectangle so that the right border is included after mirroring */
        /* it would be more correct to do this after LPtoDP but that's not what Windows does */
        rect.left--;
        rect.right--;
    }
    LPtoDP( hdc, (POINT *)&rect, 2 );
    if (rect.left > rect.right)
    {
        int tmp = rect.left;
        rect.left = rect.right;
        rect.right = tmp;
    }
    if (rect.top > rect.bottom)
    {
        int tmp = rect.top;
        rect.top = rect.bottom;
        rect.bottom = tmp;
    }
    return rect;
}

/***********************************************************************
 *           X11DRV_GetRegionData
 *
 * Calls GetRegionData on the given region and converts the rectangle
 * array to XRectangle format. The returned buffer must be freed by
 * caller using HeapFree(GetProcessHeap(),...).
 * If hdc_lptodp is not 0, the rectangles are converted through LPtoDP.
 */
RGNDATA *X11DRV_GetRegionData( HRGN hrgn, HDC hdc_lptodp )
{
    RGNDATA *data;
    DWORD size;
    unsigned int i;
    RECT *rect, tmp;
    XRectangle *xrect;

    if (!(size = GetRegionData( hrgn, 0, NULL ))) return NULL;
    if (sizeof(XRectangle) > sizeof(RECT))
    {
        /* add extra size for XRectangle array */
        int count = (size - sizeof(RGNDATAHEADER)) / sizeof(RECT);
        size += count * (sizeof(XRectangle) - sizeof(RECT));
    }
    if (!(data = HeapAlloc( GetProcessHeap(), 0, size ))) return NULL;
    if (!GetRegionData( hrgn, size, data ))
    {
        HeapFree( GetProcessHeap(), 0, data );
        return NULL;
    }

    rect = (RECT *)data->Buffer;
    xrect = (XRectangle *)data->Buffer;
    if (hdc_lptodp)  /* map to device coordinates */
    {
        LPtoDP( hdc_lptodp, (POINT *)rect, data->rdh.nCount * 2 );
        for (i = 0; i < data->rdh.nCount; i++)
        {
            if (rect[i].right < rect[i].left)
            {
                INT tmp = rect[i].right;
                rect[i].right = rect[i].left;
                rect[i].left = tmp;
            }
            if (rect[i].bottom < rect[i].top)
            {
                INT tmp = rect[i].bottom;
                rect[i].bottom = rect[i].top;
                rect[i].top = tmp;
            }
        }
    }

    if (sizeof(XRectangle) > sizeof(RECT))
    {
        int j;
        /* need to start from the end */
        for (j = data->rdh.nCount-1; j >= 0; j--)
        {
            tmp = rect[j];
            xrect[j].x      = max( min( tmp.left, SHRT_MAX), SHRT_MIN);
            xrect[j].y      = max( min( tmp.top, SHRT_MAX), SHRT_MIN);
            xrect[j].width  = max( min( tmp.right - xrect[j].x, USHRT_MAX), 0);
            xrect[j].height = max( min( tmp.bottom - xrect[j].y, USHRT_MAX), 0);
        }
    }
    else
    {
        for (i = 0; i < data->rdh.nCount; i++)
        {
            tmp = rect[i];
            xrect[i].x      = max( min( tmp.left, SHRT_MAX), SHRT_MIN);
            xrect[i].y      = max( min( tmp.top, SHRT_MAX), SHRT_MIN);
            xrect[i].width  = max( min( tmp.right - xrect[i].x, USHRT_MAX), 0);
            xrect[i].height = max( min( tmp.bottom - xrect[i].y, USHRT_MAX), 0);
        }
    }
    return data;
}


/***********************************************************************
 *           X11DRV_SetDeviceClipping
 */
void CDECL X11DRV_SetDeviceClipping( X11DRV_PDEVICE *physDev, HRGN vis_rgn, HRGN clip_rgn )
{
    RGNDATA *data;

    CombineRgn( physDev->region, vis_rgn, clip_rgn, clip_rgn ? RGN_AND : RGN_COPY );
    if (!(data = X11DRV_GetRegionData( physDev->region, 0 ))) return;

    wine_tsx11_lock();
    XSetClipRectangles( gdi_display, physDev->gc, physDev->dc_rect.left, physDev->dc_rect.top,
                        (XRectangle *)data->Buffer, data->rdh.nCount, YXBanded );
    wine_tsx11_unlock();

    if (physDev->xrender) X11DRV_XRender_SetDeviceClipping(physDev, data);

    HeapFree( GetProcessHeap(), 0, data );
}


/***********************************************************************
 *           X11DRV_SetupGCForPatBlt
 *
 * Setup the GC for a PatBlt operation using current brush.
 * If fMapColors is TRUE, X pixels are mapped to Windows colors.
 * Return FALSE if brush is BS_NULL, TRUE otherwise.
 */
BOOL X11DRV_SetupGCForPatBlt( X11DRV_PDEVICE *physDev, GC gc, BOOL fMapColors )
{
    XGCValues val;
    unsigned long mask;
    Pixmap pixmap = 0;
    POINT pt;

    if (physDev->brush.style == BS_NULL) return FALSE;
    if (physDev->brush.pixel == -1)
    {
	/* Special case used for monochrome pattern brushes.
	 * We need to swap foreground and background because
	 * Windows does it the wrong way...
	 */
	val.foreground = physDev->backgroundPixel;
	val.background = physDev->textPixel;
    }
    else
    {
	val.foreground = physDev->brush.pixel;
	val.background = physDev->backgroundPixel;
    }
    if (fMapColors && X11DRV_PALETTE_XPixelToPalette)
    {
        val.foreground = X11DRV_PALETTE_XPixelToPalette[val.foreground];
        val.background = X11DRV_PALETTE_XPixelToPalette[val.background];
    }

    val.function = X11DRV_XROPfunction[GetROP2(physDev->hdc)-1];
    /*
    ** Let's replace GXinvert by GXxor with (black xor white)
    ** This solves the selection color and leak problems in excel
    ** FIXME : Let's do that only if we work with X-pixels, not with Win-pixels
    */
    if (val.function == GXinvert)
    {
        val.foreground = (WhitePixel( gdi_display, DefaultScreen(gdi_display) ) ^
                          BlackPixel( gdi_display, DefaultScreen(gdi_display) ));
	val.function = GXxor;
    }
    val.fill_style = physDev->brush.fillStyle;
    switch(val.fill_style)
    {
    case FillStippled:
    case FillOpaqueStippled:
	if (GetBkMode(physDev->hdc)==OPAQUE) val.fill_style = FillOpaqueStippled;
	val.stipple = physDev->brush.pixmap;
	mask = GCStipple;
        break;

    case FillTiled:
        if (fMapColors && X11DRV_PALETTE_XPixelToPalette)
        {
            register int x, y;
            XImage *image;
            wine_tsx11_lock();
            pixmap = XCreatePixmap( gdi_display, root_window, 8, 8, physDev->depth );
            image = XGetImage( gdi_display, physDev->brush.pixmap, 0, 0, 8, 8,
                               AllPlanes, ZPixmap );
            for (y = 0; y < 8; y++)
                for (x = 0; x < 8; x++)
                    XPutPixel( image, x, y,
                               X11DRV_PALETTE_XPixelToPalette[XGetPixel( image, x, y)] );
            XPutImage( gdi_display, pixmap, gc, image, 0, 0, 0, 0, 8, 8 );
            XDestroyImage( image );
            wine_tsx11_unlock();
            val.tile = pixmap;
        }
        else val.tile = physDev->brush.pixmap;
	mask = GCTile;
        break;

    default:
        mask = 0;
        break;
    }
    GetBrushOrgEx( physDev->hdc, &pt );
    val.ts_x_origin = physDev->dc_rect.left + pt.x;
    val.ts_y_origin = physDev->dc_rect.top + pt.y;
    val.fill_rule = (GetPolyFillMode(physDev->hdc) == WINDING) ? WindingRule : EvenOddRule;
    wine_tsx11_lock();
    XChangeGC( gdi_display, gc,
	       GCFunction | GCForeground | GCBackground | GCFillStyle |
	       GCFillRule | GCTileStipXOrigin | GCTileStipYOrigin | mask,
	       &val );
    if (pixmap) XFreePixmap( gdi_display, pixmap );
    wine_tsx11_unlock();
    return TRUE;
}


/***********************************************************************
 *           X11DRV_SetupGCForBrush
 *
 * Setup physDev->gc for drawing operations using current brush.
 * Return FALSE if brush is BS_NULL, TRUE otherwise.
 */
BOOL X11DRV_SetupGCForBrush( X11DRV_PDEVICE *physDev )
{
    return X11DRV_SetupGCForPatBlt( physDev, physDev->gc, FALSE );
}


/***********************************************************************
 *           X11DRV_SetupGCForPen
 *
 * Setup physDev->gc for drawing operations using current pen.
 * Return FALSE if pen is PS_NULL, TRUE otherwise.
 */
static BOOL X11DRV_SetupGCForPen( X11DRV_PDEVICE *physDev )
{
    XGCValues val;
    UINT rop2 = GetROP2(physDev->hdc);

    if (physDev->pen.style == PS_NULL) return FALSE;

    switch (rop2)
    {
    case R2_BLACK :
        val.foreground = BlackPixel( gdi_display, DefaultScreen(gdi_display) );
	val.function = GXcopy;
	break;
    case R2_WHITE :
        val.foreground = WhitePixel( gdi_display, DefaultScreen(gdi_display) );
	val.function = GXcopy;
	break;
    case R2_XORPEN :
	val.foreground = physDev->pen.pixel;
	/* It is very unlikely someone wants to XOR with 0 */
	/* This fixes the rubber-drawings in paintbrush */
	if (val.foreground == 0)
            val.foreground = (WhitePixel( gdi_display, DefaultScreen(gdi_display) ) ^
                              BlackPixel( gdi_display, DefaultScreen(gdi_display) ));
	val.function = GXxor;
	break;
    default :
	val.foreground = physDev->pen.pixel;
	val.function   = X11DRV_XROPfunction[rop2-1];
    }
    val.background = physDev->backgroundPixel;
    val.fill_style = FillSolid;
    val.line_width = physDev->pen.width;
    if (val.line_width <= 1) {
	val.cap_style = CapNotLast;
    } else {
	switch (physDev->pen.endcap)
	{
	case PS_ENDCAP_SQUARE:
	    val.cap_style = CapProjecting;
	    break;
	case PS_ENDCAP_FLAT:
	    val.cap_style = CapButt;
	    break;
	case PS_ENDCAP_ROUND:
	default:
	    val.cap_style = CapRound;
	}
    }
    switch (physDev->pen.linejoin)
    {
    case PS_JOIN_BEVEL:
	val.join_style = JoinBevel;
        break;
    case PS_JOIN_MITER:
	val.join_style = JoinMiter;
        break;
    case PS_JOIN_ROUND:
    default:
	val.join_style = JoinRound;
    }

    if (physDev->pen.dash_len)
        val.line_style = ((GetBkMode(physDev->hdc) == OPAQUE) && (!physDev->pen.ext))
                         ? LineDoubleDash : LineOnOffDash;
    else
        val.line_style = LineSolid;

    wine_tsx11_lock();
    if (physDev->pen.dash_len)
        XSetDashes( gdi_display, physDev->gc, 0, physDev->pen.dashes, physDev->pen.dash_len );
    XChangeGC( gdi_display, physDev->gc,
	       GCFunction | GCForeground | GCBackground | GCLineWidth |
	       GCLineStyle | GCCapStyle | GCJoinStyle | GCFillStyle, &val );
    wine_tsx11_unlock();
    return TRUE;
}


/***********************************************************************
 *           X11DRV_SetupGCForText
 *
 * Setup physDev->gc for text drawing operations.
 * Return FALSE if the font is null, TRUE otherwise.
 */
BOOL X11DRV_SetupGCForText( X11DRV_PDEVICE *physDev )
{
    XFontStruct* xfs = XFONT_GetFontStruct( physDev->font );

    if( xfs )
    {
	XGCValues val;

	val.function   = GXcopy;  /* Text is always GXcopy */
	val.foreground = physDev->textPixel;
	val.background = physDev->backgroundPixel;
	val.fill_style = FillSolid;
	val.font       = xfs->fid;

        wine_tsx11_lock();
        XChangeGC( gdi_display, physDev->gc,
		   GCFunction | GCForeground | GCBackground | GCFillStyle |
		   GCFont, &val );
        wine_tsx11_unlock();
	return TRUE;
    }
    WARN("Physical font failure\n" );
    return FALSE;
}

/***********************************************************************
 *           X11DRV_XWStoDS
 *
 * Performs a world-to-viewport transformation on the specified width.
 */
INT X11DRV_XWStoDS( X11DRV_PDEVICE *physDev, INT width )
{
    POINT pt[2];

    pt[0].x = 0;
    pt[0].y = 0;
    pt[1].x = width;
    pt[1].y = 0;
    LPtoDP( physDev->hdc, pt, 2 );
    return pt[1].x - pt[0].x;
}

/***********************************************************************
 *           X11DRV_YWStoDS
 *
 * Performs a world-to-viewport transformation on the specified height.
 */
INT X11DRV_YWStoDS( X11DRV_PDEVICE *physDev, INT height )
{
    POINT pt[2];

    pt[0].x = 0;
    pt[0].y = 0;
    pt[1].x = 0;
    pt[1].y = height;
    LPtoDP( physDev->hdc, pt, 2 );
    return pt[1].y - pt[0].y;
}

/***********************************************************************
 *           X11DRV_LineTo
 */
BOOL CDECL
X11DRV_LineTo( X11DRV_PDEVICE *physDev, INT x, INT y )
{
    POINT pt[2];

    if (X11DRV_SetupGCForPen( physDev )) {
	/* Update the pixmap from the DIB section */
	X11DRV_LockDIBSection(physDev, DIB_Status_GdiMod);

        GetCurrentPositionEx( physDev->hdc, &pt[0] );
        pt[1].x = x;
        pt[1].y = y;
        LPtoDP( physDev->hdc, pt, 2 );

        wine_tsx11_lock();
        XDrawLine(gdi_display, physDev->drawable, physDev->gc,
                  physDev->dc_rect.left + pt[0].x, physDev->dc_rect.top + pt[0].y,
                  physDev->dc_rect.left + pt[1].x, physDev->dc_rect.top + pt[1].y );
        wine_tsx11_unlock();

	/* Update the DIBSection from the pixmap */
	X11DRV_UnlockDIBSection(physDev, TRUE);
    }
    return TRUE;
}



/***********************************************************************
 *           X11DRV_DrawArc
 *
 * Helper functions for Arc(), Chord() and Pie().
 * 'lines' is the number of lines to draw: 0 for Arc, 1 for Chord, 2 for Pie.
 *
 */
static BOOL
X11DRV_DrawArc( X11DRV_PDEVICE *physDev, INT left, INT top, INT right,
                INT bottom, INT xstart, INT ystart,
                INT xend, INT yend, INT lines )
{
    INT xcenter, ycenter, istart_angle, idiff_angle;
    INT width, oldwidth;
    double start_angle, end_angle;
    XPoint points[4];
    BOOL update = FALSE;
    POINT start, end;
    RECT rc = get_device_rect( physDev->hdc, left, top, right, bottom );

    start.x = xstart;
    start.y = ystart;
    end.x = xend;
    end.y = yend;
    LPtoDP(physDev->hdc, &start, 1);
    LPtoDP(physDev->hdc, &end, 1);

    if ((rc.left == rc.right) || (rc.top == rc.bottom)
            ||(lines && ((rc.right-rc.left==1)||(rc.bottom-rc.top==1)))) return TRUE;

    if (GetArcDirection( physDev->hdc ) == AD_CLOCKWISE)
      { POINT tmp = start; start = end; end = tmp; }

    oldwidth = width = physDev->pen.width;
    if (!width) width = 1;
    if(physDev->pen.style == PS_NULL) width = 0;

    if ((physDev->pen.style == PS_INSIDEFRAME))
    {
        if (2*width > (rc.right-rc.left)) width=(rc.right-rc.left + 1)/2;
        if (2*width > (rc.bottom-rc.top)) width=(rc.bottom-rc.top + 1)/2;
        rc.left   += width / 2;
        rc.right  -= (width - 1) / 2;
        rc.top    += width / 2;
        rc.bottom -= (width - 1) / 2;
    }
    if(width == 0) width = 1; /* more accurate */
    physDev->pen.width = width;

    xcenter = (rc.right + rc.left) / 2;
    ycenter = (rc.bottom + rc.top) / 2;
    start_angle = atan2( (double)(ycenter-start.y)*(rc.right-rc.left),
			 (double)(start.x-xcenter)*(rc.bottom-rc.top) );
    end_angle   = atan2( (double)(ycenter-end.y)*(rc.right-rc.left),
			 (double)(end.x-xcenter)*(rc.bottom-rc.top) );
    if ((start.x==end.x)&&(start.y==end.y))
      { /* A lazy program delivers xstart=xend=ystart=yend=0) */
	start_angle = 0;
	end_angle = 2* PI;
      }
    else /* notorious cases */
      if ((start_angle == PI)&&( end_angle <0))
	start_angle = - PI;
    else
      if ((end_angle == PI)&&( start_angle <0))
	end_angle = - PI;
    istart_angle = (INT)(start_angle * 180 * 64 / PI + 0.5);
    idiff_angle  = (INT)((end_angle - start_angle) * 180 * 64 / PI + 0.5);
    if (idiff_angle <= 0) idiff_angle += 360 * 64;

    /* Update the pixmap from the DIB section */
    X11DRV_LockDIBSection(physDev, DIB_Status_GdiMod);

      /* Fill arc with brush if Chord() or Pie() */

    if ((lines > 0) && X11DRV_SetupGCForBrush( physDev )) {
        wine_tsx11_lock();
        XSetArcMode( gdi_display, physDev->gc, (lines==1) ? ArcChord : ArcPieSlice);
        XFillArc( gdi_display, physDev->drawable, physDev->gc,
                  physDev->dc_rect.left + rc.left, physDev->dc_rect.top + rc.top,
                  rc.right-rc.left-1, rc.bottom-rc.top-1, istart_angle, idiff_angle );
        wine_tsx11_unlock();
	update = TRUE;
    }

      /* Draw arc and lines */

    if (X11DRV_SetupGCForPen( physDev ))
    {
        wine_tsx11_lock();
        XDrawArc( gdi_display, physDev->drawable, physDev->gc,
                  physDev->dc_rect.left + rc.left, physDev->dc_rect.top + rc.top,
                  rc.right-rc.left-1, rc.bottom-rc.top-1, istart_angle, idiff_angle );
        if (lines) {
            /* use the truncated values */
            start_angle=(double)istart_angle*PI/64./180.;
            end_angle=(double)(istart_angle+idiff_angle)*PI/64./180.;
            /* calculate the endpoints and round correctly */
            points[0].x = (int) floor(physDev->dc_rect.left + (rc.right+rc.left)/2.0 +
                    cos(start_angle) * (rc.right-rc.left-width*2+2) / 2. + 0.5);
            points[0].y = (int) floor(physDev->dc_rect.top + (rc.top+rc.bottom)/2.0 -
                    sin(start_angle) * (rc.bottom-rc.top-width*2+2) / 2. + 0.5);
            points[1].x = (int) floor(physDev->dc_rect.left + (rc.right+rc.left)/2.0 +
                    cos(end_angle) * (rc.right-rc.left-width*2+2) / 2. + 0.5);
            points[1].y = (int) floor(physDev->dc_rect.top + (rc.top+rc.bottom)/2.0 -
                    sin(end_angle) * (rc.bottom-rc.top-width*2+2) / 2. + 0.5);

            /* OK, this stuff is optimized for Xfree86
             * which is probably the server most used by
             * wine users. Other X servers will not
             * display correctly. (eXceed for instance)
             * so if you feel you must make changes, make sure that
             * you either use Xfree86 or separate your changes
             * from these (compile switch or whatever)
             */
            if (lines == 2) {
                INT dx1,dy1;
                points[3] = points[1];
                points[1].x = physDev->dc_rect.left + xcenter;
                points[1].y = physDev->dc_rect.top + ycenter;
                points[2] = points[1];
                dx1=points[1].x-points[0].x;
                dy1=points[1].y-points[0].y;
                if(((rc.top-rc.bottom) | -2) == -2)
                    if(dy1>0) points[1].y--;
                if(dx1<0) {
                    if (((-dx1)*64)<=ABS(dy1)*37) points[0].x--;
                    if(((-dx1*9))<(dy1*16)) points[0].y--;
                    if( dy1<0 && ((dx1*9)) < (dy1*16)) points[0].y--;
                } else {
                    if(dy1 < 0)  points[0].y--;
                    if(((rc.right-rc.left) | -2) == -2) points[1].x--;
                }
                dx1=points[3].x-points[2].x;
                dy1=points[3].y-points[2].y;
                if(((rc.top-rc.bottom) | -2 ) == -2)
                    if(dy1 < 0) points[2].y--;
                if( dx1<0){
                    if( dy1>0) points[3].y--;
                    if(((rc.right-rc.left) | -2) == -2 ) points[2].x--;
                }else {
                    points[3].y--;
                    if( dx1 * 64 < dy1 * -37 ) points[3].x--;
                }
                lines++;
	    }
            XDrawLines( gdi_display, physDev->drawable, physDev->gc,
                        points, lines+1, CoordModeOrigin );
        }
        wine_tsx11_unlock();
	update = TRUE;
    }

    /* Update the DIBSection of the pixmap */
    X11DRV_UnlockDIBSection(physDev, update);

    physDev->pen.width = oldwidth;
    return TRUE;
}


/***********************************************************************
 *           X11DRV_Arc
 */
BOOL CDECL
X11DRV_Arc( X11DRV_PDEVICE *physDev, INT left, INT top, INT right, INT bottom,
            INT xstart, INT ystart, INT xend, INT yend )
{
    return X11DRV_DrawArc( physDev, left, top, right, bottom,
			   xstart, ystart, xend, yend, 0 );
}


/***********************************************************************
 *           X11DRV_Pie
 */
BOOL CDECL
X11DRV_Pie( X11DRV_PDEVICE *physDev, INT left, INT top, INT right, INT bottom,
            INT xstart, INT ystart, INT xend, INT yend )
{
    return X11DRV_DrawArc( physDev, left, top, right, bottom,
			   xstart, ystart, xend, yend, 2 );
}

/***********************************************************************
 *           X11DRV_Chord
 */
BOOL CDECL
X11DRV_Chord( X11DRV_PDEVICE *physDev, INT left, INT top, INT right, INT bottom,
              INT xstart, INT ystart, INT xend, INT yend )
{
    return X11DRV_DrawArc( physDev, left, top, right, bottom,
		  	   xstart, ystart, xend, yend, 1 );
}


/***********************************************************************
 *           X11DRV_Ellipse
 */
BOOL CDECL
X11DRV_Ellipse( X11DRV_PDEVICE *physDev, INT left, INT top, INT right, INT bottom )
{
    INT width, oldwidth;
    BOOL update = FALSE;
    RECT rc = get_device_rect( physDev->hdc, left, top, right, bottom );

    if ((rc.left == rc.right) || (rc.top == rc.bottom)) return TRUE;

    oldwidth = width = physDev->pen.width;
    if (!width) width = 1;
    if(physDev->pen.style == PS_NULL) width = 0;

    if ((physDev->pen.style == PS_INSIDEFRAME))
    {
        if (2*width > (rc.right-rc.left)) width=(rc.right-rc.left + 1)/2;
        if (2*width > (rc.bottom-rc.top)) width=(rc.bottom-rc.top + 1)/2;
        rc.left   += width / 2;
        rc.right  -= (width - 1) / 2;
        rc.top    += width / 2;
        rc.bottom -= (width - 1) / 2;
    }
    if(width == 0) width = 1; /* more accurate */
    physDev->pen.width = width;

    /* Update the pixmap from the DIB section */
    X11DRV_LockDIBSection(physDev, DIB_Status_GdiMod);

    if (X11DRV_SetupGCForBrush( physDev ))
    {
        wine_tsx11_lock();
        XFillArc( gdi_display, physDev->drawable, physDev->gc,
                  physDev->dc_rect.left + rc.left, physDev->dc_rect.top + rc.top,
                  rc.right-rc.left-1, rc.bottom-rc.top-1, 0, 360*64 );
        wine_tsx11_unlock();
	update = TRUE;
    }
    if (X11DRV_SetupGCForPen( physDev ))
    {
        wine_tsx11_lock();
        XDrawArc( gdi_display, physDev->drawable, physDev->gc,
                  physDev->dc_rect.left + rc.left, physDev->dc_rect.top + rc.top,
                  rc.right-rc.left-1, rc.bottom-rc.top-1, 0, 360*64 );
        wine_tsx11_unlock();
	update = TRUE;
    }

    /* Update the DIBSection from the pixmap */
    X11DRV_UnlockDIBSection(physDev, update);

    physDev->pen.width = oldwidth;
    return TRUE;
}


/***********************************************************************
 *           X11DRV_Rectangle
 */
BOOL CDECL
X11DRV_Rectangle(X11DRV_PDEVICE *physDev, INT left, INT top, INT right, INT bottom)
{
    INT width, oldwidth, oldjoinstyle;
    BOOL update = FALSE;
    RECT rc = get_device_rect( physDev->hdc, left, top, right, bottom );

    TRACE("(%d %d %d %d)\n", left, top, right, bottom);

    if ((rc.left == rc.right) || (rc.top == rc.bottom)) return TRUE;

    oldwidth = width = physDev->pen.width;
    if (!width) width = 1;
    if(physDev->pen.style == PS_NULL) width = 0;

    if ((physDev->pen.style == PS_INSIDEFRAME))
    {
        if (2*width > (rc.right-rc.left)) width=(rc.right-rc.left + 1)/2;
        if (2*width > (rc.bottom-rc.top)) width=(rc.bottom-rc.top + 1)/2;
        rc.left   += width / 2;
        rc.right  -= (width - 1) / 2;
        rc.top    += width / 2;
        rc.bottom -= (width - 1) / 2;
    }
    if(width == 1) width = 0;
    physDev->pen.width = width;
    oldjoinstyle = physDev->pen.linejoin;
    if(physDev->pen.type != PS_GEOMETRIC)
        physDev->pen.linejoin = PS_JOIN_MITER;

    /* Update the pixmap from the DIB section */
    X11DRV_LockDIBSection(physDev, DIB_Status_GdiMod);

    if ((rc.right > rc.left + width) && (rc.bottom > rc.top + width))
    {
        if (X11DRV_SetupGCForBrush( physDev ))
	{
            wine_tsx11_lock();
            XFillRectangle( gdi_display, physDev->drawable, physDev->gc,
                            physDev->dc_rect.left + rc.left + (width + 1) / 2,
                            physDev->dc_rect.top + rc.top + (width + 1) / 2,
                            rc.right-rc.left-width-1, rc.bottom-rc.top-width-1);
            wine_tsx11_unlock();
	    update = TRUE;
	}
    }
    if (X11DRV_SetupGCForPen( physDev ))
    {
        wine_tsx11_lock();
        XDrawRectangle( gdi_display, physDev->drawable, physDev->gc,
                        physDev->dc_rect.left + rc.left, physDev->dc_rect.top + rc.top,
                        rc.right-rc.left-1, rc.bottom-rc.top-1 );
        wine_tsx11_unlock();
	update = TRUE;
    }

    /* Update the DIBSection from the pixmap */
    X11DRV_UnlockDIBSection(physDev, update);

    physDev->pen.width = oldwidth;
    physDev->pen.linejoin = oldjoinstyle;
    return TRUE;
}

/***********************************************************************
 *           X11DRV_RoundRect
 */
BOOL CDECL
X11DRV_RoundRect( X11DRV_PDEVICE *physDev, INT left, INT top, INT right,
                  INT bottom, INT ell_width, INT ell_height )
{
    INT width, oldwidth, oldendcap;
    BOOL update = FALSE;
    POINT pts[2];
    RECT rc = get_device_rect( physDev->hdc, left, top, right, bottom );

    TRACE("(%d %d %d %d  %d %d\n",
    	left, top, right, bottom, ell_width, ell_height);

    if ((rc.left == rc.right) || (rc.top == rc.bottom))
	return TRUE;

    /* Make sure ell_width and ell_height are >= 1 otherwise XDrawArc gets
       called with width/height < 0 */
    pts[0].x = pts[0].y = 0;
    pts[1].x = ell_width;
    pts[1].y = ell_height;
    LPtoDP(physDev->hdc, pts, 2);
    ell_width  = max(abs( pts[1].x - pts[0].x ), 1);
    ell_height = max(abs( pts[1].y - pts[0].y ), 1);

    oldwidth = width = physDev->pen.width;
    oldendcap = physDev->pen.endcap;
    if (!width) width = 1;
    if(physDev->pen.style == PS_NULL) width = 0;

    if ((physDev->pen.style == PS_INSIDEFRAME))
    {
        if (2*width > (rc.right-rc.left)) width=(rc.right-rc.left + 1)/2;
        if (2*width > (rc.bottom-rc.top)) width=(rc.bottom-rc.top + 1)/2;
        rc.left   += width / 2;
        rc.right  -= (width - 1) / 2;
        rc.top    += width / 2;
        rc.bottom -= (width - 1) / 2;
    }
    if(width == 0) width = 1;
    physDev->pen.width = width;
    physDev->pen.endcap = PS_ENDCAP_SQUARE;

    /* Update the pixmap from the DIB section */
    X11DRV_LockDIBSection(physDev, DIB_Status_GdiMod);

    if (X11DRV_SetupGCForBrush( physDev ))
    {
        wine_tsx11_lock();
        if (ell_width > (rc.right-rc.left) )
            if (ell_height > (rc.bottom-rc.top) )
                XFillArc( gdi_display, physDev->drawable, physDev->gc,
                          physDev->dc_rect.left + rc.left, physDev->dc_rect.top + rc.top,
                          rc.right - rc.left - 1, rc.bottom - rc.top - 1,
                          0, 360 * 64 );
            else{
                XFillArc( gdi_display, physDev->drawable, physDev->gc,
                          physDev->dc_rect.left + rc.left, physDev->dc_rect.top + rc.top,
                          rc.right - rc.left - 1, ell_height, 0, 180 * 64 );
                XFillArc( gdi_display, physDev->drawable, physDev->gc,
                          physDev->dc_rect.left + rc.left,
                          physDev->dc_rect.top + rc.bottom - ell_height - 1,
                          rc.right - rc.left - 1, ell_height, 180 * 64,
                          180 * 64 );
            }
	else if (ell_height > (rc.bottom-rc.top) ){
            XFillArc( gdi_display, physDev->drawable, physDev->gc,
                      physDev->dc_rect.left + rc.left, physDev->dc_rect.top + rc.top,
                      ell_width, rc.bottom - rc.top - 1, 90 * 64, 180 * 64 );
            XFillArc( gdi_display, physDev->drawable, physDev->gc,
                      physDev->dc_rect.left + rc.right - ell_width - 1, physDev->dc_rect.top + rc.top,
                      ell_width, rc.bottom - rc.top - 1, 270 * 64, 180 * 64 );
        }else{
            XFillArc( gdi_display, physDev->drawable, physDev->gc,
                      physDev->dc_rect.left + rc.left, physDev->dc_rect.top + rc.top,
                      ell_width, ell_height, 90 * 64, 90 * 64 );
            XFillArc( gdi_display, physDev->drawable, physDev->gc,
                      physDev->dc_rect.left + rc.left,
                      physDev->dc_rect.top + rc.bottom - ell_height - 1,
                      ell_width, ell_height, 180 * 64, 90 * 64 );
            XFillArc( gdi_display, physDev->drawable, physDev->gc,
                      physDev->dc_rect.left + rc.right - ell_width - 1,
                      physDev->dc_rect.top + rc.bottom - ell_height - 1,
                      ell_width, ell_height, 270 * 64, 90 * 64 );
            XFillArc( gdi_display, physDev->drawable, physDev->gc,
                      physDev->dc_rect.left + rc.right - ell_width - 1,
                      physDev->dc_rect.top + rc.top,
                      ell_width, ell_height, 0, 90 * 64 );
        }
        if (ell_width < rc.right - rc.left)
        {
            XFillRectangle( gdi_display, physDev->drawable, physDev->gc,
                            physDev->dc_rect.left + rc.left + (ell_width + 1) / 2,
                            physDev->dc_rect.top + rc.top + 1,
                            rc.right - rc.left - ell_width - 1,
                            (ell_height + 1) / 2 - 1);
            XFillRectangle( gdi_display, physDev->drawable, physDev->gc,
                            physDev->dc_rect.left + rc.left + (ell_width + 1) / 2,
                            physDev->dc_rect.top + rc.bottom - (ell_height) / 2 - 1,
                            rc.right - rc.left - ell_width - 1,
                            (ell_height) / 2 );
        }
        if  (ell_height < rc.bottom - rc.top)
        {
            XFillRectangle( gdi_display, physDev->drawable, physDev->gc,
                            physDev->dc_rect.left + rc.left + 1,
                            physDev->dc_rect.top + rc.top + (ell_height + 1) / 2,
                            rc.right - rc.left - 2,
                            rc.bottom - rc.top - ell_height - 1);
        }
        wine_tsx11_unlock();
	update = TRUE;
    }
    /* FIXME: this could be done with on X call
     * more efficient and probably more correct
     * on any X server: XDrawArcs will draw
     * straight horizontal and vertical lines
     * if width or height are zero.
     *
     * BTW this stuff is optimized for an Xfree86 server
     * read the comments inside the X11DRV_DrawArc function
     */
    if (X11DRV_SetupGCForPen( physDev ))
    {
        wine_tsx11_lock();
        if (ell_width > (rc.right-rc.left) )
            if (ell_height > (rc.bottom-rc.top) )
                XDrawArc( gdi_display, physDev->drawable, physDev->gc,
                          physDev->dc_rect.left + rc.left, physDev->dc_rect.top + rc.top,
                          rc.right - rc.left - 1, rc.bottom - rc.top - 1, 0 , 360 * 64 );
            else{
                XDrawArc( gdi_display, physDev->drawable, physDev->gc,
                          physDev->dc_rect.left + rc.left, physDev->dc_rect.top + rc.top,
                          rc.right - rc.left - 1, ell_height - 1, 0 , 180 * 64 );
                XDrawArc( gdi_display, physDev->drawable, physDev->gc,
                          physDev->dc_rect.left + rc.left,
                          physDev->dc_rect.top + rc.bottom - ell_height,
                          rc.right - rc.left - 1, ell_height - 1, 180 * 64 , 180 * 64 );
            }
	else if (ell_height > (rc.bottom-rc.top) ){
            XDrawArc( gdi_display, physDev->drawable, physDev->gc,
                      physDev->dc_rect.left + rc.left, physDev->dc_rect.top + rc.top,
                      ell_width - 1 , rc.bottom - rc.top - 1, 90 * 64 , 180 * 64 );
            XDrawArc( gdi_display, physDev->drawable, physDev->gc,
                      physDev->dc_rect.left + rc.right - ell_width,
                      physDev->dc_rect.top + rc.top,
                      ell_width - 1 , rc.bottom - rc.top - 1, 270 * 64 , 180 * 64 );
	}else{
            XDrawArc( gdi_display, physDev->drawable, physDev->gc,
                      physDev->dc_rect.left + rc.left, physDev->dc_rect.top + rc.top,
                      ell_width - 1, ell_height - 1, 90 * 64, 90 * 64 );
            XDrawArc( gdi_display, physDev->drawable, physDev->gc,
                      physDev->dc_rect.left + rc.left, physDev->dc_rect.top + rc.bottom - ell_height,
                      ell_width - 1, ell_height - 1, 180 * 64, 90 * 64 );
            XDrawArc( gdi_display, physDev->drawable, physDev->gc,
                      physDev->dc_rect.left + rc.right - ell_width,
                      physDev->dc_rect.top + rc.bottom - ell_height,
                      ell_width - 1, ell_height - 1, 270 * 64, 90 * 64 );
            XDrawArc( gdi_display, physDev->drawable, physDev->gc,
                      physDev->dc_rect.left + rc.right - ell_width, physDev->dc_rect.top + rc.top,
                      ell_width - 1, ell_height - 1, 0, 90 * 64 );
	}
	if (ell_width < rc.right - rc.left)
	{
            XDrawLine( gdi_display, physDev->drawable, physDev->gc,
                       physDev->dc_rect.left + rc.left + ell_width / 2,
                       physDev->dc_rect.top + rc.top,
                       physDev->dc_rect.left + rc.right - (ell_width+1) / 2,
                       physDev->dc_rect.top + rc.top);
            XDrawLine( gdi_display, physDev->drawable, physDev->gc,
                       physDev->dc_rect.left + rc.left + ell_width / 2 ,
                       physDev->dc_rect.top + rc.bottom - 1,
                       physDev->dc_rect.left + rc.right - (ell_width+1)/ 2,
                       physDev->dc_rect.top + rc.bottom - 1);
	}
	if (ell_height < rc.bottom - rc.top)
	{
            XDrawLine( gdi_display, physDev->drawable, physDev->gc,
                       physDev->dc_rect.left + rc.right - 1,
                       physDev->dc_rect.top + rc.top + ell_height / 2,
                       physDev->dc_rect.left + rc.right - 1,
                       physDev->dc_rect.top + rc.bottom - (ell_height+1) / 2);
            XDrawLine( gdi_display, physDev->drawable, physDev->gc,
                       physDev->dc_rect.left + rc.left,
                       physDev->dc_rect.top + rc.top + ell_height / 2,
                       physDev->dc_rect.left + rc.left,
                       physDev->dc_rect.top + rc.bottom - (ell_height+1) / 2);
	}
        wine_tsx11_unlock();
	update = TRUE;
    }
    /* Update the DIBSection from the pixmap */
    X11DRV_UnlockDIBSection(physDev, update);

    physDev->pen.width = oldwidth;
    physDev->pen.endcap = oldendcap;
    return TRUE;
}


/***********************************************************************
 *           X11DRV_SetPixel
 */
COLORREF CDECL
X11DRV_SetPixel( X11DRV_PDEVICE *physDev, INT x, INT y, COLORREF color )
{
    unsigned long pixel;
    POINT pt;

    pt.x = x;
    pt.y = y;
    LPtoDP( physDev->hdc, &pt, 1 );
    pixel = X11DRV_PALETTE_ToPhysical( physDev, color );

    /* Update the pixmap from the DIB section */
    X11DRV_LockDIBSection(physDev, DIB_Status_GdiMod);

    /* inefficient but simple... */
    wine_tsx11_lock();
    XSetForeground( gdi_display, physDev->gc, pixel );
    XSetFunction( gdi_display, physDev->gc, GXcopy );
    XDrawPoint( gdi_display, physDev->drawable, physDev->gc,
                physDev->dc_rect.left + pt.x, physDev->dc_rect.top + pt.y );
    wine_tsx11_unlock();

    /* Update the DIBSection from the pixmap */
    X11DRV_UnlockDIBSection(physDev, TRUE);

    return X11DRV_PALETTE_ToLogical(physDev, pixel);
}


/***********************************************************************
 *           X11DRV_GetPixel
 */
COLORREF CDECL
X11DRV_GetPixel( X11DRV_PDEVICE *physDev, INT x, INT y )
{
    static Pixmap pixmap = 0;
    XImage * image;
    int pixel;
    POINT pt;
    BOOL memdc = (GetObjectType(physDev->hdc) == OBJ_MEMDC);

    pt.x = x;
    pt.y = y;
    LPtoDP( physDev->hdc, &pt, 1 );

    /* Update the pixmap from the DIB section */
    X11DRV_LockDIBSection(physDev, DIB_Status_GdiMod);

    wine_tsx11_lock();
    if (memdc)
    {
        image = XGetImage( gdi_display, physDev->drawable,
                           physDev->dc_rect.left + pt.x, physDev->dc_rect.top + pt.y,
                           1, 1, AllPlanes, ZPixmap );
    }
    else
    {
        /* If we are reading from the screen, use a temporary copy */
        /* to avoid a BadMatch error */
        if (!pixmap) pixmap = XCreatePixmap( gdi_display, root_window,
                                             1, 1, physDev->depth );
        XCopyArea( gdi_display, physDev->drawable, pixmap, get_bitmap_gc(physDev->depth),
                   physDev->dc_rect.left + pt.x, physDev->dc_rect.top + pt.y, 1, 1, 0, 0 );
        image = XGetImage( gdi_display, pixmap, 0, 0, 1, 1, AllPlanes, ZPixmap );
    }
    pixel = XGetPixel( image, 0, 0 );
    XDestroyImage( image );
    wine_tsx11_unlock();

    /* Update the DIBSection from the pixmap */
    X11DRV_UnlockDIBSection(physDev, FALSE);
    if( physDev->depth > 1)
        pixel = X11DRV_PALETTE_ToLogical(physDev, pixel);
    else
        /* monochrome bitmaps return black or white */
        if( pixel) pixel = 0xffffff;
    return pixel;

}


/***********************************************************************
 *           X11DRV_PaintRgn
 */
BOOL CDECL
X11DRV_PaintRgn( X11DRV_PDEVICE *physDev, HRGN hrgn )
{
    if (X11DRV_SetupGCForBrush( physDev ))
    {
        unsigned int i;
        XRectangle *rect;
        RGNDATA *data = X11DRV_GetRegionData( hrgn, physDev->hdc );

        if (!data) return FALSE;
        rect = (XRectangle *)data->Buffer;
        for (i = 0; i < data->rdh.nCount; i++)
        {
            rect[i].x += physDev->dc_rect.left;
            rect[i].y += physDev->dc_rect.top;
        }

        X11DRV_LockDIBSection(physDev, DIB_Status_GdiMod);
        wine_tsx11_lock();
        XFillRectangles( gdi_display, physDev->drawable, physDev->gc, rect, data->rdh.nCount );
        wine_tsx11_unlock();
        X11DRV_UnlockDIBSection(physDev, TRUE);
        HeapFree( GetProcessHeap(), 0, data );
    }
    return TRUE;
}

/**********************************************************************
 *          X11DRV_Polyline
 */
BOOL CDECL
X11DRV_Polyline( X11DRV_PDEVICE *physDev, const POINT* pt, INT count )
{
    int i;
    XPoint *points;

    if (!(points = HeapAlloc( GetProcessHeap(), 0, sizeof(XPoint) * count )))
    {
        WARN("No memory to convert POINTs to XPoints!\n");
        return FALSE;
    }
    for (i = 0; i < count; i++)
    {
        POINT tmp = pt[i];
        LPtoDP(physDev->hdc, &tmp, 1);
        points[i].x = physDev->dc_rect.left + tmp.x;
        points[i].y = physDev->dc_rect.top + tmp.y;
    }

    if (X11DRV_SetupGCForPen ( physDev ))
    {
        X11DRV_LockDIBSection(physDev, DIB_Status_GdiMod);
        wine_tsx11_lock();
        XDrawLines( gdi_display, physDev->drawable, physDev->gc,
                    points, count, CoordModeOrigin );
        wine_tsx11_unlock();
        X11DRV_UnlockDIBSection(physDev, TRUE);
    }

    HeapFree( GetProcessHeap(), 0, points );
    return TRUE;
}


/**********************************************************************
 *          X11DRV_Polygon
 */
BOOL CDECL
X11DRV_Polygon( X11DRV_PDEVICE *physDev, const POINT* pt, INT count )
{
    register int i;
    XPoint *points;
    BOOL update = FALSE;

    if (!(points = HeapAlloc( GetProcessHeap(), 0, sizeof(XPoint) * (count+1) )))
    {
        WARN("No memory to convert POINTs to XPoints!\n");
        return FALSE;
    }
    for (i = 0; i < count; i++)
    {
        POINT tmp = pt[i];
        LPtoDP(physDev->hdc, &tmp, 1);
        points[i].x = physDev->dc_rect.left + tmp.x;
        points[i].y = physDev->dc_rect.top + tmp.y;
    }
    points[count] = points[0];

    /* Update the pixmap from the DIB section */
    X11DRV_LockDIBSection(physDev, DIB_Status_GdiMod);

    if (X11DRV_SetupGCForBrush( physDev ))
    {
        wine_tsx11_lock();
        XFillPolygon( gdi_display, physDev->drawable, physDev->gc,
                      points, count+1, Complex, CoordModeOrigin);
        wine_tsx11_unlock();
	update = TRUE;
    }
    if (X11DRV_SetupGCForPen ( physDev ))
    {
        wine_tsx11_lock();
        XDrawLines( gdi_display, physDev->drawable, physDev->gc,
                    points, count+1, CoordModeOrigin );
        wine_tsx11_unlock();
	update = TRUE;
    }

    /* Update the DIBSection from the pixmap */
    X11DRV_UnlockDIBSection(physDev, update);

    HeapFree( GetProcessHeap(), 0, points );
    return TRUE;
}


/**********************************************************************
 *          X11DRV_PolyPolygon
 */
BOOL CDECL
X11DRV_PolyPolygon( X11DRV_PDEVICE *physDev, const POINT* pt, const INT* counts, UINT polygons)
{
    HRGN hrgn;

    /* FIXME: The points should be converted to device coords before */
    /* creating the region. */

    hrgn = CreatePolyPolygonRgn( pt, counts, polygons, GetPolyFillMode( physDev->hdc ) );
    X11DRV_PaintRgn( physDev, hrgn );
    DeleteObject( hrgn );

      /* Draw the outline of the polygons */

    if (X11DRV_SetupGCForPen ( physDev ))
    {
	unsigned int i;
	int j, max = 0;
	XPoint *points;

	/* Update the pixmap from the DIB section */
	X11DRV_LockDIBSection(physDev, DIB_Status_GdiMod);

	for (i = 0; i < polygons; i++) if (counts[i] > max) max = counts[i];
        if (!(points = HeapAlloc( GetProcessHeap(), 0, sizeof(XPoint) * (max+1) )))
        {
            WARN("No memory to convert POINTs to XPoints!\n");
            return FALSE;
        }
	for (i = 0; i < polygons; i++)
	{
	    for (j = 0; j < counts[i]; j++)
	    {
                POINT tmp = *pt;
                LPtoDP(physDev->hdc, &tmp, 1);
                points[j].x = physDev->dc_rect.left + tmp.x;
                points[j].y = physDev->dc_rect.top + tmp.y;
		pt++;
	    }
	    points[j] = points[0];
            wine_tsx11_lock();
            XDrawLines( gdi_display, physDev->drawable, physDev->gc,
                        points, j + 1, CoordModeOrigin );
            wine_tsx11_unlock();
	}

	/* Update the DIBSection of the dc's bitmap */
	X11DRV_UnlockDIBSection(physDev, TRUE);

	HeapFree( GetProcessHeap(), 0, points );
    }
    return TRUE;
}


/**********************************************************************
 *          X11DRV_PolyPolyline
 */
BOOL CDECL
X11DRV_PolyPolyline( X11DRV_PDEVICE *physDev, const POINT* pt, const DWORD* counts, DWORD polylines )
{
    if (X11DRV_SetupGCForPen ( physDev ))
    {
        unsigned int i, j, max = 0;
        XPoint *points;

	/* Update the pixmap from the DIB section */
	X11DRV_LockDIBSection(physDev, DIB_Status_GdiMod);

        for (i = 0; i < polylines; i++) if (counts[i] > max) max = counts[i];
        if (!(points = HeapAlloc( GetProcessHeap(), 0, sizeof(XPoint) * max )))
        {
            WARN("No memory to convert POINTs to XPoints!\n");
            return FALSE;
        }
        for (i = 0; i < polylines; i++)
        {
            for (j = 0; j < counts[i]; j++)
            {
                POINT tmp = *pt;
                LPtoDP(physDev->hdc, &tmp, 1);
                points[j].x = physDev->dc_rect.left + tmp.x;
                points[j].y = physDev->dc_rect.top + tmp.y;
                pt++;
            }
            wine_tsx11_lock();
            XDrawLines( gdi_display, physDev->drawable, physDev->gc,
                        points, j, CoordModeOrigin );
            wine_tsx11_unlock();
        }

	/* Update the DIBSection of the dc's bitmap */
    	X11DRV_UnlockDIBSection(physDev, TRUE);

	HeapFree( GetProcessHeap(), 0, points );
    }
    return TRUE;
}


/**********************************************************************
 *          X11DRV_InternalFloodFill
 *
 * Internal helper function for flood fill.
 * (xorg,yorg) is the origin of the X image relative to the drawable.
 * (x,y) is relative to the origin of the X image.
 */
static void X11DRV_InternalFloodFill(XImage *image, X11DRV_PDEVICE *physDev,
                                     int x, int y,
                                     int xOrg, int yOrg,
                                     unsigned long pixel, WORD fillType )
{
    int left, right;

#define TO_FLOOD(x,y)  ((fillType == FLOODFILLBORDER) ? \
                        (XGetPixel(image,x,y) != pixel) : \
                        (XGetPixel(image,x,y) == pixel))

    if (!TO_FLOOD(x,y)) return;

      /* Find left and right boundaries */

    left = right = x;
    while ((left > 0) && TO_FLOOD( left-1, y )) left--;
    while ((right < image->width) && TO_FLOOD( right, y )) right++;
    XFillRectangle( gdi_display, physDev->drawable, physDev->gc,
                    xOrg + left, yOrg + y, right-left, 1 );

      /* Set the pixels of this line so we don't fill it again */

    for (x = left; x < right; x++)
    {
        if (fillType == FLOODFILLBORDER) XPutPixel( image, x, y, pixel );
        else XPutPixel( image, x, y, ~pixel );
    }

      /* Fill the line above */

    if (--y >= 0)
    {
        x = left;
        while (x < right)
        {
            while ((x < right) && !TO_FLOOD(x,y)) x++;
            if (x >= right) break;
            while ((x < right) && TO_FLOOD(x,y)) x++;
            X11DRV_InternalFloodFill(image, physDev, x-1, y,
                                     xOrg, yOrg, pixel, fillType );
        }
    }

      /* Fill the line below */

    if ((y += 2) < image->height)
    {
        x = left;
        while (x < right)
        {
            while ((x < right) && !TO_FLOOD(x,y)) x++;
            if (x >= right) break;
            while ((x < right) && TO_FLOOD(x,y)) x++;
            X11DRV_InternalFloodFill(image, physDev, x-1, y,
                                     xOrg, yOrg, pixel, fillType );
        }
    }
#undef TO_FLOOD
}


static int ExtFloodFillXGetImageErrorHandler( Display *dpy, XErrorEvent *event, void *arg )
{
    return (event->request_code == X_GetImage && event->error_code == BadMatch);
}

/**********************************************************************
 *          X11DRV_ExtFloodFill
 */
BOOL CDECL
X11DRV_ExtFloodFill( X11DRV_PDEVICE *physDev, INT x, INT y, COLORREF color,
                     UINT fillType )
{
    XImage *image;
    RECT rect;
    POINT pt;

    TRACE("X11DRV_ExtFloodFill %d,%d %06x %d\n", x, y, color, fillType );

    pt.x = x;
    pt.y = y;
    LPtoDP( physDev->hdc, &pt, 1 );
    if (!PtInRegion( physDev->region, pt.x, pt.y )) return FALSE;
    GetRgnBox( physDev->region, &rect );

    X11DRV_expect_error( gdi_display, ExtFloodFillXGetImageErrorHandler, NULL );
    image = XGetImage( gdi_display, physDev->drawable,
                       physDev->dc_rect.left + rect.left, physDev->dc_rect.top + rect.top,
                       rect.right - rect.left, rect.bottom - rect.top,
                       AllPlanes, ZPixmap );
    if(X11DRV_check_error()) image = NULL;
    if (!image) return FALSE;

    if (X11DRV_SetupGCForBrush( physDev ))
    {
        unsigned long pixel = X11DRV_PALETTE_ToPhysical( physDev, color );

	/* Update the pixmap from the DIB section */
	X11DRV_LockDIBSection(physDev, DIB_Status_GdiMod);

          /* ROP mode is always GXcopy for flood-fill */
        wine_tsx11_lock();
        XSetFunction( gdi_display, physDev->gc, GXcopy );
        X11DRV_InternalFloodFill(image, physDev,
                                 pt.x - rect.left,
                                 pt.y - rect.top,
                                 physDev->dc_rect.left + rect.left,
                                 physDev->dc_rect.top + rect.top,
                                 pixel, fillType );
        wine_tsx11_unlock();
        /* Update the DIBSection of the dc's bitmap */
        X11DRV_UnlockDIBSection(physDev, TRUE);
    }

    wine_tsx11_lock();
    XDestroyImage( image );
    wine_tsx11_unlock();
    return TRUE;
}

/**********************************************************************
 *          X11DRV_SetBkColor
 */
COLORREF CDECL
X11DRV_SetBkColor( X11DRV_PDEVICE *physDev, COLORREF color )
{
    physDev->backgroundPixel = X11DRV_PALETTE_ToPhysical( physDev, color );
    return color;
}

/**********************************************************************
 *          X11DRV_SetTextColor
 */
COLORREF CDECL
X11DRV_SetTextColor( X11DRV_PDEVICE *physDev, COLORREF color )
{
    physDev->textPixel = X11DRV_PALETTE_ToPhysical( physDev, color );
    return color;
}


static unsigned char *get_icm_profile( unsigned long *size )
{
    Atom type;
    int format;
    unsigned long count, remaining;
    unsigned char *profile, *ret = NULL;

    wine_tsx11_lock();
    XGetWindowProperty( gdi_display, DefaultRootWindow(gdi_display),
                        x11drv_atom(_ICC_PROFILE), 0, ~0UL, False, AnyPropertyType,
                        &type, &format, &count, &remaining, &profile );
    *size = get_property_size( format, count );
    if (format && count)
    {
        if ((ret = HeapAlloc( GetProcessHeap(), 0, *size ))) memcpy( ret, profile, *size );
        XFree( profile );
    }
    wine_tsx11_unlock();
    return ret;
}

typedef struct
{
    unsigned int unknown[6];
    unsigned int state[5];
    unsigned int count[2];
    unsigned char buffer[64];
} sha_ctx;

extern void WINAPI A_SHAInit( sha_ctx * );
extern void WINAPI A_SHAUpdate( sha_ctx *, const unsigned char *, unsigned int );
extern void WINAPI A_SHAFinal( sha_ctx *, unsigned char * );

static const WCHAR mntr_key[] =
    {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
     'W','i','n','d','o','w','s',' ','N','T','\\','C','u','r','r','e','n','t',
     'V','e','r','s','i','o','n','\\','I','C','M','\\','m','n','t','r',0};

static const WCHAR color_path[] =
    {'\\','s','p','o','o','l','\\','d','r','i','v','e','r','s','\\','c','o','l','o','r','\\',0};

/***********************************************************************
 *              GetICMProfile (X11DRV.@)
 */
BOOL CDECL X11DRV_GetICMProfile( X11DRV_PDEVICE *physDev, LPDWORD size, LPWSTR filename )
{
    static const WCHAR srgb[] =
        {'s','R','G','B',' ','C','o','l','o','r',' ','S','p','a','c','e',' ',
         'P','r','o','f','i','l','e','.','i','c','m',0};
    HKEY hkey;
    DWORD required, len;
    WCHAR profile[MAX_PATH], fullname[2*MAX_PATH + sizeof(color_path)/sizeof(WCHAR)];
    unsigned char *buffer;
    unsigned long buflen;

    if (!size) return FALSE;

    GetSystemDirectoryW( fullname, MAX_PATH );
    strcatW( fullname, color_path );

    len = sizeof(profile)/sizeof(WCHAR);
    if (!RegCreateKeyExW( HKEY_LOCAL_MACHINE, mntr_key, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hkey, NULL ) &&
        !RegEnumValueW( hkey, 0, profile, &len, NULL, NULL, NULL, NULL )) /* FIXME handle multiple values */
    {
        strcatW( fullname, profile );
        RegCloseKey( hkey );
    }
    else if ((buffer = get_icm_profile( &buflen )))
    {
        static const WCHAR fmt[] = {'%','0','2','x',0};
        static const WCHAR icm[] = {'.','i','c','m',0};

        unsigned char sha1sum[20];
        unsigned int i;
        sha_ctx ctx;
        HANDLE file;

        A_SHAInit( &ctx );
        A_SHAUpdate( &ctx, buffer, buflen );
        A_SHAFinal( &ctx, sha1sum );

        for (i = 0; i < sizeof(sha1sum); i++) sprintfW( &profile[i * 2], fmt, sha1sum[i] );
        memcpy( &profile[i * 2], icm, sizeof(icm) );

        strcatW( fullname, profile );
        file = CreateFileW( fullname, GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, 0 );
        if (file != INVALID_HANDLE_VALUE)
        {
            DWORD written;

            if (!WriteFile( file, buffer, buflen, &written, NULL ) || written != buflen)
                ERR( "Unable to write color profile\n" );
            CloseHandle( file );
        }
        HeapFree( GetProcessHeap(), 0, buffer );
    }
    else strcatW( fullname, srgb );

    required = strlenW( fullname ) + 1;
    if (*size < required)
    {
        *size = required;
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }
    if (filename)
    {
        strcpyW( filename, fullname );
        if (GetFileAttributesW( filename ) == INVALID_FILE_ATTRIBUTES)
            WARN( "color profile not found\n" );
    }
    *size = required;
    return TRUE;
}

/***********************************************************************
 *              EnumICMProfiles (X11DRV.@)
 */
INT CDECL X11DRV_EnumICMProfiles( X11DRV_PDEVICE *physDev, ICMENUMPROCW proc, LPARAM lparam )
{
    HKEY hkey;
    DWORD len_sysdir, len_path, len, index = 0;
    WCHAR sysdir[MAX_PATH], *profile;
    LONG res;
    INT ret;

    TRACE("%p, %p, %ld\n", physDev, proc, lparam);

    if (RegOpenKeyExW( HKEY_LOCAL_MACHINE, mntr_key, 0, KEY_ALL_ACCESS, &hkey ))
        return -1;

    len_sysdir = GetSystemDirectoryW( sysdir, MAX_PATH );
    len_path = len_sysdir + sizeof(color_path) / sizeof(color_path[0]) - 1;
    len = 64;
    for (;;)
    {
        if (!(profile = HeapAlloc( GetProcessHeap(), 0, (len_path + len) * sizeof(WCHAR) )))
        {
            RegCloseKey( hkey );
            return -1;
        }
        res = RegEnumValueW( hkey, index, profile + len_path, &len, NULL, NULL, NULL, NULL );
        while (res == ERROR_MORE_DATA)
        {
            len *= 2;
            HeapFree( GetProcessHeap(), 0, profile );
            if (!(profile = HeapAlloc( GetProcessHeap(), 0, (len_path + len) * sizeof(WCHAR) )))
            {
                RegCloseKey( hkey );
                return -1;
            }
            res = RegEnumValueW( hkey, index, profile + len_path, &len, NULL, NULL, NULL, NULL );
        }
        if (res != ERROR_SUCCESS)
        {
            HeapFree( GetProcessHeap(), 0, profile );
            break;
        }
        memcpy( profile, sysdir, len_sysdir * sizeof(WCHAR) );
        memcpy( profile + len_sysdir, color_path, sizeof(color_path) - sizeof(WCHAR) );
        ret = proc( profile, lparam );
        HeapFree( GetProcessHeap(), 0, profile );
        if (!ret) break;
        index++;
    }
    RegCloseKey( hkey );
    return -1;
}
