/* Unit test suite for tab control.
 *
 * Copyright 2003 Vitaliy Margolen
 * Copyright 2007 Hagop Hagopian
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

#include <wine/test.h>

#include <assert.h>
//#include <windows.h>
#include <wingdi.h>
#include <winuser.h>
#include <commctrl.h>
#include <stdio.h>

#include "msg.h"

#define DEFAULT_MIN_TAB_WIDTH 54
#define TAB_PADDING_X 6
#define EXTRA_ICON_PADDING 3
#define MAX_TABLEN 32

#define NUM_MSG_SEQUENCES  2
#define PARENT_SEQ_INDEX   0
#define TAB_SEQ_INDEX      1

#define expect(expected, got) ok ( expected == got, "Expected %d, got %d\n", expected, got)
#define expect_str(expected, got)\
 ok ( strcmp(expected, got) == 0, "Expected '%s', got '%s'\n", expected, got)

#define TabWidthPadded(padd_x, num) (DEFAULT_MIN_TAB_WIDTH - (TAB_PADDING_X - (padd_x)) * num)

static void CheckSize(HWND hwnd, INT width, INT height, const char *msg, int line)
{
    RECT r;

    SendMessageA(hwnd, TCM_GETITEMRECT, 0, (LPARAM)&r);
    if (width >= 0 && height < 0)
        ok_(__FILE__,line) (width == r.right - r.left, "%s: Expected width [%d] got [%d]\n",
            msg, width, r.right - r.left);
    else if (height >= 0 && width < 0)
        ok_(__FILE__,line) (height == r.bottom - r.top,  "%s: Expected height [%d] got [%d]\n",
            msg, height, r.bottom - r.top);
    else
        ok_(__FILE__,line) ((width  == r.right  - r.left) && (height == r.bottom - r.top ),
	    "%s: Expected [%d,%d] got [%d,%d]\n", msg, width, height,
            r.right - r.left, r.bottom - r.top);
}

#define CHECKSIZE(hwnd,width,height,msg) CheckSize(hwnd,width,height,msg,__LINE__)

static void TabCheckSetSize(HWND hwnd, INT set_width, INT set_height, INT exp_width,
    INT exp_height, const char *msg, int line)
{
    SendMessageA(hwnd, TCM_SETITEMSIZE, 0,
            MAKELPARAM((set_width >= 0) ? set_width : 0, (set_height >= 0) ? set_height : 0));
    if (winetest_interactive) RedrawWindow (hwnd, NULL, 0, RDW_UPDATENOW);
    CheckSize(hwnd, exp_width, exp_height, msg, line);
}

#define TABCHECKSETSIZE(hwnd,set_width,set_height,exp_width,exp_height,msg) \
    TabCheckSetSize(hwnd,set_width,set_height,exp_width,exp_height,msg,__LINE__)

static HFONT hFont;
static DRAWITEMSTRUCT g_drawitem;
static HWND parent_wnd;

static struct msg_sequence *sequences[NUM_MSG_SEQUENCES];

static const struct message add_tab_to_parent[] = {
    { TCM_INSERTITEMA, sent },
    { TCM_INSERTITEMA, sent|optional },
    { WM_NOTIFYFORMAT, sent|defwinproc },
    { WM_QUERYUISTATE, sent|wparam|lparam|defwinproc|optional, 0, 0 },
    { WM_PARENTNOTIFY, sent|defwinproc },
    { TCM_INSERTITEMA, sent },
    { TCM_INSERTITEMA, sent },
    { TCM_INSERTITEMA, sent },
    { TCM_INSERTITEMA, sent|optional },
    { 0 }
};

static const struct message add_tab_to_parent_interactive[] = {
    { TCM_INSERTITEMA, sent },
    { TCM_INSERTITEMA, sent },
    { WM_NOTIFYFORMAT, sent|defwinproc },
    { WM_QUERYUISTATE, sent|wparam|lparam|defwinproc, 0, 0 },
    { WM_PARENTNOTIFY, sent|defwinproc },
    { TCM_INSERTITEMA, sent },
    { TCM_INSERTITEMA, sent },
    { TCM_INSERTITEMA, sent },
    { WM_SHOWWINDOW, sent},
    { WM_WINDOWPOSCHANGING, sent},
    { WM_WINDOWPOSCHANGING, sent},
    { WM_NCACTIVATE, sent},
    { WM_ACTIVATE, sent},
    { WM_IME_SETCONTEXT, sent|defwinproc|optional},
    { WM_IME_NOTIFY, sent|defwinproc|optional},
    { WM_SETFOCUS, sent|defwinproc},
    { WM_WINDOWPOSCHANGED, sent},
    { WM_SIZE, sent},
    { WM_MOVE, sent},
    { 0 }
};

static const struct message add_tab_control_parent_seq[] = {
    { WM_NOTIFYFORMAT, sent },
    { WM_QUERYUISTATE, sent|wparam|lparam|optional, 0, 0 },
    { 0 }
};

static const struct message add_tab_control_parent_seq_interactive[] = {
    { WM_NOTIFYFORMAT, sent },
    { WM_QUERYUISTATE, sent|wparam|lparam, 0, 0 },
    { WM_WINDOWPOSCHANGING, sent|optional},
    { WM_NCACTIVATE, sent},
    { WM_ACTIVATE, sent},
    { WM_WINDOWPOSCHANGING, sent|optional},
    { WM_KILLFOCUS, sent},
    { WM_IME_SETCONTEXT, sent|optional},
    { WM_IME_NOTIFY, sent|optional},
    { 0 }
};

static const struct message empty_sequence[] = {
    { 0 }
};

static const struct message get_item_count_seq[] = {
    { TCM_GETITEMCOUNT, sent|wparam|lparam, 0, 0 },
    { 0 }
};

static const struct message get_row_count_seq[] = {
    { TCM_GETROWCOUNT, sent|wparam|lparam, 0, 0 },
    { 0 }
};

static const struct message get_item_rect_seq[] = {
    { TCM_GETITEMRECT, sent },
    { TCM_GETITEMRECT, sent },
    { 0 }
};

static const struct message getset_cur_focus_seq[] = {
    { TCM_SETCURFOCUS, sent|lparam, 0 },
    { TCM_GETCURFOCUS, sent|wparam|lparam, 0, 0 },
    { TCM_SETCURFOCUS, sent|lparam, 0 },
    { TCM_GETCURFOCUS, sent|wparam|lparam, 0, 0 },
    { TCM_SETCURSEL, sent|lparam, 0 },
    { TCM_SETCURFOCUS, sent|lparam, 0 },
    { TCM_GETCURFOCUS, sent|wparam|lparam, 0, 0 },
    { 0 }
};

static const struct message getset_cur_sel_seq[] = {
    { TCM_SETCURSEL, sent|lparam, 0 },
    { TCM_GETCURSEL, sent|wparam|lparam, 0, 0 },
    { TCM_GETCURFOCUS, sent|wparam|lparam, 0, 0 },
    { TCM_SETCURSEL, sent|lparam, 0 },
    { TCM_GETCURSEL, sent|wparam|lparam, 0, 0 },
    { TCM_SETCURSEL, sent|lparam, 0 },
    { TCM_SETCURSEL, sent|lparam, 0 },
    { TCM_GETCURFOCUS, sent|wparam|lparam, 0, 0 },
    { 0 }
};

static const struct message getset_extended_style_seq[] = {
    { TCM_GETEXTENDEDSTYLE, sent|wparam|lparam, 0, 0 },
    { TCM_SETEXTENDEDSTYLE, sent },
    { TCM_GETEXTENDEDSTYLE, sent|wparam|lparam, 0, 0 },
    { TCM_SETEXTENDEDSTYLE, sent },
    { TCM_GETEXTENDEDSTYLE, sent|wparam|lparam, 0, 0 },
    { 0 }
};

static const struct message getset_unicode_format_seq[] = {
    { CCM_SETUNICODEFORMAT, sent|lparam, 0 },
    { CCM_GETUNICODEFORMAT, sent|wparam|lparam, 0, 0 },
    { CCM_SETUNICODEFORMAT, sent|lparam, 0 },
    { CCM_GETUNICODEFORMAT, sent|wparam|lparam, 0, 0 },
    { CCM_SETUNICODEFORMAT, sent|lparam, 0 },
    { 0 }
};

static const struct message getset_item_seq[] = {
    { TCM_SETITEMA, sent },
    { TCM_GETITEMA, sent },
    { TCM_GETITEMA, sent },
    { 0 }
};

static const struct message getset_tooltip_seq[] = {
    { WM_NOTIFYFORMAT, sent|optional },
    { WM_QUERYUISTATE, sent|wparam|lparam|optional, 0, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam, 0 },
    { WM_NOTIFYFORMAT, sent|optional },
    { TCM_SETTOOLTIPS, sent|lparam, 0 },
    { TCM_GETTOOLTIPS, sent|wparam|lparam, 0, 0 },
    { TCM_SETTOOLTIPS, sent|lparam, 0 },
    { TCM_GETTOOLTIPS, sent|wparam|lparam, 0, 0 },
    { 0 }
};

static const struct message getset_tooltip_parent_seq[] = {
    { WM_WINDOWPOSCHANGING, sent|wparam, 0 },
    { 0 }
};

static const struct message insert_focus_seq[] = {
    { TCM_GETITEMCOUNT, sent|wparam|lparam, 0, 0 },
    { TCM_GETCURFOCUS, sent|wparam|lparam, 0, 0 },
    { TCM_INSERTITEMA, sent|wparam, 1 },
    { WM_NOTIFYFORMAT, sent|defwinproc|optional },
    { WM_QUERYUISTATE, sent|defwinproc|optional },
    { WM_PARENTNOTIFY, sent|defwinproc|optional },
    { TCM_GETITEMCOUNT, sent|wparam|lparam, 0, 0 },
    { TCM_GETCURFOCUS, sent|wparam|lparam, 0, 0 },
    { TCM_INSERTITEMA, sent|wparam, 2 },
    { WM_NOTIFYFORMAT, sent|defwinproc|optional },
    { WM_QUERYUISTATE, sent|defwinproc|optional, },
    { WM_PARENTNOTIFY, sent|defwinproc|optional },
    { TCM_GETITEMCOUNT, sent|wparam|lparam, 0, 0 },
    { TCM_GETCURFOCUS, sent|wparam|lparam, 0, 0 },
    { TCM_SETCURFOCUS, sent|wparam|lparam, -1, 0 },
    { TCM_GETCURFOCUS, sent|wparam|lparam, 0, 0 },
    { TCM_INSERTITEMA, sent|wparam, 3 },
    { TCM_GETCURFOCUS, sent|wparam|lparam, 0, 0 },
    { 0 }
};

static const struct message delete_focus_seq[] = {
    { TCM_GETITEMCOUNT, sent|wparam|lparam, 0, 0 },
    { TCM_GETCURFOCUS, sent|wparam|lparam, 0, 0 },
    { TCM_DELETEITEM, sent|wparam|lparam, 1, 0 },
    { TCM_GETITEMCOUNT, sent|wparam|lparam, 0, 0 },
    { TCM_GETCURFOCUS, sent|wparam|lparam, 0, 0 },
    { TCM_SETCURFOCUS, sent|wparam|lparam, -1, 0 },
    { TCM_GETCURFOCUS, sent|wparam|lparam, 0, 0 },
    { TCM_DELETEITEM, sent|wparam|lparam, 0, 0 },
    { TCM_GETITEMCOUNT, sent|wparam|lparam, 0, 0 },
    { TCM_GETCURFOCUS, sent|wparam|lparam, 0, 0 },
    { 0 }
};

static const struct message rbuttonup_seq[] = {
    { WM_RBUTTONUP, sent|wparam|lparam, 0, 0 },
    { WM_CONTEXTMENU, sent|defwinproc },
    { 0 }
};

static HWND
create_tabcontrol (DWORD style, DWORD mask)
{
    HWND handle;
    TCITEMA tcNewTab;
    static char text1[] = "Tab 1",
    text2[] = "Wide Tab 2",
    text3[] = "T 3";

    handle = CreateWindowA(WC_TABCONTROLA, "TestTab",
            WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TCS_FOCUSNEVER | style, 10, 10, 300, 100, NULL,
            NULL, NULL, 0);
    ok(handle != NULL, "failed to create tab wnd\n");

    SetWindowLongA(handle, GWL_STYLE, WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TCS_FOCUSNEVER | style);
    SendMessageA(handle, WM_SETFONT, 0, (LPARAM)hFont);

    tcNewTab.mask = mask;
    tcNewTab.pszText = text1;
    tcNewTab.iImage = 0;
    SendMessageA(handle, TCM_INSERTITEMA, 0, (LPARAM)&tcNewTab);
    tcNewTab.pszText = text2;
    tcNewTab.iImage = 1;
    SendMessageA(handle, TCM_INSERTITEMA, 1, (LPARAM)&tcNewTab);
    tcNewTab.pszText = text3;
    tcNewTab.iImage = 2;
    SendMessageA(handle, TCM_INSERTITEMA, 2, (LPARAM)&tcNewTab);

    if (winetest_interactive)
    {
        ShowWindow (handle, SW_SHOW);
        RedrawWindow (handle, NULL, 0, RDW_UPDATENOW);
        Sleep (1000);
    }

    return handle;
}

static LRESULT WINAPI parentWindowProcess(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static LONG defwndproc_counter = 0;
    LRESULT ret;
    struct message msg;

    /* do not log painting messages */
    if (message != WM_PAINT &&
        message != WM_ERASEBKGND &&
        message != WM_NCPAINT &&
        message != WM_NCHITTEST &&
        message != WM_GETTEXT &&
        message != WM_GETICON &&
        message != WM_DEVICECHANGE)
    {
        msg.message = message;
        msg.flags = sent|wparam|lparam;
        if (defwndproc_counter) msg.flags |= defwinproc;
        msg.wParam = wParam;
        msg.lParam = lParam;
        msg.id = 0;
        add_message(sequences, PARENT_SEQ_INDEX, &msg);
    }

    /* dump sent structure data */
    if (message == WM_DRAWITEM)
        g_drawitem = *(DRAWITEMSTRUCT*)lParam;

    defwndproc_counter++;
    ret = DefWindowProcA(hwnd, message, wParam, lParam);
    defwndproc_counter--;

    return ret;
}

