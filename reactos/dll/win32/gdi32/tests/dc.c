/*
 * Unit tests for dc functions
 *
 * Copyright (c) 2005 Huw Davies
 * Copyright (c) 2005 Dmitry Timoshkov
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

#define WINVER 0x0501 /* request latest DEVMODE */

#include <assert.h>
#include <stdio.h>

#include "wine/test.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"

static void dump_region(HRGN hrgn)
{
    DWORD i, size;
    RGNDATA *data = NULL;
    RECT *rect;

    if (!hrgn)
    {
        printf( "(null) region\n" );
        return;
    }
    if (!(size = GetRegionData( hrgn, 0, NULL ))) return;
    if (!(data = HeapAlloc( GetProcessHeap(), 0, size ))) return;
    GetRegionData( hrgn, size, data );
    printf( "%d rects:", data->rdh.nCount );
    for (i = 0, rect = (RECT *)data->Buffer; i < data->rdh.nCount; i++, rect++)
        printf( " (%d,%d)-(%d,%d)", rect->left, rect->top, rect->right, rect->bottom );
    printf( "\n" );
    HeapFree( GetProcessHeap(), 0, data );
}

static void test_savedc_2(void)
{
    HWND hwnd;
    HDC hdc;
    HRGN hrgn;
    RECT rc, rc_clip;
    int ret;

    hwnd = CreateWindowExA(0, "static", "", WS_POPUP, 0,0,100,100,
                           0, 0, 0, NULL);
    assert(hwnd != 0);
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    hrgn = CreateRectRgn(0, 0, 0, 0);
    assert(hrgn != 0);

    hdc = GetDC(hwnd);
    ok(hdc != NULL, "CreateDC rets %p\n", hdc);

    ret = GetClipBox(hdc, &rc_clip);
    ok(ret == SIMPLEREGION, "GetClipBox returned %d instead of SIMPLEREGION\n", ret);
    ret = GetClipRgn(hdc, hrgn);
    ok(ret == 0, "GetClipRgn returned %d instead of 0\n", ret);
    ret = GetRgnBox(hrgn, &rc);
    ok(ret == NULLREGION, "GetRgnBox returned %d (%d,%d-%d,%d) instead of NULLREGION\n",
       ret, rc.left, rc.top, rc.right, rc.bottom);
    /*dump_region(hrgn);*/
    SetRect(&rc, 0, 0, 100, 100);
    ok(EqualRect(&rc, &rc_clip),
       "rects are not equal: (%d,%d-%d,%d) - (%d,%d-%d,%d)\n",
       rc.left, rc.top, rc.right, rc.bottom,
       rc_clip.left, rc_clip.top, rc_clip.right, rc_clip.bottom);

    ret = SaveDC(hdc);
todo_wine
{
    ok(ret == 1, "ret = %d\n", ret);
}

    ret = IntersectClipRect(hdc, 0, 0, 50, 50);
    if (ret == COMPLEXREGION)
    {
        /* XP returns COMPLEXREGION although dump_region reports only 1 rect */
        trace("Windows BUG: IntersectClipRect returned %d instead of SIMPLEREGION\n", ret);
        /* let's make sure that it's a simple region */
        ret = GetClipRgn(hdc, hrgn);
        ok(ret == 1, "GetClipRgn returned %d instead of 1\n", ret);
        dump_region(hrgn);
    }
    else
        ok(ret == SIMPLEREGION, "IntersectClipRect returned %d instead of SIMPLEREGION\n", ret);

    ret = GetClipBox(hdc, &rc_clip);
    ok(ret == SIMPLEREGION, "GetClipBox returned %d instead of SIMPLEREGION\n", ret);
    SetRect(&rc, 0, 0, 50, 50);
    ok(EqualRect(&rc, &rc_clip), "rects are not equal\n");

    ret = RestoreDC(hdc, 1);
    ok(ret, "ret = %d\n", ret);

    ret = GetClipBox(hdc, &rc_clip);
    ok(ret == SIMPLEREGION, "GetClipBox returned %d instead of SIMPLEREGION\n", ret);
    SetRect(&rc, 0, 0, 100, 100);
    ok(EqualRect(&rc, &rc_clip), "rects are not equal\n");

    DeleteObject(hrgn);
    ReleaseDC(hwnd, hdc);
    DestroyWindow(hwnd);
}

