/* Unit test suite for list boxes.
 *
 * Copyright 2003 Ferenc Wagner
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
#include "winuser.h"
#include "winnls.h"
#include "commctrl.h"

#include "wine/test.h"
#include "v6util.h"
#include "msg.h"

enum seq_index
{
    LB_SEQ_INDEX,
    PARENT_SEQ_INDEX,
    NUM_MSG_SEQUENCES
};

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

    /* ignore window and other system events */
    if (object_id != OBJID_CLIENT) return;

    /* ignore events not from a listbox control */
    if (!GetClassNameA(hwnd, class_name, ARRAY_SIZE(class_name)) ||
        stricmp(class_name, WC_LISTBOXA) != 0)
        return;

    msg.message = event;
    msg.flags = winevent_hook|wparam|lparam;
    msg.wParam = object_id;
    msg.lParam = child_id;
    add_message(sequences, LB_SEQ_INDEX, &msg);
}

static void init_winevent_hook(void) {
    hwineventhook = SetWinEventHook(EVENT_MIN, EVENT_MAX, GetModuleHandleA(0), msg_winevent_proc,
                                    0, GetCurrentThreadId(), WINEVENT_INCONTEXT);
    if (!hwineventhook)
        win_skip( "no win event hook support\n" );
}

static void uninit_winevent_hook(void) {
    if (!hwineventhook)
        return;

    UnhookWinEvent(hwineventhook);
    hwineventhook = 0;
}

/* encoded MEASUREITEMSTRUCT into a WPARAM */
typedef struct
{
    union
    {
        struct
        {
            UINT CtlType : 4;
            UINT CtlID   : 4;
            UINT itemID  : 4;
            UINT wParam  : 20;
        } item;
        WPARAM wp;
    } u;
} MEASURE_ITEM_STRUCT;

static unsigned hash_Ly_W(const WCHAR *str)
{
    unsigned hash = 0;

    for (; *str; str++)
        hash = hash * 1664525u + (unsigned char)(*str) + 1013904223u;

    return hash;
}

static unsigned hash_Ly(const char *str)
{
    unsigned hash = 0;

    for (; *str; str++)
        hash = hash * 1664525u + (unsigned char)(*str) + 1013904223u;

    return hash;
}

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

static const char * const strings[4] = {
    "First added",
    "Second added",
    "Third added",
    "Fourth added which is very long because at some time we only had a 256 byte character buffer and "
      "that was overflowing in one of those applications that had a common dialog file open box and tried "
      "to add a 300 characters long custom filter string which of course the code did not like and crashed. "
      "Just make sure this string is longer than 256 characters."
};

static const char BAD_EXTENSION[] = "*.badtxt";

#define ID_LISTBOX 1

static LRESULT WINAPI listbox_wnd_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WNDPROC oldproc = (WNDPROC)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    static LONG defwndproc_counter = 0;
    struct message msg = { 0 };
    LRESULT ret;

    switch (message)
    {
        case WM_SIZE:
        case WM_GETTEXT:
        case WM_PAINT:
        case WM_ERASEBKGND:
        case WM_WINDOWPOSCHANGING:
        case WM_WINDOWPOSCHANGED:
        case WM_NCCALCSIZE:
        case WM_NCPAINT:
        case WM_NCHITTEST:
        case WM_DEVICECHANGE:
            break;

        default:
            msg.message = message;
            msg.flags = sent|wparam|lparam;
            if (defwndproc_counter) msg.flags |= defwinproc;
            msg.wParam = wParam;
            msg.lParam = lParam;
            add_message(sequences, LB_SEQ_INDEX, &msg);
    }

    defwndproc_counter++;
    ret = CallWindowProcA(oldproc, hwnd, message, wParam, lParam);
    defwndproc_counter--;

    return ret;
}

static HWND create_listbox(DWORD add_style, HWND parent)
{
    static const struct message addstring_seq[] =
    {
        { LB_ADDSTRING, sent },
        { WM_GETMINMAXINFO, sent|defwinproc|optional },
        { EVENT_OBJECT_CREATE, winevent_hook|wparam|lparam, OBJID_CLIENT, 1 },
        { LB_ADDSTRING, sent },
        { EVENT_OBJECT_CREATE, winevent_hook|wparam|lparam, OBJID_CLIENT, 2 },
        { LB_ADDSTRING, sent },
        { EVENT_OBJECT_CREATE, winevent_hook|wparam|lparam, OBJID_CLIENT, 3 },
        { LB_ADDSTRING, sent },
        { EVENT_OBJECT_CREATE, winevent_hook|wparam|lparam, OBJID_CLIENT, 4 },
        { 0 }
    };
    INT_PTR ctl_id = 0;
    WNDPROC oldproc;
    HWND handle;

    if (parent)
        ctl_id = ID_LISTBOX;

    handle = CreateWindowA(WC_LISTBOXA, "TestList", (LBS_STANDARD & ~LBS_SORT) | add_style, 0, 0, 100, 100,
        parent, (HMENU)ctl_id, NULL, 0);
    ok(handle != NULL, "Failed to create listbox window.\n");

    oldproc = (WNDPROC)SetWindowLongPtrA(handle, GWLP_WNDPROC, (LONG_PTR)listbox_wnd_proc);
    SetWindowLongPtrA(handle, GWLP_USERDATA, (LONG_PTR)oldproc);

    flush_sequence(sequences, LB_SEQ_INDEX);
    SendMessageA(handle, LB_ADDSTRING, 0, (LPARAM) strings[0]);
    SendMessageA(handle, LB_ADDSTRING, 0, (LPARAM) strings[1]);
    SendMessageA(handle, LB_ADDSTRING, 0, (LPARAM) strings[2]);
    SendMessageA(handle, LB_ADDSTRING, 0, (LPARAM) strings[3]);
    ok_sequence(sequences, LB_SEQ_INDEX, addstring_seq, "addstring_seq", FALSE);

    return handle;
}

struct listbox_prop
{
    DWORD add_style;
};

struct listbox_stat
{
    int selected, anchor, caret, selcount;
};

struct listbox_test
{
    struct listbox_stat  init,  init_todo;
    struct listbox_stat click, click_todo;
    struct listbox_stat  step,  step_todo;
    struct listbox_stat   sel,   sel_todo;
};

static void listbox_query(HWND handle, struct listbox_stat *results)
{
    results->selected = SendMessageA(handle, LB_GETCURSEL, 0, 0);
    results->anchor   = SendMessageA(handle, LB_GETANCHORINDEX, 0, 0);
    results->caret    = SendMessageA(handle, LB_GETCARETINDEX, 0, 0);
    results->selcount = SendMessageA(handle, LB_GETSELCOUNT, 0, 0);
}

static void buttonpress(HWND handle, WORD x, WORD y)
{
    LPARAM lp = x + (y << 16);

    SendMessageA(handle, WM_LBUTTONDOWN, MK_LBUTTON, lp);
    SendMessageA(handle, WM_LBUTTONUP, 0, lp);
}

static void keypress(HWND handle, WPARAM keycode, BYTE scancode, BOOL extended)
{
    LPARAM lp = 1 + (scancode << 16) + (extended ? KEYEVENTF_EXTENDEDKEY : 0);

    SendMessageA(handle, WM_KEYDOWN, keycode, lp);
    SendMessageA(handle, WM_KEYUP  , keycode, lp | 0xc000000);
}

#define listbox_field_ok(t, s, f, got) \
  ok (t.s.f==got.f, "style %#lx, step " #s ", field " #f \
      ": expected %d, got %d\n", style, t.s.f, got.f)

#define listbox_todo_field_ok(t, s, f, got) \
  todo_wine_if (t.s##_todo.f) { listbox_field_ok(t, s, f, got); }

#define listbox_ok(t, s, got) \
  listbox_todo_field_ok(t, s, selected, got); \
  listbox_todo_field_ok(t, s, anchor, got); \
  listbox_todo_field_ok(t, s, caret, got); \
  listbox_todo_field_ok(t, s, selcount, got)

static void run_test(DWORD style, const struct listbox_test test)
{
    static const struct message delete_seq[] =
    {
        { LB_DELETESTRING, sent|wparam|lparam, 0, 0 },
        { EVENT_OBJECT_DESTROY, winevent_hook|wparam|lparam, OBJID_CLIENT, 1 },
        { LB_DELETESTRING, sent|wparam|lparam, 0, 0 },
        { EVENT_OBJECT_DESTROY, winevent_hook|wparam|lparam, OBJID_CLIENT, 1 },
        { LB_DELETESTRING, sent|wparam|lparam, 0, 0 },
        { EVENT_OBJECT_DESTROY, winevent_hook|wparam|lparam, OBJID_CLIENT, 1 },
        { LB_DELETESTRING, sent|wparam|lparam, 0, 0 },
        { EVENT_OBJECT_DESTROY, winevent_hook|wparam|lparam, OBJID_CLIENT, 1 },
        { LB_RESETCONTENT, sent|wparam|lparam|defwinproc, 0, 0 },
        { 0 }
    };
    struct listbox_stat answer;
    int i, res, count;
    RECT second_item;
    HWND hLB;

    hLB = create_listbox (style, 0);

    listbox_query (hLB, &answer);
    listbox_ok (test, init, answer);

    SendMessageA(hLB, LB_GETITEMRECT, 1, (LPARAM) &second_item);
    buttonpress(hLB, (WORD)second_item.left, (WORD)second_item.top);

    listbox_query(hLB, &answer);
    listbox_ok(test, click, answer);

    keypress(hLB, VK_DOWN, 0x50, TRUE);

    listbox_query(hLB, &answer);
    listbox_ok(test, step, answer);

    DestroyWindow(hLB);

    hLB = create_listbox(style, 0);

    SendMessageA(hLB, LB_SELITEMRANGE, TRUE, MAKELPARAM(1, 2));
    listbox_query(hLB, &answer);
    listbox_ok(test, sel, answer);

    for (i = 0; i < 4 && !(style & LBS_NODATA); i++)
    {
        DWORD size = SendMessageA(hLB, LB_GETTEXTLEN, i, 0);
        int resA, resW;
        WCHAR *txtw;
        CHAR *txt;

        txt = calloc(1, size + 1);
        resA = SendMessageA(hLB, LB_GETTEXT, i, (LPARAM)txt);
        ok(!strcmp(txt, strings[i]), "returned string for item %d does not match %s vs %s\n", i, txt, strings[i]);

        txtw = calloc(size + 1, sizeof(*txtw));
        resW = SendMessageW(hLB, LB_GETTEXT, i, (LPARAM)txtw);
        ok(resA == resW, "Unexpected text length.\n");
        WideCharToMultiByte(CP_ACP, 0, txtw, -1, txt, size, NULL, NULL);
        ok(!strcmp (txt, strings[i]), "Unexpected string for item %d, %s vs %s.\n", i, txt, strings[i]);

        free(txtw);
        free(txt);
    }

    /* Confirm the count of items, and that an invalid delete does not remove anything */
    res = SendMessageA(hLB, LB_GETCOUNT, 0, 0);
    ok(res == 4, "Expected 4 items, got %d\n", res);
    res = SendMessageA(hLB, LB_DELETESTRING, -1, 0);
    ok(res == LB_ERR, "Expected LB_ERR items, got %d\n", res);
    res = SendMessageA(hLB, LB_DELETESTRING, 4, 0);
    ok(res == LB_ERR, "Expected LB_ERR items, got %d\n", res);
    count = SendMessageA(hLB, LB_GETCOUNT, 0, 0);
    ok(count == 4, "Unexpected item count %d.\n", count);

    /* Emptying listbox sends a LB_RESETCONTENT to itself. */
    flush_sequence(sequences, LB_SEQ_INDEX);
    for (i = count; i--;)
    {
        res = SendMessageA(hLB, LB_DELETESTRING, 0, 0);
        ok(res == i, "Unexpected return value %d.\n", res);
    }
    ok_sequence(sequences, LB_SEQ_INDEX, delete_seq, "Emptying listbox", FALSE);

    DestroyWindow(hLB);
}

static void test_item_height(void)
{
    INT itemHeight;
    TEXTMETRICA tm;
    HFONT font;
    HWND hLB;
    HDC hdc;

    hLB = create_listbox (0, 0);
    ok ((hdc = GetDCEx( hLB, 0, DCX_CACHE )) != 0, "Can't get hdc\n");
    ok ((font = GetCurrentObject(hdc, OBJ_FONT)) != 0, "Can't get the current font\n");
    ok (GetTextMetricsA( hdc, &tm ), "Can't read font metrics\n");
    ReleaseDC( hLB, hdc);

    ok (SendMessageA(hLB, WM_SETFONT, (WPARAM)font, 0) == 0, "Can't set font\n");

    itemHeight = SendMessageA(hLB, LB_GETITEMHEIGHT, 0, 0);
    ok (itemHeight == tm.tmHeight, "Item height wrong, got %d, expecting %ld\n", itemHeight, tm.tmHeight);

    DestroyWindow (hLB);

    hLB = CreateWindowA(WC_LISTBOXA, "TestList", LBS_OWNERDRAWVARIABLE,  0, 0, 100, 100, NULL, NULL, NULL, 0);

    itemHeight = SendMessageA(hLB, LB_GETITEMHEIGHT, 0, 0);
    ok(itemHeight > 0 && itemHeight <= tm.tmHeight, "Unexpected item height %d, expected %ld.\n",
        itemHeight, tm.tmHeight);
    itemHeight = SendMessageA(hLB, LB_GETITEMHEIGHT, 5, 0);
    ok(itemHeight > 0 && itemHeight <= tm.tmHeight, "Unexpected item height %d, expected %ld.\n",
        itemHeight, tm.tmHeight);
    itemHeight = SendMessageA(hLB, LB_GETITEMHEIGHT, -5, 0);
    ok(itemHeight > 0 && itemHeight <= tm.tmHeight, "Unexpected item height %d, expected %ld.\n",
        itemHeight, tm.tmHeight);

    DestroyWindow (hLB);
}

static unsigned int got_selchange, got_drawitem;

static LRESULT WINAPI main_window_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static LONG defwndproc_counter = 0;
    struct message m = { 0 };
    LRESULT ret;

    m.message = msg;
    m.flags = sent|wparam|lparam;
    if (defwndproc_counter) m.flags |= defwinproc;
    m.wParam = wParam;
    m.lParam = lParam;

    switch (msg)
    {
    case WM_MEASUREITEM:
    {
        MEASUREITEMSTRUCT *mis = (void *)lParam;
        BOOL is_unicode_data = FALSE;
        MEASURE_ITEM_STRUCT mi;

        if (mis->CtlType == ODT_LISTBOX)
        {
            HWND ctrl = GetDlgItem(hwnd, mis->CtlID);
            is_unicode_data = GetWindowLongA(ctrl, GWL_STYLE) & LBS_HASSTRINGS;
        }

        mi.u.wp = 0;
        mi.u.item.CtlType = mis->CtlType;
        mi.u.item.CtlID = mis->CtlID;
        mi.u.item.itemID = mis->itemID;
        mi.u.item.wParam = wParam;

        m.wParam = mi.u.wp;
        if (is_unicode_data)
            m.lParam = mis->itemData ? hash_Ly_W((const WCHAR *)mis->itemData) : 0;
        else
            m.lParam = mis->itemData ? hash_Ly((const char *)mis->itemData) : 0;
        add_message(sequences, PARENT_SEQ_INDEX, &m);

        ok(wParam == mis->CtlID, "got wParam=%08Ix, expected %08x\n", wParam, mis->CtlID);
        ok(mis->CtlType == ODT_LISTBOX, "mi->CtlType = %u\n", mis->CtlType);
        ok(mis->CtlID == 1, "mi->CtlID = %u\n", mis->CtlID);
        ok(mis->itemHeight, "mi->itemHeight = 0\n");

        break;
    }
    case WM_COMPAREITEM:
    {
        COMPAREITEMSTRUCT *cis = (COMPAREITEMSTRUCT *)lParam;
        HWND ctrl = GetDlgItem(hwnd, cis->CtlID);
        BOOL is_unicode_data = TRUE;

        ok(wParam == cis->CtlID, "expected %#x, got %#Ix\n", cis->CtlID, wParam);
        ok(cis->hwndItem == ctrl, "expected %p, got %p\n", ctrl, cis->hwndItem);
        ok((int)cis->itemID1 >= 0, "expected >= 0, got %d\n", cis->itemID1);
        ok((int)cis->itemID2 == -1, "expected -1, got %d\n", cis->itemID2);

        if (cis->CtlType == ODT_LISTBOX)
            is_unicode_data = GetWindowLongA(ctrl, GWL_STYLE) & LBS_HASSTRINGS;

        if (is_unicode_data)
        {
            m.wParam = cis->itemData1 ? hash_Ly_W((const WCHAR *)cis->itemData1) : 0;
            m.lParam = cis->itemData2 ? hash_Ly_W((const WCHAR *)cis->itemData2) : 0;
        }
        else
        {
            m.wParam = cis->itemData1 ? hash_Ly((const char *)cis->itemData1) : 0;
            m.lParam = cis->itemData2 ? hash_Ly((const char *)cis->itemData2) : 0;
        }
        add_message(sequences, PARENT_SEQ_INDEX, &m);
        break;
    }
    case WM_DRAWITEM:
    {
        RECT rc_item, rc_client, rc_clip;
        DRAWITEMSTRUCT *dis = (DRAWITEMSTRUCT *)lParam;

        ok(wParam == dis->CtlID, "got wParam=%08Ix instead of %08x\n", wParam, dis->CtlID);
        ok(dis->CtlType == ODT_LISTBOX, "wrong CtlType %04x\n", dis->CtlType);

        GetClientRect(dis->hwndItem, &rc_client);
        GetClipBox(dis->hDC, &rc_clip);
        ok(EqualRect(&rc_client, &rc_clip) || IsRectEmpty(&rc_clip),
           "client rect of the listbox should be equal to the clip box,"
           "or the clip box should be empty\n");

        SendMessageA(dis->hwndItem, LB_GETITEMRECT, dis->itemID, (LPARAM)&rc_item);
        ok(EqualRect(&dis->rcItem, &rc_item), "item rects are not equal\n");

        got_drawitem++;
        break;
    }

    case WM_COMMAND:
        if (HIWORD( wParam ) == LBN_SELCHANGE) got_selchange++;
        break;

    default:
        break;
    }

    defwndproc_counter++;
    ret = DefWindowProcA(hwnd, msg, wParam, lParam);
    defwndproc_counter--;

    return msg == WM_COMPAREITEM ? -1 : ret;
}

