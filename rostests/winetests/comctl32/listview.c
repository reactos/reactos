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

#define PARENT_SEQ_INDEX       0
#define PARENT_FULL_SEQ_INDEX  1
#define LISTVIEW_SEQ_INDEX     2
#define NUM_MSG_SEQUENCES      3

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
    { WM_QUERYNEWPALETTE,   sent|optional },
    { WM_WINDOWPOSCHANGING, sent|wparam, 0 },
    { WM_WINDOWPOSCHANGED,  sent|optional },
    { WM_NCCALCSIZE,        sent|wparam|optional, 1 },
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
    { LVM_SETICONSPACING, sent|lparam, 0, MAKELPARAM(20, 30) },
    { LVM_SETICONSPACING, sent|lparam, 0, MAKELPARAM(25, 35) },
    { LVM_SETICONSPACING, sent|lparam, 0, MAKELPARAM(-1, -1) },
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

static const struct message listview_ownerdata_switchto_seq[] = {
    { WM_STYLECHANGING,    sent },
    { WM_STYLECHANGED,     sent },
    { 0 }
};

static const struct message listview_getorderarray_seq[] = {
    { LVM_GETCOLUMNORDERARRAY, sent|id|wparam, 2, 0, LISTVIEW_ID },
    { HDM_GETORDERARRAY,       sent|id|wparam, 2, 0, HEADER_ID },
    { 0 }
};

static const struct message empty_seq[] = {
    { 0 }
};

static const struct message forward_erasebkgnd_parent_seq[] = {
    { WM_ERASEBKGND, sent },
    { 0 }
};

struct subclass_info
{
    WNDPROC oldproc;
};

static LRESULT WINAPI parent_wnd_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static LONG defwndproc_counter = 0;
    LRESULT ret;
    struct message msg;

    msg.message = message;
    msg.flags = sent|wparam|lparam;
    if (defwndproc_counter) msg.flags |= defwinproc;
    msg.wParam = wParam;
    msg.lParam = lParam;

    /* log system messages, except for painting */
    if (message < WM_USER &&
        message != WM_PAINT &&
        message != WM_ERASEBKGND &&
        message != WM_NCPAINT &&
        message != WM_NCHITTEST &&
        message != WM_GETTEXT &&
        message != WM_GETICON &&
        message != WM_DEVICECHANGE)
    {
        trace("parent: %p, %04x, %08lx, %08lx\n", hwnd, message, wParam, lParam);

        add_message(sequences, PARENT_SEQ_INDEX, &msg);
    }
    add_message(sequences, PARENT_FULL_SEQ_INDEX, &msg);

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
    cls.hCursor = LoadCursorA(0, IDC_ARROW);
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
    static LONG defwndproc_counter = 0;
    LRESULT ret;
    struct message msg;

    trace("listview: %p, %04x, %08lx, %08lx\n", hwnd, message, wParam, lParam);

    /* some debug output for style changing */
    if ((message == WM_STYLECHANGING ||
         message == WM_STYLECHANGED) && lParam)
    {
        STYLESTRUCT *style = (STYLESTRUCT*)lParam;
        trace("\told style: 0x%08x, new style: 0x%08x\n", style->styleOld, style->styleNew);
    }

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

