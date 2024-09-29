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

/* make sure the structures work with a comctl32 v5.x */
#ifdef __REACTOS__
#undef _WIN32_WINNT
#undef _WIN32_IE
#endif
#define _WIN32_WINNT 0x500
#define _WIN32_IE 0x500

#include <assert.h>
#include <stdarg.h>

#include <windows.h>
#include <commctrl.h>
#include <uxtheme.h>

#include "wine/heap.h"
#include "wine/test.h"

static BOOL (WINAPI *pImageList_Destroy)(HIMAGELIST);
static HIMAGELIST (WINAPI *pImageList_LoadImageA)(HINSTANCE, LPCSTR, int, int, COLORREF, UINT, UINT);

static RECT height_change_notify_rect;
static HWND hMainWnd;
static int system_font_height;


#define check_rect(name, val, exp) ok(EqualRect(&val, &exp), \
    "invalid rect (" name ") %s - expected %s\n", wine_dbgstr_rect(&val), wine_dbgstr_rect(&exp));

#define check_rect_no_top(name, val, exp) { \
        ok((val.bottom - val.top == exp.bottom - exp.top) && \
            val.left == exp.left && val.right == exp.right, \
            "invalid rect (" name ") %s - expected %s, ignoring top\n", \
            wine_dbgstr_rect(&val), wine_dbgstr_rect(&exp)); }

#define compare(val, exp, format) ok((val) == (exp), #val " value " format " expected " format "\n", (val), (exp));

#define expect_eq(line, expr, value, type, format) { type ret = expr;\
        ok((value) == ret, #expr " expected " format "  got " format " from line %d\n", (value), (ret), line); }

static INT CALLBACK is_font_installed_proc(const LOGFONTA *elf, const TEXTMETRICA *ntm, DWORD type, LPARAM lParam)
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

static void init_system_font_height(void) {
    HDC hDC;
    TEXTMETRICA tm;

    hDC = CreateCompatibleDC(NULL);
    GetTextMetricsA(hDC, &tm);
    DeleteDC(NULL);

    system_font_height = tm.tmHeight;
}

static HWND create_rebar_control(void)
{
    HWND hwnd;

    hwnd = CreateWindowA(REBARCLASSNAMEA, NULL, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
        hMainWnd, (HMENU)17, GetModuleHandleA(NULL), NULL);
    ok(hwnd != NULL, "Failed to create Rebar\n");

    SendMessageA(hwnd, WM_SETFONT, (WPARAM)GetStockObject(SYSTEM_FONT), 0);

    return hwnd;
}

static HWND build_toolbar(int nr, HWND hParent)
{
    TBBUTTON btns[8];
    HWND hToolbar = CreateWindowExA(0, TOOLBARCLASSNAMEA, NULL, WS_CHILD | WS_VISIBLE | CCS_NORESIZE, 0, 0, 0, 0,
        hParent, (HMENU)5, GetModuleHandleA(NULL), NULL);
    int iBitmapId = 0;
    int i;

    ok(hToolbar != NULL, "Toolbar creation problem\n");
    ok(SendMessageA(hToolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0) == 0, "TB_BUTTONSTRUCTSIZE failed\n");
    ok(SendMessageA(hToolbar, TB_AUTOSIZE, 0, 0) == 0, "TB_AUTOSIZE failed\n");
    ok(SendMessageA(hToolbar, WM_SETFONT, (WPARAM)GetStockObject(SYSTEM_FONT), 0)==1, "WM_SETFONT\n");

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
    ok(SendMessageA(hToolbar, TB_LOADIMAGES, iBitmapId, (LPARAM)HINST_COMMCTRL) == 0, "TB_LOADIMAGES failed\n");
    ok(SendMessageA(hToolbar, TB_ADDBUTTONSA, 5+nr, (LPARAM)btns), "TB_ADDBUTTONSA failed\n");
    return hToolbar;
}

static int g_parent_measureitem;

static LRESULT CALLBACK parent_wndproc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_NOTIFY:
            {
                NMHDR *lpnm = (NMHDR *)lParam;
                if (lpnm->code == RBN_HEIGHTCHANGE)
                    GetClientRect(lpnm->hwndFrom, &height_change_notify_rect);
            }
            break;
        case WM_MEASUREITEM:
            g_parent_measureitem++;
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
        REBARBANDINFOA rbi;
        rbi.cbSize = REBARBANDINFOA_V6_SIZE;
        rbi.fMask = RBBIM_SIZE | RBBIM_CHILDSIZE | RBBIM_STYLE;
        ok(SendMessageA(hRebar, RB_GETBANDINFOA, i, (LPARAM)&rbi), "RB_GETBANDINFOA failed\n");
        ok(SendMessageA(hRebar, RB_GETRECT, i, (LPARAM)&r), "RB_GETRECT failed\n");
        printf("%s{ {%3d, %3d, %3d, %3d}, 0x%02x, %d}, ", (i%2==0 ? "\n    " : ""), r.left, r.top, r.right, r.bottom,
            rbi.fStyle, rbi.cx);
    }
    printf("\n  }, },\n");
}

#define check_sizes() dump_sizes(hRebar);
#define check_sizes_todo(todomask) dump_sizes(hRebar);

#else

static int string_width(const CHAR *s) {
    SIZE sz;
    HDC hdc;

    hdc = CreateCompatibleDC(NULL);
    GetTextExtentPoint32A(hdc, s, strlen(s), &sz);
    DeleteDC(hdc);

    return sz.cx;
}

typedef struct {
    RECT rc;
    DWORD fStyle;
    UINT cx;
} rbband_result_t;

typedef struct {
    RECT rcClient;
    int cyBarHeight;
    int nRows;
    int *cyRowHeights;
    int nBands;
    rbband_result_t *bands;
} rbsize_result_t;

static rbsize_result_t rbsize_init(int cleft, int ctop, int cright, int cbottom, int cyBarHeight, int nRows, int nBands)
{
    rbsize_result_t ret;

    SetRect(&ret.rcClient, cleft, ctop, cright, cbottom);
    ret.cyBarHeight = cyBarHeight;
    ret.nRows = 0;
    ret.cyRowHeights = heap_alloc_zero(nRows * sizeof(int));
    ret.nBands = 0;
    ret.bands = heap_alloc_zero(nBands * sizeof(*ret.bands));

    return ret;
}

static void rbsize_add_row(rbsize_result_t *rbsr, int rowHeight) {
    rbsr->cyRowHeights[rbsr->nRows] = rowHeight;
    rbsr->nRows++;
}