static HWND create_parent( void )
{
    static ATOM class;
    WNDCLASSA cls;

    if (!class)
    {
        cls.style = 0;
        cls.lpfnWndProc = main_window_proc;
        cls.cbClsExtra = 0;
        cls.cbWndExtra = 0;
        cls.hInstance = GetModuleHandleA(NULL);
        cls.hIcon = 0;
        cls.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
        cls.hbrBackground = GetStockObject(WHITE_BRUSH);
        cls.lpszMenuName = NULL;
        cls.lpszClassName = "main_window_class";
        class = RegisterClassA( &cls );
    }

    return CreateWindowExA(0, "main_window_class", NULL, WS_POPUP | WS_VISIBLE, 100, 100, 400, 400, GetDesktopWindow(),
        0, GetModuleHandleA(NULL), NULL);
}

static void test_ownerdraw(void)
{
    static const DWORD styles[] =
    {
        0,
        LBS_NODATA
    };
    static const struct {
        UINT message;
        WPARAM wparam;
        LPARAM lparam;
        UINT drawitem;
    } testcase[] = {
        { WM_NULL, 0, 0, 0 },
        { WM_PAINT, 0, 0, 0 },
        { LB_GETCOUNT, 0, 0, 0 },
        { LB_SETCOUNT, ARRAY_SIZE(strings), 0, ARRAY_SIZE(strings) },
        { LB_ADDSTRING, 0, (LPARAM)"foo", ARRAY_SIZE(strings)+1 },
        { LB_DELETESTRING, 0, 0, ARRAY_SIZE(strings)-1 },
    };
    HWND parent, hLB;
    INT ret;
    RECT rc;
    UINT i;

    parent = create_parent();
    ok(parent != NULL, "Failed to create parent window.\n");

    for (i = 0; i < ARRAY_SIZE(styles); i++)
    {
        hLB = create_listbox(LBS_OWNERDRAWFIXED | WS_CHILD | WS_VISIBLE | styles[i], parent);
        ok(hLB != NULL, "Failed to create listbox window.\n");

        SetForegroundWindow(hLB);
        UpdateWindow(hLB);

        /* make height short enough */
        SendMessageA(hLB, LB_GETITEMRECT, 0, (LPARAM)&rc);
        SetWindowPos(hLB, 0, 0, 0, 100, rc.bottom - rc.top + 1, SWP_NOZORDER | SWP_NOMOVE);

        /* make 0 item invisible */
        SendMessageA(hLB, LB_SETTOPINDEX, 1, 0);
        ret = SendMessageA(hLB, LB_GETTOPINDEX, 0, 0);
        ok(ret == 1, "wrong top index %d\n", ret);

        SendMessageA(hLB, LB_GETITEMRECT, 0, (LPARAM)&rc);
        ok(!IsRectEmpty(&rc), "empty item rect\n");
        ok(rc.top < 0, "rc.top is not negative (%ld)\n", rc.top);

        DestroyWindow(hLB);

        /* Both FIXED and VARIABLE, FIXED should override VARIABLE. */
        hLB = CreateWindowA(WC_LISTBOXA, "TestList", LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE | styles[i],
            0, 0, 100, 100, NULL, NULL, NULL, 0);
        ok(hLB != NULL, "last error 0x%08lx\n", GetLastError());

        ok(GetWindowLongA(hLB, GWL_STYLE) & LBS_OWNERDRAWVARIABLE, "Unexpected window style.\n");

        ret = SendMessageA(hLB, LB_INSERTSTRING, -1, 0);
        ok(ret == 0, "Unexpected return value %d.\n", ret);
        ret = SendMessageA(hLB, LB_INSERTSTRING, -1, 0);
        ok(ret == 1, "Unexpected return value %d.\n", ret);

        ret = SendMessageA(hLB, LB_SETITEMHEIGHT, 0, 13);
        ok(ret == LB_OKAY, "Failed to set item height, %d.\n", ret);

        ret = SendMessageA(hLB, LB_GETITEMHEIGHT, 0, 0);
        ok(ret == 13, "Unexpected item height %d.\n", ret);

        ret = SendMessageA(hLB, LB_SETITEMHEIGHT, 1, 42);
        ok(ret == LB_OKAY, "Failed to set item height, %d.\n", ret);

        ret = SendMessageA(hLB, LB_GETITEMHEIGHT, 0, 0);
        ok(ret == 42, "Unexpected item height %d.\n", ret);

        ret = SendMessageA(hLB, LB_GETITEMHEIGHT, 1, 0);
        ok(ret == 42, "Unexpected item height %d.\n", ret);

        DestroyWindow (hLB);
    }

    /* test pending redraw state */
    for (i = 0; i < ARRAY_SIZE(testcase); i++)
    {
        winetest_push_context("%d", i);
        hLB = create_listbox(LBS_OWNERDRAWFIXED | LBS_NODATA | WS_CHILD | WS_VISIBLE, parent);
        assert(hLB);

        ret = SendMessageA(hLB, WM_SETREDRAW, FALSE, 0);
        ok(!ret, "got %d\n", ret);
        ret = SendMessageA(hLB, testcase[i].message, testcase[i].wparam, testcase[i].lparam);
        if (testcase[i].message >= LB_ADDSTRING && testcase[i].message < LB_MSGMAX &&
            testcase[i].message != LB_SETCOUNT)
            ok(ret > 0, "expected > 0, got %d\n", ret);
        else
            ok(!ret, "expected 0, got %d\n", ret);

        got_drawitem = 0;
        ret = RedrawWindow(hLB, NULL, 0, RDW_UPDATENOW);
        ok(ret, "RedrawWindow failed\n");
        ok(!got_drawitem, "got %u\n", got_drawitem);

        ret = SendMessageA(hLB, WM_SETREDRAW, TRUE, 0);
        ok(!ret, "got %d\n", ret);

        got_drawitem = 0;
        ret = RedrawWindow(hLB, NULL, 0, RDW_UPDATENOW);
        ok(ret, "RedrawWindow failed\n");
        ok(got_drawitem == testcase[i].drawitem, "expected %u, got %u\n", testcase[i].drawitem, got_drawitem);

        DestroyWindow(hLB);
        winetest_pop_context();
    }

    DestroyWindow(parent);
}

#define listbox_test_query(exp, got) \
  ok(exp.selected == got.selected, "expected selected %d, got %d\n", exp.selected, got.selected); \
  ok(exp.anchor == got.anchor, "expected anchor %d, got %d\n", exp.anchor, got.anchor); \
  ok(exp.caret == got.caret, "expected caret %d, got %d\n", exp.caret, got.caret); \
  ok(exp.selcount == got.selcount, "expected selcount %d, got %d\n", exp.selcount, got.selcount);