static void test_savedc(void)
{
    HDC hdc = CreateDCA("DISPLAY", NULL, NULL, NULL);
    int ret;

    ok(hdc != NULL, "CreateDC rets %p\n", hdc);

    ret = SaveDC(hdc);
    ok(ret == 1, "ret = %d\n", ret);
    ret = SaveDC(hdc);
    ok(ret == 2, "ret = %d\n", ret);
    ret = SaveDC(hdc);
    ok(ret == 3, "ret = %d\n", ret);
    ret = RestoreDC(hdc, -1);
    ok(ret, "ret = %d\n", ret);
    ret = SaveDC(hdc);
    ok(ret == 3, "ret = %d\n", ret);
    ret = RestoreDC(hdc, 1);
    ok(ret, "ret = %d\n", ret);
    ret = SaveDC(hdc);
    ok(ret == 1, "ret = %d\n", ret);
    ret = SaveDC(hdc);
    ok(ret == 2, "ret = %d\n", ret);
    ret = SaveDC(hdc);
    ok(ret == 3, "ret = %d\n", ret);
    ret = RestoreDC(hdc, -2);
    ok(ret, "ret = %d\n", ret);
    ret = SaveDC(hdc);
    ok(ret == 2, "ret = %d\n", ret);
    ret = RestoreDC(hdc, -2);
    ok(ret, "ret = %d\n", ret);
    ret = SaveDC(hdc);
    ok(ret == 1, "ret = %d\n", ret);
    ret = SaveDC(hdc);
    ok(ret == 2, "ret = %d\n", ret); 
    ret = RestoreDC(hdc, -4);
    ok(!ret, "ret = %d\n", ret);
    ret = RestoreDC(hdc, 3);
    ok(!ret, "ret = %d\n", ret);

    /* Under win98 the following two succeed and both clear the save stack
    ret = RestoreDC(hdc, -3);
    ok(!ret, "ret = %d\n", ret);
    ret = RestoreDC(hdc, 0);
    ok(!ret, "ret = %d\n", ret);
    */

    ret = RestoreDC(hdc, 1);
    ok(ret, "ret = %d\n", ret);

    DeleteDC(hdc);
}

static void test_GdiConvertToDevmodeW(void)
{
    DEVMODEW * (WINAPI *pGdiConvertToDevmodeW)(const DEVMODEA *);
    DEVMODEA dmA;
    DEVMODEW *dmW;
    BOOL ret;

    pGdiConvertToDevmodeW = (void *)GetProcAddress(GetModuleHandleA("gdi32.dll"), "GdiConvertToDevmodeW");
    if (!pGdiConvertToDevmodeW)
    {
        win_skip("GdiConvertToDevmodeW is not available on this platform\n");
        return;
    }

    ret = EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &dmA);
    ok(ret, "EnumDisplaySettingsExA error %u\n", GetLastError());
    ok(dmA.dmSize >= FIELD_OFFSET(DEVMODEA, dmICMMethod), "dmSize is too small: %04x\n", dmA.dmSize);
    ok(dmA.dmSize <= sizeof(DEVMODEA), "dmSize is too large: %04x\n", dmA.dmSize);

    dmW = pGdiConvertToDevmodeW(&dmA);
    ok(dmW->dmSize >= FIELD_OFFSET(DEVMODEW, dmICMMethod), "dmSize is too small: %04x\n", dmW->dmSize);
    ok(dmW->dmSize <= sizeof(DEVMODEW), "dmSize is too large: %04x\n", dmW->dmSize);
    HeapFree(GetProcessHeap(), 0, dmW);

    dmA.dmSize = FIELD_OFFSET(DEVMODEA, dmFields) + sizeof(dmA.dmFields);
    dmW = pGdiConvertToDevmodeW(&dmA);
    ok(dmW->dmSize == FIELD_OFFSET(DEVMODEW, dmFields) + sizeof(dmW->dmFields),
       "wrong size %u\n", dmW->dmSize);
    HeapFree(GetProcessHeap(), 0, dmW);

    dmA.dmICMMethod = DMICMMETHOD_NONE;
    dmA.dmSize = FIELD_OFFSET(DEVMODEA, dmICMMethod) + sizeof(dmA.dmICMMethod);
    dmW = pGdiConvertToDevmodeW(&dmA);
    ok(dmW->dmSize == FIELD_OFFSET(DEVMODEW, dmICMMethod) + sizeof(dmW->dmICMMethod),
       "wrong size %u\n", dmW->dmSize);
    ok(dmW->dmICMMethod == DMICMMETHOD_NONE,
       "expected DMICMMETHOD_NONE, got %u\n", dmW->dmICMMethod);
    HeapFree(GetProcessHeap(), 0, dmW);

    dmA.dmSize = 1024;
    dmW = pGdiConvertToDevmodeW(&dmA);
    ok(dmW->dmSize == FIELD_OFFSET(DEVMODEW, dmPanningHeight) + sizeof(dmW->dmPanningHeight),
       "wrong size %u\n", dmW->dmSize);
    HeapFree(GetProcessHeap(), 0, dmW);

    SetLastError(0xdeadbeef);
    dmA.dmSize = 0;
    dmW = pGdiConvertToDevmodeW(&dmA);
    ok(!dmW, "GdiConvertToDevmodeW should fail\n");
    ok(GetLastError() == 0xdeadbeef, "expected 0xdeadbeef, got %u\n", GetLastError());

    /* this is the minimal dmSize that XP accepts */
    dmA.dmSize = FIELD_OFFSET(DEVMODEA, dmFields);
    dmW = pGdiConvertToDevmodeW(&dmA);
    ok(dmW->dmSize == FIELD_OFFSET(DEVMODEW, dmFields),
       "expected %04x, got %04x\n", FIELD_OFFSET(DEVMODEW, dmFields), dmW->dmSize);
    HeapFree(GetProcessHeap(), 0, dmW);
}

