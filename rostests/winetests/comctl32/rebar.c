/* Unit tests for rebar.
 *
 * Copyright 2007 Mikolaj Zalewski
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
#include <stdarg.h>

#include <windows.h>
#include <commctrl.h>
#include <uxtheme.h>

#include "wine/test.h"

RECT height_change_notify_rect;
static HWND hMainWnd;
static HWND hRebar;


#define check_rect(name, val, exp) ok(val.top == exp.top && val.bottom == exp.bottom && \
    val.left == exp.left && val.right == exp.right, "invalid rect (" name ") (%d,%d) (%d,%d) - expected (%d,%d) (%d,%d)\n", \
    val.left, val.top, val.right, val.bottom, exp.left, exp.top, exp.right, exp.bottom);

#define check_rect_no_top(name, val, exp) { \
        ok((val.bottom - val.top == exp.bottom - exp.top) && \
            val.left == exp.left && val.right == exp.right, "invalid rect (" name ") (%d,%d) (%d,%d) - expected (%d,%d) (%d,%d), ignoring top\n", \
            val.left, val.top, val.right, val.bottom, exp.left, exp.top, exp.right, exp.bottom); \
    }

#define compare(val, exp, format) ok((val) == (exp), #val " value " format " expected " format "\n", (val), (exp));

#define expect_eq(expr, value, type, format) { type ret = expr; ok((value) == ret, #expr " expected " format "  got " format "\n", (value), (ret)); }

static INT CALLBACK is_font_installed_proc(const LOGFONT *elf, const TEXTMETRIC *ntm, DWORD type, LPARAM lParam)
{
    return 0;
}

static BOOL is_font_installed(const char *name)
{
    HDC hdc = GetDC(0);
    BOOL ret = FALSE;

    if(!EnumFontFamiliesA(hdc, name, is_font_installed_proc, 0))
        ret = TRUE;

    ReleaseDC(0, hdc);
    return ret;
}

static void rebuild_rebar(HWND *hRebar)
{
    if (*hRebar)
        DestroyWindow(*hRebar);

    *hRebar = CreateWindow(REBARCLASSNAME, NULL, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
        hMainWnd, (HMENU)17, GetModuleHandle(NULL), NULL);
    SendMessageA(*hRebar, WM_SETFONT, (WPARAM)GetStockObject(SYSTEM_FONT), 0);
}

static HWND build_toolbar(int nr, HWND hParent)
{
    TBBUTTON btns[8];
    HWND hToolbar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL, WS_CHILD | WS_VISIBLE | CCS_NORESIZE, 0, 0, 0, 0,
        hParent, (HMENU)5, GetModuleHandle(NULL), NULL);
    int iBitmapId = 0;
    int i;

    ok(hToolbar != NULL, "Toolbar creation problem\n");
    ok(SendMessage(hToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0) == 0, "TB_BUTTONSTRUCTSIZE failed\n");
    ok(SendMessage(hToolbar, TB_AUTOSIZE, 0, 0) == 0, "TB_AUTOSIZE failed\n");
    ok(SendMessage(hToolbar, WM_SETFONT, (WPARAM)GetStockObject(SYSTEM_FONT), 0)==1, "WM_SETFONT\n");

    for (i=0; i<5+nr; i++)
    {
        btns[i].iBitmap = i;
        btns[i].idCommand = i;
        btns[i].fsStyle = BTNS_BUTTON;
        btns[i].fsState = TBSTATE_ENABLED;
        btns[i].iString = 0;
    }

    switch (nr)
    {
        case 0: iBitmapId = IDB_HIST_SMALL_COLOR; break;
        case 1: iBitmapId = IDB_VIEW_SMALL_COLOR; break;
        case 2: iBitmapId = IDB_STD_SMALL_COLOR; break;
    }
    ok(SendMessage(hToolbar, TB_LOADIMAGES, iBitmapId, (LPARAM)HINST_COMMCTRL) == 0, "TB_LOADIMAGE failed\n");
    ok(SendMessage(hToolbar, TB_ADDBUTTONS, 5+nr, (LPARAM)btns), "TB_ADDBUTTONS failed\n");
    return hToolbar;
}

static LRESULT CALLBACK MyWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_NOTIFY:
            {
                NMHDR *lpnm = (NMHDR *)lParam;
                if (lpnm->code == RBN_HEIGHTCHANGE)
                    GetClientRect(hRebar, &height_change_notify_rect);
            }
            break;
    }
    return DefWindowProcA(hWnd, msg, wParam, lParam);
}

#if 0  /* use this to generate more tests*/

static void dump_sizes(HWND hRebar)
{
    SIZE sz;
    RECT r;
    int count;
    int i, h;

    GetClientRect(hRebar, &r);
    count = SendMessageA(hRebar, RB_GETROWCOUNT, 0, 0);
    printf("  { {%d, %d, %d, %d}, %d, %d, {", r.left, r.top, r.right, r.bottom,
        SendMessageA(hRebar, RB_GETBARHEIGHT, 0, 0), count);
    if (count == 0)
        printf("0, ");
    for (i = 0; i < count; i++)  /* rows */
        printf("%d, ", SendMessageA(hRebar, RB_GETROWHEIGHT, i, 0));
    printf("}, ");

    count = SendMessageA(hRebar, RB_GETBANDCOUNT, 0, 0);
    printf("%d, {", count);
    if (count == 0)
        printf("{{0, 0, 0, 0}, 0, 0},");
    for (i=0; i<count; i++)
    {
        REBARBANDINFO rbi;
        rbi.cbSize = sizeof(REBARBANDINFO);
        rbi.fMask = RBBIM_SIZE | RBBIM_CHILDSIZE | RBBIM_STYLE;
        ok(SendMessageA(hRebar, RB_GETBANDINFOA, i, (LPARAM)&rbi), "RB_GETBANDINFO failed\n");
        ok(SendMessageA(hRebar, RB_GETRECT, i, (LPARAM)&r), "RB_GETRECT failed\n");
        printf("%s{ {%3d, %3d, %3d, %3d}, 0x%02x, %d}, ", (i%2==0 ? "\n    " : ""), r.left, r.top, r.right, r.bottom,
            rbi.fStyle, rbi.cx);
    }
    printf("\n  }, }, \n");
}

