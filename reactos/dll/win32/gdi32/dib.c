/*
 * GDI device-independent bitmaps
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

/*
  Important information:
  
  * Current Windows versions support two different DIB structures:

    - BITMAPCOREINFO / BITMAPCOREHEADER (legacy structures; used in OS/2)
    - BITMAPINFO / BITMAPINFOHEADER
  
    Most Windows API functions taking a BITMAPINFO* / BITMAPINFOHEADER* also
    accept the old "core" structures, and so must WINE.
    You can distinguish them by looking at the first member (bcSize/biSize),
    or use the internal function DIB_GetBitmapInfo.

    
  * The palettes are stored in different formats:

    - BITMAPCOREINFO: Array of RGBTRIPLE
    - BITMAPINFO:     Array of RGBQUAD

    
  * There are even more DIB headers, but they all extend BITMAPINFOHEADER:
    
    - BITMAPV4HEADER: Introduced in Windows 95 / NT 4.0
    - BITMAPV5HEADER: Introduced in Windows 98 / 2000
    
    If biCompression is BI_BITFIELDS, the color masks are at the same position
    in all the headers (they start at bmiColors of BITMAPINFOHEADER), because
    the new headers have structure members for the masks.


  * You should never access the color table using the bmiColors member,
    because the passed structure may have one of the extended headers
    mentioned above. Use this to calculate the location:
    
    BITMAPINFO* info;
    void* colorPtr = (LPBYTE) info + (WORD) info->bmiHeader.biSize;

    
  * More information:
    Search for "Bitmap Structures" in MSDN
*/

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wownt32.h"
#include "gdi_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(bitmap);


/*
  Some of the following helper functions are duplicated in
  dlls/x11drv/dib.c
*/

/***********************************************************************
 *           DIB_GetDIBWidthBytes
 *
 * Return the width of a DIB bitmap in bytes. DIB bitmap data is 32-bit aligned.
 */
int DIB_GetDIBWidthBytes( int width, int depth )
{
    int words;

    switch(depth)
    {
	case 1:  words = (width + 31) / 32; break;
	case 4:  words = (width + 7) / 8; break;
	case 8:  words = (width + 3) / 4; break;
	case 15:
	case 16: words = (width + 1) / 2; break;
	case 24: words = (width * 3 + 3)/4; break;

	default:
            WARN("(%d): Unsupported depth\n", depth );
	/* fall through */
	case 32:
	        words = width;
    }
    return 4 * words;
}

/***********************************************************************
 *           DIB_GetDIBImageBytes
 *
 * Return the number of bytes used to hold the image in a DIB bitmap.
 */
int DIB_GetDIBImageBytes( int width, int height, int depth )
{
    return DIB_GetDIBWidthBytes( width, depth ) * abs( height );
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
        if (colors > 256) colors = 256;
        if (!colors && (info->bmiHeader.biBitCount <= 8))
            colors = 1 << info->bmiHeader.biBitCount;
        if (info->bmiHeader.biCompression == BI_BITFIELDS) masks = 3;
        size = max( info->bmiHeader.biSize, sizeof(BITMAPINFOHEADER) + masks * sizeof(DWORD) );
        return size + colors * ((coloruse == DIB_RGB_COLORS) ? sizeof(RGBQUAD) : sizeof(WORD));
    }
}


/***********************************************************************
 *           DIB_GetBitmapInfo
 *
 * Get the info from a bitmap header.
 * Return 0 for COREHEADER, 1 for INFOHEADER, -1 for error.
 */
int DIB_GetBitmapInfo( const BITMAPINFOHEADER *header, LONG *width,
                       LONG *height, WORD *planes, WORD *bpp, DWORD *compr, DWORD *size )
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
    if (header->biSize >= sizeof(BITMAPINFOHEADER)) /* assume BITMAPINFOHEADER */
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
 *           StretchDIBits   (GDI32.@)
 */
