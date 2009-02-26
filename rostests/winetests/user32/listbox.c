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

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"

#include "wine/test.h"

#ifdef VISIBLE
#define WAIT Sleep (1000)
#define REDRAW RedrawWindow (handle, NULL, 0, RDW_UPDATENOW)
#else
#define WAIT
#define REDRAW
#endif

static const char * const strings[4] = {
  "First added",
  "Second added",
  "Third added",
  "Fourth added which is very long because at some time we only had a 256 byte character buffer and that was overflowing in one of those applications that had a common dialog file open box and tried to add a 300 characters long custom filter string which of course the code did not like and crashed. Just make sure this string is longer than 256 characters."
};

static const char BAD_EXTENSION[] = "*.badtxt";

static HWND
create_listbox (DWORD add_style, HWND parent)
{
  HWND handle;
  INT_PTR ctl_id=0;
  if (parent)
    ctl_id=1;
  handle=CreateWindow ("LISTBOX", "TestList",
                            (LBS_STANDARD & ~LBS_SORT) | add_style,
                            0, 0, 100, 100,
                            parent, (HMENU)ctl_id, NULL, 0);

  assert (handle);
  SendMessage (handle, LB_ADDSTRING, 0, (LPARAM) strings[0]);
  SendMessage (handle, LB_ADDSTRING, 0, (LPARAM) strings[1]);
  SendMessage (handle, LB_ADDSTRING, 0, (LPARAM) strings[2]);
  SendMessage (handle, LB_ADDSTRING, 0, (LPARAM) strings[3]);

#ifdef VISIBLE
  ShowWindow (handle, SW_SHOW);
#endif
  REDRAW;

  return handle;
}

struct listbox_prop {
  DWORD add_style;
};

struct listbox_stat {
  int selected, anchor, caret, selcount;
};

struct listbox_test {
  struct listbox_prop prop;
  struct listbox_stat  init,  init_todo;
  struct listbox_stat click, click_todo;
  struct listbox_stat  step,  step_todo;
  struct listbox_stat   sel,   sel_todo;
};

static void
listbox_query (HWND handle, struct listbox_stat *results)
{
  results->selected = SendMessage (handle, LB_GETCURSEL, 0, 0);
  results->anchor   = SendMessage (handle, LB_GETANCHORINDEX, 0, 0);
  results->caret    = SendMessage (handle, LB_GETCARETINDEX, 0, 0);
  results->selcount = SendMessage (handle, LB_GETSELCOUNT, 0, 0);
}

static void
buttonpress (HWND handle, WORD x, WORD y)
{
  LPARAM lp=x+(y<<16);

  WAIT;
  SendMessage (handle, WM_LBUTTONDOWN, MK_LBUTTON, lp);
  SendMessage (handle, WM_LBUTTONUP, 0, lp);
  REDRAW;
}

static void
keypress (HWND handle, WPARAM keycode, BYTE scancode, BOOL extended)
{
  LPARAM lp=1+(scancode<<16)+(extended?KEYEVENTF_EXTENDEDKEY:0);

  WAIT;
  SendMessage (handle, WM_KEYDOWN, keycode, lp);
  SendMessage (handle, WM_KEYUP  , keycode, lp | 0xc000000);
  REDRAW;
}

#define listbox_field_ok(t, s, f, got) \
  ok (t.s.f==got.f, "style %#x, step " #s ", field " #f \
      ": expected %d, got %d\n", (unsigned int)t.prop.add_style, \
      t.s.f, got.f)

#define listbox_todo_field_ok(t, s, f, got) \
  if (t.s##_todo.f) todo_wine { listbox_field_ok(t, s, f, got); } \
  else listbox_field_ok(t, s, f, got)

#define listbox_ok(t, s, got) \
  listbox_todo_field_ok(t, s, selected, got); \
  listbox_todo_field_ok(t, s, anchor, got); \
  listbox_todo_field_ok(t, s, caret, got); \
  listbox_todo_field_ok(t, s, selcount, got)

static void
check (const struct listbox_test test)
{
  struct listbox_stat answer;
  HWND hLB=create_listbox (test.prop.add_style, 0);
  RECT second_item;
  int i;
  int res;

  listbox_query (hLB, &answer);
  listbox_ok (test, init, answer);

  SendMessage (hLB, LB_GETITEMRECT, (WPARAM) 1, (LPARAM) &second_item);
  buttonpress(hLB, (WORD)second_item.left, (WORD)second_item.top);

  listbox_query (hLB, &answer);
  listbox_ok (test, click, answer);

  keypress (hLB, VK_DOWN, 0x50, TRUE);

  listbox_query (hLB, &answer);
  listbox_ok (test, step, answer);

  DestroyWindow (hLB);
  hLB=create_listbox (test.prop.add_style, 0);

  SendMessage (hLB, LB_SELITEMRANGE, TRUE, MAKELPARAM(1, 2));
  listbox_query (hLB, &answer);
  listbox_ok (test, sel, answer);

  for (i=0;i<4;i++) {
	DWORD size = SendMessage (hLB, LB_GETTEXTLEN, i, 0);
	CHAR *txt;
	WCHAR *txtw;
	int resA, resW;

	txt = HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, size+1);
	resA=SendMessageA(hLB, LB_GETTEXT, i, (LPARAM)txt);
        ok(!strcmp (txt, strings[i]), "returned string for item %d does not match %s vs %s\n", i, txt, strings[i]);

	txtw = HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, 2*size+2);
	resW=SendMessageW(hLB, LB_GETTEXT, i, (LPARAM)txtw);
	if (resA != resW) {
            trace("SendMessageW(LB_GETTEXT) not supported on this platform (resA=%d resW=%d), skipping...\n",
                resA, resW);
	} else {
	    WideCharToMultiByte( CP_ACP, 0, txtw, -1, txt, size, NULL, NULL );
            ok(!strcmp (txt, strings[i]), "returned string for item %d does not match %s vs %s\n", i, txt, strings[i]);
	}

	HeapFree (GetProcessHeap(), 0, txtw);
	HeapFree (GetProcessHeap(), 0, txt);
  }
  
  /* Confirm the count of items, and that an invalid delete does not remove anything */
  res = SendMessage (hLB, LB_GETCOUNT, 0, 0);
  ok((res==4), "Expected 4 items, got %d\n", res);
  res = SendMessage (hLB, LB_DELETESTRING, -1, 0);
  ok((res==LB_ERR), "Expected LB_ERR items, got %d\n", res);
  res = SendMessage (hLB, LB_DELETESTRING, 4, 0);
  ok((res==LB_ERR), "Expected LB_ERR items, got %d\n", res);
  res = SendMessage (hLB, LB_GETCOUNT, 0, 0);
  ok((res==4), "Expected 4 items, got %d\n", res);

  WAIT;
  DestroyWindow (hLB);
}

static void check_item_height(void)
{
    HWND hLB;
    HDC hdc;
    HFONT font;
    TEXTMETRIC tm;
    INT itemHeight;

    hLB = create_listbox (0, 0);
    ok ((hdc = GetDCEx( hLB, 0, DCX_CACHE )) != 0, "Can't get hdc\n");
    ok ((font = GetCurrentObject(hdc, OBJ_FONT)) != 0, "Can't get the current font\n");
    ok (GetTextMetrics( hdc, &tm ), "Can't read font metrics\n");
    ReleaseDC( hLB, hdc);

    ok (SendMessage(hLB, WM_SETFONT, (WPARAM)font, 0) == 0, "Can't set font\n");

    itemHeight = SendMessage(hLB, LB_GETITEMHEIGHT, 0, 0);
    ok (itemHeight == tm.tmHeight, "Item height wrong, got %d, expecting %d\n", itemHeight, tm.tmHeight);

    DestroyWindow (hLB);

    hLB = CreateWindow ("LISTBOX", "TestList", LBS_OWNERDRAWVARIABLE,
                         0, 0, 100, 100, NULL, NULL, NULL, 0);
    itemHeight = SendMessage(hLB, LB_GETITEMHEIGHT, 0, 0);
    ok(itemHeight == tm.tmHeight, "itemHeight %d\n", itemHeight);
    itemHeight = SendMessage(hLB, LB_GETITEMHEIGHT, 5, 0);
    ok(itemHeight == tm.tmHeight, "itemHeight %d\n", itemHeight);
    itemHeight = SendMessage(hLB, LB_GETITEMHEIGHT, -5, 0);
    ok(itemHeight == tm.tmHeight, "itemHeight %d\n", itemHeight);
    DestroyWindow (hLB);
}

static LRESULT WINAPI main_window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_DRAWITEM:
    {
        RECT rc_item, rc_client, rc_clip;
        DRAWITEMSTRUCT *dis = (DRAWITEMSTRUCT *)lparam;

        trace("%p WM_DRAWITEM %08lx %08lx\n", hwnd, wparam, lparam);

        ok(wparam == dis->CtlID, "got wParam=%08lx instead of %08x\n",
			wparam, dis->CtlID);
        ok(dis->CtlType == ODT_LISTBOX, "wrong CtlType %04x\n", dis->CtlType);

        GetClientRect(dis->hwndItem, &rc_client);
        trace("hwndItem %p client rect (%d,%d-%d,%d)\n", dis->hwndItem,
               rc_client.left, rc_client.top, rc_client.right, rc_client.bottom);
        GetClipBox(dis->hDC, &rc_clip);
        trace("clip rect (%d,%d-%d,%d)\n", rc_clip.left, rc_clip.top, rc_clip.right, rc_clip.bottom);
        ok(EqualRect(&rc_client, &rc_clip) || IsRectEmpty(&rc_clip),
           "client rect of the listbox should be equal to the clip box,"
           "or the clip box should be empty\n");

        trace("rcItem (%d,%d-%d,%d)\n", dis->rcItem.left, dis->rcItem.top,
               dis->rcItem.right, dis->rcItem.bottom);
        SendMessage(dis->hwndItem, LB_GETITEMRECT, dis->itemID, (LPARAM)&rc_item);
        trace("item rect (%d,%d-%d,%d)\n", rc_item.left, rc_item.top, rc_item.right, rc_item.bottom);
        ok(EqualRect(&dis->rcItem, &rc_item), "item rects are not equal\n");

        break;
    }

    default:
        break;
    }

    return DefWindowProc(hwnd, msg, wparam, lparam);
}

