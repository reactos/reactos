/*
 * X11DRV brush objects
 *
 * Copyright 1993, 1994  Alexandre Julliard
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

#include <stdlib.h>

#include "wine/winbase16.h"
#include "x11drv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdi);

static const char HatchBrushes[][8] =
{
    { 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00 }, /* HS_HORIZONTAL */
    { 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08 }, /* HS_VERTICAL   */
    { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 }, /* HS_FDIAGONAL  */
    { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 }, /* HS_BDIAGONAL  */
    { 0x08, 0x08, 0x08, 0xff, 0x08, 0x08, 0x08, 0x08 }, /* HS_CROSS      */
    { 0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81 }, /* HS_DIAGCROSS  */
};

  /* Levels of each primary for dithering */
#define PRIMARY_LEVELS  3
#define TOTAL_LEVELS    (PRIMARY_LEVELS*PRIMARY_LEVELS*PRIMARY_LEVELS)

 /* Dithering matrix size  */
#define MATRIX_SIZE     8
#define MATRIX_SIZE_2   (MATRIX_SIZE*MATRIX_SIZE)

  /* Total number of possible levels for a dithered primary color */
#define DITHER_LEVELS   (MATRIX_SIZE_2 * (PRIMARY_LEVELS-1) + 1)

  /* Dithering matrix */
static const int dither_matrix[MATRIX_SIZE_2] =
{
     0, 32,  8, 40,  2, 34, 10, 42,
    48, 16, 56, 24, 50, 18, 58, 26,
    12, 44,  4, 36, 14, 46,  6, 38,
    60, 28, 52, 20, 62, 30, 54, 22,
     3, 35, 11, 43,  1, 33,  9, 41,
    51, 19, 59, 27, 49, 17, 57, 25,
    15, 47,  7, 39, 13, 45,  5, 37,
    63, 31, 55, 23, 61, 29, 53, 21
};

  /* Mapping between (R,G,B) triples and EGA colors */
static const int EGAmapping[TOTAL_LEVELS] =
{
    0,  /* 000000 -> 000000 */
    4,  /* 00007f -> 000080 */
    12, /* 0000ff -> 0000ff */
    2,  /* 007f00 -> 008000 */
    6,  /* 007f7f -> 008080 */
    6,  /* 007fff -> 008080 */
    10, /* 00ff00 -> 00ff00 */
    6,  /* 00ff7f -> 008080 */
    14, /* 00ffff -> 00ffff */
    1,  /* 7f0000 -> 800000 */
    5,  /* 7f007f -> 800080 */
    5,  /* 7f00ff -> 800080 */
    3,  /* 7f7f00 -> 808000 */
    8,  /* 7f7f7f -> 808080 */
    7,  /* 7f7fff -> c0c0c0 */
    3,  /* 7fff00 -> 808000 */
    7,  /* 7fff7f -> c0c0c0 */
    7,  /* 7fffff -> c0c0c0 */
    9,  /* ff0000 -> ff0000 */
    5,  /* ff007f -> 800080 */
    13, /* ff00ff -> ff00ff */
    3,  /* ff7f00 -> 808000 */
    7,  /* ff7f7f -> c0c0c0 */
    7,  /* ff7fff -> c0c0c0 */
    11, /* ffff00 -> ffff00 */
    7,  /* ffff7f -> c0c0c0 */
    15  /* ffffff -> ffffff */
};

#define PIXEL_VALUE(r,g,b) \
    X11DRV_PALETTE_mapEGAPixel[EGAmapping[((r)*PRIMARY_LEVELS+(g))*PRIMARY_LEVELS+(b)]]

static const COLORREF BLACK = RGB(0, 0, 0);
static const COLORREF WHITE = RGB(0xff, 0xff, 0xff);

/***********************************************************************
 *           BRUSH_DitherColor
 */
