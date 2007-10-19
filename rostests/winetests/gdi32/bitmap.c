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
#include "wingdi.h"
#include "winuser.h"
#include "mmsystem.h"

#include "wine/test.h"

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
    INT ret, width_bytes;
    char buf[512], buf_cmp[512];

    ret = GetObject(hbm, sizeof(bm), &bm);
    ok(ret == sizeof(bm), "GetObject returned %d\n", ret);

    ok(bm.bmType == 0, "wrong bm.bmType %d\n", bm.bmType);
    ok(bm.bmWidth == bmih->biWidth, "wrong bm.bmWidth %d\n", bm.bmWidth);
    ok(bm.bmHeight == bmih->biHeight, "wrong bm.bmHeight %d\n", bm.bmHeight);
    width_bytes = BITMAP_GetWidthBytes(bm.bmWidth, bm.bmBitsPixel);
    ok(bm.bmWidthBytes == width_bytes, "wrong bm.bmWidthBytes %d != %d\n", bm.bmWidthBytes, width_bytes);
    ok(bm.bmPlanes == bmih->biPlanes, "wrong bm.bmPlanes %d\n", bm.bmPlanes);
    ok(bm.bmBitsPixel == expected_depth, "wrong bm.bmBitsPixel %d != %d\n", bm.bmBitsPixel, expected_depth);
    ok(bm.bmBits == NULL, "wrong bm.bmBits %p\n", bm.bmBits);

    assert(sizeof(buf) >= bm.bmWidthBytes * bm.bmHeight);
    assert(sizeof(buf) == sizeof(buf_cmp));

    ret = GetBitmapBits(hbm, 0, NULL);
    ok(ret == bm.bmWidthBytes * bm.bmHeight, "%d != %d\n", ret, bm.bmWidthBytes * bm.bmHeight);

    memset(buf_cmp, 0xAA, sizeof(buf_cmp));
    memset(buf_cmp, 0, bm.bmWidthBytes * bm.bmHeight);

    memset(buf, 0xAA, sizeof(buf));
    ret = GetBitmapBits(hbm, sizeof(buf), buf);
    ok(ret == bm.bmWidthBytes * bm.bmHeight, "%d != %d\n", ret, bm.bmWidthBytes * bm.bmHeight);
    ok(!memcmp(buf, buf_cmp, sizeof(buf)), "buffers do not match\n");

    /* test various buffer sizes for GetObject */
    ret = GetObject(hbm, 0, NULL);
    ok(ret == sizeof(bm), "wrong size %d\n", ret);

    ret = GetObject(hbm, sizeof(bm) * 2, &bm);
    ok(ret == sizeof(bm), "wrong size %d\n", ret);

    ret = GetObject(hbm, sizeof(bm) / 2, &bm);
    ok(ret == 0, "%d != 0\n", ret);

    ret = GetObject(hbm, 0, &bm);
    ok(ret == 0, "%d != 0\n", ret);

    ret = GetObject(hbm, 1, &bm);
    ok(ret == 0, "%d != 0\n", ret);
}

