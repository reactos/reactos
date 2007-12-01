/*
 * ListView tests
 *
 * Copyright 2006 Mike McCormack for CodeWeavers
 * Copyright 2007 George Gov
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

#include <stdio.h>
#include <windows.h>
#include <commctrl.h>

#include "wine/test.h"
#include "msg.h"

#define PARENT_SEQ_INDEX    0
#define LISTVIEW_SEQ_INDEX  1
#define NUM_MSG_SEQUENCES   2

#define LISTVIEW_ID 0
#define HEADER_ID   1

#define expect(expected, got) ok(got == expected, "Expected %d, got %d\n", expected, got)
#define expect2(expected1, expected2, got1, got2) ok(expected1 == got1 && expected2 == got2, \
       "expected (%d,%d), got (%d,%d)\n", expected1, expected2, got1, got2)

HWND hwndparent;

static struct msg_sequence *sequences[NUM_MSG_SEQUENCES];

static const struct message create_parent_wnd_seq[] = {
    { WM_GETMINMAXINFO,     sent },
    { WM_NCCREATE,          sent },
    { WM_NCCALCSIZE,        sent|wparam, 0 },
    { WM_CREATE,            sent },
    { WM_SHOWWINDOW,        sent|wparam, 1 },
    { WM_WINDOWPOSCHANGING, sent|wparam, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam, 0 },
    { WM_ACTIVATEAPP,       sent|wparam, 1 },
    { WM_NCACTIVATE,        sent|wparam, 1 },
    { WM_ACTIVATE,          sent|wparam, 1 },
    { WM_IME_SETCONTEXT,    sent|wparam|defwinproc|optional, 1 },
    { WM_IME_NOTIFY,        sent|defwinproc|optional },
    { WM_SETFOCUS,          sent|wparam|defwinproc, 0 },
    /* Win9x adds SWP_NOZORDER below */
    { WM_WINDOWPOSCHANGED,  sent, /*|wparam, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE*/ },
    { WM_NCCALCSIZE,        sent|wparam|optional, 1 },
    { WM_SIZE,              sent },
    { WM_MOVE,              sent },
    { 0 }
};

static const struct message redraw_listview_seq[] = {
    { WM_PAINT,      sent|id,            0, 0, LISTVIEW_ID },
    { WM_PAINT,      sent|id,            0, 0, HEADER_ID },
    { WM_NCPAINT,    sent|id|defwinproc, 0, 0, HEADER_ID },
    { WM_ERASEBKGND, sent|id|defwinproc, 0, 0, HEADER_ID },
    { WM_NOTIFY,     sent|id|defwinproc, 0, 0, LISTVIEW_ID },
    { WM_NCPAINT,    sent|id|defwinproc, 0, 0, LISTVIEW_ID },
    { WM_ERASEBKGND, sent|id|defwinproc, 0, 0, LISTVIEW_ID },
    { 0 }
};

static const struct message listview_icon_spacing_seq[] = {
    { LVM_SETICONSPACING, sent|lparam, 0, (LPARAM) MAKELONG(20, 30) },
    { LVM_SETICONSPACING, sent|lparam, 0, (LPARAM) MAKELONG(25, 35) },
    { LVM_SETICONSPACING, sent|lparam, 0, (LPARAM) MAKELONG(-1, -1) },
    { 0 }
};

static const struct message listview_color_seq[] = {
    { LVM_SETBKCOLOR,     sent|lparam, 0, RGB(0,0,0) },
    { LVM_GETBKCOLOR,     sent },
    { LVM_SETTEXTCOLOR,   sent|lparam, 0, RGB(0,0,0) },
    { LVM_GETTEXTCOLOR,   sent },
    { LVM_SETTEXTBKCOLOR, sent|lparam, 0, RGB(0,0,0) },
    { LVM_GETTEXTBKCOLOR, sent },

    { LVM_SETBKCOLOR,     sent|lparam, 0, RGB(100,50,200) },
    { LVM_GETBKCOLOR,     sent },
    { LVM_SETTEXTCOLOR,   sent|lparam, 0, RGB(100,50,200) },
    { LVM_GETTEXTCOLOR,   sent },
    { LVM_SETTEXTBKCOLOR, sent|lparam, 0, RGB(100,50,200) },
    { LVM_GETTEXTBKCOLOR, sent },

    { LVM_SETBKCOLOR,     sent|lparam, 0, CLR_NONE },
    { LVM_GETBKCOLOR,     sent },
    { LVM_SETTEXTCOLOR,   sent|lparam, 0, CLR_NONE },
    { LVM_GETTEXTCOLOR,   sent },
    { LVM_SETTEXTBKCOLOR, sent|lparam, 0, CLR_NONE },
    { LVM_GETTEXTBKCOLOR, sent },

    { LVM_SETBKCOLOR,     sent|lparam, 0, RGB(255,255,255) },
    { LVM_GETBKCOLOR,     sent },
    { LVM_SETTEXTCOLOR,   sent|lparam, 0, RGB(255,255,255) },
    { LVM_GETTEXTCOLOR,   sent },
    { LVM_SETTEXTBKCOLOR, sent|lparam, 0, RGB(255,255,255) },
    { LVM_GETTEXTBKCOLOR, sent },
    { 0 }
};

