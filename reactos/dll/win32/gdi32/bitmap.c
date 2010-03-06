/*
 * GDI bitmap objects
 *
 * Copyright 1993 Alexandre Julliard
 *           1998 Huw D M Davies
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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "gdi_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(bitmap);


static HGDIOBJ BITMAP_SelectObject( HGDIOBJ handle, HDC hdc );
static INT BITMAP_GetObject( HGDIOBJ handle, INT count, LPVOID buffer );
static BOOL BITMAP_DeleteObject( HGDIOBJ handle );

static const struct gdi_obj_funcs bitmap_funcs =
{
    BITMAP_SelectObject,  /* pSelectObject */
    BITMAP_GetObject,     /* pGetObjectA */
    BITMAP_GetObject,     /* pGetObjectW */
    NULL,                 /* pUnrealizeObject */
    BITMAP_DeleteObject   /* pDeleteObject */
};

/***********************************************************************
 *           BITMAP_GetWidthBytes
 *
 * Return number of bytes taken by a scanline of 16-bit aligned Windows DDB
 * data.
 */
INT BITMAP_GetWidthBytes( INT bmWidth, INT bpp )
{
    switch(bpp)
    {
    case 1:
	return 2 * ((bmWidth+15) >> 4);

    case 24:
	bmWidth *= 3; /* fall through */
    case 8:
	return bmWidth + (bmWidth & 1);

    case 32:
	return bmWidth * 4;

    case 16:
    case 15:
	return bmWidth * 2;

    case 4:
	return 2 * ((bmWidth+3) >> 2);

    default:
	WARN("Unknown depth %d, please report.\n", bpp );
    }
    return -1;
}


/******************************************************************************
 * CreateBitmap [GDI32.@]
 *
 * Creates a bitmap with the specified info.
 *
 * PARAMS
 *    width  [I] bitmap width
 *    height [I] bitmap height
 *    planes [I] Number of color planes
 *    bpp    [I] Number of bits to identify a color
 *    bits   [I] Pointer to array containing color data
 *
 * RETURNS
 *    Success: Handle to bitmap
 *    Failure: 0
 */
HBITMAP WINAPI CreateBitmap( INT width, INT height, UINT planes,
                             UINT bpp, LPCVOID bits )
{
    BITMAP bm;

    bm.bmType = 0;
    bm.bmWidth = width;
    bm.bmHeight = height;
    bm.bmWidthBytes = BITMAP_GetWidthBytes( width, bpp );
    bm.bmPlanes = planes;
    bm.bmBitsPixel = bpp;
    bm.bmBits = (LPVOID)bits;

    return CreateBitmapIndirect( &bm );
}

/******************************************************************************
 * CreateCompatibleBitmap [GDI32.@]
 *
 * Creates a bitmap compatible with the DC.
 *
 * PARAMS
 *    hdc    [I] Handle to device context
 *    width  [I] Width of bitmap
 *    height [I] Height of bitmap
 *
 * RETURNS
 *    Success: Handle to bitmap
 *    Failure: 0
 */
