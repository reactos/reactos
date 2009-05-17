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

#include <assert.h>
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>

#include "wine/test.h"
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

#define TabCheckSetSize(hwnd, SetWidth, SetHeight, ExpWidth, ExpHeight, Msg)\
    SendMessage (hwnd, TCM_SETITEMSIZE, 0,\
	(LPARAM) MAKELPARAM((SetWidth >= 0) ? SetWidth:0, (SetHeight >= 0) ? SetHeight:0));\
    if (winetest_interactive) RedrawWindow (hwnd, NULL, 0, RDW_UPDATENOW);\
    CheckSize(hwnd, ExpWidth, ExpHeight, Msg);

#define CheckSize(hwnd,width,height,msg)\
    SendMessage (hwnd, TCM_GETITEMRECT, 0, (LPARAM) &rTab);\
    if ((width  >= 0) && (height < 0))\
	ok (width  == rTab.right  - rTab.left, "%s: Expected width [%d] got [%d]\n",\
        msg, (int)width,  rTab.right  - rTab.left);\
    else if ((height >= 0) && (width  < 0))\
	ok (height == rTab.bottom - rTab.top,  "%s: Expected height [%d] got [%d]\n",\
        msg, (int)height, rTab.bottom - rTab.top);\
    else\
	ok ((width  == rTab.right  - rTab.left) &&\
	    (height == rTab.bottom - rTab.top ),\
	    "%s: Expected [%d,%d] got [%d,%d]\n", msg, (int)width, (int)height,\
            rTab.right - rTab.left, rTab.bottom - rTab.top);

static HFONT hFont = 0;

static struct msg_sequence *sequences[NUM_MSG_SEQUENCES];

static const struct message create_parent_wnd_seq[] = {
    { WM_GETMINMAXINFO, sent },
    { WM_NCCREATE, sent },
    { WM_NCCALCSIZE, sent|wparam, 0 },
    { WM_CREATE, sent },
    { WM_SHOWWINDOW, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGING, sent|wparam, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam, 0 },
    { WM_ACTIVATEAPP, sent|wparam, 1 },
    { WM_NCACTIVATE, sent|wparam, 1 },
    { WM_ACTIVATE, sent|wparam, 1 },
    { WM_IME_SETCONTEXT, sent|wparam|defwinproc|optional, 1 },
    { WM_IME_NOTIFY, sent|defwinproc|optional },
    { WM_SETFOCUS, sent|wparam|defwinproc, 0 },
    /* Win9x adds SWP_NOZORDER below */
    { WM_WINDOWPOSCHANGED, sent},
    { WM_NCCALCSIZE, sent|wparam|optional, 1 },
    { WM_SIZE, sent },
    { WM_MOVE, sent },
    { 0 }
};

