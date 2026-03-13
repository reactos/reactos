/*
 * Unit test suite for brushes
 *
 * Copyright 2004 Kevin Koltzau
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

#include "wine/test.h"

typedef struct _STOCK_BRUSH {
    COLORREF color;
    int stockobj;
    const char *name;
} STOCK_BRUSH;

static void test_solidbrush(void)
{
    static const STOCK_BRUSH stock[] = {
        {RGB(255,255,255), WHITE_BRUSH, "white"},
        {RGB(192,192,192), LTGRAY_BRUSH, "ltgray"},
        {RGB(128,128,128), GRAY_BRUSH, "gray"},
        {RGB(0,0,0), BLACK_BRUSH, "black"},
        {RGB(0,0,255), -1, "blue"}
    };
    HBRUSH solidBrush;
    HBRUSH stockBrush;
    LOGBRUSH br;
    size_t i;
    INT ret;

    for(i = 0; i < ARRAY_SIZE(stock); i++) {
        solidBrush = CreateSolidBrush(stock[i].color);

        if(stock[i].stockobj != -1) {
            stockBrush = GetStockObject(stock[i].stockobj);
            ok(stockBrush!=solidBrush ||
               broken(stockBrush==solidBrush), /* win9x does return stock object */
               "Stock %s brush equals solid %s brush\n", stock[i].name, stock[i].name);
        }
        else
            stockBrush = NULL;
        memset(&br, 0, sizeof(br));
        ret = GetObjectW(solidBrush, sizeof(br), &br);
        ok( ret !=0, "GetObject on solid %s brush failed, error=%ld\n", stock[i].name, GetLastError());
        ok(br.lbStyle==BS_SOLID, "%s brush has wrong style, got %d expected %d\n", stock[i].name, br.lbStyle, BS_SOLID);
        ok(br.lbColor==stock[i].color, "%s brush has wrong color, got 0x%08lx expected 0x%08lx\n", stock[i].name, br.lbColor, stock[i].color);

        if(stockBrush) {
            /* Sanity check, make sure the colors being compared do in fact have a stock brush */
            ret = GetObjectW(stockBrush, sizeof(br), &br);
            ok( ret !=0, "GetObject on stock %s brush failed, error=%ld\n", stock[i].name, GetLastError());
            ok(br.lbColor==stock[i].color, "stock %s brush unexpected color, got 0x%08lx expected 0x%08lx\n", stock[i].name, br.lbColor, stock[i].color);
        }

        DeleteObject(solidBrush);
        ret = GetObjectW(solidBrush, sizeof(br), &br);
        ok(ret==0 ||
           broken(ret!=0), /* win9x */
           "GetObject succeeded on a deleted %s brush\n", stock[i].name);
    }
}

static void test_hatch_brush(void)
{
    int i, size;
    HBRUSH brush;
    LOGBRUSH lb;

    for (i = 0; i < 20; i++)
    {
        SetLastError( 0xdeadbeef );
        brush = CreateHatchBrush( i, RGB(12,34,56) );
        if (i < HS_API_MAX)
        {
            ok( brush != 0, "%u: CreateHatchBrush failed err %lu\n", i, GetLastError() );
            size = GetObjectW( brush, sizeof(lb), &lb );
            ok( size == sizeof(lb), "wrong size %u\n", size );
            ok( lb.lbColor == RGB(12,34,56), "wrong color %08lx\n", lb.lbColor );
            if (i <= HS_DIAGCROSS)
            {
                ok( lb.lbStyle == BS_HATCHED, "wrong style %u\n", lb.lbStyle );
                ok( lb.lbHatch == i, "wrong hatch %Iu/%u\n", lb.lbHatch, i );
            }
            else
            {
                ok( lb.lbStyle == BS_SOLID, "wrong style %u\n", lb.lbStyle );
                ok( lb.lbHatch == 0, "wrong hatch %Iu\n", lb.lbHatch );
            }
            DeleteObject( brush );
        }
        else
        {
            ok( !brush, "%u: CreateHatchBrush succeeded\n", i );
            ok( GetLastError() == 0xdeadbeef, "wrong error %lu\n", GetLastError() );
        }
    }
}

