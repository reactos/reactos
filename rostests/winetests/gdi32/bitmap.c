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

#define expect_eq(expr, value, type, format) { type ret = (expr); ok((value) == ret, #expr " expected " format " got " format "\n", value, ret); }

static BOOL is_win9x;

static INT BITMAP_GetWidthBytes( INT bmWidth, INT bpp )
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
        trace("Unknown depth %d, please report.\n", bpp );
        assert(0);
    }
    return -1;
}

static void test_bitmap_info(HBITMAP hbm, INT expected_depth, const BITMAPINFOHEADER *bmih)
{
    BITMAP bm;
    BITMAP bma[2];
    INT ret, width_bytes;
    BYTE buf[512], buf_cmp[512];
    DWORD gle;

    ret = GetObject(hbm, sizeof(bm), &bm);
    ok(ret == sizeof(bm), "GetObject returned %d\n", ret);

    ok(bm.bmType == 0 || broken(bm.bmType == 21072 /* Win9x */), "wrong bm.bmType %d\n", bm.bmType);
    ok(bm.bmWidth == bmih->biWidth, "wrong bm.bmWidth %d\n", bm.bmWidth);
    ok(bm.bmHeight == bmih->biHeight, "wrong bm.bmHeight %d\n", bm.bmHeight);
    width_bytes = BITMAP_GetWidthBytes(bm.bmWidth, bm.bmBitsPixel);
    ok(bm.bmWidthBytes == width_bytes, "wrong bm.bmWidthBytes %d != %d\n", bm.bmWidthBytes, width_bytes);
    ok(bm.bmPlanes == bmih->biPlanes, "wrong bm.bmPlanes %d\n", bm.bmPlanes);
    ok(bm.bmBitsPixel == expected_depth, "wrong bm.bmBitsPixel %d != %d\n", bm.bmBitsPixel, expected_depth);
    ok(bm.bmBits == NULL, "wrong bm.bmBits %p\n", bm.bmBits);

    assert(sizeof(buf) >= bm.bmWidthBytes * bm.bmHeight);
    assert(sizeof(buf) == sizeof(buf_cmp));

    SetLastError(0xdeadbeef);
    ret = GetBitmapBits(hbm, 0, NULL);
    gle=GetLastError();
    ok(ret == bm.bmWidthBytes * bm.bmHeight || (ret == 0 && gle == ERROR_INVALID_PARAMETER /* Win9x */), "%d != %d\n", ret, bm.bmWidthBytes * bm.bmHeight);

    memset(buf_cmp, 0xAA, sizeof(buf_cmp));
    memset(buf_cmp, 0, bm.bmWidthBytes * bm.bmHeight);

    memset(buf, 0xAA, sizeof(buf));
    ret = GetBitmapBits(hbm, sizeof(buf), buf);
    ok(ret == bm.bmWidthBytes * bm.bmHeight, "%d != %d\n", ret, bm.bmWidthBytes * bm.bmHeight);
    ok(!memcmp(buf, buf_cmp, sizeof(buf)) ||
       broken(memcmp(buf, buf_cmp, sizeof(buf))), /* win9x doesn't init the bitmap bits */
        "buffers do not match, depth %d\n", bmih->biBitCount);

    /* test various buffer sizes for GetObject */
    ret = GetObject(hbm, sizeof(*bma) * 2, bma);
    ok(ret == sizeof(*bma) || broken(ret == sizeof(*bma) * 2 /* Win9x */), "wrong size %d\n", ret);

    ret = GetObject(hbm, sizeof(bm) / 2, &bm);
    ok(ret == 0 || broken(ret == sizeof(bm) / 2 /* Win9x */), "%d != 0\n", ret);

    ret = GetObject(hbm, 0, &bm);
    ok(ret == 0, "%d != 0\n", ret);

    ret = GetObject(hbm, 1, &bm);
    ok(ret == 0 || broken(ret == 1 /* Win9x */), "%d != 0\n", ret);

    /* Don't trust Win9x not to try to write to NULL */
    if (ret == 0)
    {
        ret = GetObject(hbm, 0, NULL);
        ok(ret == sizeof(bm), "wrong size %d\n", ret);
    }
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

    /* If hdc == 0 then we get a 1 bpp bitmap */
    if (!is_win9x) {
        bmih.biBitCount = 32;
        hbm = CreateDIBitmap(0, &bmih, 0, NULL, NULL, 0);
        ok(hbm != NULL, "CreateDIBitmap failed\n");
        test_bitmap_info(hbm, 1, &bmih);
        DeleteObject(hbm);
    }

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

static INT DIB_GetWidthBytes( int width, int bpp )
{
    int words;

    switch (bpp)
    {
	case 1:  words = (width + 31) / 32; break;
	case 4:  words = (width + 7) / 8; break;
	case 8:  words = (width + 3) / 4; break;
	case 15:
	case 16: words = (width + 1) / 2; break;
	case 24: words = (width * 3 + 3)/4; break;
	case 32: words = width; break;

        default:
            words=0;
            trace("Unknown depth %d, please report.\n", bpp );
            assert(0);
            break;
    }
    return 4 * words;
}

static void test_dib_info(HBITMAP hbm, const void *bits, const BITMAPINFOHEADER *bmih)
{
    BITMAP bm;
    BITMAP bma[2];
    DIBSECTION ds;
    DIBSECTION dsa[2];
    INT ret, bm_width_bytes, dib_width_bytes;
    BYTE *buf;

    ret = GetObject(hbm, sizeof(bm), &bm);
    ok(ret == sizeof(bm), "GetObject returned %d\n", ret);

    ok(bm.bmType == 0, "wrong bm.bmType %d\n", bm.bmType);
    ok(bm.bmWidth == bmih->biWidth, "wrong bm.bmWidth %d\n", bm.bmWidth);
    ok(bm.bmHeight == bmih->biHeight, "wrong bm.bmHeight %d\n", bm.bmHeight);
    dib_width_bytes = DIB_GetWidthBytes(bm.bmWidth, bm.bmBitsPixel);
    bm_width_bytes = BITMAP_GetWidthBytes(bm.bmWidth, bm.bmBitsPixel);
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
    ok(ret == bm_width_bytes * bm.bmHeight ||
        broken(ret == 0 && GetLastError() == ERROR_INVALID_PARAMETER), /* Win9x */
        "%d != %d\n", ret, bm_width_bytes * bm.bmHeight);

    memset(buf, 0xAA, bm.bmWidthBytes * bm.bmHeight + 4096);
    ret = GetBitmapBits(hbm, bm.bmWidthBytes * bm.bmHeight + 4096, buf);
    ok(ret == bm_width_bytes * bm.bmHeight, "%d != %d\n", ret, bm_width_bytes * bm.bmHeight);

    HeapFree(GetProcessHeap(), 0, buf);

    /* test various buffer sizes for GetObject */
    memset(&ds, 0xAA, sizeof(ds));
    ret = GetObject(hbm, sizeof(*bma) * 2, bma);
    ok(ret == sizeof(*bma) || broken(ret == sizeof(*bma) * 2 /* Win9x */), "wrong size %d\n", ret);
    ok(bm.bmWidth == bmih->biWidth, "wrong bm.bmWidth %d\n", bm.bmWidth);
    ok(bm.bmHeight == bmih->biHeight, "wrong bm.bmHeight %d\n", bm.bmHeight);
    ok(bm.bmBits == bits, "wrong bm.bmBits %p != %p\n", bm.bmBits, bits);

    ret = GetObject(hbm, sizeof(bm) / 2, &bm);
    ok(ret == 0 || broken(ret == sizeof(bm) / 2 /* Win9x */), "%d != 0\n", ret);

    ret = GetObject(hbm, 0, &bm);
    ok(ret == 0, "%d != 0\n", ret);

    ret = GetObject(hbm, 1, &bm);
    ok(ret == 0 || broken(ret ==  1 /* Win9x */), "%d != 0\n", ret);

    /* test various buffer sizes for GetObject */
    ret = GetObject(hbm, 0, NULL);
    ok(ret == sizeof(bm) || broken(ret == sizeof(DIBSECTION) /* Win9x */), "wrong size %d\n", ret);

    ret = GetObject(hbm, sizeof(*dsa) * 2, dsa);
    ok(ret == sizeof(*dsa) || broken(ret == sizeof(*dsa) * 2 /* Win9x */), "wrong size %d\n", ret);

    memset(&ds, 0xAA, sizeof(ds));
    ret = GetObject(hbm, sizeof(ds), &ds);
    ok(ret == sizeof(ds), "wrong size %d\n", ret);

    ok(ds.dsBm.bmBits == bits, "wrong bm.bmBits %p != %p\n", ds.dsBm.bmBits, bits);
    if (ds.dsBm.bmWidthBytes != bm_width_bytes) /* Win2k bug */
        ok(ds.dsBmih.biSizeImage == ds.dsBm.bmWidthBytes * ds.dsBm.bmHeight, "%u != %u\n",
           ds.dsBmih.biSizeImage, ds.dsBm.bmWidthBytes * ds.dsBm.bmHeight);
    ok(bmih->biSizeImage == 0, "%u != 0\n", bmih->biSizeImage);
    ds.dsBmih.biSizeImage = 0;

    ok(ds.dsBmih.biSize == bmih->biSize, "%u != %u\n", ds.dsBmih.biSize, bmih->biSize);
    ok(ds.dsBmih.biWidth == bmih->biWidth, "%u != %u\n", ds.dsBmih.biWidth, bmih->biWidth);
    ok(ds.dsBmih.biHeight == bmih->biHeight, "%u != %u\n", ds.dsBmih.biHeight, bmih->biHeight);
    ok(ds.dsBmih.biPlanes == bmih->biPlanes, "%u != %u\n", ds.dsBmih.biPlanes, bmih->biPlanes);
    ok(ds.dsBmih.biBitCount == bmih->biBitCount, "%u != %u\n", ds.dsBmih.biBitCount, bmih->biBitCount);
    ok(ds.dsBmih.biCompression == bmih->biCompression, "%u != %u\n", ds.dsBmih.biCompression, bmih->biCompression);
    ok(ds.dsBmih.biSizeImage == bmih->biSizeImage, "%u != %u\n", ds.dsBmih.biSizeImage, bmih->biSizeImage);
    ok(ds.dsBmih.biXPelsPerMeter == bmih->biXPelsPerMeter, "%u != %u\n", ds.dsBmih.biXPelsPerMeter, bmih->biXPelsPerMeter);
    ok(ds.dsBmih.biYPelsPerMeter == bmih->biYPelsPerMeter, "%u != %u\n", ds.dsBmih.biYPelsPerMeter, bmih->biYPelsPerMeter);

    memset(&ds, 0xAA, sizeof(ds));
    ret = GetObject(hbm, sizeof(ds) - 4, &ds);
    ok(ret == sizeof(ds.dsBm) || broken(ret == (sizeof(ds) - 4) /* Win9x */), "wrong size %d\n", ret);
    ok(ds.dsBm.bmWidth == bmih->biWidth, "%u != %u\n", ds.dsBmih.biWidth, bmih->biWidth);
    ok(ds.dsBm.bmHeight == bmih->biHeight, "%u != %u\n", ds.dsBmih.biHeight, bmih->biHeight);
    ok(ds.dsBm.bmBits == bits, "%p != %p\n", ds.dsBm.bmBits, bits);

    ret = GetObject(hbm, 0, &ds);
    ok(ret == 0, "%d != 0\n", ret);

    ret = GetObject(hbm, 1, &ds);
    ok(ret == 0 || broken(ret == 1 /* Win9x */), "%d != 0\n", ret);
}

#define test_color_todo(got, exp, txt, todo) \
    if (!todo && got != exp && screen_depth < 24) { \
      todo_wine ok(0, #txt " failed at %d-bit screen depth: got 0x%06x expected 0x%06x - skipping DIB tests\n", \
                   screen_depth, (UINT)got, (UINT)exp); \
      return; \
    } else if (todo) todo_wine { ok(got == exp, #txt " failed: got 0x%06x expected 0x%06x\n", (UINT)got, (UINT)exp); } \
    else ok(got == exp, #txt " failed: got 0x%06x expected 0x%06x\n", (UINT)got, (UINT)exp) \

#define test_color(hdc, color, exp, todo_setp, todo_getp) \
{ \
    COLORREF c; \
    c = SetPixel(hdc, 0, 0, color); \
    if (!is_win9x) { test_color_todo(c, exp, SetPixel, todo_setp); } \
    c = GetPixel(hdc, 0, 0); \
    test_color_todo(c, exp, GetPixel, todo_getp); \
}

static void test_dib_bits_access( HBITMAP hdib, void *bits )
{
    MEMORY_BASIC_INFORMATION info;
    char bmibuf[sizeof(BITMAPINFO) + 256 * sizeof(RGBQUAD)];
    DWORD data[256];
    BITMAPINFO *pbmi = (BITMAPINFO *)bmibuf;
    HDC hdc = GetDC(0);
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

    ret = SetDIBits( hdc, hdib, 0, 16, data, pbmi, DIB_RGB_COLORS );
    ok(ret == 16 ||
       broken(ret == 0), /* win9x */
       "SetDIBits failed: expected 16 got %d\n", ret);

    ok(VirtualQuery(bits, &info, sizeof(info)) == sizeof(info),
        "VirtualQuery failed\n");
    ok(info.BaseAddress == bits, "%p != %p\n", info.BaseAddress, bits);
    ok(info.AllocationBase == bits, "%p != %p\n", info.AllocationBase, bits);
    ok(info.AllocationProtect == PAGE_READWRITE, "%x != PAGE_READWRITE\n", info.AllocationProtect);
    ok(info.State == MEM_COMMIT, "%x != MEM_COMMIT\n", info.State);
    ok(info.Type == MEM_PRIVATE, "%x != MEM_PRIVATE\n", info.Type);
    /* it has been protected now */
    todo_wine ok(info.Protect == PAGE_READWRITE, "%x != PAGE_READWRITE\n", info.Protect);

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
    HBITMAP hcoredib;
    char coreBits[256];
    BYTE *bits;
    RGBQUAD rgb[256];
    int ret;
    char logpalbuf[sizeof(LOGPALETTE) + 256 * sizeof(PALETTEENTRY)];
    LOGPALETTE *plogpal = (LOGPALETTE*)logpalbuf;
    WORD *index;
    DWORD *bits32;
    HPALETTE hpal, oldpal;
    DIBSECTION dibsec;
    COLORREF c0, c1;
    int i;
    int screen_depth;
    MEMORY_BASIC_INFORMATION info;

    hdc = GetDC(0);
    screen_depth = GetDeviceCaps(hdc, BITSPIXEL) * GetDeviceCaps(hdc, PLANES);

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
    ok(GetObject(hdib, sizeof(DIBSECTION), &dibsec) != 0, "GetObject failed for DIBSection\n");
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
    pbmi->bmiColors[0].rgbRed = 0xff;
    pbmi->bmiColors[0].rgbGreen = 0;
    pbmi->bmiColors[0].rgbBlue = 0;
    pbmi->bmiColors[1].rgbRed = 0;
    pbmi->bmiColors[1].rgbGreen = 0;
    pbmi->bmiColors[1].rgbBlue = 0xff;

    hdib = CreateDIBSection(hdc, pbmi, DIB_RGB_COLORS, (void**)&bits, NULL, 0);
    ok(hdib != NULL, "CreateDIBSection failed\n");
    ok(GetObject(hdib, sizeof(DIBSECTION), &dibsec) != 0, "GetObject failed for DIBSection\n");
    ok(dibsec.dsBmih.biClrUsed == 2,
        "created DIBSection: wrong biClrUsed field: %u, should be: %u\n", dibsec.dsBmih.biClrUsed, 2);

    /* Test if the old BITMAPCOREINFO structure is supported */    
        
    pbci->bmciHeader.bcSize = sizeof(BITMAPCOREHEADER);
    pbci->bmciHeader.bcBitCount = 0;

    if (!is_win9x) {
        ret = GetDIBits(hdc, hdib, 0, 16, NULL, (BITMAPINFO*) pbci, DIB_RGB_COLORS);
        ok(ret, "GetDIBits doesn't work with a BITMAPCOREHEADER\n");
        ok((pbci->bmciHeader.bcWidth == 16) && (pbci->bmciHeader.bcHeight == 16)
            && (pbci->bmciHeader.bcBitCount == 1) && (pbci->bmciHeader.bcPlanes == 1),
        "GetDIBits did't fill in the BITMAPCOREHEADER structure properly\n");

        ret = GetDIBits(hdc, hdib, 0, 16, &coreBits, (BITMAPINFO*) pbci, DIB_RGB_COLORS);
        ok(ret, "GetDIBits doesn't work with a BITMAPCOREHEADER\n");
        ok((pbci->bmciColors[0].rgbtRed == 0xff) && (pbci->bmciColors[0].rgbtGreen == 0) &&
            (pbci->bmciColors[0].rgbtBlue == 0) && (pbci->bmciColors[1].rgbtRed == 0) &&
            (pbci->bmciColors[1].rgbtGreen == 0) && (pbci->bmciColors[1].rgbtBlue == 0xff),
            "The color table has not been translated to the old BITMAPCOREINFO format\n");

        hcoredib = CreateDIBSection(hdc, (BITMAPINFO*) pbci, DIB_RGB_COLORS, (void**)&bits, NULL, 0);
        ok(hcoredib != NULL, "CreateDIBSection failed with a BITMAPCOREINFO\n");

        ZeroMemory(pbci->bmciColors, 256 * sizeof(RGBTRIPLE));
        ret = GetDIBits(hdc, hcoredib, 0, 16, &coreBits, (BITMAPINFO*) pbci, DIB_RGB_COLORS);
        ok(ret, "GetDIBits doesn't work with a BITMAPCOREHEADER\n");
        ok((pbci->bmciColors[0].rgbtRed == 0xff) && (pbci->bmciColors[0].rgbtGreen == 0) &&
            (pbci->bmciColors[0].rgbtBlue == 0) && (pbci->bmciColors[1].rgbtRed == 0) &&
            (pbci->bmciColors[1].rgbtGreen == 0) && (pbci->bmciColors[1].rgbtBlue == 0xff),
            "The color table has not been translated to the old BITMAPCOREINFO format\n");

        DeleteObject(hcoredib);
    }

    hdcmem = CreateCompatibleDC(hdc);
    oldbm = SelectObject(hdcmem, hdib);

    ret = GetDIBColorTable(hdcmem, 0, 2, rgb);
    ok(ret == 2, "GetDIBColorTable returned %d\n", ret);
    ok(!memcmp(rgb, pbmi->bmiColors, 2 * sizeof(RGBQUAD)),
       "GetDIBColorTable returns table 0: r%02x g%02x b%02x res%02x 1: r%02x g%02x b%02x res%02x\n",
       rgb[0].rgbRed, rgb[0].rgbGreen, rgb[0].rgbBlue, rgb[0].rgbReserved,
       rgb[1].rgbRed, rgb[1].rgbGreen, rgb[1].rgbBlue, rgb[1].rgbReserved);

    c0 = RGB(pbmi->bmiColors[0].rgbRed, pbmi->bmiColors[0].rgbGreen, pbmi->bmiColors[0].rgbBlue);
    c1 = RGB(pbmi->bmiColors[1].rgbRed, pbmi->bmiColors[1].rgbGreen, pbmi->bmiColors[1].rgbBlue);

    test_color(hdcmem, DIBINDEX(0), c0, 0, 1);
    test_color(hdcmem, DIBINDEX(1), c1, 0, 1);
    test_color(hdcmem, DIBINDEX(2), c0, 1, 1);
    test_color(hdcmem, PALETTEINDEX(0), c0, 1, 1);
    test_color(hdcmem, PALETTEINDEX(1), c0, 1, 1);
    test_color(hdcmem, PALETTEINDEX(2), c0, 1, 1);
    test_color(hdcmem, PALETTERGB(pbmi->bmiColors[0].rgbRed, pbmi->bmiColors[0].rgbGreen,
        pbmi->bmiColors[0].rgbBlue), c0, 1, 1);
    test_color(hdcmem, PALETTERGB(pbmi->bmiColors[1].rgbRed, pbmi->bmiColors[1].rgbGreen,
        pbmi->bmiColors[1].rgbBlue), c1, 1, 1);
    test_color(hdcmem, PALETTERGB(0, 0, 0), c0, 1, 1);
    test_color(hdcmem, PALETTERGB(0xff, 0xff, 0xff), c0, 1, 1);
    test_color(hdcmem, PALETTERGB(0, 0, 0xfe), c1, 1, 1);

    SelectObject(hdcmem, oldbm);
    DeleteObject(hdib);

    pbmi->bmiColors[0].rgbRed = 0xff;
    pbmi->bmiColors[0].rgbGreen = 0xff;
    pbmi->bmiColors[0].rgbBlue = 0xff;
    pbmi->bmiColors[1].rgbRed = 0;
    pbmi->bmiColors[1].rgbGreen = 0;
    pbmi->bmiColors[1].rgbBlue = 0;

    hdib = CreateDIBSection(hdc, pbmi, DIB_RGB_COLORS, (void**)&bits, NULL, 0);
    ok(hdib != NULL, "CreateDIBSection failed\n");

    test_dib_info(hdib, bits, &pbmi->bmiHeader);

    oldbm = SelectObject(hdcmem, hdib);

    ret = GetDIBColorTable(hdcmem, 0, 2, rgb);
    ok(ret == 2, "GetDIBColorTable returned %d\n", ret);
    ok(!memcmp(rgb, pbmi->bmiColors, 2 * sizeof(RGBQUAD)),
       "GetDIBColorTable returns table 0: r%02x g%02x b%02x res%02x 1: r%02x g%02x b%02x res%02x\n",
       rgb[0].rgbRed, rgb[0].rgbGreen, rgb[0].rgbBlue, rgb[0].rgbReserved,
       rgb[1].rgbRed, rgb[1].rgbGreen, rgb[1].rgbBlue, rgb[1].rgbReserved);

    SelectObject(hdcmem, oldbm);
    test_dib_info(hdib, bits, &pbmi->bmiHeader);
    DeleteObject(hdib);

    pbmi->bmiHeader.biBitCount = 4;
    for (i = 0; i < 16; i++) {
        pbmi->bmiColors[i].rgbRed = i;
        pbmi->bmiColors[i].rgbGreen = 16-i;
        pbmi->bmiColors[i].rgbBlue = 0;
    }
    hdib = CreateDIBSection(hdcmem, pbmi, DIB_RGB_COLORS, (void**)&bits, NULL, 0);
    ok(hdib != NULL, "CreateDIBSection failed\n");
    ok(GetObject(hdib, sizeof(DIBSECTION), &dibsec) != 0, "GetObject failed for DIB Section\n");
    ok(dibsec.dsBmih.biClrUsed == 16,
       "created DIBSection: wrong biClrUsed field: %u, should be: %u\n", dibsec.dsBmih.biClrUsed, 16);
    test_dib_info(hdib, bits, &pbmi->bmiHeader);
    DeleteObject(hdib);

    pbmi->bmiHeader.biBitCount = 8;

    for (i = 0; i < 128; i++) {
        pbmi->bmiColors[i].rgbRed = 255 - i * 2;
        pbmi->bmiColors[i].rgbGreen = i * 2;
        pbmi->bmiColors[i].rgbBlue = 0;
        pbmi->bmiColors[255 - i].rgbRed = 0;
        pbmi->bmiColors[255 - i].rgbGreen = i * 2;
        pbmi->bmiColors[255 - i].rgbBlue = 255 - i * 2;
    }
    hdib = CreateDIBSection(hdcmem, pbmi, DIB_RGB_COLORS, (void**)&bits, NULL, 0);
    ok(hdib != NULL, "CreateDIBSection failed\n");
    ok(GetObject(hdib, sizeof(DIBSECTION), &dibsec) != 0, "GetObject failed for DIB Section\n");
    ok(dibsec.dsBmih.biClrUsed == 256,
        "created DIBSection: wrong biClrUsed field: %u, should be: %u\n", dibsec.dsBmih.biClrUsed, 256);

    oldbm = SelectObject(hdcmem, hdib);

    for (i = 0; i < 256; i++) {
        test_color(hdcmem, DIBINDEX(i), 
            RGB(pbmi->bmiColors[i].rgbRed, pbmi->bmiColors[i].rgbGreen, pbmi->bmiColors[i].rgbBlue), 0, 0);
        test_color(hdcmem, PALETTERGB(pbmi->bmiColors[i].rgbRed, pbmi->bmiColors[i].rgbGreen, pbmi->bmiColors[i].rgbBlue), 
            RGB(pbmi->bmiColors[i].rgbRed, pbmi->bmiColors[i].rgbGreen, pbmi->bmiColors[i].rgbBlue), 0, 0);
    }

    SelectObject(hdcmem, oldbm);
    test_dib_info(hdib, bits, &pbmi->bmiHeader);
    DeleteObject(hdib);

    pbmi->bmiHeader.biBitCount = 1;

    /* Now create a palette and a palette indexed dib section */
    memset(plogpal, 0, sizeof(logpalbuf));
    plogpal->palVersion = 0x300;
    plogpal->palNumEntries = 2;
    plogpal->palPalEntry[0].peRed = 0xff;
    plogpal->palPalEntry[0].peBlue = 0xff;
    plogpal->palPalEntry[1].peGreen = 0xff;

    index = (WORD*)pbmi->bmiColors;
    *index++ = 0;
    *index = 1;
    hpal = CreatePalette(plogpal);
    ok(hpal != NULL, "CreatePalette failed\n");
    oldpal = SelectPalette(hdc, hpal, TRUE);
    hdib = CreateDIBSection(hdc, pbmi, DIB_PAL_COLORS, (void**)&bits, NULL, 0);
    ok(hdib != NULL, "CreateDIBSection failed\n");
    ok(GetObject(hdib, sizeof(DIBSECTION), &dibsec) != 0, "GetObject failed for DIB Section\n");
    ok(dibsec.dsBmih.biClrUsed == 2 ||
       broken(dibsec.dsBmih.biClrUsed == 0), /* win9x */
        "created DIBSection: wrong biClrUsed field: %u, should be: %u\n", dibsec.dsBmih.biClrUsed, 2);

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

    c0 = RGB(plogpal->palPalEntry[0].peRed, plogpal->palPalEntry[0].peGreen, plogpal->palPalEntry[0].peBlue);
    c1 = RGB(plogpal->palPalEntry[1].peRed, plogpal->palPalEntry[1].peGreen, plogpal->palPalEntry[1].peBlue);

    test_color(hdcmem, DIBINDEX(0), c0, 0, 1);
    test_color(hdcmem, DIBINDEX(1), c1, 0, 1);
    test_color(hdcmem, DIBINDEX(2), c0, 1, 1);
    test_color(hdcmem, PALETTEINDEX(0), c0, 0, 1);
    test_color(hdcmem, PALETTEINDEX(1), c1, 0, 1);
    test_color(hdcmem, PALETTEINDEX(2), c0, 1, 1);
    test_color(hdcmem, PALETTERGB(plogpal->palPalEntry[0].peRed, plogpal->palPalEntry[0].peGreen,
        plogpal->palPalEntry[0].peBlue), c0, 1, 1);
    test_color(hdcmem, PALETTERGB(plogpal->palPalEntry[1].peRed, plogpal->palPalEntry[1].peGreen,
        plogpal->palPalEntry[1].peBlue), c1, 1, 1);
    test_color(hdcmem, PALETTERGB(0, 0, 0), c1, 1, 1);
    test_color(hdcmem, PALETTERGB(0xff, 0xff, 0xff), c0, 1, 1);
    test_color(hdcmem, PALETTERGB(0, 0, 0xfe), c0, 1, 1);
    test_color(hdcmem, PALETTERGB(0, 1, 0), c1, 1, 1);
    test_color(hdcmem, PALETTERGB(0x3f, 0, 0x3f), c1, 1, 1);
    test_color(hdcmem, PALETTERGB(0x40, 0, 0x40), c0, 1, 1);

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
    SelectObject(hdcmem, oldpal);
    DeleteObject(hdib);
    DeleteObject(hpal);


    pbmi->bmiHeader.biBitCount = 8;

    memset(plogpal, 0, sizeof(logpalbuf));
    plogpal->palVersion = 0x300;
    plogpal->palNumEntries = 256;

    for (i = 0; i < 128; i++) {
        plogpal->palPalEntry[i].peRed = 255 - i * 2;
        plogpal->palPalEntry[i].peBlue = i * 2;
        plogpal->palPalEntry[i].peGreen = 0;
        plogpal->palPalEntry[255 - i].peRed = 0;
        plogpal->palPalEntry[255 - i].peGreen = i * 2;
        plogpal->palPalEntry[255 - i].peBlue = 255 - i * 2;
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
    ok(GetObject(hdib, sizeof(DIBSECTION), &dibsec) != 0, "GetObject failed for DIB Section\n");
    ok(dibsec.dsBmih.biClrUsed == 256 ||
       broken(dibsec.dsBmih.biClrUsed == 0), /* win9x */
        "created DIBSection: wrong biClrUsed field: %u, should be: %u\n", dibsec.dsBmih.biClrUsed, 256);

    test_dib_info(hdib, bits, &pbmi->bmiHeader);

    SelectPalette(hdc, oldpal, TRUE);
    oldbm = SelectObject(hdcmem, hdib);
    oldpal = SelectPalette(hdcmem, hpal, TRUE);

    ret = GetDIBColorTable(hdcmem, 0, 256, rgb);
    ok(ret == 256, "GetDIBColorTable returned %d\n", ret);
    for (i = 0; i < 256; i++) {
        ok(rgb[i].rgbRed == plogpal->palPalEntry[i].peRed && 
            rgb[i].rgbBlue == plogpal->palPalEntry[i].peBlue && 
            rgb[i].rgbGreen == plogpal->palPalEntry[i].peGreen, 
            "GetDIBColorTable returns table %d: r%02x g%02x b%02x res%02x\n",
            i, rgb[i].rgbRed, rgb[i].rgbGreen, rgb[i].rgbBlue, rgb[i].rgbReserved);
    }

    for (i = 0; i < 256; i++) {
        test_color(hdcmem, DIBINDEX(i), 
            RGB(plogpal->palPalEntry[i].peRed, plogpal->palPalEntry[i].peGreen, plogpal->palPalEntry[i].peBlue), 0, 0);
        test_color(hdcmem, PALETTEINDEX(i), 
            RGB(plogpal->palPalEntry[i].peRed, plogpal->palPalEntry[i].peGreen, plogpal->palPalEntry[i].peBlue), 0, 0);
        test_color(hdcmem, PALETTERGB(plogpal->palPalEntry[i].peRed, plogpal->palPalEntry[i].peGreen, plogpal->palPalEntry[i].peBlue), 
            RGB(plogpal->palPalEntry[i].peRed, plogpal->palPalEntry[i].peGreen, plogpal->palPalEntry[i].peBlue), 0, 0);
    }

    SelectPalette(hdcmem, oldpal, TRUE);
    SelectObject(hdcmem, oldbm);
    DeleteObject(hdib);
    DeleteObject(hpal);


    DeleteDC(hdcmem);
    ReleaseDC(0, hdc);
}

static void test_mono_dibsection(void)
{
    HDC hdc, memdc;
    HBITMAP old_bm, mono_ds;
    char bmibuf[sizeof(BITMAPINFO) + 256 * sizeof(RGBQUAD)];
    BITMAPINFO *pbmi = (BITMAPINFO *)bmibuf;
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
    pbmi->bmiColors[0].rgbRed = 0xff;
    pbmi->bmiColors[0].rgbGreen = 0xff;
    pbmi->bmiColors[0].rgbBlue = 0xff;
    pbmi->bmiColors[1].rgbRed = 0x0;
    pbmi->bmiColors[1].rgbGreen = 0x0;
    pbmi->bmiColors[1].rgbBlue = 0x0;

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

    pbmi->bmiColors[0].rgbRed = 0x0;
    pbmi->bmiColors[0].rgbGreen = 0x0;
    pbmi->bmiColors[0].rgbBlue = 0x0;
    pbmi->bmiColors[1].rgbRed = 0xff;
    pbmi->bmiColors[1].rgbGreen = 0xff;
    pbmi->bmiColors[1].rgbBlue = 0xff;

    SetDIBitsToDevice(memdc, 0, 0, 10, 10, 0, 0, 0, 10, bits, pbmi, DIB_RGB_COLORS);
    ok(ds_bits[0] == 0x55, "out_bits %02x\n", ds_bits[0]);

    SelectObject(memdc, old_bm);
    DeleteObject(mono_ds);

    /*
     * Next dib section is 'normal' ie color[0] is black, color[1] is white
     */

    pbmi->bmiColors[0].rgbRed = 0x0;
    pbmi->bmiColors[0].rgbGreen = 0x0;
    pbmi->bmiColors[0].rgbBlue = 0x0;
    pbmi->bmiColors[1].rgbRed = 0xff;
    pbmi->bmiColors[1].rgbGreen = 0xff;
    pbmi->bmiColors[1].rgbBlue = 0xff;

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

    /* SetDIBitsToDevice with a inverted bmi -> normal dib section */

    pbmi->bmiColors[0].rgbRed = 0xff;
    pbmi->bmiColors[0].rgbGreen = 0xff;
    pbmi->bmiColors[0].rgbBlue = 0xff;
    pbmi->bmiColors[1].rgbRed = 0x0;
    pbmi->bmiColors[1].rgbGreen = 0x0;
    pbmi->bmiColors[1].rgbBlue = 0x0;

    SetDIBitsToDevice(memdc, 0, 0, 10, 10, 0, 0, 0, 10, bits, pbmi, DIB_RGB_COLORS);
    ok(ds_bits[0] == 0x55, "out_bits %02x\n", ds_bits[0]);

    /*
     * Take that 'normal' dibsection and change its colour table to an 'inverted' one
     */

    pbmi->bmiColors[0].rgbRed = 0xff;
    pbmi->bmiColors[0].rgbGreen = 0xff;
    pbmi->bmiColors[0].rgbBlue = 0xff;
    pbmi->bmiColors[1].rgbRed = 0x0;
    pbmi->bmiColors[1].rgbGreen = 0x0;
    pbmi->bmiColors[1].rgbBlue = 0x0;
    num = SetDIBColorTable(memdc, 0, 2, pbmi->bmiColors);
    ok(num == 2, "num = %d\n", num);

    /* black border, white interior */
    Rectangle(memdc, 0, 0, 10, 10);
todo_wine {
    ok(ds_bits[0] == 0xff, "out_bits %02x\n", ds_bits[0]);
    ok(ds_bits[4] == 0x80, "out_bits %02x\n", ds_bits[4]);
 }
    /* SetDIBitsToDevice with an inverted bmi -> inverted dib section */

    memset(bits, 0, sizeof(bits));
    bits[0] = 0xaa;

    SetDIBitsToDevice(memdc, 0, 0, 10, 10, 0, 0, 0, 10, bits, pbmi, DIB_RGB_COLORS);
    ok(ds_bits[0] == 0xaa, "out_bits %02x\n", ds_bits[0]);

    /* SetDIBitsToDevice with a normal bmi -> inverted dib section */

    pbmi->bmiColors[0].rgbRed = 0x0;
    pbmi->bmiColors[0].rgbGreen = 0x0;
    pbmi->bmiColors[0].rgbBlue = 0x0;
    pbmi->bmiColors[1].rgbRed = 0xff;
    pbmi->bmiColors[1].rgbGreen = 0xff;
    pbmi->bmiColors[1].rgbBlue = 0xff;

    SetDIBitsToDevice(memdc, 0, 0, 10, 10, 0, 0, 0, 10, bits, pbmi, DIB_RGB_COLORS);
    ok(ds_bits[0] == 0x55, "out_bits %02x\n", ds_bits[0]);

    SelectObject(memdc, old_bm);
    DeleteObject(mono_ds);

    /*
     * Now a dib section with a strange colour map just for fun.  This behaves just like an inverted one.
     */
 
    pbmi->bmiColors[0].rgbRed = 0xff;
    pbmi->bmiColors[0].rgbGreen = 0x0;
    pbmi->bmiColors[0].rgbBlue = 0x0;
    pbmi->bmiColors[1].rgbRed = 0xfe;
    pbmi->bmiColors[1].rgbGreen = 0x0;
    pbmi->bmiColors[1].rgbBlue = 0x0;

    mono_ds = CreateDIBSection(hdc, pbmi, DIB_RGB_COLORS, (void**)&ds_bits, NULL, 0);
    ok(mono_ds != NULL, "CreateDIBSection rets NULL\n");
    old_bm = SelectObject(memdc, mono_ds);

    /* black border, white interior */
    Rectangle(memdc, 0, 0, 10, 10);
    ok(ds_bits[0] == 0xff, "out_bits %02x\n", ds_bits[0]);
    ok(ds_bits[4] == 0x80, "out_bits %02x\n", ds_bits[4]);

    /* SetDIBitsToDevice with a normal bmi -> inverted dib section */

    pbmi->bmiColors[0].rgbRed = 0x0;
    pbmi->bmiColors[0].rgbGreen = 0x0;
    pbmi->bmiColors[0].rgbBlue = 0x0;
    pbmi->bmiColors[1].rgbRed = 0xff;
    pbmi->bmiColors[1].rgbGreen = 0xff;
    pbmi->bmiColors[1].rgbBlue = 0xff;

    SetDIBitsToDevice(memdc, 0, 0, 10, 10, 0, 0, 0, 10, bits, pbmi, DIB_RGB_COLORS);
    ok(ds_bits[0] == 0x55, "out_bits %02x\n", ds_bits[0]);

    /* SetDIBitsToDevice with a inverted bmi -> inverted dib section */

    pbmi->bmiColors[0].rgbRed = 0xff;
    pbmi->bmiColors[0].rgbGreen = 0xff;
    pbmi->bmiColors[0].rgbBlue = 0xff;
    pbmi->bmiColors[1].rgbRed = 0x0;
    pbmi->bmiColors[1].rgbGreen = 0x0;
    pbmi->bmiColors[1].rgbBlue = 0x0;

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
    ok(!hbmp || broken(hbmp != NULL /* Win9x */), "CreateBitmap should fail\n");
    if (!hbmp)
        ok(GetLastError() == ERROR_INVALID_PARAMETER,
           "expected ERROR_INVALID_PARAMETER, got %u\n", GetLastError());
    else
        DeleteObject(hbmp);

    hbmp = CreateBitmap(15, 15, 1, 1, NULL);
    assert(hbmp != NULL);

    ret = GetObject(hbmp, sizeof(bm), &bm);
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
    ok(ret == bm.bmWidthBytes * bm.bmHeight || broken(ret == 0 /* Win9x */),
        "%d != %d\n", ret, bm.bmWidthBytes * bm.bmHeight);

    memset(buf_cmp, 0xAA, sizeof(buf_cmp));
    memset(buf_cmp, 0, bm.bmWidthBytes * bm.bmHeight);

    memset(buf, 0xAA, sizeof(buf));
    ret = GetBitmapBits(hbmp, sizeof(buf), buf);
    ok(ret == bm.bmWidthBytes * bm.bmHeight, "%d != %d\n", ret, bm.bmWidthBytes * bm.bmHeight);
    ok(!memcmp(buf, buf_cmp, sizeof(buf)) ||
       broken(memcmp(buf, buf_cmp, sizeof(buf))), /* win9x doesn't init the bitmap bits */
       "buffers do not match\n");

    hbmp_old = SelectObject(hdc, hbmp);

    ret = GetObject(hbmp, sizeof(bm), &bm);
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
    ok(!memcmp(buf, buf_cmp, sizeof(buf)) ||
       broken(memcmp(buf, buf_cmp, sizeof(buf))), /* win9x doesn't init the bitmap bits */
       "buffers do not match\n");

    hbmp_old = SelectObject(hdc, hbmp_old);
    ok(hbmp_old == hbmp, "wrong old bitmap %p\n", hbmp_old);

    /* test various buffer sizes for GetObject */
    ret = GetObject(hbmp, sizeof(*bma) * 2, bma);
    ok(ret == sizeof(*bma) || broken(ret == sizeof(*bma) * 2 /* Win9x */), "wrong size %d\n", ret);

    ret = GetObject(hbmp, sizeof(bm) / 2, &bm);
    ok(ret == 0 || broken(ret == sizeof(bm) / 2 /* Win9x */), "%d != 0\n", ret);

    ret = GetObject(hbmp, 0, &bm);
    ok(ret == 0, "%d != 0\n", ret);

    ret = GetObject(hbmp, 1, &bm);
    ok(ret == 0 || broken(ret == 1 /* Win9x */), "%d != 0\n", ret);

    DeleteObject(hbmp);
    DeleteDC(hdc);
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
    ok(GetObject(hbmp, sizeof(bmp), &bmp) == sizeof(bmp),
       "GetObject failed or returned a wrong structure size\n");
    ok(!bmp.bmBits, "bmBits must be NULL for device-dependent bitmaps\n");

    DeleteObject(hbmp);
}

static void test_GetDIBits_selected_DIB(UINT bpp)
{
    HBITMAP dib;
    BITMAPINFO * info;
    BITMAPINFO * info2;
    void * bits;
    void * bits2;
    UINT dib_size;
    HDC dib_dc, dc;
    HBITMAP old_bmp;
    BOOL equalContents;
    UINT i;
    int res;

    /* Create a DIB section with a color table */

    info  = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BITMAPINFOHEADER) + (1 << bpp) * sizeof(RGBQUAD));
    info2 = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BITMAPINFOHEADER) + (1 << bpp) * sizeof(RGBQUAD));
    assert(info);
    assert(info2);

    info->bmiHeader.biSize = sizeof(info->bmiHeader);

    /* Choose width and height such that the row length (in bytes)
       is a multiple of 4 (makes things easier) */
    info->bmiHeader.biWidth = 32;
    info->bmiHeader.biHeight = 32;
    info->bmiHeader.biPlanes = 1;
    info->bmiHeader.biBitCount = bpp;
    info->bmiHeader.biCompression = BI_RGB;

    for (i=0; i < (1u << bpp); i++)
    {
        BYTE c = i * (1 << (8 - bpp));
        info->bmiColors[i].rgbRed = c;
        info->bmiColors[i].rgbGreen = c;
        info->bmiColors[i].rgbBlue = c;
        info->bmiColors[i].rgbReserved = 0;
    }

    dib = CreateDIBSection(NULL, info, DIB_RGB_COLORS, &bits, NULL, 0);
    assert(dib);
    dib_size = bpp * (info->bmiHeader.biWidth * info->bmiHeader.biHeight) / 8;

    /* Set the bits of the DIB section */
    for (i=0; i < dib_size; i++)
    {
        ((BYTE *)bits)[i] = i % 256;
    }

    /* Select the DIB into a DC */
    dib_dc = CreateCompatibleDC(NULL);
    old_bmp = SelectObject(dib_dc, dib);
    dc = CreateCompatibleDC(NULL);
    bits2 = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dib_size);
    assert(bits2);

    /* Copy the DIB attributes but not the color table */
    memcpy(info2, info, sizeof(BITMAPINFOHEADER));

    res = GetDIBits(dc, dib, 0, info->bmiHeader.biHeight, bits2, info2, DIB_RGB_COLORS);
    ok(res, "GetDIBits failed\n");

    /* Compare the color table and the bits */
    equalContents = TRUE;
    for (i=0; i < (1u << bpp); i++)
    {
        if ((info->bmiColors[i].rgbRed != info2->bmiColors[i].rgbRed)
            || (info->bmiColors[i].rgbGreen != info2->bmiColors[i].rgbGreen)
            || (info->bmiColors[i].rgbBlue != info2->bmiColors[i].rgbBlue)
            || (info->bmiColors[i].rgbReserved != info2->bmiColors[i].rgbReserved))
        {
            equalContents = FALSE;
            break;
        }
    }
    ok(equalContents, "GetDIBits with DIB selected in DC: Invalid DIB color table\n");

    equalContents = TRUE;
    for (i=0; i < dib_size / sizeof(DWORD); i++)
    {
        if (((DWORD *)bits)[i] != ((DWORD *)bits2)[i])
        {
            equalContents = FALSE;
            break;
        }
    }
    ok(equalContents, "GetDIBits with %d bpp DIB selected in DC: Invalid DIB bits\n",bpp);

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
    BITMAPINFO * info;
    BITMAPINFO * info2;
    void * bits;
    void * bits2;
    HDC ddb_dc, dc;
    HBITMAP old_bmp;
    BOOL equalContents;
    UINT width, height;
    UINT bpp;
    UINT i, j;
    int res;

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

    info  = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));
    info2 = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));
    assert(info);
    assert(info2);

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
    assert(bits);
    assert(bits2);

    /* Get the bits */
    res = GetDIBits(dc, ddb, 0, height, bits, info, DIB_RGB_COLORS);
    ok(res, "GetDIBits failed\n");

    /* Copy the DIB attributes but not the color table */
    memcpy(info2, info, sizeof(BITMAPINFOHEADER));

    /* Select the DDB into another DC */
    old_bmp = SelectObject(ddb_dc, ddb);

    /* Get the bits */
    res = GetDIBits(dc, ddb, 0, height, bits2, info2, DIB_RGB_COLORS);
    ok(res, "GetDIBits failed\n");

    /* Compare the color table and the bits */
    if (bpp <= 8)
    {
        equalContents = TRUE;
        for (i=0; i < (1u << bpp); i++)
        {
            if ((info->bmiColors[i].rgbRed != info2->bmiColors[i].rgbRed)
                || (info->bmiColors[i].rgbGreen != info2->bmiColors[i].rgbGreen)
                || (info->bmiColors[i].rgbBlue != info2->bmiColors[i].rgbBlue)
                || (info->bmiColors[i].rgbReserved != info2->bmiColors[i].rgbReserved))
            {
                equalContents = FALSE;
                break;
            }
        }
        ok(equalContents, "GetDIBits with DDB selected in DC: Got a different color table\n");
    }

    equalContents = TRUE;
    for (i=0; i < info->bmiHeader.biSizeImage / sizeof(DWORD); i++)
    {
        if (((DWORD *)bits)[i] != ((DWORD *)bits2)[i])
        {
            equalContents = FALSE;
        }
    }
    ok(equalContents, "GetDIBits with DDB selected in DC: Got different DIB bits\n");

    /* Test the palette */
    equalContents = TRUE;
    if (info2->bmiHeader.biBitCount <= 8)
    {
        WORD *colors = (WORD*)info2->bmiColors;

        /* Get the palette indices */
        res = GetDIBits(dc, ddb, 0, 0, NULL, info2, DIB_PAL_COLORS);
        if (res == 0 && GetLastError() == ERROR_INVALID_PARAMETER) /* Win9x */
            res = GetDIBits(dc, ddb, 0, height, NULL, info2, DIB_PAL_COLORS);
        ok(res, "GetDIBits failed\n");

        for (i=0;i < 1 << info->bmiHeader.biSizeImage; i++)
        {
            if (colors[i] != i)
            {
                equalContents = FALSE;
                break;
            }
        }
    }

    ok(equalContents, "GetDIBits with DDB selected in DC: non 1:1 palette indices\n");

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
    static const BYTE dib_bits_1_9x[16 * 4] =
    {
        0,0,0xaa,0xaa, 0xff,0xff,0xaa,0xaa, 0,0,0xaa,0xaa, 0xff,0xff,0xaa,0xaa,
        0,0,0xaa,0xaa, 0xff,0xff,0xaa,0xaa, 0,0,0xaa,0xaa, 0xff,0xff,0xaa,0xaa,
        0,0,0xaa,0xaa, 0xff,0xff,0xaa,0xaa, 0,0,0xaa,0xaa, 0xff,0xff,0xaa,0xaa,
        0,0,0xaa,0xaa, 0xff,0xff,0xaa,0xaa, 0,0,0xaa,0xaa, 0xff,0xff,0xaa,0xaa
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

    hdc = GetDC(0);

    /* 1-bit source bitmap data */
    hbmp = CreateBitmap(16, 16, 1, 1, bmp_bits_1);
    ok(hbmp != 0, "CreateBitmap failed\n");

    memset(&bm, 0xAA, sizeof(bm));
    bytes = GetObject(hbmp, sizeof(bm), &bm);
    ok(bytes == sizeof(bm), "GetObject returned %d\n", bytes);
    ok(bm.bmType == 0, "wrong bmType %d\n", bm.bmType);
    ok(bm.bmWidth == 16, "wrong bmWidth %d\n", bm.bmWidth);
    ok(bm.bmHeight == 16, "wrong bmHeight %d\n", bm.bmHeight);
    ok(bm.bmWidthBytes == 2, "wrong bmWidthBytes %d\n", bm.bmWidthBytes);
    ok(bm.bmPlanes == 1, "wrong bmPlanes %u\n", bm.bmPlanes);
    ok(bm.bmBitsPixel == 1, "wrong bmBitsPixel %d\n", bm.bmBitsPixel);
    ok(!bm.bmBits, "wrong bmBits %p\n", bm.bmBits);

    bytes = GetBitmapBits(hbmp, 0, NULL);
    ok(bytes == sizeof(bmp_bits_1) || broken(bytes == 0 /* Win9x */), "expected 16*2 got %d bytes\n", bytes);
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
    bi->bmiHeader.biSizeImage = 0;
    memset(bi->bmiColors, 0xAA, sizeof(RGBQUAD) * 256);
    SetLastError(0xdeadbeef);
    lines = GetDIBits(0, hbmp, 0, bm.bmHeight, buf, bi, DIB_RGB_COLORS);
    ok(lines == 0, "GetDIBits copied %d lines with hdc = 0\n", lines);
    ok(GetLastError() == ERROR_INVALID_PARAMETER ||
       broken(GetLastError() == 0xdeadbeef), /* winnt */
       "wrong error %u\n", GetLastError());
    ok(bi->bmiHeader.biSizeImage == 0, "expected 0, got %u\n", bi->bmiHeader.biSizeImage);

    memset(buf, 0xAA, sizeof(buf));
    SetLastError(0xdeadbeef);
    lines = GetDIBits(hdc, hbmp, 0, bm.bmHeight, buf, bi, DIB_RGB_COLORS);
    ok(lines == bm.bmHeight, "GetDIBits copied %d lines of %d, error %u\n",
       lines, bm.bmHeight, GetLastError());
    ok(bi->bmiHeader.biSizeImage == sizeof(dib_bits_1) ||
       broken(bi->bmiHeader.biSizeImage == 0), /* win9x */
       "expected 16*4, got %u\n", bi->bmiHeader.biSizeImage);

    /* the color table consists of black and white */
    ok(bi->bmiColors[0].rgbRed == 0 && bi->bmiColors[0].rgbGreen == 0 &&
       bi->bmiColors[0].rgbBlue == 0 && bi->bmiColors[0].rgbReserved == 0,
       "expected bmiColors[0] 0,0,0,0 - got %x %x %x %x\n",
       bi->bmiColors[0].rgbRed, bi->bmiColors[0].rgbGreen,
       bi->bmiColors[0].rgbBlue, bi->bmiColors[0].rgbReserved);
    ok(bi->bmiColors[1].rgbRed == 0xff && bi->bmiColors[1].rgbGreen == 0xff &&
       bi->bmiColors[1].rgbBlue == 0xff && bi->bmiColors[1].rgbReserved == 0,
       "expected bmiColors[0] 0xff,0xff,0xff,0 - got %x %x %x %x\n",
       bi->bmiColors[1].rgbRed, bi->bmiColors[1].rgbGreen,
       bi->bmiColors[1].rgbBlue, bi->bmiColors[1].rgbReserved);
    for (i = 2; i < 256; i++)
    {
        ok(bi->bmiColors[i].rgbRed == 0xAA && bi->bmiColors[i].rgbGreen == 0xAA &&
           bi->bmiColors[i].rgbBlue == 0xAA && bi->bmiColors[i].rgbReserved == 0xAA,
           "expected bmiColors[%d] 0xAA,0xAA,0xAA,0xAA - got %x %x %x %x\n", i,
           bi->bmiColors[i].rgbRed, bi->bmiColors[i].rgbGreen,
           bi->bmiColors[i].rgbBlue, bi->bmiColors[i].rgbReserved);
    }

    /* returned bits are DWORD aligned and upside down */
    ok(!memcmp(buf, dib_bits_1, sizeof(dib_bits_1)) ||
       broken(!memcmp(buf, dib_bits_1_9x, sizeof(dib_bits_1_9x))), /* Win9x, WinME */
       "DIB bits don't match\n");

    /* Test the palette indices */
    memset(bi->bmiColors, 0xAA, sizeof(RGBQUAD) * 256);
    SetLastError(0xdeadbeef);
    lines = GetDIBits(hdc, hbmp, 0, 0, NULL, bi, DIB_PAL_COLORS);
    if (lines == 0 && GetLastError() == ERROR_INVALID_PARAMETER)
        win_skip("Win9x/WinMe doesn't handle 0 for the number of scan lines\n");
    else
    {
        ok(((WORD*)bi->bmiColors)[0] == 0, "Color 0 is %d\n", ((WORD*)bi->bmiColors)[0]);
        ok(((WORD*)bi->bmiColors)[1] == 1, "Color 1 is %d\n", ((WORD*)bi->bmiColors)[1]);
    }
    for (i = 2; i < 256; i++)
        ok(((WORD*)bi->bmiColors)[i] == 0xAAAA, "Color %d is %d\n", i, ((WORD*)bi->bmiColors)[1]);

    /* retrieve 24-bit DIB data */
    memset(bi, 0, sizeof(*bi));
    bi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi->bmiHeader.biWidth = bm.bmWidth;
    bi->bmiHeader.biHeight = bm.bmHeight;
    bi->bmiHeader.biPlanes = 1;
    bi->bmiHeader.biBitCount = 24;
    bi->bmiHeader.biCompression = BI_RGB;
    bi->bmiHeader.biSizeImage = 0;
    memset(bi->bmiColors, 0xAA, sizeof(RGBQUAD) * 256);
    memset(buf, 0xAA, sizeof(buf));
    SetLastError(0xdeadbeef);
    lines = GetDIBits(hdc, hbmp, 0, bm.bmHeight, buf, bi, DIB_RGB_COLORS);
    ok(lines == bm.bmHeight, "GetDIBits copied %d lines of %d, error %u\n",
       lines, bm.bmHeight, GetLastError());
    ok(bi->bmiHeader.biSizeImage == sizeof(dib_bits_24) ||
       broken(bi->bmiHeader.biSizeImage == 0), /* win9x */
       "expected 16*16*3, got %u\n", bi->bmiHeader.biSizeImage);

    /* the color table doesn't exist for 24-bit images */
    for (i = 0; i < 256; i++)
    {
        ok(bi->bmiColors[i].rgbRed == 0xAA && bi->bmiColors[i].rgbGreen == 0xAA &&
           bi->bmiColors[i].rgbBlue == 0xAA && bi->bmiColors[i].rgbReserved == 0xAA,
           "expected bmiColors[%d] 0xAA,0xAA,0xAA,0xAA - got %x %x %x %x\n", i,
           bi->bmiColors[i].rgbRed, bi->bmiColors[i].rgbGreen,
           bi->bmiColors[i].rgbBlue, bi->bmiColors[i].rgbReserved);
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
    bytes = GetObject(hbmp, sizeof(bm), &bm);
    ok(bytes == sizeof(bm), "GetObject returned %d\n", bytes);
    ok(bm.bmType == 0 ||
       broken(bm.bmType == 21072), /* win9x */
       "wrong bmType %d\n", bm.bmType);
    ok(bm.bmWidth == 16, "wrong bmWidth %d\n", bm.bmWidth);
    ok(bm.bmHeight == 16, "wrong bmHeight %d\n", bm.bmHeight);
    ok(bm.bmWidthBytes == BITMAP_GetWidthBytes(bm.bmWidth, bm.bmBitsPixel), "wrong bmWidthBytes %d\n", bm.bmWidthBytes);
    ok(bm.bmPlanes == GetDeviceCaps(hdc, PLANES), "wrong bmPlanes %u\n", bm.bmPlanes);
    ok(bm.bmBitsPixel == GetDeviceCaps(hdc, BITSPIXEL), "wrong bmBitsPixel %d\n", bm.bmBitsPixel);
    ok(!bm.bmBits, "wrong bmBits %p\n", bm.bmBits);

    bytes = GetBitmapBits(hbmp, 0, NULL);
    ok(bytes == bm.bmWidthBytes * bm.bmHeight ||
       broken(bytes == 0), /* win9x */
       "expected %d got %d bytes\n", bm.bmWidthBytes * bm.bmHeight, bytes);
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
    bi->bmiHeader.biSizeImage = 0;
    memset(bi->bmiColors, 0xAA, sizeof(RGBQUAD) * 256);
    memset(buf, 0xAA, sizeof(buf));
    SetLastError(0xdeadbeef);
    lines = GetDIBits(hdc, hbmp, 0, bm.bmHeight, buf, bi, DIB_RGB_COLORS);
    ok(lines == bm.bmHeight, "GetDIBits copied %d lines of %d, error %u\n",
       lines, bm.bmHeight, GetLastError());
    ok(bi->bmiHeader.biSizeImage == sizeof(dib_bits_1) ||
       broken(bi->bmiHeader.biSizeImage == 0), /* win9x */
       "expected 16*4, got %u\n", bi->bmiHeader.biSizeImage);

    /* the color table consists of black and white */
    ok(bi->bmiColors[0].rgbRed == 0 && bi->bmiColors[0].rgbGreen == 0 &&
       bi->bmiColors[0].rgbBlue == 0 && bi->bmiColors[0].rgbReserved == 0,
       "expected bmiColors[0] 0,0,0,0 - got %x %x %x %x\n",
       bi->bmiColors[0].rgbRed, bi->bmiColors[0].rgbGreen,
       bi->bmiColors[0].rgbBlue, bi->bmiColors[0].rgbReserved);
    ok(bi->bmiColors[1].rgbRed == 0xff && bi->bmiColors[1].rgbGreen == 0xff &&
       bi->bmiColors[1].rgbBlue == 0xff && bi->bmiColors[1].rgbReserved == 0,
       "expected bmiColors[0] 0xff,0xff,0xff,0 - got %x %x %x %x\n",
       bi->bmiColors[1].rgbRed, bi->bmiColors[1].rgbGreen,
       bi->bmiColors[1].rgbBlue, bi->bmiColors[1].rgbReserved);
    for (i = 2; i < 256; i++)
    {
        ok(bi->bmiColors[i].rgbRed == 0xAA && bi->bmiColors[i].rgbGreen == 0xAA &&
           bi->bmiColors[i].rgbBlue == 0xAA && bi->bmiColors[i].rgbReserved == 0xAA,
           "expected bmiColors[%d] 0xAA,0xAA,0xAA,0xAA - got %x %x %x %x\n", i,
           bi->bmiColors[i].rgbRed, bi->bmiColors[i].rgbGreen,
           bi->bmiColors[i].rgbBlue, bi->bmiColors[i].rgbReserved);
    }

    /* returned bits are DWORD aligned and upside down */
todo_wine
    ok(!memcmp(buf, dib_bits_1, sizeof(dib_bits_1)) ||
       broken(!memcmp(buf, dib_bits_1_9x, sizeof(dib_bits_1_9x))), /* Win9x, WinME */
       "DIB bits don't match\n");

    /* Test the palette indices */
    memset(bi->bmiColors, 0xAA, sizeof(RGBQUAD) * 256);
    SetLastError(0xdeadbeef);
    lines = GetDIBits(hdc, hbmp, 0, 0, NULL, bi, DIB_PAL_COLORS);
    if (lines == 0 && GetLastError() == ERROR_INVALID_PARAMETER)
        win_skip("Win9x/WinMe doesn't handle 0 for the number of scan lines\n");
    else
    {
        ok(((WORD*)bi->bmiColors)[0] == 0, "Color 0 is %d\n", ((WORD*)bi->bmiColors)[0]);
        ok(((WORD*)bi->bmiColors)[1] == 1, "Color 1 is %d\n", ((WORD*)bi->bmiColors)[1]);
    }
    for (i = 2; i < 256; i++)
        ok(((WORD*)bi->bmiColors)[i] == 0xAAAA, "Color %d is %d\n", i, ((WORD*)bi->bmiColors)[i]);

    /* retrieve 24-bit DIB data */
    memset(bi, 0, sizeof(*bi));
    bi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi->bmiHeader.biWidth = bm.bmWidth;
    bi->bmiHeader.biHeight = bm.bmHeight;
    bi->bmiHeader.biPlanes = 1;
    bi->bmiHeader.biBitCount = 24;
    bi->bmiHeader.biCompression = BI_RGB;
    bi->bmiHeader.biSizeImage = 0;
    memset(bi->bmiColors, 0xAA, sizeof(RGBQUAD) * 256);
    memset(buf, 0xAA, sizeof(buf));
    SetLastError(0xdeadbeef);
    lines = GetDIBits(hdc, hbmp, 0, bm.bmHeight, buf, bi, DIB_RGB_COLORS);
    ok(lines == bm.bmHeight, "GetDIBits copied %d lines of %d, error %u\n",
       lines, bm.bmHeight, GetLastError());
    ok(bi->bmiHeader.biSizeImage == sizeof(dib_bits_24) ||
       broken(bi->bmiHeader.biSizeImage == 0), /* win9x */
       "expected 16*16*3, got %u\n", bi->bmiHeader.biSizeImage);

    /* the color table doesn't exist for 24-bit images */
    for (i = 0; i < 256; i++)
    {
        ok(bi->bmiColors[i].rgbRed == 0xAA && bi->bmiColors[i].rgbGreen == 0xAA &&
           bi->bmiColors[i].rgbBlue == 0xAA && bi->bmiColors[i].rgbReserved == 0xAA,
           "expected bmiColors[%d] 0xAA,0xAA,0xAA,0xAA - got %x %x %x %x\n", i,
           bi->bmiColors[i].rgbRed, bi->bmiColors[i].rgbGreen,
           bi->bmiColors[i].rgbBlue, bi->bmiColors[i].rgbReserved);
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
    HDC hdc;
    HBITMAP hbm;
    int ret;

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
        DWORD *bitmasks = (DWORD *)dibinfo->bmiColors;

        ok( dibinfo->bmiHeader.biCompression == BI_BITFIELDS,
            "compression is %u\n", dibinfo->bmiHeader.biCompression );

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
        ok( dibinfo->bmiHeader.biSizeImage != 0xdeadbeef ||
            broken(dibinfo->bmiHeader.biSizeImage == 0xdeadbeef), /* win9x */
            "size image not set\n" );

        /* now with bits and 0 lines */
        memset(dibinfo, 0, sizeof(dibinfo_buf));
        dibinfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        dibinfo->bmiHeader.biSizeImage = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        ret = GetDIBits(hdc, hbm, 0, 0, bits, dibinfo, DIB_RGB_COLORS);
        if (ret == 0 && GetLastError() == ERROR_INVALID_PARAMETER)
            win_skip("Win9x/WinMe doesn't handle 0 for the number of scan lines\n");
        else
        {
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
    else skip("not in 16 bpp BI_BITFIELDS mode, skipping that test\n");

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
        bytes = GetObject(hbm, sizeof(bm), &bm);
        ok(bytes == sizeof(bm), "GetObject returned %d\n", bytes);
        ok(bm.bmType == 0 ||
           broken(bm.bmType == 21072), /* win9x */
           "wrong bmType %d\n", bm.bmType);
        ok(bm.bmWidth == 10, "wrong bmWidth %d\n", bm.bmWidth);
        ok(bm.bmHeight == 10, "wrong bmHeight %d\n", bm.bmHeight);
        ok(bm.bmWidthBytes == BITMAP_GetWidthBytes(bm.bmWidth, bm.bmBitsPixel), "wrong bmWidthBytes %d\n", bm.bmWidthBytes);
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

    ret = GetObject(hbmp, 0, 0);
    ok_(__FILE__, line)(ret == sizeof(BITMAP) /* XP */ ||
                        ret == sizeof(DIBSECTION) /* Win9x */, "object size %d\n", ret);

    memset(&bm, 0xDA, sizeof(bm));
    SetLastError(0xdeadbeef);
    ret = GetObject(hbmp, sizeof(bm), &bm);
    if (!ret) /* XP, only for curObj2 */ return;
    ok_(__FILE__, line)(ret == sizeof(BITMAP) ||
                        ret == sizeof(DIBSECTION) /* Win9x, only for curObj2 */,
                        "GetObject returned %d, error %u\n", ret, GetLastError());
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
    ok(bm != curObj2 || /* WinXP */
       broken(bm == curObj2) /* Win9x */,
       "0: %p, curObj2 %p\n", bm, curObj2);
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
            ok(bm == 0 ||
               broken(bm != 0), /* Win9x and WinMe */
               "CreateBitmapIndirect for %d bpp succeeded\n", i);
            ok(error == ERROR_INVALID_PARAMETER ||
               broken(error == 0xdeadbeef), /* Win9x and WinME */
               "Got error %d, expected ERROR_INVALID_PARAMETER\n", error);
            DeleteObject(bm);
            continue;
        }
        ok(bm != 0, "CreateBitmapIndirect error %u\n", GetLastError());
        GetObject(bm, sizeof(bmp), &bmp);
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
        ok(bmp.bmBitsPixel == expect ||
           broken(bmp.bmBitsPixel == i), /* Win9x and WinMe */
           "CreateBitmapIndirect for a %d bpp bitmap created a %d bpp bitmap, expected %d\n",
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
    ok(ret != 0 ||
       broken(ret == 0), /* win9x */
       "GetDIBits failed got %d\n", ret);

    for (p = ((BYTE *) info) + sizeof(info->bmiHeader); (p - ((BYTE *) info)) < info_len; p++)
        if (*p != '!')
            overwritten_bytes++;
    ok(overwritten_bytes == 0, "GetDIBits wrote past the buffer given\n");

    HeapFree(GetProcessHeap(), 0, info);
    DeleteObject(hbmp);
    ReleaseDC(NULL, screen_dc);
}

static void test_GdiAlphaBlend(void)
{
    /* test out-of-bound parameters for GdiAlphaBlend */
    HDC hdcNull;

    HDC hdcDst;
    HBITMAP bmpDst;
    HBITMAP oldDst;

    BITMAPINFO bmi;
    HDC hdcSrc;
    HBITMAP bmpSrc;
    HBITMAP oldSrc;
    LPVOID bits;

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

    memset(&bmi, 0, sizeof(bmi));  /* as of Wine 0.9.44 we require the src to be a DIB section */
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biHeight = 20;
    bmi.bmiHeader.biWidth = 20;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmpSrc = CreateDIBSection(hdcDst, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
    ok(bmpSrc != NULL, "Couldn't create source bitmap\n");

    oldDst = SelectObject(hdcDst, bmpDst);
    oldSrc = SelectObject(hdcSrc, bmpSrc);

    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.SourceConstantAlpha = 128;
    blend.AlphaFormat = 0;

    expect_eq(pGdiAlphaBlend(hdcDst, 0, 0, 20, 20, hdcSrc, 0, 0, 20, 20, blend), TRUE, BOOL, "%d");
    SetLastError(0xdeadbeef);
    expect_eq(pGdiAlphaBlend(hdcDst, 0, 0, 20, 20, hdcSrc, -1, 0, 10, 10, blend), FALSE, BOOL, "%d");
    expect_eq(GetLastError(), ERROR_INVALID_PARAMETER, int, "%d");
    expect_eq(pGdiAlphaBlend(hdcDst, 0, 0, 20, 20, hdcSrc, 0, -1, 10, 10, blend), FALSE, BOOL, "%d");
    expect_eq(pGdiAlphaBlend(hdcDst, 0, 0, 20, 20, hdcSrc, 15, 0, 10, 10, blend), FALSE, BOOL, "%d");
    expect_eq(pGdiAlphaBlend(hdcDst, 0, 0, 20, 20, hdcSrc, 10, 10, -2, 3, blend), FALSE, BOOL, "%d");
    expect_eq(pGdiAlphaBlend(hdcDst, 0, 0, 20, 20, hdcSrc, 10, 10, -2, 3, blend), FALSE, BOOL, "%d");

    SetWindowOrgEx(hdcSrc, -10, -10, NULL);
    expect_eq(pGdiAlphaBlend(hdcDst, 0, 0, 20, 20, hdcSrc, -1, 0, 10, 10, blend), TRUE, BOOL, "%d");
    expect_eq(pGdiAlphaBlend(hdcDst, 0, 0, 20, 20, hdcSrc, 0, -1, 10, 10, blend), TRUE, BOOL, "%d");
    SetMapMode(hdcSrc, MM_ANISOTROPIC);
    ScaleWindowExtEx(hdcSrc, 10, 1, 10, 1, NULL);
    expect_eq(pGdiAlphaBlend(hdcDst, 0, 0, 20, 20, hdcSrc, -1, 0, 30, 30, blend), TRUE, BOOL, "%d");
    expect_eq(pGdiAlphaBlend(hdcDst, 0, 0, 20, 20, hdcSrc, 0, -1, 30, 30, blend), TRUE, BOOL, "%d");

    SelectObject(hdcDst, oldDst);
    SelectObject(hdcSrc, oldSrc);
    DeleteObject(bmpSrc);
    DeleteObject(bmpDst);
    DeleteDC(hdcDst);
    DeleteDC(hdcSrc);

    ReleaseDC(NULL, hdcNull);

}

static void test_clipping(void)
{
    HBITMAP bmpDst;
    HBITMAP oldDst;
    HBITMAP bmpSrc;
    HBITMAP oldSrc;
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
    oldDst = SelectObject( hdcDst, bmpDst );

    bmpSrc = CreateDIBSection( hdcSrc, &bmpinfo, DIB_RGB_COLORS, &bits, NULL, 0 );
    ok(bmpSrc != NULL, "Couldn't create source bitmap\n");
    oldSrc = SelectObject( hdcSrc, bmpSrc );

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

START_TEST(bitmap)
{
    HMODULE hdll;
    is_win9x = GetWindowLongPtrW(GetDesktopWindow(), GWLP_WNDPROC) == 0;

    hdll = GetModuleHandle("gdi32.dll");
    pGdiAlphaBlend = (void*)GetProcAddress(hdll, "GdiAlphaBlend");

    test_createdibitmap();
    test_dibsections();
    test_mono_dibsection();
    test_bitmap();
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
    test_GdiAlphaBlend();
    test_bitmapinfoheadersize();
    test_get16dibits();
    test_clipping();
}
