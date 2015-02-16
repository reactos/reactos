/*
 * Unit test suite for bitmaps
 *
 * Copyright 2004 Huw Davies
 * Copyright 2006 Dmitry Timoshkov
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
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wingdi.h"
#include "winuser.h"
#include "mmsystem.h"

#include "wine/test.h"

static BOOL (WINAPI *pGdiAlphaBlend)(HDC,int,int,int,int,HDC,int,int,int,int,BLENDFUNCTION);
static BOOL (WINAPI *pGdiGradientFill)(HDC,TRIVERTEX*,ULONG,void*,ULONG,ULONG);
static DWORD (WINAPI *pSetLayout)(HDC hdc, DWORD layout);

static inline int get_bitmap_stride( int width, int bpp )
{
    return ((width * bpp + 15) >> 3) & ~1;
}

static inline int get_dib_stride( int width, int bpp )
{
    return ((width * bpp + 31) >> 3) & ~3;
}

static inline int get_dib_image_size( const BITMAPINFO *info )
{
    return get_dib_stride( info->bmiHeader.biWidth, info->bmiHeader.biBitCount )
        * abs( info->bmiHeader.biHeight );
}

static void test_bitmap_info(HBITMAP hbm, INT expected_depth, const BITMAPINFOHEADER *bmih)
{
    BITMAP bm;
    BITMAP bma[2];
    INT ret, width_bytes;
    BYTE buf[512], buf_cmp[512];

    ret = GetObjectW(hbm, sizeof(bm), &bm);
    ok(ret == sizeof(bm), "GetObject returned %d\n", ret);

    ok(bm.bmType == 0, "wrong bm.bmType %d\n", bm.bmType);
    ok(bm.bmWidth == bmih->biWidth, "wrong bm.bmWidth %d\n", bm.bmWidth);
    ok(bm.bmHeight == bmih->biHeight, "wrong bm.bmHeight %d\n", bm.bmHeight);
    width_bytes = get_bitmap_stride(bm.bmWidth, bm.bmBitsPixel);
    ok(bm.bmWidthBytes == width_bytes, "wrong bm.bmWidthBytes %d != %d\n", bm.bmWidthBytes, width_bytes);
    ok(bm.bmPlanes == bmih->biPlanes, "wrong bm.bmPlanes %d\n", bm.bmPlanes);
    ok(bm.bmBitsPixel == expected_depth, "wrong bm.bmBitsPixel %d != %d\n", bm.bmBitsPixel, expected_depth);
    ok(bm.bmBits == NULL, "wrong bm.bmBits %p\n", bm.bmBits);

    assert(sizeof(buf) >= bm.bmWidthBytes * bm.bmHeight);
    assert(sizeof(buf) == sizeof(buf_cmp));

    SetLastError(0xdeadbeef);
    ret = GetBitmapBits(hbm, 0, NULL);
    ok(ret == bm.bmWidthBytes * bm.bmHeight, "%d != %d\n", ret, bm.bmWidthBytes * bm.bmHeight);

    memset(buf_cmp, 0xAA, sizeof(buf_cmp));
    memset(buf_cmp, 0, bm.bmWidthBytes * bm.bmHeight);

    memset(buf, 0xAA, sizeof(buf));
    ret = GetBitmapBits(hbm, sizeof(buf), buf);
    ok(ret == bm.bmWidthBytes * bm.bmHeight, "%d != %d\n", ret, bm.bmWidthBytes * bm.bmHeight);
    ok(!memcmp(buf, buf_cmp, sizeof(buf)),
        "buffers do not match, depth %d\n", bmih->biBitCount);

    /* test various buffer sizes for GetObject */
    ret = GetObjectW(hbm, sizeof(*bma) * 2, bma);
    ok(ret == sizeof(*bma), "wrong size %d\n", ret);

    ret = GetObjectW(hbm, sizeof(bm) / 2, &bm);
    ok(ret == 0, "%d != 0\n", ret);

    ret = GetObjectW(hbm, 0, &bm);
    ok(ret == 0, "%d != 0\n", ret);

    ret = GetObjectW(hbm, 1, &bm);
    ok(ret == 0, "%d != 0\n", ret);

    ret = GetObjectW(hbm, 0, NULL);
    ok(ret == sizeof(bm), "wrong size %d\n", ret);
}

static void test_createdibitmap(void)
{
    HDC hdc, hdcmem;
    BITMAPINFOHEADER bmih;
    BITMAPINFO bm;
    HBITMAP hbm, hbm_colour, hbm_old;
    INT screen_depth;
    DWORD pixel;

    hdc = GetDC(0);
    screen_depth = GetDeviceCaps(hdc, BITSPIXEL);
    memset(&bmih, 0, sizeof(bmih));
    bmih.biSize = sizeof(bmih);
    bmih.biWidth = 10;
    bmih.biHeight = 10;
    bmih.biPlanes = 1;
    bmih.biBitCount = 32;
    bmih.biCompression = BI_RGB;

    hbm = CreateDIBitmap(hdc, NULL, CBM_INIT, NULL, NULL, 0);
    ok(hbm == NULL, "CreateDIBitmap should fail\n");
    hbm = CreateDIBitmap(hdc, NULL, 0, NULL, NULL, 0);
    ok(hbm == NULL, "CreateDIBitmap should fail\n");

    /* First create an un-initialised bitmap.  The depth of the bitmap
       should match that of the hdc and not that supplied in bmih.
    */

    /* First try 32 bits */
    hbm = CreateDIBitmap(hdc, &bmih, 0, NULL, NULL, 0);
    ok(hbm != NULL, "CreateDIBitmap failed\n");
    test_bitmap_info(hbm, screen_depth, &bmih);
    DeleteObject(hbm);
    
    /* Then 16 */
    bmih.biBitCount = 16;
    hbm = CreateDIBitmap(hdc, &bmih, 0, NULL, NULL, 0);
    ok(hbm != NULL, "CreateDIBitmap failed\n");
    test_bitmap_info(hbm, screen_depth, &bmih);
    DeleteObject(hbm);

    /* Then 1 */
    bmih.biBitCount = 1;
    hbm = CreateDIBitmap(hdc, &bmih, 0, NULL, NULL, 0);
    ok(hbm != NULL, "CreateDIBitmap failed\n");
    test_bitmap_info(hbm, screen_depth, &bmih);
    DeleteObject(hbm);

    /* Now with a monochrome dc we expect a monochrome bitmap */
    hdcmem = CreateCompatibleDC(hdc);

    /* First try 32 bits */
    bmih.biBitCount = 32;
    hbm = CreateDIBitmap(hdcmem, &bmih, 0, NULL, NULL, 0);
    ok(hbm != NULL, "CreateDIBitmap failed\n");
    test_bitmap_info(hbm, 1, &bmih);
    DeleteObject(hbm);
    
    /* Then 16 */
    bmih.biBitCount = 16;
    hbm = CreateDIBitmap(hdcmem, &bmih, 0, NULL, NULL, 0);
    ok(hbm != NULL, "CreateDIBitmap failed\n");
    test_bitmap_info(hbm, 1, &bmih);
    DeleteObject(hbm);
    
    /* Then 1 */
    bmih.biBitCount = 1;
    hbm = CreateDIBitmap(hdcmem, &bmih, 0, NULL, NULL, 0);
    ok(hbm != NULL, "CreateDIBitmap failed\n");
    test_bitmap_info(hbm, 1, &bmih);
    DeleteObject(hbm);

    /* Now select a polychrome bitmap into the dc and we expect
       screen_depth bitmaps again */
    hbm_colour = CreateCompatibleBitmap(hdc, bmih.biWidth, bmih.biHeight);
    test_bitmap_info(hbm_colour, screen_depth, &bmih);
    hbm_old = SelectObject(hdcmem, hbm_colour);

    /* First try 32 bits */
    bmih.biBitCount = 32;
    hbm = CreateDIBitmap(hdcmem, &bmih, 0, NULL, NULL, 0);
    ok(hbm != NULL, "CreateDIBitmap failed\n");
    test_bitmap_info(hbm, screen_depth, &bmih);
    DeleteObject(hbm);
    
    /* Then 16 */
    bmih.biBitCount = 16;
    hbm = CreateDIBitmap(hdcmem, &bmih, 0, NULL, NULL, 0);
    ok(hbm != NULL, "CreateDIBitmap failed\n");
    test_bitmap_info(hbm, screen_depth, &bmih);
    DeleteObject(hbm);
    
    /* Then 1 */
    bmih.biBitCount = 1;
    hbm = CreateDIBitmap(hdcmem, &bmih, 0, NULL, NULL, 0);
    ok(hbm != NULL, "CreateDIBitmap failed\n");
    test_bitmap_info(hbm, screen_depth, &bmih);
    DeleteObject(hbm);

    SelectObject(hdcmem, hbm_old);
    DeleteObject(hbm_colour);
    DeleteDC(hdcmem);

    bmih.biBitCount = 32;
    hbm = CreateDIBitmap(0, &bmih, 0, NULL, NULL, 0);
    ok(hbm != NULL, "CreateDIBitmap failed\n");
    test_bitmap_info(hbm, 1, &bmih);
    DeleteObject(hbm);

    /* Test how formats are converted */
    pixel = 0xffffffff;
    bmih.biBitCount = 1;
    bmih.biWidth = 1;
    bmih.biHeight = 1;

    memset(&bm, 0, sizeof(bm));
    bm.bmiHeader.biSize = sizeof(bm.bmiHeader);
    bm.bmiHeader.biWidth = 1;
    bm.bmiHeader.biHeight = 1;
    bm.bmiHeader.biPlanes = 1;
    bm.bmiHeader.biBitCount= 24;
    bm.bmiHeader.biCompression= BI_RGB;
    bm.bmiHeader.biSizeImage = 0;
    hbm = CreateDIBitmap(hdc, &bmih, CBM_INIT, &pixel, &bm, DIB_RGB_COLORS);
    ok(hbm != NULL, "CreateDIBitmap failed\n");

    pixel = 0xdeadbeef;
    bm.bmiHeader.biBitCount= 32;
    GetDIBits(hdc, hbm, 0, 1, &pixel, &bm, DIB_RGB_COLORS);
    ok(pixel == 0x00ffffff, "Reading a 32 bit pixel from a DDB returned %08x\n", pixel);
    DeleteObject(hbm);

    ReleaseDC(0, hdc);
}

static void test_dib_info(HBITMAP hbm, const void *bits, const BITMAPINFOHEADER *bmih)
{
    BITMAP bm;
    BITMAP bma[2];
    DIBSECTION ds;
    DIBSECTION dsa[2];
    INT ret, bm_width_bytes, dib_width_bytes;
    BYTE *buf;

    ret = GetObjectW(hbm, sizeof(bm), &bm);
    ok(ret == sizeof(bm), "GetObject returned %d\n", ret);

    ok(bm.bmType == 0, "wrong bm.bmType %d\n", bm.bmType);
    ok(bm.bmWidth == bmih->biWidth, "wrong bm.bmWidth %d\n", bm.bmWidth);
    ok(bm.bmHeight == abs(bmih->biHeight), "wrong bm.bmHeight %d\n", bm.bmHeight);
    dib_width_bytes = get_dib_stride(bm.bmWidth, bm.bmBitsPixel);
    bm_width_bytes = get_bitmap_stride(bm.bmWidth, bm.bmBitsPixel);
    if (bm.bmWidthBytes != dib_width_bytes) /* Win2k bug */
        ok(bm.bmWidthBytes == bm_width_bytes, "wrong bm.bmWidthBytes %d != %d\n", bm.bmWidthBytes, bm_width_bytes);
    else
        ok(bm.bmWidthBytes == dib_width_bytes, "wrong bm.bmWidthBytes %d != %d\n", bm.bmWidthBytes, dib_width_bytes);
    ok(bm.bmPlanes == bmih->biPlanes, "wrong bm.bmPlanes %d\n", bm.bmPlanes);
    ok(bm.bmBitsPixel == bmih->biBitCount, "bm.bmBitsPixel %d != %d\n", bm.bmBitsPixel, bmih->biBitCount);
    ok(bm.bmBits == bits, "wrong bm.bmBits %p != %p\n", bm.bmBits, bits);

    buf = HeapAlloc(GetProcessHeap(), 0, bm.bmWidthBytes * bm.bmHeight + 4096);

    /* GetBitmapBits returns not 32-bit aligned data */
    SetLastError(0xdeadbeef);
    ret = GetBitmapBits(hbm, 0, NULL);
    ok(ret == bm_width_bytes * bm.bmHeight,
        "%d != %d\n", ret, bm_width_bytes * bm.bmHeight);

    memset(buf, 0xAA, bm.bmWidthBytes * bm.bmHeight + 4096);
    ret = GetBitmapBits(hbm, bm.bmWidthBytes * bm.bmHeight + 4096, buf);
    ok(ret == bm_width_bytes * bm.bmHeight, "%d != %d\n", ret, bm_width_bytes * bm.bmHeight);

    HeapFree(GetProcessHeap(), 0, buf);

    /* test various buffer sizes for GetObject */
    memset(&ds, 0xAA, sizeof(ds));
    ret = GetObjectW(hbm, sizeof(*bma) * 2, bma);
    ok(ret == sizeof(*bma), "wrong size %d\n", ret);
    ok(bm.bmWidth == bmih->biWidth, "wrong bm.bmWidth %d\n", bm.bmWidth);
    ok(bm.bmHeight == abs(bmih->biHeight), "wrong bm.bmHeight %d\n", bm.bmHeight);
    ok(bm.bmBits == bits, "wrong bm.bmBits %p != %p\n", bm.bmBits, bits);

    ret = GetObjectW(hbm, sizeof(bm) / 2, &bm);
    ok(ret == 0, "%d != 0\n", ret);

    ret = GetObjectW(hbm, 0, &bm);
    ok(ret == 0, "%d != 0\n", ret);

    ret = GetObjectW(hbm, 1, &bm);
    ok(ret == 0, "%d != 0\n", ret);

    /* test various buffer sizes for GetObject */
    ret = GetObjectW(hbm, 0, NULL);
    ok(ret == sizeof(bm), "wrong size %d\n", ret);

    ret = GetObjectW(hbm, sizeof(*dsa) * 2, dsa);
    ok(ret == sizeof(*dsa), "wrong size %d\n", ret);

    memset(&ds, 0xAA, sizeof(ds));
    ret = GetObjectW(hbm, sizeof(ds), &ds);
    ok(ret == sizeof(ds), "wrong size %d\n", ret);

    ok(ds.dsBm.bmBits == bits, "wrong bm.bmBits %p != %p\n", ds.dsBm.bmBits, bits);
    if (ds.dsBm.bmWidthBytes != bm_width_bytes) /* Win2k bug */
        ok(ds.dsBmih.biSizeImage == ds.dsBm.bmWidthBytes * ds.dsBm.bmHeight, "%u != %u\n",
           ds.dsBmih.biSizeImage, ds.dsBm.bmWidthBytes * ds.dsBm.bmHeight);
    ok(bmih->biSizeImage == 0, "%u != 0\n", bmih->biSizeImage);
    ds.dsBmih.biSizeImage = 0;

    ok(ds.dsBmih.biSize == bmih->biSize, "%u != %u\n", ds.dsBmih.biSize, bmih->biSize);
    ok(ds.dsBmih.biWidth == bmih->biWidth, "%d != %d\n", ds.dsBmih.biWidth, bmih->biWidth);
    ok(ds.dsBmih.biHeight == abs(bmih->biHeight), "%d != %d\n", ds.dsBmih.biHeight, abs(bmih->biHeight));
    ok(ds.dsBmih.biPlanes == bmih->biPlanes, "%u != %u\n", ds.dsBmih.biPlanes, bmih->biPlanes);
    ok(ds.dsBmih.biBitCount == bmih->biBitCount, "%u != %u\n", ds.dsBmih.biBitCount, bmih->biBitCount);
    ok(ds.dsBmih.biCompression == bmih->biCompression ||
       ((bmih->biBitCount == 32) && broken(ds.dsBmih.biCompression == BI_BITFIELDS)), /* nt4 sp1 and 2 */
       "%u != %u\n", ds.dsBmih.biCompression, bmih->biCompression);
    ok(ds.dsBmih.biSizeImage == bmih->biSizeImage, "%u != %u\n", ds.dsBmih.biSizeImage, bmih->biSizeImage);
    ok(ds.dsBmih.biXPelsPerMeter == bmih->biXPelsPerMeter, "%d != %d\n", ds.dsBmih.biXPelsPerMeter, bmih->biXPelsPerMeter);
    ok(ds.dsBmih.biYPelsPerMeter == bmih->biYPelsPerMeter, "%d != %d\n", ds.dsBmih.biYPelsPerMeter, bmih->biYPelsPerMeter);

    memset(&ds, 0xAA, sizeof(ds));
    ret = GetObjectW(hbm, sizeof(ds) - 4, &ds);
    ok(ret == sizeof(ds.dsBm), "wrong size %d\n", ret);
    ok(ds.dsBm.bmWidth == bmih->biWidth, "%d != %d\n", ds.dsBmih.biWidth, bmih->biWidth);
    ok(ds.dsBm.bmHeight == abs(bmih->biHeight), "%d != %d\n", ds.dsBmih.biHeight, abs(bmih->biHeight));
    ok(ds.dsBm.bmBits == bits, "%p != %p\n", ds.dsBm.bmBits, bits);

    ret = GetObjectW(hbm, 0, &ds);
    ok(ret == 0, "%d != 0\n", ret);

    ret = GetObjectW(hbm, 1, &ds);
    ok(ret == 0, "%d != 0\n", ret);
}

static void _test_color( int line, HDC hdc, COLORREF color, COLORREF exp )
{
    COLORREF c;
    c = SetPixel(hdc, 0, 0, color);
    ok_(__FILE__, line)(c == exp, "SetPixel failed: got 0x%06x expected 0x%06x\n", c, exp);
    c = GetPixel(hdc, 0, 0);
    ok_(__FILE__, line)(c == exp, "GetPixel failed: got 0x%06x expected 0x%06x\n", c, exp);
    c = GetNearestColor(hdc, color);
    ok_(__FILE__, line)(c == exp, "GetNearestColor failed: got 0x%06x expected 0x%06x\n", c, exp);
}
#define test_color(hdc, color, exp) _test_color( __LINE__, hdc, color, exp )


static void test_dib_bits_access( HBITMAP hdib, void *bits )
{
    MEMORY_BASIC_INFORMATION info;
    char bmibuf[sizeof(BITMAPINFO) + 256 * sizeof(RGBQUAD)];
    DWORD data[256];
    BITMAPINFO *pbmi = (BITMAPINFO *)bmibuf;
    HDC hdc;
    char filename[MAX_PATH];
    HANDLE file;
    DWORD written;
    INT ret;

    ok(VirtualQuery(bits, &info, sizeof(info)) == sizeof(info),
        "VirtualQuery failed\n");
    ok(info.BaseAddress == bits, "%p != %p\n", info.BaseAddress, bits);
    ok(info.AllocationBase == bits, "%p != %p\n", info.AllocationBase, bits);
    ok(info.AllocationProtect == PAGE_READWRITE, "%x != PAGE_READWRITE\n", info.AllocationProtect);
    ok(info.State == MEM_COMMIT, "%x != MEM_COMMIT\n", info.State);
    ok(info.Protect == PAGE_READWRITE, "%x != PAGE_READWRITE\n", info.Protect);
    ok(info.Type == MEM_PRIVATE, "%x != MEM_PRIVATE\n", info.Type);

    memset( pbmi, 0, sizeof(bmibuf) );
    memset( data, 0xcc, sizeof(data) );
    pbmi->bmiHeader.biSize = sizeof(pbmi->bmiHeader);
    pbmi->bmiHeader.biHeight = 16;
    pbmi->bmiHeader.biWidth = 16;
    pbmi->bmiHeader.biBitCount = 32;
    pbmi->bmiHeader.biPlanes = 1;
    pbmi->bmiHeader.biCompression = BI_RGB;

    hdc = GetDC(0);

    ret = SetDIBits( hdc, hdib, 0, 16, data, pbmi, DIB_RGB_COLORS );
    ok(ret == 16, "SetDIBits failed: expected 16 got %d\n", ret);

    ReleaseDC(0, hdc);

    ok(VirtualQuery(bits, &info, sizeof(info)) == sizeof(info),
        "VirtualQuery failed\n");
    ok(info.BaseAddress == bits, "%p != %p\n", info.BaseAddress, bits);
    ok(info.AllocationBase == bits, "%p != %p\n", info.AllocationBase, bits);
    ok(info.AllocationProtect == PAGE_READWRITE, "%x != PAGE_READWRITE\n", info.AllocationProtect);
    ok(info.State == MEM_COMMIT, "%x != MEM_COMMIT\n", info.State);
    ok(info.Type == MEM_PRIVATE, "%x != MEM_PRIVATE\n", info.Type);
    ok(info.Protect == PAGE_READWRITE, "%x != PAGE_READWRITE\n", info.Protect);

    /* try writing protected bits to a file */

    GetTempFileNameA( ".", "dib", 0, filename );
    file = CreateFileA( filename, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
                        CREATE_ALWAYS, 0, 0 );
    ok( file != INVALID_HANDLE_VALUE, "failed to open %s error %u\n", filename, GetLastError() );
    ret = WriteFile( file, bits, 8192, &written, NULL );
    ok( ret, "WriteFile failed error %u\n", GetLastError() );
    if (ret) ok( written == 8192, "only wrote %u bytes\n", written );
    CloseHandle( file );
    DeleteFileA( filename );
}

static void test_dibsections(void)
{
    HDC hdc, hdcmem, hdcmem2;
    HBITMAP hdib, oldbm, hdib2, oldbm2;
    char bmibuf[sizeof(BITMAPINFO) + 256 * sizeof(RGBQUAD)];
    char bcibuf[sizeof(BITMAPCOREINFO) + 256 * sizeof(RGBTRIPLE)];
    BITMAPINFO *pbmi = (BITMAPINFO *)bmibuf;
    BITMAPCOREINFO *pbci = (BITMAPCOREINFO *)bcibuf;
    RGBQUAD *colors = pbmi->bmiColors;
    RGBTRIPLE *ccolors = pbci->bmciColors;
    HBITMAP hcoredib;
    char coreBits[256];
    BYTE *bits;
    RGBQUAD rgb[256];
    int ret;
    char logpalbuf[sizeof(LOGPALETTE) + 256 * sizeof(PALETTEENTRY)];
    LOGPALETTE *plogpal = (LOGPALETTE*)logpalbuf;
    PALETTEENTRY *palent = plogpal->palPalEntry;
    WORD *index;
    DWORD *bits32;
    HPALETTE hpal, oldpal;
    DIBSECTION dibsec;
    COLORREF c0, c1;
    int i;
    MEMORY_BASIC_INFORMATION info;

    hdc = GetDC(0);

    memset(pbmi, 0, sizeof(bmibuf));
    pbmi->bmiHeader.biSize = sizeof(pbmi->bmiHeader);
    pbmi->bmiHeader.biHeight = 100;
    pbmi->bmiHeader.biWidth = 512;
    pbmi->bmiHeader.biBitCount = 24;
    pbmi->bmiHeader.biPlanes = 1;
    pbmi->bmiHeader.biCompression = BI_RGB;

    SetLastError(0xdeadbeef);

    /* invalid pointer for BITMAPINFO
       (*bits should be NULL on error) */
    bits = (BYTE*)0xdeadbeef;
    hdib = CreateDIBSection(hdc, NULL, DIB_RGB_COLORS, (void**)&bits, NULL, 0);
    ok(hdib == NULL && bits == NULL, "CreateDIBSection failed for invalid parameter: bmi == 0x0\n");

    hdib = CreateDIBSection(hdc, pbmi, DIB_RGB_COLORS, (void**)&bits, NULL, 0);
    ok(hdib != NULL, "CreateDIBSection error %d\n", GetLastError());
    ok(GetObjectW(hdib, sizeof(DIBSECTION), &dibsec) != 0, "GetObject failed for DIBSection\n");
    ok(dibsec.dsBm.bmBits == bits, "dibsec.dsBits %p != bits %p\n", dibsec.dsBm.bmBits, bits);

    /* test the DIB memory */
    ok(VirtualQuery(bits, &info, sizeof(info)) == sizeof(info),
        "VirtualQuery failed\n");
    ok(info.BaseAddress == bits, "%p != %p\n", info.BaseAddress, bits);
    ok(info.AllocationBase == bits, "%p != %p\n", info.AllocationBase, bits);
    ok(info.AllocationProtect == PAGE_READWRITE, "%x != PAGE_READWRITE\n", info.AllocationProtect);
    ok(info.RegionSize == 0x26000, "0x%lx != 0x26000\n", info.RegionSize);
    ok(info.State == MEM_COMMIT, "%x != MEM_COMMIT\n", info.State);
    ok(info.Protect == PAGE_READWRITE, "%x != PAGE_READWRITE\n", info.Protect);
    ok(info.Type == MEM_PRIVATE, "%x != MEM_PRIVATE\n", info.Type);

    test_dib_bits_access( hdib, bits );

    test_dib_info(hdib, bits, &pbmi->bmiHeader);
    DeleteObject(hdib);

    /* Test a top-down DIB. */
    pbmi->bmiHeader.biHeight = -100;
    hdib = CreateDIBSection(hdc, pbmi, DIB_RGB_COLORS, (void**)&bits, NULL, 0);
    ok(hdib != NULL, "CreateDIBSection error %d\n", GetLastError());
    test_dib_info(hdib, bits, &pbmi->bmiHeader);
    DeleteObject(hdib);

    pbmi->bmiHeader.biHeight = 100;
    pbmi->bmiHeader.biBitCount = 8;
    pbmi->bmiHeader.biCompression = BI_RLE8;
    SetLastError(0xdeadbeef);
    hdib = CreateDIBSection(hdc, pbmi, DIB_RGB_COLORS, (void**)&bits, NULL, 0);
    ok(hdib == NULL, "CreateDIBSection should fail when asked to create a compressed DIB section\n");
    ok(GetLastError() == 0xdeadbeef, "wrong error %d\n", GetLastError());

    pbmi->bmiHeader.biBitCount = 16;
    pbmi->bmiHeader.biCompression = BI_BITFIELDS;
    ((PDWORD)pbmi->bmiColors)[0] = 0xf800;
    ((PDWORD)pbmi->bmiColors)[1] = 0x07e0;
    ((PDWORD)pbmi->bmiColors)[2] = 0x001f;
    SetLastError(0xdeadbeef);
    hdib = CreateDIBSection(hdc, pbmi, DIB_RGB_COLORS, (void**)&bits, NULL, 0);
    ok(hdib != NULL, "CreateDIBSection error %d\n", GetLastError());

    /* test the DIB memory */
    ok(VirtualQuery(bits, &info, sizeof(info)) == sizeof(info),
        "VirtualQuery failed\n");
    ok(info.BaseAddress == bits, "%p != %p\n", info.BaseAddress, bits);
    ok(info.AllocationBase == bits, "%p != %p\n", info.AllocationBase, bits);
    ok(info.AllocationProtect == PAGE_READWRITE, "%x != PAGE_READWRITE\n", info.AllocationProtect);
    ok(info.RegionSize == 0x19000, "0x%lx != 0x19000\n", info.RegionSize);
    ok(info.State == MEM_COMMIT, "%x != MEM_COMMIT\n", info.State);
    ok(info.Protect == PAGE_READWRITE, "%x != PAGE_READWRITE\n", info.Protect);
    ok(info.Type == MEM_PRIVATE, "%x != MEM_PRIVATE\n", info.Type);

    test_dib_info(hdib, bits, &pbmi->bmiHeader);
    DeleteObject(hdib);

    memset(pbmi, 0, sizeof(bmibuf));
    pbmi->bmiHeader.biSize = sizeof(pbmi->bmiHeader);
    pbmi->bmiHeader.biHeight = 16;
    pbmi->bmiHeader.biWidth = 16;
    pbmi->bmiHeader.biBitCount = 1;
    pbmi->bmiHeader.biPlanes = 1;
    pbmi->bmiHeader.biCompression = BI_RGB;
    colors[0].rgbRed = 0xff;
    colors[0].rgbGreen = 0;
    colors[0].rgbBlue = 0;
    colors[1].rgbRed = 0;
    colors[1].rgbGreen = 0;
    colors[1].rgbBlue = 0xff;

    hdib = CreateDIBSection(hdc, pbmi, DIB_RGB_COLORS, (void**)&bits, NULL, 0);
    ok(hdib != NULL, "CreateDIBSection failed\n");
    ok(GetObjectW(hdib, sizeof(DIBSECTION), &dibsec) != 0, "GetObject failed for DIBSection\n");
    ok(dibsec.dsBmih.biClrUsed == 2,
        "created DIBSection: wrong biClrUsed field: %u, should be: %u\n", dibsec.dsBmih.biClrUsed, 2);

    /* Test if the old BITMAPCOREINFO structure is supported */

    pbci->bmciHeader.bcSize = sizeof(BITMAPCOREHEADER);
    pbci->bmciHeader.bcBitCount = 0;

    ret = GetDIBits(hdc, hdib, 0, 16, NULL, (BITMAPINFO*) pbci, DIB_RGB_COLORS);
    ok(ret, "GetDIBits doesn't work with a BITMAPCOREHEADER\n");
    ok((pbci->bmciHeader.bcWidth == 16) && (pbci->bmciHeader.bcHeight == 16)
        && (pbci->bmciHeader.bcBitCount == 1) && (pbci->bmciHeader.bcPlanes == 1),
    "GetDIBits didn't fill in the BITMAPCOREHEADER structure properly\n");

    ret = GetDIBits(hdc, hdib, 0, 16, &coreBits, (BITMAPINFO*) pbci, DIB_RGB_COLORS);
    ok(ret, "GetDIBits doesn't work with a BITMAPCOREHEADER\n");
    ok((ccolors[0].rgbtRed == 0xff) && (ccolors[0].rgbtGreen == 0) &&
        (ccolors[0].rgbtBlue == 0) && (ccolors[1].rgbtRed == 0) &&
        (ccolors[1].rgbtGreen == 0) && (ccolors[1].rgbtBlue == 0xff),
        "The color table has not been translated to the old BITMAPCOREINFO format\n");

    hcoredib = CreateDIBSection(hdc, (BITMAPINFO*) pbci, DIB_RGB_COLORS, (void**)&bits, NULL, 0);
    ok(hcoredib != NULL, "CreateDIBSection failed with a BITMAPCOREINFO\n");

    ZeroMemory(ccolors, 256 * sizeof(RGBTRIPLE));
    ret = GetDIBits(hdc, hcoredib, 0, 16, &coreBits, (BITMAPINFO*) pbci, DIB_RGB_COLORS);
    ok(ret, "GetDIBits doesn't work with a BITMAPCOREHEADER\n");
    ok((ccolors[0].rgbtRed == 0xff) && (ccolors[0].rgbtGreen == 0) &&
        (ccolors[0].rgbtBlue == 0) && (ccolors[1].rgbtRed == 0) &&
        (ccolors[1].rgbtGreen == 0) && (ccolors[1].rgbtBlue == 0xff),
        "The color table has not been translated to the old BITMAPCOREINFO format\n");

    DeleteObject(hcoredib);

    hdcmem = CreateCompatibleDC(hdc);
    oldbm = SelectObject(hdcmem, hdib);

    ret = GetDIBColorTable(hdcmem, 0, 2, rgb);
    ok(ret == 2, "GetDIBColorTable returned %d\n", ret);
    ok(!memcmp(rgb, pbmi->bmiColors, 2 * sizeof(RGBQUAD)),
       "GetDIBColorTable returns table 0: r%02x g%02x b%02x res%02x 1: r%02x g%02x b%02x res%02x\n",
       rgb[0].rgbRed, rgb[0].rgbGreen, rgb[0].rgbBlue, rgb[0].rgbReserved,
       rgb[1].rgbRed, rgb[1].rgbGreen, rgb[1].rgbBlue, rgb[1].rgbReserved);

    c0 = RGB(colors[0].rgbRed, colors[0].rgbGreen, colors[0].rgbBlue);
    c1 = RGB(colors[1].rgbRed, colors[1].rgbGreen, colors[1].rgbBlue);

    test_color(hdcmem, DIBINDEX(0), c0);
    test_color(hdcmem, DIBINDEX(1), c1);
    test_color(hdcmem, DIBINDEX(2), c0);
    test_color(hdcmem, PALETTEINDEX(0), c0);
    test_color(hdcmem, PALETTEINDEX(1), c0);
    test_color(hdcmem, PALETTEINDEX(2), c0);
    test_color(hdcmem, PALETTERGB(colors[0].rgbRed, colors[0].rgbGreen, colors[0].rgbBlue), c0);
    test_color(hdcmem, PALETTERGB(colors[1].rgbRed, colors[1].rgbGreen, colors[1].rgbBlue), c1);
    test_color(hdcmem, PALETTERGB(0, 0, 0), c0);
    test_color(hdcmem, PALETTERGB(0xff, 0xff, 0xff), c0);
    test_color(hdcmem, PALETTERGB(0, 0, 0xfe), c1);

    SelectObject(hdcmem, oldbm);
    DeleteObject(hdib);

    colors[0].rgbRed = 0xff;
    colors[0].rgbGreen = 0xff;
    colors[0].rgbBlue = 0xff;
    colors[1].rgbRed = 0;
    colors[1].rgbGreen = 0;
    colors[1].rgbBlue = 0;

    hdib = CreateDIBSection(hdc, pbmi, DIB_RGB_COLORS, (void**)&bits, NULL, 0);
    ok(hdib != NULL, "CreateDIBSection failed\n");

    test_dib_info(hdib, bits, &pbmi->bmiHeader);

    oldbm = SelectObject(hdcmem, hdib);

    ret = GetDIBColorTable(hdcmem, 0, 2, rgb);
    ok(ret == 2, "GetDIBColorTable returned %d\n", ret);
    ok(!memcmp(rgb, colors, 2 * sizeof(RGBQUAD)),
       "GetDIBColorTable returns table 0: r%02x g%02x b%02x res%02x 1: r%02x g%02x b%02x res%02x\n",
       rgb[0].rgbRed, rgb[0].rgbGreen, rgb[0].rgbBlue, rgb[0].rgbReserved,
       rgb[1].rgbRed, rgb[1].rgbGreen, rgb[1].rgbBlue, rgb[1].rgbReserved);

    SelectObject(hdcmem, oldbm);
    test_dib_info(hdib, bits, &pbmi->bmiHeader);
    DeleteObject(hdib);

    pbmi->bmiHeader.biBitCount = 4;
    for (i = 0; i < 16; i++) {
        colors[i].rgbRed = i;
        colors[i].rgbGreen = 16-i;
        colors[i].rgbBlue = 0;
    }
    hdib = CreateDIBSection(hdcmem, pbmi, DIB_RGB_COLORS, (void**)&bits, NULL, 0);
    ok(hdib != NULL, "CreateDIBSection failed\n");
    ok(GetObjectW(hdib, sizeof(DIBSECTION), &dibsec) != 0, "GetObject failed for DIB Section\n");
    ok(dibsec.dsBmih.biClrUsed == 16,
       "created DIBSection: wrong biClrUsed field: %u, should be: %u\n", dibsec.dsBmih.biClrUsed, 16);
    test_dib_info(hdib, bits, &pbmi->bmiHeader);
    DeleteObject(hdib);

    pbmi->bmiHeader.biBitCount = 8;

    for (i = 0; i < 128; i++) {
        colors[i].rgbRed = 255 - i * 2;
        colors[i].rgbGreen = i * 2;
        colors[i].rgbBlue = 0;
        colors[255 - i].rgbRed = 0;
        colors[255 - i].rgbGreen = i * 2;
        colors[255 - i].rgbBlue = 255 - i * 2;
    }
    hdib = CreateDIBSection(hdcmem, pbmi, DIB_RGB_COLORS, (void**)&bits, NULL, 0);
    ok(hdib != NULL, "CreateDIBSection failed\n");
    ok(GetObjectW(hdib, sizeof(DIBSECTION), &dibsec) != 0, "GetObject failed for DIB Section\n");
    ok(dibsec.dsBmih.biClrUsed == 256,
        "created DIBSection: wrong biClrUsed field: %u, should be: %u\n", dibsec.dsBmih.biClrUsed, 256);

    oldbm = SelectObject(hdcmem, hdib);

    for (i = 0; i < 256; i++) {
        test_color(hdcmem, DIBINDEX(i), RGB(colors[i].rgbRed, colors[i].rgbGreen, colors[i].rgbBlue));
        test_color(hdcmem, PALETTERGB(colors[i].rgbRed, colors[i].rgbGreen, colors[i].rgbBlue),
                   RGB(colors[i].rgbRed, colors[i].rgbGreen, colors[i].rgbBlue));
    }

    SelectObject(hdcmem, oldbm);
    test_dib_info(hdib, bits, &pbmi->bmiHeader);
    DeleteObject(hdib);

    pbmi->bmiHeader.biBitCount = 1;

    /* Now create a palette and a palette indexed dib section */
    memset(plogpal, 0, sizeof(logpalbuf));
    plogpal->palVersion = 0x300;
    plogpal->palNumEntries = 2;
    palent[0].peRed = 0xff;
    palent[0].peBlue = 0xff;
    palent[1].peGreen = 0xff;

    index = (WORD*)pbmi->bmiColors;
    *index++ = 0;
    *index = 1;
    hpal = CreatePalette(plogpal);
    ok(hpal != NULL, "CreatePalette failed\n");
    oldpal = SelectPalette(hdc, hpal, TRUE);
    hdib = CreateDIBSection(hdc, pbmi, DIB_PAL_COLORS, (void**)&bits, NULL, 0);
    ok(hdib != NULL, "CreateDIBSection failed\n");
    ok(GetObjectW(hdib, sizeof(DIBSECTION), &dibsec) != 0, "GetObject failed for DIB Section\n");
    ok(dibsec.dsBmih.biClrUsed == 2, "created DIBSection: wrong biClrUsed field: %u, should be: %u\n", dibsec.dsBmih.biClrUsed, 2);

    /* The colour table has already been grabbed from the dc, so we select back the
       old palette */

    SelectPalette(hdc, oldpal, TRUE);
    oldbm = SelectObject(hdcmem, hdib);
    oldpal = SelectPalette(hdcmem, hpal, TRUE);

    ret = GetDIBColorTable(hdcmem, 0, 2, rgb);
    ok(ret == 2, "GetDIBColorTable returned %d\n", ret);
    ok(rgb[0].rgbRed == 0xff && rgb[0].rgbBlue == 0xff && rgb[0].rgbGreen == 0 &&
       rgb[1].rgbRed == 0    && rgb[1].rgbBlue == 0    && rgb[1].rgbGreen == 0xff,
       "GetDIBColorTable returns table 0: r%02x g%02x b%02x res%02x 1: r%02x g%02x b%02x res%02x\n",
       rgb[0].rgbRed, rgb[0].rgbGreen, rgb[0].rgbBlue, rgb[0].rgbReserved,
       rgb[1].rgbRed, rgb[1].rgbGreen, rgb[1].rgbBlue, rgb[1].rgbReserved);

    c0 = RGB(palent[0].peRed, palent[0].peGreen, palent[0].peBlue);
    c1 = RGB(palent[1].peRed, palent[1].peGreen, palent[1].peBlue);

    test_color(hdcmem, DIBINDEX(0), c0);
    test_color(hdcmem, DIBINDEX(1), c1);
    test_color(hdcmem, DIBINDEX(2), c0);
    test_color(hdcmem, PALETTEINDEX(0), c0);
    test_color(hdcmem, PALETTEINDEX(1), c1);
    test_color(hdcmem, PALETTEINDEX(2), c0);
    test_color(hdcmem, PALETTERGB(palent[0].peRed, palent[0].peGreen, palent[0].peBlue), c0);
    test_color(hdcmem, PALETTERGB(palent[1].peRed, palent[1].peGreen, palent[1].peBlue), c1);
    test_color(hdcmem, PALETTERGB(0, 0, 0), c1);
    test_color(hdcmem, PALETTERGB(0xff, 0xff, 0xff), c0);
    test_color(hdcmem, PALETTERGB(0, 0, 0xfe), c0);
    test_color(hdcmem, PALETTERGB(0, 1, 0), c1);
    test_color(hdcmem, PALETTERGB(0x3f, 0, 0x3f), c1);
    test_color(hdcmem, PALETTERGB(0x40, 0, 0x40), c0);

    /* Bottom and 2nd row from top green, everything else magenta */
    bits[0] = bits[1] = 0xff;
    bits[13 * 4] = bits[13*4 + 1] = 0xff;

    test_dib_info(hdib, bits, &pbmi->bmiHeader);

    pbmi->bmiHeader.biBitCount = 32;

    hdib2 = CreateDIBSection(NULL, pbmi, DIB_RGB_COLORS, (void **)&bits32, NULL, 0);
    ok(hdib2 != NULL, "CreateDIBSection failed\n");
    hdcmem2 = CreateCompatibleDC(hdc);
    oldbm2 = SelectObject(hdcmem2, hdib2);

    BitBlt(hdcmem2, 0, 0, 16,16, hdcmem, 0, 0, SRCCOPY);

    ok(bits32[0] == 0xff00, "lower left pixel is %08x\n", bits32[0]);
    ok(bits32[17] == 0xff00ff, "bottom but one, left pixel is %08x\n", bits32[17]);

    SelectObject(hdcmem2, oldbm2);
    test_dib_info(hdib2, bits32, &pbmi->bmiHeader);
    DeleteObject(hdib2);

    SelectObject(hdcmem, oldbm);
    SelectPalette(hdcmem, oldpal, TRUE);
    DeleteObject(hdib);
    DeleteObject(hpal);


    pbmi->bmiHeader.biBitCount = 8;

    memset(plogpal, 0, sizeof(logpalbuf));
    plogpal->palVersion = 0x300;
    plogpal->palNumEntries = 256;

    for (i = 0; i < 128; i++) {
        palent[i].peRed = 255 - i * 2;
        palent[i].peBlue = i * 2;
        palent[i].peGreen = 0;
        palent[255 - i].peRed = 0;
        palent[255 - i].peGreen = i * 2;
        palent[255 - i].peBlue = 255 - i * 2;
    }

    index = (WORD*)pbmi->bmiColors;
    for (i = 0; i < 256; i++) {
        *index++ = i;
    }

    hpal = CreatePalette(plogpal);
    ok(hpal != NULL, "CreatePalette failed\n");
    oldpal = SelectPalette(hdc, hpal, TRUE);
    hdib = CreateDIBSection(hdc, pbmi, DIB_PAL_COLORS, (void**)&bits, NULL, 0);
    ok(hdib != NULL, "CreateDIBSection failed\n");
    ok(GetObjectW(hdib, sizeof(DIBSECTION), &dibsec) != 0, "GetObject failed for DIB Section\n");
    ok(dibsec.dsBmih.biClrUsed == 256, "created DIBSection: wrong biClrUsed field: %u, should be: %u\n", dibsec.dsBmih.biClrUsed, 256);

    test_dib_info(hdib, bits, &pbmi->bmiHeader);

    SelectPalette(hdc, oldpal, TRUE);
    oldbm = SelectObject(hdcmem, hdib);
    oldpal = SelectPalette(hdcmem, hpal, TRUE);

    ret = GetDIBColorTable(hdcmem, 0, 256, rgb);
    ok(ret == 256, "GetDIBColorTable returned %d\n", ret);
    for (i = 0; i < 256; i++) {
        ok(rgb[i].rgbRed == palent[i].peRed && 
            rgb[i].rgbBlue == palent[i].peBlue && 
            rgb[i].rgbGreen == palent[i].peGreen, 
            "GetDIBColorTable returns table %d: r%02x g%02x b%02x res%02x\n",
            i, rgb[i].rgbRed, rgb[i].rgbGreen, rgb[i].rgbBlue, rgb[i].rgbReserved);
    }

    for (i = 0; i < 256; i++) {
        test_color(hdcmem, DIBINDEX(i), RGB(palent[i].peRed, palent[i].peGreen, palent[i].peBlue));
        test_color(hdcmem, PALETTEINDEX(i), RGB(palent[i].peRed, palent[i].peGreen, palent[i].peBlue));
        test_color(hdcmem, PALETTERGB(palent[i].peRed, palent[i].peGreen, palent[i].peBlue), 
                   RGB(palent[i].peRed, palent[i].peGreen, palent[i].peBlue));
    }

    SelectPalette(hdcmem, oldpal, TRUE);
    SelectObject(hdcmem, oldbm);
    DeleteObject(hdib);
    DeleteObject(hpal);

    plogpal->palNumEntries = 37;
    hpal = CreatePalette(plogpal);
    ok(hpal != NULL, "CreatePalette failed\n");
    oldpal = SelectPalette(hdc, hpal, TRUE);
    pbmi->bmiHeader.biClrUsed = 142;
    hdib = CreateDIBSection(hdc, pbmi, DIB_PAL_COLORS, (void**)&bits, NULL, 0);
    ok(hdib != NULL, "CreateDIBSection failed\n");
    ok(GetObjectW(hdib, sizeof(DIBSECTION), &dibsec) != 0, "GetObject failed for DIB Section\n");
    ok(dibsec.dsBmih.biClrUsed == 256, "created DIBSection: wrong biClrUsed field: %u, should be: %u\n", dibsec.dsBmih.biClrUsed, 256);

    test_dib_info(hdib, bits, &pbmi->bmiHeader);

    SelectPalette(hdc, oldpal, TRUE);
    oldbm = SelectObject(hdcmem, hdib);

    memset( rgb, 0xcc, sizeof(rgb) );
    ret = GetDIBColorTable(hdcmem, 0, 256, rgb);
    ok(ret == 256, "GetDIBColorTable returned %d\n", ret);
    for (i = 0; i < 256; i++)
    {
        if (i < pbmi->bmiHeader.biClrUsed)
        {
            ok(rgb[i].rgbRed == palent[i % 37].peRed &&
               rgb[i].rgbBlue == palent[i % 37].peBlue &&
               rgb[i].rgbGreen == palent[i % 37].peGreen,
               "GetDIBColorTable returns table %d: r %02x g %02x b %02x res%02x\n",
               i, rgb[i].rgbRed, rgb[i].rgbGreen, rgb[i].rgbBlue, rgb[i].rgbReserved);
            test_color(hdcmem, DIBINDEX(i),
                       RGB(palent[i % 37].peRed, palent[i % 37].peGreen, palent[i % 37].peBlue));
        }
        else
        {
            ok(rgb[i].rgbRed == 0 && rgb[i].rgbBlue == 0 && rgb[i].rgbGreen == 0,
               "GetDIBColorTable returns table %d: r %02x g %02x b %02x res%02x\n",
               i, rgb[i].rgbRed, rgb[i].rgbGreen, rgb[i].rgbBlue, rgb[i].rgbReserved);
            test_color(hdcmem, DIBINDEX(i), 0 );
        }
    }
    pbmi->bmiHeader.biClrUsed = 173;
    memset( pbmi->bmiColors, 0xcc, 256 * sizeof(RGBQUAD) );
    GetDIBits( hdc, hdib, 0, 1, NULL, pbmi, DIB_RGB_COLORS );
    ok( pbmi->bmiHeader.biClrUsed == 0, "wrong colors %u\n", pbmi->bmiHeader.biClrUsed );
    for (i = 0; i < 256; i++)
    {
        if (i < 142)
            ok(colors[i].rgbRed == palent[i % 37].peRed &&
               colors[i].rgbBlue == palent[i % 37].peBlue &&
               colors[i].rgbGreen == palent[i % 37].peGreen,
               "GetDIBits returns table %d: r %02x g %02x b %02x res%02x\n",
               i, colors[i].rgbRed, colors[i].rgbGreen, colors[i].rgbBlue, colors[i].rgbReserved);
        else
            ok(colors[i].rgbRed == 0 && colors[i].rgbBlue == 0 && colors[i].rgbGreen == 0,
               "GetDIBits returns table %d: r %02x g %02x b %02x res%02x\n",
               i, colors[i].rgbRed, colors[i].rgbGreen, colors[i].rgbBlue, colors[i].rgbReserved);
    }

    rgb[0].rgbRed = 1;
    rgb[0].rgbGreen = 2;
    rgb[0].rgbBlue = 3;
    rgb[0].rgbReserved = 123;
    ret = SetDIBColorTable( hdcmem, 0, 1, rgb );
    ok( ret == 1, "SetDIBColorTable returned unexpected result %u\n", ret );
    ok( rgb[0].rgbReserved == 123, "Expected rgbReserved = 123, got %u\n", rgb[0].rgbReserved );

    ret = GetDIBColorTable( hdcmem, 0, 1, rgb );
    ok( ret == 1, "GetDIBColorTable returned unexpected result %u\n", ret );
    ok( rgb[0].rgbRed == 1, "Expected rgbRed = 1, got %u\n", rgb[0].rgbRed );
    ok( rgb[0].rgbGreen == 2, "Expected rgbGreen = 2, got %u\n", rgb[0].rgbGreen );
    ok( rgb[0].rgbBlue == 3, "Expected rgbBlue = 3, got %u\n", rgb[0].rgbBlue );
    todo_wine ok( rgb[0].rgbReserved == 0, "Expected rgbReserved = 0, got %u\n", rgb[0].rgbReserved );

    SelectObject(hdcmem, oldbm);
    DeleteObject(hdib);
    DeleteObject(hpal);

    /* ClrUsed ignored on > 8bpp */
    pbmi->bmiHeader.biBitCount = 16;
    pbmi->bmiHeader.biClrUsed = 37;
    hdib = CreateDIBSection(hdc, pbmi, DIB_PAL_COLORS, (void**)&bits, NULL, 0);
    ok(hdib != NULL, "CreateDIBSection failed\n");
    ok(GetObjectW(hdib, sizeof(DIBSECTION), &dibsec) != 0, "GetObject failed for DIB Section\n");
    ok(dibsec.dsBmih.biClrUsed == 0, "created DIBSection: wrong biClrUsed field: %u\n", dibsec.dsBmih.biClrUsed);
    oldbm = SelectObject(hdcmem, hdib);
    ret = GetDIBColorTable(hdcmem, 0, 256, rgb);
    ok(ret == 0, "GetDIBColorTable returned %d\n", ret);
    SelectObject(hdcmem, oldbm);
    DeleteObject(hdib);

    DeleteDC(hdcmem);
    DeleteDC(hdcmem2);
    ReleaseDC(0, hdc);
}