static void test_pattern_brush(void)
{
    char buffer[sizeof(BITMAPINFOHEADER) + 2 * sizeof(RGBQUAD) + 32 * 32 / 8];
    BITMAPINFO *info = (BITMAPINFO *)buffer;
    HBITMAP bitmap, bitmap2, old_bitmap;
    HBRUSH brush, brush2;
    HDC hdc, hdc_screen;
    COLORREF color;
    BOOL result;
    LOGBRUSH br;
    BITMAP bm;
    RECT rect;
    INT ret;
    void *bits;
    DIBSECTION dib;
    HGLOBAL mem;

    bitmap = CreateBitmap( 20, 20, 1, 1, NULL );
    ok( bitmap != NULL, "CreateBitmap failed\n" );
    brush = CreatePatternBrush( bitmap );
    ok( brush != NULL, "CreatePatternBrush failed\n" );
    memset( &br, 0x55, sizeof(br) );
    ret = GetObjectW( brush, sizeof(br), &br );
    ok( ret == sizeof(br), "wrong size %u\n", ret );
    ok( br.lbStyle == BS_PATTERN, "wrong style %u\n", br.lbStyle );
    ok( br.lbColor == 0, "wrong color %lu\n", br.lbColor );
    ok( (HBITMAP)br.lbHatch == bitmap, "wrong handle %p/%p\n", (HBITMAP)br.lbHatch, bitmap );
    DeleteObject( brush );

    br.lbStyle = BS_PATTERN8X8;
    br.lbColor = 0x12345;
    br.lbHatch = (ULONG_PTR)bitmap;
    brush = CreateBrushIndirect( &br );
    ok( brush != NULL, "CreatePatternBrush failed\n" );
    memset( &br, 0x55, sizeof(br) );
    ret = GetObjectW( brush, sizeof(br), &br );
    ok( ret == sizeof(br), "wrong size %u\n", ret );
    ok( br.lbStyle == BS_PATTERN, "wrong style %u\n", br.lbStyle );
    ok( br.lbColor == 0, "wrong color %lu\n", br.lbColor );
    ok( (HBITMAP)br.lbHatch == bitmap, "wrong handle %p/%p\n", (HBITMAP)br.lbHatch, bitmap );
    ret = GetObjectW( bitmap, sizeof(dib), &dib );
    ok( ret == sizeof(dib.dsBm), "wrong size %u\n", ret );
    DeleteObject( bitmap );
    ret = GetObjectW( bitmap, sizeof(dib), &dib );
    ok( ret == 0, "wrong size %u\n", ret );
    DeleteObject( brush );

    memset( info, 0, sizeof(buffer) );
    info->bmiHeader.biSize = sizeof(info->bmiHeader);
    info->bmiHeader.biHeight = 32;
    info->bmiHeader.biWidth = 32;
    info->bmiHeader.biBitCount = 1;
    info->bmiHeader.biPlanes = 1;
    info->bmiHeader.biCompression = BI_RGB;
    bitmap = CreateDIBSection( 0, info, DIB_RGB_COLORS, &bits, NULL, 0 );
    ok( bitmap != NULL, "CreateDIBSection failed\n" );

    /* MSDN says a DIB section is not allowed, but it works fine */
    brush = CreatePatternBrush( bitmap );
    ok( brush != NULL, "CreatePatternBrush failed\n" );
    memset( &br, 0x55, sizeof(br) );
    ret = GetObjectW( brush, sizeof(br), &br );
    ok( ret == sizeof(br), "wrong size %u\n", ret );
    ok( br.lbStyle == BS_PATTERN, "wrong style %u\n", br.lbStyle );
    ok( br.lbColor == 0, "wrong color %lu\n", br.lbColor );
    ok( (HBITMAP)br.lbHatch == bitmap, "wrong handle %p/%p\n", (HBITMAP)br.lbHatch, bitmap );
    ret = GetObjectW( bitmap, sizeof(dib), &dib );
    ok( ret == sizeof(dib), "wrong size %u\n", ret );
    DeleteObject( brush );
    DeleteObject( bitmap );

    brush = CreateDIBPatternBrushPt( info, DIB_RGB_COLORS );
    ok( brush != NULL, "CreatePatternBrush failed\n" );
    memset( &br, 0x55, sizeof(br) );
    ret = GetObjectW( brush, sizeof(br), &br );
    ok( ret == sizeof(br), "wrong size %u\n", ret );
    ok( br.lbStyle == BS_DIBPATTERN, "wrong style %u\n", br.lbStyle );
    ok( br.lbColor == 0, "wrong color %lu\n", br.lbColor );
    ok( (BITMAPINFO *)br.lbHatch == info || broken(!br.lbHatch), /* nt4 */
        "wrong handle %p/%p\n", (BITMAPINFO *)br.lbHatch, info );
    DeleteObject( brush );

    br.lbStyle = BS_DIBPATTERNPT;
    br.lbColor = DIB_PAL_COLORS;
    br.lbHatch = (ULONG_PTR)info;
    brush = CreateBrushIndirect( &br );
    ok( brush != NULL, "CreatePatternBrush failed\n" );
    memset( &br, 0x55, sizeof(br) );
    ret = GetObjectW( brush, sizeof(br), &br );
    ok( ret == sizeof(br), "wrong size %u\n", ret );
    ok( br.lbStyle == BS_DIBPATTERN, "wrong style %u\n", br.lbStyle );
    ok( br.lbColor == 0, "wrong color %lu\n", br.lbColor );
    ok( (BITMAPINFO *)br.lbHatch == info || broken(!br.lbHatch), /* nt4 */
        "wrong handle %p/%p\n", (BITMAPINFO *)br.lbHatch, info );

    mem = GlobalAlloc( GMEM_MOVEABLE, sizeof(buffer) );
    memcpy( GlobalLock( mem ), buffer, sizeof(buffer) );

    br.lbStyle = BS_DIBPATTERN;
    br.lbColor = DIB_PAL_COLORS;
    br.lbHatch = (ULONG_PTR)mem;
    brush = CreateBrushIndirect( &br );
    ok( brush != NULL, "CreatePatternBrush failed\n" );
    memset( &br, 0x55, sizeof(br) );
    ret = GetObjectW( brush, sizeof(br), &br );
    ok( ret == sizeof(br), "wrong size %u\n", ret );
    ok( br.lbStyle == BS_DIBPATTERN, "wrong style %u\n", br.lbStyle );
    ok( br.lbColor == 0, "wrong color %lu\n", br.lbColor );
    ok( (HGLOBAL)br.lbHatch != mem, "wrong handle %p/%p\n", (HGLOBAL)br.lbHatch, mem );
    bits = GlobalLock( mem );
    ok( (HGLOBAL)br.lbHatch == bits || broken(!br.lbHatch), /* nt4 */
        "wrong handle %p/%p\n", (HGLOBAL)br.lbHatch, bits );
    ret = GlobalFlags( mem );
    ok( ret == 2, "wrong flags %x\n", ret );
    DeleteObject( brush );
    ret = GlobalFlags( mem );
    ok( ret == 2, "wrong flags %x\n", ret );

    brush = CreateDIBPatternBrushPt( info, DIB_PAL_COLORS );
    ok( brush != 0, "CreateDIBPatternBrushPt failed\n" );
    DeleteObject( brush );
    brush = CreateDIBPatternBrushPt( info, DIB_PAL_COLORS + 1 );
    ok( brush != 0, "CreateDIBPatternBrushPt failed\n" );
    DeleteObject( brush );
    brush = CreateDIBPatternBrushPt( info, DIB_PAL_COLORS + 2 );
    ok( !brush, "CreateDIBPatternBrushPt succeeded\n" );
    brush = CreateDIBPatternBrushPt( info, DIB_PAL_COLORS + 3 );
    ok( !brush, "CreateDIBPatternBrushPt succeeded\n" );

    info->bmiHeader.biBitCount = 8;
    info->bmiHeader.biCompression = BI_RLE8;
    brush = CreateDIBPatternBrushPt( info, DIB_RGB_COLORS );
    ok( !brush, "CreateDIBPatternBrushPt succeeded\n" );

    info->bmiHeader.biBitCount = 4;
    info->bmiHeader.biCompression = BI_RLE4;
    brush = CreateDIBPatternBrushPt( info, DIB_RGB_COLORS );
    ok( !brush, "CreateDIBPatternBrushPt succeeded\n" );

    br.lbStyle = BS_DIBPATTERN8X8;
    br.lbColor = DIB_RGB_COLORS;
    br.lbHatch = (ULONG_PTR)mem;
    brush = CreateBrushIndirect( &br );
    ok( !brush, "CreatePatternBrush succeeded\n" );

    br.lbStyle = BS_MONOPATTERN;
    br.lbColor = DIB_RGB_COLORS;
    br.lbHatch = (ULONG_PTR)mem;
    brush = CreateBrushIndirect( &br );
    ok( !brush, "CreatePatternBrush succeeded\n" );

    br.lbStyle = BS_INDEXED;
    br.lbColor = DIB_RGB_COLORS;
    br.lbHatch = (ULONG_PTR)mem;
    brush = CreateBrushIndirect( &br );
    ok( !brush, "CreatePatternBrush succeeded\n" );

    GlobalFree( mem );

    /* Test deleting bitmap after brush creation */
    /* Create hdc and bitmaps */
    hdc_screen = GetDC( NULL );
    hdc = CreateCompatibleDC( hdc_screen );
    bitmap = CreateCompatibleBitmap( hdc_screen, 16, 16 );
    bitmap2 = CreateCompatibleBitmap( hdc_screen, 16, 16 );

    /* Fill the first bitmap with 0xff5511 */
    old_bitmap = SelectObject( hdc, bitmap );
    SetRect( &rect, 0, 0, 16, 16 );
    brush = CreateSolidBrush( 0xff5511 );
    result = FillRect( hdc, &rect, brush );
    ok( result, "FillRect failed, error %ld.\n", GetLastError() );
    DeleteObject( brush );
    color = GetPixel( hdc, 10, 10 );
    ok( color == 0xff5511, "Expected color %#x, got %#lx.\n", 0xff5511, color );

    /* Create a pattern brush with the first bitmap filled with 0xff5511 */
    brush = CreatePatternBrush( bitmap );
    ok( brush != NULL, "CreatePatternBrush failed, error %lu.\n", GetLastError() );

    /* Delete the first bitmap used for pattern brush creation */
    SelectObject( hdc, bitmap2 );
    DeleteObject( bitmap );

    memset( &br, 0, sizeof(br) );
    ret = GetObjectW( brush, sizeof(br), &br );
    ok( ret == sizeof(br), "wrong size %u\n", ret );
    ok( br.lbColor == 0, "wrong color %lu\n", br.lbColor );
    ok( br.lbStyle == BS_PATTERN, "wrong style %u\n", br.lbStyle );
    ok( (HBITMAP)br.lbHatch == bitmap, "wrong handle %p/%p\n", (HBITMAP)br.lbHatch, bitmap );

    /* The first bitmap is now invalid */
    memset( &bm, 0, sizeof (bm));
    ret = GetObjectW( bitmap, sizeof(bm), &bm );
    ok( !ret, "wrong size %u\n", ret );

    /* Fill hdc with 0xabcdef */
    brush2 = CreateSolidBrush( 0xabcdef );
    result = FillRect( hdc, &rect, brush2 );
    ok( result, "FillRect failed, error %ld.\n", GetLastError() );
    color = GetPixel( hdc, 10, 10 );
    ok( color == 0xabcdef, "Expected color %#x, got %#lx.\n", 0xabcdef, color );
    DeleteObject( brush2 );

    /* Fill hdc with the brush created with the deleted bitmap */
    /* FillRect() succeeds and hdc is filled with the deleted bitmap content */
    result = FillRect( hdc, &rect, brush );
    ok( result, "FillRect failed, error %ld.\n", GetLastError() );
    color = GetPixel( hdc, 10, 10 );
    ok( color == 0xff5511, "Expected color %#x, got %#lx.\n", 0xff5511, color );
    DeleteObject( brush );

    SelectObject( hdc, old_bitmap );
    DeleteObject( bitmap2 );
    DeleteDC( hdc );
    ReleaseDC( NULL, hdc_screen );
}