static const struct message listview_item_count_seq[] = {
    { LVM_GETITEMCOUNT,   sent },
    { LVM_INSERTITEM,     sent },
    { LVM_INSERTITEM,     sent },
    { LVM_INSERTITEM,     sent },
    { LVM_GETITEMCOUNT,   sent },
    { LVM_DELETEITEM,     sent|wparam, 2 },
    { LVM_GETITEMCOUNT,   sent },
    { LVM_DELETEALLITEMS, sent },
    { LVM_GETITEMCOUNT,   sent },
    { LVM_INSERTITEM,     sent },
    { LVM_INSERTITEM,     sent },
    { LVM_GETITEMCOUNT,   sent },
    { LVM_INSERTITEM,     sent },
    { LVM_GETITEMCOUNT,   sent },
    { 0 }
};

static const struct message listview_itempos_seq[] = {
    { LVM_INSERTITEM,      sent },
    { LVM_INSERTITEM,      sent },
    { LVM_INSERTITEM,      sent },
    { LVM_SETITEMPOSITION, sent|wparam|lparam, 1, MAKELPARAM(10,5) },
    { LVM_GETITEMPOSITION, sent|wparam,        1 },
    { LVM_SETITEMPOSITION, sent|wparam|lparam, 2, MAKELPARAM(0,0) },
    { LVM_GETITEMPOSITION, sent|wparam,        2 },
    { LVM_SETITEMPOSITION, sent|wparam|lparam, 0, MAKELPARAM(20,20) },
    { LVM_GETITEMPOSITION, sent|wparam,        0 },
    { 0 }
};

struct subclass_info
{
    WNDPROC oldproc;
};

static LRESULT WINAPI parent_wnd_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static long defwndproc_counter = 0;
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

static BOOL register_parent_wnd_class(void)
{
    WNDCLASSA cls;

    cls.style = 0;
    cls.lpfnWndProc = parent_wnd_proc;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(NULL);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, (LPSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "Listview test parent class";
    return RegisterClassA(&cls);
}

static HWND create_parent_window(void)
{
    if (!register_parent_wnd_class())
        return NULL;

    return CreateWindowEx(0, "Listview test parent class",
                          "Listview test parent window",
                          WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX |
                          WS_MAXIMIZEBOX | WS_VISIBLE,
                          0, 0, 100, 100,
                          GetDesktopWindow(), NULL, GetModuleHandleA(NULL), NULL);
}

static LRESULT WINAPI listview_subclass_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    struct subclass_info *info = (struct subclass_info *)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    static long defwndproc_counter = 0;
    LRESULT ret;
    struct message msg;

    trace("listview: %p, %04x, %08lx, %08lx\n", hwnd, message, wParam, lParam);

    msg.message = message;
    msg.flags = sent|wparam|lparam;
    if (defwndproc_counter) msg.flags |= defwinproc;
    msg.wParam = wParam;
    msg.lParam = lParam;
    msg.id = LISTVIEW_ID;
    add_message(sequences, LISTVIEW_SEQ_INDEX, &msg);

    defwndproc_counter++;
    ret = CallWindowProcA(info->oldproc, hwnd, message, wParam, lParam);
    defwndproc_counter--;
    return ret;
}

static HWND create_listview_control(void)
{
    struct subclass_info *info;
    HWND hwnd;
    RECT rect;

    info = HeapAlloc(GetProcessHeap(), 0, sizeof(struct subclass_info));
    if (!info)
        return NULL;

    GetClientRect(hwndparent, &rect);
    hwnd = CreateWindowExA(0, WC_LISTVIEW, "foo",
                           WS_CHILD | WS_BORDER | WS_VISIBLE | LVS_REPORT,
                           0, 0, rect.right, rect.bottom,
                           hwndparent, NULL, GetModuleHandleA(NULL), NULL);
    ok(hwnd != NULL, "gle=%d\n", GetLastError());

    if (!hwnd)
    {
        HeapFree(GetProcessHeap(), 0, info);
        return NULL;
    }

    info->oldproc = (WNDPROC)SetWindowLongPtrA(hwnd, GWLP_WNDPROC,
                                            (LONG_PTR)listview_subclass_proc);
    SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)info);

    return hwnd;
}