#define check_sizes() dump_sizes(hRebar);
#define check_sizes_todo(todomask) dump_sizes(hRebar);

#else

typedef struct {
    RECT rc;
    DWORD fStyle;
    INT cx;
} rbband_result_t;

typedef struct {
    RECT rcClient;
    int cyBarHeight;
    int nRows;
    int cyRowHeights[50];
    int nBands;
    rbband_result_t bands[50];
} rbsize_result_t;

rbsize_result_t rbsize_results[] = {
  { {0, 0, 672, 0}, 0, 0, {0, }, 0, {{{0, 0, 0, 0}, 0, 0},
  }, },
  { {0, 0, 672, 4}, 4, 1, {4, }, 1, {
    { {  0,   0, 672,   4}, 0x00, 200},
  }, },
  { {0, 0, 672, 4}, 4, 1, {4, }, 2, {
    { {  0,   0, 200,   4}, 0x00, 200}, { {200,   0, 672,   4}, 0x04, 200},
  }, },
  { {0, 0, 672, 30}, 30, 1, {30, }, 3, {
    { {  0,   0, 200,  30}, 0x00, 200}, { {200,   0, 400,  30}, 0x04, 200},
    { {400,   0, 672,  30}, 0x00, 200},
  }, },
  { {0, 0, 672, 34}, 34, 1, {34, }, 4, {
    { {  0,   0, 200,  34}, 0x00, 200}, { {200,   0, 400,  34}, 0x04, 200},
    { {400,   0, 604,  34}, 0x00, 200}, { {604,   0, 672,  34}, 0x04, 68},
  }, },
  { {0, 0, 672, 34}, 34, 1, {34, }, 4, {
    { {  0,   0, 200,  34}, 0x00, 200}, { {200,   0, 400,  34}, 0x04, 200},
    { {400,   0, 604,  34}, 0x00, 200}, { {604,   0, 672,  34}, 0x04, 68},
  }, },
  { {0, 0, 672, 34}, 34, 1, {34, }, 4, {
    { {  0,   0, 200,  34}, 0x00, 200}, { {202,   0, 402,  34}, 0x04, 200},
    { {404,   0, 604,  34}, 0x00, 200}, { {606,   0, 672,  34}, 0x04, 66},
  }, },
  { {0, 0, 672, 70}, 70, 2, {34, 34, }, 5, {
    { {  0,   0, 142,  34}, 0x00, 200}, { {144,   0, 557,  34}, 0x00, 200},
    { {559,   0, 672,  34}, 0x04, 200}, { {  0,  36, 200,  70}, 0x00, 200},
    { {202,  36, 672,  70}, 0x04, 66},
  }, },
  { {0, 0, 672, 34}, 34, 1, {34, }, 5, {
    { {  0,   0, 167,  34}, 0x00, 200}, { {169,   0, 582,  34}, 0x00, 200},
    { {559,   0, 759,  34}, 0x08, 200}, { {584,   0, 627,  34}, 0x00, 200},
    { {629,   0, 672,  34}, 0x04, 66},
  }, },
  { {0, 0, 672, 34}, 34, 1, {34, }, 4, {
    { {  0,   0, 167,  34}, 0x00, 200}, { {169,   0, 582,  34}, 0x00, 200},
    { {584,   0, 627,  34}, 0x00, 200}, { {629,   0, 672,  34}, 0x04, 66},
  }, },
  { {0, 0, 672, 34}, 34, 1, {34, }, 3, {
    { {  0,   0, 413,  34}, 0x00, 200}, { {415,   0, 615,  34}, 0x00, 200},
    { {617,   0, 672,  34}, 0x04, 66},
  }, },
  { {0, 0, 672, 34}, 34, 1, {34, }, 2, {
    { {  0,   0, 604,  34}, 0x00, 200}, { {606,   0, 672,  34}, 0x04, 66},
  }, },
  { {0, 0, 672, 40}, 40, 2, {20, 20, }, 5, {
    { {  0,   0, 114,  20}, 0x00, 40}, { {114,   0, 184,  20}, 0x00, 70},
    { {184,   0, 424,  20}, 0x00, 240}, { {424,   0, 672,  20}, 0x00, 60},
    { {  0,  20, 672,  40}, 0x00, 200},
  }, },
  { {0, 0, 672, 40}, 40, 2, {20, 20, }, 5, {
    { {  0,   0, 114,  20}, 0x00, 40}, { {114,   0, 227,  20}, 0x00, 113},
    { {227,   0, 424,  20}, 0x00, 197}, { {424,   0, 672,  20}, 0x00, 60},
    { {  0,  20, 672,  40}, 0x00, 200},
  }, },
  { {0, 0, 672, 40}, 40, 2, {20, 20, }, 5, {
    { {  0,   0, 114,  20}, 0x00, 40}, { {114,   0, 328,  20}, 0x00, 214},
    { {328,   0, 511,  20}, 0x00, 183}, { {511,   0, 672,  20}, 0x00, 161},
    { {  0,  20, 672,  40}, 0x00, 200},
  }, },
  { {0, 0, 672, 40}, 40, 2, {20, 20, }, 5, {
    { {  0,   0, 114,  20}, 0x00, 40}, { {114,   0, 167,  20}, 0x00, 53},
    { {167,   0, 511,  20}, 0x00, 344}, { {511,   0, 672,  20}, 0x00, 161},
    { {  0,  20, 672,  40}, 0x00, 200},
  }, },
  { {0, 0, 672, 40}, 40, 2, {20, 20, }, 5, {
    { {  0,   0, 114,  20}, 0x00, 40}, { {114,   0, 328,  20}, 0x00, 214},
    { {328,   0, 511,  20}, 0x00, 183}, { {511,   0, 672,  20}, 0x00, 161},
    { {  0,  20, 672,  40}, 0x00, 200},
  }, },
  { {0, 0, 672, 40}, 40, 2, {20, 20, }, 5, {
    { {  0,   0, 114,  20}, 0x00, 40}, { {114,   0, 328,  20}, 0x00, 214},
    { {328,   0, 511,  20}, 0x00, 183}, { {511,   0, 672,  20}, 0x00, 161},
    { {  0,  20, 672,  40}, 0x00, 200},
  }, },
  { {0, 0, 672, 0}, 0, 0, {0, }, 0, {{{0, 0, 0, 0}, 0, 0},
  }, },
  { {0, 0, 672, 65}, 65, 1, {65, }, 3, {
    { {  0,   0,  90,  65}, 0x40, 90}, { { 90,   0, 180,  65}, 0x40, 90},
    { {180,   0, 672,  65}, 0x40, 90},
  }, },
  { {0, 0, 0, 226}, 0, 0, {0, }, 0, {{{0, 0, 0, 0}, 0, 0},
  }, },
  { {0, 0, 65, 226}, 65, 1, {65, }, 1, {
    { {  0,   0, 226,  65}, 0x40, 90},
  }, },
  { {0, 0, 65, 226}, 65, 1, {65, }, 2, {
    { {  0,   0,  90,  65}, 0x40, 90}, { { 90,   0, 226,  65}, 0x40, 90},
  }, },
  { {0, 0, 65, 226}, 65, 1, {65, }, 3, {
    { {  0,   0,  90,  65}, 0x40, 90}, { { 90,   0, 163,  65}, 0x40, 90},
    { {163,   0, 226,  65}, 0x40, 90},
  }, },
};

