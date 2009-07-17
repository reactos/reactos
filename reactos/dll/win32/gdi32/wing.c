/*
 * WinG support
 *
 * Copyright (C) Robert Pouliot <krynos@clic.net>
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "wownt32.h"
#include "gdi_private.h"
#include "wine/wingdi16.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wing);

/*************************************************************************
 * WING {WING}
 *
 * The Windows Game dll provides a number of functions designed to allow
 * programmers to bypass Gdi and write directly to video memory. The intention
 * was to bolster the use of Windows as a gaming platform and remove the
 * need for Dos based games using 32 bit Dos extenders.
 *
 * This initial approach could not compete with the performance of Dos games
 * (such as Doom and Warcraft) at the time, and so this dll was eventually
 * superseded by DirectX. It should not be used by new applications, and is
 * provided only for compatibility with older Windows programs.
 */

typedef enum WING_DITHER_TYPE
{
  WING_DISPERSED_4x4, WING_DISPERSED_8x8, WING_CLUSTERED_4x4
} WING_DITHER_TYPE;

/*
 * WinG DIB bitmaps can be selected into DC and then scribbled upon
 * by GDI functions. They can also be changed directly. This gives us
 * three choices
 *	- use original WinG 16-bit DLL
 *		requires working 16-bit driver interface
 * 	- implement DIB graphics driver from scratch
 *		see wing.zip size
 *	- use shared pixmaps
 *		won't work with some videocards and/or videomodes
 * 961208 - AK
 */

/***********************************************************************
 *          WinGCreateDC	(WING.1001)
 *
 * Create a new WinG device context.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  Success: A handle to the created device context.
 *  Failure: A NULL handle.
 */
HDC16 WINAPI WinGCreateDC16(void)
{
    TRACE("(void)\n");
	return CreateCompatibleDC16(0);
}

/***********************************************************************
 *  WinGRecommendDIBFormat    (WING.1002)
 *
 * Get the recommended format of bitmaps for the current display.
 *
 * PARAMS
 *  bmpi [O] Destination for format information
 *
 * RETURNS
 *  Success: TRUE. bmpi is filled with the best (fastest) bitmap format
 *  Failure: FALSE, if bmpi is NULL.
 */
BOOL16 WINAPI WinGRecommendDIBFormat16(BITMAPINFO *bmpi)
{
    TRACE("(%p)\n", bmpi);

    if (!bmpi)
	return FALSE;

    bmpi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmpi->bmiHeader.biWidth = 320;
    bmpi->bmiHeader.biHeight = -1;
    bmpi->bmiHeader.biPlanes = 1;
    bmpi->bmiHeader.biBitCount = 8;
    bmpi->bmiHeader.biCompression = BI_RGB;
    bmpi->bmiHeader.biSizeImage = 0;
    bmpi->bmiHeader.biXPelsPerMeter = 0;
    bmpi->bmiHeader.biYPelsPerMeter = 0;
    bmpi->bmiHeader.biClrUsed = 0;
    bmpi->bmiHeader.biClrImportant = 0;

    return TRUE;
}

/***********************************************************************
 *        WinGCreateBitmap    (WING.1003)
 *
 * Create a new WinG bitmap.
 *
 * PARAMS
 *  hdc  [I] WinG device context
 *  bmpi [I] Information about the bitmap
 *  bits [I] Location of the bitmap image data
 *
 * RETURNS
 *  Success: A handle to the created bitmap.
 *  Failure: A NULL handle.
 */
HBITMAP16 WINAPI WinGCreateBitmap16(HDC16 hdc, BITMAPINFO *bmpi,
                                    SEGPTR *bits)
{
    TRACE("(%d,%p,%p)\n", hdc, bmpi, bits);
    TRACE(": create %dx%dx%d bitmap\n", bmpi->bmiHeader.biWidth,
	  bmpi->bmiHeader.biHeight, bmpi->bmiHeader.biPlanes);
    return CreateDIBSection16(hdc, bmpi, 0, bits, 0, 0);
}

/***********************************************************************
 *  WinGGetDIBPointer   (WING.1004)
 */