static const struct message add_tab_to_parent[] = {
    { TCM_INSERTITEMA, sent },
    { TCM_INSERTITEMA, sent },
    { WM_NOTIFYFORMAT, sent|defwinproc },
    { WM_QUERYUISTATE, sent|wparam|lparam|defwinproc|optional, 0, 0 },
    { WM_PARENTNOTIFY, sent|defwinproc },
    { TCM_INSERTITEMA, sent },
    { TCM_INSERTITEMA, sent },
    { TCM_INSERTITEMA, sent },
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

static const struct message set_min_tab_width_seq[] = {
    { TCM_SETMINTABWIDTH, sent|wparam, 0 },
    { TCM_SETMINTABWIDTH, sent|wparam, 0 },
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
    { TCM_INSERTITEM, sent|wparam, 1 },
    { TCM_GETITEMCOUNT, sent|wparam|lparam, 0, 0 },
    { TCM_GETCURFOCUS, sent|wparam|lparam, 0, 0 },
    { TCM_INSERTITEM, sent|wparam, 2 },
    { WM_NOTIFYFORMAT, sent|defwinproc, },
    { WM_QUERYUISTATE, sent|defwinproc|optional, },
    { WM_PARENTNOTIFY, sent|defwinproc, },
    { TCM_GETITEMCOUNT, sent|wparam|lparam, 0, 0 },
    { TCM_GETCURFOCUS, sent|wparam|lparam, 0, 0 },
    { TCM_SETCURFOCUS, sent|wparam|lparam, -1, 0 },
    { TCM_GETCURFOCUS, sent|wparam|lparam, 0, 0 },
    { TCM_INSERTITEM, sent|wparam, 3 },
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


static HWND
create_tabcontrol (DWORD style, DWORD mask)
{
    HWND handle;
    TCITEM tcNewTab;
    static char text1[] = "Tab 1",
    text2[] = "Wide Tab 2",
    text3[] = "T 3";

    handle = CreateWindow (
	WC_TABCONTROLA,
	"TestTab",
	WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TCS_FOCUSNEVER | style,
        10, 10, 300, 100,
        NULL, NULL, NULL, 0);

    assert (handle);

    SetWindowLong(handle, GWL_STYLE, WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TCS_FOCUSNEVER | style);
    SendMessage (handle, WM_SETFONT, 0, (LPARAM) hFont);

    tcNewTab.mask = mask;
    tcNewTab.pszText = text1;
    tcNewTab.iImage = 0;
    SendMessage (handle, TCM_INSERTITEM, 0, (LPARAM) &tcNewTab);
    tcNewTab.pszText = text2;
    tcNewTab.iImage = 1;
    SendMessage (handle, TCM_INSERTITEM, 1, (LPARAM) &tcNewTab);
    tcNewTab.pszText = text3;
    tcNewTab.iImage = 2;
    SendMessage (handle, TCM_INSERTITEM, 2, (LPARAM) &tcNewTab);

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
        trace("parent: %p, %04x, %08lx, %08lx\n", hwnd, message, wParam, lParam);

        msg.message = message;
        msg.flags = sent|wparam|lparam;
        if (defwndproc_counter) msg.flags |= defwinproc;
        msg.wParam = wParam;
        msg.lParam = lParam;
        add_message(sequences, PARENT_SEQ_INDEX, &msg);
    }

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
    cls.hCursor = LoadCursorA(0, IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "Tab test parent class";
    return RegisterClassA(&cls);
}

static HWND createParentWindow(void)
{
    if (!registerParentWindowClass())
        return NULL;

    return CreateWindowEx(0, "Tab test parent class",
                          "Tab test parent window",
                          WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX |
                          WS_MAXIMIZEBOX | WS_VISIBLE,
                          0, 0, 100, 100,
                          GetDesktopWindow(), NULL, GetModuleHandleA(NULL), NULL);
}

struct subclass_info
{
    WNDPROC oldproc;
};

static LRESULT WINAPI tabSubclassProcess(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    struct subclass_info *info = (struct subclass_info *)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
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
        trace("tab: %p, %04x, %08lx, %08lx\n", hwnd, message, wParam, lParam);

        msg.message = message;
        msg.flags = sent|wparam|lparam;
        if (defwndproc_counter) msg.flags |= defwinproc;
        msg.wParam = wParam;
        msg.lParam = lParam;
        add_message(sequences, TAB_SEQ_INDEX, &msg);
    }

    defwndproc_counter++;
    ret = CallWindowProcA(info->oldproc, hwnd, message, wParam, lParam);
    defwndproc_counter--;

    return ret;
}

static HWND createFilledTabControl(HWND parent_wnd, DWORD style, DWORD mask, INT nTabs)
{
    HWND tabHandle;
    TCITEM tcNewTab;
    struct subclass_info *info;
    RECT rect;
    INT i;

    info = HeapAlloc(GetProcessHeap(), 0, sizeof(struct subclass_info));
    if (!info)
        return NULL;

    GetClientRect(parent_wnd, &rect);

    tabHandle = CreateWindow (
        WC_TABCONTROLA,
        "TestTab",
        WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TCS_FOCUSNEVER | style,
        0, 0, rect.right, rect.bottom,
        parent_wnd, NULL, NULL, 0);

    assert(tabHandle);

    info->oldproc = (WNDPROC)SetWindowLongPtrA(tabHandle, GWLP_WNDPROC, (LONG_PTR)tabSubclassProcess);
    SetWindowLongPtrA(tabHandle, GWLP_USERDATA, (LONG_PTR)info);

    tcNewTab.mask = mask;

    for (i = 0; i < nTabs; i++)
    {
        char tabName[MAX_TABLEN];

        sprintf(tabName, "Tab %d", i+1);
        tcNewTab.pszText = tabName;
        tcNewTab.iImage = i;
        SendMessage (tabHandle, TCM_INSERTITEM, i, (LPARAM) &tcNewTab);
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

    TOOLINFO ti;
    LPTSTR lptstr = toolTipText;
    RECT rect;

    /* Creating a tooltip window*/
    hwndTT = CreateWindowEx(
        WS_EX_TOPMOST,
        TOOLTIPS_CLASS,
        NULL,
        WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        hTab, NULL, 0, NULL);

    SetWindowPos(
        hwndTT,
        HWND_TOPMOST,
        0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    GetClientRect (hTab, &rect);

    /* Initialize members of toolinfo*/
    ti.cbSize = sizeof(TOOLINFO);
    ti.uFlags = TTF_SUBCLASS;
    ti.hwnd = hTab;
    ti.hinst = 0;
    ti.uId = 0;
    ti.lpszText = lptstr;

    ti.rect = rect;

    /* Add toolinfo structure to the tooltip control */
    SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM) &ti);

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
    INT i, dpi;

    hwTab = create_tabcontrol(TCS_FIXEDWIDTH, TCIF_TEXT|TCIF_IMAGE);
    SendMessage(hwTab, TCM_SETMINTABWIDTH, 0, nMinTabWidth);
    /* Get System default MinTabWidth */
    if (nMinTabWidth < 0)
        nMinTabWidth = SendMessage(hwTab, TCM_SETMINTABWIDTH, 0, nMinTabWidth);

    hdc = GetDC(hwTab);
    dpi = GetDeviceCaps(hdc, LOGPIXELSX);
    hOldFont = SelectObject(hdc, (HFONT)SendMessage(hwTab, WM_GETFONT, 0, 0));
    GetTextExtentPoint32A(hdc, "Tab 1", strlen("Tab 1"), &size);
    trace("Tab1 text size: size.cx=%d size.cy=%d\n", size.cx, size.cy);
    SelectObject(hdc, hOldFont);
    ReleaseDC(hwTab, hdc);

    trace ("  TCS_FIXEDWIDTH tabs no icon...\n");
    CheckSize(hwTab, dpi, -1, "default width");
    TabCheckSetSize(hwTab, 50, 20, 50, 20, "set size");
    TabCheckSetSize(hwTab, 0, 1, 0, 1, "min size");

    SendMessage(hwTab, TCM_SETIMAGELIST, 0, (LPARAM)himl);

    trace ("  TCS_FIXEDWIDTH tabs with icon...\n");
    TabCheckSetSize(hwTab, 50, 30, 50, 30, "set size > icon");
    TabCheckSetSize(hwTab, 20, 20, 25, 20, "set size < icon");
    TabCheckSetSize(hwTab, 0, 1, 25, 1, "min size");

    DestroyWindow (hwTab);

    hwTab = create_tabcontrol(TCS_FIXEDWIDTH | TCS_BUTTONS, TCIF_TEXT|TCIF_IMAGE);
    SendMessage(hwTab, TCM_SETMINTABWIDTH, 0, nMinTabWidth);

    hdc = GetDC(hwTab);
    dpi = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(hwTab, hdc);
    trace ("  TCS_FIXEDWIDTH buttons no icon...\n");
    CheckSize(hwTab, dpi, -1, "default width");
    TabCheckSetSize(hwTab, 20, 20, 20, 20, "set size 1");
    TabCheckSetSize(hwTab, 10, 50, 10, 50, "set size 2");
    TabCheckSetSize(hwTab, 0, 1, 0, 1, "min size");

    SendMessage(hwTab, TCM_SETIMAGELIST, 0, (LPARAM)himl);

    trace ("  TCS_FIXEDWIDTH buttons with icon...\n");
    TabCheckSetSize(hwTab, 50, 30, 50, 30, "set size > icon");
    TabCheckSetSize(hwTab, 20, 20, 25, 20, "set size < icon");
    TabCheckSetSize(hwTab, 0, 1, 25, 1, "min size");
    SendMessage(hwTab, TCM_SETPADDING, 0, MAKELPARAM(4,4));
    TabCheckSetSize(hwTab, 0, 1, 25, 1, "set padding, min size");

    DestroyWindow (hwTab);

    hwTab = create_tabcontrol(TCS_FIXEDWIDTH | TCS_BOTTOM, TCIF_TEXT|TCIF_IMAGE);
    SendMessage(hwTab, TCM_SETMINTABWIDTH, 0, nMinTabWidth);

    hdc = GetDC(hwTab);
    dpi = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(hwTab, hdc);
    trace ("  TCS_FIXEDWIDTH | TCS_BOTTOM tabs...\n");
    CheckSize(hwTab, dpi, -1, "no icon, default width");

    TabCheckSetSize(hwTab, 20, 20, 20, 20, "no icon, set size 1");
    TabCheckSetSize(hwTab, 10, 50, 10, 50, "no icon, set size 2");
    TabCheckSetSize(hwTab, 0, 1, 0, 1, "no icon, min size");

    SendMessage(hwTab, TCM_SETIMAGELIST, 0, (LPARAM)himl);

    TabCheckSetSize(hwTab, 50, 30, 50, 30, "with icon, set size > icon");
    TabCheckSetSize(hwTab, 20, 20, 25, 20, "with icon, set size < icon");
    TabCheckSetSize(hwTab, 0, 1, 25, 1, "with icon, min size");
    SendMessage(hwTab, TCM_SETPADDING, 0, MAKELPARAM(4,4));
    TabCheckSetSize(hwTab, 0, 1, 25, 1, "set padding, min size");

    DestroyWindow (hwTab);

    hwTab = create_tabcontrol(0, TCIF_TEXT|TCIF_IMAGE);
    SendMessage(hwTab, TCM_SETMINTABWIDTH, 0, nMinTabWidth);

    trace ("  non fixed width, with text...\n");
    CheckSize(hwTab, max(size.cx +TAB_PADDING_X*2, (nMinTabWidth < 0) ? DEFAULT_MIN_TAB_WIDTH : nMinTabWidth), -1,
              "no icon, default width");
    for (i=0; i<8; i++)
    {
        INT nTabWidth = (nMinTabWidth < 0) ? TabWidthPadded(i, 2) : nMinTabWidth;

        SendMessage(hwTab, TCM_SETIMAGELIST, 0, 0);
        SendMessage(hwTab, TCM_SETPADDING, 0, MAKELPARAM(i,i));

        TabCheckSetSize(hwTab, 50, 20, max(size.cx + i*2, nTabWidth), 20, "no icon, set size");
        TabCheckSetSize(hwTab, 0, 1, max(size.cx + i*2, nTabWidth), 1, "no icon, min size");

        SendMessage(hwTab, TCM_SETIMAGELIST, 0, (LPARAM)himl);
        nTabWidth = (nMinTabWidth < 0) ? TabWidthPadded(i, 3) : nMinTabWidth;

        TabCheckSetSize(hwTab, 50, 30, max(size.cx + 21 + i*3, nTabWidth), 30, "with icon, set size > icon");
        TabCheckSetSize(hwTab, 20, 20, max(size.cx + 21 + i*3, nTabWidth), 20, "with icon, set size < icon");
        TabCheckSetSize(hwTab, 0, 1, max(size.cx + 21 + i*3, nTabWidth), 1, "with icon, min size");
    }
    DestroyWindow (hwTab);

    hwTab = create_tabcontrol(0, TCIF_IMAGE);
    SendMessage(hwTab, TCM_SETMINTABWIDTH, 0, nMinTabWidth);

    trace ("  non fixed width, no text...\n");
    CheckSize(hwTab, (nMinTabWidth < 0) ? DEFAULT_MIN_TAB_WIDTH : nMinTabWidth, -1, "no icon, default width");
    for (i=0; i<8; i++)
    {
        INT nTabWidth = (nMinTabWidth < 0) ? TabWidthPadded(i, 2) : nMinTabWidth;

        SendMessage(hwTab, TCM_SETIMAGELIST, 0, 0);
        SendMessage(hwTab, TCM_SETPADDING, 0, MAKELPARAM(i,i));

        TabCheckSetSize(hwTab, 50, 20, nTabWidth, 20, "no icon, set size");
        TabCheckSetSize(hwTab, 0, 1, nTabWidth, 1, "no icon, min size");

        SendMessage(hwTab, TCM_SETIMAGELIST, 0, (LPARAM)himl);
        if (i > 1 && nMinTabWidth > 0 && nMinTabWidth < DEFAULT_MIN_TAB_WIDTH)
            nTabWidth += EXTRA_ICON_PADDING *(i-1);

        TabCheckSetSize(hwTab, 50, 30, nTabWidth, 30, "with icon, set size > icon");
        TabCheckSetSize(hwTab, 20, 20, nTabWidth, 20, "with icon, set size < icon");
        TabCheckSetSize(hwTab, 0, 1, nTabWidth, 1, "with icon, min size");
    }

    DestroyWindow (hwTab);

    ImageList_Destroy(himl);
    DeleteObject(hFont);
}

static void test_getters_setters(HWND parent_wnd, INT nTabs)
{
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
    ok(SendMessage(hTab, TCM_SETMINTABWIDTH, 0, -1) > 0,"TCM_SETMINTABWIDTH returned < 0\n");
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_sequence, "Set minTabWidth test parent sequence", FALSE);

    /* Testing GetItemCount */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    nTabsRetrieved = SendMessage(hTab, TCM_GETITEMCOUNT, 0, 0);
    expect(nTabs, nTabsRetrieved);
    ok_sequence(sequences, TAB_SEQ_INDEX, get_item_count_seq, "Get itemCount test sequence", FALSE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_sequence, "Getset itemCount test parent sequence", FALSE);

    /* Testing GetRowCount */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    rowCount = SendMessage(hTab, TCM_GETROWCOUNT, 0, 0);
    expect(1, rowCount);
    ok_sequence(sequences, TAB_SEQ_INDEX, get_row_count_seq, "Get rowCount test sequence", FALSE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_sequence, "Get rowCount test parent sequence", FALSE);

    /* Testing GetItemRect */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    ok(SendMessage(hTab, TCM_GETITEMRECT, 0, (LPARAM) &rTab), "GetItemRect failed.\n");

    hdc = GetDC(hTab);
    dpi = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(hTab, hdc);
    CheckSize(hTab, dpi, -1 , "Default Width");
    ok_sequence(sequences, TAB_SEQ_INDEX, get_item_rect_seq, "Get itemRect test sequence", FALSE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_sequence, "Get itemRect test parent sequence", FALSE);

    /* Testing CurFocus */
    {
        INT focusIndex;

        flush_sequences(sequences, NUM_MSG_SEQUENCES);

        /* Testing CurFocus with largest appropriate value */
        SendMessage(hTab, TCM_SETCURFOCUS, nTabs-1, 0);
        focusIndex = SendMessage(hTab, TCM_GETCURFOCUS, 0, 0);
            expect(nTabs-1, focusIndex);

        /* Testing CurFocus with negative value */
        SendMessage(hTab, TCM_SETCURFOCUS, -10, 0);
        focusIndex = SendMessage(hTab, TCM_GETCURFOCUS, 0, 0);
            expect(-1, focusIndex);

        /* Testing CurFocus with value larger than number of tabs */
        focusIndex = SendMessage(hTab, TCM_SETCURSEL, 1, 0);
        todo_wine{
            expect(-1, focusIndex);
        }

        SendMessage(hTab, TCM_SETCURFOCUS, nTabs+1, 0);
        focusIndex = SendMessage(hTab, TCM_GETCURFOCUS, 0, 0);
            expect(1, focusIndex);

        ok_sequence(sequences, TAB_SEQ_INDEX, getset_cur_focus_seq, "Getset curFoc test sequence", FALSE);
    }

    /* Testing CurSel */
    {
        INT selectionIndex;
        INT focusIndex;
        TCITEM tcItem;

        flush_sequences(sequences, NUM_MSG_SEQUENCES);

        /* Testing CurSel with largest appropriate value */
        selectionIndex = SendMessage(hTab, TCM_SETCURSEL, nTabs-1, 0);
            expect(1, selectionIndex);
        selectionIndex = SendMessage(hTab, TCM_GETCURSEL, 0, 0);
            expect(nTabs-1, selectionIndex);

        /* Focus should switch with selection */
        focusIndex = SendMessage(hTab, TCM_GETCURFOCUS, 0, 0);
            expect(nTabs-1, focusIndex);

        /* Testing CurSel with negative value */
        SendMessage(hTab, TCM_SETCURSEL, -10, 0);
        selectionIndex = SendMessage(hTab, TCM_GETCURSEL, 0, 0);
            expect(-1, selectionIndex);

        /* Testing CurSel with value larger than number of tabs */
        selectionIndex = SendMessage(hTab, TCM_SETCURSEL, 1, 0);
            expect(-1, selectionIndex);

        selectionIndex = SendMessage(hTab, TCM_SETCURSEL, nTabs+1, 0);
            expect(-1, selectionIndex);
        selectionIndex = SendMessage(hTab, TCM_GETCURFOCUS, 0, 0);
            expect(1, selectionIndex);

        ok_sequence(sequences, TAB_SEQ_INDEX, getset_cur_sel_seq, "Getset curSel test sequence", FALSE);
        ok_sequence(sequences, PARENT_SEQ_INDEX, empty_sequence, "Getset curSel test parent sequence", FALSE);

        /* selected item should have TCIS_BUTTONPRESSED state
           It doesn't depend on button state */
        memset(&tcItem, 0, sizeof(TCITEM));
        tcItem.mask = TCIF_STATE;
        tcItem.dwStateMask = TCIS_BUTTONPRESSED;
        selectionIndex = SendMessage(hTab, TCM_GETCURSEL, 0, 0);
        SendMessage(hTab, TCM_GETITEM, selectionIndex, (LPARAM) &tcItem);
        ok (tcItem.dwState & TCIS_BUTTONPRESSED, "Selected item should have TCIS_BUTTONPRESSED\n");
    }

    /* Testing ExtendedStyle */
    {
        DWORD prevExtendedStyle;
        DWORD extendedStyle;

        flush_sequences(sequences, NUM_MSG_SEQUENCES);

        /* Testing Flat Separators */
        extendedStyle = SendMessage(hTab, TCM_GETEXTENDEDSTYLE, 0, 0);
        prevExtendedStyle = SendMessage(hTab, TCM_SETEXTENDEDSTYLE, 0, TCS_EX_FLATSEPARATORS);
        expect(extendedStyle, prevExtendedStyle);

        extendedStyle = SendMessage(hTab, TCM_GETEXTENDEDSTYLE, 0, 0);
        expect(TCS_EX_FLATSEPARATORS, extendedStyle);

        /* Testing Register Drop */
        prevExtendedStyle = SendMessage(hTab, TCM_SETEXTENDEDSTYLE, 0, TCS_EX_REGISTERDROP);
            expect(extendedStyle, prevExtendedStyle);

        extendedStyle = SendMessage(hTab, TCM_GETEXTENDEDSTYLE, 0, 0);
        todo_wine{
            expect(TCS_EX_REGISTERDROP, extendedStyle);
        }

        ok_sequence(sequences, TAB_SEQ_INDEX, getset_extended_style_seq, "Getset extendedStyle test sequence", FALSE);
        ok_sequence(sequences, PARENT_SEQ_INDEX, empty_sequence, "Getset extendedStyle test parent sequence", FALSE);
    }

    /* Testing UnicodeFormat */
    {
        INT unicodeFormat;

        flush_sequences(sequences, NUM_MSG_SEQUENCES);

        unicodeFormat = SendMessage(hTab, TCM_SETUNICODEFORMAT, TRUE, 0);
        todo_wine{
            expect(0, unicodeFormat);
        }
        unicodeFormat = SendMessage(hTab, TCM_GETUNICODEFORMAT, 0, 0);
            expect(1, unicodeFormat);

        unicodeFormat = SendMessage(hTab, TCM_SETUNICODEFORMAT, FALSE, 0);
            expect(1, unicodeFormat);
        unicodeFormat = SendMessage(hTab, TCM_GETUNICODEFORMAT, 0, 0);
            expect(0, unicodeFormat);

        unicodeFormat = SendMessage(hTab, TCM_SETUNICODEFORMAT, TRUE, 0);
            expect(0, unicodeFormat);

        ok_sequence(sequences, TAB_SEQ_INDEX, getset_unicode_format_seq, "Getset unicodeFormat test sequence", FALSE);
        ok_sequence(sequences, PARENT_SEQ_INDEX, empty_sequence, "Getset unicodeFormat test parent sequence", FALSE);
    }

    /* Testing GetSet Item */
    {
        TCITEM tcItem;
        char szText[32] = "New Label";

        flush_sequences(sequences, NUM_MSG_SEQUENCES);

        tcItem.mask = TCIF_TEXT;
        tcItem.pszText = &szText[0];
        tcItem.cchTextMax = sizeof(szText);

        ok ( SendMessage(hTab, TCM_SETITEM, 0, (LPARAM) &tcItem), "Setting new item failed.\n");
        ok ( SendMessage(hTab, TCM_GETITEM, 0, (LPARAM) &tcItem), "Getting item failed.\n");
        expect_str("New Label", tcItem.pszText);

        ok ( SendMessage(hTab, TCM_GETITEM, 1, (LPARAM) &tcItem), "Getting item failed.\n");
        expect_str("Tab 2", tcItem.pszText);

        ok_sequence(sequences, TAB_SEQ_INDEX, getset_item_seq, "Getset item test sequence", FALSE);
        ok_sequence(sequences, PARENT_SEQ_INDEX, empty_sequence, "Getset item test parent sequence", FALSE);

        /* TCIS_BUTTONPRESSED doesn't depend on tab style */
        memset(&tcItem, 0, sizeof(tcItem));
        tcItem.mask = TCIF_STATE;
        tcItem.dwStateMask = TCIS_BUTTONPRESSED;
        tcItem.dwState = TCIS_BUTTONPRESSED;
        ok ( SendMessage(hTab, TCM_SETITEM, 0, (LPARAM) &tcItem), "Setting new item failed.\n");
        tcItem.dwState = 0;
        ok ( SendMessage(hTab, TCM_GETITEM, 0, (LPARAM) &tcItem), "Getting item failed.\n");
        ok (tcItem.dwState == TCIS_BUTTONPRESSED, "TCIS_BUTTONPRESSED should be set.\n");
        /* next highlight item, test that dwStateMask actually masks */
        tcItem.mask = TCIF_STATE;
        tcItem.dwStateMask = TCIS_HIGHLIGHTED;
        tcItem.dwState = TCIS_HIGHLIGHTED;
        ok ( SendMessage(hTab, TCM_SETITEM, 0, (LPARAM) &tcItem), "Setting new item failed.\n");
        tcItem.dwState = 0;
        ok ( SendMessage(hTab, TCM_GETITEM, 0, (LPARAM) &tcItem), "Getting item failed.\n");
        ok (tcItem.dwState == TCIS_HIGHLIGHTED, "TCIS_HIGHLIGHTED should be set.\n");
        tcItem.mask = TCIF_STATE;
        tcItem.dwStateMask = TCIS_BUTTONPRESSED;
        tcItem.dwState = 0;
        ok ( SendMessage(hTab, TCM_GETITEM, 0, (LPARAM) &tcItem), "Getting item failed.\n");
        ok (tcItem.dwState == TCIS_BUTTONPRESSED, "TCIS_BUTTONPRESSED should be set.\n");
    }

    /* Testing GetSet ToolTip */
    {
        HWND toolTip;
        char toolTipText[32] = "ToolTip Text Test";

        flush_sequences(sequences, NUM_MSG_SEQUENCES);

        toolTip = create_tooltip(hTab, toolTipText);
        SendMessage(hTab, TCM_SETTOOLTIPS, (LPARAM) toolTip, 0);
        ok (toolTip == (HWND) SendMessage(hTab,TCM_GETTOOLTIPS,0,0), "ToolTip was set incorrectly.\n");

        SendMessage(hTab, TCM_SETTOOLTIPS, 0, 0);
        ok (NULL  == (HWND) SendMessage(hTab,TCM_GETTOOLTIPS,0,0), "ToolTip was set incorrectly.\n");

        ok_sequence(sequences, TAB_SEQ_INDEX, getset_tooltip_seq, "Getset tooltip test sequence", TRUE);
        ok_sequence(sequences, PARENT_SEQ_INDEX, getset_tooltip_parent_seq, "Getset tooltip test parent sequence", TRUE);
    }

    DestroyWindow(hTab);
}