static Pixmap BRUSH_DitherColor( COLORREF color, int depth)
{
    /* X image for building dithered pixmap */
    static XImage *ditherImage = NULL;
    static COLORREF prevColor = 0xffffffff;
    unsigned int x, y;
    Pixmap pixmap;
    GC gc = get_bitmap_gc(depth);

    if (!ditherImage)
    {
        ditherImage = X11DRV_DIB_CreateXImage( MATRIX_SIZE, MATRIX_SIZE, depth );
        if (!ditherImage) 
        {
            ERR("Could not create dither image\n");
            return 0;
        }
    }

    wine_tsx11_lock();
    if (color != prevColor)
    {
	int r = GetRValue( color ) * DITHER_LEVELS;
	int g = GetGValue( color ) * DITHER_LEVELS;
	int b = GetBValue( color ) * DITHER_LEVELS;
	const int *pmatrix = dither_matrix;

	for (y = 0; y < MATRIX_SIZE; y++)
	{
	    for (x = 0; x < MATRIX_SIZE; x++)
	    {
		int d  = *pmatrix++ * 256;
		int dr = ((r + d) / MATRIX_SIZE_2) / 256;
		int dg = ((g + d) / MATRIX_SIZE_2) / 256;
		int db = ((b + d) / MATRIX_SIZE_2) / 256;
		XPutPixel( ditherImage, x, y, PIXEL_VALUE(dr,dg,db) );
	    }
	}
	prevColor = color;
    }

    pixmap = XCreatePixmap( gdi_display, root_window, MATRIX_SIZE, MATRIX_SIZE, depth );
    XPutImage( gdi_display, pixmap, gc, ditherImage, 0, 0,
    	       0, 0, MATRIX_SIZE, MATRIX_SIZE );
    wine_tsx11_unlock();

    return pixmap;
}


/***********************************************************************
 *           BRUSH_DitherMono
 */
static Pixmap BRUSH_DitherMono( COLORREF color )
{
    /* This makes the spray work in Win 3.11 pbrush.exe */
    /* FIXME. Extend this basic selection of dither patterns */
    static const char gray_dither[][2] = {{ 0x1, 0x0 }, /* DKGRAY */
                                          { 0x2, 0x1 }, /* GRAY */
                                          { 0x1, 0x3 }, /* LTGRAY */
    };                                      
    int gray = (30 * GetRValue(color) + 59 * GetGValue(color) + 11 * GetBValue(color)) / 100;
    int idx = gray * (sizeof gray_dither/sizeof gray_dither[0] + 1)/256 - 1;
    Pixmap pixmap;

    TRACE("color=%06x -> gray=%x\n", color, gray);

    wine_tsx11_lock();
    pixmap = XCreateBitmapFromData( gdi_display, root_window, 
                                    gray_dither[idx],
                                    2, 2 );
    wine_tsx11_unlock();
    return pixmap;
}

/***********************************************************************
 *           BRUSH_SelectSolidBrush
 */
static void BRUSH_SelectSolidBrush( X11DRV_PDEVICE *physDev, COLORREF color )
{
    COLORREF colorRGB = X11DRV_PALETTE_GetColor( physDev, color );
    if ((physDev->depth > 1) && (screen_depth <= 8) && !X11DRV_IsSolidColor( color ))
    {
	  /* Dithered brush */
	physDev->brush.pixmap = BRUSH_DitherColor( colorRGB, physDev->depth );
	physDev->brush.fillStyle = FillTiled;
	physDev->brush.pixel = 0;
    }
    else if (physDev->depth == 1 && colorRGB != WHITE && colorRGB != BLACK)
    {
	physDev->brush.pixel = 0;
	physDev->brush.pixmap = BRUSH_DitherMono( colorRGB );
	physDev->brush.fillStyle = FillTiled;
    }
    else
    {
	  /* Solid brush */
	physDev->brush.pixel = X11DRV_PALETTE_ToPhysical( physDev, color );
	physDev->brush.fillStyle = FillSolid;
    }
}


/***********************************************************************
 *           BRUSH_SelectPatternBrush
 */