HBITMAP WINAPI CreateCompatibleBitmap( HDC hdc, INT width, INT height)
{
    HBITMAP hbmpRet = 0;

    TRACE("(%p,%d,%d) =\n", hdc, width, height);

    if (GetObjectType( hdc ) != OBJ_MEMDC)
    {
        hbmpRet = CreateBitmap(width, height,
                               GetDeviceCaps(hdc, PLANES),
                               GetDeviceCaps(hdc, BITSPIXEL),
                               NULL);
    }
    else  /* Memory DC */
    {
        DIBSECTION dib;
        HBITMAP bitmap = GetCurrentObject( hdc, OBJ_BITMAP );
        INT size = GetObjectW( bitmap, sizeof(dib), &dib );

        if (!size) return 0;

        if (size == sizeof(BITMAP))
        {
            /* A device-dependent bitmap is selected in the DC */
            hbmpRet = CreateBitmap(width, height,
                                   dib.dsBm.bmPlanes,
                                   dib.dsBm.bmBitsPixel,
                                   NULL);
        }
        else
        {
            /* A DIB section is selected in the DC */
            BITMAPINFO *bi;
            void *bits;

            /* Allocate memory for a BITMAPINFOHEADER structure and a
               color table. The maximum number of colors in a color table
               is 256 which corresponds to a bitmap with depth 8.
               Bitmaps with higher depths don't have color tables. */
            bi = HeapAlloc(GetProcessHeap(), 0, sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));

            if (bi)
            {
                bi->bmiHeader.biSize          = sizeof(bi->bmiHeader);
                bi->bmiHeader.biWidth         = width;
                bi->bmiHeader.biHeight        = height;
                bi->bmiHeader.biPlanes        = dib.dsBmih.biPlanes;
                bi->bmiHeader.biBitCount      = dib.dsBmih.biBitCount;
                bi->bmiHeader.biCompression   = dib.dsBmih.biCompression;
                bi->bmiHeader.biSizeImage     = 0;
                bi->bmiHeader.biXPelsPerMeter = dib.dsBmih.biXPelsPerMeter;
                bi->bmiHeader.biYPelsPerMeter = dib.dsBmih.biYPelsPerMeter;
                bi->bmiHeader.biClrUsed       = dib.dsBmih.biClrUsed;
                bi->bmiHeader.biClrImportant  = dib.dsBmih.biClrImportant;

                if (bi->bmiHeader.biCompression == BI_BITFIELDS)
                {
                    /* Copy the color masks */
                    CopyMemory(bi->bmiColors, dib.dsBitfields, 3 * sizeof(DWORD));
                }
                else if (bi->bmiHeader.biBitCount <= 8)
                {
                    /* Copy the color table */
                    GetDIBColorTable(hdc, 0, 256, bi->bmiColors);
                }

                hbmpRet = CreateDIBSection(hdc, bi, DIB_RGB_COLORS, &bits, NULL, 0);
                HeapFree(GetProcessHeap(), 0, bi);
            }
        }
    }

    TRACE("\t\t%p\n", hbmpRet);
    return hbmpRet;
}


/******************************************************************************
 * CreateBitmapIndirect [GDI32.@]
 *
 * Creates a bitmap with the specified info.
 *
 * PARAMS
 *  bmp [I] Pointer to the bitmap info describing the bitmap
 *
 * RETURNS
 *    Success: Handle to bitmap
 *    Failure: NULL. Use GetLastError() to determine the cause.
 *
 * NOTES
 *  If a width or height of 0 are given, a 1x1 monochrome bitmap is returned.
 */
HBITMAP WINAPI CreateBitmapIndirect( const BITMAP *bmp )
{
    BITMAP bm;
    BITMAPOBJ *bmpobj;
    HBITMAP hbitmap;

    if (!bmp || bmp->bmType)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }

    if (bmp->bmWidth > 0x7ffffff || bmp->bmHeight > 0x7ffffff)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    bm = *bmp;

    if (!bm.bmWidth || !bm.bmHeight)
    {
        return GetStockObject( DEFAULT_BITMAP );
    }
    else
    {
        if (bm.bmHeight < 0)
            bm.bmHeight = -bm.bmHeight;
        if (bm.bmWidth < 0)
            bm.bmWidth = -bm.bmWidth;
    }

    if (bm.bmPlanes != 1)
    {
        FIXME("planes = %d\n", bm.bmPlanes);
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }

    /* Windows only uses 1, 4, 8, 16, 24 and 32 bpp */
    if(bm.bmBitsPixel == 1)         bm.bmBitsPixel = 1;
    else if(bm.bmBitsPixel <= 4)    bm.bmBitsPixel = 4;
    else if(bm.bmBitsPixel <= 8)    bm.bmBitsPixel = 8;
    else if(bm.bmBitsPixel <= 16)   bm.bmBitsPixel = 16;
    else if(bm.bmBitsPixel <= 24)   bm.bmBitsPixel = 24;
    else if(bm.bmBitsPixel <= 32)   bm.bmBitsPixel = 32;
    else {
        WARN("Invalid bmBitsPixel %d, returning ERROR_INVALID_PARAMETER\n", bm.bmBitsPixel);
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    /* Windows ignores the provided bm.bmWidthBytes */
    bm.bmWidthBytes = BITMAP_GetWidthBytes( bm.bmWidth, bm.bmBitsPixel );
    /* XP doesn't allow to create bitmaps larger than 128 Mb */
    if (bm.bmHeight > 128 * 1024 * 1024 / bm.bmWidthBytes)
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return 0;
    }

    /* Create the BITMAPOBJ */
    if (!(bmpobj = HeapAlloc( GetProcessHeap(), 0, sizeof(*bmpobj) )))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return 0;
    }

    bmpobj->size.cx = 0;
    bmpobj->size.cy = 0;
    bmpobj->bitmap = bm;
    bmpobj->bitmap.bmBits = NULL;
    bmpobj->funcs = NULL;
    bmpobj->dib = NULL;
    bmpobj->color_table = NULL;
    bmpobj->nb_colors = 0;

    if (!(hbitmap = alloc_gdi_handle( &bmpobj->header, OBJ_BITMAP, &bitmap_funcs )))
    {
        HeapFree( GetProcessHeap(), 0, bmpobj );
        return 0;
    }

    if (bm.bmBits)
        SetBitmapBits( hbitmap, bm.bmHeight * bm.bmWidthBytes, bm.bmBits );

    TRACE("%dx%d, %d colors returning %p\n", bm.bmWidth, bm.bmHeight,
          1 << (bm.bmPlanes * bm.bmBitsPixel), hbitmap);

    return hbitmap;
}