static void test_dib_formats(void)
{
    BITMAPINFO *bi;
    char data[256];
    void *bits;
    int planes, bpp, compr, format;
    HBITMAP hdib, hbmp;
    HDC hdc, memdc;
    UINT ret;
    BOOL format_ok, expect_ok;

    bi = HeapAlloc( GetProcessHeap(), 0, FIELD_OFFSET( BITMAPINFO, bmiColors[256] ) );
    hdc = GetDC( 0 );
    memdc = CreateCompatibleDC( 0 );
    hbmp = CreateCompatibleBitmap( hdc, 10, 10 );

    memset( data, 0xaa, sizeof(data) );

    if (!winetest_interactive)
        skip("ROSTESTS-152: Skipping loop in test_dib_formats because it's too big and causes too many failures\n");
    else
    for (bpp = 0; bpp <= 64; bpp++)
    {
        for (planes = 0; planes <= 64; planes++)
        {
            for (compr = 0; compr < 8; compr++)
            {
                for (format = DIB_RGB_COLORS; format <= DIB_PAL_COLORS; format++)
                {
                    switch (bpp)
                    {
                    case 1:
                    case 4:
                    case 8:
                    case 24: expect_ok = (compr == BI_RGB); break;
                    case 16:
                    case 32: expect_ok = (compr == BI_RGB || compr == BI_BITFIELDS); break;
                    default: expect_ok = FALSE; break;
                    }

                    memset( bi, 0, sizeof(bi->bmiHeader) );
                    bi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                    bi->bmiHeader.biWidth = 2;
                    bi->bmiHeader.biHeight = 2;
                    bi->bmiHeader.biPlanes = planes;
                    bi->bmiHeader.biBitCount = bpp;
                    bi->bmiHeader.biCompression = compr;
                    bi->bmiHeader.biSizeImage = 0;
                    memset( bi->bmiColors, 0xaa, sizeof(RGBQUAD) * 256 );
                    ret = GetDIBits(hdc, hbmp, 0, 0, data, bi, format);
                    if (expect_ok || (!bpp && compr != BI_JPEG && compr != BI_PNG) ||
                        (bpp == 4 && compr == BI_RLE4) || (bpp == 8 && compr == BI_RLE8))
                        ok( ret, "GetDIBits failed for %u/%u/%u/%u\n", bpp, planes, compr, format );
                    else
                        ok( !ret || broken(!bpp && (compr == BI_JPEG || compr == BI_PNG)), /* nt4 */
                            "GetDIBits succeeded for %u/%u/%u/%u\n", bpp, planes, compr, format );

                    /* all functions check planes except GetDIBits with 0 lines */
                    format_ok = expect_ok;
                    if (!planes) expect_ok = FALSE;
                    memset( bi, 0, sizeof(bi->bmiHeader) );
                    bi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                    bi->bmiHeader.biWidth = 2;
                    bi->bmiHeader.biHeight = 2;
                    bi->bmiHeader.biPlanes = planes;
                    bi->bmiHeader.biBitCount = bpp;
                    bi->bmiHeader.biCompression = compr;
                    bi->bmiHeader.biSizeImage = 0;
                    memset( bi->bmiColors, 0xaa, sizeof(RGBQUAD) * 256 );

                    hdib = CreateDIBSection(hdc, bi, format, &bits, NULL, 0);
                    if (expect_ok && (planes == 1 || planes * bpp <= 16) &&
                        (compr != BI_BITFIELDS || format != DIB_PAL_COLORS))
                        ok( hdib != NULL, "CreateDIBSection failed for %u/%u/%u/%u\n", bpp, planes, compr, format );
                    else
                        ok( hdib == NULL, "CreateDIBSection succeeded for %u/%u/%u/%u\n", bpp, planes, compr, format );
                    if (hdib) DeleteObject( hdib );

                    hdib = CreateDIBitmap( hdc, &bi->bmiHeader, 0, data, bi, format );
                    /* no sanity checks in CreateDIBitmap except compression */
                    if (compr == BI_JPEG || compr == BI_PNG)
                        ok( hdib == NULL || broken(hdib != NULL), /* nt4 */
                            "CreateDIBitmap succeeded for %u/%u/%u/%u\n", bpp, planes, compr, format );
                    else
                        ok( hdib != NULL, "CreateDIBitmap failed for %u/%u/%u/%u\n", bpp, planes, compr, format );
                    if (hdib) DeleteObject( hdib );

                    /* RLE needs a size */
                    bi->bmiHeader.biSizeImage = 0;
                    ret = SetDIBits(hdc, hbmp, 0, 1, data, bi, format);
                    if (expect_ok)
                        ok( ret, "SetDIBits failed for %u/%u/%u/%u\n", bpp, planes, compr, format );
                    else
                        ok( !ret ||
                            broken((bpp == 4 && compr == BI_RLE4) || (bpp == 8 && compr == BI_RLE8)), /* nt4 */
                            "SetDIBits succeeded for %u/%u/%u/%u\n", bpp, planes, compr, format );
                    ret = SetDIBitsToDevice( memdc, 0, 0, 1, 1, 0, 0, 0, 1, data, bi, format );
                    if (expect_ok)
                        ok( ret, "SetDIBitsToDevice failed for %u/%u/%u/%u\n", bpp, planes, compr, format );
                    else
                        ok( !ret ||
                            broken((bpp == 4 && compr == BI_RLE4) || (bpp == 8 && compr == BI_RLE8)), /* nt4 */
                            "SetDIBitsToDevice succeeded for %u/%u/%u/%u\n", bpp, planes, compr, format );
                    ret = StretchDIBits( memdc, 0, 0, 1, 1, 0, 0, 1, 1, data, bi, format, SRCCOPY );
                    if (expect_ok)
                        ok( ret, "StretchDIBits failed for %u/%u/%u/%u\n", bpp, planes, compr, format );
                    else
                        ok( !ret ||
                            broken((bpp == 4 && compr == BI_RLE4) || (bpp == 8 && compr == BI_RLE8)), /* nt4 */
                            "StretchDIBits succeeded for %u/%u/%u/%u\n", bpp, planes, compr, format );

                    ret = GetDIBits(hdc, hbmp, 0, 2, data, bi, format);
                    if (expect_ok)
                        ok( ret, "GetDIBits failed for %u/%u/%u/%u\n", bpp, planes, compr, format );
                    else
                        ok( !ret, "GetDIBits succeeded for %u/%u/%u/%u\n", bpp, planes, compr, format );
                    ok( bi->bmiHeader.biBitCount == bpp, "GetDIBits modified bpp %u/%u\n",
                        bpp, bi->bmiHeader.biBitCount );

                    bi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                    bi->bmiHeader.biWidth = 2;
                    bi->bmiHeader.biHeight = 2;
                    bi->bmiHeader.biPlanes = planes;
                    bi->bmiHeader.biBitCount = bpp;
                    bi->bmiHeader.biCompression = compr;
                    bi->bmiHeader.biSizeImage = 1;
                    memset( bi->bmiColors, 0xaa, sizeof(RGBQUAD) * 256 );
                    /* RLE allowed with valid biSizeImage */
                    if ((bpp == 4 && compr == BI_RLE4) || (bpp == 8 && compr == BI_RLE8)) expect_ok = TRUE;

                    ret = SetDIBits(hdc, hbmp, 0, 1, data, bi, format);
                    if (expect_ok)
                        ok( ret, "SetDIBits failed for %u/%u/%u/%u\n", bpp, planes, compr, format );
                    else
                        ok( !ret, "SetDIBits succeeded for %u/%u/%u/%u\n", bpp, planes, compr, format );
                    ret = SetDIBitsToDevice( memdc, 0, 0, 1, 1, 0, 0, 0, 1, data, bi, format );
                    if (expect_ok)
                        ok( ret, "SetDIBitsToDevice failed for %u/%u/%u/%u\n", bpp, planes, compr, format );
                    else
                        ok( !ret, "SetDIBitsToDevice succeeded for %u/%u/%u/%u\n", bpp, planes, compr, format );
                    ret = StretchDIBits( memdc, 0, 0, 1, 1, 0, 0, 1, 1, data, bi, format, SRCCOPY );
                    if (expect_ok)
                        ok( ret, "StretchDIBits failed for %u/%u/%u/%u\n", bpp, planes, compr, format );
                    else
                        ok( !ret, "StretchDIBits succeeded for %u/%u/%u/%u\n", bpp, planes, compr, format );

                    bi->bmiHeader.biSizeImage = 0;
                    ret = GetDIBits(hdc, hbmp, 0, 2, NULL, bi, format);
                    if (expect_ok || !bpp)
                        ok( ret, "GetDIBits failed for %u/%u/%u/%u\n", bpp, planes, compr, format );
                    else
                        ok( !ret || broken(format_ok && !planes),  /* nt4 */
                            "GetDIBits succeeded for %u/%u/%u/%u\n", bpp, planes, compr, format );
                }
            }
        }
    }

    memset( bi, 0, sizeof(bi->bmiHeader) );
    bi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi->bmiHeader.biWidth = 2;
    bi->bmiHeader.biHeight = 2;
    bi->bmiHeader.biPlanes = 1;
    bi->bmiHeader.biBitCount = 16;
    bi->bmiHeader.biCompression = BI_BITFIELDS;
    bi->bmiHeader.biSizeImage = 0;
    *(DWORD *)&bi->bmiColors[0] = 0;
    *(DWORD *)&bi->bmiColors[1] = 0;
    *(DWORD *)&bi->bmiColors[2] = 0;

    hdib = CreateDIBSection(hdc, bi, DIB_RGB_COLORS, &bits, NULL, 0);
    ok( hdib == NULL, "CreateDIBSection succeeded with null bitfields\n" );
    ret = SetDIBits(hdc, hbmp, 0, 1, data, bi, DIB_RGB_COLORS);
    ok( !ret, "SetDIBits succeeded with null bitfields\n" );
    /* other functions don't check */
    hdib = CreateDIBitmap( hdc, &bi->bmiHeader, 0, bits, bi, DIB_RGB_COLORS );
    ok( hdib != NULL, "CreateDIBitmap failed with null bitfields\n" );
    DeleteObject( hdib );
    ret = SetDIBitsToDevice( memdc, 0, 0, 1, 1, 0, 0, 0, 1, data, bi, DIB_RGB_COLORS );
    ok( ret, "SetDIBitsToDevice failed with null bitfields\n" );
    ret = StretchDIBits( memdc, 0, 0, 1, 1, 0, 0, 1, 1, data, bi, DIB_RGB_COLORS, SRCCOPY );
    ok( ret, "StretchDIBits failed with null bitfields\n" );
    ret = GetDIBits(hdc, hbmp, 0, 2, data, bi, DIB_RGB_COLORS);
    ok( ret, "GetDIBits failed with null bitfields\n" );
    bi->bmiHeader.biPlanes = 1;
    bi->bmiHeader.biBitCount = 16;
    bi->bmiHeader.biCompression = BI_BITFIELDS;
    bi->bmiHeader.biSizeImage = 0;
    *(DWORD *)&bi->bmiColors[0] = 0;
    *(DWORD *)&bi->bmiColors[1] = 0;
    *(DWORD *)&bi->bmiColors[2] = 0;
    ret = GetDIBits(hdc, hbmp, 0, 2, NULL, bi, DIB_RGB_COLORS);
    ok( ret, "GetDIBits failed with null bitfields\n" );

    /* all fields must be non-zero */
    *(DWORD *)&bi->bmiColors[0] = 3;
    *(DWORD *)&bi->bmiColors[1] = 0;
    *(DWORD *)&bi->bmiColors[2] = 7;
    hdib = CreateDIBSection(hdc, bi, DIB_RGB_COLORS, &bits, NULL, 0);
    ok( hdib == NULL, "CreateDIBSection succeeded with null bitfields\n" );
    ret = SetDIBits(hdc, hbmp, 0, 1, data, bi, DIB_RGB_COLORS);
    ok( !ret, "SetDIBits succeeded with null bitfields\n" );

    /* garbage is ok though */
    *(DWORD *)&bi->bmiColors[0] = 0x55;
    *(DWORD *)&bi->bmiColors[1] = 0x44;
    *(DWORD *)&bi->bmiColors[2] = 0x33;
    hdib = CreateDIBSection(hdc, bi, DIB_RGB_COLORS, &bits, NULL, 0);
    ok( hdib != NULL, "CreateDIBSection failed with bad bitfields\n" );
    if (hdib) DeleteObject( hdib );
    ret = SetDIBits(hdc, hbmp, 0, 1, data, bi, DIB_RGB_COLORS);
    ok( ret, "SetDIBits failed with bad bitfields\n" );

    bi->bmiHeader.biWidth = -2;
    bi->bmiHeader.biHeight = 2;
    bi->bmiHeader.biBitCount = 32;
    bi->bmiHeader.biCompression = BI_RGB;
    hdib = CreateDIBSection(hdc, bi, DIB_RGB_COLORS, &bits, NULL, 0);
    ok( hdib == NULL, "CreateDIBSection succeeded with negative width\n" );
    hdib = CreateDIBitmap( hdc, &bi->bmiHeader, 0, bits, bi, DIB_RGB_COLORS );
    ok( hdib == NULL, "CreateDIBitmap succeeded with negative width\n" );
    ret = SetDIBits(hdc, hbmp, 0, 1, data, bi, DIB_RGB_COLORS);
    ok( !ret, "SetDIBits succeeded with negative width\n" );
    ret = SetDIBitsToDevice( memdc, 0, 0, 1, 1, 0, 0, 0, 1, data, bi, DIB_RGB_COLORS );
    ok( !ret, "SetDIBitsToDevice succeeded with negative width\n" );
    ret = StretchDIBits( memdc, 0, 0, 1, 1, 0, 0, 1, 1, data, bi, DIB_RGB_COLORS, SRCCOPY );
    ok( !ret, "StretchDIBits succeeded with negative width\n" );
    ret = GetDIBits(hdc, hbmp, 0, 2, data, bi, DIB_RGB_COLORS);
    ok( !ret, "GetDIBits succeeded with negative width\n" );
    bi->bmiHeader.biWidth = -2;
    bi->bmiHeader.biHeight = 2;
    bi->bmiHeader.biBitCount = 32;
    bi->bmiHeader.biCompression = BI_RGB;
    ret = GetDIBits(hdc, hbmp, 0, 2, NULL, bi, DIB_RGB_COLORS);
    ok( !ret || broken(ret), /* nt4 */ "GetDIBits succeeded with negative width\n" );

    bi->bmiHeader.biWidth = 0;
    bi->bmiHeader.biHeight = 2;
    bi->bmiHeader.biBitCount = 32;
    bi->bmiHeader.biCompression = BI_RGB;
    hdib = CreateDIBSection(hdc, bi, DIB_RGB_COLORS, &bits, NULL, 0);
    ok( hdib == NULL, "CreateDIBSection succeeded with zero width\n" );
    hdib = CreateDIBitmap( hdc, &bi->bmiHeader, 0, bits, bi, DIB_RGB_COLORS );
    ok( hdib != NULL, "CreateDIBitmap failed with zero width\n" );
    DeleteObject( hdib );
    ret = SetDIBits(hdc, hbmp, 0, 1, data, bi, DIB_RGB_COLORS);
    ok( !ret || broken(ret), /* nt4 */ "SetDIBits succeeded with zero width\n" );
    ret = SetDIBitsToDevice( memdc, 0, 0, 1, 1, 0, 0, 0, 1, data, bi, DIB_RGB_COLORS );
    ok( !ret || broken(ret), /* nt4 */ "SetDIBitsToDevice succeeded with zero width\n" );
    ret = StretchDIBits( memdc, 0, 0, 1, 1, 0, 0, 1, 1, data, bi, DIB_RGB_COLORS, SRCCOPY );
    ok( !ret || broken(ret), /* nt4 */ "StretchDIBits succeeded with zero width\n" );
    ret = GetDIBits(hdc, hbmp, 0, 2, data, bi, DIB_RGB_COLORS);
    ok( !ret, "GetDIBits succeeded with zero width\n" );
    bi->bmiHeader.biWidth = 0;
    bi->bmiHeader.biHeight = 2;
    bi->bmiHeader.biBitCount = 32;
    bi->bmiHeader.biCompression = BI_RGB;
    ret = GetDIBits(hdc, hbmp, 0, 2, NULL, bi, DIB_RGB_COLORS);
    ok( !ret || broken(ret), /* nt4 */ "GetDIBits succeeded with zero width\n" );

    bi->bmiHeader.biWidth = 2;
    bi->bmiHeader.biHeight = 0;
    bi->bmiHeader.biBitCount = 32;
    bi->bmiHeader.biCompression = BI_RGB;
    hdib = CreateDIBSection(hdc, bi, DIB_RGB_COLORS, &bits, NULL, 0);
    ok( hdib == NULL, "CreateDIBSection succeeded with zero height\n" );
    hdib = CreateDIBitmap( hdc, &bi->bmiHeader, 0, bits, bi, DIB_RGB_COLORS );
    ok( hdib != NULL, "CreateDIBitmap failed with zero height\n" );
    DeleteObject( hdib );
    ret = SetDIBits(hdc, hbmp, 0, 1, data, bi, DIB_RGB_COLORS);
    ok( !ret, "SetDIBits succeeded with zero height\n" );
    ret = SetDIBitsToDevice( memdc, 0, 0, 1, 1, 0, 0, 0, 1, data, bi, DIB_RGB_COLORS );
    ok( !ret, "SetDIBitsToDevice succeeded with zero height\n" );
    ret = StretchDIBits( memdc, 0, 0, 1, 1, 0, 0, 1, 1, data, bi, DIB_RGB_COLORS, SRCCOPY );
    ok( !ret, "StretchDIBits succeeded with zero height\n" );
    ret = GetDIBits(hdc, hbmp, 0, 2, data, bi, DIB_RGB_COLORS);
    ok( !ret || broken(ret), /* nt4 */ "GetDIBits succeeded with zero height\n" );
    bi->bmiHeader.biWidth = 2;
    bi->bmiHeader.biHeight = 0;
    bi->bmiHeader.biBitCount = 32;
    bi->bmiHeader.biCompression = BI_RGB;
    ret = GetDIBits(hdc, hbmp, 0, 2, NULL, bi, DIB_RGB_COLORS);
    ok( !ret || broken(ret), /* nt4 */ "GetDIBits succeeded with zero height\n" );

    /* some functions accept DIB_PAL_COLORS+1, but not beyond */

    bi->bmiHeader.biWidth = 2;
    bi->bmiHeader.biHeight = 2;
    bi->bmiHeader.biBitCount = 1;
    bi->bmiHeader.biCompression = BI_RGB;
    hdib = CreateDIBSection(hdc, bi, DIB_PAL_COLORS+1, &bits, NULL, 0);
    ok( hdib == NULL, "CreateDIBSection succeeded with DIB_PAL_COLORS+1\n" );
    hdib = CreateDIBitmap( hdc, &bi->bmiHeader, 0, bits, bi, DIB_PAL_COLORS+1 );
    ok( hdib != NULL, "CreateDIBitmap failed with DIB_PAL_COLORS+1\n" );
    DeleteObject( hdib );
    ret = SetDIBits(hdc, hbmp, 0, 1, data, bi, DIB_PAL_COLORS+1);
    ok( !ret, "SetDIBits succeeded with DIB_PAL_COLORS+1\n" );
    ret = SetDIBitsToDevice( memdc, 0, 0, 1, 1, 0, 0, 0, 1, data, bi, DIB_PAL_COLORS+1 );
    ok( ret, "SetDIBitsToDevice failed with DIB_PAL_COLORS+1\n" );
    ret = StretchDIBits( memdc, 0, 0, 1, 1, 0, 0, 1, 1, data, bi, DIB_PAL_COLORS+1, SRCCOPY );
    ok( ret, "StretchDIBits failed with DIB_PAL_COLORS+1\n" );
    ret = GetDIBits(hdc, hbmp, 0, 2, data, bi, DIB_PAL_COLORS+1);
    ok( !ret, "GetDIBits succeeded with DIB_PAL_COLORS+1\n" );
    bi->bmiHeader.biWidth = 2;
    bi->bmiHeader.biHeight = 2;
    bi->bmiHeader.biBitCount = 1;
    bi->bmiHeader.biCompression = BI_RGB;
    ret = GetDIBits(hdc, hbmp, 0, 0, NULL, bi, DIB_PAL_COLORS+1);
    ok( !ret, "GetDIBits succeeded with DIB_PAL_COLORS+1\n" );

    bi->bmiHeader.biWidth = 2;
    bi->bmiHeader.biHeight = 2;
    bi->bmiHeader.biBitCount = 1;
    bi->bmiHeader.biCompression = BI_RGB;
    hdib = CreateDIBSection(hdc, bi, DIB_PAL_COLORS+2, &bits, NULL, 0);
    ok( hdib == NULL, "CreateDIBSection succeeded with DIB_PAL_COLORS+2\n" );
    hdib = CreateDIBitmap( hdc, &bi->bmiHeader, 0, bits, bi, DIB_PAL_COLORS+2 );
    ok( hdib == NULL, "CreateDIBitmap succeeded with DIB_PAL_COLORS+2\n" );
    DeleteObject( hdib );
    ret = SetDIBits(hdc, hbmp, 0, 1, data, bi, DIB_PAL_COLORS+2);
    ok( !ret, "SetDIBits succeeded with DIB_PAL_COLORS+2\n" );
    ret = SetDIBitsToDevice( memdc, 0, 0, 1, 1, 0, 0, 0, 1, data, bi, DIB_PAL_COLORS+2 );
    ok( !ret, "SetDIBitsToDevice succeeded with DIB_PAL_COLORS+2\n" );
    ret = StretchDIBits( memdc, 0, 0, 1, 1, 0, 0, 1, 1, data, bi, DIB_PAL_COLORS+2, SRCCOPY );
    ok( !ret, "StretchDIBits succeeded with DIB_PAL_COLORS+2\n" );
    ret = GetDIBits(hdc, hbmp, 0, 2, data, bi, DIB_PAL_COLORS+2);
    ok( !ret, "GetDIBits succeeded with DIB_PAL_COLORS+2\n" );
    bi->bmiHeader.biWidth = 2;
    bi->bmiHeader.biHeight = 2;
    bi->bmiHeader.biBitCount = 1;
    bi->bmiHeader.biCompression = BI_RGB;
    ret = GetDIBits(hdc, hbmp, 0, 0, NULL, bi, DIB_PAL_COLORS+2);
    ok( !ret, "GetDIBits succeeded with DIB_PAL_COLORS+2\n" );

    bi->bmiHeader.biWidth = 0x4000;
    bi->bmiHeader.biHeight = 0x4000;
    bi->bmiHeader.biBitCount = 1;
    bi->bmiHeader.biCompression = BI_RGB;
    hdib = CreateDIBSection(hdc, bi, DIB_RGB_COLORS, &bits, NULL, 0);
    ok( hdib != NULL, "CreateDIBSection failed with large size\n" );
    DeleteObject( hdib );

    bi->bmiHeader.biWidth = 0x8001;
    bi->bmiHeader.biHeight = 0x8001;
    bi->bmiHeader.biBitCount = 32;
    bi->bmiHeader.biCompression = BI_RGB;
    hdib = CreateDIBSection(hdc, bi, DIB_RGB_COLORS, &bits, NULL, 0);
    ok( hdib == NULL, "CreateDIBSection succeeded with size overflow\n" );

    bi->bmiHeader.biWidth = 1;
    bi->bmiHeader.biHeight = 0x40000001;
    bi->bmiHeader.biBitCount = 32;
    bi->bmiHeader.biCompression = BI_RGB;
    hdib = CreateDIBSection(hdc, bi, DIB_RGB_COLORS, &bits, NULL, 0);
    ok( hdib == NULL, "CreateDIBSection succeeded with size overflow\n" );

    bi->bmiHeader.biWidth = 2;
    bi->bmiHeader.biHeight = 0x40000001;
    bi->bmiHeader.biBitCount = 16;
    bi->bmiHeader.biCompression = BI_RGB;
    hdib = CreateDIBSection(hdc, bi, DIB_RGB_COLORS, &bits, NULL, 0);
    ok( hdib == NULL, "CreateDIBSection succeeded with size overflow\n" );

    bi->bmiHeader.biWidth = 0x40000001;
    bi->bmiHeader.biHeight = 1;
    bi->bmiHeader.biBitCount = 32;
    bi->bmiHeader.biCompression = BI_RGB;
    hdib = CreateDIBSection(hdc, bi, DIB_RGB_COLORS, &bits, NULL, 0);
    ok( hdib == NULL, "CreateDIBSection succeeded with size overflow\n" );

    bi->bmiHeader.biWidth = 0x40000001;
    bi->bmiHeader.biHeight = 4;
    bi->bmiHeader.biBitCount = 8;
    bi->bmiHeader.biCompression = BI_RGB;
    hdib = CreateDIBSection(hdc, bi, DIB_RGB_COLORS, &bits, NULL, 0);
    ok( hdib == NULL, "CreateDIBSection succeeded with size overflow\n" );

    DeleteDC( memdc );
    DeleteObject( hbmp );
    ReleaseDC( 0, hdc );
    HeapFree( GetProcessHeap(), 0, bi );
}