static void test_LB_SELITEMRANGE(void)
{
    static const struct message selitemrange_seq[] =
    {
        { LB_SELITEMRANGE, sent },
        { EVENT_OBJECT_SELECTIONWITHIN, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
        { 0 }
    };
    static const struct listbox_stat test_nosel = { 0, LB_ERR, 0, 0 };
    static const struct listbox_stat test_1 = { 0, LB_ERR, 0, 2 };
    static const struct listbox_stat test_2 = { 0, LB_ERR, 0, 3 };
    static const struct listbox_stat test_3 = { 0, LB_ERR, 0, 4 };
    struct listbox_stat answer;
    HWND hLB;
    INT ret;

    hLB = create_listbox(LBS_EXTENDEDSEL, 0);
    ok(hLB != NULL, "Failed to create listbox window.\n");

    listbox_query(hLB, &answer);
    listbox_test_query(test_nosel, answer);

    ret = SendMessageA(hLB, LB_SELITEMRANGE, TRUE, MAKELPARAM(1, 2));
    ok(ret == LB_OKAY, "LB_SELITEMRANGE returned %d instead of LB_OKAY\n", ret);
    listbox_query(hLB, &answer);
    listbox_test_query(test_1, answer);

    SendMessageA(hLB, LB_SETSEL, FALSE, -1);
    listbox_query(hLB, &answer);
    listbox_test_query(test_nosel, answer);

    ret = SendMessageA(hLB, LB_SELITEMRANGE, TRUE, MAKELPARAM(0, 4));
    ok(ret == LB_OKAY, "LB_SELITEMRANGE returned %d instead of LB_OKAY\n", ret);
    listbox_query(hLB, &answer);
    listbox_test_query(test_3, answer);

    SendMessageA(hLB, LB_SETSEL, FALSE, -1);
    listbox_query(hLB, &answer);
    listbox_test_query(test_nosel, answer);

    ret = SendMessageA(hLB, LB_SELITEMRANGE, TRUE, MAKELPARAM(-5, 5));
    ok(ret == LB_OKAY, "LB_SELITEMRANGE returned %d instead of LB_OKAY\n", ret);
    listbox_query(hLB, &answer);
    listbox_test_query(test_nosel, answer);

    SendMessageA(hLB, LB_SETSEL, FALSE, -1);
    listbox_query(hLB, &answer);
    listbox_test_query(test_nosel, answer);

    flush_sequence(sequences, LB_SEQ_INDEX);
    ret = SendMessageA(hLB, LB_SELITEMRANGE, TRUE, MAKELPARAM(2, 10));
    ok(ret == LB_OKAY, "LB_SELITEMRANGE returned %d instead of LB_OKAY\n", ret);
    ok_sequence(sequences, LB_SEQ_INDEX, selitemrange_seq, "selitemrange_seq", FALSE);

    listbox_query(hLB, &answer);
    listbox_test_query(test_1, answer);

    SendMessageA(hLB, LB_SETSEL, FALSE, -1);
    listbox_query(hLB, &answer);
    listbox_test_query(test_nosel, answer);

    ret = SendMessageA(hLB, LB_SELITEMRANGE, TRUE, MAKELPARAM(4, 10));
    ok(ret == LB_OKAY, "LB_SELITEMRANGE returned %d instead of LB_OKAY\n", ret);
    listbox_query(hLB, &answer);
    listbox_test_query(test_nosel, answer);

    SendMessageA(hLB, LB_SETSEL, FALSE, -1);
    listbox_query(hLB, &answer);
    listbox_test_query(test_nosel, answer);

    ret = SendMessageA(hLB, LB_SELITEMRANGE, TRUE, MAKELPARAM(10, 1));
    ok(ret == LB_OKAY, "LB_SELITEMRANGE returned %d instead of LB_OKAY\n", ret);
    listbox_query(hLB, &answer);
    listbox_test_query(test_2, answer);

    SendMessageA(hLB, LB_SETSEL, FALSE, -1);
    listbox_query(hLB, &answer);
    listbox_test_query(test_nosel, answer);

    ret = SendMessageA(hLB, LB_SELITEMRANGE, TRUE, MAKELPARAM(1, -1));
    ok(ret == LB_OKAY, "LB_SELITEMRANGE returned %d instead of LB_OKAY\n", ret);
    listbox_query(hLB, &answer);
    listbox_test_query(test_2, answer);

    DestroyWindow(hLB);
}

static void test_LB_SETCURSEL(void)
{
    HWND parent, hLB;
    INT ret;

    parent = create_parent();
    ok(parent != NULL, "Failed to create parent window.\n");

    hLB = create_listbox(LBS_NOINTEGRALHEIGHT | WS_CHILD, parent);
    ok(hLB != NULL, "Failed to create listbox.\n");

    SendMessageA(hLB, LB_SETITEMHEIGHT, 0, 32);

    ret = SendMessageA(hLB, LB_GETANCHORINDEX, 0, 0);
    ok(ret == -1, "Unexpected anchor index %d.\n", ret);

    ret = SendMessageA(hLB, LB_SETCURSEL, 2, 0);
    ok(ret == 2, "LB_SETCURSEL returned %d instead of 2\n", ret);
    ret = GetScrollPos(hLB, SB_VERT);
    ok(ret == 0, "expected vscroll 0, got %d\n", ret);

    ret = SendMessageA(hLB, LB_GETANCHORINDEX, 0, 0);
    ok(ret == -1, "Unexpected anchor index %d.\n", ret);

    ret = SendMessageA(hLB, LB_SETCURSEL, 3, 0);
    ok(ret == 3, "LB_SETCURSEL returned %d instead of 3\n", ret);
    ret = GetScrollPos(hLB, SB_VERT);
    ok(ret == 1, "expected vscroll 1, got %d\n", ret);

    ret = SendMessageA(hLB, LB_GETANCHORINDEX, 0, 0);
    ok(ret == -1, "Unexpected anchor index %d.\n", ret);

    DestroyWindow(hLB);

    hLB = create_listbox(0, 0);
    ok(hLB != NULL, "Failed to create ListBox window.\n");

    ret = SendMessageA(hLB, LB_SETCURSEL, 1, 0);
    ok(ret == 1, "Unexpected return value %d.\n", ret);

    ret = SendMessageA(hLB, LB_GETANCHORINDEX, 0, 0);
    ok(ret == -1, "Unexpected anchor index %d.\n", ret);

    DestroyWindow(hLB);

    /* LBS_EXTENDEDSEL */
    hLB = create_listbox(LBS_EXTENDEDSEL, 0);
    ok(hLB != NULL, "Failed to create listbox.\n");

    ret = SendMessageA(hLB, LB_GETANCHORINDEX, 0, 0);
    ok(ret == -1, "Unexpected anchor index %d.\n", ret);

    ret = SendMessageA(hLB, LB_SETCURSEL, 2, 0);
    ok(ret == -1, "LB_SETCURSEL returned %d instead of 2\n", ret);

    ret = SendMessageA(hLB, LB_GETANCHORINDEX, 0, 0);
    ok(ret == -1, "Unexpected anchor index %d.\n", ret);

    DestroyWindow(hLB);

    /* LBS_MULTIPLESEL */
    hLB = create_listbox(LBS_MULTIPLESEL, 0);
    ok(hLB != NULL, "Failed to create listbox.\n");

    ret = SendMessageA(hLB, LB_GETANCHORINDEX, 0, 0);
    ok(ret == -1, "Unexpected anchor index %d.\n", ret);

    ret = SendMessageA(hLB, LB_SETCURSEL, 2, 0);
    ok(ret == -1, "LB_SETCURSEL returned %d instead of 2\n", ret);

    ret = SendMessageA(hLB, LB_GETANCHORINDEX, 0, 0);
    ok(ret == -1, "Unexpected anchor index %d.\n", ret);

    DestroyWindow(hLB);

    /* LBS_NOSEL */
    hLB = create_listbox(LBS_NOSEL, 0);
    ok(hLB != NULL, "Failed to create ListBox window.\n");

    ret = SendMessageA(hLB, LB_GETCURSEL, 0, 0);
    ok(ret == -1, "Unexpected return value %d.\n", ret);

    ret = SendMessageA(hLB, LB_GETSEL, 0, 0);
    ok(!ret, "Unexpected return value %d.\n", ret);

    ret = SendMessageA(hLB, LB_SETCURSEL, 2, 0);
    todo_wine
    ok(ret == 2, "Unexpected return value %d.\n", ret);

    ret = SendMessageA(hLB, LB_GETSEL, 2, 0);
    ok(ret == 1, "Unexpected return value %d.\n", ret);

    ret = SendMessageA(hLB, LB_GETCURSEL, 0, 0);
    ok(ret == 2, "Unexpected return value %d.\n", ret);

    ret = SendMessageA(hLB, LB_SETCURSEL, 3, 0);
    todo_wine
    ok(ret == 3, "Unexpected return value %d.\n", ret);

    ret = SendMessageA(hLB, LB_GETSEL, 3, 0);
    ok(ret == 1, "Unexpected return value %d.\n", ret);

    ret = SendMessageA(hLB, LB_GETCURSEL, 0, 0);
    ok(ret == 3, "Unexpected return value %d.\n", ret);

    DestroyWindow(hLB);

    DestroyWindow(parent);
}

static void test_LB_SETSEL(void)
{
    HWND list;
    int ret;

    /* LBS_EXTENDEDSEL */
    list = create_listbox(LBS_EXTENDEDSEL, 0);
    ok(list != NULL, "Failed to create ListBox window.\n");

    ret = SendMessageA(list, LB_GETANCHORINDEX, 0, 0);
    ok(ret == -1, "Unexpected anchor index %d.\n", ret);

    ret = SendMessageA(list, LB_SETSEL, TRUE, 0);
    ok(ret == 0, "Unexpected return value %d.\n", ret);
    ret = SendMessageA(list, LB_GETANCHORINDEX, 0, 0);
    ok(ret == 0, "Unexpected anchor index %d.\n", ret);
    ret = SendMessageA(list, LB_GETCARETINDEX, 0, 0);
    ok(ret == 0, "Unexpected caret index %d.\n", ret);

    ret = SendMessageA(list, LB_SETSEL, TRUE, 1);
    ok(ret == 0, "Unexpected return value %d.\n", ret);
    ret = SendMessageA(list, LB_GETANCHORINDEX, 0, 0);
    ok(ret == 1, "Unexpected anchor index %d.\n", ret);
    ret = SendMessageA(list, LB_GETCARETINDEX, 0, 0);
    ok(ret == 1, "Unexpected caret index %d.\n", ret);

    ret = SendMessageA(list, LB_SETSEL, FALSE, 1);
    ok(ret == 0, "Unexpected return value %d.\n", ret);
    ret = SendMessageA(list, LB_GETANCHORINDEX, 0, 0);
    ok(ret == 1, "Unexpected anchor index %d.\n", ret);
    ret = SendMessageA(list, LB_GETCARETINDEX, 0, 0);
    ok(ret == 1, "Unexpected caret index %d.\n", ret);

    DestroyWindow(list);

    /* LBS_MULTIPLESEL */
    list = create_listbox(LBS_MULTIPLESEL, 0);
    ok(list != NULL, "Failed to create ListBox window.\n");

    ret = SendMessageA(list, LB_GETANCHORINDEX, 0, 0);
    ok(ret == -1, "Unexpected anchor index %d.\n", ret);

    ret = SendMessageA(list, LB_SETSEL, TRUE, 0);
    ok(ret == 0, "Unexpected return value %d.\n", ret);
    ret = SendMessageA(list, LB_GETANCHORINDEX, 0, 0);
    ok(ret == 0, "Unexpected anchor index %d.\n", ret);
    ret = SendMessageA(list, LB_GETCARETINDEX, 0, 0);
    ok(ret == 0, "Unexpected caret index %d.\n", ret);

    ret = SendMessageA(list, LB_SETSEL, TRUE, 1);
    ok(ret == 0, "Unexpected return value %d.\n", ret);
    ret = SendMessageA(list, LB_GETANCHORINDEX, 0, 0);
    ok(ret == 1, "Unexpected anchor index %d.\n", ret);
    ret = SendMessageA(list, LB_GETCARETINDEX, 0, 0);
    ok(ret == 1, "Unexpected caret index %d.\n", ret);

    ret = SendMessageA(list, LB_SETSEL, FALSE, 1);
    ok(ret == 0, "Unexpected return value %d.\n", ret);
    ret = SendMessageA(list, LB_GETANCHORINDEX, 0, 0);
    ok(ret == 1, "Unexpected anchor index %d.\n", ret);
    ret = SendMessageA(list, LB_GETCARETINDEX, 0, 0);
    ok(ret == 1, "Unexpected caret index %d.\n", ret);

    DestroyWindow(list);
}

static void test_listbox_height(void)
{
    HWND hList;
    int r, id;

    hList = CreateWindowA( WC_LISTBOXA, "list test", 0,
                          1, 1, 600, 100, NULL, NULL, NULL, NULL );
    ok( hList != NULL, "failed to create listbox\n");

    id = SendMessageA( hList, LB_ADDSTRING, 0, (LPARAM) "hi");
    ok( id == 0, "item id wrong\n");

    r = SendMessageA( hList, LB_SETITEMHEIGHT, 0, MAKELPARAM( 20, 0 ));
    ok( r == 0, "send message failed\n");

    r = SendMessageA(hList, LB_GETITEMHEIGHT, 0, 0 );
    ok( r == 20, "height wrong\n");

    r = SendMessageA( hList, LB_SETITEMHEIGHT, 0, MAKELPARAM( 0, 30 ));
    ok( r == -1, "send message failed\n");

    r = SendMessageA(hList, LB_GETITEMHEIGHT, 0, 0 );
    ok( r == 20, "height wrong\n");

    /* Before Windows 10 1709 (or 1703?) the item height was limited to 255.
     * Since then, with comctl32 V6 the limit is 65535.
     */
    r = SendMessageA( hList, LB_SETITEMHEIGHT, 0, MAKELPARAM( 256, 0 ));
    ok(r == 0 || broken(r == -1), "Failed to set item height, %d.\n", r);
    if (r == -1)
    {
        r = SendMessageA(hList, LB_GETITEMHEIGHT, 0, 0 );
        ok( r == 20, "Unexpected item height %d.\n", r);
    }
    else
    {
        r = SendMessageA(hList, LB_GETITEMHEIGHT, 0, 0 );
        ok( r == 256, "Unexpected item height %d.\n", r);

        r = SendMessageA( hList, LB_SETITEMHEIGHT, 0, MAKELPARAM( 65535, 0 ));
        ok(r == 0, "Failed to set item height, %d.\n", r);

        r = SendMessageA(hList, LB_GETITEMHEIGHT, 0, 0 );
        ok( r == 65535, "Unexpected item height %d.\n", r);
    }

    r = SendMessageA( hList, LB_SETITEMHEIGHT, 0, MAKELPARAM( 0xff, 0 ));
    ok( r == 0, "send message failed\n");

    r = SendMessageA(hList, LB_GETITEMHEIGHT, 0, 0 );
    ok( r == 0xff, "height wrong\n");

    DestroyWindow( hList );
}

static void test_changing_selection_styles(void)
{
    static const DWORD styles[] =
    {
        0,
        LBS_NODATA | LBS_OWNERDRAWFIXED
    };
    static const DWORD selstyles[] =
    {
        0,
        LBS_MULTIPLESEL,
        LBS_EXTENDEDSEL,
        LBS_MULTIPLESEL | LBS_EXTENDEDSEL
    };
    static const LONG selexpect_single[]  = { 0, 0, 1 };
    static const LONG selexpect_single2[] = { 1, 0, 0 };
    static const LONG selexpect_multi[]   = { 1, 0, 1 };
    static const LONG selexpect_multi2[]  = { 1, 1, 0 };

    HWND parent, listbox;
    DWORD style;
    LONG ret;
    UINT i, j, k;

    parent = create_parent();
    ok(parent != NULL, "Failed to create parent window.\n");
    for (i = 0; i < ARRAY_SIZE(styles); i++)
    {
        /* Test if changing selection styles affects selection storage */
        for (j = 0; j < ARRAY_SIZE(selstyles); j++)
        {
            LONG setcursel_expect, selitemrange_expect, getselcount_expect;
            const LONG *selexpect;

            listbox = CreateWindowA(WC_LISTBOXA, "TestList", styles[i] | selstyles[j] | WS_CHILD | WS_VISIBLE,
                                    0, 0, 100, 100, parent, (HMENU)ID_LISTBOX, NULL, 0);
            ok(listbox != NULL, "%u: Failed to create ListBox window.\n", j);

            if (selstyles[j] & (LBS_MULTIPLESEL | LBS_EXTENDEDSEL))
            {
                setcursel_expect = LB_ERR;
                selitemrange_expect = LB_OKAY;
                getselcount_expect = 2;
                selexpect = selexpect_multi;
            }
            else
            {
                setcursel_expect = 2;
                selitemrange_expect = LB_ERR;
                getselcount_expect = LB_ERR;
                selexpect = selexpect_single;
            }

            for (k = 0; k < ARRAY_SIZE(selexpect_multi); k++)
            {
                ret = SendMessageA(listbox, LB_INSERTSTRING, -1, (LPARAM)"x");
                ok(ret == k, "%u: Unexpected return value %ld, expected %d.\n", j, ret, k);
            }
            ret = SendMessageA(listbox, LB_GETCOUNT, 0, 0);
            ok(ret == ARRAY_SIZE(selexpect_multi), "%u: Unexpected count %ld.\n", j, ret);

            /* Select items with different methods */
            ret = SendMessageA(listbox, LB_SETCURSEL, 2, 0);
            ok(ret == setcursel_expect, "%u: Unexpected return value %ld.\n", j, ret);
            ret = SendMessageA(listbox, LB_SELITEMRANGE, TRUE, MAKELPARAM(0, 0));
            ok(ret == selitemrange_expect, "%u: Unexpected return value %ld.\n", j, ret);
            ret = SendMessageA(listbox, LB_SELITEMRANGE, TRUE, MAKELPARAM(2, 2));
            ok(ret == selitemrange_expect, "%u: Unexpected return value %ld.\n", j, ret);

            /* Verify that the proper items are selected */
            for (k = 0; k < ARRAY_SIZE(selexpect_multi); k++)
            {
                ret = SendMessageA(listbox, LB_GETSEL, k, 0);
                ok(ret == selexpect[k], "%u: Unexpected selection state %ld, expected %ld.\n",
                    j, ret, selexpect[k]);
            }

            /* Now change the selection style */
            style = GetWindowLongA(listbox, GWL_STYLE);
            ok((style & (LBS_MULTIPLESEL | LBS_EXTENDEDSEL)) == selstyles[j],
                "%u: unexpected window styles %#lx.\n", j, style);
            if (selstyles[j] & (LBS_MULTIPLESEL | LBS_EXTENDEDSEL))
                style &= ~selstyles[j];
            else
                style |= LBS_MULTIPLESEL | LBS_EXTENDEDSEL;
            SetWindowLongA(listbox, GWL_STYLE, style);
            style = GetWindowLongA(listbox, GWL_STYLE);
            ok(!(style & selstyles[j]), "%u: unexpected window styles %#lx.\n", j, style);

            /* Verify that the same items are selected */
            ret = SendMessageA(listbox, LB_GETSELCOUNT, 0, 0);
            ok(ret == getselcount_expect, "%u: expected %ld from LB_GETSELCOUNT, got %ld\n",
                j, getselcount_expect, ret);

            for (k = 0; k < ARRAY_SIZE(selexpect_multi); k++)
            {
                ret = SendMessageA(listbox, LB_GETSEL, k, 0);
                ok(ret == selexpect[k], "%u: Unexpected selection state %ld, expected %ld.\n",
                    j, ret, selexpect[k]);
            }

            /* Lastly see if we can still change the selection as before with old style */
            if (setcursel_expect != LB_ERR) setcursel_expect = 0;
            ret = SendMessageA(listbox, LB_SETCURSEL, 0, 0);
            ok(ret == setcursel_expect, "%u: Unexpected return value %ld.\n", j, ret);
            ret = SendMessageA(listbox, LB_SELITEMRANGE, TRUE, MAKELPARAM(1, 1));
            ok(ret == selitemrange_expect, "%u: Unexpected return value %ld.\n", j, ret);
            ret = SendMessageA(listbox, LB_SELITEMRANGE, FALSE, MAKELPARAM(2, 2));
            ok(ret == selitemrange_expect, "%u: Unexpected return value %ld.\n", j, ret);

            /* And verify the selections */
            selexpect = (selstyles[j] & (LBS_MULTIPLESEL | LBS_EXTENDEDSEL)) ? selexpect_multi2 : selexpect_single2;
            ret = SendMessageA(listbox, LB_GETSELCOUNT, 0, 0);
            ok(ret == getselcount_expect, "%u: expected %ld from LB_GETSELCOUNT, got %ld\n",
                j, getselcount_expect, ret);

            for (k = 0; k < ARRAY_SIZE(selexpect_multi); k++)
            {
                ret = SendMessageA(listbox, LB_GETSEL, k, 0);
                ok(ret == selexpect[k], "%u: Unexpected selection state %ld, expected %ld.\n",
                    j, ret, selexpect[k]);
            }

            DestroyWindow(listbox);
        }
    }
    DestroyWindow(parent);
}

static void test_itemfrompoint(void)
{
    /* WS_POPUP is required in order to have a more accurate size calculation (
       without caption). LBS_NOINTEGRALHEIGHT is required in order to test
       behavior of partially-displayed item.
     */
    HWND hList = CreateWindowA( WC_LISTBOXA, "list test",
                               WS_VISIBLE|WS_POPUP|LBS_NOINTEGRALHEIGHT,
                               1, 1, 600, 100, NULL, NULL, NULL, NULL );
    ULONG r, id;
    RECT rc;

    r = SendMessageA(hList, LB_ITEMFROMPOINT, 0, MAKELPARAM( /* x */ 30, /* y */ 30 ));
    ok( r == MAKELPARAM(0xffff, 1), "Unexpected ret value %#lx.\n", r );

    r = SendMessageA(hList, LB_ITEMFROMPOINT, 0, MAKELPARAM( 700, 30 ));
    ok( r == MAKELPARAM(0xffff, 1), "Unexpected ret value %#lx.\n", r );

    r = SendMessageA(hList, LB_ITEMFROMPOINT, 0, MAKELPARAM( 30, 300 ));
    ok( r == MAKELPARAM(0xffff, 1), "Unexpected ret value %#lx.\n", r );

    id = SendMessageA( hList, LB_ADDSTRING, 0, (LPARAM) "hi");
    ok( id == 0, "item id wrong\n");
    id = SendMessageA( hList, LB_ADDSTRING, 0, (LPARAM) "hi1");
    ok( id == 1, "item id wrong\n");

    r = SendMessageA(hList, LB_ITEMFROMPOINT, 0, MAKELPARAM( /* x */ 30, /* y */ 30 ));
    ok( r == 0x1, "ret %lx\n", r );

    r = SendMessageA(hList, LB_ITEMFROMPOINT, 0, MAKELPARAM( /* x */ 30, /* y */ 601 ));
    ok( r == MAKELPARAM(1, 1), "Unexpected ret value %#lx.\n", r );

    /* Resize control so that below assertions about sizes are valid */
    r = SendMessageA( hList, LB_GETITEMRECT, 0, (LPARAM)&rc);
    ok( r == 1, "ret %lx\n", r);
    r = MoveWindow(hList, 1, 1, 600, (rc.bottom - rc.top + 1) * 9 / 2, TRUE);
    ok( r != 0, "ret %lx\n", r);

    id = SendMessageA( hList, LB_ADDSTRING, 0, (LPARAM) "hi2");
    ok( id == 2, "item id wrong\n");
    id = SendMessageA( hList, LB_ADDSTRING, 0, (LPARAM) "hi3");
    ok( id == 3, "item id wrong\n");
    id = SendMessageA( hList, LB_ADDSTRING, 0, (LPARAM) "hi4");
    ok( id == 4, "item id wrong\n");
    id = SendMessageA( hList, LB_ADDSTRING, 0, (LPARAM) "hi5");
    ok( id == 5, "item id wrong\n");
    id = SendMessageA( hList, LB_ADDSTRING, 0, (LPARAM) "hi6");
    ok( id == 6, "item id wrong\n");
    id = SendMessageA( hList, LB_ADDSTRING, 0, (LPARAM) "hi7");
    ok( id == 7, "item id wrong\n");

    /* Set the listbox up so that id 1 is at the top, this leaves 5
       partially visible at the bottom and 6, 7 are invisible */

    SendMessageA( hList, LB_SETTOPINDEX, 1, 0);
    r = SendMessageA( hList, LB_GETTOPINDEX, 0, 0);
    ok( r == 1, "top %ld\n", r);

    r = SendMessageA( hList, LB_GETITEMRECT, 5, (LPARAM)&rc);
    ok( r == 1, "ret %lx\n", r);
    r = SendMessageA( hList, LB_GETITEMRECT, 6, (LPARAM)&rc);
    ok( r == 0, "ret %lx\n", r);

    r = SendMessageA( hList, LB_ITEMFROMPOINT, 0, MAKELPARAM(/* x */ 10, /* y */ 10) );
    ok( r == 1, "ret %lx\n", r);

    r = SendMessageA( hList, LB_ITEMFROMPOINT, 0, MAKELPARAM(1000, 10) );
    ok( r == 0x10001, "ret %lx\n", r );

    r = SendMessageA( hList, LB_ITEMFROMPOINT, 0, MAKELPARAM(10, -10) );
    ok( r == 0x10001, "ret %lx\n", r );

    r = SendMessageA( hList, LB_ITEMFROMPOINT, 0, MAKELPARAM(10, 100) );
    ok( r == 0x10005, "item %lx\n", r );

    r = SendMessageA( hList, LB_ITEMFROMPOINT, 0, MAKELPARAM(10, 200) );
    ok( r == 0x10005, "item %lx\n", r );

    DestroyWindow( hList );
}

static void test_listbox_item_data(void)
{
    HWND hList;
    int r, id;

    hList = CreateWindowA( WC_LISTBOXA, "list test", 0,
                          1, 1, 600, 100, NULL, NULL, NULL, NULL );
    ok( hList != NULL, "failed to create listbox\n");

    id = SendMessageA( hList, LB_ADDSTRING, 0, (LPARAM) "hi");
    ok( id == 0, "item id wrong\n");

    r = SendMessageA( hList, LB_SETITEMDATA, 0, MAKELPARAM( 20, 0 ));
    ok(r == TRUE, "LB_SETITEMDATA returned %d instead of TRUE\n", r);

    r = SendMessageA( hList, LB_GETITEMDATA, 0, 0);
    ok( r == 20, "get item data failed\n");

    DestroyWindow( hList );
}

static void test_listbox_LB_DIR(void)
{
    char path[MAX_PATH], curdir[MAX_PATH];
    HWND hList;
    int res, itemCount;
    int itemCount_justFiles;
    int itemCount_justDrives;
    int itemCount_allFiles;
    int itemCount_allDirs;
    int i;
    char pathBuffer[MAX_PATH];
    char * p;
    char driveletter;
    const char *wildcard = "*";
    HANDLE file;
    BOOL ret;

    GetCurrentDirectoryA(ARRAY_SIZE(curdir), curdir);

    GetTempPathA(ARRAY_SIZE(path), path);
    ret = SetCurrentDirectoryA(path);
    ok(ret, "Failed to set current directory.\n");

    ret = CreateDirectoryA("lb_dir_test", NULL);
    ok(ret, "Failed to create test directory.\n");

    file = CreateFileA( "wtest1.tmp.c", GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL );
    ok(file != INVALID_HANDLE_VALUE, "Error creating the test file: %ld\n", GetLastError());
    CloseHandle( file );

    /* NOTE: for this test to succeed, there must be no subdirectories
       under the current directory. In addition, there must be at least
       one file that fits the wildcard w*.c . Normally, the test
       directory itself satisfies both conditions.
     */
    hList = CreateWindowA( WC_LISTBOXA, "list test", WS_VISIBLE|WS_POPUP,
                          1, 1, 600, 100, NULL, NULL, NULL, NULL );
    ok(hList != NULL, "Failed to create listbox window.\n");

    /* Test for standard usage */

    /* This should list all the files in the test directory. */
    strcpy(pathBuffer, wildcard);
    SendMessageA(hList, LB_RESETCONTENT, 0, 0);
    res = SendMessageA(hList, LB_DIR, 0, (LPARAM)pathBuffer);
    if (res == -1)  /* "*" wildcard doesn't work on win9x */
    {
        wildcard = "*.*";
        strcpy(pathBuffer, wildcard);
        res = SendMessageA(hList, LB_DIR, 0, (LPARAM)pathBuffer);
    }
    ok (res >= 0, "SendMessage(LB_DIR, 0, *) failed - 0x%08lx\n", GetLastError());

    /* There should be some content in the listbox */
    itemCount = SendMessageA(hList, LB_GETCOUNT, 0, 0);
    ok (itemCount > 0, "SendMessage(LB_DIR) did NOT fill the listbox!\n");
    itemCount_allFiles = itemCount;
    ok(res + 1 == itemCount,
        "SendMessage(LB_DIR, 0, *) returned incorrect index (expected %d got %d)!\n",
        itemCount - 1, res);

    /* This tests behavior when no files match the wildcard */
    strcpy(pathBuffer, BAD_EXTENSION);
    SendMessageA(hList, LB_RESETCONTENT, 0, 0);
    res = SendMessageA(hList, LB_DIR, 0, (LPARAM)pathBuffer);
    ok (res == -1, "SendMessage(LB_DIR, 0, %s) returned %d, expected -1\n", BAD_EXTENSION, res);

    /* There should be NO content in the listbox */
    itemCount = SendMessageA(hList, LB_GETCOUNT, 0, 0);
    ok (itemCount == 0, "SendMessage(LB_DIR) DID fill the listbox!\n");


    /* This should list all the w*.c files in the test directory
     * As of this writing, this includes win.c, winstation.c, wsprintf.c
     */
    strcpy(pathBuffer, "w*.c");
    SendMessageA(hList, LB_RESETCONTENT, 0, 0);
    res = SendMessageA(hList, LB_DIR, 0, (LPARAM)pathBuffer);
    ok (res >= 0, "SendMessage(LB_DIR, 0, w*.c) failed - 0x%08lx\n", GetLastError());

    /* Path specification does NOT converted to uppercase */
    ok (!strcmp(pathBuffer, "w*.c"),
        "expected no change to pathBuffer, got %s\n", pathBuffer);

    /* There should be some content in the listbox */
    itemCount = SendMessageA(hList, LB_GETCOUNT, 0, 0);
    ok (itemCount > 0, "SendMessage(LB_DIR) did NOT fill the listbox!\n");
    itemCount_justFiles = itemCount;
    ok(res + 1 == itemCount,
        "SendMessage(LB_DIR, 0, w*.c) returned incorrect index (expected %d got %d)!\n",
        itemCount - 1, res);

    /* Every single item in the control should start with a w and end in .c */
    for (i = 0; i < itemCount; i++)
    {
        memset(pathBuffer, 0, MAX_PATH);
        SendMessageA(hList, LB_GETTEXT, i, (LPARAM)pathBuffer);
        p = pathBuffer + strlen(pathBuffer);
        ok(((pathBuffer[0] == 'w' || pathBuffer[0] == 'W') &&
            (*(p-1) == 'c' || *(p-1) == 'C') &&
            (*(p-2) == '.')), "Element %d (%s) does not fit requested w*.c\n", i, pathBuffer);
    }

    /* Test DDL_DIRECTORY */
    strcpy(pathBuffer, wildcard);
    SendMessageA(hList, LB_RESETCONTENT, 0, 0);
    res = SendMessageA(hList, LB_DIR, DDL_DIRECTORY, (LPARAM)pathBuffer);
    ok (res > 0, "SendMessage(LB_DIR, DDL_DIRECTORY, *) failed - 0x%08lx\n", GetLastError());

    /* There should be some content in the listbox.
     * All files plus "[..]"
     */
    itemCount = SendMessageA(hList, LB_GETCOUNT, 0, 0);
    itemCount_allDirs = itemCount - itemCount_allFiles;
    ok (itemCount >= itemCount_allFiles,
        "SendMessage(LB_DIR, DDL_DIRECTORY, *) filled with %d entries, expected > %d\n",
        itemCount, itemCount_allFiles);
    ok(res + 1 == itemCount,
        "SendMessage(LB_DIR, DDL_DIRECTORY, *) returned incorrect index (expected %d got %d)!\n",
        itemCount - 1, res);

    /* This tests behavior when no files match the wildcard */
    strcpy(pathBuffer, BAD_EXTENSION);
    SendMessageA(hList, LB_RESETCONTENT, 0, 0);
    res = SendMessageA(hList, LB_DIR, DDL_DIRECTORY, (LPARAM)pathBuffer);
    ok (res == -1, "SendMessage(LB_DIR, DDL_DIRECTORY, %s) returned %d, expected -1\n", BAD_EXTENSION, res);

    /* There should be NO content in the listbox */
    itemCount = SendMessageA(hList, LB_GETCOUNT, 0, 0);
    ok (itemCount == 0, "SendMessage(LB_DIR) DID fill the listbox!\n");

    /* Test DDL_DIRECTORY */
    strcpy(pathBuffer, "w*.c");
    SendMessageA(hList, LB_RESETCONTENT, 0, 0);
    res = SendMessageA(hList, LB_DIR, DDL_DIRECTORY, (LPARAM)pathBuffer);
    ok (res >= 0, "SendMessage(LB_DIR, DDL_DIRECTORY, w*.c) failed - 0x%08lx\n", GetLastError());

    /* There should be some content in the listbox. Since the parent directory does not
     * fit w*.c, there should be exactly the same number of items as without DDL_DIRECTORY
     */
    itemCount = SendMessageA(hList, LB_GETCOUNT, 0, 0);
    ok (itemCount == itemCount_justFiles,
        "SendMessage(LB_DIR, DDL_DIRECTORY, w*.c) filled with %d entries, expected %d\n",
        itemCount, itemCount_justFiles);
    ok(res + 1 == itemCount,
        "SendMessage(LB_DIR, DDL_DIRECTORY, w*.c) returned incorrect index (expected %d got %d)!\n",
        itemCount - 1, res);

    /* Every single item in the control should start with a w and end in .c. */
    for (i = 0; i < itemCount; i++)
    {
        memset(pathBuffer, 0, MAX_PATH);
        SendMessageA(hList, LB_GETTEXT, i, (LPARAM)pathBuffer);
        p = pathBuffer + strlen(pathBuffer);
        ok(
            ((pathBuffer[0] == 'w' || pathBuffer[0] == 'W') &&
            (*(p-1) == 'c' || *(p-1) == 'C') &&
            (*(p-2) == '.')), "Element %d (%s) does not fit requested w*.c\n", i, pathBuffer);
    }

    /* Test DDL_DRIVES|DDL_EXCLUSIVE */
    strcpy(pathBuffer, wildcard);
    SendMessageA(hList, LB_RESETCONTENT, 0, 0);
    res = SendMessageA(hList, LB_DIR, DDL_DRIVES|DDL_EXCLUSIVE, (LPARAM)pathBuffer);
    ok (res >= 0, "SendMessage(LB_DIR, DDL_DRIVES|DDL_EXCLUSIVE, *)  failed - 0x%08lx\n", GetLastError());

    /* There should be some content in the listbox. In particular, there should
     * be at least one element before, since the string "[-c-]" should
     * have been added. Depending on the user setting, more drives might have
     * been added.
     */
    itemCount = SendMessageA(hList, LB_GETCOUNT, 0, 0);
    ok (itemCount >= 1,
        "SendMessage(LB_DIR, DDL_DRIVES|DDL_EXCLUSIVE, *) filled with %d entries, expected at least %d\n",
        itemCount, 1);
    itemCount_justDrives = itemCount;
    ok(res + 1 == itemCount, "SendMessage(LB_DIR, DDL_DRIVES|DDL_EXCLUSIVE, *) returned incorrect index!\n");

    /* Every single item in the control should fit the format [-c-] */
    for (i = 0; i < itemCount; i++)
    {
        memset(pathBuffer, 0, MAX_PATH);
        driveletter = '\0';
        SendMessageA(hList, LB_GETTEXT, i, (LPARAM)pathBuffer);
        ok( strlen(pathBuffer) == 5, "Length of drive string is not 5\n" );
        ok( sscanf(pathBuffer, "[-%c-]", &driveletter) == 1, "Element %d (%s) does not fit [-X-]\n", i, pathBuffer);
        ok( driveletter >= 'a' && driveletter <= 'z', "Drive letter not in range a..z, got ascii %d\n", driveletter);
        if (!(driveletter >= 'a' && driveletter <= 'z'))
        {
            /* Correct after invalid entry is found */
            itemCount_justDrives--;
        }
    }

    /* This tests behavior when no files match the wildcard */
    strcpy(pathBuffer, BAD_EXTENSION);
    SendMessageA(hList, LB_RESETCONTENT, 0, 0);
    res = SendMessageA(hList, LB_DIR, DDL_DRIVES|DDL_EXCLUSIVE, (LPARAM)pathBuffer);
    ok (res == itemCount_justDrives -1, "SendMessage(LB_DIR, DDL_DRIVES|DDL_EXCLUSIVE, %s) returned %d, expected %d\n",
        BAD_EXTENSION, res, itemCount_justDrives -1);

    itemCount = SendMessageA(hList, LB_GETCOUNT, 0, 0);
    ok (itemCount == itemCount_justDrives, "SendMessage(LB_DIR) returned %d expected %d\n",
        itemCount, itemCount_justDrives);

    /* Test DDL_DRIVES. */
    strcpy(pathBuffer, wildcard);
    SendMessageA(hList, LB_RESETCONTENT, 0, 0);
    res = SendMessageA(hList, LB_DIR, DDL_DRIVES, (LPARAM)pathBuffer);
    ok (res > 0, "SendMessage(LB_DIR, DDL_DRIVES, *)  failed - 0x%08lx\n", GetLastError());

    /* There should be some content in the listbox. In particular, there should
     * be at least one element before, since the string "[-c-]" should
     * have been added. Depending on the user setting, more drives might have
     * been added.
     */
    itemCount = SendMessageA(hList, LB_GETCOUNT, 0, 0);
    ok (itemCount == itemCount_justDrives + itemCount_allFiles,
        "SendMessage(LB_DIR, DDL_DRIVES, *) filled with %d entries, expected %d\n",
        itemCount, itemCount_justDrives + itemCount_allFiles);
    ok(res + 1 == itemCount, "SendMessage(LB_DIR, DDL_DRIVES, *) returned incorrect index!\n");

    /* This tests behavior when no files match the wildcard */
    strcpy(pathBuffer, BAD_EXTENSION);
    SendMessageA(hList, LB_RESETCONTENT, 0, 0);
    res = SendMessageA(hList, LB_DIR, DDL_DRIVES, (LPARAM)pathBuffer);
    ok (res == itemCount_justDrives -1, "SendMessage(LB_DIR, DDL_DRIVES, %s) returned %d, expected %d\n",
        BAD_EXTENSION, res, itemCount_justDrives -1);

    itemCount = SendMessageA(hList, LB_GETCOUNT, 0, 0);
    ok (itemCount == res + 1, "SendMessage(LB_DIR) returned %d expected %d\n", itemCount, res + 1);

    /* Test DDL_DRIVES. */
    strcpy(pathBuffer, "w*.c");
    SendMessageA(hList, LB_RESETCONTENT, 0, 0);
    res = SendMessageA(hList, LB_DIR, DDL_DRIVES, (LPARAM)pathBuffer);
    ok (res > 0, "SendMessage(LB_DIR, DDL_DRIVES, w*.c)  failed - 0x%08lx\n", GetLastError());

    /* There should be some content in the listbox. In particular, there should
     * be at least one element before, since the string "[-c-]" should
     * have been added. Depending on the user setting, more drives might have
     * been added.
     */
    itemCount = SendMessageA(hList, LB_GETCOUNT, 0, 0);
    ok (itemCount == itemCount_justDrives + itemCount_justFiles,
        "SendMessage(LB_DIR, DDL_DRIVES, w*.c) filled with %d entries, expected %d\n",
        itemCount, itemCount_justDrives + itemCount_justFiles);
    ok(res + 1 == itemCount, "SendMessage(LB_DIR, DDL_DRIVES, w*.c) returned incorrect index!\n");

    /* Every single item in the control should fit the format [-c-], or w*.c */
    for (i = 0; i < itemCount; i++)
    {
        memset(pathBuffer, 0, MAX_PATH);
        driveletter = '\0';
        SendMessageA(hList, LB_GETTEXT, i, (LPARAM)pathBuffer);
        p = pathBuffer + strlen(pathBuffer);
        if (sscanf(pathBuffer, "[-%c-]", &driveletter) == 1)
        {
            ok( strlen(pathBuffer) == 5, "Length of drive string is not 5\n" );
            ok( driveletter >= 'a' && driveletter <= 'z', "Drive letter not in range a..z, got ascii %d\n", driveletter);
        }
        else
        {
            ok(
                ((pathBuffer[0] == 'w' || pathBuffer[0] == 'W') &&
                (*(p-1) == 'c' || *(p-1) == 'C') &&
                (*(p-2) == '.')), "Element %d (%s) does not fit requested w*.c\n", i, pathBuffer);
        }
    }

    /* Test DDL_DIRECTORY|DDL_DRIVES. This does *not* imply DDL_EXCLUSIVE */
    strcpy(pathBuffer, wildcard);
    SendMessageA(hList, LB_RESETCONTENT, 0, 0);
    res = SendMessageA(hList, LB_DIR, DDL_DIRECTORY|DDL_DRIVES, (LPARAM)pathBuffer);
    ok (res > 0, "SendMessage(LB_DIR, DDL_DIRECTORY|DDL_DRIVES, *) failed - 0x%08lx\n", GetLastError());

    /* There should be some content in the listbox. In particular, there should
     * be exactly the number of plain files, plus the number of mapped drives.
     */
    itemCount = SendMessageA(hList, LB_GETCOUNT, 0, 0);
    ok (itemCount == itemCount_allFiles + itemCount_justDrives + itemCount_allDirs,
        "SendMessage(LB_DIR, DDL_DIRECTORY|DDL_DRIVES) filled with %d entries, expected %d\n",
        itemCount, itemCount_allFiles + itemCount_justDrives + itemCount_allDirs);
    ok(res + 1 == itemCount, "SendMessage(LB_DIR, DDL_DIRECTORY|DDL_DRIVES, w*.c) returned incorrect index!\n");

    /* Every single item in the control should start with a w and end in .c,
     * except for the "[..]" string, which should appear exactly as it is,
     * and the mapped drives in the format "[-X-]".
     */
    for (i = 0; i < itemCount; i++)
    {
        memset(pathBuffer, 0, MAX_PATH);
        driveletter = '\0';
        SendMessageA(hList, LB_GETTEXT, i, (LPARAM)pathBuffer);
        if (sscanf(pathBuffer, "[-%c-]", &driveletter) == 1)
            ok( driveletter >= 'a' && driveletter <= 'z', "Drive letter not in range a..z, got ascii %d\n", driveletter);
    }

    /* This tests behavior when no files match the wildcard */
    strcpy(pathBuffer, BAD_EXTENSION);
    SendMessageA(hList, LB_RESETCONTENT, 0, 0);
    res = SendMessageA(hList, LB_DIR, DDL_DIRECTORY|DDL_DRIVES, (LPARAM)pathBuffer);
    ok (res == itemCount_justDrives -1, "SendMessage(LB_DIR, DDL_DIRECTORY|DDL_DRIVES, %s) returned %d, expected %d\n",
        BAD_EXTENSION, res, itemCount_justDrives -1);

    itemCount = SendMessageA(hList, LB_GETCOUNT, 0, 0);
    ok (itemCount == res + 1, "SendMessage(LB_DIR) returned %d expected %d\n", itemCount, res + 1);

    /* Test DDL_DIRECTORY|DDL_DRIVES. */
    strcpy(pathBuffer, "w*.c");
    SendMessageA(hList, LB_RESETCONTENT, 0, 0);
    res = SendMessageA(hList, LB_DIR, DDL_DIRECTORY|DDL_DRIVES, (LPARAM)pathBuffer);
    ok (res > 0, "SendMessage(LB_DIR, DDL_DIRECTORY|DDL_DRIVES, w*.c) failed - 0x%08lx\n", GetLastError());

    /* There should be some content in the listbox. In particular, there should
     * be exactly the number of plain files, plus the number of mapped drives.
     */
    itemCount = SendMessageA(hList, LB_GETCOUNT, 0, 0);
    ok (itemCount == itemCount_justFiles + itemCount_justDrives,
        "SendMessage(LB_DIR, DDL_DIRECTORY|DDL_DRIVES) filled with %d entries, expected %d\n",
        itemCount, itemCount_justFiles + itemCount_justDrives);
    ok(res + 1 == itemCount, "SendMessage(LB_DIR, DDL_DIRECTORY|DDL_DRIVES, w*.c) returned incorrect index!\n");

    /* Every single item in the control should start with a w and end in .c,
     * except the mapped drives in the format "[-X-]". The "[..]" directory
     * should not appear.
     */
    for (i = 0; i < itemCount; i++)
    {
        memset(pathBuffer, 0, MAX_PATH);
        driveletter = '\0';
        SendMessageA(hList, LB_GETTEXT, i, (LPARAM)pathBuffer);
        p = pathBuffer + strlen(pathBuffer);
        if (sscanf(pathBuffer, "[-%c-]", &driveletter) == 1)
            ok( driveletter >= 'a' && driveletter <= 'z', "Drive letter not in range a..z, got ascii %d\n", driveletter);
        else
            ok(
                ((pathBuffer[0] == 'w' || pathBuffer[0] == 'W') &&
                (*(p-1) == 'c' || *(p-1) == 'C') &&
                (*(p-2) == '.')), "Element %d (%s) does not fit requested w*.c\n", i, pathBuffer);
    }

    /* Test DDL_DIRECTORY|DDL_EXCLUSIVE. */
    strcpy(pathBuffer, wildcard);
    SendMessageA(hList, LB_RESETCONTENT, 0, 0);
    res = SendMessageA(hList, LB_DIR, DDL_DIRECTORY|DDL_EXCLUSIVE, (LPARAM)pathBuffer);
    ok (res != -1, "SendMessage(LB_DIR, DDL_DIRECTORY|DDL_EXCLUSIVE, *) failed err %lu\n",
        GetLastError());

    itemCount = SendMessageA(hList, LB_GETCOUNT, 0, 0);
    ok (itemCount == itemCount_allDirs,
        "SendMessage(LB_DIR, DDL_DIRECTORY|DDL_EXCLUSIVE) filled with %d entries, expected %d\n",
        itemCount, itemCount_allDirs);
    ok(res + 1 == itemCount, "SendMessage(LB_DIR, DDL_DIRECTORY|DDL_EXCLUSIVE, *) returned incorrect index!\n");

    if (itemCount)
    {
        memset(pathBuffer, 0, MAX_PATH);
        SendMessageA(hList, LB_GETTEXT, 0, (LPARAM)pathBuffer);
        ok( !strcmp(pathBuffer, "[..]"), "First element is %s, not [..]\n", pathBuffer);
    }

    /* This tests behavior when no files match the wildcard */
    strcpy(pathBuffer, BAD_EXTENSION);
    SendMessageA(hList, LB_RESETCONTENT, 0, 0);
    res = SendMessageA(hList, LB_DIR, DDL_DIRECTORY|DDL_EXCLUSIVE, (LPARAM)pathBuffer);
    ok (res == -1, "SendMessage(LB_DIR, DDL_DIRECTORY|DDL_EXCLUSIVE, %s) returned %d, expected %d\n",
        BAD_EXTENSION, res, -1);

    itemCount = SendMessageA(hList, LB_GETCOUNT, 0, 0);
    ok (itemCount == res + 1, "SendMessage(LB_DIR) returned %d expected %d\n", itemCount, res + 1);


    /* Test DDL_DIRECTORY|DDL_EXCLUSIVE. */
    strcpy(pathBuffer, "w*.c");
    SendMessageA(hList, LB_RESETCONTENT, 0, 0);
    res = SendMessageA(hList, LB_DIR, DDL_DIRECTORY|DDL_EXCLUSIVE, (LPARAM)pathBuffer);
    ok (res == LB_ERR, "SendMessage(LB_DIR, DDL_DIRECTORY|DDL_EXCLUSIVE, w*.c) returned %d expected %d\n", res, LB_ERR);

    /* There should be no elements, since "[..]" does not fit w*.c */
    itemCount = SendMessageA(hList, LB_GETCOUNT, 0, 0);
    ok (itemCount == 0,
        "SendMessage(LB_DIR, DDL_DIRECTORY|DDL_EXCLUSIVE) filled with %d entries, expected %d\n",
        itemCount, 0);

    /* Test DDL_DIRECTORY|DDL_DRIVES|DDL_EXCLUSIVE. */
    strcpy(pathBuffer, wildcard);
    SendMessageA(hList, LB_RESETCONTENT, 0, 0);
    res = SendMessageA(hList, LB_DIR, DDL_DIRECTORY|DDL_DRIVES|DDL_EXCLUSIVE, (LPARAM)pathBuffer);
    ok (res > 0, "SendMessage(LB_DIR, DDL_DIRECTORY|DDL_DRIVES|DDL_EXCLUSIVE, w*.c,) failed - 0x%08lx\n", GetLastError());

    /* There should be no plain files on the listbox */
    itemCount = SendMessageA(hList, LB_GETCOUNT, 0, 0);
    ok (itemCount == itemCount_justDrives + itemCount_allDirs,
        "SendMessage(LB_DIR, DDL_DIRECTORY|DDL_DRIVES|DDL_EXCLUSIVE) filled with %d entries, expected %d\n",
        itemCount, itemCount_justDrives + itemCount_allDirs);
    ok(res + 1 == itemCount, "SendMessage(LB_DIR, DDL_DIRECTORY|DDL_DRIVES|DDL_EXCLUSIVE, w*.c) returned incorrect index!\n");

    for (i = 0; i < itemCount; i++)
    {
        memset(pathBuffer, 0, MAX_PATH);
        driveletter = '\0';
        SendMessageA(hList, LB_GETTEXT, i, (LPARAM)pathBuffer);
        if (sscanf(pathBuffer, "[-%c-]", &driveletter) == 1)
            ok( driveletter >= 'a' && driveletter <= 'z', "Drive letter not in range a..z, got ascii %d\n", driveletter);
        else
            ok( pathBuffer[0] == '[' && pathBuffer[strlen(pathBuffer)-1] == ']',
                "Element %d (%s) does not fit expected [...]\n", i, pathBuffer);
    }

    /* This tests behavior when no files match the wildcard */
    strcpy(pathBuffer, BAD_EXTENSION);
    SendMessageA(hList, LB_RESETCONTENT, 0, 0);
    res = SendMessageA(hList, LB_DIR, DDL_DIRECTORY|DDL_DRIVES|DDL_EXCLUSIVE, (LPARAM)pathBuffer);
    ok (res == itemCount_justDrives -1, "SendMessage(LB_DIR, DDL_DIRECTORY|DDL_DRIVES|DDL_EXCLUSIVE, %s) returned %d, expected %d\n",
        BAD_EXTENSION, res, itemCount_justDrives -1);

    itemCount = SendMessageA(hList, LB_GETCOUNT, 0, 0);
    ok (itemCount == res + 1, "SendMessage(LB_DIR) returned %d expected %d\n", itemCount, res + 1);

    /* Test DDL_DIRECTORY|DDL_DRIVES|DDL_EXCLUSIVE. */
    strcpy(pathBuffer, "w*.c");
    SendMessageA(hList, LB_RESETCONTENT, 0, 0);
    res = SendMessageA(hList, LB_DIR, DDL_DIRECTORY|DDL_DRIVES|DDL_EXCLUSIVE, (LPARAM)pathBuffer);
    ok (res >= 0, "SendMessage(LB_DIR, DDL_DIRECTORY|DDL_DRIVES|DDL_EXCLUSIVE, w*.c,) failed - 0x%08lx\n", GetLastError());

    /* There should be no plain files on the listbox, and no [..], since it does not fit w*.c */
    itemCount = SendMessageA(hList, LB_GETCOUNT, 0, 0);
    ok (itemCount == itemCount_justDrives,
        "SendMessage(LB_DIR, DDL_DIRECTORY|DDL_DRIVES|DDL_EXCLUSIVE) filled with %d entries, expected %d\n",
        itemCount, itemCount_justDrives);
    ok(res + 1 == itemCount, "SendMessage(LB_DIR, DDL_DIRECTORY|DDL_DRIVES|DDL_EXCLUSIVE, w*.c) returned incorrect index!\n");

    for (i = 0; i < itemCount; i++)
    {
        memset(pathBuffer, 0, MAX_PATH);
        driveletter = '\0';
        SendMessageA(hList, LB_GETTEXT, i, (LPARAM)pathBuffer);
        ok (sscanf(pathBuffer, "[-%c-]", &driveletter) == 1, "Element %d (%s) does not fit [-X-]\n", i, pathBuffer);
        ok( driveletter >= 'a' && driveletter <= 'z', "Drive letter not in range a..z, got ascii %d\n", driveletter);
    }
    DestroyWindow(hList);

    DeleteFileA( "wtest1.tmp.c" );
    RemoveDirectoryA("lb_dir_test");

    SetCurrentDirectoryA(curdir);
}

static HWND g_listBox;
static HWND g_label;

#define ID_TEST_LABEL    1001
#define ID_TEST_LISTBOX  1002

static BOOL on_listbox_container_create(HWND hwnd, CREATESTRUCTA *lpcs)
{
    g_label = CreateWindowA(WC_STATICA, "Contents of static control before DlgDirList.",
        WS_CHILD | WS_VISIBLE, 10, 10, 512, 32, hwnd, (HMENU)ID_TEST_LABEL, NULL, 0);
    if (!g_label) return FALSE;

    g_listBox = CreateWindowA(WC_LISTBOXA, "DlgDirList test",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | WS_VSCROLL, 10, 60, 256, 256,
        hwnd, (HMENU)ID_TEST_LISTBOX, NULL, 0);
    if (!g_listBox) return FALSE;

    return TRUE;
}

static LRESULT CALLBACK listbox_container_window_procA(HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    switch (uiMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_CREATE:
        result = on_listbox_container_create(hwnd, (CREATESTRUCTA *)lParam) ? 0 : (LRESULT)-1;
        break;
    default:
        result = DefWindowProcA(hwnd, uiMsg, wParam, lParam);
        break;
    }
    return result;
}

static BOOL RegisterListboxWindowClass(HINSTANCE hInst)
{
    WNDCLASSA cls;

    cls.style = 0;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = hInst;
    cls.hIcon = NULL;
    cls.hCursor = LoadCursorA (NULL, (LPCSTR)IDC_ARROW);
    cls.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    cls.lpszMenuName = NULL;
    cls.lpfnWndProc = listbox_container_window_procA;
    cls.lpszClassName = "ListboxContainerClass";
    if (!RegisterClassA (&cls)) return FALSE;

    return TRUE;
}

static void test_listbox_dlgdir(void)
{
    HINSTANCE hInst;
    HWND hWnd;
    int res, itemCount;
    int itemCount_allDirs;
    int itemCount_justFiles;
    int itemCount_justDrives;
    int i;
    char pathBuffer[MAX_PATH];
    char itemBuffer[MAX_PATH];
    char tempBuffer[MAX_PATH];
    char * p;
    char driveletter;
    HANDLE file;
    BOOL ret;

    file = CreateFileA( "wtest1.tmp.c", GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL );
    ok(file != INVALID_HANDLE_VALUE, "Error creating the test file: %ld\n", GetLastError());
    CloseHandle( file );

    /* NOTE: for this test to succeed, there must be no subdirectories
       under the current directory. In addition, there must be at least
       one file that fits the wildcard w*.c . Normally, the test
       directory itself satisfies both conditions.
     */

    hInst = GetModuleHandleA(0);
    ret = RegisterListboxWindowClass(hInst);
    ok(ret, "Failed to register test class.\n");

    hWnd = CreateWindowA("ListboxContainerClass", "ListboxContainerClass",
                    WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            NULL, NULL, hInst, 0);
    ok(hWnd != NULL, "Failed to create container window.\n");

    /* Test for standard usage */

    /* The following should be overwritten by the directory path */
    SendMessageA(g_label, WM_SETTEXT, 0, (LPARAM)"default contents");

    /* This should list all the w*.c files in the test directory
     * As of this writing, this includes win.c, winstation.c, wsprintf.c
     */
    strcpy(pathBuffer, "w*.c");
    res = DlgDirListA(hWnd, pathBuffer, ID_TEST_LISTBOX, ID_TEST_LABEL, 0);
    ok (res == 1, "DlgDirList(*.c, 0) returned %d - expected 1 - 0x%08lx\n", res, GetLastError());

    /* Path specification gets converted to uppercase */
    ok (!strcmp(pathBuffer, "W*.C"),
        "expected conversion to uppercase, got %s\n", pathBuffer);

    /* Loaded path should have overwritten the label text */
    SendMessageA(g_label, WM_GETTEXT, MAX_PATH, (LPARAM)pathBuffer);
    ok (strcmp("default contents", pathBuffer), "DlgDirList() did not modify static control!\n");

    /* There should be some content in the listbox */
    itemCount = SendMessageA(g_listBox, LB_GETCOUNT, 0, 0);
    ok (itemCount > 0, "DlgDirList() did NOT fill the listbox!\n");
    itemCount_justFiles = itemCount;

    /* Every single item in the control should start with a w and end in .c */
    for (i = 0; i < itemCount; i++)
    {
        memset(pathBuffer, 0, MAX_PATH);
        SendMessageA(g_listBox, LB_GETTEXT, i, (LPARAM)pathBuffer);
        p = pathBuffer + strlen(pathBuffer);
        ok(((pathBuffer[0] == 'w' || pathBuffer[0] == 'W') &&
            (*(p-1) == 'c' || *(p-1) == 'C') &&
            (*(p-2) == '.')), "Element %d (%s) does not fit requested w*.c\n", i, pathBuffer);
    }

    /* Test behavior when no files match the wildcard */
    strcpy(pathBuffer, BAD_EXTENSION);
    res = DlgDirListA(hWnd, pathBuffer, ID_TEST_LISTBOX, ID_TEST_LABEL, 0);
    ok (res == 1, "DlgDirList(%s, 0) returned %d expected 1\n", BAD_EXTENSION, res);

    itemCount = SendMessageA(g_listBox, LB_GETCOUNT, 0, 0);
    ok (itemCount == 0, "DlgDirList() DID fill the listbox!\n");

    /* Test DDL_DIRECTORY */
    strcpy(pathBuffer, "w*.c");
    res = DlgDirListA(hWnd, pathBuffer, ID_TEST_LISTBOX, ID_TEST_LABEL, DDL_DIRECTORY);
    ok (res == 1, "DlgDirList(*.c, DDL_DIRECTORY) failed - 0x%08lx\n", GetLastError());

    /* There should be some content in the listbox. In particular, there should
     * be exactly more elements than before, since the directories should
     * have been added.
     */
    itemCount = SendMessageA(g_listBox, LB_GETCOUNT, 0, 0);
    itemCount_allDirs = itemCount - itemCount_justFiles;
    ok (itemCount >= itemCount_justFiles, "DlgDirList(DDL_DIRECTORY) filled with %d entries, expected > %d\n",
        itemCount, itemCount_justFiles);

    /* Every single item in the control should start with a w and end in .c,
     * except for the "[..]" string, which should appear exactly as it is.
     */
    for (i = 0; i < itemCount; i++)
    {
        memset(pathBuffer, 0, MAX_PATH);
        SendMessageA(g_listBox, LB_GETTEXT, i, (LPARAM)pathBuffer);
        p = pathBuffer + strlen(pathBuffer);
        ok( (pathBuffer[0] == '[' && pathBuffer[strlen(pathBuffer)-1] == ']') ||
            ((pathBuffer[0] == 'w' || pathBuffer[0] == 'W') &&
            (*(p-1) == 'c' || *(p-1) == 'C') &&
            (*(p-2) == '.')), "Element %d (%s) does not fit requested w*.c\n", i, pathBuffer);
    }

    /* Test behavior when no files match the wildcard */
    strcpy(pathBuffer, BAD_EXTENSION);
    res = DlgDirListA(hWnd, pathBuffer, ID_TEST_LISTBOX, ID_TEST_LABEL, DDL_DIRECTORY);
    ok (res == 1, "DlgDirList(%s, DDL_DIRECTORY) returned %d expected 1\n", BAD_EXTENSION, res);

    itemCount = SendMessageA(g_listBox, LB_GETCOUNT, 0, 0);
    ok (itemCount == itemCount_allDirs, "DlgDirList() incorrectly filled the listbox! (expected %d got %d)\n",
        itemCount_allDirs, itemCount);
    for (i = 0; i < itemCount; i++)
    {
        memset(pathBuffer, 0, MAX_PATH);
        SendMessageA(g_listBox, LB_GETTEXT, i, (LPARAM)pathBuffer);
        ok( pathBuffer[0] == '[' && pathBuffer[strlen(pathBuffer)-1] == ']',
            "Element %d (%s) does not fit requested [...]\n", i, pathBuffer);
    }

    /* Test DDL_DRIVES. At least on WinXP-SP2, this implies DDL_EXCLUSIVE */
    strcpy(pathBuffer, "w*.c");
    res = DlgDirListA(hWnd, pathBuffer, ID_TEST_LISTBOX, ID_TEST_LABEL, DDL_DRIVES);
    ok (res == 1, "DlgDirList(*.c, DDL_DRIVES) failed - 0x%08lx\n", GetLastError());

    /* There should be some content in the listbox. In particular, there should
     * be at least one element before, since the string "[-c-]" should
     * have been added. Depending on the user setting, more drives might have
     * been added.
     */
    itemCount = SendMessageA(g_listBox, LB_GETCOUNT, 0, 0);
    ok (itemCount >= 1,
        "DlgDirList(DDL_DRIVES) filled with %d entries, expected at least %d\n",
        itemCount, 1);
    itemCount_justDrives = itemCount;

    /* Every single item in the control should fit the format [-c-] */
    for (i = 0; i < itemCount; i++)
    {
        memset(pathBuffer, 0, MAX_PATH);
        driveletter = '\0';
        SendMessageA(g_listBox, LB_GETTEXT, i, (LPARAM)pathBuffer);
        ok( strlen(pathBuffer) == 5, "Length of drive string is not 5\n" );
        ok( sscanf(pathBuffer, "[-%c-]", &driveletter) == 1, "Element %d (%s) does not fit [-X-]\n", i, pathBuffer);
        ok( driveletter >= 'a' && driveletter <= 'z', "Drive letter not in range a..z, got ascii %d\n", driveletter);
        if (!(driveletter >= 'a' && driveletter <= 'z')) {
            /* Correct after invalid entry is found */
            trace("removing count of invalid entry %s\n", pathBuffer);
            itemCount_justDrives--;
        }
    }

    /* Test behavior when no files match the wildcard */
    strcpy(pathBuffer, BAD_EXTENSION);
    res = DlgDirListA(hWnd, pathBuffer, ID_TEST_LISTBOX, ID_TEST_LABEL, DDL_DRIVES);
    ok (res == 1, "DlgDirList(%s, DDL_DRIVES) returned %d expected 1\n", BAD_EXTENSION, res);

    itemCount = SendMessageA(g_listBox, LB_GETCOUNT, 0, 0);
    ok (itemCount == itemCount_justDrives, "DlgDirList() incorrectly filled the listbox!\n");

    /* Test DDL_DIRECTORY|DDL_DRIVES. This does *not* imply DDL_EXCLUSIVE */
    strcpy(pathBuffer, "w*.c");
    res = DlgDirListA(hWnd, pathBuffer, ID_TEST_LISTBOX, ID_TEST_LABEL, DDL_DIRECTORY|DDL_DRIVES);
    ok (res == 1, "DlgDirList(*.c, DDL_DIRECTORY|DDL_DRIVES) failed - 0x%08lx\n", GetLastError());

    /* There should be some content in the listbox. In particular, there should
     * be exactly the number of plain files, plus the number of mapped drives,
     * plus one "[..]"
     */
    itemCount = SendMessageA(g_listBox, LB_GETCOUNT, 0, 0);
    ok (itemCount == itemCount_justFiles + itemCount_justDrives + itemCount_allDirs,
        "DlgDirList(DDL_DIRECTORY|DDL_DRIVES) filled with %d entries, expected %d\n",
        itemCount, itemCount_justFiles + itemCount_justDrives + itemCount_allDirs);

    /* Every single item in the control should start with a w and end in .c,
     * except for the "[..]" string, which should appear exactly as it is,
     * and the mapped drives in the format "[-X-]".
     */
    for (i = 0; i < itemCount; i++)
    {
        memset(pathBuffer, 0, MAX_PATH);
        driveletter = '\0';
        SendMessageA(g_listBox, LB_GETTEXT, i, (LPARAM)pathBuffer);
        p = pathBuffer + strlen(pathBuffer);
        if (sscanf(pathBuffer, "[-%c-]", &driveletter) == 1)
            ok( driveletter >= 'a' && driveletter <= 'z', "Drive letter not in range a..z, got ascii %d\n", driveletter);
        else
            ok( (pathBuffer[0] == '[' && pathBuffer[strlen(pathBuffer)-1] == ']') ||
                ((pathBuffer[0] == 'w' || pathBuffer[0] == 'W') &&
                (*(p-1) == 'c' || *(p-1) == 'C') &&
                (*(p-2) == '.')), "Element %d (%s) does not fit requested w*.c\n", i, pathBuffer);
    }

    /* Test behavior when no files match the wildcard */
    strcpy(pathBuffer, BAD_EXTENSION);
    res = DlgDirListA(hWnd, pathBuffer, ID_TEST_LISTBOX, ID_TEST_LABEL, DDL_DIRECTORY|DDL_DRIVES);
    ok (res == 1, "DlgDirList(%s, DDL_DIRECTORY|DDL_DRIVES) returned %d expected 1\n", BAD_EXTENSION, res);

    itemCount = SendMessageA(g_listBox, LB_GETCOUNT, 0, 0);
    ok (itemCount == itemCount_justDrives + itemCount_allDirs,
        "DlgDirList() incorrectly filled the listbox! (expected %d got %d)\n",
        itemCount_justDrives + itemCount_allDirs, itemCount);

    /* Test DDL_DIRECTORY|DDL_EXCLUSIVE. */
    strcpy(pathBuffer, "w*.c");
    res = DlgDirListA(hWnd, pathBuffer, ID_TEST_LISTBOX, ID_TEST_LABEL, DDL_DIRECTORY|DDL_EXCLUSIVE);
    ok (res == 1, "DlgDirList(*.c, DDL_DIRECTORY|DDL_EXCLUSIVE) failed - 0x%08lx\n", GetLastError());

    /* There should be exactly one element: "[..]" */
    itemCount = SendMessageA(g_listBox, LB_GETCOUNT, 0, 0);
    ok (itemCount == itemCount_allDirs,
        "DlgDirList(DDL_DIRECTORY|DDL_EXCLUSIVE) filled with %d entries, expected %d\n",
        itemCount, itemCount_allDirs);

    if (itemCount && GetCurrentDirectoryA( MAX_PATH, pathBuffer ) > 3)  /* there's no [..] in drive root */
    {
        memset(pathBuffer, 0, MAX_PATH);
        SendMessageA(g_listBox, LB_GETTEXT, 0, (LPARAM)pathBuffer);
        ok( !strcmp(pathBuffer, "[..]"), "First (and only) element is not [..]\n");
    }

    /* Test behavior when no files match the wildcard */
    strcpy(pathBuffer, BAD_EXTENSION);
    res = DlgDirListA(hWnd, pathBuffer, ID_TEST_LISTBOX, ID_TEST_LABEL, DDL_DIRECTORY|DDL_EXCLUSIVE);
    ok (res == 1, "DlgDirList(%s, DDL_DIRECTORY|DDL_EXCLUSIVE) returned %d expected 1\n", BAD_EXTENSION, res);

    itemCount = SendMessageA(g_listBox, LB_GETCOUNT, 0, 0);
    ok (itemCount == itemCount_allDirs, "DlgDirList() incorrectly filled the listbox!\n");

    /* Test DDL_DIRECTORY|DDL_DRIVES|DDL_EXCLUSIVE. */
    strcpy(pathBuffer, "w*.c");
    res = DlgDirListA(hWnd, pathBuffer, ID_TEST_LISTBOX, ID_TEST_LABEL, DDL_DIRECTORY|DDL_DRIVES|DDL_EXCLUSIVE);
    ok (res == 1, "DlgDirList(*.c, DDL_DIRECTORY|DDL_DRIVES|DDL_EXCLUSIVE) failed - 0x%08lx\n", GetLastError());

    /* There should be no plain files on the listbox */
    itemCount = SendMessageA(g_listBox, LB_GETCOUNT, 0, 0);
    ok (itemCount == itemCount_justDrives + itemCount_allDirs,
        "DlgDirList(DDL_DIRECTORY|DDL_EXCLUSIVE) filled with %d entries, expected %d\n",
        itemCount, itemCount_justDrives + itemCount_allDirs);

    for (i = 0; i < itemCount; i++)
    {
        memset(pathBuffer, 0, MAX_PATH);
        driveletter = '\0';
        SendMessageA(g_listBox, LB_GETTEXT, i, (LPARAM)pathBuffer);
        if (sscanf(pathBuffer, "[-%c-]", &driveletter) == 1)
            ok( driveletter >= 'a' && driveletter <= 'z', "Drive letter not in range a..z, got ascii %d\n", driveletter);
        else
            ok( pathBuffer[0] == '[' && pathBuffer[strlen(pathBuffer)-1] == ']',
                "Element %d (%s) does not fit expected [...]\n", i, pathBuffer);
    }

    /* Test behavior when no files match the wildcard */
    strcpy(pathBuffer, BAD_EXTENSION);
    res = DlgDirListA(hWnd, pathBuffer, ID_TEST_LISTBOX, ID_TEST_LABEL, DDL_DIRECTORY|DDL_DRIVES|DDL_EXCLUSIVE);
    ok (res == 1, "DlgDirList(%s, DDL_DIRECTORY|DDL_DRIVES|DDL_EXCLUSIVE) returned %d expected 1\n", BAD_EXTENSION, res);

    itemCount = SendMessageA(g_listBox, LB_GETCOUNT, 0, 0);
    ok (itemCount == itemCount_justDrives + itemCount_allDirs, "DlgDirList() incorrectly filled the listbox!\n");

    /* Now test DlgDirSelectEx() in normal operation */
    /* Fill with everything - drives, directory and all plain files. */
    strcpy(pathBuffer, "*");
    res = DlgDirListA(hWnd, pathBuffer, ID_TEST_LISTBOX, ID_TEST_LABEL, DDL_DIRECTORY|DDL_DRIVES);
    ok (res != 0, "DlgDirList(*, DDL_DIRECTORY|DDL_DRIVES) failed - 0x%08lx\n", GetLastError());

    SendMessageA(g_listBox, LB_SETCURSEL, -1, 0); /* Unselect any current selection */
    memset(pathBuffer, 0, MAX_PATH);
    SetLastError(0xdeadbeef);
    res = DlgDirSelectExA(hWnd, pathBuffer, MAX_PATH, ID_TEST_LISTBOX);
    ok (GetLastError() == 0xdeadbeef,
        "DlgDirSelectEx() with no selection modified last error code from 0xdeadbeef to 0x%08lx\n",
        GetLastError());
    ok (res == 0, "DlgDirSelectEx() with no selection returned %d, expected 0\n", res);
    /* WinXP-SP2 leaves pathBuffer untouched, but Win98 fills it with garbage. */
    /*
    ok (!*pathBuffer, "DlgDirSelectEx() with no selection filled buffer with %s\n", pathBuffer);
    */
    /* Test proper drive/dir/file recognition */
    itemCount = SendMessageA(g_listBox, LB_GETCOUNT, 0, 0);
    for (i = 0; i < itemCount; i++)
    {
        memset(itemBuffer, 0, MAX_PATH);
        memset(pathBuffer, 0, MAX_PATH);
        memset(tempBuffer, 0, MAX_PATH);
        driveletter = '\0';
        SendMessageA(g_listBox, LB_GETTEXT, i, (LPARAM)itemBuffer);
        res = SendMessageA(g_listBox, LB_SETCURSEL, i, 0);
        ok (res == i, "SendMessageA(LB_SETCURSEL, %d) failed\n", i);
        if (sscanf(itemBuffer, "[-%c-]", &driveletter) == 1)
        {
            /* Current item is a drive letter */
            SetLastError(0xdeadbeef);
            res = DlgDirSelectExA(hWnd, pathBuffer, MAX_PATH, ID_TEST_LISTBOX);
            ok (GetLastError() == 0xdeadbeef,
               "DlgDirSelectEx() with selection at %d modified last error code from 0xdeadbeef to 0x%08lx\n",
                i, GetLastError());
            ok(res == 1, "DlgDirSelectEx() thinks %s (%s) is not a drive/directory!\n", itemBuffer, pathBuffer);

            /* For drive letters, DlgDirSelectEx tacks on a colon */
            ok (pathBuffer[0] == driveletter && pathBuffer[1] == ':' && pathBuffer[2] == '\0',
                "%d: got \"%s\" expected \"%c:\"\n", i, pathBuffer, driveletter);
        }
        else if (itemBuffer[0] == '[')
        {
            /* Current item is the parent directory */
            SetLastError(0xdeadbeef);
            res = DlgDirSelectExA(hWnd, pathBuffer, MAX_PATH, ID_TEST_LISTBOX);
            ok (GetLastError() == 0xdeadbeef,
               "DlgDirSelectEx() with selection at %d modified last error code from 0xdeadbeef to 0x%08lx\n",
                i, GetLastError());
            ok(res == 1, "DlgDirSelectEx() thinks %s (%s) is not a drive/directory!\n", itemBuffer, pathBuffer);

            /* For directories, DlgDirSelectEx tacks on a backslash */
            p = pathBuffer + strlen(pathBuffer);
            ok (*(p-1) == '\\', "DlgDirSelectEx did NOT tack on a backslash to dir, got %s\n", pathBuffer);

            tempBuffer[0] = '[';
            lstrcpynA(tempBuffer + 1, pathBuffer, strlen(pathBuffer));
            strcat(tempBuffer, "]");
            ok (!strcmp(tempBuffer, itemBuffer), "Formatted directory should be %s, got %s\n", tempBuffer, itemBuffer);
        }
        else
        {
            /* Current item is a plain file */
            SetLastError(0xdeadbeef);
            res = DlgDirSelectExA(hWnd, pathBuffer, MAX_PATH, ID_TEST_LISTBOX);
            ok (GetLastError() == 0xdeadbeef,
               "DlgDirSelectEx() with selection at %d modified last error code from 0xdeadbeef to 0x%08lx\n",
                i, GetLastError());
            ok(res == 0, "DlgDirSelectEx() thinks %s (%s) is a drive/directory!\n", itemBuffer, pathBuffer);

            /* NOTE: WinXP tacks a period on all files that lack an extension. This affects
             * for example, "Makefile", which gets reported as "Makefile."
             */
            strcpy(tempBuffer, itemBuffer);
            if (strchr(tempBuffer, '.') == NULL) strcat(tempBuffer, ".");
            ok (!strcmp(pathBuffer, tempBuffer), "Formatted file should be %s, got %s\n", tempBuffer, pathBuffer);
        }
    }

    DeleteFileA( "wtest1.tmp.c" );

    /* Now test DlgDirSelectEx() in abnormal operation */
    /* Fill list with bogus entries, that look somewhat valid */
    SendMessageA(g_listBox, LB_RESETCONTENT, 0, 0);
    SendMessageA(g_listBox, LB_ADDSTRING, 0, (LPARAM)"[notexist.dir]");
    SendMessageA(g_listBox, LB_ADDSTRING, 0, (LPARAM)"notexist.fil");
    itemCount = SendMessageA(g_listBox, LB_GETCOUNT, 0, 0);
    for (i = 0; i < itemCount; i++)
    {
        memset(itemBuffer, 0, MAX_PATH);
        memset(pathBuffer, 0, MAX_PATH);
        memset(tempBuffer, 0, MAX_PATH);
        driveletter = '\0';
        SendMessageA(g_listBox, LB_GETTEXT, i, (LPARAM)itemBuffer);
        res = SendMessageA(g_listBox, LB_SETCURSEL, i, 0);
        ok (res == i, "SendMessage(LB_SETCURSEL, %d) failed\n", i);
        if (sscanf(itemBuffer, "[-%c-]", &driveletter) == 1)
        {
            /* Current item is a drive letter */
            SetLastError(0xdeadbeef);
            res = DlgDirSelectExA(hWnd, pathBuffer, MAX_PATH, ID_TEST_LISTBOX);
            ok (GetLastError() == 0xdeadbeef,
               "DlgDirSelectEx() with selection at %d modified last error code from 0xdeadbeef to 0x%08lx\n",
                i, GetLastError());
            ok(res == 1, "DlgDirSelectEx() thinks %s (%s) is not a drive/directory!\n", itemBuffer, pathBuffer);

            /* For drive letters, DlgDirSelectEx tacks on a colon */
            ok (pathBuffer[0] == driveletter && pathBuffer[1] == ':' && pathBuffer[2] == '\0',
                "%d: got \"%s\" expected \"%c:\"\n", i, pathBuffer, driveletter);
        }
        else if (itemBuffer[0] == '[')
        {
            /* Current item is the parent directory */
            SetLastError(0xdeadbeef);
            res = DlgDirSelectExA(hWnd, pathBuffer, MAX_PATH, ID_TEST_LISTBOX);
            ok (GetLastError() == 0xdeadbeef,
               "DlgDirSelectEx() with selection at %d modified last error code from 0xdeadbeef to 0x%08lx\n",
                i, GetLastError());
            ok(res == 1, "DlgDirSelectEx() thinks %s (%s) is not a drive/directory!\n", itemBuffer, pathBuffer);

            /* For directories, DlgDirSelectEx tacks on a backslash */
            p = pathBuffer + strlen(pathBuffer);
            ok (*(p-1) == '\\', "DlgDirSelectEx did NOT tack on a backslash to dir, got %s\n", pathBuffer);

            tempBuffer[0] = '[';
            lstrcpynA(tempBuffer + 1, pathBuffer, strlen(pathBuffer));
            strcat(tempBuffer, "]");
            ok (!strcmp(tempBuffer, itemBuffer), "Formatted directory should be %s, got %s\n", tempBuffer, itemBuffer);
        }
        else
        {
            /* Current item is a plain file */
            SetLastError(0xdeadbeef);
            res = DlgDirSelectExA(hWnd, pathBuffer, MAX_PATH, ID_TEST_LISTBOX);
            ok (GetLastError() == 0xdeadbeef,
               "DlgDirSelectEx() with selection at %d modified last error code from 0xdeadbeef to 0x%08lx\n",
                i, GetLastError());
            ok(res == 0, "DlgDirSelectEx() thinks %s (%s) is a drive/directory!\n", itemBuffer, pathBuffer);

            /* NOTE: WinXP and Win98 tack a period on all files that lack an extension.
             * This affects for example, "Makefile", which gets reported as "Makefile."
             */
            strcpy(tempBuffer, itemBuffer);
            if (strchr(tempBuffer, '.') == NULL) strcat(tempBuffer, ".");
            ok (!strcmp(pathBuffer, tempBuffer), "Formatted file should be %s, got %s\n", tempBuffer, pathBuffer);
        }
    }

    /* Test behavior when loading folders from root with and without wildcard */
    strcpy(pathBuffer, "C:\\");
    res = DlgDirListA(hWnd, pathBuffer, ID_TEST_LISTBOX, 0, DDL_DIRECTORY | DDL_EXCLUSIVE);
    ok(res, "DlgDirList failed to list C:\\ folders\n");
    ok(!strcmp(pathBuffer, "*"), "DlgDirList set the invalid path spec '%s', expected '*'\n", pathBuffer);

    strcpy(pathBuffer, "C:\\*");
    res = DlgDirListA(hWnd, pathBuffer, ID_TEST_LISTBOX, 0, DDL_DIRECTORY | DDL_EXCLUSIVE);
    ok(res, "DlgDirList failed to list C:\\* folders\n");
    ok(!strcmp(pathBuffer, "*"), "DlgDirList set the invalid path spec '%s', expected '*'\n", pathBuffer);

    /* Try loading files from an invalid folder */
    SetLastError(0xdeadbeef);
    strcpy(pathBuffer, "C:\\INVALID$$DIR");
    res = DlgDirListA(hWnd, pathBuffer, ID_TEST_LISTBOX, 0, DDL_DIRECTORY | DDL_EXCLUSIVE);
    ok(!res, "DlgDirList should have failed with 0 but %d was returned\n", res);
    ok(GetLastError() == ERROR_NO_WILDCARD_CHARACTERS,
       "GetLastError should return 0x589, got 0x%lX\n",GetLastError());

    DestroyWindow(hWnd);
}

static void test_set_count( void )
{
    static const DWORD styles[] =
    {
        LBS_OWNERDRAWFIXED,
        LBS_HASSTRINGS,
    };
    HWND parent, listbox;
    unsigned int i;
    LONG ret;
    RECT r;

    parent = create_parent();
    listbox = create_listbox( LBS_OWNERDRAWFIXED | LBS_NODATA | WS_CHILD | WS_VISIBLE, parent );

    UpdateWindow( listbox );
    GetUpdateRect( listbox, &r, TRUE );
    ok( IsRectEmpty( &r ), "got non-empty rect\n");

    ret = SendMessageA( listbox, LB_SETCOUNT, 100, 0 );
    ok( ret == 0, "got %ld\n", ret );
    ret = SendMessageA( listbox, LB_GETCOUNT, 0, 0 );
    ok( ret == 100, "got %ld\n", ret );

    GetUpdateRect( listbox, &r, TRUE );
    ok( !IsRectEmpty( &r ), "got empty rect\n");

    ValidateRect( listbox, NULL );
    GetUpdateRect( listbox, &r, TRUE );
    ok( IsRectEmpty( &r ), "got non-empty rect\n");

    ret = SendMessageA( listbox, LB_SETCOUNT, 99, 0 );
    ok( ret == 0, "got %ld\n", ret );

    GetUpdateRect( listbox, &r, TRUE );
    ok( !IsRectEmpty( &r ), "got empty rect\n");

    ret = SendMessageA( listbox, LB_SETCOUNT, -5, 0 );
    ok( ret == 0, "got %ld\n", ret );
    ret = SendMessageA( listbox, LB_GETCOUNT, 0, 0 );
    ok( ret == -5, "got %ld\n", ret );

    DestroyWindow( listbox );

    for (i = 0; i < ARRAY_SIZE(styles); ++i)
    {
        listbox = create_listbox( styles[i] | WS_CHILD | WS_VISIBLE, parent );

        SetLastError( 0xdeadbeef );
        ret = SendMessageA( listbox, LB_SETCOUNT, 100, 0 );
        ok( ret == LB_ERR, "expected %d, got %ld\n", LB_ERR, ret );
        ok( GetLastError() == 0xdeadbeef, "Unexpected error %ld.\n", GetLastError() );

        DestroyWindow( listbox );
    }

    DestroyWindow( parent );
}

static void test_GetListBoxInfo(void)
{
    static const struct message getlistboxinfo_seq[] =
    {
        { LB_GETLISTBOXINFO, sent },
        { 0 }
    };
    HWND listbox, parent;
    DWORD ret;

    parent = create_parent();
    listbox = create_listbox(WS_CHILD | WS_VISIBLE, parent);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    ret = GetListBoxInfo(listbox);
    ok(ret > 0, "got %ld\n", ret);
    ok_sequence(sequences, LB_SEQ_INDEX, getlistboxinfo_seq, "GetListBoxInfo()", FALSE);

    DestroyWindow(listbox);
    DestroyWindow(parent);
}

static void test_init_storage( void )
{
    static const DWORD styles[] =
    {
        LBS_HASSTRINGS,
        LBS_NODATA | LBS_OWNERDRAWFIXED,
    };
    HWND parent, listbox;
    LONG ret, items_size;
    int i, j;

    parent = create_parent();
    for (i = 0; i < ARRAY_SIZE(styles); i++)
    {
        listbox = CreateWindowA(WC_LISTBOXA, "TestList", styles[i] | WS_CHILD,
                                0, 0, 100, 100, parent, (HMENU)ID_LISTBOX, NULL, 0);

        items_size = SendMessageA(listbox, LB_INITSTORAGE, 100, 0);
        ok(items_size >= 100, "expected at least 100, got %ld\n", items_size);

        ret = SendMessageA(listbox, LB_INITSTORAGE, 0, 0);
        ok(ret == items_size, "expected %ld, got %ld\n", items_size, ret);

        /* it doesn't grow since the space was already reserved */
        ret = SendMessageA(listbox, LB_INITSTORAGE, items_size, 0);
        ok(ret == items_size, "expected %ld, got %ld\n", items_size, ret);

        /* it doesn't shrink the reserved space */
        ret = SendMessageA(listbox, LB_INITSTORAGE, 42, 0);
        ok(ret == items_size, "expected %ld, got %ld\n", items_size, ret);

        /* now populate almost all of it so it's not reserved anymore */
        if (styles[i] & LBS_NODATA)
        {
            ret = SendMessageA(listbox, LB_SETCOUNT, items_size - 1, 0);
            ok(ret == 0, "unexpected return value %ld\n", ret);
        }
        else
        {
            for (j = 0; j < items_size - 1; j++)
            {
                ret = SendMessageA(listbox, LB_INSERTSTRING, -1, (LPARAM)"");
                ok(ret == j, "expected %d, got %ld\n", j, ret);
            }
        }

        /* we still have one more reserved slot, so it doesn't grow yet */
        ret = SendMessageA(listbox, LB_INITSTORAGE, 1, 0);
        ok(ret == items_size, "expected %ld, got %ld\n", items_size, ret);

        /* fill the slot and check again, it should grow this time */
        ret = SendMessageA(listbox, LB_INSERTSTRING, -1, (LPARAM)"");
        ok(ret == items_size - 1, "expected %ld, got %ld\n", items_size - 1, ret);
        ret = SendMessageA(listbox, LB_INITSTORAGE, 0, 0);
        ok(ret == items_size, "expected %ld, got %ld\n", items_size, ret);
        ret = SendMessageA(listbox, LB_INITSTORAGE, 1, 0);
        ok(ret > items_size, "expected it to grow past %ld, got %ld\n", items_size, ret);

        DestroyWindow(listbox);
    }
    DestroyWindow(parent);
}

static void test_missing_lbuttonup(void)
{
    HWND listbox, parent, capture;

    parent = create_parent();
    listbox = create_listbox(WS_CHILD | WS_VISIBLE, parent);

    /* Send button down without a corresponding button up */
    SendMessageA(listbox, WM_LBUTTONDOWN, 0, MAKELPARAM(10, 10));
    capture = GetCapture();
    ok(capture == listbox, "got %p expected %p\n", capture, listbox);

    /* Capture is released and LBN_SELCHANGE sent during WM_KILLFOCUS */
    got_selchange = 0;
    SetFocus(NULL);
    capture = GetCapture();
    ok(capture == NULL, "got %p\n", capture);
    ok(got_selchange, "got %d\n", got_selchange);

    DestroyWindow(listbox);
    DestroyWindow(parent);
}

static void test_extents(void)
{
    HWND listbox, parent;
    SCROLLINFO sinfo;
    DWORD res;
    BOOL br;

    parent = create_parent();

    listbox = create_listbox(WS_CHILD | WS_VISIBLE, parent);

    res = SendMessageA(listbox, LB_GETHORIZONTALEXTENT, 0, 0);
    ok(res == 0, "Got wrong initial horizontal extent: %lu\n", res);

    sinfo.cbSize = sizeof(sinfo);
    sinfo.fMask = SIF_RANGE;
    br = GetScrollInfo(listbox, SB_HORZ, &sinfo);
    ok(br == TRUE, "GetScrollInfo failed\n");
    ok(sinfo.nMin == 0, "got wrong min: %u\n", sinfo.nMin);
    ok(sinfo.nMax == 100, "got wrong max: %u\n", sinfo.nMax);
    ok((GetWindowLongA(listbox, GWL_STYLE) & WS_HSCROLL) == 0,
        "List box should not have a horizontal scroll bar\n");

    /* horizontal extent < width */
    SendMessageA(listbox, LB_SETHORIZONTALEXTENT, 64, 0);

    res = SendMessageA(listbox, LB_GETHORIZONTALEXTENT, 0, 0);
    ok(res == 64, "Got wrong horizontal extent: %lu\n", res);

    sinfo.cbSize = sizeof(sinfo);
    sinfo.fMask = SIF_RANGE;
    br = GetScrollInfo(listbox, SB_HORZ, &sinfo);
    ok(br == TRUE, "GetScrollInfo failed\n");
    ok(sinfo.nMin == 0, "got wrong min: %u\n", sinfo.nMin);
    ok(sinfo.nMax == 100, "got wrong max: %u\n", sinfo.nMax);
    ok((GetWindowLongA(listbox, GWL_STYLE) & WS_HSCROLL) == 0,
        "List box should not have a horizontal scroll bar\n");

    /* horizontal extent > width */
    SendMessageA(listbox, LB_SETHORIZONTALEXTENT, 184, 0);

    res = SendMessageA(listbox, LB_GETHORIZONTALEXTENT, 0, 0);
    ok(res == 184, "Got wrong horizontal extent: %lu\n", res);

    sinfo.cbSize = sizeof(sinfo);
    sinfo.fMask = SIF_RANGE;
    br = GetScrollInfo(listbox, SB_HORZ, &sinfo);
    ok(br == TRUE, "GetScrollInfo failed\n");
    ok(sinfo.nMin == 0, "got wrong min: %u\n", sinfo.nMin);
    ok(sinfo.nMax == 100, "got wrong max: %u\n", sinfo.nMax);
    ok((GetWindowLongA(listbox, GWL_STYLE) & WS_HSCROLL) == 0,
        "List box should not have a horizontal scroll bar\n");

    DestroyWindow(listbox);

    listbox = create_listbox(WS_CHILD | WS_VISIBLE | WS_HSCROLL, parent);

    res = SendMessageA(listbox, LB_GETHORIZONTALEXTENT, 0, 0);
    ok(res == 0, "Got wrong initial horizontal extent: %lu\n", res);

    sinfo.cbSize = sizeof(sinfo);
    sinfo.fMask = SIF_RANGE;
    br = GetScrollInfo(listbox, SB_HORZ, &sinfo);
    ok(br == TRUE, "GetScrollInfo failed\n");
    ok(sinfo.nMin == 0, "got wrong min: %u\n", sinfo.nMin);
    ok(sinfo.nMax == 100, "got wrong max: %u\n", sinfo.nMax);
    ok((GetWindowLongA(listbox, GWL_STYLE) & WS_HSCROLL) == 0,
        "List box should not have a horizontal scroll bar\n");

    /* horizontal extent < width */
    SendMessageA(listbox, LB_SETHORIZONTALEXTENT, 64, 0);

    res = SendMessageA(listbox, LB_GETHORIZONTALEXTENT, 0, 0);
    ok(res == 64, "Got wrong horizontal extent: %lu\n", res);

    sinfo.cbSize = sizeof(sinfo);
    sinfo.fMask = SIF_RANGE;
    br = GetScrollInfo(listbox, SB_HORZ, &sinfo);
    ok(br == TRUE, "GetScrollInfo failed\n");
    ok(sinfo.nMin == 0, "got wrong min: %u\n", sinfo.nMin);
    ok(sinfo.nMax == 63, "got wrong max: %u\n", sinfo.nMax);
    ok((GetWindowLongA(listbox, GWL_STYLE) & WS_HSCROLL) == 0,
        "List box should not have a horizontal scroll bar\n");

    /* horizontal extent > width */
    SendMessageA(listbox, LB_SETHORIZONTALEXTENT, 184, 0);

    res = SendMessageA(listbox, LB_GETHORIZONTALEXTENT, 0, 0);
    ok(res == 184, "Got wrong horizontal extent: %lu\n", res);

    sinfo.cbSize = sizeof(sinfo);
    sinfo.fMask = SIF_RANGE;
    br = GetScrollInfo(listbox, SB_HORZ, &sinfo);
    ok(br == TRUE, "GetScrollInfo failed\n");
    ok(sinfo.nMin == 0, "got wrong min: %u\n", sinfo.nMin);
    ok(sinfo.nMax == 183, "got wrong max: %u\n", sinfo.nMax);
    ok((GetWindowLongA(listbox, GWL_STYLE) & WS_HSCROLL) != 0,
        "List box should have a horizontal scroll bar\n");

    SendMessageA(listbox, LB_SETHORIZONTALEXTENT, 0, 0);

    res = SendMessageA(listbox, LB_GETHORIZONTALEXTENT, 0, 0);
    ok(res == 0, "Got wrong horizontal extent: %lu\n", res);

    sinfo.cbSize = sizeof(sinfo);
    sinfo.fMask = SIF_RANGE;
    br = GetScrollInfo(listbox, SB_HORZ, &sinfo);
    ok(br == TRUE, "GetScrollInfo failed\n");
    ok(sinfo.nMin == 0, "got wrong min: %u\n", sinfo.nMin);
    ok(sinfo.nMax == 0, "got wrong max: %u\n", sinfo.nMax);
    ok((GetWindowLongA(listbox, GWL_STYLE) & WS_HSCROLL) == 0,
        "List box should not have a horizontal scroll bar\n");

    DestroyWindow(listbox);


    listbox = create_listbox(WS_CHILD | WS_VISIBLE | WS_HSCROLL | LBS_DISABLENOSCROLL, parent);

    res = SendMessageA(listbox, LB_GETHORIZONTALEXTENT, 0, 0);
    ok(res == 0, "Got wrong initial horizontal extent: %lu\n", res);

    sinfo.cbSize = sizeof(sinfo);
    sinfo.fMask = SIF_RANGE;
    br = GetScrollInfo(listbox, SB_HORZ, &sinfo);
    ok(br == TRUE, "GetScrollInfo failed\n");
    ok(sinfo.nMin == 0, "got wrong min: %u\n", sinfo.nMin);
    ok(sinfo.nMax == 0, "got wrong max: %u\n", sinfo.nMax);
    ok((GetWindowLongA(listbox, GWL_STYLE) & WS_HSCROLL) != 0,
        "List box should have a horizontal scroll bar\n");

    /* horizontal extent < width */
    SendMessageA(listbox, LB_SETHORIZONTALEXTENT, 64, 0);

    res = SendMessageA(listbox, LB_GETHORIZONTALEXTENT, 0, 0);
    ok(res == 64, "Got wrong horizontal extent: %lu\n", res);

    sinfo.cbSize = sizeof(sinfo);
    sinfo.fMask = SIF_RANGE;
    br = GetScrollInfo(listbox, SB_HORZ, &sinfo);
    ok(br == TRUE, "GetScrollInfo failed\n");
    ok(sinfo.nMin == 0, "got wrong min: %u\n", sinfo.nMin);
    ok(sinfo.nMax == 63, "got wrong max: %u\n", sinfo.nMax);
    ok((GetWindowLongA(listbox, GWL_STYLE) & WS_HSCROLL) != 0,
        "List box should have a horizontal scroll bar\n");

    /* horizontal extent > width */
    SendMessageA(listbox, LB_SETHORIZONTALEXTENT, 184, 0);

    res = SendMessageA(listbox, LB_GETHORIZONTALEXTENT, 0, 0);
    ok(res == 184, "Got wrong horizontal extent: %lu\n", res);

    sinfo.cbSize = sizeof(sinfo);
    sinfo.fMask = SIF_RANGE;
    br = GetScrollInfo(listbox, SB_HORZ, &sinfo);
    ok(br == TRUE, "GetScrollInfo failed\n");
    ok(sinfo.nMin == 0, "got wrong min: %u\n", sinfo.nMin);
    ok(sinfo.nMax == 183, "got wrong max: %u\n", sinfo.nMax);
    ok((GetWindowLongA(listbox, GWL_STYLE) & WS_HSCROLL) != 0,
        "List box should have a horizontal scroll bar\n");

    SendMessageA(listbox, LB_SETHORIZONTALEXTENT, 0, 0);

    res = SendMessageA(listbox, LB_GETHORIZONTALEXTENT, 0, 0);
    ok(res == 0, "Got wrong horizontal extent: %lu\n", res);

    sinfo.cbSize = sizeof(sinfo);
    sinfo.fMask = SIF_RANGE;
    br = GetScrollInfo(listbox, SB_HORZ, &sinfo);
    ok(br == TRUE, "GetScrollInfo failed\n");
    ok(sinfo.nMin == 0, "got wrong min: %u\n", sinfo.nMin);
    ok(sinfo.nMax == 0, "got wrong max: %u\n", sinfo.nMax);
    ok((GetWindowLongA(listbox, GWL_STYLE) & WS_HSCROLL) != 0,
        "List box should have a horizontal scroll bar\n");

    DestroyWindow(listbox);

    DestroyWindow(parent);
}

static void test_listbox(void)
{
    static const struct listbox_test SS =
        /*   {add_style} */
        {{LB_ERR, LB_ERR,      0, LB_ERR}, {0,0,0,0},
         {     1,      1,      1, LB_ERR}, {0,0,0,0},
         {     2,      2,      2, LB_ERR}, {0,0,0,0},
         {LB_ERR, LB_ERR,      0, LB_ERR}, {0,0,0,0}};

        /* {selected, anchor,  caret, selcount}{TODO fields} */
    static const struct listbox_test SS_NS =
        {{LB_ERR, LB_ERR,      0, LB_ERR}, {0,0,0,0},
         {     1,      1,      1, LB_ERR}, {0,0,0,0},
         {     2,      2,      2, LB_ERR}, {0,0,0,0},
         {LB_ERR, LB_ERR,      0, LB_ERR}, {0,0,0,0}};

    static const struct listbox_test MS =
        {{     0, LB_ERR,      0,      0}, {0,0,0,0},
         {     1,      1,      1,      1}, {0,0,0,0},
         {     2,      1,      2,      1}, {0,0,0,0},
         {     0, LB_ERR,      0,      2}, {0,0,0,0}};

    static const struct listbox_test MS_NS =
        {{LB_ERR, LB_ERR,      0, LB_ERR}, {0,0,0,0},
         {     1,      1,      1, LB_ERR}, {0,0,0,0},
         {     2,      2,      2, LB_ERR}, {0,0,0,0},
         {LB_ERR, LB_ERR,      0, LB_ERR}, {0,0,0,0}};

    static const struct listbox_test ES =
        {{     0, LB_ERR,      0,      0}, {0,0,0,0},
         {     1,      1,      1,      1}, {0,0,0,0},
         {     2,      2,      2,      1}, {0,0,0,0},
         {     0, LB_ERR,      0,      2}, {0,0,0,0}};

    static const struct listbox_test ES_NS =
        {{LB_ERR, LB_ERR,      0, LB_ERR}, {0,0,0,0},
         {     1,      1,      1, LB_ERR}, {0,0,0,0},
         {     2,      2,      2, LB_ERR}, {0,0,0,0},
         {LB_ERR, LB_ERR,      0, LB_ERR}, {0,0,0,0}};

    static const struct listbox_test EMS =
        {{     0, LB_ERR,      0,      0}, {0,0,0,0},
         {     1,      1,      1,      1}, {0,0,0,0},
         {     2,      2,      2,      1}, {0,0,0,0},
         {     0, LB_ERR,      0,      2}, {0,0,0,0}};

    static const struct listbox_test EMS_NS =
        {{LB_ERR, LB_ERR,      0, LB_ERR}, {0,0,0,0},
         {     1,      1,      1, LB_ERR}, {0,0,0,0},
         {     2,      2,      2, LB_ERR}, {0,0,0,0},
         {LB_ERR, LB_ERR,      0, LB_ERR}, {0,0,0,0}};

    run_test(0, SS);
    run_test(LBS_NOSEL, SS_NS);
    run_test(LBS_MULTIPLESEL, MS);
    run_test(LBS_MULTIPLESEL | LBS_NOSEL, MS_NS);
    run_test(LBS_EXTENDEDSEL, ES);
    run_test(LBS_EXTENDEDSEL | LBS_NOSEL, ES_NS);
    run_test(LBS_EXTENDEDSEL | LBS_MULTIPLESEL, EMS);
    run_test(LBS_EXTENDEDSEL | LBS_MULTIPLESEL | LBS_NOSEL, EMS_NS);

    run_test(LBS_NODATA | LBS_OWNERDRAWFIXED, SS);
    run_test(LBS_NODATA | LBS_OWNERDRAWFIXED | LBS_NOSEL, SS_NS);
    run_test(LBS_NODATA | LBS_OWNERDRAWFIXED | LBS_MULTIPLESEL, MS);
    run_test(LBS_NODATA | LBS_OWNERDRAWFIXED | LBS_MULTIPLESEL | LBS_NOSEL, MS_NS);
    run_test(LBS_NODATA | LBS_OWNERDRAWFIXED | LBS_EXTENDEDSEL, ES);
    run_test(LBS_NODATA | LBS_OWNERDRAWFIXED | LBS_EXTENDEDSEL | LBS_NOSEL, ES_NS);
    run_test(LBS_NODATA | LBS_OWNERDRAWFIXED | LBS_EXTENDEDSEL | LBS_MULTIPLESEL, EMS);
    run_test(LBS_NODATA | LBS_OWNERDRAWFIXED | LBS_EXTENDEDSEL | LBS_MULTIPLESEL | LBS_NOSEL, EMS_NS);
}

static const struct message lb_addstring_ownerdraw_parent_seq[] =
{
    { WM_MEASUREITEM, sent|wparam|lparam, 0x1012, 0xf30604ed },
    { WM_MEASUREITEM, sent|wparam|lparam, 0x1112, 0xf30604ee },
    { WM_MEASUREITEM, sent|wparam|lparam, 0x1212, 0xf30604ef },
    { 0 }
};

static const struct message lb_addstring_sort_parent_seq[] =
{
    { WM_MEASUREITEM, sent|wparam|lparam, 0x1012, 0xf30604ed },
    { WM_COMPAREITEM, sent|wparam|lparam, 0xf30604ed, 0xf30604ee },
    { WM_MEASUREITEM, sent|wparam|lparam, 0x1112, 0xf30604ee },
    { WM_COMPAREITEM, sent|wparam|lparam, 0xf30604ed, 0xf30604ef },
    { WM_COMPAREITEM, sent|wparam|lparam, 0xf30604ee, 0xf30604ef },
    { WM_MEASUREITEM, sent|wparam|lparam, 0x1212, 0xf30604ef },
    { 0 }
};

static const struct message empty_seq[] =
{
    { 0 }
};

static void test_WM_MEASUREITEM(void)
{
    HWND parent, listbox;
    LRESULT data, ret;

    parent = create_parent();
    listbox = create_listbox(WS_CHILD | LBS_OWNERDRAWVARIABLE, parent);

    data = SendMessageA(listbox, LB_GETITEMDATA, 0, 0);
    ok(data == (LRESULT)strings[0], "data = %08Ix, expected %p\n", data, strings[0]);
    DestroyWindow(listbox);
    DestroyWindow(parent);

    parent = create_parent();
    listbox = create_listbox(WS_CHILD | LBS_OWNERDRAWVARIABLE | LBS_HASSTRINGS, parent);

    data = SendMessageA(listbox, LB_GETITEMDATA, 0, 0);
    ok(!data, "data = %08Ix\n", data);
    DestroyWindow(listbox);
    DestroyWindow(parent);

    /* LBS_HASSTRINGS */
    parent = create_parent();
    listbox = CreateWindowExA(WS_EX_NOPARENTNOTIFY, WC_LISTBOXA, NULL,
                              WS_CHILD | LBS_NOTIFY | LBS_OWNERDRAWVARIABLE | LBS_HASSTRINGS | WS_VISIBLE,
                              10, 10, 80, 80, parent, (HMENU)ID_LISTBOX, 0, NULL);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    ret = SendMessageA(listbox, LB_ADDSTRING, 0, (LPARAM)"item 0");
    ok(ret == 0, "expected 0, got %Id\n", ret);
    ret = SendMessageA(listbox, LB_ADDSTRING, 0, (LPARAM)"item 1");
    ok(ret == 1, "expected 1, got %Id\n", ret);
    ret = SendMessageA(listbox, LB_ADDSTRING, 0, (LPARAM)"item 2");
    ok(ret == 2, "expected 2, got %Id\n", ret);

    ok_sequence(sequences, PARENT_SEQ_INDEX, lb_addstring_ownerdraw_parent_seq,
        "LB_ADDSTRING (LBS_HASSTRINGS, ownerdraw)", FALSE);
    DestroyWindow(listbox);

    /* LBS_SORT, no LBS_HASSTRINGS */
    listbox = CreateWindowExA(WS_EX_NOPARENTNOTIFY, WC_LISTBOXA, NULL,
                              WS_CHILD | LBS_NOTIFY | LBS_OWNERDRAWVARIABLE | LBS_SORT | WS_VISIBLE,
                              10, 10, 80, 80, parent, (HMENU)ID_LISTBOX, 0, NULL);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    ret = SendMessageA(listbox, LB_ADDSTRING, 0, (LPARAM)"item 0");
    ok(ret == 0, "expected 0, got %Id\n", ret);
    ret = SendMessageA(listbox, LB_ADDSTRING, 0, (LPARAM)"item 1");
    ok(ret == 1, "expected 1, got %Id\n", ret);
    ret = SendMessageA(listbox, LB_ADDSTRING, 0, (LPARAM)"item 2");
    ok(ret == 2, "expected 2, got %Id\n", ret);

    ok_sequence(sequences, PARENT_SEQ_INDEX, lb_addstring_sort_parent_seq, "LB_ADDSTRING (LBS_SORT)", FALSE);
    DestroyWindow(listbox);

    /* LBS_HASSTRINGS */
    listbox = CreateWindowExA(WS_EX_NOPARENTNOTIFY, WC_LISTBOXA, NULL,
                              WS_CHILD | LBS_NOTIFY | LBS_HASSTRINGS | WS_VISIBLE,
                              10, 10, 80, 80, parent, (HMENU)ID_LISTBOX, 0, NULL);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    ret = SendMessageA(listbox, LB_ADDSTRING, 0, (LPARAM)"item 2");
    ok(ret == 0, "expected 0, got %Id\n", ret);
    ret = SendMessageA(listbox, LB_ADDSTRING, 0, (LPARAM)"item 0");
    ok(ret == 1, "expected 1, got %Id\n", ret);
    ret = SendMessageA(listbox, LB_ADDSTRING, 0, (LPARAM)"item 1");
    ok(ret == 2, "expected 2, got %Id\n", ret);

    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq, "LB_ADDSTRING (LBS_HASSTRINGS)", FALSE);
    DestroyWindow(listbox);

    /* LBS_HASSTRINGS, LBS_SORT */
    listbox = CreateWindowExA(WS_EX_NOPARENTNOTIFY, WC_LISTBOXA, NULL,
                              WS_CHILD | LBS_NOTIFY | LBS_HASSTRINGS | LBS_SORT | WS_VISIBLE,
                              10, 10, 80, 80, parent, (HMENU)ID_LISTBOX, 0, NULL);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    ret = SendMessageA(listbox, LB_ADDSTRING, 0, (LPARAM)"item 2");
    ok(ret == 0, "expected 0, got %Id\n", ret);
    ret = SendMessageA(listbox, LB_ADDSTRING, 0, (LPARAM)"item 0");
    ok(ret == 0, "expected 0, got %Id\n", ret);
    ret = SendMessageA(listbox, LB_ADDSTRING, 0, (LPARAM)"item 1");
    ok(ret == 1, "expected 1, got %Id\n", ret);

    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq, "LB_ADDSTRING (LBS_HASSTRINGS, LBS_SORT)", FALSE);
    DestroyWindow(listbox);

    DestroyWindow(parent);
}

