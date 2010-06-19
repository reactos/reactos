/*
 * X11DRV device-independent bitmaps
 *
 * Copyright 1993,1994  Alexandre Julliard
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

#include <X11/Xlib.h>
#ifdef HAVE_LIBXXSHM
#include <X11/extensions/XShm.h>
# ifdef HAVE_SYS_SHM_H
#  include <sys/shm.h>
# endif
# ifdef HAVE_SYS_IPC_H
#  include <sys/ipc.h>
# endif
#endif /* defined(HAVE_LIBXXSHM) */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "x11drv.h"
#include "wine/exception.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(bitmap);

static struct list dibs_list = LIST_INIT(dibs_list);

static CRITICAL_SECTION dibs_cs;
static CRITICAL_SECTION_DEBUG dibs_cs_debug =
{
    0, 0, &dibs_cs,
    { &dibs_cs_debug.ProcessLocksList, &dibs_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": dibs_cs") }
};
static CRITICAL_SECTION dibs_cs = { &dibs_cs_debug, -1, 0, 0, 0, 0 };

static PVOID dibs_handler;

static int ximageDepthTable[32];

/* This structure holds the arguments for DIB_SetImageBits() */
typedef struct
{
    X11DRV_PDEVICE         *physDev;
    LPCVOID                 bits;
    XImage                 *image;
    PALETTEENTRY           *palentry;
    int                     lines;
    DWORD                   infoWidth;
    WORD                    depth;
    WORD                    infoBpp;
    WORD                    compression;
    ColorShifts            *shifts;
    RGBQUAD                *colorMap;
    int                     nColorMap;
    Drawable                drawable;
    GC                      gc;
    int                     xSrc;
    int                     ySrc;
    int                     xDest;
    int                     yDest;
    int                     width;
    int                     height;
    DWORD                   rMask;
    DWORD                   gMask;
    DWORD                   bMask;
    enum x11drv_shm_mode    shm_mode;
    int                     dibpitch;
    DWORD                   sizeImage;
    X_PHYSBITMAP           *physBitmap;
} X11DRV_DIB_IMAGEBITS_DESCR;


enum Rle_EscapeCodes
{
  RLE_EOL   = 0, /* End of line */
  RLE_END   = 1, /* End of bitmap */
  RLE_DELTA = 2  /* Delta */
};


static INT X11DRV_DIB_Coerce(X_PHYSBITMAP *,INT);
static INT X11DRV_DIB_Lock(X_PHYSBITMAP *,INT);
static void X11DRV_DIB_Unlock(X_PHYSBITMAP *,BOOL);

/* 
  Some of the following helper functions are duplicated in
  dlls/gdi/dib.c
*/

/***********************************************************************
 *           DIB_DoProtectDIBSection
 */
static void X11DRV_DIB_DoProtectDIBSection( X_PHYSBITMAP *physBitmap, DWORD new_prot )
{
    DWORD old_prot;

    VirtualProtect(physBitmap->base, physBitmap->size, new_prot, &old_prot);
    TRACE("Changed protection from %d to %d\n", old_prot, new_prot);
}

/***********************************************************************
 *           X11DRV_DIB_GetXImageWidthBytes
 *
 * Return the width of an X image in bytes
 */
static inline int X11DRV_DIB_GetXImageWidthBytes( int width, int depth )
{
    if (!depth || depth > 32) goto error;

    if (!ximageDepthTable[depth-1])
    {
        XImage *testimage = XCreateImage( gdi_display, visual, depth,
                                          ZPixmap, 0, NULL, 1, 1, 32, 20 );
        if (testimage)
        {
            ximageDepthTable[depth-1] = testimage->bits_per_pixel;
            XDestroyImage( testimage );
        }
        else ximageDepthTable[depth-1] = -1;
    }
    if (ximageDepthTable[depth-1] != -1)
        return (4 * ((width * ximageDepthTable[depth-1] + 31) / 32));

 error:
    WARN( "(%d): Unsupported depth\n", depth );
    return 4 * width;
}


/***********************************************************************
 *           X11DRV_DIB_GetDIBWidthBytes
 *
 * Return the width of a DIB bitmap in bytes. DIB bitmap data is 32-bit aligned.
 */
static int X11DRV_DIB_GetDIBWidthBytes( int width, int depth )
{
    int words;

    switch(depth)
    {
    case 1:  words = (width + 31) / 32; break;
    case 4:  words = (width + 7) / 8; break;
    case 8:  words = (width + 3) / 4; break;
    case 15:
    case 16: words = (width + 1) / 2; break;
    case 24: words = (width * 3 + 3) / 4; break;
    default:
        WARN("(%d): Unsupported depth\n", depth );
        /* fall through */
    case 32:
        words = width;
    }
    return 4 * words;
}


/***********************************************************************
 *           X11DRV_DIB_GetDIBImageBytes
 *
 * Return the number of bytes used to hold the image in a DIB bitmap.
 */
static int X11DRV_DIB_GetDIBImageBytes( int width, int height, int depth )
{
    return X11DRV_DIB_GetDIBWidthBytes( width, depth ) * abs( height );
}


/***********************************************************************
 *           bitmap_info_size
 *
 * Return the size of the bitmap info structure including color table.
 */