INT WINAPI StretchDIBits(HDC hdc, INT xDst, INT yDst, INT widthDst,
                       INT heightDst, INT xSrc, INT ySrc, INT widthSrc,
                       INT heightSrc, const void *bits,
                       const BITMAPINFO *info, UINT wUsage, DWORD dwRop )
{
    DC *dc;
    INT ret;

    if (!bits || !info)
	return 0;

    if (!(dc = get_dc_ptr( hdc ))) return 0;

    if(dc->funcs->pStretchDIBits)
    {
        update_dc( dc );
        ret = dc->funcs->pStretchDIBits(dc->physDev, xDst, yDst, widthDst,
                                        heightDst, xSrc, ySrc, widthSrc,
                                        heightSrc, bits, info, wUsage, dwRop);
        release_dc_ptr( dc );
    }
    else /* use StretchBlt */
    {
        LONG height;
        LONG width;
        WORD planes, bpp;
        DWORD compr, size;
        HBITMAP hBitmap;
        BOOL fastpath = FALSE;

        release_dc_ptr( dc );

        if (DIB_GetBitmapInfo( &info->bmiHeader, &width, &height, &planes, &bpp, &compr, &size ) == -1)
        {
            ERR("Invalid bitmap\n");
            return 0;
        }

        if (width < 0)
        {
            ERR("Bitmap has a negative width\n");
            return 0;
        }

        if (xSrc == 0 && ySrc == 0 && widthDst == widthSrc && heightDst == heightSrc &&
            info->bmiHeader.biCompression == BI_RGB)
        {
            /* Windows appears to have a fast case optimization
             * that uses the wrong origin for top-down DIBs */
            if (height < 0 && heightSrc < abs(height)) ySrc = abs(height) - heightSrc;
        }

        hBitmap = GetCurrentObject(hdc, OBJ_BITMAP);

        if (xDst == 0 && yDst == 0 && xSrc == 0 && ySrc == 0 &&
            widthDst == widthSrc && heightDst == heightSrc &&
            info->bmiHeader.biCompression == BI_RGB &&
            dwRop == SRCCOPY)
        {
            BITMAPOBJ *bmp;
            if ((bmp = GDI_GetObjPtr( hBitmap, OBJ_BITMAP )))
            {
                if (bmp->bitmap.bmBitsPixel == bpp &&
                    bmp->bitmap.bmWidth == widthSrc &&
                    bmp->bitmap.bmHeight == heightSrc &&
                    bmp->bitmap.bmPlanes == planes)
                    fastpath = TRUE;
                GDI_ReleaseObj( hBitmap );
            }
        }

        if (fastpath)
        {
            /* fast path */
            TRACE("using fast path\n");
            ret = SetDIBits( hdc, hBitmap, 0, height, bits, info, wUsage);
        }
        else
        {
            /* slow path - need to use StretchBlt */
            HBITMAP hOldBitmap;
            HPALETTE hpal = NULL;
            HDC hdcMem;

            hdcMem = CreateCompatibleDC( hdc );
            hBitmap = CreateCompatibleBitmap(hdc, width, height);
            hOldBitmap = SelectObject( hdcMem, hBitmap );
            if(wUsage == DIB_PAL_COLORS)
            {
                hpal = GetCurrentObject(hdc, OBJ_PAL);
                hpal = SelectPalette(hdcMem, hpal, FALSE);
            }

            if (info->bmiHeader.biCompression == BI_RLE4 ||
	            info->bmiHeader.biCompression == BI_RLE8) {

                /* when RLE compression is used, there may be some gaps (ie the DIB doesn't
                 * contain all the rectangle described in bmiHeader, but only part of it.
                 * This mean that those undescribed pixels must be left untouched.
                 * So, we first copy on a memory bitmap the current content of the
                 * destination rectangle, blit the DIB bits on top of it - hence leaving
                 * the gaps untouched -, and blitting the rectangle back.
                 * This insure that gaps are untouched on the destination rectangle
                 * Not doing so leads to trashed images (the gaps contain what was on the
                 * memory bitmap => generally black or garbage)
                 * Unfortunately, RLE DIBs without gaps will be slowed down. But this is
                 * another speed vs correctness issue. Anyway, if speed is needed, then the
                 * pStretchDIBits function shall be implemented.
                 * ericP (2000/09/09)
                 */

                /* copy existing bitmap from destination dc */
                StretchBlt( hdcMem, xSrc, abs(height) - heightSrc - ySrc,
                            widthSrc, heightSrc, hdc, xDst, yDst, widthDst, heightDst,
                            dwRop );
            }

            ret = SetDIBits(hdcMem, hBitmap, 0, height, bits, info, wUsage);

            /* Origin for DIBitmap may be bottom left (positive biHeight) or top
               left (negative biHeight) */
            if (ret) StretchBlt( hdc, xDst, yDst, widthDst, heightDst,
                                 hdcMem, xSrc, abs(height) - heightSrc - ySrc,
                                 widthSrc, heightSrc, dwRop );
            if(hpal)
                SelectPalette(hdcMem, hpal, FALSE);
            SelectObject( hdcMem, hOldBitmap );
            DeleteDC( hdcMem );
            DeleteObject( hBitmap );
        }
    }
    return ret;
}


/******************************************************************************
 * SetDIBits [GDI32.@]
 *
 * Sets pixels in a bitmap using colors from DIB.
 *
 * PARAMS
 *    hdc       [I] Handle to device context
 *    hbitmap   [I] Handle to bitmap
 *    startscan [I] Starting scan line
 *    lines     [I] Number of scan lines
 *    bits      [I] Array of bitmap bits
 *    info      [I] Address of structure with data
 *    coloruse  [I] Type of color indexes to use
 *
 * RETURNS
 *    Success: Number of scan lines copied
 *    Failure: 0
 */
INT WINAPI SetDIBits( HDC hdc, HBITMAP hbitmap, UINT startscan,
		      UINT lines, LPCVOID bits, const BITMAPINFO *info,
		      UINT coloruse )
{
    DC *dc = get_dc_ptr( hdc );
    BOOL delete_hdc = FALSE;
    BITMAPOBJ *bitmap;
    INT result = 0;

    if (coloruse == DIB_RGB_COLORS && !dc)
    {
        hdc = CreateCompatibleDC(0);
        dc = get_dc_ptr( hdc );
        delete_hdc = TRUE;
    }

    if (!dc) return 0;

    update_dc( dc );

    if (!(bitmap = GDI_GetObjPtr( hbitmap, OBJ_BITMAP )))
    {
        release_dc_ptr( dc );
        if (delete_hdc) DeleteDC(hdc);
        return 0;
    }

    if (!bitmap->funcs && !BITMAP_SetOwnerDC( hbitmap, dc )) goto done;

    result = lines;
    if (bitmap->funcs)
    {
        if (bitmap->funcs != dc->funcs)
            ERR( "not supported: DDB bitmap %p not belonging to device %p\n", hbitmap, hdc );
        else if (dc->funcs->pSetDIBits)
            result = dc->funcs->pSetDIBits( dc->physDev, hbitmap, startscan, lines,
                                            bits, info, coloruse );
    }

 done:
    GDI_ReleaseObj( hbitmap );
    release_dc_ptr( dc );
    if (delete_hdc) DeleteDC(hdc);
    return result;
}


/***********************************************************************
 *           SetDIBitsToDevice   (GDI32.@)
 */
INT WINAPI SetDIBitsToDevice(HDC hdc, INT xDest, INT yDest, DWORD cx,
                           DWORD cy, INT xSrc, INT ySrc, UINT startscan,
                           UINT lines, LPCVOID bits, const BITMAPINFO *info,
                           UINT coloruse )
{
    INT ret;
    DC *dc;

    if (!bits) return 0;

    if (!(dc = get_dc_ptr( hdc ))) return 0;

    if(dc->funcs->pSetDIBitsToDevice)
    {
        update_dc( dc );
        ret = dc->funcs->pSetDIBitsToDevice( dc->physDev, xDest, yDest, cx, cy, xSrc,
					     ySrc, startscan, lines, bits,
					     info, coloruse );
    }
    else {
        FIXME("unimplemented on hdc %p\n", hdc);
	ret = 0;
    }

    release_dc_ptr( dc );
    return ret;
}

/***********************************************************************
 *           SetDIBColorTable    (GDI32.@)
 */
