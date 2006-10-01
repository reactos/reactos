/*
 * Unit test suite for palettes
 *
 * Copyright 2005 Glenn Wurster
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
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "mmsystem.h"

#include "wine/test.h"

static const PALETTEENTRY logpalettedata[8] = {
    { 0x10, 0x20, 0x30, PC_NOCOLLAPSE },
    { 0x20, 0x30, 0x40, PC_NOCOLLAPSE },
    { 0x30, 0x40, 0x50, PC_NOCOLLAPSE },
    { 0x40, 0x50, 0x60, PC_NOCOLLAPSE },
    { 0x50, 0x60, 0x70, PC_NOCOLLAPSE },
    { 0x60, 0x70, 0x80, PC_NOCOLLAPSE },
    { 0x70, 0x80, 0x90, PC_NOCOLLAPSE },
    { 0x80, 0x90, 0xA0, PC_NOCOLLAPSE },
};

static void test_DIB_PAL_COLORS(void) {
    HDC hdc = GetDC( NULL );
    HDC memhdc = CreateCompatibleDC( hdc );
    HBITMAP hbmp, hbmpOld;
    char bmpbuf[sizeof(BITMAPINFO) + 10 * sizeof(WORD)];
    PBITMAPINFO bmp = (PBITMAPINFO)bmpbuf;
    WORD * bmpPalPtr;
    char logpalettebuf[sizeof(LOGPALETTE) + sizeof(logpalettedata)];
    PLOGPALETTE logpalette = (PLOGPALETTE)logpalettebuf;
    HPALETTE hpal, hpalOld;
    COLORREF setColor, chkColor, getColor;
    int i;

    /* Initalize the logical palette with a few colours */
    logpalette->palVersion = 0x300;
    logpalette->palNumEntries = 8;
    memcpy( logpalette->palPalEntry, logpalettedata, sizeof(logpalettedata) );
    hpal = CreatePalette( logpalette ); 
    hpalOld = SelectPalette( memhdc, hpal, FALSE );
    ok( hpalOld != NULL, "error=%ld\n", GetLastError() );

    /* Create a DIB BMP which references colours in the logical palette */
    memset( bmp, 0x00, sizeof(BITMAPINFO) );
    bmp->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmp->bmiHeader.biWidth = 1;
    bmp->bmiHeader.biHeight = 1;
    bmp->bmiHeader.biPlanes = 1;
    bmp->bmiHeader.biBitCount = 8;
    bmp->bmiHeader.biCompression = BI_RGB;
    bmp->bmiHeader.biClrUsed = 10;
    bmp->bmiHeader.biClrImportant = 0;
    bmpPalPtr = (WORD *)&bmp->bmiColors;
    for( i = 0; i < 8; i++ ) {
        *bmpPalPtr++ = i;
    }
    *bmpPalPtr++ = 8; /* Pointer to logical palette index just outside range */
    *bmpPalPtr++ = 19; /* Pointer to bad logical palette index */

    hbmp = CreateDIBSection( memhdc, bmp, DIB_PAL_COLORS, 0, 0, 0 );
    ok( hbmp != NULL, "error=%ld\n", GetLastError() );
    hbmpOld = SelectObject( memhdc, hbmp );
    ok( hbmpOld != NULL, "error=%ld\n", GetLastError() );

    /* Test with a RGB to DIB_PAL_COLORS */
    setColor = RGB( logpalettedata[1].peRed, logpalettedata[1].peGreen, logpalettedata[1].peBlue );
    SetPixel( memhdc, 0, 0, setColor );
    chkColor = RGB( logpalettedata[1].peRed, logpalettedata[1].peGreen, logpalettedata[1].peBlue );
    getColor = GetPixel( memhdc, 0, 0 );
    ok( getColor == chkColor, "getColor=%08X\n", (UINT)getColor );

    /* Test with a valid DIBINDEX to DIB_PAL_COLORS */
    setColor = DIBINDEX( 2 );
    SetPixel( memhdc, 0, 0, setColor );
    chkColor = RGB( logpalettedata[2].peRed, logpalettedata[2].peGreen, logpalettedata[2].peBlue );
    getColor = GetPixel( memhdc, 0, 0 );
    ok( getColor == chkColor, "getColor=%08X\n", (UINT)getColor );

    /* Test with a invalid DIBINDEX to DIB_PAL_COLORS */
    setColor = DIBINDEX( 12 );
    SetPixel( memhdc, 0, 0, setColor );
    chkColor = RGB( 0, 0, 0 );
    getColor = GetPixel( memhdc, 0, 0 );
    ok( getColor == chkColor, "getColor=%08X\n", (UINT)getColor );

    /* Test for double wraparound on logical palette references from */
    /* DIBINDEX by DIB_PAL_COLORS. */
    setColor = DIBINDEX( 9 );
    SetPixel( memhdc, 0, 0, setColor );
    chkColor = RGB( logpalettedata[3].peRed, logpalettedata[3].peGreen, logpalettedata[3].peBlue );
    getColor = GetPixel( memhdc, 0, 0 );
    ok( getColor == chkColor, "getColor=%08X\n", (UINT)getColor );

    SelectPalette( memhdc, hpalOld, FALSE );
    DeleteObject( hpal );
    SelectObject( memhdc, hbmpOld );
    DeleteObject( hbmp );
    DeleteDC( memhdc );
    ReleaseDC( NULL, hdc );
}

START_TEST(palette)
{
    test_DIB_PAL_COLORS();
}
