/*
 * ListView tests
 *
 * Copyright 2006 Mike McCormack for CodeWeavers
 * Copyright 2007 George Gov
 * Copyright 2009-2012 Nikolay Sivov
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
#include "v6util.h"
#include "msg.h"

#define PARENT_SEQ_INDEX       0
#define PARENT_FULL_SEQ_INDEX  1
#define LISTVIEW_SEQ_INDEX     2
#define EDITBOX_SEQ_INDEX      3
#define COMBINED_SEQ_INDEX     4
#define NUM_MSG_SEQUENCES      5

#define LISTVIEW_ID 0
#define HEADER_ID   1

#define expect(expected, got) ok(got == expected, "Expected %d, got %d\n", expected, got)
#define expect2(expected1, expected2, got1, got2) ok(expected1 == got1 && expected2 == got2, \
       "expected (%d,%d), got (%d,%d)\n", expected1, expected2, got1, got2)

static const WCHAR testparentclassW[] =
    {'L','i','s','t','v','i','e','w',' ','t','e','s','t',' ','p','a','r','e','n','t','W', 0};

static HWND hwndparent, hwndparentW;
/* prevents edit box creation, LVN_BEGINLABELEDIT return value */
static BOOL blockEdit;
/* return nonzero on NM_HOVER */
static BOOL g_block_hover;
/* notification data for LVN_ITEMCHANGED */
static NMLISTVIEW g_nmlistview;
/* format reported to control:
   -1 falls to defproc, anything else returned */
static INT notifyFormat;
/* indicates we're running < 5.80 version */
static BOOL g_is_below_5;
/* item data passed to LVN_GETDISPINFOA */
static LVITEMA g_itema;
/* alter notification code A->W */
static BOOL g_disp_A_to_W;
/* dispinfo data sent with LVN_LVN_ENDLABELEDIT */
static NMLVDISPINFO g_editbox_disp_info;

static HWND subclass_editbox(HWND hwndListview);

static struct msg_sequence *sequences[NUM_MSG_SEQUENCES];

static const struct message create_ownerdrawfixed_parent_seq[] = {
    { WM_NOTIFYFORMAT, sent },
    { WM_QUERYUISTATE, sent|optional }, /* Win2K and higher */
    { WM_MEASUREITEM, sent },
    { WM_PARENTNOTIFY, sent },
    { 0 }
};

static const struct message redraw_listview_seq[] = {
    { WM_PAINT,      sent|id,            0, 0, LISTVIEW_ID },
    { WM_PAINT,      sent|id,            0, 0, HEADER_ID },
    { WM_NCPAINT,    sent|id|defwinproc, 0, 0, HEADER_ID },
    { WM_ERASEBKGND, sent|id|defwinproc|optional, 0, 0, HEADER_ID },
    { WM_NOTIFY,     sent|id|defwinproc, 0, 0, LISTVIEW_ID },
    { WM_NCPAINT,    sent|id|defwinproc, 0, 0, LISTVIEW_ID },
    { WM_ERASEBKGND, sent|id|defwinproc|optional, 0, 0, LISTVIEW_ID },
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
    { WM_NCPAINT,         sent|optional },
    { WM_ERASEBKGND,      sent|optional },
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
    { WM_NCPAINT,          sent|optional },
    { WM_ERASEBKGND,       sent|optional },
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

static const struct message ownderdata_select_focus_parent_seq[] = {
    { WM_NOTIFY, sent|id, 0, 0, LVN_ITEMCHANGED },
    { WM_NOTIFY, sent|id, 0, 0, LVN_GETDISPINFOA },
    { WM_NOTIFY, sent|id|optional, 0, 0, LVN_GETDISPINFOA }, /* version 4.7x */
    { 0 }
};

static const struct message ownerdata_setstate_all_parent_seq[] = {
    { WM_NOTIFY, sent|id, 0, 0, LVN_ITEMCHANGED },
    { 0 }
};

static const struct message ownerdata_defocus_all_parent_seq[] = {
    { WM_NOTIFY, sent|id, 0, 0, LVN_ITEMCHANGED },
    { WM_NOTIFY, sent|id, 0, 0, LVN_GETDISPINFOA },
    { WM_NOTIFY, sent|id|optional, 0, 0, LVN_GETDISPINFOA },
    { WM_NOTIFY, sent|id, 0, 0, LVN_ITEMCHANGED },
    { 0 }
};

static const struct message ownerdata_deselect_all_parent_seq[] = {
    { WM_NOTIFY, sent|id, 0, 0, LVN_ODCACHEHINT },
    { WM_NOTIFY, sent|id, 0, 0, LVN_ITEMCHANGED },
    { 0 }
};

static const struct message select_all_parent_seq[] = {
    { WM_NOTIFY, sent|id, 0, 0, LVN_ITEMCHANGING },
    { WM_NOTIFY, sent|id, 0, 0, LVN_ITEMCHANGED },

    { WM_NOTIFY, sent|id, 0, 0, LVN_ITEMCHANGING },
    { WM_NOTIFY, sent|id, 0, 0, LVN_ITEMCHANGED },

    { WM_NOTIFY, sent|id, 0, 0, LVN_ITEMCHANGING },
    { WM_NOTIFY, sent|id, 0, 0, LVN_ITEMCHANGED },

    { WM_NOTIFY, sent|id, 0, 0, LVN_ITEMCHANGING },
    { WM_NOTIFY, sent|id, 0, 0, LVN_ITEMCHANGED },

    { WM_NOTIFY, sent|id, 0, 0, LVN_ITEMCHANGING },
    { WM_NOTIFY, sent|id, 0, 0, LVN_ITEMCHANGED },
    { 0 }
};

static const struct message textcallback_set_again_parent_seq[] = {
    { WM_NOTIFY, sent|id, 0, 0, LVN_ITEMCHANGING },
    { WM_NOTIFY, sent|id, 0, 0, LVN_ITEMCHANGED  },
    { 0 }
};

static const struct message single_getdispinfo_parent_seq[] = {
    { WM_NOTIFY, sent|id, 0, 0, LVN_GETDISPINFOA },
    { 0 }
};

static const struct message getitemposition_seq1[] = {
    { LVM_GETITEMPOSITION, sent|id, 0, 0, LISTVIEW_ID },
    { 0 }
};

static const struct message getitemposition_seq2[] = {
    { LVM_GETITEMPOSITION, sent|id, 0, 0, LISTVIEW_ID },
    { HDM_GETITEMRECT, sent|id, 0, 0, HEADER_ID },
    { 0 }
};

static const struct message editbox_create_pos[] = {
    /* sequence sent after LVN_BEGINLABELEDIT */
    /* next two are 4.7x specific */
    { WM_WINDOWPOSCHANGING, sent },
    { WM_WINDOWPOSCHANGED, sent|optional },

    { WM_WINDOWPOSCHANGING, sent|optional },
    { WM_NCCALCSIZE, sent },
    { WM_WINDOWPOSCHANGED, sent },
    { WM_MOVE, sent|defwinproc },
    { WM_SIZE, sent|defwinproc },
    /* the rest is todo, skipped in 4.7x */
    { WM_WINDOWPOSCHANGING, sent|optional },
    { WM_WINDOWPOSCHANGED, sent|optional },
    { 0 }
};

static const struct message scroll_parent_seq[] = {
    { WM_NOTIFY, sent|id, 0, 0, LVN_BEGINSCROLL },
    { WM_NOTIFY, sent|id, 0, 0, LVN_ENDSCROLL },
    { 0 }
};

static const struct message setredraw_seq[] = {
    { WM_SETREDRAW, sent|id|wparam, FALSE, 0, LISTVIEW_ID },
    { 0 }
};

static const struct message lvs_ex_transparentbkgnd_seq[] = {
    { WM_PRINTCLIENT, sent|lparam, 0, PRF_ERASEBKGND },
    { 0 }
};

static const struct message edit_end_nochange[] = {
    { WM_NOTIFY, sent|id, 0, 0, LVN_ENDLABELEDITA },
    { WM_NOTIFY, sent|id, 0, 0, NM_CUSTOMDRAW },     /* todo */
    { WM_NOTIFY, sent|id, 0, 0, NM_SETFOCUS },
    { 0 }
};

static const struct message hover_parent[] = {
    { WM_GETDLGCODE, sent }, /* todo_wine */
    { WM_NOTIFY, sent|id, 0, 0, NM_HOVER },
    { 0 }
};

static const struct message listview_destroy[] = {
    { 0x0090, sent|optional }, /* Vista */
    { WM_PARENTNOTIFY, sent },
    { WM_SHOWWINDOW, sent },
    { WM_WINDOWPOSCHANGING, sent },
    { WM_WINDOWPOSCHANGED, sent|optional },
    { WM_DESTROY, sent },
    { WM_NOTIFY, sent|id, 0, 0, LVN_DELETEALLITEMS },
    { WM_NCDESTROY, sent },
    { 0 }
};

static const struct message listview_header_changed_seq[] = {
    { LVM_SETCOLUMNA, sent },
    { WM_NOTIFY, sent|id|defwinproc, 0, 0, LISTVIEW_ID },
    { WM_NOTIFY, sent|id|defwinproc, 0, 0, LISTVIEW_ID },
    { 0 }
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
    if (message == WM_NOTIFY && lParam) msg.id = ((NMHDR*)lParam)->code;

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
        add_message(sequences, COMBINED_SEQ_INDEX, &msg);
    }
    add_message(sequences, PARENT_FULL_SEQ_INDEX, &msg);

    switch (message)
    {
      case WM_NOTIFY:
      {
          switch (((NMHDR*)lParam)->code)
          {
          case LVN_BEGINLABELEDIT:
          {
              HWND edit = NULL;

              /* subclass edit box */
              if (!blockEdit)
                  edit = subclass_editbox(((NMHDR*)lParam)->hwndFrom);

              if (edit)
              {
                  INT len = SendMessageA(edit, EM_GETLIMITTEXT, 0, 0);
                  ok(len == 259 || broken(len == 260) /* includes NULL in NT4 */,
                      "text limit %d, expected 259\n", len);
              }

              return blockEdit;
          }
          case LVN_ENDLABELEDIT:
              {
              HWND edit;

              /* always accept new item text */
              NMLVDISPINFO *di = (NMLVDISPINFO*)lParam;
              g_editbox_disp_info = *di;
              trace("LVN_ENDLABELEDIT: text=%s\n", di->item.pszText ? di->item.pszText : "(null)");

              /* edit control still available from this notification */
              edit = (HWND)SendMessageA(((NMHDR*)lParam)->hwndFrom, LVM_GETEDITCONTROL, 0, 0);
              ok(IsWindow(edit), "expected valid edit control handle\n");
              ok((GetWindowLongA(edit, GWL_STYLE) & ES_MULTILINE) == 0, "edit is multiline\n");

              return TRUE;
              }
          case LVN_BEGINSCROLL:
          case LVN_ENDSCROLL:
              {
              NMLVSCROLL *pScroll = (NMLVSCROLL*)lParam;

              trace("LVN_%sSCROLL: (%d,%d)\n", pScroll->hdr.code == LVN_BEGINSCROLL ?
                                               "BEGIN" : "END", pScroll->dx, pScroll->dy);
              }
              break;
          case LVN_ITEMCHANGED:
              {
                  NMLISTVIEW *nmlv = (NMLISTVIEW*)lParam;
                  g_nmlistview = *nmlv;
              }
              break;
          case LVN_GETDISPINFOA:
              {
                  NMLVDISPINFOA *dispinfo = (NMLVDISPINFOA*)lParam;
                  g_itema = dispinfo->item;

                  if (g_disp_A_to_W && (dispinfo->item.mask & LVIF_TEXT))
                  {
                      static const WCHAR testW[] = {'T','E','S','T',0};
                      dispinfo->hdr.code = LVN_GETDISPINFOW;
                      memcpy(dispinfo->item.pszText, testW, sizeof(testW));
                  }

                  /* test control buffer size for text, 10 used to mask cases when control
                     is using caller buffer to process LVM_GETITEM for example */
                  if (dispinfo->item.mask & LVIF_TEXT && dispinfo->item.cchTextMax > 10)
                      ok(dispinfo->item.cchTextMax == 260 ||
                         broken(dispinfo->item.cchTextMax == 264) /* NT4 reports aligned size */,
                      "buffer size %d\n", dispinfo->item.cchTextMax);
              }
              break;
          case NM_HOVER:
              if (g_block_hover) return 1;
              break;
          }
          break;
      }
      case WM_NOTIFYFORMAT:
      {
          /* force to return format */
          if (lParam == NF_QUERY && notifyFormat != -1) return notifyFormat;
          break;
      }
    }

    defwndproc_counter++;
    ret = DefWindowProcA(hwnd, message, wParam, lParam);
    defwndproc_counter--;

    return ret;
}

static BOOL register_parent_wnd_class(BOOL Unicode)
{
    WNDCLASSA clsA;
    WNDCLASSW clsW;

    if (Unicode)
    {
        clsW.style = 0;
        clsW.lpfnWndProc = parent_wnd_proc;
        clsW.cbClsExtra = 0;
        clsW.cbWndExtra = 0;
        clsW.hInstance = GetModuleHandleW(NULL);
        clsW.hIcon = 0;
        clsW.hCursor = LoadCursorA(0, IDC_ARROW);
        clsW.hbrBackground = GetStockObject(WHITE_BRUSH);
        clsW.lpszMenuName = NULL;
        clsW.lpszClassName = testparentclassW;
    }
    else
    {
        clsA.style = 0;
        clsA.lpfnWndProc = parent_wnd_proc;
        clsA.cbClsExtra = 0;
        clsA.cbWndExtra = 0;
        clsA.hInstance = GetModuleHandleA(NULL);
        clsA.hIcon = 0;
        clsA.hCursor = LoadCursorA(0, IDC_ARROW);
        clsA.hbrBackground = GetStockObject(WHITE_BRUSH);
        clsA.lpszMenuName = NULL;
        clsA.lpszClassName = "Listview test parent class";
    }

    return Unicode ? RegisterClassW(&clsW) : RegisterClassA(&clsA);
}

static HWND create_parent_window(BOOL Unicode)
{
    static const WCHAR nameW[] = {'t','e','s','t','p','a','r','e','n','t','n','a','m','e','W',0};
    HWND hwnd;

    if (!register_parent_wnd_class(Unicode))
        return NULL;

    blockEdit = FALSE;
    notifyFormat = -1;

    if (Unicode)
        hwnd = CreateWindowExW(0, testparentclassW, nameW,
                               WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX |
                               WS_MAXIMIZEBOX | WS_VISIBLE,
                               0, 0, 100, 100,
                               GetDesktopWindow(), NULL, GetModuleHandleW(NULL), NULL);
    else
        hwnd = CreateWindowExA(0, "Listview test parent class",
                               "Listview test parent window",
                               WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX |
                               WS_MAXIMIZEBOX | WS_VISIBLE,
                               0, 0, 100, 100,
                               GetDesktopWindow(), NULL, GetModuleHandleA(NULL), NULL);
    SetWindowPos( hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE );
    return hwnd;
}

static LRESULT WINAPI listview_subclass_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WNDPROC oldproc = (WNDPROC)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
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
    add_message(sequences, COMBINED_SEQ_INDEX, &msg);

    defwndproc_counter++;
    ret = CallWindowProcA(oldproc, hwnd, message, wParam, lParam);
    defwndproc_counter--;
    return ret;
}

static HWND create_listview_control(DWORD style)
{
    WNDPROC oldproc;
    HWND hwnd;
    RECT rect;

    GetClientRect(hwndparent, &rect);
    hwnd = CreateWindowExA(0, WC_LISTVIEW, "foo",
                           WS_CHILD | WS_BORDER | WS_VISIBLE | style,
                           0, 0, rect.right, rect.bottom,
                           hwndparent, NULL, GetModuleHandleA(NULL), NULL);
    ok(hwnd != NULL, "gle=%d\n", GetLastError());

    if (!hwnd) return NULL;

    oldproc = (WNDPROC)SetWindowLongPtrA(hwnd, GWLP_WNDPROC,
                                        (LONG_PTR)listview_subclass_proc);
    SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)oldproc);

    return hwnd;
}

/* unicode listview window with specified parent */
static HWND create_listview_controlW(DWORD style, HWND parent)
{
    WNDPROC oldproc;
    HWND hwnd;
    RECT rect;
    static const WCHAR nameW[] = {'f','o','o',0};

    GetClientRect(parent, &rect);
    hwnd = CreateWindowExW(0, WC_LISTVIEWW, nameW,
                           WS_CHILD | WS_BORDER | WS_VISIBLE | style,
                           0, 0, rect.right, rect.bottom,
                           parent, NULL, GetModuleHandleW(NULL), NULL);
    ok(hwnd != NULL, "gle=%d\n", GetLastError());

    if (!hwnd) return NULL;

    oldproc = (WNDPROC)SetWindowLongPtrW(hwnd, GWLP_WNDPROC,
                                        (LONG_PTR)listview_subclass_proc);
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)oldproc);

    return hwnd;
}

static LRESULT WINAPI header_subclass_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WNDPROC oldproc = (WNDPROC)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
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
    ret = CallWindowProcA(oldproc, hwnd, message, wParam, lParam);
    defwndproc_counter--;
    return ret;
}

static HWND subclass_header(HWND hwndListview)
{
    WNDPROC oldproc;
    HWND hwnd;

    hwnd = ListView_GetHeader(hwndListview);
    oldproc = (WNDPROC)SetWindowLongPtrA(hwnd, GWLP_WNDPROC,
                                         (LONG_PTR)header_subclass_proc);
    SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)oldproc);

    return hwnd;
}

static LRESULT WINAPI editbox_subclass_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WNDPROC oldproc = (WNDPROC)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    static LONG defwndproc_counter = 0;
    LRESULT ret;
    struct message msg;

    msg.message = message;
    msg.flags = sent|wparam|lparam;
    if (defwndproc_counter) msg.flags |= defwinproc;
    msg.wParam = wParam;
    msg.lParam = lParam;

    /* all we need is sizing */
    if (message == WM_WINDOWPOSCHANGING ||
        message == WM_NCCALCSIZE ||
        message == WM_WINDOWPOSCHANGED ||
        message == WM_MOVE ||
        message == WM_SIZE)
    {
        add_message(sequences, EDITBOX_SEQ_INDEX, &msg);
    }

    defwndproc_counter++;
    ret = CallWindowProcA(oldproc, hwnd, message, wParam, lParam);
    defwndproc_counter--;
    return ret;
}

static HWND subclass_editbox(HWND hwndListview)
{
    WNDPROC oldproc;
    HWND hwnd;

    hwnd = (HWND)SendMessage(hwndListview, LVM_GETEDITCONTROL, 0, 0);
    oldproc = (WNDPROC)SetWindowLongPtrA(hwnd, GWLP_WNDPROC,
                                         (LONG_PTR)editbox_subclass_proc);
    SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)oldproc);

    return hwnd;
}

/* Performs a single LVM_HITTEST test */
static void test_lvm_hittest_(HWND hwnd, INT x, INT y, INT item, UINT flags, UINT broken_flags,
                              BOOL todo_item, BOOL todo_flags, int line)
{
    LVHITTESTINFO lpht;
    INT ret;

    lpht.pt.x = x;
    lpht.pt.y = y;
    lpht.iSubItem = 10;

    ret = SendMessage(hwnd, LVM_HITTEST, 0, (LPARAM)&lpht);

    if (todo_item)
    {
        todo_wine
        {
            ok_(__FILE__, line)(ret == item, "Expected %d retval, got %d\n", item, ret);
            ok_(__FILE__, line)(lpht.iItem == item, "Expected %d item, got %d\n", item, lpht.iItem);
            ok_(__FILE__, line)(lpht.iSubItem == 10, "Expected subitem not overwrited\n");
        }
    }
    else
    {
        ok_(__FILE__, line)(ret == item, "Expected %d retval, got %d\n", item, ret);
        ok_(__FILE__, line)(lpht.iItem == item, "Expected %d item, got %d\n", item, lpht.iItem);
        ok_(__FILE__, line)(lpht.iSubItem == 10, "Expected subitem not overwrited\n");
    }

    if (todo_flags)
    {
        todo_wine
            ok_(__FILE__, line)(lpht.flags == flags, "Expected flags 0x%x, got 0x%x\n", flags, lpht.flags);
    }
    else if (broken_flags)
        ok_(__FILE__, line)(lpht.flags == flags || broken(lpht.flags == broken_flags),
                            "Expected flags %x, got %x\n", flags, lpht.flags);
    else
        ok_(__FILE__, line)(lpht.flags == flags, "Expected flags 0x%x, got 0x%x\n", flags, lpht.flags);
}