static HWND create_custom_listview_control(DWORD style)
{
    struct subclass_info *info;
    HWND hwnd;
    RECT rect;

    info = HeapAlloc(GetProcessHeap(), 0, sizeof(struct subclass_info));
    if (!info)
        return NULL;

    GetClientRect(hwndparent, &rect);
    hwnd = CreateWindowExA(0, WC_LISTVIEW, "foo",
                           WS_CHILD | WS_BORDER | WS_VISIBLE | style,
                           0, 0, rect.right, rect.bottom,
                           hwndparent, NULL, GetModuleHandleA(NULL), NULL);
    ok(hwnd != NULL, "gle=%d\n", GetLastError());

    if (!hwnd)
    {
        HeapFree(GetProcessHeap(), 0, info);
        return NULL;
    }

    info->oldproc = (WNDPROC)SetWindowLongPtrA(hwnd, GWLP_WNDPROC,
                                            (LONG_PTR)listview_subclass_proc);
    SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)info);

    return hwnd;
}

static LRESULT WINAPI header_subclass_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    struct subclass_info *info = (struct subclass_info *)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    static long defwndproc_counter = 0;
    LRESULT ret;
    struct message msg;

    trace("header: %p, %04x, %08lx, %08lx\n", hwnd, message, wParam, lParam);

    msg.message = message;
    msg.flags = sent|wparam|lparam;
    if (defwndproc_counter) msg.flags |= defwinproc;
    msg.wParam = wParam;
    msg.lParam = lParam;
    msg.id = HEADER_ID;
    add_message(sequences, LISTVIEW_SEQ_INDEX, &msg);

    defwndproc_counter++;
    ret = CallWindowProcA(info->oldproc, hwnd, message, wParam, lParam);
    defwndproc_counter--;
    return ret;
}

static HWND subclass_header(HWND hwndListview)
{
    struct subclass_info *info;
    HWND hwnd;

    info = HeapAlloc(GetProcessHeap(), 0, sizeof(struct subclass_info));
    if (!info)
        return NULL;

    hwnd = ListView_GetHeader(hwndListview);
    info->oldproc = (WNDPROC)SetWindowLongPtrA(hwnd, GWLP_WNDPROC,
                                            (LONG_PTR)header_subclass_proc);
    SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)info);

    return hwnd;
}

static void test_images(void)
{
    HWND hwnd;
    DWORD r;
    LVITEM item;
    HIMAGELIST himl;
    HBITMAP hbmp;
    RECT r1, r2;
    static CHAR hello[] = "hello";

    himl = ImageList_Create(40, 40, 0, 4, 4);
    ok(himl != NULL, "failed to create imagelist\n");

    hbmp = CreateBitmap(40, 40, 1, 1, NULL);
    ok(hbmp != NULL, "failed to create bitmap\n");

    r = ImageList_Add(himl, hbmp, 0);
    ok(r == 0, "should be zero\n");

    hwnd = CreateWindowEx(0, "SysListView32", "foo", LVS_OWNERDRAWFIXED, 
                10, 10, 100, 200, hwndparent, NULL, NULL, NULL);
    ok(hwnd != NULL, "failed to create listview window\n");

    r = SendMessage(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, 0x940);
    ok(r == 0, "should return zero\n");

    r = SendMessage(hwnd, LVM_SETIMAGELIST, 0, (LPARAM)himl);
    ok(r == 0, "should return zero\n");

    r = SendMessage(hwnd, LVM_SETICONSPACING, 0, MAKELONG(100,50));
    /* returns dimensions */

    r = SendMessage(hwnd, LVM_GETITEMCOUNT, 0, 0);
    ok(r == 0, "should be zero items\n");

    item.mask = LVIF_IMAGE | LVIF_TEXT;
    item.iItem = 0;
    item.iSubItem = 1;
    item.iImage = 0;
    item.pszText = 0;
    r = SendMessage(hwnd, LVM_INSERTITEM, 0, (LPARAM) &item);
    ok(r == -1, "should fail\n");

    item.iSubItem = 0;
    item.pszText = hello;
    r = SendMessage(hwnd, LVM_INSERTITEM, 0, (LPARAM) &item);
    ok(r == 0, "should not fail\n");

    memset(&r1, 0, sizeof r1);
    r1.left = LVIR_ICON;
    r = SendMessage(hwnd, LVM_GETITEMRECT, 0, (LPARAM) &r1);

    r = SendMessage(hwnd, LVM_DELETEALLITEMS, 0, 0);
    ok(r == TRUE, "should not fail\n");

    item.iSubItem = 0;
    item.pszText = hello;
    r = SendMessage(hwnd, LVM_INSERTITEM, 0, (LPARAM) &item);
    ok(r == 0, "should not fail\n");

    memset(&r2, 0, sizeof r2);
    r2.left = LVIR_ICON;
    r = SendMessage(hwnd, LVM_GETITEMRECT, 0, (LPARAM) &r2);

    ok(!memcmp(&r1, &r2, sizeof r1), "rectangle should be the same\n");

    DestroyWindow(hwnd);
}