static void test_adjustrect(HWND parent_wnd)
{
    HWND hTab;
    INT r;

    ok(parent_wnd != NULL, "no parent window!\n");

    hTab = createFilledTabControl(parent_wnd, TCS_FIXEDWIDTH, 0, 0);
    ok(hTab != NULL, "Failed to create tab control\n");

    r = SendMessage(hTab, TCM_ADJUSTRECT, FALSE, 0);
    expect(-1, r);

    r = SendMessage(hTab, TCM_ADJUSTRECT, TRUE, 0);
    expect(-1, r);
}
static void test_insert_focus(HWND parent_wnd)
{
    HWND hTab;
    INT nTabsRetrieved;
    INT r;
    TCITEM tcNewTab;
    DWORD mask = TCIF_TEXT|TCIF_IMAGE;
    static char tabName[] = "TAB";
    tcNewTab.mask = mask;
    tcNewTab.pszText = tabName;

    ok(parent_wnd != NULL, "no parent window!\n");

    hTab = createFilledTabControl(parent_wnd, TCS_FIXEDWIDTH, mask, 0);
    ok(hTab != NULL, "Failed to create tab control\n");

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    nTabsRetrieved = SendMessage(hTab, TCM_GETITEMCOUNT, 0, 0);
    expect(0, nTabsRetrieved);

    r = SendMessage(hTab, TCM_GETCURFOCUS, 0, 0);
    expect(-1, r);

    tcNewTab.iImage = 1;
    r = SendMessage(hTab, TCM_INSERTITEM, 1, (LPARAM) &tcNewTab);
    expect(0, r);

    nTabsRetrieved = SendMessage(hTab, TCM_GETITEMCOUNT, 0, 0);
    expect(1, nTabsRetrieved);

    r = SendMessage(hTab, TCM_GETCURFOCUS, 0, 0);
    expect(0, r);

    tcNewTab.iImage = 2;
    r = SendMessage(hTab, TCM_INSERTITEM, 2, (LPARAM) &tcNewTab);
    expect(1, r);

    nTabsRetrieved = SendMessage(hTab, TCM_GETITEMCOUNT, 0, 0);
    expect(2, nTabsRetrieved);

    r = SendMessage(hTab, TCM_GETCURFOCUS, 0, 0);
    expect(0, r);

    r = SendMessage(hTab, TCM_SETCURFOCUS, -1, 0);
    expect(0, r);

    r = SendMessage(hTab, TCM_GETCURFOCUS, 0, 0);
    expect(-1, r);

    tcNewTab.iImage = 3;
    r = SendMessage(hTab, TCM_INSERTITEM, 3, (LPARAM) &tcNewTab);
    expect(2, r);

    r = SendMessage(hTab, TCM_GETCURFOCUS, 0, 0);
    expect(2, r);

    ok_sequence(sequences, TAB_SEQ_INDEX, insert_focus_seq, "insert_focus test sequence", TRUE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_sequence, "insert_focus parent test sequence", FALSE);

    DestroyWindow(hTab);
}