int bitmap_info_size( const BITMAPINFO * info, WORD coloruse )
{
    unsigned int colors, size, masks = 0;

    if (info->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
    {
        const BITMAPCOREHEADER *core = (const BITMAPCOREHEADER *)info;
        colors = (core->bcBitCount <= 8) ? 1 << core->bcBitCount : 0;
        return sizeof(BITMAPCOREHEADER) + colors *
             ((coloruse == DIB_RGB_COLORS) ? sizeof(RGBTRIPLE) : sizeof(WORD));
    }
    else  /* assume BITMAPINFOHEADER */
    {
        colors = info->bmiHeader.biClrUsed;
        if (!colors && (info->bmiHeader.biBitCount <= 8))
            colors = 1 << info->bmiHeader.biBitCount;
        if (info->bmiHeader.biCompression == BI_BITFIELDS) masks = 3;
        size = max( info->bmiHeader.biSize, sizeof(BITMAPINFOHEADER) + masks * sizeof(DWORD) );
        return size + colors * ((coloruse == DIB_RGB_COLORS) ? sizeof(RGBQUAD) : sizeof(WORD));
    }
}


/***********************************************************************
 *           X11DRV_DIB_CreateXImage
 *
 * Create an X image.
 */
XImage *X11DRV_DIB_CreateXImage( int width, int height, int depth )
{
    int width_bytes;
    XImage *image = NULL;
    void *data;

    wine_tsx11_lock();
    width_bytes = X11DRV_DIB_GetXImageWidthBytes( width, depth );
    data = HeapAlloc( GetProcessHeap(), 0, height * width_bytes );
    if (data) image = XCreateImage( gdi_display, visual, depth, ZPixmap, 0,
                                    data, width, height, 32, width_bytes );
    if (!image) HeapFree( GetProcessHeap(), 0, data );
    wine_tsx11_unlock();
    return image;
}


/***********************************************************************
 *           X11DRV_DIB_DestroyXImage
 *
 * Destroy an X image created with X11DRV_DIB_CreateXImage.
 */
void X11DRV_DIB_DestroyXImage( XImage *image )
{
    HeapFree( GetProcessHeap(), 0, image->data );
    image->data = NULL;
    wine_tsx11_lock();
    XDestroyImage( image );
    wine_tsx11_unlock();
}


/***********************************************************************
 *           DIB_GetBitmapInfoEx
 *
 * Get the info from a bitmap header.
 * Return 1 for INFOHEADER, 0 for COREHEADER, -1 for error.
 */
static int DIB_GetBitmapInfoEx( const BITMAPINFOHEADER *header, LONG *width,
                                LONG *height, WORD *planes, WORD *bpp,
                                WORD *compr, DWORD *size )
{
    if (header->biSize == sizeof(BITMAPCOREHEADER))
    {
        const BITMAPCOREHEADER *core = (const BITMAPCOREHEADER *)header;
        *width  = core->bcWidth;
        *height = core->bcHeight;
        *planes = core->bcPlanes;
        *bpp    = core->bcBitCount;
        *compr  = 0;
        *size   = 0;
        return 0;
    }
    if (header->biSize >= sizeof(BITMAPINFOHEADER))
    {
        *width  = header->biWidth;
        *height = header->biHeight;
        *planes = header->biPlanes;
        *bpp    = header->biBitCount;
        *compr  = header->biCompression;
        *size   = header->biSizeImage;
        return 1;
    }
    ERR("(%d): unknown/wrong size for header\n", header->biSize );
    return -1;
}


/***********************************************************************
 *           X11DRV_DIB_GetColorCount
 *
 * Computes the number of colors for the bitmap palette.
 * Should not be called for a >8-bit deep bitmap.
 */
static unsigned int X11DRV_DIB_GetColorCount(const BITMAPINFO *info)
{
    unsigned int colors;
    BOOL core_info = info->bmiHeader.biSize == sizeof(BITMAPCOREHEADER);

    if (core_info)
    {
        colors = 1 << ((const BITMAPCOREINFO*)info)->bmciHeader.bcBitCount;
    }
    else
    {
        colors = info->bmiHeader.biClrUsed;
        if (!colors) colors = 1 << info->bmiHeader.biBitCount;
    }
    if (colors > 256)
    {
        ERR("called with >256 colors!\n");
        colors = 0;
    }
    return colors;
}

/***********************************************************************
 *           DIB_GetBitmapInfo
 *
 * Get the info from a bitmap header.
 * Return 1 for INFOHEADER, 0 for COREHEADER, -1 for error.
 */
static int DIB_GetBitmapInfo( const BITMAPINFOHEADER *header, LONG *width,
                              LONG *height, WORD *bpp, WORD *compr )
{
    WORD planes;
    DWORD size;
    
    return DIB_GetBitmapInfoEx( header, width, height, &planes, bpp, compr, &size);    
}


static inline BOOL colour_is_brighter(RGBQUAD c1, RGBQUAD c2)
{
    return (c1.rgbRed * c1.rgbRed + c1.rgbGreen * c1.rgbGreen + c1.rgbBlue * c1.rgbBlue) > 
        (c2.rgbRed * c2.rgbRed + c2.rgbGreen * c2.rgbGreen + c2.rgbBlue * c2.rgbBlue);
}

/***********************************************************************
 *           X11DRV_DIB_GenColorMap
 *
 * Fills the color map of a bitmap palette. Should not be called
 * for a >8-bit deep bitmap.
 */
static int *X11DRV_DIB_GenColorMap( X11DRV_PDEVICE *physDev, int *colorMapping,
                             WORD coloruse, WORD depth, BOOL quads,
                             const void *colorPtr, int start, int end )
{
    int i;

    if (coloruse == DIB_RGB_COLORS)
    {
        if (quads)
        {
            const RGBQUAD * rgb = colorPtr;

            if (depth == 1)  /* Monochrome */
            {
                BOOL invert = FALSE;
                RGBQUAD table[2];

                if (GetDIBColorTable( physDev->hdc, 0, 2, table ) == 2)
                    invert = !colour_is_brighter(table[1], table[0]);

                for (i = start; i < end; i++, rgb++) 
                    colorMapping[i] = ((rgb->rgbRed + rgb->rgbGreen +
                                        rgb->rgbBlue > 255*3/2 && !invert) ||
                                       (rgb->rgbRed + rgb->rgbGreen +
                                        rgb->rgbBlue <= 255*3/2 && invert));
            }
            else
                for (i = start; i < end; i++, rgb++)
                    colorMapping[i] = X11DRV_PALETTE_LookupPixel(physDev->color_shifts, RGB(rgb->rgbRed,
                                                                rgb->rgbGreen,
                                                                rgb->rgbBlue));
        }
        else
        {
            const RGBTRIPLE * rgb = colorPtr;

            if (depth == 1)  /* Monochrome */
            {
                BOOL invert = FALSE;
                RGBQUAD table[2];

                if (GetDIBColorTable( physDev->hdc, 0, 2, table ) == 2)
                    invert = !colour_is_brighter(table[1], table[0]);

                for (i = start; i < end; i++, rgb++)
                    colorMapping[i] = ((rgb->rgbtRed + rgb->rgbtGreen +
                                        rgb->rgbtBlue > 255*3/2 && !invert) ||
                                       (rgb->rgbtRed + rgb->rgbtGreen +
                                        rgb->rgbtBlue <= 255*3/2 && invert));
            }
            else
                for (i = start; i < end; i++, rgb++)
                    colorMapping[i] = X11DRV_PALETTE_LookupPixel(physDev->color_shifts, RGB(rgb->rgbtRed,
                                                               rgb->rgbtGreen,
                                                               rgb->rgbtBlue));
        }
    }
    else  /* DIB_PAL_COLORS */
    {
        const WORD * index = colorPtr;

        for (i = start; i < end; i++, index++)
            colorMapping[i] = X11DRV_PALETTE_ToPhysical( physDev, PALETTEINDEX(*index) );
    }

    return colorMapping;
}

/***********************************************************************
 *           X11DRV_DIB_BuildColorMap
 *
 * Build the color map from the bitmap palette. Should not be called
 * for a >8-bit deep bitmap.
 */
static int *X11DRV_DIB_BuildColorMap( X11DRV_PDEVICE *physDev, WORD coloruse, WORD depth,
                                      const BITMAPINFO *info, int *nColors )
{
    BOOL isInfo;
    const void *colorPtr;
    int *colorMapping;


    *nColors = X11DRV_DIB_GetColorCount(info);
    if (!*nColors) return NULL;

    isInfo = info->bmiHeader.biSize != sizeof(BITMAPCOREHEADER);
    colorPtr = (const BYTE*)info + (WORD)info->bmiHeader.biSize;
    if (!(colorMapping = HeapAlloc(GetProcessHeap(), 0, *nColors * sizeof(int) )))
        return NULL;

    return X11DRV_DIB_GenColorMap( physDev, colorMapping, coloruse, depth,
                                   isInfo, colorPtr, 0, *nColors);
}

/***********************************************************************
 *           X11DRV_DIB_MapColor
 */
static int X11DRV_DIB_MapColor( int *physMap, int nPhysMap, int phys, int oldcol )
{
    int color;

    if ((oldcol < nPhysMap) && (physMap[oldcol] == phys))
        return oldcol;

    for (color = 0; color < nPhysMap; color++)
        if (physMap[color] == phys)
            return color;

    WARN("Strange color %08x\n", phys);
    return 0;
}


/*********************************************************************
 *         X11DRV_DIB_GetNearestIndex
 *
 * Helper for X11DRV_DIB_GetDIBits.
 * Returns the nearest colour table index for a given RGB.
 * Nearest is defined by minimizing the sum of the squares.
 */
static INT X11DRV_DIB_GetNearestIndex(RGBQUAD *colormap, int numColors, BYTE r, BYTE g, BYTE b)
{
    INT i, best = -1, diff, bestdiff = -1;
    RGBQUAD *color;

    for(color = colormap, i = 0; i < numColors; color++, i++) {
        diff = (r - color->rgbRed) * (r - color->rgbRed) +
	       (g - color->rgbGreen) * (g - color->rgbGreen) +
	       (b - color->rgbBlue) * (b - color->rgbBlue);
	if(diff == 0)
	    return i;
	if(best == -1 || diff < bestdiff) {
	    best = i;
	    bestdiff = diff;
	}
    }
    return best;
}
/*********************************************************************
 *         X11DRV_DIB_MaskToShift
 *
 * Helper for X11DRV_DIB_GetDIBits.
 * Returns the by how many bits to shift a given color so that it is
 * in the proper position.
 */
INT X11DRV_DIB_MaskToShift(DWORD mask)
{
    int shift;

    if (mask==0)
        return 0;

    shift=0;
    while ((mask&1)==0) {
        mask>>=1;
        shift++;
    }
    return shift;
}

/***********************************************************************
 *           X11DRV_DIB_CheckMask
 *
 * Check RGB mask if it is either 0 or matches visual's mask.
 */
static inline int X11DRV_DIB_CheckMask(int red_mask, int green_mask, int blue_mask)
{
    return ( red_mask == 0 && green_mask == 0 && blue_mask == 0 ) ||
           ( red_mask == visual->red_mask && green_mask == visual->green_mask &&
             blue_mask == visual->blue_mask );
}

/***********************************************************************
 *           X11DRV_DIB_SetImageBits_1
 *
 * SetDIBits for a 1-bit deep DIB.
 */
static void X11DRV_DIB_SetImageBits_1( int lines, const BYTE *srcbits,
                                DWORD srcwidth, DWORD dstwidth, int left,
                                int *colors, XImage *bmpImage, int linebytes)
{
    int h, width;
    const BYTE* srcbyte;
    BYTE srcval, extra;
    DWORD i, x;

    if (lines < 0 ) {
        lines = -lines;
        srcbits = srcbits + linebytes * (lines - 1);
        linebytes = -linebytes;
    }

    if ((extra = (left & 7)) != 0) {
        left &= ~7;
        dstwidth += extra;
    }
    srcbits += left >> 3;
    width = min(srcwidth, dstwidth);

    /* ==== pal 1 dib -> any bmp format ==== */
    for (h = lines-1; h >=0; h--) {
        srcbyte=srcbits;
        for (i = width/8, x = left; i > 0; i--) {
            srcval=*srcbyte++;
            XPutPixel( bmpImage, x++, h, colors[ srcval >> 7] );
            XPutPixel( bmpImage, x++, h, colors[(srcval >> 6) & 1] );
            XPutPixel( bmpImage, x++, h, colors[(srcval >> 5) & 1] );
            XPutPixel( bmpImage, x++, h, colors[(srcval >> 4) & 1] );
            XPutPixel( bmpImage, x++, h, colors[(srcval >> 3) & 1] );
            XPutPixel( bmpImage, x++, h, colors[(srcval >> 2) & 1] );
            XPutPixel( bmpImage, x++, h, colors[(srcval >> 1) & 1] );
            XPutPixel( bmpImage, x++, h, colors[ srcval       & 1] );
        }
        if (width % 8){
            srcval=*srcbyte;
            switch (width & 7)
            {
            case 7: XPutPixel(bmpImage, x++, h, colors[srcval >> 7]); srcval<<=1;
            case 6: XPutPixel(bmpImage, x++, h, colors[srcval >> 7]); srcval<<=1;
            case 5: XPutPixel(bmpImage, x++, h, colors[srcval >> 7]); srcval<<=1;
            case 4: XPutPixel(bmpImage, x++, h, colors[srcval >> 7]); srcval<<=1;
            case 3: XPutPixel(bmpImage, x++, h, colors[srcval >> 7]); srcval<<=1;
            case 2: XPutPixel(bmpImage, x++, h, colors[srcval >> 7]); srcval<<=1;
            case 1: XPutPixel(bmpImage, x++, h, colors[srcval >> 7]);
            }
        }
        srcbits += linebytes;
    }
}

/***********************************************************************
 *           X11DRV_DIB_GetImageBits_1
 *
 * GetDIBits for a 1-bit deep DIB.
 */
static void X11DRV_DIB_GetImageBits_1( int lines, BYTE *dstbits,
				       DWORD dstwidth, DWORD srcwidth,
				       RGBQUAD *colors, PALETTEENTRY *srccolors,
                                       XImage *bmpImage, int linebytes )
{
    DWORD x;
    int h, width = min(dstwidth, srcwidth);

    if (lines < 0 ) {
        lines = -lines;
        dstbits = dstbits + linebytes * (lines - 1);
        linebytes = -linebytes;
    }

    switch (bmpImage->depth)
    {
    case 1:
    case 4:
        if (X11DRV_DIB_CheckMask(bmpImage->red_mask,bmpImage->green_mask,bmpImage->blue_mask)
            && srccolors) {
            /* ==== pal 1 or 4 bmp -> pal 1 dib ==== */
            BYTE* dstbyte;

            for (h=lines-1; h>=0; h--) {
                BYTE dstval;
                dstbyte=dstbits;
                dstval=0;
                for (x=0; x<width; x++) {
                    PALETTEENTRY srcval;
                    srcval=srccolors[XGetPixel(bmpImage, x, h)];
                    dstval|=(X11DRV_DIB_GetNearestIndex
                             (colors, 2,
                              srcval.peRed,
                              srcval.peGreen,
                              srcval.peBlue) << (7 - (x & 7)));
                    if ((x&7)==7) {
                        *dstbyte++=dstval;
                        dstval=0;
                    }
                }
                if ((width&7)!=0) {
                    *dstbyte++=dstval;
                }
                /* pad with 0 to DWORD alignment */
                for (x = (x+7)&~7; x < ((width + 31) & ~31); x+=8)
                    *dstbyte++ = 0;
                dstbits += linebytes;
            }
        } else {
            goto notsupported;
        }
        break;

    case 8:
        if (X11DRV_DIB_CheckMask(bmpImage->red_mask, bmpImage->green_mask, bmpImage->blue_mask)
            && srccolors) {
            /* ==== pal 8 bmp -> pal 1 dib ==== */
            const void* srcbits;
            const BYTE* srcpixel;
            BYTE* dstbyte;

            srcbits=bmpImage->data+(lines-1)*bmpImage->bytes_per_line;

            for (h=0; h<lines; h++) {
                BYTE dstval;
                srcpixel=srcbits;
                dstbyte=dstbits;
                dstval=0;
                for (x=0; x<width; x++) {
                    PALETTEENTRY srcval;
                    srcval=srccolors[*srcpixel++];
                    dstval|=(X11DRV_DIB_GetNearestIndex
                             (colors, 2,
                              srcval.peRed,
                              srcval.peGreen,
                              srcval.peBlue) << (7-(x&7)) );
                    if ((x&7)==7) {
                        *dstbyte++=dstval;
                        dstval=0;
                    }
                }
                if ((width&7)!=0) {
                    *dstbyte=dstval;
                }
                srcbits = (const char*)srcbits - bmpImage->bytes_per_line;
                dstbits += linebytes;
            }
        } else {
            goto notsupported;
        }
        break;

    case 15:
    case 16:
        {
            const void* srcbits;
            const WORD* srcpixel;
            BYTE* dstbyte;

            srcbits=bmpImage->data+(lines-1)*bmpImage->bytes_per_line;

            if (bmpImage->green_mask==0x03e0) {
                if (bmpImage->red_mask==0x7c00) {
                    /* ==== rgb 555 bmp -> pal 1 dib ==== */
                    for (h=0; h<lines; h++) {
                        BYTE dstval;
                        srcpixel=srcbits;
                        dstbyte=dstbits;
                        dstval=0;
                        for (x=0; x<width; x++) {
                            WORD srcval;
                            srcval=*srcpixel++;
                            dstval|=(X11DRV_DIB_GetNearestIndex
                                     (colors, 2,
                                      ((srcval >>  7) & 0xf8) | /* r */
                                      ((srcval >> 12) & 0x07),
                                      ((srcval >>  2) & 0xf8) | /* g */
                                      ((srcval >>  7) & 0x07),
                                      ((srcval <<  3) & 0xf8) | /* b */
                                      ((srcval >>  2) & 0x07) ) << (7-(x&7)) );
                            if ((x&7)==7) {
                                *dstbyte++=dstval;
                                dstval=0;
                            }
                        }
                        if ((width&7)!=0) {
                            *dstbyte=dstval;
                        }
                        srcbits = (const char*)srcbits - bmpImage->bytes_per_line;
                        dstbits += linebytes;
                    }
                } else if (bmpImage->blue_mask==0x7c00) {
                    /* ==== bgr 555 bmp -> pal 1 dib ==== */
                    for (h=0; h<lines; h++) {
                        WORD dstval;
                        srcpixel=srcbits;
                        dstbyte=dstbits;
                        dstval=0;
                        for (x=0; x<width; x++) {
                            BYTE srcval;
                            srcval=*srcpixel++;
                            dstval|=(X11DRV_DIB_GetNearestIndex
                                     (colors, 2,
                                      ((srcval <<  3) & 0xf8) | /* r */
                                      ((srcval >>  2) & 0x07),
                                      ((srcval >>  2) & 0xf8) | /* g */
                                      ((srcval >>  7) & 0x07),
                                      ((srcval >>  7) & 0xf8) | /* b */
                                      ((srcval >> 12) & 0x07) ) << (7-(x&7)) );
                            if ((x&7)==7) {
                                *dstbyte++=dstval;
                                dstval=0;
                            }
                        }
                        if ((width&7)!=0) {
                            *dstbyte=dstval;
                        }
                        srcbits = (const char*)srcbits - bmpImage->bytes_per_line;
                        dstbits += linebytes;
                    }
                } else {
                    goto notsupported;
                }
            } else if (bmpImage->green_mask==0x07e0) {
                if (bmpImage->red_mask==0xf800) {
                    /* ==== rgb 565 bmp -> pal 1 dib ==== */
                    for (h=0; h<lines; h++) {
                        BYTE dstval;
                        srcpixel=srcbits;
                        dstbyte=dstbits;
                        dstval=0;
                        for (x=0; x<width; x++) {
                            WORD srcval;
                            srcval=*srcpixel++;
                            dstval|=(X11DRV_DIB_GetNearestIndex
                                     (colors, 2,
                                      ((srcval >>  8) & 0xf8) | /* r */
                                      ((srcval >> 13) & 0x07),
                                      ((srcval >>  3) & 0xfc) | /* g */
                                      ((srcval >>  9) & 0x03),
                                      ((srcval <<  3) & 0xf8) | /* b */
                                      ((srcval >>  2) & 0x07) ) << (7-(x&7)) );
                            if ((x&7)==7) {
                                *dstbyte++=dstval;
                                dstval=0;
                            }
                        }
                        if ((width&7)!=0) {
                            *dstbyte=dstval;
                        }
                        srcbits = (const char*)srcbits - bmpImage->bytes_per_line;
                        dstbits += linebytes;
                    }
                } else if (bmpImage->blue_mask==0xf800) {
                    /* ==== bgr 565 bmp -> pal 1 dib ==== */
                    for (h=0; h<lines; h++) {
                        BYTE dstval;
                        srcpixel=srcbits;
                        dstbyte=dstbits;
                        dstval=0;
                        for (x=0; x<width; x++) {
                            WORD srcval;
                            srcval=*srcpixel++;
                            dstval|=(X11DRV_DIB_GetNearestIndex
                                     (colors, 2,
                                      ((srcval <<  3) & 0xf8) | /* r */
                                      ((srcval >>  2) & 0x07),
                                      ((srcval >>  3) & 0xfc) | /* g */
                                      ((srcval >>  9) & 0x03),
                                      ((srcval >>  8) & 0xf8) | /* b */
                                      ((srcval >> 13) & 0x07) ) << (7-(x&7)) );
                            if ((x&7)==7) {
                                *dstbyte++=dstval;
                                dstval=0;
                            }
                        }
                        if ((width&7)!=0) {
                            *dstbyte=dstval;
                        }
                        srcbits = (const char*)srcbits - bmpImage->bytes_per_line;
                        dstbits += linebytes;
                    }
                } else {
                    goto notsupported;
                }
            } else {
                goto notsupported;
            }
        }
        break;

    case 24:
    case 32:
        {
            const void* srcbits;
            const BYTE *srcbyte;
            BYTE* dstbyte;
            int bytes_per_pixel;

            srcbits=bmpImage->data+(lines-1)*bmpImage->bytes_per_line;
            bytes_per_pixel=(bmpImage->bits_per_pixel==24?3:4);

            if (bmpImage->green_mask!=0x00ff00 ||
                (bmpImage->red_mask|bmpImage->blue_mask)!=0xff00ff) {
                goto notsupported;
            } else if (bmpImage->blue_mask==0xff) {
                /* ==== rgb 888 or 0888 bmp -> pal 1 dib ==== */
                for (h=0; h<lines; h++) {
                    BYTE dstval;
                    srcbyte=srcbits;
                    dstbyte=dstbits;
                    dstval=0;
                    for (x=0; x<width; x++) {
                        dstval|=(X11DRV_DIB_GetNearestIndex
                                 (colors, 2,
                                  srcbyte[2],
                                  srcbyte[1],
                                  srcbyte[0]) << (7-(x&7)) );
                        srcbyte+=bytes_per_pixel;
                        if ((x&7)==7) {
                            *dstbyte++=dstval;
                            dstval=0;
                        }
                    }
                    if ((width&7)!=0) {
                        *dstbyte=dstval;
                    }
                    srcbits = (const char*)srcbits - bmpImage->bytes_per_line;
                    dstbits += linebytes;
                }
            } else {
                /* ==== bgr 888 or 0888 bmp -> pal 1 dib ==== */
                for (h=0; h<lines; h++) {
                    BYTE dstval;
                    srcbyte=srcbits;
                    dstbyte=dstbits;
                    dstval=0;
                    for (x=0; x<width; x++) {
                        dstval|=(X11DRV_DIB_GetNearestIndex
                                 (colors, 2,
                                  srcbyte[0],
                                  srcbyte[1],
                                  srcbyte[2]) << (7-(x&7)) );
                        srcbyte+=bytes_per_pixel;
                        if ((x&7)==7) {
                            *dstbyte++=dstval;
                            dstval=0;
                        }
                    }
                    if ((width&7)!=0) {
                        *dstbyte=dstval;
                    }
                    srcbits = (const char*)srcbits - bmpImage->bytes_per_line;
                    dstbits += linebytes;
                }
            }
        }
        break;

    default:
    notsupported:
        {
            BYTE* dstbyte;
            BYTE neg = 0;
            unsigned long white = (1 << bmpImage->bits_per_pixel) - 1;

            /* ==== any bmp format -> pal 1 dib ==== */
            if ((unsigned)colors[0].rgbRed+colors[0].rgbGreen+colors[0].rgbBlue >=
                (unsigned)colors[1].rgbRed+colors[1].rgbGreen+colors[1].rgbBlue )
                neg = 1;

            WARN("from unknown %d bit bitmap (%lx,%lx,%lx) to 1 bit DIB, "
                 "%s color mapping\n",
                  bmpImage->bits_per_pixel, bmpImage->red_mask,
                  bmpImage->green_mask, bmpImage->blue_mask,
                  neg?"negative":"direct" );

            for (h=lines-1; h>=0; h--) {
                BYTE dstval;
                dstbyte=dstbits;
                dstval=0;
                for (x=0; x<width; x++) {
                    dstval|=((XGetPixel( bmpImage, x, h) >= white) ^ neg) << (7 - (x&7));
                    if ((x&7)==7) {
                        *dstbyte++=dstval;
                        dstval=0;
                    }
                }
                if ((width&7)!=0) {
                    *dstbyte=dstval;
                }
                dstbits += linebytes;
            }
        }
        break;
    }
}

/***********************************************************************
 *           X11DRV_DIB_SetImageBits_4
 *
 * SetDIBits for a 4-bit deep DIB.
 */
static void X11DRV_DIB_SetImageBits_4( int lines, const BYTE *srcbits,
                                DWORD srcwidth, DWORD dstwidth, int left,
                                int *colors, XImage *bmpImage, int linebytes)
{
    int h, width;
    const BYTE* srcbyte;
    DWORD i, x;

    if (lines < 0 ) {
        lines = -lines;
        srcbits = srcbits + linebytes * (lines - 1);
        linebytes = -linebytes;
    }

    if (left & 1) {
        left--;
        dstwidth++;
    }
    srcbits += left >> 1;
    width = min(srcwidth, dstwidth);

    /* ==== pal 4 dib -> any bmp format ==== */
    for (h = lines-1; h >= 0; h--) {
        srcbyte=srcbits;
        for (i = width/2, x = left; i > 0; i--) {
            BYTE srcval=*srcbyte++;
            XPutPixel( bmpImage, x++, h, colors[srcval >> 4] );
            XPutPixel( bmpImage, x++, h, colors[srcval & 0x0f] );
        }
        if (width & 1)
            XPutPixel( bmpImage, x, h, colors[*srcbyte >> 4] );
        srcbits += linebytes;
    }
}



/***********************************************************************
 *           X11DRV_DIB_GetImageBits_4
 *
 * GetDIBits for a 4-bit deep DIB.
 */
static void X11DRV_DIB_GetImageBits_4( int lines, BYTE *dstbits,
				       DWORD srcwidth, DWORD dstwidth,
				       RGBQUAD *colors, PALETTEENTRY *srccolors,
				       XImage *bmpImage, int linebytes )
{
    DWORD x;
    int h, width = min(srcwidth, dstwidth);

    if (lines < 0 )
    {
       lines = -lines;
       dstbits = dstbits + ( linebytes * (lines-1) );
       linebytes = -linebytes;
    }

    switch (bmpImage->depth) {
    case 1:
    case 4:
        if (X11DRV_DIB_CheckMask(bmpImage->red_mask,bmpImage->green_mask,bmpImage->blue_mask)
            && srccolors) {
            /* ==== pal 1 or 4 bmp -> pal 4 dib ==== */
            BYTE* dstbyte;

            for (h = lines-1; h >= 0; h--) {
                BYTE dstval;
                dstbyte=dstbits;
                dstval=0;
                for (x = 0; x < width; x++) {
                    PALETTEENTRY srcval;
                    srcval=srccolors[XGetPixel(bmpImage, x, h)];
                    dstval|=(X11DRV_DIB_GetNearestIndex
                             (colors, 16,
                              srcval.peRed,
                              srcval.peGreen,
                              srcval.peBlue) << (4-((x&1)<<2)));
                    if ((x&1)==1) {
                        *dstbyte++=dstval;
                        dstval=0;
                    }
                }
                if ((width&1)!=0) {
                    *dstbyte=dstval;
                }
                dstbits += linebytes;
            }
        } else {
            goto notsupported;
        }
        break;

    case 8:
        if (X11DRV_DIB_CheckMask(bmpImage->red_mask,bmpImage->green_mask,bmpImage->blue_mask)
            && srccolors) {
            /* ==== pal 8 bmp -> pal 4 dib ==== */
            const void* srcbits;
            const BYTE *srcpixel;
            BYTE* dstbyte;

            srcbits=bmpImage->data+(lines-1)*bmpImage->bytes_per_line;
            for (h=0; h<lines; h++) {
                BYTE dstval;
                srcpixel=srcbits;
                dstbyte=dstbits;
                dstval=0;
                for (x=0; x<width; x++) {
                    PALETTEENTRY srcval;
                    srcval = srccolors[*srcpixel++];
                    dstval|=(X11DRV_DIB_GetNearestIndex
                             (colors, 16,
                              srcval.peRed,
                              srcval.peGreen,
                              srcval.peBlue) << (4*(1-(x&1))) );
                    if ((x&1)==1) {
                        *dstbyte++=dstval;
                        dstval=0;
                    }
                }
                if ((width&1)!=0) {
                    *dstbyte=dstval;
                }
                srcbits = (const char*)srcbits - bmpImage->bytes_per_line;
                dstbits += linebytes;
            }
        } else {
            goto notsupported;
        }
        break;

    case 15:
    case 16:
        {
            const void* srcbits;
            const WORD* srcpixel;
            BYTE* dstbyte;

            srcbits=bmpImage->data+(lines-1)*bmpImage->bytes_per_line;

            if (bmpImage->green_mask==0x03e0) {
                if (bmpImage->red_mask==0x7c00) {
                    /* ==== rgb 555 bmp -> pal 4 dib ==== */
                    for (h=0; h<lines; h++) {
                        BYTE dstval;
                        srcpixel=srcbits;
                        dstbyte=dstbits;
                        dstval=0;
                        for (x=0; x<width; x++) {
                            WORD srcval;
                            srcval=*srcpixel++;
                            dstval|=(X11DRV_DIB_GetNearestIndex
                                     (colors, 16,
                                      ((srcval >>  7) & 0xf8) | /* r */
                                      ((srcval >> 12) & 0x07),
                                      ((srcval >>  2) & 0xf8) | /* g */
                                      ((srcval >>  7) & 0x07),
                                      ((srcval <<  3) & 0xf8) | /* b */
                                      ((srcval >>  2) & 0x07) ) << ((1-(x&1))<<2) );
                            if ((x&1)==1) {
                                *dstbyte++=dstval;
                                dstval=0;
                            }
                        }
                        if ((width&1)!=0) {
                            *dstbyte=dstval;
                        }
                        srcbits = (const char*)srcbits - bmpImage->bytes_per_line;
                        dstbits += linebytes;
                    }
                } else if (bmpImage->blue_mask==0x7c00) {
                    /* ==== bgr 555 bmp -> pal 4 dib ==== */
                    for (h=0; h<lines; h++) {
                        WORD dstval;
                        srcpixel=srcbits;
                        dstbyte=dstbits;
                        dstval=0;
                        for (x=0; x<width; x++) {
                            WORD srcval;
                            srcval=*srcpixel++;
                            dstval|=(X11DRV_DIB_GetNearestIndex
                                     (colors, 16,
                                      ((srcval <<  3) & 0xf8) | /* r */
                                      ((srcval >>  2) & 0x07),
                                      ((srcval >>  2) & 0xf8) | /* g */
                                      ((srcval >>  7) & 0x07),
                                      ((srcval >>  7) & 0xf8) | /* b */
                                      ((srcval >> 12) & 0x07) ) << ((1-(x&1))<<2) );
                            if ((x&1)==1) {
                                *dstbyte++=dstval;
                                dstval=0;
                            }
                        }
                        if ((width&1)!=0) {
                            *dstbyte=dstval;
                        }
                        srcbits = (const char*)srcbits - bmpImage->bytes_per_line;
                        dstbits += linebytes;
                    }
                } else {
                    goto notsupported;
                }
            } else if (bmpImage->green_mask==0x07e0) {
                if (bmpImage->red_mask==0xf800) {
                    /* ==== rgb 565 bmp -> pal 4 dib ==== */
                    for (h=0; h<lines; h++) {
                        BYTE dstval;
                        srcpixel=srcbits;
                        dstbyte=dstbits;
                        dstval=0;
                        for (x=0; x<width; x++) {
                            WORD srcval;
                            srcval=*srcpixel++;
                            dstval|=(X11DRV_DIB_GetNearestIndex
                                     (colors, 16,
                                      ((srcval >>  8) & 0xf8) | /* r */
                                      ((srcval >> 13) & 0x07),
                                      ((srcval >>  3) & 0xfc) | /* g */
                                      ((srcval >>  9) & 0x03),
                                      ((srcval <<  3) & 0xf8) | /* b */
                                      ((srcval >>  2) & 0x07) ) << ((1-(x&1))<<2) );
                            if ((x&1)==1) {
                                *dstbyte++=dstval;
                                dstval=0;
                            }
                        }
                        if ((width&1)!=0) {
                            *dstbyte=dstval;
                        }
                        srcbits = (const char*)srcbits - bmpImage->bytes_per_line;
                        dstbits += linebytes;
                    }
                } else if (bmpImage->blue_mask==0xf800) {
                    /* ==== bgr 565 bmp -> pal 4 dib ==== */
                    for (h=0; h<lines; h++) {
                        WORD dstval;
                        srcpixel=srcbits;
                        dstbyte=dstbits;
                        dstval=0;
                        for (x=0; x<width; x++) {
                            WORD srcval;
                            srcval=*srcpixel++;
                            dstval|=(X11DRV_DIB_GetNearestIndex
                                     (colors, 16,
                                      ((srcval <<  3) & 0xf8) | /* r */
                                      ((srcval >>  2) & 0x07),
                                      ((srcval >>  3) & 0xfc) | /* g */
                                      ((srcval >>  9) & 0x03),
                                      ((srcval >>  8) & 0xf8) | /* b */
                                      ((srcval >> 13) & 0x07) ) << ((1-(x&1))<<2) );
                            if ((x&1)==1) {
                                *dstbyte++=dstval;
                                dstval=0;
                            }
                        }
                        if ((width&1)!=0) {
                            *dstbyte=dstval;
                        }
                        srcbits = (const char*)srcbits - bmpImage->bytes_per_line;
                        dstbits += linebytes;
                    }
                } else {
                    goto notsupported;
                }
            } else {
                goto notsupported;
            }
        }
        break;

    case 24:
        if (bmpImage->bits_per_pixel==24) {
            const void* srcbits;
            const BYTE *srcbyte;
            BYTE* dstbyte;

            srcbits=bmpImage->data+(lines-1)*bmpImage->bytes_per_line;

            if (bmpImage->green_mask!=0x00ff00 ||
                (bmpImage->red_mask|bmpImage->blue_mask)!=0xff00ff) {
                goto notsupported;
            } else if (bmpImage->blue_mask==0xff) {
                /* ==== rgb 888 bmp -> pal 4 dib ==== */
                for (h=0; h<lines; h++) {
                    srcbyte=srcbits;
                    dstbyte=dstbits;
                    for (x=0; x<width/2; x++) {
                        /* Do 2 pixels at a time */
                        *dstbyte++=(X11DRV_DIB_GetNearestIndex
                                    (colors, 16,
                                     srcbyte[2],
                                     srcbyte[1],
                                     srcbyte[0]) << 4) |
                                    X11DRV_DIB_GetNearestIndex
                                    (colors, 16,
                                     srcbyte[5],
                                     srcbyte[4],
                                     srcbyte[3]);
                        srcbyte+=6;
                    }
                    if (width&1) {
                        /* And then the odd pixel */
                        *dstbyte++=(X11DRV_DIB_GetNearestIndex
                                    (colors, 16,
                                     srcbyte[2],
                                     srcbyte[1],
                                     srcbyte[0]) << 4);
                    }
                    srcbits = (const char*)srcbits - bmpImage->bytes_per_line;
                    dstbits += linebytes;
                }
            } else {
                /* ==== bgr 888 bmp -> pal 4 dib ==== */
                for (h=0; h<lines; h++) {
                    srcbyte=srcbits;
                    dstbyte=dstbits;
                    for (x=0; x<width/2; x++) {
                        /* Do 2 pixels at a time */
                        *dstbyte++=(X11DRV_DIB_GetNearestIndex
                                    (colors, 16,
                                     srcbyte[0],
                                     srcbyte[1],
                                     srcbyte[2]) << 4) |
                                    X11DRV_DIB_GetNearestIndex
                                    (colors, 16,
                                     srcbyte[3],
                                     srcbyte[4],
                                     srcbyte[5]);
                        srcbyte+=6;
                    }
                    if (width&1) {
                        /* And then the odd pixel */
                        *dstbyte++=(X11DRV_DIB_GetNearestIndex
                                    (colors, 16,
                                     srcbyte[0],
                                     srcbyte[1],
                                     srcbyte[2]) << 4);
                    }
                    srcbits = (const char*)srcbits - bmpImage->bytes_per_line;
                    dstbits += linebytes;
                }
            }
            break;
        }
        /* Fall through */

    case 32:
        {
            const void* srcbits;
            const BYTE *srcbyte;
            BYTE* dstbyte;

            srcbits=bmpImage->data+(lines-1)*bmpImage->bytes_per_line;

            if (bmpImage->green_mask!=0x00ff00 ||
                (bmpImage->red_mask|bmpImage->blue_mask)!=0xff00ff) {
                goto notsupported;
            } else if (bmpImage->blue_mask==0xff) {
                /* ==== rgb 0888 bmp -> pal 4 dib ==== */
                for (h=0; h<lines; h++) {
                    srcbyte=srcbits;
                    dstbyte=dstbits;
                    for (x=0; x<width/2; x++) {
                        /* Do 2 pixels at a time */
                        *dstbyte++=(X11DRV_DIB_GetNearestIndex
                                    (colors, 16,
                                     srcbyte[2],
                                     srcbyte[1],
                                     srcbyte[0]) << 4) |
                                    X11DRV_DIB_GetNearestIndex
                                    (colors, 16,
                                     srcbyte[6],
                                     srcbyte[5],
                                     srcbyte[4]);
                        srcbyte+=8;
                    }
                    if (width&1) {
                        /* And then the odd pixel */
                        *dstbyte++=(X11DRV_DIB_GetNearestIndex
                                    (colors, 16,
                                     srcbyte[2],
                                     srcbyte[1],
                                     srcbyte[0]) << 4);
                    }
                    srcbits = (const char*)srcbits - bmpImage->bytes_per_line;
                    dstbits += linebytes;
                }
            } else {
                /* ==== bgr 0888 bmp -> pal 4 dib ==== */
                for (h=0; h<lines; h++) {
                    srcbyte=srcbits;
                    dstbyte=dstbits;
                    for (x=0; x<width/2; x++) {
                        /* Do 2 pixels at a time */
                        *dstbyte++=(X11DRV_DIB_GetNearestIndex
                                    (colors, 16,
                                     srcbyte[0],
                                     srcbyte[1],
                                     srcbyte[2]) << 4) |
                                    X11DRV_DIB_GetNearestIndex
                                    (colors, 16,
                                     srcbyte[4],
                                     srcbyte[5],
                                     srcbyte[6]);
                        srcbyte+=8;
                    }
                    if (width&1) {
                        /* And then the odd pixel */
                        *dstbyte++=(X11DRV_DIB_GetNearestIndex
                                    (colors, 16,
                                     srcbyte[0],
                                     srcbyte[1],
                                     srcbyte[2]) << 4);
                    }
                    srcbits = (const char*)srcbits - bmpImage->bytes_per_line;
                    dstbits += linebytes;
                }
            }
        }
        break;

    default:
    notsupported:
        {
            BYTE* dstbyte;

            /* ==== any bmp format -> pal 4 dib ==== */
            WARN("from unknown %d bit bitmap (%lx,%lx,%lx) to 4 bit DIB\n",
                  bmpImage->bits_per_pixel, bmpImage->red_mask,
                  bmpImage->green_mask, bmpImage->blue_mask );
            for (h=lines-1; h>=0; h--) {
                dstbyte=dstbits;
                for (x=0; x<(width & ~1); x+=2) {
                    *dstbyte++=(X11DRV_DIB_MapColor((int*)colors, 16, XGetPixel(bmpImage, x, h), 0) << 4) |
                        X11DRV_DIB_MapColor((int*)colors, 16, XGetPixel(bmpImage, x+1, h), 0);
                }
                if (width & 1) {
                    *dstbyte=(X11DRV_DIB_MapColor((int *)colors, 16, XGetPixel(bmpImage, x, h), 0) << 4);
                }
                dstbits += linebytes;
            }
        }
        break;
    }
}

/***********************************************************************
 *           X11DRV_DIB_SetImageBits_RLE4
 *
 * SetDIBits for a 4-bit deep compressed DIB.
 */
static void X11DRV_DIB_SetImageBits_RLE4( int lines, const BYTE *bits,
					  DWORD srcwidth, DWORD dstwidth,
					  int left, int *colors,
					  XImage *bmpImage )
{
    unsigned int x = 0, width = min(srcwidth, dstwidth);
    int y = lines - 1, c, length;
    const BYTE *begin = bits;

    while (y >= 0)
    {
        length = *bits++;
	if (length) {	/* encoded */
	    c = *bits++;
	    while (length--) {
                if (x >= (left + width)) break;
                if( x >= left) XPutPixel(bmpImage, x, y, colors[c >> 4]);
                x++;
                if (!length--) break;
                if (x >= (left + width)) break;
                if( x >= left) XPutPixel(bmpImage, x, y, colors[c & 0xf]);
                x++;
	    }
	} else {
	    length = *bits++;
	    switch (length)
            {
            case RLE_EOL:
                x = 0;
                y--;
                break;

            case RLE_END:
	        return;

            case RLE_DELTA:
                x += *bits++;
                y -= *bits++;
                break;

	    default: /* absolute */
	        while (length--) {
		    c = *bits++;
                    if (x >= left && x < (left + width))
                        XPutPixel(bmpImage, x, y, colors[c >> 4]);
                    x++;
                    if (!length--) break;
                    if (x >= left && x < (left + width))
                        XPutPixel(bmpImage, x, y, colors[c & 0xf]);
                    x++;
		}
		if ((bits - begin) & 1)
		    bits++;
	    }
	}
    }
}



/***********************************************************************
 *           X11DRV_DIB_SetImageBits_8
 *
 * SetDIBits for an 8-bit deep DIB.
 */
static void X11DRV_DIB_SetImageBits_8( int lines, const BYTE *srcbits,
				DWORD srcwidth, DWORD dstwidth, int left,
                                const int *colors, XImage *bmpImage,
				int linebytes )
{
    DWORD x;
    int h, width = min(srcwidth, dstwidth);
    const BYTE* srcbyte;
    BYTE* dstbits;

    if (lines < 0 )
    {
        lines = -lines;
        srcbits = srcbits + linebytes * (lines-1);
        linebytes = -linebytes;
    }
    srcbits += left;
    srcbyte = srcbits;

    switch (bmpImage->depth) {
    case 15:
    case 16:
	/* Some X servers might have 32 bit/ 16bit deep pixel */
	if (lines && width && (bmpImage->bits_per_pixel == 16) &&
            (ImageByteOrder(gdi_display)==LSBFirst) )
	{
	    /* ==== pal 8 dib -> rgb or bgr 555 or 565 bmp ==== */
	    dstbits=(BYTE*)bmpImage->data+left*2+(lines-1)*bmpImage->bytes_per_line;
	    for (h = lines ; h--; ) {
#if defined(__i386__) && defined(__GNUC__)
		int _cl1,_cl2; /* temp outputs for asm below */
		/* Borrowed from DirectDraw */
		__asm__ __volatile__(
		"xor %%eax,%%eax\n"
		"cld\n"
		"1:\n"
		"    lodsb\n"
		"    movw (%%edx,%%eax,4),%%ax\n"
		"    stosw\n"
		"      xor %%eax,%%eax\n"
		"    loop 1b\n"
		:"=S" (srcbyte), "=D" (_cl1), "=c" (_cl2)
		:"S" (srcbyte),
		 "D" (dstbits),
		 "c" (width),
		 "d" (colors)
		:"eax", "cc", "memory"
		);
#else
		DWORD* dstpixel=(DWORD*)dstbits;
		for (x=0; x<width/2; x++) {
		    /* Do 2 pixels at a time */
		    *dstpixel++=(colors[srcbyte[1]] << 16) | colors[srcbyte[0]];
		    srcbyte+=2;
		}
		if (width&1) {
		    /* And then the odd pixel */
		    *((WORD*)dstpixel)=colors[srcbyte[0]];
		}
#endif
		srcbyte = (srcbits += linebytes);
		dstbits -= bmpImage->bytes_per_line;
	    }
	    return;
	}
	break;
    case 24:
    case 32:
	if (lines && width && (bmpImage->bits_per_pixel == 32) &&
            (ImageByteOrder(gdi_display)==LSBFirst) )
	{
	    dstbits=(BYTE*)bmpImage->data+left*4+(lines-1)*bmpImage->bytes_per_line;
	    /* ==== pal 8 dib -> rgb or bgr 0888 bmp ==== */
	    for (h = lines ; h--; ) {
#if defined(__i386__) && defined(__GNUC__)
		int _cl1,_cl2; /* temp outputs for asm below */
		/* Borrowed from DirectDraw */
		__asm__ __volatile__(
		"xor %%eax,%%eax\n"
		"cld\n"
		"1:\n"
		"    lodsb\n"
		"    movl (%%edx,%%eax,4),%%eax\n"
		"    stosl\n"
		"      xor %%eax,%%eax\n"
		"    loop 1b\n"
		:"=S" (srcbyte), "=D" (_cl1), "=c" (_cl2)
		:"S" (srcbyte),
		 "D" (dstbits),
		 "c" (width),
		 "d" (colors)
		:"eax", "cc", "memory"
		);
#else
		DWORD* dstpixel=(DWORD*)dstbits;
		for (x=0; x<width; x++) {
		    *dstpixel++=colors[*srcbyte++];
		}
#endif
		srcbyte = (srcbits += linebytes);
		dstbits -= bmpImage->bytes_per_line;
	    }
	    return;
	}
	break;
    default:
        break; /* use slow generic case below */
    }

    /* ==== pal 8 dib -> any bmp format ==== */
    for (h=lines-1; h>=0; h--) {
        for (x=left; x<width+left; x++) {
            XPutPixel(bmpImage, x, h, colors[*srcbyte++]);
        }
        srcbyte = (srcbits += linebytes);
    }
}

/***********************************************************************
 *           X11DRV_DIB_GetImageBits_8
 *
 * GetDIBits for an 8-bit deep DIB.
 */
static void X11DRV_DIB_GetImageBits_8( int lines, BYTE *dstbits,
				       DWORD srcwidth, DWORD dstwidth,
				       RGBQUAD *colors, PALETTEENTRY *srccolors,
				       XImage *bmpImage, int linebytes )
{
    DWORD x;
    int h, width = min(srcwidth, dstwidth);
    BYTE* dstbyte;

    if (lines < 0 )
    {
       lines = -lines;
       dstbits = dstbits + ( linebytes * (lines-1) );
       linebytes = -linebytes;
    }

    /*
     * Hack for now
     * This condition is true when GetImageBits has been called by
     * UpdateDIBSection. For now, GetNearestIndex is too slow to support
     * 256 colormaps, so we'll just use it for GetDIBits calls.
     * (In some cases, in an updateDIBSection, the returned colors are bad too)
     */
    if (!srccolors) goto updatesection;

    switch (bmpImage->depth) {
    case 1:
    case 4:
        if (X11DRV_DIB_CheckMask(bmpImage->red_mask,bmpImage->green_mask,bmpImage->blue_mask)
            && srccolors) {

            /* ==== pal 1 bmp -> pal 8 dib ==== */
            /* ==== pal 4 bmp -> pal 8 dib ==== */
            for (h=lines-1; h>=0; h--) {
                dstbyte=dstbits;
                for (x=0; x<width; x++) {
                    PALETTEENTRY srcval;
                    srcval=srccolors[XGetPixel(bmpImage, x, h)];
                    *dstbyte++=X11DRV_DIB_GetNearestIndex(colors, 256,
                                                          srcval.peRed,
                                                          srcval.peGreen,
                                                          srcval.peBlue);
                }
                dstbits += linebytes;
            }
        } else {
            goto notsupported;
        }
        break;

    case 8:
       if (X11DRV_DIB_CheckMask(bmpImage->red_mask,bmpImage->green_mask,bmpImage->blue_mask)
           && srccolors) {
            /* ==== pal 8 bmp -> pal 8 dib ==== */
           const void* srcbits;
           const BYTE* srcpixel;

           srcbits=bmpImage->data+(lines-1)*bmpImage->bytes_per_line;
           for (h=0; h<lines; h++) {
               srcpixel=srcbits;
               dstbyte=dstbits;
               for (x = 0; x < width; x++) {
                   PALETTEENTRY srcval;
                   srcval=srccolors[*srcpixel++];
                   *dstbyte++=X11DRV_DIB_GetNearestIndex(colors, 256,
                                                         srcval.peRed,
                                                         srcval.peGreen,
                                                         srcval.peBlue);
               }
               srcbits = (const char*)srcbits - bmpImage->bytes_per_line;
               dstbits += linebytes;
           }
       } else {
           goto notsupported;
       }
       break;

    case 15:
    case 16:
        {
            const void* srcbits;
            const WORD* srcpixel;
            BYTE* dstbyte;

            srcbits=bmpImage->data+(lines-1)*bmpImage->bytes_per_line;

            if (bmpImage->green_mask==0x03e0) {
                if (bmpImage->red_mask==0x7c00) {
                    /* ==== rgb 555 bmp -> pal 8 dib ==== */
                    for (h=0; h<lines; h++) {
                        srcpixel=srcbits;
                        dstbyte=dstbits;
                        for (x=0; x<width; x++) {
                            WORD srcval;
                            srcval=*srcpixel++;
                            *dstbyte++=X11DRV_DIB_GetNearestIndex
                                (colors, 256,
                                 ((srcval >>  7) & 0xf8) | /* r */
                                 ((srcval >> 12) & 0x07),
                                 ((srcval >>  2) & 0xf8) | /* g */
                                 ((srcval >>  7) & 0x07),
                                 ((srcval <<  3) & 0xf8) | /* b */
                                 ((srcval >>  2) & 0x07) );
                        }
                        srcbits = (const char*)srcbits - bmpImage->bytes_per_line;
                        dstbits += linebytes;
                    }
                } else if (bmpImage->blue_mask==0x7c00) {
                    /* ==== bgr 555 bmp -> pal 8 dib ==== */
                    for (h=0; h<lines; h++) {
                        srcpixel=srcbits;
                        dstbyte=dstbits;
                        for (x=0; x<width; x++) {
                            WORD srcval;
                            srcval=*srcpixel++;
                            *dstbyte++=X11DRV_DIB_GetNearestIndex
                                (colors, 256,
                                 ((srcval <<  3) & 0xf8) | /* r */
                                 ((srcval >>  2) & 0x07),
                                 ((srcval >>  2) & 0xf8) | /* g */
                                 ((srcval >>  7) & 0x07),
                                 ((srcval >>  7) & 0xf8) | /* b */
                                 ((srcval >> 12) & 0x07) );
                        }
                        srcbits = (const char*)srcbits - bmpImage->bytes_per_line;
                        dstbits += linebytes;
                    }
                } else {
                    goto notsupported;
                }
            } else if (bmpImage->green_mask==0x07e0) {
                if (bmpImage->red_mask==0xf800) {
                    /* ==== rgb 565 bmp -> pal 8 dib ==== */
                    for (h=0; h<lines; h++) {
                        srcpixel=srcbits;
                        dstbyte=dstbits;
                        for (x=0; x<width; x++) {
                            WORD srcval;
                            srcval=*srcpixel++;
                            *dstbyte++=X11DRV_DIB_GetNearestIndex
                                (colors, 256,
                                 ((srcval >>  8) & 0xf8) | /* r */
                                 ((srcval >> 13) & 0x07),
                                 ((srcval >>  3) & 0xfc) | /* g */
                                 ((srcval >>  9) & 0x03),
                                 ((srcval <<  3) & 0xf8) | /* b */
                                 ((srcval >>  2) & 0x07) );
                        }
                        srcbits = (const char*)srcbits - bmpImage->bytes_per_line;
                        dstbits += linebytes;
                    }
                } else if (bmpImage->blue_mask==0xf800) {
                    /* ==== bgr 565 bmp -> pal 8 dib ==== */
                    for (h=0; h<lines; h++) {
                        srcpixel=srcbits;
                        dstbyte=dstbits;
                        for (x=0; x<width; x++) {
                            WORD srcval;
                            srcval=*srcpixel++;
                            *dstbyte++=X11DRV_DIB_GetNearestIndex
                                (colors, 256,
                                 ((srcval <<  3) & 0xf8) | /* r */
                                 ((srcval >>  2) & 0x07),
                                 ((srcval >>  3) & 0xfc) | /* g */
                                 ((srcval >>  9) & 0x03),
                                 ((srcval >>  8) & 0xf8) | /* b */
                                 ((srcval >> 13) & 0x07) );
                        }
                        srcbits = (const char*)srcbits - bmpImage->bytes_per_line;
                        dstbits += linebytes;
                    }
                } else {
                    goto notsupported;
                }
            } else {
                goto notsupported;
            }
        }
        break;

    case 24:
    case 32:
        {
            const void* srcbits;
            const BYTE *srcbyte;
            BYTE* dstbyte;
            int bytes_per_pixel;

            srcbits=bmpImage->data+(lines-1)*bmpImage->bytes_per_line;
            bytes_per_pixel=(bmpImage->bits_per_pixel==24?3:4);

            if (bmpImage->green_mask!=0x00ff00 ||
                (bmpImage->red_mask|bmpImage->blue_mask)!=0xff00ff) {
                goto notsupported;
            } else if (bmpImage->blue_mask==0xff) {
                /* ==== rgb 888 or 0888 bmp -> pal 8 dib ==== */
                for (h=0; h<lines; h++) {
                    srcbyte=srcbits;
                    dstbyte=dstbits;
                    for (x=0; x<width; x++) {
                        *dstbyte++=X11DRV_DIB_GetNearestIndex
                            (colors, 256,
                             srcbyte[2],
                             srcbyte[1],
                             srcbyte[0]);
                        srcbyte+=bytes_per_pixel;
                    }
                    srcbits = (const char*)srcbits - bmpImage->bytes_per_line;
                    dstbits += linebytes;
                }
            } else {
                /* ==== bgr 888 or 0888 bmp -> pal 8 dib ==== */
                for (h=0; h<lines; h++) {
                    srcbyte=srcbits;
                    dstbyte=dstbits;
                    for (x=0; x<width; x++) {
                        *dstbyte++=X11DRV_DIB_GetNearestIndex
                            (colors, 256,
                             srcbyte[0],
                             srcbyte[1],
                             srcbyte[2]);
                        srcbyte+=bytes_per_pixel;
                    }
                    srcbits = (const char*)srcbits - bmpImage->bytes_per_line;
                    dstbits += linebytes;
                }
            }
        }
        break;

    default:
    notsupported:
        WARN("from unknown %d bit bitmap (%lx,%lx,%lx) to 8 bit DIB\n",
              bmpImage->depth, bmpImage->red_mask,
              bmpImage->green_mask, bmpImage->blue_mask );
    updatesection:
        /* ==== any bmp format -> pal 8 dib ==== */
        for (h=lines-1; h>=0; h--) {
            dstbyte=dstbits;
            for (x=0; x<width; x++) {
                *dstbyte=X11DRV_DIB_MapColor
                    ((int*)colors, 256,
                     XGetPixel(bmpImage, x, h), *dstbyte);
                dstbyte++;
            }
            dstbits += linebytes;
        }
        break;
    }
}

/***********************************************************************
 *	      X11DRV_DIB_SetImageBits_RLE8
 *
 * SetDIBits for an 8-bit deep compressed DIB.
 *
 * This function rewritten 941113 by James Youngman.  WINE blew out when I
 * first ran it because my desktop wallpaper is a (large) RLE8 bitmap.
 *
 * This was because the algorithm assumed that all RLE8 bitmaps end with the
 * 'End of bitmap' escape code.  This code is very much laxer in what it
 * allows to end the expansion.  Possibly too lax.  See the note by
 * case RleDelta.  BTW, MS's documentation implies that a correct RLE8
 * bitmap should end with RleEnd, but on the other hand, software exists
 * that produces ones that don't and Windows 3.1 doesn't complain a bit
 * about it.
 *
 * (No) apologies for my English spelling.  [Emacs users: c-indent-level=4].
 *			James A. Youngman <mbcstjy@afs.man.ac.uk>
 *						[JAY]
 */
static void X11DRV_DIB_SetImageBits_RLE8( int lines, const BYTE *bits,
					  DWORD srcwidth, DWORD dstwidth,
					  int left, int *colors,
					  XImage *bmpImage )
{
    unsigned int x;		/* X-position on each line.  Increases. */
    int y;			/* Line #.  Starts at lines-1, decreases */
    const BYTE *pIn = bits;     /* Pointer to current position in bits */
    BYTE length;		/* The length pf a run */
    BYTE escape_code;		/* See enum Rle8_EscapeCodes.*/

    /*
     * Note that the bitmap data is stored by Windows starting at the
     * bottom line of the bitmap and going upwards.  Within each line,
     * the data is stored left-to-right.  That's the reason why line
     * goes from lines-1 to 0.			[JAY]
     */

    x = 0;
    y = lines - 1;
    while (y >= 0)
    {
        length = *pIn++;

        /*
         * If the length byte is not zero (which is the escape value),
         * We have a run of length pixels all the same colour.  The colour
         * index is stored next.
         *
         * If the length byte is zero, we need to read the next byte to
         * know what to do.			[JAY]
         */
        if (length != 0)
        {
            /*
             * [Run-Length] Encoded mode
             */
            int color = colors[*pIn++];
            while (length-- && x < (left + dstwidth)) {
                if( x >= left) XPutPixel(bmpImage, x, y, color);
                x++;
            }
        }
        else
        {
            /*
             * Escape codes (may be an absolute sequence though)
             */
            escape_code = (*pIn++);
            switch(escape_code)
            {
            case RLE_EOL:
                x = 0;
                y--;
                break;

            case RLE_END:
                /* Not all RLE8 bitmaps end with this code.  For
                 * example, Paint Shop Pro produces some that don't.
                 * That's (I think) what caused the previous
                 * implementation to fail.  [JAY]
                 */
                return;

            case RLE_DELTA:
                x += (*pIn++);
                y -= (*pIn++);
                break;

            default:  /* switch to absolute mode */
                length = escape_code;
                while (length--)
                {
                    int color = colors[*pIn++];
                    if (x >= (left + dstwidth))
                    {
                        pIn += length;
                        break;
                    }
                    if( x >= left) XPutPixel(bmpImage, x, y, color);
                    x++;
                }
                /*
                 * If you think for a moment you'll realise that the
                 * only time we could ever possibly read an odd
                 * number of bytes is when there is a 0x00 (escape),
                 * a value >0x02 (absolute mode) and then an odd-
                 * length run.  Therefore this is the only place we
                 * need to worry about it.  Everywhere else the
                 * bytes are always read in pairs.  [JAY]
                 */
                if (escape_code & 1) pIn++; /* Throw away the pad byte. */
                break;
            } /* switch (escape_code) : Escape sequence */
        }
    }
}


/***********************************************************************
 *           X11DRV_DIB_SetImageBits_16
 *
 * SetDIBits for a 16-bit deep DIB.
 */
static void X11DRV_DIB_SetImageBits_16( int lines, const BYTE *srcbits,
                                        DWORD srcwidth, DWORD dstwidth, int left,
                                        X11DRV_PDEVICE *physDev, DWORD rSrc, DWORD gSrc, DWORD bSrc,
                                        XImage *bmpImage, int linebytes )
{
    DWORD x;
    int h, width = min(srcwidth, dstwidth);
    const dib_conversions *convs = (bmpImage->byte_order == LSBFirst) ? &dib_normal : &dib_dst_byteswap;

    if (lines < 0 )
    {
        lines = -lines;
        srcbits = srcbits + ( linebytes * (lines-1));
        linebytes = -linebytes;
    }

    switch (bmpImage->depth)
    {
    case 15:
    case 16:
        {
            char* dstbits;

            srcbits=srcbits+left*2;
            dstbits=bmpImage->data+left*2+(lines-1)*bmpImage->bytes_per_line;

            if (bmpImage->green_mask==0x03e0) {
                if (gSrc==bmpImage->green_mask) {
                    if (rSrc==bmpImage->red_mask) {
                        /* ==== rgb 555 dib -> rgb 555 bmp ==== */
                        /* ==== bgr 555 dib -> bgr 555 bmp ==== */
                        convs->Convert_5x5_asis
                            (width,lines,
                             srcbits,linebytes,
                             dstbits,-bmpImage->bytes_per_line);
                    } else if (rSrc==bmpImage->blue_mask) {
                        /* ==== rgb 555 dib -> bgr 555 bmp ==== */
                        /* ==== bgr 555 dib -> rgb 555 bmp ==== */
                        convs->Convert_555_reverse
                            (width,lines,
                             srcbits,linebytes,
                             dstbits,-bmpImage->bytes_per_line);
                    }
                } else {
                    if (rSrc==bmpImage->red_mask || bSrc==bmpImage->blue_mask) {
                        /* ==== rgb 565 dib -> rgb 555 bmp ==== */
                        /* ==== bgr 565 dib -> bgr 555 bmp ==== */
                        convs->Convert_565_to_555_asis
                            (width,lines,
                             srcbits,linebytes,
                             dstbits,-bmpImage->bytes_per_line);
                    } else {
                        /* ==== rgb 565 dib -> bgr 555 bmp ==== */
                        /* ==== bgr 565 dib -> rgb 555 bmp ==== */
                        convs->Convert_565_to_555_reverse
                            (width,lines,
                             srcbits,linebytes,
                             dstbits,-bmpImage->bytes_per_line);
                    }
                }
            } else if (bmpImage->green_mask==0x07e0) {
                if (gSrc==bmpImage->green_mask) {
                    if (rSrc==bmpImage->red_mask) {
                        /* ==== rgb 565 dib -> rgb 565 bmp ==== */
                        /* ==== bgr 565 dib -> bgr 565 bmp ==== */
                        convs->Convert_5x5_asis
                            (width,lines,
                             srcbits,linebytes,
                             dstbits,-bmpImage->bytes_per_line);
                    } else {
                        /* ==== rgb 565 dib -> bgr 565 bmp ==== */
                        /* ==== bgr 565 dib -> rgb 565 bmp ==== */
                        convs->Convert_565_reverse
                            (width,lines,
                             srcbits,linebytes,
                             dstbits,-bmpImage->bytes_per_line);
                    }
                } else {
                    if (rSrc==bmpImage->red_mask || bSrc==bmpImage->blue_mask) {
                        /* ==== rgb 555 dib -> rgb 565 bmp ==== */
                        /* ==== bgr 555 dib -> bgr 565 bmp ==== */
                        convs->Convert_555_to_565_asis
                            (width,lines,
                             srcbits,linebytes,
                             dstbits,-bmpImage->bytes_per_line);
                    } else {
                        /* ==== rgb 555 dib -> bgr 565 bmp ==== */
                        /* ==== bgr 555 dib -> rgb 565 bmp ==== */
                        convs->Convert_555_to_565_reverse
                            (width,lines,
                             srcbits,linebytes,
                             dstbits,-bmpImage->bytes_per_line);
                    }
                }
            } else {
                goto notsupported;
            }
        }
        break;

    case 24:
        if (bmpImage->bits_per_pixel==24) {
            char* dstbits;

            srcbits=srcbits+left*2;
            dstbits=bmpImage->data+left*3+(lines-1)*bmpImage->bytes_per_line;

            if (bmpImage->green_mask!=0x00ff00 ||
                (bmpImage->red_mask|bmpImage->blue_mask)!=0xff00ff) {
                goto notsupported;
            } else if ((rSrc==0x1f && bmpImage->red_mask==0xff) ||
                       (bSrc==0x1f && bmpImage->blue_mask==0xff)) {
                if (gSrc==0x03e0) {
                    /* ==== rgb 555 dib -> rgb 888 bmp ==== */
                    /* ==== bgr 555 dib -> bgr 888 bmp ==== */
                    convs->Convert_555_to_888_asis
                        (width,lines,
                         srcbits,linebytes,
                         dstbits,-bmpImage->bytes_per_line);
                } else {
                    /* ==== rgb 565 dib -> rgb 888 bmp ==== */
                    /* ==== bgr 565 dib -> bgr 888 bmp ==== */
                    convs->Convert_565_to_888_asis
                        (width,lines,
                         srcbits,linebytes,
                         dstbits,-bmpImage->bytes_per_line);
                }
            } else {
                if (gSrc==0x03e0) {
                    /* ==== rgb 555 dib -> bgr 888 bmp ==== */
                    /* ==== bgr 555 dib -> rgb 888 bmp ==== */
                    convs->Convert_555_to_888_reverse
                        (width,lines,
                         srcbits,linebytes,
                         dstbits,-bmpImage->bytes_per_line);
                } else {
                    /* ==== rgb 565 dib -> bgr 888 bmp ==== */
                    /* ==== bgr 565 dib -> rgb 888 bmp ==== */
                    convs->Convert_565_to_888_reverse
                        (width,lines,
                         srcbits,linebytes,
                         dstbits,-bmpImage->bytes_per_line);
                }
            }
            break;
        }
        /* Fall through */

    case 32:
        {
            char* dstbits;

            srcbits=srcbits+left*2;
            dstbits=bmpImage->data+left*4+(lines-1)*bmpImage->bytes_per_line;

            if (bmpImage->green_mask!=0x00ff00 ||
                (bmpImage->red_mask|bmpImage->blue_mask)!=0xff00ff) {
                goto notsupported;
            } else if ((rSrc==0x1f && bmpImage->red_mask==0xff) ||
                       (bSrc==0x1f && bmpImage->blue_mask==0xff)) {
                if (gSrc==0x03e0) {
                    /* ==== rgb 555 dib -> rgb 0888 bmp ==== */
                    /* ==== bgr 555 dib -> bgr 0888 bmp ==== */
                    convs->Convert_555_to_0888_asis
                        (width,lines,
                         srcbits,linebytes,
                         dstbits,-bmpImage->bytes_per_line);
                } else {
                    /* ==== rgb 565 dib -> rgb 0888 bmp ==== */
                    /* ==== bgr 565 dib -> bgr 0888 bmp ==== */
                    convs->Convert_565_to_0888_asis
                        (width,lines,
                         srcbits,linebytes,
                         dstbits,-bmpImage->bytes_per_line);
                }
            } else {
                if (gSrc==0x03e0) {
                    /* ==== rgb 555 dib -> bgr 0888 bmp ==== */
                    /* ==== bgr 555 dib -> rgb 0888 bmp ==== */
                    convs->Convert_555_to_0888_reverse
                        (width,lines,
                         srcbits,linebytes,
                         dstbits,-bmpImage->bytes_per_line);
                } else {
                    /* ==== rgb 565 dib -> bgr 0888 bmp ==== */
                    /* ==== bgr 565 dib -> rgb 0888 bmp ==== */
                    convs->Convert_565_to_0888_reverse
                        (width,lines,
                         srcbits,linebytes,
                         dstbits,-bmpImage->bytes_per_line);
                }
            }
        }
        break;

    default:
    notsupported:
        WARN("from 16 bit DIB (%x,%x,%x) to unknown %d bit bitmap (%lx,%lx,%lx)\n",
              rSrc, gSrc, bSrc, bmpImage->bits_per_pixel, bmpImage->red_mask,
              bmpImage->green_mask, bmpImage->blue_mask );
        /* fall through */
    case 1:
    case 4:
    case 8:
        {
            /* ==== rgb or bgr 555 or 565 dib -> pal 1, 4 or 8 ==== */
            const WORD* srcpixel;
            int rShift1,gShift1,bShift1;
            int rShift2,gShift2,bShift2;
            BYTE gMask1,gMask2;

            /* Set color scaling values */
            rShift1=16+X11DRV_DIB_MaskToShift(rSrc)-3;
            gShift1=16+X11DRV_DIB_MaskToShift(gSrc)-3;
            bShift1=16+X11DRV_DIB_MaskToShift(bSrc)-3;
            rShift2=rShift1+5;
            gShift2=gShift1+5;
            bShift2=bShift1+5;
            if (gSrc==0x03e0) {
                /* Green has 5 bits, like the others */
                gMask1=0xf8;
                gMask2=0x07;
            } else {
                /* Green has 6 bits, not 5. Compensate. */
                gShift1++;
                gShift2+=2;
                gMask1=0xfc;
                gMask2=0x03;
            }

            srcbits+=2*left;

            /* We could split it into four separate cases to optimize
             * but it is probably not worth it.
             */
            for (h=lines-1; h>=0; h--) {
                srcpixel=(const WORD*)srcbits;
                for (x=left; x<width+left; x++) {
                    DWORD srcval;
                    BYTE red,green,blue;
                    srcval=*srcpixel++ << 16;
                    red=  ((srcval >> rShift1) & 0xf8) |
                        ((srcval >> rShift2) & 0x07);
                    green=((srcval >> gShift1) & gMask1) |
                        ((srcval >> gShift2) & gMask2);
                    blue= ((srcval >> bShift1) & 0xf8) |
                        ((srcval >> bShift2) & 0x07);
                    XPutPixel(bmpImage, x, h,
                              X11DRV_PALETTE_ToPhysical
                              (physDev, RGB(red,green,blue)));
                }
                srcbits += linebytes;
            }
        }
        break;
    }
}


/***********************************************************************
 *           X11DRV_DIB_GetImageBits_16
 *
 * GetDIBits for an 16-bit deep DIB.
 */
static void X11DRV_DIB_GetImageBits_16( X11DRV_PDEVICE *physDev, int lines, BYTE *dstbits,
					DWORD dstwidth, DWORD srcwidth,
					PALETTEENTRY *srccolors,
					DWORD rDst, DWORD gDst, DWORD bDst,
					XImage *bmpImage, int linebytes )
{
    DWORD x;
    int h, width = min(srcwidth, dstwidth);
    const dib_conversions *convs = (bmpImage->byte_order == LSBFirst) ? &dib_normal : &dib_src_byteswap;

    if (lines < 0 )
    {
        lines = -lines;
        dstbits = dstbits + ( linebytes * (lines-1));
        linebytes = -linebytes;
    }

    switch (bmpImage->depth)
    {
    case 15:
    case 16:
        {
            const char* srcbits;

            srcbits=bmpImage->data+(lines-1)*bmpImage->bytes_per_line;

            if (bmpImage->green_mask==0x03e0) {
                if (gDst==bmpImage->green_mask) {
                    if (rDst==bmpImage->red_mask) {
                        /* ==== rgb 555 bmp -> rgb 555 dib ==== */
                        /* ==== bgr 555 bmp -> bgr 555 dib ==== */
                        convs->Convert_5x5_asis
                            (width,lines,
                             srcbits,-bmpImage->bytes_per_line,
                             dstbits,linebytes);
                    } else {
                        /* ==== rgb 555 bmp -> bgr 555 dib ==== */
                        /* ==== bgr 555 bmp -> rgb 555 dib ==== */
                        convs->Convert_555_reverse
                            (width,lines,
                             srcbits,-bmpImage->bytes_per_line,
                             dstbits,linebytes);
                    }
                } else {
                    if (rDst==bmpImage->red_mask || bDst==bmpImage->blue_mask) {
                        /* ==== rgb 555 bmp -> rgb 565 dib ==== */
                        /* ==== bgr 555 bmp -> bgr 565 dib ==== */
                        convs->Convert_555_to_565_asis
                            (width,lines,
                             srcbits,-bmpImage->bytes_per_line,
                             dstbits,linebytes);
                    } else {
                        /* ==== rgb 555 bmp -> bgr 565 dib ==== */
                        /* ==== bgr 555 bmp -> rgb 565 dib ==== */
                        convs->Convert_555_to_565_reverse
                            (width,lines,
                             srcbits,-bmpImage->bytes_per_line,
                             dstbits,linebytes);
                    }
                }
            } else if (bmpImage->green_mask==0x07e0) {
                if (gDst==bmpImage->green_mask) {
                    if (rDst == bmpImage->red_mask) {
                        /* ==== rgb 565 bmp -> rgb 565 dib ==== */
                        /* ==== bgr 565 bmp -> bgr 565 dib ==== */
                        convs->Convert_5x5_asis
                            (width,lines,
                             srcbits,-bmpImage->bytes_per_line,
                             dstbits,linebytes);
                    } else {
                        /* ==== rgb 565 bmp -> bgr 565 dib ==== */
                        /* ==== bgr 565 bmp -> rgb 565 dib ==== */
                        convs->Convert_565_reverse
                            (width,lines,
                             srcbits,-bmpImage->bytes_per_line,
                             dstbits,linebytes);
                    }
                } else {
                    if (rDst==bmpImage->red_mask || bDst==bmpImage->blue_mask) {
                        /* ==== rgb 565 bmp -> rgb 555 dib ==== */
                        /* ==== bgr 565 bmp -> bgr 555 dib ==== */
                        convs->Convert_565_to_555_asis
                            (width,lines,
                             srcbits,-bmpImage->bytes_per_line,
                             dstbits,linebytes);
                    } else {
                        /* ==== rgb 565 bmp -> bgr 555 dib ==== */
                        /* ==== bgr 565 bmp -> rgb 555 dib ==== */
                        convs->Convert_565_to_555_reverse
                            (width,lines,
                             srcbits,-bmpImage->bytes_per_line,
                             dstbits,linebytes);
                    }
                }
            } else {
                goto notsupported;
            }
        }
        break;

    case 24:
        if (bmpImage->bits_per_pixel == 24) {
            const char* srcbits;

            srcbits=bmpImage->data+(lines-1)*bmpImage->bytes_per_line;

            if (bmpImage->green_mask!=0x00ff00 ||
                (bmpImage->red_mask|bmpImage->blue_mask)!=0xff00ff) {
                goto notsupported;
            } else if ((rDst==0x1f && bmpImage->red_mask==0xff) ||
                    (bDst==0x1f && bmpImage->blue_mask==0xff)) {
                if (gDst==0x03e0) {
                    /* ==== rgb 888 bmp -> rgb 555 dib ==== */
                    /* ==== bgr 888 bmp -> bgr 555 dib ==== */
                    convs->Convert_888_to_555_asis
                        (width,lines,
                         srcbits,-bmpImage->bytes_per_line,
                         dstbits,linebytes);
                } else {
                    /* ==== rgb 888 bmp -> rgb 565 dib ==== */
                    /* ==== rgb 888 bmp -> rgb 565 dib ==== */
                    convs->Convert_888_to_565_asis
                        (width,lines,
                         srcbits,-bmpImage->bytes_per_line,
                         dstbits,linebytes);
                }
            } else {
                if (gDst==0x03e0) {
                    /* ==== rgb 888 bmp -> bgr 555 dib ==== */
                    /* ==== bgr 888 bmp -> rgb 555 dib ==== */
                    convs->Convert_888_to_555_reverse
                        (width,lines,
                         srcbits,-bmpImage->bytes_per_line,
                         dstbits,linebytes);
                } else {
                    /* ==== rgb 888 bmp -> bgr 565 dib ==== */
                    /* ==== bgr 888 bmp -> rgb 565 dib ==== */
                    convs->Convert_888_to_565_reverse
                        (width,lines,
                         srcbits,-bmpImage->bytes_per_line,
                         dstbits,linebytes);
                }
            }
            break;
        }
        /* Fall through */

    case 32:
        {
            const char* srcbits;

            srcbits=bmpImage->data+(lines-1)*bmpImage->bytes_per_line;

            if (bmpImage->green_mask!=0x00ff00 ||
                (bmpImage->red_mask|bmpImage->blue_mask)!=0xff00ff) {
                goto notsupported;
            } else if ((rDst==0x1f && bmpImage->red_mask==0xff) ||
                       (bDst==0x1f && bmpImage->blue_mask==0xff)) {
                if (gDst==0x03e0) {
                    /* ==== rgb 0888 bmp -> rgb 555 dib ==== */
                    /* ==== bgr 0888 bmp -> bgr 555 dib ==== */
                    convs->Convert_0888_to_555_asis
                        (width,lines,
                         srcbits,-bmpImage->bytes_per_line,
                         dstbits,linebytes);
                } else {
                    /* ==== rgb 0888 bmp -> rgb 565 dib ==== */
                    /* ==== bgr 0888 bmp -> bgr 565 dib ==== */
                    convs->Convert_0888_to_565_asis
                        (width,lines,
                         srcbits,-bmpImage->bytes_per_line,
                         dstbits,linebytes);
                }
            } else {
                if (gDst==0x03e0) {
                    /* ==== rgb 0888 bmp -> bgr 555 dib ==== */
                    /* ==== bgr 0888 bmp -> rgb 555 dib ==== */
                    convs->Convert_0888_to_555_reverse
                        (width,lines,
                         srcbits,-bmpImage->bytes_per_line,
                         dstbits,linebytes);
                } else {
                    /* ==== rgb 0888 bmp -> bgr 565 dib ==== */
                    /* ==== bgr 0888 bmp -> rgb 565 dib ==== */
                    convs->Convert_0888_to_565_reverse
                        (width,lines,
                         srcbits,-bmpImage->bytes_per_line,
                         dstbits,linebytes);
                }
            }
        }
        break;

    case 1:
    case 4:
        if (X11DRV_DIB_CheckMask(bmpImage->red_mask,bmpImage->green_mask,bmpImage->blue_mask)
            && srccolors) {
            /* ==== pal 1 or 4 bmp -> rgb or bgr 555 or 565 dib ==== */
            int rShift,gShift,bShift;
            WORD* dstpixel;

            /* Shift everything 16 bits left so that all shifts are >0,
             * even for BGR DIBs. Then a single >> 16 will bring everything
             * back into place.
             */
            rShift=16+X11DRV_DIB_MaskToShift(rDst)-3;
            gShift=16+X11DRV_DIB_MaskToShift(gDst)-3;
            bShift=16+X11DRV_DIB_MaskToShift(bDst)-3;
            if (gDst==0x07e0) {
                /* 6 bits for the green */
                gShift++;
            }
            rDst=rDst << 16;
            gDst=gDst << 16;
            bDst=bDst << 16;
            for (h = lines - 1; h >= 0; h--) {
                dstpixel=(LPWORD)dstbits;
                for (x = 0; x < width; x++) {
                    PALETTEENTRY srcval;
                    DWORD dstval;
                    srcval=srccolors[XGetPixel(bmpImage, x, h)];
                    dstval=((srcval.peRed   << rShift) & rDst) |
                           ((srcval.peGreen << gShift) & gDst) |
                           ((srcval.peBlue  << bShift) & bDst);
                    *dstpixel++=dstval >> 16;
                }
                dstbits += linebytes;
            }
        } else {
            goto notsupported;
        }
        break;

    case 8:
        if (X11DRV_DIB_CheckMask(bmpImage->red_mask,bmpImage->green_mask,bmpImage->blue_mask)
            && srccolors) {
            /* ==== pal 8 bmp -> rgb or bgr 555 or 565 dib ==== */
            int rShift,gShift,bShift;
            const BYTE* srcbits;
            const BYTE* srcpixel;
            WORD* dstpixel;

            /* Shift everything 16 bits left so that all shifts are >0,
             * even for BGR DIBs. Then a single >> 16 will bring everything
             * back into place.
             */
            rShift=16+X11DRV_DIB_MaskToShift(rDst)-3;
            gShift=16+X11DRV_DIB_MaskToShift(gDst)-3;
            bShift=16+X11DRV_DIB_MaskToShift(bDst)-3;
            if (gDst==0x07e0) {
                /* 6 bits for the green */
                gShift++;
            }
            rDst=rDst << 16;
            gDst=gDst << 16;
            bDst=bDst << 16;
            srcbits=(BYTE*)bmpImage->data+(lines-1)*bmpImage->bytes_per_line;
            for (h=0; h<lines; h++) {
                srcpixel=srcbits;
                dstpixel=(LPWORD)dstbits;
                for (x = 0; x < width; x++) {
                    PALETTEENTRY srcval;
                    DWORD dstval;
                    srcval=srccolors[*srcpixel++];
                    dstval=((srcval.peRed   << rShift) & rDst) |
                           ((srcval.peGreen << gShift) & gDst) |
                           ((srcval.peBlue  << bShift) & bDst);
                    *dstpixel++=dstval >> 16;
                }
                srcbits -= bmpImage->bytes_per_line;
                dstbits += linebytes;
            }
        } else {
            goto notsupported;
        }
        break;

    default:
    notsupported:
        {
            /* ==== any bmp format -> rgb or bgr 555 or 565 dib ==== */
            int rShift,gShift,bShift;
            WORD* dstpixel;

            WARN("from unknown %d bit bitmap (%lx,%lx,%lx) to 16 bit DIB (%x,%x,%x)\n",
                  bmpImage->depth, bmpImage->red_mask,
                  bmpImage->green_mask, bmpImage->blue_mask,
                  rDst, gDst, bDst);

            /* Shift everything 16 bits left so that all shifts are >0,
             * even for BGR DIBs. Then a single >> 16 will bring everything
             * back into place.
             */
            rShift=16+X11DRV_DIB_MaskToShift(rDst)-3;
            gShift=16+X11DRV_DIB_MaskToShift(gDst)-3;
            bShift=16+X11DRV_DIB_MaskToShift(bDst)-3;
            if (gDst==0x07e0) {
                /* 6 bits for the green */
                gShift++;
            }
            rDst=rDst << 16;
            gDst=gDst << 16;
            bDst=bDst << 16;
            for (h = lines - 1; h >= 0; h--) {
                dstpixel=(LPWORD)dstbits;
                for (x = 0; x < width; x++) {
                    COLORREF srcval;
                    DWORD dstval;
                    srcval=X11DRV_PALETTE_ToLogical(physDev, XGetPixel(bmpImage, x, h));
                    dstval=((GetRValue(srcval) << rShift) & rDst) |
                           ((GetGValue(srcval) << gShift) & gDst) |
                           ((GetBValue(srcval) << bShift) & bDst);
                    *dstpixel++=dstval >> 16;
                }
                dstbits += linebytes;
            }
        }
        break;
    }
}


/***********************************************************************
 *           X11DRV_DIB_SetImageBits_24
 *
 * SetDIBits for a 24-bit deep DIB.
 */
static void X11DRV_DIB_SetImageBits_24( int lines, const BYTE *srcbits,
                                 DWORD srcwidth, DWORD dstwidth, int left,
                                 X11DRV_PDEVICE *physDev,
                                 DWORD rSrc, DWORD gSrc, DWORD bSrc,
                                 XImage *bmpImage, DWORD linebytes )
{
    DWORD x;
    int h, width = min(srcwidth, dstwidth);
    const dib_conversions *convs = (bmpImage->byte_order == LSBFirst) ? &dib_normal : &dib_dst_byteswap;

    if (lines < 0 )
    {
        lines = -lines;
        srcbits = srcbits + linebytes * (lines - 1);
        linebytes = -linebytes;
    }

    switch (bmpImage->depth)
    {
    case 24:
        if (bmpImage->bits_per_pixel==24) {
            char* dstbits;

            srcbits=srcbits+left*3;
            dstbits=bmpImage->data+left*3+(lines-1)*bmpImage->bytes_per_line;

            if (bmpImage->green_mask!=0x00ff00 ||
                (bmpImage->red_mask|bmpImage->blue_mask)!=0xff00ff) {
                goto notsupported;
            } else if (rSrc==bmpImage->red_mask) {
                /* ==== rgb 888 dib -> rgb 888 bmp ==== */
                /* ==== bgr 888 dib -> bgr 888 bmp ==== */
                convs->Convert_888_asis
                    (width,lines,
                     srcbits,linebytes,
                     dstbits,-bmpImage->bytes_per_line);
            } else {
                /* ==== rgb 888 dib -> bgr 888 bmp ==== */
                /* ==== bgr 888 dib -> rgb 888 bmp ==== */
                convs->Convert_888_reverse
                    (width,lines,
                     srcbits,linebytes,
                     dstbits,-bmpImage->bytes_per_line);
            }
            break;
        }
        /* fall through */

    case 32:
        {
            char* dstbits;

            srcbits=srcbits+left*3;
            dstbits=bmpImage->data+left*4+(lines-1)*bmpImage->bytes_per_line;

            if (bmpImage->green_mask!=0x00ff00 ||
                (bmpImage->red_mask|bmpImage->blue_mask)!=0xff00ff) {
                goto notsupported;
            } else if (rSrc==bmpImage->red_mask) {
                /* ==== rgb 888 dib -> rgb 0888 bmp ==== */
                /* ==== bgr 888 dib -> bgr 0888 bmp ==== */
                convs->Convert_888_to_0888_asis
                    (width,lines,
                     srcbits,linebytes,
                     dstbits,-bmpImage->bytes_per_line);
            } else {
                /* ==== rgb 888 dib -> bgr 0888 bmp ==== */
                /* ==== bgr 888 dib -> rgb 0888 bmp ==== */
                convs->Convert_888_to_0888_reverse
                    (width,lines,
                     srcbits,linebytes,
                     dstbits,-bmpImage->bytes_per_line);
            }
            break;
        }

    case 15:
    case 16:
        {
            char* dstbits;

            srcbits=srcbits+left*3;
            dstbits=bmpImage->data+left*2+(lines-1)*bmpImage->bytes_per_line;

            if (bmpImage->green_mask==0x03e0) {
                if ((rSrc==0xff0000 && bmpImage->red_mask==0x7f00) ||
                    (bSrc==0xff0000 && bmpImage->blue_mask==0x7f00)) {
                    /* ==== rgb 888 dib -> rgb 555 bmp ==== */
                    /* ==== bgr 888 dib -> bgr 555 bmp ==== */
                    convs->Convert_888_to_555_asis
                        (width,lines,
                         srcbits,linebytes,
                         dstbits,-bmpImage->bytes_per_line);
                } else if ((rSrc==0xff && bmpImage->red_mask==0x7f00) ||
                           (bSrc==0xff && bmpImage->blue_mask==0x7f00)) {
                    /* ==== rgb 888 dib -> bgr 555 bmp ==== */
                    /* ==== bgr 888 dib -> rgb 555 bmp ==== */
                    convs->Convert_888_to_555_reverse
                        (width,lines,
                         srcbits,linebytes,
                         dstbits,-bmpImage->bytes_per_line);
                } else {
                    goto notsupported;
                }
            } else if (bmpImage->green_mask==0x07e0) {
                if ((rSrc==0xff0000 && bmpImage->red_mask==0xf800) ||
                    (bSrc==0xff0000 && bmpImage->blue_mask==0xf800)) {
                    /* ==== rgb 888 dib -> rgb 565 bmp ==== */
                    /* ==== bgr 888 dib -> bgr 565 bmp ==== */
                    convs->Convert_888_to_565_asis
                        (width,lines,
                         srcbits,linebytes,
                         dstbits,-bmpImage->bytes_per_line);
                } else if ((rSrc==0xff && bmpImage->red_mask==0xf800) ||
                           (bSrc==0xff && bmpImage->blue_mask==0xf800)) {
                    /* ==== rgb 888 dib -> bgr 565 bmp ==== */
                    /* ==== bgr 888 dib -> rgb 565 bmp ==== */
                    convs->Convert_888_to_565_reverse
                        (width,lines,
                         srcbits,linebytes,
                         dstbits,-bmpImage->bytes_per_line);
                } else {
                    goto notsupported;
                }
            } else {
                goto notsupported;
            }
        }
        break;

    default:
    notsupported:
        WARN("from 24 bit DIB (%x,%x,%x) to unknown %d bit bitmap (%lx,%lx,%lx)\n",
              rSrc, gSrc, bSrc, bmpImage->bits_per_pixel, bmpImage->red_mask,
              bmpImage->green_mask, bmpImage->blue_mask );
        /* fall through */
    case 1:
    case 4:
    case 8:
        {
            /* ==== rgb 888 dib -> any bmp format ==== */
            const BYTE* srcbyte;

            /* Windows only supports one 24bpp DIB format: RGB888 */
            srcbits+=left*3;
            for (h = lines - 1; h >= 0; h--) {
                srcbyte = srcbits;
                for (x = left; x < width+left; x++) {
                    XPutPixel(bmpImage, x, h,
                              X11DRV_PALETTE_ToPhysical
                              (physDev, RGB(srcbyte[2], srcbyte[1], srcbyte[0])));
                    srcbyte+=3;
                }
                srcbits += linebytes;
            }
        }
        break;
    }
}


/***********************************************************************
 *           X11DRV_DIB_GetImageBits_24
 *
 * GetDIBits for an 24-bit deep DIB.
 */
static void X11DRV_DIB_GetImageBits_24( X11DRV_PDEVICE *physDev, int lines, BYTE *dstbits,
					DWORD dstwidth, DWORD srcwidth,
					PALETTEENTRY *srccolors,
                                        DWORD rDst, DWORD gDst, DWORD bDst,
					XImage *bmpImage, DWORD linebytes )
{
    DWORD x;
    int h, width = min(srcwidth, dstwidth);
    const dib_conversions *convs = (bmpImage->byte_order == LSBFirst) ? &dib_normal : &dib_src_byteswap;

    if (lines < 0 )
    {
        lines = -lines;
        dstbits = dstbits + ( linebytes * (lines-1) );
        linebytes = -linebytes;
    }

    switch (bmpImage->depth)
    {
    case 24:
        if (bmpImage->bits_per_pixel==24) {
            const char* srcbits;

            srcbits=bmpImage->data+(lines-1)*bmpImage->bytes_per_line;

            if (bmpImage->green_mask!=0x00ff00 ||
                (bmpImage->red_mask|bmpImage->blue_mask)!=0xff00ff) {
                goto notsupported;
            } else if (rDst==bmpImage->red_mask) {
                /* ==== rgb 888 bmp -> rgb 888 dib ==== */
                /* ==== bgr 888 bmp -> bgr 888 dib ==== */
                convs->Convert_888_asis
                    (width,lines,
                     srcbits,-bmpImage->bytes_per_line,
                     dstbits,linebytes);
            } else {
                /* ==== rgb 888 bmp -> bgr 888 dib ==== */
                /* ==== bgr 888 bmp -> rgb 888 dib ==== */
                convs->Convert_888_reverse
                    (width,lines,
                     srcbits,-bmpImage->bytes_per_line,
                     dstbits,linebytes);
            }
            break;
        }
        /* fall through */

    case 32:
        {
            const char* srcbits;

            srcbits=bmpImage->data+(lines-1)*bmpImage->bytes_per_line;

            if (bmpImage->green_mask!=0x00ff00 ||
                (bmpImage->red_mask|bmpImage->blue_mask)!=0xff00ff) {
                goto notsupported;
            } else if (rDst==bmpImage->red_mask) {
                /* ==== rgb 888 bmp -> rgb 0888 dib ==== */
                /* ==== bgr 888 bmp -> bgr 0888 dib ==== */
                convs->Convert_0888_to_888_asis
                    (width,lines,
                     srcbits,-bmpImage->bytes_per_line,
                     dstbits,linebytes);
            } else {
                /* ==== rgb 888 bmp -> bgr 0888 dib ==== */
                /* ==== bgr 888 bmp -> rgb 0888 dib ==== */
                convs->Convert_0888_to_888_reverse
                    (width,lines,
                     srcbits,-bmpImage->bytes_per_line,
                     dstbits,linebytes);
            }
            break;
        }

    case 15:
    case 16:
        {
            const char* srcbits;

            srcbits=bmpImage->data+(lines-1)*bmpImage->bytes_per_line;

            if (bmpImage->green_mask==0x03e0) {
                if ((rDst==0xff0000 && bmpImage->red_mask==0x7f00) ||
                    (bDst==0xff0000 && bmpImage->blue_mask==0x7f00)) {
                    /* ==== rgb 555 bmp -> rgb 888 dib ==== */
                    /* ==== bgr 555 bmp -> bgr 888 dib ==== */
                    convs->Convert_555_to_888_asis
                        (width,lines,
                         srcbits,-bmpImage->bytes_per_line,
                         dstbits,linebytes);
                } else if ((rDst==0xff && bmpImage->red_mask==0x7f00) ||
                           (bDst==0xff && bmpImage->blue_mask==0x7f00)) {
                    /* ==== rgb 555 bmp -> bgr 888 dib ==== */
                    /* ==== bgr 555 bmp -> rgb 888 dib ==== */
                    convs->Convert_555_to_888_reverse
                        (width,lines,
                         srcbits,-bmpImage->bytes_per_line,
                         dstbits,linebytes);
                } else {
                    goto notsupported;
                }
            } else if (bmpImage->green_mask==0x07e0) {
                if ((rDst==0xff0000 && bmpImage->red_mask==0xf800) ||
                    (bDst==0xff0000 && bmpImage->blue_mask==0xf800)) {
                    /* ==== rgb 565 bmp -> rgb 888 dib ==== */
                    /* ==== bgr 565 bmp -> bgr 888 dib ==== */
                    convs->Convert_565_to_888_asis
                        (width,lines,
                         srcbits,-bmpImage->bytes_per_line,
                         dstbits,linebytes);
                } else if ((rDst==0xff && bmpImage->red_mask==0xf800) ||
                           (bDst==0xff && bmpImage->blue_mask==0xf800)) {
                    /* ==== rgb 565 bmp -> bgr 888 dib ==== */
                    /* ==== bgr 565 bmp -> rgb 888 dib ==== */
                    convs->Convert_565_to_888_reverse
                        (width,lines,
                         srcbits,-bmpImage->bytes_per_line,
                         dstbits,linebytes);
                } else {
                    goto notsupported;
                }
            } else {
                goto notsupported;
            }
        }
        break;

    case 1:
    case 4:
        if (X11DRV_DIB_CheckMask(bmpImage->red_mask,bmpImage->green_mask,bmpImage->blue_mask)
            && srccolors) {
            /* ==== pal 1 or 4 bmp -> rgb 888 dib ==== */
            BYTE* dstbyte;

            /* Windows only supports one 24bpp DIB format: rgb 888 */
            for (h = lines - 1; h >= 0; h--) {
                dstbyte=dstbits;
                for (x = 0; x < width; x++) {
                    PALETTEENTRY srcval;
                    srcval=srccolors[XGetPixel(bmpImage, x, h)];
                    dstbyte[0]=srcval.peBlue;
                    dstbyte[1]=srcval.peGreen;
                    dstbyte[2]=srcval.peRed;
                    dstbyte+=3;
                }
                dstbits += linebytes;
            }
        } else {
            goto notsupported;
        }
        break;

    case 8:
        if (X11DRV_DIB_CheckMask(bmpImage->red_mask,bmpImage->green_mask,bmpImage->blue_mask)
            && srccolors) {
            /* ==== pal 8 bmp -> rgb 888 dib ==== */
            const void* srcbits;
            const BYTE* srcpixel;
            BYTE* dstbyte;

            /* Windows only supports one 24bpp DIB format: rgb 888 */
            srcbits=bmpImage->data+(lines-1)*bmpImage->bytes_per_line;
            for (h = lines - 1; h >= 0; h--) {
                srcpixel=srcbits;
                dstbyte=dstbits;
                for (x = 0; x < width; x++ ) {
                    PALETTEENTRY srcval;
                    srcval=srccolors[*srcpixel++];
                    dstbyte[0]=srcval.peBlue;
                    dstbyte[1]=srcval.peGreen;
                    dstbyte[2]=srcval.peRed;
                    dstbyte+=3;
                }
                srcbits = (const char*)srcbits - bmpImage->bytes_per_line;
                dstbits += linebytes;
            }
        } else {
            goto notsupported;
        }
        break;

    default:
    notsupported:
        {
            /* ==== any bmp format -> 888 dib ==== */
            BYTE* dstbyte;

            WARN("from unknown %d bit bitmap (%lx,%lx,%lx) to 24 bit DIB (%x,%x,%x)\n",
                  bmpImage->depth, bmpImage->red_mask,
                  bmpImage->green_mask, bmpImage->blue_mask,
                  rDst, gDst, bDst );

            /* Windows only supports one 24bpp DIB format: rgb 888 */
            for (h = lines - 1; h >= 0; h--) {
                dstbyte=dstbits;
                for (x = 0; x < width; x++) {
                    COLORREF srcval=X11DRV_PALETTE_ToLogical
                        (physDev, XGetPixel( bmpImage, x, h ));
                    dstbyte[0]=GetBValue(srcval);
                    dstbyte[1]=GetGValue(srcval);
                    dstbyte[2]=GetRValue(srcval);
                    dstbyte+=3;
                }
                dstbits += linebytes;
            }
        }
        break;
    }
}


/***********************************************************************
 *           X11DRV_DIB_SetImageBits_32
 *
 * SetDIBits for a 32-bit deep DIB.
 */
static void X11DRV_DIB_SetImageBits_32(int lines, const BYTE *srcbits,
                                       DWORD srcwidth, DWORD dstwidth, int left,
                                       X11DRV_PDEVICE *physDev,
                                       DWORD rSrc, DWORD gSrc, DWORD bSrc,
                                       XImage *bmpImage,
                                       DWORD linebytes)
{
    DWORD x;
    int h, width = min(srcwidth, dstwidth);
    const dib_conversions *convs = (bmpImage->byte_order == LSBFirst) ? &dib_normal : &dib_dst_byteswap;

    if (lines < 0 )
    {
       lines = -lines;
       srcbits = srcbits + ( linebytes * (lines-1) );
       linebytes = -linebytes;
    }

    switch (bmpImage->depth)
    {
    case 24:
        if (bmpImage->bits_per_pixel==24) {
            char* dstbits;

            srcbits=srcbits+left*4;
            dstbits=bmpImage->data+left*3+(lines-1)*bmpImage->bytes_per_line;

            if (rSrc==bmpImage->red_mask && gSrc==bmpImage->green_mask && bSrc==bmpImage->blue_mask) {
                /* ==== rgb 0888 dib -> rgb 888 bmp ==== */
                /* ==== bgr 0888 dib -> bgr 888 bmp ==== */
                convs->Convert_0888_to_888_asis
                    (width,lines,
                     srcbits,linebytes,
                     dstbits,-bmpImage->bytes_per_line);
            } else if (bmpImage->green_mask!=0x00ff00 ||
                       (bmpImage->red_mask|bmpImage->blue_mask)!=0xff00ff) {
                goto notsupported;
                /* the tests below assume sane bmpImage masks */
            } else if (rSrc==bmpImage->blue_mask && gSrc==bmpImage->green_mask && bSrc==bmpImage->red_mask) {
                /* ==== rgb 0888 dib -> bgr 888 bmp ==== */
                /* ==== bgr 0888 dib -> rgb 888 bmp ==== */
                convs->Convert_0888_to_888_reverse
                    (width,lines,
                     srcbits,linebytes,
                     dstbits,-bmpImage->bytes_per_line);
            } else if (bmpImage->blue_mask==0xff) {
                /* ==== any 0888 dib -> rgb 888 bmp ==== */
                convs->Convert_any0888_to_rgb888
                    (width,lines,
                     srcbits,linebytes,
                     rSrc,gSrc,bSrc,
                     dstbits,-bmpImage->bytes_per_line);
            } else {
                /* ==== any 0888 dib -> bgr 888 bmp ==== */
                convs->Convert_any0888_to_bgr888
                    (width,lines,
                     srcbits,linebytes,
                     rSrc,gSrc,bSrc,
                     dstbits,-bmpImage->bytes_per_line);
            }
            break;
        }
        /* fall through */

    case 32:
        {
            char* dstbits;

            srcbits=srcbits+left*4;
            dstbits=bmpImage->data+left*4+(lines-1)*bmpImage->bytes_per_line;

            if (gSrc==bmpImage->green_mask) {
                if (rSrc==bmpImage->red_mask && bSrc==bmpImage->blue_mask) {
                    /* ==== rgb 0888 dib -> rgb 0888 bmp ==== */
                    /* ==== bgr 0888 dib -> bgr 0888 bmp ==== */
                    convs->Convert_0888_asis
                        (width,lines,
                         srcbits,linebytes,
                         dstbits,-bmpImage->bytes_per_line);
                } else if (bmpImage->green_mask!=0x00ff00 ||
                           (bmpImage->red_mask|bmpImage->blue_mask)!=0xff00ff) {
                    goto notsupported;
                    /* the tests below assume sane bmpImage masks */
                } else if (rSrc==bmpImage->blue_mask && bSrc==bmpImage->red_mask) {
                    /* ==== rgb 0888 dib -> bgr 0888 bmp ==== */
                    /* ==== bgr 0888 dib -> rgb 0888 bmp ==== */
                    convs->Convert_0888_reverse
                        (width,lines,
                         srcbits,linebytes,
                         dstbits,-bmpImage->bytes_per_line);
                } else {
                    /* ==== any 0888 dib -> any 0888 bmp ==== */
                    convs->Convert_0888_any
                        (width,lines,
                         srcbits,linebytes,
                         rSrc,gSrc,bSrc,
                         dstbits,-bmpImage->bytes_per_line,
                         bmpImage->red_mask,bmpImage->green_mask,bmpImage->blue_mask);
                }
            } else if (bmpImage->green_mask!=0x00ff00 ||
                       (bmpImage->red_mask|bmpImage->blue_mask)!=0xff00ff) {
                goto notsupported;
                /* the tests below assume sane bmpImage masks */
            } else {
                /* ==== any 0888 dib -> any 0888 bmp ==== */
                convs->Convert_0888_any
                    (width,lines,
                     srcbits,linebytes,
                     rSrc,gSrc,bSrc,
                     dstbits,-bmpImage->bytes_per_line,
                     bmpImage->red_mask,bmpImage->green_mask,bmpImage->blue_mask);
            }
        }
        break;

    case 15:
    case 16:
        {
            char* dstbits;

            srcbits=srcbits+left*4;
            dstbits=bmpImage->data+left*2+(lines-1)*bmpImage->bytes_per_line;

            if (rSrc==0xff0000 && gSrc==0x00ff00 && bSrc==0x0000ff) {
                if (bmpImage->green_mask==0x03e0) {
                    if (bmpImage->red_mask==0x7f00) {
                        /* ==== rgb 0888 dib -> rgb 555 bmp ==== */
                        convs->Convert_0888_to_555_asis
                            (width,lines,
                             srcbits,linebytes,
                             dstbits,-bmpImage->bytes_per_line);
                    } else if (bmpImage->blue_mask==0x7f00) {
                        /* ==== rgb 0888 dib -> bgr 555 bmp ==== */
                        convs->Convert_0888_to_555_reverse
                            (width,lines,
                             srcbits,linebytes,
                             dstbits,-bmpImage->bytes_per_line);
                    } else {
                        goto notsupported;
                    }
                } else if (bmpImage->green_mask==0x07e0) {
                    if (bmpImage->red_mask==0xf800) {
                        /* ==== rgb 0888 dib -> rgb 565 bmp ==== */
                        convs->Convert_0888_to_565_asis
                            (width,lines,
                             srcbits,linebytes,
                             dstbits,-bmpImage->bytes_per_line);
                    } else if (bmpImage->blue_mask==0xf800) {
                        /* ==== rgb 0888 dib -> bgr 565 bmp ==== */
                        convs->Convert_0888_to_565_reverse
                            (width,lines,
                             srcbits,linebytes,
                             dstbits,-bmpImage->bytes_per_line);
                    } else {
                        goto notsupported;
                    }
                } else {
                    goto notsupported;
                }
            } else if (rSrc==0x0000ff && gSrc==0x00ff00 && bSrc==0xff0000) {
                if (bmpImage->green_mask==0x03e0) {
                    if (bmpImage->blue_mask==0x7f00) {
                        /* ==== bgr 0888 dib -> bgr 555 bmp ==== */
                        convs->Convert_0888_to_555_asis
                            (width,lines,
                             srcbits,linebytes,
                             dstbits,-bmpImage->bytes_per_line);
                    } else if (bmpImage->red_mask==0x7f00) {
                        /* ==== bgr 0888 dib -> rgb 555 bmp ==== */
                        convs->Convert_0888_to_555_reverse
                            (width,lines,
                             srcbits,linebytes,
                             dstbits,-bmpImage->bytes_per_line);
                    } else {
                        goto notsupported;
                    }
                } else if (bmpImage->green_mask==0x07e0) {
                    if (bmpImage->blue_mask==0xf800) {
                        /* ==== bgr 0888 dib -> bgr 565 bmp ==== */
                        convs->Convert_0888_to_565_asis
                            (width,lines,
                             srcbits,linebytes,
                             dstbits,-bmpImage->bytes_per_line);
                    } else if (bmpImage->red_mask==0xf800) {
                        /* ==== bgr 0888 dib -> rgb 565 bmp ==== */
                        convs->Convert_0888_to_565_reverse
                            (width,lines,
                             srcbits,linebytes,
                             dstbits,-bmpImage->bytes_per_line);
                    } else {
                        goto notsupported;
                    }
                } else {
                    goto notsupported;
                }
            } else {
                if (bmpImage->green_mask==0x03e0 &&
                    (bmpImage->red_mask==0x7f00 ||
                     bmpImage->blue_mask==0x7f00)) {
                    /* ==== any 0888 dib -> rgb or bgr 555 bmp ==== */
                    convs->Convert_any0888_to_5x5
                        (width,lines,
                         srcbits,linebytes,
                         rSrc,gSrc,bSrc,
                         dstbits,-bmpImage->bytes_per_line,
                         bmpImage->red_mask,bmpImage->green_mask,bmpImage->blue_mask);
                } else if (bmpImage->green_mask==0x07e0 &&
                           (bmpImage->red_mask==0xf800 ||
                            bmpImage->blue_mask==0xf800)) {
                    /* ==== any 0888 dib -> rgb or bgr 565 bmp ==== */
                    convs->Convert_any0888_to_5x5
                        (width,lines,
                         srcbits,linebytes,
                         rSrc,gSrc,bSrc,
                         dstbits,-bmpImage->bytes_per_line,
                         bmpImage->red_mask,bmpImage->green_mask,bmpImage->blue_mask);
                } else {
                    goto notsupported;
                }
            }
        }
        break;

    default:
    notsupported:
        WARN("from 32 bit DIB (%x,%x,%x) to unknown %d bit bitmap (%lx,%lx,%lx)\n",
              rSrc, gSrc, bSrc, bmpImage->bits_per_pixel, bmpImage->red_mask,
              bmpImage->green_mask, bmpImage->blue_mask );
        /* fall through */
    case 1:
    case 4:
    case 8:
        {
            /* ==== any 0888 dib -> pal 1, 4 or 8 bmp ==== */
            const DWORD* srcpixel;
            int rShift,gShift,bShift;

            rShift=X11DRV_DIB_MaskToShift(rSrc);
            gShift=X11DRV_DIB_MaskToShift(gSrc);
            bShift=X11DRV_DIB_MaskToShift(bSrc);
            srcbits+=left*4;
            for (h = lines - 1; h >= 0; h--) {
                srcpixel=(const DWORD*)srcbits;
                for (x = left; x < width+left; x++) {
                    DWORD srcvalue;
                    BYTE red,green,blue;
                    srcvalue=*srcpixel++;
                    red=  (srcvalue >> rShift) & 0xff;
                    green=(srcvalue >> gShift) & 0xff;
                    blue= (srcvalue >> bShift) & 0xff;
                    XPutPixel(bmpImage, x, h, X11DRV_PALETTE_ToPhysical
                              (physDev, RGB(red,green,blue)));
                }
                srcbits += linebytes;
            }
        }
        break;
    }

}

/***********************************************************************
 *           X11DRV_DIB_GetImageBits_32
 *
 * GetDIBits for an 32-bit deep DIB.
 */
static void X11DRV_DIB_GetImageBits_32( X11DRV_PDEVICE *physDev, int lines, BYTE *dstbits,
					DWORD dstwidth, DWORD srcwidth,
					PALETTEENTRY *srccolors,
					DWORD rDst, DWORD gDst, DWORD bDst,
					XImage *bmpImage, int linebytes )
{
    DWORD x;
    int h, width = min(srcwidth, dstwidth);
    const dib_conversions *convs = (bmpImage->byte_order == LSBFirst) ? &dib_normal : &dib_src_byteswap;

    if (lines < 0 )
    {
        lines = -lines;
        dstbits = dstbits + ( linebytes * (lines-1) );
        linebytes = -linebytes;
    }

    switch (bmpImage->depth)
    {
    case 24:
        if (bmpImage->bits_per_pixel==24) {
            const void* srcbits;

            srcbits=bmpImage->data+(lines-1)*bmpImage->bytes_per_line;

            if (rDst==bmpImage->red_mask && gDst==bmpImage->green_mask && bDst==bmpImage->blue_mask) {
                /* ==== rgb 888 bmp -> rgb 0888 dib ==== */
                /* ==== bgr 888 bmp -> bgr 0888 dib ==== */
                convs->Convert_888_to_0888_asis
                    (width,lines,
                     srcbits,-bmpImage->bytes_per_line,
                     dstbits,linebytes);
            } else if (bmpImage->green_mask!=0x00ff00 ||
                       (bmpImage->red_mask|bmpImage->blue_mask)!=0xff00ff) {
                goto notsupported;
                /* the tests below assume sane bmpImage masks */
            } else if (rDst==bmpImage->blue_mask && gDst==bmpImage->green_mask && bDst==bmpImage->red_mask) {
                /* ==== rgb 888 bmp -> bgr 0888 dib ==== */
                /* ==== bgr 888 bmp -> rgb 0888 dib ==== */
                convs->Convert_888_to_0888_reverse
                    (width,lines,
                     srcbits,-bmpImage->bytes_per_line,
                     dstbits,linebytes);
            } else if (bmpImage->blue_mask==0xff) {
                /* ==== rgb 888 bmp -> any 0888 dib ==== */
                convs->Convert_rgb888_to_any0888
                    (width,lines,
                     srcbits,-bmpImage->bytes_per_line,
                     dstbits,linebytes,
                     rDst,gDst,bDst);
            } else {
                /* ==== bgr 888 bmp -> any 0888 dib ==== */
                convs->Convert_bgr888_to_any0888
                    (width,lines,
                     srcbits,-bmpImage->bytes_per_line,
                     dstbits,linebytes,
                     rDst,gDst,bDst);
            }
            break;
        }
        /* fall through */

    case 32:
        {
            const char* srcbits;

            srcbits=bmpImage->data+(lines-1)*bmpImage->bytes_per_line;

            if (gDst==bmpImage->green_mask) {
                if (rDst==bmpImage->red_mask && bDst==bmpImage->blue_mask) {
                    /* ==== rgb 0888 bmp -> rgb 0888 dib ==== */
                    /* ==== bgr 0888 bmp -> bgr 0888 dib ==== */
                    convs->Convert_0888_asis
                        (width,lines,
                         srcbits,-bmpImage->bytes_per_line,
                         dstbits,linebytes);
                } else if (bmpImage->green_mask!=0x00ff00 ||
                           (bmpImage->red_mask|bmpImage->blue_mask)!=0xff00ff) {
                    goto notsupported;
                    /* the tests below assume sane bmpImage masks */
                } else if (rDst==bmpImage->blue_mask && bDst==bmpImage->red_mask) {
                    /* ==== rgb 0888 bmp -> bgr 0888 dib ==== */
                    /* ==== bgr 0888 bmp -> rgb 0888 dib ==== */
                    convs->Convert_0888_reverse
                        (width,lines,
                         srcbits,-bmpImage->bytes_per_line,
                         dstbits,linebytes);
                } else {
                    /* ==== any 0888 bmp -> any 0888 dib ==== */
                    convs->Convert_0888_any
                        (width,lines,
                         srcbits,-bmpImage->bytes_per_line,
                         bmpImage->red_mask,bmpImage->green_mask,bmpImage->blue_mask,
                         dstbits,linebytes,
                         rDst,gDst,bDst);
                }
            } else if (bmpImage->green_mask!=0x00ff00 ||
                       (bmpImage->red_mask|bmpImage->blue_mask)!=0xff00ff) {
                goto notsupported;
                /* the tests below assume sane bmpImage masks */
            } else {
                /* ==== any 0888 bmp -> any 0888 dib ==== */
                convs->Convert_0888_any
                    (width,lines,
                     srcbits,-bmpImage->bytes_per_line,
                     bmpImage->red_mask,bmpImage->green_mask,bmpImage->blue_mask,
                     dstbits,linebytes,
                     rDst,gDst,bDst);
            }
        }
        break;

    case 15:
    case 16:
        {
            const char* srcbits;

            srcbits=bmpImage->data+(lines-1)*bmpImage->bytes_per_line;

            if (rDst==0xff0000 && gDst==0x00ff00 && bDst==0x0000ff) {
                if (bmpImage->green_mask==0x03e0) {
                    if (bmpImage->red_mask==0x7f00) {
                        /* ==== rgb 555 bmp -> rgb 0888 dib ==== */
                        convs->Convert_555_to_0888_asis
                            (width,lines,
                             srcbits,-bmpImage->bytes_per_line,
                             dstbits,linebytes);
                    } else if (bmpImage->blue_mask==0x7f00) {
                        /* ==== bgr 555 bmp -> rgb 0888 dib ==== */
                        convs->Convert_555_to_0888_reverse
                            (width,lines,
                             srcbits,-bmpImage->bytes_per_line,
                             dstbits,linebytes);
                    } else {
                        goto notsupported;
                    }
                } else if (bmpImage->green_mask==0x07e0) {
                    if (bmpImage->red_mask==0xf800) {
                        /* ==== rgb 565 bmp -> rgb 0888 dib ==== */
                        convs->Convert_565_to_0888_asis
                            (width,lines,
                             srcbits,-bmpImage->bytes_per_line,
                             dstbits,linebytes);
                    } else if (bmpImage->blue_mask==0xf800) {
                        /* ==== bgr 565 bmp -> rgb 0888 dib ==== */
                        convs->Convert_565_to_0888_reverse
                            (width,lines,
                             srcbits,-bmpImage->bytes_per_line,
                             dstbits,linebytes);
                    } else {
                        goto notsupported;
                    }
                } else {
                    goto notsupported;
                }
            } else if (rDst==0x0000ff && gDst==0x00ff00 && bDst==0xff0000) {
                if (bmpImage->green_mask==0x03e0) {
                    if (bmpImage->blue_mask==0x7f00) {
                        /* ==== bgr 555 bmp -> bgr 0888 dib ==== */
                        convs->Convert_555_to_0888_asis
                            (width,lines,
                             srcbits,-bmpImage->bytes_per_line,
                             dstbits,linebytes);
                    } else if (bmpImage->red_mask==0x7f00) {
                        /* ==== rgb 555 bmp -> bgr 0888 dib ==== */
                        convs->Convert_555_to_0888_reverse
                            (width,lines,
                             srcbits,-bmpImage->bytes_per_line,
                             dstbits,linebytes);
                    } else {
                        goto notsupported;
                    }
                } else if (bmpImage->green_mask==0x07e0) {
                    if (bmpImage->blue_mask==0xf800) {
                        /* ==== bgr 565 bmp -> bgr 0888 dib ==== */
                        convs->Convert_565_to_0888_asis
                            (width,lines,
                             srcbits,-bmpImage->bytes_per_line,
                             dstbits,linebytes);
                    } else if (bmpImage->red_mask==0xf800) {
                        /* ==== rgb 565 bmp -> bgr 0888 dib ==== */
                        convs->Convert_565_to_0888_reverse
                            (width,lines,
                             srcbits,-bmpImage->bytes_per_line,
                             dstbits,linebytes);
                    } else {
                        goto notsupported;
                    }
                } else {
                    goto notsupported;
                }
            } else {
                if (bmpImage->green_mask==0x03e0 &&
                    (bmpImage->red_mask==0x7f00 ||
                     bmpImage->blue_mask==0x7f00)) {
                    /* ==== rgb or bgr 555 bmp -> any 0888 dib ==== */
                    convs->Convert_5x5_to_any0888
                        (width,lines,
                         srcbits,-bmpImage->bytes_per_line,
                         bmpImage->red_mask,bmpImage->green_mask,bmpImage->blue_mask,
                         dstbits,linebytes,
                         rDst,gDst,bDst);
                } else if (bmpImage->green_mask==0x07e0 &&
                           (bmpImage->red_mask==0xf800 ||
                            bmpImage->blue_mask==0xf800)) {
                    /* ==== rgb or bgr 565 bmp -> any 0888 dib ==== */
                    convs->Convert_5x5_to_any0888
                        (width,lines,
                         srcbits,-bmpImage->bytes_per_line,
                         bmpImage->red_mask,bmpImage->green_mask,bmpImage->blue_mask,
                         dstbits,linebytes,
                         rDst,gDst,bDst);
                } else {
                    goto notsupported;
                }
            }
        }
        break;

    case 1:
    case 4:
        if (X11DRV_DIB_CheckMask(bmpImage->red_mask,bmpImage->green_mask,bmpImage->blue_mask)
            && srccolors) {
            /* ==== pal 1 or 4 bmp -> any 0888 dib ==== */
            int rShift,gShift,bShift;
            DWORD* dstpixel;

            rShift=X11DRV_DIB_MaskToShift(rDst);
            gShift=X11DRV_DIB_MaskToShift(gDst);
            bShift=X11DRV_DIB_MaskToShift(bDst);
            for (h = lines - 1; h >= 0; h--) {
                dstpixel=(DWORD*)dstbits;
                for (x = 0; x < width; x++) {
                    PALETTEENTRY srcval;
                    srcval = srccolors[XGetPixel(bmpImage, x, h)];
                    *dstpixel++=(srcval.peRed   << rShift) |
                                (srcval.peGreen << gShift) |
                                (srcval.peBlue  << bShift);
                }
                dstbits += linebytes;
            }
        } else {
            goto notsupported;
        }
        break;

    case 8:
        if (X11DRV_DIB_CheckMask(bmpImage->red_mask,bmpImage->green_mask,bmpImage->blue_mask)
            && srccolors) {
            /* ==== pal 8 bmp -> any 0888 dib ==== */
            int rShift,gShift,bShift;
            const void* srcbits;
            const BYTE* srcpixel;
            DWORD* dstpixel;

            rShift=X11DRV_DIB_MaskToShift(rDst);
            gShift=X11DRV_DIB_MaskToShift(gDst);
            bShift=X11DRV_DIB_MaskToShift(bDst);
            srcbits=bmpImage->data+(lines-1)*bmpImage->bytes_per_line;
            for (h = lines - 1; h >= 0; h--) {
                srcpixel=srcbits;
                dstpixel=(DWORD*)dstbits;
                for (x = 0; x < width; x++) {
                    PALETTEENTRY srcval;
                    srcval=srccolors[*srcpixel++];
                    *dstpixel++=(srcval.peRed   << rShift) |
                                (srcval.peGreen << gShift) |
                                (srcval.peBlue  << bShift);
                }
                srcbits = (const char*)srcbits - bmpImage->bytes_per_line;
                dstbits += linebytes;
            }
        } else {
            goto notsupported;
        }
        break;

    default:
    notsupported:
        {
            /* ==== any bmp format -> any 0888 dib ==== */
            int rShift,gShift,bShift;
            DWORD* dstpixel;

            WARN("from unknown %d bit bitmap (%lx,%lx,%lx) to 32 bit DIB (%x,%x,%x)\n",
                  bmpImage->depth, bmpImage->red_mask,
                  bmpImage->green_mask, bmpImage->blue_mask,
                  rDst,gDst,bDst);

            rShift=X11DRV_DIB_MaskToShift(rDst);
            gShift=X11DRV_DIB_MaskToShift(gDst);
            bShift=X11DRV_DIB_MaskToShift(bDst);
            for (h = lines - 1; h >= 0; h--) {
                dstpixel=(DWORD*)dstbits;
                for (x = 0; x < width; x++) {
                    COLORREF srcval;
                    srcval=X11DRV_PALETTE_ToLogical(physDev, XGetPixel(bmpImage, x, h));
                    *dstpixel++=(GetRValue(srcval) << rShift) |
                                (GetGValue(srcval) << gShift) |
                                (GetBValue(srcval) << bShift);
                }
                dstbits += linebytes;
            }
        }
        break;
    }
}

static int XGetSubImageErrorHandler(Display *dpy, XErrorEvent *event, void *arg)
{
    return (event->request_code == X_GetImage && event->error_code == BadMatch);
}

/***********************************************************************
 *           X11DRV_DIB_SetImageBits_GetSubImage
 *
 *  Helper for X11DRV_DIB_SetImageBits
 */
static void X11DRV_DIB_SetImageBits_GetSubImage(
        const X11DRV_DIB_IMAGEBITS_DESCR *descr, XImage *bmpImage)
{
    /* compressed bitmaps may contain gaps in them. So make a copy
     * of the existing pixels first */
    RECT bmprc, rc;

    SetRect( &bmprc, descr->xDest, descr->yDest,
             descr->xDest + descr->width , descr->yDest + descr->height );
    GetRgnBox( descr->physDev->region, &rc );
    /* convert from dc to drawable origin */
    OffsetRect( &rc, descr->physDev->dc_rect.left, descr->physDev->dc_rect.top);
    /* clip visible rect with bitmap */
    if( IntersectRect( &rc, &rc, &bmprc))
    {
        X11DRV_expect_error( gdi_display, XGetSubImageErrorHandler, NULL );
        XGetSubImage( gdi_display, descr->drawable, rc.left, rc.top,
                      rc.right - rc.left, rc.bottom - rc.top, AllPlanes,
                      ZPixmap, bmpImage,
                      descr->xSrc + rc.left - bmprc.left,
                      descr->ySrc + rc.top - bmprc.top);
        X11DRV_check_error();
    }
}

/***********************************************************************
 *           X11DRV_DIB_SetImageBits
 *
 * Transfer the bits to an X image.
 * Helper function for SetDIBits() and SetDIBitsToDevice().
 */
static int X11DRV_DIB_SetImageBits( const X11DRV_DIB_IMAGEBITS_DESCR *descr )
{
    int lines = descr->lines >= 0 ? descr->lines : -descr->lines;
    void *old_data = NULL;
    XImage *bmpImage;

    wine_tsx11_lock();
    if (descr->image)
        bmpImage = descr->image;
    else {
        bmpImage = XCreateImage( gdi_display, visual, descr->depth, ZPixmap, 0, NULL,
				 descr->infoWidth, lines, 32, 0 );
	bmpImage->data = HeapAlloc( GetProcessHeap(), 0, lines * bmpImage->bytes_per_line );
        if(bmpImage->data == NULL) {
            ERR("Out of memory!\n");
            XDestroyImage( bmpImage );
            wine_tsx11_unlock();
            return 0;
        }
        if (descr->shifts)
        {
            bmpImage->red_mask = descr->shifts->physicalRed.max << descr->shifts->physicalRed.shift;
            bmpImage->green_mask = descr->shifts->physicalGreen.max << descr->shifts->physicalGreen.shift;
            bmpImage->blue_mask = descr->shifts->physicalBlue.max << descr->shifts->physicalBlue.shift;
        }
    }
    wine_tsx11_unlock();

    TRACE("Dib: depth=%d r=%x g=%x b=%x\n",
          descr->infoBpp,descr->rMask,descr->gMask,descr->bMask);
    TRACE("Bmp: depth=%d/%d r=%lx g=%lx b=%lx\n",
          bmpImage->depth,bmpImage->bits_per_pixel,
          bmpImage->red_mask,bmpImage->green_mask,bmpImage->blue_mask);

#ifdef HAVE_LIBXXSHM
    if (descr->shm_mode == X11DRV_SHM_PIXMAP
            && descr->xSrc == 0 && descr->ySrc == 0
            && descr->xDest == 0 && descr->yDest == 0)
    {
        TRACE("Using the shared pixmap data.\n");

        wine_tsx11_lock();
        XSync( gdi_display, False );
        wine_tsx11_unlock();

        old_data = descr->image->data;
        descr->image->data = descr->physBitmap->shminfo.shmaddr;
    }
#endif

      /* Transfer the pixels */
    __TRY
    {
        switch(descr->infoBpp)
        {
        case 1:
            X11DRV_DIB_SetImageBits_1( descr->lines, descr->bits, descr->infoWidth,
                                       descr->width, descr->xSrc, (int *)(descr->colorMap),
                                       bmpImage, descr->dibpitch );
            break;
        case 4:
            if (descr->compression) {
                X11DRV_DIB_SetImageBits_GetSubImage( descr, bmpImage);
                X11DRV_DIB_SetImageBits_RLE4( descr->lines, descr->bits,
                                              descr->infoWidth, descr->width,
                                              descr->xSrc, (int *)(descr->colorMap),
                                              bmpImage );
            } else
                X11DRV_DIB_SetImageBits_4( descr->lines, descr->bits,
                                           descr->infoWidth, descr->width,
                                           descr->xSrc, (int*)(descr->colorMap),
                                           bmpImage, descr->dibpitch );
            break;
        case 8:
            if (descr->compression) {
                X11DRV_DIB_SetImageBits_GetSubImage( descr, bmpImage);
                X11DRV_DIB_SetImageBits_RLE8( descr->lines, descr->bits,
                                              descr->infoWidth, descr->width,
                                              descr->xSrc, (int *)(descr->colorMap),
                                              bmpImage );
            } else
                X11DRV_DIB_SetImageBits_8( descr->lines, descr->bits,
                                           descr->infoWidth, descr->width,
                                           descr->xSrc, (int *)(descr->colorMap),
                                           bmpImage, descr->dibpitch );
            break;
        case 15:
        case 16:
            X11DRV_DIB_SetImageBits_16( descr->lines, descr->bits,
                                        descr->infoWidth, descr->width,
                                        descr->xSrc, descr->physDev,
                                        descr->rMask, descr->gMask, descr->bMask,
                                        bmpImage, descr->dibpitch);
            break;
        case 24:
            X11DRV_DIB_SetImageBits_24( descr->lines, descr->bits,
                                        descr->infoWidth, descr->width,
                                        descr->xSrc, descr->physDev,
                                        descr->rMask, descr->gMask, descr->bMask,
                                        bmpImage, descr->dibpitch);
            break;
        case 32:
            X11DRV_DIB_SetImageBits_32( descr->lines, descr->bits,
                                        descr->infoWidth, descr->width,
                                        descr->xSrc, descr->physDev,
                                        descr->rMask, descr->gMask, descr->bMask,
                                        bmpImage, descr->dibpitch);
            break;
        default:
            WARN("(%d): Invalid depth\n", descr->infoBpp );
            break;
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        WARN( "invalid bits pointer %p\n", descr->bits );
        lines = 0;
    }
    __ENDTRY

    TRACE("XPutImage(%ld,%p,%p,%d,%d,%d,%d,%d,%d)\n",
     descr->drawable, descr->gc, bmpImage,
     descr->xSrc, descr->ySrc, descr->xDest, descr->yDest,
     descr->width, descr->height);

    wine_tsx11_lock();
    if (lines)
    {
#ifdef HAVE_LIBXXSHM
        if (descr->shm_mode == X11DRV_SHM_PIXMAP
                && descr->xSrc == 0 && descr->ySrc == 0
                && descr->xDest == 0 && descr->yDest == 0)
        {
            XSync( gdi_display, False );
        }
        else if (descr->shm_mode == X11DRV_SHM_IMAGE && descr->image)
        {
            XShmPutImage( gdi_display, descr->drawable, descr->gc, bmpImage,
                          descr->xSrc, descr->ySrc, descr->xDest, descr->yDest,
                          descr->width, descr->height, FALSE );
            XSync( gdi_display, 0 );
        }
        else
#endif
        {
            XPutImage( gdi_display, descr->drawable, descr->gc, bmpImage,
                       descr->xSrc, descr->ySrc, descr->xDest, descr->yDest,
                       descr->width, descr->height );
        }
    }

    if (old_data) descr->image->data = old_data;

    if (!descr->image) X11DRV_DIB_DestroyXImage( bmpImage );
    wine_tsx11_unlock();
    return lines;
}

/***********************************************************************
 *           X11DRV_DIB_GetImageBits
 *
 * Transfer the bits from an X image.
 */
static int X11DRV_DIB_GetImageBits( const X11DRV_DIB_IMAGEBITS_DESCR *descr )
{
    int lines = descr->lines >= 0 ? descr->lines : -descr->lines;
    void *old_data = NULL;
    XImage *bmpImage;

    wine_tsx11_lock();
   if (descr->image)
        bmpImage = descr->image;
    else {
        bmpImage = XCreateImage( gdi_display, visual, descr->depth, ZPixmap, 0, NULL,
				 descr->infoWidth, lines, 32, 0 );
	bmpImage->data = HeapAlloc( GetProcessHeap(), 0, lines * bmpImage->bytes_per_line );
        if(bmpImage->data == NULL) {
            ERR("Out of memory!\n");
            XDestroyImage( bmpImage );
            wine_tsx11_unlock();
            return 0;
        }
        if (descr->shifts)
        {
            bmpImage->red_mask = descr->shifts->physicalRed.max << descr->shifts->physicalRed.shift;
            bmpImage->green_mask = descr->shifts->physicalGreen.max << descr->shifts->physicalGreen.shift;
            bmpImage->blue_mask = descr->shifts->physicalBlue.max << descr->shifts->physicalBlue.shift;
        }
    }

#ifdef HAVE_LIBXXSHM

    /* We must not call XShmGetImage() with a bitmap which is bigger than the available area.
       If we do, XShmGetImage() will fail (X exception), as it checks for this internally. */
    if (descr->shm_mode == X11DRV_SHM_PIXMAP && descr->image
            && descr->xSrc == 0 && descr->ySrc == 0
            && descr->xDest == 0 && descr->yDest == 0
            && bmpImage->width <= (descr->width - descr->xSrc)
            && bmpImage->height <= (descr->height - descr->ySrc))
    {
        XSync( gdi_display, False );
        old_data = bmpImage->data;
        bmpImage->data = descr->physBitmap->shminfo.shmaddr;
        TRACE("Using shared pixmap data.\n");
    }
    else if (descr->shm_mode == X11DRV_SHM_IMAGE && descr->image
            && bmpImage->width <= (descr->width - descr->xSrc)
            && bmpImage->height <= (descr->height - descr->ySrc))
    {
        int saveRed, saveGreen, saveBlue;

        TRACE("XShmGetImage(%p, %ld, %p, %d, %d, %ld)\n",
                            gdi_display, descr->drawable, bmpImage,
                            descr->xSrc, descr->ySrc, AllPlanes);

 	/* We must save and restore the bmpImage's masks in order
 	 * to preserve them across the call to XShmGetImage, which
	 * decides to eliminate them since it doesn't happen to know
 	 * what the format of the image is supposed to be, even though
 	 * we do. */
        saveRed = bmpImage->red_mask;
        saveBlue= bmpImage->blue_mask;
        saveGreen = bmpImage->green_mask;

        XShmGetImage( gdi_display, descr->drawable, bmpImage,
                      descr->xSrc, descr->ySrc, AllPlanes);

        bmpImage->red_mask = saveRed;
        bmpImage->blue_mask = saveBlue;
        bmpImage->green_mask = saveGreen;
    }
    else
#endif /* HAVE_LIBXXSHM */
    {
        TRACE("XGetSubImage(%p,%ld,%d,%d,%d,%d,%ld,%d,%p,%d,%d)\n",
              gdi_display, descr->drawable, descr->xSrc, descr->ySrc, descr->width,
              lines, AllPlanes, ZPixmap, bmpImage, descr->xDest, descr->yDest);
        XGetSubImage( gdi_display, descr->drawable, descr->xSrc, descr->ySrc,
                      descr->width, lines, AllPlanes, ZPixmap,
                      bmpImage, descr->xDest, descr->yDest );
    }
    wine_tsx11_unlock();

    TRACE("Dib: depth=%2d r=%x g=%x b=%x\n",
          descr->infoBpp,descr->rMask,descr->gMask,descr->bMask);
    TRACE("Bmp: depth=%2d/%2d r=%lx g=%lx b=%lx\n",
          bmpImage->depth,bmpImage->bits_per_pixel,
          bmpImage->red_mask,bmpImage->green_mask,bmpImage->blue_mask);
      /* Transfer the pixels */
    switch(descr->infoBpp)
    {
    case 1:
          X11DRV_DIB_GetImageBits_1( descr->lines,(LPVOID)descr->bits,
				     descr->infoWidth, descr->width,
				     descr->colorMap, descr->palentry,
                                     bmpImage, descr->dibpitch );
       break;

    case 4:
        if (descr->compression) {
	   FIXME("Compression not yet supported!\n");
           if(descr->sizeImage < X11DRV_DIB_GetDIBWidthBytes( descr->infoWidth, 4 ) * abs(descr->lines))
               break;
        }
        X11DRV_DIB_GetImageBits_4( descr->lines,(LPVOID)descr->bits,
                                   descr->infoWidth, descr->width,
                                   descr->colorMap, descr->palentry,
                                   bmpImage, descr->dibpitch );
        break;
    case 8:
        if (descr->compression) {
	   FIXME("Compression not yet supported!\n");
           if(descr->sizeImage < X11DRV_DIB_GetDIBWidthBytes( descr->infoWidth, 8 ) * abs(descr->lines))
               break;
        }
        X11DRV_DIB_GetImageBits_8( descr->lines, (LPVOID)descr->bits,
                                   descr->infoWidth, descr->width,
                                   descr->colorMap, descr->palentry,
                                   bmpImage, descr->dibpitch );
        break;
    case 15:
    case 16:
       X11DRV_DIB_GetImageBits_16( descr->physDev, descr->lines, (LPVOID)descr->bits,
				   descr->infoWidth,descr->width,
				   descr->palentry,
				   descr->rMask, descr->gMask, descr->bMask,
				   bmpImage, descr->dibpitch );
       break;

    case 24:
       X11DRV_DIB_GetImageBits_24( descr->physDev, descr->lines, (LPVOID)descr->bits,
				   descr->infoWidth,descr->width,
				   descr->palentry,
				   descr->rMask, descr->gMask, descr->bMask,
                                   bmpImage, descr->dibpitch);
       break;

    case 32:
       X11DRV_DIB_GetImageBits_32( descr->physDev, descr->lines, (LPVOID)descr->bits,
				   descr->infoWidth, descr->width,
				   descr->palentry,
                                   descr->rMask, descr->gMask, descr->bMask,
                                   bmpImage, descr->dibpitch);
       break;

    default:
        WARN("(%d): Invalid depth\n", descr->infoBpp );
        break;
    }

    if (old_data) bmpImage->data = old_data;
    if (!descr->image) X11DRV_DIB_DestroyXImage( bmpImage );
    return lines;
}

/*************************************************************************
 *		X11DRV_SetDIBitsToDevice
 *
 */
INT CDECL X11DRV_SetDIBitsToDevice( X11DRV_PDEVICE *physDev, INT xDest, INT yDest, DWORD cx,
                                    DWORD cy, INT xSrc, INT ySrc,
                                    UINT startscan, UINT lines, LPCVOID bits,
                                    const BITMAPINFO *info, UINT coloruse )
{
    X11DRV_DIB_IMAGEBITS_DESCR descr;
    INT result;
    LONG width, height;
    BOOL top_down;
    POINT pt;
    int rop = X11DRV_XROPfunction[GetROP2(physDev->hdc) - 1];

    if (DIB_GetBitmapInfo( &info->bmiHeader, &width, &height,
			   &descr.infoBpp, &descr.compression ) == -1)
        return 0;

    top_down = (height < 0);
    if (top_down) height = -height;

    pt.x = xDest;
    pt.y = yDest;
    LPtoDP(physDev->hdc, &pt, 1);

    if (!lines || (startscan >= height)) return 0;
    if (!top_down && startscan + lines > height) lines = height - startscan;

    /* make xSrc,ySrc point to the upper-left corner, not the lower-left one,
     * and clamp all values to fit inside [startscan,startscan+lines]
     */
    if (ySrc + cy <= startscan + lines)
    {
        UINT y = startscan + lines - (ySrc + cy);
        if (ySrc < startscan) cy -= (startscan - ySrc);
        if (!top_down)
        {
            /* avoid getting unnecessary lines */
            ySrc = 0;
            if (y >= lines) return 0;
            lines -= y;
        }
        else
        {
            if (y >= lines) return lines;
            ySrc = y;  /* need to get all lines in top down mode */
        }
    }
    else
    {
        if (ySrc >= startscan + lines) return lines;
        pt.y += ySrc + cy - (startscan + lines);
        cy = startscan + lines - ySrc;
        ySrc = 0;
        if (cy > lines) cy = lines;
    }
    if (xSrc >= width) return lines;
    if (xSrc + cx >= width) cx = width - xSrc;
    if (!cx || !cy) return lines;

    /* Update the pixmap from the DIB section */
    X11DRV_LockDIBSection(physDev, DIB_Status_GdiMod);

    X11DRV_SetupGCForText( physDev );  /* To have the correct colors */
    wine_tsx11_lock();
    XSetFunction(gdi_display, physDev->gc, rop);
    wine_tsx11_unlock();

    switch (descr.infoBpp)
    {
       case 1:
       case 4:
       case 8:
               descr.colorMap = (RGBQUAD *)X11DRV_DIB_BuildColorMap(
                                            physDev, coloruse,
                                            physDev->depth, info, &descr.nColorMap );
               if (!descr.colorMap) return 0;
               descr.rMask = descr.gMask = descr.bMask = 0;
               break;
       case 15:
       case 16:
               descr.rMask = (descr.compression == BI_BITFIELDS) ? *(const DWORD *)info->bmiColors       : 0x7c00;
               descr.gMask = (descr.compression == BI_BITFIELDS) ? *((const DWORD *)info->bmiColors + 1) : 0x03e0;
               descr.bMask = (descr.compression == BI_BITFIELDS) ? *((const DWORD *)info->bmiColors + 2) : 0x001f;
               descr.colorMap = 0;
               break;

       case 24:
       case 32:
               descr.rMask = (descr.compression == BI_BITFIELDS) ? *(const DWORD *)info->bmiColors       : 0xff0000;
               descr.gMask = (descr.compression == BI_BITFIELDS) ? *((const DWORD *)info->bmiColors + 1) : 0x00ff00;
               descr.bMask = (descr.compression == BI_BITFIELDS) ? *((const DWORD *)info->bmiColors + 2) : 0x0000ff;
               descr.colorMap = 0;
               break;
    }

    descr.physDev    = physDev;
    descr.bits       = bits;
    descr.image      = NULL;
    descr.palentry   = NULL;
    descr.lines      = top_down ? -lines : lines;
    descr.infoWidth  = width;
    descr.depth      = physDev->depth;
    descr.shifts     = physDev->color_shifts;
    descr.drawable   = physDev->drawable;
    descr.gc         = physDev->gc;
    descr.xSrc       = xSrc;
    descr.ySrc       = ySrc;
    descr.xDest      = physDev->dc_rect.left + pt.x;
    descr.yDest      = physDev->dc_rect.top + pt.y;
    descr.width      = cx;
    descr.height     = cy;
    descr.shm_mode   = X11DRV_SHM_NONE;
    descr.dibpitch   = ((width * descr.infoBpp + 31) &~31) / 8;
    descr.physBitmap = NULL;

    result = X11DRV_DIB_SetImageBits( &descr );

    if (descr.infoBpp <= 8)
       HeapFree(GetProcessHeap(), 0, descr.colorMap);

    /* Update the DIBSection of the pixmap */
    X11DRV_UnlockDIBSection(physDev, TRUE);

    return result;
}

/***********************************************************************
 *           SetDIBits   (X11DRV.@)
 */
INT CDECL X11DRV_SetDIBits( X11DRV_PDEVICE *physDev, HBITMAP hbitmap, UINT startscan,
                            UINT lines, LPCVOID bits, const BITMAPINFO *info, UINT coloruse )
{
  X_PHYSBITMAP *physBitmap = X11DRV_get_phys_bitmap( hbitmap );
  X11DRV_DIB_IMAGEBITS_DESCR descr;
  DIBSECTION ds;
  LONG width, height, tmpheight;
  INT result;

  descr.physDev = physDev;

  if (!physBitmap) return 0;

  if (DIB_GetBitmapInfo( &info->bmiHeader, &width, &height,
			 &descr.infoBpp, &descr.compression ) == -1)
      return 0;

  tmpheight = height;
  if (height < 0) height = -height;
  if (!lines || (startscan >= height))
      return 0;

  if (!GetObjectW( hbitmap, sizeof(ds), &ds )) return 0;

  if (startscan + lines > height) lines = height - startscan;

  switch (descr.infoBpp)
  {
       case 1:
       case 4:
       case 8:
	       descr.colorMap = (RGBQUAD *)X11DRV_DIB_BuildColorMap(
                                                          descr.physDev, coloruse,
                                                          physBitmap->pixmap_depth,
                                                          info, &descr.nColorMap );
               if (!descr.colorMap) return 0;
               descr.rMask = descr.gMask = descr.bMask = 0;
               break;
       case 15:
       case 16:
               descr.rMask = (descr.compression == BI_BITFIELDS) ? *(const DWORD *)info->bmiColors       : 0x7c00;
               descr.gMask = (descr.compression == BI_BITFIELDS) ? *((const DWORD *)info->bmiColors + 1) : 0x03e0;
               descr.bMask = (descr.compression == BI_BITFIELDS) ? *((const DWORD *)info->bmiColors + 2) : 0x001f;
               descr.colorMap = 0;
               break;

       case 24:
       case 32:
               descr.rMask = (descr.compression == BI_BITFIELDS) ? *(const DWORD *)info->bmiColors       : 0xff0000;
               descr.gMask = (descr.compression == BI_BITFIELDS) ? *((const DWORD *)info->bmiColors + 1) : 0x00ff00;
               descr.bMask = (descr.compression == BI_BITFIELDS) ? *((const DWORD *)info->bmiColors + 2) : 0x0000ff;
               descr.colorMap = 0;
               break;

       default: break;
  }

  descr.bits       = bits;
  descr.image      = NULL;
  descr.palentry   = NULL;
  descr.infoWidth  = width;
  descr.lines      = tmpheight >= 0 ? lines : -lines;
  descr.depth      = physBitmap->pixmap_depth;
  descr.shifts     = physBitmap->trueColor ? &physBitmap->pixmap_color_shifts : NULL;
  descr.drawable   = physBitmap->pixmap;
  descr.gc         = get_bitmap_gc(physBitmap->pixmap_depth);
  descr.xSrc       = 0;
  descr.ySrc       = 0;
  descr.xDest      = 0;
  descr.yDest      = height - startscan - lines;
  descr.width      = ds.dsBm.bmWidth;
  descr.height     = lines;
  descr.shm_mode   = X11DRV_SHM_NONE;
  descr.dibpitch   = ((descr.infoWidth * descr.infoBpp + 31) &~31) / 8;
  descr.physBitmap = NULL;
  X11DRV_DIB_Lock( physBitmap, DIB_Status_GdiMod );
  result = X11DRV_DIB_SetImageBits( &descr );

  /* optimisation for the case where the input bits are in exactly the same
   * format as the internal representation and copying to the app bits is
   * cheap - saves a round trip to the X server */
  if (descr.compression == BI_RGB &&
      coloruse == DIB_RGB_COLORS &&
      descr.infoBpp == ds.dsBm.bmBitsPixel &&
      physBitmap->base && physBitmap->size < 65536)
  {
      unsigned int srcwidthb = X11DRV_DIB_GetDIBWidthBytes( width, descr.infoBpp );
      int dstwidthb = ds.dsBm.bmWidthBytes;
      LPBYTE dbits = physBitmap->base + startscan * dstwidthb;
      const BYTE *sbits = bits;
      int widthb;
      UINT y;

      TRACE("syncing compatible set bits to app bits\n");
      if ((tmpheight < 0) ^ physBitmap->topdown)
      {
          dbits += dstwidthb * (lines-1);
          dstwidthb = -dstwidthb;
      }
      X11DRV_DIB_DoProtectDIBSection( physBitmap, PAGE_READWRITE );
      widthb = min(srcwidthb, abs(dstwidthb));
      for (y = 0; y < lines; y++, dbits += dstwidthb, sbits += srcwidthb)
          memcpy(dbits, sbits, widthb);
      X11DRV_DIB_DoProtectDIBSection( physBitmap, PAGE_READONLY );
      physBitmap->status = DIB_Status_InSync;
  }
  X11DRV_DIB_Unlock( physBitmap, TRUE );

  HeapFree(GetProcessHeap(), 0, descr.colorMap);

  return result;
}

/***********************************************************************
 *           GetDIBits   (X11DRV.@)
 */
INT CDECL X11DRV_GetDIBits( X11DRV_PDEVICE *physDev, HBITMAP hbitmap, UINT startscan, UINT lines,
                            LPVOID bits, BITMAPINFO *info, UINT coloruse )
{
  X_PHYSBITMAP *physBitmap = X11DRV_get_phys_bitmap( hbitmap );
  DIBSECTION dib;
  X11DRV_DIB_IMAGEBITS_DESCR descr;
  PALETTEENTRY palette[256];
  size_t obj_size;
  int height;
  LONG width, tempHeight;
  int bitmap_type;
  BOOL core_header;
  void* colorPtr;
  static const PALETTEENTRY peBlack = {0,0,0,0};
  static const PALETTEENTRY peWhite = {255,255,255,0};

  if (!physBitmap) return 0;
  if (!(obj_size = GetObjectW( hbitmap, sizeof(dib), &dib ))) return 0;

  bitmap_type = DIB_GetBitmapInfo( (BITMAPINFOHEADER*)info, &width, &tempHeight, &descr.infoBpp, &descr.compression);
  if (bitmap_type == -1)
  {
      ERR("Invalid bitmap\n");
      return 0;
  }

  if (physBitmap->pixmap_depth > 1)
  {
    GetPaletteEntries( GetCurrentObject( physDev->hdc, OBJ_PAL ), 0, 256, palette );
  }
  else
  {
    palette[0] = peBlack;
    palette[1] = peWhite;
  }

  descr.lines = tempHeight;
  core_header = (bitmap_type == 0);
  colorPtr = (LPBYTE) info + (WORD) info->bmiHeader.biSize;

  TRACE("%u scanlines of (%i,%i) -> (%i,%i) starting from %u\n",
        lines, dib.dsBm.bmWidth, dib.dsBm.bmHeight, width, descr.lines, startscan);

  if( lines > dib.dsBm.bmHeight ) lines = dib.dsBm.bmHeight;

  height = descr.lines;
  if (height < 0) height = -height;
  if( lines > height ) lines = height;
  /* Top-down images have a negative biHeight, the scanlines of these images
   * were inverted in X11DRV_DIB_GetImageBits_xx
   * To prevent this we simply change the sign of lines
   * (the number of scan lines to copy).
   * Negative lines are correctly handled by X11DRV_DIB_GetImageBits_xx.
   */
  if( descr.lines < 0 && lines > 0) lines = -lines;

  if( startscan >= dib.dsBm.bmHeight ) return 0;

  descr.colorMap = NULL;

  switch (descr.infoBpp)
  {
      case 1:
      case 4:
      case 8:
          descr.rMask= descr.gMask = descr.bMask = 0;
          if(coloruse == DIB_RGB_COLORS)
              descr.colorMap = colorPtr;
          else {
              int num_colors = 1 << descr.infoBpp, i;
              RGBQUAD *rgb;
              COLORREF colref;
              WORD *index = colorPtr;
              descr.colorMap = rgb = HeapAlloc(GetProcessHeap(), 0, num_colors * sizeof(RGBQUAD));
              for(i = 0; i < num_colors; i++, rgb++, index++) {
                  colref = X11DRV_PALETTE_ToLogical(physDev, X11DRV_PALETTE_ToPhysical(physDev, PALETTEINDEX(*index)));
                  rgb->rgbRed = GetRValue(colref);
                  rgb->rgbGreen = GetGValue(colref);
                  rgb->rgbBlue = GetBValue(colref);
                  rgb->rgbReserved = 0;
              }
          }
          break;
      case 15:
      case 16:
          descr.rMask = (descr.compression == BI_BITFIELDS) ? *(const DWORD *)info->bmiColors       : 0x7c00;
          descr.gMask = (descr.compression == BI_BITFIELDS) ? *((const DWORD *)info->bmiColors + 1) : 0x03e0;
          descr.bMask = (descr.compression == BI_BITFIELDS) ? *((const DWORD *)info->bmiColors + 2) : 0x001f;
          break;
      case 24:
      case 32:
          descr.rMask = (descr.compression == BI_BITFIELDS) ? *(const DWORD *)info->bmiColors       : 0xff0000;
          descr.gMask = (descr.compression == BI_BITFIELDS) ? *((const DWORD *)info->bmiColors + 1) : 0x00ff00;
          descr.bMask = (descr.compression == BI_BITFIELDS) ? *((const DWORD *)info->bmiColors + 2) : 0x0000ff;
          break;
  }

  descr.physDev    = physDev;
  descr.palentry   = palette;
  descr.bits       = bits;
  descr.image      = physBitmap->image;
  descr.infoWidth  = width;
  descr.lines      = lines;
  descr.depth      = physBitmap->pixmap_depth;
  descr.shifts     = physBitmap->trueColor ? &physBitmap->pixmap_color_shifts : NULL;
  descr.drawable   = physBitmap->pixmap;
  descr.gc         = get_bitmap_gc(physBitmap->pixmap_depth);
  descr.width      = dib.dsBm.bmWidth;
  descr.height     = dib.dsBm.bmHeight;
  descr.xDest      = 0;
  descr.yDest      = 0;
  descr.xSrc       = 0;
  descr.sizeImage  = core_header ? 0 : info->bmiHeader.biSizeImage;
  descr.physBitmap = physBitmap;

  if (descr.lines > 0)
  {
     descr.ySrc = (descr.height-1) - (startscan + (lines-1));
  }
  else
  {
     descr.ySrc = startscan;
  }
  descr.shm_mode = physBitmap->shm_mode;
  descr.dibpitch = (obj_size == sizeof(DIBSECTION)) ? dib.dsBm.bmWidthBytes
		       : (((descr.infoWidth * descr.infoBpp + 31) &~31) / 8);

  X11DRV_DIB_Lock( physBitmap, DIB_Status_GdiMod );
  X11DRV_DIB_GetImageBits( &descr );
  X11DRV_DIB_Unlock( physBitmap, TRUE );

  if(!core_header && info->bmiHeader.biSizeImage == 0) /* Fill in biSizeImage */
      info->bmiHeader.biSizeImage = X11DRV_DIB_GetDIBImageBytes( descr.infoWidth,
                                                                 descr.lines,
                                                                 descr.infoBpp);

  if (descr.compression == BI_BITFIELDS)
  {
    *(DWORD *)info->bmiColors       = descr.rMask;
    *((DWORD *)info->bmiColors + 1) = descr.gMask;
    *((DWORD *)info->bmiColors + 2) = descr.bMask;
  }
  else if (!core_header)
  {
    /* if RLE or JPEG compression were supported,
     * this line would be invalid. */
    info->bmiHeader.biCompression = 0;
  }

  if(descr.colorMap != colorPtr)
      HeapFree(GetProcessHeap(), 0, descr.colorMap);
  return lines;
}

/***********************************************************************
 *           X11DRV_DIB_DoCopyDIBSection
 */
static void X11DRV_DIB_DoCopyDIBSection(X_PHYSBITMAP *physBitmap, BOOL toDIB,
					void *colorMap, int nColorMap,
					Drawable dest, GC gc,
					DWORD xSrc, DWORD ySrc,
					DWORD xDest, DWORD yDest,
					DWORD width, DWORD height)
{
  DIBSECTION dibSection;
  X11DRV_DIB_IMAGEBITS_DESCR descr;
  int identity[2] = {0,1};

  if (!GetObjectW( physBitmap->hbitmap, sizeof(dibSection), &dibSection )) return;

  descr.physDev     = NULL;
  descr.palentry    = NULL;
  descr.infoWidth   = dibSection.dsBmih.biWidth;
  descr.infoBpp     = dibSection.dsBmih.biBitCount;
  descr.lines       = physBitmap->topdown ? -dibSection.dsBmih.biHeight : dibSection.dsBmih.biHeight;
  descr.image       = physBitmap->image;
  descr.colorMap    = colorMap;
  descr.nColorMap   = nColorMap;
  descr.bits        = dibSection.dsBm.bmBits;
  descr.depth       = physBitmap->pixmap_depth;
  descr.shifts      = physBitmap->trueColor ? &physBitmap->pixmap_color_shifts : NULL;
  descr.compression = dibSection.dsBmih.biCompression;
  descr.physBitmap  = physBitmap;

  if(descr.infoBpp == 1)
      descr.colorMap = (void*)identity;

  switch (descr.infoBpp)
  {
    case 1:
    case 4:
    case 8:
      descr.rMask = descr.gMask = descr.bMask = 0;
      break;
    case 15:
    case 16:
      descr.rMask = (descr.compression == BI_BITFIELDS) ? dibSection.dsBitfields[0] : 0x7c00;
      descr.gMask = (descr.compression == BI_BITFIELDS) ? dibSection.dsBitfields[1] : 0x03e0;
      descr.bMask = (descr.compression == BI_BITFIELDS) ? dibSection.dsBitfields[2] : 0x001f;
      break;

    case 24:
    case 32:
      descr.rMask = (descr.compression == BI_BITFIELDS) ? dibSection.dsBitfields[0] : 0xff0000;
      descr.gMask = (descr.compression == BI_BITFIELDS) ? dibSection.dsBitfields[1] : 0x00ff00;
      descr.bMask = (descr.compression == BI_BITFIELDS) ? dibSection.dsBitfields[2] : 0x0000ff;
      break;
  }

  /* Hack for now */
  descr.drawable  = dest;
  descr.gc        = gc;
  descr.xSrc      = xSrc;
  descr.ySrc      = ySrc;
  descr.xDest     = xDest;
  descr.yDest     = yDest;
  descr.width     = width;
  descr.height    = height;
  descr.sizeImage = 0;

  descr.shm_mode = physBitmap->shm_mode;
#ifdef HAVE_LIBXXSHM
  if (physBitmap->shm_mode == X11DRV_SHM_PIXMAP && physBitmap->pixmap != dest)
  {
    descr.shm_mode = X11DRV_SHM_NONE;
  }
#endif
  descr.dibpitch = dibSection.dsBm.bmWidthBytes;

  if (toDIB)
    {
      TRACE("Copying from Pixmap to DIB bits\n");
      X11DRV_DIB_GetImageBits( &descr );
    }
  else
    {
      TRACE("Copying from DIB bits to Pixmap\n");
      X11DRV_DIB_SetImageBits( &descr );
    }
}

/***********************************************************************
 *           X11DRV_DIB_CopyDIBSection
 */
void X11DRV_DIB_CopyDIBSection(X11DRV_PDEVICE *physDevSrc, X11DRV_PDEVICE *physDevDst,
                               DWORD xSrc, DWORD ySrc, DWORD xDest, DWORD yDest,
                               DWORD width, DWORD height)
{
  DIBSECTION dib;
  X_PHYSBITMAP *physBitmap;
  unsigned int nColorMap;
  int* x11ColorMap;
  int freeColorMap;

  TRACE("(%p,%p,%d,%d,%d,%d,%d,%d)\n", physDevSrc->hdc, physDevDst->hdc,
    xSrc, ySrc, xDest, yDest, width, height);
  /* this function is meant as an optimization for BitBlt,
   * not to be called otherwise */
  physBitmap = physDevSrc->bitmap;
  if (!physBitmap || GetObjectW( physBitmap->hbitmap, sizeof(dib), &dib ) != sizeof(dib))
  {
    ERR("called for non-DIBSection!?\n");
    return;
  }
  /* while BitBlt should already have made sure we only get
   * positive values, we should check for oversize values */
  if ((xSrc < dib.dsBm.bmWidth) &&
      (ySrc < dib.dsBm.bmHeight)) {
    if (xSrc + width > dib.dsBm.bmWidth)
      width = dib.dsBm.bmWidth - xSrc;
    if (ySrc + height > dib.dsBm.bmHeight)
      height = dib.dsBm.bmHeight - ySrc;
    /* if the source bitmap is 8bpp or less, we're supposed to use the
     * DC's palette for color conversion (not the DIB color table) */
    if (dib.dsBm.bmBitsPixel <= 8) {
      HPALETTE hPalette = GetCurrentObject( physDevSrc->hdc, OBJ_PAL );
      if (!hPalette || (hPalette == GetStockObject(DEFAULT_PALETTE))) {
	/* HACK: no palette has been set in the source DC,
	 * use the DIB colormap instead - this is necessary in some
	 * cases since we need to do depth conversion in some places
	 * where real Windows can just copy data straight over */
	x11ColorMap = physBitmap->colorMap;
	nColorMap = physBitmap->nColorMap;
        freeColorMap = FALSE;
      } else {
	  const BITMAPINFO* info = (BITMAPINFO*)&dib.dsBmih;
	  unsigned int i;

	  nColorMap = X11DRV_DIB_GetColorCount(info);
	  x11ColorMap = HeapAlloc(GetProcessHeap(), 0, nColorMap * sizeof(int));
	  for (i = 0; i < nColorMap; i++)
	      x11ColorMap[i] = X11DRV_PALETTE_ToPhysical(physDevSrc, PALETTEINDEX(i));
	  freeColorMap = TRUE;
      }
    }
    else
    {
        nColorMap = 0;
        x11ColorMap = NULL;
        freeColorMap = FALSE;
    }
    /* perform the copy */
    X11DRV_DIB_DoCopyDIBSection(physBitmap, FALSE, x11ColorMap, nColorMap,
				physDevDst->drawable, physDevDst->gc, xSrc, ySrc,
                                physDevDst->dc_rect.left + xDest, physDevDst->dc_rect.top + yDest,
				width, height);
    /* free color mapping */
    if (freeColorMap)
      HeapFree(GetProcessHeap(), 0, x11ColorMap);
  }
}

/***********************************************************************
 *           X11DRV_DIB_DoUpdateDIBSection
 */
static void X11DRV_DIB_DoUpdateDIBSection(X_PHYSBITMAP *physBitmap, BOOL toDIB)
{
    BITMAP bitmap;

    GetObjectW( physBitmap->hbitmap, sizeof(bitmap), &bitmap );
    X11DRV_DIB_DoCopyDIBSection(physBitmap, toDIB,
                                physBitmap->colorMap, physBitmap->nColorMap,
                                physBitmap->pixmap, get_bitmap_gc(physBitmap->pixmap_depth),
                                0, 0, 0, 0, bitmap.bmWidth, bitmap.bmHeight);
}

/***********************************************************************
 *           X11DRV_DIB_FaultHandler
 */
static LONG CALLBACK X11DRV_DIB_FaultHandler( PEXCEPTION_POINTERS ep )
{
    X_PHYSBITMAP *physBitmap = NULL;
    BOOL found = FALSE;
    BYTE *addr;
    struct list *ptr;
    const size_t pagemask = getpagesize() - 1;

    if (ep->ExceptionRecord->ExceptionCode != EXCEPTION_ACCESS_VIOLATION)
        return EXCEPTION_CONTINUE_SEARCH;

    addr = (BYTE *)ep->ExceptionRecord->ExceptionInformation[1];

    EnterCriticalSection(&dibs_cs);
    LIST_FOR_EACH( ptr, &dibs_list )
    {
        physBitmap = LIST_ENTRY( ptr, X_PHYSBITMAP, entry );
        if ((physBitmap->base <= addr) &&
            (addr < physBitmap->base + ((physBitmap->size + pagemask) & ~pagemask)))
        {
            found = TRUE;
            break;
        }
    }
    LeaveCriticalSection(&dibs_cs);

    if (!found) return EXCEPTION_CONTINUE_SEARCH;

    if (addr >= physBitmap->base + physBitmap->size)
        WARN( "%p: access to %p beyond the end of the DIB\n", physBitmap->hbitmap, addr );

    X11DRV_DIB_Lock( physBitmap, DIB_Status_None );
    if (ep->ExceptionRecord->ExceptionInformation[0] == EXCEPTION_WRITE_FAULT) {
        /* the app tried to write the DIB bits */
        X11DRV_DIB_Coerce( physBitmap, DIB_Status_AppMod);
    } else {
        /* the app tried to read the DIB bits */
        X11DRV_DIB_Coerce( physBitmap, DIB_Status_InSync);
    }
    X11DRV_DIB_Unlock( physBitmap, TRUE );

    return EXCEPTION_CONTINUE_EXECUTION;
}

/***********************************************************************
 *           X11DRV_DIB_Coerce
 */
static INT X11DRV_DIB_Coerce(X_PHYSBITMAP *physBitmap, INT req)
{
    INT ret = DIB_Status_None;

    if (!physBitmap->image) return ret;  /* not a DIB section */
    EnterCriticalSection(&physBitmap->lock);
    ret = physBitmap->status;
    switch (req) {
    case DIB_Status_GdiMod:
      /* GDI access - request to draw on pixmap */
      switch (physBitmap->status)
      {
        default:
        case DIB_Status_None:
	  physBitmap->p_status = DIB_Status_GdiMod;
	  X11DRV_DIB_DoUpdateDIBSection( physBitmap, FALSE );
	  break;

        case DIB_Status_GdiMod:
	  TRACE("GdiMod requested in status GdiMod\n" );
	  physBitmap->p_status = DIB_Status_GdiMod;
	  break;

        case DIB_Status_InSync:
	  TRACE("GdiMod requested in status InSync\n" );
	  X11DRV_DIB_DoProtectDIBSection( physBitmap, PAGE_NOACCESS );
	  physBitmap->status = DIB_Status_GdiMod;
	  physBitmap->p_status = DIB_Status_InSync;
	  break;

        case DIB_Status_AppMod:
	  TRACE("GdiMod requested in status AppMod\n" );
	  /* make it readonly to avoid app changing data while we copy */
	  X11DRV_DIB_DoProtectDIBSection( physBitmap, PAGE_READONLY );
	  X11DRV_DIB_DoUpdateDIBSection( physBitmap, FALSE );
	  X11DRV_DIB_DoProtectDIBSection( physBitmap, PAGE_NOACCESS );
	  physBitmap->p_status = DIB_Status_AppMod;
	  physBitmap->status = DIB_Status_GdiMod;
	  break;
      }
      break;

    case DIB_Status_InSync:
      /* App access - request access to read DIB surface */
      /* (typically called from signal handler) */
      switch (physBitmap->status)
      {
        default:
        case DIB_Status_None:
	  /* shouldn't happen from signal handler */
	  break;

	case DIB_Status_GdiMod:
	  TRACE("InSync requested in status GdiMod\n" );
	  X11DRV_DIB_DoProtectDIBSection( physBitmap, PAGE_READWRITE );
	  X11DRV_DIB_DoUpdateDIBSection( physBitmap, TRUE );
	  X11DRV_DIB_DoProtectDIBSection( physBitmap, PAGE_READONLY );
	  physBitmap->status = DIB_Status_InSync;
	  break;

        case DIB_Status_InSync:
	  TRACE("InSync requested in status InSync\n" );
	  /* shouldn't happen from signal handler */
	  break;

        case DIB_Status_AppMod:
	  TRACE("InSync requested in status AppMod\n" );
	  /* no reason to do anything here, and this
	   * shouldn't happen from signal handler */
	  break;
      }
      break;

    case DIB_Status_AppMod:
      /* App access - request access to write DIB surface */
      /* (typically called from signal handler) */
      switch (physBitmap->status)
      {
        default:
        case DIB_Status_None:
	  /* shouldn't happen from signal handler */
	  break;

	case DIB_Status_GdiMod:
	  TRACE("AppMod requested in status GdiMod\n" );
	  X11DRV_DIB_DoProtectDIBSection( physBitmap, PAGE_READWRITE );
	  X11DRV_DIB_DoUpdateDIBSection( physBitmap, TRUE );
	  physBitmap->status = DIB_Status_AppMod;
	  break;

        case DIB_Status_InSync:
	  TRACE("AppMod requested in status InSync\n" );
	  X11DRV_DIB_DoProtectDIBSection( physBitmap, PAGE_READWRITE );
	  physBitmap->status = DIB_Status_AppMod;
	  break;

        case DIB_Status_AppMod:
	  TRACE("AppMod requested in status AppMod\n" );
	  /* shouldn't happen from signal handler */
	  break;
      }
      break;

      /* it is up to the caller to do the copy/conversion, probably
       * using the return value to decide where to copy from */
    }
    LeaveCriticalSection(&physBitmap->lock);
    return ret;
}

/***********************************************************************
 *           X11DRV_DIB_Lock
 */
static INT X11DRV_DIB_Lock(X_PHYSBITMAP *physBitmap, INT req)
{
    INT ret = DIB_Status_None;

    if (!physBitmap->image) return ret;  /* not a DIB section */
    TRACE("Locking %p from thread %04x\n", physBitmap->hbitmap, GetCurrentThreadId());
    EnterCriticalSection(&physBitmap->lock);
    ret = physBitmap->status;
    if (req != DIB_Status_None)
      X11DRV_DIB_Coerce(physBitmap, req);
    return ret;
}

/***********************************************************************
 *           X11DRV_DIB_Unlock
 */
static void X11DRV_DIB_Unlock(X_PHYSBITMAP *physBitmap, BOOL commit)
{
    if (!physBitmap->image) return;  /* not a DIB section */
    switch (physBitmap->status)
    {
      default:
      case DIB_Status_None:
	/* in case anyone is wondering, this is the "signal handler doesn't
	 * work" case, where we always have to be ready for app access */
	if (commit) {
	  switch (physBitmap->p_status)
	  {
	    case DIB_Status_GdiMod:
	      TRACE("Unlocking and syncing from GdiMod\n" );
	      X11DRV_DIB_DoUpdateDIBSection( physBitmap, TRUE );
	      break;

	    default:
	      TRACE("Unlocking without needing to sync\n" );
	      break;
	  }
	}
	else TRACE("Unlocking with no changes\n");
	physBitmap->p_status = DIB_Status_None;
	break;

      case DIB_Status_GdiMod:
	TRACE("Unlocking in status GdiMod\n" );
	/* DIB was protected in Coerce */
	if (!commit) {
	  /* no commit, revert to InSync if applicable */
	  if ((physBitmap->p_status == DIB_Status_InSync) ||
	      (physBitmap->p_status == DIB_Status_AppMod)) {
	    X11DRV_DIB_DoProtectDIBSection( physBitmap, PAGE_READONLY );
	    physBitmap->status = DIB_Status_InSync;
	  }
	}
	break;

      case DIB_Status_InSync:
	TRACE("Unlocking in status InSync\n" );
	/* DIB was already protected in Coerce */
	break;

      case DIB_Status_AppMod:
	TRACE("Unlocking in status AppMod\n" );
	/* DIB was already protected in Coerce */
	/* this case is ordinary only called from the signal handler,
	 * so we don't bother to check for !commit */
	break;
    }
    LeaveCriticalSection(&physBitmap->lock);
    TRACE("Unlocked %p\n", physBitmap->hbitmap);
}

/***********************************************************************
 *           X11DRV_CoerceDIBSection
 */
INT X11DRV_CoerceDIBSection(X11DRV_PDEVICE *physDev, INT req)
{
    if (!physDev || !physDev->bitmap) return DIB_Status_None;
    return X11DRV_DIB_Coerce(physDev->bitmap, req);
}

/***********************************************************************
 *           X11DRV_LockDIBSection
 */
INT X11DRV_LockDIBSection(X11DRV_PDEVICE *physDev, INT req)
{
    if (!physDev || !physDev->bitmap) return DIB_Status_None;
    return X11DRV_DIB_Lock(physDev->bitmap, req);
}

/***********************************************************************
 *           X11DRV_UnlockDIBSection
 */
void X11DRV_UnlockDIBSection(X11DRV_PDEVICE *physDev, BOOL commit)
{
    if (!physDev || !physDev->bitmap) return;
    X11DRV_DIB_Unlock(physDev->bitmap, commit);
}


#ifdef HAVE_LIBXXSHM
/***********************************************************************
 *           X11DRV_XShmErrorHandler
 *
 */
static int XShmErrorHandler( Display *dpy, XErrorEvent *event, void *arg )
{
    return 1;  /* FIXME: should check event contents */
}

/***********************************************************************
 *           X11DRV_XShmCreateImage
 *
 */
static XImage *X11DRV_XShmCreateImage( int width, int height, int bpp,
                                       XShmSegmentInfo* shminfo)
{
    XImage *image;

    image = XShmCreateImage(gdi_display, visual, bpp, ZPixmap, NULL, shminfo, width, height);
    if (image)
    {
        shminfo->shmid = shmget(IPC_PRIVATE, image->bytes_per_line * height,
                                  IPC_CREAT|0700);
        if( shminfo->shmid != -1 )
        {
            shminfo->shmaddr = shmat( shminfo->shmid, 0, 0 );
            if( shminfo->shmaddr != (char*)-1 )
            {
                BOOL ok;

                shminfo->readOnly = FALSE;
                X11DRV_expect_error( gdi_display, XShmErrorHandler, NULL );
                ok = (XShmAttach( gdi_display, shminfo ) != 0);
                XSync( gdi_display, False );
                if (X11DRV_check_error()) ok = FALSE;
                if (ok)
                {
                    shmctl(shminfo->shmid, IPC_RMID, 0);
                    return image; /* Success! */
                }
                /* An error occurred */
                shmdt(shminfo->shmaddr);
            }
            shmctl(shminfo->shmid, IPC_RMID, 0);
            shminfo->shmid = -1;
        }
        XFlush(gdi_display);
        XDestroyImage(image);
        image = NULL;
    }
    return image;
}
#endif /* HAVE_LIBXXSHM */


/***********************************************************************
 *           X11DRV_CreateDIBSection   (X11DRV.@)
 */
HBITMAP CDECL X11DRV_CreateDIBSection( X11DRV_PDEVICE *physDev, HBITMAP hbitmap,
                                       const BITMAPINFO *bmi, UINT usage )
{
    X_PHYSBITMAP *physBitmap;
    DIBSECTION dib;
    WORD bpp, compr;
    LONG w, h;
#ifdef HAVE_LIBXXSHM
    int major, minor;
    Bool pixmaps;
#endif

    DIB_GetBitmapInfo( &bmi->bmiHeader, &w, &h, &bpp, &compr );

    if (!(physBitmap = X11DRV_init_phys_bitmap( hbitmap ))) return 0;
    if (h < 0) physBitmap->topdown = TRUE;
    physBitmap->status = DIB_Status_None;

    GetObjectW( hbitmap, sizeof(dib), &dib );

    /* create color map */
    if (dib.dsBm.bmBitsPixel <= 8)
    {
        physBitmap->colorMap = X11DRV_DIB_BuildColorMap( physDev,
                                                         usage, dib.dsBm.bmBitsPixel, bmi,
                                                         &physBitmap->nColorMap );
    }

    if (!X11DRV_XRender_SetPhysBitmapDepth( physBitmap, dib.dsBm.bmBitsPixel, &dib ))
    {
        if (dib.dsBm.bmBitsPixel == 1)
        {
            physBitmap->pixmap_depth = 1;
            physBitmap->trueColor = FALSE;
        }
        else
        {
            physBitmap->pixmap_depth = screen_depth;
            physBitmap->pixmap_color_shifts = X11DRV_PALETTE_default_shifts;
            physBitmap->trueColor = (visual->class == TrueColor || visual->class == DirectColor);
        }
    }

    /* create pixmap and X image */
    wine_tsx11_lock();
#ifdef HAVE_LIBXXSHM
    physBitmap->shminfo.shmid = -1;

    if (XShmQueryVersion( gdi_display, &major, &minor, &pixmaps )
            && (physBitmap->image = X11DRV_XShmCreateImage( dib.dsBm.bmWidth, dib.dsBm.bmHeight,
                                                            physBitmap->pixmap_depth, &physBitmap->shminfo )))
    {
        if (pixmaps)
        {
            physBitmap->shm_mode = X11DRV_SHM_PIXMAP;
            physBitmap->image->data = HeapAlloc( GetProcessHeap(), 0,
                    dib.dsBm.bmHeight * physBitmap->image->bytes_per_line );
        }
        else
        {
            physBitmap->shm_mode = X11DRV_SHM_IMAGE;
            physBitmap->image->data = physBitmap->shminfo.shmaddr;
        }
    }
    else
#endif
    {
        physBitmap->shm_mode = X11DRV_SHM_NONE;
        physBitmap->image = X11DRV_DIB_CreateXImage( dib.dsBm.bmWidth, dib.dsBm.bmHeight,
                                                     physBitmap->pixmap_depth );
    }

#ifdef HAVE_LIBXXSHM
    if (physBitmap->shm_mode == X11DRV_SHM_PIXMAP)
    {
        TRACE("Creating shared pixmap for bmp %p.\n", physBitmap->hbitmap);
        physBitmap->pixmap = XShmCreatePixmap( gdi_display, root_window,
                                               physBitmap->shminfo.shmaddr, &physBitmap->shminfo,
                                               dib.dsBm.bmWidth, dib.dsBm.bmHeight,
                                               physBitmap->pixmap_depth );
    }
    else
#endif
    {
        physBitmap->pixmap = XCreatePixmap( gdi_display, root_window, dib.dsBm.bmWidth,
                                            dib.dsBm.bmHeight, physBitmap->pixmap_depth );
    }

    wine_tsx11_unlock();
    if (!physBitmap->pixmap || !physBitmap->image) return 0;

    if (physBitmap->trueColor)
    {
        ColorShifts *shifts = &physBitmap->pixmap_color_shifts;

        /* When XRender is around and used, we also support dibsections in other formats like 16-bit. In these
         * cases we need to override the mask of XImages. The reason is that during XImage creation the masks are
         * derived from a 24-bit visual (no 16-bit ones are around when X runs at 24-bit). SetImageBits and other
         * functions rely on the color masks for proper color conversion, so we need to override the masks here. */
        physBitmap->image->red_mask = shifts->physicalRed.max << shifts->physicalRed.shift;
        physBitmap->image->green_mask = shifts->physicalGreen.max << shifts->physicalGreen.shift;
        physBitmap->image->blue_mask = shifts->physicalBlue.max << shifts->physicalBlue.shift;
    }

      /* install fault handler */
    InitializeCriticalSection( &physBitmap->lock );
    physBitmap->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": X_PHYSBITMAP.lock");

    physBitmap->base   = dib.dsBm.bmBits;
    physBitmap->size   = dib.dsBmih.biSizeImage;
    physBitmap->status = DIB_Status_AppMod;

    if (!dibs_handler)
        dibs_handler = AddVectoredExceptionHandler( TRUE, X11DRV_DIB_FaultHandler );
    EnterCriticalSection( &dibs_cs );
    list_add_head( &dibs_list, &physBitmap->entry );
    LeaveCriticalSection( &dibs_cs );

    X11DRV_DIB_DoProtectDIBSection( physBitmap, PAGE_READWRITE );

    return hbitmap;
}

/***********************************************************************
 *           X11DRV_DIB_DeleteDIBSection
 */
void X11DRV_DIB_DeleteDIBSection(X_PHYSBITMAP *physBitmap, DIBSECTION *dib)
{
  BOOL last;

  EnterCriticalSection( &dibs_cs );
  list_remove( &physBitmap->entry );
  last = list_empty( &dibs_list );
  LeaveCriticalSection( &dibs_cs );

  if (last)
  {
      RemoveVectoredExceptionHandler( dibs_handler );
      dibs_handler = NULL;
  }

  if (dib->dshSection)
      X11DRV_DIB_Coerce(physBitmap, DIB_Status_InSync);

  if (physBitmap->image)
  {
      wine_tsx11_lock();
#ifdef HAVE_LIBXXSHM
      if (physBitmap->shminfo.shmid != -1)
      {
          XShmDetach( gdi_display, &(physBitmap->shminfo) );
          if (physBitmap->shm_mode == X11DRV_SHM_PIXMAP) X11DRV_DIB_DestroyXImage( physBitmap->image );
          else XDestroyImage( physBitmap->image );
          shmdt( physBitmap->shminfo.shmaddr );
          physBitmap->shminfo.shmid = -1;
          physBitmap->shm_mode = X11DRV_SHM_NONE;
      }
      else
#endif
          X11DRV_DIB_DestroyXImage( physBitmap->image );
      wine_tsx11_unlock();
  }

  HeapFree(GetProcessHeap(), 0, physBitmap->colorMap);
  physBitmap->lock.DebugInfo->Spare[0] = 0;
  DeleteCriticalSection(&physBitmap->lock);
}

/***********************************************************************
 *           SetDIBColorTable   (X11DRV.@)
 */
UINT CDECL X11DRV_SetDIBColorTable( X11DRV_PDEVICE *physDev, UINT start, UINT count, const RGBQUAD *colors )
{
    DIBSECTION dib;
    UINT ret = 0;
    X_PHYSBITMAP *physBitmap = physDev->bitmap;

    if (!physBitmap) return 0;
    GetObjectW( physBitmap->hbitmap, sizeof(dib), &dib );

    if (physBitmap->colorMap && start < physBitmap->nColorMap) {
        UINT end = count + start;
        if (end > physBitmap->nColorMap) end = physBitmap->nColorMap;
        /*
         * Changing color table might change the mapping between
         * DIB colors and X11 colors and thus alter the visible state
         * of the bitmap object.
         */
        /*
         * FIXME we need to recalculate the pen, brush, text and bkgnd pixels here,
         * at least for a 1 bpp dibsection
         */
        X11DRV_DIB_Lock( physBitmap, DIB_Status_AppMod );
        X11DRV_DIB_GenColorMap( physDev, physBitmap->colorMap, DIB_RGB_COLORS,
                                dib.dsBm.bmBitsPixel,
                                TRUE, colors, start, end );
        X11DRV_DIB_Unlock( physBitmap, TRUE );
        ret = end - start;
    }
    return ret;
}


/***********************************************************************
 *           X11DRV_DIB_CreateDIBFromBitmap
 *
 *  Allocates a packed DIB and copies the bitmap data into it.
 */
HGLOBAL X11DRV_DIB_CreateDIBFromBitmap(HDC hdc, HBITMAP hBmp)
{
    BITMAP bmp;
    HGLOBAL hPackedDIB;
    LPBYTE pPackedDIB;
    LPBITMAPINFOHEADER pbmiHeader;
    unsigned int cDataSize, cPackedSize, OffsetBits;
    int nLinesCopied;

    if (!GetObjectW( hBmp, sizeof(bmp), &bmp )) return 0;

    /*
     * A packed DIB contains a BITMAPINFO structure followed immediately by
     * an optional color palette and the pixel data.
     */

    /* Calculate the size of the packed DIB */
    cDataSize = X11DRV_DIB_GetDIBWidthBytes( bmp.bmWidth, bmp.bmBitsPixel ) * abs( bmp.bmHeight );
    cPackedSize = sizeof(BITMAPINFOHEADER)
                  + ( (bmp.bmBitsPixel <= 8) ? (sizeof(RGBQUAD) * (1 << bmp.bmBitsPixel)) : 0 )
                  + cDataSize;
    /* Get the offset to the bits */
    OffsetBits = cPackedSize - cDataSize;

    /* Allocate the packed DIB */
    TRACE("\tAllocating packed DIB of size %d\n", cPackedSize);
    hPackedDIB = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE /*| GMEM_ZEROINIT*/,
                             cPackedSize );
    if ( !hPackedDIB )
    {
        WARN("Could not allocate packed DIB!\n");
        return 0;
    }

    /* A packed DIB starts with a BITMAPINFOHEADER */
    pPackedDIB = GlobalLock(hPackedDIB);
    pbmiHeader = (LPBITMAPINFOHEADER)pPackedDIB;

    /* Init the BITMAPINFOHEADER */
    pbmiHeader->biSize = sizeof(BITMAPINFOHEADER);
    pbmiHeader->biWidth = bmp.bmWidth;
    pbmiHeader->biHeight = bmp.bmHeight;
    pbmiHeader->biPlanes = 1;
    pbmiHeader->biBitCount = bmp.bmBitsPixel;
    pbmiHeader->biCompression = BI_RGB;
    pbmiHeader->biSizeImage = 0;
    pbmiHeader->biXPelsPerMeter = pbmiHeader->biYPelsPerMeter = 0;
    pbmiHeader->biClrUsed = 0;
    pbmiHeader->biClrImportant = 0;

    /* Retrieve the DIB bits from the bitmap and fill in the
     * DIB color table if present */

    nLinesCopied = GetDIBits(hdc,                       /* Handle to device context */
                             hBmp,                      /* Handle to bitmap */
                             0,                         /* First scan line to set in dest bitmap */
                             bmp.bmHeight,              /* Number of scan lines to copy */
                             pPackedDIB + OffsetBits,   /* [out] Address of array for bitmap bits */
                             (LPBITMAPINFO) pbmiHeader, /* [out] Address of BITMAPINFO structure */
                             0);                        /* RGB or palette index */
    GlobalUnlock(hPackedDIB);

    /* Cleanup if GetDIBits failed */
    if (nLinesCopied != bmp.bmHeight)
    {
        TRACE("\tGetDIBits returned %d. Actual lines=%d\n", nLinesCopied, bmp.bmHeight);
        GlobalFree(hPackedDIB);
        hPackedDIB = 0;
    }
    return hPackedDIB;
}