static void rbsize_add_band(rbsize_result_t *rbsr, int left, int top, int right, int bottom, DWORD fStyle, UINT cx)
{
    SetRect(&(rbsr->bands[rbsr->nBands].rc), left, top, right, bottom);
    rbsr->bands[rbsr->nBands].fStyle = fStyle;
    rbsr->bands[rbsr->nBands].cx = cx;
    rbsr->nBands++;
}

static rbsize_result_t *rbsize_results;

#define rbsize_results_num 27

static void rbsize_results_init(void)
{
    rbsize_results = heap_alloc(rbsize_results_num * sizeof(*rbsize_results));

    rbsize_results[0] = rbsize_init(0, 0, 672, 0, 0, 0, 0);

    rbsize_results[1] = rbsize_init(0, 0, 672, 4, 4, 1, 1);
    rbsize_add_row(&rbsize_results[1], 4);
    rbsize_add_band(&rbsize_results[1], 0, 0, 672, 4, 0x00, 200);

    rbsize_results[2] = rbsize_init(0, 0, 672, 4, 4, 1, 2);
    rbsize_add_row(&rbsize_results[2], 4);
    rbsize_add_band(&rbsize_results[2], 0, 0, 200, 4, 0x00, 200);
    rbsize_add_band(&rbsize_results[2], 200, 0, 672, 4, 0x04, 200);

    rbsize_results[3] = rbsize_init(0, 0, 672, 30, 30, 1, 3);
    rbsize_add_row(&rbsize_results[3], 30);
    rbsize_add_band(&rbsize_results[3], 0, 0, 200, 30, 0x00, 200);
    rbsize_add_band(&rbsize_results[3], 200, 0, 400, 30, 0x04, 200);
    rbsize_add_band(&rbsize_results[3], 400, 0, 672, 30, 0x00, 200);

    rbsize_results[4] = rbsize_init(0, 0, 672, 34, 34, 1, 4);
    rbsize_add_row(&rbsize_results[4], 34);
    rbsize_add_band(&rbsize_results[4], 0, 0, 200, 34, 0x00, 200);
    rbsize_add_band(&rbsize_results[4], 200, 0, 400, 34, 0x04, 200);
    rbsize_add_band(&rbsize_results[4], 400, 0, 604, 34, 0x00, 200);
    rbsize_add_band(&rbsize_results[4], 604, 0, 672, 34, 0x04, 68);

    rbsize_results[5] = rbsize_init(0, 0, 672, 34, 34, 1, 4);
    rbsize_add_row(&rbsize_results[5], 34);
    rbsize_add_band(&rbsize_results[5], 0, 0, 200, 34, 0x00, 200);
    rbsize_add_band(&rbsize_results[5], 200, 0, 400, 34, 0x04, 200);
    rbsize_add_band(&rbsize_results[5], 400, 0, 604, 34, 0x00, 200);
    rbsize_add_band(&rbsize_results[5], 604, 0, 672, 34, 0x04, 68);

    rbsize_results[6] = rbsize_init(0, 0, 672, 34, 34, 1, 4);
    rbsize_add_row(&rbsize_results[6], 34);
    rbsize_add_band(&rbsize_results[6], 0, 0, 200, 34, 0x00, 200);
    rbsize_add_band(&rbsize_results[6], 202, 0, 402, 34, 0x04, 200);
    rbsize_add_band(&rbsize_results[6], 404, 0, 604, 34, 0x00, 200);
    rbsize_add_band(&rbsize_results[6], 606, 0, 672, 34, 0x04, 66);

    rbsize_results[7] = rbsize_init(0, 0, 672, 70, 70, 2, 5);
    rbsize_add_row(&rbsize_results[7], 34);
    rbsize_add_row(&rbsize_results[7], 34);
    rbsize_add_band(&rbsize_results[7], 0, 0, 142, 34, 0x00, 200);
    rbsize_add_band(&rbsize_results[7], 144, 0, 557, 34, 0x00, 200);
    rbsize_add_band(&rbsize_results[7], 559, 0, 672, 34, 0x04, 200);
    rbsize_add_band(&rbsize_results[7], 0, 36, 200, 70, 0x00, 200);
    rbsize_add_band(&rbsize_results[7], 202, 36, 672, 70, 0x04, 66);

    rbsize_results[8] = rbsize_init(0, 0, 672, 34, 34, 1, 5);
    rbsize_add_row(&rbsize_results[8], 34);
    rbsize_add_band(&rbsize_results[8], 0, 0, 167, 34, 0x00, 200);
    rbsize_add_band(&rbsize_results[8], 169, 0, 582, 34, 0x00, 200);
    rbsize_add_band(&rbsize_results[8], 559, 0, 759, 34, 0x08, 200);
    rbsize_add_band(&rbsize_results[8], 584, 0, 627, 34, 0x00, 200);
    rbsize_add_band(&rbsize_results[8], 629, 0, 672, 34, 0x04, 66);

    rbsize_results[9] = rbsize_init(0, 0, 672, 34, 34, 1, 4);
    rbsize_add_row(&rbsize_results[9], 34);
    rbsize_add_band(&rbsize_results[9], 0, 0, 167, 34, 0x00, 200);
    rbsize_add_band(&rbsize_results[9], 169, 0, 582, 34, 0x00, 200);
    rbsize_add_band(&rbsize_results[9], 584, 0, 627, 34, 0x00, 200);
    rbsize_add_band(&rbsize_results[9], 629, 0, 672, 34, 0x04, 66);

    rbsize_results[10] = rbsize_init(0, 0, 672, 34, 34, 1, 3);
    rbsize_add_row(&rbsize_results[10], 34);
    rbsize_add_band(&rbsize_results[10], 0, 0, 413, 34, 0x00, 200);
    rbsize_add_band(&rbsize_results[10], 415, 0, 615, 34, 0x00, 200);
    rbsize_add_band(&rbsize_results[10], 617, 0, 672, 34, 0x04, 66);

    rbsize_results[11] = rbsize_init(0, 0, 672, 34, 34, 1, 2);
    rbsize_add_row(&rbsize_results[11], 34);
    rbsize_add_band(&rbsize_results[11], 0, 0, 604, 34, 0x00, 200);
    rbsize_add_band(&rbsize_results[11], 606, 0, 672, 34, 0x04, 66);

    rbsize_results[12] = rbsize_init(0, 0, 672, 8 + 2*system_font_height, 40, 2, 5);
    rbsize_add_row(&rbsize_results[12], 4 + system_font_height);
    rbsize_add_row(&rbsize_results[12], 4 + system_font_height);
    rbsize_add_band(&rbsize_results[12], 0, 0, 87 + string_width("ABC"), 4 + system_font_height, 0x00, 40);
    rbsize_add_band(&rbsize_results[12], 87 + string_width("ABC"), 0, 157 + string_width("ABC"), 4 + system_font_height, 0x00, 70);
    rbsize_add_band(&rbsize_results[12], 157 + string_width("ABC"), 0, 397 + string_width("ABC"), 4 + system_font_height, 0x00, 240);
    rbsize_add_band(&rbsize_results[12], 397 + string_width("ABC"), 0, 672, 4 + system_font_height, 0x00, 60);
    rbsize_add_band(&rbsize_results[12], 0, 4 + system_font_height, 672, 8 + 2*system_font_height, 0x00, 200);

    rbsize_results[13] = rbsize_init(0, 0, 672, 8 + 2*system_font_height, 40, 2, 5);
    rbsize_add_row(&rbsize_results[13], 4 + system_font_height);
    rbsize_add_row(&rbsize_results[13], 4 + system_font_height);
    rbsize_add_band(&rbsize_results[13], 0, 0, 87 + string_width("ABC"), 4 + system_font_height, 0x00, 40);
    rbsize_add_band(&rbsize_results[13], 87 + string_width("ABC"), 0, 200 + string_width("ABC"), 4 + system_font_height, 0x00, 113);
    rbsize_add_band(&rbsize_results[13], 200 + string_width("ABC"), 0, 397 + string_width("ABC"), 4 + system_font_height, 0x00, 197);
    rbsize_add_band(&rbsize_results[13], 397 + string_width("ABC"), 0, 672, 4 + system_font_height, 0x00, 60);
    rbsize_add_band(&rbsize_results[13], 0, 4 + system_font_height, 672, 8 + 2*system_font_height, 0x00, 200);

    rbsize_results[14] = rbsize_init(0, 0, 672, 8 + 2*system_font_height, 40, 2, 5);
    rbsize_add_row(&rbsize_results[14], 4 + system_font_height);
    rbsize_add_row(&rbsize_results[14], 4 + system_font_height);
    rbsize_add_band(&rbsize_results[14], 0, 0, 87 + string_width("ABC"), 4 + system_font_height, 0x00, 40);
    rbsize_add_band(&rbsize_results[14], 87 + string_width("ABC"), 0, 412 - string_width("MMMMMMM"), 4 + system_font_height, 0x00, 325 - string_width("ABC") - string_width("MMMMMMM"));
    rbsize_add_band(&rbsize_results[14], 412 - string_width("MMMMMMM"), 0, 595 - string_width("MMMMMMM"), 4 + system_font_height, 0x00, 183);
    rbsize_add_band(&rbsize_results[14], 595 - string_width("MMMMMMM"), 0, 672, 4 + system_font_height, 0x00, 77 + string_width("MMMMMMM"));
    rbsize_add_band(&rbsize_results[14], 0, 4 + system_font_height, 672, 8 + 2*system_font_height, 0x00, 200);

    rbsize_results[15] = rbsize_init(0, 0, 672, 8 + 2*system_font_height, 40, 2, 5);
    rbsize_add_row(&rbsize_results[15], 4 + system_font_height);
    rbsize_add_row(&rbsize_results[15], 4 + system_font_height);
    rbsize_add_band(&rbsize_results[15], 0, 0, 87 + string_width("ABC"), 4 + system_font_height, 0x00, 40);
    rbsize_add_band(&rbsize_results[15], 87 + string_width("ABC"), 0, 140 + string_width("ABC"), 4 + system_font_height, 0x00, 53);
    rbsize_add_band(&rbsize_results[15], 140 + string_width("ABC"), 0, 595 - string_width("MMMMMMM"), 4 + system_font_height, 0x00, 455 - string_width("MMMMMMM") - string_width("ABC"));
    rbsize_add_band(&rbsize_results[15], 595 - string_width("MMMMMMM"), 0, 672, 4 + system_font_height, 0x00, 77 + string_width("MMMMMMM"));
    rbsize_add_band(&rbsize_results[15], 0, 4 + system_font_height, 672, 8 + 2*system_font_height, 0x00, 200);

    rbsize_results[16] = rbsize_init(0, 0, 672, 8 + 2*system_font_height, 40, 2, 5);
    rbsize_add_row(&rbsize_results[16], 4 + system_font_height);
    rbsize_add_row(&rbsize_results[16], 4 + system_font_height);
    rbsize_add_band(&rbsize_results[16], 0, 0, 87 + string_width("ABC"), 4 + system_font_height, 0x00, 40);
    rbsize_add_band(&rbsize_results[16], 87 + string_width("ABC"), 0, 412 - string_width("MMMMMMM"), 4 + system_font_height, 0x00, 325 - string_width("ABC") - string_width("MMMMMMM"));
    rbsize_add_band(&rbsize_results[16], 412 - string_width("MMMMMMM"), 0, 595 - string_width("MMMMMMM"), 4 + system_font_height, 0x00, 183);
    rbsize_add_band(&rbsize_results[16], 595 - string_width("MMMMMMM"), 0, 672, 4 + system_font_height, 0x00, 77 + string_width("MMMMMMM"));
    rbsize_add_band(&rbsize_results[16], 0, 4 + system_font_height, 672, 8 + 2*system_font_height, 0x00, 200);

    rbsize_results[17] = rbsize_init(0, 0, 672, 8 + 2*system_font_height, 40, 2, 5);
    rbsize_add_row(&rbsize_results[17], 4 + system_font_height);
    rbsize_add_row(&rbsize_results[17], 4 + system_font_height);
    rbsize_add_band(&rbsize_results[17], 0, 0, 87 + string_width("ABC"), 4 + system_font_height, 0x00, 40);
    rbsize_add_band(&rbsize_results[17], 87 + string_width("ABC"), 0, 412 - string_width("MMMMMMM"), 4 + system_font_height, 0x00, 325 - string_width("ABC") - string_width("MMMMMMM"));
    rbsize_add_band(&rbsize_results[17], 412 - string_width("MMMMMMM"), 0, 595 - string_width("MMMMMMM"), 4 + system_font_height, 0x00, 183);
    rbsize_add_band(&rbsize_results[17], 595 - string_width("MMMMMMM"), 0, 672, 4 + system_font_height, 0x00, 77 + string_width("MMMMMMM"));
    rbsize_add_band(&rbsize_results[17], 0, 4 + system_font_height, 672, 8 + 2*system_font_height, 0x00, 200);

    rbsize_results[18] = rbsize_init(0, 0, 672, 56, 56, 2, 5);
    rbsize_add_row(&rbsize_results[18], 28);
    rbsize_add_row(&rbsize_results[18], 28);
    rbsize_add_band(&rbsize_results[18], 0, 0, 87 + string_width("ABC"), 28, 0x00, 40);
    rbsize_add_band(&rbsize_results[18], 87 + string_width("ABC"), 0, 412 - string_width("MMMMMMM"), 28, 0x00, 325 - string_width("ABC") - string_width("MMMMMMM"));
    rbsize_add_band(&rbsize_results[18], 412 - string_width("MMMMMMM"), 0, 595 - string_width("MMMMMMM"), 28, 0x00, 183);
    rbsize_add_band(&rbsize_results[18], 595 - string_width("MMMMMMM"), 0, 672, 28, 0x00, 77 + string_width("MMMMMMM"));
    rbsize_add_band(&rbsize_results[18], 0, 28, 672, 56, 0x00, 200);

    rbsize_results[19] = rbsize_init(0, 0, 672, 8 + 2*system_font_height, 40, 2, 5);
    rbsize_add_row(&rbsize_results[19], 4 + system_font_height);
    rbsize_add_row(&rbsize_results[19], 4 + system_font_height);
    rbsize_add_band(&rbsize_results[19], 0, 0, 87 + string_width("ABC"), 4 + system_font_height, 0x00, 40);
    rbsize_add_band(&rbsize_results[19], 87 + string_width("ABC"), 0, 412 - string_width("MMMMMMM"), 4 + system_font_height, 0x00, 325 - string_width("ABC") - string_width("MMMMMMM"));
    rbsize_add_band(&rbsize_results[19], 412 - string_width("MMMMMMM"), 0, 595 - string_width("MMMMMMM"), 4 + system_font_height, 0x00, 183);
    rbsize_add_band(&rbsize_results[19], 595 - string_width("MMMMMMM"), 0, 672, 4 + system_font_height, 0x00, 77 + string_width("MMMMMMM"));
    rbsize_add_band(&rbsize_results[19], 0, 4 + system_font_height, 672, 8 + 2*system_font_height, 0x00, 200);

    rbsize_results[20] = rbsize_init(0, 0, 672, 56, 56, 2, 5);
    rbsize_add_row(&rbsize_results[20], 28);
    rbsize_add_row(&rbsize_results[20], 28);
    rbsize_add_band(&rbsize_results[20], 0, 0, 87 + string_width("ABC"), 28, 0x00, 40);
    rbsize_add_band(&rbsize_results[20], 87 + string_width("ABC"), 0, 412 - string_width("MMMMMMM"), 28, 0x00, 325 - string_width("ABC") - string_width("MMMMMMM"));
    rbsize_add_band(&rbsize_results[20], 412 - string_width("MMMMMMM"), 0,  595 - string_width("MMMMMMM"), 28, 0x00, 183);
    rbsize_add_band(&rbsize_results[20],  595 - string_width("MMMMMMM"), 0, 672, 28, 0x00, 77 + string_width("MMMMMMM"));
    rbsize_add_band(&rbsize_results[20], 0, 28, 672, 56, 0x00, 200);

    rbsize_results[21] = rbsize_init(0, 0, 672, 0, 0, 0, 0);

    rbsize_results[22] = rbsize_init(0, 0, 672, 65, 56, 1, 3);
    rbsize_add_row(&rbsize_results[22], 65);
    rbsize_add_band(&rbsize_results[22], 0, 0, 90, 65, 0x40, 90);
    rbsize_add_band(&rbsize_results[22], 90, 0, 180, 65, 0x40, 90);
    rbsize_add_band(&rbsize_results[22], 180, 0, 672, 65, 0x40, 90);

    rbsize_results[23] = rbsize_init(0, 0, 0, 226, 0, 0, 0);

    rbsize_results[24] = rbsize_init(0, 0, 65, 226, 65, 1, 1);
    rbsize_add_row(&rbsize_results[24], 65);
    rbsize_add_band(&rbsize_results[24], 0, 0, 226, 65, 0x40, 90);

    rbsize_results[25] = rbsize_init(0, 0, 65, 226, 65, 1, 2);
    rbsize_add_row(&rbsize_results[25], 65);
    rbsize_add_band(&rbsize_results[25], 0, 0, 90, 65, 0x40, 90);
    rbsize_add_band(&rbsize_results[25], 90, 0, 226, 65, 0x40, 90);

    rbsize_results[26] = rbsize_init(0, 0, 65, 226, 65, 1, 3);
    rbsize_add_row(&rbsize_results[26], 65);
    rbsize_add_band(&rbsize_results[26], 0, 0, 90, 65, 0x40, 90);
    rbsize_add_band(&rbsize_results[26], 90, 0, 163, 65, 0x40, 90);
    rbsize_add_band(&rbsize_results[26], 163, 0, 226, 65, 0x40, 90);
}