static void test_mono_dibsection(void)
{
    HDC hdc, memdc;
    HBITMAP old_bm, mono_ds;
    char bmibuf[sizeof(BITMAPINFO) + 256 * sizeof(RGBQUAD)];
    BITMAPINFO *pbmi = (BITMAPINFO *)bmibuf;
    RGBQUAD *colors = pbmi->bmiColors;
    BYTE bits[10 * 4];
    BYTE *ds_bits;
    int num;

    hdc = GetDC(0);

    memdc = CreateCompatibleDC(hdc);

    memset(pbmi, 0, sizeof(bmibuf));
    pbmi->bmiHeader.biSize = sizeof(pbmi->bmiHeader);
    pbmi->bmiHeader.biHeight = 10;
    pbmi->bmiHeader.biWidth = 10;
    pbmi->bmiHeader.biBitCount = 1;
    pbmi->bmiHeader.biPlanes = 1;
    pbmi->bmiHeader.biCompression = BI_RGB;
    colors[0].rgbRed = 0xff;
    colors[0].rgbGreen = 0xff;
    colors[0].rgbBlue = 0xff;
    colors[1].rgbRed = 0x0;
    colors[1].rgbGreen = 0x0;
    colors[1].rgbBlue = 0x0;

    /*
     * First dib section is 'inverted' ie color[0] is white, color[1] is black
     */

    mono_ds = CreateDIBSection(hdc, pbmi, DIB_RGB_COLORS, (void**)&ds_bits, NULL, 0);
    ok(mono_ds != NULL, "CreateDIBSection rets NULL\n");
    old_bm = SelectObject(memdc, mono_ds);

    /* black border, white interior */
    Rectangle(memdc, 0, 0, 10, 10);
    ok(ds_bits[0] == 0xff, "out_bits %02x\n", ds_bits[0]);
    ok(ds_bits[4] == 0x80, "out_bits %02x\n", ds_bits[4]);

    /* SetDIBitsToDevice with an inverted bmi -> inverted dib section */

    memset(bits, 0, sizeof(bits));
    bits[0] = 0xaa;

    SetDIBitsToDevice(memdc, 0, 0, 10, 10, 0, 0, 0, 10, bits, pbmi, DIB_RGB_COLORS);
    ok(ds_bits[0] == 0xaa, "out_bits %02x\n", ds_bits[0]);

    /* SetDIBitsToDevice with a normal bmi -> inverted dib section */

    colors[0].rgbRed = 0x0;
    colors[0].rgbGreen = 0x0;
    colors[0].rgbBlue = 0x0;
    colors[1].rgbRed = 0xff;
    colors[1].rgbGreen = 0xff;
    colors[1].rgbBlue = 0xff;

    SetDIBitsToDevice(memdc, 0, 0, 10, 10, 0, 0, 0, 10, bits, pbmi, DIB_RGB_COLORS);
    ok(ds_bits[0] == 0x55, "out_bits %02x\n", ds_bits[0]);

    SelectObject(memdc, old_bm);
    DeleteObject(mono_ds);

    /*
     * Next dib section is 'normal' ie color[0] is black, color[1] is white
     */

    colors[0].rgbRed = 0x0;
    colors[0].rgbGreen = 0x0;
    colors[0].rgbBlue = 0x0;
    colors[1].rgbRed = 0xff;
    colors[1].rgbGreen = 0xff;
    colors[1].rgbBlue = 0xff;

    mono_ds = CreateDIBSection(hdc, pbmi, DIB_RGB_COLORS, (void**)&ds_bits, NULL, 0);
    ok(mono_ds != NULL, "CreateDIBSection rets NULL\n");
    old_bm = SelectObject(memdc, mono_ds);

    /* black border, white interior */
    Rectangle(memdc, 0, 0, 10, 10);
    ok(ds_bits[0] == 0x00, "out_bits %02x\n", ds_bits[0]);
    ok(ds_bits[4] == 0x7f, "out_bits %02x\n", ds_bits[4]);

    /* SetDIBitsToDevice with a normal bmi -> normal dib section */

    SetDIBitsToDevice(memdc, 0, 0, 10, 10, 0, 0, 0, 10, bits, pbmi, DIB_RGB_COLORS);
    ok(ds_bits[0] == 0xaa, "out_bits %02x\n", ds_bits[0]);

    /* SetDIBitsToDevice with an inverted bmi -> normal dib section */

    colors[0].rgbRed = 0xff;
    colors[0].rgbGreen = 0xff;
    colors[0].rgbBlue = 0xff;
    colors[1].rgbRed = 0x0;
    colors[1].rgbGreen = 0x0;
    colors[1].rgbBlue = 0x0;

    SetDIBitsToDevice(memdc, 0, 0, 10, 10, 0, 0, 0, 10, bits, pbmi, DIB_RGB_COLORS);
    ok(ds_bits[0] == 0x55, "out_bits %02x\n", ds_bits[0]);

    /*
     * Take that 'normal' dibsection and change its colour table to an 'inverted' one
     */

    colors[0].rgbRed = 0xff;
    colors[0].rgbGreen = 0xff;
    colors[0].rgbBlue = 0xff;
    colors[1].rgbRed = 0x0;
    colors[1].rgbGreen = 0x0;
    colors[1].rgbBlue = 0x0;
    num = SetDIBColorTable(memdc, 0, 2, colors);
    ok(num == 2, "num = %d\n", num);

    /* black border, white interior */
    Rectangle(memdc, 0, 0, 10, 10);
    ok(ds_bits[0] == 0xff, "out_bits %02x\n", ds_bits[0]);
    ok(ds_bits[4] == 0x80, "out_bits %02x\n", ds_bits[4]);

    /* SetDIBitsToDevice with an inverted bmi -> inverted dib section */

    memset(bits, 0, sizeof(bits));
    bits[0] = 0xaa;

    SetDIBitsToDevice(memdc, 0, 0, 10, 10, 0, 0, 0, 10, bits, pbmi, DIB_RGB_COLORS);
    ok(ds_bits[0] == 0xaa, "out_bits %02x\n", ds_bits[0]);

    /* SetDIBitsToDevice with a normal bmi -> inverted dib section */

    colors[0].rgbRed = 0x0;
    colors[0].rgbGreen = 0x0;
    colors[0].rgbBlue = 0x0;
    colors[1].rgbRed = 0xff;
    colors[1].rgbGreen = 0xff;
    colors[1].rgbBlue = 0xff;

    SetDIBitsToDevice(memdc, 0, 0, 10, 10, 0, 0, 0, 10, bits, pbmi, DIB_RGB_COLORS);
    ok(ds_bits[0] == 0x55, "out_bits %02x\n", ds_bits[0]);

    SelectObject(memdc, old_bm);
    DeleteObject(mono_ds);

    /*
     * Now a dib section with a strange colour map just for fun.  This behaves just like an inverted one.
     */
 
    colors[0].rgbRed = 0xff;
    colors[0].rgbGreen = 0x0;
    colors[0].rgbBlue = 0x0;
    colors[1].rgbRed = 0xfe;
    colors[1].rgbGreen = 0x0;
    colors[1].rgbBlue = 0x0;

    mono_ds = CreateDIBSection(hdc, pbmi, DIB_RGB_COLORS, (void**)&ds_bits, NULL, 0);
    ok(mono_ds != NULL, "CreateDIBSection rets NULL\n");
    old_bm = SelectObject(memdc, mono_ds);

    /* black border, white interior */
    Rectangle(memdc, 0, 0, 10, 10);
    ok(ds_bits[0] == 0xff, "out_bits %02x\n", ds_bits[0]);
    ok(ds_bits[4] == 0x80, "out_bits %02x\n", ds_bits[4]);

    /* SetDIBitsToDevice with a normal bmi -> inverted dib section */

    colors[0].rgbRed = 0x0;
    colors[0].rgbGreen = 0x0;
    colors[0].rgbBlue = 0x0;
    colors[1].rgbRed = 0xff;
    colors[1].rgbGreen = 0xff;
    colors[1].rgbBlue = 0xff;

    SetDIBitsToDevice(memdc, 0, 0, 10, 10, 0, 0, 0, 10, bits, pbmi, DIB_RGB_COLORS);
    ok(ds_bits[0] == 0x55, "out_bits %02x\n", ds_bits[0]);

    /* SetDIBitsToDevice with an inverted bmi -> inverted dib section */

    colors[0].rgbRed = 0xff;
    colors[0].rgbGreen = 0xff;
    colors[0].rgbBlue = 0xff;
    colors[1].rgbRed = 0x0;
    colors[1].rgbGreen = 0x0;
    colors[1].rgbBlue = 0x0;

    SetDIBitsToDevice(memdc, 0, 0, 10, 10, 0, 0, 0, 10, bits, pbmi, DIB_RGB_COLORS);
    ok(ds_bits[0] == 0xaa, "out_bits %02x\n", ds_bits[0]);

    SelectObject(memdc, old_bm);
    DeleteObject(mono_ds);

    DeleteDC(memdc);
    ReleaseDC(0, hdc);
}

static void test_bitmap(void)
{
    char buf[256], buf_cmp[256];
    HBITMAP hbmp, hbmp_old;
    HDC hdc;
    BITMAP bm;
    BITMAP bma[2];
    INT ret;

    hdc = CreateCompatibleDC(0);
    assert(hdc != 0);

    SetLastError(0xdeadbeef);
    hbmp = CreateBitmap(0x7ffffff, 1, 1, 1, NULL);
    if (!hbmp)
    {
        ok(GetLastError() == ERROR_NOT_ENOUGH_MEMORY /* XP */ ||
           GetLastError() == ERROR_INVALID_PARAMETER /* Win2k */,
           "expected ERROR_NOT_ENOUGH_MEMORY, got %u\n", GetLastError());
    }
    else
        DeleteObject(hbmp);

    SetLastError(0xdeadbeef);
    hbmp = CreateBitmap(0x7ffffff, 9, 1, 1, NULL);
    if (!hbmp)
    {
        ok(GetLastError() == ERROR_NOT_ENOUGH_MEMORY /* XP */ ||
           GetLastError() == ERROR_INVALID_PARAMETER /* Win2k */,
           "expected ERROR_NOT_ENOUGH_MEMORY, got %u\n", GetLastError());
    }
    else
        DeleteObject(hbmp);

    SetLastError(0xdeadbeef);
    hbmp = CreateBitmap(0x7ffffff + 1, 1, 1, 1, NULL);
    ok(!hbmp, "CreateBitmap should fail\n");
    if (!hbmp)
        ok(GetLastError() == ERROR_INVALID_PARAMETER,
           "expected ERROR_INVALID_PARAMETER, got %u\n", GetLastError());
    else
        DeleteObject(hbmp);

    hbmp = CreateBitmap(15, 15, 1, 1, NULL);
    assert(hbmp != NULL);

    ret = GetObjectW(hbmp, sizeof(bm), &bm);
    ok(ret == sizeof(bm), "wrong size %d\n", ret);

    ok(bm.bmType == 0, "wrong bm.bmType %d\n", bm.bmType);
    ok(bm.bmWidth == 15, "wrong bm.bmWidth %d\n", bm.bmWidth);
    ok(bm.bmHeight == 15, "wrong bm.bmHeight %d\n", bm.bmHeight);
    ok(bm.bmWidthBytes == 2, "wrong bm.bmWidthBytes %d\n", bm.bmWidthBytes);
    ok(bm.bmPlanes == 1, "wrong bm.bmPlanes %d\n", bm.bmPlanes);
    ok(bm.bmBitsPixel == 1, "wrong bm.bmBitsPixel %d\n", bm.bmBitsPixel);
    ok(bm.bmBits == NULL, "wrong bm.bmBits %p\n", bm.bmBits);

    assert(sizeof(buf) >= bm.bmWidthBytes * bm.bmHeight);
    assert(sizeof(buf) == sizeof(buf_cmp));

    ret = GetBitmapBits(hbmp, 0, NULL);
    ok(ret == bm.bmWidthBytes * bm.bmHeight, "%d != %d\n", ret, bm.bmWidthBytes * bm.bmHeight);

    memset(buf_cmp, 0xAA, sizeof(buf_cmp));
    memset(buf_cmp, 0, bm.bmWidthBytes * bm.bmHeight);

    memset(buf, 0xAA, sizeof(buf));
    ret = GetBitmapBits(hbmp, sizeof(buf), buf);
    ok(ret == bm.bmWidthBytes * bm.bmHeight, "%d != %d\n", ret, bm.bmWidthBytes * bm.bmHeight);
    ok(!memcmp(buf, buf_cmp, sizeof(buf)), "buffers do not match\n");

    hbmp_old = SelectObject(hdc, hbmp);

    ret = GetObjectW(hbmp, sizeof(bm), &bm);
    ok(ret == sizeof(bm), "wrong size %d\n", ret);

    ok(bm.bmType == 0, "wrong bm.bmType %d\n", bm.bmType);
    ok(bm.bmWidth == 15, "wrong bm.bmWidth %d\n", bm.bmWidth);
    ok(bm.bmHeight == 15, "wrong bm.bmHeight %d\n", bm.bmHeight);
    ok(bm.bmWidthBytes == 2, "wrong bm.bmWidthBytes %d\n", bm.bmWidthBytes);
    ok(bm.bmPlanes == 1, "wrong bm.bmPlanes %d\n", bm.bmPlanes);
    ok(bm.bmBitsPixel == 1, "wrong bm.bmBitsPixel %d\n", bm.bmBitsPixel);
    ok(bm.bmBits == NULL, "wrong bm.bmBits %p\n", bm.bmBits);

    memset(buf, 0xAA, sizeof(buf));
    ret = GetBitmapBits(hbmp, sizeof(buf), buf);
    ok(ret == bm.bmWidthBytes * bm.bmHeight, "%d != %d\n", ret, bm.bmWidthBytes * bm.bmHeight);
    ok(!memcmp(buf, buf_cmp, sizeof(buf)), "buffers do not match\n");

    hbmp_old = SelectObject(hdc, hbmp_old);
    ok(hbmp_old == hbmp, "wrong old bitmap %p\n", hbmp_old);

    /* test various buffer sizes for GetObject */
    ret = GetObjectW(hbmp, sizeof(*bma) * 2, bma);
    ok(ret == sizeof(*bma), "wrong size %d\n", ret);

    ret = GetObjectW(hbmp, sizeof(bm) / 2, &bm);
    ok(ret == 0, "%d != 0\n", ret);

    ret = GetObjectW(hbmp, 0, &bm);
    ok(ret == 0, "%d != 0\n", ret);

    ret = GetObjectW(hbmp, 1, &bm);
    ok(ret == 0, "%d != 0\n", ret);

    DeleteObject(hbmp);
    DeleteDC(hdc);
}

static COLORREF get_nearest( int r, int g, int b )
{
    return (r*r + g*g + b*b < (255-r)*(255-r) + (255-g)*(255-g) + (255-b)*(255-b)) ? 0x000000 : 0xffffff;
}

static BOOL is_black_pen( COLORREF fg, COLORREF bg, int r, int g, int b )
{
    if (fg == 0 || bg == 0xffffff) return RGB(r,g,b) != 0xffffff && RGB(r,g,b) != bg;
    return RGB(r,g,b) == 0x000000 || RGB(r,g,b) == bg;
}

static void test_bitmap_colors( HDC hdc, COLORREF fg, COLORREF bg, int r, int g, int b )
{
    static const WORD pattern_bits[] = { 0x5555, 0xaaaa, 0x5555, 0xaaaa, 0x5555, 0xaaaa, 0x5555, 0xaaaa };
    char buffer[FIELD_OFFSET( BITMAPINFO, bmiColors ) + 256 * sizeof(RGBQUAD)];
    BITMAPINFO *info = (BITMAPINFO *)buffer;
    RGBQUAD *colors = info->bmiColors;
    WORD bits[16];
    void *bits_ptr;
    COLORREF res;
    HBRUSH old_brush;
    HPEN old_pen;
    HBITMAP bitmap;
    HDC memdc;

    res = SetPixel( hdc, 0, 0, RGB(r,g,b) );
    ok( res == get_nearest( r, g, b ),
        "wrong result %06x for %02x,%02x,%02x fg %06x bg %06x\n", res, r, g, b, fg, bg );
    res = GetPixel( hdc, 0, 0 );
    ok( res == get_nearest( r, g, b ),
        "wrong result %06x for %02x,%02x,%02x fg %06x bg %06x\n", res, r, g, b, fg, bg );
    res = GetNearestColor( hdc, RGB(r,g,b) );
    ok( res == get_nearest( r, g, b ),
        "wrong result %06x for %02x,%02x,%02x fg %06x bg %06x\n", res, r, g, b, fg, bg );

    /* solid pen */
    old_pen = SelectObject( hdc, CreatePen( PS_SOLID, 1, RGB(r,g,b) ));
    MoveToEx( hdc, 0, 0, NULL );
    LineTo( hdc, 16, 0 );
    res = GetPixel( hdc, 0, 0 );
    ok( res == (is_black_pen( fg, bg, r, g, b ) ? 0 : 0xffffff),
        "wrong result %06x for %02x,%02x,%02x fg %06x bg %06x\n", res, r, g, b, fg, bg );
    GetBitmapBits( GetCurrentObject( hdc, OBJ_BITMAP ), sizeof(bits), bits );
    ok( bits[0] == (is_black_pen( fg, bg, r, g, b ) ? 0x00 : 0xffff),
        "wrong bits %04x for %02x,%02x,%02x fg %06x bg %06x\n", bits[0], r, g, b, fg, bg );
    DeleteObject( SelectObject( hdc, old_pen ));

    /* mono DDB pattern brush */
    bitmap = CreateBitmap( 16, 8, 1, 1, pattern_bits );
    old_brush = SelectObject( hdc, CreatePatternBrush( bitmap ));
    PatBlt( hdc, 0, 0, 16, 16, PATCOPY );
    GetBitmapBits( GetCurrentObject( hdc, OBJ_BITMAP ), sizeof(bits), bits );
    ok( bits[0] == 0x5555 || broken(bits[0] == 0xaada) /* XP SP1 & 2003 SP0 */,
        "wrong bits %04x for %02x,%02x,%02x fg %06x bg %06x\n", bits[0], r, g, b, fg, bg );
    DeleteObject( SelectObject( hdc, old_brush ));

    /* mono DDB bitmap */
    memdc = CreateCompatibleDC( hdc );
    SelectObject( memdc, bitmap );
    BitBlt( hdc, 0, 0, 16, 8, memdc, 0, 0, SRCCOPY );
    GetBitmapBits( GetCurrentObject( hdc, OBJ_BITMAP ), sizeof(bits), bits );
    ok( bits[0] == 0x5555,
        "wrong bits %04x for %02x,%02x,%02x fg %06x bg %06x\n", bits[0], r, g, b, fg, bg );
    SetTextColor( memdc, RGB(255,255,255) );
    SetBkColor( memdc, RGB(0,0,0) );
    BitBlt( hdc, 0, 0, 16, 8, memdc, 0, 0, SRCCOPY );
    GetBitmapBits( GetCurrentObject( hdc, OBJ_BITMAP ), sizeof(bits), bits );
    ok( bits[0] == 0x5555,
        "wrong bits %04x for %02x,%02x,%02x fg %06x bg %06x\n", bits[0], r, g, b, fg, bg );

    /* mono DIB section */
    memset( buffer, 0, sizeof(buffer) );
    info->bmiHeader.biSize = sizeof(info->bmiHeader);
    info->bmiHeader.biHeight = -16;
    info->bmiHeader.biWidth = 16;
    info->bmiHeader.biBitCount = 1;
    info->bmiHeader.biPlanes = 1;
    info->bmiHeader.biCompression = BI_RGB;
    colors[0].rgbRed = 0xff;
    colors[0].rgbGreen = 0xff;
    colors[0].rgbBlue = 0xf0;
    colors[1].rgbRed = 0x20;
    colors[1].rgbGreen = 0x0;
    colors[1].rgbBlue = 0x0;
    bitmap = CreateDIBSection( 0, info, DIB_RGB_COLORS, &bits_ptr, NULL, 0 );
    memset( bits_ptr, 0x55, 64 );
    DeleteObject( SelectObject( memdc, bitmap ));
    BitBlt( hdc, 0, 0, 16, 8, memdc, 0, 0, SRCCOPY );
    GetBitmapBits( GetCurrentObject( hdc, OBJ_BITMAP ), sizeof(bits), bits );
    ok( bits[0] == 0x5555,
        "wrong bits %04x for %02x,%02x,%02x fg %06x bg %06x\n", bits[0], r, g, b, fg, bg );

    colors[0].rgbRed = 0x0;
    colors[0].rgbGreen = 0x0;
    colors[0].rgbBlue = 0x10;
    colors[1].rgbRed = 0xff;
    colors[1].rgbGreen = 0xf0;
    colors[1].rgbBlue = 0xff;
    bitmap = CreateDIBSection( 0, info, DIB_RGB_COLORS, &bits_ptr, NULL, 0 );
    memset( bits_ptr, 0x55, 64 );
    DeleteObject( SelectObject( memdc, bitmap ));
    BitBlt( hdc, 0, 0, 16, 8, memdc, 0, 0, SRCCOPY );
    GetBitmapBits( GetCurrentObject( hdc, OBJ_BITMAP ), sizeof(bits), bits );
    ok( bits[0] == 0xaaaa,
        "wrong bits %04x for %02x,%02x,%02x fg %06x bg %06x\n", bits[0], r, g, b, fg, bg );

    SetTextColor( memdc, RGB(0,20,0) );
    SetBkColor( memdc, RGB(240,240,240) );
    BitBlt( hdc, 0, 0, 16, 8, memdc, 0, 0, SRCCOPY );
    GetBitmapBits( GetCurrentObject( hdc, OBJ_BITMAP ), sizeof(bits), bits );
    ok( bits[0] == 0x5555,
        "wrong bits %04x for %02x,%02x,%02x fg %06x bg %06x\n", bits[0], r, g, b, fg, bg );

    SetTextColor( memdc, RGB(250,250,250) );
    SetBkColor( memdc, RGB(10,10,10) );
    BitBlt( hdc, 0, 0, 16, 8, memdc, 0, 0, SRCCOPY );
    GetBitmapBits( GetCurrentObject( hdc, OBJ_BITMAP ), sizeof(bits), bits );
    ok( bits[0] == 0xaaaa,
        "wrong bits %04x for %02x,%02x,%02x fg %06x bg %06x\n", bits[0], r, g, b, fg, bg );
    DeleteDC( memdc );
    DeleteObject( bitmap );
}

static void test_mono_bitmap(void)
{
    static const COLORREF colors[][2] =
    {
        { RGB(0x00,0x00,0x00), RGB(0xff,0xff,0xff) },
        { RGB(0xff,0xff,0xff), RGB(0x00,0x00,0x00) },
        { RGB(0x00,0x00,0x00), RGB(0xff,0xff,0xfe) },
        { RGB(0x00,0x01,0x00), RGB(0xff,0xff,0xff) },
        { RGB(0x00,0x00,0x00), RGB(0x80,0x80,0x80) },
        { RGB(0x80,0x80,0x80), RGB(0xff,0xff,0xff) },
        { RGB(0x30,0x40,0x50), RGB(0x60,0x70,0x80) },
        { RGB(0xa0,0xa0,0xa0), RGB(0x20,0x30,0x10) },
        { PALETTEINDEX(0), PALETTEINDEX(255) },
        { PALETTEINDEX(1), PALETTEINDEX(2) },
    };

    HBITMAP hbmp;
    HDC hdc;
    DWORD col;
    int i, r, g, b;

    if (!winetest_interactive)
    {
        skip("ROSTESTS-153: Skipping test_mono_bitmap because it causes too many failures and takes too long\n");
        return;
    }

    hdc = CreateCompatibleDC(0);
    assert(hdc != 0);

    hbmp = CreateBitmap(16, 16, 1, 1, NULL);
    assert(hbmp != NULL);

    SelectObject( hdc, hbmp );

    for (col = 0; col < sizeof(colors) / sizeof(colors[0]); col++)
    {
        SetTextColor( hdc, colors[col][0] );
        SetBkColor( hdc, colors[col][1] );

        for (i = 0; i < 256; i++)
        {
            HPALETTE pal = GetCurrentObject( hdc, OBJ_PAL );
            PALETTEENTRY ent;

            if (!GetPaletteEntries( pal, i, 1, &ent )) GetPaletteEntries( pal, 0, 1, &ent );
            test_color( hdc, PALETTEINDEX(i), get_nearest( ent.peRed, ent.peGreen, ent.peBlue ));
            test_color( hdc, DIBINDEX(i), (i == 1) ? 0xffffff : 0x000000 );
        }

        for (r = 0; r < 256; r += 15)
            for (g = 0; g < 256; g += 15)
                for (b = 0; b < 256; b += 15)
                    test_bitmap_colors( hdc, colors[col][0], colors[col][1], r, g, b );
    }

    DeleteDC(hdc);
    DeleteObject(hbmp);
}

static void test_bmBits(void)
{
    BYTE bits[4];
    HBITMAP hbmp;
    BITMAP bmp;

    memset(bits, 0, sizeof(bits));
    hbmp = CreateBitmap(2, 2, 1, 4, bits);
    ok(hbmp != NULL, "CreateBitmap failed\n");

    memset(&bmp, 0xFF, sizeof(bmp));
    ok(GetObjectW(hbmp, sizeof(bmp), &bmp) == sizeof(bmp),
       "GetObject failed or returned a wrong structure size\n");
    ok(!bmp.bmBits, "bmBits must be NULL for device-dependent bitmaps\n");

    DeleteObject(hbmp);
}

static void test_GetDIBits_selected_DIB(UINT bpp)
{
    HBITMAP dib;
    BITMAPINFO *info;
    BITMAPINFO *info2;
    void * bits;
    void * bits2;
    UINT dib_size, dib32_size;
    DWORD pixel;
    HDC dib_dc, dc;
    HBITMAP old_bmp;
    UINT i;
    int res;

    info = HeapAlloc(GetProcessHeap(), 0, FIELD_OFFSET(BITMAPINFO, bmiColors[256]));
    info2 = HeapAlloc(GetProcessHeap(), 0, FIELD_OFFSET(BITMAPINFO, bmiColors[256]));

    /* Create a DIB section with a color table */

    info->bmiHeader.biSize          = sizeof(info->bmiHeader);
    info->bmiHeader.biWidth         = 32;
    info->bmiHeader.biHeight        = 32;
    info->bmiHeader.biPlanes        = 1;
    info->bmiHeader.biBitCount      = bpp;
    info->bmiHeader.biCompression   = BI_RGB;
    info->bmiHeader.biXPelsPerMeter = 0;
    info->bmiHeader.biYPelsPerMeter = 0;
    info->bmiHeader.biClrUsed       = 0;
    info->bmiHeader.biClrImportant  = 0;

    for (i=0; i < (1u << bpp); i++)
    {
        BYTE c = i * (1 << (8 - bpp));
        info->bmiColors[i].rgbRed = c;
        info->bmiColors[i].rgbGreen = c;
        info->bmiColors[i].rgbBlue = c;
        info->bmiColors[i].rgbReserved = 0;
    }

    dib = CreateDIBSection(NULL, info, DIB_RGB_COLORS, &bits, NULL, 0);
    dib_size = bpp * (info->bmiHeader.biWidth * info->bmiHeader.biHeight) / 8;
    dib32_size = 32 * (info->bmiHeader.biWidth * info->bmiHeader.biHeight) / 8;

    /* Set the bits of the DIB section */
    for (i=0; i < dib_size; i++)
    {
        ((BYTE *)bits)[i] = i % 256;
    }

    /* Select the DIB into a DC */
    dib_dc = CreateCompatibleDC(NULL);
    old_bmp = SelectObject(dib_dc, dib);
    dc = CreateCompatibleDC(NULL);
    bits2 = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dib32_size);

    /* Copy the DIB attributes but not the color table */
    memcpy(info2, info, sizeof(BITMAPINFOHEADER));

    res = GetDIBits(dc, dib, 0, info->bmiHeader.biHeight, bits2, info2, DIB_RGB_COLORS);
    ok( res == info->bmiHeader.biHeight, "got %d (bpp %d)\n", res, bpp );

    /* Compare the color table and the bits */
    for (i=0; i < (1u << bpp); i++)
        ok( info->bmiColors[i].rgbRed      == info2->bmiColors[i].rgbRed   &&
            info->bmiColors[i].rgbGreen    == info2->bmiColors[i].rgbGreen &&
            info->bmiColors[i].rgbBlue     == info2->bmiColors[i].rgbBlue  &&
            info->bmiColors[i].rgbReserved == info2->bmiColors[i].rgbReserved,
            "color table entry %d differs (bpp %d)\n", i, bpp );

    ok( !memcmp( bits, bits2, dib_size ), "bit mismatch (bpp %d)\n", bpp );

    /* Test various combinations of lines = 0 and bits2 = NULL */
    memset( info2->bmiColors, 0xcc, 256 * sizeof(RGBQUAD) );
    res = GetDIBits( dc, dib, 0, 0, bits2, info2, DIB_RGB_COLORS );
    ok( res == 1, "got %d (bpp %d)\n", res, bpp );
    ok( !memcmp( info->bmiColors, info2->bmiColors, (1 << bpp) * sizeof(RGBQUAD) ),
        "color table mismatch (bpp %d)\n", bpp );

    memset( info2->bmiColors, 0xcc, 256 * sizeof(RGBQUAD) );
    res = GetDIBits( dc, dib, 0, 0, NULL, info2, DIB_RGB_COLORS );
    ok( res == 1, "got %d (bpp %d)\n", res, bpp );
    ok( !memcmp( info->bmiColors, info2->bmiColors, (1 << bpp) * sizeof(RGBQUAD) ),
        "color table mismatch (bpp %d)\n", bpp );

    memset( info2->bmiColors, 0xcc, 256 * sizeof(RGBQUAD) );
    res = GetDIBits( dc, dib, 0, info->bmiHeader.biHeight, NULL, info2, DIB_RGB_COLORS );
    ok( res == 1, "got %d (bpp %d)\n", res, bpp );
    ok( !memcmp( info->bmiColors, info2->bmiColors, (1 << bpp) * sizeof(RGBQUAD) ),
        "color table mismatch (bpp %d)\n", bpp );

    /* Map into a 32bit-DIB */
    info2->bmiHeader.biBitCount = 32;
    res = GetDIBits(dc, dib, 0, info->bmiHeader.biHeight, bits2, info2, DIB_RGB_COLORS);
    ok( res == info->bmiHeader.biHeight, "got %d (bpp %d)\n", res, bpp );

    /* Check if last pixel was set */
    pixel = ((DWORD *)bits2)[info->bmiHeader.biWidth * info->bmiHeader.biHeight - 1];
    ok(pixel != 0, "Pixel: 0x%08x\n", pixel);

    HeapFree(GetProcessHeap(), 0, bits2);
    DeleteDC(dc);

    SelectObject(dib_dc, old_bmp);
    DeleteDC(dib_dc);
    DeleteObject(dib);
    HeapFree(GetProcessHeap(), 0, info2);
    HeapFree(GetProcessHeap(), 0, info);
}

static void test_GetDIBits_selected_DDB(BOOL monochrome)
{
    HBITMAP ddb;
    BITMAPINFO *info;
    BITMAPINFO *info2;
    void * bits;
    void * bits2;
    HDC ddb_dc, dc;
    HBITMAP old_bmp;
    UINT width, height;
    UINT bpp;
    UINT i, j;
    int res;

    info = HeapAlloc(GetProcessHeap(), 0, FIELD_OFFSET(BITMAPINFO, bmiColors[256]));
    info2 = HeapAlloc(GetProcessHeap(), 0, FIELD_OFFSET(BITMAPINFO, bmiColors[256]));

    width = height = 16;

    /* Create a DDB (device-dependent bitmap) */
    if (monochrome)
    {
        bpp = 1;
        ddb = CreateBitmap(width, height, 1, 1, NULL);
    }
    else
    {
        HDC screen_dc = GetDC(NULL);
        bpp = GetDeviceCaps(screen_dc, BITSPIXEL) * GetDeviceCaps(screen_dc, PLANES);
        ddb = CreateCompatibleBitmap(screen_dc, width, height);
        ReleaseDC(NULL, screen_dc);
    }

    /* Set the pixels */
    ddb_dc = CreateCompatibleDC(NULL);
    old_bmp = SelectObject(ddb_dc, ddb);
    for (i = 0; i < width; i++)
    {
        for (j=0; j < height; j++)
        {
            BYTE c = (i * width + j) % 256;
            SetPixelV(ddb_dc, i, j, RGB(c, c, c));
        }
    }
    SelectObject(ddb_dc, old_bmp);

    info->bmiHeader.biSize = sizeof(info->bmiHeader);
    info->bmiHeader.biWidth = width;
    info->bmiHeader.biHeight = height;
    info->bmiHeader.biPlanes = 1;
    info->bmiHeader.biBitCount = bpp;
    info->bmiHeader.biCompression = BI_RGB;

    dc = CreateCompatibleDC(NULL);

    /* Fill in biSizeImage */
    GetDIBits(dc, ddb, 0, height, NULL, info, DIB_RGB_COLORS);
    ok(info->bmiHeader.biSizeImage != 0, "GetDIBits failed to get the DIB attributes\n");

    bits = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, info->bmiHeader.biSizeImage);
    bits2 = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, info->bmiHeader.biSizeImage);

    /* Get the bits */
    res = GetDIBits(dc, ddb, 0, height, bits, info, DIB_RGB_COLORS);
    ok( res == height, "got %d (bpp %d)\n", res, bpp );

    /* Copy the DIB attributes but not the color table */
    memcpy(info2, info, sizeof(BITMAPINFOHEADER));

    /* Select the DDB into another DC */
    old_bmp = SelectObject(ddb_dc, ddb);

    /* Get the bits */
    res = GetDIBits(dc, ddb, 0, height, bits2, info2, DIB_RGB_COLORS);
    ok( res == height, "got %d (bpp %d)\n", res, bpp );

    /* Compare the color table and the bits */
    if (bpp <= 8)
    {
        for (i=0; i < (1u << bpp); i++)
            ok( info->bmiColors[i].rgbRed      == info2->bmiColors[i].rgbRed   &&
                info->bmiColors[i].rgbGreen    == info2->bmiColors[i].rgbGreen &&
                info->bmiColors[i].rgbBlue     == info2->bmiColors[i].rgbBlue  &&
                info->bmiColors[i].rgbReserved == info2->bmiColors[i].rgbReserved,
                "color table entry %d differs (bpp %d)\n", i, bpp );
    }

    ok( !memcmp( bits, bits2, info->bmiHeader.biSizeImage ), "bit mismatch (bpp %d)\n", bpp );

    /* Test the palette */
    if (info2->bmiHeader.biBitCount <= 8)
    {
        WORD *colors = (WORD*)info2->bmiColors;

        /* Get the palette indices */
        res = GetDIBits(dc, ddb, 0, 0, NULL, info2, DIB_PAL_COLORS);
        ok( res == 1, "got %d (bpp %d)\n", res, bpp );

        for (i = 0; i < (1 << info->bmiHeader.biBitCount); i++)
            ok( colors[i] == i, "%d: got %d (bpp %d)\n", i, colors[i], bpp );
    }

    HeapFree(GetProcessHeap(), 0, bits2);
    HeapFree(GetProcessHeap(), 0, bits);
    DeleteDC(dc);

    SelectObject(ddb_dc, old_bmp);
    DeleteDC(ddb_dc);
    DeleteObject(ddb);
    HeapFree(GetProcessHeap(), 0, info2);
    HeapFree(GetProcessHeap(), 0, info);
}