static int rbsize_numtests = 0;

#define check_sizes_todo(todomask) { \
        RECT rc; \
        REBARBANDINFO rbi; \
        int count, i/*, mask=(todomask)*/; \
        rbsize_result_t *res = &rbsize_results[rbsize_numtests]; \
        assert(rbsize_numtests < sizeof(rbsize_results)/sizeof(rbsize_results[0])); \
        GetClientRect(hRebar, &rc); \
        check_rect("client", rc, res->rcClient); \
        count = SendMessage(hRebar, RB_GETROWCOUNT, 0, 0); \
        compare(count, res->nRows, "%d"); \
        for (i=0; i<min(count, res->nRows); i++) { \
            int height = SendMessageA(hRebar, RB_GETROWHEIGHT, 0, 0);\
            ok(height == res->cyRowHeights[i], "Height mismatch for row %d - %d vs %d\n", i, res->cyRowHeights[i], height); \
        } \
        count = SendMessage(hRebar, RB_GETBANDCOUNT, 0, 0); \
        compare(count, res->nBands, "%d"); \
        for (i=0; i<min(count, res->nBands); i++) { \
            ok(SendMessageA(hRebar, RB_GETRECT, i, (LPARAM)&rc) == 1, "RB_ITEMRECT\n"); \
            if (!(res->bands[i].fStyle & RBBS_HIDDEN)) \
                check_rect("band", rc, res->bands[i].rc); \
            rbi.cbSize = sizeof(REBARBANDINFO); \
            rbi.fMask = RBBIM_STYLE | RBBIM_SIZE; \
            ok(SendMessageA(hRebar, RB_GETBANDINFO,  i, (LPARAM)&rbi) == 1, "RB_GETBANDINFO\n"); \
            compare(rbi.fStyle, res->bands[i].fStyle, "%x"); \
            compare(rbi.cx, res->bands[i].cx, "%d"); \
        } \
        rbsize_numtests++; \
    }

#define check_sizes() check_sizes_todo(0)

#endif

static void add_band_w(HWND hRebar, LPCSTR lpszText, int cxMinChild, int cx, int cxIdeal)
{
    CHAR buffer[MAX_PATH];
    REBARBANDINFO rbi;

    if (lpszText != NULL)
        strcpy(buffer, lpszText);
    rbi.cbSize = sizeof(rbi);
    rbi.fMask = RBBIM_SIZE | RBBIM_CHILDSIZE | RBBIM_CHILD | RBBIM_IDEALSIZE | RBBIM_TEXT;
    rbi.cx = cx;
    rbi.cxMinChild = cxMinChild;
    rbi.cxIdeal = cxIdeal;
    rbi.cyMinChild = 20;
    rbi.hwndChild = build_toolbar(1, hRebar);
    rbi.lpText = (lpszText ? buffer : NULL);
    SendMessage(hRebar, RB_INSERTBAND, -1, (LPARAM)&rbi);
}