static void rbsize_results_free(void)
{
    int i;

    for (i = 0; i < rbsize_results_num; i++) {
        heap_free(rbsize_results[i].cyRowHeights);
        heap_free(rbsize_results[i].bands);
    }
    heap_free(rbsize_results);
    rbsize_results = NULL;
}

static int rbsize_numtests = 0;

#define check_sizes_todo(todomask) { \
        RECT rc; \
        REBARBANDINFOA rbi; \
        int count, i/*, mask=(todomask)*/; \
        const rbsize_result_t *res = &rbsize_results[rbsize_numtests]; \
        GetClientRect(hRebar, &rc); \
        check_rect("client", rc, res->rcClient); \
        count = SendMessageA(hRebar, RB_GETROWCOUNT, 0, 0); \
        compare(count, res->nRows, "%d"); \
        for (i=0; i<min(count, res->nRows); i++) { \
            int height = SendMessageA(hRebar, RB_GETROWHEIGHT, 0, 0);\
            ok(height == res->cyRowHeights[i], "Height mismatch for row %d - %d vs %d\n", i, res->cyRowHeights[i], height); \
        } \
        count = SendMessageA(hRebar, RB_GETBANDCOUNT, 0, 0); \
        compare(count, res->nBands, "%d"); \
        for (i=0; i<min(count, res->nBands); i++) { \
            ok(SendMessageA(hRebar, RB_GETRECT, i, (LPARAM)&rc) == 1, "RB_GETRECT\n"); \
            if (!(res->bands[i].fStyle & RBBS_HIDDEN)) \
                check_rect("band", rc, res->bands[i].rc); \
            rbi.cbSize = REBARBANDINFOA_V6_SIZE; \
            rbi.fMask = RBBIM_STYLE | RBBIM_SIZE; \
            ok(SendMessageA(hRebar, RB_GETBANDINFOA,  i, (LPARAM)&rbi) == 1, "RB_GETBANDINFOA\n"); \
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
    REBARBANDINFOA rbi;

    if (lpszText != NULL)
        strcpy(buffer, lpszText);
    rbi.cbSize = REBARBANDINFOA_V6_SIZE;
    rbi.fMask = RBBIM_SIZE | RBBIM_CHILDSIZE | RBBIM_CHILD | RBBIM_IDEALSIZE | RBBIM_TEXT;
    rbi.cx = cx;
    rbi.cxMinChild = cxMinChild;
    rbi.cxIdeal = cxIdeal;
    rbi.cyMinChild = 20;
    rbi.hwndChild = build_toolbar(1, hRebar);
    rbi.lpText = (lpszText ? buffer : NULL);
    SendMessageA(hRebar, RB_INSERTBANDA, -1, (LPARAM)&rbi);
}

