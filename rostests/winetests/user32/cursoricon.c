/*
 * Unit test suite for cursors and icons.
 *
 * Copyright 2006 Michael Kaufmann
 * Copyright 2007 Dmitry Timoshkov
 * Copyright 2007-2008 Andrew Riedi
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

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wingdi.h"
#include "winuser.h"

#include "pshpack1.h"

typedef struct
{
    BYTE bWidth;
    BYTE bHeight;
    BYTE bColorCount;
    BYTE bReserved;
    WORD xHotspot;
    WORD yHotspot;
    DWORD dwDIBSize;
    DWORD dwDIBOffset;
} CURSORICONFILEDIRENTRY;

typedef struct
{
    WORD idReserved;
    WORD idType;
    WORD idCount;
    CURSORICONFILEDIRENTRY idEntries[1];
} CURSORICONFILEDIR;

#include "poppack.h"

static char **test_argv;
static int test_argc;
static HWND child = 0;
static HWND parent = 0;
static HANDLE child_process;

#define PROC_INIT (WM_USER+1)

static LRESULT CALLBACK callback_child(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL ret;
    DWORD error;

    switch (msg)
    {
        /* Destroy the cursor. */
        case WM_USER+1:
            SetLastError(0xdeadbeef);
            ret = DestroyCursor((HCURSOR) lParam);
            error = GetLastError();
            todo_wine ok(!ret || broken(ret) /* win9x */, "DestroyCursor on the active cursor succeeded.\n");
            ok(error == ERROR_DESTROY_OBJECT_OF_OTHER_THREAD ||
               error == 0xdeadbeef,  /* vista */
                "Last error: %u\n", error);
            return TRUE;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

