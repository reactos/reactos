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

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>

#include "wine/test.h"
#include "msg.h"

#define TAB_PADDING_X 6
#define EXTRA_ICON_PADDING 3
#define MAX_TABLEN 32
#define MIN_CHAR_LENGTH 6

#define NUM_MSG_SEQUENCES  2
#define PARENT_SEQ_INDEX   0
#define TAB_SEQ_INDEX      1

#define expect(expected,got) expect_(__LINE__, expected, got)
static inline void expect_(unsigned line, DWORD expected, DWORD got)
{
    ok_(__FILE__, line)(expected == got, "Expected %ld, got %ld\n", expected, got);
}

#define expect_str(expected, got)\
 ok ( strcmp(expected, got) == 0, "Expected '%s', got '%s'\n", expected, got)

#define TabWidthPadded(default_min_tab_width, padd_x, num) ((default_min_tab_width) - (TAB_PADDING_X - (padd_x)) * num)

static HIMAGELIST (WINAPI *pImageList_Create)(INT,INT,UINT,INT,INT);
static BOOL (WINAPI *pImageList_Destroy)(HIMAGELIST);
static INT (WINAPI *pImageList_GetImageCount)(HIMAGELIST);
static INT (WINAPI *pImageList_ReplaceIcon)(HIMAGELIST,INT,HICON);