static void layout_test(void)
{
    HWND hRebar = NULL;
    REBARBANDINFO rbi;

    rebuild_rebar(&hRebar);
    check_sizes();
    rbi.cbSize = sizeof(rbi);
    rbi.fMask = RBBIM_SIZE | RBBIM_CHILDSIZE | RBBIM_CHILD;
    rbi.cx = 200;
    rbi.cxMinChild = 100;
    rbi.cyMinChild = 30;
    rbi.hwndChild = NULL;
    SendMessageA(hRebar, RB_INSERTBAND, -1, (LPARAM)&rbi);
    check_sizes();

    rbi.fMask |= RBBIM_STYLE;
    rbi.fStyle = RBBS_CHILDEDGE;
    SendMessageA(hRebar, RB_INSERTBAND, -1, (LPARAM)&rbi);
    check_sizes();

    rbi.fStyle = 0;
    rbi.cx = 200;
    rbi.cxMinChild = 30;
    rbi.cyMinChild = 30;
    rbi.hwndChild = build_toolbar(0, hRebar);
    SendMessageA(hRebar, RB_INSERTBAND, -1, (LPARAM)&rbi);
    check_sizes();

    rbi.fStyle = RBBS_CHILDEDGE;
    rbi.cx = 68;
    rbi.hwndChild = build_toolbar(0, hRebar);
    SendMessageA(hRebar, RB_INSERTBAND, -1, (LPARAM)&rbi);
    check_sizes();

    SetWindowLong(hRebar, GWL_STYLE, GetWindowLong(hRebar, GWL_STYLE) | RBS_BANDBORDERS);
    check_sizes();      /* a style change won't start a relayout */
    rbi.fMask = RBBIM_SIZE;
    rbi.cx = 66;
    SendMessageA(hRebar, RB_SETBANDINFO, 3, (LPARAM)&rbi);
    check_sizes();      /* here it will be relayouted */

    /* this will force a new row */
    rbi.fMask = RBBIM_SIZE | RBBIM_CHILDSIZE | RBBIM_CHILD;
    rbi.cx = 200;
    rbi.cxMinChild = 400;
    rbi.cyMinChild = 30;
    rbi.hwndChild = build_toolbar(0, hRebar);
    SendMessageA(hRebar, RB_INSERTBAND, 1, (LPARAM)&rbi);
    check_sizes();

    rbi.fMask = RBBIM_STYLE;
    rbi.fStyle = RBBS_HIDDEN;
    SendMessageA(hRebar, RB_SETBANDINFO, 2, (LPARAM)&rbi);
    check_sizes();

    SendMessageA(hRebar, RB_DELETEBAND, 2, 0);
    check_sizes();
    SendMessageA(hRebar, RB_DELETEBAND, 0, 0);
    check_sizes();
    SendMessageA(hRebar, RB_DELETEBAND, 1, 0);
    check_sizes();

    rebuild_rebar(&hRebar);
    add_band_w(hRebar, "ABC",     70,  40, 100);
    add_band_w(hRebar, NULL,      40,  70, 100);
    add_band_w(hRebar, NULL,     170, 240, 100);
    add_band_w(hRebar, "MMMMMMM", 60,  60, 100);
    add_band_w(hRebar, NULL,     200, 200, 100);
    check_sizes();
    SendMessageA(hRebar, RB_MAXIMIZEBAND, 1, TRUE);
    check_sizes();
    SendMessageA(hRebar, RB_MAXIMIZEBAND, 1, TRUE);
    check_sizes();
    SendMessageA(hRebar, RB_MAXIMIZEBAND, 2, FALSE);
    check_sizes();
    SendMessageA(hRebar, RB_MINIMIZEBAND, 2, 0);
    check_sizes();
    SendMessageA(hRebar, RB_MINIMIZEBAND, 0, 0);
    check_sizes();

    /* VARHEIGHT resizing test on a horizontal rebar */
    rebuild_rebar(&hRebar);
    SetWindowLong(hRebar, GWL_STYLE, GetWindowLong(hRebar, GWL_STYLE) | RBS_AUTOSIZE);
    check_sizes();
    rbi.fMask = RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE | RBBIM_STYLE;
    rbi.fStyle = RBBS_VARIABLEHEIGHT;
    rbi.cxMinChild = 50;
    rbi.cyMinChild = 10;
    rbi.cyIntegral = 11;
    rbi.cyChild = 70;
    rbi.cyMaxChild = 200;
    rbi.cx = 90;
    rbi.hwndChild = build_toolbar(0, hRebar);
    SendMessageA(hRebar, RB_INSERTBAND, -1, (LPARAM)&rbi);

    rbi.cyChild = 50;
    rbi.hwndChild = build_toolbar(0, hRebar);
    SendMessageA(hRebar, RB_INSERTBAND, -1, (LPARAM)&rbi);

    rbi.cyMinChild = 40;
    rbi.cyChild = 50;
    rbi.cyIntegral = 5;
    rbi.hwndChild = build_toolbar(0, hRebar);
    SendMessageA(hRebar, RB_INSERTBAND, -1, (LPARAM)&rbi);
    check_sizes();

    /* VARHEIGHT resizing on a vertical rebar */
    rebuild_rebar(&hRebar);
    SetWindowLong(hRebar, GWL_STYLE, GetWindowLong(hRebar, GWL_STYLE) | CCS_VERT | RBS_AUTOSIZE);
    check_sizes();
    rbi.fMask = RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE | RBBIM_STYLE;
    rbi.fStyle = RBBS_VARIABLEHEIGHT;
    rbi.cxMinChild = 50;
    rbi.cyMinChild = 10;
    rbi.cyIntegral = 11;
    rbi.cyChild = 70;
    rbi.cyMaxChild = 90;
    rbi.cx = 90;
    rbi.hwndChild = build_toolbar(0, hRebar);
    SendMessageA(hRebar, RB_INSERTBAND, -1, (LPARAM)&rbi);
    check_sizes();

    rbi.cyChild = 50;
    rbi.hwndChild = build_toolbar(0, hRebar);
    SendMessageA(hRebar, RB_INSERTBAND, -1, (LPARAM)&rbi);
    check_sizes();

    rbi.cyMinChild = 40;
    rbi.cyChild = 50;
    rbi.cyIntegral = 5;
    rbi.hwndChild = build_toolbar(0, hRebar);
    SendMessageA(hRebar, RB_INSERTBAND, -1, (LPARAM)&rbi);
    check_sizes();

    DestroyWindow(hRebar);
}