SEGPTR WINAPI WinGGetDIBPointer16(HBITMAP16 hWinGBitmap, BITMAPINFO* bmpi)
{
    BITMAPOBJ* bmp = GDI_GetObjPtr( HBITMAP_32(hWinGBitmap), OBJ_BITMAP );
    SEGPTR res = 0;

    TRACE("(%d,%p)\n", hWinGBitmap, bmpi);
    if (!bmp) return 0;

    if (bmpi) FIXME(": Todo - implement setting BITMAPINFO\n");

    res = bmp->segptr_bits;
    GDI_ReleaseObj( HBITMAP_32(hWinGBitmap) );
    return res;
}

/***********************************************************************
 *  WinGSetDIBColorTable   (WING.1006)
 *
 * Set all or part of the color table for a WinG device context.
 *
 * PARAMS
 *  hdc    [I] WinG device context
 *  start  [I] Start color
 *  num    [I] Number of entries to set
 *  colors [I] Array of color data
 *
 * RETURNS
 *  The number of entries set.
 */
UINT16 WINAPI WinGSetDIBColorTable16(HDC16 hdc, UINT16 start, UINT16 num,
                                     RGBQUAD *colors)
{
    TRACE("(%d,%d,%d,%p)\n", hdc, start, num, colors);
    return SetDIBColorTable16(hdc, start, num, colors);
}

/***********************************************************************
 *  WinGGetDIBColorTable   (WING.1005)
 *
 * Get all or part of the color table for a WinG device context.
 *
 * PARAMS
 *  hdc    [I] WinG device context
 *  start  [I] Start color
 *  num    [I] Number of entries to set
 *  colors [O] Destination for the array of color data
 *
 * RETURNS
 *  The number of entries retrieved.
 */
UINT16 WINAPI WinGGetDIBColorTable16(HDC16 hdc, UINT16 start, UINT16 num,
				     RGBQUAD *colors)
{
    TRACE("(%d,%d,%d,%p)\n", hdc, start, num, colors);
    return GetDIBColorTable16(hdc, start, num, colors);
}

/***********************************************************************
 *  WinGCreateHalfTonePalette   (WING.1007)
 *
 * Create a half tone palette.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  Success: A handle to the created palette.
 *  Failure: A NULL handle.
 */
HPALETTE16 WINAPI WinGCreateHalfTonePalette16(void)
{
    HDC16 hdc = CreateCompatibleDC16(0);
    HPALETTE16 ret = CreateHalftonePalette16(hdc);
    TRACE("(void)\n");
    DeleteDC16(hdc);
    return ret;
}

/***********************************************************************
 *  WinGCreateHalfToneBrush   (WING.1008)
 *
 * Create a half tone brush for a WinG device context.
 *
 * PARAMS
 *  winDC [I] WinG device context
 *  col   [I] Color
 *  type  [I] Desired dithering type.
 *
 * RETURNS
 *  Success: A handle to the created brush.
 *  Failure: A NULL handle.
 */
HBRUSH16 WINAPI WinGCreateHalfToneBrush16(HDC16 winDC, COLORREF col,
                                            WING_DITHER_TYPE type)
{
    TRACE("(%d,%d,%d)\n", winDC, col, type);
    return CreateSolidBrush16(col);
}

/***********************************************************************
 *  WinGStretchBlt   (WING.1009)
 *
 * See StretchBlt16.
 */
BOOL16 WINAPI WinGStretchBlt16(HDC16 destDC, INT16 xDest, INT16 yDest,
                               INT16 widDest, INT16 heiDest,
                               HDC16 srcDC, INT16 xSrc, INT16 ySrc,
                               INT16 widSrc, INT16 heiSrc)
{
    BOOL16 retval;
    TRACE("(%d,%d,...)\n", destDC, srcDC);
    SetStretchBltMode16 ( destDC, COLORONCOLOR );
    retval=StretchBlt16(destDC, xDest, yDest, widDest, heiDest, srcDC,
			xSrc, ySrc, widSrc, heiSrc, SRCCOPY);
    SetStretchBltMode16 ( destDC, BLACKONWHITE );
    return retval;
}

/***********************************************************************
 *  WinGBitBlt   (WING.1010)
 *
 * See BitBlt16.
 */
BOOL16 WINAPI WinGBitBlt16(HDC16 destDC, INT16 xDest, INT16 yDest,
                           INT16 widDest, INT16 heiDest, HDC16 srcDC,
                           INT16 xSrc, INT16 ySrc)
{
    TRACE("(%d,%d,...)\n", destDC, srcDC);
    return BitBlt16(destDC, xDest, yDest, widDest, heiDest, srcDC,
		    xSrc, ySrc, SRCCOPY);
}