static HWND create_listview_control(DWORD style)
{
    struct subclass_info *info;
    HWND hwnd;
    RECT rect;

    info = HeapAlloc(GetProcessHeap(), 0, sizeof(struct subclass_info));
    if (!info)
        return NULL;

    GetClientRect(hwndparent, &rect);
    hwnd = CreateWindowExA(0, WC_LISTVIEW, "foo",
                           WS_CHILD | WS_BORDER | WS_VISIBLE | LVS_REPORT | style,
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
    static LONG defwndproc_counter = 0;
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

    /* Toggle checkbox tests (bug 9934) */
    memset (&item, 0xcc, sizeof(item));
    item.mask = LVIF_STATE;
    item.iItem = 3;
    item.iSubItem = 0;
    item.state = LVIS_FOCUSED;
    item.stateMask = LVIS_FOCUSED;
    r = SendMessage(hwnd, LVM_SETITEM, 0, (LPARAM) &item);
    expect(1, r);

    item.iItem = 3;
    item.mask = LVIF_STATE;
    item.stateMask = 0xffff;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    ok(item.state == 0x1aab, "state %x\n", item.state);

    r = SendMessage(hwnd, WM_KEYDOWN, VK_SPACE, 0);
    expect(0, r);
    r = SendMessage(hwnd, WM_KEYUP, VK_SPACE, 0);
    expect(0, r);

    item.iItem = 3;
    item.mask = LVIF_STATE;
    item.stateMask = 0xffff;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    ok(item.state == 0x2aab, "state %x\n", item.state);

    r = SendMessage(hwnd, WM_KEYDOWN, VK_SPACE, 0);
    expect(0, r);
    r = SendMessage(hwnd, WM_KEYUP, VK_SPACE, 0);
    expect(0, r);

    item.iItem = 3;
    item.mask = LVIF_STATE;
    item.stateMask = 0xffff;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    ok(item.state == 0x1aab, "state %x\n", item.state);

    DestroyWindow(hwnd);
}

static void insert_column(HWND hwnd, int idx)
{
    LVCOLUMN column;
    DWORD rc;

    memset(&column, 0xcc, sizeof(column));
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

    memset(&item, 0xcc, sizeof (item));
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

    /* LVIS_SELECTED with zero stateMask */
    /* set */
    memset (&item, 0, sizeof (item));
    item.mask = LVIF_STATE;
    item.state = LVIS_SELECTED;
    item.stateMask = 0;
    item.iItem = 0;
    item.iSubItem = 0;
    r = SendMessage(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item);
    ok(r == 0, "ret %d\n", r);
    /* get */
    memset (&item, 0xcc, sizeof (item));
    item.mask = LVIF_STATE;
    item.stateMask = LVIS_SELECTED;
    item.state = 0;
    item.iItem = 0;
    item.iSubItem = 0;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    ok(r != 0, "ret %d\n", r);
    ok(item.state & LVIS_SELECTED, "Expected LVIS_SELECTED\n");
    SendMessage(hwnd, LVM_DELETEITEM, 0, 0);

    /* LVIS_SELECTED with zero stateMask */
    /* set */
    memset (&item, 0, sizeof (item));
    item.mask = LVIF_STATE;
    item.state = LVIS_FOCUSED;
    item.stateMask = 0;
    item.iItem = 0;
    item.iSubItem = 0;
    r = SendMessage(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item);
    ok(r == 0, "ret %d\n", r);
    /* get */
    memset (&item, 0xcc, sizeof (item));
    item.mask = LVIF_STATE;
    item.stateMask = LVIS_FOCUSED;
    item.state = 0;
    item.iItem = 0;
    item.iSubItem = 0;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    ok(r != 0, "ret %d\n", r);
    ok(item.state & LVIS_FOCUSED, "Expected LVIS_FOCUSED\n");
    SendMessage(hwnd, LVM_DELETEITEM, 0, 0);

    /* LVIS_CUT with LVIS_FOCUSED stateMask */
    /* set */
    memset (&item, 0, sizeof (item));
    item.mask = LVIF_STATE;
    item.state = LVIS_CUT;
    item.stateMask = LVIS_FOCUSED;
    item.iItem = 0;
    item.iSubItem = 0;
    r = SendMessage(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item);
    ok(r == 0, "ret %d\n", r);
    /* get */
    memset (&item, 0xcc, sizeof (item));
    item.mask = LVIF_STATE;
    item.stateMask = LVIS_CUT;
    item.state = 0;
    item.iItem = 0;
    item.iSubItem = 0;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    ok(r != 0, "ret %d\n", r);
    ok(item.state & LVIS_CUT, "Expected LVIS_CUT\n");
    SendMessage(hwnd, LVM_DELETEITEM, 0, 0);

    /* Insert an item with just a param */
    memset (&item, 0xcc, sizeof (item));
    item.mask = LVIF_PARAM;
    item.iItem = 0;
    item.iSubItem = 0;
    item.lParam = lparamTest;
    r = SendMessage(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item);
    ok(r == 0, "ret %d\n", r);

    /* Test getting of the param */
    memset (&item, 0xcc, sizeof (item));
    item.mask = LVIF_PARAM;
    item.iItem = 0;
    item.iSubItem = 0;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    ok(r != 0, "ret %d\n", r);
    ok(item.lParam == lparamTest, "got lParam %lx, expected %lx\n", item.lParam, lparamTest);

    /* Set up a subitem */
    memset (&item, 0xcc, sizeof (item));
    item.mask = LVIF_TEXT;
    item.iItem = 0;
    item.iSubItem = 1;
    item.pszText = text;
    r = SendMessage(hwnd, LVM_SETITEMA, 0, (LPARAM) &item);
    ok(r != 0, "ret %d\n", r);

    /* Query param from subitem: returns main item param */
    memset (&item, 0xcc, sizeof (item));
    item.mask = LVIF_PARAM;
    item.iItem = 0;
    item.iSubItem = 1;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    ok(r != 0, "ret %d\n", r);
    ok(item.lParam == lparamTest, "got lParam %lx, expected %lx\n", item.lParam, lparamTest);

    /* Set up param on first subitem: no effect */
    memset (&item, 0xcc, sizeof (item));
    item.mask = LVIF_PARAM;
    item.iItem = 0;
    item.iSubItem = 1;
    item.lParam = lparamTest+1;
    r = SendMessage(hwnd, LVM_SETITEMA, 0, (LPARAM) &item);
    ok(r == 0, "ret %d\n", r);

    /* Query param from subitem again: should still return main item param */
    memset (&item, 0xcc, sizeof (item));
    item.mask = LVIF_PARAM;
    item.iItem = 0;
    item.iSubItem = 1;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    ok(r != 0, "ret %d\n", r);
    ok(item.lParam == lparamTest, "got lParam %lx, expected %lx\n", item.lParam, lparamTest);

    /**** Some tests of state highlighting ****/
    memset (&item, 0xcc, sizeof (item));
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

    memset (&item, 0xcc, sizeof (item));
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

    /* some notnull but meaningless masks */
    memset (&item, 0, sizeof(item));
    item.mask = LVIF_NORECOMPUTE;
    item.iItem = 0;
    item.iSubItem = 0;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    ok(r != 0, "ret %d\n", r);
    memset (&item, 0, sizeof(item));
    item.mask = LVIF_DI_SETITEM;
    item.iItem = 0;
    item.iSubItem = 0;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    ok(r != 0, "ret %d\n", r);

    DestroyWindow(hwnd);
}

static void test_columns(void)
{
    HWND hwnd, hwndheader;
    LVCOLUMN column;
    DWORD rc;
    INT order[2];

    hwnd = CreateWindowEx(0, "SysListView32", "foo", LVS_REPORT,
                10, 10, 100, 200, hwndparent, NULL, NULL, NULL);
    ok(hwnd != NULL, "failed to create listview window\n");

    /* Add a column with no mask */
    memset(&column, 0xcc, sizeof(column));
    column.mask = 0;
    rc = ListView_InsertColumn(hwnd, 0, &column);
    ok(rc==0, "Inserting column with no mask failed with %d\n", rc);

    /* Check its width */
    rc = ListView_GetColumnWidth(hwnd, 0);
    ok(rc==10 ||
       broken(rc==0), /* win9x */
       "Inserting column with no mask failed to set width to 10 with %d\n", rc);

    DestroyWindow(hwnd);

    /* LVM_GETCOLUMNORDERARRAY */
    hwnd = create_listview_control(0);
    hwndheader = subclass_header(hwnd);

    memset(&column, 0, sizeof(column));
    column.mask = LVCF_WIDTH;
    column.cx = 100;
    rc = ListView_InsertColumn(hwnd, 0, &column);
    ok(rc == 0, "Inserting column failed with %d\n", rc);

    column.cx = 200;
    rc = ListView_InsertColumn(hwnd, 1, &column);
    ok(rc == 1, "Inserting column failed with %d\n", rc);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    rc = SendMessage(hwnd, LVM_GETCOLUMNORDERARRAY, 2, (LPARAM)&order);
    ok(rc != 0, "Expected LVM_GETCOLUMNORDERARRAY to succeed\n");
    ok(order[0] == 0, "Expected order 0, got %d\n", order[0]);
    ok(order[1] == 1, "Expected order 1, got %d\n", order[1]);

    ok_sequence(sequences, LISTVIEW_SEQ_INDEX, listview_getorderarray_seq, "get order array", FALSE);

    DestroyWindow(hwnd);
}
/* test setting imagelist between WM_NCCREATE and WM_CREATE */
static WNDPROC listviewWndProc;
static HIMAGELIST test_create_imagelist;

static LRESULT CALLBACK create_test_wndproc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT ret;

    if (uMsg == WM_CREATE)
    {
        LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;
        lpcs->style |= LVS_REPORT;
    }
    ret = CallWindowProc(listviewWndProc, hwnd, uMsg, wParam, lParam);
    if (uMsg == WM_CREATE) SendMessage(hwnd, LVM_SETIMAGELIST, 0, (LPARAM)test_create_imagelist);
    return ret;
}