static void test_GetDIBits(void)
{
    /* 2-bytes aligned 1-bit bitmap data: 16x16 */
    static const BYTE bmp_bits_1[16 * 2] =
    {
        0xff,0xff, 0,0, 0xff,0xff, 0,0,
        0xff,0xff, 0,0, 0xff,0xff, 0,0,
        0xff,0xff, 0,0, 0xff,0xff, 0,0,
        0xff,0xff, 0,0, 0xff,0xff, 0,0
    };
    /* 4-bytes aligned 1-bit DIB data: 16x16 */
    static const BYTE dib_bits_1[16 * 4] =
    {
        0,0,0,0, 0xff,0xff,0,0, 0,0,0,0, 0xff,0xff,0,0,
        0,0,0,0, 0xff,0xff,0,0, 0,0,0,0, 0xff,0xff,0,0,
        0,0,0,0, 0xff,0xff,0,0, 0,0,0,0, 0xff,0xff,0,0,
        0,0,0,0, 0xff,0xff,0,0, 0,0,0,0, 0xff,0xff,0,0
    };
    /* 2-bytes aligned 24-bit bitmap data: 16x16 */
    static const BYTE bmp_bits_24[16 * 16*3] =
    {
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    };
    /* 4-bytes aligned 24-bit DIB data: 16x16 */
    static const BYTE dib_bits_24[16 * 16*3] =
    {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff
    };
    HBITMAP hbmp;
    BITMAP bm;
    HDC hdc;
    int i, bytes, lines;
    BYTE buf[1024];
    char bi_buf[sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * 256];
    BITMAPINFO *bi = (BITMAPINFO *)bi_buf;
    RGBQUAD *colors = bi->bmiColors;
    PALETTEENTRY pal_ents[20];

    hdc = GetDC(0);

    /* 1-bit source bitmap data */
    hbmp = CreateBitmap(16, 16, 1, 1, bmp_bits_1);
    ok(hbmp != 0, "CreateBitmap failed\n");

    memset(&bm, 0xAA, sizeof(bm));
    bytes = GetObjectW(hbmp, sizeof(bm), &bm);
    ok(bytes == sizeof(bm), "GetObject returned %d\n", bytes);
    ok(bm.bmType == 0, "wrong bmType %d\n", bm.bmType);
    ok(bm.bmWidth == 16, "wrong bmWidth %d\n", bm.bmWidth);
    ok(bm.bmHeight == 16, "wrong bmHeight %d\n", bm.bmHeight);
    ok(bm.bmWidthBytes == 2, "wrong bmWidthBytes %d\n", bm.bmWidthBytes);
    ok(bm.bmPlanes == 1, "wrong bmPlanes %u\n", bm.bmPlanes);
    ok(bm.bmBitsPixel == 1, "wrong bmBitsPixel %d\n", bm.bmBitsPixel);
    ok(!bm.bmBits, "wrong bmBits %p\n", bm.bmBits);

    bytes = GetBitmapBits(hbmp, 0, NULL);
    ok(bytes == sizeof(bmp_bits_1), "expected 16*2 got %d bytes\n", bytes);
    bytes = GetBitmapBits(hbmp, sizeof(buf), buf);
    ok(bytes == sizeof(bmp_bits_1), "expected 16*2 got %d bytes\n", bytes);
    ok(!memcmp(buf, bmp_bits_1, sizeof(bmp_bits_1)), "bitmap bits don't match\n");

    /* retrieve 1-bit DIB data */
    memset(bi, 0, sizeof(*bi));
    bi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi->bmiHeader.biWidth = bm.bmWidth;
    bi->bmiHeader.biHeight = bm.bmHeight;
    bi->bmiHeader.biPlanes = 1;
    bi->bmiHeader.biBitCount = 1;
    bi->bmiHeader.biCompression = BI_RGB;
    bi->bmiHeader.biClrUsed = 37;
    bi->bmiHeader.biSizeImage = 0;
    memset(colors, 0xAA, sizeof(RGBQUAD) * 256);
    SetLastError(0xdeadbeef);
    lines = GetDIBits(0, hbmp, 0, bm.bmHeight, buf, bi, DIB_RGB_COLORS);
    ok(lines == 0, "GetDIBits copied %d lines with hdc = 0\n", lines);
    ok(GetLastError() == ERROR_INVALID_PARAMETER ||
       broken(GetLastError() == 0xdeadbeef), /* winnt */
       "wrong error %u\n", GetLastError());
    ok(bi->bmiHeader.biSizeImage == 0, "expected 0, got %u\n", bi->bmiHeader.biSizeImage);
    ok(bi->bmiHeader.biClrUsed == 37 || broken(bi->bmiHeader.biClrUsed == 0),
       "wrong biClrUsed %u\n", bi->bmiHeader.biClrUsed);

    memset(buf, 0xAA, sizeof(buf));
    SetLastError(0xdeadbeef);
    lines = GetDIBits(hdc, hbmp, 0, bm.bmHeight, buf, bi, DIB_RGB_COLORS);
    ok(lines == bm.bmHeight, "GetDIBits copied %d lines of %d, error %u\n",
       lines, bm.bmHeight, GetLastError());
    ok(bi->bmiHeader.biSizeImage == sizeof(dib_bits_1), "expected 16*4, got %u\n", bi->bmiHeader.biSizeImage);
    ok(bi->bmiHeader.biClrUsed == 0, "wrong biClrUsed %u\n", bi->bmiHeader.biClrUsed);

    /* the color table consists of black and white */
    ok(colors[0].rgbRed == 0 && colors[0].rgbGreen == 0 &&
       colors[0].rgbBlue == 0 && colors[0].rgbReserved == 0,
       "expected bmiColors[0] 0,0,0,0 - got %x %x %x %x\n",
       colors[0].rgbRed, colors[0].rgbGreen, colors[0].rgbBlue, colors[0].rgbReserved);
    ok(colors[1].rgbRed == 0xff && colors[1].rgbGreen == 0xff &&
       colors[1].rgbBlue == 0xff && colors[1].rgbReserved == 0,
       "expected bmiColors[0] 0xff,0xff,0xff,0 - got %x %x %x %x\n",
       colors[1].rgbRed, colors[1].rgbGreen, colors[1].rgbBlue, colors[1].rgbReserved);
    for (i = 2; i < 256; i++)
    {
        ok(colors[i].rgbRed == 0xAA && colors[i].rgbGreen == 0xAA &&
           colors[i].rgbBlue == 0xAA && colors[i].rgbReserved == 0xAA,
           "expected bmiColors[%d] 0xAA,0xAA,0xAA,0xAA - got %x %x %x %x\n", i,
           colors[i].rgbRed, colors[i].rgbGreen, colors[i].rgbBlue, colors[i].rgbReserved);
    }

    /* returned bits are DWORD aligned and upside down */
    ok(!memcmp(buf, dib_bits_1, sizeof(dib_bits_1)), "DIB bits don't match\n");

    /* Test the palette indices */
    memset(colors, 0xAA, sizeof(RGBQUAD) * 256);
    SetLastError(0xdeadbeef);
    lines = GetDIBits(hdc, hbmp, 0, 0, NULL, bi, DIB_PAL_COLORS);
    ok(((WORD*)colors)[0] == 0, "Color 0 is %d\n", ((WORD*)colors)[0]);
    ok(((WORD*)colors)[1] == 1, "Color 1 is %d\n", ((WORD*)colors)[1]);
    for (i = 2; i < 256; i++)
        ok(((WORD*)colors)[i] == 0xAAAA, "Color %d is %d\n", i, ((WORD*)colors)[1]);

    /* retrieve 24-bit DIB data */
    memset(bi, 0, sizeof(*bi));
    bi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi->bmiHeader.biWidth = bm.bmWidth;
    bi->bmiHeader.biHeight = bm.bmHeight;
    bi->bmiHeader.biPlanes = 1;
    bi->bmiHeader.biBitCount = 24;
    bi->bmiHeader.biCompression = BI_RGB;
    bi->bmiHeader.biClrUsed = 37;
    bi->bmiHeader.biSizeImage = 0;
    memset(colors, 0xAA, sizeof(RGBQUAD) * 256);
    memset(buf, 0xAA, sizeof(buf));
    SetLastError(0xdeadbeef);
    lines = GetDIBits(hdc, hbmp, 0, bm.bmHeight, buf, bi, DIB_RGB_COLORS);
    ok(lines == bm.bmHeight, "GetDIBits copied %d lines of %d, error %u\n",
       lines, bm.bmHeight, GetLastError());
    ok(bi->bmiHeader.biSizeImage == sizeof(dib_bits_24), "expected 16*16*3, got %u\n", bi->bmiHeader.biSizeImage);
    ok(bi->bmiHeader.biClrUsed == 0, "wrong biClrUsed %u\n", bi->bmiHeader.biClrUsed);

    /* the color table doesn't exist for 24-bit images */
    for (i = 0; i < 256; i++)
    {
        ok(colors[i].rgbRed == 0xAA && colors[i].rgbGreen == 0xAA &&
           colors[i].rgbBlue == 0xAA && colors[i].rgbReserved == 0xAA,
           "expected bmiColors[%d] 0xAA,0xAA,0xAA,0xAA - got %x %x %x %x\n", i,
           colors[i].rgbRed, colors[i].rgbGreen, colors[i].rgbBlue, colors[i].rgbReserved);
    }

    /* returned bits are DWORD aligned and upside down */
    ok(!memcmp(buf, dib_bits_24, sizeof(dib_bits_24)), "DIB bits don't match\n");
    DeleteObject(hbmp);

    /* 24-bit source bitmap data */
    hbmp = CreateCompatibleBitmap(hdc, 16, 16);
    ok(hbmp != 0, "CreateBitmap failed\n");
    SetLastError(0xdeadbeef);
    bi->bmiHeader.biHeight = -bm.bmHeight; /* indicate bottom-up data */
    lines = SetDIBits(hdc, hbmp, 0, bm.bmHeight, bmp_bits_24, bi, DIB_RGB_COLORS);
    ok(lines == bm.bmHeight, "SetDIBits copied %d lines of %d, error %u\n",
       lines, bm.bmHeight, GetLastError());

    memset(&bm, 0xAA, sizeof(bm));
    bytes = GetObjectW(hbmp, sizeof(bm), &bm);
    ok(bytes == sizeof(bm), "GetObject returned %d\n", bytes);
    ok(bm.bmType == 0, "wrong bmType %d\n", bm.bmType);
    ok(bm.bmWidth == 16, "wrong bmWidth %d\n", bm.bmWidth);
    ok(bm.bmHeight == 16, "wrong bmHeight %d\n", bm.bmHeight);
    ok(bm.bmWidthBytes == get_bitmap_stride(bm.bmWidth, bm.bmBitsPixel), "wrong bmWidthBytes %d\n", bm.bmWidthBytes);
    ok(bm.bmPlanes == GetDeviceCaps(hdc, PLANES), "wrong bmPlanes %u\n", bm.bmPlanes);
    ok(bm.bmBitsPixel == GetDeviceCaps(hdc, BITSPIXEL), "wrong bmBitsPixel %d\n", bm.bmBitsPixel);
    ok(!bm.bmBits, "wrong bmBits %p\n", bm.bmBits);

    bytes = GetBitmapBits(hbmp, 0, NULL);
    ok(bytes == bm.bmWidthBytes * bm.bmHeight, "expected %d got %d bytes\n", bm.bmWidthBytes * bm.bmHeight, bytes);
    bytes = GetBitmapBits(hbmp, sizeof(buf), buf);
    ok(bytes == bm.bmWidthBytes * bm.bmHeight, "expected %d got %d bytes\n",
       bm.bmWidthBytes * bm.bmHeight, bytes);

    /* retrieve 1-bit DIB data */
    memset(bi, 0, sizeof(*bi));
    bi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi->bmiHeader.biWidth = bm.bmWidth;
    bi->bmiHeader.biHeight = bm.bmHeight;
    bi->bmiHeader.biPlanes = 1;
    bi->bmiHeader.biBitCount = 1;
    bi->bmiHeader.biCompression = BI_RGB;
    bi->bmiHeader.biClrUsed = 37;
    bi->bmiHeader.biSizeImage = 0;
    memset(colors, 0xAA, sizeof(RGBQUAD) * 256);
    memset(buf, 0xAA, sizeof(buf));
    SetLastError(0xdeadbeef);
    lines = GetDIBits(hdc, hbmp, 0, bm.bmHeight, buf, bi, DIB_RGB_COLORS);
    ok(lines == bm.bmHeight, "GetDIBits copied %d lines of %d, error %u\n",
       lines, bm.bmHeight, GetLastError());
    ok(bi->bmiHeader.biSizeImage == sizeof(dib_bits_1), "expected 16*4, got %u\n", bi->bmiHeader.biSizeImage);
    ok(bi->bmiHeader.biClrUsed == 0, "wrong biClrUsed %u\n", bi->bmiHeader.biClrUsed);

    /* the color table consists of black and white */
    ok(colors[0].rgbRed == 0 && colors[0].rgbGreen == 0 &&
       colors[0].rgbBlue == 0 && colors[0].rgbReserved == 0,
       "expected bmiColors[0] 0,0,0,0 - got %x %x %x %x\n",
       colors[0].rgbRed, colors[0].rgbGreen, colors[0].rgbBlue, colors[0].rgbReserved);
    ok(colors[1].rgbRed == 0xff && colors[1].rgbGreen == 0xff &&
       colors[1].rgbBlue == 0xff && colors[1].rgbReserved == 0,
       "expected bmiColors[0] 0xff,0xff,0xff,0 - got %x %x %x %x\n",
       colors[1].rgbRed, colors[1].rgbGreen, colors[1].rgbBlue, colors[1].rgbReserved);
    for (i = 2; i < 256; i++)
    {
        ok(colors[i].rgbRed == 0xAA && colors[i].rgbGreen == 0xAA &&
           colors[i].rgbBlue == 0xAA && colors[i].rgbReserved == 0xAA,
           "expected bmiColors[%d] 0xAA,0xAA,0xAA,0xAA - got %x %x %x %x\n", i,
           colors[i].rgbRed, colors[i].rgbGreen, colors[i].rgbBlue, colors[i].rgbReserved);
    }

    /* returned bits are DWORD aligned and upside down */
    ok(!memcmp(buf, dib_bits_1, sizeof(dib_bits_1)), "DIB bits don't match\n");

    /* Test the palette indices */
    memset(colors, 0xAA, sizeof(RGBQUAD) * 256);
    SetLastError(0xdeadbeef);
    lines = GetDIBits(hdc, hbmp, 0, 0, NULL, bi, DIB_PAL_COLORS);
    ok(((WORD*)colors)[0] == 0, "Color 0 is %d\n", ((WORD*)colors)[0]);
    ok(((WORD*)colors)[1] == 1, "Color 1 is %d\n", ((WORD*)colors)[1]);
    for (i = 2; i < 256; i++)
        ok(((WORD*)colors)[i] == 0xAAAA, "Color %d is %d\n", i, ((WORD*)colors)[i]);

    /* retrieve 4-bit DIB data */
    memset(bi, 0, sizeof(*bi));
    bi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi->bmiHeader.biWidth = bm.bmWidth;
    bi->bmiHeader.biHeight = bm.bmHeight;
    bi->bmiHeader.biPlanes = 1;
    bi->bmiHeader.biBitCount = 4;
    bi->bmiHeader.biCompression = BI_RGB;
    bi->bmiHeader.biClrUsed = 37;
    bi->bmiHeader.biSizeImage = 0;
    memset(colors, 0xAA, sizeof(RGBQUAD) * 256);
    memset(buf, 0xAA, sizeof(buf));
    SetLastError(0xdeadbeef);
    lines = GetDIBits(hdc, hbmp, 0, bm.bmHeight, buf, bi, DIB_RGB_COLORS);
    ok(lines == bm.bmHeight, "GetDIBits copied %d lines of %d, error %u\n",
       lines, bm.bmHeight, GetLastError());
    ok(bi->bmiHeader.biClrUsed == 0, "wrong biClrUsed %u\n", bi->bmiHeader.biClrUsed);

    GetPaletteEntries( GetStockObject(DEFAULT_PALETTE), 0, 20, pal_ents );

    for (i = 0; i < 16; i++)
    {
        RGBQUAD expect;
        int entry = i < 8 ? i : i + 4;

        if(entry == 7) entry = 12;
        else if(entry == 12) entry = 7;

        expect.rgbRed   = pal_ents[entry].peRed;
        expect.rgbGreen = pal_ents[entry].peGreen;
        expect.rgbBlue  = pal_ents[entry].peBlue;
        expect.rgbReserved = 0;

        ok(!memcmp(colors + i, &expect, sizeof(expect)),
           "expected bmiColors[%d] %x %x %x %x - got %x %x %x %x\n", i,
           expect.rgbRed, expect.rgbGreen, expect.rgbBlue, expect.rgbReserved,
           colors[i].rgbRed, colors[i].rgbGreen, colors[i].rgbBlue, colors[i].rgbReserved);
    }

    /* retrieve 8-bit DIB data */
    memset(bi, 0, sizeof(*bi));
    bi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi->bmiHeader.biWidth = bm.bmWidth;
    bi->bmiHeader.biHeight = bm.bmHeight;
    bi->bmiHeader.biPlanes = 1;
    bi->bmiHeader.biBitCount = 8;
    bi->bmiHeader.biCompression = BI_RGB;
    bi->bmiHeader.biClrUsed = 37;
    bi->bmiHeader.biSizeImage = 0;
    memset(colors, 0xAA, sizeof(RGBQUAD) * 256);
    memset(buf, 0xAA, sizeof(buf));
    SetLastError(0xdeadbeef);
    lines = GetDIBits(hdc, hbmp, 0, bm.bmHeight, buf, bi, DIB_RGB_COLORS);
    ok(lines == bm.bmHeight, "GetDIBits copied %d lines of %d, error %u\n",
       lines, bm.bmHeight, GetLastError());
    ok(bi->bmiHeader.biClrUsed == 0, "wrong biClrUsed %u\n", bi->bmiHeader.biClrUsed);

    GetPaletteEntries( GetStockObject(DEFAULT_PALETTE), 0, 20, pal_ents );

    for (i = 0; i < 256; i++)
    {
        RGBQUAD expect;

        if (i < 10 || i >= 246)
        {
            int entry = i < 10 ? i : i - 236;
            expect.rgbRed   = pal_ents[entry].peRed;
            expect.rgbGreen = pal_ents[entry].peGreen;
            expect.rgbBlue  = pal_ents[entry].peBlue;
        }
        else
        {
            expect.rgbRed   = (i & 0x07) << 5;
            expect.rgbGreen = (i & 0x38) << 2;
            expect.rgbBlue  =  i & 0xc0;
        }
        expect.rgbReserved = 0;

        ok(!memcmp(colors + i, &expect, sizeof(expect)),
           "expected bmiColors[%d] %x %x %x %x - got %x %x %x %x\n", i,
           expect.rgbRed, expect.rgbGreen, expect.rgbBlue, expect.rgbReserved,
           colors[i].rgbRed, colors[i].rgbGreen, colors[i].rgbBlue, colors[i].rgbReserved);
    }

    /* retrieve 24-bit DIB data */
    memset(bi, 0, sizeof(*bi));
    bi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi->bmiHeader.biWidth = bm.bmWidth;
    bi->bmiHeader.biHeight = bm.bmHeight;
    bi->bmiHeader.biPlanes = 1;
    bi->bmiHeader.biBitCount = 24;
    bi->bmiHeader.biCompression = BI_RGB;
    bi->bmiHeader.biClrUsed = 37;
    bi->bmiHeader.biSizeImage = 0;
    memset(colors, 0xAA, sizeof(RGBQUAD) * 256);
    memset(buf, 0xAA, sizeof(buf));
    SetLastError(0xdeadbeef);
    lines = GetDIBits(hdc, hbmp, 0, bm.bmHeight, buf, bi, DIB_RGB_COLORS);
    ok(lines == bm.bmHeight, "GetDIBits copied %d lines of %d, error %u\n",
       lines, bm.bmHeight, GetLastError());
    ok(bi->bmiHeader.biSizeImage == sizeof(dib_bits_24), "expected 16*16*3, got %u\n", bi->bmiHeader.biSizeImage);
    ok(bi->bmiHeader.biClrUsed == 0, "wrong biClrUsed %u\n", bi->bmiHeader.biClrUsed);

    /* the color table doesn't exist for 24-bit images */
    for (i = 0; i < 256; i++)
    {
        ok(colors[i].rgbRed == 0xAA && colors[i].rgbGreen == 0xAA &&
           colors[i].rgbBlue == 0xAA && colors[i].rgbReserved == 0xAA,
           "expected bmiColors[%d] 0xAA,0xAA,0xAA,0xAA - got %x %x %x %x\n", i,
           colors[i].rgbRed, colors[i].rgbGreen, colors[i].rgbBlue, colors[i].rgbReserved);
    }

    /* returned bits are DWORD aligned and upside down */
    ok(!memcmp(buf, dib_bits_24, sizeof(dib_bits_24)), "DIB bits don't match\n");
    DeleteObject(hbmp);

    ReleaseDC(0, hdc);
}

static void test_GetDIBits_BI_BITFIELDS(void)
{
    /* Try a screen resolution detection technique
     * from the September 1999 issue of Windows Developer's Journal
     * which seems to be in widespread use.
     * http://www.lesher.ws/highcolor.html
     * http://www.lesher.ws/vidfmt.c
     * It hinges on being able to retrieve the bitmaps
     * for the three primary colors in non-paletted 16 bit mode.
     */
    char dibinfo_buf[sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD)];
    DWORD bits[32];
    LPBITMAPINFO dibinfo = (LPBITMAPINFO) dibinfo_buf;
    DWORD *bitmasks = (DWORD *)dibinfo->bmiColors;
    HDC hdc;
    HBITMAP hbm;
    int ret;
    void *ptr;

    memset(dibinfo, 0, sizeof(dibinfo_buf));
    dibinfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

    hdc = GetDC(NULL);
    ok(hdc != NULL, "GetDC failed?\n");
    hbm = CreateCompatibleBitmap(hdc, 1, 1);
    ok(hbm != NULL, "CreateCompatibleBitmap failed?\n");

    /* Call GetDIBits to fill in bmiHeader.  */
    ret = GetDIBits(hdc, hbm, 0, 1, NULL, dibinfo, DIB_RGB_COLORS);
    ok(ret == 1, "GetDIBits failed\n");
    if (dibinfo->bmiHeader.biBitCount > 8)
    {
        ok( dibinfo->bmiHeader.biCompression == BI_BITFIELDS ||
            broken( dibinfo->bmiHeader.biCompression == BI_RGB ), /* nt4 sp3 */
            "compression is %u (%d bpp)\n", dibinfo->bmiHeader.biCompression, dibinfo->bmiHeader.biBitCount );

        if (dibinfo->bmiHeader.biCompression == BI_BITFIELDS)
        {
            ok( !bitmasks[0], "red mask is set\n" );
            ok( !bitmasks[1], "green mask is set\n" );
            ok( !bitmasks[2], "blue mask is set\n" );

            /* test with NULL bits pointer and correct bpp */
            dibinfo->bmiHeader.biSizeImage = 0xdeadbeef;
            ret = GetDIBits(hdc, hbm, 0, 1, NULL, dibinfo, DIB_RGB_COLORS);
            ok(ret == 1, "GetDIBits failed\n");

            ok( bitmasks[0] != 0, "red mask is not set\n" );
            ok( bitmasks[1] != 0, "green mask is not set\n" );
            ok( bitmasks[2] != 0, "blue mask is not set\n" );
            ok( dibinfo->bmiHeader.biSizeImage != 0xdeadbeef, "size image not set\n" );

            /* test with valid bits pointer */
            memset(dibinfo, 0, sizeof(dibinfo_buf));
            dibinfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            ret = GetDIBits(hdc, hbm, 0, 1, NULL, dibinfo, DIB_RGB_COLORS);
            ok(ret == 1, "GetDIBits failed ret %u err %u\n",ret,GetLastError());
            dibinfo->bmiHeader.biSizeImage = 0xdeadbeef;
            ret = GetDIBits(hdc, hbm, 0, 1, bits, dibinfo, DIB_RGB_COLORS);
            ok(ret == 1, "GetDIBits failed ret %u err %u\n",ret,GetLastError());

            ok( bitmasks[0] != 0, "red mask is not set\n" );
            ok( bitmasks[1] != 0, "green mask is not set\n" );
            ok( bitmasks[2] != 0, "blue mask is not set\n" );
            ok( dibinfo->bmiHeader.biSizeImage != 0xdeadbeef, "size image not set\n" );

            /* now with bits and 0 lines */
            memset(dibinfo, 0, sizeof(dibinfo_buf));
            dibinfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            dibinfo->bmiHeader.biSizeImage = 0xdeadbeef;
            SetLastError(0xdeadbeef);
            ret = GetDIBits(hdc, hbm, 0, 0, bits, dibinfo, DIB_RGB_COLORS);
            ok(ret == 1, "GetDIBits failed ret %u err %u\n",ret,GetLastError());

            ok( !bitmasks[0], "red mask is set\n" );
            ok( !bitmasks[1], "green mask is set\n" );
            ok( !bitmasks[2], "blue mask is set\n" );
            ok( dibinfo->bmiHeader.biSizeImage != 0xdeadbeef, "size image not set\n" );

            memset(bitmasks, 0, 3*sizeof(DWORD));
            dibinfo->bmiHeader.biSizeImage = 0xdeadbeef;
            ret = GetDIBits(hdc, hbm, 0, 0, bits, dibinfo, DIB_RGB_COLORS);
            ok(ret == 1, "GetDIBits failed ret %u err %u\n",ret,GetLastError());

            ok( bitmasks[0] != 0, "red mask is not set\n" );
            ok( bitmasks[1] != 0, "green mask is not set\n" );
            ok( bitmasks[2] != 0, "blue mask is not set\n" );
            ok( dibinfo->bmiHeader.biSizeImage != 0xdeadbeef, "size image not set\n" );
        }
    }
    else skip("bitmap in colortable mode, skipping BI_BITFIELDS tests\n");

    DeleteObject(hbm);

    /* same thing now with a 32-bpp DIB section */

    dibinfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    dibinfo->bmiHeader.biWidth = 1;
    dibinfo->bmiHeader.biHeight = 1;
    dibinfo->bmiHeader.biPlanes = 1;
    dibinfo->bmiHeader.biBitCount = 32;
    dibinfo->bmiHeader.biCompression = BI_RGB;
    dibinfo->bmiHeader.biSizeImage = 0;
    dibinfo->bmiHeader.biXPelsPerMeter = 0;
    dibinfo->bmiHeader.biYPelsPerMeter = 0;
    dibinfo->bmiHeader.biClrUsed = 0;
    dibinfo->bmiHeader.biClrImportant = 0;
    bitmasks[0] = 0x0000ff;
    bitmasks[1] = 0x00ff00;
    bitmasks[2] = 0xff0000;
    hbm = CreateDIBSection( hdc, dibinfo, DIB_RGB_COLORS, &ptr, NULL, 0 );
    ok( hbm != 0, "failed to create bitmap\n" );

    memset(dibinfo, 0, sizeof(dibinfo_buf));
    dibinfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    ret = GetDIBits(hdc, hbm, 0, 0, NULL, dibinfo, DIB_RGB_COLORS);
    ok(ret == 1, "GetDIBits failed\n");
    ok( dibinfo->bmiHeader.biBitCount == 32, "wrong bit count %u\n", dibinfo->bmiHeader.biBitCount );

    ok( dibinfo->bmiHeader.biCompression == BI_BITFIELDS ||
        broken( dibinfo->bmiHeader.biCompression == BI_RGB ), /* nt4 sp3 */
        "compression is %u\n", dibinfo->bmiHeader.biCompression );
    ok( !bitmasks[0], "red mask is set\n" );
    ok( !bitmasks[1], "green mask is set\n" );
    ok( !bitmasks[2], "blue mask is set\n" );

    dibinfo->bmiHeader.biSizeImage = 0xdeadbeef;
    ret = GetDIBits(hdc, hbm, 0, 1, bits, dibinfo, DIB_RGB_COLORS);
    ok(ret == 1, "GetDIBits failed\n");
    ok( dibinfo->bmiHeader.biBitCount == 32, "wrong bit count %u\n", dibinfo->bmiHeader.biBitCount );
    ok( dibinfo->bmiHeader.biCompression == BI_BITFIELDS ||
        broken( dibinfo->bmiHeader.biCompression == BI_RGB ), /* nt4 sp3 */
        "compression is %u\n", dibinfo->bmiHeader.biCompression );
    if (dibinfo->bmiHeader.biCompression == BI_BITFIELDS)
    {
        ok( bitmasks[0] == 0xff0000, "wrong red mask %08x\n", bitmasks[0] );
        ok( bitmasks[1] == 0x00ff00, "wrong green mask %08x\n", bitmasks[1] );
        ok( bitmasks[2] == 0x0000ff, "wrong blue mask %08x\n", bitmasks[2] );
    }
    ok( dibinfo->bmiHeader.biSizeImage != 0xdeadbeef, "size image not set\n" );

    DeleteObject(hbm);

    dibinfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    dibinfo->bmiHeader.biWidth = 1;
    dibinfo->bmiHeader.biHeight = 1;
    dibinfo->bmiHeader.biPlanes = 1;
    dibinfo->bmiHeader.biBitCount = 32;
    dibinfo->bmiHeader.biCompression = BI_BITFIELDS;
    dibinfo->bmiHeader.biSizeImage = 0;
    dibinfo->bmiHeader.biXPelsPerMeter = 0;
    dibinfo->bmiHeader.biYPelsPerMeter = 0;
    dibinfo->bmiHeader.biClrUsed = 0;
    dibinfo->bmiHeader.biClrImportant = 0;
    bitmasks[0] = 0x0000ff;
    bitmasks[1] = 0x00ff00;
    bitmasks[2] = 0xff0000;
    hbm = CreateDIBSection( hdc, dibinfo, DIB_RGB_COLORS, &ptr, NULL, 0 );
    ok( hbm != 0, "failed to create bitmap\n" );

    if (hbm)
    {
        memset(dibinfo, 0, sizeof(dibinfo_buf));
        dibinfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        ret = GetDIBits(hdc, hbm, 0, 0, NULL, dibinfo, DIB_RGB_COLORS);
        ok(ret == 1, "GetDIBits failed\n");

        ok( dibinfo->bmiHeader.biCompression == BI_BITFIELDS,
            "compression is %u\n", dibinfo->bmiHeader.biCompression );
        ok( !bitmasks[0], "red mask is set\n" );
        ok( !bitmasks[1], "green mask is set\n" );
        ok( !bitmasks[2], "blue mask is set\n" );

        dibinfo->bmiHeader.biSizeImage = 0xdeadbeef;
        ret = GetDIBits(hdc, hbm, 0, 1, bits, dibinfo, DIB_RGB_COLORS);
        ok(ret == 1, "GetDIBits failed\n");
        ok( bitmasks[0] == 0x0000ff, "wrong red mask %08x\n", bitmasks[0] );
        ok( bitmasks[1] == 0x00ff00, "wrong green mask %08x\n", bitmasks[1] );
        ok( bitmasks[2] == 0xff0000, "wrong blue mask %08x\n", bitmasks[2] );
        ok( dibinfo->bmiHeader.biSizeImage != 0xdeadbeef, "size image not set\n" );

        DeleteObject(hbm);
    }

    /* 24-bpp DIB sections don't have bitfields */

    dibinfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    dibinfo->bmiHeader.biWidth = 1;
    dibinfo->bmiHeader.biHeight = 1;
    dibinfo->bmiHeader.biPlanes = 1;
    dibinfo->bmiHeader.biBitCount = 24;
    dibinfo->bmiHeader.biCompression = BI_BITFIELDS;
    dibinfo->bmiHeader.biSizeImage = 0;
    dibinfo->bmiHeader.biXPelsPerMeter = 0;
    dibinfo->bmiHeader.biYPelsPerMeter = 0;
    dibinfo->bmiHeader.biClrUsed = 0;
    dibinfo->bmiHeader.biClrImportant = 0;
    hbm = CreateDIBSection( hdc, dibinfo, DIB_RGB_COLORS, &ptr, NULL, 0 );
    ok( hbm == 0, "creating 24-bpp BI_BITFIELDS dibsection should fail\n" );
    dibinfo->bmiHeader.biCompression = BI_RGB;
    hbm = CreateDIBSection( hdc, dibinfo, DIB_RGB_COLORS, &ptr, NULL, 0 );
    ok( hbm != 0, "failed to create bitmap\n" );

    memset(dibinfo, 0, sizeof(dibinfo_buf));
    dibinfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    ret = GetDIBits(hdc, hbm, 0, 0, NULL, dibinfo, DIB_RGB_COLORS);
    ok(ret == 1, "GetDIBits failed\n");
    ok( dibinfo->bmiHeader.biBitCount == 24, "wrong bit count %u\n", dibinfo->bmiHeader.biBitCount );

    ok( dibinfo->bmiHeader.biCompression == BI_RGB,
        "compression is %u\n", dibinfo->bmiHeader.biCompression );
    ok( !bitmasks[0], "red mask is set\n" );
    ok( !bitmasks[1], "green mask is set\n" );
    ok( !bitmasks[2], "blue mask is set\n" );

    dibinfo->bmiHeader.biSizeImage = 0xdeadbeef;
    ret = GetDIBits(hdc, hbm, 0, 1, bits, dibinfo, DIB_RGB_COLORS);
    ok(ret == 1, "GetDIBits failed\n");
    ok( dibinfo->bmiHeader.biBitCount == 24, "wrong bit count %u\n", dibinfo->bmiHeader.biBitCount );
    ok( !bitmasks[0], "red mask is set\n" );
    ok( !bitmasks[1], "green mask is set\n" );
    ok( !bitmasks[2], "blue mask is set\n" );
    ok( dibinfo->bmiHeader.biSizeImage != 0xdeadbeef, "size image not set\n" );

    DeleteObject(hbm);
    ReleaseDC(NULL, hdc);
}

static void test_select_object(void)
{
    HDC hdc;
    HBITMAP hbm, hbm_old;
    INT planes, bpp, i;
    DWORD depths[] = {8, 15, 16, 24, 32};
    BITMAP bm;
    DWORD bytes;

    hdc = GetDC(0);
    ok(hdc != 0, "GetDC(0) failed\n");
    hbm = CreateCompatibleBitmap(hdc, 10, 10);
    ok(hbm != 0, "CreateCompatibleBitmap failed\n");

    hbm_old = SelectObject(hdc, hbm);
    ok(hbm_old == 0, "SelectObject should fail\n");

    DeleteObject(hbm);
    ReleaseDC(0, hdc);

    hdc = CreateCompatibleDC(0);
    ok(hdc != 0, "GetDC(0) failed\n");
    hbm = CreateCompatibleBitmap(hdc, 10, 10);
    ok(hbm != 0, "CreateCompatibleBitmap failed\n");

    hbm_old = SelectObject(hdc, hbm);
    ok(hbm_old != 0, "SelectObject failed\n");
    hbm_old = SelectObject(hdc, hbm_old);
    ok(hbm_old == hbm, "SelectObject failed\n");

    DeleteObject(hbm);

    /* test an 1-bpp bitmap */
    planes = GetDeviceCaps(hdc, PLANES);
    bpp = 1;

    hbm = CreateBitmap(10, 10, planes, bpp, NULL);
    ok(hbm != 0, "CreateBitmap failed\n");

    hbm_old = SelectObject(hdc, hbm);
    ok(hbm_old != 0, "SelectObject failed\n");
    hbm_old = SelectObject(hdc, hbm_old);
    ok(hbm_old == hbm, "SelectObject failed\n");

    DeleteObject(hbm);

    for(i = 0; i < sizeof(depths)/sizeof(depths[0]); i++) {
        /* test a color bitmap to dc bpp matching */
        planes = GetDeviceCaps(hdc, PLANES);
        bpp = GetDeviceCaps(hdc, BITSPIXEL);

        hbm = CreateBitmap(10, 10, planes, depths[i], NULL);
        ok(hbm != 0, "CreateBitmap failed\n");

        hbm_old = SelectObject(hdc, hbm);
        if(depths[i] == bpp ||
          (bpp == 16 && depths[i] == 15)        /* 16 and 15 bpp are compatible */
          ) {
            ok(hbm_old != 0, "SelectObject failed, BITSPIXEL: %d, created depth: %d\n", bpp, depths[i]);
            SelectObject(hdc, hbm_old);
        } else {
            ok(hbm_old == 0, "SelectObject should fail. BITSPIXELS: %d, created depth: %d\n", bpp, depths[i]);
        }

        memset(&bm, 0xAA, sizeof(bm));
        bytes = GetObjectW(hbm, sizeof(bm), &bm);
        ok(bytes == sizeof(bm), "GetObject returned %d\n", bytes);
        ok(bm.bmType == 0, "wrong bmType %d\n", bm.bmType);
        ok(bm.bmWidth == 10, "wrong bmWidth %d\n", bm.bmWidth);
        ok(bm.bmHeight == 10, "wrong bmHeight %d\n", bm.bmHeight);
        ok(bm.bmWidthBytes == get_bitmap_stride(bm.bmWidth, bm.bmBitsPixel), "wrong bmWidthBytes %d\n", bm.bmWidthBytes);
        ok(bm.bmPlanes == planes, "wrong bmPlanes %u\n", bm.bmPlanes);
        if(depths[i] == 15) {
            ok(bm.bmBitsPixel == 16, "wrong bmBitsPixel %d(15 bpp special)\n", bm.bmBitsPixel);
        } else {
            ok(bm.bmBitsPixel == depths[i], "wrong bmBitsPixel %d\n", bm.bmBitsPixel);
        }
        ok(!bm.bmBits, "wrong bmBits %p\n", bm.bmBits);

        DeleteObject(hbm);
    }

    DeleteDC(hdc);
}

static void test_mono_1x1_bmp_dbg(HBITMAP hbmp, int line)
{
    INT ret;
    BITMAP bm;

    ret = GetObjectType(hbmp);
    ok_(__FILE__, line)(ret == OBJ_BITMAP, "the object %p is not bitmap\n", hbmp);

    ret = GetObjectW(hbmp, 0, 0);
    ok_(__FILE__, line)(ret == sizeof(BITMAP), "object size %d\n", ret);

    memset(&bm, 0xDA, sizeof(bm));
    SetLastError(0xdeadbeef);
    ret = GetObjectW(hbmp, sizeof(bm), &bm);
    if (!ret) /* XP, only for curObj2 */ return;
    ok_(__FILE__, line)(ret == sizeof(BITMAP), "GetObject returned %d, error %u\n", ret, GetLastError());
    ok_(__FILE__, line)(bm.bmType == 0, "wrong bmType, expected 0 got %d\n", bm.bmType);
    ok_(__FILE__, line)(bm.bmWidth == 1, "wrong bmWidth, expected 1 got %d\n", bm.bmWidth);
    ok_(__FILE__, line)(bm.bmHeight == 1, "wrong bmHeight, expected 1 got %d\n", bm.bmHeight);
    ok_(__FILE__, line)(bm.bmWidthBytes == 2, "wrong bmWidthBytes, expected 2 got %d\n", bm.bmWidthBytes);
    ok_(__FILE__, line)(bm.bmPlanes == 1, "wrong bmPlanes, expected 1 got %u\n", bm.bmPlanes);
    ok_(__FILE__, line)(bm.bmBitsPixel == 1, "wrong bmBitsPixel, expected 1 got %d\n", bm.bmBitsPixel);
    ok_(__FILE__, line)(!bm.bmBits, "wrong bmBits %p\n", bm.bmBits);
}

#define test_mono_1x1_bmp(a) test_mono_1x1_bmp_dbg((a), __LINE__)

static void test_CreateBitmap(void)
{
    BITMAP bmp;
    HDC screenDC = GetDC(0);
    HDC hdc = CreateCompatibleDC(screenDC);
    UINT i, expect = 0;

    /* all of these are the stock monochrome bitmap */
    HBITMAP bm = CreateCompatibleBitmap(hdc, 0, 0);
    HBITMAP bm1 = CreateCompatibleBitmap(screenDC, 0, 0);
    HBITMAP bm4 = CreateBitmap(0, 1, 0, 0, 0);
    HBITMAP bm5 = CreateDiscardableBitmap(hdc, 0, 0);
    HBITMAP curObj1 = GetCurrentObject(hdc, OBJ_BITMAP);
    HBITMAP curObj2 = GetCurrentObject(screenDC, OBJ_BITMAP);

    /* these 2 are not the stock monochrome bitmap */
    HBITMAP bm2 = CreateCompatibleBitmap(hdc, 1, 1);
    HBITMAP bm3 = CreateBitmap(1, 1, 1, 1, 0);

    HBITMAP old1 = SelectObject(hdc, bm2);
    HBITMAP old2 = SelectObject(screenDC, bm3);
    SelectObject(hdc, old1);
    SelectObject(screenDC, old2);

    ok(bm == bm1 && bm == bm4 && bm == bm5 && bm == curObj1 && bm == old1,
       "0: %p, 1: %p, 4: %p, 5: %p, curObj1 %p, old1 %p\n",
       bm, bm1, bm4, bm5, curObj1, old1);
    ok(bm != bm2 && bm != bm3, "0: %p, 2: %p, 3: %p\n", bm, bm2, bm3);
todo_wine
    ok(bm != curObj2, "0: %p, curObj2 %p\n", bm, curObj2);
    ok(old2 == 0, "old2 %p\n", old2);

    test_mono_1x1_bmp(bm);
    test_mono_1x1_bmp(bm1);
    test_mono_1x1_bmp(bm2);
    test_mono_1x1_bmp(bm3);
    test_mono_1x1_bmp(bm4);
    test_mono_1x1_bmp(bm5);
    test_mono_1x1_bmp(old1);
    test_mono_1x1_bmp(curObj1);

    DeleteObject(bm);
    DeleteObject(bm1);
    DeleteObject(bm2);
    DeleteObject(bm3);
    DeleteObject(bm4);
    DeleteObject(bm5);

    DeleteDC(hdc);
    ReleaseDC(0, screenDC);

    /* show that Windows ignores the provided bm.bmWidthBytes */
    bmp.bmType = 0;
    bmp.bmWidth = 1;
    bmp.bmHeight = 1;
    bmp.bmWidthBytes = 28;
    bmp.bmPlanes = 1;
    bmp.bmBitsPixel = 1;
    bmp.bmBits = NULL;
    bm = CreateBitmapIndirect(&bmp);
    ok(bm != 0, "CreateBitmapIndirect error %u\n", GetLastError());
    test_mono_1x1_bmp(bm);
    DeleteObject(bm);

    /* Test how the bmBitsPixel field is treated */
    for(i = 1; i <= 33; i++) {
        bmp.bmType = 0;
        bmp.bmWidth = 1;
        bmp.bmHeight = 1;
        bmp.bmWidthBytes = 28;
        bmp.bmPlanes = 1;
        bmp.bmBitsPixel = i;
        bmp.bmBits = NULL;
        SetLastError(0xdeadbeef);
        bm = CreateBitmapIndirect(&bmp);
        if(i > 32) {
            DWORD error = GetLastError();
            ok(bm == 0, "CreateBitmapIndirect for %d bpp succeeded\n", i);
            ok(error == ERROR_INVALID_PARAMETER, "Got error %d, expected ERROR_INVALID_PARAMETER\n", error);
            DeleteObject(bm);
            continue;
        }
        ok(bm != 0, "CreateBitmapIndirect error %u\n", GetLastError());
        GetObjectW(bm, sizeof(bmp), &bmp);
        if(i == 1) {
            expect = 1;
        } else if(i <= 4) {
            expect = 4;
        } else if(i <= 8) {
            expect = 8;
        } else if(i <= 16) {
            expect = 16;
        } else if(i <= 24) {
            expect = 24;
        } else if(i <= 32) {
            expect = 32;
        }
        ok(bmp.bmBitsPixel == expect, "CreateBitmapIndirect for a %d bpp bitmap created a %d bpp bitmap, expected %d\n",
           i, bmp.bmBitsPixel, expect);
        DeleteObject(bm);
    }
}