/***********************************************************************
 * GetBitmapBits [GDI32.@]
 *
 * Copies bitmap bits of bitmap to buffer.
 *
 * RETURNS
 *    Success: Number of bytes copied
 *    Failure: 0
 */
LONG WINAPI GetBitmapBits(
    HBITMAP hbitmap, /* [in]  Handle to bitmap */
    LONG count,        /* [in]  Number of bytes to copy */
    LPVOID bits)       /* [out] Pointer to buffer to receive bits */
{
    BITMAPOBJ *bmp = GDI_GetObjPtr( hbitmap, OBJ_BITMAP );
    LONG height, ret;

    if (!bmp) return 0;

    if (bmp->dib)  /* simply copy the bits from the DIB */
    {
        DIBSECTION *dib = bmp->dib;
        const char *src = dib->dsBm.bmBits;
        INT width_bytes = BITMAP_GetWidthBytes(dib->dsBm.bmWidth, dib->dsBm.bmBitsPixel);
        LONG max = width_bytes * bmp->bitmap.bmHeight;

        if (!bits)
        {
            ret = max;
            goto done;
        }

        if (count > max) count = max;
        ret = count;

        /* GetBitmapBits returns not 32-bit aligned data */

        if (bmp->dib->dsBmih.biHeight >= 0)  /* not top-down, need to flip contents vertically */
        {
            src += dib->dsBm.bmWidthBytes * dib->dsBm.bmHeight;
            while (count > 0)
            {
                src -= dib->dsBm.bmWidthBytes;
                memcpy( bits, src, min( count, width_bytes ) );
                bits = (char *)bits + width_bytes;
                count -= width_bytes;
            }
        }
        else
        {
            while (count > 0)
            {
                memcpy( bits, src, min( count, width_bytes ) );
                src += dib->dsBm.bmWidthBytes;
                bits = (char *)bits + width_bytes;
                count -= width_bytes;
            }
        }
        goto done;
    }

    /* If the bits vector is null, the function should return the read size */
    if(bits == NULL)
    {
        ret = bmp->bitmap.bmWidthBytes * bmp->bitmap.bmHeight;
        goto done;
    }

    if (count < 0) {
	WARN("(%d): Negative number of bytes passed???\n", count );
	count = -count;
    }

    /* Only get entire lines */
    height = count / bmp->bitmap.bmWidthBytes;
    if (height > bmp->bitmap.bmHeight) height = bmp->bitmap.bmHeight;
    count = height * bmp->bitmap.bmWidthBytes;
    if (count == 0)
      {
	WARN("Less than one entire line requested\n");
        ret = 0;
        goto done;
      }


    TRACE("(%p, %d, %p) %dx%d %d colors fetched height: %d\n",
          hbitmap, count, bits, bmp->bitmap.bmWidth, bmp->bitmap.bmHeight,
          1 << bmp->bitmap.bmBitsPixel, height );

    if(bmp->funcs && bmp->funcs->pGetBitmapBits)
    {
        TRACE("Calling device specific BitmapBits\n");
        ret = bmp->funcs->pGetBitmapBits(hbitmap, bits, count);
    } else {

        if(!bmp->bitmap.bmBits) {
	    TRACE("Bitmap is empty\n");
	    memset(bits, 0, count);
	    ret = count;
	} else {
	    memcpy(bits, bmp->bitmap.bmBits, count);
	    ret = count;
	}

    }
 done:
    GDI_ReleaseObj( hbitmap );
    return ret;
}