static void test_checkboxes(void)
{
    HWND hwnd;
    LVITEMA item;
    DWORD r;
    static CHAR text[]  = "Text",
                text2[] = "Text2",
                text3[] = "Text3";

    hwnd = CreateWindowEx(0, "SysListView32", "foo", LVS_REPORT, 
                10, 10, 100, 200, hwndparent, NULL, NULL, NULL);
    ok(hwnd != NULL, "failed to create listview window\n");

    /* first without LVS_EX_CHECKBOXES set and an item and check that state is preserved */
    item.mask = LVIF_TEXT | LVIF_STATE;
    item.stateMask = 0xffff;
    item.state = 0xfccc;
    item.iItem = 0;
    item.iSubItem = 0;
    item.pszText = text;
    r = SendMessage(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item);
    ok(r == 0, "ret %d\n", r);

    item.iItem = 0;
    item.mask = LVIF_STATE;
    item.stateMask = 0xffff;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    ok(item.state == 0xfccc, "state %x\n", item.state);

    /* Don't set LVIF_STATE */
    item.mask = LVIF_TEXT;
    item.stateMask = 0xffff;
    item.state = 0xfccc;
    item.iItem = 1;
    item.iSubItem = 0;
    item.pszText = text;
    r = SendMessage(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item);
    ok(r == 1, "ret %d\n", r);

    item.iItem = 1;
    item.mask = LVIF_STATE;
    item.stateMask = 0xffff;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    ok(item.state == 0, "state %x\n", item.state);

    r = SendMessage(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);
    ok(r == 0, "should return zero\n");

    /* Having turned on checkboxes, check that all existing items are set to 0x1000 (unchecked) */
    item.iItem = 0;
    item.mask = LVIF_STATE;
    item.stateMask = 0xffff;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    ok(item.state == 0x1ccc, "state %x\n", item.state);

    /* Now add an item without specifying a state and check that its state goes to 0x1000 */
    item.iItem = 2;
    item.mask = LVIF_TEXT;
    item.state = 0;
    item.pszText = text2;
    r = SendMessage(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item);
    ok(r == 2, "ret %d\n", r);

    item.iItem = 2;
    item.mask = LVIF_STATE;
    item.stateMask = 0xffff;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    ok(item.state == 0x1000, "state %x\n", item.state);

    /* Add a further item this time specifying a state and still its state goes to 0x1000 */
    item.iItem = 3;
    item.mask = LVIF_TEXT | LVIF_STATE;
    item.stateMask = 0xffff;
    item.state = 0x2aaa;
    item.pszText = text3;
    r = SendMessage(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item);
    ok(r == 3, "ret %d\n", r);

    item.iItem = 3;
    item.mask = LVIF_STATE;
    item.stateMask = 0xffff;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    ok(item.state == 0x1aaa, "state %x\n", item.state);

    /* Set an item's state to checked */
    item.iItem = 3;
    item.mask = LVIF_STATE;
    item.stateMask = 0xf000;
    item.state = 0x2000;
    r = SendMessage(hwnd, LVM_SETITEMA, 0, (LPARAM) &item);

    item.iItem = 3;
    item.mask = LVIF_STATE;
    item.stateMask = 0xffff;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    ok(item.state == 0x2aaa, "state %x\n", item.state);

    /* Check that only the bits we asked for are returned,
     * and that all the others are set to zero
     */
    item.iItem = 3;
    item.mask = LVIF_STATE;
    item.stateMask = 0xf000;
    item.state = 0xffff;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    ok(item.state == 0x2000, "state %x\n", item.state);

    /* Set the style again and check that doesn't change an item's state */
    r = SendMessage(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);
    ok(r == LVS_EX_CHECKBOXES, "ret %x\n", r);

    item.iItem = 3;
    item.mask = LVIF_STATE;
    item.stateMask = 0xffff;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    ok(item.state == 0x2aaa, "state %x\n", item.state);

    /* Unsetting the checkbox extended style doesn't change an item's state */
    r = SendMessage(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_CHECKBOXES, 0);
    ok(r == LVS_EX_CHECKBOXES, "ret %x\n", r);

    item.iItem = 3;
    item.mask = LVIF_STATE;
    item.stateMask = 0xffff;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    ok(item.state == 0x2aaa, "state %x\n", item.state);

    /* Now setting the style again will change an item's state */
    r = SendMessage(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);
    ok(r == 0, "ret %x\n", r);

    item.iItem = 3;
    item.mask = LVIF_STATE;
    item.stateMask = 0xffff;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    ok(item.state == 0x1aaa, "state %x\n", item.state);

    DestroyWindow(hwnd);
}