#if 0       /* use this to generate more tests */

static void dump_client(HWND hRebar)
{
    RECT r;
    BOOL notify;
    GetWindowRect(hRebar, &r);
    MapWindowPoints(HWND_DESKTOP, hMainWnd, (LPPOINT)&r, 2);
    if (height_change_notify_rect.top != -1)
    {
        RECT rcClient;
        GetClientRect(hRebar, &rcClient);
        assert(EqualRect(&rcClient, &height_change_notify_rect));
        notify = TRUE;
    }
    else
        notify = FALSE;
    printf("    {{%d, %d, %d, %d}, %d, %s},\n", r.left, r.top, r.right, r.bottom, SendMessage(hRebar, RB_GETROWCOUNT, 0, 0),
        notify ? "TRUE" : "FALSE");
    SetRect(&height_change_notify_rect, -1, -1, -1, -1);
}

#define comment(fmt, arg1) printf("/* " fmt " */\n", arg1);
#define check_client() dump_client(hRebar)

#else

typedef struct {
    RECT rc;
    INT iNumRows;
    BOOL heightNotify;
} rbresize_test_result_t;

rbresize_test_result_t resize_results[] = {
/* style 00000001 */
    {{0, 2, 672, 2}, 0, FALSE},
    {{0, 2, 672, 22}, 1, TRUE},
    {{0, 2, 672, 22}, 1, FALSE},
    {{0, 2, 672, 22}, 1, FALSE},
    {{0, 2, 672, 22}, 1, FALSE},
    {{0, 2, 672, 22}, 0, FALSE},
/* style 00000041 */
    {{0, 0, 672, 0}, 0, FALSE},
    {{0, 0, 672, 20}, 1, TRUE},
    {{0, 0, 672, 20}, 1, FALSE},
    {{0, 0, 672, 20}, 1, FALSE},
    {{0, 0, 672, 20}, 1, FALSE},
    {{0, 0, 672, 20}, 0, FALSE},
/* style 00000003 */
    {{0, 226, 672, 226}, 0, FALSE},
    {{0, 206, 672, 226}, 1, TRUE},
    {{0, 206, 672, 226}, 1, FALSE},
    {{0, 206, 672, 226}, 1, FALSE},
    {{0, 206, 672, 226}, 1, FALSE},
    {{0, 206, 672, 226}, 0, FALSE},
/* style 00000043 */
    {{0, 226, 672, 226}, 0, FALSE},
    {{0, 206, 672, 226}, 1, TRUE},
    {{0, 206, 672, 226}, 1, FALSE},
    {{0, 206, 672, 226}, 1, FALSE},
    {{0, 206, 672, 226}, 1, FALSE},
    {{0, 206, 672, 226}, 0, FALSE},
/* style 00000080 */
    {{2, 0, 2, 226}, 0, FALSE},
    {{2, 0, 22, 226}, 1, TRUE},
    {{2, 0, 22, 226}, 1, FALSE},
    {{2, 0, 22, 226}, 1, FALSE},
    {{2, 0, 22, 226}, 1, FALSE},
    {{2, 0, 22, 226}, 0, FALSE},
/* style 00000083 */
    {{672, 0, 672, 226}, 0, FALSE},
    {{652, 0, 672, 226}, 1, TRUE},
    {{652, 0, 672, 226}, 1, FALSE},
    {{652, 0, 672, 226}, 1, FALSE},
    {{652, 0, 672, 226}, 1, FALSE},
    {{652, 0, 672, 226}, 0, FALSE},
/* style 00000008 */
    {{10, 11, 510, 11}, 0, FALSE},
    {{10, 15, 510, 35}, 1, TRUE},
    {{10, 17, 510, 37}, 1, FALSE},
    {{10, 14, 110, 54}, 2, TRUE},
    {{0, 4, 0, 44}, 2, FALSE},
    {{0, 6, 0, 46}, 2, FALSE},
    {{0, 8, 0, 48}, 2, FALSE},
    {{0, 12, 0, 32}, 1, TRUE},
    {{0, 4, 100, 24}, 0, FALSE},
/* style 00000048 */
    {{10, 5, 510, 5}, 0, FALSE},
    {{10, 5, 510, 25}, 1, TRUE},
    {{10, 5, 510, 25}, 1, FALSE},
    {{10, 10, 110, 50}, 2, TRUE},
    {{0, 0, 0, 40}, 2, FALSE},
    {{0, 0, 0, 40}, 2, FALSE},
    {{0, 0, 0, 40}, 2, FALSE},
    {{0, 0, 0, 20}, 1, TRUE},
    {{0, 0, 100, 20}, 0, FALSE},
/* style 00000004 */
    {{10, 5, 510, 20}, 0, FALSE},
    {{10, 5, 510, 20}, 1, TRUE},
    {{10, 10, 110, 110}, 2, TRUE},
    {{0, 0, 0, 0}, 2, FALSE},
    {{0, 0, 0, 0}, 2, FALSE},
    {{0, 0, 0, 0}, 2, FALSE},
    {{0, 0, 0, 0}, 1, TRUE},
    {{0, 0, 100, 100}, 0, FALSE},
/* style 00000002 */
    {{0, 5, 672, 5}, 0, FALSE},
    {{0, 5, 672, 25}, 1, TRUE},
    {{0, 10, 672, 30}, 1, FALSE},
    {{0, 0, 672, 20}, 1, FALSE},
    {{0, 0, 672, 20}, 1, FALSE},
    {{0, 0, 672, 20}, 0, FALSE},
/* style 00000082 */
    {{10, 0, 10, 226}, 0, FALSE},
    {{10, 0, 30, 226}, 1, TRUE},
    {{10, 0, 30, 226}, 1, FALSE},
    {{0, 0, 20, 226}, 1, FALSE},
    {{0, 0, 20, 226}, 1, FALSE},
    {{0, 0, 20, 226}, 0, FALSE},
/* style 00800001 */
    {{-2, 0, 674, 4}, 0, FALSE},
    {{-2, 0, 674, 24}, 1, TRUE},
    {{-2, 0, 674, 24}, 1, FALSE},
    {{-2, 0, 674, 24}, 1, FALSE},
    {{-2, 0, 674, 24}, 1, FALSE},
    {{-2, 0, 674, 24}, 0, FALSE},
/* style 00800048 */
    {{10, 5, 510, 9}, 0, FALSE},
    {{10, 5, 510, 29}, 1, TRUE},
    {{10, 5, 510, 29}, 1, FALSE},
    {{10, 10, 110, 54}, 2, TRUE},
    {{0, 0, 0, 44}, 2, FALSE},
    {{0, 0, 0, 44}, 2, FALSE},
    {{0, 0, 0, 44}, 2, FALSE},
    {{0, 0, 0, 24}, 1, TRUE},
    {{0, 0, 100, 24}, 0, FALSE},
/* style 00800004 */
    {{10, 5, 510, 20}, 0, FALSE},
    {{10, 5, 510, 20}, 1, TRUE},
    {{10, 10, 110, 110}, 2, TRUE},
    {{0, 0, 0, 0}, 2, FALSE},
    {{0, 0, 0, 0}, 2, FALSE},
    {{0, 0, 0, 0}, 2, FALSE},
    {{0, 0, 0, 0}, 1, TRUE},
    {{0, 0, 100, 100}, 0, FALSE},
/* style 00800002 */
    {{-2, 5, 674, 9}, 0, FALSE},
    {{-2, 5, 674, 29}, 1, TRUE},
    {{-2, 10, 674, 34}, 1, FALSE},
    {{-2, 0, 674, 24}, 1, FALSE},
    {{-2, 0, 674, 24}, 1, FALSE},
    {{-2, 0, 674, 24}, 0, FALSE},
};