#define test_lvm_hittest(a,b,c,d,e,f,g,h) test_lvm_hittest_(a,b,c,d,e,f,g,h,__LINE__)

/* Performs a single LVM_SUBITEMHITTEST test */
static void test_lvm_subitemhittest_(HWND hwnd, INT x, INT y, INT item, INT subitem, UINT flags,
                                     BOOL todo_item, BOOL todo_subitem, BOOL todo_flags, int line)
{
    LVHITTESTINFO lpht;
    INT ret;

    lpht.pt.x = x;
    lpht.pt.y = y;

    ret = SendMessage(hwnd, LVM_SUBITEMHITTEST, 0, (LPARAM)&lpht);

    if (todo_item)
    {
        todo_wine
        {
            ok_(__FILE__, line)(ret == item, "Expected %d retval, got %d\n", item, ret);
            ok_(__FILE__, line)(lpht.iItem == item, "Expected %d item, got %d\n", item, lpht.iItem);
        }
    }
    else
    {
        ok_(__FILE__, line)(ret == item, "Expected %d retval, got %d\n", item, ret);
        ok_(__FILE__, line)(lpht.iItem == item, "Expected %d item, got %d\n", item, lpht.iItem);
    }

    if (todo_subitem)
    {
        todo_wine
            ok_(__FILE__, line)(lpht.iSubItem == subitem, "Expected subitem %d, got %d\n", subitem, lpht.iSubItem);
    }
    else
        ok_(__FILE__, line)(lpht.iSubItem == subitem, "Expected subitem %d, got %d\n", subitem, lpht.iSubItem);

    if (todo_flags)
    {
        todo_wine
            ok_(__FILE__, line)(lpht.flags == flags, "Expected flags 0x%x, got 0x%x\n", flags, lpht.flags);
    }
    else
        ok_(__FILE__, line)(lpht.flags == flags, "Expected flags 0x%x, got 0x%x\n", flags, lpht.flags);
}

#define test_lvm_subitemhittest(a,b,c,d,e,f,g,h,i) test_lvm_subitemhittest_(a,b,c,d,e,f,g,h,i,__LINE__)

static void test_images(void)
{
    HWND hwnd;
    INT r;
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

    r = SendMessage(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
                    LVS_EX_UNDERLINEHOT | LVS_EX_FLATSB | LVS_EX_ONECLICKACTIVATE);

    ok(r == 0, "should return zero\n");

    r = SendMessage(hwnd, LVM_SETIMAGELIST, 0, (LPARAM)himl);
    ok(r == 0, "should return zero\n");

    r = SendMessage(hwnd, LVM_SETICONSPACING, 0, MAKELONG(100,50));
    ok(r != 0, "got 0\n");

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
    expect(1, r);

    r = SendMessage(hwnd, LVM_DELETEALLITEMS, 0, 0);
    ok(r == TRUE, "should not fail\n");

    item.iSubItem = 0;
    item.pszText = hello;
    r = SendMessage(hwnd, LVM_INSERTITEM, 0, (LPARAM) &item);
    ok(r == 0, "should not fail\n");

    memset(&r2, 0, sizeof r2);
    r2.left = LVIR_ICON;
    r = SendMessage(hwnd, LVM_GETITEMRECT, 0, (LPARAM) &r2);
    expect(1, r);

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
    expect(0, r);

    item.iItem = 0;
    item.mask = LVIF_STATE;
    item.stateMask = 0xffff;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(1, r);
    ok(item.state == 0xfccc, "state %x\n", item.state);

    /* Don't set LVIF_STATE */
    item.mask = LVIF_TEXT;
    item.stateMask = 0xffff;
    item.state = 0xfccc;
    item.iItem = 1;
    item.iSubItem = 0;
    item.pszText = text;
    r = SendMessage(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item);
    expect(1, r);

    item.iItem = 1;
    item.mask = LVIF_STATE;
    item.stateMask = 0xffff;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(1, r);
    ok(item.state == 0, "state %x\n", item.state);

    r = SendMessage(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);
    expect(0, r);

    /* Having turned on checkboxes, check that all existing items are set to 0x1000 (unchecked) */
    item.iItem = 0;
    item.mask = LVIF_STATE;
    item.stateMask = 0xffff;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(1, r);
    if (item.state != 0x1ccc)
    {
        win_skip("LVS_EX_CHECKBOXES style is unavailable. Skipping.\n");
        DestroyWindow(hwnd);
        return;
    }

    /* Now add an item without specifying a state and check that its state goes to 0x1000 */
    item.iItem = 2;
    item.mask = LVIF_TEXT;
    item.state = 0;
    item.pszText = text2;
    r = SendMessage(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item);
    expect(2, r);

    item.iItem = 2;
    item.mask = LVIF_STATE;
    item.stateMask = 0xffff;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(1, r);
    ok(item.state == 0x1000, "state %x\n", item.state);

    /* Add a further item this time specifying a state and still its state goes to 0x1000 */
    item.iItem = 3;
    item.mask = LVIF_TEXT | LVIF_STATE;
    item.stateMask = 0xffff;
    item.state = 0x2aaa;
    item.pszText = text3;
    r = SendMessage(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item);
    expect(3, r);

    item.iItem = 3;
    item.mask = LVIF_STATE;
    item.stateMask = 0xffff;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(1, r);
    ok(item.state == 0x1aaa, "state %x\n", item.state);

    /* Set an item's state to checked */
    item.iItem = 3;
    item.mask = LVIF_STATE;
    item.stateMask = 0xf000;
    item.state = 0x2000;
    r = SendMessage(hwnd, LVM_SETITEMA, 0, (LPARAM) &item);
    expect(1, r);

    item.iItem = 3;
    item.mask = LVIF_STATE;
    item.stateMask = 0xffff;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(1, r);
    ok(item.state == 0x2aaa, "state %x\n", item.state);

    /* Check that only the bits we asked for are returned,
     * and that all the others are set to zero
     */
    item.iItem = 3;
    item.mask = LVIF_STATE;
    item.stateMask = 0xf000;
    item.state = 0xffff;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(1, r);
    ok(item.state == 0x2000, "state %x\n", item.state);

    /* Set the style again and check that doesn't change an item's state */
    r = SendMessage(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);
    ok(r == LVS_EX_CHECKBOXES, "ret %x\n", r);

    item.iItem = 3;
    item.mask = LVIF_STATE;
    item.stateMask = 0xffff;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(1, r);
    ok(item.state == 0x2aaa, "state %x\n", item.state);

    /* Unsetting the checkbox extended style doesn't change an item's state */
    r = SendMessage(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_CHECKBOXES, 0);
    ok(r == LVS_EX_CHECKBOXES, "ret %x\n", r);

    item.iItem = 3;
    item.mask = LVIF_STATE;
    item.stateMask = 0xffff;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(1, r);
    ok(item.state == 0x2aaa, "state %x\n", item.state);

    /* Now setting the style again will change an item's state */
    r = SendMessage(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);
    expect(0, r);

    item.iItem = 3;
    item.mask = LVIF_STATE;
    item.stateMask = 0xffff;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(1, r);
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
    expect(1, r);
    ok(item.state == 0x1aab, "state %x\n", item.state);

    r = SendMessage(hwnd, WM_KEYDOWN, VK_SPACE, 0);
    expect(0, r);
    r = SendMessage(hwnd, WM_KEYUP, VK_SPACE, 0);
    expect(0, r);

    item.iItem = 3;
    item.mask = LVIF_STATE;
    item.stateMask = 0xffff;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(1, r);
    ok(item.state == 0x2aab, "state %x\n", item.state);

    r = SendMessage(hwnd, WM_KEYDOWN, VK_SPACE, 0);
    expect(0, r);
    r = SendMessage(hwnd, WM_KEYUP, VK_SPACE, 0);
    expect(0, r);

    item.iItem = 3;
    item.mask = LVIF_STATE;
    item.stateMask = 0xffff;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(1, r);
    ok(item.state == 0x1aab, "state %x\n", item.state);

    DestroyWindow(hwnd);
}

static void insert_column(HWND hwnd, int idx)
{
    LVCOLUMN column;
    INT rc;

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
    INT rc;

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
    static CHAR text[] = "Text";
    char buffA[5];
    HWND hwnd;
    LVITEMA item;
    DWORD r;

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
    expect(0, r);
    /* get */
    memset (&item, 0xcc, sizeof (item));
    item.mask = LVIF_STATE;
    item.stateMask = LVIS_SELECTED;
    item.state = 0;
    item.iItem = 0;
    item.iSubItem = 0;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(1, r);
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
    expect(0, r);
    /* get */
    memset (&item, 0xcc, sizeof (item));
    item.mask = LVIF_STATE;
    item.stateMask = LVIS_FOCUSED;
    item.state = 0;
    item.iItem = 0;
    item.iSubItem = 0;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(1, r);
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
    expect(0, r);
    /* get */
    memset (&item, 0xcc, sizeof (item));
    item.mask = LVIF_STATE;
    item.stateMask = LVIS_CUT;
    item.state = 0;
    item.iItem = 0;
    item.iSubItem = 0;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(1, r);
    ok(item.state & LVIS_CUT, "Expected LVIS_CUT\n");
    SendMessage(hwnd, LVM_DELETEITEM, 0, 0);

    /* Insert an item with just a param */
    memset (&item, 0xcc, sizeof (item));
    item.mask = LVIF_PARAM;
    item.iItem = 0;
    item.iSubItem = 0;
    item.lParam = lparamTest;
    r = SendMessage(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item);
    expect(0, r);

    /* Test getting of the param */
    memset (&item, 0xcc, sizeof (item));
    item.mask = LVIF_PARAM;
    item.iItem = 0;
    item.iSubItem = 0;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(1, r);
    ok(item.lParam == lparamTest, "got lParam %lx, expected %lx\n", item.lParam, lparamTest);

    /* Set up a subitem */
    memset (&item, 0xcc, sizeof (item));
    item.mask = LVIF_TEXT;
    item.iItem = 0;
    item.iSubItem = 1;
    item.pszText = text;
    r = SendMessage(hwnd, LVM_SETITEMA, 0, (LPARAM) &item);
    expect(1, r);

    item.mask = LVIF_TEXT;
    item.iItem = 0;
    item.iSubItem = 1;
    item.pszText = buffA;
    item.cchTextMax = sizeof(buffA);
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(1, r);
    ok(!memcmp(item.pszText, text, sizeof(text)), "got text %s, expected %s\n", item.pszText, text);

    /* set up with extra flag */
    /* 1. reset subitem text */
    item.mask = LVIF_TEXT;
    item.iItem = 0;
    item.iSubItem = 1;
    item.pszText = NULL;
    r = SendMessage(hwnd, LVM_SETITEMA, 0, (LPARAM) &item);
    expect(1, r);

    item.mask = LVIF_TEXT;
    item.iItem = 0;
    item.iSubItem = 1;
    item.pszText = buffA;
    buffA[0] = 'a';
    item.cchTextMax = sizeof(buffA);
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(1, r);
    ok(item.pszText[0] == 0, "got %p\n", item.pszText);

    /* 2. set new text with extra flag specified */
    item.mask = LVIF_TEXT | LVIF_DI_SETITEM;
    item.iItem = 0;
    item.iSubItem = 1;
    item.pszText = text;
    r = SendMessage(hwnd, LVM_SETITEMA, 0, (LPARAM) &item);
    ok(r == 1 || broken(r == 0) /* NT4 */, "ret %d\n", r);

    if (r == 1)
    {
        item.mask = LVIF_TEXT;
        item.iItem = 0;
        item.iSubItem = 1;
        item.pszText = buffA;
        buffA[0] = 'a';
        item.cchTextMax = sizeof(buffA);
        r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
        expect(1, r);
        ok(!memcmp(item.pszText, text, sizeof(text)), "got %s, expected %s\n", item.pszText, text);
    }

    /* Query param from subitem: returns main item param */
    memset (&item, 0xcc, sizeof (item));
    item.mask = LVIF_PARAM;
    item.iItem = 0;
    item.iSubItem = 1;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(1, r);
    ok(item.lParam == lparamTest, "got lParam %lx, expected %lx\n", item.lParam, lparamTest);

    /* Set up param on first subitem: no effect */
    memset (&item, 0xcc, sizeof (item));
    item.mask = LVIF_PARAM;
    item.iItem = 0;
    item.iSubItem = 1;
    item.lParam = lparamTest+1;
    r = SendMessage(hwnd, LVM_SETITEMA, 0, (LPARAM) &item);
    expect(0, r);

    /* Query param from subitem again: should still return main item param */
    memset (&item, 0xcc, sizeof (item));
    item.mask = LVIF_PARAM;
    item.iItem = 0;
    item.iSubItem = 1;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(1, r);
    ok(item.lParam == lparamTest, "got lParam %lx, expected %lx\n", item.lParam, lparamTest);

    /**** Some tests of state highlighting ****/
    memset (&item, 0xcc, sizeof (item));
    item.mask = LVIF_STATE;
    item.iItem = 0;
    item.iSubItem = 0;
    item.state = LVIS_SELECTED;
    item.stateMask = LVIS_SELECTED | LVIS_DROPHILITED;
    r = SendMessage(hwnd, LVM_SETITEM, 0, (LPARAM) &item);
    expect(1, r);
    item.iSubItem = 1;
    item.state = LVIS_DROPHILITED;
    r = SendMessage(hwnd, LVM_SETITEM, 0, (LPARAM) &item);
    expect(1, r);

    memset (&item, 0xcc, sizeof (item));
    item.mask = LVIF_STATE;
    item.iItem = 0;
    item.iSubItem = 0;
    item.stateMask = -1;
    r = SendMessage(hwnd, LVM_GETITEM, 0, (LPARAM) &item);
    expect(1, r);
    ok(item.state == LVIS_SELECTED, "got state %x, expected %x\n", item.state, LVIS_SELECTED);
    item.iSubItem = 1;
    r = SendMessage(hwnd, LVM_GETITEM, 0, (LPARAM) &item);
    expect(1, r);
    todo_wine ok(item.state == LVIS_DROPHILITED, "got state %x, expected %x\n", item.state, LVIS_DROPHILITED);

    /* some notnull but meaningless masks */
    memset (&item, 0, sizeof(item));
    item.mask = LVIF_NORECOMPUTE;
    item.iItem = 0;
    item.iSubItem = 0;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(1, r);
    memset (&item, 0, sizeof(item));
    item.mask = LVIF_DI_SETITEM;
    item.iItem = 0;
    item.iSubItem = 0;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(1, r);

    /* set text to callback value already having it */
    r = SendMessage(hwnd, LVM_DELETEALLITEMS, 0, 0);
    expect(TRUE, r);
    memset (&item, 0, sizeof (item));
    item.mask  = LVIF_TEXT;
    item.pszText = LPSTR_TEXTCALLBACK;
    item.iItem = 0;
    r = SendMessage(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item);
    expect(0, r);
    memset (&item, 0, sizeof (item));

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    item.pszText = LPSTR_TEXTCALLBACK;
    r = SendMessage(hwnd, LVM_SETITEMTEXT, 0 , (LPARAM) &item);
    expect(TRUE, r);

    ok_sequence(sequences, PARENT_SEQ_INDEX, textcallback_set_again_parent_seq,
                "check callback text comparison rule", FALSE);

    DestroyWindow(hwnd);
}

static void test_columns(void)
{
    HWND hwnd;
    LVCOLUMNA column;
    LVITEMA item;
    INT order[2];
    CHAR buff[5];
    DWORD rc;

    hwnd = CreateWindowExA(0, "SysListView32", "foo", LVS_REPORT,
                10, 10, 100, 200, hwndparent, NULL, NULL, NULL);
    ok(hwnd != NULL, "failed to create listview window\n");

    /* Add a column with no mask */
    memset(&column, 0xcc, sizeof(column));
    column.mask = 0;
    rc = SendMessageA(hwnd, LVM_INSERTCOLUMNA, 0, (LPARAM)&column);
    ok(rc == 0, "Inserting column with no mask failed with %d\n", rc);

    /* Check its width */
    rc = SendMessageA(hwnd, LVM_GETCOLUMNWIDTH, 0, 0);
    ok(rc == 10, "Inserting column with no mask failed to set width to 10 with %d\n", rc);

    DestroyWindow(hwnd);

    /* LVM_GETCOLUMNORDERARRAY */
    hwnd = create_listview_control(LVS_REPORT);
    subclass_header(hwnd);

    memset(&column, 0, sizeof(column));
    column.mask = LVCF_WIDTH;
    column.cx = 100;
    rc = SendMessageA(hwnd, LVM_INSERTCOLUMNA, 0, (LPARAM)&column);
    expect(0, rc);

    column.cx = 200;
    rc = SendMessageA(hwnd, LVM_INSERTCOLUMNA, 1, (LPARAM)&column);
    expect(1, rc);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    rc = SendMessageA(hwnd, LVM_GETCOLUMNORDERARRAY, 2, (LPARAM)&order);
    expect(1, rc);
    ok(order[0] == 0, "Expected order 0, got %d\n", order[0]);
    ok(order[1] == 1, "Expected order 1, got %d\n", order[1]);

    ok_sequence(sequences, LISTVIEW_SEQ_INDEX, listview_getorderarray_seq, "get order array", FALSE);

    /* after column added subitem is considered as present */
    insert_item(hwnd, 0);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    item.pszText = buff;
    item.cchTextMax = sizeof(buff);
    item.iItem = 0;
    item.iSubItem = 1;
    item.mask = LVIF_TEXT;
    memset(&g_itema, 0, sizeof(g_itema));
    rc = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM)&item);
    expect(1, rc);
    ok(g_itema.iSubItem == 1, "got %d\n", g_itema.iSubItem);

    ok_sequence(sequences, PARENT_SEQ_INDEX, single_getdispinfo_parent_seq,
        "get subitem text after column added", FALSE);

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
    DWORD style;

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

    if (!IsWindow(hHeader))
    {
        /* version 4.0 */
        win_skip("LVM_GETHEADER not implemented. Skipping.\n");
        DestroyWindow(hList);
        return;
    }

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
    expect(0, r);
    hHeader = (HWND)SendMessage(hList, LVM_GETHEADER, 0, 0);
    ok(IsWindow(hHeader), "Header should be created\n");
    ok(hHeader == GetDlgItem(hList, 0), "Expected header as dialog item\n");
    style = GetWindowLong(hHeader, GWL_STYLE);
    ok(!(style & HDS_HIDDEN), "Not expected HDS_HIDDEN\n");
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
    expect(0, r);
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
    expect(0, r);
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
    ok(IsWindow(hHeader) ||
       broken(!IsWindow(hHeader)), /* 4.7x common controls */
       "Header should be created\n");
    ok(hHeader == GetDlgItem(hList, 0), "Expected header as dialog item\n");
    DestroyWindow(hList);

    /* setting LVS_EX_GRIDLINES creates header */
    hList = CreateWindow("SysListView32", "Test", LVS_REPORT, 0, 0, 100, 100, NULL, NULL,
                          GetModuleHandle(NULL), 0);
    hHeader = (HWND)SendMessage(hList, LVM_GETHEADER, 0, 0);
    ok(!IsWindow(hHeader), "Header shouldn't be created\n");
    ok(NULL == GetDlgItem(hList, 0), "NULL dialog item expected\n");
    SendMessage(hList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_GRIDLINES);
    hHeader = (HWND)SendMessage(hList, LVM_GETHEADER, 0, 0);
    ok(IsWindow(hHeader) ||
       broken(!IsWindow(hHeader)), /* 4.7x common controls */
       "Header should be created\n");
    ok(hHeader == GetDlgItem(hList, 0), "Expected header as dialog item\n");
    DestroyWindow(hList);

    /* setting LVS_EX_FULLROWSELECT creates header */
    hList = CreateWindow("SysListView32", "Test", LVS_REPORT, 0, 0, 100, 100, NULL, NULL,
                          GetModuleHandle(NULL), 0);
    hHeader = (HWND)SendMessage(hList, LVM_GETHEADER, 0, 0);
    ok(!IsWindow(hHeader), "Header shouldn't be created\n");
    ok(NULL == GetDlgItem(hList, 0), "NULL dialog item expected\n");
    SendMessage(hList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);
    hHeader = (HWND)SendMessage(hList, LVM_GETHEADER, 0, 0);
    ok(IsWindow(hHeader) ||
       broken(!IsWindow(hHeader)), /* 4.7x common controls */
       "Header should be created\n");
    ok(hHeader == GetDlgItem(hList, 0), "Expected header as dialog item\n");
    DestroyWindow(hList);

    /* not report style accepts LVS_EX_HEADERDRAGDROP too */
    hList = create_listview_control(LVS_ICON);
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
    expect(1, r);

    hHeader = (HWND)SendMessage(hList, LVM_GETHEADER, 0, 0);
    ok(!IsWindow(hHeader), "Header shouldn't be created\n");
    ok(GetDlgItem(hList, 0) == NULL, "NULL dialog item expected\n");

    DestroyWindow(hList);

    /* WM_MEASUREITEM should be sent when created with LVS_OWNERDRAWFIXED */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    hList = create_listview_control(LVS_OWNERDRAWFIXED | LVS_REPORT);
    ok_sequence(sequences, PARENT_SEQ_INDEX, create_ownerdrawfixed_parent_seq,
                "created with LVS_OWNERDRAWFIXED|LVS_REPORT - parent seq", FALSE);
    DestroyWindow(hList);
}