static LRESULT CALLBACK callback_parent(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == PROC_INIT)
    {
        child = (HWND) wParam;
        return TRUE;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

static void do_child(void)
{
    WNDCLASS class;
    MSG msg;
    BOOL ret;

    /* Register a new class. */
    class.style = CS_GLOBALCLASS;
    class.lpfnWndProc = callback_child;
    class.cbClsExtra = 0;
    class.cbWndExtra = 0;
    class.hInstance = GetModuleHandle(NULL);
    class.hIcon = NULL;
    class.hCursor = NULL;
    class.hbrBackground = NULL;
    class.lpszMenuName = NULL;
    class.lpszClassName = "cursor_child";

    SetLastError(0xdeadbeef);
    ret = RegisterClass(&class);
    ok(ret, "Failed to register window class.  Error: %u\n", GetLastError());

    /* Create a window. */
    child = CreateWindowA("cursor_child", "cursor_child", WS_POPUP | WS_VISIBLE,
        0, 0, 200, 200, 0, 0, 0, NULL);
    ok(child != 0, "CreateWindowA failed.  Error: %u\n", GetLastError());

    /* Let the parent know our HWND. */
    PostMessage(parent, PROC_INIT, (WPARAM) child, 0);

    /* Receive messages. */
    while ((ret = GetMessage(&msg, 0, 0, 0)))
    {
        ok(ret != -1, "GetMessage failed.  Error: %u\n", GetLastError());
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

static void do_parent(void)
{
    char path_name[MAX_PATH];
    PROCESS_INFORMATION info;
    STARTUPINFOA startup;
    WNDCLASS class;
    MSG msg;
    BOOL ret;

    /* Register a new class. */
    class.style = CS_GLOBALCLASS;
    class.lpfnWndProc = callback_parent;
    class.cbClsExtra = 0;
    class.cbWndExtra = 0;
    class.hInstance = GetModuleHandle(NULL);
    class.hIcon = NULL;
    class.hCursor = NULL;
    class.hbrBackground = NULL;
    class.lpszMenuName = NULL;
    class.lpszClassName = "cursor_parent";

    SetLastError(0xdeadbeef);
    ret = RegisterClass(&class);
    ok(ret, "Failed to register window class.  Error: %u\n", GetLastError());

    /* Create a window. */
    parent = CreateWindowA("cursor_parent", "cursor_parent", WS_POPUP | WS_VISIBLE,
        0, 0, 200, 200, 0, 0, 0, NULL);
    ok(parent != 0, "CreateWindowA failed.  Error: %u\n", GetLastError());

    /* Start child process. */
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;

    sprintf(path_name, "%s cursoricon %lx", test_argv[0], (INT_PTR)parent);
    ok(CreateProcessA(NULL, path_name, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess failed.\n");
    child_process = info.hProcess;

    /* Wait for child window handle. */
    while ((child == 0) && (ret = GetMessage(&msg, parent, 0, 0)))
    {
        ok(ret != -1, "GetMessage failed.  Error: %u\n", GetLastError());
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

static void finish_child_process(void)
{
    SendMessage(child, WM_CLOSE, 0, 0);
    winetest_wait_child_process( child_process );
    CloseHandle(child_process);
}

static void test_child_process(void)
{
    static const BYTE bmp_bits[4096];
    HCURSOR cursor;
    ICONINFO cursorInfo;
    UINT display_bpp;
    HDC hdc;

    /* Create and set a dummy cursor. */
    hdc = GetDC(0);
    display_bpp = GetDeviceCaps(hdc, BITSPIXEL);
    ReleaseDC(0, hdc);

    cursorInfo.fIcon = FALSE;
    cursorInfo.xHotspot = 0;
    cursorInfo.yHotspot = 0;
    cursorInfo.hbmMask = CreateBitmap(32, 32, 1, 1, bmp_bits);
    cursorInfo.hbmColor = CreateBitmap(32, 32, 1, display_bpp, bmp_bits);

    cursor = CreateIconIndirect(&cursorInfo);
    ok(cursor != NULL, "CreateIconIndirect returned %p.\n", cursor);

    SetCursor(cursor);

    /* Destroy the cursor. */
    SendMessage(child, WM_USER+1, 0, (LPARAM) cursor);
}

static void test_CopyImage_Check(HBITMAP bitmap, UINT flags, INT copyWidth, INT copyHeight,
                                  INT expectedWidth, INT expectedHeight, WORD expectedDepth, BOOL dibExpected)
{
    HBITMAP copy;
    BITMAP origBitmap;
    BITMAP copyBitmap;
    BOOL orig_is_dib;
    BOOL copy_is_dib;

    copy = CopyImage(bitmap, IMAGE_BITMAP, copyWidth, copyHeight, flags);
    ok(copy != NULL, "CopyImage() failed\n");
    if (copy != NULL)
    {
        GetObject(bitmap, sizeof(origBitmap), &origBitmap);
        GetObject(copy, sizeof(copyBitmap), &copyBitmap);
        orig_is_dib = (origBitmap.bmBits != NULL);
        copy_is_dib = (copyBitmap.bmBits != NULL);

        if (copy_is_dib && dibExpected
            && copyBitmap.bmBitsPixel == 24
            && (expectedDepth == 16 || expectedDepth == 32))
        {
            /* Windows 95 doesn't create DIBs with a depth of 16 or 32 bit */
            if (GetVersion() & 0x80000000)
            {
                expectedDepth = 24;
            }
        }

        if (copy_is_dib && !dibExpected && !(flags & LR_CREATEDIBSECTION))
        {
            /* It's not forbidden to create a DIB section if the flag
               LR_CREATEDIBSECTION is absent.
               Windows 9x does this if the bitmap has a depth that doesn't
               match the screen depth, Windows NT doesn't */
            dibExpected = TRUE;
            expectedDepth = origBitmap.bmBitsPixel;
        }

        ok((!(dibExpected ^ copy_is_dib)
             && (copyBitmap.bmWidth == expectedWidth)
             && (copyBitmap.bmHeight == expectedHeight)
             && (copyBitmap.bmBitsPixel == expectedDepth)),
             "CopyImage ((%s, %dx%d, %u bpp), %d, %d, %#x): Expected (%s, %dx%d, %u bpp), got (%s, %dx%d, %u bpp)\n",
                  orig_is_dib ? "DIB" : "DDB", origBitmap.bmWidth, origBitmap.bmHeight, origBitmap.bmBitsPixel,
                  copyWidth, copyHeight, flags,
                  dibExpected ? "DIB" : "DDB", expectedWidth, expectedHeight, expectedDepth,
                  copy_is_dib ? "DIB" : "DDB", copyBitmap.bmWidth, copyBitmap.bmHeight, copyBitmap.bmBitsPixel);

        DeleteObject(copy);
    }
}

static void test_CopyImage_Bitmap(int depth)
{
    HBITMAP ddb, dib;
    HDC screenDC;
    BITMAPINFO * info;
    VOID * bits;
    int screen_depth;
    unsigned int i;

    /* Create a device-independent bitmap (DIB) */
    info = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));
    info->bmiHeader.biSize = sizeof(info->bmiHeader);
    info->bmiHeader.biWidth = 2;
    info->bmiHeader.biHeight = 2;
    info->bmiHeader.biPlanes = 1;
    info->bmiHeader.biBitCount = depth;
    info->bmiHeader.biCompression = BI_RGB;

    for (i=0; i < 256; i++)
    {
        info->bmiColors[i].rgbRed = i;
        info->bmiColors[i].rgbGreen = i;
        info->bmiColors[i].rgbBlue = 255 - i;
        info->bmiColors[i].rgbReserved = 0;
    }

    dib = CreateDIBSection(NULL, info, DIB_RGB_COLORS, &bits, NULL, 0);

    /* Create a device-dependent bitmap (DDB) */
    screenDC = GetDC(NULL);
    screen_depth = GetDeviceCaps(screenDC, BITSPIXEL);
    if (depth == 1 || depth == screen_depth)
    {
        ddb = CreateBitmap(2, 2, 1, depth, NULL);
    }
    else
    {
        ddb = NULL;
    }
    ReleaseDC(NULL, screenDC);

    if (ddb != NULL)
    {
        test_CopyImage_Check(ddb, 0, 0, 0, 2, 2, depth == 1 ? 1 : screen_depth, FALSE);
        test_CopyImage_Check(ddb, 0, 0, 5, 2, 5, depth == 1 ? 1 : screen_depth, FALSE);
        test_CopyImage_Check(ddb, 0, 5, 0, 5, 2, depth == 1 ? 1 : screen_depth, FALSE);
        test_CopyImage_Check(ddb, 0, 5, 5, 5, 5, depth == 1 ? 1 : screen_depth, FALSE);

        test_CopyImage_Check(ddb, LR_MONOCHROME, 0, 0, 2, 2, 1, FALSE);
        test_CopyImage_Check(ddb, LR_MONOCHROME, 5, 0, 5, 2, 1, FALSE);
        test_CopyImage_Check(ddb, LR_MONOCHROME, 0, 5, 2, 5, 1, FALSE);
        test_CopyImage_Check(ddb, LR_MONOCHROME, 5, 5, 5, 5, 1, FALSE);

        test_CopyImage_Check(ddb, LR_CREATEDIBSECTION, 0, 0, 2, 2, depth, TRUE);
        test_CopyImage_Check(ddb, LR_CREATEDIBSECTION, 5, 0, 5, 2, depth, TRUE);
        test_CopyImage_Check(ddb, LR_CREATEDIBSECTION, 0, 5, 2, 5, depth, TRUE);
        test_CopyImage_Check(ddb, LR_CREATEDIBSECTION, 5, 5, 5, 5, depth, TRUE);

        /* LR_MONOCHROME is ignored if LR_CREATEDIBSECTION is present */
        test_CopyImage_Check(ddb, LR_MONOCHROME | LR_CREATEDIBSECTION, 0, 0, 2, 2, depth, TRUE);
        test_CopyImage_Check(ddb, LR_MONOCHROME | LR_CREATEDIBSECTION, 5, 0, 5, 2, depth, TRUE);
        test_CopyImage_Check(ddb, LR_MONOCHROME | LR_CREATEDIBSECTION, 0, 5, 2, 5, depth, TRUE);
        test_CopyImage_Check(ddb, LR_MONOCHROME | LR_CREATEDIBSECTION, 5, 5, 5, 5, depth, TRUE);

        DeleteObject(ddb);
    }

    if (depth != 1)
    {
        test_CopyImage_Check(dib, 0, 0, 0, 2, 2, screen_depth, FALSE);
        test_CopyImage_Check(dib, 0, 5, 0, 5, 2, screen_depth, FALSE);
        test_CopyImage_Check(dib, 0, 0, 5, 2, 5, screen_depth, FALSE);
        test_CopyImage_Check(dib, 0, 5, 5, 5, 5, screen_depth, FALSE);
    }

    test_CopyImage_Check(dib, LR_MONOCHROME, 0, 0, 2, 2, 1, FALSE);
    test_CopyImage_Check(dib, LR_MONOCHROME, 5, 0, 5, 2, 1, FALSE);
    test_CopyImage_Check(dib, LR_MONOCHROME, 0, 5, 2, 5, 1, FALSE);
    test_CopyImage_Check(dib, LR_MONOCHROME, 5, 5, 5, 5, 1, FALSE);

    test_CopyImage_Check(dib, LR_CREATEDIBSECTION, 0, 0, 2, 2, depth, TRUE);
    test_CopyImage_Check(dib, LR_CREATEDIBSECTION, 5, 0, 5, 2, depth, TRUE);
    test_CopyImage_Check(dib, LR_CREATEDIBSECTION, 0, 5, 2, 5, depth, TRUE);
    test_CopyImage_Check(dib, LR_CREATEDIBSECTION, 5, 5, 5, 5, depth, TRUE);

    /* LR_MONOCHROME is ignored if LR_CREATEDIBSECTION is present */
    test_CopyImage_Check(dib, LR_MONOCHROME | LR_CREATEDIBSECTION, 0, 0, 2, 2, depth, TRUE);
    test_CopyImage_Check(dib, LR_MONOCHROME | LR_CREATEDIBSECTION, 5, 0, 5, 2, depth, TRUE);
    test_CopyImage_Check(dib, LR_MONOCHROME | LR_CREATEDIBSECTION, 0, 5, 2, 5, depth, TRUE);
    test_CopyImage_Check(dib, LR_MONOCHROME | LR_CREATEDIBSECTION, 5, 5, 5, 5, depth, TRUE);

    DeleteObject(dib);

    if (depth == 1)
    {
        /* Special case: A monochrome DIB is converted to a monochrome DDB if
           the colors in the color table are black and white.

           Skip this test on Windows 95, it always creates a monochrome DDB
           in this case */

        if (!(GetVersion() & 0x80000000))
        {
            info->bmiHeader.biBitCount = 1;
            info->bmiColors[0].rgbRed = 0xFF;
            info->bmiColors[0].rgbGreen = 0;
            info->bmiColors[0].rgbBlue = 0;
            info->bmiColors[1].rgbRed = 0;
            info->bmiColors[1].rgbGreen = 0xFF;
            info->bmiColors[1].rgbBlue = 0;

            dib = CreateDIBSection(NULL, info, DIB_RGB_COLORS, &bits, NULL, 0);
            test_CopyImage_Check(dib, 0, 0, 0, 2, 2, screen_depth, FALSE);
            test_CopyImage_Check(dib, 0, 5, 0, 5, 2, screen_depth, FALSE);
            test_CopyImage_Check(dib, 0, 0, 5, 2, 5, screen_depth, FALSE);
            test_CopyImage_Check(dib, 0, 5, 5, 5, 5, screen_depth, FALSE);
            DeleteObject(dib);

            info->bmiHeader.biBitCount = 1;
            info->bmiColors[0].rgbRed = 0;
            info->bmiColors[0].rgbGreen = 0;
            info->bmiColors[0].rgbBlue = 0;
            info->bmiColors[1].rgbRed = 0xFF;
            info->bmiColors[1].rgbGreen = 0xFF;
            info->bmiColors[1].rgbBlue = 0xFF;

            dib = CreateDIBSection(NULL, info, DIB_RGB_COLORS, &bits, NULL, 0);
            test_CopyImage_Check(dib, 0, 0, 0, 2, 2, 1, FALSE);
            test_CopyImage_Check(dib, 0, 5, 0, 5, 2, 1, FALSE);
            test_CopyImage_Check(dib, 0, 0, 5, 2, 5, 1, FALSE);
            test_CopyImage_Check(dib, 0, 5, 5, 5, 5, 1, FALSE);
            DeleteObject(dib);

            info->bmiHeader.biBitCount = 1;
            info->bmiColors[0].rgbRed = 0xFF;
            info->bmiColors[0].rgbGreen = 0xFF;
            info->bmiColors[0].rgbBlue = 0xFF;
            info->bmiColors[1].rgbRed = 0;
            info->bmiColors[1].rgbGreen = 0;
            info->bmiColors[1].rgbBlue = 0;

            dib = CreateDIBSection(NULL, info, DIB_RGB_COLORS, &bits, NULL, 0);
            test_CopyImage_Check(dib, 0, 0, 0, 2, 2, 1, FALSE);
            test_CopyImage_Check(dib, 0, 5, 0, 5, 2, 1, FALSE);
            test_CopyImage_Check(dib, 0, 0, 5, 2, 5, 1, FALSE);
            test_CopyImage_Check(dib, 0, 5, 5, 5, 5, 1, FALSE);
            DeleteObject(dib);
        }
    }

    HeapFree(GetProcessHeap(), 0, info);
}

static void test_initial_cursor(void)
{
    HCURSOR cursor, cursor2;
    DWORD error;

    cursor = GetCursor();

    /* Check what handle GetCursor() returns if a cursor is not set yet. */
    SetLastError(0xdeadbeef);
    cursor2 = LoadCursor(NULL, IDC_WAIT);
    todo_wine {
        ok(cursor == cursor2, "cursor (%p) is not IDC_WAIT (%p).\n", cursor, cursor2);
    }
    error = GetLastError();
    ok(error == 0xdeadbeef, "Last error: 0x%08x\n", error);
}

static void test_icon_info_dbg(HICON hIcon, UINT exp_cx, UINT exp_cy, UINT exp_bpp, int line)
{
    ICONINFO info;
    DWORD ret;
    BITMAP bmMask, bmColor;

    ret = GetIconInfo(hIcon, &info);
    ok_(__FILE__, line)(ret, "GetIconInfo failed\n");

    /* CreateIcon under XP causes info.fIcon to be 0 */
    ok_(__FILE__, line)(info.xHotspot == exp_cx/2, "info.xHotspot = %u\n", info.xHotspot);
    ok_(__FILE__, line)(info.yHotspot == exp_cy/2, "info.yHotspot = %u\n", info.yHotspot);
    ok_(__FILE__, line)(info.hbmMask != 0, "info.hbmMask is NULL\n");

    ret = GetObject(info.hbmMask, sizeof(bmMask), &bmMask);
    ok_(__FILE__, line)(ret == sizeof(bmMask), "GetObject(info.hbmMask) failed, ret %u\n", ret);

    if (exp_bpp == 1)
        ok_(__FILE__, line)(info.hbmColor == 0, "info.hbmColor should be NULL\n");

    if (info.hbmColor)
    {
        HDC hdc;
        UINT display_bpp;

        hdc = GetDC(0);
        display_bpp = GetDeviceCaps(hdc, BITSPIXEL);
        ReleaseDC(0, hdc);

        ret = GetObject(info.hbmColor, sizeof(bmColor), &bmColor);
        ok_(__FILE__, line)(ret == sizeof(bmColor), "GetObject(info.hbmColor) failed, ret %u\n", ret);

        ok_(__FILE__, line)(bmColor.bmBitsPixel == display_bpp /* XP */ ||
           bmColor.bmBitsPixel == exp_bpp /* Win98 */,
           "bmColor.bmBitsPixel = %d\n", bmColor.bmBitsPixel);
        ok_(__FILE__, line)(bmColor.bmWidth == exp_cx, "bmColor.bmWidth = %d\n", bmColor.bmWidth);
        ok_(__FILE__, line)(bmColor.bmHeight == exp_cy, "bmColor.bmHeight = %d\n", bmColor.bmHeight);

        ok_(__FILE__, line)(bmMask.bmBitsPixel == 1, "bmMask.bmBitsPixel = %d\n", bmMask.bmBitsPixel);
        ok_(__FILE__, line)(bmMask.bmWidth == exp_cx, "bmMask.bmWidth = %d\n", bmMask.bmWidth);
        ok_(__FILE__, line)(bmMask.bmHeight == exp_cy, "bmMask.bmHeight = %d\n", bmMask.bmHeight);
    }
    else
    {
        ok_(__FILE__, line)(bmMask.bmBitsPixel == 1, "bmMask.bmBitsPixel = %d\n", bmMask.bmBitsPixel);
        ok_(__FILE__, line)(bmMask.bmWidth == exp_cx, "bmMask.bmWidth = %d\n", bmMask.bmWidth);
        ok_(__FILE__, line)(bmMask.bmHeight == exp_cy * 2, "bmMask.bmHeight = %d\n", bmMask.bmHeight);
    }
}

#define test_icon_info(a,b,c,d) test_icon_info_dbg((a),(b),(c),(d),__LINE__)

static void test_CreateIcon(void)
{
    static const BYTE bmp_bits[1024];
    HICON hIcon;
    HBITMAP hbmMask, hbmColor;
    BITMAPINFO *bmpinfo;
    ICONINFO info;
    HDC hdc;
    void *bits;
    UINT display_bpp;

    hdc = GetDC(0);
    display_bpp = GetDeviceCaps(hdc, BITSPIXEL);

    /* these crash under XP
    hIcon = CreateIcon(0, 16, 16, 1, 1, bmp_bits, NULL);
    hIcon = CreateIcon(0, 16, 16, 1, 1, NULL, bmp_bits);
    */

    hIcon = CreateIcon(0, 16, 16, 1, 1, bmp_bits, bmp_bits);
    ok(hIcon != 0, "CreateIcon failed\n");
    test_icon_info(hIcon, 16, 16, 1);
    DestroyIcon(hIcon);

    hIcon = CreateIcon(0, 16, 16, 1, display_bpp, bmp_bits, bmp_bits);
    ok(hIcon != 0, "CreateIcon failed\n");
    test_icon_info(hIcon, 16, 16, display_bpp);
    DestroyIcon(hIcon);

    hbmMask = CreateBitmap(16, 16, 1, 1, bmp_bits);
    ok(hbmMask != 0, "CreateBitmap failed\n");
    hbmColor = CreateBitmap(16, 16, 1, display_bpp, bmp_bits);
    ok(hbmColor != 0, "CreateBitmap failed\n");

    info.fIcon = TRUE;
    info.xHotspot = 8;
    info.yHotspot = 8;
    info.hbmMask = 0;
    info.hbmColor = 0;
    SetLastError(0xdeadbeaf);
    hIcon = CreateIconIndirect(&info);
    ok(!hIcon, "CreateIconIndirect should fail\n");
    ok(GetLastError() == 0xdeadbeaf, "wrong error %u\n", GetLastError());

    info.fIcon = TRUE;
    info.xHotspot = 8;
    info.yHotspot = 8;
    info.hbmMask = 0;
    info.hbmColor = hbmColor;
    SetLastError(0xdeadbeaf);
    hIcon = CreateIconIndirect(&info);
    ok(!hIcon, "CreateIconIndirect should fail\n");
    ok(GetLastError() == 0xdeadbeaf, "wrong error %u\n", GetLastError());

    info.fIcon = TRUE;
    info.xHotspot = 8;
    info.yHotspot = 8;
    info.hbmMask = hbmMask;
    info.hbmColor = hbmColor;
    hIcon = CreateIconIndirect(&info);
    ok(hIcon != 0, "CreateIconIndirect failed\n");
    test_icon_info(hIcon, 16, 16, display_bpp);
    DestroyIcon(hIcon);

    DeleteObject(hbmMask);
    DeleteObject(hbmColor);

    hbmMask = CreateBitmap(16, 32, 1, 1, bmp_bits);
    ok(hbmMask != 0, "CreateBitmap failed\n");

    info.fIcon = TRUE;
    info.xHotspot = 8;
    info.yHotspot = 8;
    info.hbmMask = hbmMask;
    info.hbmColor = 0;
    SetLastError(0xdeadbeaf);
    hIcon = CreateIconIndirect(&info);
    ok(hIcon != 0, "CreateIconIndirect failed\n");
    test_icon_info(hIcon, 16, 16, 1);
    DestroyIcon(hIcon);

    DeleteObject(hbmMask);
    DeleteObject(hbmColor);

    /* test creating an icon from a DIB section */

    bmpinfo = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, FIELD_OFFSET(BITMAPINFO,bmiColors[256]));
    bmpinfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmpinfo->bmiHeader.biWidth = 32;
    bmpinfo->bmiHeader.biHeight = 32;
    bmpinfo->bmiHeader.biPlanes = 1;
    bmpinfo->bmiHeader.biBitCount = 8;
    bmpinfo->bmiHeader.biCompression = BI_RGB;
    hbmColor = CreateDIBSection( hdc, bmpinfo, DIB_RGB_COLORS, &bits, NULL, 0 );
    ok(hbmColor != NULL, "Expected a handle to the DIB\n");
    if (bits)
        memset( bits, 0x55, 32 * 32 * bmpinfo->bmiHeader.biBitCount / 8 );
    bmpinfo->bmiHeader.biBitCount = 1;
    hbmMask = CreateDIBSection( hdc, bmpinfo, DIB_RGB_COLORS, &bits, NULL, 0 );
    ok(hbmMask != NULL, "Expected a handle to the DIB\n");
    if (bits)
        memset( bits, 0x55, 32 * 32 * bmpinfo->bmiHeader.biBitCount / 8 );

    info.fIcon = TRUE;
    info.xHotspot = 8;
    info.yHotspot = 8;
    info.hbmMask = hbmColor;
    info.hbmColor = hbmMask;
    SetLastError(0xdeadbeaf);
    hIcon = CreateIconIndirect(&info);
    ok(hIcon != 0, "CreateIconIndirect failed\n");
    test_icon_info(hIcon, 32, 32, 8);
    DestroyIcon(hIcon);
    DeleteObject(hbmColor);

    bmpinfo->bmiHeader.biBitCount = 16;
    hbmColor = CreateDIBSection( hdc, bmpinfo, DIB_RGB_COLORS, &bits, NULL, 0 );
    ok(hbmColor != NULL, "Expected a handle to the DIB\n");
    if (bits)
        memset( bits, 0x55, 32 * 32 * bmpinfo->bmiHeader.biBitCount / 8 );

    info.fIcon = TRUE;
    info.xHotspot = 8;
    info.yHotspot = 8;
    info.hbmMask = hbmColor;
    info.hbmColor = hbmMask;
    SetLastError(0xdeadbeaf);
    hIcon = CreateIconIndirect(&info);
    ok(hIcon != 0, "CreateIconIndirect failed\n");
    test_icon_info(hIcon, 32, 32, 8);
    DestroyIcon(hIcon);
    DeleteObject(hbmColor);

    bmpinfo->bmiHeader.biBitCount = 32;
    hbmColor = CreateDIBSection( hdc, bmpinfo, DIB_RGB_COLORS, &bits, NULL, 0 );
    ok(hbmColor != NULL, "Expected a handle to the DIB\n");
    if (bits)
        memset( bits, 0x55, 32 * 32 * bmpinfo->bmiHeader.biBitCount / 8 );

    info.fIcon = TRUE;
    info.xHotspot = 8;
    info.yHotspot = 8;
    info.hbmMask = hbmColor;
    info.hbmColor = hbmMask;
    SetLastError(0xdeadbeaf);
    hIcon = CreateIconIndirect(&info);
    ok(hIcon != 0, "CreateIconIndirect failed\n");
    test_icon_info(hIcon, 32, 32, 8);
    DestroyIcon(hIcon);

    DeleteObject(hbmMask);
    DeleteObject(hbmColor);
    HeapFree( GetProcessHeap(), 0, bmpinfo );

    ReleaseDC(0, hdc);
}

/* Shamelessly ripped from dlls/oleaut32/tests/olepicture.c */
/* 1x1 pixel gif */
static const unsigned char gifimage[35] = {
0x47,0x49,0x46,0x38,0x37,0x61,0x01,0x00,0x01,0x00,0x80,0x00,0x00,0xff,0xff,0xff,
0xff,0xff,0xff,0x2c,0x00,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x00,0x02,0x02,0x44,
0x01,0x00,0x3b
};

/* 1x1 pixel jpg */
static const unsigned char jpgimage[285] = {
0xff,0xd8,0xff,0xe0,0x00,0x10,0x4a,0x46,0x49,0x46,0x00,0x01,0x01,0x01,0x01,0x2c,
0x01,0x2c,0x00,0x00,0xff,0xdb,0x00,0x43,0x00,0x05,0x03,0x04,0x04,0x04,0x03,0x05,
0x04,0x04,0x04,0x05,0x05,0x05,0x06,0x07,0x0c,0x08,0x07,0x07,0x07,0x07,0x0f,0x0b,
0x0b,0x09,0x0c,0x11,0x0f,0x12,0x12,0x11,0x0f,0x11,0x11,0x13,0x16,0x1c,0x17,0x13,
0x14,0x1a,0x15,0x11,0x11,0x18,0x21,0x18,0x1a,0x1d,0x1d,0x1f,0x1f,0x1f,0x13,0x17,
0x22,0x24,0x22,0x1e,0x24,0x1c,0x1e,0x1f,0x1e,0xff,0xdb,0x00,0x43,0x01,0x05,0x05,
0x05,0x07,0x06,0x07,0x0e,0x08,0x08,0x0e,0x1e,0x14,0x11,0x14,0x1e,0x1e,0x1e,0x1e,
0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,
0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,
0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0xff,0xc0,
0x00,0x11,0x08,0x00,0x01,0x00,0x01,0x03,0x01,0x22,0x00,0x02,0x11,0x01,0x03,0x11,
0x01,0xff,0xc4,0x00,0x15,0x00,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0xff,0xc4,0x00,0x14,0x10,0x01,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xc4,
0x00,0x14,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0xff,0xc4,0x00,0x14,0x11,0x01,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xda,0x00,0x0c,0x03,0x01,
0x00,0x02,0x11,0x03,0x11,0x00,0x3f,0x00,0xb2,0xc0,0x07,0xff,0xd9
};

/* 1x1 pixel png */
static const unsigned char pngimage[285] = {
0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a,0x00,0x00,0x00,0x0d,0x49,0x48,0x44,0x52,
0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x08,0x02,0x00,0x00,0x00,0x90,0x77,0x53,
0xde,0x00,0x00,0x00,0x09,0x70,0x48,0x59,0x73,0x00,0x00,0x0b,0x13,0x00,0x00,0x0b,
0x13,0x01,0x00,0x9a,0x9c,0x18,0x00,0x00,0x00,0x07,0x74,0x49,0x4d,0x45,0x07,0xd5,
0x06,0x03,0x0f,0x07,0x2d,0x12,0x10,0xf0,0xfd,0x00,0x00,0x00,0x0c,0x49,0x44,0x41,
0x54,0x08,0xd7,0x63,0xf8,0xff,0xff,0x3f,0x00,0x05,0xfe,0x02,0xfe,0xdc,0xcc,0x59,
0xe7,0x00,0x00,0x00,0x00,0x49,0x45,0x4e,0x44,0xae,0x42,0x60,0x82
};

/* 1x1 pixel bmp */
static const unsigned char bmpimage[66] = {
0x42,0x4d,0x42,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3e,0x00,0x00,0x00,0x28,0x00,
0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x00,0x00,
0x00,0x00,0x04,0x00,0x00,0x00,0x12,0x0b,0x00,0x00,0x12,0x0b,0x00,0x00,0x02,0x00,
0x00,0x00,0x02,0x00,0x00,0x00,0xff,0xff,0xff,0x00,0xff,0xff,0xff,0x00,0x00,0x00,
0x00,0x00
};

/* 2x2 pixel gif */
static const unsigned char gif4pixel[42] = {
0x47,0x49,0x46,0x38,0x37,0x61,0x02,0x00,0x02,0x00,0xa1,0x00,0x00,0x00,0x00,0x00,
0x39,0x62,0xfc,0xff,0x1a,0xe5,0xff,0xff,0xff,0x2c,0x00,0x00,0x00,0x00,0x02,0x00,
0x02,0x00,0x00,0x02,0x03,0x14,0x16,0x05,0x00,0x3b
};

static void test_LoadImageFile(const unsigned char * image_data,
    unsigned int image_size, const char * ext, BOOL expect_success)
{
    HANDLE handle;
    BOOL ret;
    DWORD error, bytes_written;
    char filename[64];

    strcpy(filename, "test.");
    strcat(filename, ext);

    /* Create the test image. */
    handle = CreateFileA(filename, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_NEW,
        FILE_ATTRIBUTE_NORMAL, NULL);
    ok(handle != INVALID_HANDLE_VALUE, "CreateFileA failed. %u\n", GetLastError());
    ret = WriteFile(handle, image_data, image_size, &bytes_written, NULL);
    ok(bytes_written == image_size, "test file created improperly.\n");
    CloseHandle(handle);

    /* Load as cursor. For all tested formats, this should fail */
    SetLastError(0xdeadbeef);
    handle = LoadImageA(NULL, filename, IMAGE_CURSOR, 0, 0, LR_LOADFROMFILE);
    ok(handle == NULL, "LoadImage(%s) as IMAGE_CURSOR succeeded incorrectly.\n", ext);
    error = GetLastError();
    ok(error == 0 ||
        broken(error == 0xdeadbeef) || /* Win9x */
        broken(error == ERROR_BAD_PATHNAME), /* Win98, WinMe */
        "Last error: %u\n", error);
    if (handle != NULL) DestroyCursor(handle);

    /* Load as icon. For all tested formats, this should fail */
    SetLastError(0xdeadbeef);
    handle = LoadImageA(NULL, filename, IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
    ok(handle == NULL, "LoadImage(%s) as IMAGE_ICON succeeded incorrectly.\n", ext);
    error = GetLastError();
    ok(error == 0 ||
        broken(error == 0xdeadbeef) || /* Win9x */
        broken(error == ERROR_BAD_PATHNAME), /* Win98, WinMe */
        "Last error: %u\n", error);
    if (handle != NULL) DestroyIcon(handle);

    /* Load as bitmap. Should succeed if bmp, fail for everything else */
    SetLastError(0xdeadbeef);
    handle = LoadImageA(NULL, filename, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    if (expect_success)
	ok(handle != NULL, "LoadImage(%s) as IMAGE_BITMAP failed.\n", ext);
    else ok(handle == NULL, "LoadImage(%s) as IMAGE_BITMAP succeeded incorrectly.\n", ext);
    error = GetLastError();
    ok(error == 0 ||
        error == 0xdeadbeef, /* Win9x, WinMe */
        "Last error: %u\n", error);
    if (handle != NULL) DeleteObject(handle);

    DeleteFileA(filename);
}

static void test_LoadImage(void)
{
    HANDLE handle;
    BOOL ret;
    DWORD error, bytes_written;
    CURSORICONFILEDIR *icon_data;
    CURSORICONFILEDIRENTRY *icon_entry;
    BITMAPINFOHEADER *icon_header;
    ICONINFO icon_info;

#define ICON_WIDTH 32
#define ICON_HEIGHT 32
#define ICON_AND_SIZE (ICON_WIDTH*ICON_HEIGHT/8)
#define ICON_BPP 32
#define ICON_SIZE \
    (sizeof(CURSORICONFILEDIR) + sizeof(BITMAPINFOHEADER) \
    + ICON_AND_SIZE + ICON_AND_SIZE*ICON_BPP)

    /* Set icon data. */
    icon_data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ICON_SIZE);
    icon_data->idReserved = 0;
    icon_data->idType = 1;
    icon_data->idCount = 1;

    icon_entry = icon_data->idEntries;
    icon_entry->bWidth = ICON_WIDTH;
    icon_entry->bHeight = ICON_HEIGHT;
    icon_entry->bColorCount = 0;
    icon_entry->bReserved = 0;
    icon_entry->xHotspot = 1;
    icon_entry->yHotspot = 1;
    icon_entry->dwDIBSize = ICON_SIZE - sizeof(CURSORICONFILEDIR);
    icon_entry->dwDIBOffset = sizeof(CURSORICONFILEDIR);

    icon_header = (BITMAPINFOHEADER *) ((BYTE *) icon_data + icon_entry->dwDIBOffset);
    icon_header->biSize = sizeof(BITMAPINFOHEADER);
    icon_header->biWidth = ICON_WIDTH;
    icon_header->biHeight = ICON_HEIGHT*2;
    icon_header->biPlanes = 1;
    icon_header->biBitCount = ICON_BPP;
    icon_header->biSizeImage = 0; /* Uncompressed bitmap. */

    /* Create the icon. */
    handle = CreateFileA("icon.ico", GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_NEW,
        FILE_ATTRIBUTE_NORMAL, NULL);
    ok(handle != INVALID_HANDLE_VALUE, "CreateFileA failed. %u\n", GetLastError());
    ret = WriteFile(handle, icon_data, ICON_SIZE, &bytes_written, NULL);
    ok(bytes_written == ICON_SIZE, "icon.ico created improperly.\n");
    CloseHandle(handle);

    /* Test loading an icon as a cursor. */
    SetLastError(0xdeadbeef);
    handle = LoadImageA(NULL, "icon.ico", IMAGE_CURSOR, 0, 0, LR_LOADFROMFILE);
    ok(handle != NULL, "LoadImage() failed.\n");
    error = GetLastError();
    ok(error == 0 ||
        broken(error == 0xdeadbeef) || /* Win9x */
        broken(error == ERROR_BAD_PATHNAME), /* Win98, WinMe */
        "Last error: %u\n", error);

    /* Test the icon information. */
    SetLastError(0xdeadbeef);
    ret = GetIconInfo(handle, &icon_info);
    ok(ret, "GetIconInfo() failed.\n");
    error = GetLastError();
    ok(error == 0xdeadbeef, "Last error: %u\n", error);

    if (ret)
    {
        ok(icon_info.fIcon == FALSE, "fIcon != FALSE.\n");
        ok(icon_info.xHotspot == 1, "xHotspot is %u.\n", icon_info.xHotspot);
        ok(icon_info.yHotspot == 1, "yHotspot is %u.\n", icon_info.yHotspot);
        ok(icon_info.hbmColor != NULL || broken(!icon_info.hbmColor) /* no color cursor support */,
           "No hbmColor!\n");
        ok(icon_info.hbmMask != NULL, "No hbmMask!\n");
    }

    /* Clean up. */
    SetLastError(0xdeadbeef);
    ret = DestroyCursor(handle);
    ok(ret, "DestroyCursor() failed.\n");
    error = GetLastError();
    ok(error == 0xdeadbeef, "Last error: %u\n", error);

    HeapFree(GetProcessHeap(), 0, icon_data);
    DeleteFileA("icon.ico");

    test_LoadImageFile(bmpimage, sizeof(bmpimage), "bmp", 1);
    test_LoadImageFile(gifimage, sizeof(gifimage), "gif", 0);
    test_LoadImageFile(gif4pixel, sizeof(gif4pixel), "gif", 0);
    test_LoadImageFile(jpgimage, sizeof(jpgimage), "jpg", 0);
    test_LoadImageFile(pngimage, sizeof(pngimage), "png", 0);
}

static void test_CreateIconFromResource(void)
{
    HANDLE handle;
    BOOL ret;
    DWORD error;
    BITMAPINFOHEADER *icon_header;
    INT16 *hotspot;
    ICONINFO icon_info;

#define ICON_RES_WIDTH 32
#define ICON_RES_HEIGHT 32
#define ICON_RES_AND_SIZE (ICON_WIDTH*ICON_HEIGHT/8)
#define ICON_RES_BPP 32
#define ICON_RES_SIZE \
    (sizeof(BITMAPINFOHEADER) + ICON_AND_SIZE + ICON_AND_SIZE*ICON_BPP)
#define CRSR_RES_SIZE (2*sizeof(INT16) + ICON_RES_SIZE)

    /* Set icon data. */
    hotspot = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, CRSR_RES_SIZE);

    /* Cursor resources have an extra hotspot, icon resources not. */
    hotspot[0] = 3;
    hotspot[1] = 3;

    icon_header = (BITMAPINFOHEADER *) (hotspot + 2);
    icon_header->biSize = sizeof(BITMAPINFOHEADER);
    icon_header->biWidth = ICON_WIDTH;
    icon_header->biHeight = ICON_HEIGHT*2;
    icon_header->biPlanes = 1;
    icon_header->biBitCount = ICON_BPP;
    icon_header->biSizeImage = 0; /* Uncompressed bitmap. */

    /* Test creating a cursor. */
    SetLastError(0xdeadbeef);
    handle = CreateIconFromResource((PBYTE) hotspot, CRSR_RES_SIZE, FALSE, 0x00030000);
    ok(handle != NULL, "Create cursor failed.\n");

    /* Test the icon information. */
    SetLastError(0xdeadbeef);
    ret = GetIconInfo(handle, &icon_info);
    ok(ret, "GetIconInfo() failed.\n");
    error = GetLastError();
    ok(error == 0xdeadbeef, "Last error: %u\n", error);

    if (ret)
    {
        ok(icon_info.fIcon == FALSE, "fIcon != FALSE.\n");
        ok(icon_info.xHotspot == 3, "xHotspot is %u.\n", icon_info.xHotspot);
        ok(icon_info.yHotspot == 3, "yHotspot is %u.\n", icon_info.yHotspot);
        ok(icon_info.hbmColor != NULL || broken(!icon_info.hbmColor) /* no color cursor support */,
           "No hbmColor!\n");
        ok(icon_info.hbmMask != NULL, "No hbmMask!\n");
    }

    /* Clean up. */
    SetLastError(0xdeadbeef);
    ret = DestroyCursor(handle);
    ok(ret, "DestroyCursor() failed.\n");
    error = GetLastError();
    ok(error == 0xdeadbeef, "Last error: %u\n", error);

    /* Test creating an icon. */
    SetLastError(0xdeadbeef);
    handle = CreateIconFromResource((PBYTE) icon_header, ICON_RES_SIZE, TRUE,
				    0x00030000);
    ok(handle != NULL, "Create icon failed.\n");

    /* Test the icon information. */
    SetLastError(0xdeadbeef);
    ret = GetIconInfo(handle, &icon_info);
    ok(ret, "GetIconInfo() failed.\n");
    error = GetLastError();
    ok(error == 0xdeadbeef, "Last error: %u\n", error);

    if (ret)
    {
        ok(icon_info.fIcon == TRUE, "fIcon != TRUE.\n");
	/* Icons always have hotspot in the middle */
        ok(icon_info.xHotspot == ICON_WIDTH/2, "xHotspot is %u.\n", icon_info.xHotspot);
        ok(icon_info.yHotspot == ICON_HEIGHT/2, "yHotspot is %u.\n", icon_info.yHotspot);
        ok(icon_info.hbmColor != NULL, "No hbmColor!\n");
        ok(icon_info.hbmMask != NULL, "No hbmMask!\n");
    }

    /* Clean up. */
    SetLastError(0xdeadbeef);
    ret = DestroyCursor(handle);
    ok(ret, "DestroyCursor() failed.\n");
    error = GetLastError();
    ok(error == 0xdeadbeef, "Last error: %u\n", error);

    HeapFree(GetProcessHeap(), 0, hotspot);
}

static void test_DestroyCursor(void)
{
    static const BYTE bmp_bits[4096];
    ICONINFO cursorInfo;
    HCURSOR cursor, cursor2;
    BOOL ret;
    DWORD error;
    UINT display_bpp;
    HDC hdc;

    hdc = GetDC(0);
    display_bpp = GetDeviceCaps(hdc, BITSPIXEL);
    ReleaseDC(0, hdc);

    cursorInfo.fIcon = FALSE;
    cursorInfo.xHotspot = 0;
    cursorInfo.yHotspot = 0;
    cursorInfo.hbmMask = CreateBitmap(32, 32, 1, 1, bmp_bits);
    cursorInfo.hbmColor = CreateBitmap(32, 32, 1, display_bpp, bmp_bits);

    cursor = CreateIconIndirect(&cursorInfo);
    ok(cursor != NULL, "CreateIconIndirect returned %p\n", cursor);
    if(!cursor) {
        return;
    }
    SetCursor(cursor);

    SetLastError(0xdeadbeef);
    ret = DestroyCursor(cursor);
    ok(!ret || broken(ret)  /* succeeds on win9x */, "DestroyCursor on the active cursor succeeded\n");
    error = GetLastError();
    ok(error == 0xdeadbeef, "Last error: %u\n", error);
    if (!ret)
    {
        cursor2 = GetCursor();
        ok(cursor2 == cursor, "Active was set to %p when trying to destroy it\n", cursor2);
        SetCursor(NULL);

        /* Trying to destroy the cursor properly fails now with
         * ERROR_INVALID_CURSOR_HANDLE.  This happens because we called
         * DestroyCursor() 2+ times after calling SetCursor().  The calls to
         * GetCursor() and SetCursor(NULL) in between make no difference. */
        ret = DestroyCursor(cursor);
        todo_wine {
            ok(!ret, "DestroyCursor succeeded.\n");
            error = GetLastError();
            ok(error == ERROR_INVALID_CURSOR_HANDLE || error == 0xdeadbeef, /* vista */
               "Last error: 0x%08x\n", error);
        }
    }

    DeleteObject(cursorInfo.hbmMask);
    DeleteObject(cursorInfo.hbmColor);

    /* Try testing DestroyCursor() now using LoadCursor() cursors. */
    cursor = LoadCursor(NULL, IDC_ARROW);

    SetLastError(0xdeadbeef);
    ret = DestroyCursor(cursor);
    ok(ret || broken(!ret) /* fails on win9x */, "DestroyCursor on the active cursor failed.\n");
    error = GetLastError();
    ok(error == 0xdeadbeef, "Last error: 0x%08x\n", error);

    /* Try setting the cursor to a destroyed OEM cursor. */
    SetLastError(0xdeadbeef);
    SetCursor(cursor);
    error = GetLastError();
    todo_wine {
        ok(error == 0xdeadbeef, "Last error: 0x%08x\n", error);
    }

    /* Check if LoadCursor() returns the same handle with the same icon. */
    cursor2 = LoadCursor(NULL, IDC_ARROW);
    ok(cursor2 == cursor, "cursor == %p, cursor2 == %p\n", cursor, cursor2);

    /* Check if LoadCursor() returns the same handle with a different icon. */
    cursor2 = LoadCursor(NULL, IDC_WAIT);
    ok(cursor2 != cursor, "cursor == %p, cursor2 == %p\n", cursor, cursor2);
}

START_TEST(cursoricon)
{
    test_argc = winetest_get_mainargs(&test_argv);

    if (test_argc >= 3)
    {
        /* Child process. */
        sscanf (test_argv[2], "%x", (unsigned int *) &parent);

        ok(parent != NULL, "Parent not found.\n");
        if (parent == NULL)
            ExitProcess(1);

        do_child();
        return;
    }

    test_CopyImage_Bitmap(1);
    test_CopyImage_Bitmap(4);
    test_CopyImage_Bitmap(8);
    test_CopyImage_Bitmap(16);
    test_CopyImage_Bitmap(24);
    test_CopyImage_Bitmap(32);
    test_initial_cursor();
    test_CreateIcon();
    test_LoadImage();
    test_CreateIconFromResource();
    test_DestroyCursor();
    do_parent();
    test_child_process();
    finish_child_process();
}