static int resize_numtests = 0;

#define comment(fmt, arg1)
#define check_client() { \
        RECT r; \
        rbresize_test_result_t *res = &resize_results[resize_numtests++]; \
        assert(resize_numtests <= sizeof(resize_results)/sizeof(resize_results[0])); \
        GetWindowRect(hRebar, &r); \
        MapWindowPoints(HWND_DESKTOP, hMainWnd, (LPPOINT)&r, 2); \
        if ((dwStyles[i] & (CCS_NOPARENTALIGN|CCS_NODIVIDER)) == CCS_NOPARENTALIGN) {\
            check_rect_no_top("client", r, res->rc); /* the top coordinate changes after every layout and is very implementation-dependent */ \
        } else { \
            check_rect("client", r, res->rc); \
        } \
        expect_eq((int)SendMessage(hRebar, RB_GETROWCOUNT, 0, 0), res->iNumRows, int, "%d"); \
        if (res->heightNotify) { \
            RECT rcClient; \
            GetClientRect(hRebar, &rcClient); \
            check_rect("notify", height_change_notify_rect, rcClient); \
        } else ok(height_change_notify_rect.top == -1, "Unexpected RBN_HEIGHTCHANGE received\n"); \
        SetRect(&height_change_notify_rect, -1, -1, -1, -1); \
    }

#endif