static void test_ownerdraw(void)
{
    WNDCLASS cls;
    HWND parent, hLB;
    INT ret;
    RECT rc;

    cls.style = 0;
    cls.lpfnWndProc = main_window_proc;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandle(0);
    cls.hIcon = 0;
    cls.hCursor = LoadCursor(0, IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "main_window_class";
    assert(RegisterClass(&cls));

    parent = CreateWindowEx(0, "main_window_class", NULL,
                            WS_POPUP | WS_VISIBLE,
                            100, 100, 400, 400,
                            GetDesktopWindow(), 0,
                            GetModuleHandle(0), NULL);
    assert(parent);

    hLB = create_listbox(LBS_OWNERDRAWFIXED | WS_CHILD | WS_VISIBLE, parent);
    assert(hLB);

    UpdateWindow(hLB);

    /* make height short enough */
    SendMessage(hLB, LB_GETITEMRECT, 0, (LPARAM)&rc);
    SetWindowPos(hLB, 0, 0, 0, 100, rc.bottom - rc.top + 1,
                 SWP_NOZORDER | SWP_NOMOVE);

    /* make 0 item invisible */
    SendMessage(hLB, LB_SETTOPINDEX, 1, 0);
    ret = SendMessage(hLB, LB_GETTOPINDEX, 0, 0);
    ok(ret == 1, "wrong top index %d\n", ret);

    SendMessage(hLB, LB_GETITEMRECT, 0, (LPARAM)&rc);
    trace("item 0 rect (%d,%d-%d,%d)\n", rc.left, rc.top, rc.right, rc.bottom);
    ok(!IsRectEmpty(&rc), "empty item rect\n");
    ok(rc.top < 0, "rc.top is not negative (%d)\n", rc.top);

    DestroyWindow(hLB);
    DestroyWindow(parent);
}

#define listbox_test_query(exp, got) \
  ok(exp.selected == got.selected, "expected selected %d, got %d\n", exp.selected, got.selected); \
  ok(exp.anchor == got.anchor, "expected anchor %d, got %d\n", exp.anchor, got.anchor); \
  ok(exp.caret == got.caret, "expected caret %d, got %d\n", exp.caret, got.caret); \
  ok(exp.selcount == got.selcount, "expected selcount %d, got %d\n", exp.selcount, got.selcount);

static void test_selection(void)
{
    static const struct listbox_stat test_nosel = { 0, LB_ERR, 0, 0 };
    static const struct listbox_stat test_1 = { 0, LB_ERR, 0, 2 };
    static const struct listbox_stat test_2 = { 0, LB_ERR, 0, 3 };
    static const struct listbox_stat test_3 = { 0, LB_ERR, 0, 4 };
    HWND hLB;
    struct listbox_stat answer;
    INT ret;

    trace("testing LB_SELITEMRANGE\n");

    hLB = create_listbox(LBS_EXTENDEDSEL, 0);
    assert(hLB);

    listbox_query(hLB, &answer);
    listbox_test_query(test_nosel, answer);

    ret = SendMessage(hLB, LB_SELITEMRANGE, TRUE, MAKELPARAM(1, 2));
    ok(ret == LB_OKAY, "LB_SELITEMRANGE returned %d instead of LB_OKAY\n", ret);
    listbox_query(hLB, &answer);
    listbox_test_query(test_1, answer);

    SendMessage(hLB, LB_SETSEL, FALSE, (LPARAM)-1);
    listbox_query(hLB, &answer);
    listbox_test_query(test_nosel, answer);

    ret = SendMessage(hLB, LB_SELITEMRANGE, TRUE, MAKELPARAM(0, 4));
    ok(ret == LB_OKAY, "LB_SELITEMRANGE returned %d instead of LB_OKAY\n", ret);
    listbox_query(hLB, &answer);
    listbox_test_query(test_3, answer);

    SendMessage(hLB, LB_SETSEL, FALSE, (LPARAM)-1);
    listbox_query(hLB, &answer);
    listbox_test_query(test_nosel, answer);

    ret = SendMessage(hLB, LB_SELITEMRANGE, TRUE, MAKELPARAM(-5, 5));
    ok(ret == LB_OKAY, "LB_SELITEMRANGE returned %d instead of LB_OKAY\n", ret);
    listbox_query(hLB, &answer);
    listbox_test_query(test_nosel, answer);

    SendMessage(hLB, LB_SETSEL, FALSE, (LPARAM)-1);
    listbox_query(hLB, &answer);
    listbox_test_query(test_nosel, answer);

    ret = SendMessage(hLB, LB_SELITEMRANGE, TRUE, MAKELPARAM(2, 10));
    ok(ret == LB_OKAY, "LB_SELITEMRANGE returned %d instead of LB_OKAY\n", ret);
    listbox_query(hLB, &answer);
    listbox_test_query(test_1, answer);

    SendMessage(hLB, LB_SETSEL, FALSE, (LPARAM)-1);
    listbox_query(hLB, &answer);
    listbox_test_query(test_nosel, answer);

    ret = SendMessage(hLB, LB_SELITEMRANGE, TRUE, MAKELPARAM(4, 10));
    ok(ret == LB_OKAY, "LB_SELITEMRANGE returned %d instead of LB_OKAY\n", ret);
    listbox_query(hLB, &answer);
    listbox_test_query(test_nosel, answer);

    SendMessage(hLB, LB_SETSEL, FALSE, (LPARAM)-1);
    listbox_query(hLB, &answer);
    listbox_test_query(test_nosel, answer);

    ret = SendMessage(hLB, LB_SELITEMRANGE, TRUE, MAKELPARAM(10, 1));
    ok(ret == LB_OKAY, "LB_SELITEMRANGE returned %d instead of LB_OKAY\n", ret);
    listbox_query(hLB, &answer);
    listbox_test_query(test_2, answer);

    SendMessage(hLB, LB_SETSEL, FALSE, (LPARAM)-1);
    listbox_query(hLB, &answer);
    listbox_test_query(test_nosel, answer);

    ret = SendMessage(hLB, LB_SELITEMRANGE, TRUE, MAKELPARAM(1, -1));
    ok(ret == LB_OKAY, "LB_SELITEMRANGE returned %d instead of LB_OKAY\n", ret);
    listbox_query(hLB, &answer);
    listbox_test_query(test_2, answer);

    DestroyWindow(hLB);
}

static void test_listbox_height(void)
{
    HWND hList;
    int r, id;

    hList = CreateWindow( "ListBox", "list test", 0, 
                          1, 1, 600, 100, NULL, NULL, NULL, NULL );
    ok( hList != NULL, "failed to create listbox\n");

    id = SendMessage( hList, LB_ADDSTRING, 0, (LPARAM) "hi");
    ok( id == 0, "item id wrong\n");

    r = SendMessage( hList, LB_SETITEMHEIGHT, 0, MAKELPARAM( 20, 0 ));
    ok( r == 0, "send message failed\n");

    r = SendMessage(hList, LB_GETITEMHEIGHT, 0, 0 );
    ok( r == 20, "height wrong\n");

    r = SendMessage( hList, LB_SETITEMHEIGHT, 0, MAKELPARAM( 0, 30 ));
    ok( r == -1, "send message failed\n");

    r = SendMessage(hList, LB_GETITEMHEIGHT, 0, 0 );
    ok( r == 20, "height wrong\n");

    r = SendMessage( hList, LB_SETITEMHEIGHT, 0, MAKELPARAM( 0x100, 0 ));
    ok( r == -1, "send message failed\n");

    r = SendMessage(hList, LB_GETITEMHEIGHT, 0, 0 );
    ok( r == 20, "height wrong\n");

    r = SendMessage( hList, LB_SETITEMHEIGHT, 0, MAKELPARAM( 0xff, 0 ));
    ok( r == 0, "send message failed\n");

    r = SendMessage(hList, LB_GETITEMHEIGHT, 0, 0 );
    ok( r == 0xff, "height wrong\n");

    DestroyWindow( hList );
}

static void test_itemfrompoint(void)
{
    /* WS_POPUP is required in order to have a more accurate size calculation (
       without caption). LBS_NOINTEGRALHEIGHT is required in order to test
       behavior of partially-displayed item.
     */
    HWND hList = CreateWindow( "ListBox", "list test",
                               WS_VISIBLE|WS_POPUP|LBS_NOINTEGRALHEIGHT,
                               1, 1, 600, 100, NULL, NULL, NULL, NULL );
    ULONG r, id;
    RECT rc;

    /* For an empty listbox win2k returns 0x1ffff, win98 returns 0x10000, nt4 returns 0xffffffff */
    r = SendMessage(hList, LB_ITEMFROMPOINT, 0, MAKELPARAM( /* x */ 30, /* y */ 30 ));
    ok( r == 0x1ffff || r == 0x10000 || r == 0xffffffff, "ret %x\n", r );

    r = SendMessage(hList, LB_ITEMFROMPOINT, 0, MAKELPARAM( 700, 30 ));
    ok( r == 0x1ffff || r == 0x10000 || r == 0xffffffff, "ret %x\n", r );

    r = SendMessage(hList, LB_ITEMFROMPOINT, 0, MAKELPARAM( 30, 300 ));
    ok( r == 0x1ffff || r == 0x10000 || r == 0xffffffff, "ret %x\n", r );

    id = SendMessage( hList, LB_ADDSTRING, 0, (LPARAM) "hi");
    ok( id == 0, "item id wrong\n");
    id = SendMessage( hList, LB_ADDSTRING, 0, (LPARAM) "hi1");
    ok( id == 1, "item id wrong\n");

    r = SendMessage(hList, LB_ITEMFROMPOINT, 0, MAKELPARAM( /* x */ 30, /* y */ 30 ));
    ok( r == 0x1, "ret %x\n", r );

    r = SendMessage(hList, LB_ITEMFROMPOINT, 0, MAKELPARAM( /* x */ 30, /* y */ 601 ));
    ok( r == 0x10001 || broken(r == 1), /* nt4 */
        "ret %x\n", r );

    /* Resize control so that below assertions about sizes are valid */
    r = SendMessage( hList, LB_GETITEMRECT, 0, (LPARAM)&rc);
    ok( r == 1, "ret %x\n", r);
    r = MoveWindow(hList, 1, 1, 600, (rc.bottom - rc.top + 1) * 9 / 2, TRUE);
    ok( r != 0, "ret %x\n", r);

    id = SendMessage( hList, LB_ADDSTRING, 0, (LPARAM) "hi2");
    ok( id == 2, "item id wrong\n");
    id = SendMessage( hList, LB_ADDSTRING, 0, (LPARAM) "hi3");
    ok( id == 3, "item id wrong\n");
    id = SendMessage( hList, LB_ADDSTRING, 0, (LPARAM) "hi4");
    ok( id == 4, "item id wrong\n");
    id = SendMessage( hList, LB_ADDSTRING, 0, (LPARAM) "hi5");
    ok( id == 5, "item id wrong\n");
    id = SendMessage( hList, LB_ADDSTRING, 0, (LPARAM) "hi6");
    ok( id == 6, "item id wrong\n");
    id = SendMessage( hList, LB_ADDSTRING, 0, (LPARAM) "hi7");
    ok( id == 7, "item id wrong\n");

    /* Set the listbox up so that id 1 is at the top, this leaves 5
       partially visible at the bottom and 6, 7 are invisible */

    SendMessage( hList, LB_SETTOPINDEX, 1, 0);
    r = SendMessage( hList, LB_GETTOPINDEX, 0, 0);
    ok( r == 1, "top %d\n", r);

    r = SendMessage( hList, LB_GETITEMRECT, 5, (LPARAM)&rc);
    ok( r == 1, "ret %x\n", r);
    r = SendMessage( hList, LB_GETITEMRECT, 6, (LPARAM)&rc);
    ok( r == 0, "ret %x\n", r);

    r = SendMessage( hList, LB_ITEMFROMPOINT, 0, MAKELPARAM(/* x */ 10, /* y */ 10) );
    ok( r == 1, "ret %x\n", r);

    r = SendMessage( hList, LB_ITEMFROMPOINT, 0, MAKELPARAM(1000, 10) );
    ok( r == 0x10001 || broken(r == 1), /* nt4 */
        "ret %x\n", r );

    r = SendMessage( hList, LB_ITEMFROMPOINT, 0, MAKELPARAM(10, -10) );
    ok( r == 0x10001 || broken(r == 1), /* nt4 */
        "ret %x\n", r );

    r = SendMessage( hList, LB_ITEMFROMPOINT, 0, MAKELPARAM(10, 100) );
    ok( r == 0x10005 || broken(r == 5), /* nt4 */
        "item %x\n", r );

    r = SendMessage( hList, LB_ITEMFROMPOINT, 0, MAKELPARAM(10, 200) );
    ok( r == 0x10005 || broken(r == 5), /* nt4 */
        "item %x\n", r );

    DestroyWindow( hList );
}

static void test_listbox_item_data(void)
{
    HWND hList;
    int r, id;

    hList = CreateWindow( "ListBox", "list test", 0,
                          1, 1, 600, 100, NULL, NULL, NULL, NULL );
    ok( hList != NULL, "failed to create listbox\n");

    id = SendMessage( hList, LB_ADDSTRING, 0, (LPARAM) "hi");
    ok( id == 0, "item id wrong\n");

    r = SendMessage( hList, LB_SETITEMDATA, 0, MAKELPARAM( 20, 0 ));
    ok(r == TRUE, "LB_SETITEMDATA returned %d instead of TRUE\n", r);

    r = SendMessage( hList, LB_GETITEMDATA, 0, 0);
    ok( r == 20, "get item data failed\n");

    DestroyWindow( hList );
}

static void test_listbox_LB_DIR(void)
{
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

    file = CreateFileA( "wtest1.tmp.c", GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL );
    ok(file != INVALID_HANDLE_VALUE, "Error creating the test file: %d\n", GetLastError());
    CloseHandle( file );

    /* NOTE: for this test to succeed, there must be no subdirectories
       under the current directory. In addition, there must be at least
       one file that fits the wildcard w*.c . Normally, the test
       directory itself satisfies both conditions.
     */
    hList = CreateWindow( "ListBox", "list test", WS_VISIBLE|WS_POPUP,
                          1, 1, 600, 100, NULL, NULL, NULL, NULL );
    assert(hList);

    /* Test for standard usage */

    /* This should list all the files in the test directory. */
    strcpy(pathBuffer, wildcard);
    SendMessage(hList, LB_RESETCONTENT, 0, 0);
    res = SendMessage(hList, LB_DIR, 0, (LPARAM)pathBuffer);
    if (res == -1)  /* "*" wildcard doesn't work on win9x */
    {
        wildcard = "*.*";
        strcpy(pathBuffer, wildcard);
        res = SendMessage(hList, LB_DIR, 0, (LPARAM)pathBuffer);
    }
    ok (res >= 0, "SendMessage(LB_DIR, 0, *) failed - 0x%08x\n", GetLastError());

    /* There should be some content in the listbox */
    itemCount = SendMessage(hList, LB_GETCOUNT, 0, 0);
    ok (itemCount > 0, "SendMessage(LB_DIR) did NOT fill the listbox!\n");
    itemCount_allFiles = itemCount;
    ok(res + 1 == itemCount,
        "SendMessage(LB_DIR, 0, *) returned incorrect index (expected %d got %d)!\n",
        itemCount - 1, res);

    /* This tests behavior when no files match the wildcard */
    strcpy(pathBuffer, BAD_EXTENSION);
    SendMessage(hList, LB_RESETCONTENT, 0, 0);
    res = SendMessage(hList, LB_DIR, 0, (LPARAM)pathBuffer);
    ok (res == -1, "SendMessage(LB_DIR, 0, %s) returned %d, expected -1\n", BAD_EXTENSION, res);

    /* There should be NO content in the listbox */
    itemCount = SendMessage(hList, LB_GETCOUNT, 0, 0);
    ok (itemCount == 0, "SendMessage(LB_DIR) DID fill the listbox!\n");


    /* This should list all the w*.c files in the test directory
     * As of this writing, this includes win.c, winstation.c, wsprintf.c
     */
    strcpy(pathBuffer, "w*.c");
    SendMessage(hList, LB_RESETCONTENT, 0, 0);
    res = SendMessage(hList, LB_DIR, 0, (LPARAM)pathBuffer);
    ok (res >= 0, "SendMessage(LB_DIR, 0, w*.c) failed - 0x%08x\n", GetLastError());

    /* Path specification does NOT converted to uppercase */
    ok (!strcmp(pathBuffer, "w*.c"),
        "expected no change to pathBuffer, got %s\n", pathBuffer);

    /* There should be some content in the listbox */
    itemCount = SendMessage(hList, LB_GETCOUNT, 0, 0);
    ok (itemCount > 0, "SendMessage(LB_DIR) did NOT fill the listbox!\n");
    itemCount_justFiles = itemCount;
    ok(res + 1 == itemCount,
        "SendMessage(LB_DIR, 0, w*.c) returned incorrect index (expected %d got %d)!\n",
        itemCount - 1, res);

    /* Every single item in the control should start with a w and end in .c */
    for (i = 0; i < itemCount; i++) {
        memset(pathBuffer, 0, MAX_PATH);
        SendMessage(hList, LB_GETTEXT, i, (LPARAM)pathBuffer);
        p = pathBuffer + strlen(pathBuffer);
        ok(((pathBuffer[0] == 'w' || pathBuffer[0] == 'W') &&
            (*(p-1) == 'c' || *(p-1) == 'C') &&
            (*(p-2) == '.')), "Element %d (%s) does not fit requested w*.c\n", i, pathBuffer);
    }

    /* Test DDL_DIRECTORY */
    strcpy(pathBuffer, wildcard);
    SendMessage(hList, LB_RESETCONTENT, 0, 0);
    res = SendMessage(hList, LB_DIR, DDL_DIRECTORY, (LPARAM)pathBuffer);
    ok (res > 0, "SendMessage(LB_DIR, DDL_DIRECTORY, *) failed - 0x%08x\n", GetLastError());

    /* There should be some content in the listbox.
     * All files plus "[..]"
     */
    itemCount = SendMessage(hList, LB_GETCOUNT, 0, 0);
    itemCount_allDirs = itemCount - itemCount_allFiles;
    ok (itemCount > itemCount_allFiles,
        "SendMessage(LB_DIR, DDL_DIRECTORY, *) filled with %d entries, expected > %d\n",
        itemCount, itemCount_allFiles);
    ok(res + 1 == itemCount,
        "SendMessage(LB_DIR, DDL_DIRECTORY, *) returned incorrect index (expected %d got %d)!\n",
        itemCount - 1, res);

    /* This tests behavior when no files match the wildcard */
    strcpy(pathBuffer, BAD_EXTENSION);
    SendMessage(hList, LB_RESETCONTENT, 0, 0);
    res = SendMessage(hList, LB_DIR, DDL_DIRECTORY, (LPARAM)pathBuffer);
    ok (res == -1, "SendMessage(LB_DIR, DDL_DIRECTORY, %s) returned %d, expected -1\n", BAD_EXTENSION, res);

    /* There should be NO content in the listbox */
    itemCount = SendMessage(hList, LB_GETCOUNT, 0, 0);
    ok (itemCount == 0, "SendMessage(LB_DIR) DID fill the listbox!\n");


    /* Test DDL_DIRECTORY */
    strcpy(pathBuffer, "w*.c");
    SendMessage(hList, LB_RESETCONTENT, 0, 0);
    res = SendMessage(hList, LB_DIR, DDL_DIRECTORY, (LPARAM)pathBuffer);
    ok (res >= 0, "SendMessage(LB_DIR, DDL_DIRECTORY, w*.c) failed - 0x%08x\n", GetLastError());

    /* There should be some content in the listbox. Since the parent directory does not
     * fit w*.c, there should be exactly the same number of items as without DDL_DIRECTORY
     */
    itemCount = SendMessage(hList, LB_GETCOUNT, 0, 0);
    ok (itemCount == itemCount_justFiles,
        "SendMessage(LB_DIR, DDL_DIRECTORY, w*.c) filled with %d entries, expected %d\n",
        itemCount, itemCount_justFiles);
    ok(res + 1 == itemCount,
        "SendMessage(LB_DIR, DDL_DIRECTORY, w*.c) returned incorrect index (expected %d got %d)!\n",
        itemCount - 1, res);

    /* Every single item in the control should start with a w and end in .c. */
    for (i = 0; i < itemCount; i++) {
        memset(pathBuffer, 0, MAX_PATH);
        SendMessage(hList, LB_GETTEXT, i, (LPARAM)pathBuffer);
        p = pathBuffer + strlen(pathBuffer);
        ok(
            ((pathBuffer[0] == 'w' || pathBuffer[0] == 'W') &&
            (*(p-1) == 'c' || *(p-1) == 'C') &&
            (*(p-2) == '.')), "Element %d (%s) does not fit requested w*.c\n", i, pathBuffer);
    }


    /* Test DDL_DRIVES|DDL_EXCLUSIVE */
    strcpy(pathBuffer, wildcard);
    SendMessage(hList, LB_RESETCONTENT, 0, 0);
    res = SendMessage(hList, LB_DIR, DDL_DRIVES|DDL_EXCLUSIVE, (LPARAM)pathBuffer);
    ok (res >= 0, "SendMessage(LB_DIR, DDL_DRIVES|DDL_EXCLUSIVE, *)  failed - 0x%08x\n", GetLastError());

    /* There should be some content in the listbox. In particular, there should
     * be at least one element before, since the string "[-c-]" should
     * have been added. Depending on the user setting, more drives might have
     * been added.
     */
    itemCount = SendMessage(hList, LB_GETCOUNT, 0, 0);
    ok (itemCount >= 1,
        "SendMessage(LB_DIR, DDL_DRIVES|DDL_EXCLUSIVE, *) filled with %d entries, expected at least %d\n",
        itemCount, 1);
    itemCount_justDrives = itemCount;
    ok(res + 1 == itemCount, "SendMessage(LB_DIR, DDL_DRIVES|DDL_EXCLUSIVE, *) returned incorrect index!\n");

    /* Every single item in the control should fit the format [-c-] */
    for (i = 0; i < itemCount; i++) {
        memset(pathBuffer, 0, MAX_PATH);
        driveletter = '\0';
        SendMessage(hList, LB_GETTEXT, i, (LPARAM)pathBuffer);
        ok( strlen(pathBuffer) == 5, "Length of drive string is not 5\n" );
        ok( sscanf(pathBuffer, "[-%c-]", &driveletter) == 1, "Element %d (%s) does not fit [-X-]\n", i, pathBuffer);
        ok( driveletter >= 'a' && driveletter <= 'z', "Drive letter not in range a..z, got ascii %d\n", driveletter);
        if (!(driveletter >= 'a' && driveletter <= 'z')) {
            /* Correct after invalid entry is found */
            trace("removing count of invalid entry %s\n", pathBuffer);
            itemCount_justDrives--;
        }
    }

    /* This tests behavior when no files match the wildcard */
    strcpy(pathBuffer, BAD_EXTENSION);
    SendMessage(hList, LB_RESETCONTENT, 0, 0);
    res = SendMessage(hList, LB_DIR, DDL_DRIVES|DDL_EXCLUSIVE, (LPARAM)pathBuffer);
    ok (res == itemCount_justDrives -1, "SendMessage(LB_DIR, DDL_DRIVES|DDL_EXCLUSIVE, %s) returned %d, expected %d\n",
        BAD_EXTENSION, res, itemCount_justDrives -1);

    itemCount = SendMessage(hList, LB_GETCOUNT, 0, 0);
    ok (itemCount == itemCount_justDrives, "SendMessage(LB_DIR) returned %d expected %d\n",
        itemCount, itemCount_justDrives);

    trace("Files with w*.c: %d Mapped drives: %d Directories: 1\n",
        itemCount_justFiles, itemCount_justDrives);

    /* Test DDL_DRIVES. */
    strcpy(pathBuffer, wildcard);
    SendMessage(hList, LB_RESETCONTENT, 0, 0);
    res = SendMessage(hList, LB_DIR, DDL_DRIVES, (LPARAM)pathBuffer);
    ok (res > 0, "SendMessage(LB_DIR, DDL_DRIVES, *)  failed - 0x%08x\n", GetLastError());

    /* There should be some content in the listbox. In particular, there should
     * be at least one element before, since the string "[-c-]" should
     * have been added. Depending on the user setting, more drives might have
     * been added.
     */
    itemCount = SendMessage(hList, LB_GETCOUNT, 0, 0);
    ok (itemCount == itemCount_justDrives + itemCount_allFiles,
        "SendMessage(LB_DIR, DDL_DRIVES, *) filled with %d entries, expected %d\n",
        itemCount, itemCount_justDrives + itemCount_allFiles);
    ok(res + 1 == itemCount, "SendMessage(LB_DIR, DDL_DRIVES, *) returned incorrect index!\n");

    /* This tests behavior when no files match the wildcard */
    strcpy(pathBuffer, BAD_EXTENSION);
    SendMessage(hList, LB_RESETCONTENT, 0, 0);
    res = SendMessage(hList, LB_DIR, DDL_DRIVES, (LPARAM)pathBuffer);
    ok (res == itemCount_justDrives -1, "SendMessage(LB_DIR, DDL_DRIVES, %s) returned %d, expected %d\n",
        BAD_EXTENSION, res, itemCount_justDrives -1);

    itemCount = SendMessage(hList, LB_GETCOUNT, 0, 0);
    ok (itemCount == res + 1, "SendMessage(LB_DIR) returned %d expected %d\n", itemCount, res + 1);


    /* Test DDL_DRIVES. */
    strcpy(pathBuffer, "w*.c");
    SendMessage(hList, LB_RESETCONTENT, 0, 0);
    res = SendMessage(hList, LB_DIR, DDL_DRIVES, (LPARAM)pathBuffer);
    ok (res > 0, "SendMessage(LB_DIR, DDL_DRIVES, w*.c)  failed - 0x%08x\n", GetLastError());

    /* There should be some content in the listbox. In particular, there should
     * be at least one element before, since the string "[-c-]" should
     * have been added. Depending on the user setting, more drives might have
     * been added.
     */
    itemCount = SendMessage(hList, LB_GETCOUNT, 0, 0);
    ok (itemCount == itemCount_justDrives + itemCount_justFiles,
        "SendMessage(LB_DIR, DDL_DRIVES, w*.c) filled with %d entries, expected %d\n",
        itemCount, itemCount_justDrives + itemCount_justFiles);
    ok(res + 1 == itemCount, "SendMessage(LB_DIR, DDL_DRIVES, w*.c) returned incorrect index!\n");

    /* Every single item in the control should fit the format [-c-], or w*.c */
    for (i = 0; i < itemCount; i++) {
        memset(pathBuffer, 0, MAX_PATH);
        driveletter = '\0';
        SendMessage(hList, LB_GETTEXT, i, (LPARAM)pathBuffer);
        p = pathBuffer + strlen(pathBuffer);
        if (sscanf(pathBuffer, "[-%c-]", &driveletter) == 1) {
            ok( strlen(pathBuffer) == 5, "Length of drive string is not 5\n" );
            ok( driveletter >= 'a' && driveletter <= 'z', "Drive letter not in range a..z, got ascii %d\n", driveletter);
        } else {
            ok(
                ((pathBuffer[0] == 'w' || pathBuffer[0] == 'W') &&
                (*(p-1) == 'c' || *(p-1) == 'C') &&
                (*(p-2) == '.')), "Element %d (%s) does not fit requested w*.c\n", i, pathBuffer);
        }
    }


    /* Test DDL_DIRECTORY|DDL_DRIVES. This does *not* imply DDL_EXCLUSIVE */
    strcpy(pathBuffer, wildcard);
    SendMessage(hList, LB_RESETCONTENT, 0, 0);
    res = SendMessage(hList, LB_DIR, DDL_DIRECTORY|DDL_DRIVES, (LPARAM)pathBuffer);
    ok (res > 0, "SendMessage(LB_DIR, DDL_DIRECTORY|DDL_DRIVES, *) failed - 0x%08x\n", GetLastError());

    /* There should be some content in the listbox. In particular, there should
     * be exactly the number of plain files, plus the number of mapped drives.
     */
    itemCount = SendMessage(hList, LB_GETCOUNT, 0, 0);
    ok (itemCount == itemCount_allFiles + itemCount_justDrives + itemCount_allDirs,
        "SendMessage(LB_DIR, DDL_DIRECTORY|DDL_DRIVES) filled with %d entries, expected %d\n",
        itemCount, itemCount_allFiles + itemCount_justDrives + itemCount_allDirs);
    ok(res + 1 == itemCount, "SendMessage(LB_DIR, DDL_DIRECTORY|DDL_DRIVES, w*.c) returned incorrect index!\n");

    /* Every single item in the control should start with a w and end in .c,
     * except for the "[..]" string, which should appear exactly as it is,
     * and the mapped drives in the format "[-X-]".
     */
    for (i = 0; i < itemCount; i++) {
        memset(pathBuffer, 0, MAX_PATH);
        driveletter = '\0';
        SendMessage(hList, LB_GETTEXT, i, (LPARAM)pathBuffer);
        p = pathBuffer + strlen(pathBuffer);
        if (sscanf(pathBuffer, "[-%c-]", &driveletter) == 1) {
            ok( driveletter >= 'a' && driveletter <= 'z', "Drive letter not in range a..z, got ascii %d\n", driveletter);
        }
    }

    /* This tests behavior when no files match the wildcard */
    strcpy(pathBuffer, BAD_EXTENSION);
    SendMessage(hList, LB_RESETCONTENT, 0, 0);
    res = SendMessage(hList, LB_DIR, DDL_DIRECTORY|DDL_DRIVES, (LPARAM)pathBuffer);
    ok (res == itemCount_justDrives -1, "SendMessage(LB_DIR, DDL_DIRECTORY|DDL_DRIVES, %s) returned %d, expected %d\n",
        BAD_EXTENSION, res, itemCount_justDrives -1);

    itemCount = SendMessage(hList, LB_GETCOUNT, 0, 0);
    ok (itemCount == res + 1, "SendMessage(LB_DIR) returned %d expected %d\n", itemCount, res + 1);



    /* Test DDL_DIRECTORY|DDL_DRIVES. */
    strcpy(pathBuffer, "w*.c");
    SendMessage(hList, LB_RESETCONTENT, 0, 0);
    res = SendMessage(hList, LB_DIR, DDL_DIRECTORY|DDL_DRIVES, (LPARAM)pathBuffer);
    ok (res > 0, "SendMessage(LB_DIR, DDL_DIRECTORY|DDL_DRIVES, w*.c) failed - 0x%08x\n", GetLastError());

    /* There should be some content in the listbox. In particular, there should
     * be exactly the number of plain files, plus the number of mapped drives.
     */
    itemCount = SendMessage(hList, LB_GETCOUNT, 0, 0);
    ok (itemCount == itemCount_justFiles + itemCount_justDrives,
        "SendMessage(LB_DIR, DDL_DIRECTORY|DDL_DRIVES) filled with %d entries, expected %d\n",
        itemCount, itemCount_justFiles + itemCount_justDrives);
    ok(res + 1 == itemCount, "SendMessage(LB_DIR, DDL_DIRECTORY|DDL_DRIVES, w*.c) returned incorrect index!\n");

    /* Every single item in the control should start with a w and end in .c,
     * except the mapped drives in the format "[-X-]". The "[..]" directory
     * should not appear.
     */
    for (i = 0; i < itemCount; i++) {
        memset(pathBuffer, 0, MAX_PATH);
        driveletter = '\0';
        SendMessage(hList, LB_GETTEXT, i, (LPARAM)pathBuffer);
        p = pathBuffer + strlen(pathBuffer);
        if (sscanf(pathBuffer, "[-%c-]", &driveletter) == 1) {
            ok( driveletter >= 'a' && driveletter <= 'z', "Drive letter not in range a..z, got ascii %d\n", driveletter);
        } else {
            ok(
                ((pathBuffer[0] == 'w' || pathBuffer[0] == 'W') &&
                (*(p-1) == 'c' || *(p-1) == 'C') &&
                (*(p-2) == '.')), "Element %d (%s) does not fit requested w*.c\n", i, pathBuffer);
        }
    }

    /* Test DDL_DIRECTORY|DDL_EXCLUSIVE. */
    strcpy(pathBuffer, wildcard);
    SendMessage(hList, LB_RESETCONTENT, 0, 0);
    res = SendMessage(hList, LB_DIR, DDL_DIRECTORY|DDL_EXCLUSIVE, (LPARAM)pathBuffer);
    ok (res != -1, "SendMessage(LB_DIR, DDL_DIRECTORY|DDL_EXCLUSIVE, *) failed err %u\n", GetLastError());

    itemCount = SendMessage(hList, LB_GETCOUNT, 0, 0);
    ok (itemCount == itemCount_allDirs,
        "SendMessage(LB_DIR, DDL_DIRECTORY|DDL_EXCLUSIVE) filled with %d entries, expected %d\n",
        itemCount, itemCount_allDirs);
    ok(res + 1 == itemCount, "SendMessage(LB_DIR, DDL_DIRECTORY|DDL_EXCLUSIVE, *) returned incorrect index!\n");

    if (itemCount && GetCurrentDirectoryA( MAX_PATH, pathBuffer ) > 3)  /* there's no [..] in drive root */
    {
        memset(pathBuffer, 0, MAX_PATH);
        SendMessage(hList, LB_GETTEXT, 0, (LPARAM)pathBuffer);
        ok( !strcmp(pathBuffer, "[..]"), "First element is not [..]\n");
    }

    /* This tests behavior when no files match the wildcard */
    strcpy(pathBuffer, BAD_EXTENSION);
    SendMessage(hList, LB_RESETCONTENT, 0, 0);
    res = SendMessage(hList, LB_DIR, DDL_DIRECTORY|DDL_EXCLUSIVE, (LPARAM)pathBuffer);
    ok (res == -1, "SendMessage(LB_DIR, DDL_DIRECTORY|DDL_EXCLUSIVE, %s) returned %d, expected %d\n",
        BAD_EXTENSION, res, -1);

    itemCount = SendMessage(hList, LB_GETCOUNT, 0, 0);
    ok (itemCount == res + 1, "SendMessage(LB_DIR) returned %d expected %d\n", itemCount, res + 1);


    /* Test DDL_DIRECTORY|DDL_EXCLUSIVE. */
    strcpy(pathBuffer, "w*.c");
    SendMessage(hList, LB_RESETCONTENT, 0, 0);
    res = SendMessage(hList, LB_DIR, DDL_DIRECTORY|DDL_EXCLUSIVE, (LPARAM)pathBuffer);
    ok (res == LB_ERR, "SendMessage(LB_DIR, DDL_DIRECTORY|DDL_EXCLUSIVE, w*.c) returned %d expected %d\n", res, LB_ERR);

    /* There should be no elements, since "[..]" does not fit w*.c */
    itemCount = SendMessage(hList, LB_GETCOUNT, 0, 0);
    ok (itemCount == 0,
        "SendMessage(LB_DIR, DDL_DIRECTORY|DDL_EXCLUSIVE) filled with %d entries, expected %d\n",
        itemCount, 0);

    /* Test DDL_DIRECTORY|DDL_DRIVES|DDL_EXCLUSIVE. */
    strcpy(pathBuffer, wildcard);
    SendMessage(hList, LB_RESETCONTENT, 0, 0);
    res = SendMessage(hList, LB_DIR, DDL_DIRECTORY|DDL_DRIVES|DDL_EXCLUSIVE, (LPARAM)pathBuffer);
    ok (res > 0, "SendMessage(LB_DIR, DDL_DIRECTORY|DDL_DRIVES|DDL_EXCLUSIVE, w*.c,) failed - 0x%08x\n", GetLastError());

    /* There should be no plain files on the listbox */
    itemCount = SendMessage(hList, LB_GETCOUNT, 0, 0);
    ok (itemCount == itemCount_justDrives + itemCount_allDirs,
        "SendMessage(LB_DIR, DDL_DIRECTORY|DDL_DRIVES|DDL_EXCLUSIVE) filled with %d entries, expected %d\n",
        itemCount, itemCount_justDrives + itemCount_allDirs);
    ok(res + 1 == itemCount, "SendMessage(LB_DIR, DDL_DIRECTORY|DDL_DRIVES|DDL_EXCLUSIVE, w*.c) returned incorrect index!\n");

    for (i = 0; i < itemCount; i++) {
        memset(pathBuffer, 0, MAX_PATH);
        driveletter = '\0';
        SendMessage(hList, LB_GETTEXT, i, (LPARAM)pathBuffer);
        p = pathBuffer + strlen(pathBuffer);
        if (sscanf(pathBuffer, "[-%c-]", &driveletter) == 1) {
            ok( driveletter >= 'a' && driveletter <= 'z', "Drive letter not in range a..z, got ascii %d\n", driveletter);
        } else {
            ok( pathBuffer[0] == '[' && pathBuffer[strlen(pathBuffer)-1] == ']',
                "Element %d (%s) does not fit expected [...]\n", i, pathBuffer);
        }
    }

    /* This tests behavior when no files match the wildcard */
    strcpy(pathBuffer, BAD_EXTENSION);
    SendMessage(hList, LB_RESETCONTENT, 0, 0);
    res = SendMessage(hList, LB_DIR, DDL_DIRECTORY|DDL_DRIVES|DDL_EXCLUSIVE, (LPARAM)pathBuffer);
    ok (res == itemCount_justDrives -1, "SendMessage(LB_DIR, DDL_DIRECTORY|DDL_DRIVES|DDL_EXCLUSIVE, %s) returned %d, expected %d\n",
        BAD_EXTENSION, res, itemCount_justDrives -1);

    itemCount = SendMessage(hList, LB_GETCOUNT, 0, 0);
    ok (itemCount == res + 1, "SendMessage(LB_DIR) returned %d expected %d\n", itemCount, res + 1);

    /* Test DDL_DIRECTORY|DDL_DRIVES|DDL_EXCLUSIVE. */
    strcpy(pathBuffer, "w*.c");
    SendMessage(hList, LB_RESETCONTENT, 0, 0);
    res = SendMessage(hList, LB_DIR, DDL_DIRECTORY|DDL_DRIVES|DDL_EXCLUSIVE, (LPARAM)pathBuffer);
    ok (res >= 0, "SendMessage(LB_DIR, DDL_DIRECTORY|DDL_DRIVES|DDL_EXCLUSIVE, w*.c,) failed - 0x%08x\n", GetLastError());

    /* There should be no plain files on the listbox, and no [..], since it does not fit w*.c */
    itemCount = SendMessage(hList, LB_GETCOUNT, 0, 0);
    ok (itemCount == itemCount_justDrives,
        "SendMessage(LB_DIR, DDL_DIRECTORY|DDL_DRIVES|DDL_EXCLUSIVE) filled with %d entries, expected %d\n",
        itemCount, itemCount_justDrives);
    ok(res + 1 == itemCount, "SendMessage(LB_DIR, DDL_DIRECTORY|DDL_DRIVES|DDL_EXCLUSIVE, w*.c) returned incorrect index!\n");

    for (i = 0; i < itemCount; i++) {
        memset(pathBuffer, 0, MAX_PATH);
        driveletter = '\0';
        SendMessage(hList, LB_GETTEXT, i, (LPARAM)pathBuffer);
        p = pathBuffer + strlen(pathBuffer);
        ok (sscanf(pathBuffer, "[-%c-]", &driveletter) == 1, "Element %d (%s) does not fit [-X-]\n", i, pathBuffer);
        ok( driveletter >= 'a' && driveletter <= 'z', "Drive letter not in range a..z, got ascii %d\n", driveletter);
    }
    DestroyWindow(hList);

    DeleteFileA( "wtest1.tmp.c" );
}

HWND g_listBox;
HWND g_label;

#define ID_TEST_LABEL    1001
#define ID_TEST_LISTBOX  1002

static BOOL on_listbox_container_create (HWND hwnd, LPCREATESTRUCT lpcs)
{
    g_label = CreateWindow(
        "Static",
        "Contents of static control before DlgDirList.",
        WS_CHILD | WS_VISIBLE,
        10, 10, 512, 32,
        hwnd, (HMENU)ID_TEST_LABEL, NULL, 0);
    if (!g_label) return FALSE;
    g_listBox = CreateWindow(
        "ListBox",
        "DlgDirList test",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | WS_VSCROLL,
        10, 60, 256, 256,
        hwnd, (HMENU)ID_TEST_LISTBOX, NULL, 0);
    if (!g_listBox) return FALSE;

    return TRUE;
}

static LRESULT CALLBACK listbox_container_window_procA (
    HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    switch (uiMsg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_CREATE:
        result = on_listbox_container_create(hwnd, (LPCREATESTRUCTA) lParam)
            ? 0 : (LRESULT)-1;
        break;
    default:
        result = DefWindowProcA (hwnd, uiMsg, wParam, lParam);
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
    cls.hCursor = LoadCursorA (NULL, IDC_ARROW);
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

    file = CreateFileA( "wtest1.tmp.c", GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL );
    ok(file != INVALID_HANDLE_VALUE, "Error creating the test file: %d\n", GetLastError());
    CloseHandle( file );

    /* NOTE: for this test to succeed, there must be no subdirectories
       under the current directory. In addition, there must be at least
       one file that fits the wildcard w*.c . Normally, the test
       directory itself satisfies both conditions.
     */

    hInst = GetModuleHandleA(0);
    if (!RegisterListboxWindowClass(hInst)) assert(0);
    hWnd = CreateWindow("ListboxContainerClass", "ListboxContainerClass",
                    WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            NULL, NULL, hInst, 0);
    assert(hWnd);

    /* Test for standard usage */

    /* The following should be overwritten by the directory path */
    SendMessage(g_label, WM_SETTEXT, 0, (LPARAM)"default contents");

    /* This should list all the w*.c files in the test directory
     * As of this writing, this includes win.c, winstation.c, wsprintf.c
     */
    strcpy(pathBuffer, "w*.c");
    res = DlgDirList(hWnd, pathBuffer, ID_TEST_LISTBOX, ID_TEST_LABEL, 0);
    ok (res == 1, "DlgDirList(*.c, 0) returned %d - expected 1 - 0x%08x\n", res, GetLastError());

    /* Path specification gets converted to uppercase */
    ok (!strcmp(pathBuffer, "W*.C"),
        "expected conversion to uppercase, got %s\n", pathBuffer);

    /* Loaded path should have overwritten the label text */
    SendMessage(g_label, WM_GETTEXT, (WPARAM)MAX_PATH, (LPARAM)pathBuffer);
    trace("Static control after DlgDirList: %s\n", pathBuffer);
    ok (strcmp("default contents", pathBuffer), "DlgDirList() did not modify static control!\n");

    /* There should be some content in the listbox */
    itemCount = SendMessage(g_listBox, LB_GETCOUNT, 0, 0);
    ok (itemCount > 0, "DlgDirList() did NOT fill the listbox!\n");
    itemCount_justFiles = itemCount;

    /* Every single item in the control should start with a w and end in .c */
    for (i = 0; i < itemCount; i++) {
        memset(pathBuffer, 0, MAX_PATH);
        SendMessage(g_listBox, LB_GETTEXT, i, (LPARAM)pathBuffer);
        p = pathBuffer + strlen(pathBuffer);
        ok(((pathBuffer[0] == 'w' || pathBuffer[0] == 'W') &&
            (*(p-1) == 'c' || *(p-1) == 'C') &&
            (*(p-2) == '.')), "Element %d (%s) does not fit requested w*.c\n", i, pathBuffer);
    }

    /* Test behavior when no files match the wildcard */
    strcpy(pathBuffer, BAD_EXTENSION);
    res = DlgDirList(hWnd, pathBuffer, ID_TEST_LISTBOX, ID_TEST_LABEL, 0);
    ok (res == 1, "DlgDirList(%s, 0) returned %d expected 1\n", BAD_EXTENSION, res);

    itemCount = SendMessage(g_listBox, LB_GETCOUNT, 0, 0);
    ok (itemCount == 0, "DlgDirList() DID fill the listbox!\n");

    /* Test DDL_DIRECTORY */
    strcpy(pathBuffer, "w*.c");
    res = DlgDirList(hWnd, pathBuffer, ID_TEST_LISTBOX, ID_TEST_LABEL,
        DDL_DIRECTORY);
    ok (res == 1, "DlgDirList(*.c, DDL_DIRECTORY) failed - 0x%08x\n", GetLastError());

    /* There should be some content in the listbox. In particular, there should
     * be exactly more elements than before, since the directories should
     * have been added.
     */
    itemCount = SendMessage(g_listBox, LB_GETCOUNT, 0, 0);
    itemCount_allDirs = itemCount - itemCount_justFiles;
    ok (itemCount >= itemCount_justFiles,
        "DlgDirList(DDL_DIRECTORY) filled with %d entries, expected > %d\n",
        itemCount, itemCount_justFiles);

    /* Every single item in the control should start with a w and end in .c,
     * except for the "[..]" string, which should appear exactly as it is.
     */
    for (i = 0; i < itemCount; i++) {
        memset(pathBuffer, 0, MAX_PATH);
        SendMessage(g_listBox, LB_GETTEXT, i, (LPARAM)pathBuffer);
        p = pathBuffer + strlen(pathBuffer);
        ok( (pathBuffer[0] == '[' && pathBuffer[strlen(pathBuffer)-1] == ']') ||
            ((pathBuffer[0] == 'w' || pathBuffer[0] == 'W') &&
            (*(p-1) == 'c' || *(p-1) == 'C') &&
            (*(p-2) == '.')), "Element %d (%s) does not fit requested w*.c\n", i, pathBuffer);
    }

    /* Test behavior when no files match the wildcard */
    strcpy(pathBuffer, BAD_EXTENSION);
    res = DlgDirList(hWnd, pathBuffer, ID_TEST_LISTBOX, ID_TEST_LABEL,
        DDL_DIRECTORY);
    ok (res == 1, "DlgDirList(%s, DDL_DIRECTORY) returned %d expected 1\n", BAD_EXTENSION, res);

    itemCount = SendMessage(g_listBox, LB_GETCOUNT, 0, 0);
    ok (itemCount == itemCount_allDirs,
        "DlgDirList() incorrectly filled the listbox! (expected %d got %d)\n",
        itemCount_allDirs, itemCount);
    for (i = 0; i < itemCount; i++) {
        memset(pathBuffer, 0, MAX_PATH);
        SendMessage(g_listBox, LB_GETTEXT, i, (LPARAM)pathBuffer);
        p = pathBuffer + strlen(pathBuffer);
        ok( pathBuffer[0] == '[' && pathBuffer[strlen(pathBuffer)-1] == ']',
            "Element %d (%s) does not fit requested [...]\n", i, pathBuffer);
    }


    /* Test DDL_DRIVES. At least on WinXP-SP2, this implies DDL_EXCLUSIVE */
    strcpy(pathBuffer, "w*.c");
    res = DlgDirList(hWnd, pathBuffer, ID_TEST_LISTBOX, ID_TEST_LABEL,
        DDL_DRIVES);
    ok (res == 1, "DlgDirList(*.c, DDL_DRIVES) failed - 0x%08x\n", GetLastError());

    /* There should be some content in the listbox. In particular, there should
     * be at least one element before, since the string "[-c-]" should
     * have been added. Depending on the user setting, more drives might have
     * been added.
     */
    itemCount = SendMessage(g_listBox, LB_GETCOUNT, 0, 0);
    ok (itemCount >= 1,
        "DlgDirList(DDL_DRIVES) filled with %d entries, expected at least %d\n",
        itemCount, 1);
    itemCount_justDrives = itemCount;

    /* Every single item in the control should fit the format [-c-] */
    for (i = 0; i < itemCount; i++) {
        memset(pathBuffer, 0, MAX_PATH);
        driveletter = '\0';
        SendMessage(g_listBox, LB_GETTEXT, i, (LPARAM)pathBuffer);
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
    res = DlgDirList(hWnd, pathBuffer, ID_TEST_LISTBOX, ID_TEST_LABEL,
        DDL_DRIVES);
    ok (res == 1, "DlgDirList(%s, DDL_DRIVES) returned %d expected 1\n", BAD_EXTENSION, res);

    itemCount = SendMessage(g_listBox, LB_GETCOUNT, 0, 0);
    ok (itemCount == itemCount_justDrives, "DlgDirList() incorrectly filled the listbox!\n");


    /* Test DDL_DIRECTORY|DDL_DRIVES. This does *not* imply DDL_EXCLUSIVE */
    strcpy(pathBuffer, "w*.c");
    res = DlgDirList(hWnd, pathBuffer, ID_TEST_LISTBOX, ID_TEST_LABEL,
        DDL_DIRECTORY|DDL_DRIVES);
    ok (res == 1, "DlgDirList(*.c, DDL_DIRECTORY|DDL_DRIVES) failed - 0x%08x\n", GetLastError());

    /* There should be some content in the listbox. In particular, there should
     * be exactly the number of plain files, plus the number of mapped drives,
     * plus one "[..]"
     */
    itemCount = SendMessage(g_listBox, LB_GETCOUNT, 0, 0);
    ok (itemCount == itemCount_justFiles + itemCount_justDrives + itemCount_allDirs,
        "DlgDirList(DDL_DIRECTORY|DDL_DRIVES) filled with %d entries, expected %d\n",
        itemCount, itemCount_justFiles + itemCount_justDrives + itemCount_allDirs);

    /* Every single item in the control should start with a w and end in .c,
     * except for the "[..]" string, which should appear exactly as it is,
     * and the mapped drives in the format "[-X-]".
     */
    for (i = 0; i < itemCount; i++) {
        memset(pathBuffer, 0, MAX_PATH);
        driveletter = '\0';
        SendMessage(g_listBox, LB_GETTEXT, i, (LPARAM)pathBuffer);
        p = pathBuffer + strlen(pathBuffer);
        if (sscanf(pathBuffer, "[-%c-]", &driveletter) == 1) {
            ok( driveletter >= 'a' && driveletter <= 'z', "Drive letter not in range a..z, got ascii %d\n", driveletter);
        } else {
            ok( (pathBuffer[0] == '[' && pathBuffer[strlen(pathBuffer)-1] == ']') ||
                ((pathBuffer[0] == 'w' || pathBuffer[0] == 'W') &&
                (*(p-1) == 'c' || *(p-1) == 'C') &&
                (*(p-2) == '.')), "Element %d (%s) does not fit requested w*.c\n", i, pathBuffer);
        }
    }

    /* Test behavior when no files match the wildcard */
    strcpy(pathBuffer, BAD_EXTENSION);
    res = DlgDirList(hWnd, pathBuffer, ID_TEST_LISTBOX, ID_TEST_LABEL,
        DDL_DIRECTORY|DDL_DRIVES);
    ok (res == 1, "DlgDirList(%s, DDL_DIRECTORY|DDL_DRIVES) returned %d expected 1\n", BAD_EXTENSION, res);

    itemCount = SendMessage(g_listBox, LB_GETCOUNT, 0, 0);
    ok (itemCount == itemCount_justDrives + itemCount_allDirs,
        "DlgDirList() incorrectly filled the listbox! (expected %d got %d)\n",
        itemCount_justDrives + itemCount_allDirs, itemCount);



    /* Test DDL_DIRECTORY|DDL_EXCLUSIVE. */
    strcpy(pathBuffer, "w*.c");
    res = DlgDirList(hWnd, pathBuffer, ID_TEST_LISTBOX, ID_TEST_LABEL,
        DDL_DIRECTORY|DDL_EXCLUSIVE);
    ok (res == 1, "DlgDirList(*.c, DDL_DIRECTORY|DDL_EXCLUSIVE) failed - 0x%08x\n", GetLastError());

    /* There should be exactly one element: "[..]" */
    itemCount = SendMessage(g_listBox, LB_GETCOUNT, 0, 0);
    ok (itemCount == itemCount_allDirs,
        "DlgDirList(DDL_DIRECTORY|DDL_EXCLUSIVE) filled with %d entries, expected %d\n",
        itemCount, itemCount_allDirs);

    if (itemCount && GetCurrentDirectoryA( MAX_PATH, pathBuffer ) > 3)  /* there's no [..] in drive root */
    {
        memset(pathBuffer, 0, MAX_PATH);
        SendMessage(g_listBox, LB_GETTEXT, 0, (LPARAM)pathBuffer);
        ok( !strcmp(pathBuffer, "[..]"), "First (and only) element is not [..]\n");
    }

    /* Test behavior when no files match the wildcard */
    strcpy(pathBuffer, BAD_EXTENSION);
    res = DlgDirList(hWnd, pathBuffer, ID_TEST_LISTBOX, ID_TEST_LABEL,
        DDL_DIRECTORY|DDL_EXCLUSIVE);
    ok (res == 1, "DlgDirList(%s, DDL_DIRECTORY|DDL_EXCLUSIVE) returned %d expected 1\n", BAD_EXTENSION, res);

    itemCount = SendMessage(g_listBox, LB_GETCOUNT, 0, 0);
    ok (itemCount == itemCount_allDirs, "DlgDirList() incorrectly filled the listbox!\n");


    /* Test DDL_DIRECTORY|DDL_DRIVES|DDL_EXCLUSIVE. */
    strcpy(pathBuffer, "w*.c");
    res = DlgDirList(hWnd, pathBuffer, ID_TEST_LISTBOX, ID_TEST_LABEL,
        DDL_DIRECTORY|DDL_DRIVES|DDL_EXCLUSIVE);
    ok (res == 1, "DlgDirList(*.c, DDL_DIRECTORY|DDL_DRIVES|DDL_EXCLUSIVE) failed - 0x%08x\n", GetLastError());

    /* There should be no plain files on the listbox */
    itemCount = SendMessage(g_listBox, LB_GETCOUNT, 0, 0);
    ok (itemCount == itemCount_justDrives + itemCount_allDirs,
        "DlgDirList(DDL_DIRECTORY|DDL_EXCLUSIVE) filled with %d entries, expected %d\n",
        itemCount, itemCount_justDrives + itemCount_allDirs);

    for (i = 0; i < itemCount; i++) {
        memset(pathBuffer, 0, MAX_PATH);
        driveletter = '\0';
        SendMessage(g_listBox, LB_GETTEXT, i, (LPARAM)pathBuffer);
        p = pathBuffer + strlen(pathBuffer);
        if (sscanf(pathBuffer, "[-%c-]", &driveletter) == 1) {
            ok( driveletter >= 'a' && driveletter <= 'z', "Drive letter not in range a..z, got ascii %d\n", driveletter);
        } else {
            ok( pathBuffer[0] == '[' && pathBuffer[strlen(pathBuffer)-1] == ']',
                "Element %d (%s) does not fit expected [...]\n", i, pathBuffer);
        }
    }

    /* Test behavior when no files match the wildcard */
    strcpy(pathBuffer, BAD_EXTENSION);
    res = DlgDirList(hWnd, pathBuffer, ID_TEST_LISTBOX, ID_TEST_LABEL,
        DDL_DIRECTORY|DDL_DRIVES|DDL_EXCLUSIVE);
    ok (res == 1, "DlgDirList(%s, DDL_DIRECTORY|DDL_DRIVES|DDL_EXCLUSIVE) returned %d expected 1\n", BAD_EXTENSION, res);

    itemCount = SendMessage(g_listBox, LB_GETCOUNT, 0, 0);
    ok (itemCount == itemCount_justDrives + itemCount_allDirs,
        "DlgDirList() incorrectly filled the listbox!\n");


    /* Now test DlgDirSelectEx() in normal operation */
    /* Fill with everything - drives, directory and all plain files. */
    strcpy(pathBuffer, "*");
    res = DlgDirList(hWnd, pathBuffer, ID_TEST_LISTBOX, ID_TEST_LABEL,
        DDL_DIRECTORY|DDL_DRIVES);
    ok (res != 0, "DlgDirList(*, DDL_DIRECTORY|DDL_DRIVES) failed - 0x%08x\n", GetLastError());

    SendMessage(g_listBox, LB_SETCURSEL, -1, 0); /* Unselect any current selection */
    memset(pathBuffer, 0, MAX_PATH);
    SetLastError(0xdeadbeef);
    res = DlgDirSelectEx(hWnd, pathBuffer, MAX_PATH, ID_TEST_LISTBOX);
    ok (GetLastError() == 0xdeadbeef,
        "DlgDirSelectEx() with no selection modified last error code from 0xdeadbeef to 0x%08x\n",
        GetLastError());
    ok (res == 0, "DlgDirSelectEx() with no selection returned %d, expected 0\n", res);
    /* WinXP-SP2 leaves pathBuffer untouched, but Win98 fills it with garbage. */
    /*
    ok (strlen(pathBuffer) == 0, "DlgDirSelectEx() with no selection filled buffer with %s\n", pathBuffer);
    */
    /* Test proper drive/dir/file recognition */
    itemCount = SendMessage(g_listBox, LB_GETCOUNT, 0, 0);
    for (i = 0; i < itemCount; i++) {
        memset(itemBuffer, 0, MAX_PATH);
        memset(pathBuffer, 0, MAX_PATH);
        memset(tempBuffer, 0, MAX_PATH);
        driveletter = '\0';
        SendMessage(g_listBox, LB_GETTEXT, i, (LPARAM)itemBuffer);
        res = SendMessage(g_listBox, LB_SETCURSEL, i, 0);
        ok (res == i, "SendMessage(LB_SETCURSEL, %d) failed\n", i);
        if (sscanf(itemBuffer, "[-%c-]", &driveletter) == 1) {
            /* Current item is a drive letter */
            SetLastError(0xdeadbeef);
            res = DlgDirSelectEx(hWnd, pathBuffer, MAX_PATH, ID_TEST_LISTBOX);
            ok (GetLastError() == 0xdeadbeef,
               "DlgDirSelectEx() with selection at %d modified last error code from 0xdeadbeef to 0x%08x\n",
                i, GetLastError());
            ok(res == 1, "DlgDirSelectEx() thinks %s (%s) is not a drive/directory!\n", itemBuffer, pathBuffer);

            /* For drive letters, DlgDirSelectEx tacks on a colon */
            ok (pathBuffer[0] == driveletter && pathBuffer[1] == ':' && pathBuffer[2] == '\0',
                "%d: got \"%s\" expected \"%c:\"\n", i, pathBuffer, driveletter);
        } else if (itemBuffer[0] == '[') {
            /* Current item is the parent directory */
            SetLastError(0xdeadbeef);
            res = DlgDirSelectEx(hWnd, pathBuffer, MAX_PATH, ID_TEST_LISTBOX);
            ok (GetLastError() == 0xdeadbeef,
               "DlgDirSelectEx() with selection at %d modified last error code from 0xdeadbeef to 0x%08x\n",
                i, GetLastError());
            ok(res == 1, "DlgDirSelectEx() thinks %s (%s) is not a drive/directory!\n", itemBuffer, pathBuffer);

            /* For directories, DlgDirSelectEx tacks on a backslash */
            p = pathBuffer + strlen(pathBuffer);
            ok (*(p-1) == '\\', "DlgDirSelectEx did NOT tack on a backslash to dir, got %s\n", pathBuffer);

            tempBuffer[0] = '[';
            strncpy(tempBuffer + 1, pathBuffer, strlen(pathBuffer) - 1);
            strcat(tempBuffer, "]");
            ok (!strcmp(tempBuffer, itemBuffer), "Formatted directory should be %s, got %s\n", tempBuffer, itemBuffer);
        } else {
            /* Current item is a plain file */
            SetLastError(0xdeadbeef);
            res = DlgDirSelectEx(hWnd, pathBuffer, MAX_PATH, ID_TEST_LISTBOX);
            ok (GetLastError() == 0xdeadbeef,
               "DlgDirSelectEx() with selection at %d modified last error code from 0xdeadbeef to 0x%08x\n",
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

    /* Now test DlgDirSelectEx() in abnormal operation */
    /* Fill list with bogus entries, that look somewhat valid */
    SendMessage(g_listBox, LB_RESETCONTENT, 0, 0);
    SendMessage(g_listBox, LB_ADDSTRING, 0, (LPARAM)"[notexist.dir]");
    SendMessage(g_listBox, LB_ADDSTRING, 0, (LPARAM)"notexist.fil");
    itemCount = SendMessage(g_listBox, LB_GETCOUNT, 0, 0);
    for (i = 0; i < itemCount; i++) {
        memset(itemBuffer, 0, MAX_PATH);
        memset(pathBuffer, 0, MAX_PATH);
        memset(tempBuffer, 0, MAX_PATH);
        driveletter = '\0';
        SendMessage(g_listBox, LB_GETTEXT, i, (LPARAM)itemBuffer);
        res = SendMessage(g_listBox, LB_SETCURSEL, i, 0);
        ok (res == i, "SendMessage(LB_SETCURSEL, %d) failed\n", i);
        if (sscanf(itemBuffer, "[-%c-]", &driveletter) == 1) {
            /* Current item is a drive letter */
            SetLastError(0xdeadbeef);
            res = DlgDirSelectEx(hWnd, pathBuffer, MAX_PATH, ID_TEST_LISTBOX);
            ok (GetLastError() == 0xdeadbeef,
               "DlgDirSelectEx() with selection at %d modified last error code from 0xdeadbeef to 0x%08x\n",
                i, GetLastError());
            ok(res == 1, "DlgDirSelectEx() thinks %s (%s) is not a drive/directory!\n", itemBuffer, pathBuffer);

            /* For drive letters, DlgDirSelectEx tacks on a colon */
            ok (pathBuffer[0] == driveletter && pathBuffer[1] == ':' && pathBuffer[2] == '\0',
                "%d: got \"%s\" expected \"%c:\"\n", i, pathBuffer, driveletter);
        } else if (itemBuffer[0] == '[') {
            /* Current item is the parent directory */
            SetLastError(0xdeadbeef);
            res = DlgDirSelectEx(hWnd, pathBuffer, MAX_PATH, ID_TEST_LISTBOX);
            ok (GetLastError() == 0xdeadbeef,
               "DlgDirSelectEx() with selection at %d modified last error code from 0xdeadbeef to 0x%08x\n",
                i, GetLastError());
            ok(res == 1, "DlgDirSelectEx() thinks %s (%s) is not a drive/directory!\n", itemBuffer, pathBuffer);

            /* For directories, DlgDirSelectEx tacks on a backslash */
            p = pathBuffer + strlen(pathBuffer);
            ok (*(p-1) == '\\', "DlgDirSelectEx did NOT tack on a backslash to dir, got %s\n", pathBuffer);

            tempBuffer[0] = '[';
            strncpy(tempBuffer + 1, pathBuffer, strlen(pathBuffer) - 1);
            strcat(tempBuffer, "]");
            ok (!strcmp(tempBuffer, itemBuffer), "Formatted directory should be %s, got %s\n", tempBuffer, itemBuffer);
        } else {
            /* Current item is a plain file */
            SetLastError(0xdeadbeef);
            res = DlgDirSelectEx(hWnd, pathBuffer, MAX_PATH, ID_TEST_LISTBOX);
            ok (GetLastError() == 0xdeadbeef,
               "DlgDirSelectEx() with selection at %d modified last error code from 0xdeadbeef to 0x%08x\n",
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
    DestroyWindow(hWnd);

    DeleteFileA( "wtest1.tmp.c" );
}

START_TEST(listbox)
{
  const struct listbox_test SS =
/*   {add_style} */
    {{0},
     {LB_ERR, LB_ERR,      0, LB_ERR}, {0,0,0,0},
     {     1,      1,      1, LB_ERR}, {0,0,0,0},
     {     2,      2,      2, LB_ERR}, {0,0,0,0},
     {LB_ERR, LB_ERR,      0, LB_ERR}, {0,0,0,0}};
/* {selected, anchor,  caret, selcount}{TODO fields} */
  const struct listbox_test SS_NS =
    {{LBS_NOSEL},
     {LB_ERR, LB_ERR,      0, LB_ERR}, {0,0,0,0},
     {     1,      1,      1, LB_ERR}, {0,0,0,0},
     {     2,      2,      2, LB_ERR}, {0,0,0,0},
     {LB_ERR, LB_ERR,      0, LB_ERR}, {0,0,0,0}};
  const struct listbox_test MS =
    {{LBS_MULTIPLESEL},
     {     0, LB_ERR,      0,      0}, {0,0,0,0},
     {     1,      1,      1,      1}, {0,0,0,0},
     {     2,      1,      2,      1}, {0,0,0,0},
     {     0, LB_ERR,      0,      2}, {0,0,0,0}};
  const struct listbox_test MS_NS =
    {{LBS_MULTIPLESEL | LBS_NOSEL},
     {LB_ERR, LB_ERR,      0, LB_ERR}, {0,0,0,0},
     {     1,      1,      1, LB_ERR}, {0,0,0,0},
     {     2,      2,      2, LB_ERR}, {0,0,0,0},
     {LB_ERR, LB_ERR,      0, LB_ERR}, {0,0,0,0}};
  const struct listbox_test ES =
    {{LBS_EXTENDEDSEL},
     {     0, LB_ERR,      0,      0}, {0,0,0,0},
     {     1,      1,      1,      1}, {0,0,0,0},
     {     2,      2,      2,      1}, {0,0,0,0},
     {     0, LB_ERR,      0,      2}, {0,0,0,0}};
  const struct listbox_test ES_NS =
    {{LBS_EXTENDEDSEL | LBS_NOSEL},
     {LB_ERR, LB_ERR,      0, LB_ERR}, {0,0,0,0},
     {     1,      1,      1, LB_ERR}, {0,0,0,0},
     {     2,      2,      2, LB_ERR}, {0,0,0,0},
     {LB_ERR, LB_ERR,      0, LB_ERR}, {0,0,0,0}};
  const struct listbox_test EMS =
    {{LBS_EXTENDEDSEL | LBS_MULTIPLESEL},
     {     0, LB_ERR,      0,      0}, {0,0,0,0},
     {     1,      1,      1,      1}, {0,0,0,0},
     {     2,      2,      2,      1}, {0,0,0,0},
     {     0, LB_ERR,      0,      2}, {0,0,0,0}};
  const struct listbox_test EMS_NS =
    {{LBS_EXTENDEDSEL | LBS_MULTIPLESEL | LBS_NOSEL},
     {LB_ERR, LB_ERR,      0, LB_ERR}, {0,0,0,0},
     {     1,      1,      1, LB_ERR}, {0,0,0,0},
     {     2,      2,      2, LB_ERR}, {0,0,0,0},
     {LB_ERR, LB_ERR,      0, LB_ERR}, {0,0,0,0}};

  trace (" Testing single selection...\n");
  check (SS);
  trace (" ... with NOSEL\n");
  check (SS_NS);
  trace (" Testing multiple selection...\n");
  check (MS);
  trace (" ... with NOSEL\n");
  check (MS_NS);
  trace (" Testing extended selection...\n");
  check (ES);
  trace (" ... with NOSEL\n");
  check (ES_NS);
  trace (" Testing extended and multiple selection...\n");
  check (EMS);
  trace (" ... with NOSEL\n");
  check (EMS_NS);

  check_item_height();
  test_ownerdraw();
  test_selection();
  test_listbox_height();
  test_itemfrompoint();
  test_listbox_item_data();
  test_listbox_LB_DIR();
  test_listbox_dlgdir();
}
