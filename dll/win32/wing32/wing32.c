/*
 * WinG support
 *
 * Copyright 2007 Dmitry Timoshkov
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
#include <windef.h>
#include <winuser.h>
#include <wingdi.h>

/***********************************************************************
 *           WinGCreateDC   (WING32.@)
 */
HDC CALLBACK WinGCreateDC( void )
{
    return CreateCompatibleDC( NULL );
}

/***********************************************************************
 *           WinGRecommendDIBFormat   (WING32.@)
 */
BOOL CALLBACK WinGRecommendDIBFormat( BITMAPINFO *bmi )
{
    if (!bmi) return FALSE;

    bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi->bmiHeader.biWidth = 320;
    bmi->bmiHeader.biHeight = -1;
    bmi->bmiHeader.biPlanes = 1;
    bmi->bmiHeader.biBitCount = 8;
    bmi->bmiHeader.biCompression = BI_RGB;
    bmi->bmiHeader.biSizeImage = 0;
    bmi->bmiHeader.biXPelsPerMeter = 0;
    bmi->bmiHeader.biYPelsPerMeter = 0;
    bmi->bmiHeader.biClrUsed = 0;
    bmi->bmiHeader.biClrImportant = 0;

    return TRUE;
}

/***********************************************************************
 *           WinGCreateBitmap   (WING32.@)
 */
HBITMAP CALLBACK WinGCreateBitmap( HDC hdc, BITMAPINFO *bmi, void **bits )
{
    return CreateDIBSection( hdc, bmi, 0, bits, 0, 0 );
}

/***********************************************************************
 *           WinGGetDIBPointer   (WING32.@)
 */
void * CALLBACK WinGGetDIBPointer( HBITMAP hbmp, BITMAPINFO *bmi )
{
    DIBSECTION ds;

    if (GetObject( hbmp, sizeof(ds), &ds ) == sizeof(ds))
    {
        if (bmi != NULL)
            memcpy( &bmi->bmiHeader, &ds.dsBmih, sizeof(*bmi) );

        return ds.dsBm.bmBits;
    }
    return NULL;
}

/***********************************************************************
 *           WinGSetDIBColorTable   (WING32.@)
 */
UINT CALLBACK WinGSetDIBColorTable( HDC hdc, UINT start, UINT end, RGBQUAD *colors )
{
    return SetDIBColorTable( hdc, start, end, colors );
}

/***********************************************************************
 *           WinGGetDIBColorTable   (WING32.@)
 */
UINT CALLBACK WinGGetDIBColorTable( HDC hdc, UINT start, UINT end, RGBQUAD *colors )
{
    return GetDIBColorTable( hdc, start, end, colors );
}

/***********************************************************************
 *           WinGCreateHalfTonePalette   (WING32.@)
 */
HPALETTE CALLBACK WinGCreateHalfTonePalette( void )
{
    HDC hdc;
    HPALETTE hpal;

    hdc = GetDC( NULL );
    hpal = CreateHalftonePalette( hdc );
    ReleaseDC( NULL, hdc );

    return hpal;
}

/***********************************************************************
 *           WinGCreateHalfToneBrush   (WING32.@)
 */
HBRUSH CALLBACK WinGCreateHalfToneBrush( HDC hdc, COLORREF color, INT type )
{
    return CreateSolidBrush( color );
}

/***********************************************************************
 *           WinGStretchBlt   (WING32.@)
 */
BOOL CALLBACK WinGStretchBlt( HDC hdcDst, INT xDst, INT yDst, INT widthDst, INT heightDst,
                            HDC hdcSrc, INT xSrc, INT ySrc, INT widthSrc, INT heightSrc )
{
    int old_blt_mode;
    BOOL ret;

    old_blt_mode = SetStretchBltMode( hdcDst, COLORONCOLOR );
    ret = StretchBlt( hdcDst, xDst, yDst, widthDst, heightDst,
                      hdcSrc, xSrc, ySrc, widthSrc, heightSrc, SRCCOPY );
    SetStretchBltMode( hdcDst, old_blt_mode );
    return ret;
}

/***********************************************************************
 *           WinGBitBlt   (WING32.@)
 */
BOOL CALLBACK WinGBitBlt( HDC hdcDst, INT xDst, INT yDst, INT width,
                        INT height, HDC hdcSrc, INT xSrc, INT ySrc )
{
    return BitBlt( hdcDst, xDst, yDst, width, height, hdcSrc, xSrc, ySrc, SRCCOPY );
}