static void test_delete_focus(HWND parent_wnd)
{
    HWND hTab;
    INT nTabsRetrieved;
    INT r;

    ok(parent_wnd != NULL, "no parent window!\n");

    hTab = createFilledTabControl(parent_wnd, TCS_FIXEDWIDTH, TCIF_TEXT|TCIF_IMAGE, 2);
    ok(hTab != NULL, "Failed to create tab control\n");

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    nTabsRetrieved = SendMessage(hTab, TCM_GETITEMCOUNT, 0, 0);
    expect(2, nTabsRetrieved);

    r = SendMessage(hTab, TCM_GETCURFOCUS, 0, 0);
    expect(0, r);

    r = SendMessage(hTab, TCM_DELETEITEM, 1, 0);
    expect(1, r);

    nTabsRetrieved = SendMessage(hTab, TCM_GETITEMCOUNT, 0, 0);
    expect(1, nTabsRetrieved);

    r = SendMessage(hTab, TCM_GETCURFOCUS, 0, 0);
    expect(0, r);

    r = SendMessage(hTab, TCM_SETCURFOCUS, -1, 0);
    expect(0, r);

    r = SendMessage(hTab, TCM_GETCURFOCUS, 0, 0);
    expect(-1, r);

    r = SendMessage(hTab, TCM_DELETEITEM, 0, 0);
    expect(1, r);

    nTabsRetrieved = SendMessage(hTab, TCM_GETITEMCOUNT, 0, 0);
    expect(0, nTabsRetrieved);

    r = SendMessage(hTab, TCM_GETCURFOCUS, 0, 0);
    expect(-1, r);

    ok_sequence(sequences, TAB_SEQ_INDEX, delete_focus_seq, "delete_focus test sequence", FALSE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_sequence, "delete_focus parent test sequence", FALSE);

    DestroyWindow(hTab);
}