static void test_createdibitmap(void)
{
    HDC hdc, hdcmem;
    BITMAPINFOHEADER bmih;
    HBITMAP hbm, hbm_colour, hbm_old;
    INT screen_depth;

    hdc = GetDC(0);
    screen_depth = GetDeviceCaps(hdc, BITSPIXEL);
    memset(&bmih, 0, sizeof(bmih));
    bmih.biSize = sizeof(bmih);
    bmih.biWidth = 10;
    bmih.biHeight = 10;
    bmih.biPlanes = 1;
    bmih.biBitCount = 32;
    bmih.biCompression = BI_RGB;

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
    hbm_colour = CreateCompatibleBitmap(hdc, 1, 1);
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
    DIBSECTION ds;
    INT ret, width_bytes;
    BYTE *buf;

    ret = GetObject(hbm, sizeof(bm), &bm);
    ok(ret == sizeof(bm), "GetObject returned %d\n", ret);

    ok(bm.bmType == 0, "wrong bm.bmType %d\n", bm.bmType);
    ok(bm.bmWidth == bmih->biWidth, "wrong bm.bmWidth %d\n", bm.bmWidth);
    ok(bm.bmHeight == bmih->biHeight, "wrong bm.bmHeight %d\n", bm.bmHeight);
    width_bytes = DIB_GetWidthBytes(bm.bmWidth, bm.bmBitsPixel);
    ok(bm.bmWidthBytes == width_bytes, "wrong bm.bmWidthBytes %d != %d\n", bm.bmWidthBytes, width_bytes);
    ok(bm.bmPlanes == bmih->biPlanes, "wrong bm.bmPlanes %d\n", bm.bmPlanes);
    ok(bm.bmBitsPixel == bmih->biBitCount, "bm.bmBitsPixel %d != %d\n", bm.bmBitsPixel, bmih->biBitCount);
    ok(bm.bmBits == bits, "wrong bm.bmBits %p != %p\n", bm.bmBits, bits);

    buf = HeapAlloc(GetProcessHeap(), 0, bm.bmWidthBytes * bm.bmHeight + 4096);

    width_bytes = BITMAP_GetWidthBytes(bm.bmWidth, bm.bmBitsPixel);

    /* GetBitmapBits returns not 32-bit aligned data */
    ret = GetBitmapBits(hbm, 0, NULL);
    ok(ret == width_bytes * bm.bmHeight, "%d != %d\n", ret, width_bytes * bm.bmHeight);

    memset(buf, 0xAA, bm.bmWidthBytes * bm.bmHeight + 4096);
    ret = GetBitmapBits(hbm, bm.bmWidthBytes * bm.bmHeight + 4096, buf);
    ok(ret == width_bytes * bm.bmHeight, "%d != %d\n", ret, width_bytes * bm.bmHeight);

    HeapFree(GetProcessHeap(), 0, buf);

    /* test various buffer sizes for GetObject */
    memset(&ds, 0xAA, sizeof(ds));
    ret = GetObject(hbm, sizeof(bm) * 2, &bm);
    ok(ret == sizeof(bm), "wrong size %d\n", ret);
    ok(bm.bmWidth == bmih->biWidth, "wrong bm.bmWidth %d\n", bm.bmWidth);
    ok(bm.bmHeight == bmih->biHeight, "wrong bm.bmHeight %d\n", bm.bmHeight);
    ok(bm.bmBits == bits, "wrong bm.bmBits %p != %p\n", bm.bmBits, bits);

    ret = GetObject(hbm, sizeof(bm) / 2, &bm);
    ok(ret == 0, "%d != 0\n", ret);

    ret = GetObject(hbm, 0, &bm);
    ok(ret == 0, "%d != 0\n", ret);

    ret = GetObject(hbm, 1, &bm);
    ok(ret == 0, "%d != 0\n", ret);

    /* test various buffer sizes for GetObject */
    ret = GetObject(hbm, 0, NULL);
    ok(ret == sizeof(bm), "wrong size %d\n", ret);

    memset(&ds, 0xAA, sizeof(ds));
    ret = GetObject(hbm, sizeof(ds) * 2, &ds);
    ok(ret == sizeof(ds), "wrong size %d\n", ret);

    ok(ds.dsBm.bmBits == bits, "wrong bm.bmBits %p != %p\n", ds.dsBm.bmBits, bits);
    ok(ds.dsBmih.biSizeImage == ds.dsBm.bmWidthBytes * ds.dsBm.bmHeight, "%lu != %u\n",
       ds.dsBmih.biSizeImage, ds.dsBm.bmWidthBytes * ds.dsBm.bmHeight);
    ok(bmih->biSizeImage == 0, "%lu != 0\n", bmih->biSizeImage);
    ds.dsBmih.biSizeImage = 0;

    ok(ds.dsBmih.biSize == bmih->biSize, "%lu != %lu\n", ds.dsBmih.biSize, bmih->biSize);
    ok(ds.dsBmih.biWidth == bmih->biWidth, "%lu != %lu\n", ds.dsBmih.biWidth, bmih->biWidth);
    ok(ds.dsBmih.biHeight == bmih->biHeight, "%lu != %lu\n", ds.dsBmih.biHeight, bmih->biHeight);
    ok(ds.dsBmih.biPlanes == bmih->biPlanes, "%u != %u\n", ds.dsBmih.biPlanes, bmih->biPlanes);
    ok(ds.dsBmih.biBitCount == bmih->biBitCount, "%u != %u\n", ds.dsBmih.biBitCount, bmih->biBitCount);
    ok(ds.dsBmih.biCompression == bmih->biCompression, "%lu != %lu\n", ds.dsBmih.biCompression, bmih->biCompression);
    ok(ds.dsBmih.biSizeImage == bmih->biSizeImage, "%lu != %lu\n", ds.dsBmih.biSizeImage, bmih->biSizeImage);
    ok(ds.dsBmih.biXPelsPerMeter == bmih->biXPelsPerMeter, "%lu != %lu\n", ds.dsBmih.biXPelsPerMeter, bmih->biXPelsPerMeter);
    ok(ds.dsBmih.biYPelsPerMeter == bmih->biYPelsPerMeter, "%lu != %lu\n", ds.dsBmih.biYPelsPerMeter, bmih->biYPelsPerMeter);

    memset(&ds, 0xAA, sizeof(ds));
    ret = GetObject(hbm, sizeof(ds) - 4, &ds);
    ok(ret == sizeof(ds.dsBm), "wrong size %d\n", ret);
    ok(ds.dsBm.bmWidth == bmih->biWidth, "%lu != %lu\n", ds.dsBmih.biWidth, bmih->biWidth);
    ok(ds.dsBm.bmHeight == bmih->biHeight, "%lu != %lu\n", ds.dsBmih.biHeight, bmih->biHeight);
    ok(ds.dsBm.bmBits == bits, "%p != %p\n", ds.dsBm.bmBits, bits);

    ret = GetObject(hbm, 0, &ds);
    ok(ret == 0, "%d != 0\n", ret);

    ret = GetObject(hbm, 1, &ds);
    ok(ret == 0, "%d != 0\n", ret);
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
    hdib = CreateDIBSection(hdc, pbmi, DIB_RGB_COLORS, (void**)&bits, NULL, 0);
    ok(hdib != NULL, "CreateDIBSection error %ld\n", GetLastError());
    ok(GetObject(hdib, sizeof(DIBSECTION), &dibsec) != 0, "GetObject failed for DIBSection\n");
    ok(dibsec.dsBm.bmBits == bits, "dibsec.dsBits %p != bits %p\n", dibsec.dsBm.bmBits, bits);

    /* test the DIB memory */
    ok(VirtualQuery(bits, &info, sizeof(info)) == sizeof(info),
        "VirtualQuery failed\n");
    ok(info.BaseAddress == bits, "%p != %p\n", info.BaseAddress, bits);
    ok(info.AllocationBase == bits, "%p != %p\n", info.AllocationBase, bits);
    ok(info.AllocationProtect == PAGE_READWRITE, "%lx != PAGE_READWRITE\n", info.AllocationProtect);
    ok(info.RegionSize == 0x26000, "0x%lx != 0x26000\n", info.RegionSize);
    ok(info.State == MEM_COMMIT, "%lx != MEM_COMMIT\n", info.State);
    ok(info.Protect == PAGE_READWRITE, "%lx != PAGE_READWRITE\n", info.Protect);
    ok(info.Type == MEM_PRIVATE, "%lx != MEM_PRIVATE\n", info.Type);

    test_dib_info(hdib, bits, &pbmi->bmiHeader);
    DeleteObject(hdib);

    pbmi->bmiHeader.biBitCount = 8;
    pbmi->bmiHeader.biCompression = BI_RLE8;
    SetLastError(0xdeadbeef);
    hdib = CreateDIBSection(hdc, pbmi, DIB_RGB_COLORS, (void**)&bits, NULL, 0);
    ok(hdib == NULL, "CreateDIBSection should fail when asked to create a compressed DIB section\n");
    ok(GetLastError() == 0xdeadbeef, "wrong error %ld\n", GetLastError());

    pbmi->bmiHeader.biBitCount = 16;
    pbmi->bmiHeader.biCompression = BI_BITFIELDS;
    ((PDWORD)pbmi->bmiColors)[0] = 0xf800;
    ((PDWORD)pbmi->bmiColors)[1] = 0x07e0;
    ((PDWORD)pbmi->bmiColors)[2] = 0x001f;
    SetLastError(0xdeadbeef);
    hdib = CreateDIBSection(hdc, pbmi, DIB_RGB_COLORS, (void**)&bits, NULL, 0);
    ok(hdib != NULL, "CreateDIBSection error %ld\n", GetLastError());

    /* test the DIB memory */
    ok(VirtualQuery(bits, &info, sizeof(info)) == sizeof(info),
        "VirtualQuery failed\n");
    ok(info.BaseAddress == bits, "%p != %p\n", info.BaseAddress, bits);
    ok(info.AllocationBase == bits, "%p != %p\n", info.AllocationBase, bits);
    ok(info.AllocationProtect == PAGE_READWRITE, "%lx != PAGE_READWRITE\n", info.AllocationProtect);
    ok(info.RegionSize == 0x19000, "0x%lx != 0x19000\n", info.RegionSize);
    ok(info.State == MEM_COMMIT, "%lx != MEM_COMMIT\n", info.State);
    ok(info.Protect == PAGE_READWRITE, "%lx != PAGE_READWRITE\n", info.Protect);
    ok(info.Type == MEM_PRIVATE, "%lx != MEM_PRIVATE\n", info.Type);

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
        "created DIBSection: wrong biClrUsed field: %lu, should be: %u\n", dibsec.dsBmih.biClrUsed, 2);

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
       "created DIBSection: wrong biClrUsed field: %lu, should be: %u\n", dibsec.dsBmih.biClrUsed, 16);
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
        "created DIBSection: wrong biClrUsed field: %lu, should be: %u\n", dibsec.dsBmih.biClrUsed, 256);

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
    ok(dibsec.dsBmih.biClrUsed == 2,
        "created DIBSection: wrong biClrUsed field: %lu, should be: %u\n", dibsec.dsBmih.biClrUsed, 2);

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

    ok(bits32[0] == 0xff00, "lower left pixel is %08lx\n", bits32[0]);
    ok(bits32[17] == 0xff00ff, "bottom but one, left pixel is %08lx\n", bits32[17]);

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
    ok(dibsec.dsBmih.biClrUsed == 256,
        "created DIBSection: wrong biClrUsed field: %lu, should be: %u\n", dibsec.dsBmih.biClrUsed, 256);

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