UINT WINAPI SetDIBColorTable( HDC hdc, UINT startpos, UINT entries, CONST RGBQUAD *colors )
{
    DC * dc;
    UINT result = 0;
    BITMAPOBJ * bitmap;

    if (!(dc = get_dc_ptr( hdc ))) return 0;

    if ((bitmap = GDI_GetObjPtr( dc->hBitmap, OBJ_BITMAP )))
    {
        /* Check if currently selected bitmap is a DIB */
        if (bitmap->color_table)
        {
            if (startpos < bitmap->nb_colors)
            {
                if (startpos + entries > bitmap->nb_colors) entries = bitmap->nb_colors - startpos;
                memcpy(bitmap->color_table + startpos, colors, entries * sizeof(RGBQUAD));
                result = entries;
            }
        }
        GDI_ReleaseObj( dc->hBitmap );
    }

    if (dc->funcs->pSetDIBColorTable)
        dc->funcs->pSetDIBColorTable(dc->physDev, startpos, entries, colors);

    release_dc_ptr( dc );
    return result;
}


/***********************************************************************
 *           GetDIBColorTable    (GDI32.@)
 */
UINT WINAPI GetDIBColorTable( HDC hdc, UINT startpos, UINT entries, RGBQUAD *colors )
{
    DC * dc;
    UINT result = 0;

    if (!(dc = get_dc_ptr( hdc ))) return 0;

    if (dc->funcs->pGetDIBColorTable)
        result = dc->funcs->pGetDIBColorTable(dc->physDev, startpos, entries, colors);
    else
    {
        BITMAPOBJ *bitmap = GDI_GetObjPtr( dc->hBitmap, OBJ_BITMAP );
        if (bitmap)
        {
            /* Check if currently selected bitmap is a DIB */
            if (bitmap->color_table)
            {
                if (startpos < bitmap->nb_colors)
                {
                    if (startpos + entries > bitmap->nb_colors) entries = bitmap->nb_colors - startpos;
                    memcpy(colors, bitmap->color_table + startpos, entries * sizeof(RGBQUAD));
                    result = entries;
                }
            }
            GDI_ReleaseObj( dc->hBitmap );
        }
    }
    release_dc_ptr( dc );
    return result;
}

/* FIXME the following two structs should be combined with __sysPalTemplate in
   objects/color.c - this should happen after de-X11-ing both of these
   files.
   NB. RGBQUAD and PALETTEENTRY have different orderings of red, green
   and blue - sigh */

static const RGBQUAD EGAColorsQuads[16] = {
/* rgbBlue, rgbGreen, rgbRed, rgbReserved */
    { 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x80, 0x00 },
    { 0x00, 0x80, 0x00, 0x00 },
    { 0x00, 0x80, 0x80, 0x00 },
    { 0x80, 0x00, 0x00, 0x00 },
    { 0x80, 0x00, 0x80, 0x00 },
    { 0x80, 0x80, 0x00, 0x00 },
    { 0x80, 0x80, 0x80, 0x00 },
    { 0xc0, 0xc0, 0xc0, 0x00 },
    { 0x00, 0x00, 0xff, 0x00 },
    { 0x00, 0xff, 0x00, 0x00 },
    { 0x00, 0xff, 0xff, 0x00 },
    { 0xff, 0x00, 0x00, 0x00 },
    { 0xff, 0x00, 0xff, 0x00 },
    { 0xff, 0xff, 0x00, 0x00 },
    { 0xff, 0xff, 0xff, 0x00 }
};

static const RGBTRIPLE EGAColorsTriples[16] = {
/* rgbBlue, rgbGreen, rgbRed */
    { 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x80 },
    { 0x00, 0x80, 0x00 },
    { 0x00, 0x80, 0x80 },
    { 0x80, 0x00, 0x00 },
    { 0x80, 0x00, 0x80 },
    { 0x80, 0x80, 0x00 },
    { 0x80, 0x80, 0x80 },
    { 0xc0, 0xc0, 0xc0 },
    { 0x00, 0x00, 0xff },
    { 0x00, 0xff, 0x00 },
    { 0x00, 0xff, 0xff },
    { 0xff, 0x00, 0x00 } ,
    { 0xff, 0x00, 0xff },
    { 0xff, 0xff, 0x00 },
    { 0xff, 0xff, 0xff }
};

static const RGBQUAD DefLogPaletteQuads[20] = { /* Copy of Default Logical Palette */
/* rgbBlue, rgbGreen, rgbRed, rgbReserved */
    { 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x80, 0x00 },
    { 0x00, 0x80, 0x00, 0x00 },
    { 0x00, 0x80, 0x80, 0x00 },
    { 0x80, 0x00, 0x00, 0x00 },
    { 0x80, 0x00, 0x80, 0x00 },
    { 0x80, 0x80, 0x00, 0x00 },
    { 0xc0, 0xc0, 0xc0, 0x00 },
    { 0xc0, 0xdc, 0xc0, 0x00 },
    { 0xf0, 0xca, 0xa6, 0x00 },
    { 0xf0, 0xfb, 0xff, 0x00 },
    { 0xa4, 0xa0, 0xa0, 0x00 },
    { 0x80, 0x80, 0x80, 0x00 },
    { 0x00, 0x00, 0xf0, 0x00 },
    { 0x00, 0xff, 0x00, 0x00 },
    { 0x00, 0xff, 0xff, 0x00 },
    { 0xff, 0x00, 0x00, 0x00 },
    { 0xff, 0x00, 0xff, 0x00 },
    { 0xff, 0xff, 0x00, 0x00 },
    { 0xff, 0xff, 0xff, 0x00 }
};

static const RGBTRIPLE DefLogPaletteTriples[20] = { /* Copy of Default Logical Palette */
/* rgbBlue, rgbGreen, rgbRed */
    { 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x80 },
    { 0x00, 0x80, 0x00 },
    { 0x00, 0x80, 0x80 },
    { 0x80, 0x00, 0x00 },
    { 0x80, 0x00, 0x80 },
    { 0x80, 0x80, 0x00 },
    { 0xc0, 0xc0, 0xc0 },
    { 0xc0, 0xdc, 0xc0 },
    { 0xf0, 0xca, 0xa6 },
    { 0xf0, 0xfb, 0xff },
    { 0xa4, 0xa0, 0xa0 },
    { 0x80, 0x80, 0x80 },
    { 0x00, 0x00, 0xf0 },
    { 0x00, 0xff, 0x00 },
    { 0x00, 0xff, 0xff },
    { 0xff, 0x00, 0x00 },
    { 0xff, 0x00, 0xff },
    { 0xff, 0xff, 0x00 },
    { 0xff, 0xff, 0xff}
};