static void test_removeimage(HWND parent_wnd)
{
    static const BYTE bits[32];
    HWND hwTab;
    INT i;
    TCITEM item;
    HICON hicon;
    HIMAGELIST himl = ImageList_Create(16, 16, ILC_COLOR, 3, 4);

    hicon = CreateIcon(NULL, 16, 16, 1, 1, bits, bits);
    ImageList_AddIcon(himl, hicon);
    ImageList_AddIcon(himl, hicon);
    ImageList_AddIcon(himl, hicon);

    hwTab = create_tabcontrol(TCS_FIXEDWIDTH, TCIF_TEXT|TCIF_IMAGE);
    SendMessage(hwTab, TCM_SETIMAGELIST, 0, (LPARAM)himl);

    memset(&item, 0, sizeof(TCITEM));
    item.mask = TCIF_IMAGE;

    for(i = 0; i < 3; i++) {
        SendMessage(hwTab, TCM_GETITEM, i, (LPARAM)&item);
        expect(i, item.iImage);
    }

    /* remove image middle image */
    SendMessage(hwTab, TCM_REMOVEIMAGE, 1, 0);
    expect(2, ImageList_GetImageCount(himl));
    item.iImage = -1;
    SendMessage(hwTab, TCM_GETITEM, 0, (LPARAM)&item);
    expect(0, item.iImage);
    item.iImage = 0;
    SendMessage(hwTab, TCM_GETITEM, 1, (LPARAM)&item);
    expect(-1, item.iImage);
    item.iImage = 0;
    SendMessage(hwTab, TCM_GETITEM, 2, (LPARAM)&item);
    expect(1, item.iImage);
    /* remove first image */
    SendMessage(hwTab, TCM_REMOVEIMAGE, 0, 0);
    expect(1, ImageList_GetImageCount(himl));
    item.iImage = 0;
    SendMessage(hwTab, TCM_GETITEM, 0, (LPARAM)&item);
    expect(-1, item.iImage);
    item.iImage = 0;
    SendMessage(hwTab, TCM_GETITEM, 1, (LPARAM)&item);
    expect(-1, item.iImage);
    item.iImage = -1;
    SendMessage(hwTab, TCM_GETITEM, 2, (LPARAM)&item);
    expect(0, item.iImage);
    /* remove the last one */
    SendMessage(hwTab, TCM_REMOVEIMAGE, 0, 0);
    expect(0, ImageList_GetImageCount(himl));
    for(i = 0; i < 3; i++) {
        item.iImage = 0;
        SendMessage(hwTab, TCM_GETITEM, i, (LPARAM)&item);
        expect(-1, item.iImage);
    }

    DestroyWindow(hwTab);
    ImageList_Destroy(himl);
    DestroyIcon(hicon);
}

START_TEST(tab)
{
    HWND parent_wnd;
    LOGFONTA logfont;

    lstrcpyA(logfont.lfFaceName, "Arial");
    memset(&logfont, 0, sizeof(logfont));
    logfont.lfHeight = -12;
    logfont.lfWeight = FW_NORMAL;
    logfont.lfCharSet = ANSI_CHARSET;
    hFont = CreateFontIndirectA(&logfont);

    InitCommonControls();

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

    init_msg_sequences(sequences, NUM_MSG_SEQUENCES);

    parent_wnd = createParentWindow();
    ok(parent_wnd != NULL, "Failed to create parent window!\n");

    /* Testing getters and setters with 5 tabs */
    test_getters_setters(parent_wnd, 5);

    test_adjustrect(parent_wnd);

    test_insert_focus(parent_wnd);
    test_delete_focus(parent_wnd);
    test_removeimage(parent_wnd);

    DestroyWindow(parent_wnd);
}