static void test_redraw(void)
{
    HWND hwnd;
    HDC hdc;
    BOOL res;
    DWORD r;

    hwnd = create_listview_control(LVS_REPORT);
    subclass_header(hwnd);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

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
    ok(r == 1, "Expected not zero result\n");
    ok_sequence(sequences, PARENT_FULL_SEQ_INDEX, forward_erasebkgnd_parent_seq,
                "forward WM_ERASEBKGND on CLR_NONE", FALSE);

    res = ListView_SetBkColor(hwnd, CLR_DEFAULT);
    expect(TRUE, res);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    r = SendMessageA(hwnd, WM_ERASEBKGND, (WPARAM)hdc, 0);
    expect(1, r);
    ok_sequence(sequences, PARENT_FULL_SEQ_INDEX, empty_seq,
                "don't forward WM_ERASEBKGND on non-CLR_NONE", FALSE);

    /* 2. With backbuffer */
    SendMessageA(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_DOUBLEBUFFER,
                                                     LVS_EX_DOUBLEBUFFER);
    res = ListView_SetBkColor(hwnd, CLR_NONE);
    expect(TRUE, res);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    r = SendMessageA(hwnd, WM_ERASEBKGND, (WPARAM)hdc, 0);
    expect(1, r);
    ok_sequence(sequences, PARENT_FULL_SEQ_INDEX, forward_erasebkgnd_parent_seq,
                "forward WM_ERASEBKGND on CLR_NONE", FALSE);

    res = ListView_SetBkColor(hwnd, CLR_DEFAULT);
    expect(TRUE, res);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    r = SendMessageA(hwnd, WM_ERASEBKGND, (WPARAM)hdc, 0);
    todo_wine expect(1, r);
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

    hwnd = create_listview_control(LVS_REPORT);

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
    INT r;

    hwnd = create_listview_control(LVS_ICON);
    ok(hwnd != NULL, "failed to create a listview window\n");

    r = SendMessage(hwnd, WM_NOTIFYFORMAT, (WPARAM)hwndparent, NF_REQUERY);
    expect(NFR_ANSI, r);

    /* reset the icon spacing to defaults */
    SendMessage(hwnd, LVM_SETICONSPACING, 0, MAKELPARAM(-1, -1));

    /* now we can request what the defaults are */
    r = SendMessage(hwnd, LVM_SETICONSPACING, 0, MAKELPARAM(-1, -1));
    w = LOWORD(r);
    h = HIWORD(r);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    r = SendMessage(hwnd, LVM_SETICONSPACING, 0, MAKELPARAM(20, 30));
    ok(r == MAKELONG(w, h) ||
       broken(r == MAKELONG(w, w)), /* win98 */
       "Expected %d, got %d\n", MAKELONG(w, h), r);

    r = SendMessage(hwnd, LVM_SETICONSPACING, 0, MAKELPARAM(25, 35));
    if (r == 0)
    {
        /* version 4.0 */
        win_skip("LVM_SETICONSPACING unimplemented. Skipping.\n");
        DestroyWindow(hwnd);
        return;
    }
    expect(MAKELONG(20,30), r);

    r = SendMessage(hwnd, LVM_SETICONSPACING, 0, MAKELPARAM(-1,-1));
    expect(MAKELONG(25,35), r);

    ok_sequence(sequences, LISTVIEW_SEQ_INDEX, listview_icon_spacing_seq, "test icon spacing seq", FALSE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    DestroyWindow(hwnd);
}

static void test_color(void)
{
    RECT rect;
    HWND hwnd;
    DWORD r;
    int i;

    COLORREF color;
    COLORREF colors[4] = {RGB(0,0,0), RGB(100,50,200), CLR_NONE, RGB(255,255,255)};

    hwnd = create_listview_control(LVS_REPORT);
    ok(hwnd != NULL, "failed to create a listview window\n");

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    for (i = 0; i < 4; i++)
    {
        color = colors[i];

        r = SendMessage(hwnd, LVM_SETBKCOLOR, 0, color);
        expect(TRUE, r);
        r = SendMessage(hwnd, LVM_GETBKCOLOR, 0, 0);
        expect(color, r);

        r = SendMessage(hwnd, LVM_SETTEXTCOLOR, 0, color);
        expect (TRUE, r);
        r = SendMessage(hwnd, LVM_GETTEXTCOLOR, 0, 0);
        expect(color, r);

        r = SendMessage(hwnd, LVM_SETTEXTBKCOLOR, 0, color);
        expect(TRUE, r);
        r = SendMessage(hwnd, LVM_GETTEXTBKCOLOR, 0, 0);
        expect(color, r);
    }

    ok_sequence(sequences, LISTVIEW_SEQ_INDEX, listview_color_seq, "test color seq", FALSE);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    /* invalidation test done separately to avoid a message chain mess */
    r = ValidateRect(hwnd, NULL);
    expect(TRUE, r);
    r = SendMessage(hwnd, LVM_SETBKCOLOR, 0, colors[0]);
    expect(TRUE, r);

    rect.right = rect.bottom = 1;
    r = GetUpdateRect(hwnd, &rect, TRUE);
    todo_wine expect(FALSE, r);
    ok(rect.right == 0 && rect.bottom == 0, "got update rectangle\n");

    r = ValidateRect(hwnd, NULL);
    expect(TRUE, r);
    r = SendMessage(hwnd, LVM_SETTEXTCOLOR, 0, colors[0]);
    expect(TRUE, r);

    rect.right = rect.bottom = 1;
    r = GetUpdateRect(hwnd, &rect, TRUE);
    todo_wine expect(FALSE, r);
    ok(rect.right == 0 && rect.bottom == 0, "got update rectangle\n");

    r = ValidateRect(hwnd, NULL);
    expect(TRUE, r);
    r = SendMessage(hwnd, LVM_SETTEXTBKCOLOR, 0, colors[0]);
    expect(TRUE, r);

    rect.right = rect.bottom = 1;
    r = GetUpdateRect(hwnd, &rect, TRUE);
    todo_wine expect(FALSE, r);
    ok(rect.right == 0 && rect.bottom == 0, "got update rectangle\n");

    DestroyWindow(hwnd);
}

static void test_item_count(void)
{
    /* LVM_INSERTITEM, LVM_DELETEITEM, LVM_DELETEALLITEMS, LVM_GETITEMCOUNT */

    HWND hwnd;
    DWORD r;
    HDC hdc;
    HFONT hOldFont;
    TEXTMETRICA tm;
    RECT rect;
    INT height;

    LVITEM item0;
    LVITEM item1;
    LVITEM item2;
    static CHAR item0text[] = "item0";
    static CHAR item1text[] = "item1";
    static CHAR item2text[] = "item2";

    hwnd = create_listview_control(LVS_REPORT);
    ok(hwnd != NULL, "failed to create a listview window\n");

    /* resize in dpiaware manner to fit all 3 items added */
    hdc = GetDC(0);
    hOldFont = SelectObject(hdc, GetStockObject(SYSTEM_FONT));
    GetTextMetricsA(hdc, &tm);
    /* 2 extra pixels for bounds and header border */
    height = tm.tmHeight + 2;
    SelectObject(hdc, hOldFont);
    ReleaseDC(0, hdc);

    GetWindowRect(hwnd, &rect);
    /* 3 items + 1 header + 1 to be sure */
    MoveWindow(hwnd, 0, 0, rect.right - rect.left, 5 * height, FALSE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

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

    hwnd = create_listview_control(LVS_ICON);
    ok(hwnd != NULL, "failed to create a listview window\n");

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

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

    hwnd = create_listview_control(LVS_ICON);
    ok(hwnd != NULL, "failed to create a listview window\n");
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    r = SendMessage(hwnd, LVM_GETORIGIN, 0, (LPARAM)&position);
    expect(TRUE, r);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    DestroyWindow(hwnd);

    hwnd = create_listview_control(LVS_SMALLICON);
    ok(hwnd != NULL, "failed to create a listview window\n");
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    r = SendMessage(hwnd, LVM_GETORIGIN, 0, (LPARAM)&position);
    expect(TRUE, r);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    DestroyWindow(hwnd);

    hwnd = create_listview_control(LVS_LIST);
    ok(hwnd != NULL, "failed to create a listview window\n");
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    r = SendMessage(hwnd, LVM_GETORIGIN, 0, (LPARAM)&position);
    expect(FALSE, r);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    DestroyWindow(hwnd);

    hwnd = create_listview_control(LVS_REPORT);
    ok(hwnd != NULL, "failed to create a listview window\n");
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

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
    INT r;
    int i,j,item_count,selected_count;
    static const int items=5;
    BYTE kstate[256];
    select_task task;
    LONG_PTR style;
    LVITEMA item;

    static struct t_select_task task_list[] = {
        { "using VK_DOWN", 0, VK_DOWN, -1, -1 },
        { "using VK_UP", -1, VK_UP, -1, -1 },
        { "using VK_END", 0, VK_END, 1, -1 },
        { "using VK_HOME", -1, VK_HOME, 1, -1 }
    };

    hwnd = create_listview_control(LVS_REPORT);

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
    hwnd = create_listview_control(LVS_REPORT);
    for (i=0;i<items;i++) {
	    insert_item(hwnd, 0);
    }
    item_count = (int)SendMessage(hwnd, LVM_GETITEMCOUNT, 0, 0);
    expect(items,item_count);

    /* try with NULL pointer */
    r = SendMessageA(hwnd, LVM_SETITEMSTATE, 0, 0);
    expect(FALSE, r);

    /* select all, check notifications */
    ListView_SetItemState(hwnd, -1, 0, LVIS_SELECTED);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    item.stateMask = LVIS_SELECTED;
    item.state     = LVIS_SELECTED;
    r = SendMessageA(hwnd, LVM_SETITEMSTATE, -1, (LPARAM)&item);
    expect(TRUE, r);

    ok_sequence(sequences, PARENT_SEQ_INDEX, select_all_parent_seq,
                "select all notification", FALSE);

    /* deselect all items */
    ListView_SetItemState(hwnd, -1, 0, LVIS_SELECTED);
    SendMessage(hwnd, LVM_SETSELECTIONMARK, 0, -1);
    for (i=0;i<3;i++) {
        ListView_SetItemState(hwnd, i, LVIS_SELECTED, LVIS_SELECTED);
    }

    r = SendMessage(hwnd, LVM_GETSELECTEDCOUNT, 0, 0);
    expect(3, r);
    r = SendMessage(hwnd, LVM_GETSELECTIONMARK, 0, 0);
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
    expect(-1, r);

    /* try to select all on LVS_SINGLESEL */
    memset(&item, 0, sizeof(item));
    item.stateMask = LVIS_SELECTED;
    r = SendMessage(hwnd, LVM_SETITEMSTATE, -1, (LPARAM)&item);
    expect(TRUE, r);
    SendMessage(hwnd, LVM_SETSELECTIONMARK, 0, -1);

    item.stateMask = LVIS_SELECTED;
    item.state     = LVIS_SELECTED;
    r = SendMessage(hwnd, LVM_SETITEMSTATE, -1, (LPARAM)&item);
    expect(FALSE, r);

    r = ListView_GetSelectedCount(hwnd);
    expect(0, r);
    r = ListView_GetSelectionMark(hwnd);
    expect(-1, r);

    /* try to deselect all on LVS_SINGLESEL */
    item.stateMask = LVIS_SELECTED;
    item.state     = LVIS_SELECTED;
    r = SendMessage(hwnd, LVM_SETITEMSTATE, 0, (LPARAM)&item);
    expect(TRUE, r);

    item.stateMask = LVIS_SELECTED;
    item.state     = 0;
    r = SendMessage(hwnd, LVM_SETITEMSTATE, -1, (LPARAM)&item);
    expect(TRUE, r);
    r = ListView_GetSelectedCount(hwnd);
    expect(0, r);

    /* 1. selection mark is update when new focused item is set */
    style = GetWindowLongPtrA(hwnd, GWL_STYLE);
    SetWindowLongPtrA(hwnd, GWL_STYLE, style & ~LVS_SINGLESEL);

    r = SendMessage(hwnd, LVM_SETSELECTIONMARK, 0, -1);
    expect(-1, r);

    item.stateMask = LVIS_FOCUSED;
    item.state     = LVIS_FOCUSED;
    r = SendMessage(hwnd, LVM_SETITEMSTATE, 0, (LPARAM)&item);
    expect(TRUE, r);

    r = SendMessage(hwnd, LVM_GETSELECTIONMARK, 0, 0);
    expect(0, r);

    /* it's not updated if already set */
    item.stateMask = LVIS_FOCUSED;
    item.state     = LVIS_FOCUSED;
    r = SendMessage(hwnd, LVM_SETITEMSTATE, 1, (LPARAM)&item);
    expect(TRUE, r);

    r = SendMessage(hwnd, LVM_GETSELECTIONMARK, 0, 0);
    expect(0, r);

    r = SendMessage(hwnd, LVM_SETSELECTIONMARK, 0, -1);
    expect(0, r);

    item.stateMask = LVIS_FOCUSED;
    item.state     = LVIS_FOCUSED;
    r = SendMessage(hwnd, LVM_SETITEMSTATE, 1, (LPARAM)&item);
    expect(TRUE, r);

    r = SendMessage(hwnd, LVM_GETSELECTIONMARK, 0, 0);
    expect(-1, r);

    /* need to reset focused item first */
    item.stateMask = LVIS_FOCUSED;
    item.state     = 0;
    r = SendMessage(hwnd, LVM_SETITEMSTATE, -1, (LPARAM)&item);
    expect(TRUE, r);

    item.stateMask = LVIS_FOCUSED;
    item.state     = LVIS_FOCUSED;
    r = SendMessage(hwnd, LVM_SETITEMSTATE, 2, (LPARAM)&item);
    expect(TRUE, r);

    r = SendMessage(hwnd, LVM_GETSELECTIONMARK, 0, 0);
    expect(2, r);

    item.stateMask = LVIS_FOCUSED;
    item.state     = 0;
    r = SendMessage(hwnd, LVM_SETITEMSTATE, -1, (LPARAM)&item);
    expect(TRUE, r);

    r = SendMessage(hwnd, LVM_GETSELECTIONMARK, 0, 0);
    expect(2, r);

    /* 2. same tests, with LVM_SETITEM */
    style = GetWindowLongPtrA(hwnd, GWL_STYLE);
    SetWindowLongPtrA(hwnd, GWL_STYLE, style & ~LVS_SINGLESEL);

    r = SendMessage(hwnd, LVM_SETSELECTIONMARK, 0, -1);
    expect(2, r);

    item.stateMask = LVIS_FOCUSED;
    item.state     = LVIS_FOCUSED;
    item.mask      = LVIF_STATE;
    item.iItem = item.iSubItem = 0;
    r = SendMessage(hwnd, LVM_SETITEMA, 0, (LPARAM)&item);
    expect(TRUE, r);

    r = SendMessage(hwnd, LVM_GETSELECTIONMARK, 0, 0);
    expect(0, r);

    /* it's not updated if already set */
    item.stateMask = LVIS_FOCUSED;
    item.state     = LVIS_FOCUSED;
    item.mask      = LVIF_STATE;
    item.iItem     = 1;
    item.iSubItem  = 0;
    r = SendMessage(hwnd, LVM_SETITEMA, 0, (LPARAM)&item);
    expect(TRUE, r);

    r = SendMessage(hwnd, LVM_GETSELECTIONMARK, 0, 0);
    expect(0, r);

    r = SendMessage(hwnd, LVM_SETSELECTIONMARK, 0, -1);
    expect(0, r);

    item.stateMask = LVIS_FOCUSED;
    item.state     = LVIS_FOCUSED;
    item.mask      = LVIF_STATE;
    item.iItem     = 1;
    item.iSubItem  = 0;
    r = SendMessage(hwnd, LVM_SETITEMA, 0, (LPARAM)&item);
    expect(TRUE, r);

    r = SendMessage(hwnd, LVM_GETSELECTIONMARK, 0, 0);
    expect(-1, r);

    /* need to reset focused item first */
    item.stateMask = LVIS_FOCUSED;
    item.state     = 0;
    r = SendMessage(hwnd, LVM_SETITEMSTATE, -1, (LPARAM)&item);
    expect(TRUE, r);

    item.stateMask = LVIS_FOCUSED;
    item.state     = LVIS_FOCUSED;
    item.mask      = LVIF_STATE;
    item.iItem     = 2;
    item.iSubItem  = 0;
    r = SendMessage(hwnd, LVM_SETITEMA, 0, (LPARAM)&item);
    expect(TRUE, r);

    r = SendMessage(hwnd, LVM_GETSELECTIONMARK, 0, 0);
    expect(2, r);

    item.stateMask = LVIS_FOCUSED;
    item.state     = 0;
    r = SendMessage(hwnd, LVM_SETITEMSTATE, -1, (LPARAM)&item);
    expect(TRUE, r);

    r = SendMessage(hwnd, LVM_GETSELECTIONMARK, 0, 0);
    expect(2, r);

    DestroyWindow(hwnd);
}

static void test_subitem_rect(void)
{
    HWND hwnd;
    DWORD r;
    LVCOLUMN col;
    RECT rect, rect2;
    INT arr[3];

    /* test LVM_GETSUBITEMRECT for header */
    hwnd = create_listview_control(LVS_REPORT);
    ok(hwnd != NULL, "failed to create a listview window\n");
    /* add some columns */
    memset(&col, 0, sizeof(LVCOLUMN));
    col.mask = LVCF_WIDTH;
    col.cx = 100;
    r = SendMessage(hwnd, LVM_INSERTCOLUMN, 0, (LPARAM)&col);
    expect(0, r);
    col.cx = 150;
    r = SendMessage(hwnd, LVM_INSERTCOLUMN, 1, (LPARAM)&col);
    expect(1, r);
    col.cx = 200;
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
    expect(1, r);

    expect(100, rect.left);
    expect(250, rect.right);
todo_wine
    expect(3, rect.top);

    rect.left = LVIR_BOUNDS;
    rect.top  = 2;
    rect.right = rect.bottom = 0;
    r = SendMessage(hwnd, LVM_GETSUBITEMRECT, -1, (LPARAM)&rect);
    expect(1, r);

    expect(250, rect.left);
    expect(450, rect.right);
todo_wine
    expect(3, rect.top);

    /* item LVS_REPORT padding isn't applied to subitems */
    insert_item(hwnd, 0);

    rect.left = LVIR_BOUNDS;
    rect.top  = 1;
    rect.right = rect.bottom = 0;
    r = SendMessage(hwnd, LVM_GETSUBITEMRECT, 0, (LPARAM)&rect);
    expect(1, r);
    expect(100, rect.left);
    expect(250, rect.right);

    rect.left = LVIR_ICON;
    rect.top  = 1;
    rect.right = rect.bottom = 0;
    r = SendMessage(hwnd, LVM_GETSUBITEMRECT, 0, (LPARAM)&rect);
    expect(1, r);
    /* no icon attached - zero width rectangle, with no left padding */
    expect(100, rect.left);
    expect(100, rect.right);

    rect.left = LVIR_LABEL;
    rect.top  = 1;
    rect.right = rect.bottom = 0;
    r = SendMessage(hwnd, LVM_GETSUBITEMRECT, 0, (LPARAM)&rect);
    expect(1, r);
    /* same as full LVIR_BOUNDS */
    expect(100, rect.left);
    expect(250, rect.right);

    SendMessage(hwnd, LVM_SCROLL, 10, 0);

    rect.left = LVIR_BOUNDS;
    rect.top  = 1;
    rect.right = rect.bottom = 0;
    r = SendMessage(hwnd, LVM_GETSUBITEMRECT, 0, (LPARAM)&rect);
    expect(1, r);
    expect(90, rect.left);
    expect(240, rect.right);

    SendMessage(hwnd, LVM_SCROLL, -10, 0);

    DestroyWindow(hwnd);

    /* test subitem rects after re-arranging columns */
    hwnd = create_listview_control(LVS_REPORT);
    ok(hwnd != NULL, "failed to create a listview window\n");
    memset(&col, 0, sizeof(LVCOLUMN));
    col.mask = LVCF_WIDTH;

    col.cx = 100;
    r = SendMessage(hwnd, LVM_INSERTCOLUMN, 0, (LPARAM)&col);
    expect(0, r);

    col.cx = 200;
    r = SendMessage(hwnd, LVM_INSERTCOLUMN, 1, (LPARAM)&col);
    expect(1, r);

    col.cx = 300;
    r = SendMessage(hwnd, LVM_INSERTCOLUMN, 2, (LPARAM)&col);
    expect(2, r);

    insert_item(hwnd, 0);
    insert_item(hwnd, 1);

    /* wrong item is refused for main item */
    rect.left = LVIR_BOUNDS;
    rect.top  = 0;
    rect.right = rect.bottom = -1;
    r = SendMessage(hwnd, LVM_GETSUBITEMRECT, 2, (LPARAM)&rect);
    expect(FALSE, r);

    /* for subitems rectangle is calculated even if there's no item added */
    rect.left = LVIR_BOUNDS;
    rect.top  = 1;
    rect.right = rect.bottom = -1;
    r = SendMessage(hwnd, LVM_GETSUBITEMRECT, 1, (LPARAM)&rect);
    expect(TRUE, r);

    rect2.left = LVIR_BOUNDS;
    rect2.top  = 1;
    rect2.right = rect2.bottom = -1;
    r = SendMessage(hwnd, LVM_GETSUBITEMRECT, 2, (LPARAM)&rect2);
todo_wine {
    expect(TRUE, r);
    expect(rect.right, rect2.right);
    expect(rect.left, rect2.left);
    expect(rect.bottom, rect2.top);
    ok(rect2.bottom > rect2.top, "expected not zero height\n");
}

    arr[0] = 1; arr[1] = 0; arr[2] = 2;
    r = SendMessage(hwnd, LVM_SETCOLUMNORDERARRAY, 3, (LPARAM)arr);
    expect(TRUE, r);

    rect.left = LVIR_BOUNDS;
    rect.top  = 0;
    rect.right = rect.bottom = -1;
    r = SendMessage(hwnd, LVM_GETSUBITEMRECT, 0, (LPARAM)&rect);
    expect(TRUE, r);
    expect(0, rect.left);
    expect(600, rect.right);

    rect.left = LVIR_BOUNDS;
    rect.top  = 1;
    rect.right = rect.bottom = -1;
    r = SendMessage(hwnd, LVM_GETSUBITEMRECT, 0, (LPARAM)&rect);
    expect(TRUE, r);
    expect(0, rect.left);
    expect(200, rect.right);

    rect2.left = LVIR_BOUNDS;
    rect2.top  = 1;
    rect2.right = rect2.bottom = -1;
    r = SendMessage(hwnd, LVM_GETSUBITEMRECT, 1, (LPARAM)&rect2);
    expect(TRUE, r);
    expect(0, rect2.left);
    expect(200, rect2.right);
    /* items are of the same height */
    ok(rect2.top > 0, "expected positive item height\n");
    expect(rect.bottom, rect2.top);
    expect(rect.bottom * 2 - rect.top, rect2.bottom);

    rect.left = LVIR_BOUNDS;
    rect.top  = 2;
    rect.right = rect.bottom = -1;
    r = SendMessage(hwnd, LVM_GETSUBITEMRECT, 0, (LPARAM)&rect);
    expect(TRUE, r);
    expect(300, rect.left);
    expect(600, rect.right);

    DestroyWindow(hwnd);

    /* try it for non LVS_REPORT style */
    hwnd = CreateWindow("SysListView32", "Test", LVS_ICON, 0, 0, 100, 100, NULL, NULL,
                         GetModuleHandle(NULL), 0);
    rect.left = LVIR_BOUNDS;
    rect.top  = 1;
    rect.right = rect.bottom = -10;
    r = SendMessage(hwnd, LVM_GETSUBITEMRECT, -1, (LPARAM)&rect);
    expect(0, r);
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
    INT r;
    LONG_PTR style;
    static CHAR names[][5] = {"A", "B", "C", "D", "0"};
    CHAR buff[10];

    hwnd = create_listview_control(LVS_REPORT);
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
    hwnd = create_listview_control(LVS_REPORT);
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
    static char test_str[] = "test";

    HWND hwnd;
    LONG_PTR style, ret;
    DWORD res;
    LVITEMA item;

    /* it isn't possible to set LVS_OWNERDATA after creation */
    if (g_is_below_5)
    {
        win_skip("set LVS_OWNERDATA after creation leads to crash on < 5.80\n");
    }
    else
    {
        hwnd = create_listview_control(LVS_REPORT);
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
    }

    /* try to set LVS_OWNERDATA after creation just having it */
    hwnd = create_listview_control(LVS_OWNERDATA | LVS_REPORT);
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
    if (g_is_below_5)
    {
        win_skip("remove LVS_OWNERDATA after creation leads to crash on < 5.80\n");
    }
    else
    {
        hwnd = create_listview_control(LVS_OWNERDATA | LVS_REPORT);
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
    }

    /* try select an item */
    hwnd = create_listview_control(LVS_OWNERDATA | LVS_REPORT);
    ok(hwnd != NULL, "failed to create a listview window\n");
    res = SendMessageA(hwnd, LVM_SETITEMCOUNT, 1, 0);
    expect(1, res);
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

    /* LVM_SETITEM and LVM_SETITEMTEXT is unsupported on LVS_OWNERDATA */
    hwnd = create_listview_control(LVS_OWNERDATA | LVS_REPORT);
    ok(hwnd != NULL, "failed to create a listview window\n");
    res = SendMessageA(hwnd, LVM_SETITEMCOUNT, 1, 0);
    expect(1, res);
    res = SendMessageA(hwnd, LVM_GETITEMCOUNT, 0, 0);
    expect(1, res);
    memset(&item, 0, sizeof(item));
    item.mask = LVIF_STATE;
    item.iItem = 0;
    item.stateMask = LVIS_SELECTED;
    item.state     = LVIS_SELECTED;
    res = SendMessageA(hwnd, LVM_SETITEM, 0, (LPARAM)&item);
    expect(FALSE, res);
    memset(&item, 0, sizeof(item));
    item.pszText = test_str;
    res = SendMessageA(hwnd, LVM_SETITEMTEXT, 0, (LPARAM)&item);
    expect(FALSE, res);
    DestroyWindow(hwnd);

    /* check notifications after focused/selected changed */
    hwnd = create_listview_control(LVS_OWNERDATA | LVS_REPORT);
    ok(hwnd != NULL, "failed to create a listview window\n");
    res = SendMessageA(hwnd, LVM_SETITEMCOUNT, 20, 0);
    expect(1, res);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    memset(&item, 0, sizeof(item));
    item.stateMask = LVIS_SELECTED;
    item.state     = LVIS_SELECTED;
    res = SendMessageA(hwnd, LVM_SETITEMSTATE, 0, (LPARAM)&item);
    expect(TRUE, res);

    ok_sequence(sequences, PARENT_SEQ_INDEX, ownderdata_select_focus_parent_seq,
                "ownerdata select notification", TRUE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    memset(&item, 0, sizeof(item));
    item.stateMask = LVIS_FOCUSED;
    item.state     = LVIS_FOCUSED;
    res = SendMessageA(hwnd, LVM_SETITEMSTATE, 0, (LPARAM)&item);
    expect(TRUE, res);

    ok_sequence(sequences, PARENT_SEQ_INDEX, ownderdata_select_focus_parent_seq,
                "ownerdata focus notification", TRUE);

    /* select all, check notifications */
    item.stateMask = LVIS_SELECTED;
    item.state     = 0;
    res = SendMessageA(hwnd, LVM_SETITEMSTATE, -1, (LPARAM)&item);
    expect(TRUE, res);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    item.stateMask = LVIS_SELECTED;
    item.state     = LVIS_SELECTED;

    memset(&g_nmlistview, 0xcc, sizeof(g_nmlistview));
    res = SendMessageA(hwnd, LVM_SETITEMSTATE, -1, (LPARAM)&item);
    expect(TRUE, res);
    ok(g_nmlistview.iItem == -1, "got item %d\n", g_nmlistview.iItem);
    ok(g_nmlistview.iSubItem == 0, "got subitem %d\n", g_nmlistview.iSubItem);
    ok(g_nmlistview.uNewState == LVIS_SELECTED, "got new state 0x%08x\n", g_nmlistview.uNewState);
    ok(g_nmlistview.uOldState == 0, "got old state 0x%08x\n", g_nmlistview.uOldState);
    ok(g_nmlistview.uChanged == LVIF_STATE, "got changed 0x%08x\n", g_nmlistview.uChanged);
    ok(g_nmlistview.ptAction.x == 0 && g_nmlistview.ptAction.y == 0, "got wrong ptAction value\n");
    ok(g_nmlistview.lParam == 0, "got wrong lparam\n");

    ok_sequence(sequences, PARENT_SEQ_INDEX, ownerdata_setstate_all_parent_seq,
                "ownerdata select all notification", FALSE);

    /* select all again, note that all items are selected already */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    item.stateMask = LVIS_SELECTED;
    item.state     = LVIS_SELECTED;

    memset(&g_nmlistview, 0xcc, sizeof(g_nmlistview));
    res = SendMessageA(hwnd, LVM_SETITEMSTATE, -1, (LPARAM)&item);
    expect(TRUE, res);
    ok(g_nmlistview.iItem == -1, "got item %d\n", g_nmlistview.iItem);
    ok(g_nmlistview.iSubItem == 0, "got subitem %d\n", g_nmlistview.iSubItem);
    ok(g_nmlistview.uNewState == LVIS_SELECTED, "got new state 0x%08x\n", g_nmlistview.uNewState);
    ok(g_nmlistview.uOldState == 0, "got old state 0x%08x\n", g_nmlistview.uOldState);
    ok(g_nmlistview.uChanged == LVIF_STATE, "got changed 0x%08x\n", g_nmlistview.uChanged);
    ok(g_nmlistview.ptAction.x == 0 && g_nmlistview.ptAction.y == 0, "got wrong ptAction value\n");
    ok(g_nmlistview.lParam == 0, "got wrong lparam\n");

    ok_sequence(sequences, PARENT_SEQ_INDEX, ownerdata_setstate_all_parent_seq,
                "ownerdata select all notification", FALSE);

    /* deselect all */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    item.stateMask = LVIS_SELECTED;
    item.state     = 0;

    memset(&g_nmlistview, 0xcc, sizeof(g_nmlistview));
    res = SendMessageA(hwnd, LVM_SETITEMSTATE, -1, (LPARAM)&item);
    expect(TRUE, res);
    ok(g_nmlistview.iItem == -1, "got item %d\n", g_nmlistview.iItem);
    ok(g_nmlistview.iSubItem == 0, "got subitem %d\n", g_nmlistview.iSubItem);
    ok(g_nmlistview.uNewState == 0, "got new state 0x%08x\n", g_nmlistview.uNewState);
    ok(g_nmlistview.uOldState == LVIS_SELECTED, "got old state 0x%08x\n", g_nmlistview.uOldState);
    ok(g_nmlistview.uChanged == LVIF_STATE, "got changed 0x%08x\n", g_nmlistview.uChanged);
    ok(g_nmlistview.ptAction.x == 0 && g_nmlistview.ptAction.y == 0, "got wrong ptAction value\n");
    ok(g_nmlistview.lParam == 0, "got wrong lparam\n");

    ok_sequence(sequences, PARENT_SEQ_INDEX, ownerdata_deselect_all_parent_seq,
                "ownerdata deselect all notification", TRUE);

    /* nothing selected, deselect all again */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    item.stateMask = LVIS_SELECTED;
    item.state     = 0;

    res = SendMessageA(hwnd, LVM_SETITEMSTATE, -1, (LPARAM)&item);
    expect(TRUE, res);

    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq, "ownerdata deselect all notification", TRUE);

    /* select one, then deselect all */
    item.stateMask = LVIS_SELECTED;
    item.state     = LVIS_SELECTED;
    res = SendMessageA(hwnd, LVM_SETITEMSTATE, 0, (LPARAM)&item);
    expect(TRUE, res);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    item.stateMask = LVIS_SELECTED;
    item.state     = 0;

    memset(&g_nmlistview, 0xcc, sizeof(g_nmlistview));
    res = SendMessageA(hwnd, LVM_SETITEMSTATE, -1, (LPARAM)&item);
    expect(TRUE, res);
    ok(g_nmlistview.iItem == -1, "got item %d\n", g_nmlistview.iItem);
    ok(g_nmlistview.iSubItem == 0, "got subitem %d\n", g_nmlistview.iSubItem);
    ok(g_nmlistview.uNewState == 0, "got new state 0x%08x\n", g_nmlistview.uNewState);
    ok(g_nmlistview.uOldState == LVIS_SELECTED, "got old state 0x%08x\n", g_nmlistview.uOldState);
    ok(g_nmlistview.uChanged == LVIF_STATE, "got changed 0x%08x\n", g_nmlistview.uChanged);
    ok(g_nmlistview.ptAction.x == 0 && g_nmlistview.ptAction.y == 0, "got wrong ptAction value\n");
    ok(g_nmlistview.lParam == 0, "got wrong lparam\n");

    ok_sequence(sequences, PARENT_SEQ_INDEX, ownerdata_deselect_all_parent_seq,
                "ownerdata select all notification", TRUE);

    /* remove focused, try to focus all */
    item.stateMask = LVIS_FOCUSED;
    item.state     = LVIS_FOCUSED;
    res = SendMessageA(hwnd, LVM_SETITEMSTATE, 0, (LPARAM)&item);
    expect(TRUE, res);
    item.stateMask = LVIS_FOCUSED;
    item.state     = 0;
    res = SendMessageA(hwnd, LVM_SETITEMSTATE, -1, (LPARAM)&item);
    expect(TRUE, res);
    item.stateMask = LVIS_FOCUSED;
    res = SendMessageA(hwnd, LVM_GETITEMSTATE, 0, LVIS_FOCUSED);
    expect(0, res);

    /* setting all to focused returns failure value */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    item.stateMask = LVIS_FOCUSED;
    item.state     = LVIS_FOCUSED;

    res = SendMessageA(hwnd, LVM_SETITEMSTATE, -1, (LPARAM)&item);
    expect(FALSE, res);

    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq,
                "ownerdata focus all notification", FALSE);

    /* focus single item, remove all */
    item.stateMask = LVIS_FOCUSED;
    item.state     = LVIS_FOCUSED;
    res = SendMessage(hwnd, LVM_SETITEMSTATE, 0, (LPARAM)&item);
    expect(TRUE, res);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    item.stateMask = LVIS_FOCUSED;
    item.state     = 0;

    memset(&g_nmlistview, 0xcc, sizeof(g_nmlistview));
    res = SendMessageA(hwnd, LVM_SETITEMSTATE, -1, (LPARAM)&item);
    expect(TRUE, res);
    ok(g_nmlistview.iItem == -1, "got item %d\n", g_nmlistview.iItem);
    ok(g_nmlistview.iSubItem == 0, "got subitem %d\n", g_nmlistview.iSubItem);
    ok(g_nmlistview.uNewState == 0, "got new state 0x%08x\n", g_nmlistview.uNewState);
    ok(g_nmlistview.uOldState == LVIS_FOCUSED, "got old state 0x%08x\n", g_nmlistview.uOldState);
    ok(g_nmlistview.uChanged == LVIF_STATE, "got changed 0x%08x\n", g_nmlistview.uChanged);
    ok(g_nmlistview.ptAction.x == 0 && g_nmlistview.ptAction.y == 0, "got wrong ptAction value\n");
    ok(g_nmlistview.lParam == 0, "got wrong lparam\n");

    ok_sequence(sequences, PARENT_SEQ_INDEX, ownerdata_defocus_all_parent_seq,
                "ownerdata remove focus all notification", TRUE);

    /* set all cut */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    item.stateMask = LVIS_CUT;
    item.state     = LVIS_CUT;

    memset(&g_nmlistview, 0xcc, sizeof(g_nmlistview));
    res = SendMessageA(hwnd, LVM_SETITEMSTATE, -1, (LPARAM)&item);
    expect(TRUE, res);
    ok(g_nmlistview.iItem == -1, "got item %d\n", g_nmlistview.iItem);
    ok(g_nmlistview.iSubItem == 0, "got subitem %d\n", g_nmlistview.iSubItem);
    ok(g_nmlistview.uNewState == LVIS_CUT, "got new state 0x%08x\n", g_nmlistview.uNewState);
    ok(g_nmlistview.uOldState == 0, "got old state 0x%08x\n", g_nmlistview.uOldState);
    ok(g_nmlistview.uChanged == LVIF_STATE, "got changed 0x%08x\n", g_nmlistview.uChanged);
    ok(g_nmlistview.ptAction.x == 0 && g_nmlistview.ptAction.y == 0, "got wrong ptAction value\n");
    ok(g_nmlistview.lParam == 0, "got wrong lparam\n");

    ok_sequence(sequences, PARENT_SEQ_INDEX, ownerdata_setstate_all_parent_seq,
                "ownerdata cut all notification", FALSE);

    /* all marked cut, try again */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    item.stateMask = LVIS_CUT;
    item.state     = LVIS_CUT;

    memset(&g_nmlistview, 0xcc, sizeof(g_nmlistview));
    res = SendMessageA(hwnd, LVM_SETITEMSTATE, -1, (LPARAM)&item);
    expect(TRUE, res);
    ok(g_nmlistview.iItem == -1, "got item %d\n", g_nmlistview.iItem);
    ok(g_nmlistview.iSubItem == 0, "got subitem %d\n", g_nmlistview.iSubItem);
    ok(g_nmlistview.uNewState == LVIS_CUT, "got new state 0x%08x\n", g_nmlistview.uNewState);
    ok(g_nmlistview.uOldState == 0, "got old state 0x%08x\n", g_nmlistview.uOldState);
    ok(g_nmlistview.uChanged == LVIF_STATE, "got changed 0x%08x\n", g_nmlistview.uChanged);
    ok(g_nmlistview.ptAction.x == 0 && g_nmlistview.ptAction.y == 0, "got wrong ptAction value\n");
    ok(g_nmlistview.lParam == 0, "got wrong lparam\n");

    ok_sequence(sequences, PARENT_SEQ_INDEX, ownerdata_setstate_all_parent_seq,
                "ownerdata cut all notification #2", FALSE);

    DestroyWindow(hwnd);

    /* check notifications on LVM_GETITEM */
    /* zero callback mask */
    hwnd = create_listview_control(LVS_OWNERDATA | LVS_REPORT);
    ok(hwnd != NULL, "failed to create a listview window\n");
    res = SendMessageA(hwnd, LVM_SETITEMCOUNT, 1, 0);
    expect(1, res);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    memset(&item, 0, sizeof(item));
    item.stateMask = LVIS_SELECTED;
    item.mask      = LVIF_STATE;
    res = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM)&item);
    expect(TRUE, res);

    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq,
                "ownerdata getitem selected state 1", FALSE);

    /* non zero callback mask but not we asking for */
    res = SendMessageA(hwnd, LVM_SETCALLBACKMASK, LVIS_OVERLAYMASK, 0);
    expect(TRUE, res);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    memset(&item, 0, sizeof(item));
    item.stateMask = LVIS_SELECTED;
    item.mask      = LVIF_STATE;
    res = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM)&item);
    expect(TRUE, res);

    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq,
                "ownerdata getitem selected state 2", FALSE);

    /* LVIS_OVERLAYMASK callback mask, asking for index */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    memset(&item, 0, sizeof(item));
    item.stateMask = LVIS_OVERLAYMASK;
    item.mask      = LVIF_STATE;
    res = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM)&item);
    expect(TRUE, res);

    ok_sequence(sequences, PARENT_SEQ_INDEX, single_getdispinfo_parent_seq,
                "ownerdata getitem selected state 2", FALSE);

    DestroyWindow(hwnd);

    /* LVS_SORTASCENDING/LVS_SORTDESCENDING aren't compatible with LVS_OWNERDATA */
    hwnd = create_listview_control(LVS_OWNERDATA | LVS_SORTASCENDING | LVS_REPORT);
    ok(hwnd != NULL, "failed to create a listview window\n");
    style = GetWindowLongPtrA(hwnd, GWL_STYLE);
    ok(style & LVS_OWNERDATA, "Expected LVS_OWNERDATA\n");
    ok(style & LVS_SORTASCENDING, "Expected LVS_SORTASCENDING to be set\n");
    SetWindowLongPtrA(hwnd, GWL_STYLE, style & ~LVS_SORTASCENDING);
    style = GetWindowLongPtrA(hwnd, GWL_STYLE);
    ok(!(style & LVS_SORTASCENDING), "Expected LVS_SORTASCENDING not set\n");
    DestroyWindow(hwnd);
    /* apparently it's allowed to switch these style on after creation */
    hwnd = create_listview_control(LVS_OWNERDATA | LVS_REPORT);
    ok(hwnd != NULL, "failed to create a listview window\n");
    style = GetWindowLongPtrA(hwnd, GWL_STYLE);
    ok(style & LVS_OWNERDATA, "Expected LVS_OWNERDATA\n");
    SetWindowLongPtrA(hwnd, GWL_STYLE, style | LVS_SORTASCENDING);
    style = GetWindowLongPtrA(hwnd, GWL_STYLE);
    ok(style & LVS_SORTASCENDING, "Expected LVS_SORTASCENDING to be set\n");
    DestroyWindow(hwnd);

    hwnd = create_listview_control(LVS_OWNERDATA | LVS_REPORT);
    ok(hwnd != NULL, "failed to create a listview window\n");
    style = GetWindowLongPtrA(hwnd, GWL_STYLE);
    ok(style & LVS_OWNERDATA, "Expected LVS_OWNERDATA\n");
    SetWindowLongPtrA(hwnd, GWL_STYLE, style | LVS_SORTDESCENDING);
    style = GetWindowLongPtrA(hwnd, GWL_STYLE);
    ok(style & LVS_SORTDESCENDING, "Expected LVS_SORTDESCENDING to be set\n");
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
    hwnd = create_listview_control(LVS_REPORT);
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
    hwnd = create_listview_control(LVS_OWNERDATA | LVS_REPORT);
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

    hwnd = create_listview_control(LVS_REPORT);
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
    hwnd = create_listview_control(LVS_NOSORTHEADER | LVS_REPORT);
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

