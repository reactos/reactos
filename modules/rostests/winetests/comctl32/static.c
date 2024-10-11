/* Unit test suite for static controls.
 *
 * Copyright 2007 Google (Mikolaj Zalewski)
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
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#define OEMRESOURCE
#include "winuser.h"
#include "commctrl.h"
#include "resources.h"

#include "wine/test.h"

#include "v6util.h"

#define CTRL_ID 1995

static HWND hMainWnd;
static int g_nReceivedColorStatic;

/* try to make sure pending X events have been processed before continuing */
static void flush_events(void)
{
    MSG msg;
    int diff = 200;
    int min_timeout = 100;
    DWORD time = GetTickCount() + diff;

    while (diff > 0)
    {
        if (MsgWaitForMultipleObjects( 0, NULL, FALSE, min_timeout, QS_ALLINPUT ) == WAIT_TIMEOUT) break;
        while (PeekMessageA( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );
        diff = time - GetTickCount();
    }
}

static HWND create_static(DWORD style)
{
    return CreateWindowA(WC_STATICA, "Test", WS_VISIBLE|WS_CHILD|style, 5, 5, 100, 100, hMainWnd, (HMENU)CTRL_ID, NULL, 0);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_CTLCOLORSTATIC:
        {
            HDC hdc = (HDC)wparam;
            HRGN hrgn = CreateRectRgn(0, 0, 1, 1);
            ok(GetClipRgn(hdc, hrgn) == 1, "Static controls during a WM_CTLCOLORSTATIC must have a clipping region\n");
            DeleteObject(hrgn);
            g_nReceivedColorStatic++;
            return (LRESULT) GetStockObject(BLACK_BRUSH);
        }
        break;
    }

    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static void test_updates(int style)
{
    HWND hStatic = create_static(style);
    RECT r1 = {5, 5, 30, 30}, rcClient;
    int exp;
    LONG exstyle;

    flush_events();
    trace("Testing style 0x%x\n", style);

    exstyle = GetWindowLongW(hStatic, GWL_EXSTYLE);
    if (style == SS_ETCHEDHORZ || style == SS_ETCHEDVERT || style == SS_SUNKEN)
        ok(exstyle == WS_EX_STATICEDGE, "expected WS_EX_STATICEDGE, got %ld\n", exstyle);
    else
        ok(exstyle == 0, "expected 0, got %ld\n", exstyle);

    GetClientRect(hStatic, &rcClient);
    if (style == SS_ETCHEDVERT)
        ok(rcClient.right == 0, "expected zero width, got %ld\n", rcClient.right);
    else
        ok(rcClient.right > 0, "expected non-zero width, got %ld\n", rcClient.right);
    if (style == SS_ETCHEDHORZ)
        ok(rcClient.bottom == 0, "expected zero height, got %ld\n", rcClient.bottom);
    else
        ok(rcClient.bottom > 0, "expected non-zero height, got %ld\n", rcClient.bottom);

    g_nReceivedColorStatic = 0;
    /* during each update parent WndProc will test the WM_CTLCOLORSTATIC message */
    InvalidateRect(hMainWnd, NULL, FALSE);
    UpdateWindow(hMainWnd);
    InvalidateRect(hMainWnd, &r1, FALSE);
    UpdateWindow(hMainWnd);
    InvalidateRect(hStatic, &r1, FALSE);
    UpdateWindow(hStatic);
    InvalidateRect(hStatic, NULL, FALSE);
    UpdateWindow(hStatic);

    if ((style & SS_TYPEMASK) == SS_BITMAP)
    {
        HDC hdc = GetDC(hStatic);
        COLORREF colour = GetPixel(hdc, 10, 10);
        ok(colour == 0, "Unexpected pixel color.\n");
        ReleaseDC(hStatic, hdc);
    }

    if (style != SS_ETCHEDHORZ && style != SS_ETCHEDVERT)
        exp = 4;
    else
        exp = 2; /* SS_ETCHEDHORZ/SS_ETCHEDVERT have empty client rect so WM_CTLCOLORSTATIC is sent only when parent window is invalidated */

    ok(g_nReceivedColorStatic == exp, "expected %u, got %u\n", exp, g_nReceivedColorStatic);
    DestroyWindow(hStatic);
}

static void test_set_text(void)
{
    HWND hStatic = create_static(SS_SIMPLE);
    char buffA[10];

    GetWindowTextA(hStatic, buffA, sizeof(buffA));
    ok(!strcmp(buffA, "Test"), "got wrong text %s\n", buffA);

    SetWindowTextA(hStatic, NULL);
    GetWindowTextA(hStatic, buffA, sizeof(buffA));
    ok(buffA[0] == 0, "got wrong text %s\n", buffA);

    DestroyWindow(hStatic);
}

static void remove_alpha(HBITMAP hbitmap)
{
    BITMAP bm;
    BYTE *ptr;
    int i;

    GetObjectW(hbitmap, sizeof(bm), &bm);
    ok(bm.bmBits != NULL, "bmBits is NULL\n");

    for (i = 0, ptr = bm.bmBits; i < bm.bmWidth * bm.bmHeight; i++, ptr += 4)
        ptr[3] = 0;
}

static void test_image(HBITMAP image, BOOL is_dib, BOOL is_premult, BOOL is_alpha)
{
    BITMAP bm;
    HDC hdc;
    BITMAPINFO info;
    BYTE bits[4];

    GetObjectW(image, sizeof(bm), &bm);
    ok(bm.bmWidth == 1, "got %d\n", bm.bmWidth);
    ok(bm.bmHeight == 1, "got %d\n", bm.bmHeight);
    ok(bm.bmBitsPixel == 32, "got %d\n", bm.bmBitsPixel);
    if (is_dib)
    {
        ok(bm.bmBits != NULL, "bmBits is NULL\n");
        memcpy(bits, bm.bmBits, 4);
        if (is_premult)
            ok(bits[0] == 0x05 &&  bits[1] == 0x09 &&  bits[2] == 0x0e && bits[3] == 0x44,
               "bits: %02x %02x %02x %02x\n", bits[0], bits[1], bits[2], bits[3]);
        else if (is_alpha)
            ok(bits[0] == 0x11 &&  bits[1] == 0x22 &&  bits[2] == 0x33 && bits[3] == 0x44,
               "bits: %02x %02x %02x %02x\n", bits[0], bits[1], bits[2], bits[3]);
        else
            ok(bits[0] == 0x11 &&  bits[1] == 0x22 &&  bits[2] == 0x33 && bits[3] == 0,
               "bits: %02x %02x %02x %02x\n", bits[0], bits[1], bits[2], bits[3]);
    }
    else
        ok(bm.bmBits == NULL, "bmBits is not NULL\n");

    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = bm.bmWidth;
    info.bmiHeader.biHeight = bm.bmHeight;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    info.bmiHeader.biSizeImage = 4;
    info.bmiHeader.biXPelsPerMeter = 0;
    info.bmiHeader.biYPelsPerMeter = 0;
    info.bmiHeader.biClrUsed = 0;
    info.bmiHeader.biClrImportant = 0;

    hdc = CreateCompatibleDC(0);
    GetDIBits(hdc, image, 0, bm.bmHeight, bits, &info, DIB_RGB_COLORS);
    DeleteDC(hdc);

    if (is_premult)
        ok(bits[0] == 0x05 &&  bits[1] == 0x09 &&  bits[2] == 0x0e && bits[3] == 0x44,
           "bits: %02x %02x %02x %02x\n", bits[0], bits[1], bits[2], bits[3]);
    else if (is_alpha)
        ok(bits[0] == 0x11 &&  bits[1] == 0x22 &&  bits[2] == 0x33 && bits[3] == 0x44,
           "bits: %02x %02x %02x %02x\n", bits[0], bits[1], bits[2], bits[3]);
    else
        ok(bits[0] == 0x11 &&  bits[1] == 0x22 &&  bits[2] == 0x33 && bits[3] == 0,
           "bits: %02x %02x %02x %02x\n", bits[0], bits[1], bits[2], bits[3]);
}

static void test_set_image(void)
{
    char resource[4];
    WCHAR resource_unicode[3];
    HWND hwnd = create_static(SS_BITMAP);
    HBITMAP bmp1, bmp2, image;

    image = LoadImageW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDB_BITMAP_1x1_32BPP), IMAGE_BITMAP, 0, 0, 0);
    ok(image != NULL, "LoadImage failed\n");

    test_image(image, FALSE, FALSE, TRUE);

    bmp1 = (HBITMAP)SendMessageW(hwnd, STM_GETIMAGE, IMAGE_BITMAP, 0);
    ok(bmp1 == NULL, "got not NULL\n");

    bmp1 = (HBITMAP)SendMessageW(hwnd, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)image);
    ok(bmp1 == NULL, "got not NULL\n");

    bmp1 = (HBITMAP)SendMessageW(hwnd, STM_GETIMAGE, IMAGE_BITMAP, 0);
    ok(bmp1 != NULL, "got NULL\n");
    ok(bmp1 != image, "bmp == image\n");
    test_image(bmp1, TRUE, TRUE, TRUE);

    bmp2 = (HBITMAP)SendMessageW(hwnd, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)image);
    ok(bmp2 != NULL, "got NULL\n");
    ok(bmp2 != image, "bmp == image\n");
    ok(bmp1 == bmp2, "bmp1 != bmp2\n");
    test_image(bmp2, TRUE, TRUE, TRUE);

    DeleteObject(image);

    image = LoadImageW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDB_BITMAP_1x1_32BPP), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    ok(image != NULL, "LoadImage failed\n");

    test_image(image, TRUE, FALSE, TRUE);
    remove_alpha(image);
    test_image(image, TRUE, FALSE, FALSE);

    bmp1 = (HBITMAP)SendMessageW(hwnd, STM_GETIMAGE, IMAGE_BITMAP, 0);
    ok(bmp1 != NULL, "got NULL\n");
    ok(bmp1 != image, "bmp == image\n");
    test_image(bmp1, TRUE, TRUE, TRUE);

    bmp2 = (HBITMAP)SendMessageW(hwnd, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)image);
    ok(bmp2 != NULL, "got NULL\n");
    ok(bmp2 != image, "bmp == image\n");
    ok(bmp1 == bmp2, "bmp1 != bmp2\n");
    test_image(bmp1, TRUE, TRUE, FALSE);

    bmp2 = (HBITMAP)SendMessageW(hwnd, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)image);
    ok(bmp2 != NULL, "got NULL\n");
    ok(bmp2 == image, "bmp1 != image\n");
    test_image(bmp2, TRUE, FALSE, FALSE);

    DestroyWindow(hwnd);

    test_image(image, TRUE, FALSE, FALSE);

    resource[0] = '\xff';
    resource[1] = PtrToUlong(MAKEINTRESOURCEW(IDB_BITMAP_1x1_32BPP));
    resource[2] = PtrToUlong(MAKEINTRESOURCEW(IDB_BITMAP_1x1_32BPP)) >> 8;
    resource[3] = 0;

    resource_unicode[0] = 0xffff;
    resource_unicode[1] = PtrToUlong(MAKEINTRESOURCEW(IDB_BITMAP_1x1_32BPP));
    resource_unicode[2] = 0;

    hwnd = CreateWindowW(L"Static", resource_unicode, WS_VISIBLE|WS_CHILD|SS_BITMAP, 5, 5, 100, 100,
                         hMainWnd, (HMENU)CTRL_ID, GetModuleHandleW(NULL), 0);

    bmp1 = (HBITMAP)SendMessageW(hwnd, STM_GETIMAGE, IMAGE_BITMAP, 0);
    ok(bmp1 != NULL, "got NULL\n");
    ok(bmp1 != image, "bmp == image\n");
    test_image(bmp1, TRUE, TRUE, TRUE);
    DestroyWindow(hwnd);

    hwnd = CreateWindowA("Static", resource, WS_VISIBLE|WS_CHILD|SS_BITMAP, 5, 5, 100, 100,
                         hMainWnd, (HMENU)CTRL_ID, GetModuleHandleW(NULL), 0);

    bmp1 = (HBITMAP)SendMessageA(hwnd, STM_GETIMAGE, IMAGE_BITMAP, 0);
    ok(bmp1 != NULL, "got NULL\n");
    ok(bmp1 != image, "bmp == image\n");
    test_image(bmp1, TRUE, TRUE, TRUE);
    DestroyWindow(hwnd);

    DeleteObject(image);
}