/******************************************************************************
 * GetDIBits [GDI32.@]
 *
 * Retrieves bits of bitmap and copies to buffer.
 *
 * RETURNS
 *    Success: Number of scan lines copied from bitmap
 *    Failure: 0
 */
INT WINAPI GetDIBits(
    HDC hdc,         /* [in]  Handle to device context */
    HBITMAP hbitmap, /* [in]  Handle to bitmap */
    UINT startscan,  /* [in]  First scan line to set in dest bitmap */
    UINT lines,      /* [in]  Number of scan lines to copy */
    LPVOID bits,       /* [out] Address of array for bitmap bits */
    BITMAPINFO * info, /* [out] Address of structure with bitmap data */
    UINT coloruse)   /* [in]  RGB or palette index */
{
    DC * dc;
    BITMAPOBJ * bmp;
    int i;
    int bitmap_type;
    BOOL core_header;
    LONG width;
    LONG height;
    WORD planes, bpp;
    DWORD compr, size;
    void* colorPtr;
    RGBTRIPLE* rgbTriples;
    RGBQUAD* rgbQuads;

    if (!info) return 0;

    bitmap_type = DIB_GetBitmapInfo( &info->bmiHeader, &width, &height, &planes, &bpp, &compr, &size);
    if (bitmap_type == -1)
    {
        ERR("Invalid bitmap format\n");
        return 0;
    }
    core_header = (bitmap_type == 0);
    if (!(dc = get_dc_ptr( hdc )))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    update_dc( dc );
    if (!(bmp = GDI_GetObjPtr( hbitmap, OBJ_BITMAP )))
    {
        release_dc_ptr( dc );
	return 0;
    }

    colorPtr = (LPBYTE) info + (WORD) info->bmiHeader.biSize;
    rgbTriples = colorPtr;
    rgbQuads = colorPtr;

    /* Transfer color info */

    switch (bpp)
    {
    case 0:  /* query bitmap info only */
        if (core_header)
        {
            BITMAPCOREHEADER* coreheader = (BITMAPCOREHEADER*) info;
            coreheader->bcWidth = bmp->bitmap.bmWidth;
            coreheader->bcHeight = bmp->bitmap.bmHeight;
            coreheader->bcPlanes = 1;
            coreheader->bcBitCount = bmp->bitmap.bmBitsPixel;
        }
        else
        {
            info->bmiHeader.biWidth = bmp->bitmap.bmWidth;
            info->bmiHeader.biHeight = bmp->bitmap.bmHeight;
            info->bmiHeader.biPlanes = 1;
            info->bmiHeader.biSizeImage =
                DIB_GetDIBImageBytes( bmp->bitmap.bmWidth,
                                      bmp->bitmap.bmHeight,
                                      bmp->bitmap.bmBitsPixel );
            if (bmp->dib)
            {
                info->bmiHeader.biBitCount = bmp->dib->dsBm.bmBitsPixel;
                switch (bmp->dib->dsBm.bmBitsPixel)
                {
                case 16:
                case 32:
                    info->bmiHeader.biCompression = BI_BITFIELDS;
                    break;
                default:
                    info->bmiHeader.biCompression = BI_RGB;
                    break;
                }
            }
            else
            {
                info->bmiHeader.biCompression = (bmp->bitmap.bmBitsPixel > 8) ? BI_BITFIELDS : BI_RGB;
                info->bmiHeader.biBitCount = bmp->bitmap.bmBitsPixel;
            }
            info->bmiHeader.biXPelsPerMeter = 0;
            info->bmiHeader.biYPelsPerMeter = 0;
            info->bmiHeader.biClrUsed = 0;
            info->bmiHeader.biClrImportant = 0;

            /* Windows 2000 doesn't touch the additional struct members if
               it's a BITMAPV4HEADER or a BITMAPV5HEADER */
        }
        lines = abs(bmp->bitmap.bmHeight);
        goto done;

    case 1:
    case 4:
    case 8:
        if (!core_header) info->bmiHeader.biClrUsed = 0;

	/* If the bitmap object already has a dib section at the
	   same color depth then get the color map from it */
	if (bmp->dib && bmp->dib->dsBm.bmBitsPixel == bpp) {
            if(coloruse == DIB_RGB_COLORS) {
                unsigned int colors = min( bmp->nb_colors, 1 << bpp );

                if (core_header)
                {
                    /* Convert the color table (RGBQUAD to RGBTRIPLE) */
                    RGBTRIPLE* index = rgbTriples;

                    for (i=0; i < colors; i++, index++)
                    {
                        index->rgbtRed   = bmp->color_table[i].rgbRed;
                        index->rgbtGreen = bmp->color_table[i].rgbGreen;
                        index->rgbtBlue  = bmp->color_table[i].rgbBlue;
                    }
                }
                else
                {
                    if (colors != 1 << bpp) info->bmiHeader.biClrUsed = colors;
                    memcpy(colorPtr, bmp->color_table, colors * sizeof(RGBQUAD));
                }
            }
            else {
                WORD *index = colorPtr;
                for(i = 0; i < 1 << info->bmiHeader.biBitCount; i++, index++)
                    *index = i;
            }
        }
        else {
            if (coloruse == DIB_PAL_COLORS) {
                for (i = 0; i < (1 << bpp); i++)
                    ((WORD *)colorPtr)[i] = (WORD)i;
            }
            else if(bpp > 1 && bpp == bmp->bitmap.bmBitsPixel) {
                /* For color DDBs in native depth (mono DDBs always have
                   a black/white palette):
                   Generate the color map from the selected palette */
                PALETTEENTRY palEntry[256];

                memset( palEntry, 0, sizeof(palEntry) );
                if (!GetPaletteEntries( dc->hPalette, 0, 1 << bmp->bitmap.bmBitsPixel, palEntry ))
                {
                    release_dc_ptr( dc );
                    GDI_ReleaseObj( hbitmap );
                    return 0;
                }
                for (i = 0; i < (1 << bmp->bitmap.bmBitsPixel); i++) {
                    if (core_header)
                    {
                        rgbTriples[i].rgbtRed   = palEntry[i].peRed;
                        rgbTriples[i].rgbtGreen = palEntry[i].peGreen;
                        rgbTriples[i].rgbtBlue  = palEntry[i].peBlue;
                    }
                    else
                    {
                        rgbQuads[i].rgbRed      = palEntry[i].peRed;
                        rgbQuads[i].rgbGreen    = palEntry[i].peGreen;
                        rgbQuads[i].rgbBlue     = palEntry[i].peBlue;
                        rgbQuads[i].rgbReserved = 0;
                    }
                }
            } else {
                switch (bpp) {
                case 1:
                    if (core_header)
                    {
                        rgbTriples[0].rgbtRed = rgbTriples[0].rgbtGreen =
                            rgbTriples[0].rgbtBlue = 0;
                        rgbTriples[1].rgbtRed = rgbTriples[1].rgbtGreen =
                            rgbTriples[1].rgbtBlue = 0xff;
                    }
                    else
                    {    
                        rgbQuads[0].rgbRed = rgbQuads[0].rgbGreen =
                            rgbQuads[0].rgbBlue = 0;
                        rgbQuads[0].rgbReserved = 0;
                        rgbQuads[1].rgbRed = rgbQuads[1].rgbGreen =
                            rgbQuads[1].rgbBlue = 0xff;
                        rgbQuads[1].rgbReserved = 0;
                    }
                    break;

                case 4:
                    if (core_header)
                        memcpy(colorPtr, EGAColorsTriples, sizeof(EGAColorsTriples));
                    else
                        memcpy(colorPtr, EGAColorsQuads, sizeof(EGAColorsQuads));

                    break;

                case 8:
                    {
                        if (core_header)
                        {
                            INT r, g, b;
                            RGBTRIPLE *color;

                            memcpy(rgbTriples, DefLogPaletteTriples,
                                       10 * sizeof(RGBTRIPLE));
                            memcpy(rgbTriples + 246, DefLogPaletteTriples + 10,
                                       10 * sizeof(RGBTRIPLE));
                            color = rgbTriples + 10;
                            for(r = 0; r <= 5; r++) /* FIXME */
                                for(g = 0; g <= 5; g++)
                                    for(b = 0; b <= 5; b++) {
                                        color->rgbtRed =   (r * 0xff) / 5;
                                        color->rgbtGreen = (g * 0xff) / 5;
                                        color->rgbtBlue =  (b * 0xff) / 5;
                                        color++;
                                    }
                        }
                        else
                        {
                            INT r, g, b;
                            RGBQUAD *color;

                            memcpy(rgbQuads, DefLogPaletteQuads,
                                       10 * sizeof(RGBQUAD));
                            memcpy(rgbQuads + 246, DefLogPaletteQuads + 10,
                                   10 * sizeof(RGBQUAD));
                            color = rgbQuads + 10;
                            for(r = 0; r <= 5; r++) /* FIXME */
                                for(g = 0; g <= 5; g++)
                                    for(b = 0; b <= 5; b++) {
                                        color->rgbRed =   (r * 0xff) / 5;
                                        color->rgbGreen = (g * 0xff) / 5;
                                        color->rgbBlue =  (b * 0xff) / 5;
                                        color->rgbReserved = 0;
                                        color++;
                                    }
                        }
                    }
                }
            }
        }
        break;

    case 15:
        if (info->bmiHeader.biCompression == BI_BITFIELDS)
        {
            ((PDWORD)info->bmiColors)[0] = 0x7c00;
            ((PDWORD)info->bmiColors)[1] = 0x03e0;
            ((PDWORD)info->bmiColors)[2] = 0x001f;
        }
        break;

    case 16:
        if (info->bmiHeader.biCompression == BI_BITFIELDS)
        {
            if (bmp->dib) memcpy( info->bmiColors, bmp->dib->dsBitfields, 3 * sizeof(DWORD) );
            else
            {
                ((PDWORD)info->bmiColors)[0] = 0xf800;
                ((PDWORD)info->bmiColors)[1] = 0x07e0;
                ((PDWORD)info->bmiColors)[2] = 0x001f;
            }
        }
        break;

    case 24:
    case 32:
        if (info->bmiHeader.biCompression == BI_BITFIELDS)
        {
            if (bmp->dib) memcpy( info->bmiColors, bmp->dib->dsBitfields, 3 * sizeof(DWORD) );
            else
            {
                ((PDWORD)info->bmiColors)[0] = 0xff0000;
                ((PDWORD)info->bmiColors)[1] = 0x00ff00;
                ((PDWORD)info->bmiColors)[2] = 0x0000ff;
            }
        }
        break;
    }

    if (bits && lines)
    {
        /* If the bitmap object already have a dib section that contains image data, get the bits from it */
        if(bmp->dib && bmp->dib->dsBm.bmBitsPixel >= 15 && bpp >= 15)
        {
            /*FIXME: Only RGB dibs supported for now */
            unsigned int srcwidth = bmp->dib->dsBm.bmWidth, srcwidthb = bmp->dib->dsBm.bmWidthBytes;
            unsigned int dstwidth = width;
            int dstwidthb = DIB_GetDIBWidthBytes( width, bpp );
            LPBYTE dbits = bits, sbits = (LPBYTE) bmp->dib->dsBm.bmBits + (startscan * srcwidthb);
            unsigned int x, y, width, widthb;

            if ((height < 0) ^ (bmp->dib->dsBmih.biHeight < 0))
            {
                dbits = (LPBYTE)bits + (dstwidthb * (lines-1));
                dstwidthb = -dstwidthb;
            }

            switch( bpp ) {

	    case 15:
            case 16: /* 16 bpp dstDIB */
                {
                    LPWORD dstbits = (LPWORD)dbits;
                    WORD rmask = 0x7c00, gmask= 0x03e0, bmask = 0x001f;

                    /* FIXME: BI_BITFIELDS not supported yet */

                    switch(bmp->dib->dsBm.bmBitsPixel) {

                    case 16: /* 16 bpp srcDIB -> 16 bpp dstDIB */
                        {
                            widthb = min(srcwidthb, abs(dstwidthb));
                            /* FIXME: BI_BITFIELDS not supported yet */
                            for (y = 0; y < lines; y++, dbits+=dstwidthb, sbits+=srcwidthb)
                                memcpy(dbits, sbits, widthb);
                        }
                        break;

                    case 24: /* 24 bpp srcDIB -> 16 bpp dstDIB */
                        {
                            LPBYTE srcbits = sbits;

                            width = min(srcwidth, dstwidth);
                            for( y = 0; y < lines; y++) {
                                for( x = 0; x < width; x++, srcbits += 3)
                                    *dstbits++ = ((srcbits[0] >> 3) & bmask) |
                                                 (((WORD)srcbits[1] << 2) & gmask) |
                                                 (((WORD)srcbits[2] << 7) & rmask);

                                dstbits = (LPWORD)(dbits+=dstwidthb);
                                srcbits = (sbits += srcwidthb);
                            }
                        }
                        break;

                    case 32: /* 32 bpp srcDIB -> 16 bpp dstDIB */
                        {
                            LPDWORD srcbits = (LPDWORD)sbits;
                            DWORD val;

                            width = min(srcwidth, dstwidth);
                            for( y = 0; y < lines; y++) {
                                for( x = 0; x < width; x++ ) {
                                    val = *srcbits++;
                                    *dstbits++ = (WORD)(((val >> 3) & bmask) | ((val >> 6) & gmask) |
                                                       ((val >> 9) & rmask));
                                }
                                dstbits = (LPWORD)(dbits+=dstwidthb);
                                srcbits = (LPDWORD)(sbits+=srcwidthb);
                            }
                        }
                        break;

                    default: /* ? bit bmp -> 16 bit DIB */
                        FIXME("15/16 bit DIB %d bit bitmap\n",
                        bmp->bitmap.bmBitsPixel);
                        break;
                    }
                }
                break;

            case 24: /* 24 bpp dstDIB */
                {
                    LPBYTE dstbits = dbits;

                    switch(bmp->dib->dsBm.bmBitsPixel) {

                    case 16: /* 16 bpp srcDIB -> 24 bpp dstDIB */
                        {
                            LPWORD srcbits = (LPWORD)sbits;
                            WORD val;

                            width = min(srcwidth, dstwidth);
                            /* FIXME: BI_BITFIELDS not supported yet */
                            for( y = 0; y < lines; y++) {
                                for( x = 0; x < width; x++ ) {
                                    val = *srcbits++;
                                    *dstbits++ = (BYTE)(((val << 3) & 0xf8) | ((val >> 2) & 0x07));
                                    *dstbits++ = (BYTE)(((val >> 2) & 0xf8) | ((val >> 7) & 0x07));
                                    *dstbits++ = (BYTE)(((val >> 7) & 0xf8) | ((val >> 12) & 0x07));
                                }
                                dstbits = dbits+=dstwidthb;
                                srcbits = (LPWORD)(sbits+=srcwidthb);
                            }
                        }
                        break;

                    case 24: /* 24 bpp srcDIB -> 24 bpp dstDIB */
                        {
                            widthb = min(srcwidthb, abs(dstwidthb));
                            for (y = 0; y < lines; y++, dbits+=dstwidthb, sbits+=srcwidthb)
                                memcpy(dbits, sbits, widthb);
                        }
                        break;

                    case 32: /* 32 bpp srcDIB -> 24 bpp dstDIB */
                        {
                            LPBYTE srcbits = sbits;

                            width = min(srcwidth, dstwidth);
                            for( y = 0; y < lines; y++) {
                                for( x = 0; x < width; x++, srcbits++ ) {
                                    *dstbits++ = *srcbits++;
                                    *dstbits++ = *srcbits++;
                                    *dstbits++ = *srcbits++;
                                }
                                dstbits = dbits+=dstwidthb;
                                srcbits = sbits+=srcwidthb;
                            }
                        }
                        break;

                    default: /* ? bit bmp -> 24 bit DIB */
                        FIXME("24 bit DIB %d bit bitmap\n",
                              bmp->bitmap.bmBitsPixel);
                        break;
                    }
                }
                break;

            case 32: /* 32 bpp dstDIB */
                {
                    LPDWORD dstbits = (LPDWORD)dbits;

                    /* FIXME: BI_BITFIELDS not supported yet */

                    switch(bmp->dib->dsBm.bmBitsPixel) {
                        case 16: /* 16 bpp srcDIB -> 32 bpp dstDIB */
                        {
                            LPWORD srcbits = (LPWORD)sbits;
                            DWORD val;

                            width = min(srcwidth, dstwidth);
                            /* FIXME: BI_BITFIELDS not supported yet */
                            for( y = 0; y < lines; y++) {
                                for( x = 0; x < width; x++ ) {
                                    val = (DWORD)*srcbits++;
                                    *dstbits++ = ((val << 3) & 0xf8) | ((val >> 2) & 0x07) |
                                                 ((val << 6) & 0xf800) | ((val << 1) & 0x0700) |
                                                 ((val << 9) & 0xf80000) | ((val << 4) & 0x070000);
                                }
                                dstbits=(LPDWORD)(dbits+=dstwidthb);
                                srcbits=(LPWORD)(sbits+=srcwidthb);
                            }
                        }
                        break;

                    case 24: /* 24 bpp srcDIB -> 32 bpp dstDIB */
                        {
                            LPBYTE srcbits = sbits;

                            width = min(srcwidth, dstwidth);
                            for( y = 0; y < lines; y++) {
                                for( x = 0; x < width; x++, srcbits+=3 )
                                    *dstbits++ =  srcbits[0] |
                                                 (srcbits[1] <<  8) |
                                                 (srcbits[2] << 16);
                                dstbits=(LPDWORD)(dbits+=dstwidthb);
                                srcbits=(sbits+=srcwidthb);
                            }
                        }
                        break;

                    case 32: /* 32 bpp srcDIB -> 32 bpp dstDIB */
                        {
                            widthb = min(srcwidthb, abs(dstwidthb));
                            /* FIXME: BI_BITFIELDS not supported yet */
                            for (y = 0; y < lines; y++, dbits+=dstwidthb, sbits+=srcwidthb) {
                                memcpy(dbits, sbits, widthb);
                            }
                        }
                        break;

                    default: /* ? bit bmp -> 32 bit DIB */
                        FIXME("32 bit DIB %d bit bitmap\n",
                        bmp->bitmap.bmBitsPixel);
                        break;
                    }
                }
                break;

            default: /* ? bit DIB */
                FIXME("Unsupported DIB depth %d\n", info->bmiHeader.biBitCount);
                break;
            }
        }
        /* Otherwise, get bits from the XImage */
        else
        {
            if (!bmp->funcs && !BITMAP_SetOwnerDC( hbitmap, dc )) lines = 0;
            else
            {
                if (bmp->funcs && bmp->funcs->pGetDIBits)
                    lines = bmp->funcs->pGetDIBits( dc->physDev, hbitmap, startscan,
                                                    lines, bits, info, coloruse );
                else
                    lines = 0;  /* FIXME: should copy from bmp->bitmap.bmBits */
            }
        }
    }
    else lines = abs(height);

    /* The knowledge base article Q81498 ("DIBs and Their Uses") states that
       if bits == NULL and bpp != 0, only biSizeImage and the color table are
       filled in. */
    if (!core_header)
    {
        /* FIXME: biSizeImage should be calculated according to the selected
           compression algorithm if biCompression != BI_RGB */
        info->bmiHeader.biSizeImage = DIB_GetDIBImageBytes( width, height, bpp );
        TRACE("biSizeImage = %d, ", info->bmiHeader.biSizeImage);
    }
    TRACE("biWidth = %d, biHeight = %d\n", width, height);

done:
    release_dc_ptr( dc );
    GDI_ReleaseObj( hbitmap );
    return lines;
}