static void test_LBS_NODATA(void)
{
    static const DWORD invalid_styles[] =
    {
        0,
        LBS_OWNERDRAWVARIABLE,
        LBS_SORT,
        LBS_HASSTRINGS,
        LBS_OWNERDRAWFIXED | LBS_SORT,
        LBS_OWNERDRAWFIXED | LBS_HASSTRINGS,
    };
    static const UINT invalid_idx[] = { -2, 2 };
    static const UINT valid_idx[] = { 0, 1 };
    static const ULONG_PTR zero_data;
    HWND listbox, parent;
    INT ret, text_len;
    unsigned int i;
    ULONG_PTR data;
    BOOL is_wow64;

    listbox = CreateWindowA(WC_LISTBOXA, "TestList", LBS_NODATA | LBS_OWNERDRAWFIXED | WS_VISIBLE,
        0, 0, 100, 100, NULL, NULL, NULL, 0);
    ok(listbox != NULL, "Failed to create ListBox window.\n");

    ret = SendMessageA(listbox, LB_INSERTSTRING, -1, 0);
    ok(ret == 0, "Unexpected return value %d.\n", ret);
    ret = SendMessageA(listbox, LB_INSERTSTRING, -1, 0);
    ok(ret == 1, "Unexpected return value %d.\n", ret);
    ret = SendMessageA(listbox, LB_GETCOUNT, 0, 0);
    ok(ret == 2, "Unexpected return value %d.\n", ret);

    /* Invalid indices. */
    for (i = 0; i < ARRAY_SIZE(invalid_idx); ++i)
    {
        ret = SendMessageA(listbox, LB_SETITEMDATA, invalid_idx[i], 42);
        ok(ret == LB_ERR, "Unexpected return value %d.\n", ret);
        ret = SendMessageA(listbox, LB_GETTEXTLEN, invalid_idx[i], 0);
        ok(ret == LB_ERR, "Unexpected return value %d.\n", ret);
        if (ret == LB_ERR)
        {
            ret = SendMessageA(listbox, LB_GETTEXT, invalid_idx[i], (LPARAM)&data);
            ok(ret == LB_ERR, "Unexpected return value %d.\n", ret);
        }
        ret = SendMessageA(listbox, LB_GETITEMDATA, invalid_idx[i], 0);
        ok(ret == LB_ERR, "Unexpected return value %d.\n", ret);
    }

    IsWow64Process(GetCurrentProcess(), &is_wow64);
#ifdef _WIN64
    text_len = 8;
#else
    text_len = is_wow64 ? 8 : 4;
#endif

    /* Valid indices. */
    for (i = 0; i < ARRAY_SIZE(valid_idx); ++i)
    {
        ret = SendMessageA(listbox, LB_SETITEMDATA, valid_idx[i], 42);
        ok(ret == TRUE, "Unexpected return value %d.\n", ret);
        ret = SendMessageA(listbox, LB_GETTEXTLEN, valid_idx[i], 0);
        todo_wine_if(is_wow64)
        ok(ret == text_len, "Unexpected return value %d.\n", ret);

        memset(&data, 0xee, sizeof(data));
        ret = SendMessageA(listbox, LB_GETTEXT, valid_idx[i], (LPARAM)&data);
        ok(ret == sizeof(data), "Unexpected return value %d.\n", ret);
        ok(!memcmp(&data, &zero_data, sizeof(data)), "Unexpected item data.\n");

        ret = SendMessageA(listbox, LB_GETITEMDATA, valid_idx[i], 0);
        ok(ret == 0, "Unexpected return value %d.\n", ret);
    }

    /* More messages that don't work with LBS_NODATA. */
    ret = SendMessageA(listbox, LB_FINDSTRING, 1, 0);
    ok(ret == LB_ERR, "Unexpected return value %d.\n", ret);
    ret = SendMessageA(listbox, LB_FINDSTRING, 1, 42);
    ok(ret == LB_ERR, "Unexpected return value %d.\n", ret);
    ret = SendMessageA(listbox, LB_FINDSTRINGEXACT, 1, 0);
    ok(ret == LB_ERR, "Unexpected return value %d.\n", ret);
    ret = SendMessageA(listbox, LB_FINDSTRINGEXACT, 1, 42);
    ok(ret == LB_ERR, "Unexpected return value %d.\n", ret);
    ret = SendMessageA(listbox, LB_SELECTSTRING, 1, 0);
    ok(ret == LB_ERR, "Unexpected return value %d.\n", ret);
    ret = SendMessageA(listbox, LB_SELECTSTRING, 1, 42);
    ok(ret == LB_ERR, "Unexpected return value %d.\n", ret);

    DestroyWindow(listbox);

    /* Invalid window style combinations. */
    parent = create_parent();
    ok(parent != NULL, "Failed to create parent window.\n");

    for (i = 0; i < ARRAY_SIZE(invalid_styles); ++i)
    {
        DWORD style;

        listbox = CreateWindowA(WC_LISTBOXA, "TestList", LBS_NODATA | WS_CHILD | invalid_styles[i],
                0, 0, 100, 100, parent, (HMENU)ID_LISTBOX, NULL, 0);
        ok(listbox != NULL, "Failed to create a listbox.\n");

        style = GetWindowLongA(listbox, GWL_STYLE);
        ok((style & invalid_styles[i]) == invalid_styles[i], "%u: unexpected window styles %#lx.\n", i, style);
        ret = SendMessageA(listbox, LB_SETCOUNT, 100, 0);
        ok(ret == LB_ERR, "%u: unexpected return value %d.\n", i, ret);
        DestroyWindow(listbox);
    }

    DestroyWindow(parent);
}