/******************************************************************************
 * SetBitmapBits [GDI32.@]
 *
 * Sets bits of color data for a bitmap.
 *
 * RETURNS
 *    Success: Number of bytes used in setting the bitmap bits
 *    Failure: 0
 */
LONG WINAPI SetBitmapBits(
    HBITMAP hbitmap, /* [in] Handle to bitmap */
    LONG count,        /* [in] Number of bytes in bitmap array */
    LPCVOID bits)      /* [in] Address of array with bitmap bits */
{
    BITMAPOBJ *bmp = GDI_GetObjPtr( hbitmap, OBJ_BITMAP );
    LONG height, ret;

    if ((!bmp) || (!bits))
	return 0;

    if (count < 0) {
	WARN("(%d): Negative number of bytes passed???\n", count );
	count = -count;
    }

    if (bmp->dib)  /* simply copy the bits into the DIB */
    {
        DIBSECTION *dib = bmp->dib;
        char *dest = dib->dsBm.bmBits;
        LONG max = dib->dsBm.bmWidthBytes * dib->dsBm.bmHeight;
        if (count > max) count = max;
        ret = count;

        if (bmp->dib->dsBmih.biHeight >= 0)  /* not top-down, need to flip contents vertically */
        {
            dest += dib->dsBm.bmWidthBytes * dib->dsBm.bmHeight;
            while (count > 0)
            {
                dest -= dib->dsBm.bmWidthBytes;
                memcpy( dest, bits, min( count, dib->dsBm.bmWidthBytes ) );
                bits = (const char *)bits + dib->dsBm.bmWidthBytes;
                count -= dib->dsBm.bmWidthBytes;
            }
        }
        else memcpy( dest, bits, count );

        GDI_ReleaseObj( hbitmap );
        return ret;
    }

    /* Only get entire lines */
    height = count / bmp->bitmap.bmWidthBytes;
    if (height > bmp->bitmap.bmHeight) height = bmp->bitmap.bmHeight;
    count = height * bmp->bitmap.bmWidthBytes;

    TRACE("(%p, %d, %p) %dx%d %d colors fetched height: %d\n",
          hbitmap, count, bits, bmp->bitmap.bmWidth, bmp->bitmap.bmHeight,
          1 << bmp->bitmap.bmBitsPixel, height );

    if(bmp->funcs && bmp->funcs->pSetBitmapBits) {

        TRACE("Calling device specific BitmapBits\n");
        ret = bmp->funcs->pSetBitmapBits(hbitmap, bits, count);
    } else {

        if(!bmp->bitmap.bmBits) /* Alloc enough for entire bitmap */
	    bmp->bitmap.bmBits = HeapAlloc( GetProcessHeap(), 0, count );
	if(!bmp->bitmap.bmBits) {
	    WARN("Unable to allocate bit buffer\n");
	    ret = 0;
	} else {
	    memcpy(bmp->bitmap.bmBits, bits, count);
	    ret = count;
	}
    }

    GDI_ReleaseObj( hbitmap );
    return ret;
}

/**********************************************************************
 *		BITMAP_CopyBitmap
 *
 */