/***********************************************************************
 *           CreateDIBitmap    (GDI32.@)
 *
 * Creates a DDB (device dependent bitmap) from a DIB.
 * The DDB will have the same color depth as the reference DC.
 */
HBITMAP WINAPI CreateDIBitmap( HDC hdc, const BITMAPINFOHEADER *header,
                            DWORD init, LPCVOID bits, const BITMAPINFO *data,
                            UINT coloruse )
{
    HBITMAP handle;
    LONG width;
    LONG height;
    WORD planes, bpp;
    DWORD compr, size;
    DC *dc;

    if (!header) return 0;

    if (DIB_GetBitmapInfo( header, &width, &height, &planes, &bpp, &compr, &size ) == -1) return 0;
    
    if (width < 0)
    {
        TRACE("Bitmap has a negative width\n");
        return 0;
    }
    
    /* Top-down DIBs have a negative height */
    if (height < 0) height = -height;

    TRACE("hdc=%p, header=%p, init=%u, bits=%p, data=%p, coloruse=%u (bitmap: width=%d, height=%d, bpp=%u, compr=%u)\n",
           hdc, header, init, bits, data, coloruse, width, height, bpp, compr);
    
    if (hdc == NULL)
        handle = CreateBitmap( width, height, 1, 1, NULL );
    else
        handle = CreateCompatibleBitmap( hdc, width, height );

    if (handle)
    {
        if (init & CBM_INIT)
        {
            if (SetDIBits( hdc, handle, 0, height, bits, data, coloruse ) == 0)
            {
                DeleteObject( handle );
                handle = 0;
            }
        }

        else if (hdc && ((dc = get_dc_ptr( hdc )) != NULL) )
        {
            if (!BITMAP_SetOwnerDC( handle, dc ))
            {
                DeleteObject( handle );
                handle = 0;
            }
            release_dc_ptr( dc );
        }
    }

    return handle;
}