static void test_CreateCompatibleDC(void)
{
    BOOL bRet;
    HDC hDC;
    HDC hNewDC;

    /* Create a DC compatible with the screen */
    hDC = CreateCompatibleDC(NULL);
    ok(hDC != NULL, "CreateCompatibleDC returned %p\n", hDC);

    /* Delete this DC, this should succeed */
    bRet = DeleteDC(hDC);
    ok(bRet == TRUE, "DeleteDC returned %u\n", bRet);

    /* Try to create a DC compatible to the deleted DC. This has to fail */
    hNewDC = CreateCompatibleDC(hDC);
    ok(hNewDC == NULL, "CreateCompatibleDC returned %p\n", hNewDC);
}

static void test_DC_bitmap(void)
{
    HDC hdc, hdcmem;
    DWORD bits[64];
    HBITMAP hbmp, oldhbmp;
    COLORREF col;
    int i, bitspixel;

    /* fill bitmap data with b&w pattern */
    for( i = 0; i < 64; i++) bits[i] = i & 1 ? 0 : 0xffffff;

    hdc = GetDC(0);
    ok( hdc != NULL, "CreateDC rets %p\n", hdc);
    bitspixel = GetDeviceCaps( hdc, BITSPIXEL);
    /* create a memory dc */
    hdcmem = CreateCompatibleDC( hdc);
    ok( hdcmem != NULL, "CreateCompatibleDC rets %p\n", hdcmem);
    /* tests */
    /* test monochrome bitmap: should always work */
    hbmp = CreateBitmap(32, 32, 1, 1, bits);
    ok( hbmp != NULL, "CreateBitmap returns %p\n", hbmp);
    oldhbmp = SelectObject( hdcmem, hbmp);
    ok( oldhbmp != NULL, "SelectObject returned NULL\n" ); /* a memdc always has a bitmap selected */
    col = GetPixel( hdcmem, 0, 0);
    ok( col == 0xffffff, "GetPixel returned %08x, expected 00ffffff\n", col);
    col = GetPixel( hdcmem, 1, 1);
    ok( col == 0x000000, "GetPixel returned %08x, expected 00000000\n", col);
    col = GetPixel( hdcmem, 100, 1);
    ok( col == CLR_INVALID, "GetPixel returned %08x, expected ffffffff\n", col);
    SelectObject( hdcmem, oldhbmp);
    DeleteObject( hbmp);

    /* test with 2 bits color depth, not likely to succeed */
    hbmp = CreateBitmap(16, 16, 1, 2, bits);
    ok( hbmp != NULL, "CreateBitmap returns %p\n", hbmp);
    oldhbmp = SelectObject( hdcmem, hbmp);
    if( bitspixel != 2)
        ok( !oldhbmp, "SelectObject of a bitmap with 2 bits/pixel should return  NULL\n");
    if( oldhbmp) SelectObject( hdcmem, oldhbmp);
    DeleteObject( hbmp);

    /* test with 16 bits color depth, might succeed */
    hbmp = CreateBitmap(6, 6, 1, 16, bits);
    ok( hbmp != NULL, "CreateBitmap returns %p\n", hbmp);
    oldhbmp = SelectObject( hdcmem, hbmp);
    if( bitspixel == 16) {
        ok( oldhbmp != NULL, "SelectObject returned NULL\n" );
        col = GetPixel( hdcmem, 0, 0);
        ok( col == 0xffffff,
            "GetPixel of a bitmap with 16 bits/pixel returned %08x, expected 00ffffff\n", col);
        col = GetPixel( hdcmem, 1, 1);
        ok( col == 0x000000,
            "GetPixel of a bitmap with 16 bits/pixel returned returned %08x, expected 00000000\n", col);
    }
    if( oldhbmp) SelectObject( hdcmem, oldhbmp);
    DeleteObject( hbmp);

    /* test with 32 bits color depth, probably succeed */
    hbmp = CreateBitmap(4, 4, 1, 32, bits);
    ok( hbmp != NULL, "CreateBitmap returns %p\n", hbmp);
    oldhbmp = SelectObject( hdcmem, hbmp);
    if( bitspixel == 32) {
        ok( oldhbmp != NULL, "SelectObject returned NULL\n" );
        col = GetPixel( hdcmem, 0, 0);
        ok( col == 0xffffff,
            "GetPixel of a bitmap with 32 bits/pixel returned %08x, expected 00ffffff\n", col);
        col = GetPixel( hdcmem, 1, 1);
        ok( col == 0x000000,
            "GetPixel of a bitmap with 32 bits/pixel returned returned %08x, expected 00000000\n", col);
    }
    if( oldhbmp) SelectObject( hdcmem, oldhbmp);
    DeleteObject( hbmp);
    ReleaseDC( 0, hdc );
}

START_TEST(dc)
{
    test_savedc();
    test_savedc_2();
    test_GdiConvertToDevmodeW();
    test_CreateCompatibleDC();
    test_DC_bitmap();
}