HBITMAP BITMAP_CopyBitmap(HBITMAP hbitmap)
{
    HBITMAP res = 0;
    BITMAP bm;

    if (!GetObjectW( hbitmap, sizeof(bm), &bm )) return 0;
    res = CreateBitmapIndirect(&bm);

    if(res) {
        char *buf = HeapAlloc( GetProcessHeap(), 0, bm.bmWidthBytes *
			       bm.bmHeight );
        GetBitmapBits (hbitmap, bm.bmWidthBytes * bm.bmHeight, buf);
	SetBitmapBits (res, bm.bmWidthBytes * bm.bmHeight, buf);
	HeapFree( GetProcessHeap(), 0, buf );
    }
    return res;
}


/***********************************************************************
 *           BITMAP_SetOwnerDC
 *
 * Set the type of DC that owns the bitmap. This is used when the
 * bitmap is selected into a device to initialize the bitmap function
 * table.
 */
BOOL BITMAP_SetOwnerDC( HBITMAP hbitmap, DC *dc )
{
    BITMAPOBJ *bitmap;
    BOOL ret;

    /* never set the owner of the stock bitmap since it can be selected in multiple DCs */
    if (hbitmap == GetStockObject(DEFAULT_BITMAP)) return TRUE;

    if (!(bitmap = GDI_GetObjPtr( hbitmap, OBJ_BITMAP ))) return FALSE;

    ret = TRUE;
    if (!bitmap->funcs)  /* not owned by a DC yet */
    {
        if (dc->funcs->pCreateBitmap) ret = dc->funcs->pCreateBitmap( dc->physDev, hbitmap,
                                                                      bitmap->bitmap.bmBits );
        if (ret) bitmap->funcs = dc->funcs;
    }
    else if (bitmap->funcs != dc->funcs)
    {
        FIXME( "Trying to select bitmap %p in different DC type\n", hbitmap );
        ret = FALSE;
    }
    GDI_ReleaseObj( hbitmap );
    return ret;
}


/***********************************************************************
 *           BITMAP_SelectObject
 */