static BOOL registerParentWindowClass(void)
{
    WNDCLASSA cls;

    cls.style = 0;
    cls.lpfnWndProc = parentWindowProcess;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(NULL);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "Tab test parent class";
    return RegisterClassA(&cls);
}

static HWND createParentWindow(void)
{
    if (!registerParentWindowClass())
        return NULL;

    return CreateWindowExA(0, "Tab test parent class", "Tab test parent window",
            WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE, 0, 0, 100, 100,
            GetDesktopWindow(), NULL, GetModuleHandleA(NULL), NULL);
}

static LRESULT WINAPI tabSubclassProcess(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WNDPROC oldproc = (WNDPROC)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    static LONG defwndproc_counter = 0;
    LRESULT ret;
    struct message msg;

    /* do not log painting messages */
    if (message != WM_PAINT &&
        message != WM_ERASEBKGND &&
        message != WM_NCPAINT &&
        message != WM_NCHITTEST &&
        message != WM_GETTEXT &&
        message != WM_GETICON &&
        message != WM_DEVICECHANGE)
    {
        msg.message = message;
        msg.flags = sent|wparam|lparam;
        if (defwndproc_counter) msg.flags |= defwinproc;
        msg.wParam = wParam;
        msg.lParam = lParam;
        msg.id = 0;
        add_message(sequences, TAB_SEQ_INDEX, &msg);
    }

    defwndproc_counter++;
    ret = CallWindowProcA(oldproc, hwnd, message, wParam, lParam);
    defwndproc_counter--;

    return ret;
}

