/*
 * PROJECT:         ReactOS
 * LICENSE:         GNU LGPL by FSF v2.1 or any later
 * FILE:            dll/win32/winent.drv/bitblt.c
 * PURPOSE:         GDI driver stub for ReactOS/Windows
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 *                  Some code is taken from winex11.drv (c) Wine project
 */

/* INCLUDES ***************************************************************/

#include "winent.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(bitmap);

/* FUNCTIONS **************************************************************/

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
 *           DIB_GetBitmapInfo
 *
 * Get the info from a bitmap header.
 * Return 1 for INFOHEADER, 0 for COREHEADER, -1 for error.
 */
int DIB_GetBitmapInfo( const BITMAPINFOHEADER *header, LONG *width,
                              LONG *height, WORD *bpp, WORD *compr )
{
    WORD planes;
    DWORD size;

    return DIB_GetBitmapInfoEx( header, width, height, &planes, bpp, compr, &size);    
}

INT DIB_GetDIBWidthBytes(INT width, INT depth)
{
    return ((width * depth + 31) & ~31) >> 3;
}

INT CDECL RosDrv_SetDIBitsToDevice( PDC_ATTR pdcattr, INT xDest, INT yDest, DWORD cx,
                                    DWORD cy, INT xSrc, INT ySrc,
                                    UINT startscan, UINT lines, LPCVOID bits,
                                    const BITMAPINFO *info, UINT coloruse )
{
    BOOL top_down;
    LONG height, width;
    WORD infoBpp, compression;
    POINT pt;

    pt.x = xDest; pt.y = yDest;
    LPtoDP(pdcattr->hdc, &pt, 1);
    pt.x += pdcattr->dc_rect.left;
    pt.y += pdcattr->dc_rect.top;

    /* Perform extensive parameter checking */
    if (DIB_GetBitmapInfo( &info->bmiHeader, &width, &height,
        &infoBpp, &compression ) == -1)
        return 0;

    top_down = (height < 0);
    if (top_down) height = -height;

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

    return RosGdiSetDIBitsToDevice(pdcattr->hKernelDC, pt.x, pt.y, cx, cy,
        xSrc, ySrc, startscan, top_down ? -lines : lines, bits, info, coloruse);
}

INT CDECL RosDrv_SetDIBits( PDC_ATTR pdcattr, HBITMAP hbitmap, UINT startscan,
                            UINT lines, LPCVOID bits, const BITMAPINFO *info, UINT coloruse )
{
    LONG height, width;
    WORD infoBpp, compression;
    PVOID safeBits = NULL;
    INT result, bitsSize;

    /* Perform extensive parameter checking */
    if (DIB_GetBitmapInfo( &info->bmiHeader, &width, &height,
        &infoBpp, &compression ) == -1)
        return 0;

    if (height < 0) height = -height;
    if (!lines || (startscan >= height))
        return 0;

    if (startscan + lines > height) lines = height - startscan;

    /* Create a safe copy of bits */
    bitsSize = DIB_GetDIBWidthBytes( width, infoBpp ) * height;

    if ( bits )
    {
        safeBits = HeapAlloc( GetProcessHeap(), 0, bitsSize );
        if (safeBits)
        {
            _SEH2_TRY
            {
                RtlCopyMemory( safeBits, bits, bitsSize );
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                ERR("StretchDIBits failed to read bits 0x%p, size: %d\n", bits, bitsSize);
            }
            _SEH2_END
        }
    }

    hbitmap = (HBITMAP)MapUserHandle(hbitmap);

    result = RosGdiSetDIBits(pdcattr->hKernelDC, hbitmap, startscan,
        lines, safeBits, info, coloruse);

    /* Free safe copy of bits */
    if ( safeBits )
        HeapFree( GetProcessHeap(), 0, safeBits );

    return result;
}

INT CDECL RosDrv_GetDIBits( PDC_ATTR pdcattr, HBITMAP hbitmap, UINT startscan, UINT lines,
                            LPVOID bits, BITMAPINFO *info, UINT coloruse )
{
    size_t obj_size;
    DIBSECTION dib;

    /* Check if this bitmap has a DIB section */
    if (!(obj_size = GetObjectW( hbitmap, sizeof(dib), &dib ))) return 0;

    hbitmap = (HBITMAP)MapUserHandle(hbitmap);

    /* Perform GetDIBits */
    return RosGdiGetDIBits(pdcattr->hKernelDC, hbitmap, startscan, lines, bits, info, coloruse, &dib);
}

HBITMAP CDECL RosDrv_CreateDIBSection( PDC_ATTR pdcattr, HBITMAP hbitmap,
                                       const BITMAPINFO *bmi, UINT usage )
{
    DIBSECTION dib;
    LONG height, width;
    WORD infoBpp, compression;
    HBITMAP hKbitmap;

    GetObjectW( hbitmap, sizeof(dib), &dib );

    /* Get parameters to check if it's topdown or not.
       GetObject doesn't return this info */
    DIB_GetBitmapInfo(&bmi->bmiHeader, &width, &height, &infoBpp, &compression);

    // TODO: Should pass as a flag instead
    if (height < 0) dib.dsBmih.biHeight *= -1;

    hKbitmap = RosGdiCreateDIBSection(pdcattr->hKernelDC, bmi, usage, &dib);

    AddHandleMapping(hKbitmap, hbitmap);

    return hbitmap;
}

UINT CDECL RosDrv_SetDIBColorTable( PDC_ATTR pdcattr, UINT start, UINT count, const RGBQUAD *colors )
{
    return RosGdiSetDIBColorTable(pdcattr->hKernelDC, start, count, colors);
}


/* EOF */