static void test_bitmapinfoheadersize(void)
{
    HBITMAP hdib;
    BITMAPINFO bmi;
    BITMAPCOREINFO bci;
    HDC hdc = GetDC(0);

    memset(&bmi, 0, sizeof(BITMAPINFO));
    bmi.bmiHeader.biHeight = 100;
    bmi.bmiHeader.biWidth = 512;
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biPlanes = 1;

    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER) - 1;

    hdib = CreateDIBSection(hdc, &bmi, 0, NULL, NULL, 0);
    ok(hdib == NULL, "CreateDIBSection succeeded\n");

    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

    SetLastError(0xdeadbeef);
    hdib = CreateDIBSection(hdc, &bmi, 0, NULL, NULL, 0);
    ok(hdib != NULL, "CreateDIBSection error %d\n", GetLastError());
    DeleteObject(hdib);

    bmi.bmiHeader.biSize++;

    SetLastError(0xdeadbeef);
    hdib = CreateDIBSection(hdc, &bmi, 0, NULL, NULL, 0);
    ok(hdib != NULL ||
       broken(!hdib), /* Win98, WinMe */
       "CreateDIBSection error %d\n", GetLastError());
    DeleteObject(hdib);

    bmi.bmiHeader.biSize = sizeof(BITMAPINFO);

    SetLastError(0xdeadbeef);
    hdib = CreateDIBSection(hdc, &bmi, 0, NULL, NULL, 0);
    ok(hdib != NULL ||
       broken(!hdib), /* Win98, WinMe */
       "CreateDIBSection error %d\n", GetLastError());
    DeleteObject(hdib);

    bmi.bmiHeader.biSize++;

    SetLastError(0xdeadbeef);
    hdib = CreateDIBSection(hdc, &bmi, 0, NULL, NULL, 0);
    ok(hdib != NULL ||
       broken(!hdib), /* Win98, WinMe */
       "CreateDIBSection error %d\n", GetLastError());
    DeleteObject(hdib);

    bmi.bmiHeader.biSize = sizeof(BITMAPV4HEADER);

    SetLastError(0xdeadbeef);
    hdib = CreateDIBSection(hdc, &bmi, 0, NULL, NULL, 0);
    ok(hdib != NULL, "CreateDIBSection error %d\n", GetLastError());
    DeleteObject(hdib);

    bmi.bmiHeader.biSize = sizeof(BITMAPV5HEADER);

    SetLastError(0xdeadbeef);
    hdib = CreateDIBSection(hdc, &bmi, 0, NULL, NULL, 0);
    ok(hdib != NULL ||
       broken(!hdib), /* Win95 */
       "CreateDIBSection error %d\n", GetLastError());
    DeleteObject(hdib);

    memset(&bci, 0, sizeof(BITMAPCOREINFO));
    bci.bmciHeader.bcHeight = 100;
    bci.bmciHeader.bcWidth = 512;
    bci.bmciHeader.bcBitCount = 24;
    bci.bmciHeader.bcPlanes = 1;

    bci.bmciHeader.bcSize = sizeof(BITMAPCOREHEADER) - 1;

    hdib = CreateDIBSection(hdc, (BITMAPINFO *)&bci, 0, NULL, NULL, 0);
    ok(hdib == NULL, "CreateDIBSection succeeded\n");

    bci.bmciHeader.bcSize = sizeof(BITMAPCOREHEADER);

    SetLastError(0xdeadbeef);
    hdib = CreateDIBSection(hdc, (BITMAPINFO *)&bci, 0, NULL, NULL, 0);
    ok(hdib != NULL, "CreateDIBSection error %d\n", GetLastError());
    DeleteObject(hdib);

    bci.bmciHeader.bcSize++;

    hdib = CreateDIBSection(hdc, (BITMAPINFO *)&bci, 0, NULL, NULL, 0);
    ok(hdib == NULL, "CreateDIBSection succeeded\n");

    bci.bmciHeader.bcSize = sizeof(BITMAPCOREINFO);

    hdib = CreateDIBSection(hdc, (BITMAPINFO *)&bci, 0, NULL, NULL, 0);
    ok(hdib == NULL, "CreateDIBSection succeeded\n");

    ReleaseDC(0, hdc);
}

static void test_get16dibits(void)
{
    BYTE bits[4 * (16 / sizeof(BYTE))];
    HBITMAP hbmp;
    HDC screen_dc = GetDC(NULL);
    int ret;
    BITMAPINFO * info;
    int info_len = sizeof(BITMAPINFOHEADER) + 1024;
    BYTE *p;
    int overwritten_bytes = 0;

    memset(bits, 0, sizeof(bits));
    hbmp = CreateBitmap(2, 2, 1, 16, bits);
    ok(hbmp != NULL, "CreateBitmap failed\n");

    info  = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, info_len);
    assert(info);

    memset(info, '!', info_len);
    memset(info, 0, sizeof(info->bmiHeader));

    info->bmiHeader.biSize = sizeof(info->bmiHeader);
    info->bmiHeader.biWidth = 2;
    info->bmiHeader.biHeight = 2;
    info->bmiHeader.biPlanes = 1;
    info->bmiHeader.biCompression = BI_RGB;

    ret = GetDIBits(screen_dc, hbmp, 0, 0, NULL, info, 0);
    ok(ret != 0, "GetDIBits failed got %d\n", ret);

    for (p = ((BYTE *) info) + sizeof(info->bmiHeader); (p - ((BYTE *) info)) < info_len; p++)
        if (*p != '!')
            overwritten_bytes++;
    ok(overwritten_bytes == 0, "GetDIBits wrote past the buffer given\n");

    HeapFree(GetProcessHeap(), 0, info);
    DeleteObject(hbmp);
    ReleaseDC(NULL, screen_dc);
}

static void check_BitBlt_pixel(HDC hdcDst, HDC hdcSrc, UINT32 *dstBuffer, UINT32 *srcBuffer,
                               DWORD dwRop, UINT32 expected, int line)
{
    *srcBuffer = 0xFEDCBA98;
    *dstBuffer = 0x89ABCDEF;
    BitBlt(hdcDst, 0, 0, 1, 1, hdcSrc, 0, 0, dwRop);
    ok(expected == *dstBuffer,
        "BitBlt with dwRop %06X. Expected 0x%08X, got 0x%08X from line %d\n",
        dwRop, expected, *dstBuffer, line);
}

static void test_BitBlt(void)
{
    HBITMAP bmpDst, bmpSrc;
    HBITMAP oldDst, oldSrc;
    HDC hdcScreen, hdcDst, hdcSrc;
    UINT32 *dstBuffer, *srcBuffer;
    HBRUSH hBrush, hOldBrush;
    BITMAPINFO bitmapInfo;

    memset(&bitmapInfo, 0, sizeof(BITMAPINFO));
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = 1;
    bitmapInfo.bmiHeader.biHeight = 1;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;
    bitmapInfo.bmiHeader.biSizeImage = sizeof(UINT32);

    hdcScreen = CreateCompatibleDC(0);
    hdcDst = CreateCompatibleDC(hdcScreen);
    hdcSrc = CreateCompatibleDC(hdcDst);

    /* Setup the destination dib section */
    bmpDst = CreateDIBSection(hdcScreen, &bitmapInfo, DIB_RGB_COLORS, (void**)&dstBuffer,
        NULL, 0);
    oldDst = SelectObject(hdcDst, bmpDst);

    hBrush = CreateSolidBrush(0x12345678);
    hOldBrush = SelectObject(hdcDst, hBrush);

    /* Setup the source dib section */
    bmpSrc = CreateDIBSection(hdcScreen, &bitmapInfo, DIB_RGB_COLORS, (void**)&srcBuffer,
        NULL, 0);
    oldSrc = SelectObject(hdcSrc, bmpSrc);

    check_BitBlt_pixel(hdcDst, hdcSrc, dstBuffer, srcBuffer, SRCCOPY, 0xFEDCBA98, __LINE__);
    check_BitBlt_pixel(hdcDst, hdcSrc, dstBuffer, srcBuffer, SRCPAINT, 0xFFFFFFFF, __LINE__);
    check_BitBlt_pixel(hdcDst, hdcSrc, dstBuffer, srcBuffer, SRCAND, 0x88888888, __LINE__);
    check_BitBlt_pixel(hdcDst, hdcSrc, dstBuffer, srcBuffer, SRCINVERT, 0x77777777, __LINE__);
    check_BitBlt_pixel(hdcDst, hdcSrc, dstBuffer, srcBuffer, SRCERASE, 0x76543210, __LINE__);
    check_BitBlt_pixel(hdcDst, hdcSrc, dstBuffer, srcBuffer, NOTSRCCOPY, 0x01234567, __LINE__);
    check_BitBlt_pixel(hdcDst, hdcSrc, dstBuffer, srcBuffer, NOTSRCERASE, 0x00000000, __LINE__);
    check_BitBlt_pixel(hdcDst, hdcSrc, dstBuffer, srcBuffer, MERGECOPY, 0x00581210, __LINE__);
    check_BitBlt_pixel(hdcDst, hdcSrc, dstBuffer, srcBuffer, MERGEPAINT, 0x89ABCDEF, __LINE__);
    check_BitBlt_pixel(hdcDst, hdcSrc, dstBuffer, srcBuffer, PATCOPY, 0x00785634, __LINE__);
    check_BitBlt_pixel(hdcDst, hdcSrc, dstBuffer, srcBuffer, PATPAINT, 0x89FBDFFF, __LINE__);
    check_BitBlt_pixel(hdcDst, hdcSrc, dstBuffer, srcBuffer, PATINVERT, 0x89D39BDB, __LINE__);
    check_BitBlt_pixel(hdcDst, hdcSrc, dstBuffer, srcBuffer, DSTINVERT, 0x76543210, __LINE__);
    check_BitBlt_pixel(hdcDst, hdcSrc, dstBuffer, srcBuffer, BLACKNESS, 0x00000000, __LINE__);
    check_BitBlt_pixel(hdcDst, hdcSrc, dstBuffer, srcBuffer, WHITENESS, 0xFFFFFFFF, __LINE__);

    /* Tidy up */
    SelectObject(hdcSrc, oldSrc);
    DeleteObject(bmpSrc);
    DeleteDC(hdcSrc);

    SelectObject(hdcDst, hOldBrush);
    DeleteObject(hBrush);
    SelectObject(hdcDst, oldDst);
    DeleteObject(bmpDst);
    DeleteDC(hdcDst);


    DeleteDC(hdcScreen);
}

static void check_StretchBlt_pixel(HDC hdcDst, HDC hdcSrc, UINT32 *dstBuffer, UINT32 *srcBuffer,
                                   DWORD dwRop, UINT32 expected, int line)
{
    *srcBuffer = 0xFEDCBA98;
    *dstBuffer = 0x89ABCDEF;
    StretchBlt(hdcDst, 0, 0, 2, 1, hdcSrc, 0, 0, 1, 1, dwRop);
    ok(expected == *dstBuffer,
        "StretchBlt with dwRop %06X. Expected 0x%08X, got 0x%08X from line %d\n",
        dwRop, expected, *dstBuffer, line);
}

static void check_StretchBlt_stretch(HDC hdcDst, HDC hdcSrc, BITMAPINFO *dst_info, UINT32 *dstBuffer, UINT32 *srcBuffer,
                                     int nXOriginDest, int nYOriginDest, int nWidthDest, int nHeightDest,
                                     int nXOriginSrc, int nYOriginSrc, int nWidthSrc, int nHeightSrc,
                                     UINT32 *expected, int line)
{
    int dst_size = get_dib_image_size( dst_info );

    memset(dstBuffer, 0, dst_size);
    StretchBlt(hdcDst, nXOriginDest, nYOriginDest, nWidthDest, nHeightDest,
               hdcSrc, nXOriginSrc, nYOriginSrc, nWidthSrc, nHeightSrc, SRCCOPY);
    ok(memcmp(dstBuffer, expected, dst_size) == 0,
        "StretchBlt expected { %08X, %08X, %08X, %08X } got { %08X, %08X, %08X, %08X } "
        "stretching { %d, %d, %d, %d } to { %d, %d, %d, %d } from line %d\n",
        expected[0], expected[1], expected[2], expected[3],
        dstBuffer[0], dstBuffer[1], dstBuffer[2], dstBuffer[3],
        nXOriginSrc, nYOriginSrc, nWidthSrc, nHeightSrc,
        nXOriginDest, nYOriginDest, nWidthDest, nHeightDest, line);
}

static void test_StretchBlt(void)
{
    HBITMAP bmpDst, bmpSrc;
    HBITMAP oldDst, oldSrc;
    HDC hdcScreen, hdcDst, hdcSrc;
    UINT32 *dstBuffer, *srcBuffer;
    HBRUSH hBrush, hOldBrush;
    BITMAPINFO biDst, biSrc;
    UINT32 expected[256];
    RGBQUAD colors[2];

    memset(&biDst, 0, sizeof(BITMAPINFO));
    biDst.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    biDst.bmiHeader.biWidth = 16;
    biDst.bmiHeader.biHeight = -16;
    biDst.bmiHeader.biPlanes = 1;
    biDst.bmiHeader.biBitCount = 32;
    biDst.bmiHeader.biCompression = BI_RGB;
    memcpy(&biSrc, &biDst, sizeof(BITMAPINFO));

    hdcScreen = CreateCompatibleDC(0);
    hdcDst = CreateCompatibleDC(hdcScreen);
    hdcSrc = CreateCompatibleDC(hdcDst);

    /* Pixel Tests */
    bmpDst = CreateDIBSection(hdcScreen, &biDst, DIB_RGB_COLORS, (void**)&dstBuffer,
        NULL, 0);
    oldDst = SelectObject(hdcDst, bmpDst);

    bmpSrc = CreateDIBSection(hdcScreen, &biSrc, DIB_RGB_COLORS, (void**)&srcBuffer,
        NULL, 0);
    oldSrc = SelectObject(hdcSrc, bmpSrc);

    hBrush = CreateSolidBrush(0x012345678);
    hOldBrush = SelectObject(hdcDst, hBrush);

    check_StretchBlt_pixel(hdcDst, hdcSrc, dstBuffer, srcBuffer, SRCCOPY, 0xFEDCBA98, __LINE__);
    check_StretchBlt_pixel(hdcDst, hdcSrc, dstBuffer, srcBuffer, SRCPAINT, 0xFFFFFFFF, __LINE__);
    check_StretchBlt_pixel(hdcDst, hdcSrc, dstBuffer, srcBuffer, SRCAND, 0x88888888, __LINE__);
    check_StretchBlt_pixel(hdcDst, hdcSrc, dstBuffer, srcBuffer, SRCINVERT, 0x77777777, __LINE__);
    check_StretchBlt_pixel(hdcDst, hdcSrc, dstBuffer, srcBuffer, SRCERASE, 0x76543210, __LINE__);
    check_StretchBlt_pixel(hdcDst, hdcSrc, dstBuffer, srcBuffer, NOTSRCCOPY, 0x01234567, __LINE__);
    check_StretchBlt_pixel(hdcDst, hdcSrc, dstBuffer, srcBuffer, NOTSRCERASE, 0x00000000, __LINE__);
    check_StretchBlt_pixel(hdcDst, hdcSrc, dstBuffer, srcBuffer, MERGECOPY, 0x00581210, __LINE__);
    check_StretchBlt_pixel(hdcDst, hdcSrc, dstBuffer, srcBuffer, MERGEPAINT, 0x89ABCDEF, __LINE__);
    check_StretchBlt_pixel(hdcDst, hdcSrc, dstBuffer, srcBuffer, PATCOPY, 0x00785634, __LINE__);
    check_StretchBlt_pixel(hdcDst, hdcSrc, dstBuffer, srcBuffer, PATPAINT, 0x89FBDFFF, __LINE__);
    check_StretchBlt_pixel(hdcDst, hdcSrc, dstBuffer, srcBuffer, PATINVERT, 0x89D39BDB, __LINE__);
    check_StretchBlt_pixel(hdcDst, hdcSrc, dstBuffer, srcBuffer, DSTINVERT, 0x76543210, __LINE__);
    check_StretchBlt_pixel(hdcDst, hdcSrc, dstBuffer, srcBuffer, BLACKNESS, 0x00000000, __LINE__);
    check_StretchBlt_pixel(hdcDst, hdcSrc, dstBuffer, srcBuffer, WHITENESS, 0xFFFFFFFF, __LINE__);

    SelectObject(hdcDst, hOldBrush);
    DeleteObject(hBrush);

    /* Top-down to top-down tests */
    srcBuffer[0] = 0xCAFED00D, srcBuffer[1] = 0xFEEDFACE;
    srcBuffer[16] = 0xFEDCBA98, srcBuffer[17] = 0x76543210;

    memset( expected, 0, get_dib_image_size( &biDst ) );
    expected[0] = 0xCAFED00D, expected[1] = 0xFEEDFACE;
    expected[16] = 0xFEDCBA98, expected[17] = 0x76543210;
    check_StretchBlt_stretch(hdcDst, hdcSrc, &biDst, dstBuffer, srcBuffer,
                             0, 0, 2, 2, 0, 0, 2, 2, expected, __LINE__);

    expected[0] = 0xCAFED00D, expected[1] = 0x00000000;
    expected[16] = 0x00000000, expected[17] = 0x00000000;
    check_StretchBlt_stretch(hdcDst, hdcSrc, &biDst, dstBuffer, srcBuffer,
                             0, 0, 1, 1, 0, 0, 1, 1, expected, __LINE__);

    expected[0] = 0xCAFED00D, expected[1] = 0xCAFED00D;
    expected[16] = 0xCAFED00D, expected[17] = 0xCAFED00D;
    check_StretchBlt_stretch(hdcDst, hdcSrc, &biDst, dstBuffer, srcBuffer,
                             0, 0, 2, 2, 0, 0, 1, 1, expected, __LINE__);

    /* This is an example of the dst width (height) == 1 exception, explored below */
    expected[0] = 0xCAFED00D, expected[1] = 0x00000000;
    expected[16] = 0x00000000, expected[17] = 0x00000000;
    check_StretchBlt_stretch(hdcDst, hdcSrc, &biDst, dstBuffer, srcBuffer,
                             0, 0, 1, 1, 0, 0, 2, 2, expected, __LINE__);

    expected[0] = 0x76543210, expected[1] = 0xFEDCBA98;
    expected[16] = 0xFEEDFACE, expected[17] = 0xCAFED00D;
    check_StretchBlt_stretch(hdcDst, hdcSrc, &biDst, dstBuffer, srcBuffer,
                             0, 0, 2, 2, 1, 1, -2, -2, expected, __LINE__);

    expected[0] = 0x76543210, expected[1] = 0xFEDCBA98;
    expected[16] = 0xFEEDFACE, expected[17] = 0xCAFED00D;
    check_StretchBlt_stretch(hdcDst, hdcSrc, &biDst, dstBuffer, srcBuffer,
                             1, 1, -2, -2, 0, 0, 2, 2, expected, __LINE__);

    expected[0] = 0xCAFED00D, expected[1] = 0x00000000;
    expected[16] = 0x00000000, expected[17] = 0x00000000;
    todo_wine check_StretchBlt_stretch(hdcDst, hdcSrc, &biDst, dstBuffer, srcBuffer,
                                       1, 1, -2, -2, 1, 1, -2, -2, expected, __LINE__);

    expected[0] = 0x00000000, expected[1] = 0x00000000;
    expected[16] = 0x00000000, expected[17] = 0xCAFED00D, expected[18] = 0xFEEDFACE;
    expected[32] = 0x00000000, expected[33] = 0xFEDCBA98, expected[34] = 0x76543210;

    check_StretchBlt_stretch(hdcDst, hdcSrc, &biDst, dstBuffer, srcBuffer,
                             1, 1, 2, 2, 0, 0, 2, 2, expected, __LINE__);

    /* when dst width is 1 merge src width - 1 pixels */
    memset( srcBuffer, 0, get_dib_image_size( &biSrc ) );
    srcBuffer[0] = 0x0000ff00, srcBuffer[1] = 0x0000f0f0, srcBuffer[2] = 0x0000cccc, srcBuffer[3] = 0x0000aaaa;
    srcBuffer[16] = 0xFEDCBA98, srcBuffer[17] = 0x76543210;

    memset( expected, 0, get_dib_image_size( &biDst ) );
    expected[0] = srcBuffer[0];
    check_StretchBlt_stretch(hdcDst, hdcSrc, &biDst, dstBuffer, srcBuffer,
                             0, 0, 1, 1, 0, 0, 2, 1, expected, __LINE__);

    expected[0] = srcBuffer[0] & srcBuffer[1];
    check_StretchBlt_stretch(hdcDst, hdcSrc, &biDst, dstBuffer, srcBuffer,
                             0, 0, 1, 1, 0, 0, 3, 1, expected, __LINE__);

    expected[0] = srcBuffer[0] & srcBuffer[1] & srcBuffer[2];
    check_StretchBlt_stretch(hdcDst, hdcSrc, &biDst, dstBuffer, srcBuffer,
                             0, 0, 1, 1, 0, 0, 4, 1, expected, __LINE__);

    /* this doesn't happen if the src width is -ve */
    expected[0] = srcBuffer[1] & srcBuffer[2];
    check_StretchBlt_stretch(hdcDst, hdcSrc, &biDst, dstBuffer, srcBuffer,
                             0, 0, 1, 1, 2, 0, -2, 1, expected, __LINE__);

    /* when dst width > 1 behaviour reverts to what one would expect */
    expected[0] = srcBuffer[0] & srcBuffer[1], expected[1] = srcBuffer[2] & srcBuffer[3];
    check_StretchBlt_stretch(hdcDst, hdcSrc, &biDst, dstBuffer, srcBuffer,
                             0, 0, 2, 1, 0, 0, 4, 1, expected, __LINE__);

    /* similarly in the vertical direction */
    memset( expected, 0, get_dib_image_size( &biDst ) );
    expected[0] = srcBuffer[0];
    check_StretchBlt_stretch(hdcDst, hdcSrc, &biDst, dstBuffer, srcBuffer,
                             0, 0, 1, 1, 0, 0, 1, 2, expected, __LINE__);

    /* check that it's the dst size in device units that needs to be 1 */
    SetMapMode( hdcDst, MM_ISOTROPIC );
    SetWindowExtEx( hdcDst, 200, 200, NULL );
    SetViewportExtEx( hdcDst, 100, 100, NULL );

    expected[0] = srcBuffer[0] & srcBuffer[1] & srcBuffer[2];
    check_StretchBlt_stretch(hdcDst, hdcSrc, &biDst, dstBuffer, srcBuffer,
                             0, 0, 2, 2, 0, 0, 4, 1, expected, __LINE__);
    SetMapMode( hdcDst, MM_TEXT );

    SelectObject(hdcDst, oldDst);
    DeleteObject(bmpDst);

    /* Top-down to bottom-up tests */
    memset( srcBuffer, 0, get_dib_image_size( &biSrc ) );
    srcBuffer[0] = 0xCAFED00D, srcBuffer[1] = 0xFEEDFACE;
    srcBuffer[16] = 0xFEDCBA98, srcBuffer[17] = 0x76543210;

    biDst.bmiHeader.biHeight = 16;
    bmpDst = CreateDIBSection(hdcScreen, &biDst, DIB_RGB_COLORS, (void**)&dstBuffer,
        NULL, 0);
    oldDst = SelectObject(hdcDst, bmpDst);

    memset( expected, 0, get_dib_image_size( &biDst ) );

    expected[224] = 0xFEDCBA98, expected[225] = 0x76543210;
    expected[240] = 0xCAFED00D, expected[241] = 0xFEEDFACE;
    check_StretchBlt_stretch(hdcDst, hdcSrc, &biDst, dstBuffer, srcBuffer,
                             0, 0, 2, 2, 0, 0, 2, 2, expected, __LINE__);

    expected[224] = 0xFEEDFACE, expected[225] = 0xCAFED00D;
    expected[240] = 0x76543210, expected[241] = 0xFEDCBA98;
    check_StretchBlt_stretch(hdcDst, hdcSrc, &biDst, dstBuffer, srcBuffer,
                             0, 0, 2, 2, 1, 1, -2, -2, expected, __LINE__);

    SelectObject(hdcSrc, oldSrc);
    DeleteObject(bmpSrc);

    /* Bottom-up to bottom-up tests */
    biSrc.bmiHeader.biHeight = 16;
    bmpSrc = CreateDIBSection(hdcScreen, &biSrc, DIB_RGB_COLORS, (void**)&srcBuffer,
        NULL, 0);
    srcBuffer[224] = 0xCAFED00D, srcBuffer[225] = 0xFEEDFACE;
    srcBuffer[240] = 0xFEDCBA98, srcBuffer[241] = 0x76543210;
    oldSrc = SelectObject(hdcSrc, bmpSrc);

    memset( expected, 0, get_dib_image_size( &biDst ) );

    expected[224] = 0xCAFED00D, expected[225] = 0xFEEDFACE;
    expected[240] = 0xFEDCBA98, expected[241] = 0x76543210;
    check_StretchBlt_stretch(hdcDst, hdcSrc, &biDst, dstBuffer, srcBuffer,
                             0, 0, 2, 2, 0, 0, 2, 2, expected, __LINE__);

    expected[224] = 0x76543210, expected[225] = 0xFEDCBA98;
    expected[240] = 0xFEEDFACE, expected[241] = 0xCAFED00D;
    check_StretchBlt_stretch(hdcDst, hdcSrc, &biDst, dstBuffer, srcBuffer,
                             0, 0, 2, 2, 1, 1, -2, -2, expected, __LINE__);

    SelectObject(hdcDst, oldDst);
    DeleteObject(bmpDst);

    /* Bottom-up to top-down tests */
    biDst.bmiHeader.biHeight = -16;
    bmpDst = CreateDIBSection(hdcScreen, &biDst, DIB_RGB_COLORS, (void**)&dstBuffer,
        NULL, 0);
    oldDst = SelectObject(hdcDst, bmpDst);

    memset( expected, 0, get_dib_image_size( &biDst ) );
    expected[0] = 0xFEDCBA98, expected[1] = 0x76543210;
    expected[16] = 0xCAFED00D, expected[17] = 0xFEEDFACE;
    check_StretchBlt_stretch(hdcDst, hdcSrc, &biDst, dstBuffer, srcBuffer,
                             0, 0, 2, 2, 0, 0, 2, 2, expected, __LINE__);

    expected[0] = 0xFEEDFACE, expected[1] = 0xCAFED00D;
    expected[16] = 0x76543210, expected[17] = 0xFEDCBA98;
    check_StretchBlt_stretch(hdcDst, hdcSrc, &biDst, dstBuffer, srcBuffer,
                             0, 0, 2, 2, 1, 1, -2, -2, expected, __LINE__);

    SelectObject(hdcSrc, oldSrc);
    DeleteObject(bmpSrc);

    biSrc.bmiHeader.biHeight = -2;
    biSrc.bmiHeader.biBitCount = 24;
    bmpSrc = CreateDIBSection(hdcScreen, &biSrc, DIB_RGB_COLORS, (void**)&srcBuffer, NULL, 0);
    oldSrc = SelectObject(hdcSrc, bmpSrc);

    memset( expected, 0, get_dib_image_size( &biDst ) );
    expected[0] = 0xFEEDFACE, expected[1] = 0xCAFED00D;
    expected[2] = 0x76543210, expected[3] = 0xFEDCBA98;
    memcpy(dstBuffer, expected, 4 * sizeof(*dstBuffer));
    StretchBlt(hdcSrc, 0, 0, 4, 1, hdcDst, 0, 0, 4, 1, SRCCOPY );
    memset(dstBuffer, 0x55, 4 * sizeof(*dstBuffer));
    StretchBlt(hdcDst, 0, 0, 4, 1, hdcSrc, 0, 0, 4, 1, SRCCOPY );
    expected[0] = 0x00EDFACE, expected[1] = 0x00FED00D;
    expected[2] = 0x00543210, expected[3] = 0x00DCBA98;
    ok(!memcmp(dstBuffer, expected, 16),
       "StretchBlt expected { %08X, %08X, %08X, %08X } got { %08X, %08X, %08X, %08X }\n",
        expected[0], expected[1], expected[2], expected[3],
        dstBuffer[0], dstBuffer[1], dstBuffer[2], dstBuffer[3] );

    expected[0] = 0xFEEDFACE, expected[1] = 0xCAFED00D;
    expected[2] = 0x76543210, expected[3] = 0xFEDCBA98;
    memcpy(srcBuffer, expected, 4 * sizeof(*dstBuffer));
    memset(dstBuffer, 0x55, 4 * sizeof(*dstBuffer));
    StretchBlt(hdcDst, 0, 0, 4, 1, hdcSrc, 0, 0, 4, 1, SRCCOPY );
    expected[0] = 0x00EDFACE, expected[1] = 0x00D00DFE;
    expected[2] = 0x0010CAFE, expected[3] = 0x00765432;
    ok(!memcmp(dstBuffer, expected, 16),
       "StretchBlt expected { %08X, %08X, %08X, %08X } got { %08X, %08X, %08X, %08X }\n",
        expected[0], expected[1], expected[2], expected[3],
        dstBuffer[0], dstBuffer[1], dstBuffer[2], dstBuffer[3] );

    SelectObject(hdcSrc, oldSrc);
    DeleteObject(bmpSrc);

    biSrc.bmiHeader.biBitCount = 1;
    bmpSrc = CreateDIBSection(hdcScreen, &biSrc, DIB_RGB_COLORS, (void**)&srcBuffer, NULL, 0);
    oldSrc = SelectObject(hdcSrc, bmpSrc);
    *((DWORD *)colors + 0) = 0x123456;
    *((DWORD *)colors + 1) = 0x335577;
    SetDIBColorTable( hdcSrc, 0, 2, colors );
    srcBuffer[0] = 0x55555555;
    memset(dstBuffer, 0xcc, 4 * sizeof(*dstBuffer));
    SetTextColor( hdcDst, 0 );
    SetBkColor( hdcDst, 0 );
    StretchBlt(hdcDst, 0, 0, 4, 1, hdcSrc, 0, 0, 4, 1, SRCCOPY );
    expected[0] = expected[2] = 0x00123456;
    expected[1] = expected[3] = 0x00335577;
    ok(!memcmp(dstBuffer, expected, 16),
       "StretchBlt expected { %08X, %08X, %08X, %08X } got { %08X, %08X, %08X, %08X }\n",
        expected[0], expected[1], expected[2], expected[3],
        dstBuffer[0], dstBuffer[1], dstBuffer[2], dstBuffer[3] );

    SelectObject(hdcSrc, oldSrc);
    DeleteObject(bmpSrc);

    bmpSrc = CreateBitmap( 16, 16, 1, 1, 0 );
    oldSrc = SelectObject(hdcSrc, bmpSrc);
    SetPixel( hdcSrc, 0, 0, 0 );
    SetPixel( hdcSrc, 1, 0, 0xffffff );
    SetPixel( hdcSrc, 2, 0, 0xffffff );
    SetPixel( hdcSrc, 3, 0, 0 );
    memset(dstBuffer, 0xcc, 4 * sizeof(*dstBuffer));
    SetTextColor( hdcDst, RGB(0x22,0x44,0x66) );
    SetBkColor( hdcDst, RGB(0x65,0x43,0x21) );
    StretchBlt(hdcDst, 0, 0, 4, 1, hdcSrc, 0, 0, 4, 1, SRCCOPY );
    expected[0] = expected[3] = 0x00224466;
    expected[1] = expected[2] = 0x00654321;
    ok(!memcmp(dstBuffer, expected, 16),
       "StretchBlt expected { %08X, %08X, %08X, %08X } got { %08X, %08X, %08X, %08X }\n",
        expected[0], expected[1], expected[2], expected[3],
        dstBuffer[0], dstBuffer[1], dstBuffer[2], dstBuffer[3] );

    SelectObject(hdcSrc, oldSrc);
    DeleteObject(bmpSrc);

    DeleteDC(hdcSrc);

    SelectObject(hdcDst, oldDst);
    DeleteObject(bmpDst);
    DeleteDC(hdcDst);

    DeleteDC(hdcScreen);
}

static void check_StretchDIBits_pixel(HDC hdcDst, UINT32 *dstBuffer, UINT32 *srcBuffer,
                                      DWORD dwRop, UINT32 expected, int line)
{
    const UINT32 buffer[2] = { 0xFEDCBA98, 0 };
    BITMAPINFO bitmapInfo;

    memset(&bitmapInfo, 0, sizeof(BITMAPINFO));
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = 2;
    bitmapInfo.bmiHeader.biHeight = 1;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;
    bitmapInfo.bmiHeader.biSizeImage = sizeof(buffer);

    *dstBuffer = 0x89ABCDEF;

    StretchDIBits(hdcDst, 0, 0, 2, 1, 0, 0, 1, 1, &buffer, &bitmapInfo, DIB_RGB_COLORS, dwRop);
    ok(expected == *dstBuffer,
        "StretchDIBits with dwRop %06X. Expected 0x%08X, got 0x%08X from line %d\n",
        dwRop, expected, *dstBuffer, line);
}

static INT check_StretchDIBits_stretch( HDC hdcDst, UINT32 *dstBuffer, UINT32 *srcBuffer,
                                        int nXOriginDest, int nYOriginDest, int nWidthDest, int nHeightDest,
                                        int nXOriginSrc, int nYOriginSrc, int nWidthSrc, int nHeightSrc,
                                        UINT32 expected[4], int line)
{
    BITMAPINFO bitmapInfo;
    INT ret;

    memset(&bitmapInfo, 0, sizeof(BITMAPINFO));
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = 2;
    bitmapInfo.bmiHeader.biHeight = -2;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    memset(dstBuffer, 0, 16);
    ret = StretchDIBits(hdcDst, nXOriginDest, nYOriginDest, nWidthDest, nHeightDest,
                        nXOriginSrc, nYOriginSrc, nWidthSrc, nHeightSrc,
                        srcBuffer, &bitmapInfo, DIB_RGB_COLORS, SRCCOPY);
    ok(memcmp(dstBuffer, expected, 16) == 0,
        "StretchDIBits expected { %08X, %08X, %08X, %08X } got { %08X, %08X, %08X, %08X } "
        "stretching { %d, %d, %d, %d } to { %d, %d, %d, %d } from line %d\n",
        expected[0], expected[1], expected[2], expected[3],
        dstBuffer[0], dstBuffer[1], dstBuffer[2], dstBuffer[3],
        nXOriginSrc, nYOriginSrc, nWidthSrc, nHeightSrc,
        nXOriginDest, nYOriginDest, nWidthDest, nHeightDest, line);
    return ret;
}

