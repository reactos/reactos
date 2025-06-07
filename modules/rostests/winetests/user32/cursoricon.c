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

#define RIFF_FOURCC( c0, c1, c2, c3 ) \
        ( (DWORD)(BYTE)(c0) | ( (DWORD)(BYTE)(c1) << 8 ) | \
        ( (DWORD)(BYTE)(c2) << 16 ) | ( (DWORD)(BYTE)(c3) << 24 ) )

#define ANI_RIFF_ID RIFF_FOURCC('R', 'I', 'F', 'F')
#define ANI_LIST_ID RIFF_FOURCC('L', 'I', 'S', 'T')
#define ANI_ACON_ID RIFF_FOURCC('A', 'C', 'O', 'N')
#define ANI_anih_ID RIFF_FOURCC('a', 'n', 'i', 'h')
#define ANI_seq__ID RIFF_FOURCC('s', 'e', 'q', ' ')
#define ANI_fram_ID RIFF_FOURCC('f', 'r', 'a', 'm')
#define ANI_icon_ID RIFF_FOURCC('i', 'c', 'o', 'n')
#define ANI_rate_ID RIFF_FOURCC('r', 'a', 't', 'e')

#define ANI_FLAG_ICON       0x1
#define ANI_FLAG_SEQUENCE   0x2

typedef struct {
    DWORD header_size;
    DWORD num_frames;
    DWORD num_steps;
    DWORD width;
    DWORD height;
    DWORD bpp;
    DWORD num_planes;
    DWORD display_rate;
    DWORD flags;
} ani_header;

typedef struct {
    BYTE data[32*32*4];
    BYTE mask_data[32*32/8];
} ani_data32x32x32;

typedef struct {
    CURSORICONFILEDIR    icon_info;  /* animated cursor frame information */
    BITMAPINFOHEADER     bmi_header; /* animated cursor frame header */
    ani_data32x32x32     bmi_data;   /* animated cursor frame DIB data */
} ani_frame32x32x32;

typedef struct {
    DWORD                chunk_id;   /* ANI_anih_ID */
    DWORD                chunk_size; /* actual size of data */
    ani_header           header;     /* animated cursor header */
} riff_header_t;

typedef struct {
    DWORD                chunk_id;   /* ANI_LIST_ID */
    DWORD                chunk_size; /* actual size of data */
    DWORD                chunk_type; /* ANI_fram_ID */
} riff_list_t;

typedef struct {
    DWORD                chunk_id;   /* ANI_icon_ID */
    DWORD                chunk_size; /* actual size of data */
    ani_frame32x32x32    data;       /* animated cursor frame */
} riff_icon32x32x32_t;

typedef struct {
    DWORD                chunk_id;   /* ANI_RIFF_ID */
    DWORD                chunk_size; /* actual size of data */
    DWORD                chunk_type; /* ANI_ACON_ID */
    riff_header_t        header;     /* RIFF animated cursor header */
    riff_list_t          frame_list; /* RIFF animated cursor frame list info */
    riff_icon32x32x32_t  frames[1];  /* array of animated cursor frames */
} riff_cursor1_t;

typedef struct {
    DWORD                chunk_id;   /* ANI_RIFF_ID */
    DWORD                chunk_size; /* actual size of data */
    DWORD                chunk_type; /* ANI_ACON_ID */
    riff_header_t        header;     /* RIFF animated cursor header */
    riff_list_t          frame_list; /* RIFF animated cursor frame list info */
    riff_icon32x32x32_t  frames[3];  /* array of three animated cursor frames */
} riff_cursor3_t;

typedef struct {
    DWORD                chunk_id;   /* ANI_rate_ID */
    DWORD                chunk_size; /* actual size of data */
    DWORD                rate[3];    /* animated cursor rate data */
} riff_rate3_t;

typedef struct {
    DWORD                chunk_id;   /* ANI_seq__ID */
    DWORD                chunk_size; /* actual size of data */
    DWORD                order[3];   /* animated cursor sequence data */
} riff_seq3_t;

typedef struct {
    DWORD                chunk_id;   /* ANI_RIFF_ID */
    DWORD                chunk_size; /* actual size of data */
    DWORD                chunk_type; /* ANI_ACON_ID */
    riff_header_t        header;     /* RIFF animated cursor header */
    riff_seq3_t          seq;        /* sequence data for three cursor frames */
    riff_rate3_t         rates;      /* rate data for three cursor frames */
    riff_list_t          frame_list; /* RIFF animated cursor frame list info */
    riff_icon32x32x32_t  frames[3];  /* array of three animated cursor frames */
} riff_cursor3_seq_t;

#define EMPTY_ICON32 \
{ \
    ANI_icon_ID, \
    sizeof(ani_frame32x32x32), \
    { \
        { \
            0x0, /* reserved */ \
            0,   /* type: icon(1), cursor(2) */ \
            1,   /* count */ \
            { \
                { \
                    32,                        /* width */ \
                    32,                        /* height */ \
                    0,                         /* color count */ \
                    0x0,                       /* reserved */ \
                    16,                        /* x hotspot */ \
                    16,                        /* y hotspot */ \
                    sizeof(ani_data32x32x32),  /* DIB size */ \
                    sizeof(CURSORICONFILEDIR)  /* DIB offset */ \
                } \
            } \
        }, \
        { \
              sizeof(BITMAPINFOHEADER),  /* structure for DIB-type data */ \
              32,                        /* width */ \
              32*2,                      /* actual height times two */ \
              1,                         /* planes */ \
              32,                        /* bpp */ \
              BI_RGB,                    /* compression */ \
              0,                         /* image size */ \
              0,                         /* biXPelsPerMeter */ \
              0,                         /* biYPelsPerMeter */ \
              0,                         /* biClrUsed */ \
              0                          /* biClrImportant */ \
        } \
        /* DIB data: left uninitialized */ \
    } \
}

riff_cursor1_t empty_anicursor = {
    ANI_RIFF_ID,
    sizeof(empty_anicursor) - sizeof(DWORD)*2,
    ANI_ACON_ID,
    {
        ANI_anih_ID,
        sizeof(ani_header),
        {
            sizeof(ani_header),
            1,            /* frames */
            1,            /* steps */
            32,           /* width */
            32,           /* height */
            32,           /* depth */
            1,            /* planes */
            10,           /* display rate in jiffies */
            ANI_FLAG_ICON /* flags */
        }
    },
    {
        ANI_LIST_ID,
        sizeof(riff_icon32x32x32_t)*(1 /*frames*/) + sizeof(DWORD),
        ANI_fram_ID,
    },
    {
        EMPTY_ICON32
    }
};

riff_cursor3_t empty_anicursor3 = {
    ANI_RIFF_ID,
    sizeof(empty_anicursor3) - sizeof(DWORD)*2,
    ANI_ACON_ID,
    {
        ANI_anih_ID,
        sizeof(ani_header),
        {
            sizeof(ani_header),
            3,            /* frames */
            3,            /* steps */
            32,           /* width */
            32,           /* height */
            32,           /* depth */
            1,            /* planes */
            0xbeef,       /* display rate in jiffies */
            ANI_FLAG_ICON /* flags */
        }
    },
    {
        ANI_LIST_ID,
        sizeof(riff_icon32x32x32_t)*(3 /*frames*/) + sizeof(DWORD),
        ANI_fram_ID,
    },
    {
        EMPTY_ICON32,
        EMPTY_ICON32,
        EMPTY_ICON32
    }
};

riff_cursor3_seq_t empty_anicursor3_seq = {
    ANI_RIFF_ID,
    sizeof(empty_anicursor3_seq) - sizeof(DWORD)*2,
    ANI_ACON_ID,
    {
        ANI_anih_ID,
        sizeof(ani_header),
        {
            sizeof(ani_header),
            3,                              /* frames */
            3,                              /* steps */
            32,                             /* width */
            32,                             /* height */
            32,                             /* depth */
            1,                              /* planes */
            0xbeef,                         /* display rate in jiffies */
            ANI_FLAG_ICON|ANI_FLAG_SEQUENCE /* flags */
        }
    },
    {
        ANI_seq__ID,
        sizeof(riff_seq3_t) - sizeof(DWORD)*2,
        { 2, 0, 1} /* show frames in a uniquely identifiable order */
    },
    {
        ANI_rate_ID,
        sizeof(riff_rate3_t) - sizeof(DWORD)*2,
        { 0xc0de, 0xcafe, 0xbabe}
    },
    {
        ANI_LIST_ID,
        sizeof(riff_icon32x32x32_t)*(3 /*frames*/) + sizeof(DWORD),
        ANI_fram_ID,
    },
    {
        EMPTY_ICON32,
        EMPTY_ICON32,
        EMPTY_ICON32
    }
};

#include "poppack.h"

static char **test_argv;
static int test_argc;
static HWND child = 0;
static HWND parent = 0;
static HANDLE child_process;

#define PROC_INIT (WM_USER+1)

static BOOL (WINAPI *pGetCursorInfo)(CURSORINFO *);
static BOOL (WINAPI *pGetIconInfoExA)(HICON,ICONINFOEXA *);
static BOOL (WINAPI *pGetIconInfoExW)(HICON,ICONINFOEXW *);

static const BOOL is_win64 = (sizeof(void *) > sizeof(int));