static void test_STM_SETIMAGE(void)
{
    DWORD type;
    HWND hwnd;
    HICON icon, old_image;
    HBITMAP bmp;
    HENHMETAFILE emf;
    HDC dc;

    icon = LoadIconW(0, (LPCWSTR)IDI_APPLICATION);
    bmp = LoadBitmapW(0, (LPCWSTR)OBM_CLOSE);
    dc = CreateEnhMetaFileW(0, NULL, NULL, NULL);
    LineTo(dc, 1, 1);
    emf = CloseEnhMetaFile(dc);
    DeleteDC(dc);

    for (type = SS_LEFT; type < SS_ETCHEDFRAME; type++)
    {
        winetest_push_context("%lu", type);

        hwnd = create_static(type);
        ok(hwnd != 0, "failed to create static type %#lx\n", type);

        /* set icon */
        g_nReceivedColorStatic = 0;
        old_image = (HICON)SendMessageW(hwnd, STM_SETIMAGE, IMAGE_ICON, (LPARAM)icon);
        ok(!old_image, "got %p\n", old_image);
        if (type == SS_ICON)
            ok(g_nReceivedColorStatic, "Unexpected WM_CTLCOLORSTATIC value %d\n", g_nReceivedColorStatic);
        else
            ok(!g_nReceivedColorStatic, "Unexpected WM_CTLCOLORSTATIC value %d\n", g_nReceivedColorStatic);

        g_nReceivedColorStatic = 0;
        old_image = (HICON)SendMessageW(hwnd, STM_SETIMAGE, IMAGE_ICON, (LPARAM)icon);
        if (type == SS_ICON)
        {
            ok(old_image != 0, "got %p\n", old_image);
            ok(g_nReceivedColorStatic, "Unexpected WM_CTLCOLORSTATIC value %d\n", g_nReceivedColorStatic);
        }
        else
        {
            ok(!old_image, "got %p\n", old_image);
            ok(!g_nReceivedColorStatic, "Unexpected WM_CTLCOLORSTATIC value %d\n", g_nReceivedColorStatic);
        }

        g_nReceivedColorStatic = 0;
        old_image = (HICON)SendMessageW(hwnd, STM_SETICON, (WPARAM)icon, 0);
        if (type == SS_ICON)
        {
            ok(old_image != 0, "got %p\n", old_image);
            ok(g_nReceivedColorStatic, "Unexpected WM_CTLCOLORSTATIC value %d\n", g_nReceivedColorStatic);
        }
        else
        {
            ok(!old_image, "got %p\n", old_image);
            ok(!g_nReceivedColorStatic, "Unexpected WM_CTLCOLORSTATIC value %d\n", g_nReceivedColorStatic);
        }

        /* set bitmap */
        g_nReceivedColorStatic = 0;
        old_image = (HICON)SendMessageW(hwnd, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bmp);
        ok(!old_image, "got %p\n", old_image);
        if (type == SS_BITMAP)
            ok(g_nReceivedColorStatic, "Unexpected WM_CTLCOLORSTATIC value %d\n", g_nReceivedColorStatic);
        else
            ok(!g_nReceivedColorStatic, "Unexpected WM_CTLCOLORSTATIC value %d\n", g_nReceivedColorStatic);

        g_nReceivedColorStatic = 0;
        old_image = (HICON)SendMessageW(hwnd, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bmp);
        if (type == SS_BITMAP)
        {
            ok(old_image != 0, "got %p\n", old_image);
            ok(g_nReceivedColorStatic, "Unexpected WM_CTLCOLORSTATIC value %d\n", g_nReceivedColorStatic);
        }
        else
        {
            ok(!old_image, "got %p\n", old_image);
            ok(!g_nReceivedColorStatic, "Unexpected WM_CTLCOLORSTATIC value %d\n", g_nReceivedColorStatic);
        }

        /* set EMF */
        g_nReceivedColorStatic = 0;
        old_image = (HICON)SendMessageW(hwnd, STM_SETIMAGE, IMAGE_ENHMETAFILE, (LPARAM)emf);
        ok(!old_image, "got %p\n", old_image);
        if (type == SS_ENHMETAFILE)
            ok(g_nReceivedColorStatic, "Unexpected WM_CTLCOLORSTATIC value %d\n", g_nReceivedColorStatic);
        else
            ok(!g_nReceivedColorStatic, "Unexpected WM_CTLCOLORSTATIC value %d\n", g_nReceivedColorStatic);

        g_nReceivedColorStatic = 0;
        old_image = (HICON)SendMessageW(hwnd, STM_SETIMAGE, IMAGE_ENHMETAFILE, (LPARAM)emf);
        if (type == SS_ENHMETAFILE)
        {
            ok(old_image != 0, "got %p\n", old_image);
            ok(g_nReceivedColorStatic, "Unexpected WM_CTLCOLORSTATIC value %d\n", g_nReceivedColorStatic);
        }
        else
        {
            ok(!old_image, "got %p\n", old_image);
            ok(!g_nReceivedColorStatic, "Unexpected WM_CTLCOLORSTATIC value %d\n", g_nReceivedColorStatic);
        }

        DestroyWindow(hwnd);

        winetest_pop_context();
    }

    DestroyIcon(icon);
    DeleteObject(bmp);
    DeleteEnhMetaFile(emf);
}