static void resize_test(void)
{
    DWORD dwStyles[] = {CCS_TOP, CCS_TOP | CCS_NODIVIDER, CCS_BOTTOM, CCS_BOTTOM | CCS_NODIVIDER, CCS_VERT, CCS_RIGHT,
        CCS_NOPARENTALIGN, CCS_NOPARENTALIGN | CCS_NODIVIDER, CCS_NORESIZE, CCS_NOMOVEY, CCS_NOMOVEY | CCS_VERT,
        CCS_TOP | WS_BORDER, CCS_NOPARENTALIGN | CCS_NODIVIDER | WS_BORDER, CCS_NORESIZE | WS_BORDER,
        CCS_NOMOVEY | WS_BORDER};

    const int styles_count = sizeof(dwStyles) / sizeof(dwStyles[0]);
    int i;

    for (i = 0; i < styles_count; i++)
    {
        comment("style %08x", dwStyles[i]);
        SetRect(&height_change_notify_rect, -1, -1, -1, -1);
        hRebar = CreateWindow(REBARCLASSNAME, "A", dwStyles[i] | WS_CHILD | WS_VISIBLE, 10, 5, 500, 15, hMainWnd, NULL, GetModuleHandle(NULL), 0);
        check_client();
        add_band_w(hRebar, NULL, 70, 100, 0);
        if (dwStyles[i] & CCS_NOPARENTALIGN)  /* the window drifts downward for CCS_NOPARENTALIGN without CCS_NODIVIDER */
            check_client();
        add_band_w(hRebar, NULL, 70, 100, 0);
        check_client();
        MoveWindow(hRebar, 10, 10, 100, 100, TRUE);
        check_client();
        MoveWindow(hRebar, 0, 0, 0, 0, TRUE);
        check_client();
        /* try to fool the rebar by sending invalid width/height - won't work */
        if (dwStyles[i] & (CCS_NORESIZE | CCS_NOPARENTALIGN))
        {
            WINDOWPOS pos;
            pos.hwnd = hRebar;
            pos.hwndInsertAfter = NULL;
            pos.cx = 500;
            pos.cy = 500;
            pos.x = 10;
            pos.y = 10;
            pos.flags = 0;
            SendMessage(hRebar, WM_WINDOWPOSCHANGING, 0, (LPARAM)&pos);
            SendMessage(hRebar, WM_WINDOWPOSCHANGED, 0, (LPARAM)&pos);
            check_client();
            SendMessage(hRebar, WM_SIZE, SIZE_RESTORED, MAKELONG(500, 500));
            check_client();
        }
        SendMessage(hRebar, RB_DELETEBAND, 0, 0);
        check_client();
        SendMessage(hRebar, RB_DELETEBAND, 0, 0);
        MoveWindow(hRebar, 0, 0, 100, 100, TRUE);
        check_client();
        DestroyWindow(hRebar);
    }
}

static void expect_band_content(UINT uBand, INT fStyle, COLORREF clrFore,
    COLORREF clrBack, LPCSTR lpText, int iImage, HWND hwndChild,
    INT cxMinChild, INT cyMinChild, INT cx, HBITMAP hbmBack, INT wID,
    INT cyChild, INT cyMaxChild, INT cyIntegral, INT cxIdeal, LPARAM lParam,
    INT cxHeader)
{
    CHAR buf[MAX_PATH] = "abc";
    REBARBANDINFO rb;

    memset(&rb, 0xdd, sizeof(rb));
    rb.cbSize = sizeof(rb);
    rb.fMask = RBBIM_BACKGROUND | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_COLORS
        | RBBIM_HEADERSIZE | RBBIM_ID | RBBIM_IDEALSIZE | RBBIM_IMAGE | RBBIM_LPARAM
        | RBBIM_SIZE | RBBIM_STYLE | RBBIM_TEXT;
    rb.lpText = buf;
    rb.cch = MAX_PATH;
    ok(SendMessageA(hRebar, RB_GETBANDINFOA, uBand, (LPARAM)&rb), "RB_GETBANDINFO failed\n");
    expect_eq(rb.fStyle, fStyle, int, "%x");
    todo_wine expect_eq(rb.clrFore, clrFore, COLORREF, "%x");
    todo_wine expect_eq(rb.clrBack, clrBack, unsigned, "%x");
    expect_eq(strcmp(rb.lpText, lpText), 0, int, "%d");
    expect_eq(rb.iImage, iImage, int, "%x");
    expect_eq(rb.hwndChild, hwndChild, HWND, "%p");
    expect_eq(rb.cxMinChild, cxMinChild, int, "%d");
    expect_eq(rb.cyMinChild, cyMinChild, int, "%d");
    expect_eq(rb.cx, cx, int, "%d");
    expect_eq(rb.hbmBack, hbmBack, HBITMAP, "%p");
    expect_eq(rb.wID, wID, int, "%d");
    /* the values of cyChild, cyMaxChild and cyIntegral can't be read unless the band is RBBS_VARIABLEHEIGHT */
    expect_eq(rb.cyChild, cyChild, int, "%x");
    expect_eq(rb.cyMaxChild, cyMaxChild, int, "%x");
    expect_eq(rb.cyIntegral, cyIntegral, int, "%x");
    expect_eq(rb.cxIdeal, cxIdeal, int, "%d");
    expect_eq(rb.lParam, lParam, LPARAM, "%ld");
    expect_eq(rb.cxHeader, cxHeader, int, "%d");
}