static void test_layout(void)
{
    HWND hRebar;
    REBARBANDINFOA rbi;
    HIMAGELIST himl;
    REBARINFO ri;
    int count;

    rbsize_results_init();

    hRebar = create_rebar_control();
    check_sizes();
    rbi.cbSize = REBARBANDINFOA_V6_SIZE;
    rbi.fMask = RBBIM_SIZE | RBBIM_CHILDSIZE | RBBIM_CHILD;
    rbi.cx = 200;
    rbi.cxMinChild = 100;
    rbi.cyMinChild = 30;
    rbi.hwndChild = NULL;
    SendMessageA(hRebar, RB_INSERTBANDA, -1, (LPARAM)&rbi);
    check_sizes();

    rbi.fMask |= RBBIM_STYLE;
    rbi.fStyle = RBBS_CHILDEDGE;
    SendMessageA(hRebar, RB_INSERTBANDA, -1, (LPARAM)&rbi);
    check_sizes();

    rbi.fStyle = 0;
    rbi.cx = 200;
    rbi.cxMinChild = 30;
    rbi.cyMinChild = 30;
    rbi.hwndChild = build_toolbar(0, hRebar);
    SendMessageA(hRebar, RB_INSERTBANDA, -1, (LPARAM)&rbi);
    check_sizes();

    rbi.fStyle = RBBS_CHILDEDGE;
    rbi.cx = 68;
    rbi.hwndChild = build_toolbar(0, hRebar);
    SendMessageA(hRebar, RB_INSERTBANDA, -1, (LPARAM)&rbi);
    check_sizes();

    SetWindowLongA(hRebar, GWL_STYLE, GetWindowLongA(hRebar, GWL_STYLE) | RBS_BANDBORDERS);
    check_sizes();      /* a style change won't start a relayout */
    rbi.fMask = RBBIM_SIZE;
    rbi.cx = 66;
    SendMessageA(hRebar, RB_SETBANDINFOA, 3, (LPARAM)&rbi);
    check_sizes();      /* here it will be relayouted */

    /* this will force a new row */
    rbi.fMask = RBBIM_SIZE | RBBIM_CHILDSIZE | RBBIM_CHILD;
    rbi.cx = 200;
    rbi.cxMinChild = 400;
    rbi.cyMinChild = 30;
    rbi.hwndChild = build_toolbar(0, hRebar);
    SendMessageA(hRebar, RB_INSERTBANDA, 1, (LPARAM)&rbi);
    check_sizes();

    rbi.fMask = RBBIM_STYLE;
    rbi.fStyle = RBBS_HIDDEN;
    SendMessageA(hRebar, RB_SETBANDINFOA, 2, (LPARAM)&rbi);
    check_sizes();

    SendMessageA(hRebar, RB_DELETEBAND, 2, 0);
    check_sizes();
    SendMessageA(hRebar, RB_DELETEBAND, 0, 0);
    check_sizes();
    SendMessageA(hRebar, RB_DELETEBAND, 1, 0);
    check_sizes();

    DestroyWindow(hRebar);

    hRebar = create_rebar_control();
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

    /* an image will increase the band height */
    himl = pImageList_LoadImageA(GetModuleHandleA("comctl32"), MAKEINTRESOURCEA(121), 24, 2,
            CLR_NONE, IMAGE_BITMAP, LR_DEFAULTCOLOR);
    ri.cbSize = sizeof(ri);
    ri.fMask = RBIM_IMAGELIST;
    ri.himl = himl;
    ok(SendMessageA(hRebar, RB_SETBARINFO, 0, (LPARAM)&ri), "RB_SETBARINFO failed\n");
    rbi.fMask = RBBIM_IMAGE;
    rbi.iImage = 1;
    SendMessageA(hRebar, RB_SETBANDINFOA, 1, (LPARAM)&rbi);
    check_sizes();

    /* after removing it everything is back to normal*/
    rbi.iImage = -1;
    SendMessageA(hRebar, RB_SETBANDINFOA, 1, (LPARAM)&rbi);
    check_sizes();

    /* Only -1 means that the image is not present. Other invalid values increase the height */
    rbi.iImage = -2;
    SendMessageA(hRebar, RB_SETBANDINFOA, 1, (LPARAM)&rbi);
    check_sizes();

    DestroyWindow(hRebar);

    /* VARHEIGHT resizing test on a horizontal rebar */
    hRebar = create_rebar_control();
    SetWindowLongA(hRebar, GWL_STYLE, GetWindowLongA(hRebar, GWL_STYLE) | RBS_AUTOSIZE);
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
    SendMessageA(hRebar, RB_INSERTBANDA, -1, (LPARAM)&rbi);

    rbi.cyChild = 50;
    rbi.hwndChild = build_toolbar(0, hRebar);
    SendMessageA(hRebar, RB_INSERTBANDA, -1, (LPARAM)&rbi);

    rbi.cyMinChild = 40;
    rbi.cyChild = 50;
    rbi.cyIntegral = 5;
    rbi.hwndChild = build_toolbar(0, hRebar);
    SendMessageA(hRebar, RB_INSERTBANDA, -1, (LPARAM)&rbi);
    check_sizes();

    DestroyWindow(hRebar);

    /* VARHEIGHT resizing on a vertical rebar */
    hRebar = create_rebar_control();
    SetWindowLongA(hRebar, GWL_STYLE, GetWindowLongA(hRebar, GWL_STYLE) | CCS_VERT | RBS_AUTOSIZE);
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
    SendMessageA(hRebar, RB_INSERTBANDA, -1, (LPARAM)&rbi);
    check_sizes();

    rbi.cyChild = 50;
    rbi.hwndChild = build_toolbar(0, hRebar);
    SendMessageA(hRebar, RB_INSERTBANDA, -1, (LPARAM)&rbi);
    check_sizes();

    rbi.cyMinChild = 40;
    rbi.cyChild = 50;
    rbi.cyIntegral = 5;
    rbi.hwndChild = build_toolbar(0, hRebar);
    SendMessageA(hRebar, RB_INSERTBANDA, -1, (LPARAM)&rbi);
    check_sizes();

    DestroyWindow(hRebar);
    pImageList_Destroy(himl);

    /* One hidden band. */
    hRebar = create_rebar_control();

    rbi.cbSize = REBARBANDINFOA_V6_SIZE;
    rbi.fMask = RBBIM_STYLE | RBBIM_SIZE | RBBIM_CHILDSIZE | RBBIM_CHILD;
    rbi.fStyle = RBBS_HIDDEN;
    rbi.cx = 200;
    rbi.cxMinChild = 100;
    rbi.cyMinChild = 30;
    rbi.hwndChild = NULL;

    SendMessageA(hRebar, RB_INSERTBANDA, -1, (LPARAM)&rbi);
    count = SendMessageA(hRebar, RB_GETROWCOUNT, 0, 0);
    ok(!count, "Unexpected row count %d.\n", count);

    DestroyWindow(hRebar);

    rbsize_results_free();
}