START_TEST(static)
{
    static const char classname[] = "testclass";
    WNDCLASSEXA wndclass;
    ULONG_PTR ctx_cookie;
    HANDLE hCtx;

    if (!load_v6_module(&ctx_cookie, &hCtx))
        return;

    wndclass.cbSize         = sizeof(wndclass);
    wndclass.style          = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc    = WndProc;
    wndclass.cbClsExtra     = 0;
    wndclass.cbWndExtra     = 0;
    wndclass.hInstance      = GetModuleHandleA(NULL);
    wndclass.hIcon          = LoadIconA(NULL, (LPCSTR)IDI_APPLICATION);
    wndclass.hIconSm        = LoadIconA(NULL, (LPCSTR)IDI_APPLICATION);
    wndclass.hCursor        = LoadCursorA(NULL, (LPCSTR)IDC_ARROW);
    wndclass.hbrBackground  = GetStockObject(WHITE_BRUSH);
    wndclass.lpszClassName  = classname;
    wndclass.lpszMenuName   = NULL;
    RegisterClassExA(&wndclass);

    hMainWnd = CreateWindowA(classname, "Test", WS_OVERLAPPEDWINDOW, 0, 0, 500, 500, NULL, NULL,
        GetModuleHandleA(NULL), NULL);
    ShowWindow(hMainWnd, SW_SHOW);

    test_updates(0);
    test_updates(SS_ICON);
    test_updates(SS_BLACKRECT);
    test_updates(SS_WHITERECT);
    test_updates(SS_BLACKFRAME);
    test_updates(SS_WHITEFRAME);
    test_updates(SS_USERITEM);
    test_updates(SS_SIMPLE);
    test_updates(SS_OWNERDRAW);
    test_updates(SS_BITMAP);
    test_updates(SS_BITMAP | SS_CENTERIMAGE);
    test_updates(SS_ETCHEDHORZ);
    test_updates(SS_ETCHEDVERT);
    test_updates(SS_ETCHEDFRAME);
    test_updates(SS_SUNKEN);
    test_set_text();
    test_set_image();
    test_STM_SETIMAGE();

    DestroyWindow(hMainWnd);

    unload_v6_module(ctx_cookie, hCtx);
}
