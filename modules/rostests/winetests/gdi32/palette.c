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

    /* Initialize the logical palette with a few colours */
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

    /* Test with an invalid DIBINDEX to DIB_PAL_COLORS */
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

static void test_palette_entries(void)
{
    char logpalettebuf[sizeof(LOGPALETTE) + sizeof(logpalettedata)];
    PLOGPALETTE logpalette = (PLOGPALETTE)logpalettebuf;
    HPALETTE hpal;
    UINT res=0;
    PALETTEENTRY palEntry = { 0x1, 0x2, 0x3, 0xff };
    PALETTEENTRY getEntryResult;

    /* Initialize the logical palette with a few colours */
    logpalette->palVersion = 0x300;
    logpalette->palNumEntries = 8;
    memcpy( logpalette->palPalEntry, logpalettedata, sizeof(logpalettedata) );
    hpal = CreatePalette( logpalette );

    /* Set a new entry with peFlags to 0xff */
    SetPaletteEntries(hpal, 0, 1, &palEntry);

    /* Retrieve the entry to see if GDI32 performs any filtering on peFlags */
    res = GetPaletteEntries(hpal, 0, 1, &getEntryResult);
    ok(res == 1, "GetPaletteEntries should have returned 1 but returned %d\n", res);

    ok( palEntry.peFlags == getEntryResult.peFlags, "palEntry.peFlags (%#x) != getEntryResult.peFlags (%#x)\n", palEntry.peFlags, getEntryResult.peFlags );

    /* Try setting the system palette */
    hpal = GetStockObject(DEFAULT_PALETTE);
    res = SetPaletteEntries(hpal, 0, 1, &palEntry);
    ok(!res, "SetPaletteEntries() should have failed\n");

    res = GetPaletteEntries(hpal, 0, 1, &getEntryResult);
    ok(res == 1, "GetPaletteEntries should have returned 1 but returned %d\n", res);
    ok(memcmp(&palEntry, &getEntryResult, sizeof(PALETTEENTRY)), "entries should not match\n");
}

static void test_halftone_palette(void)
{
    HDC hdc;
    HPALETTE pal;
    PALETTEENTRY entries[256];
    PALETTEENTRY defpal[20];
    int i, count;

    hdc = GetDC(0);

    count = GetPaletteEntries( GetStockObject(DEFAULT_PALETTE), 0, 20, defpal );
    ok( count == 20, "wrong size %u\n", count );

    pal = CreateHalftonePalette( hdc );
    count = GetPaletteEntries( pal, 0, 256, entries );
    ok( count == 256 || broken(count <= 20), /* nt 4 */
        "wrong size %u\n", count );

    /* first and last 8 match the default palette */
    if (count >= 20)
    {
        for (i = 0; i < 8; i++)
        {
            ok( entries[i].peRed   == defpal[i].peRed &&
                entries[i].peGreen == defpal[i].peGreen &&
                entries[i].peBlue  == defpal[i].peBlue &&
                !entries[i].peFlags,
                "%u: wrong color %02x,%02x,%02x,%02x instead of %02x,%02x,%02x\n", i,
                entries[i].peRed, entries[i].peGreen, entries[i].peBlue, entries[i].peFlags,
                defpal[i].peRed, defpal[i].peGreen, defpal[i].peBlue );
        }
        for (i = count - 8; i < count; i++)
        {
            int idx = i - count + 20;
            ok( entries[i].peRed   == defpal[idx].peRed &&
                entries[i].peGreen == defpal[idx].peGreen &&
                entries[i].peBlue  == defpal[idx].peBlue &&
                !entries[i].peFlags,
                "%u: wrong color %02x,%02x,%02x,%02x instead of %02x,%02x,%02x\n", i,
                entries[i].peRed, entries[i].peGreen, entries[i].peBlue, entries[i].peFlags,
                defpal[idx].peRed, defpal[idx].peGreen, defpal[idx].peBlue );
        }
    }
    DeleteObject( pal );
    ReleaseDC( 0, hdc );
}