static void test_setredraw(void)
{
    HWND hwnd;
    DWORD_PTR style;
    DWORD ret;
    HDC hdc;
    RECT rect;

    hwnd = create_listview_control(LVS_OWNERDATA | LVS_REPORT);
    ok(hwnd != NULL, "failed to create a listview window\n");

    /* Passing WM_SETREDRAW to DefWinProc removes WS_VISIBLE.
       ListView seems to handle it internally without DefWinProc */

    /* default value first */
    ret = SendMessage(hwnd, WM_SETREDRAW, TRUE, 0);
    expect(0, ret);
    /* disable */
    style = GetWindowLongPtr(hwnd, GWL_STYLE);
    ok(style & WS_VISIBLE, "Expected WS_VISIBLE to be set\n");
    ret = SendMessage(hwnd, WM_SETREDRAW, FALSE, 0);
    expect(0, ret);
    style = GetWindowLongPtr(hwnd, GWL_STYLE);
    ok(style & WS_VISIBLE, "Expected WS_VISIBLE to be set\n");
    ret = SendMessage(hwnd, WM_SETREDRAW, TRUE, 0);
    expect(0, ret);

    /* check update rect after redrawing */
    ret = SendMessage(hwnd, WM_SETREDRAW, FALSE, 0);
    expect(0, ret);
    InvalidateRect(hwnd, NULL, FALSE);
    RedrawWindow(hwnd, NULL, NULL, RDW_UPDATENOW);
    rect.right = rect.bottom = 1;
    GetUpdateRect(hwnd, &rect, FALSE);
    expect(0, rect.right);
    expect(0, rect.bottom);

    /* WM_ERASEBKGND */
    hdc = GetWindowDC(hwndparent);
    ret = SendMessage(hwnd, WM_ERASEBKGND, (WPARAM)hdc, 0);
    expect(TRUE, ret);
    ret = SendMessage(hwnd, WM_SETREDRAW, FALSE, 0);
    expect(0, ret);
    ret = SendMessage(hwnd, WM_ERASEBKGND, (WPARAM)hdc, 0);
    expect(TRUE, ret);
    ret = SendMessage(hwnd, WM_SETREDRAW, TRUE, 0);
    expect(0, ret);
    ReleaseDC(hwndparent, hdc);

    /* check notification messages to show that repainting is disabled */
    ret = SendMessage(hwnd, LVM_SETITEMCOUNT, 1, 0);
    expect(TRUE, ret);
    ret = SendMessage(hwnd, WM_SETREDRAW, FALSE, 0);
    expect(0, ret);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq,
                "redraw after WM_SETREDRAW (FALSE)", FALSE);

    ret = SendMessage(hwnd, LVM_SETBKCOLOR, 0, CLR_NONE);
    expect(TRUE, ret);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq,
                "redraw after WM_SETREDRAW (FALSE) with CLR_NONE bkgnd", FALSE);

    /* message isn't forwarded to header */
    subclass_header(hwnd);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    ret = SendMessage(hwnd, WM_SETREDRAW, FALSE, 0);
    expect(0, ret);
    ok_sequence(sequences, LISTVIEW_SEQ_INDEX, setredraw_seq,
                "WM_SETREDRAW: not forwarded to header", FALSE);

    DestroyWindow(hwnd);
}