static HWND createFilledTabControl(HWND parent_wnd, DWORD style, DWORD mask, INT nTabs)
{
    HWND tabHandle;
    TCITEMA tcNewTab;
    WNDPROC oldproc;
    RECT rect;
    INT i;

    GetClientRect(parent_wnd, &rect);

    tabHandle = CreateWindowA(WC_TABCONTROLA, "TestTab",
            WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TCS_FOCUSNEVER | style, 0, 0, rect.right,
            rect.bottom, parent_wnd, NULL, NULL, 0);
    ok(tabHandle != NULL, "failed to create tab wnd\n");

    oldproc = (WNDPROC)SetWindowLongPtrA(tabHandle, GWLP_WNDPROC, (LONG_PTR)tabSubclassProcess);
    SetWindowLongPtrA(tabHandle, GWLP_USERDATA, (LONG_PTR)oldproc);

    tcNewTab.mask = mask;

    for (i = 0; i < nTabs; i++)
    {
        char tabName[MAX_TABLEN];

        sprintf(tabName, "Tab %d", i+1);
        tcNewTab.pszText = tabName;
        tcNewTab.iImage = i;
        SendMessageA(tabHandle, TCM_INSERTITEMA, i, (LPARAM)&tcNewTab);
    }

    if (winetest_interactive)
    {
        ShowWindow (tabHandle, SW_SHOW);
        RedrawWindow (tabHandle, NULL, 0, RDW_UPDATENOW);
        Sleep (1000);
    }

    return tabHandle;
}