static void CheckSize(HWND hwnd, INT width, INT height, const char *msg, int line)
{
    RECT r;

    SendMessageA(hwnd, TCM_GETITEMRECT, 0, (LPARAM)&r);
    if (width >= 0 && height < 0)
        ok_(__FILE__,line) (width == r.right - r.left, "%s: Expected width [%d] got [%ld]\n",
            msg, width, r.right - r.left);
    else if (height >= 0 && width < 0)
        ok_(__FILE__,line) (height == r.bottom - r.top,  "%s: Expected height [%d] got [%ld]\n",
            msg, height, r.bottom - r.top);
    else
        ok_(__FILE__,line) ((width  == r.right  - r.left) && (height == r.bottom - r.top ),
	    "%s: Expected [%d,%d] got [%ld,%ld]\n", msg, width, height,
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
static LRESULT tcn_selchanging_result;
static DWORD winevent_hook_thread_id;

static struct msg_sequence *sequences[NUM_MSG_SEQUENCES];

static void CALLBACK msg_winevent_proc(HWINEVENTHOOK hevent,
				    DWORD event,
				    HWND hwnd,
				    LONG object_id,
				    LONG child_id,
				    DWORD thread_id,
				    DWORD event_time)
{
    struct message msg = {0};
    char class_name[256];

    ok(thread_id == winevent_hook_thread_id, "we didn't ask for events from other threads\n");

    /* ignore window and other system events */
    if (object_id != OBJID_CLIENT) return;

    /* ignore events not from a tab control */
    if (!GetClassNameA(hwnd, class_name, ARRAY_SIZE(class_name)) ||
        strcmp(class_name, WC_TABCONTROLA) != 0)
        return;

    msg.message = event;
    msg.flags = winevent_hook|wparam|lparam;
    msg.wParam = object_id;
    msg.lParam = child_id;
    add_message(sequences, TAB_SEQ_INDEX, &msg);
}

static void init_winevent_hook(void) {
    hwineventhook = SetWinEventHook(EVENT_MIN, EVENT_MAX, GetModuleHandleA(0), msg_winevent_proc,
        0, GetCurrentThreadId(), WINEVENT_INCONTEXT);
    winevent_hook_thread_id = GetCurrentThreadId();
    if (!hwineventhook)
        win_skip( "no win event hook support\n" );
}

static void uninit_winevent_hook(void) {
    if (!hwineventhook)
        return;

    UnhookWinEvent(hwineventhook);
    hwineventhook = 0;
}

static const struct message empty_sequence[] = {
    { 0 }
};

static const struct message get_row_count_seq[] = {
    { TCM_GETROWCOUNT, sent|wparam|lparam, 0, 0 },
    { 0 }
};

static const struct message getset_cur_focus_seq[] = {
    { TCM_SETCURFOCUS, sent|lparam, 0 },
    { EVENT_OBJECT_SELECTION, winevent_hook|wparam|lparam, OBJID_CLIENT, 5 },
    { TCM_GETCURFOCUS, sent|wparam|lparam, 0, 0 },
    { TCM_SETCURFOCUS, sent|lparam, 0 },
    { EVENT_OBJECT_SELECTION, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { TCM_GETCURFOCUS, sent|wparam|lparam, 0, 0 },
    { TCM_SETCURSEL, sent|lparam, 0 },
    { EVENT_OBJECT_SELECTION, winevent_hook|wparam|lparam, OBJID_CLIENT, 2 },
    { TCM_SETCURFOCUS, sent|lparam, 0 },
    { TCM_GETCURFOCUS, sent|wparam|lparam, 0, 0 },
    { 0 }
};

static const struct message getset_cur_focus_buttons_seq[] = {
    { TCM_SETCURFOCUS, sent|lparam, 0 },
    { TCM_GETITEMCOUNT, sent|defwinproc|optional },
    { TCM_GETITEMCOUNT, sent|defwinproc|optional },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 5 },
    { TCM_GETCURFOCUS, sent|wparam|lparam, 0, 0 },
    { TCM_SETCURFOCUS, sent|lparam, 0 },
    { TCM_GETCURFOCUS, sent|wparam|lparam, 0, 0 },
    { TCM_SETCURSEL, sent|lparam, 0 },
    { EVENT_OBJECT_SELECTION, winevent_hook|wparam|lparam, OBJID_CLIENT, 2 },
    { TCM_SETCURFOCUS, sent|lparam, 0 },
    { TCM_GETCURFOCUS, sent|wparam|lparam, 0, 0 },
    { 0 }
};

static const struct message getset_cur_sel_seq[] = {
    { TCM_SETCURSEL, sent|lparam, 0 },
    { EVENT_OBJECT_SELECTION, winevent_hook|wparam|lparam, OBJID_CLIENT, 5 },
    { TCM_GETCURSEL, sent|wparam|lparam, 0, 0 },
    { TCM_GETCURFOCUS, sent|wparam|lparam, 0, 0 },
    { TCM_SETCURSEL, sent|lparam, 0 },
    { EVENT_OBJECT_SELECTION, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { TCM_GETCURSEL, sent|wparam|lparam, 0, 0 },
    { TCM_SETCURSEL, sent|lparam, 0 },
    { EVENT_OBJECT_SELECTION, winevent_hook|wparam|lparam, OBJID_CLIENT, 2 },
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
    { EVENT_OBJECT_CREATE, winevent_hook|wparam|lparam, OBJID_CLIENT, 1 },
    { WM_NOTIFYFORMAT, sent|defwinproc|optional },
    { WM_QUERYUISTATE, sent|defwinproc|optional },
    { WM_PARENTNOTIFY, sent|defwinproc|optional },
    { TCM_GETITEMCOUNT, sent|wparam|lparam, 0, 0 },
    { TCM_GETCURFOCUS, sent|wparam|lparam, 0, 0 },
    { TCM_INSERTITEMA, sent|wparam, 2 },
    { WM_NOTIFYFORMAT, sent|defwinproc|optional },
    { WM_QUERYUISTATE, sent|defwinproc|optional, },
    { WM_PARENTNOTIFY, sent|defwinproc|optional },
    { EVENT_OBJECT_CREATE, winevent_hook|wparam|lparam, OBJID_CLIENT, 2 },
    { TCM_GETITEMCOUNT, sent|wparam|lparam, 0, 0 },
    { TCM_GETCURFOCUS, sent|wparam|lparam, 0, 0 },
    { TCM_SETCURFOCUS, sent|wparam|lparam, -1, 0 },
    { EVENT_OBJECT_SELECTION, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { TCM_GETCURFOCUS, sent|wparam|lparam, 0, 0 },
    { TCM_INSERTITEMA, sent|wparam, 3 },
    { EVENT_OBJECT_CREATE, winevent_hook|wparam|lparam, OBJID_CLIENT, 3 },
    { TCM_GETCURFOCUS, sent|wparam|lparam, 0, 0 },
    { 0 }
};

static const struct message delete_focus_seq[] = {
    { TCM_GETITEMCOUNT, sent|wparam|lparam, 0, 0 },
    { TCM_GETCURFOCUS, sent|wparam|lparam, 0, 0 },
    { TCM_DELETEITEM, sent|wparam|lparam, 1, 0 },
    { EVENT_OBJECT_DESTROY, winevent_hook|wparam|lparam, OBJID_CLIENT, 2 },
    { TCM_GETITEMCOUNT, sent|wparam|lparam, 0, 0 },
    { TCM_GETCURFOCUS, sent|wparam|lparam, 0, 0 },
    { TCM_SETCURFOCUS, sent|wparam|lparam, -1, 0 },
    { EVENT_OBJECT_SELECTION, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { TCM_GETCURFOCUS, sent|wparam|lparam, 0, 0 },
    { TCM_DELETEITEM, sent|wparam|lparam, 0, 0 },
    { EVENT_OBJECT_DESTROY, winevent_hook|wparam|lparam, OBJID_CLIENT, 1 },
    { TCM_GETITEMCOUNT, sent|wparam|lparam, 0, 0 },
    { TCM_GETCURFOCUS, sent|wparam|lparam, 0, 0 },
    { 0 }
};

static const struct message rbuttonup_seq[] = {
    { WM_RBUTTONUP, sent|wparam|lparam, 0, 0 },
    { WM_CONTEXTMENU, sent|defwinproc },
    { 0 }
};

static const struct message full_selchange_parent_seq[] = {
    { WM_NOTIFY, sent|id, 0, 0, TCN_SELCHANGING },
    { WM_NOTIFY, sent|id, 0, 0, TCN_SELCHANGE },
    { 0 }
};

static const struct message selchanging_parent_seq[] = {
    { WM_NOTIFY, sent|id, 0, 0, TCN_SELCHANGING },
    { 0 }
};

static const struct message selchange_parent_seq[] = {
    { WM_NOTIFY, sent|id, 0, 0, TCN_SELCHANGE },
    { 0 }
};

static const struct message setfocus_parent_seq[] = {
    { WM_NOTIFY, sent|id, 0, 0, TCN_FOCUSCHANGE },
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
    SendMessageA(handle, WM_SETFONT, (WPARAM)hFont, (LPARAM)0);

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

static LRESULT WINAPI parent_wnd_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static LONG defwndproc_counter = 0;
    struct message msg = { 0 };
    LRESULT ret;

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
        if (message == WM_NOTIFY && lParam)
            msg.id = ((NMHDR*)lParam)->code;
        add_message(sequences, PARENT_SEQ_INDEX, &msg);
    }

    /* dump sent structure data */
    if (message == WM_DRAWITEM)
        g_drawitem = *(DRAWITEMSTRUCT*)lParam;

    if (message == WM_NOTIFY)
    {
        NMHDR *nmhdr = (NMHDR *)lParam;
        if (nmhdr && nmhdr->code == TCN_SELCHANGING)
            return tcn_selchanging_result;
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
    cls.lpfnWndProc = parent_wnd_proc;
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

static LRESULT WINAPI tab_subclass_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WNDPROC oldproc = (WNDPROC)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    static LONG defwndproc_counter = 0;
    struct message msg = { 0 };
    LRESULT ret;

    /* do not log painting messages */
    if (message != WM_PAINT &&
        message != WM_ERASEBKGND &&
        message != WM_NCPAINT &&
        message != WM_NCHITTEST &&
        message != WM_GETTEXT &&
        message != WM_GETICON &&
        message != WM_DEVICECHANGE &&
        message != WM_GETOBJECT)
    {
        msg.message = message;
        msg.flags = sent|wparam|lparam;
        if (defwndproc_counter) msg.flags |= defwinproc;
        msg.wParam = wParam;
        msg.lParam = lParam;
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

    oldproc = (WNDPROC)SetWindowLongPtrA(tabHandle, GWLP_WNDPROC, (LONG_PTR)tab_subclass_proc);
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
    HIMAGELIST himl = pImageList_Create(21, 21, ILC_COLOR, 3, 4);
    SIZE size;
    HDC hdc;
    HFONT hOldFont;
    INT i, dpi, exp;
    INT default_min_tab_width;
    TEXTMETRICW text_metrics;

    hwTab = create_tabcontrol(TCS_FIXEDWIDTH, TCIF_TEXT|TCIF_IMAGE);
    SendMessageA(hwTab, TCM_SETMINTABWIDTH, 0, nMinTabWidth);

    hdc = GetDC(hwTab);
    dpi = GetDeviceCaps(hdc, LOGPIXELSX);
    hOldFont = SelectObject(hdc, (HFONT)SendMessageA(hwTab, WM_GETFONT, 0, 0));
    GetTextExtentPoint32A(hdc, "Tab 1", strlen("Tab 1"), &size);
    trace("Tab1 text size: size.cx=%ld size.cy=%ld\n", size.cx, size.cy);
    GetTextMetricsW(hdc, &text_metrics);
    default_min_tab_width = text_metrics.tmAveCharWidth * MIN_CHAR_LENGTH + TAB_PADDING_X * 2;
    SelectObject(hdc, hOldFont);
    ReleaseDC(hwTab, hdc);

    trace ("default_min_tab_width: %d\n", default_min_tab_width);
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
    exp = max(size.cx +TAB_PADDING_X*2, (nMinTabWidth < 0) ? default_min_tab_width : nMinTabWidth);
    SendMessageA( hwTab, TCM_GETITEMRECT, 0, (LPARAM)&rTab );
    ok( rTab.right  - rTab.left == exp || broken(rTab.right  - rTab.left == default_min_tab_width),
        "no icon, default width: Expected width [%d] got [%ld]\n", exp, rTab.right - rTab.left );

    CHECKSIZE(hwTab, max(size.cx + TAB_PADDING_X*2, nMinTabWidth < 0 ? default_min_tab_width : nMinTabWidth), size.cy + 5, "Initial values");
    for (i=0; i<8; i++)
    {
        INT nTabWidth = (nMinTabWidth < 0) ? TabWidthPadded(default_min_tab_width, i, 2) : nMinTabWidth;

        SendMessageA(hwTab, TCM_SETIMAGELIST, 0, 0);
        SendMessageA(hwTab, TCM_SETPADDING, 0, MAKELPARAM(i, i));

        TABCHECKSETSIZE(hwTab, 50, 20, max(size.cx + i*2, nTabWidth), 20, "no icon, set size");
        TABCHECKSETSIZE(hwTab, 0, 1, max(size.cx + i*2, nTabWidth), 1, "no icon, min size");

        SendMessageA(hwTab, TCM_SETIMAGELIST, 0, (LPARAM)himl);
        nTabWidth = (nMinTabWidth < 0) ? TabWidthPadded(default_min_tab_width, i, 3) : nMinTabWidth;

        TABCHECKSETSIZE(hwTab, 50, 30, max(size.cx + 21 + i*3, nTabWidth), 30, "with icon, set size > icon");
        TABCHECKSETSIZE(hwTab, 20, 20, max(size.cx + 21 + i*3, nTabWidth), 20, "with icon, set size < icon");
        TABCHECKSETSIZE(hwTab, 0, 1, max(size.cx + 21 + i*3, nTabWidth), 1, "with icon, min size");

        /* tests that a change to padding only doesn't impact reported size */
        SendMessageA(hwTab, TCM_SETPADDING, 0, MAKELPARAM(i + 1, i + 1));
        CHECKSIZE(hwTab, max(size.cx + 21 + i*3, nTabWidth), 1, "with icon, min size, padding only change");
        TABCHECKSETSIZE(hwTab, 0, 1, max(size.cx + 21 + i*3, nTabWidth), 1, "with icon, min size, same size");
    }
    DestroyWindow (hwTab);

    hwTab = create_tabcontrol(0, TCIF_IMAGE);
    SendMessageA(hwTab, TCM_SETMINTABWIDTH, 0, nMinTabWidth);

    trace ("  non fixed width, no text...\n");
    exp = (nMinTabWidth < 0) ? default_min_tab_width : nMinTabWidth;
    SendMessageA( hwTab, TCM_GETITEMRECT, 0, (LPARAM)&rTab );
    ok( rTab.right  - rTab.left == exp || broken(rTab.right  - rTab.left == default_min_tab_width),
        "no icon, default width: Expected width [%d] got [%ld]\n", exp, rTab.right - rTab.left );

    CHECKSIZE(hwTab, nMinTabWidth < 0 ? default_min_tab_width : nMinTabWidth, size.cy + 5, "Initial values");
    for (i=0; i<8; i++)
    {
        INT nTabWidth = (nMinTabWidth < 0) ? TabWidthPadded(default_min_tab_width, i, 2) : nMinTabWidth;

        SendMessageA(hwTab, TCM_SETIMAGELIST, 0, 0);
        SendMessageA(hwTab, TCM_SETPADDING, 0, MAKELPARAM(i, i));

        TABCHECKSETSIZE(hwTab, 50, 20, nTabWidth, 20, "no icon, set size");
        TABCHECKSETSIZE(hwTab, 0, 1, nTabWidth, 1, "no icon, min size");

        SendMessageA(hwTab, TCM_SETIMAGELIST, 0, (LPARAM)himl);
        if (i > 1 && nMinTabWidth > 0 && nMinTabWidth < default_min_tab_width)
            nTabWidth += EXTRA_ICON_PADDING *(i-1);

        TABCHECKSETSIZE(hwTab, 50, 30, nTabWidth, 30, "with icon, set size > icon");
        TABCHECKSETSIZE(hwTab, 20, 20, nTabWidth, 20, "with icon, set size < icon");
        TABCHECKSETSIZE(hwTab, 0, 1, nTabWidth, 1, "with icon, min size");

        /* tests that a change to padding only doesn't impact reported size */
        SendMessageA(hwTab, TCM_SETPADDING, 0, MAKELPARAM(i + 1, i + 1));
        CHECKSIZE(hwTab, nTabWidth, 1, "with icon, min size, padding only change");
        TABCHECKSETSIZE(hwTab, 0, 1, nTabWidth, 1, "with icon, min size, same size");
    }

    DestroyWindow (hwTab);

    pImageList_Destroy(himl);
}

static void test_width(void)
{
    HFONT oldFont = hFont;
    const char *fonts[] = {
        "System",
        "Arial",
        "Tahoma",
        "Courier New",
        "MS Shell Dlg",
    };

    LOGFONTA logfont = {
        .lfHeight = -12,
        .lfWeight = FW_NORMAL,
        .lfCharSet = ANSI_CHARSET
    };

    for(int i = 0; i < sizeof(fonts)/sizeof(fonts[0]); i++) {
        trace ("Testing with the '%s' font\n", fonts[i]);
        lstrcpyA(logfont.lfFaceName, fonts[i]);
        hFont = CreateFontIndirectA(&logfont);

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

    hFont = oldFont;
}

static void test_setitemsize(void)
{
    HWND hwTab;
    LRESULT result;
    HDC hdc;
    INT dpi;

    hwTab = create_tabcontrol(0, TCIF_TEXT);
    SendMessageA(hwTab, TCM_SETMINTABWIDTH, 0, -1);

    hdc = GetDC(hwTab);
    dpi = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(hwTab, hdc);

    result = SendMessageA(hwTab, TCM_SETITEMSIZE, 0, MAKELPARAM(50, 20));
    ok (LOWORD(result) == dpi, "Excepted width to be %d, got %d\n", dpi, LOWORD(result));

    result = SendMessageA(hwTab, TCM_SETITEMSIZE, 0, MAKELPARAM(0, 1));
    ok (LOWORD(result) == 50, "Excepted width to be 50, got %d\n", LOWORD(result));
    ok (HIWORD(result) == 20, "Excepted height to be 20, got %d\n", HIWORD(result));

    DestroyWindow (hwTab);
}

static void test_curfocus(void)
{
    const INT nTabs = 5;
    INT ret;
    HWND hTab;

    hTab = createFilledTabControl(parent_wnd, TCS_FIXEDWIDTH, TCIF_TEXT|TCIF_IMAGE, nTabs);
    ok(hTab != NULL, "Failed to create tab control\n");

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    /* Testing CurFocus with largest appropriate value */
    ret = SendMessageA(hTab, TCM_SETCURFOCUS, nTabs - 1, 0);
    ok(ret == 0, "Unexpected ret value %d.\n", ret);
    ret = SendMessageA(hTab, TCM_GETCURFOCUS, 0, 0);
    ok(ret == nTabs - 1, "Unexpected focus index %d.\n", ret);

    /* Testing CurFocus with negative value */
    ret = SendMessageA(hTab, TCM_SETCURFOCUS, -10, 0);
    ok(ret == 0, "Unexpected ret value %d.\n", ret);
    ret = SendMessageA(hTab, TCM_GETCURFOCUS, 0, 0);
    ok(ret == -1, "Unexpected focus index %d.\n", ret);

    /* Testing CurFocus with value larger than number of tabs */
    ret = SendMessageA(hTab, TCM_SETCURSEL, 1, 0);
    ok(ret == -1, "Unexpected focus index %d.\n", ret);

    ret = SendMessageA(hTab, TCM_SETCURFOCUS, nTabs + 1, 0);
    ok(ret == 0, "Unexpected ret value %d.\n", ret);
    ret = SendMessageA(hTab, TCM_GETCURFOCUS, 0, 0);
    ok(ret == 1, "Unexpected focus index %d.\n", ret);

    ok_sequence(sequences, TAB_SEQ_INDEX, getset_cur_focus_seq, "Set focused tab sequence", FALSE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_sequence, "Set focused tab parent sequence", TRUE);

    DestroyWindow(hTab);

    /* TCS_BUTTONS */
    hTab = createFilledTabControl(parent_wnd, TCS_BUTTONS, TCIF_TEXT|TCIF_IMAGE, nTabs);
    ok(hTab != NULL, "Failed to create tab control\n");

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    /* Testing CurFocus with largest appropriate value */
    ret = SendMessageA(hTab, TCM_SETCURFOCUS, nTabs - 1, 0);
    ok(ret == 0, "Unexpected ret value %d.\n", ret);
    ret = SendMessageA(hTab, TCM_GETCURFOCUS, 0, 0);
    ok(ret == nTabs - 1, "Unexpected focus index %d.\n", ret);

    /* Testing CurFocus with negative value */
    ret = SendMessageA(hTab, TCM_SETCURFOCUS, -10, 0);
    ok(ret == 0, "Unexpected ret value %d.\n", ret);
    ret = SendMessageA(hTab, TCM_GETCURFOCUS, 0, 0);
    todo_wine
    ok(ret == nTabs - 1, "Unexpected focus index %d.\n", ret);

    /* Testing CurFocus with value larger than number of tabs */
    ret = SendMessageA(hTab, TCM_SETCURSEL, 1, 0);
    todo_wine
    ok(ret == 0, "Unexpected focus index %d.\n", ret);

    ret = SendMessageA(hTab, TCM_SETCURFOCUS, nTabs + 1, 0);
    ok(ret == 0, "Unexpected ret value %d.\n", ret);
    ret = SendMessageA(hTab, TCM_GETCURFOCUS, 0, 0);
    todo_wine
    ok(ret == nTabs - 1, "Unexpected focus index %d.\n", ret);

    ok_sequence(sequences, TAB_SEQ_INDEX, getset_cur_focus_buttons_seq, "TCS_BUTTONS: set focused tab sequence", FALSE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, setfocus_parent_seq, "TCS_BUTTONS: set focused tab parent sequence", TRUE);

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
    ok(ret == TRUE, "got %ld\n", ret);

    /* set some item data */
    tcItem.lParam = ~0;
    tcItem.mask = TCIF_PARAM;

    ret = SendMessageA(hTab, TCM_INSERTITEMA, 0, (LPARAM)&tcItem);
    ok(ret == 0, "got %ld\n", ret);

    /* all sizeof(LPARAM) returned anyway when using sizeof(LPARAM)-1 size */
    memset(&lparam, 0xaa, sizeof(lparam));
    tcItem.lParam = lparam;
    tcItem.mask = TCIF_PARAM;
    ret = SendMessageA(hTab, TCM_GETITEMA, 0, (LPARAM)&tcItem);
    expect(TRUE, ret);
    /* everything higher specified size is preserved */
    memset(&lparam, 0xff, sizeof(lparam)-1);
    ok(tcItem.lParam == lparam, "Expected 0x%Ix, got 0x%Ix\n", lparam, tcItem.lParam);

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
    ok(tcItem.lParam == 0, "Expected zero lParam, got %Iu\n", tcItem.lParam);

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
    ok(tcItem.lParam == 0, "Expected zero lParam, got %Iu\n", tcItem.lParam);

    memset(&tcItem, 0xcc, sizeof(tcItem));
    tcItem.mask = TCIF_PARAM;
    ret = SendMessageA(hTab, TCM_GETITEMA, -2, (LPARAM)&tcItem);
    expect(FALSE, ret);
    ok(tcItem.lParam == 0, "Expected zero lParam, got %Iu\n", tcItem.lParam);

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
    HWND hTab, toolTip, hwnd;
    const INT nTabs = 5;
    int ret;

    hTab = createFilledTabControl(parent_wnd, TCS_FIXEDWIDTH, TCIF_TEXT|TCIF_IMAGE, nTabs);
    ok(hTab != NULL, "Failed to create tab control\n");

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    toolTip = create_tooltip(hTab, toolTipText);
    ret = SendMessageA(hTab, TCM_SETTOOLTIPS, (WPARAM)toolTip, 0);
    ok(ret == 0, "Unexpected ret value %d.\n", ret);
    hwnd = (HWND)SendMessageA(hTab, TCM_GETTOOLTIPS, 0, 0);
    ok(toolTip == hwnd, "Unexpected tooltip window.\n");

    ret = SendMessageA(hTab, TCM_SETTOOLTIPS, 0, 0);
    ok(ret == 0, "Unexpected ret value %d.\n", ret);
    hwnd = (HWND)SendMessageA(hTab, TCM_GETTOOLTIPS, 0, 0);
    ok(hwnd == NULL, "Unexpected tooltip window.\n");
    ok(IsWindow(toolTip), "Expected tooltip window to be alive.\n");

    ok_sequence(sequences, TAB_SEQ_INDEX, getset_tooltip_seq, "Getset tooltip test sequence", TRUE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, getset_tooltip_parent_seq, "Getset tooltip test parent sequence", TRUE);

    DestroyWindow(hTab);
    DestroyWindow(toolTip);
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
    HIMAGELIST himl = pImageList_Create(16, 16, ILC_COLOR, 3, 4);

    hicon = CreateIcon(NULL, 16, 16, 1, 1, bits, bits);
    pImageList_ReplaceIcon(himl, -1, hicon);
    pImageList_ReplaceIcon(himl, -1, hicon);
    pImageList_ReplaceIcon(himl, -1, hicon);

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
    i = pImageList_GetImageCount(himl);
    ok(i == 2, "Unexpected image count %d.\n", i);
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
    i = pImageList_GetImageCount(himl);
    ok(i == 1, "Unexpected image count %d.\n", i);
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
    i = pImageList_GetImageCount(himl);
    ok(i == 0, "Unexpected image count %d.\n", i);
    for(i = 0; i < 3; i++) {
        item.iImage = 0;
        SendMessageA(hwTab, TCM_GETITEMA, i, (LPARAM)&item);
        expect(-1, item.iImage);
    }

    DestroyWindow(hwTab);
    pImageList_Destroy(himl);
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
    ok(ret == FALSE, "got %ld\n", ret);

    ret = SendMessageA(hTab, TCM_SETITEMEXTRA, 2, 0);
    ok(ret == TRUE, "got %ld\n", ret);
    DestroyWindow(hTab);

    /* it's not possible to change extra data size for control with tabs */
    hTab = createFilledTabControl(parent_wnd, TCS_FIXEDWIDTH, TCIF_TEXT|TCIF_IMAGE, 4);
    ok(hTab != NULL, "Failed to create tab control\n");

    ret = SendMessageA(hTab, TCM_SETITEMEXTRA, 2, 0);
    ok(ret == FALSE, "got %ld\n", ret);
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
    ok(g_drawitem.itemData == itemdata, "got 0x%Ix, expected 0x%Ix\n", g_drawitem.itemData, itemdata);

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
    ok(*(ULONG_PTR*)g_drawitem.itemData == itemdata, "got 0x%Ix, expected 0x%Ix\n", g_drawitem.itemData, itemdata);

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
        "got 0x%Ix, expected 0x%Ix\n", g_drawitem.itemData, itemdata);

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
        ok(style == ptr->act_style, "expected style 0x%08lx, got style 0x%08lx\n", ptr->act_style, style);

        DestroyWindow(hTab);
        ptr++;
    }
}

static void init_functions(void)
{
    HMODULE hComCtl32 = LoadLibraryA("comctl32.dll");

#define X(f) p##f = (void*)GetProcAddress(hComCtl32, #f);
    X(ImageList_Create);
    X(ImageList_Destroy);
    X(ImageList_GetImageCount);
    X(ImageList_ReplaceIcon);
#undef X
}

static void test_TCN_SELCHANGING(void)
{
    const INT nTabs = 5;
    HWND hTab;
    INT ret;

    hTab = createFilledTabControl(parent_wnd, WS_CHILD|TCS_FIXEDWIDTH, TCIF_TEXT|TCIF_IMAGE, nTabs);
    ok(hTab != NULL, "Failed to create tab control\n");

    /* Initially first tab is focused. */
    ret = SendMessageA(hTab, TCM_GETCURFOCUS, 0, 0);
    ok(ret == 0, "Unexpected tab focus %d.\n", ret);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    /* Setting focus to currently focused item should do nothing. */
    ret = SendMessageA(hTab, TCM_SETCURFOCUS, 0, 0);
    ok(ret == 0, "Unexpected ret value %d.\n", ret);

    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_sequence, "Set focus to focused tab sequence", FALSE);

    /* Allow selection change. */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    tcn_selchanging_result = 0;
    ret = SendMessageA(hTab, TCM_SETCURFOCUS, nTabs - 1, 0);
    ok(ret == 0, "Unexpected ret value %d.\n", ret);

    ok_sequence(sequences, PARENT_SEQ_INDEX, full_selchange_parent_seq, "Focus change allowed sequence", FALSE);

    ret = SendMessageA(hTab, TCM_GETCURFOCUS, 0, 0);
    ok(ret == nTabs - 1, "Unexpected focused tab %d.\n", ret);
    ret = SendMessageA(hTab, TCM_GETCURSEL, 0, 0);
    ok(ret == nTabs - 1, "Unexpected selected tab %d.\n", ret);

    /* Forbid selection change. */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    tcn_selchanging_result = 1;
    ret = SendMessageA(hTab, TCM_SETCURFOCUS, 0, 0);
    ok(ret == 0, "Unexpected ret value %d.\n", ret);

    ok_sequence(sequences, PARENT_SEQ_INDEX, selchanging_parent_seq, "Focus change disallowed sequence", FALSE);

    ret = SendMessageA(hTab, TCM_GETCURFOCUS, 0, 0);
    todo_wine
    ok(ret == nTabs - 1, "Unexpected focused tab %d.\n", ret);
    ret = SendMessageA(hTab, TCM_GETCURSEL, 0, 0);
    todo_wine
    ok(ret == nTabs - 1, "Unexpected selected tab %d.\n", ret);

    /* Removing focus sends only TCN_SELCHANGE */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    ret = SendMessageA(hTab, TCM_SETCURFOCUS, -1, 0);
    ok(ret == 0, "Unexpected ret value %d.\n", ret);

    ok_sequence(sequences, PARENT_SEQ_INDEX, selchange_parent_seq, "Remove focus sequence", FALSE);

    ret = SendMessageA(hTab, TCM_GETCURFOCUS, 0, 0);
    ok(ret == -1, "Unexpected focused tab %d.\n", ret);

    tcn_selchanging_result = 0;

    DestroyWindow(hTab);
}

static void test_TCM_GETROWCOUNT(void)
{
    const INT nTabs = 5;
    HWND hTab;
    INT count;

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    hTab = createFilledTabControl(parent_wnd, TCS_FIXEDWIDTH, TCIF_TEXT|TCIF_IMAGE, nTabs);
    ok(hTab != NULL, "Failed to create tab control\n");

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    count = SendMessageA(hTab, TCM_GETROWCOUNT, 0, 0);
    ok(count == 1, "Unexpected row count %d.\n", count);

    ok_sequence(sequences, TAB_SEQ_INDEX, get_row_count_seq, "Get rowCount test sequence", FALSE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_sequence, "Get rowCount test parent sequence", FALSE);

    DestroyWindow(hTab);
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

    init_functions();

    init_msg_sequences(sequences, NUM_MSG_SEQUENCES);

    init_winevent_hook();

    parent_wnd = createParentWindow();
    ok(parent_wnd != NULL, "Failed to create parent window!\n");

    test_width();
    test_setitemsize();
    test_curfocus();
    test_cursel();
    test_extendedstyle();
    test_unicodeformat();
    test_getset_item();
    test_getset_tooltips();
    test_adjustrect();
    test_insert_focus();
    test_delete_focus();
    test_delete_selection();
    test_removeimage();
    test_TCM_SETITEMEXTRA();
    test_TCS_OWNERDRAWFIXED();
    test_WM_CONTEXTMENU();
    test_create();
    test_TCN_SELCHANGING();
    test_TCM_GETROWCOUNT();

    uninit_winevent_hook();

    DestroyWindow(parent_wnd);
}