static void test_hittest(void)
{
    HWND hwnd;
    DWORD r;
    RECT bounds;
    LVITEMA item;
    static CHAR text[] = "1234567890ABCDEFGHIJKLMNOPQRST";
    POINT pos;
    INT x, y, i;
    WORD vert;
    HIMAGELIST himl, himl2;
    HBITMAP hbmp;

    hwnd = create_listview_control(LVS_REPORT);
    ok(hwnd != NULL, "failed to create a listview window\n");

    /* LVS_REPORT with a single subitem (2 columns) */
    insert_column(hwnd, 0);
    insert_column(hwnd, 1);
    insert_item(hwnd, 0);

    item.iSubItem = 0;
    /* the only purpose of that line is to be as long as a half item rect */
    item.pszText  = text;
    r = SendMessage(hwnd, LVM_SETITEMTEXT, 0, (LPARAM)&item);
    expect(TRUE, r);

    r = SendMessage(hwnd, LVM_SETCOLUMNWIDTH, 0, MAKELPARAM(100, 0));
    expect(TRUE, r);
    r = SendMessage(hwnd, LVM_SETCOLUMNWIDTH, 1, MAKELPARAM(100, 0));
    expect(TRUE, r);

    memset(&bounds, 0, sizeof(bounds));
    bounds.left = LVIR_BOUNDS;
    r = SendMessage(hwnd, LVM_GETITEMRECT, 0, (LPARAM)&bounds);
    expect(1, r);
    ok(bounds.bottom - bounds.top > 0, "Expected non zero item height\n");
    ok(bounds.right - bounds.left > 0, "Expected non zero item width\n");
    r = SendMessage(hwnd, LVM_GETITEMSPACING, TRUE, 0);
    vert = HIWORD(r);
    ok(bounds.bottom - bounds.top == vert,
        "Vertical spacing inconsistent (%d != %d)\n", bounds.bottom - bounds.top, vert);
    r = SendMessage(hwnd, LVM_GETITEMPOSITION, 0, (LPARAM)&pos);
    expect(TRUE, r);

    /* LVS_EX_FULLROWSELECT not set, no icons attached */

    /* outside columns by x position - valid is [0, 199] */
    x = -1;
    y = pos.y + (bounds.bottom - bounds.top) / 2;
    test_lvm_hittest(hwnd, x, y, -1, LVHT_TOLEFT, 0, FALSE, FALSE);
    test_lvm_subitemhittest(hwnd, x, y, -1, -1, LVHT_NOWHERE, FALSE, FALSE, FALSE);

    x = pos.x + 50; /* column half width */
    y = pos.y + (bounds.bottom - bounds.top) / 2;
    test_lvm_hittest(hwnd, x, y, 0, LVHT_ONITEMLABEL, 0, FALSE, FALSE);
    test_lvm_subitemhittest(hwnd, x, y, 0, 0, LVHT_ONITEMLABEL, FALSE, FALSE, FALSE);
    x = pos.x + 150; /* outside column */
    y = pos.y + (bounds.bottom - bounds.top) / 2;
    test_lvm_hittest(hwnd, x, y, -1, LVHT_TORIGHT, 0, FALSE, FALSE);
    test_lvm_subitemhittest(hwnd, x, y, 0, 1, LVHT_ONITEMLABEL, FALSE, FALSE, FALSE);
    y = (bounds.bottom - bounds.top) / 2;
    test_lvm_hittest(hwnd, x, y, -1, LVHT_TORIGHT, 0, FALSE, TRUE);
    test_lvm_subitemhittest(hwnd, x, y, 0, 1, LVHT_ONITEMLABEL, FALSE, FALSE, FALSE);
    /* outside possible client rectangle (to right) */
    x = pos.x + 500;
    y = pos.y + (bounds.bottom - bounds.top) / 2;
    test_lvm_hittest(hwnd, x, y, -1, LVHT_TORIGHT, 0, FALSE, FALSE);
    test_lvm_subitemhittest(hwnd, x, y, -1, -1, LVHT_NOWHERE, FALSE, FALSE, FALSE);
    y = (bounds.bottom - bounds.top) / 2;
    test_lvm_hittest(hwnd, x, y, -1, LVHT_TORIGHT, 0, FALSE, TRUE);
    test_lvm_subitemhittest(hwnd, x, y, -1, -1, LVHT_NOWHERE, FALSE, FALSE, FALSE);
    /* subitem returned with -1 item too */
    x = pos.x + 150;
    y = bounds.top - vert;
    test_lvm_subitemhittest(hwnd, x, y, -1, 1, LVHT_NOWHERE, FALSE, FALSE, FALSE);
    test_lvm_subitemhittest(hwnd, x, y - vert + 1, -1, 1, LVHT_NOWHERE, FALSE, FALSE, FALSE);
    /* return values appear to underflow with negative indices */
    i = -2;
    y = y - vert;
    while (i > -10) {
        test_lvm_subitemhittest(hwnd, x, y, i, 1, LVHT_ONITEMLABEL, TRUE, FALSE, TRUE);
        test_lvm_subitemhittest(hwnd, x, y - vert + 1, i, 1, LVHT_ONITEMLABEL, TRUE, FALSE, TRUE);
        y = y - vert;
        i--;
    }
    /* parent client area is 100x100 by default */
    MoveWindow(hwnd, 0, 0, 300, 100, FALSE);
    x = pos.x + 150; /* outside column */
    y = pos.y + (bounds.bottom - bounds.top) / 2;
    test_lvm_hittest(hwnd, x, y, -1, LVHT_NOWHERE, 0, FALSE, FALSE);
    test_lvm_subitemhittest(hwnd, x, y, 0, 1, LVHT_ONITEMLABEL, FALSE, FALSE, FALSE);
    y = (bounds.bottom - bounds.top) / 2;
    test_lvm_hittest(hwnd, x, y, -1, LVHT_NOWHERE, 0, FALSE, TRUE);
    test_lvm_subitemhittest(hwnd, x, y, 0, 1, LVHT_ONITEMLABEL, FALSE, FALSE, FALSE);
    /* the same with LVS_EX_FULLROWSELECT */
    SendMessage(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);
    x = pos.x + 150; /* outside column */
    y = pos.y + (bounds.bottom - bounds.top) / 2;
    test_lvm_hittest(hwnd, x, y, 0, LVHT_ONITEM, LVHT_ONITEMLABEL, FALSE, FALSE);
    test_lvm_subitemhittest(hwnd, x, y, 0, 1, LVHT_ONITEMLABEL, FALSE, FALSE, FALSE);
    y = (bounds.bottom - bounds.top) / 2;
    test_lvm_subitemhittest(hwnd, x, y, 0, 1, LVHT_ONITEMLABEL, FALSE, FALSE, FALSE);
    MoveWindow(hwnd, 0, 0, 100, 100, FALSE);
    x = pos.x + 150; /* outside column */
    y = pos.y + (bounds.bottom - bounds.top) / 2;
    test_lvm_hittest(hwnd, x, y, -1, LVHT_TORIGHT, 0, FALSE, FALSE);
    test_lvm_subitemhittest(hwnd, x, y, 0, 1, LVHT_ONITEMLABEL, FALSE, FALSE, FALSE);
    y = (bounds.bottom - bounds.top) / 2;
    test_lvm_hittest(hwnd, x, y, -1, LVHT_TORIGHT, 0, FALSE, TRUE);
    test_lvm_subitemhittest(hwnd, x, y, 0, 1, LVHT_ONITEMLABEL, FALSE, FALSE, FALSE);
    /* outside possible client rectangle (to right) */
    x = pos.x + 500;
    y = pos.y + (bounds.bottom - bounds.top) / 2;
    test_lvm_hittest(hwnd, x, y, -1, LVHT_TORIGHT, 0, FALSE, FALSE);
    test_lvm_subitemhittest(hwnd, x, y, -1, -1, LVHT_NOWHERE, FALSE, FALSE, FALSE);
    y = (bounds.bottom - bounds.top) / 2;
    test_lvm_hittest(hwnd, x, y, -1, LVHT_TORIGHT, 0, FALSE, TRUE);
    test_lvm_subitemhittest(hwnd, x, y, -1, -1, LVHT_NOWHERE, FALSE, FALSE, FALSE);
    /* try with icons, state icons index is 1 based so at least 2 bitmaps needed */
    himl = ImageList_Create(16, 16, 0, 4, 4);
    ok(himl != NULL, "failed to create imagelist\n");
    hbmp = CreateBitmap(16, 16, 1, 1, NULL);
    ok(hbmp != NULL, "failed to create bitmap\n");
    r = ImageList_Add(himl, hbmp, 0);
    ok(r == 0, "should be zero\n");
    hbmp = CreateBitmap(16, 16, 1, 1, NULL);
    ok(hbmp != NULL, "failed to create bitmap\n");
    r = ImageList_Add(himl, hbmp, 0);
    ok(r == 1, "should be one\n");

    r = SendMessage(hwnd, LVM_SETIMAGELIST, LVSIL_STATE, (LPARAM)himl);
    expect(0, r);

    item.mask = LVIF_IMAGE;
    item.iImage = 0;
    item.iItem = 0;
    item.iSubItem = 0;
    r = SendMessage(hwnd, LVM_SETITEM, 0, (LPARAM)&item);
    expect(TRUE, r);
    /* on state icon */
    x = pos.x + 8;
    y = pos.y + (bounds.bottom - bounds.top) / 2;
    test_lvm_hittest(hwnd, x, y, 0, LVHT_ONITEMSTATEICON, 0, FALSE, FALSE);
    test_lvm_subitemhittest(hwnd, x, y, 0, 0, LVHT_ONITEMSTATEICON, FALSE, FALSE, FALSE);
    y = (bounds.bottom - bounds.top) / 2;
    test_lvm_subitemhittest(hwnd, x, y, 0, 0, LVHT_ONITEMSTATEICON, FALSE, FALSE, FALSE);

    /* state icons indices are 1 based, check with valid index */
    item.mask = LVIF_STATE;
    item.state = INDEXTOSTATEIMAGEMASK(1);
    item.stateMask = LVIS_STATEIMAGEMASK;
    item.iItem = 0;
    item.iSubItem = 0;
    r = SendMessage(hwnd, LVM_SETITEM, 0, (LPARAM)&item);
    expect(TRUE, r);
    /* on state icon */
    x = pos.x + 8;
    y = pos.y + (bounds.bottom - bounds.top) / 2;
    test_lvm_hittest(hwnd, x, y, 0, LVHT_ONITEMSTATEICON, 0, FALSE, FALSE);
    test_lvm_subitemhittest(hwnd, x, y, 0, 0, LVHT_ONITEMSTATEICON, FALSE, FALSE, FALSE);
    y = (bounds.bottom - bounds.top) / 2;
    test_lvm_subitemhittest(hwnd, x, y, 0, 0, LVHT_ONITEMSTATEICON, FALSE, FALSE, FALSE);

    himl2 = (HIMAGELIST)SendMessage(hwnd, LVM_SETIMAGELIST, LVSIL_STATE, 0);
    ok(himl2 == himl, "should return handle\n");

    r = SendMessage(hwnd, LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)himl);
    expect(0, r);
    /* on item icon */
    x = pos.x + 8;
    y = pos.y + (bounds.bottom - bounds.top) / 2;
    test_lvm_hittest(hwnd, x, y, 0, LVHT_ONITEMICON, 0, FALSE, FALSE);
    test_lvm_subitemhittest(hwnd, x, y, 0, 0, LVHT_ONITEMICON, FALSE, FALSE, FALSE);
    y = (bounds.bottom - bounds.top) / 2;
    test_lvm_subitemhittest(hwnd, x, y, 0, 0, LVHT_ONITEMICON, FALSE, FALSE, FALSE);

    DestroyWindow(hwnd);
}

static void test_getviewrect(void)
{
    HWND hwnd;
    DWORD r;
    RECT rect;
    LVITEMA item;

    hwnd = create_listview_control(LVS_REPORT);
    ok(hwnd != NULL, "failed to create a listview window\n");

    /* empty */
    r = SendMessage(hwnd, LVM_GETVIEWRECT, 0, (LPARAM)&rect);
    expect(TRUE, r);

    insert_column(hwnd, 0);
    insert_column(hwnd, 1);

    memset(&item, 0, sizeof(item));
    item.iItem = 0;
    item.iSubItem = 0;
    SendMessage(hwnd, LVM_INSERTITEMA, 0, (LPARAM)&item);

    r = SendMessage(hwnd, LVM_SETCOLUMNWIDTH, 0, MAKELPARAM(100, 0));
    expect(TRUE, r);
    r = SendMessage(hwnd, LVM_SETCOLUMNWIDTH, 1, MAKELPARAM(120, 0));
    expect(TRUE, r);

    rect.left = rect.right = rect.top = rect.bottom = -1;
    r = SendMessage(hwnd, LVM_GETVIEWRECT, 0, (LPARAM)&rect);
    expect(TRUE, r);
    /* left is set to (2e31-1) - XP SP2 */
    expect(0, rect.right);
    expect(0, rect.top);
    expect(0, rect.bottom);

    /* switch to LVS_ICON */
    SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) & ~LVS_REPORT);

    rect.left = rect.right = rect.top = rect.bottom = -1;
    r = SendMessage(hwnd, LVM_GETVIEWRECT, 0, (LPARAM)&rect);
    expect(TRUE, r);
    expect(0, rect.left);
    expect(0, rect.top);
    /* precise value differs for 2k, XP and Vista */
    ok(rect.bottom > 0, "Expected positive bottom value, got %d\n", rect.bottom);
    ok(rect.right  > 0, "Expected positive right value, got %d\n", rect.right);

    DestroyWindow(hwnd);
}

static void test_getitemposition(void)
{
    HWND hwnd, header;
    DWORD r;
    POINT pt;
    RECT rect;

    hwnd = create_listview_control(LVS_REPORT);
    ok(hwnd != NULL, "failed to create a listview window\n");
    header = subclass_header(hwnd);

    /* LVS_REPORT, single item, no columns added */
    insert_item(hwnd, 0);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    pt.x = pt.y = -1;
    r = SendMessage(hwnd, LVM_GETITEMPOSITION, 0, (LPARAM)&pt);
    expect(TRUE, r);
    ok_sequence(sequences, LISTVIEW_SEQ_INDEX, getitemposition_seq1, "get item position 1", FALSE);

    /* LVS_REPORT, single item, single column */
    insert_column(hwnd, 0);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    pt.x = pt.y = -1;
    r = SendMessage(hwnd, LVM_GETITEMPOSITION, 0, (LPARAM)&pt);
    expect(TRUE, r);
    ok_sequence(sequences, LISTVIEW_SEQ_INDEX, getitemposition_seq2, "get item position 2", TRUE);

    memset(&rect, 0, sizeof(rect));
    SendMessage(header, HDM_GETITEMRECT, 0, (LPARAM)&rect);
    /* some padding? */
    expect(2, pt.x);
    /* offset by header height */
    expect(rect.bottom - rect.top, pt.y);

    DestroyWindow(hwnd);
}