#if 0       /* use this to generate more tests */

static void dump_client(HWND hRebar)
{
    RECT r;
    BOOL notify;
    GetWindowRect(hRebar, &r);
    MapWindowPoints(HWND_DESKTOP, hMainWnd, &r, 2);
    if (height_change_notify_rect.top != -1)
    {
        RECT rcClient;
        GetClientRect(hRebar, &rcClient);
        assert(EqualRect(&rcClient, &height_change_notify_rect));
        notify = TRUE;
    }
    else
        notify = FALSE;
    printf("    {{%d, %d, %d, %d}, %d, %s},\n", r.left, r.top, r.right, r.bottom, SendMessageA(hRebar, RB_GETROWCOUNT, 0, 0),
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

static const rbresize_test_result_t resize_results[] = {
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

static DWORD resize_numtests = 0;

#define comment(fmt, arg1)
#define check_client() { \
        RECT r; \
        int value; \
        const rbresize_test_result_t *res = &resize_results[resize_numtests++]; \
        assert(resize_numtests <= ARRAY_SIZE(resize_results)); \
        GetWindowRect(hRebar, &r); \
        MapWindowPoints(HWND_DESKTOP, hMainWnd, (LPPOINT)&r, 2); \
        if ((dwStyles[i] & (CCS_NOPARENTALIGN|CCS_NODIVIDER)) == CCS_NOPARENTALIGN) {\
            check_rect_no_top("client", r, res->rc); /* the top coordinate changes after every layout and is very implementation-dependent */ \
        } else { \
            check_rect("client", r, res->rc); \
        } \
        value = (int)SendMessageA(hRebar, RB_GETROWCOUNT, 0, 0); \
        ok(res->iNumRows == value, "RB_GETROWCOUNT expected %d got %d\n", res->iNumRows, value); \
        if (res->heightNotify) { \
            RECT rcClient; \
            GetClientRect(hRebar, &rcClient); \
            check_rect("notify", height_change_notify_rect, rcClient); \
        } else ok(height_change_notify_rect.top == -1, "Unexpected RBN_HEIGHTCHANGE received\n"); \
        SetRect(&height_change_notify_rect, -1, -1, -1, -1); \
    }

#endif

static void test_resize(void)
{
    DWORD dwStyles[] = {CCS_TOP, CCS_TOP | CCS_NODIVIDER, CCS_BOTTOM, CCS_BOTTOM | CCS_NODIVIDER, CCS_VERT, CCS_RIGHT,
        CCS_NOPARENTALIGN, CCS_NOPARENTALIGN | CCS_NODIVIDER, CCS_NORESIZE, CCS_NOMOVEY, CCS_NOMOVEY | CCS_VERT,
        CCS_TOP | WS_BORDER, CCS_NOPARENTALIGN | CCS_NODIVIDER | WS_BORDER, CCS_NORESIZE | WS_BORDER,
        CCS_NOMOVEY | WS_BORDER};

    const int styles_count = ARRAY_SIZE(dwStyles);
    int i;

    for (i = 0; i < styles_count; i++)
    {
        HWND hRebar;

        comment("style %08x", dwStyles[i]);
        SetRect(&height_change_notify_rect, -1, -1, -1, -1);
        hRebar = CreateWindowA(REBARCLASSNAMEA, "A", dwStyles[i] | WS_CHILD | WS_VISIBLE, 10, 5, 500, 15, hMainWnd, NULL, GetModuleHandleA(NULL), 0);
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
            SendMessageA(hRebar, WM_WINDOWPOSCHANGING, 0, (LPARAM)&pos);
            SendMessageA(hRebar, WM_WINDOWPOSCHANGED, 0, (LPARAM)&pos);
            check_client();
            SendMessageA(hRebar, WM_SIZE, SIZE_RESTORED, MAKELONG(500, 500));
            check_client();
        }
        SendMessageA(hRebar, RB_DELETEBAND, 0, 0);
        check_client();
        SendMessageA(hRebar, RB_DELETEBAND, 0, 0);
        MoveWindow(hRebar, 0, 0, 100, 100, TRUE);
        check_client();
        DestroyWindow(hRebar);
    }
}

static void expect_band_content_(int line, HWND hRebar, UINT uBand, INT fStyle, COLORREF clrFore,
    COLORREF clrBack, LPCSTR lpText, int iImage, HWND hwndChild,
    INT cxMinChild, INT cyMinChild, INT cx, HBITMAP hbmBack, INT wID,
    INT cyChild, INT cyMaxChild, INT cyIntegral, INT cxIdeal, LPARAM lParam,
    UINT cxHeader, UINT cxHeader_broken)
{
    CHAR buf[MAX_PATH] = "abc";
    REBARBANDINFOA rb;

    memset(&rb, 0xdd, sizeof(rb));
    rb.cbSize = REBARBANDINFOA_V6_SIZE;
    rb.fMask = RBBIM_BACKGROUND | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_COLORS
        | RBBIM_HEADERSIZE | RBBIM_ID | RBBIM_IDEALSIZE | RBBIM_IMAGE | RBBIM_LPARAM
        | RBBIM_SIZE | RBBIM_STYLE | RBBIM_TEXT;
    rb.lpText = buf;
    rb.cch = MAX_PATH;
    ok(SendMessageA(hRebar, RB_GETBANDINFOA, uBand, (LPARAM)&rb), "RB_GETBANDINFOA failed from line %d\n", line);
    expect_eq(line, rb.fStyle, fStyle, int, "%x");
    expect_eq(line, rb.clrFore, clrFore, COLORREF, "%x");
    expect_eq(line, rb.clrBack, clrBack, COLORREF, "%x");
    expect_eq(line, strcmp(rb.lpText, lpText), 0, int, "%d");
    expect_eq(line, rb.iImage, iImage, int, "%x");
    expect_eq(line, rb.hwndChild, hwndChild, HWND, "%p");
    expect_eq(line, rb.cxMinChild, cxMinChild, int, "%d");
    expect_eq(line, rb.cyMinChild, cyMinChild, int, "%d");
    expect_eq(line, rb.cx, cx, int, "%d");
    expect_eq(line, rb.hbmBack, hbmBack, HBITMAP, "%p");
    expect_eq(line, rb.wID, wID, int, "%d");
    /* the values of cyChild, cyMaxChild and cyIntegral can't be read unless the band is RBBS_VARIABLEHEIGHT */
    expect_eq(line, rb.cyChild, cyChild, int, "%x");
    expect_eq(line, rb.cyMaxChild, cyMaxChild, int, "%x");
    expect_eq(line, rb.cyIntegral, cyIntegral, int, "%x");
    expect_eq(line, rb.cxIdeal, cxIdeal, int, "%d");
    expect_eq(line, rb.lParam, lParam, LPARAM, "%ld");
    ok(rb.cxHeader == cxHeader || rb.cxHeader == cxHeader + 1 || broken(rb.cxHeader == cxHeader_broken),
        "expected %d for %d from line %d\n", cxHeader, rb.cxHeader, line);
}

#define expect_band_content(hRebar, uBand, fStyle, clrFore, clrBack,\
 lpText, iImage, hwndChild, cxMinChild, cyMinChild, cx, hbmBack, wID,\
 cyChild, cyMaxChild, cyIntegral, cxIdeal, lParam, cxHeader, cxHeader_broken) \
 expect_band_content_(__LINE__, hRebar, uBand, fStyle, clrFore, clrBack,\
 lpText, iImage, hwndChild, cxMinChild, cyMinChild, cx, hbmBack, wID,\
 cyChild, cyMaxChild, cyIntegral, cxIdeal, lParam, cxHeader, cxHeader_broken)