static void insert_column(HWND hwnd, int idx)
{
    LVCOLUMN column;
    DWORD rc;

    memset(&column, 0xaa, sizeof(column));
    column.mask = LVCF_SUBITEM;
    column.iSubItem = idx;

    rc = ListView_InsertColumn(hwnd, idx, &column);
    expect(idx, rc);
}

static void insert_item(HWND hwnd, int idx)
{
    static CHAR text[] = "foo";

    LVITEMA item;
    DWORD rc;

    memset(&item, 0xaa, sizeof (item));
    item.mask = LVIF_TEXT;
    item.iItem = idx;
    item.iSubItem = 0;
    item.pszText = text;

    rc = ListView_InsertItem(hwnd, &item);
    expect(idx, rc);
}

static void test_items(void)
{
    const LPARAM lparamTest = 0x42;
    HWND hwnd;
    LVITEMA item;
    DWORD r;
    static CHAR text[] = "Text";

    hwnd = CreateWindowEx(0, "SysListView32", "foo", LVS_REPORT,
                10, 10, 100, 200, hwndparent, NULL, NULL, NULL);
    ok(hwnd != NULL, "failed to create listview window\n");

    /*
     * Test setting/getting item params
     */

    /* Set up two columns */
    insert_column(hwnd, 0);
    insert_column(hwnd, 1);

    /* Insert an item with just a param */
    memset (&item, 0xaa, sizeof (item));
    item.mask = LVIF_PARAM;
    item.iItem = 0;
    item.iSubItem = 0;
    item.lParam = lparamTest;
    r = SendMessage(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item);
    ok(r == 0, "ret %d\n", r);

    /* Test getting of the param */
    memset (&item, 0xaa, sizeof (item));
    item.mask = LVIF_PARAM;
    item.iItem = 0;
    item.iSubItem = 0;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    ok(r != 0, "ret %d\n", r);
    ok(item.lParam == lparamTest, "got lParam %lx, expected %lx\n", item.lParam, lparamTest);

    /* Set up a subitem */
    memset (&item, 0xaa, sizeof (item));
    item.mask = LVIF_TEXT;
    item.iItem = 0;
    item.iSubItem = 1;
    item.pszText = text;
    r = SendMessage(hwnd, LVM_SETITEMA, 0, (LPARAM) &item);
    ok(r != 0, "ret %d\n", r);

    /* Query param from subitem: returns main item param */
    memset (&item, 0xaa, sizeof (item));
    item.mask = LVIF_PARAM;
    item.iItem = 0;
    item.iSubItem = 1;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    ok(r != 0, "ret %d\n", r);
    ok(item.lParam == lparamTest, "got lParam %lx, expected %lx\n", item.lParam, lparamTest);

    /* Set up param on first subitem: no effect */
    memset (&item, 0xaa, sizeof (item));
    item.mask = LVIF_PARAM;
    item.iItem = 0;
    item.iSubItem = 1;
    item.lParam = lparamTest+1;
    r = SendMessage(hwnd, LVM_SETITEMA, 0, (LPARAM) &item);
    ok(r == 0, "ret %d\n", r);

    /* Query param from subitem again: should still return main item param */
    memset (&item, 0xaa, sizeof (item));
    item.mask = LVIF_PARAM;
    item.iItem = 0;
    item.iSubItem = 1;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    ok(r != 0, "ret %d\n", r);
    ok(item.lParam == lparamTest, "got lParam %lx, expected %lx\n", item.lParam, lparamTest);

    /**** Some tests of state highlighting ****/
    memset (&item, 0xaa, sizeof (item));
    item.mask = LVIF_STATE;
    item.iItem = 0;
    item.iSubItem = 0;
    item.state = LVIS_SELECTED;
    item.stateMask = LVIS_SELECTED | LVIS_DROPHILITED;
    r = SendMessage(hwnd, LVM_SETITEM, 0, (LPARAM) &item);
    ok(r != 0, "ret %d\n", r);
    item.iSubItem = 1;
    item.state = LVIS_DROPHILITED;
    r = SendMessage(hwnd, LVM_SETITEM, 0, (LPARAM) &item);
    ok(r != 0, "ret %d\n", r);

    memset (&item, 0xaa, sizeof (item));
    item.mask = LVIF_STATE;
    item.iItem = 0;
    item.iSubItem = 0;
    item.stateMask = -1;
    r = SendMessage(hwnd, LVM_GETITEM, 0, (LPARAM) &item);
    ok(r != 0, "ret %d\n", r);
    ok(item.state == LVIS_SELECTED, "got state %x, expected %x\n", item.state, LVIS_SELECTED);
    item.iSubItem = 1;
    r = SendMessage(hwnd, LVM_GETITEM, 0, (LPARAM) &item);
    ok(r != 0, "ret %d\n", r);
    todo_wine ok(item.state == LVIS_DROPHILITED, "got state %x, expected %x\n", item.state, LVIS_DROPHILITED);

    DestroyWindow(hwnd);
}