static void test_columnscreation(void)
{
    HWND hwnd, header;
    DWORD r;

    hwnd = create_listview_control(LVS_REPORT);
    ok(hwnd != NULL, "failed to create a listview window\n");

    insert_item(hwnd, 0);

    /* headers columns aren't created automatically */
    header = (HWND)SendMessage(hwnd, LVM_GETHEADER, 0, 0);
    ok(IsWindow(header), "Expected header handle\n");
    r = SendMessage(header, HDM_GETITEMCOUNT, 0, 0);
    expect(0, r);

    DestroyWindow(hwnd);
}

static void test_getitemrect(void)
{
    HWND hwnd;
    HIMAGELIST himl, himl_ret;
    HBITMAP hbm;
    RECT rect;
    DWORD r;
    LVITEMA item;
    LVCOLUMNA col;
    INT order[2];
    POINT pt;
    HDC hdc;

    /* rectangle isn't empty for empty text items */
    hwnd = create_listview_control(LVS_LIST);
    memset(&item, 0, sizeof(item));
    item.mask = 0;
    item.iItem = 0;
    r = SendMessage(hwnd, LVM_INSERTITEMA, 0, (LPARAM)&item);
    expect(0, r);
    rect.left = LVIR_LABEL;
    r = SendMessage(hwnd, LVM_GETITEMRECT, 0, (LPARAM)&rect);
    expect(TRUE, r);
    expect(0, rect.left);
    expect(0, rect.top);
    hdc = GetDC(hwnd);
    todo_wine expect(((GetDeviceCaps(hdc, LOGPIXELSX) + 15) / 16) * 16, rect.right);
    ReleaseDC(hwnd, hdc);
    DestroyWindow(hwnd);

    hwnd = create_listview_control(LVS_REPORT);
    ok(hwnd != NULL, "failed to create a listview window\n");

    /* empty item */
    memset(&item, 0, sizeof(item));
    item.iItem = 0;
    item.iSubItem = 0;
    r = SendMessage(hwnd, LVM_INSERTITEMA, 0, (LPARAM)&item);
    expect(0, r);

    rect.left = LVIR_BOUNDS;
    rect.right = rect.top = rect.bottom = -1;
    r = SendMessage(hwnd, LVM_GETITEMRECT, 0, (LPARAM)&rect);
    expect(TRUE, r);

    /* zero width rectangle with no padding */
    expect(0, rect.left);
    expect(0, rect.right);

    insert_column(hwnd, 0);
    insert_column(hwnd, 1);

    col.mask = LVCF_WIDTH;
    col.cx   = 50;
    r = SendMessage(hwnd, LVM_SETCOLUMN, 0, (LPARAM)&col);
    expect(TRUE, r);

    col.mask = LVCF_WIDTH;
    col.cx   = 100;
    r = SendMessage(hwnd, LVM_SETCOLUMN, 1, (LPARAM)&col);
    expect(TRUE, r);

    rect.left = LVIR_BOUNDS;
    rect.right = rect.top = rect.bottom = -1;
    r = SendMessage(hwnd, LVM_GETITEMRECT, 0, (LPARAM)&rect);
    expect(TRUE, r);

    /* still no left padding */
    expect(0, rect.left);
    expect(150, rect.right);

    rect.left = LVIR_SELECTBOUNDS;
    rect.right = rect.top = rect.bottom = -1;
    r = SendMessage(hwnd, LVM_GETITEMRECT, 0, (LPARAM)&rect);
    expect(TRUE, r);
    /* padding */
    expect(2, rect.left);

    rect.left = LVIR_LABEL;
    rect.right = rect.top = rect.bottom = -1;
    r = SendMessage(hwnd, LVM_GETITEMRECT, 0, (LPARAM)&rect);
    expect(TRUE, r);
    /* padding, column width */
    expect(2, rect.left);
    expect(50, rect.right);

    /* no icons attached */
    rect.left = LVIR_ICON;
    rect.right = rect.top = rect.bottom = -1;
    r = SendMessage(hwnd, LVM_GETITEMRECT, 0, (LPARAM)&rect);
    expect(TRUE, r);
    /* padding */
    expect(2, rect.left);
    expect(2, rect.right);

    /* change order */
    order[0] = 1; order[1] = 0;
    r = SendMessage(hwnd, LVM_SETCOLUMNORDERARRAY, 2, (LPARAM)&order);
    expect(TRUE, r);
    pt.x = -1;
    r = SendMessage(hwnd, LVM_GETITEMPOSITION, 0, (LPARAM)&pt);
    expect(TRUE, r);
    /* 1 indexed column width + padding */
    expect(102, pt.x);
    /* rect is at zero too */
    rect.left = LVIR_BOUNDS;
    rect.right = rect.top = rect.bottom = -1;
    r = SendMessage(hwnd, LVM_GETITEMRECT, 0, (LPARAM)&rect);
    expect(TRUE, r);
    expect(0, rect.left);
    /* just width sum */
    expect(150, rect.right);

    rect.left = LVIR_SELECTBOUNDS;
    rect.right = rect.top = rect.bottom = -1;
    r = SendMessage(hwnd, LVM_GETITEMRECT, 0, (LPARAM)&rect);
    expect(TRUE, r);
    /* column width + padding */
    expect(102, rect.left);

    /* back to initial order */
    order[0] = 0; order[1] = 1;
    r = SendMessage(hwnd, LVM_SETCOLUMNORDERARRAY, 2, (LPARAM)&order);
    expect(TRUE, r);

    /* state icons */
    himl = ImageList_Create(16, 16, 0, 2, 2);
    ok(himl != NULL, "failed to create imagelist\n");
    hbm = CreateBitmap(16, 16, 1, 1, NULL);
    ok(hbm != NULL, "failed to create bitmap\n");
    r = ImageList_Add(himl, hbm, 0);
    expect(0, r);
    hbm = CreateBitmap(16, 16, 1, 1, NULL);
    ok(hbm != NULL, "failed to create bitmap\n");
    r = ImageList_Add(himl, hbm, 0);
    expect(1, r);

    r = SendMessage(hwnd, LVM_SETIMAGELIST, LVSIL_STATE, (LPARAM)himl);
    expect(0, r);

    item.mask = LVIF_STATE;
    item.state = INDEXTOSTATEIMAGEMASK(1);
    item.stateMask = LVIS_STATEIMAGEMASK;
    item.iItem = 0;
    item.iSubItem = 0;
    r = SendMessage(hwnd, LVM_SETITEM, 0, (LPARAM)&item);
    expect(TRUE, r);

    /* icon bounds */
    rect.left = LVIR_ICON;
    rect.right = rect.top = rect.bottom = -1;
    r = SendMessage(hwnd, LVM_GETITEMRECT, 0, (LPARAM)&rect);
    expect(TRUE, r);
    /* padding + stateicon width */
    expect(18, rect.left);
    expect(18, rect.right);
    /* label bounds */
    rect.left = LVIR_LABEL;
    rect.right = rect.top = rect.bottom = -1;
    r = SendMessage(hwnd, LVM_GETITEMRECT, 0, (LPARAM)&rect);
    expect(TRUE, r);
    /* padding + stateicon width -> column width */
    expect(18, rect.left);
    expect(50, rect.right);

    himl_ret = (HIMAGELIST)SendMessage(hwnd, LVM_SETIMAGELIST, LVSIL_STATE, 0);
    ok(himl_ret == himl, "got %p, expected %p\n", himl_ret, himl);

    r = SendMessage(hwnd, LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)himl);
    expect(0, r);

    item.mask = LVIF_STATE | LVIF_IMAGE;
    item.iImage = 1;
    item.state = 0;
    item.stateMask = ~0;
    item.iItem = 0;
    item.iSubItem = 0;
    r = SendMessage(hwnd, LVM_SETITEM, 0, (LPARAM)&item);
    expect(TRUE, r);

    /* icon bounds */
    rect.left = LVIR_ICON;
    rect.right = rect.top = rect.bottom = -1;
    r = SendMessage(hwnd, LVM_GETITEMRECT, 0, (LPARAM)&rect);
    expect(TRUE, r);
    /* padding, icon width */
    expect(2, rect.left);
    expect(18, rect.right);
    /* label bounds */
    rect.left = LVIR_LABEL;
    rect.right = rect.top = rect.bottom = -1;
    r = SendMessage(hwnd, LVM_GETITEMRECT, 0, (LPARAM)&rect);
    expect(TRUE, r);
    /* padding + icon width -> column width */
    expect(18, rect.left);
    expect(50, rect.right);

    /* select bounds */
    rect.left = LVIR_SELECTBOUNDS;
    rect.right = rect.top = rect.bottom = -1;
    r = SendMessage(hwnd, LVM_GETITEMRECT, 0, (LPARAM)&rect);
    expect(TRUE, r);
    /* padding, column width */
    expect(2, rect.left);
    expect(50, rect.right);

    /* try with indentation */
    item.mask = LVIF_INDENT;
    item.iIndent = 1;
    item.iItem = 0;
    item.iSubItem = 0;
    r = SendMessage(hwnd, LVM_SETITEM, 0, (LPARAM)&item);
    expect(TRUE, r);

    /* bounds */
    rect.left = LVIR_BOUNDS;
    rect.right = rect.top = rect.bottom = -1;
    r = SendMessage(hwnd, LVM_GETITEMRECT, 0, (LPARAM)&rect);
    expect(TRUE, r);
    /* padding + 1 icon width, column width */
    expect(0, rect.left);
    expect(150, rect.right);

    /* select bounds */
    rect.left = LVIR_SELECTBOUNDS;
    rect.right = rect.top = rect.bottom = -1;
    r = SendMessage(hwnd, LVM_GETITEMRECT, 0, (LPARAM)&rect);
    expect(TRUE, r);
    /* padding + 1 icon width, column width */
    expect(2 + 16, rect.left);
    expect(50, rect.right);

    /* label bounds */
    rect.left = LVIR_LABEL;
    rect.right = rect.top = rect.bottom = -1;
    r = SendMessage(hwnd, LVM_GETITEMRECT, 0, (LPARAM)&rect);
    expect(TRUE, r);
    /* padding + 2 icon widths, column width */
    expect(2 + 16*2, rect.left);
    expect(50, rect.right);

    /* icon bounds */
    rect.left = LVIR_ICON;
    rect.right = rect.top = rect.bottom = -1;
    r = SendMessage(hwnd, LVM_GETITEMRECT, 0, (LPARAM)&rect);
    expect(TRUE, r);
    /* padding + 1 icon width indentation, icon width */
    expect(2 + 16, rect.left);
    expect(34, rect.right);

    DestroyWindow(hwnd);
}

static void test_editbox(void)
{
    static CHAR testitemA[]  = "testitem";
    static CHAR testitem1A[] = "testitem_quitelongname";
    static CHAR testitem2A[] = "testITEM_quitelongname";
    static CHAR buffer[25];
    HWND hwnd, hwndedit, hwndedit2, header;
    LVITEMA item;
    INT r;

    hwnd = create_listview_control(LVS_EDITLABELS | LVS_REPORT);
    ok(hwnd != NULL, "failed to create a listview window\n");

    insert_column(hwnd, 0);

    memset(&item, 0, sizeof(item));
    item.mask = LVIF_TEXT;
    item.pszText = testitemA;
    item.iItem = 0;
    item.iSubItem = 0;
    r = SendMessage(hwnd, LVM_INSERTITEMA, 0, (LPARAM)&item);
    expect(0, r);

    /* test notifications without edit created */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    r = SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(0, EN_SETFOCUS), (LPARAM)0xdeadbeef);
    expect(0, r);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq,
                "edit box WM_COMMAND (EN_SETFOCUS), no edit created", FALSE);
    /* same thing but with valid window */
    hwndedit = CreateWindowA("Edit", "Test edit", WS_VISIBLE | WS_CHILD, 0, 0, 20,
                10, hwnd, (HMENU)1, (HINSTANCE)GetWindowLongPtrA(hwnd, GWLP_HINSTANCE), 0);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    r = SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(0, EN_SETFOCUS), (LPARAM)hwndedit);
    expect(0, r);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq,
                "edit box WM_COMMAND (EN_SETFOCUS), no edit created #2", FALSE);
    DestroyWindow(hwndedit);

    /* setting focus is necessary */
    SetFocus(hwnd);
    hwndedit = (HWND)SendMessage(hwnd, LVM_EDITLABEL, 0, 0);
    ok(IsWindow(hwndedit), "Expected Edit window to be created\n");

    /* test children Z-order after Edit box created */
    header = (HWND)SendMessageA(hwnd, LVM_GETHEADER, 0, 0);
    ok(IsWindow(header), "Expected header to be created\n");
    ok(GetTopWindow(hwnd) == header, "Expected header to be on top\n");
    ok(GetNextWindow(header, GW_HWNDNEXT) == hwndedit, "got %p\n", GetNextWindow(header, GW_HWNDNEXT));

    /* modify initial string */
    r = SendMessage(hwndedit, WM_SETTEXT, 0, (LPARAM)testitem1A);
    expect(TRUE, r);

    /* edit window is resized and repositioned,
       check again for Z-order - it should be preserved */
    ok(GetTopWindow(hwnd) == header, "Expected header to be on top\n");
    ok(GetNextWindow(header, GW_HWNDNEXT) == hwndedit, "got %p\n", GetNextWindow(header, GW_HWNDNEXT));

    /* return focus to listview */
    SetFocus(hwnd);

    memset(&item, 0, sizeof(item));
    item.mask = LVIF_TEXT;
    item.pszText = buffer;
    item.cchTextMax = sizeof(buffer);
    item.iItem = 0;
    item.iSubItem = 0;
    r = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM)&item);
    expect(TRUE, r);

    ok(strcmp(buffer, testitem1A) == 0, "Expected item text to change\n");

    /* send LVM_EDITLABEL on already created edit */
    SetFocus(hwnd);
    hwndedit = (HWND)SendMessage(hwnd, LVM_EDITLABEL, 0, 0);
    ok(IsWindow(hwndedit), "Expected Edit window to be created\n");
    /* focus will be set to edit */
    ok(GetFocus() == hwndedit, "Expected Edit window to be focused\n");
    hwndedit2 = (HWND)SendMessage(hwnd, LVM_EDITLABEL, 0, 0);
    ok(IsWindow(hwndedit2), "Expected Edit window to be created\n");

    /* creating label disabled when control isn't focused */
    SetFocus(0);
    hwndedit = (HWND)SendMessage(hwnd, LVM_EDITLABEL, 0, 0);
    todo_wine ok(hwndedit == NULL, "Expected Edit window not to be created\n");

    /* check EN_KILLFOCUS handling */
    memset(&item, 0, sizeof(item));
    item.pszText = testitemA;
    item.iItem = 0;
    item.iSubItem = 0;
    r = SendMessage(hwnd, LVM_SETITEMTEXTA, 0, (LPARAM)&item);
    expect(TRUE, r);

    SetFocus(hwnd);
    hwndedit = (HWND)SendMessage(hwnd, LVM_EDITLABEL, 0, 0);
    ok(IsWindow(hwndedit), "Expected Edit window to be created\n");
    /* modify edit and notify control that it lost focus */
    r = SendMessage(hwndedit, WM_SETTEXT, 0, (LPARAM)testitem1A);
    expect(TRUE, r);
    g_editbox_disp_info.item.pszText = NULL;
    r = SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(0, EN_KILLFOCUS), (LPARAM)hwndedit);
    expect(0, r);
    ok(g_editbox_disp_info.item.pszText != NULL, "expected notification with not null text\n");

    memset(&item, 0, sizeof(item));
    item.pszText = buffer;
    item.cchTextMax = sizeof(buffer);
    item.iItem = 0;
    item.iSubItem = 0;
    r = SendMessage(hwnd, LVM_GETITEMTEXTA, 0, (LPARAM)&item);
    expect(lstrlen(item.pszText), r);
    ok(strcmp(buffer, testitem1A) == 0, "Expected item text to change\n");
    ok(!IsWindow(hwndedit), "Expected Edit window to be freed\n");

    /* change item name to differ in casing only */
    SetFocus(hwnd);
    hwndedit = (HWND)SendMessage(hwnd, LVM_EDITLABEL, 0, 0);
    ok(IsWindow(hwndedit), "Expected Edit window to be created\n");
    /* modify edit and notify control that it lost focus */
    r = SendMessage(hwndedit, WM_SETTEXT, 0, (LPARAM)testitem2A);
    expect(TRUE, r);
    g_editbox_disp_info.item.pszText = NULL;
    r = SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(0, EN_KILLFOCUS), (LPARAM)hwndedit);
    expect(0, r);
    ok(g_editbox_disp_info.item.pszText != NULL, "got %p\n", g_editbox_disp_info.item.pszText);

    memset(&item, 0, sizeof(item));
    item.pszText = buffer;
    item.cchTextMax = sizeof(buffer);
    item.iItem = 0;
    item.iSubItem = 0;
    r = SendMessage(hwnd, LVM_GETITEMTEXTA, 0, (LPARAM)&item);
    expect(lstrlen(item.pszText), r);
    ok(strcmp(buffer, testitem2A) == 0, "got %s, expected %s\n", buffer, testitem2A);
    ok(!IsWindow(hwndedit), "Expected Edit window to be freed\n");

    /* end edit without saving */
    SetFocus(hwnd);
    hwndedit = (HWND)SendMessage(hwnd, LVM_EDITLABEL, 0, 0);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    r = SendMessage(hwndedit, WM_KEYDOWN, VK_ESCAPE, 0);
    expect(0, r);
    ok_sequence(sequences, PARENT_SEQ_INDEX, edit_end_nochange,
                "edit box - end edit, no change, escape", TRUE);
    /* end edit with saving */
    SetFocus(hwnd);
    hwndedit = (HWND)SendMessage(hwnd, LVM_EDITLABEL, 0, 0);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    r = SendMessage(hwndedit, WM_KEYDOWN, VK_RETURN, 0);
    expect(0, r);
    ok_sequence(sequences, PARENT_SEQ_INDEX, edit_end_nochange,
                "edit box - end edit, no change, return", TRUE);

    memset(&item, 0, sizeof(item));
    item.pszText = buffer;
    item.cchTextMax = sizeof(buffer);
    item.iItem = 0;
    item.iSubItem = 0;
    r = SendMessage(hwnd, LVM_GETITEMTEXTA, 0, (LPARAM)&item);
    expect(lstrlen(item.pszText), r);
    ok(strcmp(buffer, testitem2A) == 0, "Expected item text to change\n");

    /* LVM_EDITLABEL with -1 destroys current edit */
    hwndedit = (HWND)SendMessage(hwnd, LVM_GETEDITCONTROL, 0, 0);
    ok(hwndedit == NULL, "Expected Edit window not to be created\n");
    /* no edit present */
    hwndedit = (HWND)SendMessage(hwnd, LVM_EDITLABEL, -1, 0);
    ok(hwndedit == NULL, "Expected Edit window not to be created\n");
    hwndedit = (HWND)SendMessage(hwnd, LVM_EDITLABEL, 0, 0);
    ok(IsWindow(hwndedit), "Expected Edit window to be created\n");
    /* edit present */
    ok(GetFocus() == hwndedit, "Expected Edit to be focused\n");
    hwndedit2 = (HWND)SendMessage(hwnd, LVM_EDITLABEL, -1, 0);
    ok(hwndedit2 == NULL, "Expected Edit window not to be created\n");
    ok(!IsWindow(hwndedit), "Expected Edit window to be destroyed\n");
    ok(GetFocus() == hwnd, "Expected List to be focused\n");
    /* check another negative value */
    hwndedit = (HWND)SendMessage(hwnd, LVM_EDITLABEL, 0, 0);
    ok(IsWindow(hwndedit), "Expected Edit window to be created\n");
    ok(GetFocus() == hwndedit, "Expected Edit to be focused\n");
    hwndedit2 = (HWND)SendMessage(hwnd, LVM_EDITLABEL, -2, 0);
    ok(hwndedit2 == NULL, "Expected Edit window not to be created\n");
    ok(!IsWindow(hwndedit), "Expected Edit window to be destroyed\n");
    ok(GetFocus() == hwnd, "Expected List to be focused\n");
    /* and value greater than max item index */
    hwndedit = (HWND)SendMessage(hwnd, LVM_EDITLABEL, 0, 0);
    ok(IsWindow(hwndedit), "Expected Edit window to be created\n");
    ok(GetFocus() == hwndedit, "Expected Edit to be focused\n");
    r = SendMessage(hwnd, LVM_GETITEMCOUNT, 0, 0);
    hwndedit2 = (HWND)SendMessage(hwnd, LVM_EDITLABEL, r, 0);
    ok(hwndedit2 == NULL, "Expected Edit window not to be created\n");
    ok(!IsWindow(hwndedit), "Expected Edit window to be destroyed\n");
    ok(GetFocus() == hwnd, "Expected List to be focused\n");

    /* messaging tests */
    SetFocus(hwnd);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    blockEdit = FALSE;
    hwndedit = (HWND)SendMessage(hwnd, LVM_EDITLABEL, 0, 0);
    ok(IsWindow(hwndedit), "Expected Edit window to be created\n");
    /* testing only sizing messages */
    ok_sequence(sequences, EDITBOX_SEQ_INDEX, editbox_create_pos,
                "edit box create - sizing", FALSE);

    /* WM_COMMAND with EN_KILLFOCUS isn't forwarded to parent */
    SetFocus(hwnd);
    hwndedit = (HWND)SendMessage(hwnd, LVM_EDITLABEL, 0, 0);
    ok(IsWindow(hwndedit), "Expected Edit window to be created\n");
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    r = SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(0, EN_KILLFOCUS), (LPARAM)hwndedit);
    expect(0, r);
    ok_sequence(sequences, PARENT_SEQ_INDEX, edit_end_nochange,
                "edit box WM_COMMAND (EN_KILLFOCUS)", TRUE);

    DestroyWindow(hwnd);
}