static void test_LB_FINDSTRING(void)
{
    static const WCHAR *strings[] =
    {
        L"abci",
        L"AbCI",
        L"abcI",
        L"abc\xcdzz",
        L"abc\xedzz",
        L"abc\xcd",
        L"abc\xed",
        L"abcO",
        L"abc\xd8",
        L"abcP",
    };
    static const struct { const WCHAR *str; LRESULT from, res, exact, alt_res, alt_exact; } tests[] =
    {
        { L"ab",        -1, 0, -1, 0, -1 },
        { L"abc",       -1, 0, -1, 0, -1 },
        { L"abci",      -1, 0, 0, 0, 0 },
        { L"ABCI",      -1, 0, 0, 0, 0 },
        { L"ABC\xed",   -1, 3, 3, 3, 3 },
        { L"ABC\xcd",    4, 5, 3, 5, 3 },
        { L"abcp",      -1, 9, 9, 8, 8 },
    };
    HWND listbox;
    unsigned int i;
    LRESULT ret;

    listbox = CreateWindowW( L"listbox", L"TestList", LBS_HASSTRINGS | LBS_SORT,
                             0, 0, 100, 100, NULL, NULL, NULL, 0 );
    ok( listbox != NULL, "Failed to create listbox\n" );
    SendMessageW( listbox, LB_SETLOCALE, MAKELANGID( LANG_FRENCH, SUBLANG_DEFAULT ), 0 );
    for (i = 0; i < ARRAY_SIZE(strings); i++) SendMessageW( listbox, LB_ADDSTRING, 0, (LPARAM)strings[i] );

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        ret = SendMessageW( listbox, LB_FINDSTRING, tests[i].from, (LPARAM)tests[i].str );
        ok( ret == tests[i].res, "%u: wrong result %Id / %Id\n", i, ret, tests[i].res );
        ret = SendMessageW( listbox, LB_FINDSTRINGEXACT, tests[i].from, (LPARAM)tests[i].str );
        ok( ret == tests[i].exact, "%u: wrong result %Id / %Id\n", i, ret, tests[i].exact );
    }

    SendMessageW( listbox, LB_RESETCONTENT, 0, 0 );
    SendMessageW( listbox, LB_SETLOCALE, MAKELANGID( LANG_SWEDISH, SUBLANG_DEFAULT ), 0 );
    for (i = 0; i < ARRAY_SIZE(strings); i++) SendMessageW( listbox, LB_ADDSTRING, 0, (LPARAM)strings[i] );
    ret = SendMessageW( listbox, LB_FINDSTRING, -1, (LPARAM)L"abcp" );
    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        ret = SendMessageW( listbox, LB_FINDSTRING, tests[i].from, (LPARAM)tests[i].str );
        ok( ret == tests[i].alt_res, "%u: wrong result %Id / %Id\n", i, ret, tests[i].alt_res );
        ret = SendMessageW( listbox, LB_FINDSTRINGEXACT, tests[i].from, (LPARAM)tests[i].str );
        ok( ret == tests[i].alt_exact, "%u: wrong result %Id / %Id\n", i, ret, tests[i].alt_exact );
    }

    SendMessageW( listbox, LB_RESETCONTENT, 0, 0 );
    SendMessageW( listbox, LB_ADDSTRING, 0, (LPARAM)L"abc" );
    SendMessageW( listbox, LB_ADDSTRING, 0, (LPARAM)L"[abc]" );
    SendMessageW( listbox, LB_ADDSTRING, 0, (LPARAM)L"[-abc-]" );
    ret = SendMessageW( listbox, LB_FINDSTRING, -1, (LPARAM)L"abc" );
    ok( ret == 0, "wrong result %Id\n", ret );
    ret = SendMessageW( listbox, LB_FINDSTRINGEXACT, -1, (LPARAM)L"abc" );
    todo_wine
    ok( ret == 0, "wrong result %Id\n", ret );
    ret = SendMessageW( listbox, LB_FINDSTRING, 0, (LPARAM)L"abc" );
    ok( ret == 1, "wrong result %Id\n", ret );
    ret = SendMessageW( listbox, LB_FINDSTRINGEXACT, 0, (LPARAM)L"abc" );
    todo_wine
    ok( ret == 0, "wrong result %Id\n", ret );
    ret = SendMessageW( listbox, LB_FINDSTRING, 1, (LPARAM)L"abc" );
    ok( ret == 2, "wrong result %Id\n", ret );
    ret = SendMessageW( listbox, LB_FINDSTRINGEXACT, 1, (LPARAM)L"abc" );
    todo_wine
    ok( ret == 0, "wrong result %Id\n", ret );
    DestroyWindow( listbox );
}