void test_mono_dibsection(void)
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
    INT ret;

    hdc = CreateCompatibleDC(0);
    assert(hdc != 0);

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
    ok(ret == bm.bmWidthBytes * bm.bmHeight, "%d != %d\n", ret, bm.bmWidthBytes * bm.bmHeight);

    memset(buf_cmp, 0xAA, sizeof(buf_cmp));
    memset(buf_cmp, 0, bm.bmWidthBytes * bm.bmHeight);

    memset(buf, 0xAA, sizeof(buf));
    ret = GetBitmapBits(hbmp, sizeof(buf), buf);
    ok(ret == bm.bmWidthBytes * bm.bmHeight, "%d != %d\n", ret, bm.bmWidthBytes * bm.bmHeight);
    ok(!memcmp(buf, buf_cmp, sizeof(buf)), "buffers do not match\n");

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
    ok(!memcmp(buf, buf_cmp, sizeof(buf)), "buffers do not match\n");

    hbmp_old = SelectObject(hdc, hbmp_old);
    ok(hbmp_old == hbmp, "wrong old bitmap %p\n", hbmp_old);

    /* test various buffer sizes for GetObject */
    ret = GetObject(hbmp, sizeof(bm) * 2, &bm);
    ok(ret == sizeof(bm), "wrong size %d\n", ret);

    ret = GetObject(hbmp, sizeof(bm) / 2, &bm);
    ok(ret == 0, "%d != 0\n", ret);

    ret = GetObject(hbmp, 0, &bm);
    ok(ret == 0, "%d != 0\n", ret);

    ret = GetObject(hbmp, 1, &bm);
    ok(ret == 0, "%d != 0\n", ret);

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

    info  = (BITMAPINFO *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BITMAPINFOHEADER) + (1 << bpp) * sizeof(RGBQUAD));
    info2 = (BITMAPINFO *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BITMAPINFOHEADER) + (1 << bpp) * sizeof(RGBQUAD));
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

    for (i=0; i < (1 << bpp); i++)
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
    old_bmp = (HBITMAP) SelectObject(dib_dc, dib);
    dc = CreateCompatibleDC(NULL);
    bits2 = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dib_size);
    assert(bits2);

    /* Copy the DIB attributes but not the color table */
    memcpy(info2, info, sizeof(BITMAPINFOHEADER));

    res = GetDIBits(dc, dib, 0, info->bmiHeader.biHeight, bits2, info2, DIB_RGB_COLORS);
    ok(res, "GetDIBits failed\n");

    /* Compare the color table and the bits */
    equalContents = TRUE;
    for (i=0; i < (1 << bpp); i++)
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
    todo_wine
    {
        ok(equalContents, "GetDIBits with DIB selected in DC: Invalid DIB color table\n");
    }

    equalContents = TRUE;
    for (i=0; i < dib_size / sizeof(DWORD); i++)
    {
        if (((DWORD *)bits)[i] != ((DWORD *)bits2)[i])
        {
            equalContents = FALSE;
            break;
        }
    }
    todo_wine
    {
        ok(equalContents, "GetDIBits with DIB selected in DC: Invalid DIB bits\n");
    }

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
    old_bmp = (HBITMAP) SelectObject(ddb_dc, ddb);
    for (i = 0; i < width; i++)
    {
        for (j=0; j < height; j++)
        {
            BYTE c = (i * width + j) % 256;
            SetPixelV(ddb_dc, i, j, RGB(c, c, c));
        }
    }
    SelectObject(ddb_dc, old_bmp);

    info  = (BITMAPINFO *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));
    info2 = (BITMAPINFO *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));
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
    old_bmp = (HBITMAP) SelectObject(ddb_dc, ddb);

    /* Get the bits */
    res = GetDIBits(dc, ddb, 0, height, bits2, info2, DIB_RGB_COLORS);
    ok(res, "GetDIBits failed\n");

    /* Compare the color table and the bits */
    if (bpp <= 8)
    {
        equalContents = TRUE;
        for (i=0; i < (1 << bpp); i++)
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

    HeapFree(GetProcessHeap(), 0, bits2);
    HeapFree(GetProcessHeap(), 0, bits);
    DeleteDC(dc);

    SelectObject(ddb_dc, old_bmp);
    DeleteDC(ddb_dc);
    DeleteObject(ddb);

    HeapFree(GetProcessHeap(), 0, info2);
    HeapFree(GetProcessHeap(), 0, info);
}

START_TEST(bitmap)
{
    is_win9x = GetWindowLongPtrW(GetDesktopWindow(), GWLP_WNDPROC) == 0;

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
}