static HWND create_tooltip (HWND hTab, char toolTipText[])
{
    HWND hwndTT;

    TTTOOLINFOA ti;
    LPSTR lptstr = toolTipText;
    RECT rect;

    /* Creating a tooltip window*/
    hwndTT = CreateWindowExA(WS_EX_TOPMOST, TOOLTIPS_CLASSA, NULL,
            WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            CW_USEDEFAULT, hTab, NULL, 0, NULL);

    SetWindowPos(
        hwndTT,
        HWND_TOPMOST,
        0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    GetClientRect (hTab, &rect);

    /* Initialize members of toolinfo*/
    ti.cbSize = sizeof(TTTOOLINFOA);
    ti.uFlags = TTF_SUBCLASS;
    ti.hwnd = hTab;
    ti.hinst = 0;
    ti.uId = 0;
    ti.lpszText = lptstr;

    ti.rect = rect;

    /* Add toolinfo structure to the tooltip control */
    SendMessageA(hwndTT, TTM_ADDTOOLA, 0, (LPARAM)&ti);

    return hwndTT;
}

static void test_tab(INT nMinTabWidth)
{
    HWND hwTab;
    RECT rTab;
    HIMAGELIST himl = ImageList_Create(21, 21, ILC_COLOR, 3, 4);
    SIZE size;
    HDC hdc;
    HFONT hOldFont;
    INT i, dpi, exp;

    hwTab = create_tabcontrol(TCS_FIXEDWIDTH, TCIF_TEXT|TCIF_IMAGE);
    SendMessageA(hwTab, TCM_SETMINTABWIDTH, 0, nMinTabWidth);
    /* Get System default MinTabWidth */
    if (nMinTabWidth < 0)
        nMinTabWidth = SendMessageA(hwTab, TCM_SETMINTABWIDTH, 0, nMinTabWidth);

    hdc = GetDC(hwTab);
    dpi = GetDeviceCaps(hdc, LOGPIXELSX);
    hOldFont = SelectObject(hdc, (HFONT)SendMessageA(hwTab, WM_GETFONT, 0, 0));
    GetTextExtentPoint32A(hdc, "Tab 1", strlen("Tab 1"), &size);
    trace("Tab1 text size: size.cx=%d size.cy=%d\n", size.cx, size.cy);
    SelectObject(hdc, hOldFont);
    ReleaseDC(hwTab, hdc);

    trace ("  TCS_FIXEDWIDTH tabs no icon...\n");
    CHECKSIZE(hwTab, dpi, -1, "default width");
    TABCHECKSETSIZE(hwTab, 50, 20, 50, 20, "set size");
    TABCHECKSETSIZE(hwTab, 0, 1, 0, 1, "min size");

    SendMessageA(hwTab, TCM_SETIMAGELIST, 0, (LPARAM)himl);

    trace ("  TCS_FIXEDWIDTH tabs with icon...\n");
    TABCHECKSETSIZE(hwTab, 50, 30, 50, 30, "set size > icon");
    TABCHECKSETSIZE(hwTab, 20, 20, 25, 20, "set size < icon");
    TABCHECKSETSIZE(hwTab, 0, 1, 25, 1, "min size");

    DestroyWindow (hwTab);

    hwTab = create_tabcontrol(TCS_FIXEDWIDTH | TCS_BUTTONS, TCIF_TEXT|TCIF_IMAGE);
    SendMessageA(hwTab, TCM_SETMINTABWIDTH, 0, nMinTabWidth);

    hdc = GetDC(hwTab);
    dpi = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(hwTab, hdc);
    trace ("  TCS_FIXEDWIDTH buttons no icon...\n");
    CHECKSIZE(hwTab, dpi, -1, "default width");
    TABCHECKSETSIZE(hwTab, 20, 20, 20, 20, "set size 1");
    TABCHECKSETSIZE(hwTab, 10, 50, 10, 50, "set size 2");
    TABCHECKSETSIZE(hwTab, 0, 1, 0, 1, "min size");

    SendMessageA(hwTab, TCM_SETIMAGELIST, 0, (LPARAM)himl);

    trace ("  TCS_FIXEDWIDTH buttons with icon...\n");
    TABCHECKSETSIZE(hwTab, 50, 30, 50, 30, "set size > icon");
    TABCHECKSETSIZE(hwTab, 20, 20, 25, 20, "set size < icon");
    TABCHECKSETSIZE(hwTab, 0, 1, 25, 1, "min size");
    SendMessageA(hwTab, TCM_SETPADDING, 0, MAKELPARAM(4, 4));
    TABCHECKSETSIZE(hwTab, 0, 1, 25, 1, "set padding, min size");

    DestroyWindow (hwTab);

    hwTab = create_tabcontrol(TCS_FIXEDWIDTH | TCS_BOTTOM, TCIF_TEXT|TCIF_IMAGE);
    SendMessageA(hwTab, TCM_SETMINTABWIDTH, 0, nMinTabWidth);

    hdc = GetDC(hwTab);
    dpi = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(hwTab, hdc);
    trace ("  TCS_FIXEDWIDTH | TCS_BOTTOM tabs...\n");
    CHECKSIZE(hwTab, dpi, -1, "no icon, default width");

    TABCHECKSETSIZE(hwTab, 20, 20, 20, 20, "no icon, set size 1");
    TABCHECKSETSIZE(hwTab, 10, 50, 10, 50, "no icon, set size 2");
    TABCHECKSETSIZE(hwTab, 0, 1, 0, 1, "no icon, min size");

    SendMessageA(hwTab, TCM_SETIMAGELIST, 0, (LPARAM)himl);

    TABCHECKSETSIZE(hwTab, 50, 30, 50, 30, "with icon, set size > icon");
    TABCHECKSETSIZE(hwTab, 20, 20, 25, 20, "with icon, set size < icon");
    TABCHECKSETSIZE(hwTab, 0, 1, 25, 1, "with icon, min size");
    SendMessageA(hwTab, TCM_SETPADDING, 0, MAKELPARAM(4, 4));
    TABCHECKSETSIZE(hwTab, 0, 1, 25, 1, "set padding, min size");

    DestroyWindow (hwTab);

    hwTab = create_tabcontrol(0, TCIF_TEXT|TCIF_IMAGE);
    SendMessageA(hwTab, TCM_SETMINTABWIDTH, 0, nMinTabWidth);

    trace ("  non fixed width, with text...\n");
    exp = max(size.cx +TAB_PADDING_X*2, (nMinTabWidth < 0) ? DEFAULT_MIN_TAB_WIDTH : nMinTabWidth);
    SendMessageA( hwTab, TCM_GETITEMRECT, 0, (LPARAM)&rTab );
    ok( rTab.right  - rTab.left == exp || broken(rTab.right  - rTab.left == DEFAULT_MIN_TAB_WIDTH),
        "no icon, default width: Expected width [%d] got [%d]\n", exp, rTab.right - rTab.left );

    for (i=0; i<8; i++)
    {
        INT nTabWidth = (nMinTabWidth < 0) ? TabWidthPadded(i, 2) : nMinTabWidth;

        SendMessageA(hwTab, TCM_SETIMAGELIST, 0, 0);
        SendMessageA(hwTab, TCM_SETPADDING, 0, MAKELPARAM(i, i));

        TABCHECKSETSIZE(hwTab, 50, 20, max(size.cx + i*2, nTabWidth), 20, "no icon, set size");
        TABCHECKSETSIZE(hwTab, 0, 1, max(size.cx + i*2, nTabWidth), 1, "no icon, min size");

        SendMessageA(hwTab, TCM_SETIMAGELIST, 0, (LPARAM)himl);
        nTabWidth = (nMinTabWidth < 0) ? TabWidthPadded(i, 3) : nMinTabWidth;

        TABCHECKSETSIZE(hwTab, 50, 30, max(size.cx + 21 + i*3, nTabWidth), 30, "with icon, set size > icon");
        TABCHECKSETSIZE(hwTab, 20, 20, max(size.cx + 21 + i*3, nTabWidth), 20, "with icon, set size < icon");
        TABCHECKSETSIZE(hwTab, 0, 1, max(size.cx + 21 + i*3, nTabWidth), 1, "with icon, min size");
    }
    DestroyWindow (hwTab);

    hwTab = create_tabcontrol(0, TCIF_IMAGE);
    SendMessageA(hwTab, TCM_SETMINTABWIDTH, 0, nMinTabWidth);

    trace ("  non fixed width, no text...\n");
    exp = (nMinTabWidth < 0) ? DEFAULT_MIN_TAB_WIDTH : nMinTabWidth;
    SendMessageA( hwTab, TCM_GETITEMRECT, 0, (LPARAM)&rTab );
    ok( rTab.right  - rTab.left == exp || broken(rTab.right  - rTab.left == DEFAULT_MIN_TAB_WIDTH),
        "no icon, default width: Expected width [%d] got [%d]\n", exp, rTab.right - rTab.left );

    for (i=0; i<8; i++)
    {
        INT nTabWidth = (nMinTabWidth < 0) ? TabWidthPadded(i, 2) : nMinTabWidth;

        SendMessageA(hwTab, TCM_SETIMAGELIST, 0, 0);
        SendMessageA(hwTab, TCM_SETPADDING, 0, MAKELPARAM(i, i));

        TABCHECKSETSIZE(hwTab, 50, 20, nTabWidth, 20, "no icon, set size");
        TABCHECKSETSIZE(hwTab, 0, 1, nTabWidth, 1, "no icon, min size");

        SendMessageA(hwTab, TCM_SETIMAGELIST, 0, (LPARAM)himl);
        if (i > 1 && nMinTabWidth > 0 && nMinTabWidth < DEFAULT_MIN_TAB_WIDTH)
            nTabWidth += EXTRA_ICON_PADDING *(i-1);

        TABCHECKSETSIZE(hwTab, 50, 30, nTabWidth, 30, "with icon, set size > icon");
        TABCHECKSETSIZE(hwTab, 20, 20, nTabWidth, 20, "with icon, set size < icon");
        TABCHECKSETSIZE(hwTab, 0, 1, nTabWidth, 1, "with icon, min size");
    }

    DestroyWindow (hwTab);

    ImageList_Destroy(himl);
}

static void test_width(void)
{
    trace ("Testing with default MinWidth\n");
    test_tab(-1);
    trace ("Testing with MinWidth set to -3\n");
    test_tab(-3);
    trace ("Testing with MinWidth set to 24\n");
    test_tab(24);
    trace ("Testing with MinWidth set to 54\n");
    test_tab(54);
    trace ("Testing with MinWidth set to 94\n");
    test_tab(94);
}

static void test_curfocus(void)
{
    const INT nTabs = 5;
    INT focusIndex;
    HWND hTab;

    hTab = createFilledTabControl(parent_wnd, TCS_FIXEDWIDTH, TCIF_TEXT|TCIF_IMAGE, nTabs);
    ok(hTab != NULL, "Failed to create tab control\n");

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    /* Testing CurFocus with largest appropriate value */
    SendMessageA(hTab, TCM_SETCURFOCUS, nTabs - 1, 0);
    focusIndex = SendMessageA(hTab, TCM_GETCURFOCUS, 0, 0);
    expect(nTabs-1, focusIndex);

    /* Testing CurFocus with negative value */
    SendMessageA(hTab, TCM_SETCURFOCUS, -10, 0);
    focusIndex = SendMessageA(hTab, TCM_GETCURFOCUS, 0, 0);
    expect(-1, focusIndex);

    /* Testing CurFocus with value larger than number of tabs */
    focusIndex = SendMessageA(hTab, TCM_SETCURSEL, 1, 0);
    expect(-1, focusIndex);

    SendMessageA(hTab, TCM_SETCURFOCUS, nTabs + 1, 0);
    focusIndex = SendMessageA(hTab, TCM_GETCURFOCUS, 0, 0);
    expect(1, focusIndex);

    ok_sequence(sequences, TAB_SEQ_INDEX, getset_cur_focus_seq, "Getset curFoc test sequence", FALSE);

    DestroyWindow(hTab);
}

static void test_cursel(void)
{
    const INT nTabs = 5;
    INT selectionIndex;
    INT focusIndex;
    TCITEMA tcItem;
    HWND hTab;

    hTab = createFilledTabControl(parent_wnd, TCS_FIXEDWIDTH, TCIF_TEXT|TCIF_IMAGE, nTabs);
    ok(hTab != NULL, "Failed to create tab control\n");

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    /* Testing CurSel with largest appropriate value */
    selectionIndex = SendMessageA(hTab, TCM_SETCURSEL, nTabs - 1, 0);
    expect(0, selectionIndex);
    selectionIndex = SendMessageA(hTab, TCM_GETCURSEL, 0, 0);
    expect(nTabs-1, selectionIndex);

    /* Focus should switch with selection */
    focusIndex = SendMessageA(hTab, TCM_GETCURFOCUS, 0, 0);
    expect(nTabs-1, focusIndex);

    /* Testing CurSel with negative value */
    SendMessageA(hTab, TCM_SETCURSEL, -10, 0);
    selectionIndex = SendMessageA(hTab, TCM_GETCURSEL, 0, 0);
    expect(-1, selectionIndex);

    /* Testing CurSel with value larger than number of tabs */
    selectionIndex = SendMessageA(hTab, TCM_SETCURSEL, 1, 0);
    expect(-1, selectionIndex);

    selectionIndex = SendMessageA(hTab, TCM_SETCURSEL, nTabs + 1, 0);
    expect(-1, selectionIndex);
    selectionIndex = SendMessageA(hTab, TCM_GETCURFOCUS, 0, 0);
    expect(1, selectionIndex);

    ok_sequence(sequences, TAB_SEQ_INDEX, getset_cur_sel_seq, "Getset curSel test sequence", FALSE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_sequence, "Getset curSel test parent sequence", FALSE);

    /* selected item should have TCIS_BUTTONPRESSED state
       It doesn't depend on button state */
    memset(&tcItem, 0, sizeof(TCITEMA));
    tcItem.mask = TCIF_STATE;
    tcItem.dwStateMask = TCIS_BUTTONPRESSED;
    selectionIndex = SendMessageA(hTab, TCM_GETCURSEL, 0, 0);
    SendMessageA(hTab, TCM_GETITEMA, selectionIndex, (LPARAM)&tcItem);
    ok (tcItem.dwState & TCIS_BUTTONPRESSED || broken(tcItem.dwState == 0), /* older comctl32 */
        "Selected item should have TCIS_BUTTONPRESSED\n");

    /* now deselect all and check previously selected item state */
    focusIndex = SendMessageA(hTab, TCM_GETCURFOCUS, 0, 0);
    ok(focusIndex == 1, "got %d\n", focusIndex);

    selectionIndex = SendMessageA(hTab, TCM_SETCURSEL, -1, 0);
    ok(selectionIndex == 1, "got %d\n", selectionIndex);

    memset(&tcItem, 0, sizeof(TCITEMA));

    /* focus is reset too */
    focusIndex = SendMessageA(hTab, TCM_GETCURFOCUS, 0, 0);
    ok(focusIndex == -1, "got %d\n", focusIndex);

    tcItem.mask = TCIF_STATE;
    tcItem.dwStateMask = TCIS_BUTTONPRESSED;
    SendMessageA(hTab, TCM_GETITEMA, selectionIndex, (LPARAM)&tcItem);
    ok(tcItem.dwState == 0, "got state %d\n", tcItem.dwState);

    DestroyWindow(hTab);
}

static void test_extendedstyle(void)
{
    const INT nTabs = 5;
    DWORD prevExtendedStyle;
    DWORD extendedStyle;
    HWND hTab;

    hTab = createFilledTabControl(parent_wnd, TCS_FIXEDWIDTH, TCIF_TEXT|TCIF_IMAGE, nTabs);
    ok(hTab != NULL, "Failed to create tab control\n");

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    /* Testing Flat Separators */
    extendedStyle = SendMessageA(hTab, TCM_GETEXTENDEDSTYLE, 0, 0);
    prevExtendedStyle = SendMessageA(hTab, TCM_SETEXTENDEDSTYLE, 0, TCS_EX_FLATSEPARATORS);
    expect(extendedStyle, prevExtendedStyle);

    extendedStyle = SendMessageA(hTab, TCM_GETEXTENDEDSTYLE, 0, 0);
    expect(TCS_EX_FLATSEPARATORS, extendedStyle);

    /* Testing Register Drop */
    prevExtendedStyle = SendMessageA(hTab, TCM_SETEXTENDEDSTYLE, 0, TCS_EX_REGISTERDROP);
    expect(extendedStyle, prevExtendedStyle);

    extendedStyle = SendMessageA(hTab, TCM_GETEXTENDEDSTYLE, 0, 0);
    todo_wine{
        expect(TCS_EX_REGISTERDROP, extendedStyle);
    }

    ok_sequence(sequences, TAB_SEQ_INDEX, getset_extended_style_seq, "Getset extendedStyle test sequence", FALSE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_sequence, "Getset extendedStyle test parent sequence", FALSE);

    DestroyWindow(hTab);
}

static void test_unicodeformat(void)
{
    const INT nTabs = 5;
    INT unicodeFormat;
    HWND hTab;

    hTab = createFilledTabControl(parent_wnd, TCS_FIXEDWIDTH, TCIF_TEXT|TCIF_IMAGE, nTabs);
    ok(hTab != NULL, "Failed to create tab control\n");

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    unicodeFormat = SendMessageA(hTab, TCM_SETUNICODEFORMAT, TRUE, 0);
    todo_wine{
        expect(0, unicodeFormat);
    }
    unicodeFormat = SendMessageA(hTab, TCM_GETUNICODEFORMAT, 0, 0);
    expect(1, unicodeFormat);

    unicodeFormat = SendMessageA(hTab, TCM_SETUNICODEFORMAT, FALSE, 0);
    expect(1, unicodeFormat);
    unicodeFormat = SendMessageA(hTab, TCM_GETUNICODEFORMAT, 0, 0);
    expect(0, unicodeFormat);

    unicodeFormat = SendMessageA(hTab, TCM_SETUNICODEFORMAT, TRUE, 0);
    expect(0, unicodeFormat);

    ok_sequence(sequences, TAB_SEQ_INDEX, getset_unicode_format_seq, "Getset unicodeFormat test sequence", FALSE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_sequence, "Getset unicodeFormat test parent sequence", FALSE);

    DestroyWindow(hTab);
}

static void test_getset_item(void)
{
    char szText[32] = "New Label";
    const INT nTabs = 5;
    TCITEMA tcItem;
    LPARAM lparam;
    DWORD ret;
    HWND hTab;

    hTab = CreateWindowA(
	WC_TABCONTROLA,
	"TestTab",
	WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TCS_FOCUSNEVER | TCS_FIXEDWIDTH | TCS_OWNERDRAWFIXED,
        10, 10, 300, 100,
        parent_wnd, NULL, NULL, 0);

    ok(GetParent(hTab) == NULL, "got %p, expected null parent\n", GetParent(hTab));

    ret = SendMessageA(hTab, TCM_SETITEMEXTRA, sizeof(LPARAM)-1, 0);
    ok(ret == TRUE, "got %d\n", ret);

    /* set some item data */
    tcItem.lParam = ~0;
    tcItem.mask = TCIF_PARAM;

    ret = SendMessageA(hTab, TCM_INSERTITEMA, 0, (LPARAM)&tcItem);
    ok(ret == 0, "got %d\n", ret);

    /* all sizeof(LPARAM) returned anyway when using sizeof(LPARAM)-1 size */
    memset(&lparam, 0xaa, sizeof(lparam));
    tcItem.lParam = lparam;
    tcItem.mask = TCIF_PARAM;
    ret = SendMessageA(hTab, TCM_GETITEMA, 0, (LPARAM)&tcItem);
    expect(TRUE, ret);
    /* everything higher specified size is preserved */
    memset(&lparam, 0xff, sizeof(lparam)-1);
    ok(tcItem.lParam == lparam, "Expected 0x%lx, got 0x%lx\n", lparam, tcItem.lParam);

    DestroyWindow(hTab);

    hTab = createFilledTabControl(parent_wnd, TCS_FIXEDWIDTH, TCIF_TEXT|TCIF_IMAGE, nTabs);
    ok(hTab != NULL, "Failed to create tab control\n");

    /* passing invalid index should result in initialization to zero
       for members mentioned in mask requested */

    /* valid range here is [0,4] */
    memset(&tcItem, 0xcc, sizeof(tcItem));
    tcItem.mask = TCIF_PARAM;
    ret = SendMessageA(hTab, TCM_GETITEMA, 5, (LPARAM)&tcItem);
    expect(FALSE, ret);
    ok(tcItem.lParam == 0, "Expected zero lParam, got %lu\n", tcItem.lParam);

    memset(&tcItem, 0xcc, sizeof(tcItem));
    tcItem.mask = TCIF_IMAGE;
    ret = SendMessageA(hTab, TCM_GETITEMA, 5, (LPARAM)&tcItem);
    expect(FALSE, ret);
    expect(0, tcItem.iImage);

    memset(&tcItem, 0xcc, sizeof(tcItem));
    tcItem.mask = TCIF_TEXT;
    tcItem.pszText = szText;
    szText[0] = 'a';
    ret = SendMessageA(hTab, TCM_GETITEMA, 5, (LPARAM)&tcItem);
    expect(FALSE, ret);
    expect('a', szText[0]);

    memset(&tcItem, 0xcc, sizeof(tcItem));
    tcItem.mask = TCIF_STATE;
    tcItem.dwStateMask = 0;
    tcItem.dwState = TCIS_BUTTONPRESSED;
    ret = SendMessageA(hTab, TCM_GETITEMA, 5, (LPARAM)&tcItem);
    expect(FALSE, ret);
    ok(tcItem.dwState == 0, "Expected zero dwState, got %u\n", tcItem.dwState);

    memset(&tcItem, 0xcc, sizeof(tcItem));
    tcItem.mask = TCIF_STATE;
    tcItem.dwStateMask = TCIS_BUTTONPRESSED;
    tcItem.dwState = TCIS_BUTTONPRESSED;
    ret = SendMessageA(hTab, TCM_GETITEMA, 5, (LPARAM)&tcItem);
    expect(FALSE, ret);
    ok(tcItem.dwState == 0, "Expected zero dwState\n");

    /* check with negative index to be sure */
    memset(&tcItem, 0xcc, sizeof(tcItem));
    tcItem.mask = TCIF_PARAM;
    ret = SendMessageA(hTab, TCM_GETITEMA, -1, (LPARAM)&tcItem);
    expect(FALSE, ret);
    ok(tcItem.lParam == 0, "Expected zero lParam, got %lu\n", tcItem.lParam);

    memset(&tcItem, 0xcc, sizeof(tcItem));
    tcItem.mask = TCIF_PARAM;
    ret = SendMessageA(hTab, TCM_GETITEMA, -2, (LPARAM)&tcItem);
    expect(FALSE, ret);
    ok(tcItem.lParam == 0, "Expected zero lParam, got %lu\n", tcItem.lParam);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    tcItem.mask = TCIF_TEXT;
    tcItem.pszText = &szText[0];
    tcItem.cchTextMax = sizeof(szText);

    strcpy(szText, "New Label");
    ok(SendMessageA(hTab, TCM_SETITEMA, 0, (LPARAM)&tcItem), "Setting new item failed.\n");
    ok(SendMessageA(hTab, TCM_GETITEMA, 0, (LPARAM)&tcItem), "Getting item failed.\n");
    expect_str("New Label", tcItem.pszText);

    ok(SendMessageA(hTab, TCM_GETITEMA, 1, (LPARAM)&tcItem), "Getting item failed.\n");
    expect_str("Tab 2", tcItem.pszText);

    ok_sequence(sequences, TAB_SEQ_INDEX, getset_item_seq, "Getset item test sequence", FALSE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_sequence, "Getset item test parent sequence", FALSE);

    /* TCIS_BUTTONPRESSED doesn't depend on tab style */
    memset(&tcItem, 0, sizeof(tcItem));
    tcItem.mask = TCIF_STATE;
    tcItem.dwStateMask = TCIS_BUTTONPRESSED;
    tcItem.dwState = TCIS_BUTTONPRESSED;
    ok(SendMessageA(hTab, TCM_SETITEMA, 0, (LPARAM)&tcItem), "Setting new item failed.\n");
    tcItem.dwState = 0;
    ok(SendMessageA(hTab, TCM_GETITEMA, 0, (LPARAM)&tcItem), "Getting item failed.\n");
    if (tcItem.dwState)
    {
        ok (tcItem.dwState == TCIS_BUTTONPRESSED, "TCIS_BUTTONPRESSED should be set.\n");
        /* next highlight item, test that dwStateMask actually masks */
        tcItem.mask = TCIF_STATE;
        tcItem.dwStateMask = TCIS_HIGHLIGHTED;
        tcItem.dwState = TCIS_HIGHLIGHTED;
        ok(SendMessageA(hTab, TCM_SETITEMA, 0, (LPARAM)&tcItem), "Setting new item failed.\n");
        tcItem.dwState = 0;
        ok(SendMessageA(hTab, TCM_GETITEMA, 0, (LPARAM)&tcItem), "Getting item failed.\n");
        ok (tcItem.dwState == TCIS_HIGHLIGHTED, "TCIS_HIGHLIGHTED should be set.\n");
        tcItem.mask = TCIF_STATE;
        tcItem.dwStateMask = TCIS_BUTTONPRESSED;
        tcItem.dwState = 0;
        ok(SendMessageA(hTab, TCM_GETITEMA, 0, (LPARAM)&tcItem), "Getting item failed.\n");
        ok (tcItem.dwState == TCIS_BUTTONPRESSED, "TCIS_BUTTONPRESSED should be set.\n");
    }
    else win_skip( "Item state mask not supported\n" );

    DestroyWindow(hTab);
}

static void test_getset_tooltips(void)
{
    char toolTipText[32] = "ToolTip Text Test";
    const INT nTabs = 5;
    HWND hTab, toolTip;

    hTab = createFilledTabControl(parent_wnd, TCS_FIXEDWIDTH, TCIF_TEXT|TCIF_IMAGE, nTabs);
    ok(hTab != NULL, "Failed to create tab control\n");

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    toolTip = create_tooltip(hTab, toolTipText);
    SendMessageA(hTab, TCM_SETTOOLTIPS, (LPARAM)toolTip, 0);
    ok(toolTip == (HWND)SendMessageA(hTab, TCM_GETTOOLTIPS, 0,0), "ToolTip was set incorrectly.\n");

    SendMessageA(hTab, TCM_SETTOOLTIPS, 0, 0);
    ok(!SendMessageA(hTab, TCM_GETTOOLTIPS, 0,0), "ToolTip was set incorrectly.\n");

    ok_sequence(sequences, TAB_SEQ_INDEX, getset_tooltip_seq, "Getset tooltip test sequence", TRUE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, getset_tooltip_parent_seq, "Getset tooltip test parent sequence", TRUE);

    DestroyWindow(hTab);
}

static void test_misc(void)
{
    const INT nTabs = 5;
    HWND hTab;
    RECT rTab;
    INT nTabsRetrieved;
    INT rowCount;
    INT dpi;
    HDC hdc;

    ok(parent_wnd != NULL, "no parent window!\n");
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    hTab = createFilledTabControl(parent_wnd, TCS_FIXEDWIDTH, TCIF_TEXT|TCIF_IMAGE, nTabs);
    ok(hTab != NULL, "Failed to create tab control\n");

    if(!winetest_interactive)
        ok_sequence(sequences, TAB_SEQ_INDEX, add_tab_to_parent,
                    "Tab sequence, after adding tab control to parent", TRUE);
    else
        ok_sequence(sequences, TAB_SEQ_INDEX, add_tab_to_parent_interactive,
                    "Tab sequence, after adding tab control to parent", TRUE);

    if(!winetest_interactive)
        ok_sequence(sequences, PARENT_SEQ_INDEX, add_tab_control_parent_seq,
                    "Parent after sequence, adding tab control to parent", TRUE);
    else
        ok_sequence(sequences, PARENT_SEQ_INDEX, add_tab_control_parent_seq_interactive,
                    "Parent after sequence, adding tab control to parent", TRUE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    ok(SendMessageA(hTab, TCM_SETMINTABWIDTH, 0, -1) > 0, "TCM_SETMINTABWIDTH returned < 0\n");
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_sequence, "Set minTabWidth test parent sequence", FALSE);

    /* Testing GetItemCount */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    nTabsRetrieved = SendMessageA(hTab, TCM_GETITEMCOUNT, 0, 0);
    expect(nTabs, nTabsRetrieved);
    ok_sequence(sequences, TAB_SEQ_INDEX, get_item_count_seq, "Get itemCount test sequence", FALSE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_sequence, "Getset itemCount test parent sequence", FALSE);

    /* Testing GetRowCount */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    rowCount = SendMessageA(hTab, TCM_GETROWCOUNT, 0, 0);
    expect(1, rowCount);
    ok_sequence(sequences, TAB_SEQ_INDEX, get_row_count_seq, "Get rowCount test sequence", FALSE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_sequence, "Get rowCount test parent sequence", FALSE);

    /* Testing GetItemRect */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    ok(SendMessageA(hTab, TCM_GETITEMRECT, 0, (LPARAM)&rTab), "GetItemRect failed.\n");

    hdc = GetDC(hTab);
    dpi = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(hTab, hdc);
    CHECKSIZE(hTab, dpi, -1 , "Default Width");
    ok_sequence(sequences, TAB_SEQ_INDEX, get_item_rect_seq, "Get itemRect test sequence", FALSE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_sequence, "Get itemRect test parent sequence", FALSE);

    DestroyWindow(hTab);
}

static void test_adjustrect(void)
{
    HWND hTab;
    INT r;

    ok(parent_wnd != NULL, "no parent window!\n");

    hTab = createFilledTabControl(parent_wnd, TCS_FIXEDWIDTH, 0, 0);
    ok(hTab != NULL, "Failed to create tab control\n");

    r = SendMessageA(hTab, TCM_ADJUSTRECT, FALSE, 0);
    expect(-1, r);

    r = SendMessageA(hTab, TCM_ADJUSTRECT, TRUE, 0);
    expect(-1, r);
}

static void test_insert_focus(void)
{
    HWND hTab;
    INT nTabsRetrieved;
    INT r;
    TCITEMA tcNewTab;
    DWORD mask = TCIF_TEXT|TCIF_IMAGE;
    static char tabName[] = "TAB";
    tcNewTab.mask = mask;
    tcNewTab.pszText = tabName;

    ok(parent_wnd != NULL, "no parent window!\n");

    hTab = createFilledTabControl(parent_wnd, TCS_FIXEDWIDTH, mask, 0);
    ok(hTab != NULL, "Failed to create tab control\n");

    r = SendMessageA(hTab, TCM_GETCURSEL, 0, 0);
    expect(-1, r);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    nTabsRetrieved = SendMessageA(hTab, TCM_GETITEMCOUNT, 0, 0);
    expect(0, nTabsRetrieved);

    r = SendMessageA(hTab, TCM_GETCURFOCUS, 0, 0);
    expect(-1, r);

    tcNewTab.iImage = 1;
    r = SendMessageA(hTab, TCM_INSERTITEMA, 1, (LPARAM)&tcNewTab);
    expect(0, r);

    nTabsRetrieved = SendMessageA(hTab, TCM_GETITEMCOUNT, 0, 0);
    expect(1, nTabsRetrieved);

    r = SendMessageA(hTab, TCM_GETCURFOCUS, 0, 0);
    expect(0, r);

    tcNewTab.iImage = 2;
    r = SendMessageA(hTab, TCM_INSERTITEMA, 2, (LPARAM)&tcNewTab);
    expect(1, r);

    nTabsRetrieved = SendMessageA(hTab, TCM_GETITEMCOUNT, 0, 0);
    expect(2, nTabsRetrieved);

    r = SendMessageA(hTab, TCM_GETCURFOCUS, 0, 0);
    expect(0, r);

    r = SendMessageA(hTab, TCM_SETCURFOCUS, -1, 0);
    expect(0, r);

    r = SendMessageA(hTab, TCM_GETCURFOCUS, 0, 0);
    expect(-1, r);

    tcNewTab.iImage = 3;
    r = SendMessageA(hTab, TCM_INSERTITEMA, 3, (LPARAM)&tcNewTab);
    expect(2, r);

    r = SendMessageA(hTab, TCM_GETCURFOCUS, 0, 0);
    expect(2, r);

    ok_sequence(sequences, TAB_SEQ_INDEX, insert_focus_seq, "insert_focus test sequence", FALSE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_sequence, "insert_focus parent test sequence", TRUE);

    DestroyWindow(hTab);
}

static void test_delete_focus(void)
{
    HWND hTab;
    INT nTabsRetrieved;
    INT r;

    ok(parent_wnd != NULL, "no parent window!\n");

    hTab = createFilledTabControl(parent_wnd, TCS_FIXEDWIDTH, TCIF_TEXT|TCIF_IMAGE, 2);
    ok(hTab != NULL, "Failed to create tab control\n");

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    nTabsRetrieved = SendMessageA(hTab, TCM_GETITEMCOUNT, 0, 0);
    expect(2, nTabsRetrieved);

    r = SendMessageA(hTab, TCM_GETCURFOCUS, 0, 0);
    expect(0, r);

    r = SendMessageA(hTab, TCM_DELETEITEM, 1, 0);
    expect(1, r);

    nTabsRetrieved = SendMessageA(hTab, TCM_GETITEMCOUNT, 0, 0);
    expect(1, nTabsRetrieved);

    r = SendMessageA(hTab, TCM_GETCURFOCUS, 0, 0);
    expect(0, r);

    r = SendMessageA(hTab, TCM_SETCURFOCUS, -1, 0);
    expect(0, r);

    r = SendMessageA(hTab, TCM_GETCURFOCUS, 0, 0);
    expect(-1, r);

    r = SendMessageA(hTab, TCM_DELETEITEM, 0, 0);
    expect(1, r);

    nTabsRetrieved = SendMessageA(hTab, TCM_GETITEMCOUNT, 0, 0);
    expect(0, nTabsRetrieved);

    r = SendMessageA(hTab, TCM_GETCURFOCUS, 0, 0);
    expect(-1, r);

    ok_sequence(sequences, TAB_SEQ_INDEX, delete_focus_seq, "delete_focus test sequence", FALSE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_sequence, "delete_focus parent test sequence", TRUE);

    DestroyWindow(hTab);
}

static void test_removeimage(void)
{
    static const BYTE bits[32];
    HWND hwTab;
    INT i;
    TCITEMA item;
    HICON hicon;
    HIMAGELIST himl = ImageList_Create(16, 16, ILC_COLOR, 3, 4);

    hicon = CreateIcon(NULL, 16, 16, 1, 1, bits, bits);
    ImageList_AddIcon(himl, hicon);
    ImageList_AddIcon(himl, hicon);
    ImageList_AddIcon(himl, hicon);

    hwTab = create_tabcontrol(TCS_FIXEDWIDTH, TCIF_TEXT|TCIF_IMAGE);
    SendMessageA(hwTab, TCM_SETIMAGELIST, 0, (LPARAM)himl);

    memset(&item, 0, sizeof(TCITEMA));
    item.mask = TCIF_IMAGE;

    for(i = 0; i < 3; i++) {
        SendMessageA(hwTab, TCM_GETITEMA, i, (LPARAM)&item);
        expect(i, item.iImage);
    }

    /* remove image middle image */
    SendMessageA(hwTab, TCM_REMOVEIMAGE, 1, 0);
    expect(2, ImageList_GetImageCount(himl));
    item.iImage = -1;
    SendMessageA(hwTab, TCM_GETITEMA, 0, (LPARAM)&item);
    expect(0, item.iImage);
    item.iImage = 0;
    SendMessageA(hwTab, TCM_GETITEMA, 1, (LPARAM)&item);
    expect(-1, item.iImage);
    item.iImage = 0;
    SendMessageA(hwTab, TCM_GETITEMA, 2, (LPARAM)&item);
    expect(1, item.iImage);
    /* remove first image */
    SendMessageA(hwTab, TCM_REMOVEIMAGE, 0, 0);
    expect(1, ImageList_GetImageCount(himl));
    item.iImage = 0;
    SendMessageA(hwTab, TCM_GETITEMA, 0, (LPARAM)&item);
    expect(-1, item.iImage);
    item.iImage = 0;
    SendMessageA(hwTab, TCM_GETITEMA, 1, (LPARAM)&item);
    expect(-1, item.iImage);
    item.iImage = -1;
    SendMessageA(hwTab, TCM_GETITEMA, 2, (LPARAM)&item);
    expect(0, item.iImage);
    /* remove the last one */
    SendMessageA(hwTab, TCM_REMOVEIMAGE, 0, 0);
    expect(0, ImageList_GetImageCount(himl));
    for(i = 0; i < 3; i++) {
        item.iImage = 0;
        SendMessageA(hwTab, TCM_GETITEMA, i, (LPARAM)&item);
        expect(-1, item.iImage);
    }

    DestroyWindow(hwTab);
    ImageList_Destroy(himl);
    DestroyIcon(hicon);
}

static void test_delete_selection(void)
{
    HWND hTab;
    INT ret;

    hTab = createFilledTabControl(parent_wnd, TCS_FIXEDWIDTH, TCIF_TEXT|TCIF_IMAGE, 4);
    ok(hTab != NULL, "Failed to create tab control\n");

    ret = SendMessageA(hTab, TCM_SETCURSEL, 3, 0);
    expect(0, ret);
    ret = SendMessageA(hTab, TCM_GETCURSEL, 0, 0);
    expect(3, ret);
    /* delete selected item - selection goes to -1 */
    ret = SendMessageA(hTab, TCM_DELETEITEM, 3, 0);
    expect(TRUE, ret);

    ret = SendMessageA(hTab, TCM_GETCURSEL, 0, 0);
    expect(-1, ret);

    DestroyWindow(hTab);
}

static void test_TCM_SETITEMEXTRA(void)
{
    HWND hTab;
    DWORD ret;

    hTab = CreateWindowA(
	WC_TABCONTROLA,
	"TestTab",
	WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TCS_FOCUSNEVER | TCS_FIXEDWIDTH,
        10, 10, 300, 100,
        parent_wnd, NULL, NULL, 0);

    /* zero is valid size too */
    ret = SendMessageA(hTab, TCM_SETITEMEXTRA, 0, 0);
    if (ret == FALSE)
    {
        win_skip("TCM_SETITEMEXTRA not supported\n");
        DestroyWindow(hTab);
        return;
    }

    ret = SendMessageA(hTab, TCM_SETITEMEXTRA, -1, 0);
    ok(ret == FALSE, "got %d\n", ret);

    ret = SendMessageA(hTab, TCM_SETITEMEXTRA, 2, 0);
    ok(ret == TRUE, "got %d\n", ret);
    DestroyWindow(hTab);

    /* it's not possible to change extra data size for control with tabs */
    hTab = createFilledTabControl(parent_wnd, TCS_FIXEDWIDTH, TCIF_TEXT|TCIF_IMAGE, 4);
    ok(hTab != NULL, "Failed to create tab control\n");

    ret = SendMessageA(hTab, TCM_SETITEMEXTRA, 2, 0);
    ok(ret == FALSE, "got %d\n", ret);
    DestroyWindow(hTab);
}

static void test_TCS_OWNERDRAWFIXED(void)
{
    LPARAM lparam;
    ULONG_PTR itemdata, itemdata2;
    TCITEMA item;
    HWND hTab;
    BOOL ret;

    hTab = createFilledTabControl(parent_wnd, TCS_FIXEDWIDTH|TCS_OWNERDRAWFIXED, TCIF_TEXT|TCIF_IMAGE, 4);
    ok(hTab != NULL, "Failed to create tab control\n");

    ok(GetParent(hTab) == NULL, "got %p, expected null parent\n", GetParent(hTab));

    /* set some item data */
    memset(&lparam, 0xde, sizeof(LPARAM));

    item.mask = TCIF_PARAM;
    item.lParam = lparam;
    ret = SendMessageA(hTab, TCM_SETITEMA, 0, (LPARAM)&item);
    ok(ret == TRUE, "got %d\n", ret);

    memset(&g_drawitem, 0, sizeof(g_drawitem));

    ShowWindow(hTab, SW_SHOW);
    RedrawWindow(hTab, NULL, 0, RDW_UPDATENOW);

    itemdata = 0;
    memset(&itemdata, 0xde, 4);
    ok(g_drawitem.itemData == itemdata, "got 0x%lx, expected 0x%lx\n", g_drawitem.itemData, itemdata);

    DestroyWindow(hTab);

    /* now with custom extra data length */
    hTab = CreateWindowA(
	WC_TABCONTROLA,
	"TestTab",
	WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TCS_FOCUSNEVER | TCS_FIXEDWIDTH | TCS_OWNERDRAWFIXED,
        10, 10, 300, 100,
        parent_wnd, NULL, NULL, 0);

    ok(GetParent(hTab) == NULL, "got %p, expected null parent\n", GetParent(hTab));

    ret = SendMessageA(hTab, TCM_SETITEMEXTRA, sizeof(LPARAM)+1, 0);
    ok(ret == TRUE, "got %d\n", ret);

    /* set some item data */
    memset(&lparam, 0xde, sizeof(LPARAM));
    item.mask = TCIF_PARAM;
    item.lParam = lparam;

    ret = SendMessageA(hTab, TCM_INSERTITEMA, 0, (LPARAM)&item);
    ok(ret == 0, "got %d\n", ret);

    memset(&g_drawitem, 0, sizeof(g_drawitem));

    ShowWindow(hTab, SW_SHOW);
    RedrawWindow(hTab, NULL, 0, RDW_UPDATENOW);

    memset(&itemdata, 0xde, sizeof(ULONG_PTR));
    ok(*(ULONG_PTR*)g_drawitem.itemData == itemdata, "got 0x%lx, expected 0x%lx\n", g_drawitem.itemData, itemdata);

    DestroyWindow(hTab);

    /* same thing, but size smaller than default */
    hTab = CreateWindowA(
	WC_TABCONTROLA,
	"TestTab",
	WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TCS_FOCUSNEVER | TCS_FIXEDWIDTH | TCS_OWNERDRAWFIXED,
        10, 10, 300, 100,
        parent_wnd, NULL, NULL, 0);

    ok(GetParent(hTab) == NULL, "got %p, expected null parent\n", GetParent(hTab));

    ret = SendMessageA(hTab, TCM_SETITEMEXTRA, sizeof(LPARAM)-1, 0);
    ok(ret == TRUE, "got %d\n", ret);

    memset(&lparam, 0xde, sizeof(lparam));
    item.mask = TCIF_PARAM;
    item.lParam = lparam;

    ret = SendMessageA(hTab, TCM_INSERTITEMA, 0, (LPARAM)&item);
    ok(ret == 0, "got %d\n", ret);

    memset(&g_drawitem, 0, sizeof(g_drawitem));

    ShowWindow(hTab, SW_SHOW);
    RedrawWindow(hTab, NULL, 0, RDW_UPDATENOW);

    itemdata = itemdata2 = 0;
    memset(&itemdata, 0xde, 4);
    memset(&itemdata2, 0xde, sizeof(LPARAM)-1);
    ok(g_drawitem.itemData == itemdata || broken(g_drawitem.itemData == itemdata2) /* win98 */,
        "got 0x%lx, expected 0x%lx\n", g_drawitem.itemData, itemdata);

    DestroyWindow(hTab);
}

static void test_WM_CONTEXTMENU(void)
{
    HWND hTab;

    hTab = createFilledTabControl(parent_wnd, TCS_FIXEDWIDTH, TCIF_TEXT|TCIF_IMAGE, 4);
    ok(hTab != NULL, "Failed to create tab control\n");

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    SendMessageA(hTab, WM_RBUTTONUP, 0, 0);

    ok_sequence(sequences, TAB_SEQ_INDEX, rbuttonup_seq, "WM_RBUTTONUP response sequence", FALSE);

    DestroyWindow(hTab);
}

struct tabcreate_style {
    DWORD style;
    DWORD act_style;
};

static const struct tabcreate_style create_styles[] =
{
    { WS_CHILD|TCS_BOTTOM|TCS_VERTICAL, WS_CHILD|WS_CLIPSIBLINGS|TCS_BOTTOM|TCS_VERTICAL|TCS_MULTILINE },
    { WS_CHILD|TCS_VERTICAL,            WS_CHILD|WS_CLIPSIBLINGS|TCS_VERTICAL|TCS_MULTILINE },
    { 0 }
};

static void test_create(void)
{
    const struct tabcreate_style *ptr = create_styles;
    DWORD style;
    HWND hTab;

    while (ptr->style)
    {
        hTab = CreateWindowA(WC_TABCONTROLA, "TestTab", ptr->style,
            10, 10, 300, 100, parent_wnd, NULL, NULL, 0);
        style = GetWindowLongA(hTab, GWL_STYLE);
        ok(style == ptr->act_style, "expected style 0x%08x, got style 0x%08x\n", ptr->act_style, style);

        DestroyWindow(hTab);
        ptr++;
    }
}

START_TEST(tab)
{
    LOGFONTA logfont;

    lstrcpyA(logfont.lfFaceName, "Arial");
    memset(&logfont, 0, sizeof(logfont));
    logfont.lfHeight = -12;
    logfont.lfWeight = FW_NORMAL;
    logfont.lfCharSet = ANSI_CHARSET;
    hFont = CreateFontIndirectA(&logfont);

    InitCommonControls();

    test_width();

    init_msg_sequences(sequences, NUM_MSG_SEQUENCES);

    parent_wnd = createParentWindow();
    ok(parent_wnd != NULL, "Failed to create parent window!\n");

    test_curfocus();
    test_cursel();
    test_extendedstyle();
    test_unicodeformat();
    test_getset_item();
    test_getset_tooltips();
    test_misc();

    test_adjustrect();

    test_insert_focus();
    test_delete_focus();
    test_delete_selection();
    test_removeimage();
    test_TCM_SETITEMEXTRA();
    test_TCS_OWNERDRAWFIXED();
    test_WM_CONTEXTMENU();
    test_create();

    DestroyWindow(parent_wnd);
}