static void test_create(void)
{
    HWND hList;
    HWND hHeader;
    LONG_PTR ret;
    LONG r;
    LVCOLUMNA col;
    RECT rect;
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
    ok(hHeader == GetDlgItem(hList, 0), "Expected header as dialog item\n");
    DestroyWindow(hList);

    /* header isn't created on LVS_ICON and LVS_LIST styles */
    hList = CreateWindow("SysListView32", "Test", WS_VISIBLE, 0, 0, 100, 100, NULL, NULL,
                          GetModuleHandle(NULL), 0);
    hHeader = (HWND)SendMessage(hList, LVM_GETHEADER, 0, 0);
    ok(!IsWindow(hHeader), "Header shouldn't be created\n");
    ok(NULL == GetDlgItem(hList, 0), "NULL dialog item expected\n");
    /* insert column */
    memset(&col, 0, sizeof(LVCOLUMNA));
    col.mask = LVCF_WIDTH;
    col.cx = 100;
    r = SendMessage(hList, LVM_INSERTCOLUMN, 0, (LPARAM)&col);
    ok(r == 0, "Expected 0 column's inserted\n");
    hHeader = (HWND)SendMessage(hList, LVM_GETHEADER, 0, 0);
    ok(IsWindow(hHeader), "Header should be created\n");
    ok(hHeader == GetDlgItem(hList, 0), "Expected header as dialog item\n");
    DestroyWindow(hList);

    hList = CreateWindow("SysListView32", "Test", WS_VISIBLE|LVS_LIST, 0, 0, 100, 100, NULL, NULL,
                          GetModuleHandle(NULL), 0);
    hHeader = (HWND)SendMessage(hList, LVM_GETHEADER, 0, 0);
    ok(!IsWindow(hHeader), "Header shouldn't be created\n");
    ok(NULL == GetDlgItem(hList, 0), "NULL dialog item expected\n");
    /* insert column */
    memset(&col, 0, sizeof(LVCOLUMNA));
    col.mask = LVCF_WIDTH;
    col.cx = 100;
    r = SendMessage(hList, LVM_INSERTCOLUMN, 0, (LPARAM)&col);
    ok(r == 0, "Expected 0 column's inserted\n");
    hHeader = (HWND)SendMessage(hList, LVM_GETHEADER, 0, 0);
    ok(IsWindow(hHeader), "Header should be created\n");
    ok(hHeader == GetDlgItem(hList, 0), "Expected header as dialog item\n");
    DestroyWindow(hList);

    /* try to switch LVS_ICON -> LVS_REPORT and back LVS_ICON -> LVS_REPORT */
    hList = CreateWindow("SysListView32", "Test", WS_VISIBLE, 0, 0, 100, 100, NULL, NULL,
                          GetModuleHandle(NULL), 0);
    ret = SetWindowLongPtr(hList, GWL_STYLE, GetWindowLongPtr(hList, GWL_STYLE) | LVS_REPORT);
    ok(ret & WS_VISIBLE, "Style wrong, should have WS_VISIBLE\n");
    hHeader = (HWND)SendMessage(hList, LVM_GETHEADER, 0, 0);
    ok(IsWindow(hHeader), "Header should be created\n");
    ret = SetWindowLongPtr(hList, GWL_STYLE, GetWindowLong(hList, GWL_STYLE) & ~LVS_REPORT);
    ok((ret & WS_VISIBLE) && (ret & LVS_REPORT), "Style wrong, should have WS_VISIBLE|LVS_REPORT\n");
    hHeader = (HWND)SendMessage(hList, LVM_GETHEADER, 0, 0);
    ok(IsWindow(hHeader), "Header should be created\n");
    ok(hHeader == GetDlgItem(hList, 0), "Expected header as dialog item\n");
    DestroyWindow(hList);

    /* try to switch LVS_LIST -> LVS_REPORT and back LVS_LIST -> LVS_REPORT */
    hList = CreateWindow("SysListView32", "Test", WS_VISIBLE|LVS_LIST, 0, 0, 100, 100, NULL, NULL,
                          GetModuleHandle(NULL), 0);
    ret = SetWindowLongPtr(hList, GWL_STYLE,
                          (GetWindowLongPtr(hList, GWL_STYLE) & ~LVS_LIST) | LVS_REPORT);
    ok(((ret & WS_VISIBLE) && (ret & LVS_LIST)), "Style wrong, should have WS_VISIBLE|LVS_LIST\n");
    hHeader = (HWND)SendMessage(hList, LVM_GETHEADER, 0, 0);
    ok(IsWindow(hHeader), "Header should be created\n");
    ok(hHeader == GetDlgItem(hList, 0), "Expected header as dialog item\n");
    ret = SetWindowLongPtr(hList, GWL_STYLE,
                          (GetWindowLongPtr(hList, GWL_STYLE) & ~LVS_REPORT) | LVS_LIST);
    ok(((ret & WS_VISIBLE) && (ret & LVS_REPORT)), "Style wrong, should have WS_VISIBLE|LVS_REPORT\n");
    hHeader = (HWND)SendMessage(hList, LVM_GETHEADER, 0, 0);
    ok(IsWindow(hHeader), "Header should be created\n");
    ok(hHeader == GetDlgItem(hList, 0), "Expected header as dialog item\n");
    DestroyWindow(hList);

    /* LVS_REPORT without WS_VISIBLE */
    hList = CreateWindow("SysListView32", "Test", LVS_REPORT, 0, 0, 100, 100, NULL, NULL,
                          GetModuleHandle(NULL), 0);
    hHeader = (HWND)SendMessage(hList, LVM_GETHEADER, 0, 0);
    ok(!IsWindow(hHeader), "Header shouldn't be created\n");
    ok(NULL == GetDlgItem(hList, 0), "NULL dialog item expected\n");
    /* insert column */
    memset(&col, 0, sizeof(LVCOLUMNA));
    col.mask = LVCF_WIDTH;
    col.cx = 100;
    r = SendMessage(hList, LVM_INSERTCOLUMN, 0, (LPARAM)&col);
    ok(r == 0, "Expected 0 column's inserted\n");
    hHeader = (HWND)SendMessage(hList, LVM_GETHEADER, 0, 0);
    ok(IsWindow(hHeader), "Header should be created\n");
    ok(hHeader == GetDlgItem(hList, 0), "Expected header as dialog item\n");
    DestroyWindow(hList);

    /* LVS_REPORT without WS_VISIBLE, try to show it */
    hList = CreateWindow("SysListView32", "Test", LVS_REPORT, 0, 0, 100, 100, NULL, NULL,
                          GetModuleHandle(NULL), 0);
    hHeader = (HWND)SendMessage(hList, LVM_GETHEADER, 0, 0);
    ok(!IsWindow(hHeader), "Header shouldn't be created\n");
    ok(NULL == GetDlgItem(hList, 0), "NULL dialog item expected\n");
    ShowWindow(hList, SW_SHOW);
    hHeader = (HWND)SendMessage(hList, LVM_GETHEADER, 0, 0);
    ok(IsWindow(hHeader), "Header should be created\n");
    ok(hHeader == GetDlgItem(hList, 0), "Expected header as dialog item\n");
    DestroyWindow(hList);

    /* LVS_REPORT with LVS_NOCOLUMNHEADER */
    hList = CreateWindow("SysListView32", "Test", LVS_REPORT|LVS_NOCOLUMNHEADER|WS_VISIBLE,
                          0, 0, 100, 100, NULL, NULL, GetModuleHandle(NULL), 0);
    hHeader = (HWND)SendMessage(hList, LVM_GETHEADER, 0, 0);
    ok(IsWindow(hHeader), "Header should be created\n");
    ok(hHeader == GetDlgItem(hList, 0), "Expected header as dialog item\n");
    /* HDS_DRAGDROP set by default */
    ok(GetWindowLongPtr(hHeader, GWL_STYLE) & HDS_DRAGDROP, "Expected header to have HDS_DRAGDROP\n");
    DestroyWindow(hList);

    /* setting LVS_EX_HEADERDRAGDROP creates header */
    hList = CreateWindow("SysListView32", "Test", LVS_REPORT, 0, 0, 100, 100, NULL, NULL,
                          GetModuleHandle(NULL), 0);
    hHeader = (HWND)SendMessage(hList, LVM_GETHEADER, 0, 0);
    ok(!IsWindow(hHeader), "Header shouldn't be created\n");
    ok(NULL == GetDlgItem(hList, 0), "NULL dialog item expected\n");
    SendMessage(hList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_HEADERDRAGDROP);
    hHeader = (HWND)SendMessage(hList, LVM_GETHEADER, 0, 0);
    ok(IsWindow(hHeader), "Header should be created\n");
    ok(hHeader == GetDlgItem(hList, 0), "Expected header as dialog item\n");
    DestroyWindow(hList);

    /* not report style accepts LVS_EX_HEADERDRAGDROP too */
    hList = create_custom_listview_control(0);
    SendMessage(hList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_HEADERDRAGDROP);
    r = SendMessage(hList, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
    ok(r & LVS_EX_HEADERDRAGDROP, "Expected LVS_EX_HEADERDRAGDROP to be set\n");
    DestroyWindow(hList);

    /* requesting header info with LVM_GETSUBITEMRECT doesn't create it */
    hList = CreateWindow("SysListView32", "Test", LVS_REPORT, 0, 0, 100, 100, NULL, NULL,
                          GetModuleHandle(NULL), 0);
    ok(!IsWindow(hHeader), "Header shouldn't be created\n");
    ok(NULL == GetDlgItem(hList, 0), "NULL dialog item expected\n");

    rect.left = LVIR_BOUNDS;
    rect.top  = 1;
    rect.right = rect.bottom = -10;
    r = SendMessage(hList, LVM_GETSUBITEMRECT, -1, (LPARAM)&rect);
    ok(r != 0, "Expected not-null LRESULT\n");

    hHeader = (HWND)SendMessage(hList, LVM_GETHEADER, 0, 0);
    ok(!IsWindow(hHeader), "Header shouldn't be created\n");
    ok(NULL == GetDlgItem(hList, 0), "NULL dialog item expected\n");

    DestroyWindow(hList);
}

static void test_redraw(void)
{
    HWND hwnd, hwndheader;
    HDC hdc;
    BOOL res;
    DWORD r;

    hwnd = create_listview_control(0);
    hwndheader = subclass_header(hwnd);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    trace("invalidate & update\n");
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);
    ok_sequence(sequences, LISTVIEW_SEQ_INDEX, redraw_listview_seq, "redraw listview", FALSE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    /* forward WM_ERASEBKGND to parent on CLR_NONE background color */
    /* 1. Without backbuffer */
    res = ListView_SetBkColor(hwnd, CLR_NONE);
    expect(TRUE, res);

    hdc = GetWindowDC(hwndparent);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    r = SendMessageA(hwnd, WM_ERASEBKGND, (WPARAM)hdc, 0);
    ok(r != 0, "Expected not zero result\n");
    ok_sequence(sequences, PARENT_FULL_SEQ_INDEX, forward_erasebkgnd_parent_seq,
                "forward WM_ERASEBKGND on CLR_NONE", FALSE);

    res = ListView_SetBkColor(hwnd, CLR_DEFAULT);
    expect(TRUE, res);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    r = SendMessageA(hwnd, WM_ERASEBKGND, (WPARAM)hdc, 0);
    ok(r != 0, "Expected not zero result\n");
    ok_sequence(sequences, PARENT_FULL_SEQ_INDEX, empty_seq,
                "don't forward WM_ERASEBKGND on non-CLR_NONE", FALSE);

    /* 2. With backbuffer */
    SendMessageA(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_DOUBLEBUFFER,
                                                     LVS_EX_DOUBLEBUFFER);
    res = ListView_SetBkColor(hwnd, CLR_NONE);
    expect(TRUE, res);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    r = SendMessageA(hwnd, WM_ERASEBKGND, (WPARAM)hdc, 0);
    ok(r != 0, "Expected not zero result\n");
    ok_sequence(sequences, PARENT_FULL_SEQ_INDEX, forward_erasebkgnd_parent_seq,
                "forward WM_ERASEBKGND on CLR_NONE", FALSE);

    res = ListView_SetBkColor(hwnd, CLR_DEFAULT);
    expect(TRUE, res);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    r = SendMessageA(hwnd, WM_ERASEBKGND, (WPARAM)hdc, 0);
    todo_wine ok(r != 0, "Expected not zero result\n");
    ok_sequence(sequences, PARENT_FULL_SEQ_INDEX, empty_seq,
                "don't forward WM_ERASEBKGND on non-CLR_NONE", FALSE);

    ReleaseDC(hwndparent, hdc);

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

    hwnd = create_listview_control(0);

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

    HWND hwnd;
    WORD w, h;
    DWORD r;

    hwnd = create_custom_listview_control(LVS_ICON);
    ok(hwnd != NULL, "failed to create a listview window\n");

    r = SendMessage(hwnd, WM_NOTIFYFORMAT, (WPARAM)hwndparent, (LPARAM)NF_REQUERY);
    expect(NFR_ANSI, r);

    /* reset the icon spacing to defaults */
    SendMessage(hwnd, LVM_SETICONSPACING, 0, MAKELPARAM(-1, -1));

    /* now we can request what the defaults are */
    r = SendMessage(hwnd, LVM_SETICONSPACING, 0, MAKELPARAM(-1, -1));
    w = LOWORD(r);
    h = HIWORD(r);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    trace("test icon spacing\n");

    r = SendMessage(hwnd, LVM_SETICONSPACING, 0, MAKELPARAM(20, 30));
    ok(r == MAKELONG(w, h) ||
       broken(r == MAKELONG(w, w)), /* win98 */
       "Expected %d, got %d\n", MAKELONG(w, h), r);

    r = SendMessage(hwnd, LVM_SETICONSPACING, 0, MAKELPARAM(25, 35));
    expect(MAKELONG(20,30), r);

    r = SendMessage(hwnd, LVM_SETICONSPACING, 0, MAKELPARAM(-1,-1));
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

    hwnd = create_listview_control(0);
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

    hwnd = create_listview_control(0);
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
    r = SendMessage(hwnd, LVM_DELETEITEM, 2, 0);
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

static void test_getorigin(void)
{
    /* LVM_GETORIGIN */

    HWND hwnd;
    DWORD r;
    POINT position;

    position.x = position.y = 0;

    hwnd = create_custom_listview_control(LVS_ICON);
    ok(hwnd != NULL, "failed to create a listview window\n");
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    trace("test get origin results\n");
    r = SendMessage(hwnd, LVM_GETORIGIN, 0, (LPARAM)&position);
    expect(TRUE, r);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    DestroyWindow(hwnd);

    hwnd = create_custom_listview_control(LVS_SMALLICON);
    ok(hwnd != NULL, "failed to create a listview window\n");
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    trace("test get origin results\n");
    r = SendMessage(hwnd, LVM_GETORIGIN, 0, (LPARAM)&position);
    expect(TRUE, r);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    DestroyWindow(hwnd);

    hwnd = create_custom_listview_control(LVS_LIST);
    ok(hwnd != NULL, "failed to create a listview window\n");
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    trace("test get origin results\n");
    r = SendMessage(hwnd, LVM_GETORIGIN, 0, (LPARAM)&position);
    expect(FALSE, r);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    DestroyWindow(hwnd);

    hwnd = create_custom_listview_control(LVS_REPORT);
    ok(hwnd != NULL, "failed to create a listview window\n");
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    trace("test get origin results\n");
    r = SendMessage(hwnd, LVM_GETORIGIN, 0, (LPARAM)&position);
    expect(FALSE, r);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    DestroyWindow(hwnd);

}

static void test_multiselect(void)
{
    typedef struct t_select_task
    {
	const char *descr;
        int initPos;
        int loopVK;
        int count;
	int result;
    } select_task;

    HWND hwnd;
    DWORD r;
    int i,j,item_count,selected_count;
    static const int items=5;
    BYTE kstate[256];
    select_task task;
    LONG_PTR style;

    static struct t_select_task task_list[] = {
        { "using VK_DOWN", 0, VK_DOWN, -1, -1 },
        { "using VK_UP", -1, VK_UP, -1, -1 },
        { "using VK_END", 0, VK_END, 1, -1 },
        { "using VK_HOME", -1, VK_HOME, 1, -1 }
    };


    hwnd = create_listview_control(0);

    for (i=0;i<items;i++) {
	    insert_item(hwnd, 0);
    }

    item_count = (int)SendMessage(hwnd, LVM_GETITEMCOUNT, 0, 0);

    expect(items,item_count);

    for (i=0;i<4;i++) {
        task = task_list[i];

	/* deselect all items */
	ListView_SetItemState(hwnd, -1, 0, LVIS_SELECTED);
	SendMessage(hwnd, LVM_SETSELECTIONMARK, 0, -1);

	/* set initial position */
	SendMessage(hwnd, LVM_SETSELECTIONMARK, 0, (task.initPos == -1 ? item_count -1 : task.initPos));
	ListView_SetItemState(hwnd,(task.initPos == -1 ? item_count -1 : task.initPos),LVIS_SELECTED ,LVIS_SELECTED);

	selected_count = (int)SendMessage(hwnd, LVM_GETSELECTEDCOUNT, 0, 0);

	ok(selected_count == 1, "There should be only one selected item at the beginning (is %d)\n",selected_count);

	/* Set SHIFT key pressed */
        GetKeyboardState(kstate);
        kstate[VK_SHIFT]=0x80;
        SetKeyboardState(kstate);

	for (j=1;j<=(task.count == -1 ? item_count : task.count);j++) {
	    r = SendMessage(hwnd, WM_KEYDOWN, task.loopVK, 0);
	    expect(0,r);
	    r = SendMessage(hwnd, WM_KEYUP, task.loopVK, 0);
	    expect(0,r);
	}

	selected_count = (int)SendMessage(hwnd, LVM_GETSELECTEDCOUNT, 0, 0);

	ok((task.result == -1 ? item_count : task.result) == selected_count, "Failed multiple selection %s. There should be %d selected items (is %d)\n", task.descr, item_count, selected_count);

	/* Set SHIFT key released */
	GetKeyboardState(kstate);
        kstate[VK_SHIFT]=0x00;
        SetKeyboardState(kstate);
    }
    DestroyWindow(hwnd);

    /* make multiple selection, then switch to LVS_SINGLESEL */
    hwnd = create_listview_control(0);
    for (i=0;i<items;i++) {
	    insert_item(hwnd, 0);
    }
    item_count = (int)SendMessage(hwnd, LVM_GETITEMCOUNT, 0, 0);
    expect(items,item_count);
    /* deselect all items */
    ListView_SetItemState(hwnd, -1, 0, LVIS_SELECTED);
    SendMessage(hwnd, LVM_SETSELECTIONMARK, 0, -1);
    for (i=0;i<3;i++) {
        ListView_SetItemState(hwnd, i, LVIS_SELECTED, LVIS_SELECTED);
    }

    r = SendMessage(hwnd, LVM_GETSELECTEDCOUNT, 0, 0);
    expect(3, r);
    r = SendMessage(hwnd, LVM_GETSELECTIONMARK, 0, 0);
todo_wine
    expect(-1, r);

    style = GetWindowLongPtrA(hwnd, GWL_STYLE);
    ok(!(style & LVS_SINGLESEL), "LVS_SINGLESEL isn't expected\n");
    SetWindowLongPtrA(hwnd, GWL_STYLE, style | LVS_SINGLESEL);
    /* check that style is accepted */
    style = GetWindowLongPtrA(hwnd, GWL_STYLE);
    ok(style & LVS_SINGLESEL, "LVS_SINGLESEL expected\n");

    for (i=0;i<3;i++) {
        r = ListView_GetItemState(hwnd, i, LVIS_SELECTED);
        ok(r & LVIS_SELECTED, "Expected item %d to be selected\n", i);
    }
    r = SendMessage(hwnd, LVM_GETSELECTEDCOUNT, 0, 0);
    expect(3, r);
    SendMessage(hwnd, LVM_GETSELECTIONMARK, 0, 0);
    expect(3, r);

    /* select one more */
    ListView_SetItemState(hwnd, 3, LVIS_SELECTED, LVIS_SELECTED);

    for (i=0;i<3;i++) {
        r = ListView_GetItemState(hwnd, i, LVIS_SELECTED);
        ok(!(r & LVIS_SELECTED), "Expected item %d to be unselected\n", i);
    }
    r = ListView_GetItemState(hwnd, 3, LVIS_SELECTED);
    ok(r & LVIS_SELECTED, "Expected item %d to be selected\n", i);

    r = SendMessage(hwnd, LVM_GETSELECTEDCOUNT, 0, 0);
    expect(1, r);
    r = SendMessage(hwnd, LVM_GETSELECTIONMARK, 0, 0);
todo_wine
    expect(-1, r);

    DestroyWindow(hwnd);
}

static void test_subitem_rect(void)
{
    HWND hwnd;
    DWORD r;
    LVCOLUMN col;
    RECT rect;

    /* test LVM_GETSUBITEMRECT for header */
    hwnd = create_listview_control(0);
    ok(hwnd != NULL, "failed to create a listview window\n");
    /* add some columns */
    memset(&col, 0, sizeof(LVCOLUMN));
    col.mask = LVCF_WIDTH;
    col.cx = 100;
    r = -1;
    r = SendMessage(hwnd, LVM_INSERTCOLUMN, 0, (LPARAM)&col);
    expect(0, r);
    col.cx = 150;
    r = -1;
    r = SendMessage(hwnd, LVM_INSERTCOLUMN, 1, (LPARAM)&col);
    expect(1, r);
    col.cx = 200;
    r = -1;
    r = SendMessage(hwnd, LVM_INSERTCOLUMN, 2, (LPARAM)&col);
    expect(2, r);
    /* item = -1 means header, subitem index is 1 based */
    rect.left = LVIR_BOUNDS;
    rect.top  = 0;
    rect.right = rect.bottom = 0;
    r = SendMessage(hwnd, LVM_GETSUBITEMRECT, -1, (LPARAM)&rect);
    expect(0, r);

    rect.left = LVIR_BOUNDS;
    rect.top  = 1;
    rect.right = rect.bottom = 0;
    r = SendMessage(hwnd, LVM_GETSUBITEMRECT, -1, (LPARAM)&rect);

    ok(r != 0, "Expected not-null LRESULT\n");
    expect(100, rect.left);
    expect(250, rect.right);
todo_wine
    expect(3, rect.top);

    rect.left = LVIR_BOUNDS;
    rect.top  = 2;
    rect.right = rect.bottom = 0;
    r = SendMessage(hwnd, LVM_GETSUBITEMRECT, -1, (LPARAM)&rect);

    ok(r != 0, "Expected not-null LRESULT\n");
    expect(250, rect.left);
    expect(450, rect.right);
todo_wine
    expect(3, rect.top);

    DestroyWindow(hwnd);

    /* try it for non LVS_REPORT style */
    hwnd = CreateWindow("SysListView32", "Test", LVS_ICON, 0, 0, 100, 100, NULL, NULL,
                         GetModuleHandle(NULL), 0);
    rect.left = LVIR_BOUNDS;
    rect.top  = 1;
    rect.right = rect.bottom = -10;
    r = SendMessage(hwnd, LVM_GETSUBITEMRECT, -1, (LPARAM)&rect);
    ok(r == 0, "Expected not-null LRESULT\n");
    /* rect is unchanged */
    expect(0, rect.left);
    expect(-10, rect.right);
    expect(1, rect.top);
    expect(-10, rect.bottom);
    DestroyWindow(hwnd);
}

/* comparison callback for test_sorting */
static INT WINAPI test_CallBackCompare(LPARAM first, LPARAM second, LPARAM lParam)
{
    if (first == second) return 0;
    return (first > second ? 1 : -1);
}

static void test_sorting(void)
{
    HWND hwnd;
    LVITEMA item = {0};
    DWORD r;
    LONG_PTR style;
    static CHAR names[][5] = {"A", "B", "C", "D", "0"};
    CHAR buff[10];

    hwnd = create_listview_control(0);
    ok(hwnd != NULL, "failed to create a listview window\n");

    /* insert some items */
    item.mask = LVIF_PARAM | LVIF_STATE;
    item.state = LVIS_SELECTED;
    item.iItem = 0;
    item.iSubItem = 0;
    item.lParam = 3;
    r = SendMessage(hwnd, LVM_INSERTITEM, 0, (LPARAM) &item);
    expect(0, r);

    item.mask = LVIF_PARAM;
    item.iItem = 1;
    item.iSubItem = 0;
    item.lParam = 2;
    r = SendMessage(hwnd, LVM_INSERTITEM, 0, (LPARAM) &item);
    expect(1, r);

    item.mask = LVIF_STATE | LVIF_PARAM;
    item.state = LVIS_SELECTED;
    item.iItem = 2;
    item.iSubItem = 0;
    item.lParam = 4;
    r = SendMessage(hwnd, LVM_INSERTITEM, 0, (LPARAM) &item);
    expect(2, r);

    r = SendMessage(hwnd, LVM_GETSELECTIONMARK, 0, 0);
    expect(-1, r);

    r = SendMessage(hwnd, LVM_GETSELECTEDCOUNT, 0, 0);
    expect(2, r);

    r = SendMessage(hwnd, LVM_SORTITEMS, 0, (LPARAM)test_CallBackCompare);
    expect(TRUE, r);

    r = SendMessage(hwnd, LVM_GETSELECTEDCOUNT, 0, 0);
    expect(2, r);
    r = SendMessage(hwnd, LVM_GETSELECTIONMARK, 0, 0);
    expect(-1, r);
    r = SendMessage(hwnd, LVM_GETITEMSTATE, 0, LVIS_SELECTED);
    expect(0, r);
    r = SendMessage(hwnd, LVM_GETITEMSTATE, 1, LVIS_SELECTED);
    expect(LVIS_SELECTED, r);
    r = SendMessage(hwnd, LVM_GETITEMSTATE, 2, LVIS_SELECTED);
    expect(LVIS_SELECTED, r);

    DestroyWindow(hwnd);

    /* switch to LVS_SORTASCENDING when some items added */
    hwnd = create_listview_control(0);
    ok(hwnd != NULL, "failed to create a listview window\n");

    item.mask = LVIF_TEXT;
    item.iItem = 0;
    item.iSubItem = 0;
    item.pszText = names[1];
    r = SendMessage(hwnd, LVM_INSERTITEM, 0, (LPARAM) &item);
    expect(0, r);

    item.mask = LVIF_TEXT;
    item.iItem = 1;
    item.iSubItem = 0;
    item.pszText = names[2];
    r = SendMessage(hwnd, LVM_INSERTITEM, 0, (LPARAM) &item);
    expect(1, r);

    item.mask = LVIF_TEXT;
    item.iItem = 2;
    item.iSubItem = 0;
    item.pszText = names[0];
    r = SendMessage(hwnd, LVM_INSERTITEM, 0, (LPARAM) &item);
    expect(2, r);

    style = GetWindowLongPtrA(hwnd, GWL_STYLE);
    SetWindowLongPtrA(hwnd, GWL_STYLE, style | LVS_SORTASCENDING);
    style = GetWindowLongPtrA(hwnd, GWL_STYLE);
    ok(style & LVS_SORTASCENDING, "Expected LVS_SORTASCENDING to be set\n");

    /* no sorting performed when switched to LVS_SORTASCENDING */
    item.mask = LVIF_TEXT;
    item.iItem = 0;
    item.pszText = buff;
    item.cchTextMax = sizeof(buff);
    r = SendMessage(hwnd, LVM_GETITEM, 0, (LPARAM) &item);
    expect(TRUE, r);
    ok(lstrcmp(buff, names[1]) == 0, "Expected '%s', got '%s'\n", names[1], buff);

    item.iItem = 1;
    r = SendMessage(hwnd, LVM_GETITEM, 0, (LPARAM) &item);
    expect(TRUE, r);
    ok(lstrcmp(buff, names[2]) == 0, "Expected '%s', got '%s'\n", names[2], buff);

    item.iItem = 2;
    r = SendMessage(hwnd, LVM_GETITEM, 0, (LPARAM) &item);
    expect(TRUE, r);
    ok(lstrcmp(buff, names[0]) == 0, "Expected '%s', got '%s'\n", names[0], buff);

    /* adding new item doesn't resort list */
    item.mask = LVIF_TEXT;
    item.iItem = 3;
    item.iSubItem = 0;
    item.pszText = names[3];
    r = SendMessage(hwnd, LVM_INSERTITEM, 0, (LPARAM) &item);
    expect(3, r);

    item.mask = LVIF_TEXT;
    item.iItem = 0;
    item.pszText = buff;
    item.cchTextMax = sizeof(buff);
    r = SendMessage(hwnd, LVM_GETITEM, 0, (LPARAM) &item);
    expect(TRUE, r);
    ok(lstrcmp(buff, names[1]) == 0, "Expected '%s', got '%s'\n", names[1], buff);

    item.iItem = 1;
    r = SendMessage(hwnd, LVM_GETITEM, 0, (LPARAM) &item);
    expect(TRUE, r);
    ok(lstrcmp(buff, names[2]) == 0, "Expected '%s', got '%s'\n", names[2], buff);

    item.iItem = 2;
    r = SendMessage(hwnd, LVM_GETITEM, 0, (LPARAM) &item);
    expect(TRUE, r);
    ok(lstrcmp(buff, names[0]) == 0, "Expected '%s', got '%s'\n", names[0], buff);

    item.iItem = 3;
    r = SendMessage(hwnd, LVM_GETITEM, 0, (LPARAM) &item);
    expect(TRUE, r);
    ok(lstrcmp(buff, names[3]) == 0, "Expected '%s', got '%s'\n", names[3], buff);

    /* corner case - item should be placed at first position */
    item.mask = LVIF_TEXT;
    item.iItem = 4;
    item.iSubItem = 0;
    item.pszText = names[4];
    r = SendMessage(hwnd, LVM_INSERTITEM, 0, (LPARAM) &item);
    expect(0, r);

    item.iItem = 0;
    item.pszText = buff;
    item.cchTextMax = sizeof(buff);
    r = SendMessage(hwnd, LVM_GETITEM, 0, (LPARAM) &item);
    expect(TRUE, r);
    ok(lstrcmp(buff, names[4]) == 0, "Expected '%s', got '%s'\n", names[4], buff);

    item.iItem = 1;
    item.pszText = buff;
    item.cchTextMax = sizeof(buff);
    r = SendMessage(hwnd, LVM_GETITEM, 0, (LPARAM) &item);
    expect(TRUE, r);
    ok(lstrcmp(buff, names[1]) == 0, "Expected '%s', got '%s'\n", names[1], buff);

    item.iItem = 2;
    r = SendMessage(hwnd, LVM_GETITEM, 0, (LPARAM) &item);
    expect(TRUE, r);
    ok(lstrcmp(buff, names[2]) == 0, "Expected '%s', got '%s'\n", names[2], buff);

    item.iItem = 3;
    r = SendMessage(hwnd, LVM_GETITEM, 0, (LPARAM) &item);
    expect(TRUE, r);
    ok(lstrcmp(buff, names[0]) == 0, "Expected '%s', got '%s'\n", names[0], buff);

    item.iItem = 4;
    r = SendMessage(hwnd, LVM_GETITEM, 0, (LPARAM) &item);
    expect(TRUE, r);
    ok(lstrcmp(buff, names[3]) == 0, "Expected '%s', got '%s'\n", names[3], buff);

    DestroyWindow(hwnd);
}

static void test_ownerdata(void)
{
    HWND hwnd;
    LONG_PTR style, ret;
    DWORD res;
    LVITEMA item;

    /* it isn't possible to set LVS_OWNERDATA after creation */
    hwnd = create_listview_control(0);
    ok(hwnd != NULL, "failed to create a listview window\n");
    style = GetWindowLongPtrA(hwnd, GWL_STYLE);
    ok(!(style & LVS_OWNERDATA) && style, "LVS_OWNERDATA isn't expected\n");

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    ret = SetWindowLongPtrA(hwnd, GWL_STYLE, style | LVS_OWNERDATA);
    ok(ret == style, "Expected set GWL_STYLE to succeed\n");
    ok_sequence(sequences, LISTVIEW_SEQ_INDEX, listview_ownerdata_switchto_seq,
                "try to switch to LVS_OWNERDATA seq", FALSE);

    style = GetWindowLongPtrA(hwnd, GWL_STYLE);
    ok(!(style & LVS_OWNERDATA), "LVS_OWNERDATA isn't expected\n");
    DestroyWindow(hwnd);

    /* try to set LVS_OWNERDATA after creation just having it */
    hwnd = create_listview_control(LVS_OWNERDATA);
    ok(hwnd != NULL, "failed to create a listview window\n");
    style = GetWindowLongPtrA(hwnd, GWL_STYLE);
    ok(style & LVS_OWNERDATA, "LVS_OWNERDATA is expected\n");

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    ret = SetWindowLongPtrA(hwnd, GWL_STYLE, style | LVS_OWNERDATA);
    ok(ret == style, "Expected set GWL_STYLE to succeed\n");
    ok_sequence(sequences, LISTVIEW_SEQ_INDEX, listview_ownerdata_switchto_seq,
                "try to switch to LVS_OWNERDATA seq", FALSE);
    DestroyWindow(hwnd);

    /* try to remove LVS_OWNERDATA after creation just having it */
    hwnd = create_listview_control(LVS_OWNERDATA);
    ok(hwnd != NULL, "failed to create a listview window\n");
    style = GetWindowLongPtrA(hwnd, GWL_STYLE);
    ok(style & LVS_OWNERDATA, "LVS_OWNERDATA is expected\n");

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    ret = SetWindowLongPtrA(hwnd, GWL_STYLE, style & ~LVS_OWNERDATA);
    ok(ret == style, "Expected set GWL_STYLE to succeed\n");
    ok_sequence(sequences, LISTVIEW_SEQ_INDEX, listview_ownerdata_switchto_seq,
                "try to switch to LVS_OWNERDATA seq", FALSE);
    style = GetWindowLongPtrA(hwnd, GWL_STYLE);
    ok(style & LVS_OWNERDATA, "LVS_OWNERDATA is expected\n");
    DestroyWindow(hwnd);

    /* try select an item */
    hwnd = create_listview_control(LVS_OWNERDATA);
    ok(hwnd != NULL, "failed to create a listview window\n");
    res = SendMessageA(hwnd, LVM_SETITEMCOUNT, 1, 0);
    ok(res != 0, "Expected LVM_SETITEMCOUNT to succeed\n");
    res = SendMessageA(hwnd, LVM_GETSELECTEDCOUNT, 0, 0);
    expect(0, res);
    memset(&item, 0, sizeof(item));
    item.stateMask = LVIS_SELECTED;
    item.state     = LVIS_SELECTED;
    res = SendMessageA(hwnd, LVM_SETITEMSTATE, 0, (LPARAM)&item);
    expect(TRUE, res);
    res = SendMessageA(hwnd, LVM_GETSELECTEDCOUNT, 0, 0);
    expect(1, res);
    res = SendMessageA(hwnd, LVM_GETITEMCOUNT, 0, 0);
    expect(1, res);
    DestroyWindow(hwnd);

    /* LVM_SETITEM is unsupported on LVS_OWNERDATA */
    hwnd = create_listview_control(LVS_OWNERDATA);
    ok(hwnd != NULL, "failed to create a listview window\n");
    res = SendMessageA(hwnd, LVM_SETITEMCOUNT, 1, 0);
    ok(res != 0, "Expected LVM_SETITEMCOUNT to succeed\n");
    res = SendMessageA(hwnd, LVM_GETITEMCOUNT, 0, 0);
    expect(1, res);
    memset(&item, 0, sizeof(item));
    item.mask = LVIF_STATE;
    item.iItem = 0;
    item.stateMask = LVIS_SELECTED;
    item.state     = LVIS_SELECTED;
    res = SendMessageA(hwnd, LVM_SETITEM, 0, (LPARAM)&item);
    expect(FALSE, res);
    DestroyWindow(hwnd);
}

static void test_norecompute(void)
{
    static CHAR testA[] = "test";
    CHAR buff[10];
    LVITEMA item;
    HWND hwnd;
    DWORD res;

    /* self containing control */
    hwnd = create_listview_control(0);
    ok(hwnd != NULL, "failed to create a listview window\n");
    memset(&item, 0, sizeof(item));
    item.mask = LVIF_TEXT | LVIF_STATE;
    item.iItem = 0;
    item.stateMask = LVIS_SELECTED;
    item.state     = LVIS_SELECTED;
    item.pszText   = testA;
    res = SendMessageA(hwnd, LVM_INSERTITEM, 0, (LPARAM)&item);
    expect(0, res);
    /* retrieve with LVIF_NORECOMPUTE */
    item.mask  = LVIF_TEXT | LVIF_NORECOMPUTE;
    item.iItem = 0;
    item.pszText    = buff;
    item.cchTextMax = sizeof(buff)/sizeof(CHAR);
    res = SendMessageA(hwnd, LVM_GETITEM, 0, (LPARAM)&item);
    expect(TRUE, res);
    ok(lstrcmp(buff, testA) == 0, "Expected (%s), got (%s)\n", testA, buff);

    item.mask = LVIF_TEXT;
    item.iItem = 1;
    item.pszText = LPSTR_TEXTCALLBACK;
    res = SendMessageA(hwnd, LVM_INSERTITEM, 0, (LPARAM)&item);
    expect(1, res);

    item.mask  = LVIF_TEXT | LVIF_NORECOMPUTE;
    item.iItem = 1;
    item.pszText    = buff;
    item.cchTextMax = sizeof(buff)/sizeof(CHAR);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    res = SendMessageA(hwnd, LVM_GETITEM, 0, (LPARAM)&item);
    expect(TRUE, res);
    ok(item.pszText == LPSTR_TEXTCALLBACK, "Expected (%p), got (%p)\n",
       LPSTR_TEXTCALLBACK, (VOID*)item.pszText);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq, "retrieve with LVIF_NORECOMPUTE seq", FALSE);

    DestroyWindow(hwnd);

    /* LVS_OWNERDATA */
    hwnd = create_listview_control(LVS_OWNERDATA);
    ok(hwnd != NULL, "failed to create a listview window\n");

    item.mask = LVIF_STATE;
    item.stateMask = LVIS_SELECTED;
    item.state     = LVIS_SELECTED;
    item.iItem = 0;
    res = SendMessageA(hwnd, LVM_INSERTITEM, 0, (LPARAM)&item);
    expect(0, res);

    item.mask  = LVIF_TEXT | LVIF_NORECOMPUTE;
    item.iItem = 0;
    item.pszText    = buff;
    item.cchTextMax = sizeof(buff)/sizeof(CHAR);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    res = SendMessageA(hwnd, LVM_GETITEM, 0, (LPARAM)&item);
    expect(TRUE, res);
    ok(item.pszText == LPSTR_TEXTCALLBACK, "Expected (%p), got (%p)\n",
       LPSTR_TEXTCALLBACK, (VOID*)item.pszText);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq, "retrieve with LVIF_NORECOMPUTE seq 2", FALSE);

    DestroyWindow(hwnd);
}