/* Copy/synthesize RGB palette from BITMAPINFO. Ripped from dlls/winex11.drv/dib.c */
static void DIB_CopyColorTable( DC *dc, BITMAPOBJ *bmp, WORD coloruse, const BITMAPINFO *info )
{
    RGBQUAD *colorTable;
    unsigned int colors, i;
    BOOL core_info = info->bmiHeader.biSize == sizeof(BITMAPCOREHEADER);

    if (core_info)
    {
        colors = 1 << ((const BITMAPCOREINFO*) info)->bmciHeader.bcBitCount;
    }
    else
    {
        colors = info->bmiHeader.biClrUsed;
        if (!colors) colors = 1 << info->bmiHeader.biBitCount;
    }

    if (colors > 256) {
        ERR("called with >256 colors!\n");
        return;
    }

    if (!(colorTable = HeapAlloc(GetProcessHeap(), 0, colors * sizeof(RGBQUAD) ))) return;

    if(coloruse == DIB_RGB_COLORS)
    {
        if (core_info)
        {
           /* Convert RGBTRIPLEs to RGBQUADs */
           for (i=0; i < colors; i++)
           {
               colorTable[i].rgbRed   = ((const BITMAPCOREINFO*) info)->bmciColors[i].rgbtRed;
               colorTable[i].rgbGreen = ((const BITMAPCOREINFO*) info)->bmciColors[i].rgbtGreen;
               colorTable[i].rgbBlue  = ((const BITMAPCOREINFO*) info)->bmciColors[i].rgbtBlue;
               colorTable[i].rgbReserved = 0;
           }
        }
        else
        {
            memcpy(colorTable, (const BYTE*) info + (WORD) info->bmiHeader.biSize, colors * sizeof(RGBQUAD));
        }
    }
    else
    {
        PALETTEENTRY entries[256];
        const WORD *index = (const WORD*) ((const BYTE*) info + (WORD) info->bmiHeader.biSize);
        UINT count = GetPaletteEntries( dc->hPalette, 0, colors, entries );

        for (i = 0; i < colors; i++, index++)
        {
            PALETTEENTRY *entry = &entries[*index % count];
            colorTable[i].rgbRed = entry->peRed;
            colorTable[i].rgbGreen = entry->peGreen;
            colorTable[i].rgbBlue = entry->peBlue;
            colorTable[i].rgbReserved = 0;
        }
    }
    bmp->color_table = colorTable;
    bmp->nb_colors = colors;
}