static void test_StretchDIBits(void)
{
    HBITMAP bmpDst;
    HBITMAP oldDst;
    HDC hdcScreen, hdcDst;
    UINT32 *dstBuffer, srcBuffer[4];
    HBRUSH hBrush, hOldBrush;
    BITMAPINFO biDst;
    UINT32 expected[4];
    INT ret;

    memset(&biDst, 0, sizeof(BITMAPINFO));
    biDst.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    biDst.bmiHeader.biWidth = 2;
    biDst.bmiHeader.biHeight = -2;
    biDst.bmiHeader.biPlanes = 1;
    biDst.bmiHeader.biBitCount = 32;
    biDst.bmiHeader.biCompression = BI_RGB;

    hdcScreen = CreateCompatibleDC(0);
    hdcDst = CreateCompatibleDC(hdcScreen);

    /* Pixel Tests */
    bmpDst = CreateDIBSection(hdcScreen, &biDst, DIB_RGB_COLORS, (void**)&dstBuffer,
        NULL, 0);
    oldDst = SelectObject(hdcDst, bmpDst);

    hBrush = CreateSolidBrush(0x012345678);
    hOldBrush = SelectObject(hdcDst, hBrush);

    check_StretchDIBits_pixel(hdcDst, dstBuffer, srcBuffer, SRCCOPY, 0xFEDCBA98, __LINE__);
    check_StretchDIBits_pixel(hdcDst, dstBuffer, srcBuffer, SRCPAINT, 0xFFFFFFFF, __LINE__);
    check_StretchDIBits_pixel(hdcDst, dstBuffer, srcBuffer, SRCAND, 0x88888888, __LINE__);
    check_StretchDIBits_pixel(hdcDst, dstBuffer, srcBuffer, SRCINVERT, 0x77777777, __LINE__);
    check_StretchDIBits_pixel(hdcDst, dstBuffer, srcBuffer, SRCERASE, 0x76543210, __LINE__);
    check_StretchDIBits_pixel(hdcDst, dstBuffer, srcBuffer, NOTSRCCOPY, 0x01234567, __LINE__);
    check_StretchDIBits_pixel(hdcDst, dstBuffer, srcBuffer, NOTSRCERASE, 0x00000000, __LINE__);
    check_StretchDIBits_pixel(hdcDst, dstBuffer, srcBuffer, MERGECOPY, 0x00581210, __LINE__);
    check_StretchDIBits_pixel(hdcDst, dstBuffer, srcBuffer, MERGEPAINT, 0x89ABCDEF, __LINE__);
    check_StretchDIBits_pixel(hdcDst, dstBuffer, srcBuffer, PATCOPY, 0x00785634, __LINE__);
    check_StretchDIBits_pixel(hdcDst, dstBuffer, srcBuffer, PATPAINT, 0x89FBDFFF, __LINE__);
    check_StretchDIBits_pixel(hdcDst, dstBuffer, srcBuffer, PATINVERT, 0x89D39BDB, __LINE__);
    check_StretchDIBits_pixel(hdcDst, dstBuffer, srcBuffer, DSTINVERT, 0x76543210, __LINE__);
    check_StretchDIBits_pixel(hdcDst, dstBuffer, srcBuffer, BLACKNESS, 0x00000000, __LINE__);
    check_StretchDIBits_pixel(hdcDst, dstBuffer, srcBuffer, WHITENESS, 0xFFFFFFFF, __LINE__);

    SelectObject(hdcDst, hOldBrush);
    DeleteObject(hBrush);

    /* Top-down destination tests */
    srcBuffer[0] = 0xCAFED00D, srcBuffer[1] = 0xFEEDFACE;
    srcBuffer[2] = 0xFEDCBA98, srcBuffer[3] = 0x76543210;

    expected[0] = 0xCAFED00D, expected[1] = 0xFEEDFACE;
    expected[2] = 0xFEDCBA98, expected[3] = 0x76543210;
    ret = check_StretchDIBits_stretch(hdcDst, dstBuffer, srcBuffer,
                                      0, 0, 2, 2, 0, 0, 2, 2, expected, __LINE__);
    ok( ret == 2, "got ret %d\n", ret );

    expected[0] = 0xCAFED00D, expected[1] = 0x00000000;
    expected[2] = 0x00000000, expected[3] = 0x00000000;
    ret = check_StretchDIBits_stretch(hdcDst, dstBuffer, srcBuffer,
                                      0, 0, 1, 1, 0, 0, 1, 1, expected, __LINE__);
    todo_wine ok( ret == 1, "got ret %d\n", ret );

    expected[0] = 0xFEDCBA98, expected[1] = 0xFEDCBA98;
    expected[2] = 0xFEDCBA98, expected[3] = 0xFEDCBA98;
    ret = check_StretchDIBits_stretch(hdcDst, dstBuffer, srcBuffer,
                                      0, 0, 2, 2, 0, 0, 1, 1, expected, __LINE__);
    ok( ret == 2, "got ret %d\n", ret );

    expected[0] = 0x42441000, expected[1] = 0x00000000;
    expected[2] = 0x00000000, expected[3] = 0x00000000;
    ret = check_StretchDIBits_stretch(hdcDst, dstBuffer, srcBuffer,
                                      0, 0, 1, 1, 0, 0, 2, 2, expected, __LINE__);
    ok( ret == 2, "got ret %d\n", ret );

    expected[0] = 0x00000000, expected[1] = 0x00000000;
    expected[2] = 0x00000000, expected[3] = 0x00000000;
    ret = check_StretchDIBits_stretch(hdcDst, dstBuffer, srcBuffer,
                                      0, 0, 2, 2, 1, 1, -2, -2, expected, __LINE__);
    ok( ret == 0, "got ret %d\n", ret );

    expected[0] = 0x00000000, expected[1] = 0x00000000;
    expected[2] = 0x00000000, expected[3] = 0x00000000;
    ret = check_StretchDIBits_stretch(hdcDst, dstBuffer, srcBuffer,
                                      0, 0, 2, 2, 1, 1, -2, -2, expected, __LINE__);
    ok( ret == 0, "got ret %d\n", ret );

    expected[0] = 0x00000000, expected[1] = 0x00000000;
    expected[2] = 0x00000000, expected[3] = 0x00000000;
    ret = check_StretchDIBits_stretch(hdcDst, dstBuffer, srcBuffer,
                                      1, 1, -2, -2, 1, 1, -2, -2, expected, __LINE__);
    ok( ret == 0, "got ret %d\n", ret );

    expected[0] = 0x00000000, expected[1] = 0x00000000;
    expected[2] = 0x00000000, expected[3] = 0xCAFED00D;
    ret = check_StretchDIBits_stretch(hdcDst, dstBuffer, srcBuffer,
                                      1, 1, 2, 2, 0, 0, 2, 2, expected, __LINE__);
    ok( ret == 2, "got ret %d\n", ret );

    expected[0] = 0x00000000, expected[1] = 0x00000000;
    expected[2] = 0x00000000, expected[3] = 0x00000000;
    ret = check_StretchDIBits_stretch(hdcDst, dstBuffer, srcBuffer,
                                      2, 2, 4, 4, 0, 0, 2, 2, expected, __LINE__);
    ok( ret == 2, "got ret %d\n", ret );

    expected[0] = 0x00000000, expected[1] = 0x00000000;
    expected[2] = 0x00000000, expected[3] = 0x00000000;
    ret = check_StretchDIBits_stretch(hdcDst, dstBuffer, srcBuffer,
                                      -4, -4, 4, 4, 0, 0, 4, 4, expected, __LINE__);
    ok( ret == 2, "got ret %d\n", ret );

    SelectObject(hdcDst, oldDst);
    DeleteObject(bmpDst);

    /* Bottom up destination tests */
    biDst.bmiHeader.biHeight = 2;
    bmpDst = CreateDIBSection(hdcScreen, &biDst, DIB_RGB_COLORS, (void**)&dstBuffer,
        NULL, 0);
    oldDst = SelectObject(hdcDst, bmpDst);

    expected[0] = 0xFEDCBA98, expected[1] = 0x76543210;
    expected[2] = 0xCAFED00D, expected[3] = 0xFEEDFACE;
    check_StretchDIBits_stretch(hdcDst, dstBuffer, srcBuffer,
                                0, 0, 2, 2, 0, 0, 2, 2, expected, __LINE__);

    /* Tidy up */
    SelectObject(hdcDst, oldDst);
    DeleteObject(bmpDst);
    DeleteDC(hdcDst);

    DeleteDC(hdcScreen);
}