static LRESULT CALLBACK callback_child(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        /* Destroy the cursor. */
        case WM_USER+1:
        {
            HCURSOR cursor = (HCURSOR)lParam;
            ICONINFO info;
            BOOL ret;
            DWORD error;

            memset(&info, 0, sizeof(info));
            ret = GetIconInfo(cursor, &info);
            todo_wine ok(ret, "GetIconInfoEx failed with error %lu\n", GetLastError());
            todo_wine ok(info.hbmColor != NULL, "info.hmbColor was not set\n");
            todo_wine ok(info.hbmMask != NULL, "info.hmbColor was not set\n");
            DeleteObject(info.hbmColor);
            DeleteObject(info.hbmMask);

            SetLastError(0xdeadbeef);
            ret = DestroyCursor(cursor);
            error = GetLastError();
            ok(!ret, "DestroyCursor on the active cursor succeeded.\n");
            ok(error == ERROR_DESTROY_OBJECT_OF_OTHER_THREAD ||
               error == 0xdeadbeef,  /* vista */
                "Last error: %lu\n", error);
            return TRUE;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static LRESULT CALLBACK callback_parent(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == PROC_INIT)
    {
        child = (HWND) wParam;
        return TRUE;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static void do_child(void)
{
    WNDCLASSA class;
    MSG msg;
    BOOL ret;

    /* Register a new class. */
    class.style = CS_GLOBALCLASS;
    class.lpfnWndProc = callback_child;
    class.cbClsExtra = 0;
    class.cbWndExtra = 0;
    class.hInstance = GetModuleHandleA(NULL);
    class.hIcon = NULL;
    class.hCursor = NULL;
    class.hbrBackground = NULL;
    class.lpszMenuName = NULL;
    class.lpszClassName = "cursor_child";

    SetLastError(0xdeadbeef);
    ret = RegisterClassA(&class);
    ok(ret, "Failed to register window class.  Error: %lu\n", GetLastError());

    /* Create a window. */
    child = CreateWindowA("cursor_child", "cursor_child", WS_POPUP | WS_VISIBLE,
        0, 0, 200, 200, 0, 0, 0, NULL);
    ok(child != 0, "CreateWindowA failed.  Error: %lu\n", GetLastError());

    /* Let the parent know our HWND. */
    PostMessageA(parent, PROC_INIT, (WPARAM) child, 0);

    /* Receive messages. */
    while ((ret = GetMessageA(&msg, 0, 0, 0)))
    {
        ok(ret != -1, "GetMessage failed.  Error: %lu\n", GetLastError());
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}

static void do_parent(void)
{
    char path_name[MAX_PATH];
    PROCESS_INFORMATION info;
    STARTUPINFOA startup;
    WNDCLASSA class;
    MSG msg;
    BOOL ret;

    /* Register a new class. */
    class.style = CS_GLOBALCLASS;
    class.lpfnWndProc = callback_parent;
    class.cbClsExtra = 0;
    class.cbWndExtra = 0;
    class.hInstance = GetModuleHandleA(NULL);
    class.hIcon = NULL;
    class.hCursor = NULL;
    class.hbrBackground = NULL;
    class.lpszMenuName = NULL;
    class.lpszClassName = "cursor_parent";

    SetLastError(0xdeadbeef);
    ret = RegisterClassA(&class);
    ok(ret, "Failed to register window class.  Error: %lu\n", GetLastError());

    /* Create a window. */
    parent = CreateWindowA("cursor_parent", "cursor_parent", WS_POPUP | WS_VISIBLE,
        0, 0, 200, 200, 0, 0, 0, NULL);
    ok(parent != 0, "CreateWindowA failed.  Error: %lu\n", GetLastError());

    /* Start child process. */
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;

    sprintf(path_name, "%s cursoricon %Ix", test_argv[0], (INT_PTR)parent);
    ok(CreateProcessA(NULL, path_name, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess failed.\n");
    child_process = info.hProcess;

    /* Wait for child window handle. */
    while ((child == 0) && (ret = GetMessageA(&msg, parent, 0, 0)))
    {
        ok(ret != -1, "GetMessage failed.  Error: %lu\n", GetLastError());
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}

static void finish_child_process(void)
{
    SendMessageA(child, WM_CLOSE, 0, 0);
    wait_child_process( child_process );
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
    SendMessageA(child, WM_USER+1, 0, (LPARAM) cursor);
}

static BOOL color_match(COLORREF a, COLORREF b)
{
    /* 5-bit accuracy is a sufficient test. This will match as long as
     * colors are never truncated to less that 3x5-bit accuracy i.e.
     * palettized. */
    return (a & 0x00F8F8F8) == (b & 0x00F8F8F8);
}

static void check_copy_image(HBITMAP bitmap, UINT type, UINT flags, INT copyWidth, INT copyHeight,
        INT expectedWidth, INT expectedHeight, WORD expectedDepth, BOOL dibExpected)
{
    HBITMAP copy;
    BITMAP origBitmap;
    BITMAP copyBitmap;
    BOOL copy_is_dib;
    BOOL ret;

    GetObjectA(bitmap, sizeof(origBitmap), &origBitmap);

    winetest_push_context("%s, type %#x, flags %#x, size %ux%u",
            origBitmap.bmBits ? "DIB" : "DDB", type, flags, copyWidth, copyHeight);

    if (type == IMAGE_BITMAP)
    {
        copy = CopyImage(bitmap, type, copyWidth, copyHeight, flags);
    }
    else
    {
        ICONINFO info =
        {
            .fIcon = (type == IMAGE_ICON),
            .hbmColor = bitmap,
            .hbmMask = bitmap,
        };
        HICON icon = CreateIconIndirect(&info);
        ok(!!icon, "Failed to create icon, error %lu\n", GetLastError());
        copy = CopyImage(icon, type, copyWidth, copyHeight, flags);
        ret = DestroyIcon(icon);
        ok(ret, "Failed to destroy icon, error %lu\n", GetLastError());
    }
    ok(copy != NULL, "CopyImage() failed\n");
    if (copy != NULL)
    {
        if (type == IMAGE_BITMAP)
        {
            GetObjectA(copy, sizeof(copyBitmap), &copyBitmap);
        }
        else
        {
            ICONINFO info;

            ret = GetIconInfo((HICON)copy, &info);
            ok(ret, "Failed to get icon info, error %lu\n", GetLastError());
            GetObjectA(info.hbmColor, sizeof(copyBitmap), &copyBitmap);
        }
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

        if (type != IMAGE_BITMAP)
        {
            dibExpected = FALSE;
            expectedDepth = 32;
        }

        ok(copy_is_dib == dibExpected, "Expected %s, got %s\n",
                dibExpected ? "DIB" : "DDB", copy_is_dib ? "DIB" : "DDB");
        ok(copyBitmap.bmWidth == expectedWidth, "Expected width %u, got %u\n",
                expectedWidth, copyBitmap.bmWidth);
        ok(copyBitmap.bmHeight == expectedHeight, "Expected height %u, got %u\n",
                expectedHeight, copyBitmap.bmHeight);
        ok(copyBitmap.bmBitsPixel == expectedDepth, "Expected depth %u, got %u\n",
                expectedDepth, copyBitmap.bmBitsPixel);

        if (type != IMAGE_BITMAP)
        {
            ICONINFO info;

            ret = GetIconInfo((HICON)copy, &info);
            ok(ret, "Failed to get icon info, error %lu\n", GetLastError());
            GetObjectA(info.hbmMask, sizeof(copyBitmap), &copyBitmap);

            ok(!copyBitmap.bmBits, "Expected DDB\n");
            ok(copyBitmap.bmWidth == expectedWidth, "Expected mask width %u, got %u\n",
                    expectedWidth, copyBitmap.bmWidth);
            ok(copyBitmap.bmHeight == expectedHeight, "Expected mask height %u, got %u\n",
                    expectedHeight, copyBitmap.bmHeight);
            ok(copyBitmap.bmBitsPixel == 1, "Got mask depth %u\n", copyBitmap.bmBitsPixel);
        }

        if (type == IMAGE_BITMAP)
            DeleteObject(copy);
        else if (type == IMAGE_ICON)
            DestroyIcon((HICON)copy);
        else
            DestroyCursor((HCURSOR)copy);
    }

    winetest_pop_context();
}

static void do_test_copy_image(UINT type, UINT depth)
{
    HBITMAP ddb, dib;
    HDC screenDC;
    BITMAPINFO * info;
    VOID * bits;
    int screen_depth;
    unsigned int i;

    /* Create a device-independent bitmap (DIB) */
    info = calloc(1, sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));
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
        check_copy_image(ddb, type, 0, 0, 0, 2, 2, depth == 1 ? 1 : screen_depth, FALSE);
        check_copy_image(ddb, type, 0, 0, 5, 2, 5, depth == 1 ? 1 : screen_depth, FALSE);
        check_copy_image(ddb, type, 0, 5, 0, 5, 2, depth == 1 ? 1 : screen_depth, FALSE);
        check_copy_image(ddb, type, 0, 5, 5, 5, 5, depth == 1 ? 1 : screen_depth, FALSE);

        check_copy_image(ddb, type, LR_MONOCHROME, 0, 0, 2, 2, 1, FALSE);
        check_copy_image(ddb, type, LR_MONOCHROME, 5, 0, 5, 2, 1, FALSE);
        check_copy_image(ddb, type, LR_MONOCHROME, 0, 5, 2, 5, 1, FALSE);
        check_copy_image(ddb, type, LR_MONOCHROME, 5, 5, 5, 5, 1, FALSE);

        check_copy_image(ddb, type, LR_CREATEDIBSECTION, 0, 0, 2, 2, depth, TRUE);
        check_copy_image(ddb, type, LR_CREATEDIBSECTION, 5, 0, 5, 2, depth, TRUE);
        check_copy_image(ddb, type, LR_CREATEDIBSECTION, 0, 5, 2, 5, depth, TRUE);
        check_copy_image(ddb, type, LR_CREATEDIBSECTION, 5, 5, 5, 5, depth, TRUE);

        /* LR_MONOCHROME is ignored if LR_CREATEDIBSECTION is present */
        check_copy_image(ddb, type, LR_MONOCHROME | LR_CREATEDIBSECTION, 0, 0, 2, 2, depth, TRUE);
        check_copy_image(ddb, type, LR_MONOCHROME | LR_CREATEDIBSECTION, 5, 0, 5, 2, depth, TRUE);
        check_copy_image(ddb, type, LR_MONOCHROME | LR_CREATEDIBSECTION, 0, 5, 2, 5, depth, TRUE);
        check_copy_image(ddb, type, LR_MONOCHROME | LR_CREATEDIBSECTION, 5, 5, 5, 5, depth, TRUE);

        DeleteObject(ddb);
    }

    if (depth != 1)
    {
        check_copy_image(dib, type, 0, 0, 0, 2, 2, screen_depth, FALSE);
        check_copy_image(dib, type, 0, 5, 0, 5, 2, screen_depth, FALSE);
        check_copy_image(dib, type, 0, 0, 5, 2, 5, screen_depth, FALSE);
        check_copy_image(dib, type, 0, 5, 5, 5, 5, screen_depth, FALSE);
    }

    check_copy_image(dib, type, LR_MONOCHROME, 0, 0, 2, 2, 1, FALSE);
    check_copy_image(dib, type, LR_MONOCHROME, 5, 0, 5, 2, 1, FALSE);
    check_copy_image(dib, type, LR_MONOCHROME, 0, 5, 2, 5, 1, FALSE);
    check_copy_image(dib, type, LR_MONOCHROME, 5, 5, 5, 5, 1, FALSE);

    check_copy_image(dib, type, LR_CREATEDIBSECTION, 0, 0, 2, 2, depth, TRUE);
    check_copy_image(dib, type, LR_CREATEDIBSECTION, 5, 0, 5, 2, depth, TRUE);
    check_copy_image(dib, type, LR_CREATEDIBSECTION, 0, 5, 2, 5, depth, TRUE);
    check_copy_image(dib, type, LR_CREATEDIBSECTION, 5, 5, 5, 5, depth, TRUE);

    /* LR_MONOCHROME is ignored if LR_CREATEDIBSECTION is present */
    check_copy_image(dib, type, LR_MONOCHROME | LR_CREATEDIBSECTION, 0, 0, 2, 2, depth, TRUE);
    check_copy_image(dib, type, LR_MONOCHROME | LR_CREATEDIBSECTION, 5, 0, 5, 2, depth, TRUE);
    check_copy_image(dib, type, LR_MONOCHROME | LR_CREATEDIBSECTION, 0, 5, 2, 5, depth, TRUE);
    check_copy_image(dib, type, LR_MONOCHROME | LR_CREATEDIBSECTION, 5, 5, 5, 5, depth, TRUE);

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
            check_copy_image(dib, type, 0, 0, 0, 2, 2, screen_depth, FALSE);
            check_copy_image(dib, type, 0, 5, 0, 5, 2, screen_depth, FALSE);
            check_copy_image(dib, type, 0, 0, 5, 2, 5, screen_depth, FALSE);
            check_copy_image(dib, type, 0, 5, 5, 5, 5, screen_depth, FALSE);
            DeleteObject(dib);

            info->bmiHeader.biBitCount = 1;
            info->bmiColors[0].rgbRed = 0;
            info->bmiColors[0].rgbGreen = 0;
            info->bmiColors[0].rgbBlue = 0;
            info->bmiColors[1].rgbRed = 0xFF;
            info->bmiColors[1].rgbGreen = 0xFF;
            info->bmiColors[1].rgbBlue = 0xFF;

            dib = CreateDIBSection(NULL, info, DIB_RGB_COLORS, &bits, NULL, 0);
            check_copy_image(dib, type, 0, 0, 0, 2, 2, 1, FALSE);
            check_copy_image(dib, type, 0, 5, 0, 5, 2, 1, FALSE);
            check_copy_image(dib, type, 0, 0, 5, 2, 5, 1, FALSE);
            check_copy_image(dib, type, 0, 5, 5, 5, 5, 1, FALSE);
            DeleteObject(dib);

            info->bmiHeader.biBitCount = 1;
            info->bmiColors[0].rgbRed = 0xFF;
            info->bmiColors[0].rgbGreen = 0xFF;
            info->bmiColors[0].rgbBlue = 0xFF;
            info->bmiColors[1].rgbRed = 0;
            info->bmiColors[1].rgbGreen = 0;
            info->bmiColors[1].rgbBlue = 0;

            dib = CreateDIBSection(NULL, info, DIB_RGB_COLORS, &bits, NULL, 0);
            check_copy_image(dib, type, 0, 0, 0, 2, 2, 1, FALSE);
            check_copy_image(dib, type, 0, 5, 0, 5, 2, 1, FALSE);
            check_copy_image(dib, type, 0, 0, 5, 2, 5, 1, FALSE);
            check_copy_image(dib, type, 0, 5, 5, 5, 5, 1, FALSE);
            DeleteObject(dib);
        }
    }

    free(info);
}

static void test_initial_cursor(void)
{
    HCURSOR cursor, cursor2;
    DWORD error;

    cursor = GetCursor();

    /* Check what handle GetCursor() returns if a cursor is not set yet. */
    SetLastError(0xdeadbeef);
    cursor2 = LoadCursorA(NULL, (LPCSTR)IDC_WAIT);
    todo_wine {
        ok(cursor == cursor2, "cursor (%p) is not IDC_WAIT (%p).\n", cursor, cursor2);
    }
    error = GetLastError();
    ok(error == 0xdeadbeef, "Last error: 0x%08lx\n", error);
}

static void test_icon_info_(HICON hIcon, UINT exp_cx, UINT exp_cy,
        UINT exp_mask_cy, UINT exp_bpp, const BYTE *mask_bits, UINT has_color, int line)
{
    BYTE ret_bits[1024];
    ICONINFO info;
    DWORD ret;
    BITMAP bmMask, bmColor;

    ret = GetIconInfo(hIcon, &info);
    ok_(__FILE__, line)(ret, "GetIconInfo failed\n");

    /* CreateIcon under XP causes info.fIcon to be 0 */
    ok_(__FILE__, line)(info.xHotspot == exp_cx/2, "info.xHotspot = %lu\n", info.xHotspot);
    ok_(__FILE__, line)(info.yHotspot == exp_cy/2, "info.yHotspot = %lu\n", info.yHotspot);
    ok_(__FILE__, line)(info.hbmMask != 0, "info.hbmMask is NULL\n");

    if (mask_bits)
    {
        unsigned int exp_stride = (exp_cx + 7) / 8;
        exp_stride = (exp_stride + 1) & ~1;
        ret = GetBitmapBits(info.hbmMask, sizeof(ret_bits), ret_bits);
        ok_(__FILE__, line)(ret == exp_stride * exp_mask_cy, "GetBitmapBits returned %lu\n", ret);
        ok_(__FILE__, line)(!memcmp(ret_bits, mask_bits, ret), "mask bitmap didn't match\n");
    }

    ret = GetObjectA(info.hbmMask, sizeof(bmMask), &bmMask);
    ok_(__FILE__, line)(ret == sizeof(bmMask), "GetObject(info.hbmMask) failed, ret %lu\n", ret);

    ok_(__FILE__, line)(!!info.hbmColor == has_color, "got hbmColor %p\n", info.hbmColor);

    if (info.hbmColor)
    {
        HDC hdc;
        UINT display_bpp;

        hdc = GetDC(0);
        display_bpp = GetDeviceCaps(hdc, BITSPIXEL);
        ReleaseDC(0, hdc);

        ret = GetObjectA(info.hbmColor, sizeof(bmColor), &bmColor);
        ok_(__FILE__, line)(ret == sizeof(bmColor), "GetObject(info.hbmColor) failed, ret %lu\n", ret);

        ok_(__FILE__, line)(bmColor.bmBitsPixel == display_bpp,
           "bmColor.bmBitsPixel = %d\n", bmColor.bmBitsPixel);
        ok_(__FILE__, line)(bmColor.bmWidth == exp_cx, "bmColor.bmWidth = %d\n", bmColor.bmWidth);
        ok_(__FILE__, line)(bmColor.bmHeight == exp_cy, "bmColor.bmHeight = %d\n", bmColor.bmHeight);

        ok_(__FILE__, line)(bmMask.bmBitsPixel == 1, "bmMask.bmBitsPixel = %d\n", bmMask.bmBitsPixel);
        ok_(__FILE__, line)(bmMask.bmWidth == exp_cx, "bmMask.bmWidth = %d\n", bmMask.bmWidth);
        ok_(__FILE__, line)(bmMask.bmHeight == exp_mask_cy, "bmMask.bmHeight = %d\n", bmMask.bmHeight);
    }
    else
    {
        ok_(__FILE__, line)(bmMask.bmBitsPixel == 1, "bmMask.bmBitsPixel = %d\n", bmMask.bmBitsPixel);
        ok_(__FILE__, line)(bmMask.bmWidth == exp_cx, "bmMask.bmWidth = %d\n", bmMask.bmWidth);
        ok_(__FILE__, line)(bmMask.bmHeight == exp_mask_cy, "bmMask.bmHeight = %d\n", bmMask.bmHeight);
    }
    if (pGetIconInfoExA)
    {
        ICONINFOEXA infoex;

        memset( &infoex, 0xcc, sizeof(infoex) );
        SetLastError( 0xdeadbeef );
        infoex.cbSize = sizeof(infoex) - 1;
        ret = pGetIconInfoExA( hIcon, &infoex );
        ok_(__FILE__, line)(!ret, "GetIconInfoEx succeeded\n");
        ok_(__FILE__, line)(GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %ld\n", GetLastError());

        SetLastError( 0xdeadbeef );
        infoex.cbSize = sizeof(infoex) + 1;
        ret = pGetIconInfoExA( hIcon, &infoex );
        ok_(__FILE__, line)(!ret, "GetIconInfoEx succeeded\n");
        ok_(__FILE__, line)(GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %ld\n", GetLastError());

        SetLastError( 0xdeadbeef );
        infoex.cbSize = sizeof(infoex);
        ret = pGetIconInfoExA( (HICON)0xdeadbabe, &infoex );
        ok_(__FILE__, line)(!ret, "GetIconInfoEx succeeded\n");
        ok_(__FILE__, line)(GetLastError() == ERROR_INVALID_CURSOR_HANDLE,
                            "wrong error %ld\n", GetLastError());

        infoex.cbSize = sizeof(infoex);
        ret = pGetIconInfoExA( hIcon, &infoex );
        ok_(__FILE__, line)(ret, "GetIconInfoEx failed err %ld\n", GetLastError());
        ok_(__FILE__, line)(infoex.wResID == 0, "GetIconInfoEx wrong resid %x\n", infoex.wResID);
        ok_(__FILE__, line)(infoex.szModName[0] == 0, "GetIconInfoEx wrong module %s\n", infoex.szModName);
        ok_(__FILE__, line)(infoex.szResName[0] == 0, "GetIconInfoEx wrong name %s\n", infoex.szResName);
    }
}

#define test_icon_info(a,b,c,d,e,f,g) test_icon_info_(a,b,c,d,e,f,g,__LINE__)

static void test_CreateIcon(void)
{
    BYTE bmp_bits[1024];
    HICON hIcon;
    HBITMAP hbmMask, hbmColor;
    BITMAPINFO *bmpinfo;
    ICONINFO info;
    HDC hdc;
    void *bits;
    UINT display_bpp;
    int i;

    for (i = 0; i < sizeof(bmp_bits); ++i)
        bmp_bits[i] = 111 * i;

    hdc = GetDC(0);
    display_bpp = GetDeviceCaps(hdc, BITSPIXEL);

    /* these crash under XP
    hIcon = CreateIcon(0, 16, 16, 1, 1, bmp_bits, NULL);
    hIcon = CreateIcon(0, 16, 16, 1, 1, NULL, bmp_bits);
    */

    hIcon = CreateIcon(0, 16, 16, 1, 1, bmp_bits, &bmp_bits[16 * 16 / 8]);
    ok(hIcon != 0, "CreateIcon failed\n");
    test_icon_info(hIcon, 16, 16, 32, 1, bmp_bits, FALSE);
    DestroyIcon(hIcon);

    hIcon = CreateCursor(0, 8, 8, 16, 16, bmp_bits, &bmp_bits[16 * 16 / 8]);
    ok(hIcon != 0, "CreateCursor failed\n");
    test_icon_info(hIcon, 16, 16, 32, 1, bmp_bits, FALSE);
    DestroyCursor(hIcon);

    hIcon = CreateIcon(0, 16, 16, 1, display_bpp, bmp_bits, bmp_bits);
    ok(hIcon != 0, "CreateIcon failed\n");
    test_icon_info(hIcon, 16, 16, 16, display_bpp, bmp_bits, TRUE);
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
    ok(GetLastError() == 0xdeadbeaf, "wrong error %lu\n", GetLastError());

    info.fIcon = TRUE;
    info.xHotspot = 8;
    info.yHotspot = 8;
    info.hbmMask = 0;
    info.hbmColor = hbmColor;
    SetLastError(0xdeadbeaf);
    hIcon = CreateIconIndirect(&info);
    ok(!hIcon, "CreateIconIndirect should fail\n");
    ok(GetLastError() == 0xdeadbeaf, "wrong error %lu\n", GetLastError());

    info.fIcon = TRUE;
    info.xHotspot = 8;
    info.yHotspot = 8;
    info.hbmMask = hbmMask;
    info.hbmColor = hbmColor;
    hIcon = CreateIconIndirect(&info);
    ok(hIcon != 0, "CreateIconIndirect failed\n");
    test_icon_info(hIcon, 16, 16, 16, display_bpp, bmp_bits, TRUE);
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
    test_icon_info(hIcon, 16, 16, 32, 1, bmp_bits, FALSE);
    DestroyIcon(hIcon);

    info.hbmMask = hbmMask;
    info.hbmColor = hbmMask;
    SetLastError(0xdeadbeaf);
    hIcon = CreateIconIndirect(&info);
    ok(hIcon != 0, "CreateIconIndirect failed\n");
    test_icon_info(hIcon, 16, 32, 32, 1, bmp_bits, TRUE);
    DestroyIcon(hIcon);

    DeleteObject(hbmMask);

    for (i = 0; i <= 4; i++)
    {
        hbmMask = CreateBitmap(1, i, 1, 1, bmp_bits);
        ok(hbmMask != 0, "CreateBitmap failed\n");

        info.fIcon = TRUE;
        info.xHotspot = 0;
        info.yHotspot = 0;
        info.hbmMask = hbmMask;
        info.hbmColor = 0;
        SetLastError(0xdeadbeaf);
        hIcon = CreateIconIndirect(&info);
        ok(hIcon != 0, "CreateIconIndirect failed\n");
        test_icon_info(hIcon, 1, i / 2, max(i,1), 1, NULL, FALSE);
        DestroyIcon(hIcon);
        DeleteObject(hbmMask);
    }

    hbmMask = CreateBitmap(16, 32, 1, 16, bmp_bits);
    ok(hbmMask != 0, "CreateBitmap failed\n");

    info.hbmMask = hbmMask;
    info.hbmColor = 0;
    SetLastError(0xdeadbeaf);
    hIcon = CreateIconIndirect(&info);
    ok(hIcon != 0, "CreateIconIndirect failed\n");
    test_icon_info(hIcon, 16, 16, 32, 1, NULL, FALSE);
    DestroyIcon(hIcon);

    DeleteObject(hbmMask);

    /* test creating an icon from a DIB section */

    bmpinfo = calloc( 1, FIELD_OFFSET(BITMAPINFO,bmiColors[256]) );
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
    test_icon_info(hIcon, 32, 32, 32, 8, NULL, TRUE);
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
    test_icon_info(hIcon, 32, 32, 32, 8, NULL, TRUE);
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
    test_icon_info(hIcon, 32, 32, 32, 8, NULL, TRUE);
    DestroyIcon(hIcon);

    DeleteObject(hbmMask);
    DeleteObject(hbmColor);
    free(bmpinfo);

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

/* 1x1 pixel bmp with gap between palette and bitmap. Correct bitmap contains only
   zeroes, gap is 0xFF. */
static unsigned char bmpimage[70] = {
0x42,0x4d,0x46,0x00,0x00,0x00,0xDE,0xAD,0xBE,0xEF,0x42,0x00,0x00,0x00,0x28,0x00,
0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x00,0x00,
0x00,0x00,0x04,0x00,0x00,0x00,0x12,0x0b,0x00,0x00,0x12,0x0b,0x00,0x00,0x02,0x00,
0x00,0x00,0x02,0x00,0x00,0x00,0xff,0xff,0xff,0x00,0x55,0x55,0x55,0x00,0xFF,0xFF,
0xFF,0xFF,0x00,0x00,0x00,0x00
};

/* 1x1 pixel bmp using BITMAPCOREHEADER with 24 bits colors (1 bit color fails to load) */
static const unsigned char bmpcoreimage[38] = {
0x42,0x4d,0x1e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1a,0x00,0x00,0x00,0x0c,0x00,
0x00,0x00,0x01,0x00,0x01,0x00,0x01,0x00,0x18,0x00,0xff,0xff,0xff,0x00
};

/* 2x2 pixel gif */
static const unsigned char gif4pixel[42] = {
0x47,0x49,0x46,0x38,0x37,0x61,0x02,0x00,0x02,0x00,0xa1,0x00,0x00,0x00,0x00,0x00,
0x39,0x62,0xfc,0xff,0x1a,0xe5,0xff,0xff,0xff,0x2c,0x00,0x00,0x00,0x00,0x02,0x00,
0x02,0x00,0x00,0x02,0x03,0x14,0x16,0x05,0x00,0x3b
};

/* An invalid cursor with an invalid dwDIBOffset */
static const unsigned char invalid_dwDIBOffset[] = {
  0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00
};

static const DWORD biSize_tests[] = {
    0,
    sizeof(BITMAPCOREHEADER) - 1,
    sizeof(BITMAPCOREHEADER) + 1,
    sizeof(BITMAPINFOHEADER) - 1,
    sizeof(BITMAPINFOHEADER) + 1,
    sizeof(BITMAPV4HEADER) - 1,
    sizeof(BITMAPV4HEADER) + 1,
    sizeof(BITMAPV5HEADER) - 1,
    sizeof(BITMAPV5HEADER) + 1,
    (sizeof(BITMAPCOREHEADER) + sizeof(BITMAPINFOHEADER)) / 2,
    (sizeof(BITMAPV4HEADER) + sizeof(BITMAPV5HEADER)) / 2,
    0xdeadbeef,
    0xffffffff
};

static void test_LoadImageBitmap(HBITMAP hbm)
{
    BITMAP bm;
    BITMAPINFO bmi;
    DWORD ret, pixel = 0;
    HDC hdc = GetDC(NULL);

    ret = GetObjectA(hbm, sizeof(bm), &bm);
    ok(ret == sizeof(bm), "GetObject returned %ld\n", ret);

    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = bm.bmWidth;
    bmi.bmiHeader.biHeight = bm.bmHeight;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount= 24;
    bmi.bmiHeader.biCompression= BI_RGB;
    ret = GetDIBits(hdc, hbm, 0, bm.bmHeight, &pixel, &bmi, DIB_RGB_COLORS);
    ok(ret == bm.bmHeight, "%ld lines were converted, not %d\n", ret, bm.bmHeight);

    ok(color_match(pixel, 0x00ffffff), "Pixel is 0x%08lx\n", pixel);

    ReleaseDC(NULL, hdc);
}

static void test_LoadImageFile(const char * test_desc, const unsigned char * image_data,
    unsigned int image_size, const char * ext, BOOL expect_success)
{
    HANDLE handle;
    BOOL ret;
    DWORD error, bytes_written;
    char filename[64];

    winetest_push_context("%s", test_desc);
    strcpy(filename, "test.");
    strcat(filename, ext);

    /* Create the test image. */
    handle = CreateFileA(filename, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_NEW,
        FILE_ATTRIBUTE_NORMAL, NULL);
    ok(handle != INVALID_HANDLE_VALUE, "CreateFileA failed. %lu\n", GetLastError());
    ret = WriteFile(handle, image_data, image_size, &bytes_written, NULL);
    ok(ret && bytes_written == image_size, "test file created improperly.\n");
    CloseHandle(handle);

    /* Load as cursor. For all tested formats, this should fail */
    SetLastError(0xdeadbeef);
    handle = LoadImageA(NULL, filename, IMAGE_CURSOR, 0, 0, LR_LOADFROMFILE);
    ok(handle == NULL, "IMAGE_CURSOR succeeded incorrectly\n");
    error = GetLastError();
    ok(error == 0, "Last error: %lu\n", error);
    if (handle != NULL) DestroyCursor(handle);

    /* Load as icon. For all tested formats, this should fail */
    SetLastError(0xdeadbeef);
    handle = LoadImageA(NULL, filename, IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
    ok(handle == NULL, "IMAGE_ICON succeeded incorrectly\n");
    error = GetLastError();
    ok(error == 0, "Last error: %lu\n", error);
    if (handle != NULL) DestroyIcon(handle);

    /* Load as bitmap. Should succeed for correct bmp, fail for everything else */
    SetLastError(0xdeadbeef);
    handle = LoadImageA(NULL, filename, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    error = GetLastError();
    ok(error == 0, "Last error: %lu\n", error);

    if (expect_success) {
        ok(handle != NULL || broken(image_data == bmpcoreimage) /* pre-security update */,
           "IMAGE_BITMAP failed\n");
        if (handle != NULL) test_LoadImageBitmap(handle);
    }
    else ok(handle == NULL, "IMAGE_BITMAP succeeded incorrectly\n");

    if (handle != NULL) DeleteObject(handle);
    DeleteFileA(filename);
    winetest_pop_context();
}

typedef struct {
    unsigned width;
    unsigned height;
    BOOL invalid_offset;
} test_icon_entries_t;

static void create_ico_file(const char *filename, const test_icon_entries_t *test_icon_entries, unsigned entry_cnt)
{
    CURSORICONFILEDIRENTRY *icon_entry;
    BITMAPINFOHEADER *icon_header;
    CURSORICONFILEDIR *dir;
    BYTE *buf, *bitmap_ptr;
    DWORD bytes_written;
    size_t icon_size;
    HANDLE file;
    unsigned i;
    BOOL ret;

    const unsigned icon_bpp = 32;

    icon_size = FIELD_OFFSET(CURSORICONFILEDIR, idEntries[entry_cnt]) + sizeof(BITMAPINFOHEADER)*entry_cnt;
    for(i=0; i<entry_cnt; i++)
        icon_size += icon_bpp * test_icon_entries[i].width * test_icon_entries[i].height / 8;

    buf = calloc(1, icon_size);
    dir = (CURSORICONFILEDIR*)buf;

    dir->idReserved = 0;
    dir->idType = 1;
    dir->idCount = entry_cnt;

    bitmap_ptr = buf + FIELD_OFFSET(CURSORICONFILEDIR, idEntries[entry_cnt]);
    for(i=0; i<entry_cnt; i++) {
        icon_entry = dir->idEntries+i;
        icon_entry->bWidth = test_icon_entries[i].width;
        icon_entry->bHeight = test_icon_entries[i].height;
        icon_entry->bColorCount = 0;
        icon_entry->bReserved = 0;
        icon_entry->xHotspot = 1;
        icon_entry->yHotspot = 1;
        icon_entry->dwDIBSize = sizeof(BITMAPINFOHEADER) + icon_entry->bWidth * icon_entry->bHeight * icon_bpp / 8;
        icon_entry->dwDIBOffset = test_icon_entries[i].invalid_offset ? 0xffffffff : bitmap_ptr - buf;

        icon_header = (BITMAPINFOHEADER*)bitmap_ptr;
        bitmap_ptr += icon_entry->dwDIBSize;

        icon_header->biSize = sizeof(BITMAPINFOHEADER);
        icon_header->biWidth = icon_entry->bWidth;
        icon_header->biHeight = icon_entry->bHeight;
        icon_header->biPlanes = 1;
        icon_header->biBitCount = icon_bpp;
        icon_header->biSizeImage = 0; /* Uncompressed bitmap. */
    }

    memset(bitmap_ptr, 0xf0, buf+icon_size-bitmap_ptr);

    /* Create the icon. */
    file = CreateFileA(filename, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFileA failed. %lu\n", GetLastError());
    ret = WriteFile(file, buf, icon_size, &bytes_written, NULL);
    ok(ret && bytes_written == icon_size, "icon.ico created improperly.\n");
    CloseHandle(file);

    free(buf);
}

static void create_bitmap_file(const char *filename, const BITMAPINFO *bmi, const unsigned char *bits)
{
    unsigned int clr_used, bmi_size, bits_size, stride;
    const BITMAPINFOHEADER *h = &bmi->bmiHeader;
    BITMAPFILEHEADER hdr;
    DWORD bytes_written;
    HANDLE file;
    BOOL ret;

    clr_used = h->biBitCount <= 8 ? 1u << h->biBitCount : 0;
    stride = ((h->biBitCount * h->biWidth + 7) / 8 + 3) & ~3;
    bits_size = h->biHeight * stride;
    bmi_size = h->biSize + clr_used * sizeof(RGBQUAD);

    hdr.bfType = 0x4d42;
    hdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + bmi_size;
    hdr.bfSize = hdr.bfOffBits + bits_size;
    hdr.bfReserved1 = 0;
    hdr.bfReserved2 = 0;

    file = CreateFileA(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFileA failed, result %lu.\n", GetLastError());
    ret = WriteFile(file, &hdr, sizeof(hdr), &bytes_written, NULL);
    ok(ret && bytes_written == sizeof(hdr), "Unexpected WriteFile() result, ret %#x, bytes_written %lu.\n",
            ret, bytes_written);
    ret = WriteFile(file, bmi, bmi_size, &bytes_written, NULL);
    ok(ret && bytes_written == bmi_size, "Unexpected WriteFile() result, ret %#x, bytes_written %lu.\n",
            ret, bytes_written);
    ret = WriteFile(file, bits, bits_size, &bytes_written, NULL);
    ok(ret && bytes_written == bits_size, "Unexpected WriteFile() result, ret %#x, bytes_written %lu.\n",
            ret, bytes_written);
    CloseHandle(file);
}

static void test_LoadImage_working_directory_run(char *path)
{
    DWORD bytes_written;
    HANDLE handle;
    BOOL ret;
    char path_icon[MAX_PATH];
    char path_image[MAX_PATH];
    static const test_icon_entries_t icon_desc = {32, 32};

    sprintf(path_icon, "%s\\icon.ico", path);
    sprintf(path_image,  "%s\\test.bmp", path);

    /* Create Files */
    create_ico_file(path_icon, &icon_desc, 1);

    handle = CreateFileA(path_image, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(handle != INVALID_HANDLE_VALUE, "run %s: CreateFileA failed. %lu\n", path, GetLastError());
    ret = WriteFile(handle, bmpimage, sizeof(bmpimage), &bytes_written, NULL);
    ok(ret && bytes_written == sizeof(bmpimage), "run %s: Test file created improperly.\n", path);
    CloseHandle(handle);

    /* Test cursor */
    handle = LoadImageA(NULL, "icon.ico", IMAGE_CURSOR, 0, 0, LR_LOADFROMFILE);
    ok(handle != NULL, "run %s: LoadImage() failed.\n", path);

    ret = DestroyIcon(handle);
    ok(ret, "run %s: DestroyIcon failed: %ld\n", path, GetLastError());

    /* Test image */
    handle = LoadImageA(NULL, "test.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    ok(handle != NULL, "run %s: LoadImageA failed.\n", path);

    ret = DeleteObject(handle);
    ok(ret, "run %s: DeleteObject failed: %ld\n", path, GetLastError());

    /* Cleanup */
    ret = DeleteFileA(path_image);
    ok(ret, "run %s: DeleteFileA failed: %ld\n", path, GetLastError());
    ret = DeleteFileA(path_icon);
    ok(ret, "run %s: DeleteFileA failed: %ld\n", path, GetLastError());
}

static void test_LoadImage_working_directory(void)
{
    char old_working_dir[MAX_PATH];
    char temp_dir_current[MAX_PATH];
    char temp_dir_PATH[MAX_PATH];
    char executable_path[MAX_PATH];
    int pos_slash;
    char old_PATH[10000];
    char new_PATH[10000 + MAX_PATH];
    BOOL ret;

    GetCurrentDirectoryA(ARRAY_SIZE(old_working_dir), old_working_dir);

    GetTempPathA(ARRAY_SIZE(temp_dir_current), temp_dir_current);
    strcat(temp_dir_current, "wine-test-dir-current\\");
    GetTempPathA(ARRAY_SIZE(temp_dir_PATH), temp_dir_PATH);
    strcat(temp_dir_PATH,    "wine-test-dir-path\\");

    GetModuleFileNameA(NULL, executable_path, ARRAY_SIZE(executable_path));
    pos_slash = strrchr(executable_path, '\\') - executable_path;
    executable_path[pos_slash + 1] = 0;

    CreateDirectoryA(temp_dir_current, NULL);
    CreateDirectoryA(temp_dir_PATH, NULL);

    SetCurrentDirectoryA(temp_dir_current);

    GetEnvironmentVariableA("PATH", old_PATH, ARRAY_SIZE(old_PATH));
    sprintf(new_PATH, "%s;%s", old_PATH, temp_dir_PATH);
    SetEnvironmentVariableA("PATH", new_PATH);

    test_LoadImage_working_directory_run(temp_dir_current);
    test_LoadImage_working_directory_run(executable_path);
    test_LoadImage_working_directory_run(temp_dir_PATH);

    SetCurrentDirectoryA(old_working_dir);
    SetEnvironmentVariableA("PATH", old_PATH);

    ret = RemoveDirectoryA(temp_dir_current);
    ok(ret, "RemoveDirectoryA failed: %ld\n", GetLastError());
    ret = RemoveDirectoryA(temp_dir_PATH);
    ok(ret, "RemoveDirectoryA failed: %ld\n", GetLastError());
}

static void test_LoadImage(void)
{
    HANDLE handle;
    BOOL ret;
    DWORD error;
    BITMAPINFOHEADER *bitmap_header;
    ICONINFO icon_info;
    int i;

#define ICON_WIDTH 32
#define ICON_HEIGHT 32
#define ICON_AND_SIZE (ICON_WIDTH*ICON_HEIGHT/8)
#define ICON_BPP 32
#define ICON_SIZE \
    (sizeof(CURSORICONFILEDIR) + sizeof(BITMAPINFOHEADER) \
    + ICON_AND_SIZE + ICON_AND_SIZE*ICON_BPP)

    static const test_icon_entries_t icon_desc = {32, 32};

    create_ico_file("icon.ico", &icon_desc, 1);

    /* Test loading an icon as a cursor. */
    SetLastError(0xdeadbeef);
    handle = LoadImageA(NULL, "icon.ico", IMAGE_CURSOR, 0, 0, LR_LOADFROMFILE);
    ok(handle != NULL, "LoadImage() failed.\n");
    error = GetLastError();
    ok(error == 0, "Last error: %lu\n", error);

    /* Test the icon information. */
    SetLastError(0xdeadbeef);
    ret = GetIconInfo(handle, &icon_info);
    ok(ret, "GetIconInfo() failed.\n");
    error = GetLastError();
    ok(error == 0xdeadbeef, "Last error: %lu\n", error);

    if (ret)
    {
        ok(icon_info.fIcon == FALSE, "fIcon != FALSE.\n");
        ok(icon_info.xHotspot == 1, "xHotspot is %lu.\n", icon_info.xHotspot);
        ok(icon_info.yHotspot == 1, "yHotspot is %lu.\n", icon_info.yHotspot);
        ok(icon_info.hbmColor != NULL, "No hbmColor!\n");
        ok(icon_info.hbmMask != NULL, "No hbmMask!\n");
    }

    if (pGetIconInfoExA)
    {
        ICONINFOEXA infoex;
        infoex.cbSize = sizeof(infoex);
        ret = pGetIconInfoExA( handle, &infoex );
        ok( ret, "GetIconInfoEx failed err %ld\n", GetLastError() );
        ok( infoex.wResID == 0, "GetIconInfoEx wrong resid %x\n", infoex.wResID );
        ok( infoex.szModName[0] == 0, "GetIconInfoEx wrong module %s\n", infoex.szModName );
        ok( infoex.szResName[0] == 0, "GetIconInfoEx wrong name %s\n", infoex.szResName );
    }
    else win_skip( "GetIconInfoEx not available\n" );

    /* Clean up. */
    SetLastError(0xdeadbeef);
    ret = DestroyCursor(handle);
    ok(ret, "DestroyCursor() failed.\n");
    error = GetLastError();
    ok(error == 0xdeadbeef, "Last error: %lu\n", error);

    DeleteFileA("icon.ico");

    /* Test a system icon */
    handle = LoadIconA( 0, (LPCSTR)IDI_HAND );
    ok(handle != NULL, "LoadImage() failed.\n");
    if (pGetIconInfoExA)
    {
        ICONINFOEXA infoexA;
        ICONINFOEXW infoexW;
        infoexA.cbSize = sizeof(infoexA);
        ret = pGetIconInfoExA( handle, &infoexA );
        ok( ret, "GetIconInfoEx failed err %ld\n", GetLastError() );
        ok( infoexA.wResID == (UINT_PTR)IDI_HAND, "GetIconInfoEx wrong resid %x\n", infoexA.wResID );
        /* the A version is broken on 64-bit, it truncates the string after the first char */
        if (is_win64 && infoexA.szModName[0] && infoexA.szModName[1] == 0)
            trace( "GetIconInfoExA broken on Win64\n" );
        else
            ok( GetModuleHandleA(infoexA.szModName) == GetModuleHandleA("user32.dll"),
                "GetIconInfoEx wrong module %s\n", infoexA.szModName );
        ok( infoexA.szResName[0] == 0, "GetIconInfoEx wrong name %s\n", infoexA.szResName );
        infoexW.cbSize = sizeof(infoexW);
        ret = pGetIconInfoExW( handle, &infoexW );
        ok( ret, "GetIconInfoEx failed err %ld\n", GetLastError() );
        ok( infoexW.wResID == (UINT_PTR)IDI_HAND, "GetIconInfoEx wrong resid %x\n", infoexW.wResID );
        ok( GetModuleHandleW(infoexW.szModName) == GetModuleHandleA("user32.dll"),
            "GetIconInfoEx wrong module %s\n", wine_dbgstr_w(infoexW.szModName) );
        ok( infoexW.szResName[0] == 0, "GetIconInfoEx wrong name %s\n", wine_dbgstr_w(infoexW.szResName) );
    }
    SetLastError(0xdeadbeef);
    DestroyIcon(handle);

    test_LoadImageFile("BMP", bmpimage, sizeof(bmpimage), "bmp", 1);
    test_LoadImageFile("BMP (coreinfo)", bmpcoreimage, sizeof(bmpcoreimage), "bmp", 1);
    test_LoadImageFile("GIF", gifimage, sizeof(gifimage), "gif", 0);
    test_LoadImageFile("GIF (2x2 pixel)", gif4pixel, sizeof(gif4pixel), "gif", 0);
    test_LoadImageFile("JPG", jpgimage, sizeof(jpgimage), "jpg", 0);
    test_LoadImageFile("PNG", pngimage, sizeof(pngimage), "png", 0);

    /* Check failure for broken BMP images */
    bitmap_header = (BITMAPINFOHEADER *)(bmpimage + sizeof(BITMAPFILEHEADER));

    bitmap_header->biHeight = 65536;
    test_LoadImageFile("BMP (too high)", bmpimage, sizeof(bmpimage), "bmp", 0);
    bitmap_header->biHeight = 1;

    bitmap_header->biWidth = 65536;
    test_LoadImageFile("BMP (too wide)", bmpimage, sizeof(bmpimage), "bmp", 0);
    bitmap_header->biWidth = 1;

    for (i = 0; i < ARRAY_SIZE(biSize_tests); i++) {
        bitmap_header->biSize = biSize_tests[i];
        test_LoadImageFile("BMP (broken biSize)", bmpimage, sizeof(bmpimage), "bmp", 0);
    }
    bitmap_header->biSize = sizeof(BITMAPINFOHEADER);

    test_LoadImageFile("Cursor (invalid dwDIBOffset)", invalid_dwDIBOffset, sizeof(invalid_dwDIBOffset), "cur", 0);

    /* Test in which paths images with a relative path can be found */
    test_LoadImage_working_directory();
}

static BOOL CALLBACK find_res_proc(HMODULE module, LPCSTR type, LPSTR name, LONG_PTR param)
{
    char **res_name = (char **)param;
    *res_name = name;
    return FALSE;
}

static void test_CreateIconFromResource(void)
{
    HANDLE handle, old_handle;
    BOOL ret;
    DWORD error;
    BITMAPINFOHEADER *icon_header;
    INT16 *hotspot;
    ICONINFO icon_info;
    HMODULE user32;
    char *res_name;
    HRSRC rsrc;
    HGLOBAL res;
    BYTE *bits;
    UINT size;


#define ICON_RES_WIDTH 32
#define ICON_RES_HEIGHT 32
#define ICON_RES_AND_SIZE (ICON_WIDTH*ICON_HEIGHT/8)
#define ICON_RES_BPP 32
#define ICON_RES_SIZE \
    (sizeof(BITMAPINFOHEADER) + ICON_AND_SIZE + ICON_AND_SIZE*ICON_BPP)
#define CRSR_RES_SIZE (2*sizeof(INT16) + ICON_RES_SIZE)

    /* Set icon data. */
    hotspot = calloc(1, CRSR_RES_SIZE);

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
    ret = DestroyCursor(handle);
    ok(ret, "Destroy cursor failed, error %lu.\n", GetLastError());

    /* Test the icon information. */
    SetLastError(0xdeadbeef);
    ret = GetIconInfo(handle, &icon_info);
    ok(ret, "GetIconInfo() failed.\n");
    error = GetLastError();
    ok(error == 0xdeadbeef, "Last error: %lu\n", error);

    if (ret)
    {
        ok(icon_info.fIcon == FALSE, "fIcon != FALSE.\n");
        ok(icon_info.xHotspot == 3, "xHotspot is %lu.\n", icon_info.xHotspot);
        ok(icon_info.yHotspot == 3, "yHotspot is %lu.\n", icon_info.yHotspot);
        ok(icon_info.hbmColor != NULL, "No hbmColor!\n");
        ok(icon_info.hbmMask != NULL, "No hbmMask!\n");
    }

    if (pGetIconInfoExA)
    {
        ICONINFOEXA infoex;
        infoex.cbSize = sizeof(infoex);
        ret = pGetIconInfoExA( handle, &infoex );
        ok( ret, "GetIconInfoEx failed err %ld\n", GetLastError() );
        ok( infoex.wResID == 0, "GetIconInfoEx wrong resid %x\n", infoex.wResID );
        ok( infoex.szModName[0] == 0, "GetIconInfoEx wrong module %s\n", infoex.szModName );
        ok( infoex.szResName[0] == 0, "GetIconInfoEx wrong name %s\n", infoex.szResName );
    }

    /* Test creating an icon. */
    SetLastError(0xdeadbeef);
    handle = CreateIconFromResource((PBYTE) icon_header, ICON_RES_SIZE, TRUE,
				    0x00030000);
    ok(handle != NULL, "Create icon failed.\n");
    ret = DestroyIcon(handle);
    ok(ret, "Destroy icon failed, error %lu.\n", GetLastError());

    /* Test the icon information. */
    SetLastError(0xdeadbeef);
    ret = GetIconInfo(handle, &icon_info);
    ok(ret, "GetIconInfo() failed.\n");
    error = GetLastError();
    ok(error == 0xdeadbeef, "Last error: %lu\n", error);

    if (ret)
    {
        ok(icon_info.fIcon == TRUE, "fIcon != TRUE.\n");
	/* Icons always have hotspot in the middle */
        ok(icon_info.xHotspot == ICON_WIDTH/2, "xHotspot is %lu.\n", icon_info.xHotspot);
        ok(icon_info.yHotspot == ICON_HEIGHT/2, "yHotspot is %lu.\n", icon_info.yHotspot);
        ok(icon_info.hbmColor != NULL, "No hbmColor!\n");
        ok(icon_info.hbmMask != NULL, "No hbmMask!\n");
    }

    /* Rejection of NULL pointer crashes at least on WNT4WSSP6, W2KPROSP4, WXPPROSP3
     *
     * handle = CreateIconFromResource(NULL, ICON_RES_SIZE, TRUE, 0x00030000);
     * ok(handle == NULL, "Invalid pointer accepted (%p)\n", handle);
     */

    /* Test creating an animated cursor. */
    empty_anicursor.frames[0].data.icon_info.idType = 2; /* type: cursor */
    empty_anicursor.frames[0].data.icon_info.idEntries[0].xHotspot = 3;
    empty_anicursor.frames[0].data.icon_info.idEntries[0].yHotspot = 3;
    handle = CreateIconFromResource((PBYTE) &empty_anicursor, sizeof(empty_anicursor), FALSE, 0x00030000);
    ok(handle != NULL, "Create cursor failed.\n");
    ret = DestroyCursor(handle);
    ok(ret, "Destroy cursor failed, error %lu.\n", GetLastError());

    /* Test the animated cursor's information. */
    SetLastError(0xdeadbeef);
    ret = GetIconInfo(handle, &icon_info);
    todo_wine
    ok(ret, "GetIconInfo() failed.\n");
    error = GetLastError();
    todo_wine
    ok(error == 0xdeadbeef, "Last error: %lu\n", error);

    if (ret)
    {
        ok(icon_info.fIcon == FALSE, "fIcon != FALSE.\n");
        ok(icon_info.xHotspot == 3, "xHotspot is %lu.\n", icon_info.xHotspot);
        ok(icon_info.yHotspot == 3, "yHotspot is %lu.\n", icon_info.yHotspot);
        ok(icon_info.hbmColor != NULL, "No hbmColor!\n");
        ok(icon_info.hbmMask != NULL, "No hbmMask!\n");
    }

    /* Test creating and destroying a non-shared icon. */
    handle = CreateIconFromResourceEx((BYTE *)icon_header, ICON_RES_SIZE, TRUE, 0x00030000,
                                       0, 0, LR_DEFAULTSIZE);
    ok(handle != NULL, "Create icon failed, error %lu.\n", GetLastError());
    ret = DestroyIcon(handle);
    ok(ret, "Destroy icon failed, error %lu.\n", GetLastError());
    ret = GetIconInfo(handle, &icon_info);
    ok(!ret, "Get info succeeded.\n");
    ok(GetLastError() == ERROR_INVALID_CURSOR_HANDLE, "Got unexpected error %lu.\n", error);

    /* Test creating and destroying a shared icon from heap bits. */
    handle = CreateIconFromResourceEx((BYTE *)icon_header, ICON_RES_SIZE, TRUE, 0x00030000,
                                       0, 0, LR_DEFAULTSIZE | LR_SHARED);
    ok(handle != NULL, "Create icon failed, error %lu.\n", GetLastError());
    ret = DestroyIcon(handle);
    ok(ret, "Destroy icon failed, error %lu.\n", GetLastError());
    ret = GetIconInfo(handle, &icon_info);
    ok(ret, "Get info failed, error %lu.\n", GetLastError());

    /* Test creating a shared icon from heap bits that has been created before. */
    old_handle = handle;
    handle = CreateIconFromResourceEx((BYTE *)icon_header, ICON_RES_SIZE, TRUE, 0x00030000,
                                       0, 0, LR_DEFAULTSIZE | LR_SHARED);
    ok(handle != NULL, "Create icon failed, error %lu.\n", GetLastError());
    ok(handle != old_handle, "Expect a different handle.\n");

    free(hotspot);

    /* Get icon resource bits */
    user32 = GetModuleHandleA("user32.dll");
    EnumResourceNamesA(user32, (const char *)RT_GROUP_ICON, find_res_proc, (LONG_PTR)&res_name);
    rsrc = FindResourceA(user32, res_name, (const char *)RT_GROUP_ICON);
    ok(rsrc != NULL, "Find resource failed, error %lu.\n", GetLastError());
    res = LoadResource(user32, rsrc);
    ok(res != NULL, "Load resource failed, error %lu.\n", GetLastError());
    bits = LockResource(res);
    ok(bits != NULL, "Lock resource failed, error %lu.\n", GetLastError());

    res_name = MAKEINTRESOURCEA(LookupIconIdFromDirectory(bits, TRUE));
    rsrc = FindResourceA(user32, res_name, (const char *)RT_ICON);
    ok(rsrc != NULL, "Find resource failed, error %lu.\n", GetLastError());
    size = SizeofResource(user32, rsrc);
    ok(size != 0, "Get resource size failed, error %lu.\n", GetLastError());
    res = LoadResource(user32, rsrc);
    ok(res != NULL, "Load resource failed, error %lu.\n", GetLastError());
    bits = LockResource(res);
    ok(bits != NULL, "Lock resource failed, error %lu.\n", GetLastError());

    /* Test creating and destroying a shared icon from resource bits. */
    handle = CreateIconFromResourceEx(bits, size, TRUE, 0x00030000, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
    ok(handle != NULL, "Create icon failed, error %lu\n", GetLastError());
    ret = DestroyIcon(handle);
    ok(ret, "Destroy icon failed, error %lu.\n", GetLastError());
    ret = GetIconInfo(handle, &icon_info);
    ok(ret, "Get info failed, error %lu.\n", GetLastError());

    /* Test creating a shared icon from resource bits that has been created before. */
    old_handle = handle;
    handle = CreateIconFromResourceEx(bits, size, TRUE, 0x00030000, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
    ok(handle != NULL, "Create icon failed, error %lu.\n", GetLastError());
    ok(handle != old_handle, "Expect a different handle.\n");
}

static int check_cursor_data( HDC hdc, HCURSOR hCursor, void *data, int length)
{
    char *image = NULL;
    BITMAPINFO *info;
    ICONINFO iinfo;
    DWORD ret;
    int i;

    ret = GetIconInfo( hCursor, &iinfo );
    ok(ret, "GetIconInfo() failed\n");
    if (!ret) return 0;
    ret = 0;
    info = malloc( FIELD_OFFSET( BITMAPINFO, bmiColors[256] ));
    ok(info != NULL, "malloc() failed\n");
    if (!info) return 0;

    info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info->bmiHeader.biWidth = 32;
    info->bmiHeader.biHeight = 32;
    info->bmiHeader.biPlanes = 1;
    info->bmiHeader.biBitCount = 32;
    info->bmiHeader.biCompression = BI_RGB;
    info->bmiHeader.biSizeImage = 32 * 32 * 4;
    info->bmiHeader.biXPelsPerMeter = 0;
    info->bmiHeader.biYPelsPerMeter = 0;
    info->bmiHeader.biClrUsed = 0;
    info->bmiHeader.biClrImportant = 0;
    image = malloc( info->bmiHeader.biSizeImage );
    ok(image != NULL, "malloc() failed\n");
    if (!image) goto cleanup;
    ret = GetDIBits( hdc, iinfo.hbmColor, 0, 32, image, info, DIB_RGB_COLORS );
    ok(ret, "GetDIBits() failed\n");
    for (i = 0; ret && i < length / sizeof(COLORREF); i++)
    {
        ret = color_match( ((COLORREF *)data)[i], ((COLORREF *)image)[i] );
        ok(ret, "%04x: Expected 0x%lx, actually 0x%lx\n", i, ((COLORREF *)data)[i], ((COLORREF *)image)[i] );
    }
cleanup:
    free( image );
    free( info );
    return ret;
}

static HCURSOR (WINAPI *pGetCursorFrameInfo)(HCURSOR hCursor, DWORD unk1, DWORD istep, DWORD *rate, DWORD *steps);
static void test_GetCursorFrameInfo(void)
{
    DWORD frame_identifier[] = { 0x10Ad, 0xc001, 0x1c05 };
    HBITMAP bmp = NULL, bmpOld = NULL;
    DWORD rate, steps;
    BITMAPINFOHEADER *icon_header;
    BITMAPINFO bitmapInfo;
    HDC hdc = NULL;
    void *bits = 0;
    INT16 *hotspot;
    HANDLE h1, h2;
    BOOL ret;
    int i;

    if (!pGetCursorFrameInfo)
    {
        win_skip( "GetCursorFrameInfo not supported, skipping tests.\n" );
        return;
    }

    hdc = CreateCompatibleDC(0);
    ok(hdc != 0, "CreateCompatibleDC(0) failed to return a valid DC\n");
    if (!hdc)
        return;

    memset(&bitmapInfo, 0, sizeof(bitmapInfo));
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = 3;
    bitmapInfo.bmiHeader.biHeight = 3;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;
    bitmapInfo.bmiHeader.biSizeImage = sizeof(UINT32);
    bmp = CreateDIBSection(hdc, &bitmapInfo, DIB_RGB_COLORS, &bits, NULL, 0);
    ok (bmp && bits, "CreateDIBSection failed to return a valid bitmap and buffer\n");
    if (!bmp || !bits)
        goto cleanup;
    bmpOld = SelectObject(hdc, bmp);

#define ICON_RES_WIDTH 32
#define ICON_RES_HEIGHT 32
#define ICON_RES_AND_SIZE (ICON_WIDTH*ICON_HEIGHT/8)
#define ICON_RES_BPP 32
#define ICON_RES_SIZE \
    (sizeof(BITMAPINFOHEADER) + ICON_AND_SIZE + ICON_AND_SIZE*ICON_BPP)
#define CRSR_RES_SIZE (2*sizeof(INT16) + ICON_RES_SIZE)

    /* Set icon data. */
    hotspot = calloc(1, CRSR_RES_SIZE);

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

    /* Creating a static cursor. */
    SetLastError(0xdeadbeef);
    h1 = CreateIconFromResource((PBYTE) hotspot, CRSR_RES_SIZE, FALSE, 0x00030000);
    ok(h1 != NULL, "Create cursor failed (error = %ld).\n", GetLastError());

    /* Check GetCursorFrameInfo behavior on a static cursor */
    rate = steps = 0xdead;
    h2 = pGetCursorFrameInfo(h1, 0xdead, 0xdead, &rate, &steps);
    ok(h1 == h2, "GetCursorFrameInfo() failed: (%p != %p).\n", h1, h2);
    ok(rate == 0, "GetCursorFrameInfo() unexpected param 4 value (0x%lx != 0x0).\n", rate);
    ok(steps == 1, "GetCursorFrameInfo() unexpected param 5 value (%ld != 1).\n", steps);

    /* Clean up static cursor. */
    SetLastError(0xdeadbeef);
    ret = DestroyCursor(h1);
    ok(ret, "DestroyCursor() failed (error = %ld).\n", GetLastError());

    /* Creating a single-frame animated cursor. */
    empty_anicursor.frames[0].data.icon_info.idType = 2; /* type: cursor */
    empty_anicursor.frames[0].data.icon_info.idEntries[0].xHotspot = 3;
    empty_anicursor.frames[0].data.icon_info.idEntries[0].yHotspot = 3;
    memcpy( &empty_anicursor.frames[0].data.bmi_data.data[0], &frame_identifier[0], sizeof(DWORD) );
    SetLastError(0xdeadbeef);
    h1 = CreateIconFromResource((PBYTE) &empty_anicursor, sizeof(empty_anicursor), FALSE, 0x00030000);
    ok(h1 != NULL, "Create cursor failed (error = %ld).\n", GetLastError());

    /* Check GetCursorFrameInfo behavior on a single-frame animated cursor */
    rate = steps = 0xdead;
    h2 = pGetCursorFrameInfo(h1, 0xdead, 0, &rate, &steps);
    ok(h1 == h2, "GetCursorFrameInfo() failed: (%p != %p).\n", h1, h2);
    ret = check_cursor_data( hdc, h2, &frame_identifier[0], sizeof(DWORD) );
    ok(ret, "GetCursorFrameInfo() returned wrong cursor data for frame 0.\n");
    ok(rate == 0x0, "GetCursorFrameInfo() unexpected param 4 value (0x%lx != 0x0).\n", rate);
    ok(steps == empty_anicursor.header.header.num_steps,
        "GetCursorFrameInfo() unexpected param 5 value (%ld != 1).\n", steps);

    /* Clean up single-frame animated cursor. */
    SetLastError(0xdeadbeef);
    ret = DestroyCursor(h1);
    ok(ret, "DestroyCursor() failed (error = %ld).\n", GetLastError());

    /* Creating a multi-frame animated cursor. */
    for (i=0; i<empty_anicursor3.header.header.num_frames; i++)
    {
        empty_anicursor3.frames[i].data.icon_info.idType = 2; /* type: cursor */
        empty_anicursor3.frames[i].data.icon_info.idEntries[0].xHotspot = 3;
        empty_anicursor3.frames[i].data.icon_info.idEntries[0].yHotspot = 3;
        memcpy( &empty_anicursor3.frames[i].data.bmi_data.data[0], &frame_identifier[i], sizeof(DWORD) );
    }
    SetLastError(0xdeadbeef);
    h1 = CreateIconFromResource((PBYTE) &empty_anicursor3, sizeof(empty_anicursor3), FALSE, 0x00030000);
    ok(h1 != NULL, "Create cursor failed (error = %ld).\n", GetLastError());

    /* Check number of steps in multi-frame animated cursor */
    i=0;
    while (DrawIconEx(hdc, 0, 0, h1, 32, 32, i, NULL, DI_NORMAL))
        i++;
    ok(i == empty_anicursor3.header.header.num_steps,
        "Unexpected number of steps in cursor (%d != %ld)\n",
        i, empty_anicursor3.header.header.num_steps);

    /* Check GetCursorFrameInfo behavior on a multi-frame animated cursor */
    for (i=0; i<empty_anicursor3.header.header.num_frames; i++)
    {
        rate = steps = 0xdead;
        h2 = pGetCursorFrameInfo(h1, 0xdead, i, &rate, &steps);
        ok(h1 != h2 && h2 != 0, "GetCursorFrameInfo() failed for cursor %p: (%p, %p).\n", h1, h1, h2);
        ret = check_cursor_data( hdc, h2, &frame_identifier[i], sizeof(DWORD) );
        ok(ret, "GetCursorFrameInfo() returned wrong cursor data for frame %d.\n", i);
        ok(rate == empty_anicursor3.header.header.display_rate,
            "GetCursorFrameInfo() unexpected param 4 value (0x%lx != 0x%lx).\n",
            rate, empty_anicursor3.header.header.display_rate);
        ok(steps == empty_anicursor3.header.header.num_steps,
            "GetCursorFrameInfo() unexpected param 5 value (%ld != %ld).\n",
            steps, empty_anicursor3.header.header.num_steps);
    }

    /* Check GetCursorFrameInfo behavior on rate 3 of a multi-frame animated cursor */
    rate = steps = 0xdead;
    h2 = pGetCursorFrameInfo(h1, 0xdead, 3, &rate, &steps);
    ok(h2 == 0, "GetCursorFrameInfo() failed for cursor %p: (%p != 0).\n", h1, h2);
    ok(rate == 0xdead, "GetCursorFrameInfo() unexpected param 4 value (0x%lx != 0xdead).\n", rate);
    ok(steps == 0xdead, "GetCursorFrameInfo() unexpected param 5 value (0x%lx != 0xdead).\n", steps);

    /* Clean up multi-frame animated cursor. */
    SetLastError(0xdeadbeef);
    ret = DestroyCursor(h1);
    ok(ret, "DestroyCursor() failed (error = %ld).\n", GetLastError());

    /* Create a multi-frame animated cursor with num_steps == 1 */
    empty_anicursor3.header.header.num_steps = 1;
    SetLastError(0xdeadbeef);
    h1 = CreateIconFromResource((PBYTE) &empty_anicursor3, sizeof(empty_anicursor3), FALSE, 0x00030000);
    ok(h1 != NULL, "Create cursor failed (error = %ld).\n", GetLastError());

    /* Check number of steps in multi-frame animated cursor (mismatch between steps and frames) */
    i=0;
    while (DrawIconEx(hdc, 0, 0, h1, 32, 32, i, NULL, DI_NORMAL))
        i++;
    ok(i == empty_anicursor3.header.header.num_steps,
        "Unexpected number of steps in cursor (%d != %ld)\n",
        i, empty_anicursor3.header.header.num_steps);

    /* Check GetCursorFrameInfo behavior on rate 0 for a multi-frame animated cursor (with num_steps == 1) */
    rate = steps = 0xdead;
    h2 = pGetCursorFrameInfo(h1, 0xdead, 0, &rate, &steps);
    ok(h1 != h2 && h2 != 0, "GetCursorFrameInfo() failed for cursor %p: (%p, %p).\n", h1, h1, h2);
    ret = check_cursor_data( hdc, h2, &frame_identifier[0], sizeof(DWORD) );
    ok(ret, "GetCursorFrameInfo() returned wrong cursor data for frame 0.\n");
    ok(rate == empty_anicursor3.header.header.display_rate,
        "GetCursorFrameInfo() unexpected param 4 value (0x%lx != 0x%lx).\n",
        rate, empty_anicursor3.header.header.display_rate);
    todo_wine
    ok(steps == empty_anicursor3.header.header.num_steps,
        "GetCursorFrameInfo() unexpected param 5 value (%ld != ~0).\n", steps);

    /* Check GetCursorFrameInfo behavior on rate 1 for a multi-frame animated cursor (with num_steps == 1) */
    rate = steps = 0xdead;
    h2 = pGetCursorFrameInfo(h1, 0xdead, 1, &rate, &steps);
    ok(h2 == 0, "GetCursorFrameInfo() failed for cursor %p: (%p != 0).\n", h1, h2);
    ok(rate == 0xdead, "GetCursorFrameInfo() unexpected param 4 value (0x%lx != 0xdead).\n", rate);
    ok(steps == 0xdead, "GetCursorFrameInfo() unexpected param 5 value (%ld != 0xdead).\n", steps);

    /* Clean up multi-frame animated cursor. */
    SetLastError(0xdeadbeef);
    ret = DestroyCursor(h1);
    ok(ret, "DestroyCursor() failed (error = %ld).\n", GetLastError());

    /* Creating a multi-frame animated cursor with rate data. */
    for (i=0; i<empty_anicursor3_seq.header.header.num_frames; i++)
    {
        empty_anicursor3_seq.frames[i].data.icon_info.idType = 2; /* type: cursor */
        empty_anicursor3_seq.frames[i].data.icon_info.idEntries[0].xHotspot = 3;
        empty_anicursor3_seq.frames[i].data.icon_info.idEntries[0].yHotspot = 3;
        memcpy( &empty_anicursor3_seq.frames[i].data.bmi_data.data[0], &frame_identifier[i], sizeof(DWORD) );
    }
    SetLastError(0xdeadbeef);
    h1 = CreateIconFromResource((PBYTE) &empty_anicursor3_seq, sizeof(empty_anicursor3_seq), FALSE, 0x00030000);
    ok(h1 != NULL, "Create cursor failed (error = %lx).\n", GetLastError());

    /* Check number of steps in multi-frame animated cursor with rate data */
    i=0;
    while (DrawIconEx(hdc, 0, 0, h1, 32, 32, i, NULL, DI_NORMAL))
        i++;
    ok(i == empty_anicursor3_seq.header.header.num_steps,
        "Unexpected number of steps in cursor (%d != %ld)\n",
        i, empty_anicursor3_seq.header.header.num_steps);

    /* Check GetCursorFrameInfo behavior on a multi-frame animated cursor with rate data */
    for (i=0; i<empty_anicursor3_seq.header.header.num_frames; i++)
    {
        int frame_id = empty_anicursor3_seq.seq.order[i];

        rate = steps = 0xdead;
        h2 = pGetCursorFrameInfo(h1, 0xdead, i, &rate, &steps);
        ok(h1 != h2 && h2 != 0, "GetCursorFrameInfo() failed for cursor %p: (%p, %p).\n", h1, h1, h2);
        ret = check_cursor_data( hdc, h2, &frame_identifier[frame_id], sizeof(DWORD) );
        ok(ret, "GetCursorFrameInfo() returned wrong cursor data for frame %d.\n", i);
        ok(rate == empty_anicursor3_seq.rates.rate[i],
            "GetCursorFrameInfo() unexpected param 4 value (0x%lx != 0x%lx).\n",
            rate, empty_anicursor3_seq.rates.rate[i]);
        ok(steps == empty_anicursor3_seq.header.header.num_steps,
            "GetCursorFrameInfo() unexpected param 5 value (%ld != %ld).\n",
            steps, empty_anicursor3_seq.header.header.num_steps);
    }

    /* Clean up multi-frame animated cursor with rate data. */
    SetLastError(0xdeadbeef);
    ret = DestroyCursor(h1);
    ok(ret, "DestroyCursor() failed (error = %ld).\n", GetLastError());

    free(hotspot);
cleanup:
    if(bmpOld) SelectObject(hdc, bmpOld);
    if(bmp) DeleteObject(bmp);
    if(hdc) DeleteDC(hdc);
}

static HICON create_test_icon(HDC hdc, int width, int height, int bpp,
                              BOOL maskvalue, UINT32 *color, int colorSize)
{
    ICONINFO iconInfo;
    BITMAPINFO bitmapInfo;
    void *buffer = NULL;
    UINT32 mask = maskvalue ? 0xFFFFFFFF : 0x00000000;

    memset(&bitmapInfo, 0, sizeof(bitmapInfo));
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = width;
    bitmapInfo.bmiHeader.biHeight = height;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = bpp;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;
    bitmapInfo.bmiHeader.biSizeImage = colorSize;

    iconInfo.fIcon = TRUE;
    iconInfo.xHotspot = 0;
    iconInfo.yHotspot = 0;

    iconInfo.hbmMask = CreateBitmap( width, height, 1, 1, &mask );
    if(!iconInfo.hbmMask) return NULL;

    iconInfo.hbmColor = CreateDIBSection(hdc, &bitmapInfo, DIB_RGB_COLORS, &buffer, NULL, 0);
    if(!iconInfo.hbmColor || !buffer)
    {
        DeleteObject(iconInfo.hbmMask);
        return NULL;
    }

    memcpy(buffer, color, colorSize);

    return CreateIconIndirect(&iconInfo);
}

static void check_alpha_draw(HDC hdc, BOOL drawiconex, BOOL alpha, int bpp, int line)
{
    HICON hicon;
    UINT32 color[2];
    COLORREF modern_expected, legacy_expected, result;

    color[0] = 0x00A0B0C0;
    color[1] = alpha ? 0xFF000000 : 0x00000000;
    modern_expected = alpha ? 0x00FFFFFF : 0x00C0B0A0;
    legacy_expected = 0x00C0B0A0;

    hicon = create_test_icon(hdc, 2, 1, bpp, 0, color, sizeof(color));
    if (!hicon) return;

    SetPixelV(hdc, 0, 0, 0x00FFFFFF);

    if(drawiconex)
        DrawIconEx(hdc, 0, 0, hicon, 2, 1, 0, NULL, DI_NORMAL);
    else
        DrawIcon(hdc, 0, 0, hicon);

    result = GetPixel(hdc, 0, 0);
    ok (color_match(result, modern_expected),
        "%s. Expected a close match to %06lX (modern) or %06lX (legacy) with %s. "
        "Got %06lX from line %d\n",
        alpha ? "Alpha blending" : "Not alpha blending", modern_expected, legacy_expected,
        drawiconex ? "DrawIconEx" : "DrawIcon", result, line);
}

static void check_DrawIcon(HDC hdc, BOOL maskvalue, UINT32 color, int bpp, COLORREF background,
                           COLORREF modern_expected, COLORREF legacy_expected, int line)
{
    COLORREF result;
    HICON hicon = create_test_icon(hdc, 1, 1, bpp, maskvalue, &color, sizeof(color));
    if (!hicon) return;
    SetPixelV(hdc, 0, 0, background);
    SetPixelV(hdc, GetSystemMetrics(SM_CXICON)-1, GetSystemMetrics(SM_CYICON)-1, background);
    SetPixelV(hdc, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), background);
    DrawIcon(hdc, 0, 0, hicon);
    result = GetPixel(hdc, 0, 0);

    ok (color_match(result, modern_expected),
        "Overlaying Mask %d on Color %06X with DrawIcon. "
        "Expected a close match to %06lX (modern), or %06lX (legacy). Got %06lX from line %d\n",
        maskvalue, color, modern_expected, legacy_expected, result, line);

    result = GetPixel(hdc, GetSystemMetrics(SM_CXICON)-1, GetSystemMetrics(SM_CYICON)-1);

    ok (color_match(result, modern_expected),
        "Overlaying Mask %d on Color %06X with DrawIcon. "
        "Expected a close match to %06lX (modern), or %06lX (legacy). Got %06lX from line %d\n",
        maskvalue, color, modern_expected, legacy_expected, result, line);

    result = GetPixel(hdc, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON));

    ok (color_match(result, background),
        "Overlaying Mask %d on Color %06X with DrawIcon. "
        "Expected unchanged background color %06lX. Got %06lX from line %d\n",
        maskvalue, color, background, result, line);
}

static void test_DrawIcon(void)
{
    BITMAPINFO bitmapInfo;
    HDC hdcDst = NULL;
    HBITMAP bmpDst = NULL;
    HBITMAP bmpOld = NULL;
    void *bits = 0;

    hdcDst = CreateCompatibleDC(0);
    ok(hdcDst != 0, "CreateCompatibleDC(0) failed to return a valid DC\n");
    if (!hdcDst)
        return;

    if(GetDeviceCaps(hdcDst, BITSPIXEL) <= 8)
    {
        skip("Windows will distort DrawIcon colors at 8-bpp and less due to palettizing.\n");
        goto cleanup;
    }

    memset(&bitmapInfo, 0, sizeof(bitmapInfo));
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = GetSystemMetrics(SM_CXICON)+1;
    bitmapInfo.bmiHeader.biHeight = GetSystemMetrics(SM_CYICON)+1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;
    bitmapInfo.bmiHeader.biSizeImage = sizeof(UINT32);

    bmpDst = CreateDIBSection(hdcDst, &bitmapInfo, DIB_RGB_COLORS, &bits, NULL, 0);
    ok (bmpDst && bits, "CreateDIBSection failed to return a valid bitmap and buffer\n");
    if (!bmpDst || !bits)
        goto cleanup;
    bmpOld = SelectObject(hdcDst, bmpDst);

    /* Mask is only heeded if alpha channel is always zero */
    check_DrawIcon(hdcDst, FALSE, 0x00A0B0C0, 32, 0x00FFFFFF, 0x00C0B0A0, 0x00C0B0A0, __LINE__);
    check_DrawIcon(hdcDst, TRUE, 0x00A0B0C0, 32, 0x00FFFFFF, 0x003F4F5F, 0x003F4F5F, __LINE__);

    /* Test alpha blending */
    /* Windows 2000 and up will alpha blend, earlier Windows versions will not */
    check_DrawIcon(hdcDst, FALSE, 0xFFA0B0C0, 32, 0x00FFFFFF, 0x00C0B0A0, 0x00C0B0A0, __LINE__);
    check_DrawIcon(hdcDst, TRUE, 0xFFA0B0C0, 32, 0x00FFFFFF, 0x00C0B0A0, 0x003F4F5F, __LINE__);

    check_DrawIcon(hdcDst, FALSE, 0x80A0B0C0, 32, 0x00000000, 0x00605850, 0x00C0B0A0, __LINE__);
    check_DrawIcon(hdcDst, TRUE, 0x80A0B0C0, 32, 0x00000000, 0x00605850, 0x00C0B0A0, __LINE__);
    check_DrawIcon(hdcDst, FALSE, 0x80A0B0C0, 32, 0x00FFFFFF, 0x00DFD7CF, 0x00C0B0A0, __LINE__);
    check_DrawIcon(hdcDst, TRUE, 0x80A0B0C0, 32, 0x00FFFFFF, 0x00DFD7CF, 0x003F4F5F, __LINE__);

    check_DrawIcon(hdcDst, FALSE, 0x01FFFFFF, 32, 0x00000000, 0x00010101, 0x00FFFFFF, __LINE__);
    check_DrawIcon(hdcDst, TRUE, 0x01FFFFFF, 32, 0x00000000, 0x00010101, 0x00FFFFFF, __LINE__);

    /* Test detecting of alpha channel */
    /* If a single pixel's alpha channel is non-zero, the icon
       will be alpha blended, otherwise it will be draw with
       and + xor blts. */
    check_alpha_draw(hdcDst, FALSE, FALSE, 32, __LINE__);
    check_alpha_draw(hdcDst, FALSE, TRUE, 32, __LINE__);

cleanup:
    if(bmpOld)
        SelectObject(hdcDst, bmpOld);
    if(bmpDst)
        DeleteObject(bmpDst);
    if(hdcDst)
        DeleteDC(hdcDst);
}

static void check_DrawIconEx(HDC hdc, BOOL maskvalue, UINT32 color, int bpp, UINT flags, COLORREF background,
                             COLORREF modern_expected, COLORREF legacy_expected, int line)
{
    COLORREF result;
    HICON hicon = create_test_icon(hdc, 1, 1, bpp, maskvalue, &color, sizeof(color));
    if (!hicon) return;
    SetPixelV(hdc, 0, 0, background);
    DrawIconEx(hdc, 0, 0, hicon, 1, 1, 0, NULL, flags);
    result = GetPixel(hdc, 0, 0);

    ok (color_match(result, modern_expected),
        "Overlaying Mask %d on Color %06X with DrawIconEx flags %08X. "
        "Expected a close match to %06lX (modern) or %06lX (legacy). Got %06lX from line %d\n",
        maskvalue, color, flags, modern_expected, legacy_expected, result, line);
}

static void test_DrawIconEx(void)
{
    BITMAPINFO bitmapInfo;
    HDC hdcDst = NULL;
    HBITMAP bmpDst = NULL;
    HBITMAP bmpOld = NULL;
    void *bits = 0;

    hdcDst = CreateCompatibleDC(0);
    ok(hdcDst != 0, "CreateCompatibleDC(0) failed to return a valid DC\n");
    if (!hdcDst)
        return;

    if(GetDeviceCaps(hdcDst, BITSPIXEL) <= 8)
    {
        skip("Windows will distort DrawIconEx colors at 8-bpp and less due to palettizing.\n");
        goto cleanup;
    }

    memset(&bitmapInfo, 0, sizeof(bitmapInfo));
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = 1;
    bitmapInfo.bmiHeader.biHeight = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;
    bitmapInfo.bmiHeader.biSizeImage = sizeof(UINT32);
    bmpDst = CreateDIBSection(hdcDst, &bitmapInfo, DIB_RGB_COLORS, &bits, NULL, 0);
    ok (bmpDst && bits, "CreateDIBSection failed to return a valid bitmap and buffer\n");
    if (!bmpDst || !bits)
        goto cleanup;
    bmpOld = SelectObject(hdcDst, bmpDst);

    /* Test null, image only, and mask only drawing */
    check_DrawIconEx(hdcDst, FALSE, 0x00A0B0C0, 32, 0, 0x00102030, 0x00102030, 0x00102030, __LINE__);
    check_DrawIconEx(hdcDst, TRUE, 0x00A0B0C0, 32, 0, 0x00102030, 0x00102030, 0x00102030, __LINE__);

    check_DrawIconEx(hdcDst, FALSE, 0x80A0B0C0, 32, DI_MASK, 0x00123456, 0x00000000, 0x00000000, __LINE__);
    check_DrawIconEx(hdcDst, TRUE, 0x80A0B0C0, 32, DI_MASK, 0x00123456, 0x00FFFFFF, 0x00FFFFFF, __LINE__);

    check_DrawIconEx(hdcDst, FALSE, 0x00A0B0C0, 32, DI_IMAGE, 0x00FFFFFF, 0x00C0B0A0, 0x00C0B0A0, __LINE__);
    check_DrawIconEx(hdcDst, TRUE, 0x00A0B0C0, 32, DI_IMAGE, 0x00FFFFFF, 0x00C0B0A0, 0x00C0B0A0, __LINE__);

    /* Test normal drawing */
    check_DrawIconEx(hdcDst, FALSE, 0x00A0B0C0, 32, DI_NORMAL, 0x00FFFFFF, 0x00C0B0A0, 0x00C0B0A0, __LINE__);
    check_DrawIconEx(hdcDst, TRUE, 0x00A0B0C0, 32, DI_NORMAL, 0x00FFFFFF, 0x003F4F5F, 0x003F4F5F, __LINE__);
    check_DrawIconEx(hdcDst, FALSE, 0xFFA0B0C0, 32, DI_NORMAL, 0x00FFFFFF, 0x00C0B0A0, 0x00C0B0A0, __LINE__);

    /* Test alpha blending */
    /* Windows 2000 and up will alpha blend, earlier Windows versions will not */
    check_DrawIconEx(hdcDst, TRUE, 0xFFA0B0C0, 32, DI_NORMAL, 0x00FFFFFF, 0x00C0B0A0, 0x003F4F5F, __LINE__);

    check_DrawIconEx(hdcDst, FALSE, 0x80A0B0C0, 32, DI_NORMAL, 0x00000000, 0x00605850, 0x00C0B0A0, __LINE__);
    check_DrawIconEx(hdcDst, TRUE, 0x80A0B0C0, 32, DI_NORMAL, 0x00000000, 0x00605850, 0x00C0B0A0, __LINE__);
    check_DrawIconEx(hdcDst, FALSE, 0x80A0B0C0, 32, DI_NORMAL, 0x00FFFFFF, 0x00DFD7CF, 0x00C0B0A0, __LINE__);
    check_DrawIconEx(hdcDst, TRUE, 0x80A0B0C0, 32, DI_NORMAL, 0x00FFFFFF, 0x00DFD7CF, 0x003F4F5F, __LINE__);

    check_DrawIconEx(hdcDst, FALSE, 0x01FFFFFF, 32, DI_NORMAL, 0x00000000, 0x00010101, 0x00FFFFFF, __LINE__);
    check_DrawIconEx(hdcDst, TRUE, 0x01FFFFFF, 32, DI_NORMAL, 0x00000000, 0x00010101, 0x00FFFFFF, __LINE__);

    /* Test detecting of alpha channel */
    /* If a single pixel's alpha channel is non-zero, the icon
       will be alpha blended, otherwise it will be draw with
       and + xor blts. */
    check_alpha_draw(hdcDst, TRUE, FALSE, 32, __LINE__);
    check_alpha_draw(hdcDst, TRUE, TRUE, 32, __LINE__);

cleanup:
    if(bmpOld)
        SelectObject(hdcDst, bmpOld);
    if(bmpDst)
        DeleteObject(bmpDst);
    if(hdcDst)
        DeleteDC(hdcDst);
}

static void check_DrawState_Size(HDC hdc, BOOL maskvalue, UINT32 color, int bpp, HBRUSH hbr, UINT flags, int line)
{
    COLORREF result, background;
    BOOL passed[2];
    HICON hicon = create_test_icon(hdc, 1, 1, bpp, maskvalue, &color, sizeof(color));
    background = 0x00FFFFFF;
    /* Set color of the 2 pixels that will be checked afterwards */
    SetPixelV(hdc, 0, 0, background);
    SetPixelV(hdc, 2, 2, background);

    /* Let DrawState calculate the size of the icon (it's 1x1) */
    DrawStateA(hdc, hbr, NULL, (LPARAM) hicon, 0, 1, 1, 0, 0, (DST_ICON | flags ));

    result = GetPixel(hdc, 0, 0);
    passed[0] = color_match(result, background);
    result = GetPixel(hdc, 2, 2);
    passed[0] = passed[0] & color_match(result, background);

    /* Check if manually specifying the icon size DOESN'T work */

    /* IMPORTANT: For Icons, DrawState wants the size of the source image, not the
     *            size in which it should be ultimately drawn. Therefore giving
     *            width/height 2x2 if the icon is only 1x1 pixels in size should
     *            result in drawing it with size 1x1. The size parameters must be
     *            ignored if a Icon has to be drawn! */
    DrawStateA(hdc, hbr, NULL, (LPARAM) hicon, 0, 1, 1, 2, 2, (DST_ICON | flags ));

    result = GetPixel(hdc, 0, 0);
    passed[1] = color_match(result, background);
    result = GetPixel(hdc, 2, 2);
    passed[1] = passed[0] & color_match(result, background);

    if(!passed[0]&&!passed[1])
        ok (passed[1],
        "DrawState failed to draw a 1x1 Icon in the correct size, independent of the "
        "width and height settings passed to it, for Icon with: Overlaying Mask %d on "
        "Color %06X with flags %08X. Line %d\n",
        maskvalue, color, (DST_ICON | flags), line);
    else if(!passed[1])
        ok (passed[1],
        "DrawState failed to draw a 1x1 Icon in the correct size, if the width and height "
        "parameters passed to it are bigger than the real Icon size, for Icon with: Overlaying "
        "Mask %d on Color %06X with flags %08X. Line %d\n",
        maskvalue, color, (DST_ICON | flags), line);
    else
        ok (passed[0],
        "DrawState failed to draw a 1x1 Icon in the correct size, if the width and height "
        "parameters passed to it are 0, for Icon with: Overlaying Mask %d on "
        "Color %06X with flags %08X. Line %d\n",
        maskvalue, color, (DST_ICON | flags), line);
}

static void check_DrawState_Color(HDC hdc, BOOL maskvalue, UINT32 color, int bpp, HBRUSH hbr, UINT flags,
                             COLORREF background, COLORREF modern_expected, COLORREF legacy_expected, int line)
{
    COLORREF result;
    HICON hicon = create_test_icon(hdc, 1, 1, bpp, maskvalue, &color, sizeof(color));
    if (!hicon) return;
    /* Set color of the pixel that will be checked afterwards */
    SetPixelV(hdc, 1, 1, background);

    DrawStateA(hdc, hbr, NULL, (LPARAM) hicon, 0, 1, 1, 0, 0, ( DST_ICON | flags ));

    /* Check the color of the pixel is correct */
    result = GetPixel(hdc, 1, 1);

    ok (color_match(result, modern_expected),
        "DrawState drawing Icon with Overlaying Mask %d on Color %06X with flags %08X. "
        "Expected a close match to %06lX (modern) or %06lX (legacy). Got %06lX from line %d\n",
        maskvalue, color, (DST_ICON | flags), modern_expected, legacy_expected, result, line);
}

static void test_DrawState(void)
{
    BITMAPINFO bitmapInfo;
    HDC hdcDst = NULL;
    HBITMAP bmpDst = NULL;
    HBITMAP bmpOld = NULL;
    void *bits = 0;

    hdcDst = CreateCompatibleDC(0);
    ok(hdcDst != 0, "CreateCompatibleDC(0) failed to return a valid DC\n");
    if (!hdcDst)
        return;

    if(GetDeviceCaps(hdcDst, BITSPIXEL) <= 8)
    {
        skip("Windows will distort DrawIconEx colors at 8-bpp and less due to palettizing.\n");
        goto cleanup;
    }

    memset(&bitmapInfo, 0, sizeof(bitmapInfo));
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = 3;
    bitmapInfo.bmiHeader.biHeight = 3;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;
    bitmapInfo.bmiHeader.biSizeImage = sizeof(UINT32);
    bmpDst = CreateDIBSection(hdcDst, &bitmapInfo, DIB_RGB_COLORS, &bits, NULL, 0);
    ok (bmpDst && bits, "CreateDIBSection failed to return a valid bitmap and buffer\n");
    if (!bmpDst || !bits)
        goto cleanup;
    bmpOld = SelectObject(hdcDst, bmpDst);

    /* potential flags to test with DrawState are: */
    /* DSS_DISABLED embosses the icon */
    /* DSS_MONO draw Icon using a brush as parameter 5 */
    /* DSS_NORMAL draw Icon without any modifications */
    /* DSS_UNION draw the Icon dithered */

    check_DrawState_Size(hdcDst, FALSE, 0x00A0B0C0, 32, 0, DSS_NORMAL, __LINE__);
    check_DrawState_Color(hdcDst, FALSE, 0x00A0B0C0, 32, 0, DSS_NORMAL, 0x00FFFFFF, 0x00C0B0A0, 0x00C0B0A0, __LINE__);

cleanup:
    if(bmpOld)
        SelectObject(hdcDst, bmpOld);
    if(bmpDst)
        DeleteObject(bmpDst);
    if(hdcDst)
        DeleteDC(hdcDst);
}

static DWORD parent_id;

static DWORD CALLBACK set_cursor_thread( void *arg )
{
    HCURSOR ret;

    PeekMessageA( 0, 0, 0, 0, PM_NOREMOVE );  /* create a msg queue */
    if (parent_id)
    {
        BOOL ret = AttachThreadInput( GetCurrentThreadId(), parent_id, TRUE );
        ok( ret, "AttachThreadInput failed\n" );
    }
    if (arg) ret = SetCursor( (HCURSOR)arg );
    else ret = GetCursor();
    return (DWORD_PTR)ret;
}

static void test_SetCursor(void)
{
    static const BYTE bmp_bits[4096];
    ICONINFO cursorInfo;
    HCURSOR cursor, old_cursor, global_cursor = 0;
    DWORD error, id, result;
    UINT display_bpp;
    HDC hdc;
    HANDLE thread;
    CURSORINFO info;

    if (pGetCursorInfo)
    {
        memset( &info, 0, sizeof(info) );
        info.cbSize = sizeof(info);
        if (!pGetCursorInfo( &info ))
        {
            win_skip( "GetCursorInfo not working\n" );
            pGetCursorInfo = NULL;
        }
        else global_cursor = info.hCursor;
    }
    cursor = GetCursor();
    thread = CreateThread( NULL, 0, set_cursor_thread, 0, 0, &id );
    WaitForSingleObject( thread, 1000 );
    GetExitCodeThread( thread, &result );
    ok( result == (DWORD_PTR)cursor, "wrong thread cursor %lx/%p\n", result, cursor );

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
    old_cursor = SetCursor( cursor );

    if (pGetCursorInfo)
    {
        info.cbSize = sizeof(info);
        ok( pGetCursorInfo( &info ), "GetCursorInfo failed\n" );
        /* global cursor doesn't change since we don't have a window */
        ok( info.hCursor == global_cursor, "expected global cursor %p\n", global_cursor );
    }
    thread = CreateThread( NULL, 0, set_cursor_thread, 0, 0, &id );
    WaitForSingleObject( thread, 1000 );
    GetExitCodeThread( thread, &result );
    ok( result == (DWORD_PTR)old_cursor, "wrong thread cursor %lx/%p\n", result, old_cursor );

    SetCursor( 0 );
    ok( GetCursor() == 0, "wrong cursor %p\n", GetCursor() );
    thread = CreateThread( NULL, 0, set_cursor_thread, 0, 0, &id );
    WaitForSingleObject( thread, 1000 );
    GetExitCodeThread( thread, &result );
    ok( result == (DWORD_PTR)old_cursor, "wrong thread cursor %lx/%p\n", result, old_cursor );

    thread = CreateThread( NULL, 0, set_cursor_thread, cursor, 0, &id );
    WaitForSingleObject( thread, 1000 );
    GetExitCodeThread( thread, &result );
    ok( result == (DWORD_PTR)old_cursor, "wrong thread cursor %lx/%p\n", result, old_cursor );
    ok( GetCursor() == 0, "wrong cursor %p/0\n", GetCursor() );

    parent_id = GetCurrentThreadId();
    thread = CreateThread( NULL, 0, set_cursor_thread, cursor, 0, &id );
    WaitForSingleObject( thread, 1000 );
    GetExitCodeThread( thread, &result );
    ok( result == (DWORD_PTR)old_cursor, "wrong thread cursor %lx/%p\n", result, old_cursor );
    ok( GetCursor() == cursor, "wrong cursor %p/0\n", cursor );

    if (pGetCursorInfo)
    {
        info.cbSize = sizeof(info);
        ok( pGetCursorInfo( &info ), "GetCursorInfo failed\n" );
        ok( info.hCursor == global_cursor, "expected global cursor %p\n", global_cursor );
    }
    SetCursor( old_cursor );
    DestroyCursor( cursor );

    SetLastError( 0xdeadbeef );
    cursor = SetCursor( (HCURSOR)0xbadbad );
    error = GetLastError();
    ok( cursor == 0, "wrong cursor %p/0\n", cursor );
    ok( error == ERROR_INVALID_CURSOR_HANDLE, "wrong error %lu\n", error );

    if (pGetCursorInfo)
    {
        info.cbSize = sizeof(info);
        ok( pGetCursorInfo( &info ), "GetCursorInfo failed\n" );
        ok( info.hCursor == global_cursor, "expected global cursor %p\n", global_cursor );
    }
}

static HANDLE event_start, event_next;

static DWORD CALLBACK show_cursor_thread( void *arg )
{
    DWORD count = (DWORD_PTR)arg;
    int ret;

    PeekMessageA( 0, 0, 0, 0, PM_NOREMOVE );  /* create a msg queue */
    if (parent_id)
    {
        BOOL ret = AttachThreadInput( GetCurrentThreadId(), parent_id, TRUE );
        ok( ret, "AttachThreadInput failed\n" );
    }
    if (!count) ret = ShowCursor( FALSE );
    else while (count--) ret = ShowCursor( TRUE );
    SetEvent( event_start );
    WaitForSingleObject( event_next, 2000 );
    return ret;
}

static void test_ShowCursor(void)
{
    int count;
    DWORD id, result;
    HANDLE thread;
    CURSORINFO info;

    if (pGetCursorInfo)
    {
        memset( &info, 0, sizeof(info) );
        info.cbSize = sizeof(info);
        ok( pGetCursorInfo( &info ), "GetCursorInfo failed\n" );
        ok( info.flags & (CURSOR_SHOWING | CURSOR_SUPPRESSED), "Got unexpected cursor state\n" );
    }

    event_start = CreateEventW( NULL, FALSE, FALSE, NULL );
    event_next = CreateEventW( NULL, FALSE, FALSE, NULL );

    count = ShowCursor( TRUE );
    ok( count == 1, "wrong count %d\n", count );
    count = ShowCursor( TRUE );
    ok( count == 2, "wrong count %d\n", count );
    count = ShowCursor( FALSE );
    ok( count == 1, "wrong count %d\n", count );
    count = ShowCursor( FALSE );
    ok( count == 0, "wrong count %d\n", count );
    count = ShowCursor( FALSE );
    ok( count == -1, "wrong count %d\n", count );
    count = ShowCursor( FALSE );
    ok( count == -2, "wrong count %d\n", count );

    if (pGetCursorInfo)
    {
        info.cbSize = sizeof(info);
        ok( pGetCursorInfo( &info ), "GetCursorInfo failed\n" );
        /* global show count is not affected since we don't have a window */
        ok( info.flags & (CURSOR_SHOWING | CURSOR_SUPPRESSED), "Got unexpected cursor state\n" );
    }

    parent_id = 0;
    thread = CreateThread( NULL, 0, show_cursor_thread, NULL, 0, &id );
    WaitForSingleObject( event_start, 1000 );
    count = ShowCursor( FALSE );
    ok( count == -3, "wrong count %d\n", count );
    SetEvent( event_next );
    WaitForSingleObject( thread, 1000 );
    GetExitCodeThread( thread, &result );
    ok( result == -1, "wrong thread count %ld\n", result );
    count = ShowCursor( FALSE );
    ok( count == -4, "wrong count %d\n", count );

    thread = CreateThread( NULL, 0, show_cursor_thread, (void *)1, 0, &id );
    WaitForSingleObject( event_start, 1000 );
    count = ShowCursor( TRUE );
    ok( count == -3, "wrong count %d\n", count );
    SetEvent( event_next );
    WaitForSingleObject( thread, 1000 );
    GetExitCodeThread( thread, &result );
    ok( result == 1, "wrong thread count %ld\n", result );
    count = ShowCursor( TRUE );
    ok( count == -2, "wrong count %d\n", count );

    parent_id = GetCurrentThreadId();
    thread = CreateThread( NULL, 0, show_cursor_thread, NULL, 0, &id );
    WaitForSingleObject( event_start, 1000 );
    count = ShowCursor( TRUE );
    ok( count == -2, "wrong count %d\n", count );
    SetEvent( event_next );
    WaitForSingleObject( thread, 1000 );
    GetExitCodeThread( thread, &result );
    ok( result == -3, "wrong thread count %ld\n", result );
    count = ShowCursor( FALSE );
    ok( count == -2, "wrong count %d\n", count );

    thread = CreateThread( NULL, 0, show_cursor_thread, (void *)3, 0, &id );
    WaitForSingleObject( event_start, 1000 );
    count = ShowCursor( TRUE );
    ok( count == 2, "wrong count %d\n", count );
    SetEvent( event_next );
    WaitForSingleObject( thread, 1000 );
    GetExitCodeThread( thread, &result );
    ok( result == 1, "wrong thread count %ld\n", result );
    count = ShowCursor( FALSE );
    ok( count == -2, "wrong count %d\n", count );

    if (pGetCursorInfo)
    {
        info.cbSize = sizeof(info);
        ok( pGetCursorInfo( &info ), "GetCursorInfo failed\n" );
        ok( info.flags & (CURSOR_SHOWING | CURSOR_SUPPRESSED), "Got unexpected cursor state\n" );
    }

    count = ShowCursor( TRUE );
    ok( count == -1, "wrong count %d\n", count );
    count = ShowCursor( TRUE );
    ok( count == 0, "wrong count %d\n", count );

    if (pGetCursorInfo)
    {
        info.cbSize = sizeof(info);
        ok( pGetCursorInfo( &info ), "GetCursorInfo failed\n" );
        ok( info.flags & (CURSOR_SHOWING | CURSOR_SUPPRESSED), "Got unexpected cursor state\n" );
    }
}


static void test_DestroyCursor(void)
{
    static const BYTE bmp_bits[4096];
    ICONINFO cursorInfo, new_info;
    HCURSOR cursor, cursor2, new_cursor;
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
    ok(!ret, "DestroyCursor on the active cursor succeeded\n");
    error = GetLastError();
    ok(error == 0xdeadbeef, "Last error: %lu\n", error);

    new_cursor = GetCursor();
    ok(new_cursor == cursor, "GetCursor returned %p/%p\n", new_cursor, cursor);

    SetLastError(0xdeadbeef);
    ret = GetIconInfo( cursor, &new_info );
    ok( !ret, "GetIconInfo succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_CURSOR_HANDLE, "wrong error %lu\n", GetLastError() );

    if (ret)  /* nt4 delays destruction until cursor changes */
    {
        DeleteObject( new_info.hbmColor );
        DeleteObject( new_info.hbmMask );

        SetLastError(0xdeadbeef);
        ret = DestroyCursor( cursor );
        ok( !ret, "DestroyCursor succeeded\n" );
        ok( GetLastError() == ERROR_INVALID_CURSOR_HANDLE || GetLastError() == 0xdeadbeef,
            "wrong error %lu\n", GetLastError() );

        SetLastError(0xdeadbeef);
        cursor2 = SetCursor( cursor );
        ok( cursor2 == cursor, "SetCursor returned %p/%p\n", cursor2, cursor);
        ok( GetLastError() == ERROR_INVALID_CURSOR_HANDLE || GetLastError() == 0xdeadbeef,
            "wrong error %lu\n", GetLastError() );
    }
    else
    {
        SetLastError(0xdeadbeef);
        cursor2 = CopyCursor( cursor );
        ok(!cursor2, "CopyCursor succeeded\n" );
        ok( GetLastError() == ERROR_INVALID_CURSOR_HANDLE, "wrong error %lu\n", GetLastError() );

        SetLastError(0xdeadbeef);
        ret = DestroyCursor( cursor );
        ok( !ret, "DestroyCursor succeeded\n" );
        ok( GetLastError() == ERROR_INVALID_CURSOR_HANDLE || GetLastError() == 0xdeadbeef,
            "wrong error %lu\n", GetLastError() );

        SetLastError(0xdeadbeef);
        cursor2 = SetCursor( cursor );
        ok(!cursor2, "SetCursor returned %p/%p\n", cursor2, cursor);
        ok( GetLastError() == ERROR_INVALID_CURSOR_HANDLE || GetLastError() == 0xdeadbeef,
            "wrong error %lu\n", GetLastError() );
    }

    cursor2 = GetCursor();
    ok(cursor2 == new_cursor, "GetCursor returned %p/%p\n", cursor2, new_cursor);

    SetLastError(0xdeadbeef);
    cursor2 = SetCursor( 0 );
    ok( cursor2 != new_cursor, "SetCursor returned %p/%p\n", cursor2, cursor );
    ok( GetLastError() == 0xdeadbeef, "wrong error %lu\n", GetLastError() );

    cursor2 = GetCursor();
    ok(!cursor2, "GetCursor returned %p/%p\n", cursor2, cursor);

    SetLastError(0xdeadbeef);
    ret = DestroyCursor(cursor);
    ok( !ret, "DestroyCursor succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_CURSOR_HANDLE || GetLastError() == 0xdeadbeef,
        "wrong error %lu\n", GetLastError() );

    DeleteObject(cursorInfo.hbmMask);
    DeleteObject(cursorInfo.hbmColor);

    /* Try testing DestroyCursor() now using LoadCursor() cursors. */
    cursor = LoadCursorA(NULL, (LPCSTR)IDC_ARROW);

    SetLastError(0xdeadbeef);
    ret = DestroyCursor(cursor);
    ok(ret, "DestroyCursor on the active cursor failed.\n");
    error = GetLastError();
    ok(error == 0xdeadbeef, "Last error: 0x%08lx\n", error);

    /* Try setting the cursor to a destroyed OEM cursor. */
    SetLastError(0xdeadbeef);
    SetCursor(cursor);
    error = GetLastError();
    ok(error == 0xdeadbeef, "Last error: 0x%08lx\n", error);

    /* Check if LoadCursor() returns the same handle with the same icon. */
    cursor2 = LoadCursorA(NULL, (LPCSTR)IDC_ARROW);
    ok(cursor2 == cursor, "cursor == %p, cursor2 == %p\n", cursor, cursor2);

    /* Check if LoadCursor() returns the same handle with a different icon. */
    cursor2 = LoadCursorA(NULL, (LPCSTR)IDC_WAIT);
    ok(cursor2 != cursor, "cursor == %p, cursor2 == %p\n", cursor, cursor2);
}

static void test_PrivateExtractIcons(void)
{
    HICON icon;
    UINT ret;

    static const test_icon_entries_t icon_desc[] = {{0,0,TRUE}, {16,16,TRUE}, {32,32}, {64,64,TRUE}};

    create_ico_file("extract.ico", icon_desc, ARRAY_SIZE(icon_desc));

    ret = PrivateExtractIconsA("extract.ico", 0, 32, 32, &icon, NULL, 1, 0);
    ok(ret == 1, "PrivateExtractIconsA returned %u\n", ret);
    ok(icon != NULL, "icon == NULL\n");

    test_icon_info(icon, 32, 32, 32, 32, NULL, TRUE);
    DestroyIcon(icon);

    DeleteFileA("extract.ico");
}

static void test_monochrome_icon(void)
{
    HANDLE handle;
    BOOL ret;
    DWORD bytes_written;
    CURSORICONFILEDIR *icon_data;
    CURSORICONFILEDIRENTRY *icon_entry;
    BITMAPINFO *bitmap_info;
    BITMAPCOREINFO *core_info;
    ICONINFO icon_info;
    ULONG icon_size;
    BOOL monochrome, use_core_info;

    icon_data = malloc(sizeof(CURSORICONFILEDIR) + sizeof(BITMAPINFOHEADER) +
                       2 * sizeof(RGBQUAD) + sizeof(ULONG));

    for (monochrome = FALSE; monochrome <= TRUE; monochrome++)
    for (use_core_info = FALSE; use_core_info <= TRUE; use_core_info++)
    {
        trace("%s, %s\n",
              monochrome ? "monochrome" : "colored",
              use_core_info ? "core info" : "bitmap info");

        icon_size = sizeof(CURSORICONFILEDIR) +
                    (use_core_info ? sizeof(BITMAPCOREHEADER) : sizeof(BITMAPINFOHEADER)) +
                    /* 2 * sizeof(RGBTRIPLE) + padding comes out the same */
                    2 * sizeof(RGBQUAD) +
                    sizeof(ULONG);
        ZeroMemory(icon_data, icon_size);
        icon_data->idReserved = 0;
        icon_data->idType = 1;
        icon_data->idCount = 1;

        icon_entry = icon_data->idEntries;
        icon_entry->bWidth = 1;
        icon_entry->bHeight = 1;
        icon_entry->bColorCount = 0;
        icon_entry->bReserved = 0;
        icon_entry->xHotspot = 0;
        icon_entry->yHotspot = 0;
        icon_entry->dwDIBSize = icon_size - sizeof(CURSORICONFILEDIR);
        icon_entry->dwDIBOffset = sizeof(CURSORICONFILEDIR);

        if (use_core_info)
        {
            core_info = (BITMAPCOREINFO *) ((BYTE *) icon_data + icon_entry->dwDIBOffset);
            core_info->bmciHeader.bcSize = sizeof(BITMAPCOREHEADER);
            core_info->bmciHeader.bcWidth = 1;
            core_info->bmciHeader.bcHeight = 2;
            core_info->bmciHeader.bcPlanes = 1;
            core_info->bmciHeader.bcBitCount = 1;
            core_info->bmciColors[0].rgbtBlue = monochrome ? 0x00 : 0xff;
            core_info->bmciColors[0].rgbtGreen = 0x00;
            core_info->bmciColors[0].rgbtRed = 0x00;
            core_info->bmciColors[1].rgbtBlue = 0xff;
            core_info->bmciColors[1].rgbtGreen = 0xff;
            core_info->bmciColors[1].rgbtRed = 0xff;
        }
        else
        {
            bitmap_info = (BITMAPINFO *) ((BYTE *) icon_data + icon_entry->dwDIBOffset);
            bitmap_info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bitmap_info->bmiHeader.biWidth = 1;
            bitmap_info->bmiHeader.biHeight = 2;
            bitmap_info->bmiHeader.biPlanes = 1;
            bitmap_info->bmiHeader.biBitCount = 1;
            bitmap_info->bmiHeader.biSizeImage = 0; /* Uncompressed bitmap. */
            bitmap_info->bmiColors[0].rgbBlue = monochrome ? 0x00 : 0xff;
            bitmap_info->bmiColors[0].rgbGreen = 0x00;
            bitmap_info->bmiColors[0].rgbRed = 0x00;
            bitmap_info->bmiColors[1].rgbBlue = 0xff;
            bitmap_info->bmiColors[1].rgbGreen = 0xff;
            bitmap_info->bmiColors[1].rgbRed = 0xff;
        }

        handle = CreateFileA("icon.ico", GENERIC_WRITE, 0, NULL, CREATE_NEW,
            FILE_ATTRIBUTE_NORMAL, NULL);
        ok(handle != INVALID_HANDLE_VALUE, "CreateFileA failed. %lu\n", GetLastError());
        ret = WriteFile(handle, icon_data, icon_size, &bytes_written, NULL);
        ok(ret && bytes_written == icon_size, "icon.ico created improperly.\n");
        CloseHandle(handle);

        handle = LoadImageA(NULL, "icon.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
        if (!monochrome && !use_core_info) ok(handle != NULL, "LoadImage() failed with %lu.\n", GetLastError());
        else todo_wine ok(handle == NULL || broken(!use_core_info) /* Win7 */, "LoadImage() failed with %lu.\n", GetLastError());
        if (handle == NULL)
        {
            skip("Icon failed to load: %s, %s\n",
                 monochrome ? "monochrome" : "colored",
                 use_core_info ? "core info" : "bitmap info");
            DeleteFileA("icon.ico");
            continue;
        }

        ret = GetIconInfo(handle, &icon_info);
        ok(ret, "GetIconInfo() failed with %lu.\n", GetLastError());
        if (ret)
        {
            ok(icon_info.fIcon == TRUE, "fIcon is %u.\n", icon_info.fIcon);
            ok(icon_info.xHotspot == 0, "xHotspot is %lu.\n", icon_info.xHotspot);
            ok(icon_info.yHotspot == 0, "yHotspot is %lu.\n", icon_info.yHotspot);
            if (monochrome)
                ok(icon_info.hbmColor == NULL, "Got hbmColor %p!\n", icon_info.hbmColor);
            else
                ok(icon_info.hbmColor != NULL, "No hbmColor!\n");
            ok(icon_info.hbmMask != NULL, "No hbmMask!\n");
        }

        ret = DestroyIcon(handle);
        ok(ret, "DestroyIcon() failed with %lu.\n", GetLastError());
        DeleteFileA("icon.ico");
    }

    free(icon_data);
}

static COLORREF get_color_from_bits(const unsigned char *bits, const BITMAPINFO *bmi,
        unsigned int row, unsigned int column)
{
    const BITMAPINFOHEADER *h = &bmi->bmiHeader;
    unsigned int stride, shift, mask;
    const unsigned char *data;
    RGBQUAD color;
    WORD color16;

    stride = ((h->biBitCount * h->biWidth + 7) / 8 + 3) & ~3;
    data = bits + row * stride + column * h->biBitCount / 8;
    if (h->biBitCount >= 24)
        return RGB(data[2], data[1], data[0]);

    if (h->biBitCount == 16)
    {
        color16 = ((WORD)data[1] << 8) | data[0];
        return RGB(((color16 >> 10) & 0x1f) << 3, ((color16 >> 5) & 0x1f) << 3,
                (color16 & 0x1f) << 3);
    }
    shift = 8 - h->biBitCount - (column * h->biBitCount) % 8;
    mask = ~(~0u << h->biBitCount);
    color = bmi->bmiColors[(data[0] >> shift) & mask];
    return RGB(color.rgbRed, color.rgbGreen, color.rgbBlue);
}

#define compare_bitmap_bits(a, b, c, d, e, f, g, h) compare_bitmap_bits_(__LINE__, a, b, c, d, e, f, g, h)
static void compare_bitmap_bits_(unsigned int line, HDC hdc, HBITMAP bitmap, BITMAPINFO *bmi,
        size_t result_bits_size, const unsigned char *expected_bits, unsigned int test_index,
        BOOL allow_todo, const unsigned char *expected_broken_bits)
{
    unsigned char *result_bits;
    unsigned int row, column;
    int ret;

    result_bits = malloc(result_bits_size);
    ret = GetDIBits(hdc, bitmap, 0, bmi->bmiHeader.biHeight,
            result_bits, bmi, DIB_RGB_COLORS);
    ok(ret == bmi->bmiHeader.biHeight, "Unexpected GetDIBits result %d, GetLastError() %lu.\n",
            ret, GetLastError());
    for (row = 0; row < bmi->bmiHeader.biHeight; ++row)
        for (column = 0; column < bmi->bmiHeader.biWidth; ++column)
        {
            COLORREF result, expected;

            result = get_color_from_bits(result_bits, bmi, row, column);
            expected = get_color_from_bits(expected_bits, bmi, row, column);

            todo_wine_if(allow_todo && result != expected)
            ok_(__FILE__, line)(result == expected || broken(expected_broken_bits
                    && result == get_color_from_bits(expected_broken_bits, bmi, row, column)),
                    "Colors do not match, got 0x%06lx, expected 0x%06lx, test_index %u, row %u, column %u.\n",
                    result, expected, test_index, row, column);
        }
    free(result_bits);
}

static void test_Image_StretchMode(void)
{
    static const unsigned char test_bits_24[] =
    {
        0x00, 0xff, 0x00,  0x00, 0xff, 0x00,  0x00, 0xff, 0xff,  0x00, 0xff, 0x00,
        0x00, 0xff, 0x00,  0xff, 0xff, 0x00,  0xff, 0xff, 0x00,  0x00, 0xff, 0x00,
        0x00, 0xff, 0xff,  0x00, 0xff, 0x00,  0x00, 0xff, 0xff,  0x00, 0xff, 0x00,
        0xff, 0xff, 0x00,  0x00, 0xff, 0xff,  0x00, 0xff, 0x00,  0x00, 0xff, 0x00,
    };
    static const unsigned char expected_broken_bits_24[] =
    {
        0x3f, 0xff, 0x00,  0x3f, 0xff, 0x3f,  0x00, 0x00,
        0x3f, 0xff, 0x7f,  0x00, 0xff, 0x3f,  0x00, 0x00,
    };
    static const unsigned char expected_bits_24[] =
    {
        0x00, 0xff, 0x00,  0x00, 0xff, 0x00,  0x00, 0x00,
        0x00, 0xff, 0x00,  0x00, 0xff, 0x00,  0x00, 0x00,
    };
#define rgb16(r, g, b) ((WORD)(((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3)))
    static const WORD test_bits_16[] =
    {
        rgb16(0x00, 0x20, 0x00), rgb16(0x00, 0x40, 0x00), rgb16(0x00, 0x40, 0xff), rgb16(0x00, 0x20, 0x00),
        rgb16(0x00, 0x60, 0x00), rgb16(0xff, 0x80, 0x00), rgb16(0xff, 0x60, 0x00), rgb16(0x00, 0x80, 0x00),
        rgb16(0x00, 0x20, 0xff), rgb16(0x00, 0x40, 0x00), rgb16(0x00, 0x40, 0xff), rgb16(0x00, 0x20, 0x00),
        rgb16(0xff, 0x80, 0x00), rgb16(0x00, 0x60, 0xff), rgb16(0x00, 0x80, 0x00), rgb16(0x00, 0x60, 0x00),
    };
    static const WORD expected_bits_16[] =
    {
        rgb16(0x00, 0x40, 0x00), rgb16(0x00, 0x20, 0x00),
        rgb16(0x00, 0x40, 0x00), rgb16(0x00, 0x20, 0x00),
    };
#undef rgb16
    static const unsigned char test_bits_8[] =
    {
        0x00, 0xff, 0x00, 0xff,
        0x00, 0x00, 0x00, 0x00,
        0xff, 0x55, 0x00, 0xff,
        0x00, 0xff, 0xff, 0x00,
    };
    static const unsigned char expected_bits_8[] =
    {
        0xff, 0xff, 0x00, 0x00,
        0x55, 0xff, 0x00, 0x00,
    };
    static const unsigned char test_bits_1[] =
    {
        0x30, 0x0, 0x0, 0x0,
        0x30, 0x0, 0x0, 0x0,
        0x40, 0x0, 0x0, 0x0,
        0xc0, 0x0, 0x0, 0x0,
    };
    static const unsigned char expected_bits_1[] =
    {
        0x40, 0x0, 0x0, 0x0,
        0x0,  0x0, 0x0, 0x0,
    };
    static const RGBQUAD colors_bits_1[] =
    {
        {0, 0, 0},
        {0xff, 0xff, 0xff},
    };
    static RGBQUAD colors_bits_8[256];

    static const struct
    {
        LONG width, height, output_width, output_height;
        WORD bit_count;
        const unsigned char *test_bits, *expected_bits;
        size_t test_bits_size, result_bits_size;
        const RGBQUAD *bmi_colors;
        size_t bmi_colors_size;
        BOOL allow_todo;
        const unsigned char *expected_broken_bits;
    }
    tests[] =
    {
        {4, 4, 2, 2, 24, test_bits_24, expected_bits_24,
                sizeof(test_bits_24), sizeof(expected_bits_24), NULL, 0, TRUE,
                /* Broken on Windows before Win10 1607+ */ expected_broken_bits_24},
        {4, 4, 2, 2, 1, test_bits_1, expected_bits_1,
                sizeof(test_bits_1), sizeof(expected_bits_1), colors_bits_1,
                sizeof(colors_bits_1)},
        {4, 4, 2, 2, 8, test_bits_8, expected_bits_8,
                sizeof(test_bits_8), sizeof(expected_bits_8), colors_bits_8,
                sizeof(colors_bits_8), TRUE},
        {4, 4, 2, 2, 16, (const unsigned char *)test_bits_16, (const unsigned char *)expected_bits_16,
                sizeof(test_bits_16), sizeof(expected_bits_16), NULL, 0, TRUE},
    };
    static const char filename[] = "test.bmp";
    BITMAPINFO *bmi, *bmi_output;
    HBITMAP bitmap, bitmap_copy;
    unsigned int test_index;
    unsigned char *bits;
    size_t bmi_size;
    unsigned int i;
    HDC hdc;

    bmi_size = sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD);
    bmi = calloc(1, bmi_size);
    bmi_output = calloc(1, bmi_size);
    bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi->bmiHeader.biPlanes = 1;
    bmi->bmiHeader.biCompression = BI_RGB;

    for (i = 0; i < 256; ++i)
        colors_bits_8[i].rgbRed = colors_bits_8[i].rgbGreen = colors_bits_8[i].rgbBlue = i;

    hdc = GetDC(NULL);

    for (test_index = 0; test_index < ARRAY_SIZE(tests); ++test_index)
    {
        if (tests[test_index].bmi_colors)
            memcpy(bmi->bmiColors, tests[test_index].bmi_colors, tests[test_index].bmi_colors_size);
        else
            memset(bmi->bmiColors, 0, 256 * sizeof(RGBQUAD));

        bmi->bmiHeader.biWidth = tests[test_index].width;
        bmi->bmiHeader.biHeight = tests[test_index].height;
        bmi->bmiHeader.biBitCount = tests[test_index].bit_count;
        memcpy(bmi_output, bmi, bmi_size);
        bmi_output->bmiHeader.biWidth = tests[test_index].output_width;
        bmi_output->bmiHeader.biHeight = tests[test_index].output_height;

        bitmap = CreateDIBSection(hdc, bmi, DIB_RGB_COLORS, (void **)&bits, NULL, 0);
        ok(bitmap && bits, "CreateDIBSection() failed, result %lu.\n", GetLastError());
        memcpy(bits, tests[test_index].test_bits, tests[test_index].test_bits_size);

        bitmap_copy = CopyImage(bitmap, IMAGE_BITMAP, tests[test_index].output_width,
                tests[test_index].output_height, LR_CREATEDIBSECTION);
        ok(!!bitmap_copy, "CopyImage() failed, result %lu.\n", GetLastError());

        compare_bitmap_bits(hdc, bitmap_copy, bmi_output, tests[test_index].result_bits_size,
                tests[test_index].expected_bits, test_index, tests[test_index].allow_todo,
                tests[test_index].expected_broken_bits);
        DeleteObject(bitmap);
        DeleteObject(bitmap_copy);

        create_bitmap_file(filename, bmi, tests[test_index].test_bits);
        bitmap = LoadImageA(NULL, filename, IMAGE_BITMAP, tests[test_index].output_width,
                tests[test_index].output_height, LR_CREATEDIBSECTION | LR_LOADFROMFILE);
        ok(!!bitmap, "LoadImageA() failed, result %lu.\n", GetLastError());
        DeleteFileA(filename);
        compare_bitmap_bits(hdc, bitmap, bmi_output, tests[test_index].result_bits_size,
                tests[test_index].expected_bits, test_index, tests[test_index].allow_todo,
                tests[test_index].expected_broken_bits);
        DeleteObject(bitmap);
    }
    ReleaseDC(0, hdc);
    free(bmi_output);
    free(bmi);
}

static void test_copy_image(void)
{
    static const UINT types[] = {IMAGE_BITMAP, IMAGE_ICON, IMAGE_CURSOR};
    static const UINT depths[] = {1, 4, 8, 16, 24, 32};
    unsigned int i, j;

    for (i = 0; i < ARRAY_SIZE(types); ++i)
    {
        for (j = 0; j < ARRAY_SIZE(depths); ++j)
            do_test_copy_image(types[i], depths[j]);
    }
}

START_TEST(cursoricon)
{
    pGetCursorInfo = (void *)GetProcAddress( GetModuleHandleA("user32.dll"), "GetCursorInfo" );
    pGetIconInfoExA = (void *)GetProcAddress( GetModuleHandleA("user32.dll"), "GetIconInfoExA" );
    pGetIconInfoExW = (void *)GetProcAddress( GetModuleHandleA("user32.dll"), "GetIconInfoExW" );
    pGetCursorFrameInfo = (void *)GetProcAddress( GetModuleHandleA("user32.dll"), "GetCursorFrameInfo" );
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

    test_copy_image();
    test_Image_StretchMode();
    test_initial_cursor();
    test_CreateIcon();
    test_LoadImage();
    test_CreateIconFromResource();
    test_GetCursorFrameInfo();
    test_DrawIcon();
    test_DrawIconEx();
    test_DrawState();
    test_SetCursor();
    test_ShowCursor();
    test_DestroyCursor();
    test_PrivateExtractIcons();
    test_monochrome_icon();
    do_parent();
    test_child_process();
    finish_child_process();
}