static void test_notifyformat(void)
{
    HWND hwnd, header;
    DWORD r;

    hwnd = create_listview_control(LVS_REPORT);
    ok(hwnd != NULL, "failed to create a listview window\n");

    /* CCM_GETUNICODEFORMAT == LVM_GETUNICODEFORMAT,
       CCM_SETUNICODEFORMAT == LVM_SETUNICODEFORMAT */
    r = SendMessage(hwnd, LVM_GETUNICODEFORMAT, 0, 0);
    expect(0, r);
    SendMessage(hwnd, WM_NOTIFYFORMAT, 0, NF_QUERY);
    /* set */
    r = SendMessage(hwnd, LVM_SETUNICODEFORMAT, 1, 0);
    expect(0, r);
    r = SendMessage(hwnd, LVM_GETUNICODEFORMAT, 0, 0);
    if (r == 1)
    {
        r = SendMessage(hwnd, LVM_SETUNICODEFORMAT, 0, 0);
        expect(1, r);
        r = SendMessage(hwnd, LVM_GETUNICODEFORMAT, 0, 0);
        expect(0, r);
    }
    else
    {
        win_skip("LVM_GETUNICODEFORMAT is unsupported\n");
        DestroyWindow(hwnd);
        return;
    }

    DestroyWindow(hwnd);

    /* test failure in parent WM_NOTIFYFORMAT  */
    notifyFormat = 0;
    hwnd = create_listview_control(LVS_REPORT);
    ok(hwnd != NULL, "failed to create a listview window\n");
    header = (HWND)SendMessage(hwnd, LVM_GETHEADER, 0, 0);
    ok(IsWindow(header), "expected header to be created\n");
    r = SendMessage(hwnd, LVM_GETUNICODEFORMAT, 0, 0);
    expect(0, r);
    r = SendMessage(header, HDM_GETUNICODEFORMAT, 0, 0);
    ok( r == 1, "Expected 1, got %d\n", r );
    r = SendMessage(hwnd, WM_NOTIFYFORMAT, 0, NF_QUERY);
    ok(r != 0, "Expected valid format\n");

    notifyFormat = NFR_UNICODE;
    r = SendMessage(hwnd, WM_NOTIFYFORMAT, 0, NF_REQUERY);
    expect(NFR_UNICODE, r);
    r = SendMessage(hwnd, LVM_GETUNICODEFORMAT, 0, 0);
    expect(1, r);
    r = SendMessage(header, HDM_GETUNICODEFORMAT, 0, 0);
    ok( r == 1, "Expected 1, got %d\n", r );

    notifyFormat = NFR_ANSI;
    r = SendMessage(hwnd, WM_NOTIFYFORMAT, 0, NF_REQUERY);
    expect(NFR_ANSI, r);
    r = SendMessage(hwnd, LVM_GETUNICODEFORMAT, 0, 0);
    expect(0, r);
    r = SendMessage(header, HDM_GETUNICODEFORMAT, 0, 0);
    ok( r == 1, "Expected 1, got %d\n", r );

    DestroyWindow(hwnd);

    hwndparentW = create_parent_window(TRUE);
    ok(IsWindow(hwndparentW), "Unicode parent creation failed\n");
    if (!IsWindow(hwndparentW))  return;

    notifyFormat = -1;
    hwnd = create_listview_controlW(LVS_REPORT, hwndparentW);
    ok(hwnd != NULL, "failed to create a listview window\n");
    header = (HWND)SendMessage(hwnd, LVM_GETHEADER, 0, 0);
    ok(IsWindow(header), "expected header to be created\n");
    r = SendMessageW(hwnd, LVM_GETUNICODEFORMAT, 0, 0);
    expect(1, r);
    r = SendMessage(header, HDM_GETUNICODEFORMAT, 0, 0);
    expect(1, r);
    DestroyWindow(hwnd);
    /* receiving error code defaulting to ansi */
    notifyFormat = 0;
    hwnd = create_listview_controlW(LVS_REPORT, hwndparentW);
    ok(hwnd != NULL, "failed to create a listview window\n");
    header = (HWND)SendMessage(hwnd, LVM_GETHEADER, 0, 0);
    ok(IsWindow(header), "expected header to be created\n");
    r = SendMessageW(hwnd, LVM_GETUNICODEFORMAT, 0, 0);
    expect(0, r);
    r = SendMessage(header, HDM_GETUNICODEFORMAT, 0, 0);
    expect(1, r);
    DestroyWindow(hwnd);
    /* receiving ansi code from unicode window, use it */
    notifyFormat = NFR_ANSI;
    hwnd = create_listview_controlW(LVS_REPORT, hwndparentW);
    ok(hwnd != NULL, "failed to create a listview window\n");
    header = (HWND)SendMessage(hwnd, LVM_GETHEADER, 0, 0);
    ok(IsWindow(header), "expected header to be created\n");
    r = SendMessageW(hwnd, LVM_GETUNICODEFORMAT, 0, 0);
    expect(0, r);
    r = SendMessage(header, HDM_GETUNICODEFORMAT, 0, 0);
    expect(1, r);
    DestroyWindow(hwnd);
    /* unicode listview with ansi parent window */
    notifyFormat = -1;
    hwnd = create_listview_controlW(LVS_REPORT, hwndparent);
    ok(hwnd != NULL, "failed to create a listview window\n");
    header = (HWND)SendMessage(hwnd, LVM_GETHEADER, 0, 0);
    ok(IsWindow(header), "expected header to be created\n");
    r = SendMessageW(hwnd, LVM_GETUNICODEFORMAT, 0, 0);
    expect(0, r);
    r = SendMessage(header, HDM_GETUNICODEFORMAT, 0, 0);
    expect(1, r);
    DestroyWindow(hwnd);
    /* unicode listview with ansi parent window, return error code */
    notifyFormat = 0;
    hwnd = create_listview_controlW(LVS_REPORT, hwndparent);
    ok(hwnd != NULL, "failed to create a listview window\n");
    header = (HWND)SendMessage(hwnd, LVM_GETHEADER, 0, 0);
    ok(IsWindow(header), "expected header to be created\n");
    r = SendMessageW(hwnd, LVM_GETUNICODEFORMAT, 0, 0);
    expect(0, r);
    r = SendMessage(header, HDM_GETUNICODEFORMAT, 0, 0);
    expect(1, r);
    DestroyWindow(hwnd);

    DestroyWindow(hwndparentW);
}

static void test_indentation(void)
{
    HWND hwnd;
    LVITEMA item;
    DWORD r;

    hwnd = create_listview_control(LVS_REPORT);
    ok(hwnd != NULL, "failed to create a listview window\n");

    memset(&item, 0, sizeof(item));
    item.mask = LVIF_INDENT;
    item.iItem = 0;
    item.iIndent = I_INDENTCALLBACK;
    r = SendMessage(hwnd, LVM_INSERTITEMA, 0, (LPARAM)&item);
    expect(0, r);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    item.iItem = 0;
    item.mask = LVIF_INDENT;
    r = SendMessage(hwnd, LVM_GETITEM, 0, (LPARAM)&item);
    expect(TRUE, r);

    ok_sequence(sequences, PARENT_SEQ_INDEX, single_getdispinfo_parent_seq,
                "get indent dispinfo", FALSE);

    DestroyWindow(hwnd);
}

static INT CALLBACK DummyCompareEx(LPARAM first, LPARAM second, LPARAM param)
{
    return 0;
}

static BOOL is_below_comctl_5(void)
{
    HWND hwnd;
    BOOL ret;

    hwnd = create_listview_control(LVS_REPORT);
    ok(hwnd != NULL, "failed to create a listview window\n");
    insert_item(hwnd, 0);

    ret = SendMessage(hwnd, LVM_SORTITEMSEX, 0, (LPARAM)&DummyCompareEx);

    DestroyWindow(hwnd);

    return !ret;
}

static void test_get_set_view(void)
{
    HWND hwnd;
    DWORD ret;
    DWORD_PTR style;

    /* test style->view mapping */
    hwnd = create_listview_control(LVS_REPORT);
    ok(hwnd != NULL, "failed to create a listview window\n");

    ret = SendMessage(hwnd, LVM_GETVIEW, 0, 0);
    expect(LV_VIEW_DETAILS, ret);

    style = GetWindowLongPtr(hwnd, GWL_STYLE);
    /* LVS_ICON == 0 */
    SetWindowLongPtr(hwnd, GWL_STYLE, style & ~LVS_REPORT);
    ret = SendMessage(hwnd, LVM_GETVIEW, 0, 0);
    expect(LV_VIEW_ICON, ret);

    style = GetWindowLongPtr(hwnd, GWL_STYLE);
    SetWindowLongPtr(hwnd, GWL_STYLE, style | LVS_SMALLICON);
    ret = SendMessage(hwnd, LVM_GETVIEW, 0, 0);
    expect(LV_VIEW_SMALLICON, ret);

    style = GetWindowLongPtr(hwnd, GWL_STYLE);
    SetWindowLongPtr(hwnd, GWL_STYLE, (style & ~LVS_SMALLICON) | LVS_LIST);
    ret = SendMessage(hwnd, LVM_GETVIEW, 0, 0);
    expect(LV_VIEW_LIST, ret);

    /* switching view doesn't touch window style */
    ret = SendMessage(hwnd, LVM_SETVIEW, LV_VIEW_DETAILS, 0);
    expect(1, ret);
    style = GetWindowLongPtr(hwnd, GWL_STYLE);
    ok(style & LVS_LIST, "Expected style to be preserved\n");
    ret = SendMessage(hwnd, LVM_SETVIEW, LV_VIEW_ICON, 0);
    expect(1, ret);
    style = GetWindowLongPtr(hwnd, GWL_STYLE);
    ok(style & LVS_LIST, "Expected style to be preserved\n");
    ret = SendMessage(hwnd, LVM_SETVIEW, LV_VIEW_SMALLICON, 0);
    expect(1, ret);
    style = GetWindowLongPtr(hwnd, GWL_STYLE);
    ok(style & LVS_LIST, "Expected style to be preserved\n");

    DestroyWindow(hwnd);
}

static void test_canceleditlabel(void)
{
    HWND hwnd, hwndedit;
    DWORD ret;
    CHAR buff[10];
    LVITEMA itema;
    static CHAR test[] = "test";
    static const CHAR test1[] = "test1";

    hwnd = create_listview_control(LVS_EDITLABELS | LVS_REPORT);
    ok(hwnd != NULL, "failed to create a listview window\n");

    insert_item(hwnd, 0);

    /* try without edit created */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    ret = SendMessage(hwnd, LVM_CANCELEDITLABEL, 0, 0);
    expect(TRUE, ret);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq,
                "cancel edit label without edit", FALSE);

    /* cancel without data change */
    SetFocus(hwnd);
    hwndedit = (HWND)SendMessage(hwnd, LVM_EDITLABEL, 0, 0);
    ok(IsWindow(hwndedit), "Expected edit control to be created\n");
    ret = SendMessage(hwnd, LVM_CANCELEDITLABEL, 0, 0);
    expect(TRUE, ret);
    ok(!IsWindow(hwndedit), "Expected edit control to be destroyed\n");

    /* cancel after data change */
    memset(&itema, 0, sizeof(itema));
    itema.pszText = test;
    ret = SendMessage(hwnd, LVM_SETITEMTEXT, 0, (LPARAM)&itema);
    expect(TRUE, ret);
    SetFocus(hwnd);
    hwndedit = (HWND)SendMessage(hwnd, LVM_EDITLABEL, 0, 0);
    ok(IsWindow(hwndedit), "Expected edit control to be created\n");
    ret = SetWindowText(hwndedit, test1);
    expect(1, ret);
    ret = SendMessage(hwnd, LVM_CANCELEDITLABEL, 0, 0);
    expect(TRUE, ret);
    ok(!IsWindow(hwndedit), "Expected edit control to be destroyed\n");
    memset(&itema, 0, sizeof(itema));
    itema.pszText = buff;
    itema.cchTextMax = sizeof(buff)/sizeof(CHAR);
    ret = SendMessage(hwnd, LVM_GETITEMTEXT, 0, (LPARAM)&itema);
    expect(5, ret);
    ok(strcmp(buff, test1) == 0, "Expected label text not to change\n");

    DestroyWindow(hwnd);
}

static void test_mapidindex(void)
{
    HWND hwnd;
    INT ret;

    /* LVM_MAPINDEXTOID unsupported with LVS_OWNERDATA */
    hwnd = create_listview_control(LVS_OWNERDATA | LVS_REPORT);
    ok(hwnd != NULL, "failed to create a listview window\n");
    insert_item(hwnd, 0);
    ret = SendMessage(hwnd, LVM_MAPINDEXTOID, 0, 0);
    expect(-1, ret);
    DestroyWindow(hwnd);

    hwnd = create_listview_control(LVS_REPORT);
    ok(hwnd != NULL, "failed to create a listview window\n");

    /* LVM_MAPINDEXTOID with invalid index */
    ret = SendMessage(hwnd, LVM_MAPINDEXTOID, 0, 0);
    expect(-1, ret);

    insert_item(hwnd, 0);
    insert_item(hwnd, 1);

    ret = SendMessage(hwnd, LVM_MAPINDEXTOID, -1, 0);
    expect(-1, ret);
    ret = SendMessage(hwnd, LVM_MAPINDEXTOID, 2, 0);
    expect(-1, ret);

    ret = SendMessage(hwnd, LVM_MAPINDEXTOID, 0, 0);
    expect(0, ret);
    ret = SendMessage(hwnd, LVM_MAPINDEXTOID, 1, 0);
    expect(1, ret);
    /* remove 0 indexed item, id retained */
    SendMessage(hwnd, LVM_DELETEITEM, 0, 0);
    ret = SendMessage(hwnd, LVM_MAPINDEXTOID, 0, 0);
    expect(1, ret);
    /* new id starts from previous value */
    insert_item(hwnd, 1);
    ret = SendMessage(hwnd, LVM_MAPINDEXTOID, 1, 0);
    expect(2, ret);

    /* get index by id */
    ret = SendMessage(hwnd, LVM_MAPIDTOINDEX, -1, 0);
    expect(-1, ret);
    ret = SendMessage(hwnd, LVM_MAPIDTOINDEX, 0, 0);
    expect(-1, ret);
    ret = SendMessage(hwnd, LVM_MAPIDTOINDEX, 1, 0);
    expect(0, ret);
    ret = SendMessage(hwnd, LVM_MAPIDTOINDEX, 2, 0);
    expect(1, ret);

    DestroyWindow(hwnd);
}

static void test_getitemspacing(void)
{
    HWND hwnd;
    DWORD ret;
    INT cx, cy;
    HIMAGELIST himl;
    HBITMAP hbmp;
    LVITEMA itema;

    cx = GetSystemMetrics(SM_CXICONSPACING) - GetSystemMetrics(SM_CXICON);
    cy = GetSystemMetrics(SM_CYICONSPACING) - GetSystemMetrics(SM_CYICON);

    /* LVS_ICON */
    hwnd = create_listview_control(LVS_ICON);
    ret = SendMessage(hwnd, LVM_GETITEMSPACING, FALSE, 0);
todo_wine {
    expect(cx, LOWORD(ret));
    expect(cy, HIWORD(ret));
}
    /* now try with icons */
    himl = ImageList_Create(40, 40, 0, 4, 4);
    ok(himl != NULL, "failed to create imagelist\n");
    hbmp = CreateBitmap(40, 40, 1, 1, NULL);
    ok(hbmp != NULL, "failed to create bitmap\n");
    ret = ImageList_Add(himl, hbmp, 0);
    expect(0, ret);
    ret = SendMessage(hwnd, LVM_SETIMAGELIST, 0, (LPARAM)himl);
    expect(0, ret);

    itema.mask = LVIF_IMAGE;
    itema.iImage = 0;
    itema.iItem = 0;
    itema.iSubItem = 0;
    ret = SendMessage(hwnd, LVM_INSERTITEM, 0, (LPARAM)&itema);
    expect(0, ret);
    ret = SendMessage(hwnd, LVM_GETITEMSPACING, FALSE, 0);
todo_wine {
    /* spacing + icon size returned */
    expect(cx + 40, LOWORD(ret));
    expect(cy + 40, HIWORD(ret));
}
    DestroyWindow(hwnd);
    /* LVS_SMALLICON */
    hwnd = create_listview_control(LVS_SMALLICON);
    ret = SendMessage(hwnd, LVM_GETITEMSPACING, FALSE, 0);
todo_wine {
    expect(cx, LOWORD(ret));
    expect(cy, HIWORD(ret));
}
    DestroyWindow(hwnd);
    /* LVS_REPORT */
    hwnd = create_listview_control(LVS_REPORT);
    ret = SendMessage(hwnd, LVM_GETITEMSPACING, FALSE, 0);
todo_wine {
    expect(cx, LOWORD(ret));
    expect(cy, HIWORD(ret));
}
    DestroyWindow(hwnd);
    /* LVS_LIST */
    hwnd = create_listview_control(LVS_LIST);
    ret = SendMessage(hwnd, LVM_GETITEMSPACING, FALSE, 0);
todo_wine {
    expect(cx, LOWORD(ret));
    expect(cy, HIWORD(ret));
}
    DestroyWindow(hwnd);
}

static void test_getcolumnwidth(void)
{
    HWND hwnd;
    INT ret;
    DWORD_PTR style;
    LVCOLUMNA col;
    LVITEMA itema;
    HDC hdc;

    /* default column width */
    hwnd = create_listview_control(LVS_ICON);
    ret = SendMessage(hwnd, LVM_GETCOLUMNWIDTH, 0, 0);
    expect(0, ret);
    style = GetWindowLong(hwnd, GWL_STYLE);
    SetWindowLong(hwnd, GWL_STYLE, style | LVS_LIST);
    ret = SendMessage(hwnd, LVM_GETCOLUMNWIDTH, 0, 0);
    todo_wine expect(8, ret);
    style = GetWindowLong(hwnd, GWL_STYLE) & ~LVS_LIST;
    SetWindowLong(hwnd, GWL_STYLE, style | LVS_REPORT);
    col.mask = 0;
    ret = SendMessage(hwnd, LVM_INSERTCOLUMNA, 0, (LPARAM)&col);
    expect(0, ret);
    ret = SendMessage(hwnd, LVM_GETCOLUMNWIDTH, 0, 0);
    expect(10, ret);
    DestroyWindow(hwnd);

    /* default column width with item added */
    hwnd = create_listview_control(LVS_LIST);
    memset(&itema, 0, sizeof(itema));
    SendMessage(hwnd, LVM_INSERTITEMA, 0, (LPARAM)&itema);
    ret = SendMessage(hwnd, LVM_GETCOLUMNWIDTH, 0, 0);
    hdc = GetDC(hwnd);
    todo_wine expect(((GetDeviceCaps(hdc, LOGPIXELSX) + 15) / 16) * 16, ret);
    ReleaseDC(hwnd, hdc);
    DestroyWindow(hwnd);
}