static void test_nosortheader(void)
{
    HWND hwnd, header;
    LONG_PTR style;

    hwnd = create_listview_control(0);
    ok(hwnd != NULL, "failed to create a listview window\n");

    header = (HWND)SendMessageA(hwnd, LVM_GETHEADER, 0, 0);
    ok(IsWindow(header), "header expected\n");

    style = GetWindowLongPtr(header, GWL_STYLE);
    ok(style & HDS_BUTTONS, "expected header to have HDS_BUTTONS\n");

    style = GetWindowLongPtr(hwnd, GWL_STYLE);
    SetWindowLongPtr(hwnd, GWL_STYLE, style | LVS_NOSORTHEADER);
    /* HDS_BUTTONS retained */
    style = GetWindowLongPtr(header, GWL_STYLE);
    ok(style & HDS_BUTTONS, "expected header to retain HDS_BUTTONS\n");

    DestroyWindow(hwnd);

    /* create with LVS_NOSORTHEADER */
    hwnd = create_listview_control(LVS_NOSORTHEADER);
    ok(hwnd != NULL, "failed to create a listview window\n");

    header = (HWND)SendMessageA(hwnd, LVM_GETHEADER, 0, 0);
    ok(IsWindow(header), "header expected\n");

    style = GetWindowLongPtr(header, GWL_STYLE);
    ok(!(style & HDS_BUTTONS), "expected header to have no HDS_BUTTONS\n");

    style = GetWindowLongPtr(hwnd, GWL_STYLE);
    SetWindowLongPtr(hwnd, GWL_STYLE, style & ~LVS_NOSORTHEADER);
    /* not changed here */
    style = GetWindowLongPtr(header, GWL_STYLE);
    ok(!(style & HDS_BUTTONS), "expected header to have no HDS_BUTTONS\n");

    DestroyWindow(hwnd);
}

START_TEST(listview)
{
    HMODULE hComctl32;
    BOOL (WINAPI *pInitCommonControlsEx)(const INITCOMMONCONTROLSEX*);

    hComctl32 = GetModuleHandleA("comctl32.dll");
    pInitCommonControlsEx = (void*)GetProcAddress(hComctl32, "InitCommonControlsEx");
    if (pInitCommonControlsEx)
    {
        INITCOMMONCONTROLSEX iccex;
        iccex.dwSize = sizeof(iccex);
        iccex.dwICC  = ICC_LISTVIEW_CLASSES;
        pInitCommonControlsEx(&iccex);
    }
    else
        InitCommonControls();

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
    test_getorigin();
    test_multiselect();
    test_subitem_rect();
    test_sorting();
    test_ownerdata();
    test_norecompute();
    test_nosortheader();
}