static BOOL BRUSH_SelectPatternBrush( X11DRV_PDEVICE *physDev, HBITMAP hbitmap )
{
    BITMAP bitmap;
    X_PHYSBITMAP *physBitmap = X11DRV_get_phys_bitmap( hbitmap );

    if (!physBitmap || !GetObjectW( hbitmap, sizeof(bitmap), &bitmap )) return FALSE;

    if ((physDev->depth == 1) && (physBitmap->pixmap_depth != 1))
    {
        wine_tsx11_lock();
        /* Special case: a color pattern on a monochrome DC */
        physDev->brush.pixmap = XCreatePixmap( gdi_display, root_window,
                                               bitmap.bmWidth, bitmap.bmHeight, 1);
        /* FIXME: should probably convert to monochrome instead */
        XCopyPlane( gdi_display, physBitmap->pixmap, physDev->brush.pixmap,
                    get_bitmap_gc(1), 0, 0, bitmap.bmWidth, bitmap.bmHeight, 0, 0, 1 );
        wine_tsx11_unlock();
    }
    else
    {
        /* XRender is needed because of possible depth conversion */
        X11DRV_XRender_CopyBrush(physDev, physBitmap, bitmap.bmWidth, bitmap.bmHeight);
    }

    if (physBitmap->pixmap_depth > 1)
    {
	physDev->brush.fillStyle = FillTiled;
	physDev->brush.pixel = 0;  /* Ignored */
    }
    else
    {
	physDev->brush.fillStyle = FillOpaqueStippled;
	physDev->brush.pixel = -1;  /* Special case (see DC_SetupGCForBrush) */
    }
    return TRUE;
}


/***********************************************************************
 *           SelectBrush   (X11DRV.@)
 */
HBRUSH CDECL X11DRV_SelectBrush( X11DRV_PDEVICE *physDev, HBRUSH hbrush )
{
    LOGBRUSH logbrush;
    HBITMAP hBitmap;
    BITMAPINFO * bmpInfo;

    if (!GetObjectA( hbrush, sizeof(logbrush), &logbrush )) return 0;

    TRACE("hdc=%p hbrush=%p\n", physDev->hdc,hbrush);

    if (physDev->brush.pixmap)
    {
        wine_tsx11_lock();
        XFreePixmap( gdi_display, physDev->brush.pixmap );
        wine_tsx11_unlock();
        physDev->brush.pixmap = 0;
    }
    physDev->brush.style = logbrush.lbStyle;
    if (hbrush == GetStockObject( DC_BRUSH ))
        logbrush.lbColor = GetDCBrushColor( physDev->hdc );

    switch(logbrush.lbStyle)
    {
      case BS_NULL:
	TRACE("BS_NULL\n" );
	break;

      case BS_SOLID:
        TRACE("BS_SOLID\n" );
	BRUSH_SelectSolidBrush( physDev, logbrush.lbColor );
	break;

      case BS_HATCHED:
	TRACE("BS_HATCHED\n" );
	physDev->brush.pixel = X11DRV_PALETTE_ToPhysical( physDev, logbrush.lbColor );
        wine_tsx11_lock();
        physDev->brush.pixmap = XCreateBitmapFromData( gdi_display, root_window,
                                                       HatchBrushes[logbrush.lbHatch], 8, 8 );
        wine_tsx11_unlock();
	physDev->brush.fillStyle = FillStippled;
	break;

      case BS_PATTERN:
	TRACE("BS_PATTERN\n");
	if (!BRUSH_SelectPatternBrush( physDev, (HBITMAP)logbrush.lbHatch )) return 0;
	break;

      case BS_DIBPATTERN:
	TRACE("BS_DIBPATTERN\n");
        if ((bmpInfo = GlobalLock( (HGLOBAL)logbrush.lbHatch )))
	{
	    int size = bitmap_info_size( bmpInfo, logbrush.lbColor );
	    hBitmap = CreateDIBitmap( physDev->hdc, &bmpInfo->bmiHeader,
                                        CBM_INIT, ((char *)bmpInfo) + size,
                                        bmpInfo,
                                        (WORD)logbrush.lbColor );
	    BRUSH_SelectPatternBrush( physDev, hBitmap );
	    DeleteObject( hBitmap );
            GlobalUnlock( (HGLOBAL)logbrush.lbHatch );
	}

	break;
    }
    return hbrush;
}


/***********************************************************************
 *           SetDCBrushColor (X11DRV.@)
 */
COLORREF CDECL X11DRV_SetDCBrushColor( X11DRV_PDEVICE *physDev, COLORREF crColor )
{
    if (GetCurrentObject(physDev->hdc, OBJ_BRUSH) == GetStockObject( DC_BRUSH ))
        BRUSH_SelectSolidBrush( physDev, crColor );

    return crColor;
}