static void test_palette_brush(void)
{
    char buffer[sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD) + 16 * 16];
    BITMAPINFO *info = (BITMAPINFO *)buffer;
    WORD *indices = (WORD *)info->bmiColors;
    char pal_buffer[sizeof(LOGPALETTE) + 256 * sizeof(PALETTEENTRY)];
    LOGPALETTE *pal = (LOGPALETTE *)pal_buffer;
    HDC hdc = CreateCompatibleDC( 0 );
    DWORD *dib_bits;
    HBITMAP dib;
    HBRUSH brush;
    int i;
    HPALETTE palette, palette2;

    memset( info, 0, sizeof(*info) );
    info->bmiHeader.biSize        = sizeof(info->bmiHeader);
    info->bmiHeader.biWidth       = 16;
    info->bmiHeader.biHeight      = 16;
    info->bmiHeader.biPlanes      = 1;
    info->bmiHeader.biBitCount    = 32;
    info->bmiHeader.biCompression = BI_RGB;
    dib = CreateDIBSection( NULL, info, DIB_RGB_COLORS, (void**)&dib_bits, NULL, 0 );
    ok( dib != NULL, "CreateDIBSection failed\n" );

    info->bmiHeader.biBitCount = 8;
    for (i = 0; i < 256; i++) indices[i] = 255 - i;
    for (i = 0; i < 256; i++) ((BYTE *)(indices + 256))[i] = i;
    brush = CreateDIBPatternBrushPt( info, DIB_PAL_COLORS );
    ok( brush != NULL, "CreateDIBPatternBrushPt failed\n" );

    pal->palVersion = 0x300;
    pal->palNumEntries = 256;
    for (i = 0; i < 256; i++)
    {
        pal->palPalEntry[i].peRed = i * 2;
        pal->palPalEntry[i].peGreen = i * 2;
        pal->palPalEntry[i].peBlue = i * 2;
        pal->palPalEntry[i].peFlags = 0;
    }
    palette = CreatePalette( pal );

    ok( SelectObject( hdc, dib ) != NULL, "SelectObject failed\n" );
    ok( SelectPalette( hdc, palette, 0 ) != NULL, "SelectPalette failed\n" );
    ok( SelectObject( hdc, brush ) != NULL, "SelectObject failed\n" );
    memset( dib_bits, 0xaa, 16 * 16 * 4 );
    PatBlt( hdc, 0, 0, 16, 16, PATCOPY );
    for (i = 0; i < 256; i++)
    {
        DWORD expect = (pal->palPalEntry[255 - i].peRed << 16 |
                        pal->palPalEntry[255 - i].peGreen << 8 |
                        pal->palPalEntry[255 - i].peBlue);
        ok( dib_bits[i] == expect, "wrong bits %lx/%lx at %u,%u\n", dib_bits[i], expect, i % 16, i / 16 );
    }

    for (i = 0; i < 256; i++) pal->palPalEntry[i].peRed = i * 3;
    palette2 = CreatePalette( pal );
    ok( SelectPalette( hdc, palette2, 0 ) != NULL, "SelectPalette failed\n" );
    memset( dib_bits, 0xaa, 16 * 16 * 4 );
    PatBlt( hdc, 0, 0, 16, 16, PATCOPY );
    for (i = 0; i < 256; i++)
    {
        DWORD expect = (pal->palPalEntry[255 - i].peRed << 16 |
                        pal->palPalEntry[255 - i].peGreen << 8 |
                        pal->palPalEntry[255 - i].peBlue);
        ok( dib_bits[i] == expect, "wrong bits %lx/%lx at %u,%u\n", dib_bits[i], expect, i % 16, i / 16 );
    }
    DeleteDC( hdc );
    DeleteObject( dib );
    DeleteObject( brush );
    DeleteObject( palette );
    DeleteObject( palette2 );
}