static void test_bandinfo(void)
{
    REBARBANDINFOA rb;
    CHAR szABC[] = "ABC";
    CHAR szABCD[] = "ABCD";
    HWND hRebar;

    hRebar = create_rebar_control();
    rb.cbSize = REBARBANDINFOA_V6_SIZE;
    rb.fMask = 0;
    if (!SendMessageA(hRebar, RB_INSERTBANDA, 0, (LPARAM)&rb))
    {
        win_skip( "V6 info not supported\n" );
        DestroyWindow(hRebar);
        return;
    }
    expect_band_content(hRebar, 0, 0, 0, GetSysColor(COLOR_3DFACE), "", -1, NULL, 0, 0, 0, NULL, 0, 0xdddddddd, 0xdddddddd, 0xdddddddd, 0, 0, 0, -1);

    rb.fMask = RBBIM_CHILDSIZE;
    rb.cxMinChild = 15;
    rb.cyMinChild = 20;
    rb.cyChild = 30;
    rb.cyMaxChild = 20;
    rb.cyIntegral = 10;
    ok(SendMessageA(hRebar, RB_SETBANDINFOA, 0, (LPARAM)&rb), "RB_SETBANDINFOA failed\n");
    expect_band_content(hRebar, 0, 0, 0, GetSysColor(COLOR_3DFACE), "", -1, NULL, 15, 20, 0, NULL, 0, 0xdddddddd, 0xdddddddd, 0xdddddddd, 0, 0, 0, -1);

    rb.fMask = RBBIM_TEXT;
    rb.lpText = szABC;
    ok(SendMessageA(hRebar, RB_SETBANDINFOA, 0, (LPARAM)&rb), "RB_SETBANDINFOA failed\n");
    expect_band_content(hRebar, 0, 0, 0, GetSysColor(COLOR_3DFACE), "ABC", -1, NULL, 15, 20, 0, NULL, 0, 0xdddddddd, 0xdddddddd, 0xdddddddd, 0, 0, 3 + 2*system_font_height, -1);

    rb.cbSize = REBARBANDINFOA_V6_SIZE;
    rb.fMask = 0;
    ok(SendMessageA(hRebar, RB_INSERTBANDA, 1, (LPARAM)&rb), "RB_INSERTBANDA failed\n");
    expect_band_content(hRebar, 1, 0, 0, GetSysColor(COLOR_3DFACE), "", -1, NULL, 0, 0, 0, NULL, 0, 0xdddddddd, 0xdddddddd, 0xdddddddd, 0, 0, 9, -1);
    expect_band_content(hRebar, 0, 0, 0, GetSysColor(COLOR_3DFACE), "ABC", -1, NULL, 15, 20, 0, NULL, 0, 0xdddddddd, 0xdddddddd, 0xdddddddd, 0, 0, 8 + 2*system_font_height, -1);

    rb.fMask = RBBIM_HEADERSIZE;
    rb.cxHeader = 50;
    ok(SendMessageA(hRebar, RB_SETBANDINFOA, 0, (LPARAM)&rb), "RB_SETBANDINFOA failed\n");
    expect_band_content(hRebar, 0, 0x40000000, 0, GetSysColor(COLOR_3DFACE), "ABC", -1, NULL, 15, 20, 0, NULL, 0, 0xdddddddd, 0xdddddddd, 0xdddddddd, 0, 0, 50, -1);

    rb.cxHeader = 5;
    ok(SendMessageA(hRebar, RB_SETBANDINFOA, 0, (LPARAM)&rb), "RB_SETBANDINFOA failed\n");
    expect_band_content(hRebar, 0, 0x40000000, 0, GetSysColor(COLOR_3DFACE), "ABC", -1, NULL, 15, 20, 0, NULL, 0, 0xdddddddd, 0xdddddddd, 0xdddddddd, 0, 0, 5, -1);

    rb.fMask = RBBIM_TEXT;
    rb.lpText = szABCD;
    ok(SendMessageA(hRebar, RB_SETBANDINFOA, 0, (LPARAM)&rb), "RB_SETBANDINFOA failed\n");
    expect_band_content(hRebar, 0, 0x40000000, 0, GetSysColor(COLOR_3DFACE), "ABCD", -1, NULL, 15, 20, 0, NULL, 0, 0xdddddddd, 0xdddddddd, 0xdddddddd, 0, 0, 5, -1);
    rb.fMask = RBBIM_STYLE | RBBIM_TEXT;
    rb.fStyle = RBBS_VARIABLEHEIGHT;
    rb.lpText = szABC;
    ok(SendMessageA(hRebar, RB_SETBANDINFOA, 0, (LPARAM)&rb), "RB_SETBANDINFOA failed\n");
    expect_band_content(hRebar, 0, RBBS_VARIABLEHEIGHT, 0, GetSysColor(COLOR_3DFACE), "ABC", -1, NULL, 15, 20, 0, NULL, 0, 20, 0x7fffffff, 0, 0, 0, 8 + 2*system_font_height, 5);

    DestroyWindow(hRebar);
}