static void bandinfo_test(void)
{
    REBARBANDINFOA rb;
    CHAR szABC[] = "ABC";
    CHAR szABCD[] = "ABCD";

    rebuild_rebar(&hRebar);
    rb.cbSize = sizeof(REBARBANDINFO);
    rb.fMask = 0;
    ok(SendMessageA(hRebar, RB_INSERTBANDA, 0, (LPARAM)&rb), "RB_INSERTBAND failed\n");
    expect_band_content(0, 0, 0, GetSysColor(COLOR_3DFACE), "", -1, NULL, 0, 0, 0, NULL, 0, 0xdddddddd, 0xdddddddd, 0xdddddddd, 0, 0, 0);

    rb.fMask = RBBIM_CHILDSIZE;
    rb.cxMinChild = 15;
    rb.cyMinChild = 20;
    rb.cyChild = 30;
    rb.cyMaxChild = 20;
    rb.cyIntegral = 10;
    ok(SendMessageA(hRebar, RB_SETBANDINFOA, 0, (LPARAM)&rb), "RB_INSERTBAND failed\n");
    expect_band_content(0, 0, 0, GetSysColor(COLOR_3DFACE), "", -1, NULL, 15, 20, 0, NULL, 0, 0xdddddddd, 0xdddddddd, 0xdddddddd, 0, 0, 0);

    rb.fMask = RBBIM_TEXT;
    rb.lpText = szABC;
    ok(SendMessageA(hRebar, RB_SETBANDINFOA, 0, (LPARAM)&rb), "RB_INSERTBAND failed\n");
    expect_band_content(0, 0, 0, GetSysColor(COLOR_3DFACE), "ABC", -1, NULL, 15, 20, 0, NULL, 0, 0xdddddddd, 0xdddddddd, 0xdddddddd, 0, 0, 35);

    rb.cbSize = sizeof(REBARBANDINFO);
    rb.fMask = 0;
    ok(SendMessageA(hRebar, RB_INSERTBANDA, 1, (LPARAM)&rb), "RB_INSERTBAND failed\n");
    expect_band_content(1, 0, 0, GetSysColor(COLOR_3DFACE), "", -1, NULL, 0, 0, 0, NULL, 0, 0xdddddddd, 0xdddddddd, 0xdddddddd, 0, 0, 9);
    expect_band_content(0, 0, 0, GetSysColor(COLOR_3DFACE), "ABC", -1, NULL, 15, 20, 0, NULL, 0, 0xdddddddd, 0xdddddddd, 0xdddddddd, 0, 0, 40);

    rb.fMask = RBBIM_HEADERSIZE;
    rb.cxHeader = 50;
    ok(SendMessageA(hRebar, RB_SETBANDINFOA, 0, (LPARAM)&rb), "RB_INSERTBAND failed\n");
    expect_band_content(0, 0x40000000, 0, GetSysColor(COLOR_3DFACE), "ABC", -1, NULL, 15, 20, 0, NULL, 0, 0xdddddddd, 0xdddddddd, 0xdddddddd, 0, 0, 50);

    rb.cxHeader = 5;
    ok(SendMessageA(hRebar, RB_SETBANDINFOA, 0, (LPARAM)&rb), "RB_INSERTBAND failed\n");
    expect_band_content(0, 0x40000000, 0, GetSysColor(COLOR_3DFACE), "ABC", -1, NULL, 15, 20, 0, NULL, 0, 0xdddddddd, 0xdddddddd, 0xdddddddd, 0, 0, 5);

    rb.fMask = RBBIM_TEXT;
    rb.lpText = szABCD;
    ok(SendMessageA(hRebar, RB_SETBANDINFOA, 0, (LPARAM)&rb), "RB_INSERTBAND failed\n");
    expect_band_content(0, 0x40000000, 0, GetSysColor(COLOR_3DFACE), "ABCD", -1, NULL, 15, 20, 0, NULL, 0, 0xdddddddd, 0xdddddddd, 0xdddddddd, 0, 0, 5);
    rb.fMask = RBBIM_STYLE | RBBIM_TEXT;
    rb.fStyle = RBBS_VARIABLEHEIGHT;
    rb.lpText = szABC;
    ok(SendMessageA(hRebar, RB_SETBANDINFOA, 0, (LPARAM)&rb), "RB_INSERTBAND failed\n");
    expect_band_content(0, RBBS_VARIABLEHEIGHT, 0, GetSysColor(COLOR_3DFACE), "ABC", -1, NULL, 15, 20, 0, NULL, 0, 20, 0x7fffffff, 0, 0, 0, 40);

    DestroyWindow(hRebar);
}

START_TEST(rebar)
{
    INITCOMMONCONTROLSEX icc;
    WNDCLASSA wc;
    MSG msg;
    RECT rc;

    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_COOL_CLASSES;
    InitCommonControlsEx(&icc);

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.hIcon = NULL;
    wc.hCursor = LoadCursorA(NULL, IDC_IBEAM);
    wc.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "MyTestWnd";
    wc.lpfnWndProc = MyWndProc;
    RegisterClassA(&wc);
    hMainWnd = CreateWindowExA(0, "MyTestWnd", "Blah", WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT, 672+2*GetSystemMetrics(SM_CXSIZEFRAME),
      226+GetSystemMetrics(SM_CYCAPTION)+2*GetSystemMetrics(SM_CYSIZEFRAME),
      NULL, NULL, GetModuleHandleA(NULL), 0);
    GetClientRect(hMainWnd, &rc);
    ShowWindow(hMainWnd, SW_SHOW);

    bandinfo_test();

    if(is_font_installed("System") && is_font_installed("Tahoma"))
    {
        layout_test();
        resize_test();
    } else
        skip("Missing System or Tahoma font\n");

    PostQuitMessage(0);
    while(GetMessageA(&msg,0,0,0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    DestroyWindow(hMainWnd);
}