#define ok_selected_value(list, selected) \
        _ok_selected_value(list, selected, __LINE__)
static void _ok_selected_value(HWND list, const char *selected, int line)
{
    char buffer[20] = {0};
    int index = SendMessageA(list, LB_GETCURSEL, 0, 0);
    SendMessageA(list, LB_GETTEXT, index, (LPARAM)buffer);
    ok_(__FILE__, line)(!strcmp(buffer, selected), "Got %s\n", buffer);
}

static void test_keypresses(void)
{
    HWND list;
    int i;
    const char *strings_to_add[] = {
        "b_eta", "a_lpha", "be_ta", "al_pha", "beta", "alpha", "gamma", "epsilon", "le"
    };

    /* Test with an unsorted list */

    list = CreateWindowA(WC_LISTBOXA, "TestList", (LBS_STANDARD & ~LBS_SORT), 0, 0, 100, 100, NULL, NULL, NULL, 0);
    ok(list != NULL, "Failed to create listbox window.\n");

    for (i = 0; i < ARRAY_SIZE(strings_to_add); i++)
    {
        SendMessageA(list, LB_ADDSTRING, 0, (LPARAM)strings_to_add[i]);
    }

    SendMessageA(list, WM_CHAR, (WPARAM)'a', 0);
    ok_selected_value(list, "a_lpha");

    SendMessageA(list, WM_CHAR, (WPARAM)'l', 0);
    ok_selected_value(list, "le");

    SendMessageA(list, WM_CHAR, (WPARAM)'p', 0);
    ok_selected_value(list, "le");

    SendMessageA(list, WM_CHAR, (WPARAM)'b', 0);
    ok_selected_value(list, "b_eta");

    SendMessageA(list, WM_CHAR, (WPARAM)'e', 0);
    ok_selected_value(list, "epsilon");

    SendMessageA(list, WM_CHAR, (WPARAM)'t', 0);
    ok_selected_value(list, "epsilon");

    DestroyWindow(list);

    /* Test with a sorted list */

    list = CreateWindowA(WC_LISTBOXA, "TestList", LBS_STANDARD, 0, 0, 100, 100, NULL, NULL, NULL, 0);

    for (i = 0; i < ARRAY_SIZE(strings_to_add); i++)
    {
        SendMessageA(list, LB_ADDSTRING, 0, (LPARAM)strings_to_add[i]);
    }

    SendMessageA(list, WM_CHAR, (WPARAM)'a', 0);
    todo_wine
    ok_selected_value(list, "a_lpha");

    SendMessageA(list, WM_CHAR, (WPARAM)'l', 0);
    todo_wine
    ok_selected_value(list, "al_pha");

    SendMessageA(list, WM_CHAR, (WPARAM)'p', 0);
    todo_wine
    ok_selected_value(list, "alpha");

    /* Windows needs a certain time to pass until it starts a new search */

    SendMessageA(list, WM_CHAR, (WPARAM)'b', 0);
    todo_wine
    ok_selected_value(list, "alpha");

    Sleep(2100);
    flush_events();

    SendMessageA(list, WM_CHAR, (WPARAM)'b', 0);
    todo_wine
    ok_selected_value(list, "b_eta");

    DestroyWindow(list);
}