/***********************************************************************
 *           CreateDIBSection    (GDI32.@)
 */
HBITMAP WINAPI CreateDIBSection(HDC hdc, CONST BITMAPINFO *bmi, UINT usage,
                                VOID **bits, HANDLE section, DWORD offset)
{
    HBITMAP ret = 0;
    DC *dc;
    BOOL bDesktopDC = FALSE;
    DIBSECTION *dib;
    BITMAPOBJ *bmp;
    int bitmap_type;
    LONG width, height;
    WORD planes, bpp;
    DWORD compression, sizeImage;
    void *mapBits = NULL;

    if(!bmi){
        if(bits) *bits = NULL;
        return NULL;
    }

    if (((bitmap_type = DIB_GetBitmapInfo( &bmi->bmiHeader, &width, &height,
                                           &planes, &bpp, &compression, &sizeImage )) == -1))
        return 0;

    switch (bpp)
    {
    case 16:
    case 32:
        if (compression == BI_BITFIELDS) break;
        /* fall through */
    case 1:
    case 4:
    case 8:
    case 24:
        if (compression == BI_RGB) break;
        /* fall through */
    default:
        WARN( "invalid %u bpp compression %u\n", bpp, compression );
        return 0;
    }

    if (!(dib = HeapAlloc( GetProcessHeap(), 0, sizeof(*dib) ))) return 0;

    TRACE("format (%d,%d), planes %d, bpp %d, size %d, %s\n",
          width, height, planes, bpp, sizeImage, usage == DIB_PAL_COLORS? "PAL" : "RGB");

    dib->dsBm.bmType       = 0;
    dib->dsBm.bmWidth      = width;
    dib->dsBm.bmHeight     = height >= 0 ? height : -height;
    dib->dsBm.bmWidthBytes = DIB_GetDIBWidthBytes(width, bpp);
    dib->dsBm.bmPlanes     = planes;
    dib->dsBm.bmBitsPixel  = bpp;
    dib->dsBm.bmBits       = NULL;

    if (!bitmap_type)  /* core header */
    {
        /* convert the BITMAPCOREHEADER to a BITMAPINFOHEADER */
        dib->dsBmih.biSize = sizeof(BITMAPINFOHEADER);
        dib->dsBmih.biWidth = width;
        dib->dsBmih.biHeight = height;
        dib->dsBmih.biPlanes = planes;
        dib->dsBmih.biBitCount = bpp;
        dib->dsBmih.biCompression = compression;
        dib->dsBmih.biXPelsPerMeter = 0;
        dib->dsBmih.biYPelsPerMeter = 0;
        dib->dsBmih.biClrUsed = 0;
        dib->dsBmih.biClrImportant = 0;
    }
    else
    {
        /* truncate extended bitmap headers (BITMAPV4HEADER etc.) */
        dib->dsBmih = bmi->bmiHeader;
        dib->dsBmih.biSize = sizeof(BITMAPINFOHEADER);
    }

    /* set number of entries in bmi.bmiColors table */
    if( bpp <= 8 )
        dib->dsBmih.biClrUsed = 1 << bpp;

    dib->dsBmih.biSizeImage = dib->dsBm.bmWidthBytes * dib->dsBm.bmHeight;

    /* set dsBitfields values */
    if (usage == DIB_PAL_COLORS || bpp <= 8)
    {
        dib->dsBitfields[0] = dib->dsBitfields[1] = dib->dsBitfields[2] = 0;
    }
    else switch( bpp )
    {
    case 15:
    case 16:
        dib->dsBitfields[0] = (compression == BI_BITFIELDS) ? *(const DWORD *)bmi->bmiColors       : 0x7c00;
        dib->dsBitfields[1] = (compression == BI_BITFIELDS) ? *((const DWORD *)bmi->bmiColors + 1) : 0x03e0;
        dib->dsBitfields[2] = (compression == BI_BITFIELDS) ? *((const DWORD *)bmi->bmiColors + 2) : 0x001f;
        break;
    case 24:
    case 32:
        dib->dsBitfields[0] = (compression == BI_BITFIELDS) ? *(const DWORD *)bmi->bmiColors       : 0xff0000;
        dib->dsBitfields[1] = (compression == BI_BITFIELDS) ? *((const DWORD *)bmi->bmiColors + 1) : 0x00ff00;
        dib->dsBitfields[2] = (compression == BI_BITFIELDS) ? *((const DWORD *)bmi->bmiColors + 2) : 0x0000ff;
        break;
    }

    /* get storage location for DIB bits */

    if (section)
    {
        SYSTEM_INFO SystemInfo;
        DWORD mapOffset;
        INT mapSize;

        GetSystemInfo( &SystemInfo );
        mapOffset = offset - (offset % SystemInfo.dwAllocationGranularity);
        mapSize = dib->dsBmih.biSizeImage + (offset - mapOffset);
        mapBits = MapViewOfFile( section, FILE_MAP_ALL_ACCESS, 0, mapOffset, mapSize );
        if (mapBits) dib->dsBm.bmBits = (char *)mapBits + (offset - mapOffset);
    }
    else
    {
        offset = 0;
        dib->dsBm.bmBits = VirtualAlloc( NULL, dib->dsBmih.biSizeImage,
                                         MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE );
    }
    dib->dshSection = section;
    dib->dsOffset = offset;

    if (!dib->dsBm.bmBits)
    {
        HeapFree( GetProcessHeap(), 0, dib );
        return 0;
    }

    /* If the reference hdc is null, take the desktop dc */
    if (hdc == 0)
    {
        hdc = CreateCompatibleDC(0);
        bDesktopDC = TRUE;
    }

    if (!(dc = get_dc_ptr( hdc ))) goto error;

    /* create Device Dependent Bitmap and add DIB pointer */
    ret = CreateBitmap( dib->dsBm.bmWidth, dib->dsBm.bmHeight, 1,
                        (bpp == 1) ? 1 : GetDeviceCaps(hdc, BITSPIXEL), NULL );

    if (ret && ((bmp = GDI_GetObjPtr(ret, OBJ_BITMAP))))
    {
        bmp->dib = dib;
        bmp->funcs = dc->funcs;
        /* create local copy of DIB palette */
        if (bpp <= 8) DIB_CopyColorTable( dc, bmp, usage, bmi );
        GDI_ReleaseObj( ret );

        if (dc->funcs->pCreateDIBSection)
        {
            if (!dc->funcs->pCreateDIBSection(dc->physDev, ret, bmi, usage))
            {
                DeleteObject( ret );
                ret = 0;
            }
        }
    }

    release_dc_ptr( dc );
    if (bDesktopDC) DeleteDC( hdc );
    if (ret && bits) *bits = dib->dsBm.bmBits;
    return ret;

error:
    if (bDesktopDC) DeleteDC( hdc );
    if (section) UnmapViewOfFile( mapBits );
    else if (!offset) VirtualFree( dib->dsBm.bmBits, 0, MEM_RELEASE );
    HeapFree( GetProcessHeap(), 0, dib );
    return 0;
}