static void test_brush_org( void )
{
    HDC hdc = GetDC( 0 );
    POINT old, pt;
    BOOL ret;

    ret = SetBrushOrgEx( hdc, 0, 0, &old );
    ok(ret, "Unexpected return value %d.\n", ret);

    ret = SetBrushOrgEx( hdc, 1, 1, &pt );
    ok(ret, "Unexpected return value %d.\n", ret);
    ok( pt.x == 0 && pt.y == 0, "got %ld,%ld\n", pt.x, pt.y );
    ret = SetBrushOrgEx( hdc, 0x10000, -1, &pt );
    ok(ret, "Unexpected return value %d.\n", ret);
    ok( pt.x == 1 && pt.y == 1, "got %ld,%ld\n", pt.x, pt.y );
    ret = SetBrushOrgEx( hdc, old.x, old.y, &pt );
    ok(ret, "Unexpected return value %d.\n", ret);
    ok( pt.x == 0x10000 && pt.y == -1, "got %ld,%ld\n", pt.x, pt.y );

    ret = GetBrushOrgEx( hdc, &pt );
    ok(ret, "Unexpected return value %d.\n", ret);
    ok( pt.x == 0 && pt.y == 0, "got %ld,%ld\n", pt.x, pt.y );
    pt.x = 10;
    pt.y = 20;
    ret = FixBrushOrgEx( hdc, 2, 3, &pt );
    todo_wine
    ok(!ret, "Unexpected return value %d.\n", ret);
    todo_wine
    ok( pt.x == 10 && pt.y == 20, "got %ld,%ld\n", pt.x, pt.y );
    ret = GetBrushOrgEx( hdc, &pt );
    ok(ret, "Unexpected return value %d.\n", ret);
    todo_wine
    ok( pt.x == 0 && pt.y == 0, "got %ld,%ld\n", pt.x, pt.y );

    ReleaseDC( 0, hdc );
}

static void test_null_brush(void)
{
    LOGBRUSH lb;
    HBRUSH brush;

    lb.lbStyle = BS_NULL;
    brush = CreateBrushIndirect(&lb);
    ok(brush == GetStockObject(NULL_BRUSH), "brush is not NULL_BRUSH\n");
}

START_TEST(brush)
{
    test_solidbrush();
    test_hatch_brush();
    test_pattern_brush();
    test_palette_brush();
    test_brush_org();
    test_null_brush();
}