static void test_scrollnotify(void)
{
    HWND hwnd;
    DWORD ret;

    hwnd = create_listview_control(LVS_REPORT);

    insert_column(hwnd, 0);
    insert_column(hwnd, 1);
    insert_item(hwnd, 0);

    /* make it scrollable - resize */
    ret = SendMessage(hwnd, LVM_SETCOLUMNWIDTH, 0, MAKELPARAM(100, 0));
    expect(TRUE, ret);
    ret = SendMessage(hwnd, LVM_SETCOLUMNWIDTH, 1, MAKELPARAM(100, 0));
    expect(TRUE, ret);

    /* try with dummy call */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    ret = SendMessage(hwnd, LVM_SCROLL, 0, 0);
    expect(TRUE, ret);
    ok_sequence(sequences, PARENT_SEQ_INDEX, scroll_parent_seq,
                "scroll notify 1", TRUE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    ret = SendMessage(hwnd, LVM_SCROLL, 1, 0);
    expect(TRUE, ret);
    ok_sequence(sequences, PARENT_SEQ_INDEX, scroll_parent_seq,
                "scroll notify 2", TRUE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    ret = SendMessage(hwnd, LVM_SCROLL, 1, 1);
    expect(TRUE, ret);
    ok_sequence(sequences, PARENT_SEQ_INDEX, scroll_parent_seq,
                "scroll notify 3", TRUE);

    DestroyWindow(hwnd);
}

static void test_LVS_EX_TRANSPARENTBKGND(void)
{
    HWND hwnd;
    DWORD ret;
    HDC hdc;

    hwnd = create_listview_control(LVS_REPORT);

    ret = SendMessage(hwnd, LVM_SETBKCOLOR, 0, RGB(0, 0, 0));
    expect(TRUE, ret);

    SendMessage(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_TRANSPARENTBKGND,
                                                    LVS_EX_TRANSPARENTBKGND);

    ret = SendMessage(hwnd, LVM_GETBKCOLOR, 0, 0);
    if (ret != CLR_NONE)
    {
        win_skip("LVS_EX_TRANSPARENTBKGND unsupported\n");
        DestroyWindow(hwnd);
        return;
    }

    /* try to set some back color and check this style bit */
    ret = SendMessage(hwnd, LVM_SETBKCOLOR, 0, RGB(0, 0, 0));
    expect(TRUE, ret);
    ret = SendMessage(hwnd, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
    ok(!(ret & LVS_EX_TRANSPARENTBKGND), "Expected LVS_EX_TRANSPARENTBKGND to unset\n");

    /* now test what this style actually does */
    SendMessage(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_TRANSPARENTBKGND,
                                                    LVS_EX_TRANSPARENTBKGND);

    hdc = GetWindowDC(hwndparent);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    SendMessageA(hwnd, WM_ERASEBKGND, (WPARAM)hdc, 0);
    ok_sequence(sequences, PARENT_SEQ_INDEX, lvs_ex_transparentbkgnd_seq,
                "LVS_EX_TRANSPARENTBKGND parent", FALSE);

    ReleaseDC(hwndparent, hdc);

    DestroyWindow(hwnd);
}

static void test_approximate_viewrect(void)
{
    HWND hwnd;
    DWORD ret;
    HIMAGELIST himl;
    HBITMAP hbmp;
    LVITEMA itema;
    static CHAR test[] = "abracadabra, a very long item label";

    hwnd = create_listview_control(LVS_ICON);
    himl = ImageList_Create(40, 40, 0, 4, 4);
    ok(himl != NULL, "failed to create imagelist\n");
    hbmp = CreateBitmap(40, 40, 1, 1, NULL);
    ok(hbmp != NULL, "failed to create bitmap\n");
    ret = ImageList_Add(himl, hbmp, 0);
    expect(0, ret);
    ret = SendMessage(hwnd, LVM_SETIMAGELIST, 0, (LPARAM)himl);
    expect(0, ret);

    itema.mask = LVIF_IMAGE;
    itema.iImage = 0;
    itema.iItem = 0;
    itema.iSubItem = 0;
    ret = SendMessage(hwnd, LVM_INSERTITEM, 0, (LPARAM)&itema);
    expect(0, ret);

    ret = SendMessage(hwnd, LVM_SETICONSPACING, 0, MAKELPARAM(75, 75));
    if (ret == 0)
    {
        /* version 4.0 */
        win_skip("LVM_SETICONSPACING unimplemented. Skipping.\n");
        return;
    }

    ret = SendMessage(hwnd, LVM_APPROXIMATEVIEWRECT, 11, MAKELPARAM(100,100));
    expect(MAKELONG(77,827), ret);

    ret = SendMessage(hwnd, LVM_SETICONSPACING, 0, MAKELPARAM(50, 50));
    ok(ret != 0, "got 0\n");

    ret = SendMessage(hwnd, LVM_APPROXIMATEVIEWRECT, 11, MAKELPARAM(100,100));
    expect(MAKELONG(102,302), ret);

    ret = SendMessage(hwnd, LVM_APPROXIMATEVIEWRECT, -1, MAKELPARAM(100,100));
    expect(MAKELONG(52,52), ret);

    itema.pszText = test;
    ret = SendMessage(hwnd, LVM_SETITEMTEXT, 0, (LPARAM)&itema);
    expect(TRUE, ret);
    ret = SendMessage(hwnd, LVM_APPROXIMATEVIEWRECT, -1, MAKELPARAM(100,100));
    expect(MAKELONG(52,52), ret);

    ret = SendMessage(hwnd, LVM_APPROXIMATEVIEWRECT, 0, MAKELPARAM(100,100));
    expect(MAKELONG(52,2), ret);
    ret = SendMessage(hwnd, LVM_APPROXIMATEVIEWRECT, 1, MAKELPARAM(100,100));
    expect(MAKELONG(52,52), ret);
    ret = SendMessage(hwnd, LVM_APPROXIMATEVIEWRECT, 2, MAKELPARAM(100,100));
    expect(MAKELONG(102,52), ret);
    ret = SendMessage(hwnd, LVM_APPROXIMATEVIEWRECT, 3, MAKELPARAM(100,100));
    expect(MAKELONG(102,102), ret);
    ret = SendMessage(hwnd, LVM_APPROXIMATEVIEWRECT, 4, MAKELPARAM(100,100));
    expect(MAKELONG(102,102), ret);
    ret = SendMessage(hwnd, LVM_APPROXIMATEVIEWRECT, 5, MAKELPARAM(100,100));
    expect(MAKELONG(102,152), ret);
    ret = SendMessage(hwnd, LVM_APPROXIMATEVIEWRECT, 6, MAKELPARAM(100,100));
    expect(MAKELONG(102,152), ret);
    ret = SendMessage(hwnd, LVM_APPROXIMATEVIEWRECT, 7, MAKELPARAM(160,100));
    expect(MAKELONG(152,152), ret);

    DestroyWindow(hwnd);
}

static void test_finditem(void)
{
    LVFINDINFOA fi;
    static char f[5];
    HWND hwnd;
    INT r;

    hwnd = create_listview_control(LVS_REPORT);
    insert_item(hwnd, 0);

    memset(&fi, 0, sizeof(fi));

    /* full string search, inserted text was "foo" */
    strcpy(f, "foo");
    fi.flags = LVFI_STRING;
    fi.psz = f;
    r = SendMessage(hwnd, LVM_FINDITEMA, -1, (LPARAM)&fi);
    expect(0, r);
    /* partial string search, inserted text was "foo" */
    strcpy(f, "fo");
    fi.flags = LVFI_STRING | LVFI_PARTIAL;
    fi.psz = f;
    r = SendMessage(hwnd, LVM_FINDITEMA, -1, (LPARAM)&fi);
    expect(0, r);
    /* partial string search, part after start char */
    strcpy(f, "oo");
    fi.flags = LVFI_STRING | LVFI_PARTIAL;
    fi.psz = f;
    r = SendMessage(hwnd, LVM_FINDITEMA, -1, (LPARAM)&fi);
    expect(-1, r);

    /* try with LVFI_SUBSTRING */
    strcpy(f, "fo");
    fi.flags = LVFI_SUBSTRING;
    fi.psz = f;
    r = SendMessage(hwnd, LVM_FINDITEMA, -1, (LPARAM)&fi);
    if (r == -1)
    {
        win_skip("LVFI_SUBSTRING not supported\n");
        DestroyWindow(hwnd);
        return;
    }
    expect(0, r);
    strcpy(f, "f");
    fi.flags = LVFI_SUBSTRING;
    fi.psz = f;
    r = SendMessage(hwnd, LVM_FINDITEMA, -1, (LPARAM)&fi);
    expect(0, r);
    strcpy(f, "o");
    fi.flags = LVFI_SUBSTRING;
    fi.psz = f;
    r = SendMessage(hwnd, LVM_FINDITEMA, -1, (LPARAM)&fi);
    expect(-1, r);

    strcpy(f, "f");
    fi.flags = LVFI_SUBSTRING | LVFI_STRING;
    fi.psz = f;
    r = SendMessage(hwnd, LVM_FINDITEMA, -1, (LPARAM)&fi);
    expect(0, r);

    DestroyWindow(hwnd);
}

static void test_LVS_EX_HEADERINALLVIEWS(void)
{
    HWND hwnd, header;
    DWORD style;

    hwnd = create_listview_control(LVS_ICON);

    SendMessage(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_HEADERINALLVIEWS,
                                                    LVS_EX_HEADERINALLVIEWS);

    header = (HWND)SendMessage(hwnd, LVM_GETHEADER, 0, 0);
    if (!IsWindow(header))
    {
        win_skip("LVS_EX_HEADERINALLVIEWS unsupported\n");
        DestroyWindow(hwnd);
        return;
    }

    /* LVS_NOCOLUMNHEADER works as before */
    style = GetWindowLongA(hwnd, GWL_STYLE);
    SetWindowLongW(hwnd, GWL_STYLE, style | LVS_NOCOLUMNHEADER);
    style = GetWindowLongA(header, GWL_STYLE);
    ok(style & HDS_HIDDEN, "Expected HDS_HIDDEN\n");
    style = GetWindowLongA(hwnd, GWL_STYLE);
    SetWindowLongW(hwnd, GWL_STYLE, style & ~LVS_NOCOLUMNHEADER);
    style = GetWindowLongA(header, GWL_STYLE);
    ok(!(style & HDS_HIDDEN), "Expected HDS_HIDDEN to be unset\n");

    /* try to remove style */
    SendMessage(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_HEADERINALLVIEWS, 0);
    header = (HWND)SendMessage(hwnd, LVM_GETHEADER, 0, 0);
    ok(IsWindow(header), "Expected header to be created\n");
    style = GetWindowLongA(header, GWL_STYLE);
    ok(!(style & HDS_HIDDEN), "HDS_HIDDEN not expected\n");

    DestroyWindow(hwnd);

    /* check other styles */
    hwnd = create_listview_control(LVS_LIST);
    SendMessage(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_HEADERINALLVIEWS,
                                                    LVS_EX_HEADERINALLVIEWS);
    header = (HWND)SendMessage(hwnd, LVM_GETHEADER, 0, 0);
    ok(IsWindow(header), "Expected header to be created\n");
    DestroyWindow(hwnd);

    hwnd = create_listview_control(LVS_SMALLICON);
    SendMessage(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_HEADERINALLVIEWS,
                                                    LVS_EX_HEADERINALLVIEWS);
    header = (HWND)SendMessage(hwnd, LVM_GETHEADER, 0, 0);
    ok(IsWindow(header), "Expected header to be created\n");
    DestroyWindow(hwnd);

    hwnd = create_listview_control(LVS_REPORT);
    SendMessage(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_HEADERINALLVIEWS,
                                                    LVS_EX_HEADERINALLVIEWS);
    header = (HWND)SendMessage(hwnd, LVM_GETHEADER, 0, 0);
    ok(IsWindow(header), "Expected header to be created\n");
    DestroyWindow(hwnd);
}

static void test_hover(void)
{
    HWND hwnd;
    DWORD r;

    hwnd = create_listview_control(LVS_ICON);

    /* test WM_MOUSEHOVER forwarding */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    r = SendMessage(hwnd, WM_MOUSEHOVER, 0, 0);
    expect(0, r);
    ok_sequence(sequences, PARENT_SEQ_INDEX, hover_parent, "NM_HOVER allow test", TRUE);
    g_block_hover = TRUE;
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    r = SendMessage(hwnd, WM_MOUSEHOVER, 0, 0);
    expect(0, r);
    ok_sequence(sequences, PARENT_SEQ_INDEX, hover_parent, "NM_HOVER block test", TRUE);
    g_block_hover = FALSE;

    r = SendMessage(hwnd, LVM_SETHOVERTIME, 0, 500);
    expect(HOVER_DEFAULT, r);
    r = SendMessage(hwnd, LVM_GETHOVERTIME, 0, 0);
    expect(500, r);

    DestroyWindow(hwnd);
}

static void test_destroynotify(void)
{
    HWND hwnd;

    hwnd = create_listview_control(LVS_REPORT);
    ok(hwnd != NULL, "failed to create listview window\n");

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    DestroyWindow(hwnd);
    ok_sequence(sequences, COMBINED_SEQ_INDEX, listview_destroy, "check destroy order", FALSE);
}

static void test_header_notification(void)
{
    static char textA[] = "newtext";
    HWND list, header;
    HDITEMA item;
    NMHEADER nmh;
    LVCOLUMNA col;
    DWORD ret;
    BOOL r;

    list = create_listview_control(LVS_REPORT);
    ok(list != NULL, "failed to create listview window\n");

    memset(&col, 0, sizeof(col));
    col.mask = LVCF_WIDTH;
    col.cx = 100;
    ret = SendMessage(list, LVM_INSERTCOLUMNA, 0, (LPARAM)&col);
    expect(0, ret);

    /* check list parent notification after header item changed,
       this test should be placed before header subclassing to avoid
       Listview -> Header messages to be logged */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    col.mask = LVCF_TEXT;
    col.pszText = textA;
    r = SendMessage(list, LVM_SETCOLUMNA, 0, (LPARAM)&col);
    expect(TRUE, r);

    ok_sequence(sequences, LISTVIEW_SEQ_INDEX, listview_header_changed_seq,
                "header notify, listview", FALSE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq,
                "header notify, parent", FALSE);

    header = subclass_header(list);

    ret = SendMessage(header, HDM_GETITEMCOUNT, 0, 0);
    expect(1, ret);

    memset(&item, 0, sizeof(item));
    item.mask = HDI_WIDTH;
    ret = SendMessage(header, HDM_GETITEMA, 0, (LPARAM)&item);
    expect(1, ret);
    expect(100, item.cxy);

    nmh.hdr.hwndFrom = header;
    nmh.hdr.idFrom = GetWindowLongPtr(header, GWLP_ID);
    nmh.hdr.code = HDN_ITEMCHANGEDA;
    nmh.iItem = 0;
    nmh.iButton = 0;
    item.mask = HDI_WIDTH;
    item.cxy = 50;
    nmh.pitem = &item;
    ret = SendMessage(list, WM_NOTIFY, 0, (LPARAM)&nmh);
    expect(0, ret);

    DestroyWindow(list);
}

static void test_createdragimage(void)
{
    HIMAGELIST himl;
    POINT pt;
    HWND list;

    list = create_listview_control(LVS_ICON);
    ok(list != NULL, "failed to create listview window\n");

    insert_item(list, 0);

    /* NULL point */
    himl = (HIMAGELIST)SendMessageA(list, LVM_CREATEDRAGIMAGE, 0, 0);
    ok(himl == NULL, "got %p\n", himl);

    himl = (HIMAGELIST)SendMessageA(list, LVM_CREATEDRAGIMAGE, 0, (LPARAM)&pt);
    ok(himl != NULL, "got %p\n", himl);
    ImageList_Destroy(himl);

    DestroyWindow(list);
}

static void test_dispinfo(void)
{
    static const char testA[] = "TEST";
    WCHAR buff[10];
    LVITEMA item;
    HWND hwnd;
    DWORD ret;

    hwnd = create_listview_control(LVS_ICON);
    ok(hwnd != NULL, "failed to create listview window\n");

    insert_item(hwnd, 0);

    memset(&item, 0, sizeof(item));
    item.pszText = LPSTR_TEXTCALLBACKA;
    ret = SendMessageA(hwnd, LVM_SETITEMTEXTA, 0, (LPARAM)&item);
    expect(1, ret);

    g_disp_A_to_W = TRUE;
    item.pszText = (char*)buff;
    item.cchTextMax = sizeof(buff)/sizeof(WCHAR);
    ret = SendMessageA(hwnd, LVM_GETITEMTEXTA, 0, (LPARAM)&item);
    ok(ret == sizeof(testA)-1, "got %d, expected 4\n", ret);
    g_disp_A_to_W = FALSE;

    ok(memcmp(item.pszText, testA, sizeof(testA)) == 0,
        "got %s, expected %s\n", item.pszText, testA);

    DestroyWindow(hwnd);
}

static void test_LVM_SETITEMTEXT(void)
{
    static char testA[] = "TEST";
    LVITEMA item;
    HWND hwnd;
    DWORD ret;

    hwnd = create_listview_control(LVS_ICON);
    ok(hwnd != NULL, "failed to create listview window\n");

    insert_item(hwnd, 0);

    /* null item pointer */
    ret = SendMessage(hwnd, LVM_SETITEMTEXTA, 0, 0);
    expect(FALSE, ret);

    ret = SendMessage(hwnd, LVM_SETITEMTEXTW, 0, 0);
    expect(FALSE, ret);

    /* index out of bounds */
    item.pszText = testA;
    item.cchTextMax = 0; /* ignored */
    item.iSubItem = 0;

    ret = SendMessageA(hwnd, LVM_SETITEMTEXTA, 1, (LPARAM)&item);
    expect(FALSE, ret);

    ret = SendMessageA(hwnd, LVM_SETITEMTEXTA, -1, (LPARAM)&item);
    expect(FALSE, ret);

    ret = SendMessageA(hwnd, LVM_SETITEMTEXTA, 0, (LPARAM)&item);
    expect(TRUE, ret);

    DestroyWindow(hwnd);
}

START_TEST(listview)
{
    HMODULE hComctl32;
    BOOL (WINAPI *pInitCommonControlsEx)(const INITCOMMONCONTROLSEX*);

    ULONG_PTR ctx_cookie;
    HANDLE hCtx;
    HWND hwnd;

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

    hwndparent = create_parent_window(FALSE);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    g_is_below_5 = is_below_comctl_5();

    test_header_notification();
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
    test_getitemrect();
    test_subitem_rect();
    test_sorting();
    test_ownerdata();
    test_norecompute();
    test_nosortheader();
    test_setredraw();
    test_hittest();
    test_getviewrect();
    test_getitemposition();
    test_columnscreation();
    test_editbox();
    test_notifyformat();
    test_indentation();
    test_getitemspacing();
    test_getcolumnwidth();
    test_approximate_viewrect();
    test_finditem();
    test_hover();
    test_destroynotify();
    test_createdragimage();
    test_dispinfo();
    test_LVM_SETITEMTEXT();

    if (!load_v6_module(&ctx_cookie, &hCtx))
    {
        DestroyWindow(hwndparent);
        return;
    }

    /* this is a XP SP3 failure workaround */
    hwnd = CreateWindowExA(0, WC_LISTVIEW, "foo",
                           WS_CHILD | WS_BORDER | WS_VISIBLE | LVS_REPORT,
                           0, 0, 100, 100,
                           hwndparent, NULL, GetModuleHandleA(NULL), NULL);
    if (!IsWindow(hwnd))
    {
        win_skip("FIXME: failed to create ListView window.\n");
        unload_v6_module(ctx_cookie, hCtx);
        DestroyWindow(hwndparent);
        return;
    }
    else
        DestroyWindow(hwnd);

    /* comctl32 version 6 tests start here */
    test_get_set_view();
    test_canceleditlabel();
    test_mapidindex();
    test_scrollnotify();
    test_LVS_EX_TRANSPARENTBKGND();
    test_LVS_EX_HEADERINALLVIEWS();

    unload_v6_module(ctx_cookie, hCtx);

    DestroyWindow(hwndparent);
}