static void test_integral_resize(void)
{
    int scroll_height = GetSystemMetrics(SM_CYHSCROLL);
    int edge_width = GetSystemMetrics(SM_CXEDGE);
    int edge_height = GetSystemMetrics(SM_CYEDGE);
    HWND parent, listbox;
    RECT rect, expect;
    int ret;

    parent = create_parent();
    listbox = CreateWindowExA(WS_EX_CLIENTEDGE, WC_LISTBOXA, NULL,
            WS_CHILD | WS_HSCROLL, 0, 0, 199, 199, parent, NULL, NULL, NULL);
    ok(!!listbox, "got error %lu\n", GetLastError());

    ret = SendMessageA(listbox, LB_SETHORIZONTALEXTENT, 300, 0);
    ok(!ret, "got %d\n", ret);

    ret = SendMessageA(listbox, LB_SETITEMHEIGHT, 0, 40);
    ok(!ret, "got %d\n", ret);

    SetWindowPos(listbox, 0, 0, 0, 199, 199, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);

    /* The whole point of this functionality is to force an integral number of
     * items to be onscreen, but native fails to account for the scrollbar. */

    GetWindowRect(listbox, &rect);
    SetRect(&expect, 100, 100, 299, 260 + (edge_height * 2));
    ok(EqualRect(&rect, &expect), "expected %s, got %s\n", wine_dbgstr_rect(&expect), wine_dbgstr_rect(&rect));

    GetClientRect(listbox, &rect);
    SetRect(&expect, 0, 0, 199 - (edge_width * 2), 160 - scroll_height);
    ok(EqualRect(&rect, &expect), "expected %s, got %s\n", wine_dbgstr_rect(&expect), wine_dbgstr_rect(&rect));

    DestroyWindow(listbox);
    DestroyWindow(parent);
}

START_TEST(listbox)
{
    ULONG_PTR ctx_cookie;
    HANDLE hCtx;

    if (!load_v6_module(&ctx_cookie, &hCtx))
        return;

    init_msg_sequences(sequences, NUM_MSG_SEQUENCES);

    init_winevent_hook();

    test_listbox();
    test_item_height();
    test_ownerdraw();
    test_LB_SELITEMRANGE();
    test_LB_SETCURSEL();
    test_listbox_height();
    test_changing_selection_styles();
    test_itemfrompoint();
    test_listbox_item_data();
    test_listbox_LB_DIR();
    test_listbox_dlgdir();
    test_set_count();
    test_init_storage();
    test_GetListBoxInfo();
    test_missing_lbuttonup();
    test_extents();
    test_WM_MEASUREITEM();
    test_LB_SETSEL();
    test_LBS_NODATA();
    test_LB_FINDSTRING();
    test_keypresses();
    test_integral_resize();

    uninit_winevent_hook();

    unload_v6_module(ctx_cookie, hCtx);
}