static void test_colors(void)
{
    COLORSCHEME scheme;
    COLORREF clr;
    BOOL ret;
    HWND hRebar;
    REBARBANDINFOA bi;

    hRebar = create_rebar_control();

    /* test default colors */
    clr = SendMessageA(hRebar, RB_GETTEXTCOLOR, 0, 0);
    compare(clr, CLR_NONE, "%x");
    clr = SendMessageA(hRebar, RB_GETBKCOLOR, 0, 0);
    compare(clr, CLR_NONE, "%x");

    scheme.dwSize = sizeof(scheme);
    scheme.clrBtnHighlight = 0;
    scheme.clrBtnShadow = 0;
    ret = SendMessageA(hRebar, RB_GETCOLORSCHEME, 0, (LPARAM)&scheme);
    if (ret)
    {
        compare(scheme.clrBtnHighlight, CLR_DEFAULT, "%x");
        compare(scheme.clrBtnShadow, CLR_DEFAULT, "%x");
    }
    else
        skip("RB_GETCOLORSCHEME not supported\n");

    /* check default band colors */
    add_band_w(hRebar, "", 0, 10, 10);
    bi.cbSize = REBARBANDINFOA_V6_SIZE;
    bi.fMask = RBBIM_COLORS;
    bi.clrFore = bi.clrBack = 0xc0ffe;
    ret = SendMessageA(hRebar, RB_GETBANDINFOA, 0, (LPARAM)&bi);
    ok(ret, "RB_GETBANDINFOA failed\n");
    compare(bi.clrFore, RGB(0, 0, 0), "%x");
    compare(bi.clrBack, GetSysColor(COLOR_3DFACE), "%x");

    SendMessageA(hRebar, RB_SETTEXTCOLOR, 0, RGB(255, 0, 0));
    bi.clrFore = bi.clrBack = 0xc0ffe;
    ret = SendMessageA(hRebar, RB_GETBANDINFOA, 0, (LPARAM)&bi);
    ok(ret, "RB_GETBANDINFOA failed\n");
    compare(bi.clrFore, RGB(0, 0, 0), "%x");

    DestroyWindow(hRebar);
}