static void check_system_palette_entries(HDC hdc)
{
    PALETTEENTRY entries[256];
    PALETTEENTRY defpal[20];
    int i, count;

    memset( defpal, 0xaa, sizeof(defpal) );
    count = GetPaletteEntries( GetStockObject(DEFAULT_PALETTE), 0, 20, defpal );
    ok( count == 20, "wrong size %u\n", count );

    memset( entries, 0x55, sizeof(entries) );
    count = GetSystemPaletteEntries( hdc, 0, 256, entries );
    ok( count == 0, "wrong size %u\n", count);
    for (i = 0; i < 10; i++)
    {
        ok( entries[i].peRed   == defpal[i].peRed &&
            entries[i].peGreen == defpal[i].peGreen &&
            entries[i].peBlue  == defpal[i].peBlue &&
            !entries[i].peFlags,
            "%u: wrong color %02x,%02x,%02x,%02x instead of %02x,%02x,%02x\n", i,
            entries[i].peRed, entries[i].peGreen, entries[i].peBlue, entries[i].peFlags,
            defpal[i].peRed, defpal[i].peGreen, defpal[i].peBlue );
    }
    for (i = 10; i < 246; ++i)
    {
        ok( !entries[i].peRed   &&
            !entries[i].peGreen &&
            !entries[i].peBlue  &&
            !entries[i].peFlags,
            "%u: wrong color %02x,%02x,%02x,%02x instead of 0,0,0\n", i,
            entries[i].peRed, entries[i].peGreen, entries[i].peBlue, entries[i].peFlags);
    }
    for (i = 246; i < 256; i++)
    {
        int idx = i - 246 + 10;
        ok( entries[i].peRed   == defpal[idx].peRed &&
            entries[i].peGreen == defpal[idx].peGreen &&
            entries[i].peBlue  == defpal[idx].peBlue &&
            !entries[i].peFlags,
            "%u: wrong color %02x,%02x,%02x,%02x instead of %02x,%02x,%02x\n", i,
            entries[i].peRed, entries[i].peGreen, entries[i].peBlue, entries[i].peFlags,
            defpal[idx].peRed, defpal[idx].peGreen, defpal[idx].peBlue );
    }

    memset( entries, 0x55, sizeof(entries) );
    count = GetSystemPaletteEntries( hdc, 0, 10, entries );
    ok( count == 0, "wrong size %u\n", count);
    for (i = 0; i < 10; i++)
    {
        ok( entries[i].peRed   == defpal[i].peRed &&
            entries[i].peGreen == defpal[i].peGreen &&
            entries[i].peBlue  == defpal[i].peBlue &&
            !entries[i].peFlags,
            "%u: wrong color %02x,%02x,%02x,%02x instead of %02x,%02x,%02x\n", i,
            entries[i].peRed, entries[i].peGreen, entries[i].peBlue, entries[i].peFlags,
            defpal[i].peRed, defpal[i].peGreen, defpal[i].peBlue );
    }

    memset( entries, 0x55, sizeof(entries) );
    count = GetSystemPaletteEntries( hdc, 10, 246, entries );
    ok( count == 0, "wrong size %u\n", count);
    for (i = 0; i < 236; ++i)
    {
        ok( !entries[i].peRed   &&
            !entries[i].peGreen &&
            !entries[i].peBlue  &&
            !entries[i].peFlags,
            "%u: wrong color %02x,%02x,%02x,%02x instead of 0,0,0\n", i,
            entries[i].peRed, entries[i].peGreen, entries[i].peBlue, entries[i].peFlags);
    }
    for (i = 236; i < 246; i++)
    {
        int idx = i - 236 + 10;
        ok( entries[i].peRed   == defpal[idx].peRed &&
            entries[i].peGreen == defpal[idx].peGreen &&
            entries[i].peBlue  == defpal[idx].peBlue &&
            !entries[i].peFlags,
            "%u: wrong color %02x,%02x,%02x,%02x instead of %02x,%02x,%02x\n", i,
            entries[i].peRed, entries[i].peGreen, entries[i].peBlue, entries[i].peFlags,
            defpal[idx].peRed, defpal[idx].peGreen, defpal[idx].peBlue );
    }

    memset( entries, 0x55, sizeof(entries) );
    count = GetSystemPaletteEntries( hdc, 246, 10, entries );
    ok( count == 0, "wrong size %u\n", count);
    for (i = 0; i < 10; i++)
    {
        int idx = i + 10;
        ok( entries[i].peRed   == defpal[idx].peRed &&
            entries[i].peGreen == defpal[idx].peGreen &&
            entries[i].peBlue  == defpal[idx].peBlue &&
            !entries[i].peFlags,
            "%u: wrong color %02x,%02x,%02x,%02x instead of %02x,%02x,%02x\n", i,
            entries[i].peRed, entries[i].peGreen, entries[i].peBlue, entries[i].peFlags,
            defpal[idx].peRed, defpal[idx].peGreen, defpal[idx].peBlue );
    }
}

static void test_system_palette_entries(void)
{
    HDC hdc;
    HDC metafile_dc;
    HMETAFILE metafile;

    hdc = GetDC(0);

    if (!(GetDeviceCaps( hdc, RASTERCAPS ) & RC_PALETTE))
    {
        check_system_palette_entries(hdc);
    }
    else
    {
        skip( "device is palette-based, skipping test\n" );
    }

    ReleaseDC( 0, hdc );

    metafile_dc = CreateMetaFileA(NULL);

    check_system_palette_entries(metafile_dc);

    metafile = CloseMetaFile(metafile_dc);
    DeleteMetaFile(metafile);

    check_system_palette_entries(ULongToHandle(0xdeadbeef));
}

START_TEST(palette)
{
    test_DIB_PAL_COLORS();
    test_palette_entries();
    test_halftone_palette();
    test_system_palette_entries();
}