static void test_columns(void)
{
    HWND hwnd;
    LVCOLUMN column;
    DWORD rc;

    hwnd = CreateWindowEx(0, "SysListView32", "foo", LVS_REPORT,
                10, 10, 100, 200, hwndparent, NULL, NULL, NULL);
    ok(hwnd != NULL, "failed to create listview window\n");

    /* Add a column with no mask */
    memset(&column, 0xaa, sizeof(column));
    column.mask = 0;
    rc = ListView_InsertColumn(hwnd, 0, &column);
    ok(rc==0, "Inserting column with no mask failed with %d\n", rc);

    /* Check its width */
    rc = ListView_GetColumnWidth(hwnd, 0);
    ok(rc==10, "Inserting column with no mask failed to set width to 10 with %d\n", rc);

    DestroyWindow(hwnd);
}
/* test setting imagelist between WM_NCCREATE and WM_CREATE */
static WNDPROC listviewWndProc;
static HIMAGELIST test_create_imagelist;

static LRESULT CALLBACK create_test_wndproc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_CREATE)
    {
        LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;
        lpcs->style |= LVS_REPORT;
        SendMessage(hwnd, LVM_SETIMAGELIST, 0, (LPARAM)test_create_imagelist);
    }
    return CallWindowProc(listviewWndProc, hwnd, uMsg, wParam, lParam);
}

static void test_create(void)
{
    HWND hList;
    HWND hHeader;
    WNDCLASSEX cls;
    cls.cbSize = sizeof(WNDCLASSEX);
    ok(GetClassInfoEx(GetModuleHandle(NULL), "SysListView32", &cls), "GetClassInfoEx failed\n");
    listviewWndProc = cls.lpfnWndProc;
    cls.lpfnWndProc = create_test_wndproc;
    cls.lpszClassName = "MyListView32";
    ok(RegisterClassEx(&cls), "RegisterClassEx failed\n");

    test_create_imagelist = ImageList_Create(16, 16, 0, 5, 10);
    hList = CreateWindow("MyListView32", "Test", WS_VISIBLE, 0, 0, 100, 100, NULL, NULL, GetModuleHandle(NULL), 0);
    ok((HIMAGELIST)SendMessage(hList, LVM_GETIMAGELIST, 0, 0) == test_create_imagelist, "Image list not obtained\n");
    hHeader = (HWND)SendMessage(hList, LVM_GETHEADER, 0, 0);
    ok(IsWindow(hHeader) && IsWindowVisible(hHeader), "Listview not in report mode\n");
    DestroyWindow(hList);
}

static void test_redraw(void)
{
    HWND hwnd, hwndheader;

    hwnd = create_listview_control();
    hwndheader = subclass_header(hwnd);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    trace("invalidate & update\n");
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);
    ok_sequence(sequences, LISTVIEW_SEQ_INDEX, redraw_listview_seq, "redraw listview", FALSE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    DestroyWindow(hwnd);
}

static LRESULT WINAPI cd_wndproc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    COLORREF clr, c0ffee = RGB(0xc0, 0xff, 0xee);

    if(msg == WM_NOTIFY) {
        NMHDR *nmhdr = (PVOID)lp;
        if(nmhdr->code == NM_CUSTOMDRAW) {
            NMLVCUSTOMDRAW *nmlvcd = (PVOID)nmhdr;
            trace("NMCUSTOMDRAW (0x%.8x)\n", nmlvcd->nmcd.dwDrawStage);
            switch(nmlvcd->nmcd.dwDrawStage) {
            case CDDS_PREPAINT:
                SetBkColor(nmlvcd->nmcd.hdc, c0ffee);
                return CDRF_NOTIFYITEMDRAW;
            case CDDS_ITEMPREPAINT:
                nmlvcd->clrTextBk = CLR_DEFAULT;
                return CDRF_NOTIFYSUBITEMDRAW;
            case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
                clr = GetBkColor(nmlvcd->nmcd.hdc);
                todo_wine ok(clr == c0ffee, "clr=%.8x\n", clr);
                return CDRF_NOTIFYPOSTPAINT;
            case CDDS_ITEMPOSTPAINT | CDDS_SUBITEM:
                clr = GetBkColor(nmlvcd->nmcd.hdc);
                todo_wine ok(clr == c0ffee, "clr=%.8x\n", clr);
                return CDRF_DODEFAULT;
            }
            return CDRF_DODEFAULT;
        }
    }

    return DefWindowProcA(hwnd, msg, wp, lp);
}