static void test_GdiAlphaBlend(void)
{
    HDC hdcNull;
    HDC hdcDst;
    HBITMAP bmpDst;
    BITMAPINFO *bmi;
    HDC hdcSrc;
    HBITMAP bmpSrc;
    HBITMAP oldSrc;
    LPVOID bits;
    BOOL ret;
    BLENDFUNCTION blend;

    if (!pGdiAlphaBlend)
    {
        win_skip("GdiAlphaBlend() is not implemented\n");
        return;
    }

    hdcNull = GetDC(NULL);
    hdcDst = CreateCompatibleDC(hdcNull);
    bmpDst = CreateCompatibleBitmap(hdcNull, 100, 100);
    hdcSrc = CreateCompatibleDC(hdcNull);

    bmi = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, FIELD_OFFSET( BITMAPINFO, bmiColors[3] ));
    bmi->bmiHeader.biSize = sizeof(bmi->bmiHeader);
    bmi->bmiHeader.biHeight = 20;
    bmi->bmiHeader.biWidth = 20;
    bmi->bmiHeader.biBitCount = 32;
    bmi->bmiHeader.biPlanes = 1;
    bmi->bmiHeader.biCompression = BI_RGB;
    bmpSrc = CreateDIBSection(hdcDst, bmi, DIB_RGB_COLORS, &bits, NULL, 0);
    ok(bmpSrc != NULL, "Couldn't create source bitmap\n");

    SelectObject(hdcDst, bmpDst);
    oldSrc = SelectObject(hdcSrc, bmpSrc);

    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.SourceConstantAlpha = 128;
    blend.AlphaFormat = 0;

    SetLastError(0xdeadbeef);
    ret = pGdiAlphaBlend(hdcDst, 0, 0, 20, 20, hdcSrc, 0, 0, 20, 20, blend);
    ok( ret, "GdiAlphaBlend failed err %u\n", GetLastError() );

    SetLastError(0xdeadbeef);
    ret = pGdiAlphaBlend(hdcDst, 0, 0, 20, 20, hdcSrc, -1, 0, 10, 10, blend);
    ok( !ret, "GdiAlphaBlend succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );

    ret = pGdiAlphaBlend(hdcDst, 0, 0, 20, 20, hdcSrc, 0, -1, 10, 10, blend);
    ok( !ret, "GdiAlphaBlend succeeded\n" );
    ret = pGdiAlphaBlend(hdcDst, 0, 0, 20, 20, hdcSrc, 15, 0, 10, 10, blend);
    ok( !ret, "GdiAlphaBlend succeeded\n" );
    ret = pGdiAlphaBlend(hdcDst, 0, 0, 20, 20, hdcSrc, 10, 10, -2, 3, blend);
    ok( !ret, "GdiAlphaBlend succeeded\n" );
    ret = pGdiAlphaBlend(hdcDst, 0, 0, 20, 20, hdcSrc, 10, 10, -2, 3, blend);
    ok( !ret, "GdiAlphaBlend succeeded\n" );

    SetWindowOrgEx(hdcSrc, -10, -10, NULL);
    SetLastError(0xdeadbeef);
    ret = pGdiAlphaBlend(hdcDst, 0, 0, 20, 20, hdcSrc, -1, 0, 10, 10, blend);
    ok( ret, "GdiAlphaBlend failed err %u\n", GetLastError() );
    SetLastError(0xdeadbeef);
    ret = pGdiAlphaBlend(hdcDst, 0, 0, 20, 20, hdcSrc, 0, -1, 10, 10, blend);
    ok( ret, "GdiAlphaBlend failed err %u\n", GetLastError() );
    SetMapMode(hdcSrc, MM_ANISOTROPIC);
    ScaleWindowExtEx(hdcSrc, 10, 1, 10, 1, NULL);
    SetLastError(0xdeadbeef);
    ret = pGdiAlphaBlend(hdcDst, 0, 0, 20, 20, hdcSrc, -1, 0, 30, 30, blend);
    ok( ret, "GdiAlphaBlend failed err %u\n", GetLastError() );
    SetLastError(0xdeadbeef);
    ret = pGdiAlphaBlend(hdcDst, 0, 0, 20, 20, hdcSrc, 0, -1, 30, 30, blend);
    ok( ret, "GdiAlphaBlend failed err %u\n", GetLastError() );

    SetMapMode(hdcDst, MM_ANISOTROPIC);
    SetViewportExtEx(hdcDst, -1, -1, NULL);
    SetLastError(0xdeadbeef);
    ret = pGdiAlphaBlend(hdcDst, 0, 0, 20, 20, hdcSrc, 0, -1, 50, 50, blend);
    todo_wine
    ok( ret, "GdiAlphaBlend failed err %u\n", GetLastError() );
    SetLastError(0xdeadbeef);
    ret = pGdiAlphaBlend(hdcDst, -20, -20, 20, 20, hdcSrc, 0, -1, 50, 50, blend);
    ok( ret, "GdiAlphaBlend failed err %u\n", GetLastError() );
    SetLastError(0xdeadbeef);
    ret = pGdiAlphaBlend(hdcDst, -20, -20, -20, -20, hdcSrc, 0, -1, 50, 50, blend);
    ok( !ret, "GdiAlphaBlend succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );
    SetLastError(0xdeadbeef);
    ret = pGdiAlphaBlend(hdcDst, -20, 0, -20, 20, hdcSrc, 0, -1, 50, 50, blend);
    ok( !ret, "GdiAlphaBlend succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );
    SetLastError(0xdeadbeef);
    ret = pGdiAlphaBlend(hdcDst, 0, -20, 20, -20, hdcSrc, 0, -1, 50, 50, blend);
    ok( !ret, "GdiAlphaBlend succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );
    SetMapMode(hdcDst, MM_TEXT);

    SetViewportExtEx(hdcSrc, -1, -1, NULL);
    SetLastError(0xdeadbeef);
    ret = pGdiAlphaBlend(hdcDst, 0, 0, 20, 20, hdcSrc, -20, -20, -30, -30, blend);
    ok( !ret, "GdiAlphaBlend succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );
    SetLastError(0xdeadbeef);
    ret = pGdiAlphaBlend(hdcDst, 0, 0, 20, 20, hdcSrc, -20, -20, 30, -30, blend);
    ok( !ret, "GdiAlphaBlend succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );
    SetLastError(0xdeadbeef);
    ret = pGdiAlphaBlend(hdcDst, 0, 0, 20, 20, hdcSrc, -20, -20, -30, 30, blend);
    ok( !ret, "GdiAlphaBlend succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );
    SetLastError(0xdeadbeef);
    ret = pGdiAlphaBlend(hdcDst, 0, 0, 20, 20, hdcSrc, -20, -20, 30, 30, blend);
    ok( !ret, "GdiAlphaBlend succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );
    SetLastError(0xdeadbeef);
    ret = pGdiAlphaBlend(hdcDst, 0, 0, 20, 20, hdcSrc, 20, 20, 30, 30, blend);
    ok( !ret, "GdiAlphaBlend succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );
    SetLastError(0xdeadbeef);
    ret = pGdiAlphaBlend(hdcDst, 0, 0, 20, 20, hdcSrc, -60, -60, 30, 30, blend);
    ok( !ret, "GdiAlphaBlend succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );
    SetViewportExtEx(hdcSrc, 1, 1, NULL);

    SetLastError(0xdeadbeef);
    ret = pGdiAlphaBlend(hdcDst, 0, 0, 20, 20, NULL, 0, 0, 20, 20, blend);
    ok( !ret, "GdiAlphaBlend succeeded\n" );
    ok( GetLastError() == 0xdeadbeef, "wrong error %u\n", GetLastError() );

    /* overlapping source and dest not allowed */

    SetLastError(0xdeadbeef);
    ret = pGdiAlphaBlend(hdcDst, 0, 0, 20, 20, hdcDst, 19, 19, 20, 20, blend);
    ok( !ret, "GdiAlphaBlend succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );

    SetLastError(0xdeadbeef);
    ret = pGdiAlphaBlend(hdcDst, 20, 20, 20, 20, hdcDst, 1, 1, 20, 20, blend);
    ok( !ret, "GdiAlphaBlend succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );

    SetLastError(0xdeadbeef);
    ret = pGdiAlphaBlend(hdcDst, 0, 0, 20, 20, hdcDst, 20, 10, 20, 20, blend);
    ok( ret, "GdiAlphaBlend succeeded\n" );
    SetLastError(0xdeadbeef);
    ret = pGdiAlphaBlend(hdcDst, 0, 0, 20, 20, hdcDst, 10, 20, 20, 20, blend);
    ok( ret, "GdiAlphaBlend succeeded\n" );

    /* AC_SRC_ALPHA requires 32-bpp BI_RGB format */

    blend.AlphaFormat = AC_SRC_ALPHA;
    SetLastError(0xdeadbeef);
    ret = pGdiAlphaBlend(hdcDst, 0, 0, 20, 20, hdcSrc, 0, 0, 20, 20, blend);
    ok( ret, "GdiAlphaBlend failed err %u\n", GetLastError() );

    bmi->bmiHeader.biCompression = BI_BITFIELDS;
    ((DWORD *)bmi->bmiColors)[0] = 0xff0000;
    ((DWORD *)bmi->bmiColors)[1] = 0x00ff00;
    ((DWORD *)bmi->bmiColors)[2] = 0x0000ff;
    bmpSrc = CreateDIBSection(hdcDst, bmi, DIB_RGB_COLORS, &bits, NULL, 0);
    ok(bmpSrc != NULL, "Couldn't create source bitmap\n");
    oldSrc = SelectObject(hdcSrc, bmpSrc);
    DeleteObject( oldSrc );

    SetLastError(0xdeadbeef);
    ret = pGdiAlphaBlend(hdcDst, 0, 0, 20, 20, hdcSrc, 0, 0, 20, 20, blend);
    ok( ret, "GdiAlphaBlend failed err %u\n", GetLastError() );

    bmi->bmiHeader.biCompression = BI_BITFIELDS;
    ((DWORD *)bmi->bmiColors)[0] = 0x0000ff;
    ((DWORD *)bmi->bmiColors)[1] = 0x00ff00;
    ((DWORD *)bmi->bmiColors)[2] = 0xff0000;
    bmpSrc = CreateDIBSection(hdcDst, bmi, DIB_RGB_COLORS, &bits, NULL, 0);
    ok(bmpSrc != NULL, "Couldn't create source bitmap\n");
    oldSrc = SelectObject(hdcSrc, bmpSrc);
    DeleteObject( oldSrc );

    SetLastError(0xdeadbeef);
    ret = pGdiAlphaBlend(hdcDst, 0, 0, 20, 20, hdcSrc, 0, 0, 20, 20, blend);
    ok( !ret, "GdiAlphaBlend succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );

    bmi->bmiHeader.biBitCount = 24;
    bmi->bmiHeader.biCompression = BI_RGB;
    bmpSrc = CreateDIBSection(hdcDst, bmi, DIB_RGB_COLORS, &bits, NULL, 0);
    ok(bmpSrc != NULL, "Couldn't create source bitmap\n");
    oldSrc = SelectObject(hdcSrc, bmpSrc);
    DeleteObject( oldSrc );

    SetLastError(0xdeadbeef);
    ret = pGdiAlphaBlend(hdcDst, 0, 0, 20, 20, hdcSrc, 0, 0, 20, 20, blend);
    ok( !ret, "GdiAlphaBlend succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );

    bmi->bmiHeader.biBitCount = 1;
    bmpSrc = CreateDIBSection(hdcDst, bmi, DIB_RGB_COLORS, &bits, NULL, 0);
    ok(bmpSrc != NULL, "Couldn't create source bitmap\n");
    oldSrc = SelectObject(hdcSrc, bmpSrc);
    DeleteObject( oldSrc );

    SetLastError(0xdeadbeef);
    ret = pGdiAlphaBlend(hdcDst, 0, 0, 20, 20, hdcSrc, 0, 0, 20, 20, blend);
    ok( !ret, "GdiAlphaBlend succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );

    bmpSrc = CreateBitmap( 100, 100, 1, 1, NULL );
    ok(bmpSrc != NULL, "Couldn't create source bitmap\n");
    oldSrc = SelectObject(hdcSrc, bmpSrc);
    DeleteObject( oldSrc );

    SetLastError(0xdeadbeef);
    ret = pGdiAlphaBlend(hdcDst, 0, 0, 20, 20, hdcSrc, 0, 0, 20, 20, blend);
    ok( !ret, "GdiAlphaBlend succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );

    DeleteDC(hdcDst);
    DeleteDC(hdcSrc);
    DeleteObject(bmpSrc);
    DeleteObject(bmpDst);

    ReleaseDC(NULL, hdcNull);
    HeapFree(GetProcessHeap(), 0, bmi);
}

static void test_GdiGradientFill(void)
{
    HDC hdc;
    BOOL ret;
    HBITMAP bmp;
    BITMAPINFO *bmi;
    void *bits;
    GRADIENT_RECT rect[] = { { 0, 0 }, { 0, 1 }, { 2, 3 } };
    GRADIENT_TRIANGLE tri[] = { { 0, 0, 0 }, { 0, 1, 2 }, { 0, 2, 1 }, { 0, 1, 3 } };
    TRIVERTEX vt[3] = { { 2,  2,  0xff00, 0x0000, 0x0000, 0x8000 },
                        { 10, 10, 0x0000, 0xff00, 0x0000, 0x8000 },
                        { 20, 10, 0x0000, 0x0000, 0xff00, 0xff00 } };

    if (!pGdiGradientFill)
    {
        win_skip( "GdiGradientFill is not implemented\n" );
        return;
    }

    hdc = CreateCompatibleDC( NULL );
    bmi = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, FIELD_OFFSET( BITMAPINFO, bmiColors[3] ));
    bmi->bmiHeader.biSize = sizeof(bmi->bmiHeader);
    bmi->bmiHeader.biHeight = 20;
    bmi->bmiHeader.biWidth = 20;
    bmi->bmiHeader.biBitCount = 32;
    bmi->bmiHeader.biPlanes = 1;
    bmi->bmiHeader.biCompression = BI_RGB;
    bmp = CreateDIBSection(hdc, bmi, DIB_RGB_COLORS, &bits, NULL, 0);
    ok( bmp != NULL, "couldn't create bitmap\n" );
    SelectObject( hdc, bmp );

    SetLastError( 0xdeadbeef );
    ret = pGdiGradientFill( hdc, vt, 3, rect, 1, GRADIENT_FILL_RECT_H );
    ok( ret, "GdiGradientFill failed err %u\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = pGdiGradientFill( hdc, vt, 3, rect, 1, 3 );
    ok( !ret, "GdiGradientFill succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = pGdiGradientFill( (HDC)0xdead, vt, 3, rect, 1, GRADIENT_FILL_RECT_H );
    ok( !ret, "GdiGradientFill succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = pGdiGradientFill( NULL, NULL, 0, rect, 1, GRADIENT_FILL_RECT_H );
    ok( !ret, "GdiGradientFill succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );
    ret = pGdiGradientFill( hdc, NULL, 0, rect, 1, GRADIENT_FILL_RECT_H );
    ok( !ret, "GdiGradientFill succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = pGdiGradientFill( hdc, NULL, 3, rect, 1, GRADIENT_FILL_RECT_H );
    ok( !ret, "GdiGradientFill succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = pGdiGradientFill( hdc, vt, 3, NULL, 0, GRADIENT_FILL_RECT_H );
    ok( !ret, "GdiGradientFill succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = pGdiGradientFill( hdc, vt, 3, NULL, 1, GRADIENT_FILL_RECT_H );
    ok( !ret, "GdiGradientFill succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = pGdiGradientFill( hdc, vt, 3, rect, 0, GRADIENT_FILL_RECT_H );
    ok( !ret, "GdiGradientFill succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = pGdiGradientFill( hdc, vt, 3, rect, 3, GRADIENT_FILL_RECT_H );
    ok( !ret, "GdiGradientFill succeeded\n" );
    ok( GetLastError() == 0xdeadbeef, "wrong error %u\n", GetLastError() );
    rect[2].UpperLeft = rect[2].LowerRight = 1;
    SetLastError( 0xdeadbeef );
    ret = pGdiGradientFill( hdc, vt, 3, rect, 3, GRADIENT_FILL_RECT_H );
    ok( ret, "GdiGradientFill failed err %u\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = pGdiGradientFill( hdc, vt, 1, rect, 1, GRADIENT_FILL_RECT_H );
    ok( ret, "GdiGradientFill failed err %u\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = pGdiGradientFill( hdc, vt, 1, tri, 0, GRADIENT_FILL_TRIANGLE );
    ok( !ret, "GdiGradientFill succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = pGdiGradientFill( hdc, vt, 1, tri, 1, GRADIENT_FILL_TRIANGLE );
    ok( ret, "GdiGradientFill failed err %u\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = pGdiGradientFill( hdc, vt, 3, tri, 2, GRADIENT_FILL_TRIANGLE );
    ok( ret, "GdiGradientFill failed err %u\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = pGdiGradientFill( hdc, vt, 3, tri, 3, GRADIENT_FILL_TRIANGLE );
    ok( ret, "GdiGradientFill failed err %u\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = pGdiGradientFill( hdc, vt, 3, tri, 4, GRADIENT_FILL_TRIANGLE );
    ok( !ret, "GdiGradientFill succeeded\n" );
    ok( GetLastError() == 0xdeadbeef, "wrong error %u\n", GetLastError() );
    tri[3].Vertex3 = 1;
    SetLastError( 0xdeadbeef );
    ret = pGdiGradientFill( hdc, vt, 3, tri, 4, GRADIENT_FILL_TRIANGLE );
    ok( !ret, "GdiGradientFill succeeded\n" );
    ok( GetLastError() == 0xdeadbeef, "wrong error %u\n", GetLastError() );
    tri[3].Vertex3 = 0;
    SetLastError( 0xdeadbeef );
    ret = pGdiGradientFill( hdc, vt, 3, tri, 4, GRADIENT_FILL_TRIANGLE );
    ok( !ret, "GdiGradientFill succeeded\n" );
    ok( GetLastError() == 0xdeadbeef, "wrong error %u\n", GetLastError() );
    tri[3].Vertex1 = tri[3].Vertex2 = tri[3].Vertex3 = 1;
    SetLastError( 0xdeadbeef );
    ret = pGdiGradientFill( hdc, vt, 3, tri, 4, GRADIENT_FILL_TRIANGLE );
    ok( ret, "GdiGradientFill failed err %u\n", GetLastError() );

    DeleteDC( hdc );
    DeleteObject( bmp );
    HeapFree(GetProcessHeap(), 0, bmi);
}

static void test_clipping(void)
{
    HBITMAP bmpDst;
    HBITMAP bmpSrc;
    HRGN hRgn;
    LPVOID bits;
    BOOL result;

    HDC hdcDst = CreateCompatibleDC( NULL );
    HDC hdcSrc = CreateCompatibleDC( NULL );

    BITMAPINFO bmpinfo={{0}};
    bmpinfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmpinfo.bmiHeader.biWidth = 100;
    bmpinfo.bmiHeader.biHeight = 100;
    bmpinfo.bmiHeader.biPlanes = 1;
    bmpinfo.bmiHeader.biBitCount = GetDeviceCaps( hdcDst, BITSPIXEL );
    bmpinfo.bmiHeader.biCompression = BI_RGB;

    bmpDst = CreateDIBSection( hdcDst, &bmpinfo, DIB_RGB_COLORS, &bits, NULL, 0 );
    ok(bmpDst != NULL, "Couldn't create destination bitmap\n");
    SelectObject( hdcDst, bmpDst );

    bmpSrc = CreateDIBSection( hdcSrc, &bmpinfo, DIB_RGB_COLORS, &bits, NULL, 0 );
    ok(bmpSrc != NULL, "Couldn't create source bitmap\n");
    SelectObject( hdcSrc, bmpSrc );

    result = BitBlt( hdcDst, 0, 0, 100, 100, hdcSrc, 100, 100, SRCCOPY );
    ok(result, "BitBlt failed\n");

    hRgn = CreateRectRgn( 0,0,0,0 );
    SelectClipRgn( hdcDst, hRgn );

    result = BitBlt( hdcDst, 0, 0, 100, 100, hdcSrc, 0, 0, SRCCOPY );
    ok(result, "BitBlt failed\n");

    DeleteObject( bmpDst );
    DeleteObject( bmpSrc );
    DeleteObject( hRgn );
    DeleteDC( hdcDst );
    DeleteDC( hdcSrc );
}

static void test_32bit_ddb(void)
{
    char buffer[sizeof(BITMAPINFOHEADER) + sizeof(DWORD)];
    BITMAPINFO *biDst = (BITMAPINFO *)buffer;
    HBITMAP bmpSrc, bmpDst;
    HBITMAP oldSrc, oldDst;
    HDC hdcSrc, hdcDst, hdcScreen;
    HBRUSH brush;
    DWORD *dstBuffer, *data;
    DWORD colorSrc = 0x40201008;

    memset(biDst, 0, sizeof(BITMAPINFOHEADER));
    biDst->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    biDst->bmiHeader.biWidth = 1;
    biDst->bmiHeader.biHeight = -1;
    biDst->bmiHeader.biPlanes = 1;
    biDst->bmiHeader.biBitCount = 32;
    biDst->bmiHeader.biCompression = BI_RGB;

    hdcScreen = CreateCompatibleDC(0);
    if(GetDeviceCaps(hdcScreen, BITSPIXEL) != 32)
    {
        DeleteDC(hdcScreen);
        trace("Skipping 32-bit DDB test\n");
        return;
    }

    hdcSrc = CreateCompatibleDC(hdcScreen);
    bmpSrc = CreateBitmap(1, 1, 1, 32, &colorSrc);
    oldSrc = SelectObject(hdcSrc, bmpSrc);

    hdcDst = CreateCompatibleDC(hdcScreen);
    bmpDst = CreateDIBSection(hdcDst, biDst, DIB_RGB_COLORS, (void**)&dstBuffer, NULL, 0);
    oldDst = SelectObject(hdcDst, bmpDst);

    StretchBlt(hdcDst, 0, 0, 1, 1, hdcSrc, 0, 0, 1, 1, SRCCOPY);
    ok(dstBuffer[0] == colorSrc, "Expected color=%x, received color=%x\n", colorSrc, dstBuffer[0]);

    if (pGdiAlphaBlend)
    {
        BLENDFUNCTION blend;
        BOOL ret;

        blend.BlendOp = AC_SRC_OVER;
        blend.BlendFlags = 0;
        blend.SourceConstantAlpha = 128;
        blend.AlphaFormat = 0;
        dstBuffer[0] = 0x80808080;
        ret = pGdiAlphaBlend( hdcDst, 0, 0, 1, 1, hdcSrc, 0, 0, 1, 1, blend );
        ok( ret, "GdiAlphaBlend failed err %u\n", GetLastError() );
        ok(dstBuffer[0] == 0x60504844, "wrong color %x\n", dstBuffer[0]);
        blend.AlphaFormat = AC_SRC_ALPHA;
        dstBuffer[0] = 0x80808080;
        ret = pGdiAlphaBlend( hdcDst, 0, 0, 1, 1, hdcSrc, 0, 0, 1, 1, blend );
        ok( ret, "GdiAlphaBlend failed err %u\n", GetLastError() );
        ok(dstBuffer[0] == 0x90807874, "wrong color %x\n", dstBuffer[0]);
    }

    data = (DWORD *)biDst->bmiColors;
    data[0] = 0x20304050;
    brush = CreateDIBPatternBrushPt( biDst, DIB_RGB_COLORS );
    ok( brush != 0, "brush creation failed\n" );
    SelectObject( hdcSrc, brush );
    PatBlt( hdcSrc, 0, 0, 1, 1, PATCOPY );
    BitBlt( hdcDst, 0, 0, 1, 1, hdcSrc, 0, 0, SRCCOPY );
    ok(dstBuffer[0] == data[0], "Expected color=%x, received color=%x\n", data[0], dstBuffer[0]);
    SelectObject( hdcSrc, GetStockObject(BLACK_BRUSH) );
    DeleteObject( brush );

    biDst->bmiHeader.biBitCount = 24;
    brush = CreateDIBPatternBrushPt( biDst, DIB_RGB_COLORS );
    ok( brush != 0, "brush creation failed\n" );
    SelectObject( hdcSrc, brush );
    PatBlt( hdcSrc, 0, 0, 1, 1, PATCOPY );
    BitBlt( hdcDst, 0, 0, 1, 1, hdcSrc, 0, 0, SRCCOPY );
    ok(dstBuffer[0] == (data[0] & ~0xff000000),
       "Expected color=%x, received color=%x\n", data[0] & 0xff000000, dstBuffer[0]);
    SelectObject( hdcSrc, GetStockObject(BLACK_BRUSH) );
    DeleteObject( brush );

    /* Tidy up */
    SelectObject(hdcDst, oldDst);
    DeleteObject(bmpDst);
    DeleteDC(hdcDst);

    SelectObject(hdcSrc, oldSrc);
    DeleteObject(bmpSrc);
    DeleteDC(hdcSrc);

    DeleteDC(hdcScreen);
}

/*
 * Used by test_GetDIBits_top_down to create the bitmap to test against.
 */
static void setup_picture(char *picture, int bpp)
{
    int i;

    switch(bpp)
    {
        case 16:
        case 32:
            /*Set the first byte in each pixel to the index of that pixel.*/
            for (i = 0; i < 4; i++)
                picture[i * (bpp / 8)] = i;
            break;
        case 24:
            picture[0] = 0;
            picture[3] = 1;
            /*Each scanline in a bitmap must be a multiple of 4 bytes long.*/
            picture[8] = 2;
            picture[11] = 3;
            break;
    }
}

static void test_GetDIBits_top_down(int bpp)
{
    BITMAPINFO bi;
    HBITMAP bmptb, bmpbt;
    HDC hdc;
    int pictureOut[4];
    int *picture;
    int statusCode;

    memset( &bi, 0, sizeof(bi) );
    bi.bmiHeader.biSize=sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth=2;
    bi.bmiHeader.biHeight=2;
    bi.bmiHeader.biPlanes=1;
    bi.bmiHeader.biBitCount=bpp;
    bi.bmiHeader.biCompression=BI_RGB;

    /*Get the device context for the screen.*/
    hdc = GetDC(NULL);
    ok(hdc != NULL, "Could not get a handle to a device context.\n");

    /*Create the bottom to top image (image's bottom scan line is at the top in memory).*/
    bmpbt = CreateDIBSection(hdc, &bi, DIB_RGB_COLORS, (void**)&picture, NULL, 0);
    ok(bmpbt != NULL, "Could not create a DIB section.\n");
    /*Now that we have a pointer to the pixels, we write to them.*/
    setup_picture((char*)picture, bpp);
    /*Create the top to bottom image (images' bottom scan line is at the bottom in memory).*/
    bi.bmiHeader.biHeight=-2; /*We specify that we want a top to bottom image by specifying a negative height.*/
    bmptb = CreateDIBSection(hdc, &bi, DIB_RGB_COLORS, (void**)&picture, NULL, 0);
    ok(bmptb != NULL, "Could not create a DIB section.\n");
    /*Write to this top to bottom bitmap.*/
    setup_picture((char*)picture, bpp);

    bi.bmiHeader.biWidth = 1;

    bi.bmiHeader.biHeight = 2;
    statusCode = GetDIBits(hdc, bmpbt, 0, 1, pictureOut, &bi, DIB_RGB_COLORS);
    ok(statusCode, "Failed to call GetDIBits. Status code: %d.\n", statusCode);
    /*Check the first byte of the pixel.*/
    ok((char)pictureOut[0] == 0, "Bottom-up -> bottom-up: first pixel should be 0 but was %d.\n", (char)pictureOut[0]);
    statusCode = GetDIBits(hdc, bmptb, 0, 1, pictureOut, &bi, DIB_RGB_COLORS);
    ok(statusCode, "Failed to call GetDIBits. Status code: %d.\n", statusCode);
    ok((char)pictureOut[0] == 2, "Top-down -> bottom-up: first pixel should be 2 but was %d.\n", (char)pictureOut[0]);
    /*Check second scanline.*/
    statusCode = GetDIBits(hdc, bmptb, 1, 1, pictureOut, &bi, DIB_RGB_COLORS);
    ok(statusCode, "Failed to call GetDIBits. Status code: %d.\n", statusCode);
    ok((char)pictureOut[0] == 0, "Top-down -> bottom-up: first pixel should be 0 but was %d.\n", (char)pictureOut[0]);
    statusCode = GetDIBits(hdc, bmpbt, 1, 1, pictureOut, &bi, DIB_RGB_COLORS);
    ok(statusCode, "Failed to call GetDIBits. Status code: %d.\n", statusCode);
    ok((char)pictureOut[0] == 2, "Bottom-up -> bottom-up: first pixel should be 2 but was %d.\n", (char)pictureOut[0]);
    /*Check both scanlines.*/
    statusCode = GetDIBits(hdc, bmptb, 0, 2, pictureOut, &bi, DIB_RGB_COLORS);
    ok(statusCode, "Failed to call GetDIBits. Status code: %d.\n", statusCode);
    ok((char)pictureOut[0] == 2, "Top-down -> bottom-up: first scanline should be 2 but was %d.\n", (char)pictureOut[0]);
    ok((char)pictureOut[1] == 0, "Top-down -> bottom-up: second scanline should be 0 but was %d.\n", (char)pictureOut[0]);
    statusCode = GetDIBits(hdc, bmpbt, 0, 2, pictureOut, &bi, DIB_RGB_COLORS);
    ok(statusCode, "Failed to call GetDIBits. Status code: %d.\n", statusCode);
    ok((char)pictureOut[0] == 0, "Bottom up -> bottom-up: first scanline should be 0 but was %d.\n", (char)pictureOut[0]);
    ok((char)pictureOut[1] == 2, "Bottom up -> bottom-up: second scanline should be 2 but was %d.\n", (char)pictureOut[0]);

    /*Make destination bitmap top-down.*/
    bi.bmiHeader.biHeight = -2;
    statusCode = GetDIBits(hdc, bmpbt, 0, 1, pictureOut, &bi, DIB_RGB_COLORS);
    ok(statusCode, "Failed to call GetDIBits. Status code: %d.\n", statusCode);
    ok((char)pictureOut[0] == 0, "Bottom-up -> top-down: first pixel should be 0 but was %d.\n", (char)pictureOut[0]);
    statusCode = GetDIBits(hdc, bmptb, 0, 1, pictureOut, &bi, DIB_RGB_COLORS);
    ok(statusCode, "Failed to call GetDIBits. Status code: %d.\n", statusCode);
    ok((char)pictureOut[0] == 2, "Top-down -> top-down: first pixel should be 2 but was %d.\n", (char)pictureOut[0]);
    /*Check second scanline.*/
    statusCode = GetDIBits(hdc, bmptb, 1, 1, pictureOut, &bi, DIB_RGB_COLORS);
    ok(statusCode, "Failed to call GetDIBits. Status code: %d.\n", statusCode);
    ok((char)pictureOut[0] == 0, "Top-down -> bottom-up: first pixel should be 0 but was %d.\n", (char)pictureOut[0]);
    statusCode = GetDIBits(hdc, bmpbt, 1, 1, pictureOut, &bi, DIB_RGB_COLORS);
    ok(statusCode, "Failed to call GetDIBits. Status code: %d.\n", statusCode);
    ok((char)pictureOut[0] == 2, "Top-down -> bottom-up: first pixel should be 2 but was %d.\n", (char)pictureOut[0]);
    /*Check both scanlines.*/
    statusCode = GetDIBits(hdc, bmptb, 0, 2, pictureOut, &bi, DIB_RGB_COLORS);
    ok(statusCode, "Failed to call GetDIBits. Status code: %d.\n", statusCode);
    ok((char)pictureOut[0] == 0, "Top-down -> top-down: first scanline should be 0 but was %d.\n", (char)pictureOut[0]);
    ok((char)pictureOut[1] == 2, "Top-down -> top-down: second scanline should be 2 but was %d.\n", (char)pictureOut[0]);
    statusCode = GetDIBits(hdc, bmpbt, 0, 2, pictureOut, &bi, DIB_RGB_COLORS);
    ok(statusCode, "Failed to call GetDIBits. Status code: %d.\n", statusCode);
    ok((char)pictureOut[0] == 2, "Bottom up -> top-down: first scanline should be 2 but was %d.\n", (char)pictureOut[0]);
    ok((char)pictureOut[1] == 0, "Bottom up -> top-down: second scanline should be 0 but was %d.\n", (char)pictureOut[0]);

    DeleteObject(bmpbt);
    DeleteObject(bmptb);
}

static void test_GetSetDIBits_rtl(void)
{
    HDC hdc, hdc_mem;
    HBITMAP bitmap, orig_bitmap;
    BITMAPINFO info;
    int ret;
    DWORD bits_1[8 * 8], bits_2[8 * 8];

    if(!pSetLayout)
    {
        win_skip("Don't have SetLayout\n");
        return;
    }

    hdc = GetDC( NULL );
    hdc_mem = CreateCompatibleDC( hdc );
    pSetLayout( hdc_mem, LAYOUT_LTR );

    bitmap = CreateCompatibleBitmap( hdc, 8, 8 );
    orig_bitmap = SelectObject( hdc_mem, bitmap );
    SetPixel( hdc_mem, 0, 0, RGB(0xff, 0, 0) );
    SelectObject( hdc_mem, orig_bitmap );

    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = 8;
    info.bmiHeader.biHeight = 8;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;

    /* First show that GetDIBits ignores the layout mode. */

    ret = GetDIBits( hdc_mem, bitmap, 0, 8, bits_1, &info, DIB_RGB_COLORS );
    ok(ret == 8, "got %d\n", ret);
    ok(bits_1[56] == 0xff0000, "got %08x\n", bits_1[56]); /* check we have a red pixel */

    pSetLayout( hdc_mem, LAYOUT_RTL );

    ret = GetDIBits( hdc_mem, bitmap, 0, 8, bits_2, &info, DIB_RGB_COLORS );
    ok(ret == 8, "got %d\n", ret);

    ok(!memcmp( bits_1, bits_2, sizeof(bits_1) ), "bits differ\n");

    /* Now to show that SetDIBits also ignores the mode, we perform a SetDIBits
       followed by a GetDIBits and show that the bits remain unchanged. */

    pSetLayout( hdc_mem, LAYOUT_LTR );

    ret = SetDIBits( hdc_mem, bitmap, 0, 8, bits_1, &info, DIB_RGB_COLORS );
    ok(ret == 8, "got %d\n", ret);
    ret = GetDIBits( hdc_mem, bitmap, 0, 8, bits_2, &info, DIB_RGB_COLORS );
    ok(ret == 8, "got %d\n", ret);
    ok(!memcmp( bits_1, bits_2, sizeof(bits_1) ), "bits differ\n");

    pSetLayout( hdc_mem, LAYOUT_RTL );

    ret = SetDIBits( hdc_mem, bitmap, 0, 8, bits_1, &info, DIB_RGB_COLORS );
    ok(ret == 8, "got %d\n", ret);
    ret = GetDIBits( hdc_mem, bitmap, 0, 8, bits_2, &info, DIB_RGB_COLORS );
    ok(ret == 8, "got %d\n", ret);
    ok(!memcmp( bits_1, bits_2, sizeof(bits_1) ), "bits differ\n");

    DeleteObject( bitmap );
    DeleteDC( hdc_mem );
    ReleaseDC( NULL, hdc );
}

static void test_GetDIBits_scanlines(void)
{
    BITMAPINFO *info;
    DWORD *dib_bits;
    HDC hdc = GetDC( NULL );
    HBITMAP dib;
    DWORD data[128], inverted_bits[64];
    int i, ret;

    info = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, FIELD_OFFSET( BITMAPINFO, bmiColors[256] ) );

    info->bmiHeader.biSize        = sizeof(info->bmiHeader);
    info->bmiHeader.biWidth       = 8;
    info->bmiHeader.biHeight      = 8;
    info->bmiHeader.biPlanes      = 1;
    info->bmiHeader.biBitCount    = 32;
    info->bmiHeader.biCompression = BI_RGB;

    dib = CreateDIBSection( NULL, info, DIB_RGB_COLORS, (void**)&dib_bits, NULL, 0 );

    for (i = 0; i < 64; i++)
    {
        dib_bits[i] = i;
        inverted_bits[56 - (i & ~7) + (i & 7)] = i;
    }

    /* b-u -> b-u */

    memset( data, 0xaa, sizeof(data) );

    ret = GetDIBits( hdc, dib, 0, 8, data, info, DIB_RGB_COLORS );
    ok( ret == 8, "got %d\n", ret );
    ok( !memcmp( data, dib_bits, 64 * 4 ), "bits differ\n");
    memset( data, 0xaa, sizeof(data) );

    ret = GetDIBits( hdc, dib, 1, 5, data, info, DIB_RGB_COLORS );
    ok( ret == 5, "got %d\n", ret );
    ok( !memcmp( data, dib_bits + 8, 40 * 4 ), "bits differ\n");
    for (i = 40; i < 64; i++) ok( data[i] == 0xaaaaaaaa, "%d: got %08x\n", i, data[i] );
    memset( data, 0xaa, sizeof(data) );

    ret = GetDIBits( hdc, dib, 1, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 7, "got %d\n", ret );
    ok( !memcmp( data, dib_bits + 8, 56 * 4 ), "bits differ\n");
    for (i = 56; i < 64; i++) ok( data[i] == 0xaaaaaaaa, "%d: got %08x\n", i, data[i] );
    memset( data, 0xaa, sizeof(data) );

    ret = GetDIBits( hdc, dib, 9, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 1, "got %d\n", ret );
    for (i = 0; i < 64; i++) ok( data[i] == 0xaaaaaaaa, "%d: got %08x\n", i, data[i] );
    memset( data, 0xaa, sizeof(data) );

    info->bmiHeader.biHeight = 16;
    ret = GetDIBits( hdc, dib, 1, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 5, "got %d\n", ret );
    for (i = 0; i < 56; i++) ok( data[i] == 0, "%d: got %08x\n", i, data[i] );
    ok( !memcmp( data + 56, dib_bits, 40 * 4 ), "bits differ\n");
    for (i = 96; i < 128; i++) ok( data[i] == 0xaaaaaaaa, "%d: got %08x\n", i, data[i] );
    memset( data, 0xaa, sizeof(data) );

    ret = GetDIBits( hdc, dib, 2, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 6, "got %d\n", ret );
    for (i = 0; i < 48; i++) ok( data[i] == 0, "%d: got %08x\n", i, data[i] );
    ok( !memcmp( data + 48, dib_bits, 48 * 4 ), "bits differ\n");
    for (i = 96; i < 128; i++) ok( data[i] == 0xaaaaaaaa, "%d: got %08x\n", i, data[i] );
    memset( data, 0xaa, sizeof(data) );

    ret = GetDIBits( hdc, dib, 2, 3, data, info, DIB_RGB_COLORS );
    ok( ret == 0, "got %d\n", ret );
    for (i = 0; i < 24; i++) ok( data[i] == 0, "%d: got %08x\n", i, data[i] );
    for (i = 24; i < 128; i++) ok( data[i] == 0xaaaaaaaa, "%d: got %08x\n", i, data[i] );
    memset( data, 0xaa, sizeof(data) );

    info->bmiHeader.biHeight = 5;
    ret = GetDIBits( hdc, dib, 1, 2, data, info, DIB_RGB_COLORS );
    ok( ret == 2, "got %d\n", ret );
    ok( !memcmp( data, dib_bits + 32, 16 * 4 ), "bits differ\n");
    for (i = 16; i < 128; i++) ok( data[i] == 0xaaaaaaaa, "%d: got %08x\n", i, data[i] );
    memset( data, 0xaa, sizeof(data) );

    /* b-u -> t-d */

    info->bmiHeader.biHeight = -8;
    ret = GetDIBits( hdc, dib, 0, 8, data, info, DIB_RGB_COLORS );
    ok( ret == 8, "got %d\n", ret );
    ok( !memcmp( data, inverted_bits, 64 * 4 ), "bits differ\n");
    memset( data, 0xaa, sizeof(data) );

    ret = GetDIBits( hdc, dib, 1, 5, data, info, DIB_RGB_COLORS );
    ok( ret == 5, "got %d\n", ret );
    ok( !memcmp( data, inverted_bits + 16, 40 * 4 ), "bits differ\n");
    for (i = 40; i < 64; i++) ok( data[i] == 0xaaaaaaaa, "%d: got %08x\n", i, data[i] );
    memset( data, 0xaa, sizeof(data) );

    ret = GetDIBits( hdc, dib, 1, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 7, "got %d\n", ret );
    ok( !memcmp( data, inverted_bits, 56 * 4 ), "bits differ\n");
    for (i = 56; i < 64; i++) ok( data[i] == 0xaaaaaaaa, "%d: got %08x\n", i, data[i] );
    memset( data, 0xaa, sizeof(data) );

    ret = GetDIBits( hdc, dib, 4, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 4, "got %d\n", ret );
    ok( !memcmp( data, inverted_bits, 32 * 4 ), "bits differ\n");
    for (i = 32; i < 64; i++) ok( data[i] == 0xaaaaaaaa, "%d: got %08x\n", i, data[i] );
    memset( data, 0xaa, sizeof(data) );

    ret = GetDIBits( hdc, dib, 3, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 5, "got %d\n", ret );
    ok( !memcmp( data, inverted_bits, 40 * 4 ), "bits differ\n");
    for (i = 40; i < 64; i++) ok( data[i] == 0xaaaaaaaa, "%d: got %08x\n", i, data[i] );
    memset( data, 0xaa, sizeof(data) );

    ret = GetDIBits( hdc, dib, 3, 13, data, info, DIB_RGB_COLORS );
    ok( ret == 5, "got %d\n", ret );
    ok( !memcmp( data, inverted_bits, 40 * 4 ), "bits differ\n");
    for (i = 40; i < 64; i++) ok( data[i] == 0xaaaaaaaa, "%d: got %08x\n", i, data[i] );
    memset( data, 0xaa, sizeof(data) );

    info->bmiHeader.biHeight = -16;
    ret = GetDIBits( hdc, dib, 0, 16, data, info, DIB_RGB_COLORS );
    ok( ret == 8, "got %d\n", ret );
    ok( !memcmp( data, inverted_bits, 64 * 4 ), "bits differ\n");
    for (i = 64; i < 128; i++) ok( data[i] == 0, "%d: got %08x\n", i, data[i] );
    memset( data, 0xaa, sizeof(data) );

    ret = GetDIBits( hdc, dib, 1, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 5, "got %d\n", ret );
    ok( !memcmp( data, inverted_bits + 24, 40 * 4 ), "bits differ\n");
    for (i = 40; i < 96; i++) ok( data[i] == 0, "%d: got %08x\n", i, data[i] );
    for (i = 96; i < 128; i++) ok( data[i] == 0xaaaaaaaa, "%d: got %08x\n", i, data[i] );
    memset( data, 0xaa, sizeof(data) );

    ret = GetDIBits( hdc, dib, 4, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 8, "got %d\n", ret );
    ok( !memcmp( data, inverted_bits, 64 * 4 ), "bits differ\n");
    for (i = 64; i < 96; i++) ok( data[i] == 0, "%d: got %08x\n", i, data[i] );
    for (i = 96; i < 128; i++) ok( data[i] == 0xaaaaaaaa, "%d: got %08x\n", i, data[i] );
    memset( data, 0xaa, sizeof(data) );

    ret = GetDIBits( hdc, dib, 5, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 8, "got %d\n", ret );
    ok( !memcmp( data, inverted_bits, 64 * 4 ), "bits differ\n");
    for (i = 64; i < 88; i++) ok( data[i] == 0, "%d: got %08x\n", i, data[i] );
    for (i = 88; i < 128; i++) ok( data[i] == 0xaaaaaaaa, "%d: got %08x\n", i, data[i] );
    memset( data, 0xaa, sizeof(data) );

    ret = GetDIBits( hdc, dib, 9, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 7, "got %d\n", ret );
    ok( !memcmp( data, inverted_bits, 56 * 4 ), "bits differ\n");
    for (i = 56; i < 128; i++) ok( data[i] == 0xaaaaaaaa, "%d: got %08x\n", i, data[i] );
    memset( data, 0xaa, sizeof(data) );

    ret = GetDIBits( hdc, dib, 18, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 1, "got %d\n", ret );
    for (i = 0; i < 128; i++) ok( data[i] == 0xaaaaaaaa, "%d: got %08x\n", i, data[i] );
    memset( data, 0xaa, sizeof(data) );

    info->bmiHeader.biHeight = -5;
    ret = GetDIBits( hdc, dib, 1, 2, data, info, DIB_RGB_COLORS );
    ok( ret == 2, "got %d\n", ret );
    ok( !memcmp( data, inverted_bits + 16, 16 * 4 ), "bits differ\n");
    for (i = 16; i < 128; i++) ok( data[i] == 0xaaaaaaaa, "%d: got %08x\n", i, data[i] );
    memset( data, 0xaa, sizeof(data) );

    DeleteObject( dib );

    info->bmiHeader.biSize        = sizeof(info->bmiHeader);
    info->bmiHeader.biWidth       = 8;
    info->bmiHeader.biHeight      = -8;
    info->bmiHeader.biPlanes      = 1;
    info->bmiHeader.biBitCount    = 32;
    info->bmiHeader.biCompression = BI_RGB;

    dib = CreateDIBSection( NULL, info, DIB_RGB_COLORS, (void**)&dib_bits, NULL, 0 );

    for (i = 0; i < 64; i++) dib_bits[i] = i;

    /* t-d -> t-d */

    info->bmiHeader.biHeight = -8;
    ret = GetDIBits( hdc, dib, 0, 8, data, info, DIB_RGB_COLORS );
    ok( ret == 8, "got %d\n", ret );
    ok( !memcmp( data, dib_bits, 64 * 4 ), "bits differ\n");
    memset( data, 0xaa, sizeof(data) );

    ret = GetDIBits( hdc, dib, 1, 5, data, info, DIB_RGB_COLORS );
    ok( ret == 5, "got %d\n", ret );
    ok( !memcmp( data, dib_bits + 16, 40 * 4 ), "bits differ\n");
    for (i = 40; i < 64; i++) ok( data[i] == 0xaaaaaaaa, "%d: got %08x\n", i, data[i] );
    memset( data, 0xaa, sizeof(data) );

    ret = GetDIBits( hdc, dib, 1, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 7, "got %d\n", ret );
    ok( !memcmp( data, dib_bits, 56 * 4 ), "bits differ\n");
    for (i = 56; i < 64; i++) ok( data[i] == 0xaaaaaaaa, "%d: got %08x\n", i, data[i] );
    memset( data, 0xaa, sizeof(data) );

    ret = GetDIBits( hdc, dib, 4, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 4, "got %d\n", ret );
    ok( !memcmp( data, dib_bits, 32 * 4 ), "bits differ\n");
    for (i = 32; i < 64; i++) ok( data[i] == 0xaaaaaaaa, "%d: got %08x\n", i, data[i] );
    memset( data, 0xaa, sizeof(data) );

    ret = GetDIBits( hdc, dib, 3, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 5, "got %d\n", ret );
    ok( !memcmp( data, dib_bits, 40 * 4 ), "bits differ\n");
    for (i = 40; i < 64; i++) ok( data[i] == 0xaaaaaaaa, "%d: got %08x\n", i, data[i] );
    memset( data, 0xaa, sizeof(data) );

    ret = GetDIBits( hdc, dib, 3, 13, data, info, DIB_RGB_COLORS );
    ok( ret == 5, "got %d\n", ret );
    ok( !memcmp( data, dib_bits, 40 * 4 ), "bits differ\n");
    for (i = 40; i < 64; i++) ok( data[i] == 0xaaaaaaaa, "%d: got %08x\n", i, data[i] );
    memset( data, 0xaa, sizeof(data) );

    info->bmiHeader.biHeight = -16;
    ret = GetDIBits( hdc, dib, 0, 16, data, info, DIB_RGB_COLORS );
    ok( ret == 8, "got %d\n", ret );
    ok( !memcmp( data, dib_bits, 64 * 4 ), "bits differ\n");
    for (i = 64; i < 128; i++) ok( data[i] == 0, "%d: got %08x\n", i, data[i] );
    memset( data, 0xaa, sizeof(data) );

    ret = GetDIBits( hdc, dib, 1, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 5, "got %d\n", ret );
    ok( !memcmp( data, dib_bits + 24, 40 * 4 ), "bits differ\n");
    for (i = 40; i < 96; i++) ok( data[i] == 0, "%d: got %08x\n", i, data[i] );
    for (i = 96; i < 128; i++) ok( data[i] == 0xaaaaaaaa, "%d: got %08x\n", i, data[i] );
    memset( data, 0xaa, sizeof(data) );

    ret = GetDIBits( hdc, dib, 4, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 8, "got %d\n", ret );
    ok( !memcmp( data, dib_bits, 64 * 4 ), "bits differ\n");
    for (i = 64; i < 96; i++) ok( data[i] == 0, "%d: got %08x\n", i, data[i] );
    for (i = 96; i < 128; i++) ok( data[i] == 0xaaaaaaaa, "%d: got %08x\n", i, data[i] );
    memset( data, 0xaa, sizeof(data) );

    ret = GetDIBits( hdc, dib, 5, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 8, "got %d\n", ret );
    ok( !memcmp( data, dib_bits, 64 * 4 ), "bits differ\n");
    for (i = 64; i < 88; i++) ok( data[i] == 0, "%d: got %08x\n", i, data[i] );
    for (i = 88; i < 128; i++) ok( data[i] == 0xaaaaaaaa, "%d: got %08x\n", i, data[i] );
    memset( data, 0xaa, sizeof(data) );

    ret = GetDIBits( hdc, dib, 9, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 7, "got %d\n", ret );
    ok( !memcmp( data, dib_bits, 56 * 4 ), "bits differ\n");
    for (i = 56; i < 128; i++) ok( data[i] == 0xaaaaaaaa, "%d: got %08x\n", i, data[i] );
    memset( data, 0xaa, sizeof(data) );

    info->bmiHeader.biHeight = -5;
    ret = GetDIBits( hdc, dib, 1, 2, data, info, DIB_RGB_COLORS );
    ok( ret == 2, "got %d\n", ret );
    ok( !memcmp( data, dib_bits + 16, 16 * 4 ), "bits differ\n");
    for (i = 16; i < 128; i++) ok( data[i] == 0xaaaaaaaa, "%d: got %08x\n", i, data[i] );
    memset( data, 0xaa, sizeof(data) );


    /* t-d -> b-u */

    info->bmiHeader.biHeight = 8;

    ret = GetDIBits( hdc, dib, 0, 8, data, info, DIB_RGB_COLORS );
    ok( ret == 8, "got %d\n", ret );
    ok( !memcmp( data, inverted_bits, 64 * 4 ), "bits differ\n");
    memset( data, 0xaa, sizeof(data) );

    ret = GetDIBits( hdc, dib, 1, 5, data, info, DIB_RGB_COLORS );
    ok( ret == 5, "got %d\n", ret );
    ok( !memcmp( data, inverted_bits + 8, 40 * 4 ), "bits differ\n");
    for (i = 40; i < 64; i++) ok( data[i] == 0xaaaaaaaa, "%d: got %08x\n", i, data[i] );
    memset( data, 0xaa, sizeof(data) );

    ret = GetDIBits( hdc, dib, 1, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 7, "got %d\n", ret );
    ok( !memcmp( data, inverted_bits + 8, 56 * 4 ), "bits differ\n");
    for (i = 56; i < 64; i++) ok( data[i] == 0xaaaaaaaa, "%d: got %08x\n", i, data[i] );
    memset( data, 0xaa, sizeof(data) );

    ret = GetDIBits( hdc, dib, 9, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 1, "got %d\n", ret );
    for (i = 0; i < 64; i++) ok( data[i] == 0xaaaaaaaa, "%d: got %08x\n", i, data[i] );
    memset( data, 0xaa, sizeof(data) );

    info->bmiHeader.biHeight = 16;
    ret = GetDIBits( hdc, dib, 1, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 5, "got %d\n", ret );
    for (i = 0; i < 56; i++) ok( data[i] == 0, "%d: got %08x\n", i, data[i] );
    ok( !memcmp( data + 56, inverted_bits, 40 * 4 ), "bits differ\n");
    for (i = 96; i < 128; i++) ok( data[i] == 0xaaaaaaaa, "%d: got %08x\n", i, data[i] );
    memset( data, 0xaa, sizeof(data) );

    ret = GetDIBits( hdc, dib, 2, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 6, "got %d\n", ret );
    for (i = 0; i < 48; i++) ok( data[i] == 0, "%d: got %08x\n", i, data[i] );
    ok( !memcmp( data + 48, inverted_bits, 48 * 4 ), "bits differ\n");
    for (i = 96; i < 128; i++) ok( data[i] == 0xaaaaaaaa, "%d: got %08x\n", i, data[i] );
    memset( data, 0xaa, sizeof(data) );

    ret = GetDIBits( hdc, dib, 2, 3, data, info, DIB_RGB_COLORS );
    ok( ret == 0, "got %d\n", ret );
    for (i = 0; i < 24; i++) ok( data[i] == 0, "%d: got %08x\n", i, data[i] );
    for (i = 24; i < 128; i++) ok( data[i] == 0xaaaaaaaa, "%d: got %08x\n", i, data[i] );
    memset( data, 0xaa, sizeof(data) );

    info->bmiHeader.biHeight = 5;
    ret = GetDIBits( hdc, dib, 1, 2, data, info, DIB_RGB_COLORS );
    ok( ret == 2, "got %d\n", ret );
    ok( !memcmp( data, inverted_bits + 32, 16 * 4 ), "bits differ\n");
    for (i = 16; i < 128; i++) ok( data[i] == 0xaaaaaaaa, "%d: got %08x\n", i, data[i] );

    DeleteObject( dib );

    ReleaseDC( NULL, hdc );
    HeapFree( GetProcessHeap(), 0, info );
}


static void test_SetDIBits(void)
{
    char palbuf[sizeof(LOGPALETTE) + 256 * sizeof(PALETTEENTRY)];
    LOGPALETTE *pal = (LOGPALETTE *)palbuf;
    PALETTEENTRY *palent = pal->palPalEntry;
    HPALETTE palette;
    BITMAPINFO *info;
    DWORD *dib_bits;
    HDC hdc = GetDC( NULL );
    DWORD data[128], inverted_data[128];
    HBITMAP dib;
    int i, ret;

    info = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, FIELD_OFFSET( BITMAPINFO, bmiColors[256] ) );

    info->bmiHeader.biSize        = sizeof(info->bmiHeader);
    info->bmiHeader.biWidth       = 8;
    info->bmiHeader.biHeight      = 8;
    info->bmiHeader.biPlanes      = 1;
    info->bmiHeader.biBitCount    = 32;
    info->bmiHeader.biCompression = BI_RGB;

    dib = CreateDIBSection( NULL, info, DIB_RGB_COLORS, (void**)&dib_bits, NULL, 0 );
    memset( dib_bits, 0xaa, 64 * 4 );

    for (i = 0; i < 128; i++)
    {
        data[i] = i;
        inverted_data[120 - (i & ~7) + (i & 7)] = i;
    }

    /* b-u -> b-u */

    ret = SetDIBits( hdc, dib, 0, 8, data, info, DIB_RGB_COLORS );
    ok( ret == 8, "got %d\n", ret );
    ok( !memcmp( dib_bits, data, 64 * 4 ), "bits differ\n");
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBits( hdc, dib, 1, 5, data, info, DIB_RGB_COLORS );
    ok( ret == 5, "got %d\n", ret );
    for (i = 0; i < 8; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    ok( !memcmp( dib_bits + 8, data, 40 * 4 ), "bits differ\n");
    for (i = 48; i < 64; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    /* top of dst is aligned with startscans down for the top of the src.
       Then starting from the bottom of src, lines rows are copied across. */

    info->bmiHeader.biHeight = 16;
    ret = SetDIBits( hdc, dib, 1, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 12, "got %d\n", ret );
    ok( !memcmp( dib_bits, data + 56,  40 * 4 ), "bits differ\n");
    for (i = 40; i < 64; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    info->bmiHeader.biHeight = 5;
    ret = SetDIBits( hdc, dib, 1, 2, data, info, DIB_RGB_COLORS );
    ok( ret == 2, "got %d\n", ret );
    for (i = 0; i < 32; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    ok( !memcmp( dib_bits + 32, data,  16 * 4 ), "bits differ\n");
    for (i = 48; i < 64; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    /* t-d -> b-u */
    info->bmiHeader.biHeight = -8;
    ret = SetDIBits( hdc, dib, 0, 8, data, info, DIB_RGB_COLORS );
    ok( ret == 8, "got %d\n", ret );
    ok( !memcmp( dib_bits, inverted_data + 64,  64 * 4 ), "bits differ\n");
    memset( dib_bits, 0xaa, 64 * 4 );

    /* top of dst now lines up with -(abs(src_h) - startscan - lines) and
       we copy lines rows from the top of the src */

    ret = SetDIBits( hdc, dib, 1, 5, data, info, DIB_RGB_COLORS );
    ok( ret == 5, "got %d\n", ret );
    for (i = 0; i < 8; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    ok( !memcmp( dib_bits + 8, inverted_data + 88, 40 * 4 ), "bits differ\n");
    for (i = 48; i < 64; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    info->bmiHeader.biHeight = -16;
    ret = SetDIBits( hdc, dib, 1, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 12, "got %d\n", ret );
    ok( !memcmp( dib_bits, inverted_data + 88, 40 * 4 ), "bits differ\n");
    for (i = 40; i < 64; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBits( hdc, dib, 4, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 12, "got %d\n", ret );
    ok( !memcmp( dib_bits, inverted_data + 64, 64 * 4 ), "bits differ\n");
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBits( hdc, dib, 5, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 12, "got %d\n", ret );
    ok( !memcmp( dib_bits, inverted_data + 56, 64 * 4 ), "bits differ\n");
    memset( dib_bits, 0xaa, 64 * 4 );

    info->bmiHeader.biHeight = -5;
    ret = SetDIBits( hdc, dib, 1, 2, data, info, DIB_RGB_COLORS );
    ok( ret == 2, "got %d\n", ret );
    for (i = 0; i < 32; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    ok( !memcmp( dib_bits + 32, inverted_data + 112, 16 * 4 ), "bits differ\n");
    for (i = 48; i < 64; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    DeleteObject( dib );

    info->bmiHeader.biHeight = -8;

    dib = CreateDIBSection( NULL, info, DIB_RGB_COLORS, (void**)&dib_bits, NULL, 0 );
    memset( dib_bits, 0xaa, 16 * 16 * 4 );

    /* t-d -> t-d */

    /* like the t-d -> b-u case. */

    ret = SetDIBits( hdc, dib, 0, 8, data, info, DIB_RGB_COLORS );
    ok( ret == 8, "got %d\n", ret );
    ok( !memcmp( dib_bits, data, 64 * 4 ), "bits differ\n");
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBits( hdc, dib, 1, 5, data, info, DIB_RGB_COLORS );
    ok( ret == 5, "got %d\n", ret );
    for (i = 0; i < 16; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    ok( !memcmp( dib_bits + 16, data, 40 * 4 ), "bits differ\n");
    for (i = 56; i < 64; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    info->bmiHeader.biHeight = -16;
    ret = SetDIBits( hdc, dib, 1, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 12, "got %d\n", ret );
    for (i = 0; i < 24; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    ok( !memcmp( dib_bits + 24, data,  40 * 4 ), "bits differ\n");
    memset( dib_bits, 0xaa, 64 * 4 );

    info->bmiHeader.biHeight = -5;
    ret = SetDIBits( hdc, dib, 1, 2, data, info, DIB_RGB_COLORS );
    ok( ret == 2, "got %d\n", ret );
    for (i = 0; i < 16; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    ok( !memcmp( dib_bits + 16, data,  16 * 4 ), "bits differ\n");
    for (i = 32; i < 64; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    /* b-u -> t-d */
    /* like the b-u -> b-u case */

    info->bmiHeader.biHeight = 8;
    ret = SetDIBits( hdc, dib, 0, 8, data, info, DIB_RGB_COLORS );
    ok( ret == 8, "got %d\n", ret );
    ok( !memcmp( dib_bits, inverted_data + 64, 64 * 4 ), "bits differ\n");
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBits( hdc, dib, 1, 5, data, info, DIB_RGB_COLORS );
    ok( ret == 5, "got %d\n", ret );
    for (i = 0; i < 16; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    ok( !memcmp( dib_bits + 16, inverted_data + 88, 40 * 4 ), "bits differ\n");
    for (i = 56; i < 64; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    info->bmiHeader.biHeight = 16;
    ret = SetDIBits( hdc, dib, 1, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 12, "got %d\n", ret );
    for (i = 0; i < 24; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    ok( !memcmp( dib_bits + 24, inverted_data + 32, 40 * 4 ), "bits differ\n");
    memset( dib_bits, 0xaa, 64 * 4 );

    info->bmiHeader.biHeight = 5;
    ret = SetDIBits( hdc, dib, 1, 2, data, info, DIB_RGB_COLORS );
    ok( ret == 2, "got %d\n", ret );
    for (i = 0; i < 16; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    ok( !memcmp( dib_bits + 16, inverted_data + 112, 16 * 4 ), "bits differ\n");
    for (i = 32; i < 64; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    /* handling of partial color table */

    info->bmiHeader.biHeight   = -8;
    info->bmiHeader.biBitCount = 8;
    info->bmiHeader.biClrUsed  = 137;
    for (i = 0; i < 256; i++)
    {
        info->bmiColors[i].rgbRed      = 255 - i;
        info->bmiColors[i].rgbGreen    = i * 2;
        info->bmiColors[i].rgbBlue     = i;
        info->bmiColors[i].rgbReserved = 0;
    }
    for (i = 0; i < 64; i++) ((BYTE *)data)[i] = i * 4 + 1;
    ret = SetDIBits( hdc, dib, 0, 8, data, info, DIB_RGB_COLORS );
    ok( ret == 8, "got %d\n", ret );
    for (i = 0; i < 64; i++)
    {
        int idx = i * 4 + 1;
        DWORD expect = idx >= info->bmiHeader.biClrUsed ? 0 : (info->bmiColors[idx].rgbRed << 16 |
                                                               info->bmiColors[idx].rgbGreen << 8 |
                                                               info->bmiColors[idx].rgbBlue);
        ok( dib_bits[i] == expect, "%d: got %08x instead of %08x\n", i, dib_bits[i], expect );
    }
    memset( dib_bits, 0xaa, 64 * 4 );

    /* handling of DIB_PAL_COLORS */

    pal->palVersion = 0x300;
    pal->palNumEntries = 137;
    info->bmiHeader.biClrUsed = 221;
    for (i = 0; i < 256; i++)
    {
        palent[i].peRed   = i * 2;
        palent[i].peGreen = 255 - i;
        palent[i].peBlue  = i;
    }
    palette = CreatePalette( pal );
    ok( palette != 0, "palette creation failed\n" );
    SelectPalette( hdc, palette, FALSE );
    for (i = 0; i < 256; i++) ((WORD *)info->bmiColors)[i] = 255 - i;
    ret = SetDIBits( hdc, dib, 0, 8, data, info, DIB_PAL_COLORS );
    ok( ret == 8, "got %d\n", ret );
    for (i = 0; i < 64; i++)
    {
        int idx = i * 4 + 1;
        int ent = (255 - idx) % pal->palNumEntries;
        DWORD expect = idx >= info->bmiHeader.biClrUsed ? 0 :
                        (palent[ent].peRed << 16 | palent[ent].peGreen << 8 | palent[ent].peBlue);
        ok( dib_bits[i] == expect || broken(dib_bits[i] == 0),  /* various Windows versions get some values wrong */
            "%d: got %08x instead of %08x\n", i, dib_bits[i], expect );
    }
    memset( dib_bits, 0xaa, 64 * 4 );

    ReleaseDC( NULL, hdc );
    DeleteObject( dib );
    DeleteObject( palette );
    HeapFree( GetProcessHeap(), 0, info );
}

static void test_SetDIBits_RLE4(void)
{
    BITMAPINFO *info;
    DWORD *dib_bits;
    HDC hdc = GetDC( NULL );
    BYTE rle4_data[26] = { 0x03, 0x52, 0x07, 0x68, 0x00, 0x00,     /* 5, 2, 5, 6, 8, 6, 8, 6, (8, 6,) <eol> */
                           0x00, 0x03, 0x14, 0x50, 0x00, 0x05,
                           0x79, 0xfd, 0xb0, 0x00, 0x00, 0x00,     /* 1, 4, 5, 7, 9, f, d, b <pad> <eol> */
                           0x00, 0x02, 0x01, 0x02, 0x05, 0x87,     /* dx=1, dy=2, 8, 7, 8, 7, 8 */
                           0x00, 0x01 };                           /* <eod> */
    HBITMAP dib;
    int i, ret;
    DWORD bottom_up[64] = { 0x00050505, 0x00020202, 0x00050505, 0x00060606, 0x00080808, 0x00060606, 0x00080808, 0x00060606,
                            0x00010101, 0x00040404, 0x00050505, 0x00070707, 0x00090909, 0x000f0f0f, 0x000d0d0d, 0x000b0b0b,
                            0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa,
                            0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa,
                            0xaaaaaaaa, 0x00080808, 0x00070707, 0x00080808, 0x00070707, 0x00080808, 0xaaaaaaaa, 0xaaaaaaaa,
                            0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa,
                            0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa,
                            0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa };

    info = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, FIELD_OFFSET( BITMAPINFO, bmiColors[256] ) );

    info->bmiHeader.biSize        = sizeof(info->bmiHeader);
    info->bmiHeader.biWidth       = 8;
    info->bmiHeader.biHeight      = 8;
    info->bmiHeader.biPlanes      = 1;
    info->bmiHeader.biBitCount    = 32;
    info->bmiHeader.biCompression = BI_RGB;

    dib = CreateDIBSection( NULL, info, DIB_RGB_COLORS, (void**)&dib_bits, NULL, 0 );
    memset( dib_bits, 0xaa, 64 * 4 );

    info->bmiHeader.biBitCount    = 4;
    info->bmiHeader.biCompression = BI_RLE4;
    info->bmiHeader.biSizeImage   = sizeof(rle4_data);

    for (i = 0; i < 16; i++)
    {
        info->bmiColors[i].rgbRed      = i;
        info->bmiColors[i].rgbGreen    = i;
        info->bmiColors[i].rgbBlue     = i;
        info->bmiColors[i].rgbReserved = 0;
    }

    ret = SetDIBits( hdc, dib, 0, 8, rle4_data, info, DIB_RGB_COLORS );
    ok( ret == 8, "got %d\n", ret );
    ok( !memcmp( dib_bits, bottom_up, sizeof(bottom_up) ), "bits differ\n" );
    memset( dib_bits, 0xaa, 64 * 4 );

    DeleteObject( dib );
    ReleaseDC( NULL, hdc );
    HeapFree( GetProcessHeap(), 0, info );
}

static void test_SetDIBits_RLE8(void)
{
    BITMAPINFO *info;
    DWORD *dib_bits;
    HDC hdc = GetDC( NULL );
    BYTE rle8_data[20] = { 0x03, 0x02, 0x04, 0xf0, 0x00, 0x00,     /* 2, 2, 2, f0, f0, f0, f0, <eol> */
                           0x00, 0x03, 0x04, 0x05, 0x06, 0x00,     /* 4, 5, 6, <pad> */
                           0x00, 0x02, 0x01, 0x02, 0x05, 0x80,     /* dx=1, dy=2, 80, 80, 80, 80, (80) */
                           0x00, 0x01 };                           /* <eod> */
    HBITMAP dib;
    int i, ret;
    DWORD bottom_up[64] = { 0x00020202, 0x00020202, 0x00020202, 0x00f0f0f0, 0x00f0f0f0, 0x00f0f0f0, 0x00f0f0f0, 0xaaaaaaaa,
                            0x00040404, 0x00050505, 0x00060606, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa,
                            0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa,
                            0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0x00808080, 0x00808080, 0x00808080, 0x00808080,
                            0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa,
                            0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa,
                            0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa,
                            0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa };
    DWORD top_down[64]  = { 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa,
                            0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa,
                            0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa,
                            0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa,
                            0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0x00808080, 0x00808080, 0x00808080, 0x00808080,
                            0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa,
                            0x00040404, 0x00050505, 0x00060606, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa,
                            0x00020202, 0x00020202, 0x00020202, 0x00f0f0f0, 0x00f0f0f0, 0x00f0f0f0, 0x00f0f0f0, 0xaaaaaaaa };

    info = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, FIELD_OFFSET( BITMAPINFO, bmiColors[256] ) );

    info->bmiHeader.biSize        = sizeof(info->bmiHeader);
    info->bmiHeader.biWidth       = 8;
    info->bmiHeader.biHeight      = 8;
    info->bmiHeader.biPlanes      = 1;
    info->bmiHeader.biBitCount    = 32;
    info->bmiHeader.biCompression = BI_RGB;

    dib = CreateDIBSection( NULL, info, DIB_RGB_COLORS, (void**)&dib_bits, NULL, 0 );
    memset( dib_bits, 0xaa, 64 * 4 );

    info->bmiHeader.biBitCount    = 8;
    info->bmiHeader.biCompression = BI_RLE8;
    info->bmiHeader.biSizeImage   = sizeof(rle8_data);

    for (i = 0; i < 256; i++)
    {
        info->bmiColors[i].rgbRed      = i;
        info->bmiColors[i].rgbGreen    = i;
        info->bmiColors[i].rgbBlue     = i;
        info->bmiColors[i].rgbReserved = 0;
    }

    ret = SetDIBits( hdc, dib, 0, 8, rle8_data, info, DIB_RGB_COLORS );
    ok( ret == 8, "got %d\n", ret );
    ok( !memcmp( dib_bits, bottom_up, sizeof(bottom_up) ), "bits differ\n");
    memset( dib_bits, 0xaa, 64 * 4 );

    /* startscan and lines are ignored, unless lines == 0 */
    ret = SetDIBits( hdc, dib, 1, 8, rle8_data, info, DIB_RGB_COLORS );
    ok( ret == 8, "got %d\n", ret );
    ok( !memcmp( dib_bits, bottom_up, sizeof(bottom_up) ), "bits differ\n");
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBits( hdc, dib, 1, 1, rle8_data, info, DIB_RGB_COLORS );
    ok( ret == 8, "got %d\n", ret );
    ok( !memcmp( dib_bits, bottom_up, sizeof(bottom_up) ), "bits differ\n");
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBits( hdc, dib, 1, 0, rle8_data, info, DIB_RGB_COLORS );
    ok( ret == 0, "got %d\n", ret );
    for (i = 0; i < 64; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    /* reduce width to 4, left-hand side of dst is touched. */
    info->bmiHeader.biWidth = 4;
    ret = SetDIBits( hdc, dib, 0, 8, rle8_data, info, DIB_RGB_COLORS );
    ok( ret == 8, "got %d\n", ret );
    for (i = 0; i < 64; i++)
    {
        DWORD expect = (i & 4) ? 0xaaaaaaaa : bottom_up[i];
        ok( dib_bits[i] == expect, "%d: got %08x\n", i, dib_bits[i] );
    }
    memset( dib_bits, 0xaa, 64 * 4 );

    /* Show that the top lines are aligned by adjusting the height of the src */

    /* reduce the height to 4 -> top 4 lines of dst are touched (corresponding to last half of the bits). */
    info->bmiHeader.biWidth  = 8;
    info->bmiHeader.biHeight = 4;
    ret = SetDIBits( hdc, dib, 0, 8, rle8_data, info, DIB_RGB_COLORS );
    ok( ret == 4, "got %d\n", ret );
    for (i = 0; i < 32; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    ok( !memcmp( dib_bits + 32, bottom_up, 32 * 4 ), "bits differ\n");
    memset( dib_bits, 0xaa, 64 * 4 );

    /* increase the height to 9 -> everything moves down one row. */
    info->bmiHeader.biHeight = 9;
    ret = SetDIBits( hdc, dib, 0, 8, rle8_data, info, DIB_RGB_COLORS );
    ok( ret == 9, "got %d\n", ret );
    ok( !memcmp( dib_bits, bottom_up + 8, 56 * 4 ), "bits differ\n");
    for (i = 0; i < 8; i++) ok( dib_bits[56 + i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[56 + i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    /* top-down compressed dibs are invalid */
    info->bmiHeader.biHeight = -8;
    SetLastError( 0xdeadbeef );
    ret = SetDIBits( hdc, dib, 0, 8, rle8_data, info, DIB_RGB_COLORS );
    ok( ret == 0, "got %d\n", ret );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "got %x\n", GetLastError() );
    DeleteObject( dib );

    /* top-down dst */

    info->bmiHeader.biHeight      = -8;
    info->bmiHeader.biBitCount    = 32;
    info->bmiHeader.biCompression = BI_RGB;
    info->bmiHeader.biSizeImage   = 0;

    dib = CreateDIBSection( NULL, info, DIB_RGB_COLORS, (void**)&dib_bits, NULL, 0 );
    memset( dib_bits, 0xaa, 16 * 16 * 4 );

    info->bmiHeader.biHeight      = 8;
    info->bmiHeader.biBitCount    = 8;
    info->bmiHeader.biCompression = BI_RLE8;
    info->bmiHeader.biSizeImage   = sizeof(rle8_data);

    ret = SetDIBits( hdc, dib, 0, 8, rle8_data, info, DIB_RGB_COLORS );
    ok( ret == 8, "got %d\n", ret );
    ok( !memcmp( dib_bits, top_down, sizeof(top_down) ), "bits differ\n");
    memset( dib_bits, 0xaa, 64 * 4 );

    info->bmiHeader.biHeight = 4;
    ret = SetDIBits( hdc, dib, 0, 8, rle8_data, info, DIB_RGB_COLORS );
    ok( ret == 4, "got %d\n", ret );
    ok( !memcmp( dib_bits, top_down + 32, 32 * 4 ), "bits differ\n");
    for (i = 32; i < 64; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    info->bmiHeader.biHeight = 9;
    ret = SetDIBits( hdc, dib, 0, 8, rle8_data, info, DIB_RGB_COLORS );
    ok( ret == 9, "got %d\n", ret );
    for (i = 0; i < 8; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    ok( !memcmp( dib_bits + 8, top_down, 56 * 4 ), "bits differ\n");
    memset( dib_bits, 0xaa, 64 * 4 );

    DeleteObject( dib );
    ReleaseDC( NULL, hdc );
    HeapFree( GetProcessHeap(), 0, info );
}

static void test_SetDIBitsToDevice(void)
{
    char palbuf[sizeof(LOGPALETTE) + 256 * sizeof(PALETTEENTRY)];
    LOGPALETTE *pal = (LOGPALETTE *)palbuf;
    PALETTEENTRY *palent = pal->palPalEntry;
    HPALETTE palette;
    BITMAPINFO *info;
    DWORD *dib_bits;
    HDC hdc = CreateCompatibleDC( 0 );
    DWORD data[128], inverted_data[128];
    HBITMAP dib;
    int i, ret;

    info = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, FIELD_OFFSET( BITMAPINFO, bmiColors[256] ) );

    info->bmiHeader.biSize        = sizeof(info->bmiHeader);
    info->bmiHeader.biWidth       = 8;
    info->bmiHeader.biHeight      = 8;
    info->bmiHeader.biPlanes      = 1;
    info->bmiHeader.biBitCount    = 32;
    info->bmiHeader.biCompression = BI_RGB;

    dib = CreateDIBSection( NULL, info, DIB_RGB_COLORS, (void**)&dib_bits, NULL, 0 );
    memset( dib_bits, 0xaa, 64 * 4 );
    SelectObject( hdc, dib );

    for (i = 0; i < 128; i++)
    {
        data[i] = i;
        inverted_data[120 - (i & ~7) + (i & 7)] = i;
    }

    /* b-u -> b-u */

    ret = SetDIBitsToDevice( hdc, 0, 0, 8, 8, 0, 0, 0, 8, data, info, DIB_RGB_COLORS );
    ok( ret == 8, "got %d\n", ret );
    for (i = 0; i < 64; i++) ok( dib_bits[i] == data[i], "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBitsToDevice( hdc, 0, 0, 8, 8, 0, 0, 1, 5, data, info, DIB_RGB_COLORS );
    ok( ret == 5, "got %d\n", ret );
    for (i = 0; i < 8; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    for (i = 8; i < 48; i++) ok( dib_bits[i] == data[i - 8], "%d: got %08x\n", i, dib_bits[i] );
    for (i = 48; i < 64; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBitsToDevice( hdc, 0, 0, 8, 8, 0, 3, 1, 5, data, info, DIB_RGB_COLORS );
    ok( ret == 5, "got %d\n", ret );
    for (i = 0; i < 24; i++) ok( dib_bits[i] == data[i + 16], "%d: got %08x\n", i, dib_bits[i] );
    for (i = 24; i < 64; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    info->bmiHeader.biHeight = 16;
    ret = SetDIBitsToDevice( hdc, 0, 0, 8, 8, 0, 0, 1, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 7, "got %d\n", ret );
    for (i = 0; i < 8; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    for (i = 8; i < 64; i++) ok( dib_bits[i] == data[i - 8], "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBitsToDevice( hdc, 0, 2, 8, 8, 0, 6, 1, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 12, "got %d\n", ret );
    for (i = 0; i < 40; i++) ok( dib_bits[i] == data[i + 56], "%d: got %08x\n", i, dib_bits[i] );
    for (i = 40; i < 64; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBitsToDevice( hdc, 0, -4, 8, 8, 0, 3, 1, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 10, "got %d\n", ret );
    for (i = 0; i < 32; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    for (i = 32; i < 64; i++) ok( dib_bits[i] == data[i - 16], "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBitsToDevice( hdc, 0, 4, 8, 8, 0, -3, 1, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 4, "got %d\n", ret );
    for (i = 0; i < 32; i++) ok( dib_bits[i] == data[i], "%d: got %08x\n", i, dib_bits[i] );
    for (i = 32; i < 64; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBitsToDevice( hdc, 0, 2, 8, 5, 0, -2, 1, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 2, "got %d\n", ret );
    for (i = 0; i < 32; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    for (i = 32; i < 48; i++) ok( dib_bits[i] == data[i - 32], "%d: got %08x\n", i, dib_bits[i] );
    for (i = 48; i < 64; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    info->bmiHeader.biHeight = 5;
    ret = SetDIBitsToDevice( hdc, 0, 0, 8, 8, 0, 0, 2, 2, data, info, DIB_RGB_COLORS );
    ok( ret == 2, "got %d\n", ret );
    for (i = 0; i < 16; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    for (i = 16; i < 32; i++) ok( dib_bits[i] == data[i - 16], "%d: got %08x\n", i, dib_bits[i] );
    for (i = 32; i < 64; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBitsToDevice( hdc, 3, 3, 2, 2, 1, 2, 1, 5, data, info, DIB_RGB_COLORS );
    ok( ret == 3, "got %d\n", ret );
    for (i = 0; i < 64; i++)
        if (i == 27 || i == 28 || i == 35 || i == 36)
            ok( dib_bits[i] == data[i - 18], "%d: got %08x\n", i, dib_bits[i] );
        else
            ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBitsToDevice( hdc, 0, 0, 16, 16, 0, 0, 0, 5, data, info, DIB_RGB_COLORS );
    ok( ret == 5, "got %d\n", ret );
    for (i = 0; i < 64; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBitsToDevice( hdc, 0, 2, 8, 4, 0, -1, 3, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 0, "got %d\n", ret );
    for (i = 0; i < 64; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    SetMapMode( hdc, MM_ANISOTROPIC );
    SetWindowExtEx( hdc, 3, 3, NULL );
    ret = SetDIBitsToDevice( hdc, 2, 2, 2, 2, 1, 2, 1, 5, data, info, DIB_RGB_COLORS );
    ok( ret == 3, "got %d\n", ret );
    for (i = 0; i < 64; i++)
        if (i == 41 || i == 42 || i == 49 || i == 50)
            ok( dib_bits[i] == data[i - 32], "%d: got %08x\n", i, dib_bits[i] );
        else
            ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    SetWindowExtEx( hdc, -1, -1, NULL );
    ret = SetDIBitsToDevice( hdc, 2, 2, 4, 4, 1, 2, 1, 5, data, info, DIB_RGB_COLORS );
    ok( ret == 4, "got %d\n", ret );
    for (i = 0; i < 64; i++)
        if (i == 48 || i == 49 || i == 56 || i == 57)
            ok( dib_bits[i] == data[i - 37], "%d: got %08x\n", i, dib_bits[i] );
        else
            ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );
    SetMapMode( hdc, MM_TEXT );

    if (pSetLayout)
    {
        pSetLayout( hdc, LAYOUT_RTL );
        ret = SetDIBitsToDevice( hdc, 1, 2, 3, 2, 1, 2, 1, 5, data, info, DIB_RGB_COLORS );
        ok( ret == 3, "got %d\n", ret );
        for (i = 0; i < 64; i++)
            if (i == 36 || i == 37 || i == 38 || i == 44 || i == 45 || i == 46)
                ok( dib_bits[i] == data[i - 27], "%d: got %08x\n", i, dib_bits[i] );
            else
                ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
        memset( dib_bits, 0xaa, 64 * 4 );
        pSetLayout( hdc, LAYOUT_LTR );
    }

    /* t-d -> b-u */
    info->bmiHeader.biHeight = -8;
    ret = SetDIBitsToDevice( hdc, 0, 0, 8, 8, 0, 0, 0, 8, data, info, DIB_RGB_COLORS );
    ok( ret == 8, "got %d\n", ret );
    for (i = 0; i < 64; i++) ok( dib_bits[i] == inverted_data[i + 64], "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBitsToDevice( hdc, 0, 0, 8, 8, 0, 0, 1, 5, data, info, DIB_RGB_COLORS );
    ok( ret == 5, "got %d\n", ret );
    for (i = 0; i < 8; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    for (i = 8; i < 48; i++) ok( dib_bits[i] == inverted_data[i + 80], "%d: got %08x\n", i, dib_bits[i] );
    for (i = 48; i < 64; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBitsToDevice( hdc, 0, 0, 8, 8, 0, 4, 1, 5, data, info, DIB_RGB_COLORS );
    ok( ret == 5, "got %d\n", ret );
    for (i = 0; i < 16; i++) ok( dib_bits[i] == inverted_data[i + 112], "%d: got %08x\n", i, dib_bits[i] );
    for (i = 16; i < 64; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    info->bmiHeader.biHeight = -16;
    ret = SetDIBitsToDevice( hdc, 0, 0, 8, 8, 0, 0, 1, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 12, "got %d\n", ret );
    for (i = 0; i < 8; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    for (i = 8; i < 64; i++) ok( dib_bits[i] == inverted_data[i + 24], "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBitsToDevice( hdc, 0, 4, 8, 8, 0, 7, 1, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 12, "got %d\n", ret );
    for (i = 0; i < 16; i++) ok( dib_bits[i] == inverted_data[i + 112], "%d: got %08x\n", i, dib_bits[i] );
    for (i = 16; i < 64; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBitsToDevice( hdc, 0, 0, 8, 8, 0, 0, 4, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 12, "got %d\n", ret );
    for (i = 0; i < 32; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    for (i = 32; i < 64; i++) ok( dib_bits[i] == inverted_data[i], "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBitsToDevice( hdc, 0, -3, 8, 8, 0, 2, 4, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 12, "got %d\n", ret );
    for (i = 0; i < 40; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    for (i = 40; i < 64; i++) ok( dib_bits[i] == inverted_data[i - 8], "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBitsToDevice( hdc, 0, 3, 8, 8, 0, -2, 4, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 12, "got %d\n", ret );
    for (i = 0; i < 24; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    for (i = 24; i < 40; i++) ok( dib_bits[i] == inverted_data[i + 8], "%d: got %08x\n", i, dib_bits[i] );
    for (i = 40; i < 64; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBitsToDevice( hdc, 0, 0, 8, 8, 0, 0, 5, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 12, "got %d\n", ret );
    for (i = 0; i < 40; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    for (i = 40; i < 64; i++) ok( dib_bits[i] == inverted_data[i - 8], "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBitsToDevice( hdc, 0, 2, 8, 4, 0, -1, 3, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 12, "got %d\n", ret );
    for (i = 0; i < 64; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBitsToDevice( hdc, 5, -7, 8, 16, -2, -4, 0, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 12, "got %d\n", ret );
    for (i = 0; i < 64; i++)
        if (i == 31 || i == 39 || i == 47 || i == 55 || i == 63)
            ok( dib_bits[i] == inverted_data[i + 1], "%d: got %08x\n", i, dib_bits[i] );
        else
            ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    info->bmiHeader.biHeight = -5;
    ret = SetDIBitsToDevice( hdc, 0, 0, 8, 8, 0, 0, 1, 2, data, info, DIB_RGB_COLORS );
    ok( ret == 2, "got %d\n", ret );
    for (i = 0; i < 8; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    for (i = 8; i < 24; i++) ok( dib_bits[i] == inverted_data[i + 104], "%d: got %08x\n", i, dib_bits[i] );
    for (i = 24; i < 64; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBitsToDevice( hdc, 5, 4, 2, 2, 6, 3, 1, 5, data, info, DIB_RGB_COLORS );
    ok( ret == 5, "got %d\n", ret );
    for (i = 0; i < 64; i++)
        if (i == 21 || i == 22 || i == 29 || i == 30)
            ok( dib_bits[i] == inverted_data[i + 89], "%d: got %08x\n", i, dib_bits[i] );
        else
            ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBitsToDevice( hdc, 0, 0, 16, 16, 0, 0, 0, 5, data, info, DIB_RGB_COLORS );
    ok( ret == 5, "got %d\n", ret );
    for (i = 0; i < 64; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    info->bmiHeader.biHeight = -8;

    dib = CreateDIBSection( NULL, info, DIB_RGB_COLORS, (void**)&dib_bits, NULL, 0 );
    DeleteObject( SelectObject( hdc, dib ));
    memset( dib_bits, 0xaa, 16 * 16 * 4 );

    /* t-d -> t-d */

    ret = SetDIBitsToDevice( hdc, 0, 0, 8, 8, 0, 0, 0, 8, data, info, DIB_RGB_COLORS );
    ok( ret == 8, "got %d\n", ret );
    for (i = 0; i < 64; i++) ok( dib_bits[i] == data[i], "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBitsToDevice( hdc, 0, 0, 8, 8, 0, 0, 1, 5, data, info, DIB_RGB_COLORS );
    ok( ret == 5, "got %d\n", ret );
    for (i = 0; i < 16; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    for (i = 16; i < 56; i++) ok( dib_bits[i] == data[i - 16], "%d: got %08x\n", i, dib_bits[i] );
    for (i = 56; i < 64; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBitsToDevice( hdc, 0, 3, 8, 3, 0, 2, 1, 5, data, info, DIB_RGB_COLORS );
    ok( ret == 5, "got %d\n", ret );
    for (i = 0; i < 24; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    for (i = 24; i < 48; i++) ok( dib_bits[i] == data[i - 16], "%d: got %08x\n", i, dib_bits[i] );
    for (i = 48; i < 64; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    info->bmiHeader.biHeight = -16;
    ret = SetDIBitsToDevice( hdc, 0, 0, 8, 8, 0, 0, 1, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 12, "got %d\n", ret );
    for (i = 0; i < 56; i++) ok( dib_bits[i] == data[i + 40], "%d: got %08x\n", i, dib_bits[i] );
    for (i = 56; i < 64; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBitsToDevice( hdc, 5, -7, 8, 16, -1, -8, 0, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 12, "got %d\n", ret );
    for (i = 0; i < 64; i++)
        if (i == 6 || i == 7)
            ok( dib_bits[i] == data[i + 82], "%d: got %08x\n", i, dib_bits[i] );
        else
            ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    info->bmiHeader.biHeight = -5;
    ret = SetDIBitsToDevice( hdc, 0, 0, 8, 8, 0, 0, 1, 2, data, info, DIB_RGB_COLORS );
    ok( ret == 2, "got %d\n", ret );
    for (i = 0; i < 40; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    for (i = 40; i < 56; i++) ok( dib_bits[i] == data[i - 40], "%d: got %08x\n", i, dib_bits[i] );
    for (i = 56; i < 64; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBitsToDevice( hdc, 7, 2, 8, 8, 1, 0, 0, 5, data, info, DIB_RGB_COLORS );
    ok( ret == 5, "got %d\n", ret );
    for (i = 0; i < 64; i++)
        if (i == 47 || i == 55 || i == 63)
            ok( dib_bits[i] == data[i - 46], "%d: got %08x\n", i, dib_bits[i] );
        else
            ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBitsToDevice( hdc, 0, 0, 16, 16, 0, 0, 0, 5, data, info, DIB_RGB_COLORS );
    ok( ret == 5, "got %d\n", ret );
    for (i = 0; i < 64; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    /* b-u -> t-d */

    info->bmiHeader.biHeight = 8;
    ret = SetDIBitsToDevice( hdc, 0, 0, 8, 8, 0, 0, 0, 8, data, info, DIB_RGB_COLORS );
    ok( ret == 8, "got %d\n", ret );
    for (i = 0; i < 64; i++) ok( dib_bits[i] == inverted_data[i + 64], "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBitsToDevice( hdc, 0, 0, 8, 8, 0, 0, 1, 5, data, info, DIB_RGB_COLORS );
    ok( ret == 5, "got %d\n", ret );
    for (i = 0; i < 16; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    for (i = 16; i < 56; i++) ok( dib_bits[i] == inverted_data[i + 72], "%d: got %08x\n", i, dib_bits[i] );
    for (i = 56; i < 64; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    info->bmiHeader.biHeight = 16;
    ret = SetDIBitsToDevice( hdc, 0, 0, 8, 8, 0, 0, 1, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 7, "got %d\n", ret );
    for (i = 0; i < 56; i++) ok( dib_bits[i] == inverted_data[i + 72], "%d: got %08x\n", i, dib_bits[i] );
    for (i = 56; i < 64; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBitsToDevice( hdc, 4, 4, 8, 8, 0, -4, 1, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 3, "got %d\n", ret );
    for (i = 0; i < 64; i++)
        if ((i >= 36 && i <= 39) || (i >= 44 && i <= 47) || (i >= 52 && i <= 55))
            ok( dib_bits[i] == inverted_data[i + 68], "%d: got %08x\n", i, dib_bits[i] );
        else
            ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBitsToDevice( hdc, 4, 4, 8, 8, -30, -30, 1, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 0, "got %d\n", ret );
    for (i = 0; i < 64; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBitsToDevice( hdc, 5, -5, 8, 16, -2, -4, 4, 12, data, info, DIB_RGB_COLORS );
    ok( ret == 8, "got %d\n", ret );
    for (i = 0; i < 64; i++)
        if (i == 7 || i == 15 || i == 23)
            ok( dib_bits[i] == inverted_data[i + 97], "%d: got %08x\n", i, dib_bits[i] );
        else
            ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    info->bmiHeader.biHeight = 5;
    ret = SetDIBitsToDevice( hdc, 0, 0, 8, 8, 0, 0, 1, 2, data, info, DIB_RGB_COLORS );
    ok( ret == 2, "got %d\n", ret );
    for (i = 0; i < 40; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    for (i = 40; i < 56; i++) ok( dib_bits[i] == inverted_data[i + 72], "%d: got %08x\n", i, dib_bits[i] );
    for (i = 56; i < 64; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBitsToDevice( hdc, 0, 0, 16, 16, 0, 0, 0, 5, data, info, DIB_RGB_COLORS );
    ok( ret == 5, "got %d\n", ret );
    for (i = 0; i < 64; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    /* handling of partial color table */

    info->bmiHeader.biHeight   = -8;
    info->bmiHeader.biBitCount = 8;
    info->bmiHeader.biClrUsed  = 137;
    for (i = 0; i < 256; i++)
    {
        info->bmiColors[i].rgbRed      = 255 - i;
        info->bmiColors[i].rgbGreen    = i * 2;
        info->bmiColors[i].rgbBlue     = i;
        info->bmiColors[i].rgbReserved = 0;
    }
    for (i = 0; i < 64; i++) ((BYTE *)data)[i] = i * 4 + 1;
    ret = SetDIBitsToDevice( hdc, 0, 0, 8, 8, 0, 0, 0, 8, data, info, DIB_RGB_COLORS );
    ok( ret == 8, "got %d\n", ret );
    for (i = 0; i < 64; i++)
    {
        int idx = i * 4 + 1;
        DWORD expect = idx >= info->bmiHeader.biClrUsed ? 0 : (info->bmiColors[idx].rgbRed << 16 |
                                                               info->bmiColors[idx].rgbGreen << 8 |
                                                               info->bmiColors[idx].rgbBlue);
        ok( dib_bits[i] == expect, "%d: got %08x instead of %08x\n", i, dib_bits[i], expect );
    }
    memset( dib_bits, 0xaa, 64 * 4 );

    /* handling of DIB_PAL_COLORS */

    pal->palVersion = 0x300;
    pal->palNumEntries = 137;
    info->bmiHeader.biClrUsed = 221;
    for (i = 0; i < 256; i++)
    {
        palent[i].peRed   = i * 2;
        palent[i].peGreen = 255 - i;
        palent[i].peBlue  = i;
    }
    palette = CreatePalette( pal );
    ok( palette != 0, "palette creation failed\n" );
    SelectPalette( hdc, palette, FALSE );
    for (i = 0; i < 256; i++) ((WORD *)info->bmiColors)[i] = 255 - i;
    ret = SetDIBitsToDevice( hdc, 0, 0, 8, 8, 0, 0, 0, 8, data, info, DIB_PAL_COLORS );
    ok( ret == 8, "got %d\n", ret );
    for (i = 0; i < 64; i++)
    {
        int idx = i * 4 + 1;
        int ent = (255 - idx) % pal->palNumEntries;
        DWORD expect = idx >= info->bmiHeader.biClrUsed ? 0 :
                        (palent[ent].peRed << 16 | palent[ent].peGreen << 8 | palent[ent].peBlue);
        ok( dib_bits[i] == expect || broken(dib_bits[i] == 0),
            "%d: got %08x instead of %08x\n", i, dib_bits[i], expect );
    }
    memset( dib_bits, 0xaa, 64 * 4 );

    DeleteDC( hdc );
    DeleteObject( dib );
    DeleteObject( palette );
    HeapFree( GetProcessHeap(), 0, info );
}

static void test_SetDIBitsToDevice_RLE8(void)
{
    BITMAPINFO *info;
    DWORD *dib_bits;
    HDC hdc = CreateCompatibleDC( 0 );
    BYTE rle8_data[20] = { 0x04, 0x02, 0x03, 0xf0, 0x00, 0x00,     /* 2, 2, 2, 2, f0, f0, f0, <eol> */
                           0x00, 0x03, 0x04, 0x05, 0x06, 0x00,     /* 4, 5, 6, <pad> */
                           0x00, 0x02, 0x01, 0x02, 0x05, 0x80,     /* dx=1, dy=2, 80, 80, 80, 80, (80) */
                           0x00, 0x01 };                           /* <eod> */
    HBITMAP dib;
    int i, ret;
    DWORD bottom_up[64] = { 0x00020202, 0x00020202, 0x00020202, 0x00020202, 0x00f0f0f0, 0x00f0f0f0, 0x00f0f0f0, 0xaaaaaaaa,
                            0x00040404, 0x00050505, 0x00060606, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa,
                            0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa,
                            0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0x00808080, 0x00808080, 0x00808080, 0x00808080,
                            0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa,
                            0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa,
                            0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa,
                            0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa };
    DWORD top_down[64]  = { 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa,
                            0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa,
                            0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa,
                            0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa,
                            0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0x00808080, 0x00808080, 0x00808080, 0x00808080,
                            0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa,
                            0x00040404, 0x00050505, 0x00060606, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa,
                            0x00020202, 0x00020202, 0x00020202, 0x00020202, 0x00f0f0f0, 0x00f0f0f0, 0x00f0f0f0, 0xaaaaaaaa };

    info = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, FIELD_OFFSET( BITMAPINFO, bmiColors[256] ) );

    info->bmiHeader.biSize        = sizeof(info->bmiHeader);
    info->bmiHeader.biWidth       = 8;
    info->bmiHeader.biHeight      = 8;
    info->bmiHeader.biPlanes      = 1;
    info->bmiHeader.biBitCount    = 32;
    info->bmiHeader.biCompression = BI_RGB;

    dib = CreateDIBSection( NULL, info, DIB_RGB_COLORS, (void**)&dib_bits, NULL, 0 );
    memset( dib_bits, 0xaa, 64 * 4 );
    SelectObject( hdc, dib );

    info->bmiHeader.biBitCount    = 8;
    info->bmiHeader.biCompression = BI_RLE8;
    info->bmiHeader.biSizeImage   = sizeof(rle8_data);

    for (i = 0; i < 256; i++)
    {
        info->bmiColors[i].rgbRed      = i;
        info->bmiColors[i].rgbGreen    = i;
        info->bmiColors[i].rgbBlue     = i;
        info->bmiColors[i].rgbReserved = 0;
    }

    ret = SetDIBitsToDevice( hdc, 0, 0, 8, 8, 0, 0, 0, 8, rle8_data, info, DIB_RGB_COLORS );
    ok( ret == 8, "got %d\n", ret );
    for (i = 0; i < 64; i++) ok( dib_bits[i] == bottom_up[i], "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    /* startscan and lines are ignored, unless lines == 0 */
    ret = SetDIBitsToDevice( hdc, 0, 0, 8, 8, 0, 0, 1, 8, rle8_data, info, DIB_RGB_COLORS );
    ok( ret == 8, "got %d\n", ret );
    for (i = 0; i < 64; i++) ok( dib_bits[i] == bottom_up[i], "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBitsToDevice( hdc, 0, 0, 8, 8, 0, 0, 1, 1, rle8_data, info, DIB_RGB_COLORS );
    ok( ret == 8, "got %d\n", ret );
    for (i = 0; i < 64; i++) ok( dib_bits[i] == bottom_up[i], "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBitsToDevice( hdc, 0, 0, 8, 8, 0, 0, 1, 0, rle8_data, info, DIB_RGB_COLORS );
    ok( ret == 0, "got %d\n", ret );
    for (i = 0; i < 64; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    info->bmiHeader.biWidth = 2;
    ret = SetDIBitsToDevice( hdc, 0, 0, 8, 8, 0, 0, 0, 8, rle8_data, info, DIB_RGB_COLORS );
    ok( ret == 8, "got %d\n", ret );
    for (i = 0; i < 64; i++) ok( dib_bits[i] == bottom_up[i], "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    info->bmiHeader.biWidth  = 8;
    info->bmiHeader.biHeight = 2;
    ret = SetDIBitsToDevice( hdc, 0, 0, 8, 8, 0, 0, 0, 8, rle8_data, info, DIB_RGB_COLORS );
    ok( ret == 2, "got %d\n", ret );
    for (i = 0; i < 64; i++) ok( dib_bits[i] == bottom_up[i], "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    info->bmiHeader.biHeight = 9;
    ret = SetDIBitsToDevice( hdc, 0, 0, 8, 8, 0, 0, 0, 8, rle8_data, info, DIB_RGB_COLORS );
    ok( ret == 9, "got %d\n", ret );
    for (i = 0; i < 64; i++) ok( dib_bits[i] == bottom_up[i], "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBitsToDevice( hdc, 0, 0, 8, 8, 0, 0, 0, 9, rle8_data, info, DIB_RGB_COLORS );
    ok( ret == 9, "got %d\n", ret );
    for (i = 0; i < 64; i++) ok( dib_bits[i] == bottom_up[i], "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    info->bmiHeader.biHeight = 8;
    ret = SetDIBitsToDevice( hdc, 0, 0, 8, 8, 0, 0, 1, 9, rle8_data, info, DIB_RGB_COLORS );
    ok( ret == 8, "got %d\n", ret );
    for (i = 0; i < 64; i++) ok( dib_bits[i] == bottom_up[i], "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBitsToDevice( hdc, 0, 3, 8, 8, 0, 0, 0, 8, rle8_data, info, DIB_RGB_COLORS );
    ok( ret == 8, "got %d\n", ret );
    for (i = 0; i < 40; i++) ok( dib_bits[i] == bottom_up[i + 24], "%d: got %08x\n", i, dib_bits[i] );
    for (i = 40; i < 64; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBitsToDevice( hdc, 0, 3, 4, 4, 0, 0, 0, 8, rle8_data, info, DIB_RGB_COLORS );
    ok( ret == 8, "got %d\n", ret );
    for (i = 0; i < 8; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    for (i = 8; i < 40; i++)
        if (i & 4) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
        else ok( dib_bits[i] == bottom_up[i - 8], "%d: got %08x\n", i, dib_bits[i] );
    for (i = 40; i < 64; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBitsToDevice( hdc, 3, 3, 8, 4, 0, 0, 0, 8, rle8_data, info, DIB_RGB_COLORS );
    ok( ret == 8, "got %d\n", ret );
    for (i = 0; i < 8; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    for (i = 8; i < 40; i++)
        if ((i & 7) < 3) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
        else ok( dib_bits[i] == bottom_up[i - 11], "%d: got %08x\n", i, dib_bits[i] );
    for (i = 40; i < 64; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBitsToDevice( hdc, 2, 3, 8, 4, 2, 0, 0, 8, rle8_data, info, DIB_RGB_COLORS );
    ok( ret == 8, "got %d\n", ret );
    for (i = 0; i < 8; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    for (i = 8; i < 40; i++)
        if ((i & 7) < 2) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
        else ok( dib_bits[i] == bottom_up[i - 8], "%d: got %08x\n", i, dib_bits[i] );
    for (i = 40; i < 64; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    info->bmiHeader.biWidth = 37;
    info->bmiHeader.biHeight = 37;
    ret = SetDIBitsToDevice( hdc, -2, 1, 10, 5, 2, -1, 12, 24, rle8_data, info, DIB_RGB_COLORS );
    ok( ret == 37, "got %d\n", ret );
    for (i = 0; i < 24; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    for (i = 24; i < 64; i++)
        if (i == 52) ok( dib_bits[i] == 0x00808080, "%d: got %08x\n", i, dib_bits[i] );
        else if (i & 4) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
        else ok( dib_bits[i] == bottom_up[i - 20], "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    /* top-down compressed dibs are invalid */
    info->bmiHeader.biWidth = 8;
    info->bmiHeader.biHeight = -8;
    SetLastError( 0xdeadbeef );
    ret = SetDIBitsToDevice( hdc, 0, 0, 8, 8, 0, 0, 0, 8, rle8_data, info, DIB_RGB_COLORS );
    ok( ret == 0, "got %d\n", ret );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "got %x\n", GetLastError() );

    /* top-down dst */

    info->bmiHeader.biHeight      = -8;
    info->bmiHeader.biBitCount    = 32;
    info->bmiHeader.biCompression = BI_RGB;
    info->bmiHeader.biSizeImage   = 0;

    dib = CreateDIBSection( NULL, info, DIB_RGB_COLORS, (void**)&dib_bits, NULL, 0 );
    memset( dib_bits, 0xaa, 16 * 16 * 4 );
    DeleteObject( SelectObject( hdc, dib ));

    info->bmiHeader.biHeight      = 8;
    info->bmiHeader.biBitCount    = 8;
    info->bmiHeader.biCompression = BI_RLE8;
    info->bmiHeader.biSizeImage   = sizeof(rle8_data);

    ret = SetDIBitsToDevice( hdc, 0, 0, 8, 8, 0, 0, 0, 8, rle8_data, info, DIB_RGB_COLORS );
    ok( ret == 8, "got %d\n", ret );
    for (i = 0; i < 64; i++) ok( dib_bits[i] == top_down[i], "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBitsToDevice( hdc, 0, 0, 8, 8, 0, 0, 0, 9, rle8_data, info, DIB_RGB_COLORS );
    ok( ret == 8, "got %d\n", ret );
    for (i = 0; i < 64; i++) ok( dib_bits[i] == top_down[i], "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    info->bmiHeader.biHeight = 4;
    ret = SetDIBitsToDevice( hdc, 0, 0, 8, 8, 0, 0, 0, 8, rle8_data, info, DIB_RGB_COLORS );
    ok( ret == 4, "got %d\n", ret );
    for (i = 0; i < 64; i++) ok( dib_bits[i] == top_down[i], "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    info->bmiHeader.biHeight = 9;
    ret = SetDIBitsToDevice( hdc, 0, 0, 8, 8, 0, 0, 0, 8, rle8_data, info, DIB_RGB_COLORS );
    ok( ret == 9, "got %d\n", ret );
    for (i = 0; i < 64; i++) ok( dib_bits[i] == top_down[i], "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBitsToDevice( hdc, 0, 0, 8, 8, 0, 0, 0, 9, rle8_data, info, DIB_RGB_COLORS );
    ok( ret == 9, "got %d\n", ret );
    for (i = 0; i < 64; i++) ok( dib_bits[i] == top_down[i], "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    ret = SetDIBitsToDevice( hdc, 2, 3, 8, 6, 2, 2, 0, 8, rle8_data, info, DIB_RGB_COLORS );
    ok( ret == 9, "got %d\n", ret );
    for (i = 0; i < 24; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    for (i = 24; i < 64; i++) ok( dib_bits[i] == top_down[i - 24], "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    info->bmiHeader.biWidth = 37;
    info->bmiHeader.biHeight = 37;
    ret = SetDIBitsToDevice( hdc, -2, 1, 10, 5, 2, -1, 12, 24, rle8_data, info, DIB_RGB_COLORS );
    ok( ret == 37, "got %d\n", ret );
    for (i = 0; i < 40; i++)
        if (i == 12) ok( dib_bits[i] == 0x00808080, "%d: got %08x\n", i, dib_bits[i] );
        else if (i & 4) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
        else ok( dib_bits[i] == top_down[i + 28], "%d: got %08x\n", i, dib_bits[i] );
    for (i = 40; i < 64; i++) ok( dib_bits[i] == 0xaaaaaaaa, "%d: got %08x\n", i, dib_bits[i] );
    memset( dib_bits, 0xaa, 64 * 4 );

    DeleteDC( hdc );
    DeleteObject( dib );
    HeapFree( GetProcessHeap(), 0, info );
}

START_TEST(bitmap)
{
    HMODULE hdll;

    hdll = GetModuleHandleA("gdi32.dll");
    pGdiAlphaBlend   = (void*)GetProcAddress(hdll, "GdiAlphaBlend");
    pGdiGradientFill = (void*)GetProcAddress(hdll, "GdiGradientFill");
    pSetLayout       = (void*)GetProcAddress(hdll, "SetLayout");

    test_createdibitmap();
    test_dibsections();
    test_dib_formats();
    test_mono_dibsection();
    test_bitmap();
    test_mono_bitmap();
    test_bmBits();
    test_GetDIBits_selected_DIB(1);
    test_GetDIBits_selected_DIB(4);
    test_GetDIBits_selected_DIB(8);
    test_GetDIBits_selected_DDB(TRUE);
    test_GetDIBits_selected_DDB(FALSE);
    test_GetDIBits();
    test_GetDIBits_BI_BITFIELDS();
    test_select_object();
    test_CreateBitmap();
    test_BitBlt();
    test_StretchBlt();
    test_StretchDIBits();
    test_GdiAlphaBlend();
    test_GdiGradientFill();
    test_32bit_ddb();
    test_bitmapinfoheadersize();
    test_get16dibits();
    test_clipping();
    test_GetDIBits_top_down(16);
    test_GetDIBits_top_down(24);
    test_GetDIBits_top_down(32);
    test_GetSetDIBits_rtl();
    test_GetDIBits_scanlines();
    test_SetDIBits();
    test_SetDIBits_RLE4();
    test_SetDIBits_RLE8();
    test_SetDIBitsToDevice();
    test_SetDIBitsToDevice_RLE8();
}