/**************************************************************************
 *	        X11DRV_DIB_CreateDIBFromPixmap
 *
 *  Allocates a packed DIB and copies the Pixmap data into it.
 *  The Pixmap passed in is deleted after the conversion.
 */
HGLOBAL X11DRV_DIB_CreateDIBFromPixmap(Pixmap pixmap, HDC hdc)
{
    HDC hdcMem;
    X_PHYSBITMAP *physBitmap;
    Pixmap orig_pixmap;
    HBITMAP hBmp = 0;
    HGLOBAL hPackedDIB = 0;
    Window root;
    int x,y;               /* Unused */
    unsigned border_width; /* Unused */
    unsigned int depth, width, height;

    /* Get the Pixmap dimensions and bit depth */
    wine_tsx11_lock();
    if (!XGetGeometry(gdi_display, pixmap, &root, &x, &y, &width, &height,
                      &border_width, &depth)) depth = 0;
    wine_tsx11_unlock();
    if (!depth) return 0;

    TRACE("\tPixmap properties: width=%d, height=%d, depth=%d\n",
          width, height, depth);

    /*
     * Create an HBITMAP with the same dimensions and BPP as the pixmap,
     * and make it a container for the pixmap passed.
     */
    if (!(hBmp = CreateBitmap( width, height, 1, depth_to_bpp(depth), NULL ))) return 0;

    /* force bitmap to be owned by a screen DC */
    hdcMem = CreateCompatibleDC( hdc );
    SelectObject( hdcMem, SelectObject( hdcMem, hBmp ));
    DeleteDC( hdcMem );

    physBitmap = X11DRV_get_phys_bitmap( hBmp );

    /* swap the new pixmap in */
    orig_pixmap = physBitmap->pixmap;
    physBitmap->pixmap = pixmap;

    /*
     * Create a packed DIB from the Pixmap wrapper bitmap created above.
     * A packed DIB contains a BITMAPINFO structure followed immediately by
     * an optional color palette and the pixel data.
     */
    hPackedDIB = X11DRV_DIB_CreateDIBFromBitmap(hdc, hBmp);

    /* we can now get rid of the HBITMAP and its original pixmap */
    physBitmap->pixmap = orig_pixmap;
    DeleteObject(hBmp);

    TRACE("\tReturning packed DIB %p\n", hPackedDIB);
    return hPackedDIB;
}


/**************************************************************************
 *	           X11DRV_DIB_CreatePixmapFromDIB
 *
 *    Creates a Pixmap from a packed DIB
 */
Pixmap X11DRV_DIB_CreatePixmapFromDIB( HGLOBAL hPackedDIB, HDC hdc )
{
    Pixmap pixmap;
    X_PHYSBITMAP *physBitmap;
    HBITMAP hBmp;
    LPBITMAPINFO pbmi;

    /* Create a DDB from the DIB */

    pbmi = GlobalLock(hPackedDIB);
    hBmp = CreateDIBitmap(hdc, &pbmi->bmiHeader, CBM_INIT,
                          (LPBYTE)pbmi + bitmap_info_size( pbmi, DIB_RGB_COLORS ),
                          pbmi, DIB_RGB_COLORS);
    GlobalUnlock(hPackedDIB);

    /* clear the physBitmap so that we can steal its pixmap */
    physBitmap = X11DRV_get_phys_bitmap( hBmp );
    pixmap = physBitmap->pixmap;
    physBitmap->pixmap = 0;

    /* Delete the DDB we created earlier now that we have stolen its pixmap */
    DeleteObject(hBmp);

    TRACE("Returning Pixmap %lx\n", pixmap);
    return pixmap;
}