static void test_customdraw(void)
{
    HWND hwnd;
    WNDPROC oldwndproc;

    hwnd = create_listview_control();

    insert_column(hwnd, 0);
    insert_column(hwnd, 1);
    insert_item(hwnd, 0);

    oldwndproc = (WNDPROC)SetWindowLongPtr(hwndparent, GWLP_WNDPROC,
                                           (LONG_PTR)cd_wndproc);

    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);

    SetWindowLongPtr(hwndparent, GWLP_WNDPROC, (LONG_PTR)oldwndproc);

    DestroyWindow(hwnd);
}

static void test_icon_spacing(void)
{
    /* LVM_SETICONSPACING */
    /* note: LVM_SETICONSPACING returns the previous icon spacing if successful */
    /* note: the first test will fail if the default icon spacing is not (43,43) */

    HWND hwnd;
    DWORD r;

    hwnd = create_custom_listview_control(LVS_ICON);
    ok(hwnd != NULL, "failed to create a listview window\n");

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    trace("test icon spacing\n");
    todo_wine {
        r = SendMessage(hwnd, LVM_SETICONSPACING, 0, (LPARAM) MAKELONG(20, 30));
        expect(MAKELONG(43,43), r);
    }
        r = SendMessage(hwnd, LVM_SETICONSPACING, 0, (LPARAM) MAKELONG(25, 35));
        expect(MAKELONG(20,30), r);
        r = SendMessage(hwnd, LVM_SETICONSPACING, 0, (LPARAM) MAKELONG(-1,-1));
        expect(MAKELONG(25,35), r);

    ok_sequence(sequences, LISTVIEW_SEQ_INDEX, listview_icon_spacing_seq, "test icon spacing seq", FALSE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    DestroyWindow(hwnd);
}

static void test_color(void)
{
    /* SETBKCOLOR/GETBKCOLOR, SETTEXTCOLOR/GETTEXTCOLOR, SETTEXTBKCOLOR/GETTEXTBKCOLOR */

    HWND hwnd;
    DWORD r;
    int i;

    COLORREF color;
    COLORREF colors[4] = {RGB(0,0,0), RGB(100,50,200), CLR_NONE, RGB(255,255,255)};

    hwnd = create_listview_control();
    ok(hwnd != NULL, "failed to create a listview window\n");

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    trace("test color seq\n");
    for (i = 0; i < 4; i++)
    {
        color = colors[i];

        r = SendMessage(hwnd, LVM_SETBKCOLOR, 0, color);
        expect(TRUE, r);
        r = SendMessage(hwnd, LVM_GETBKCOLOR, 0, color);
        expect(color, r);

        r = SendMessage(hwnd, LVM_SETTEXTCOLOR, 0, color);
        expect (TRUE, r);
        r = SendMessage(hwnd, LVM_GETTEXTCOLOR, 0, color);
        expect(color, r);

        r = SendMessage(hwnd, LVM_SETTEXTBKCOLOR, 0, color);
        expect(TRUE, r);
        r = SendMessage(hwnd, LVM_GETTEXTBKCOLOR, 0, color);
        expect(color, r);
    }

    ok_sequence(sequences, LISTVIEW_SEQ_INDEX, listview_color_seq, "test color seq", FALSE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    DestroyWindow(hwnd);
}

static void test_item_count(void)
{
    /* LVM_INSERTITEM, LVM_DELETEITEM, LVM_DELETEALLITEMS, LVM_GETITEMCOUNT */

    HWND hwnd;
    DWORD r;

    LVITEM item0;
    LVITEM item1;
    LVITEM item2;
    static CHAR item0text[] = "item0";
    static CHAR item1text[] = "item1";
    static CHAR item2text[] = "item2";

    hwnd = create_listview_control();
    ok(hwnd != NULL, "failed to create a listview window\n");

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    trace("test item count\n");

    r = SendMessage(hwnd, LVM_GETITEMCOUNT, 0, 0);
    expect(0, r);

    /* [item0] */
    item0.mask = LVIF_TEXT;
    item0.iItem = 0;
    item0.iSubItem = 0;
    item0.pszText = item0text;
    r = SendMessage(hwnd, LVM_INSERTITEM, 0, (LPARAM) &item0);
    expect(0, r);

    /* [item0, item1] */
    item1.mask = LVIF_TEXT;
    item1.iItem = 1;
    item1.iSubItem = 0;
    item1.pszText = item1text;
    r = SendMessage(hwnd, LVM_INSERTITEM, 0, (LPARAM) &item1);
    expect(1, r);

    /* [item0, item1, item2] */
    item2.mask = LVIF_TEXT;
    item2.iItem = 2;
    item2.iSubItem = 0;
    item2.pszText = item2text;
    r = SendMessage(hwnd, LVM_INSERTITEM, 0, (LPARAM) &item2);
    expect(2, r);

    r = SendMessage(hwnd, LVM_GETITEMCOUNT, 0, 0);
    expect(3, r);

    /* [item0, item1] */
    r = SendMessage(hwnd, LVM_DELETEITEM, (WPARAM) 2, 0);
    expect(TRUE, r);

    r = SendMessage(hwnd, LVM_GETITEMCOUNT, 0, 0);
    expect(2, r);

    /* [] */
    r = SendMessage(hwnd, LVM_DELETEALLITEMS, 0, 0);
    expect(TRUE, r);

    r = SendMessage(hwnd, LVM_GETITEMCOUNT, 0, 0);
    expect(0, r);

    /* [item0] */
    r = SendMessage(hwnd, LVM_INSERTITEM, 0, (LPARAM) &item1);
    expect(0, r);

    /* [item0, item1] */
    r = SendMessage(hwnd, LVM_INSERTITEM, 0, (LPARAM) &item1);
    expect(1, r);

    r = SendMessage(hwnd, LVM_GETITEMCOUNT, 0, 0);
    expect(2, r);

    /* [item0, item1, item2] */
    r = SendMessage(hwnd, LVM_INSERTITEM, 0, (LPARAM) &item2);
    expect(2, r);

    r = SendMessage(hwnd, LVM_GETITEMCOUNT, 0, 0);
    expect(3, r);

    ok_sequence(sequences, LISTVIEW_SEQ_INDEX, listview_item_count_seq, "test item count seq", FALSE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    DestroyWindow(hwnd);
}

static void test_item_position(void)
{
    /* LVM_SETITEMPOSITION/LVM_GETITEMPOSITION */

    HWND hwnd;
    DWORD r;
    POINT position;

    LVITEM item0;
    LVITEM item1;
    LVITEM item2;
    static CHAR item0text[] = "item0";
    static CHAR item1text[] = "item1";
    static CHAR item2text[] = "item2";

    hwnd = create_custom_listview_control(LVS_ICON);
    ok(hwnd != NULL, "failed to create a listview window\n");

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    trace("test item position\n");

    /* [item0] */
    item0.mask = LVIF_TEXT;
    item0.iItem = 0;
    item0.iSubItem = 0;
    item0.pszText = item0text;
    r = SendMessage(hwnd, LVM_INSERTITEM, 0, (LPARAM) &item0);
    expect(0, r);

    /* [item0, item1] */
    item1.mask = LVIF_TEXT;
    item1.iItem = 1;
    item1.iSubItem = 0;
    item1.pszText = item1text;
    r = SendMessage(hwnd, LVM_INSERTITEM, 0, (LPARAM) &item1);
    expect(1, r);

    /* [item0, item1, item2] */
    item2.mask = LVIF_TEXT;
    item2.iItem = 2;
    item2.iSubItem = 0;
    item2.pszText = item2text;
    r = SendMessage(hwnd, LVM_INSERTITEM, 0, (LPARAM) &item2);
    expect(2, r);

    r = SendMessage(hwnd, LVM_SETITEMPOSITION, 1, MAKELPARAM(10,5));
    expect(TRUE, r);
    r = SendMessage(hwnd, LVM_GETITEMPOSITION, 1, (LPARAM) &position);
    expect(TRUE, r);
    expect2(10, 5, position.x, position.y);

    r = SendMessage(hwnd, LVM_SETITEMPOSITION, 2, MAKELPARAM(0,0));
    expect(TRUE, r);
    r = SendMessage(hwnd, LVM_GETITEMPOSITION, 2, (LPARAM) &position);
    expect(TRUE, r);
    expect2(0, 0, position.x, position.y);

    r = SendMessage(hwnd, LVM_SETITEMPOSITION, 0, MAKELPARAM(20,20));
    expect(TRUE, r);
    r = SendMessage(hwnd, LVM_GETITEMPOSITION, 0, (LPARAM) &position);
    expect(TRUE, r);
    expect2(20, 20, position.x, position.y);

    ok_sequence(sequences, LISTVIEW_SEQ_INDEX, listview_itempos_seq, "test item position seq", TRUE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    DestroyWindow(hwnd);
}

START_TEST(listview)
{
    INITCOMMONCONTROLSEX icc;

    icc.dwICC = 0;
    icc.dwSize = sizeof icc;
    InitCommonControlsEx(&icc);

    init_msg_sequences(sequences, NUM_MSG_SEQUENCES);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    hwndparent = create_parent_window();
    ok_sequence(sequences, PARENT_SEQ_INDEX, create_parent_wnd_seq, "create parent window", TRUE);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    test_images();
    test_checkboxes();
    test_items();
    test_create();
    test_redraw();
    test_customdraw();
    test_icon_spacing();
    test_color();
    test_item_count();
    test_item_position();
    test_columns();
}