static HGDIOBJ BITMAP_SelectObject( HGDIOBJ handle, HDC hdc )
{
    HGDIOBJ ret;
    BITMAPOBJ *bitmap;
    DC *dc;

    if (!(dc = get_dc_ptr( hdc ))) return 0;

    if (GetObjectType( hdc ) != OBJ_MEMDC)
    {
        ret = 0;
        goto done;
    }
    ret = dc->hBitmap;
    if (handle == dc->hBitmap) goto done;  /* nothing to do */

    if (!(bitmap = GDI_GetObjPtr( handle, OBJ_BITMAP )))
    {
        ret = 0;
        goto done;
    }

    if (bitmap->header.selcount && (handle != GetStockObject(DEFAULT_BITMAP)))
    {
        WARN( "Bitmap already selected in another DC\n" );
        GDI_ReleaseObj( handle );
        ret = 0;
        goto done;
    }

    if (!bitmap->funcs && !BITMAP_SetOwnerDC( handle, dc ))
    {
        GDI_ReleaseObj( handle );
        ret = 0;
        goto done;
    }

    if (dc->funcs->pSelectBitmap && !dc->funcs->pSelectBitmap( dc->physDev, handle ))
    {
        GDI_ReleaseObj( handle );
        ret = 0;
    }
    else
    {
        dc->hBitmap = handle;
        GDI_inc_ref_count( handle );
        dc->dirty = 0;
        SetRectRgn( dc->hVisRgn, 0, 0, bitmap->bitmap.bmWidth, bitmap->bitmap.bmHeight);
        GDI_ReleaseObj( handle );
        DC_InitDC( dc );
        GDI_dec_ref_count( ret );
    }

 done:
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           BITMAP_DeleteObject
 */
static BOOL BITMAP_DeleteObject( HGDIOBJ handle )
{
    const DC_FUNCTIONS *funcs;
    BITMAPOBJ *bmp = GDI_GetObjPtr( handle, OBJ_BITMAP );

    if (!bmp) return FALSE;
    funcs = bmp->funcs;
    GDI_ReleaseObj( handle );

    if (funcs && funcs->pDeleteBitmap) funcs->pDeleteBitmap( handle );

    if (!(bmp = free_gdi_handle( handle ))) return FALSE;

    HeapFree( GetProcessHeap(), 0, bmp->bitmap.bmBits );

    if (bmp->dib)
    {
        DIBSECTION *dib = bmp->dib;

        if (dib->dsBm.bmBits)
        {
            if (dib->dshSection)
            {
                SYSTEM_INFO SystemInfo;
                GetSystemInfo( &SystemInfo );
                UnmapViewOfFile( (char *)dib->dsBm.bmBits -
                                 (dib->dsOffset % SystemInfo.dwAllocationGranularity) );
            }
            else if (!dib->dsOffset)
                VirtualFree(dib->dsBm.bmBits, 0L, MEM_RELEASE );
        }
        HeapFree(GetProcessHeap(), 0, dib);
        HeapFree(GetProcessHeap(), 0, bmp->color_table);
    }
    return HeapFree( GetProcessHeap(), 0, bmp );
}


/***********************************************************************
 *           BITMAP_GetObject
 */
static INT BITMAP_GetObject( HGDIOBJ handle, INT count, LPVOID buffer )
{
    INT ret;
    BITMAPOBJ *bmp = GDI_GetObjPtr( handle, OBJ_BITMAP );

    if (!bmp) return 0;

    if (!buffer) ret = sizeof(BITMAP);
    else if (count < sizeof(BITMAP)) ret = 0;
    else if (bmp->dib)
    {
	if (count >= sizeof(DIBSECTION))
	{
            DIBSECTION *dib = buffer;
            *dib = *bmp->dib;
            dib->dsBmih.biHeight = abs( dib->dsBmih.biHeight );
            ret = sizeof(DIBSECTION);
	}
	else /* if (count >= sizeof(BITMAP)) */
        {
            DIBSECTION *dib = bmp->dib;
            memcpy( buffer, &dib->dsBm, sizeof(BITMAP) );
            ret = sizeof(BITMAP);
	}
    }
    else
    {
        memcpy( buffer, &bmp->bitmap, sizeof(BITMAP) );
        ((BITMAP *) buffer)->bmBits = NULL;
        ret = sizeof(BITMAP);
    }
    GDI_ReleaseObj( handle );
    return ret;
}


/******************************************************************************
 * CreateDiscardableBitmap [GDI32.@]
 *
 * Creates a discardable bitmap.
 *
 * RETURNS
 *    Success: Handle to bitmap
 *    Failure: NULL
 */
HBITMAP WINAPI CreateDiscardableBitmap(
    HDC hdc,    /* [in] Handle to device context */
    INT width,  /* [in] Bitmap width */
    INT height) /* [in] Bitmap height */
{
    return CreateCompatibleBitmap( hdc, width, height );
}


/******************************************************************************
 * GetBitmapDimensionEx [GDI32.@]
 *
 * Retrieves dimensions of a bitmap.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI GetBitmapDimensionEx(
    HBITMAP hbitmap, /* [in]  Handle to bitmap */
    LPSIZE size)     /* [out] Address of struct receiving dimensions */
{
    BITMAPOBJ * bmp = GDI_GetObjPtr( hbitmap, OBJ_BITMAP );
    if (!bmp) return FALSE;
    *size = bmp->size;
    GDI_ReleaseObj( hbitmap );
    return TRUE;
}


/******************************************************************************
 * SetBitmapDimensionEx [GDI32.@]
 *
 * Assigns dimensions to a bitmap.
 * MSDN says that this function will fail if hbitmap is a handle created by
 * CreateDIBSection, but that's not true on Windows 2000.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI SetBitmapDimensionEx(
    HBITMAP hbitmap, /* [in]  Handle to bitmap */
    INT x,           /* [in]  Bitmap width */
    INT y,           /* [in]  Bitmap height */
    LPSIZE prevSize) /* [out] Address of structure for orig dims */
{
    BITMAPOBJ * bmp = GDI_GetObjPtr( hbitmap, OBJ_BITMAP );
    if (!bmp) return FALSE;
    if (prevSize) *prevSize = bmp->size;
    bmp->size.cx = x;
    bmp->size.cy = y;
    GDI_ReleaseObj( hbitmap );
    return TRUE;
}