static BOOL register_parent_wnd_class(void)
{
    WNDCLASSA wc;

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.hIcon = NULL;
    wc.hCursor = LoadCursorA(NULL, (LPCSTR)IDC_IBEAM);
    wc.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "MyTestWnd";
    wc.lpfnWndProc = parent_wndproc;

    return RegisterClassA(&wc);
}

static HWND create_parent_window(void)
{
    HWND hwnd;

    if (!register_parent_wnd_class()) return NULL;

    hwnd = CreateWindowExA(0, "MyTestWnd", "Blah", WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT, 672+2*GetSystemMetrics(SM_CXSIZEFRAME),
      226+GetSystemMetrics(SM_CYCAPTION)+2*GetSystemMetrics(SM_CYSIZEFRAME),
      NULL, NULL, GetModuleHandleA(NULL), 0);

    ShowWindow(hwnd, SW_SHOW);
    return hwnd;
}

static void test_showband(void)
{
    HWND hRebar;
    REBARBANDINFOA rbi;
    BOOL ret;

    hRebar = create_rebar_control();

    /* no bands */
    ret = SendMessageA(hRebar, RB_SHOWBAND, 0, TRUE);
    ok(ret == FALSE, "got %d\n", ret);

    rbi.cbSize = REBARBANDINFOA_V6_SIZE;
    rbi.fMask = RBBIM_SIZE | RBBIM_CHILDSIZE | RBBIM_CHILD;
    rbi.cx = 200;
    rbi.cxMinChild = 100;
    rbi.cyMinChild = 30;
    rbi.hwndChild = NULL;
    SendMessageA(hRebar, RB_INSERTBANDA, -1, (LPARAM)&rbi);

    /* index out of range */
    ret = SendMessageA(hRebar, RB_SHOWBAND, 1, TRUE);
    ok(ret == FALSE, "got %d\n", ret);

    ret = SendMessageA(hRebar, RB_SHOWBAND, 0, TRUE);
    ok(ret == TRUE, "got %d\n", ret);

    DestroyWindow(hRebar);
}

static void test_notification(void)
{
    MEASUREITEMSTRUCT mis;
    HWND rebar;

    rebar = create_rebar_control();

    g_parent_measureitem = 0;
    SendMessageA(rebar, WM_MEASUREITEM, 0, (LPARAM)&mis);
    ok(g_parent_measureitem == 1, "got %d\n", g_parent_measureitem);

    DestroyWindow(rebar);
}

static void init_functions(void)
{
    HMODULE hComCtl32 = LoadLibraryA("comctl32.dll");

#define X(f) p##f = (void*)GetProcAddress(hComCtl32, #f);
    X(ImageList_Destroy);
    X(ImageList_LoadImageA);
#undef X
}

START_TEST(rebar)
{
    MSG msg;

    init_system_font_height();
    init_functions();

    hMainWnd = create_parent_window();

    test_bandinfo();
    test_colors();
    test_showband();
    test_notification();

    if(!is_font_installed("System") || !is_font_installed("Tahoma"))
    {
        skip("Missing System or Tahoma font\n");
        goto out;
    }

    test_layout();
    test_resize();

out:
    PostQuitMessage(0);
    while(GetMessageA(&msg,0,0,0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    DestroyWindow(hMainWnd);
}
