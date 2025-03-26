/*
 * ListView tests
 *
 * Copyright 2006 Mike McCormack for CodeWeavers
 * Copyright 2007 George Gov
 * Copyright 2009-2014 Nikolay Sivov
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

static HIMAGELIST (WINAPI *pImageList_Create)(int, int, UINT, int, int);
static BOOL (WINAPI *pImageList_Destroy)(HIMAGELIST);
static int (WINAPI *pImageList_Add)(HIMAGELIST, HBITMAP, HBITMAP);
static BOOL (WINAPI *p_TrackMouseEvent)(TRACKMOUSEEVENT *);

enum seq_index {
    PARENT_SEQ_INDEX,
    PARENT_FULL_SEQ_INDEX,
    PARENT_CD_SEQ_INDEX,
    LISTVIEW_SEQ_INDEX,
    EDITBOX_SEQ_INDEX,
    COMBINED_SEQ_INDEX,
    NUM_MSG_SEQUENCES
};

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
/* notification data for LVN_ITEMCHANGING */
static NMLISTVIEW g_nmlistview_changing;
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
static NMLVDISPINFOA g_editbox_disp_info;
/* when this is set focus will be tested on LVN_DELETEITEM */
static BOOL g_focus_test_LVN_DELETEITEM;
/* Whether to send WM_KILLFOCUS to the edit control during LVN_ENDLABELEDIT */
static BOOL g_WM_KILLFOCUS_on_LVN_ENDLABELEDIT;

static HWND subclass_editbox(HWND hwndListview);

static void init_functions(void)
{
    HMODULE hComCtl32 = LoadLibraryA("comctl32.dll");

#define X(f) p##f = (void*)GetProcAddress(hComCtl32, #f);
    X(ImageList_Create);
    X(ImageList_Destroy);
    X(ImageList_Add);
    X(_TrackMouseEvent);
#undef X
}

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
    { LVM_INSERTITEMA,    sent },
    { LVM_INSERTITEMA,    sent },
    { LVM_INSERTITEMA,    sent },
    { LVM_GETITEMCOUNT,   sent },
    { LVM_DELETEITEM,     sent|wparam, 2 },
    { WM_NCPAINT,         sent|optional },
    { WM_ERASEBKGND,      sent|optional },
    { LVM_GETITEMCOUNT,   sent },
    { LVM_DELETEALLITEMS, sent },
    { LVM_GETITEMCOUNT,   sent },
    { LVM_INSERTITEMA,    sent },
    { LVM_INSERTITEMA,    sent },
    { LVM_GETITEMCOUNT,   sent },
    { LVM_INSERTITEMA,    sent },
    { LVM_GETITEMCOUNT,   sent },
    { 0 }
};

static const struct message listview_itempos_seq[] = {
    { LVM_INSERTITEMA,     sent },
    { LVM_INSERTITEMA,     sent },
    { LVM_INSERTITEMA,     sent },
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
    { LVM_GETCOLUMNORDERARRAY, sent|id|wparam, 0, 0, LISTVIEW_ID },
    { HDM_GETORDERARRAY,       sent|id|wparam, 0, 0, HEADER_ID },
    { 0 }
};

static const struct message listview_setorderarray_seq[] = {
    { LVM_SETCOLUMNORDERARRAY, sent|id|wparam, 2, 0, LISTVIEW_ID },
    { HDM_SETORDERARRAY,       sent|id|wparam, 2, 0, HEADER_ID },
    { LVM_SETCOLUMNORDERARRAY, sent|id|wparam, 0, 0, LISTVIEW_ID },
    { HDM_SETORDERARRAY,       sent|id|wparam, 0, 0, HEADER_ID },
    { 0 }
};

static const struct message empty_seq[] = {
    { 0 }
};

static const struct message parent_focus_change_ownerdata_seq[] = {
    { WM_NOTIFY, sent|id, 0, 0, LVN_ITEMCHANGED },
    { WM_NOTIFY, sent|id, 0, 0, LVN_GETDISPINFOA },
    { 0 }
};

static const struct message forward_erasebkgnd_parent_seq[] = {
    { WM_ERASEBKGND, sent },
    { 0 }
};

static const struct message ownerdata_select_focus_parent_seq[] = {
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

static const struct message change_all_parent_seq[] = {
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

static const struct message changing_all_parent_seq[] = {
    { WM_NOTIFY, sent|id, 0, 0, LVN_ITEMCHANGING },
    { WM_NOTIFY, sent|id, 0, 0, LVN_ITEMCHANGING },
    { WM_NOTIFY, sent|id, 0, 0, LVN_ITEMCHANGING },
    { WM_NOTIFY, sent|id, 0, 0, LVN_ITEMCHANGING },
    { WM_NOTIFY, sent|id, 0, 0, LVN_ITEMCHANGING },
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

static const struct message getsubitemrect_seq[] = {
    { LVM_GETSUBITEMRECT, sent|id|wparam, -1, 0, LISTVIEW_ID },
    { HDM_GETITEMRECT, sent|id, 0, 0, HEADER_ID },
    { LVM_GETSUBITEMRECT, sent|id|wparam, 0, 0, LISTVIEW_ID },
    { HDM_GETITEMRECT, sent|id, 0, 0, HEADER_ID },
    { LVM_GETSUBITEMRECT, sent|id|wparam, -10, 0, LISTVIEW_ID },
    { HDM_GETITEMRECT, sent|id, 0, 0, HEADER_ID },
    { LVM_GETSUBITEMRECT, sent|id|wparam, 20, 0, LISTVIEW_ID },
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

static const struct message listview_ownerdata_destroy[] = {
    { 0x0090, sent|optional }, /* Vista */
    { WM_PARENTNOTIFY, sent },
    { WM_SHOWWINDOW, sent },
    { WM_WINDOWPOSCHANGING, sent },
    { WM_WINDOWPOSCHANGED, sent|optional },
    { WM_DESTROY, sent },
    { WM_NCDESTROY, sent },
    { 0 }
};

static const struct message listview_ownerdata_deleteall[] = {
    { LVM_DELETEALLITEMS, sent },
    { WM_NOTIFY, sent|id, 0, 0, LVN_DELETEALLITEMS },
    { 0 }
};

static const struct message listview_header_changed_seq[] = {
    { LVM_SETCOLUMNA, sent },
    { WM_NOTIFY, sent|id|defwinproc, 0, 0, LISTVIEW_ID },
    { WM_NOTIFY, sent|id|defwinproc, 0, 0, LISTVIEW_ID },
    { 0 }
};

static const struct message parent_header_click_seq[] = {
    { WM_NOTIFY, sent|id, 0, 0, LVN_COLUMNCLICK },
    { WM_NOTIFY, sent|id, 0, 0, HDN_ITEMCLICKA },
    { 0 }
};

static const struct message parent_header_divider_dclick_seq[] = {
    { WM_NOTIFY, sent|id, 0, 0, HDN_ITEMCHANGINGA },
    { WM_NOTIFY, sent|id, 0, 0, NM_CUSTOMDRAW },
    { WM_NOTIFY, sent|id, 0, 0, NM_CUSTOMDRAW },
    { WM_NOTIFY, sent|id, 0, 0, HDN_ITEMCHANGEDA },
    { WM_NOTIFY, sent|id, 0, 0, HDN_DIVIDERDBLCLICKA },
    { 0 }
};

static const struct message listview_set_imagelist[] = {
    { LVM_SETIMAGELIST, sent|id, 0, 0, LISTVIEW_ID },
    { 0 }
};

static const struct message listview_header_set_imagelist[] = {
    { LVM_SETIMAGELIST, sent|id, 0, 0, LISTVIEW_ID },
    { HDM_SETIMAGELIST, sent|id, 0, 0, HEADER_ID },
    { 0 }
};

static const struct message parent_insert_focused_seq[] = {
    { WM_NOTIFY, sent|id, 0, 0, LVN_ITEMCHANGING },
    { WM_NOTIFY, sent|id, 0, 0, LVN_ITEMCHANGING },
    { WM_NOTIFY, sent|id, 0, 0, LVN_ITEMCHANGED },
    { WM_NOTIFY, sent|id, 0, 0, LVN_ITEMCHANGED },
    { WM_NOTIFY, sent|id, 0, 0, LVN_INSERTITEM },
    { 0 }
};

static const struct message parent_report_cd_seq[] = {
    { WM_NOTIFY, sent|id|custdraw, 0, 0, NM_CUSTOMDRAW, CDDS_PREPAINT },
    { WM_NOTIFY, sent|id|custdraw, 0, 0, NM_CUSTOMDRAW, CDDS_ITEMPREPAINT },
    { WM_NOTIFY, sent|id|custdraw, 0, 0, NM_CUSTOMDRAW, CDDS_ITEMPREPAINT|CDDS_SUBITEM },
    { WM_NOTIFY, sent|id|custdraw, 0, 0, NM_CUSTOMDRAW, CDDS_ITEMPOSTPAINT|CDDS_SUBITEM },
    { WM_NOTIFY, sent|id|custdraw, 0, 0, NM_CUSTOMDRAW, CDDS_ITEMPREPAINT|CDDS_SUBITEM },
    { WM_NOTIFY, sent|id|custdraw, 0, 0, NM_CUSTOMDRAW, CDDS_ITEMPOSTPAINT|CDDS_SUBITEM },
    { WM_NOTIFY, sent|id|custdraw, 0, 0, NM_CUSTOMDRAW, CDDS_ITEMPOSTPAINT },
    { WM_NOTIFY, sent|id|custdraw, 0, 0, NM_CUSTOMDRAW, CDDS_POSTPAINT },
    { 0 }
};

static const struct message parent_list_cd_seq[] = {
    { WM_NOTIFY, sent|id|custdraw, 0, 0, NM_CUSTOMDRAW, CDDS_PREPAINT },
    { WM_NOTIFY, sent|id|custdraw, 0, 0, NM_CUSTOMDRAW, CDDS_ITEMPREPAINT },
    { WM_NOTIFY, sent|id|custdraw, 0, 0, NM_CUSTOMDRAW, CDDS_ITEMPOSTPAINT },
    { WM_NOTIFY, sent|id|custdraw, 0, 0, NM_CUSTOMDRAW, CDDS_POSTPAINT },
    { 0 }
};

static const struct message listview_end_label_edit[] = {
    { WM_NOTIFY,  sent|id, 0, 0, LVN_ENDLABELEDITA },
    { WM_NOTIFY,  sent|id, 0, 0, LVN_ITEMCHANGING},
    { WM_NOTIFY,  sent|id, 0, 0, LVN_ITEMCHANGED },
    { WM_NOTIFY,  sent|id|optional, 0, 0, NM_CUSTOMDRAW }, /* XP */
    { WM_NOTIFY,  sent|id, 0, 0, NM_SETFOCUS },
    { 0 }
};

static const struct message listview_end_label_edit_kill_focus[] = {
    { WM_NOTIFY,  sent|id, 0, 0, LVN_ENDLABELEDITA },
    { WM_COMMAND, sent|id|optional, 0, 0, EN_KILLFOCUS }, /* todo: not sent by wine yet */
    { WM_NOTIFY,  sent|id, 0, 0, LVN_ITEMCHANGING },
    { WM_NOTIFY,  sent|id, 0, 0, LVN_ITEMCHANGED },
    { WM_NOTIFY,  sent|id|optional, 0, 0, NM_CUSTOMDRAW }, /* XP */
    { WM_NOTIFY,  sent|id, 0, 0, NM_SETFOCUS },
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
    if (message == WM_COMMAND) msg.id = HIWORD(wParam);

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
          case LVN_BEGINLABELEDITA:
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
          case LVN_ENDLABELEDITA:
              {
              HWND edit;

              /* always accept new item text */
              NMLVDISPINFOA *di = (NMLVDISPINFOA*)lParam;
              g_editbox_disp_info = *di;

              /* edit control still available from this notification */
              edit = (HWND)SendMessageA(((NMHDR*)lParam)->hwndFrom, LVM_GETEDITCONTROL, 0, 0);
              ok(IsWindow(edit), "expected valid edit control handle\n");
              ok((GetWindowLongA(edit, GWL_STYLE) & ES_MULTILINE) == 0, "edit is multiline\n");

              if (g_WM_KILLFOCUS_on_LVN_ENDLABELEDIT)
                  SendMessageA(edit, WM_KILLFOCUS, 0, 0);

              return TRUE;
              }
          case LVN_ITEMCHANGING:
              {
                  NMLISTVIEW *nmlv = (NMLISTVIEW*)lParam;
                  g_nmlistview_changing = *nmlv;
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
          case LVN_DELETEITEM:
              if (g_focus_test_LVN_DELETEITEM)
              {
                  NMLISTVIEW *nmlv = (NMLISTVIEW*)lParam;
                  UINT state;

                  state = SendMessageA(((NMHDR*)lParam)->hwndFrom, LVM_GETITEMSTATE, nmlv->iItem, LVIS_FOCUSED);
                  ok(state == 0, "got state %x\n", state);
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
    if (IsWindowUnicode(hwnd))
        ret = DefWindowProcW(hwnd, message, wParam, lParam);
    else
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
        clsW.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
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
        clsA.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
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
    hwnd = CreateWindowExA(0, WC_LISTVIEWA, "foo",
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

static BOOL is_win_xp(void)
{
    HWND hwnd, header;
    BOOL ret;

    hwnd = create_listview_control(LVS_ICON);
    SendMessageA(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_HEADERINALLVIEWS, LVS_EX_HEADERINALLVIEWS);
    header = (HWND)SendMessageA(hwnd, LVM_GETHEADER, 0, 0);
    ret = !IsWindow(header);

    DestroyWindow(hwnd);

    return ret;
}

static LRESULT WINAPI header_subclass_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WNDPROC oldproc = (WNDPROC)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    static LONG defwndproc_counter = 0;
    struct message msg = { 0 };
    LRESULT ret;

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

    hwnd = (HWND)SendMessageA(hwndListview, LVM_GETHEADER, 0, 0);
    oldproc = (WNDPROC)SetWindowLongPtrA(hwnd, GWLP_WNDPROC,
                                         (LONG_PTR)header_subclass_proc);
    SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)oldproc);

    return hwnd;
}

static LRESULT WINAPI editbox_subclass_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WNDPROC oldproc = (WNDPROC)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    static LONG defwndproc_counter = 0;
    struct message msg = { 0 };
    LRESULT ret;

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

    hwnd = (HWND)SendMessageA(hwndListview, LVM_GETEDITCONTROL, 0, 0);
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

    ret = SendMessageA(hwnd, LVM_HITTEST, 0, (LPARAM)&lpht);

    todo_wine_if(todo_item)
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

    ret = SendMessageA(hwnd, LVM_SUBITEMHITTEST, 0, (LPARAM)&lpht);

    todo_wine_if(todo_item)
    {
        ok_(__FILE__, line)(ret == item, "Expected %d retval, got %d\n", item, ret);
        ok_(__FILE__, line)(lpht.iItem == item, "Expected %d item, got %d\n", item, lpht.iItem);
    }

    todo_wine_if(todo_subitem)
        ok_(__FILE__, line)(lpht.iSubItem == subitem, "Expected subitem %d, got %d\n", subitem, lpht.iSubItem);

    todo_wine_if(todo_flags)
        ok_(__FILE__, line)(lpht.flags == flags, "Expected flags 0x%x, got 0x%x\n", flags, lpht.flags);
}

#define test_lvm_subitemhittest(a,b,c,d,e,f,g,h,i) test_lvm_subitemhittest_(a,b,c,d,e,f,g,h,i,__LINE__)

static void test_images(void)
{
    HWND hwnd;
    INT r;
    LVITEMA item;
    HIMAGELIST himl;
    HBITMAP hbmp;
    RECT r1, r2;
    static CHAR hello[] = "hello";

    himl = pImageList_Create(40, 40, 0, 4, 4);
    ok(himl != NULL, "failed to create imagelist\n");

    hbmp = CreateBitmap(40, 40, 1, 1, NULL);
    ok(hbmp != NULL, "failed to create bitmap\n");

    r = pImageList_Add(himl, hbmp, 0);
    ok(r == 0, "should be zero\n");

    hwnd = CreateWindowExA(0, WC_LISTVIEWA, "foo", LVS_OWNERDRAWFIXED,
                10, 10, 100, 200, hwndparent, NULL, NULL, NULL);
    ok(hwnd != NULL, "failed to create listview window\n");

    r = SendMessageA(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
                    LVS_EX_UNDERLINEHOT | LVS_EX_FLATSB | LVS_EX_ONECLICKACTIVATE);

    ok(r == 0, "should return zero\n");

    r = SendMessageA(hwnd, LVM_SETIMAGELIST, 0, (LPARAM)himl);
    ok(r == 0, "should return zero\n");

    r = SendMessageA(hwnd, LVM_SETICONSPACING, 0, MAKELONG(100,50));
    ok(r != 0, "got 0\n");

    /* returns dimensions */

    r = SendMessageA(hwnd, LVM_GETITEMCOUNT, 0, 0);
    ok(r == 0, "should be zero items\n");

    item.mask = LVIF_IMAGE | LVIF_TEXT;
    item.iItem = 0;
    item.iSubItem = 1;
    item.iImage = 0;
    item.pszText = 0;
    r = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item);
    ok(r == -1, "should fail\n");

    item.iSubItem = 0;
    item.pszText = hello;
    r = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item);
    ok(r == 0, "should not fail\n");

    SetRect(&r1, LVIR_ICON, 0, 0, 0);
    r = SendMessageA(hwnd, LVM_GETITEMRECT, 0, (LPARAM) &r1);
    expect(1, r);

    r = SendMessageA(hwnd, LVM_DELETEALLITEMS, 0, 0);
    ok(r == TRUE, "should not fail\n");

    item.iSubItem = 0;
    item.pszText = hello;
    r = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item);
    ok(r == 0, "should not fail\n");

    SetRect(&r2, LVIR_ICON, 0, 0, 0);
    r = SendMessageA(hwnd, LVM_GETITEMRECT, 0, (LPARAM) &r2);
    expect(1, r);

    ok(EqualRect(&r1, &r2), "rectangle should be the same\n");

    DestroyWindow(hwnd);

    /* I_IMAGECALLBACK set for item, try to get image with invalid subitem. */
    hwnd = create_listview_control(LVS_REPORT);
    ok(hwnd != NULL, "Failed to create listview.\n");

    memset(&item, 0, sizeof(item));
    item.mask = LVIF_IMAGE;
    item.iImage = I_IMAGECALLBACK;
    r = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM)&item);
    ok(!r, "Failed to insert item.\n");

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    memset(&item, 0, sizeof(item));
    item.mask = LVIF_IMAGE;
    r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM)&item);
    ok(r, "Failed to get item.\n");

    ok_sequence(sequences, PARENT_SEQ_INDEX, single_getdispinfo_parent_seq, "get image dispinfo 1", FALSE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    memset(&item, 0, sizeof(item));
    item.mask = LVIF_IMAGE;
    item.iSubItem = 1;
    r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM)&item);
    ok(r, "Failed to get item.\n");

    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq, "get image dispinfo 2", FALSE);

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

    hwnd = CreateWindowExA(0, WC_LISTVIEWA, "foo", LVS_REPORT,
                10, 10, 100, 200, hwndparent, NULL, NULL, NULL);
    ok(hwnd != NULL, "failed to create listview window\n");

    /* first without LVS_EX_CHECKBOXES set and an item and check that state is preserved */
    item.mask = LVIF_TEXT | LVIF_STATE;
    item.stateMask = 0xffff;
    item.state = 0xfccc;
    item.iItem = 0;
    item.iSubItem = 0;
    item.pszText = text;
    r = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item);
    expect(0, r);

    item.iItem = 0;
    item.mask = LVIF_STATE;
    item.stateMask = 0xffff;
    r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(1, r);
    ok(item.state == 0xfccc, "state %x\n", item.state);

    /* Don't set LVIF_STATE */
    item.mask = LVIF_TEXT;
    item.stateMask = 0xffff;
    item.state = 0xfccc;
    item.iItem = 1;
    item.iSubItem = 0;
    item.pszText = text;
    r = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item);
    expect(1, r);

    item.iItem = 1;
    item.mask = LVIF_STATE;
    item.stateMask = 0xffff;
    r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(1, r);
    ok(item.state == 0, "state %x\n", item.state);

    r = SendMessageA(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);
    expect(0, r);

    /* Having turned on checkboxes, check that all existing items are set to 0x1000 (unchecked) */
    item.iItem = 0;
    item.mask = LVIF_STATE;
    item.stateMask = 0xffff;
    r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
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
    r = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item);
    expect(2, r);

    item.iItem = 2;
    item.mask = LVIF_STATE;
    item.stateMask = 0xffff;
    r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(1, r);
    ok(item.state == 0x1000, "state %x\n", item.state);

    /* Add a further item this time specifying a state and still its state goes to 0x1000 */
    item.iItem = 3;
    item.mask = LVIF_TEXT | LVIF_STATE;
    item.stateMask = 0xffff;
    item.state = 0x2aaa;
    item.pszText = text3;
    r = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item);
    expect(3, r);

    item.iItem = 3;
    item.mask = LVIF_STATE;
    item.stateMask = 0xffff;
    r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(1, r);
    ok(item.state == 0x1aaa, "state %x\n", item.state);

    /* Set an item's state to checked */
    item.iItem = 3;
    item.mask = LVIF_STATE;
    item.stateMask = 0xf000;
    item.state = 0x2000;
    r = SendMessageA(hwnd, LVM_SETITEMA, 0, (LPARAM) &item);
    expect(1, r);

    item.iItem = 3;
    item.mask = LVIF_STATE;
    item.stateMask = 0xffff;
    r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(1, r);
    ok(item.state == 0x2aaa, "state %x\n", item.state);

    /* Check that only the bits we asked for are returned,
     * and that all the others are set to zero
     */
    item.iItem = 3;
    item.mask = LVIF_STATE;
    item.stateMask = 0xf000;
    item.state = 0xffff;
    r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(1, r);
    ok(item.state == 0x2000, "state %x\n", item.state);

    /* Set the style again and check that doesn't change an item's state */
    r = SendMessageA(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);
    ok(r == LVS_EX_CHECKBOXES, "ret %x\n", r);

    item.iItem = 3;
    item.mask = LVIF_STATE;
    item.stateMask = 0xffff;
    r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(1, r);
    ok(item.state == 0x2aaa, "state %x\n", item.state);

    /* Unsetting the checkbox extended style doesn't change an item's state */
    r = SendMessageA(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_CHECKBOXES, 0);
    ok(r == LVS_EX_CHECKBOXES, "ret %x\n", r);

    item.iItem = 3;
    item.mask = LVIF_STATE;
    item.stateMask = 0xffff;
    r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(1, r);
    ok(item.state == 0x2aaa, "state %x\n", item.state);

    /* Now setting the style again will change an item's state */
    r = SendMessageA(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);
    expect(0, r);

    item.iItem = 3;
    item.mask = LVIF_STATE;
    item.stateMask = 0xffff;
    r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(1, r);
    ok(item.state == 0x1aaa, "state %x\n", item.state);

    /* Toggle checkbox tests (bug 9934) */
    memset (&item, 0xcc, sizeof(item));
    item.mask = LVIF_STATE;
    item.iItem = 3;
    item.iSubItem = 0;
    item.state = LVIS_FOCUSED;
    item.stateMask = LVIS_FOCUSED;
    r = SendMessageA(hwnd, LVM_SETITEMA, 0, (LPARAM) &item);
    expect(1, r);

    item.iItem = 3;
    item.mask = LVIF_STATE;
    item.stateMask = 0xffff;
    r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(1, r);
    ok(item.state == 0x1aab, "state %x\n", item.state);

    r = SendMessageA(hwnd, WM_KEYDOWN, VK_SPACE, 0);
    expect(0, r);
    r = SendMessageA(hwnd, WM_KEYUP, VK_SPACE, 0);
    expect(0, r);

    item.iItem = 3;
    item.mask = LVIF_STATE;
    item.stateMask = 0xffff;
    r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(1, r);
    ok(item.state == 0x2aab, "state %x\n", item.state);

    r = SendMessageA(hwnd, WM_KEYDOWN, VK_SPACE, 0);
    expect(0, r);
    r = SendMessageA(hwnd, WM_KEYUP, VK_SPACE, 0);
    expect(0, r);

    item.iItem = 3;
    item.mask = LVIF_STATE;
    item.stateMask = 0xffff;
    r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(1, r);
    ok(item.state == 0x1aab, "state %x\n", item.state);

    DestroyWindow(hwnd);
}

static void insert_column(HWND hwnd, int idx)
{
    LVCOLUMNA column;
    INT rc;

    memset(&column, 0xcc, sizeof(column));
    column.mask = LVCF_SUBITEM;
    column.iSubItem = idx;

    rc = SendMessageA(hwnd, LVM_INSERTCOLUMNA, idx, (LPARAM)&column);
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

    rc = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM)&item);
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

    hwnd = CreateWindowExA(0, WC_LISTVIEWA, "foo", LVS_REPORT,
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
    r = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item);
    expect(0, r);
    /* get */
    memset (&item, 0xcc, sizeof (item));
    item.mask = LVIF_STATE;
    item.stateMask = LVIS_SELECTED;
    item.state = 0;
    item.iItem = 0;
    item.iSubItem = 0;
    r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(1, r);
    ok(item.state & LVIS_SELECTED, "Expected LVIS_SELECTED\n");
    r = SendMessageA(hwnd, LVM_DELETEITEM, 0, 0);
    ok(r, "got %d\n", r);

    /* LVIS_SELECTED with zero stateMask */
    /* set */
    memset (&item, 0, sizeof (item));
    item.mask = LVIF_STATE;
    item.state = LVIS_FOCUSED;
    item.stateMask = 0;
    item.iItem = 0;
    item.iSubItem = 0;
    r = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item);
    expect(0, r);
    /* get */
    memset (&item, 0xcc, sizeof (item));
    item.mask = LVIF_STATE;
    item.stateMask = LVIS_FOCUSED;
    item.state = 0;
    item.iItem = 0;
    item.iSubItem = 0;
    r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(1, r);
    ok(item.state & LVIS_FOCUSED, "Expected LVIS_FOCUSED\n");
    r = SendMessageA(hwnd, LVM_DELETEITEM, 0, 0);
    ok(r, "got %d\n", r);

    /* LVIS_CUT with LVIS_FOCUSED stateMask */
    /* set */
    memset (&item, 0, sizeof (item));
    item.mask = LVIF_STATE;
    item.state = LVIS_CUT;
    item.stateMask = LVIS_FOCUSED;
    item.iItem = 0;
    item.iSubItem = 0;
    r = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item);
    expect(0, r);
    /* get */
    memset (&item, 0xcc, sizeof (item));
    item.mask = LVIF_STATE;
    item.stateMask = LVIS_CUT;
    item.state = 0;
    item.iItem = 0;
    item.iSubItem = 0;
    r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(1, r);
    ok(item.state & LVIS_CUT, "Expected LVIS_CUT\n");
    r = SendMessageA(hwnd, LVM_DELETEITEM, 0, 0);
    ok(r, "got %d\n", r);

    /* Insert an item with just a param */
    memset (&item, 0xcc, sizeof (item));
    item.mask = LVIF_PARAM;
    item.iItem = 0;
    item.iSubItem = 0;
    item.lParam = lparamTest;
    r = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item);
    expect(0, r);

    /* Test getting of the param */
    memset (&item, 0xcc, sizeof (item));
    item.mask = LVIF_PARAM;
    item.iItem = 0;
    item.iSubItem = 0;
    r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(1, r);
    ok(item.lParam == lparamTest, "got lParam %lx, expected %lx\n", item.lParam, lparamTest);

    /* Set up a subitem */
    memset (&item, 0xcc, sizeof (item));
    item.mask = LVIF_TEXT;
    item.iItem = 0;
    item.iSubItem = 1;
    item.pszText = text;
    r = SendMessageA(hwnd, LVM_SETITEMA, 0, (LPARAM) &item);
    expect(1, r);

    item.mask = LVIF_TEXT;
    item.iItem = 0;
    item.iSubItem = 1;
    item.pszText = buffA;
    item.cchTextMax = sizeof(buffA);
    r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(1, r);
    ok(!memcmp(item.pszText, text, sizeof(text)), "got text %s, expected %s\n", item.pszText, text);

    /* set up with extra flag */
    /* 1. reset subitem text */
    item.mask = LVIF_TEXT;
    item.iItem = 0;
    item.iSubItem = 1;
    item.pszText = NULL;
    r = SendMessageA(hwnd, LVM_SETITEMA, 0, (LPARAM) &item);
    expect(1, r);

    item.mask = LVIF_TEXT;
    item.iItem = 0;
    item.iSubItem = 1;
    item.pszText = buffA;
    buffA[0] = 'a';
    item.cchTextMax = sizeof(buffA);
    r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(1, r);
    ok(item.pszText[0] == 0, "got %p\n", item.pszText);

    /* 2. set new text with extra flag specified */
    item.mask = LVIF_TEXT | LVIF_DI_SETITEM;
    item.iItem = 0;
    item.iSubItem = 1;
    item.pszText = text;
    r = SendMessageA(hwnd, LVM_SETITEMA, 0, (LPARAM) &item);
    ok(r == 1 || broken(r == 0) /* NT4 */, "ret %d\n", r);

    if (r == 1)
    {
        item.mask = LVIF_TEXT;
        item.iItem = 0;
        item.iSubItem = 1;
        item.pszText = buffA;
        buffA[0] = 'a';
        item.cchTextMax = sizeof(buffA);
        r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
        expect(1, r);
        ok(!memcmp(item.pszText, text, sizeof(text)), "got %s, expected %s\n", item.pszText, text);
    }

    /* Query param from subitem: returns main item param */
    memset (&item, 0xcc, sizeof (item));
    item.mask = LVIF_PARAM;
    item.iItem = 0;
    item.iSubItem = 1;
    r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(1, r);
    ok(item.lParam == lparamTest, "got lParam %lx, expected %lx\n", item.lParam, lparamTest);

    /* Set up param on first subitem: no effect */
    memset (&item, 0xcc, sizeof (item));
    item.mask = LVIF_PARAM;
    item.iItem = 0;
    item.iSubItem = 1;
    item.lParam = lparamTest+1;
    r = SendMessageA(hwnd, LVM_SETITEMA, 0, (LPARAM) &item);
    expect(0, r);

    /* Query param from subitem again: should still return main item param */
    memset (&item, 0xcc, sizeof (item));
    item.mask = LVIF_PARAM;
    item.iItem = 0;
    item.iSubItem = 1;
    r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(1, r);
    ok(item.lParam == lparamTest, "got lParam %lx, expected %lx\n", item.lParam, lparamTest);

    /**** Some tests of state highlighting ****/
    memset (&item, 0xcc, sizeof (item));
    item.mask = LVIF_STATE;
    item.iItem = 0;
    item.iSubItem = 0;
    item.state = LVIS_SELECTED;
    item.stateMask = LVIS_SELECTED | LVIS_DROPHILITED;
    r = SendMessageA(hwnd, LVM_SETITEMA, 0, (LPARAM) &item);
    expect(1, r);
    item.iSubItem = 1;
    item.state = LVIS_DROPHILITED;
    r = SendMessageA(hwnd, LVM_SETITEMA, 0, (LPARAM) &item);
    expect(1, r);

    memset (&item, 0xcc, sizeof (item));
    item.mask = LVIF_STATE;
    item.iItem = 0;
    item.iSubItem = 0;
    item.stateMask = -1;
    r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(1, r);
    ok(item.state == LVIS_SELECTED, "got state %x, expected %x\n", item.state, LVIS_SELECTED);
    item.iSubItem = 1;
    r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(1, r);
    todo_wine ok(item.state == LVIS_DROPHILITED, "got state %x, expected %x\n", item.state, LVIS_DROPHILITED);

    /* some notnull but meaningless masks */
    memset (&item, 0, sizeof(item));
    item.mask = LVIF_NORECOMPUTE;
    item.iItem = 0;
    item.iSubItem = 0;
    r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(1, r);
    memset (&item, 0, sizeof(item));
    item.mask = LVIF_DI_SETITEM;
    item.iItem = 0;
    item.iSubItem = 0;
    r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(1, r);

    /* set text to callback value already having it */
    r = SendMessageA(hwnd, LVM_DELETEALLITEMS, 0, 0);
    expect(TRUE, r);
    memset (&item, 0, sizeof (item));
    item.mask  = LVIF_TEXT;
    item.pszText = LPSTR_TEXTCALLBACKA;
    item.iItem = 0;
    r = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item);
    expect(0, r);
    memset (&item, 0, sizeof (item));

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    item.pszText = LPSTR_TEXTCALLBACKA;
    r = SendMessageA(hwnd, LVM_SETITEMTEXTA, 0 , (LPARAM) &item);
    expect(TRUE, r);

    ok_sequence(sequences, PARENT_SEQ_INDEX, textcallback_set_again_parent_seq,
                "check callback text comparison rule", FALSE);

    DestroyWindow(hwnd);
}

static void test_columns(void)
{
    HWND hwnd, header;
    LVCOLUMNA column;
    LVITEMA item;
    INT order[2];
    CHAR buff[5];
    DWORD rc;

    hwnd = CreateWindowExA(0, WC_LISTVIEWA, "foo", LVS_LIST,
                10, 10, 100, 200, hwndparent, NULL, NULL, NULL);
    ok(hwnd != NULL, "failed to create listview window\n");

    header = (HWND)SendMessageA(hwnd, LVM_GETHEADER, 0, 0);
    ok(header == NULL, "got %p\n", header);

    rc = SendMessageA(hwnd, LVM_GETCOLUMNORDERARRAY, 2, (LPARAM)&order);
    ok(rc == 0, "got %d\n", rc);

    header = (HWND)SendMessageA(hwnd, LVM_GETHEADER, 0, 0);
    ok(header == NULL, "got %p\n", header);

    DestroyWindow(hwnd);

    hwnd = CreateWindowExA(0, WC_LISTVIEWA, "foo", LVS_REPORT,
                10, 10, 100, 200, hwndparent, NULL, NULL, NULL);
    ok(hwnd != NULL, "failed to create listview window\n");

    rc = SendMessageA(hwnd, LVM_DELETECOLUMN, -1, 0);
    ok(!rc, "got %d\n", rc);

    rc = SendMessageA(hwnd, LVM_DELETECOLUMN, 0, 0);
    ok(!rc, "got %d\n", rc);

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

    rc = SendMessageA(hwnd, LVM_GETCOLUMNORDERARRAY, 0, 0);
    expect(0, rc);

    ok_sequence(sequences, LISTVIEW_SEQ_INDEX, listview_getorderarray_seq, "get order array", FALSE);

    /* LVM_SETCOLUMNORDERARRAY */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    order[0] = 0;
    order[1] = 1;
    rc = SendMessageA(hwnd, LVM_SETCOLUMNORDERARRAY, 2, (LPARAM)&order);
    expect(1, rc);

    rc = SendMessageA(hwnd, LVM_SETCOLUMNORDERARRAY, 0, 0);
    expect(0, rc);

    ok_sequence(sequences, LISTVIEW_SEQ_INDEX, listview_setorderarray_seq, "set order array", FALSE);

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

    /* Columns are not created right away. */
    hwnd = create_listview_control(LVS_REPORT);
    ok(hwnd != NULL, "Failed to create a listview window.\n");

    insert_item(hwnd, 0);

    header = (HWND)SendMessageA(hwnd, LVM_GETHEADER, 0, 0);
    ok(IsWindow(header), "Expected header handle.\n");
    rc = SendMessageA(header, HDM_GETITEMCOUNT, 0, 0);
    ok(!rc, "Unexpected column count.\n");

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
        CREATESTRUCTA *lpcs = (CREATESTRUCTA*)lParam;
        lpcs->style |= LVS_REPORT;
    }
    ret = CallWindowProcA(listviewWndProc, hwnd, uMsg, wParam, lParam);
    if (uMsg == WM_CREATE) SendMessageA(hwnd, LVM_SETIMAGELIST, 0, (LPARAM)test_create_imagelist);
    return ret;
}

/* Header creation is delayed in classic implementation. */
#define TEST_NO_HEADER(a) test_header_presence_(a, FALSE, __LINE__)
#define TEST_HEADER_EXPECTED(a) test_header_presence_(a, TRUE, __LINE__)
#define TEST_NO_HEADER2(a, b) test_header_presence_(a, b, __LINE__)
static void test_header_presence_(HWND hwnd, BOOL present, int line)
{
    HWND header = (HWND)SendMessageA(hwnd, LVM_GETHEADER, 0, 0);

    if (present)
    {
        ok_(__FILE__, line)(IsWindow(header), "Header should have been created.\n");
        if (header) /* FIXME: remove when todo's are fixed */
            ok_(__FILE__, line)(header == GetDlgItem(hwnd, 0), "Dialog item expected.\n");
    }
    else
    {
        ok_(__FILE__, line)(!IsWindow(header), "Header shouldn't be created.\n");
        ok_(__FILE__, line)(NULL == GetDlgItem(hwnd, 0), "NULL dialog item expected.\n");
    }
}

static void test_create(BOOL is_version_6)
{
    static const WCHAR testtextW[] = {'t','e','s','t',' ','t','e','x','t',0};
    char buff[16];
    HWND hList;
    HWND hHeader;
    LONG_PTR ret;
    LONG r;
    LVCOLUMNA col;
    RECT rect;
    WNDCLASSEXA cls;
    DWORD style;
    ATOM class;

    if (is_win_xp() && is_version_6)
    {
        win_skip("Skipping some tests on XP.\n");
        return;
    }

    cls.cbSize = sizeof(WNDCLASSEXA);
    r = GetClassInfoExA(GetModuleHandleA(NULL), WC_LISTVIEWA, &cls);
    ok(r, "Failed to get class info.\n");
    listviewWndProc = cls.lpfnWndProc;
    cls.lpfnWndProc = create_test_wndproc;
    cls.lpszClassName = "MyListView32";
    class = RegisterClassExA(&cls);
    ok(class, "Failed to register class.\n");

    test_create_imagelist = pImageList_Create(16, 16, 0, 5, 10);
    hList = CreateWindowA("MyListView32", "Test", WS_VISIBLE, 0, 0, 100, 100, NULL, NULL, GetModuleHandleA(NULL), 0);
    ok((HIMAGELIST)SendMessageA(hList, LVM_GETIMAGELIST, 0, 0) == test_create_imagelist, "Image list not obtained\n");
    hHeader = (HWND)SendMessageA(hList, LVM_GETHEADER, 0, 0);
    ok(IsWindow(hHeader) && IsWindowVisible(hHeader), "Listview not in report mode\n");
    ok(hHeader == GetDlgItem(hList, 0), "Expected header as dialog item\n");
    DestroyWindow(hList);

    /* header isn't created on LVS_ICON and LVS_LIST styles */
    hList = CreateWindowA(WC_LISTVIEWA, "Test", WS_VISIBLE, 0, 0, 100, 100, NULL, NULL, GetModuleHandleA(NULL), 0);
    TEST_NO_HEADER(hList);

    /* insert column */
    memset(&col, 0, sizeof(LVCOLUMNA));
    col.mask = LVCF_WIDTH;
    col.cx = 100;
    r = SendMessageA(hList, LVM_INSERTCOLUMNA, 0, (LPARAM)&col);
    expect(0, r);
    TEST_HEADER_EXPECTED(hList);
    hHeader = (HWND)SendMessageA(hList, LVM_GETHEADER, 0, 0);
    style = GetWindowLongA(hHeader, GWL_STYLE);
    ok(!(style & HDS_HIDDEN), "Not expected HDS_HIDDEN\n");
    DestroyWindow(hList);

    hList = CreateWindowA(WC_LISTVIEWA, "Test", WS_VISIBLE|LVS_LIST, 0, 0, 100, 100, NULL, NULL,
                           GetModuleHandleA(NULL), 0);
    TEST_NO_HEADER(hList);
    /* insert column */
    memset(&col, 0, sizeof(LVCOLUMNA));
    col.mask = LVCF_WIDTH;
    col.cx = 100;
    r = SendMessageA(hList, LVM_INSERTCOLUMNA, 0, (LPARAM)&col);
    expect(0, r);
    TEST_HEADER_EXPECTED(hList);
    DestroyWindow(hList);

    /* try to switch LVS_ICON -> LVS_REPORT and back LVS_ICON -> LVS_REPORT */
    hList = CreateWindowA(WC_LISTVIEWA, "Test", WS_VISIBLE, 0, 0, 100, 100, NULL, NULL,
                           GetModuleHandleA(NULL), 0);
    ret = SetWindowLongPtrA(hList, GWL_STYLE, GetWindowLongPtrA(hList, GWL_STYLE) | LVS_REPORT);
    ok(ret & WS_VISIBLE, "Style wrong, should have WS_VISIBLE\n");
    TEST_HEADER_EXPECTED(hList);
    ret = SetWindowLongPtrA(hList, GWL_STYLE, GetWindowLongA(hList, GWL_STYLE) & ~LVS_REPORT);
    ok((ret & WS_VISIBLE) && (ret & LVS_REPORT), "Style wrong, should have WS_VISIBLE|LVS_REPORT\n");
    TEST_HEADER_EXPECTED(hList);
    DestroyWindow(hList);

    /* try to switch LVS_LIST -> LVS_REPORT and back LVS_LIST -> LVS_REPORT */
    hList = CreateWindowA(WC_LISTVIEWA, "Test", WS_VISIBLE|LVS_LIST, 0, 0, 100, 100, NULL, NULL,
                           GetModuleHandleA(NULL), 0);
    ret = SetWindowLongPtrA(hList, GWL_STYLE,
                           (GetWindowLongPtrA(hList, GWL_STYLE) & ~LVS_LIST) | LVS_REPORT);
    ok(((ret & WS_VISIBLE) && (ret & LVS_LIST)), "Style wrong, should have WS_VISIBLE|LVS_LIST\n");
    TEST_HEADER_EXPECTED(hList);
    ret = SetWindowLongPtrA(hList, GWL_STYLE, (GetWindowLongPtrA(hList, GWL_STYLE) & ~LVS_REPORT) | LVS_LIST);
    ok(((ret & WS_VISIBLE) && (ret & LVS_REPORT)), "Style wrong, should have WS_VISIBLE|LVS_REPORT\n");
    TEST_HEADER_EXPECTED(hList);
    DestroyWindow(hList);

    /* LVS_REPORT without WS_VISIBLE */
    hList = CreateWindowA(WC_LISTVIEWA, "Test", LVS_REPORT, 0, 0, 100, 100, NULL, NULL,
                          GetModuleHandleA(NULL), 0);
    hHeader = (HWND)SendMessageA(hList, LVM_GETHEADER, 0, 0);
todo_wine_if(is_version_6)
    TEST_NO_HEADER2(hList, is_version_6);

    /* insert column */
    memset(&col, 0, sizeof(LVCOLUMNA));
    col.mask = LVCF_WIDTH;
    col.cx = 100;
    r = SendMessageA(hList, LVM_INSERTCOLUMNA, 0, (LPARAM)&col);
    expect(0, r);
    TEST_HEADER_EXPECTED(hList);
    DestroyWindow(hList);

    /* LVS_REPORT without WS_VISIBLE, try to show it */
    hList = CreateWindowA(WC_LISTVIEWA, "Test", LVS_REPORT, 0, 0, 100, 100, NULL, NULL,
                          GetModuleHandleA(NULL), 0);
todo_wine_if(is_version_6)
    TEST_NO_HEADER2(hList, is_version_6);

    ShowWindow(hList, SW_SHOW);
    TEST_HEADER_EXPECTED(hList);
    DestroyWindow(hList);

    /* LVS_REPORT with LVS_NOCOLUMNHEADER */
    hList = CreateWindowA(WC_LISTVIEWA, "Test", LVS_REPORT|LVS_NOCOLUMNHEADER|WS_VISIBLE,
                          0, 0, 100, 100, NULL, NULL, GetModuleHandleA(NULL), 0);
    TEST_HEADER_EXPECTED(hList);
    hHeader = (HWND)SendMessageA(hList, LVM_GETHEADER, 0, 0);
    /* HDS_DRAGDROP set by default */
    ok(GetWindowLongPtrA(hHeader, GWL_STYLE) & HDS_DRAGDROP, "Expected header to have HDS_DRAGDROP\n");
    DestroyWindow(hList);

    /* setting LVS_EX_HEADERDRAGDROP creates header */
    hList = CreateWindowA(WC_LISTVIEWA, "Test", LVS_REPORT, 0, 0, 100, 100, NULL, NULL,
                          GetModuleHandleA(NULL), 0);
todo_wine_if(is_version_6)
    TEST_NO_HEADER2(hList, is_version_6);

    SendMessageA(hList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_HEADERDRAGDROP);
    TEST_HEADER_EXPECTED(hList);
    DestroyWindow(hList);

    /* setting LVS_EX_GRIDLINES creates header */
    hList = CreateWindowA(WC_LISTVIEWA, "Test", LVS_REPORT, 0, 0, 100, 100, NULL, NULL,
                          GetModuleHandleA(NULL), 0);
todo_wine_if(is_version_6)
    TEST_NO_HEADER2(hList, is_version_6);

    SendMessageA(hList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_GRIDLINES);
    TEST_HEADER_EXPECTED(hList);
    DestroyWindow(hList);

    /* setting LVS_EX_FULLROWSELECT creates header */
    hList = CreateWindowA(WC_LISTVIEWA, "Test", LVS_REPORT, 0, 0, 100, 100, NULL, NULL,
                          GetModuleHandleA(NULL), 0);
todo_wine_if(is_version_6)
    TEST_NO_HEADER2(hList, is_version_6);
    SendMessageA(hList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);
    TEST_HEADER_EXPECTED(hList);
    DestroyWindow(hList);

    /* not report style accepts LVS_EX_HEADERDRAGDROP too */
    hList = create_listview_control(LVS_ICON);
    SendMessageA(hList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_HEADERDRAGDROP);
    r = SendMessageA(hList, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
    ok(r & LVS_EX_HEADERDRAGDROP, "Expected LVS_EX_HEADERDRAGDROP to be set\n");
    DestroyWindow(hList);

    /* requesting header info with LVM_GETSUBITEMRECT doesn't create it */
    hList = CreateWindowA(WC_LISTVIEWA, "Test", LVS_REPORT, 0, 0, 100, 100, NULL, NULL,
                          GetModuleHandleA(NULL), 0);
todo_wine_if(is_version_6)
    TEST_NO_HEADER2(hList, is_version_6);

    SetRect(&rect, LVIR_BOUNDS, 1, -10, -10);
    r = SendMessageA(hList, LVM_GETSUBITEMRECT, -1, (LPARAM)&rect);
    ok(r == 1, "Unexpected ret value %d.\n", r);
    /* right value contains garbage, probably because header columns are not set up */
    ok(rect.bottom >= 0, "Unexpected rectangle.\n");

todo_wine_if(is_version_6)
    TEST_NO_HEADER2(hList, is_version_6);
    DestroyWindow(hList);

    /* WM_MEASUREITEM should be sent when created with LVS_OWNERDRAWFIXED */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    hList = create_listview_control(LVS_OWNERDRAWFIXED | LVS_REPORT);
    ok_sequence(sequences, PARENT_SEQ_INDEX, create_ownerdrawfixed_parent_seq,
                "created with LVS_OWNERDRAWFIXED|LVS_REPORT - parent seq", FALSE);
    DestroyWindow(hList);

    /* Test that window text is preserved. */
    hList = CreateWindowExA(0, WC_LISTVIEWA, "test text", WS_CHILD | WS_BORDER | WS_VISIBLE,
        0, 0, 100, 100, hwndparent, NULL, GetModuleHandleA(NULL), NULL);
    ok(hList != NULL, "Failed to create ListView window.\n");
    *buff = 0;
    GetWindowTextA(hList, buff, sizeof(buff));
    ok(!strcmp(buff, "test text"), "Unexpected window text %s.\n", buff);
    DestroyWindow(hList);

    hList = CreateWindowExW(0, WC_LISTVIEWW, testtextW, WS_CHILD | WS_BORDER | WS_VISIBLE,
        0, 0, 100, 100, hwndparent, NULL, GetModuleHandleA(NULL), NULL);
    ok(hList != NULL, "Failed to create ListView window.\n");
    *buff = 0;
    GetWindowTextA(hList, buff, sizeof(buff));
    ok(!strcmp(buff, "test text"), "Unexpected window text %s.\n", buff);
    DestroyWindow(hList);

    r = UnregisterClassA("MyListView32", NULL);
    ok(r, "Failed to unregister test class.\n");
}

static void test_redraw(void)
{
    HWND hwnd;
    HDC hdc;
    BOOL res;
    DWORD r;
    RECT rect;

    hwnd = create_listview_control(LVS_REPORT);
    subclass_header(hwnd);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);
    ok_sequence(sequences, LISTVIEW_SEQ_INDEX, redraw_listview_seq, "redraw listview", FALSE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    /* forward WM_ERASEBKGND to parent on CLR_NONE background color */
    /* 1. Without backbuffer */
    res = SendMessageA(hwnd, LVM_SETBKCOLOR, 0, CLR_NONE);
    expect(TRUE, res);

    hdc = GetWindowDC(hwndparent);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    r = SendMessageA(hwnd, WM_ERASEBKGND, (WPARAM)hdc, 0);
    ok(r == 1, "Expected not zero result\n");
    ok_sequence(sequences, PARENT_FULL_SEQ_INDEX, forward_erasebkgnd_parent_seq,
                "forward WM_ERASEBKGND on CLR_NONE", FALSE);

    res = SendMessageA(hwnd, LVM_SETBKCOLOR, 0, CLR_DEFAULT);
    expect(TRUE, res);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    r = SendMessageA(hwnd, WM_ERASEBKGND, (WPARAM)hdc, 0);
    expect(1, r);
    ok_sequence(sequences, PARENT_FULL_SEQ_INDEX, empty_seq,
                "don't forward WM_ERASEBKGND on non-CLR_NONE", FALSE);

    /* 2. With backbuffer */
    SendMessageA(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_DOUBLEBUFFER,
                                                     LVS_EX_DOUBLEBUFFER);
    res = SendMessageA(hwnd, LVM_SETBKCOLOR, 0, CLR_NONE);
    expect(TRUE, res);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    r = SendMessageA(hwnd, WM_ERASEBKGND, (WPARAM)hdc, 0);
    expect(1, r);
    ok_sequence(sequences, PARENT_FULL_SEQ_INDEX, forward_erasebkgnd_parent_seq,
                "forward WM_ERASEBKGND on CLR_NONE", FALSE);

    res = SendMessageA(hwnd, LVM_SETBKCOLOR, 0, CLR_DEFAULT);
    expect(TRUE, res);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    r = SendMessageA(hwnd, WM_ERASEBKGND, (WPARAM)hdc, 0);
    todo_wine expect(1, r);
    ok_sequence(sequences, PARENT_FULL_SEQ_INDEX, empty_seq,
                "don't forward WM_ERASEBKGND on non-CLR_NONE", FALSE);

    ReleaseDC(hwndparent, hdc);

    /* test setting the window style to what it already was */
    UpdateWindow(hwnd);
    SetWindowLongA(hwnd, GWL_STYLE, GetWindowLongA(hwnd, GWL_STYLE));
    GetUpdateRect(hwnd, &rect, FALSE);
    ok(rect.left == 0 && rect.top == 0 && rect.right == 0 && rect.bottom == 0,
       "Expected empty update rect, got %s\n", wine_dbgstr_rect(&rect));

    DestroyWindow(hwnd);
}

static LRESULT WINAPI cd_wndproc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    COLORREF clr, c0ffee = RGB(0xc0, 0xff, 0xee);

    if(message == WM_NOTIFY) {
        NMHDR *nmhdr = (NMHDR*)lParam;
        if(nmhdr->code == NM_CUSTOMDRAW) {
            NMLVCUSTOMDRAW *nmlvcd = (NMLVCUSTOMDRAW*)nmhdr;
            BOOL showsel_always = !!(GetWindowLongA(nmlvcd->nmcd.hdr.hwndFrom, GWL_STYLE) & LVS_SHOWSELALWAYS);
            BOOL is_selected = !!(nmlvcd->nmcd.uItemState & CDIS_SELECTED);
            struct message msg;

            msg.message = message;
            msg.flags = sent|wparam|lparam|custdraw;
            msg.wParam = wParam;
            msg.lParam = lParam;
            msg.id = nmhdr->code;
            msg.stage = nmlvcd->nmcd.dwDrawStage;
            add_message(sequences, PARENT_CD_SEQ_INDEX, &msg);

            switch(nmlvcd->nmcd.dwDrawStage) {
            case CDDS_PREPAINT:
                SetBkColor(nmlvcd->nmcd.hdc, c0ffee);
                return CDRF_NOTIFYITEMDRAW|CDRF_NOTIFYPOSTPAINT;
            case CDDS_ITEMPREPAINT:
                clr = GetBkColor(nmlvcd->nmcd.hdc);
                todo_wine_if(nmlvcd->iSubItem)
                    ok(clr == c0ffee, "Unexpected background color %#x.\n", clr);
                nmlvcd->clrTextBk = CLR_DEFAULT;
                nmlvcd->clrText = RGB(0, 255, 0);
                return CDRF_NOTIFYSUBITEMDRAW|CDRF_NOTIFYPOSTPAINT;
            case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
                clr = GetBkColor(nmlvcd->nmcd.hdc);
                todo_wine_if(showsel_always && is_selected && nmlvcd->iSubItem)
                {
                    ok(nmlvcd->clrTextBk == CLR_DEFAULT, "Unexpected text background %#x.\n", nmlvcd->clrTextBk);
                    ok(nmlvcd->clrText == RGB(0, 255, 0), "Unexpected text color %#x.\n", nmlvcd->clrText);
                }
                if (showsel_always && is_selected && nmlvcd->iSubItem)
                    ok(clr == GetSysColor(COLOR_3DFACE), "Unexpected background color %#x.\n", clr);
                else
                todo_wine_if(nmlvcd->iSubItem)
                    ok(clr == c0ffee, "clr=%.8x\n", clr);
                return CDRF_NOTIFYPOSTPAINT;
            case CDDS_ITEMPOSTPAINT | CDDS_SUBITEM:
                clr = GetBkColor(nmlvcd->nmcd.hdc);
                if (showsel_always && is_selected)
                    ok(clr == GetSysColor(COLOR_3DFACE), "Unexpected background color %#x.\n", clr);
                else
                {
                todo_wine
                    ok(clr == c0ffee, "Unexpected background color %#x.\n", clr);
                }

                todo_wine_if(showsel_always)
                {
                    ok(nmlvcd->clrTextBk == CLR_DEFAULT, "Unexpected text background color %#x.\n", nmlvcd->clrTextBk);
                    ok(nmlvcd->clrText == RGB(0, 255, 0), "got 0x%x\n", nmlvcd->clrText);
                }
                return CDRF_DODEFAULT;
            }
            return CDRF_DODEFAULT;
        }
    }

    return DefWindowProcA(hwnd, message, wParam, lParam);
}

static void test_customdraw(void)
{
    HWND hwnd;
    WNDPROC oldwndproc;
    LVITEMA item;

    hwnd = create_listview_control(LVS_REPORT);

    insert_column(hwnd, 0);
    insert_column(hwnd, 1);
    insert_item(hwnd, 0);

    oldwndproc = (WNDPROC)SetWindowLongPtrA(hwndparent, GWLP_WNDPROC,
                                           (LONG_PTR)cd_wndproc);

    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);

    /* message tests */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);
    ok_sequence(sequences, PARENT_CD_SEQ_INDEX, parent_report_cd_seq, "parent customdraw, LVS_REPORT", FALSE);

    /* Check colors when item is selected. */
    item.mask = LVIF_STATE;
    item.stateMask = LVIS_SELECTED;
    item.state = LVIS_SELECTED;
    SendMessageA(hwnd, LVM_SETITEMSTATE, 0, (LPARAM)&item);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);
    ok_sequence(sequences, PARENT_CD_SEQ_INDEX, parent_report_cd_seq,
            "parent customdraw, item selected, LVS_REPORT, selection", FALSE);

    SetWindowLongW(hwnd, GWL_STYLE, GetWindowLongW(hwnd, GWL_STYLE) | LVS_SHOWSELALWAYS);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);
    ok_sequence(sequences, PARENT_CD_SEQ_INDEX, parent_report_cd_seq,
            "parent customdraw, item selected, LVS_SHOWSELALWAYS, LVS_REPORT", FALSE);

    DestroyWindow(hwnd);

    hwnd = create_listview_control(LVS_LIST);

    insert_column(hwnd, 0);
    insert_column(hwnd, 1);
    insert_item(hwnd, 0);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);
    ok_sequence(sequences, PARENT_CD_SEQ_INDEX, parent_list_cd_seq, "parent customdraw, LVS_LIST", FALSE);

    SetWindowLongPtrA(hwndparent, GWLP_WNDPROC, (LONG_PTR)oldwndproc);
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

    r = SendMessageA(hwnd, WM_NOTIFYFORMAT, (WPARAM)hwndparent, NF_REQUERY);
    expect(NFR_ANSI, r);

    /* reset the icon spacing to defaults */
    SendMessageA(hwnd, LVM_SETICONSPACING, 0, MAKELPARAM(-1, -1));

    /* now we can request what the defaults are */
    r = SendMessageA(hwnd, LVM_SETICONSPACING, 0, MAKELPARAM(-1, -1));
    w = LOWORD(r);
    h = HIWORD(r);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    r = SendMessageA(hwnd, LVM_SETICONSPACING, 0, MAKELPARAM(20, 30));
    ok(r == MAKELONG(w, h) ||
       broken(r == MAKELONG(w, w)), /* win98 */
       "Expected %d, got %d\n", MAKELONG(w, h), r);

    r = SendMessageA(hwnd, LVM_SETICONSPACING, 0, MAKELPARAM(25, 35));
    expect(MAKELONG(20,30), r);

    r = SendMessageA(hwnd, LVM_SETICONSPACING, 0, MAKELPARAM(-1,-1));
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

        r = SendMessageA(hwnd, LVM_SETBKCOLOR, 0, color);
        expect(TRUE, r);
        r = SendMessageA(hwnd, LVM_GETBKCOLOR, 0, 0);
        expect(color, r);

        r = SendMessageA(hwnd, LVM_SETTEXTCOLOR, 0, color);
        expect (TRUE, r);
        r = SendMessageA(hwnd, LVM_GETTEXTCOLOR, 0, 0);
        expect(color, r);

        r = SendMessageA(hwnd, LVM_SETTEXTBKCOLOR, 0, color);
        expect(TRUE, r);
        r = SendMessageA(hwnd, LVM_GETTEXTBKCOLOR, 0, 0);
        expect(color, r);
    }

    ok_sequence(sequences, LISTVIEW_SEQ_INDEX, listview_color_seq, "test color seq", FALSE);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    /* invalidation test done separately to avoid a message chain mess */
    r = ValidateRect(hwnd, NULL);
    expect(TRUE, r);
    r = SendMessageA(hwnd, LVM_SETBKCOLOR, 0, colors[0]);
    expect(TRUE, r);

    rect.right = rect.bottom = 1;
    r = GetUpdateRect(hwnd, &rect, TRUE);
    todo_wine expect(FALSE, r);
    ok(rect.right == 0 && rect.bottom == 0, "got update rectangle\n");

    r = ValidateRect(hwnd, NULL);
    expect(TRUE, r);
    r = SendMessageA(hwnd, LVM_SETTEXTCOLOR, 0, colors[0]);
    expect(TRUE, r);

    rect.right = rect.bottom = 1;
    r = GetUpdateRect(hwnd, &rect, TRUE);
    todo_wine expect(FALSE, r);
    ok(rect.right == 0 && rect.bottom == 0, "got update rectangle\n");

    r = ValidateRect(hwnd, NULL);
    expect(TRUE, r);
    r = SendMessageA(hwnd, LVM_SETTEXTBKCOLOR, 0, colors[0]);
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

    LVITEMA item0;
    LVITEMA item1;
    LVITEMA item2;
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

    r = SendMessageA(hwnd, LVM_GETITEMCOUNT, 0, 0);
    expect(0, r);

    /* [item0] */
    item0.mask = LVIF_TEXT;
    item0.iItem = 0;
    item0.iSubItem = 0;
    item0.pszText = item0text;
    r = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item0);
    expect(0, r);

    /* [item0, item1] */
    item1.mask = LVIF_TEXT;
    item1.iItem = 1;
    item1.iSubItem = 0;
    item1.pszText = item1text;
    r = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item1);
    expect(1, r);

    /* [item0, item1, item2] */
    item2.mask = LVIF_TEXT;
    item2.iItem = 2;
    item2.iSubItem = 0;
    item2.pszText = item2text;
    r = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item2);
    expect(2, r);

    r = SendMessageA(hwnd, LVM_GETITEMCOUNT, 0, 0);
    expect(3, r);

    /* [item0, item1] */
    r = SendMessageA(hwnd, LVM_DELETEITEM, 2, 0);
    expect(TRUE, r);

    r = SendMessageA(hwnd, LVM_GETITEMCOUNT, 0, 0);
    expect(2, r);

    /* [] */
    r = SendMessageA(hwnd, LVM_DELETEALLITEMS, 0, 0);
    expect(TRUE, r);

    r = SendMessageA(hwnd, LVM_GETITEMCOUNT, 0, 0);
    expect(0, r);

    /* [item0] */
    r = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item1);
    expect(0, r);

    /* [item0, item1] */
    r = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item1);
    expect(1, r);

    r = SendMessageA(hwnd, LVM_GETITEMCOUNT, 0, 0);
    expect(2, r);

    /* [item0, item1, item2] */
    r = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item2);
    expect(2, r);

    r = SendMessageA(hwnd, LVM_GETITEMCOUNT, 0, 0);
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

    LVITEMA item0;
    LVITEMA item1;
    LVITEMA item2;
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
    r = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item0);
    expect(0, r);

    /* [item0, item1] */
    item1.mask = LVIF_TEXT;
    item1.iItem = 1;
    item1.iSubItem = 0;
    item1.pszText = item1text;
    r = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item1);
    expect(1, r);

    /* [item0, item1, item2] */
    item2.mask = LVIF_TEXT;
    item2.iItem = 2;
    item2.iSubItem = 0;
    item2.pszText = item2text;
    r = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item2);
    expect(2, r);

    r = SendMessageA(hwnd, LVM_SETITEMPOSITION, 1, MAKELPARAM(10,5));
    expect(TRUE, r);
    r = SendMessageA(hwnd, LVM_GETITEMPOSITION, 1, (LPARAM) &position);
    expect(TRUE, r);
    expect2(10, 5, position.x, position.y);

    r = SendMessageA(hwnd, LVM_SETITEMPOSITION, 2, MAKELPARAM(0,0));
    expect(TRUE, r);
    r = SendMessageA(hwnd, LVM_GETITEMPOSITION, 2, (LPARAM) &position);
    expect(TRUE, r);
    expect2(0, 0, position.x, position.y);

    r = SendMessageA(hwnd, LVM_SETITEMPOSITION, 0, MAKELPARAM(20,20));
    expect(TRUE, r);
    r = SendMessageA(hwnd, LVM_GETITEMPOSITION, 0, (LPARAM) &position);
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

    r = SendMessageA(hwnd, LVM_GETORIGIN, 0, (LPARAM)&position);
    expect(TRUE, r);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    DestroyWindow(hwnd);

    hwnd = create_listview_control(LVS_SMALLICON);
    ok(hwnd != NULL, "failed to create a listview window\n");
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    r = SendMessageA(hwnd, LVM_GETORIGIN, 0, (LPARAM)&position);
    expect(TRUE, r);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    DestroyWindow(hwnd);

    hwnd = create_listview_control(LVS_LIST);
    ok(hwnd != NULL, "failed to create a listview window\n");
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    r = SendMessageA(hwnd, LVM_GETORIGIN, 0, (LPARAM)&position);
    expect(FALSE, r);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    DestroyWindow(hwnd);

    hwnd = create_listview_control(LVS_REPORT);
    ok(hwnd != NULL, "failed to create a listview window\n");
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    r = SendMessageA(hwnd, LVM_GETORIGIN, 0, (LPARAM)&position);
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
    int i, j;
    static const int items=5;
    DWORD item_count;
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

    for (i = 0; i < items; i++)
        insert_item(hwnd, 0);

    item_count = SendMessageA(hwnd, LVM_GETITEMCOUNT, 0, 0);
    expect(items, item_count);

    r = SendMessageA(hwnd, LVM_SETSELECTIONMARK, 0, -1);
    ok(r == -1, "got %d\n", r);

    r = SendMessageA(hwnd, LVM_SETSELECTIONMARK, 0, 0);
    ok(r == -1, "got %d\n", r);

    r = SendMessageA(hwnd, LVM_SETSELECTIONMARK, 0, 0);
    ok(r == 0, "got %d\n", r);

    /* out of range index */
    r = SendMessageA(hwnd, LVM_SETSELECTIONMARK, 0, items);
    ok(r == 0, "got %d\n", r);

    r = SendMessageA(hwnd, LVM_GETSELECTIONMARK, 0, 0);
    ok(r == 0, "got %d\n", r);

    r = SendMessageA(hwnd, LVM_SETSELECTIONMARK, 0, -2);
    ok(r == 0, "got %d\n", r);

    r = SendMessageA(hwnd, LVM_GETSELECTIONMARK, 0, 0);
    ok(r == 0, "got %d\n", r);

    for (i = 0; i < ARRAY_SIZE(task_list); i++) {
        DWORD selected_count;
        LVITEMA item;

        task = task_list[i];

	/* deselect all items */
        item.state = 0;
        item.stateMask = LVIS_SELECTED;
        r = SendMessageA(hwnd, LVM_SETITEMSTATE, -1, (LPARAM)&item);
        ok(r, "got %d\n", r);
	SendMessageA(hwnd, LVM_SETSELECTIONMARK, 0, -1);

	/* set initial position */
        r = SendMessageA(hwnd, LVM_SETSELECTIONMARK, 0, (task.initPos == -1 ? item_count -1 : task.initPos));
        ok(r, "got %d\n", r);

        item.state = LVIS_SELECTED;
        item.stateMask = LVIS_SELECTED;
        r = SendMessageA(hwnd, LVM_SETITEMSTATE, task.initPos == -1 ? item_count-1 : task.initPos, (LPARAM)&item);
        ok(r, "got %d\n", r);

	selected_count = SendMessageA(hwnd, LVM_GETSELECTEDCOUNT, 0, 0);
	ok(selected_count == 1, "expected 1, got %d\n", selected_count);

	/* Set SHIFT key pressed */
        GetKeyboardState(kstate);
        kstate[VK_SHIFT]=0x80;
        SetKeyboardState(kstate);

	for (j=1;j<=(task.count == -1 ? item_count : task.count);j++) {
	    r = SendMessageA(hwnd, WM_KEYDOWN, task.loopVK, 0);
	    expect(0,r);
	    r = SendMessageA(hwnd, WM_KEYUP, task.loopVK, 0);
	    expect(0,r);
	}

	selected_count = SendMessageA(hwnd, LVM_GETSELECTEDCOUNT, 0, 0);

	ok((task.result == -1 ? item_count : task.result) == selected_count,
            "Failed multiple selection %s. There should be %d selected items (is %d)\n",
            task.descr, item_count, selected_count);

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
    item_count = SendMessageA(hwnd, LVM_GETITEMCOUNT, 0, 0);
    expect(items,item_count);

    /* try with NULL pointer */
    r = SendMessageA(hwnd, LVM_SETITEMSTATE, 0, 0);
    expect(FALSE, r);

    /* select all, check notifications */
    item.state = 0;
    item.stateMask = LVIS_SELECTED;
    r = SendMessageA(hwnd, LVM_SETITEMSTATE, -1, (LPARAM)&item);
    ok(r, "got %d\n", r);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    item.stateMask = LVIS_SELECTED;
    item.state     = LVIS_SELECTED;
    r = SendMessageA(hwnd, LVM_SETITEMSTATE, -1, (LPARAM)&item);
    expect(TRUE, r);

    ok_sequence(sequences, PARENT_SEQ_INDEX, change_all_parent_seq,
                "select all notification", FALSE);

    /* select all again (all selected already) */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    memset(&g_nmlistview_changing, 0xcc, sizeof(g_nmlistview_changing));

    item.stateMask = LVIS_SELECTED;
    item.state     = LVIS_SELECTED;
    r = SendMessageA(hwnd, LVM_SETITEMSTATE, -1, (LPARAM)&item);
    expect(TRUE, r);

    ok(g_nmlistview_changing.uNewState == LVIS_SELECTED, "got 0x%x\n", g_nmlistview_changing.uNewState);
    ok(g_nmlistview_changing.uOldState == LVIS_SELECTED, "got 0x%x\n", g_nmlistview_changing.uOldState);
    ok(g_nmlistview_changing.uChanged == LVIF_STATE, "got 0x%x\n", g_nmlistview_changing.uChanged);

    ok_sequence(sequences, PARENT_SEQ_INDEX, changing_all_parent_seq,
                "select all notification 2", FALSE);

    /* deselect all items */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    item.state = 0;
    item.stateMask = LVIS_SELECTED;
    r = SendMessageA(hwnd, LVM_SETITEMSTATE, -1, (LPARAM)&item);
    ok(r, "got %d\n", r);

    ok_sequence(sequences, PARENT_SEQ_INDEX, change_all_parent_seq,
                "deselect all notification", FALSE);

    /* deselect all items again */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    item.state = 0;
    item.stateMask = LVIS_SELECTED;
    SendMessageA(hwnd, LVM_SETITEMSTATE, -1, (LPARAM)&item);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq, "deselect all notification 2", FALSE);

    /* any non-zero state value does the same */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    memset(&g_nmlistview_changing, 0xcc, sizeof(g_nmlistview_changing));

    item.stateMask = LVIS_SELECTED;
    item.state     = LVIS_CUT;
    r = SendMessageA(hwnd, LVM_SETITEMSTATE, -1, (LPARAM)&item);
    expect(TRUE, r);

    ok(g_nmlistview_changing.uNewState == 0, "got 0x%x\n", g_nmlistview_changing.uNewState);
    ok(g_nmlistview_changing.uOldState == 0, "got 0x%x\n", g_nmlistview_changing.uOldState);
    ok(g_nmlistview_changing.uChanged == LVIF_STATE, "got 0x%x\n", g_nmlistview_changing.uChanged);

    ok_sequence(sequences, PARENT_SEQ_INDEX, changing_all_parent_seq,
                "set state all notification 3", FALSE);

    r = SendMessageA(hwnd, LVM_SETSELECTIONMARK, 0, -1);
    ok(r, "got %d\n", r);
    for (i = 0; i < 3; i++) {
        item.state = LVIS_SELECTED;
        item.stateMask = LVIS_SELECTED;
        r = SendMessageA(hwnd, LVM_SETITEMSTATE, i, (LPARAM)&item);
        ok(r, "got %d\n", r);
    }

    r = SendMessageA(hwnd, LVM_GETSELECTEDCOUNT, 0, 0);
    expect(3, r);
    r = SendMessageA(hwnd, LVM_GETSELECTIONMARK, 0, 0);
    expect(-1, r);

    style = GetWindowLongPtrA(hwnd, GWL_STYLE);
    ok(!(style & LVS_SINGLESEL), "LVS_SINGLESEL isn't expected\n");
    SetWindowLongPtrA(hwnd, GWL_STYLE, style | LVS_SINGLESEL);
    /* check that style is accepted */
    style = GetWindowLongPtrA(hwnd, GWL_STYLE);
    ok(style & LVS_SINGLESEL, "LVS_SINGLESEL expected\n");

    for (i=0;i<3;i++) {
        r = SendMessageA(hwnd, LVM_GETITEMSTATE, i, LVIS_SELECTED);
        ok(r & LVIS_SELECTED, "Expected item %d to be selected\n", i);
    }
    r = SendMessageA(hwnd, LVM_GETSELECTEDCOUNT, 0, 0);
    expect(3, r);
    r = SendMessageA(hwnd, LVM_GETSELECTIONMARK, 0, 0);
    ok(r == -1, "got %d\n", r);

    /* select one more */
    item.state = LVIS_SELECTED;
    item.stateMask = LVIS_SELECTED;
    r = SendMessageA(hwnd, LVM_SETITEMSTATE, 3, (LPARAM)&item);
    ok(r, "got %d\n", r);

    for (i=0;i<3;i++) {
        r = SendMessageA(hwnd, LVM_GETITEMSTATE, i, LVIS_SELECTED);
        ok(!(r & LVIS_SELECTED), "Expected item %d to be unselected\n", i);
    }

    r = SendMessageA(hwnd, LVM_GETITEMSTATE, 3, LVIS_SELECTED);
    ok(r & LVIS_SELECTED, "Expected item %d to be selected\n", i);

    r = SendMessageA(hwnd, LVM_GETSELECTEDCOUNT, 0, 0);
    expect(1, r);
    r = SendMessageA(hwnd, LVM_GETSELECTIONMARK, 0, 0);
    expect(-1, r);

    /* try to select all on LVS_SINGLESEL */
    memset(&item, 0, sizeof(item));
    item.stateMask = LVIS_SELECTED;
    r = SendMessageA(hwnd, LVM_SETITEMSTATE, -1, (LPARAM)&item);
    expect(TRUE, r);
    r = SendMessageA(hwnd, LVM_SETSELECTIONMARK, 0, -1);
    ok(r == -1, "got %d\n", r);

    item.stateMask = LVIS_SELECTED;
    item.state     = LVIS_SELECTED;
    r = SendMessageA(hwnd, LVM_SETITEMSTATE, -1, (LPARAM)&item);
    expect(FALSE, r);

    r = SendMessageA(hwnd, LVM_GETSELECTEDCOUNT, 0, 0);
    expect(0, r);
    r = SendMessageA(hwnd, LVM_GETSELECTIONMARK, 0, 0);
    expect(-1, r);

    /* try to deselect all on LVS_SINGLESEL */
    item.stateMask = LVIS_SELECTED;
    item.state     = LVIS_SELECTED;
    r = SendMessageA(hwnd, LVM_SETITEMSTATE, 0, (LPARAM)&item);
    expect(TRUE, r);

    item.stateMask = LVIS_SELECTED;
    item.state     = 0;
    r = SendMessageA(hwnd, LVM_SETITEMSTATE, -1, (LPARAM)&item);
    expect(TRUE, r);
    r = SendMessageA(hwnd, LVM_GETSELECTEDCOUNT, 0, 0);
    expect(0, r);

    /* 1. selection mark is update when new focused item is set */
    style = GetWindowLongPtrA(hwnd, GWL_STYLE);
    SetWindowLongPtrA(hwnd, GWL_STYLE, style & ~LVS_SINGLESEL);

    r = SendMessageA(hwnd, LVM_SETSELECTIONMARK, 0, -1);
    expect(-1, r);

    item.stateMask = LVIS_FOCUSED;
    item.state     = LVIS_FOCUSED;
    r = SendMessageA(hwnd, LVM_SETITEMSTATE, 0, (LPARAM)&item);
    expect(TRUE, r);

    r = SendMessageA(hwnd, LVM_GETSELECTIONMARK, 0, 0);
    expect(0, r);

    /* it's not updated if already set */
    item.stateMask = LVIS_FOCUSED;
    item.state     = LVIS_FOCUSED;
    r = SendMessageA(hwnd, LVM_SETITEMSTATE, 1, (LPARAM)&item);
    expect(TRUE, r);

    r = SendMessageA(hwnd, LVM_GETSELECTIONMARK, 0, 0);
    expect(0, r);

    r = SendMessageA(hwnd, LVM_SETSELECTIONMARK, 0, -1);
    expect(0, r);

    item.stateMask = LVIS_FOCUSED;
    item.state     = LVIS_FOCUSED;
    r = SendMessageA(hwnd, LVM_SETITEMSTATE, 1, (LPARAM)&item);
    expect(TRUE, r);

    r = SendMessageA(hwnd, LVM_GETSELECTIONMARK, 0, 0);
    expect(-1, r);

    /* need to reset focused item first */
    item.stateMask = LVIS_FOCUSED;
    item.state     = 0;
    r = SendMessageA(hwnd, LVM_SETITEMSTATE, -1, (LPARAM)&item);
    expect(TRUE, r);

    item.stateMask = LVIS_FOCUSED;
    item.state     = LVIS_FOCUSED;
    r = SendMessageA(hwnd, LVM_SETITEMSTATE, 2, (LPARAM)&item);
    expect(TRUE, r);

    r = SendMessageA(hwnd, LVM_GETSELECTIONMARK, 0, 0);
    expect(2, r);

    item.stateMask = LVIS_FOCUSED;
    item.state     = 0;
    r = SendMessageA(hwnd, LVM_SETITEMSTATE, -1, (LPARAM)&item);
    expect(TRUE, r);

    r = SendMessageA(hwnd, LVM_GETSELECTIONMARK, 0, 0);
    expect(2, r);

    /* 2. same tests, with LVM_SETITEM */
    style = GetWindowLongPtrA(hwnd, GWL_STYLE);
    SetWindowLongPtrA(hwnd, GWL_STYLE, style & ~LVS_SINGLESEL);

    r = SendMessageA(hwnd, LVM_SETSELECTIONMARK, 0, -1);
    expect(2, r);

    item.stateMask = LVIS_FOCUSED;
    item.state     = LVIS_FOCUSED;
    item.mask      = LVIF_STATE;
    item.iItem = item.iSubItem = 0;
    r = SendMessageA(hwnd, LVM_SETITEMA, 0, (LPARAM)&item);
    expect(TRUE, r);

    r = SendMessageA(hwnd, LVM_GETSELECTIONMARK, 0, 0);
    expect(0, r);

    /* it's not updated if already set */
    item.stateMask = LVIS_FOCUSED;
    item.state     = LVIS_FOCUSED;
    item.mask      = LVIF_STATE;
    item.iItem     = 1;
    item.iSubItem  = 0;
    r = SendMessageA(hwnd, LVM_SETITEMA, 0, (LPARAM)&item);
    expect(TRUE, r);

    r = SendMessageA(hwnd, LVM_GETSELECTIONMARK, 0, 0);
    expect(0, r);

    r = SendMessageA(hwnd, LVM_SETSELECTIONMARK, 0, -1);
    expect(0, r);

    item.stateMask = LVIS_FOCUSED;
    item.state     = LVIS_FOCUSED;
    item.mask      = LVIF_STATE;
    item.iItem     = 1;
    item.iSubItem  = 0;
    r = SendMessageA(hwnd, LVM_SETITEMA, 0, (LPARAM)&item);
    expect(TRUE, r);

    r = SendMessageA(hwnd, LVM_GETSELECTIONMARK, 0, 0);
    expect(-1, r);

    /* need to reset focused item first */
    item.stateMask = LVIS_FOCUSED;
    item.state     = 0;
    r = SendMessageA(hwnd, LVM_SETITEMSTATE, -1, (LPARAM)&item);
    expect(TRUE, r);

    item.stateMask = LVIS_FOCUSED;
    item.state     = LVIS_FOCUSED;
    item.mask      = LVIF_STATE;
    item.iItem     = 2;
    item.iSubItem  = 0;
    r = SendMessageA(hwnd, LVM_SETITEMA, 0, (LPARAM)&item);
    expect(TRUE, r);

    r = SendMessageA(hwnd, LVM_GETSELECTIONMARK, 0, 0);
    expect(2, r);

    item.stateMask = LVIS_FOCUSED;
    item.state     = 0;
    r = SendMessageA(hwnd, LVM_SETITEMSTATE, -1, (LPARAM)&item);
    expect(TRUE, r);

    r = SendMessageA(hwnd, LVM_GETSELECTIONMARK, 0, 0);
    expect(2, r);

    DestroyWindow(hwnd);
}

static void test_subitem_rect(void)
{
    HWND hwnd;
    DWORD r;
    LVCOLUMNA col;
    RECT rect, rect2;
    INT arr[3];

    /* test LVM_GETSUBITEMRECT for header */
    hwnd = create_listview_control(LVS_REPORT);
    ok(hwnd != NULL, "failed to create a listview window\n");
    /* add some columns */
    memset(&col, 0, sizeof(LVCOLUMNA));
    col.mask = LVCF_WIDTH;
    col.cx = 100;
    r = SendMessageA(hwnd, LVM_INSERTCOLUMNA, 0, (LPARAM)&col);
    expect(0, r);
    col.cx = 150;
    r = SendMessageA(hwnd, LVM_INSERTCOLUMNA, 1, (LPARAM)&col);
    expect(1, r);
    col.cx = 200;
    r = SendMessageA(hwnd, LVM_INSERTCOLUMNA, 2, (LPARAM)&col);
    expect(2, r);
    /* item = -1 means header, subitem index is 1 based */
    SetRect(&rect, LVIR_BOUNDS, 0, 0, 0);
    r = SendMessageA(hwnd, LVM_GETSUBITEMRECT, -1, (LPARAM)&rect);
    expect(0, r);

    SetRect(&rect, LVIR_BOUNDS, 1, 0, 0);
    r = SendMessageA(hwnd, LVM_GETSUBITEMRECT, -1, (LPARAM)&rect);
    expect(1, r);

    expect(100, rect.left);
    expect(250, rect.right);
    expect(3, rect.top);

    SetRect(&rect, LVIR_BOUNDS, 2, 0, 0);
    r = SendMessageA(hwnd, LVM_GETSUBITEMRECT, -1, (LPARAM)&rect);
    expect(1, r);

    expect(250, rect.left);
    expect(450, rect.right);
    expect(3, rect.top);

    /* item LVS_REPORT padding isn't applied to subitems */
    insert_item(hwnd, 0);

    SetRect(&rect, LVIR_BOUNDS, 1, 0, 0);
    r = SendMessageA(hwnd, LVM_GETSUBITEMRECT, 0, (LPARAM)&rect);
    expect(1, r);
    expect(100, rect.left);
    expect(250, rect.right);

    SetRect(&rect, LVIR_ICON, 1, 0, 0);
    r = SendMessageA(hwnd, LVM_GETSUBITEMRECT, 0, (LPARAM)&rect);
    expect(1, r);
    /* no icon attached - zero width rectangle, with no left padding */
    expect(100, rect.left);
    expect(100, rect.right);

    SetRect(&rect, LVIR_LABEL, 1, 0, 0);
    r = SendMessageA(hwnd, LVM_GETSUBITEMRECT, 0, (LPARAM)&rect);
    expect(1, r);
    /* same as full LVIR_BOUNDS */
    expect(100, rect.left);
    expect(250, rect.right);

    r = SendMessageA(hwnd, LVM_SCROLL, 10, 0);
    ok(r, "got %d\n", r);

    SetRect(&rect, LVIR_BOUNDS, 1, 0, 0);
    r = SendMessageA(hwnd, LVM_GETSUBITEMRECT, 0, (LPARAM)&rect);
    expect(1, r);
    expect(90, rect.left);
    expect(240, rect.right);

    SendMessageA(hwnd, LVM_SCROLL, -10, 0);

    /* test header interaction */
    subclass_header(hwnd);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    SetRect(&rect, LVIR_BOUNDS, 1, 0, 0);
    r = SendMessageA(hwnd, LVM_GETSUBITEMRECT, -1, (LPARAM)&rect);
    expect(1, r);

    SetRect(&rect, LVIR_BOUNDS, 1, 0, 0);
    r = SendMessageA(hwnd, LVM_GETSUBITEMRECT, 0, (LPARAM)&rect);
    expect(1, r);

    SetRect(&rect, LVIR_BOUNDS, 1, 0, 0);
    r = SendMessageA(hwnd, LVM_GETSUBITEMRECT, -10, (LPARAM)&rect);
    expect(1, r);

    SetRect(&rect, LVIR_BOUNDS, 1, 0, 0);
    r = SendMessageA(hwnd, LVM_GETSUBITEMRECT, 20, (LPARAM)&rect);
    expect(1, r);

    ok_sequence(sequences, LISTVIEW_SEQ_INDEX, getsubitemrect_seq, "LVM_GETSUBITEMRECT negative index", FALSE);

    DestroyWindow(hwnd);

    /* test subitem rects after re-arranging columns */
    hwnd = create_listview_control(LVS_REPORT);
    ok(hwnd != NULL, "failed to create a listview window\n");
    memset(&col, 0, sizeof(LVCOLUMNA));
    col.mask = LVCF_WIDTH;

    col.cx = 100;
    r = SendMessageA(hwnd, LVM_INSERTCOLUMNA, 0, (LPARAM)&col);
    expect(0, r);

    col.cx = 200;
    r = SendMessageA(hwnd, LVM_INSERTCOLUMNA, 1, (LPARAM)&col);
    expect(1, r);

    col.cx = 300;
    r = SendMessageA(hwnd, LVM_INSERTCOLUMNA, 2, (LPARAM)&col);
    expect(2, r);

    insert_item(hwnd, 0);
    insert_item(hwnd, 1);

    /* wrong item is refused for main item */
    SetRect(&rect, LVIR_BOUNDS, 0, -1, -1);
    r = SendMessageA(hwnd, LVM_GETSUBITEMRECT, 2, (LPARAM)&rect);
    expect(FALSE, r);

    /* for subitems rectangle is calculated even if there's no item added */
    SetRect(&rect, LVIR_BOUNDS, 1, -1, -1);
    r = SendMessageA(hwnd, LVM_GETSUBITEMRECT, 1, (LPARAM)&rect);
    expect(TRUE, r);

    SetRect(&rect2, LVIR_BOUNDS, 1, -1, -1);
    r = SendMessageA(hwnd, LVM_GETSUBITEMRECT, 2, (LPARAM)&rect2);
    expect(TRUE, r);
    expect(rect.right, rect2.right);
    expect(rect.left, rect2.left);
    expect(rect.bottom, rect2.top);
    ok(rect2.bottom > rect2.top, "expected not zero height\n");

    arr[0] = 1; arr[1] = 0; arr[2] = 2;
    r = SendMessageA(hwnd, LVM_SETCOLUMNORDERARRAY, 3, (LPARAM)arr);
    expect(TRUE, r);

    SetRect(&rect, LVIR_BOUNDS, 0, -1, -1);
    r = SendMessageA(hwnd, LVM_GETSUBITEMRECT, 0, (LPARAM)&rect);
    expect(TRUE, r);
    expect(0, rect.left);
    expect(600, rect.right);

    SetRect(&rect, LVIR_BOUNDS, 1, -1, -1);
    r = SendMessageA(hwnd, LVM_GETSUBITEMRECT, 0, (LPARAM)&rect);
    expect(TRUE, r);
    expect(0, rect.left);
    expect(200, rect.right);

    SetRect(&rect2, LVIR_BOUNDS, 1, -1, -1);
    r = SendMessageA(hwnd, LVM_GETSUBITEMRECT, 1, (LPARAM)&rect2);
    expect(TRUE, r);
    expect(0, rect2.left);
    expect(200, rect2.right);
    /* items are of the same height */
    ok(rect2.top > 0, "expected positive item height\n");
    expect(rect.bottom, rect2.top);
    expect(rect.bottom * 2 - rect.top, rect2.bottom);

    SetRect(&rect, LVIR_BOUNDS, 2, -1, -1);
    r = SendMessageA(hwnd, LVM_GETSUBITEMRECT, 0, (LPARAM)&rect);
    expect(TRUE, r);
    expect(300, rect.left);
    expect(600, rect.right);

    DestroyWindow(hwnd);

    /* try it for non LVS_REPORT style */
    hwnd = CreateWindowA(WC_LISTVIEWA, "Test", LVS_ICON, 0, 0, 100, 100, NULL, NULL,
                         GetModuleHandleA(NULL), 0);
    SetRect(&rect, LVIR_BOUNDS, 1, -10, -10);
    r = SendMessageA(hwnd, LVM_GETSUBITEMRECT, -1, (LPARAM)&rect);
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
    r = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item);
    expect(0, r);

    item.mask = LVIF_PARAM;
    item.iItem = 1;
    item.iSubItem = 0;
    item.lParam = 2;
    r = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item);
    expect(1, r);

    item.mask = LVIF_STATE | LVIF_PARAM;
    item.state = LVIS_SELECTED;
    item.iItem = 2;
    item.iSubItem = 0;
    item.lParam = 4;
    r = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item);
    expect(2, r);

    r = SendMessageA(hwnd, LVM_GETSELECTIONMARK, 0, 0);
    expect(-1, r);

    r = SendMessageA(hwnd, LVM_GETSELECTEDCOUNT, 0, 0);
    expect(2, r);

    r = SendMessageA(hwnd, LVM_SORTITEMS, 0, (LPARAM)test_CallBackCompare);
    expect(TRUE, r);

    r = SendMessageA(hwnd, LVM_GETSELECTEDCOUNT, 0, 0);
    expect(2, r);
    r = SendMessageA(hwnd, LVM_GETSELECTIONMARK, 0, 0);
    expect(-1, r);
    r = SendMessageA(hwnd, LVM_GETITEMSTATE, 0, LVIS_SELECTED);
    expect(0, r);
    r = SendMessageA(hwnd, LVM_GETITEMSTATE, 1, LVIS_SELECTED);
    expect(LVIS_SELECTED, r);
    r = SendMessageA(hwnd, LVM_GETITEMSTATE, 2, LVIS_SELECTED);
    expect(LVIS_SELECTED, r);

    DestroyWindow(hwnd);

    /* switch to LVS_SORTASCENDING when some items added */
    hwnd = create_listview_control(LVS_REPORT);
    ok(hwnd != NULL, "failed to create a listview window\n");

    item.mask = LVIF_TEXT;
    item.iItem = 0;
    item.iSubItem = 0;
    item.pszText = names[1];
    r = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item);
    expect(0, r);

    item.mask = LVIF_TEXT;
    item.iItem = 1;
    item.iSubItem = 0;
    item.pszText = names[2];
    r = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item);
    expect(1, r);

    item.mask = LVIF_TEXT;
    item.iItem = 2;
    item.iSubItem = 0;
    item.pszText = names[0];
    r = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item);
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
    r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(TRUE, r);
    ok(lstrcmpA(buff, names[1]) == 0, "Expected '%s', got '%s'\n", names[1], buff);

    item.iItem = 1;
    r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(TRUE, r);
    ok(lstrcmpA(buff, names[2]) == 0, "Expected '%s', got '%s'\n", names[2], buff);

    item.iItem = 2;
    r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(TRUE, r);
    ok(lstrcmpA(buff, names[0]) == 0, "Expected '%s', got '%s'\n", names[0], buff);

    /* adding new item doesn't resort list */
    item.mask = LVIF_TEXT;
    item.iItem = 3;
    item.iSubItem = 0;
    item.pszText = names[3];
    r = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item);
    expect(3, r);

    item.mask = LVIF_TEXT;
    item.iItem = 0;
    item.pszText = buff;
    item.cchTextMax = sizeof(buff);
    r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(TRUE, r);
    ok(lstrcmpA(buff, names[1]) == 0, "Expected '%s', got '%s'\n", names[1], buff);

    item.iItem = 1;
    r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(TRUE, r);
    ok(lstrcmpA(buff, names[2]) == 0, "Expected '%s', got '%s'\n", names[2], buff);

    item.iItem = 2;
    r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(TRUE, r);
    ok(lstrcmpA(buff, names[0]) == 0, "Expected '%s', got '%s'\n", names[0], buff);

    item.iItem = 3;
    r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(TRUE, r);
    ok(lstrcmpA(buff, names[3]) == 0, "Expected '%s', got '%s'\n", names[3], buff);

    /* corner case - item should be placed at first position */
    item.mask = LVIF_TEXT;
    item.iItem = 4;
    item.iSubItem = 0;
    item.pszText = names[4];
    r = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item);
    expect(0, r);

    item.iItem = 0;
    item.pszText = buff;
    item.cchTextMax = sizeof(buff);
    r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(TRUE, r);
    ok(lstrcmpA(buff, names[4]) == 0, "Expected '%s', got '%s'\n", names[4], buff);

    item.iItem = 1;
    item.pszText = buff;
    item.cchTextMax = sizeof(buff);
    r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(TRUE, r);
    ok(lstrcmpA(buff, names[1]) == 0, "Expected '%s', got '%s'\n", names[1], buff);

    item.iItem = 2;
    r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(TRUE, r);
    ok(lstrcmpA(buff, names[2]) == 0, "Expected '%s', got '%s'\n", names[2], buff);

    item.iItem = 3;
    r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(TRUE, r);
    ok(lstrcmpA(buff, names[0]) == 0, "Expected '%s', got '%s'\n", names[0], buff);

    item.iItem = 4;
    r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM) &item);
    expect(TRUE, r);
    ok(lstrcmpA(buff, names[3]) == 0, "Expected '%s', got '%s'\n", names[3], buff);

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
    res = SendMessageA(hwnd, LVM_SETITEMA, 0, (LPARAM)&item);
    expect(FALSE, res);
    memset(&item, 0, sizeof(item));
    item.pszText = test_str;
    res = SendMessageA(hwnd, LVM_SETITEMTEXTA, 0, (LPARAM)&item);
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

    ok_sequence(sequences, PARENT_SEQ_INDEX, ownerdata_select_focus_parent_seq,
                "ownerdata select notification", TRUE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    memset(&item, 0, sizeof(item));
    item.stateMask = LVIS_FOCUSED;
    item.state     = LVIS_FOCUSED;
    res = SendMessageA(hwnd, LVM_SETITEMSTATE, 0, (LPARAM)&item);
    expect(TRUE, res);

    ok_sequence(sequences, PARENT_SEQ_INDEX, ownerdata_select_focus_parent_seq,
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

    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq, "ownerdata deselect all notification", FALSE);

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
    res = SendMessageA(hwnd, LVM_SETITEMSTATE, 0, (LPARAM)&item);
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

    /* The focused item is updated after the invalidation */
    hwnd = create_listview_control(LVS_OWNERDATA | LVS_REPORT);
    ok(hwnd != NULL, "failed to create a listview window\n");
    res = SendMessageA(hwnd, LVM_SETITEMCOUNT, 3, 0);
    expect(TRUE, res);

    memset(&item, 0, sizeof(item));
    item.stateMask = LVIS_FOCUSED;
    item.state     = LVIS_FOCUSED;
    res = SendMessageA(hwnd, LVM_SETITEMSTATE, 0, (LPARAM)&item);
    expect(TRUE, res);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    res = SendMessageA(hwnd, LVM_SETITEMCOUNT, 0, 0);
    expect(TRUE, res);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq,
                "ownerdata setitemcount", FALSE);

    res = SendMessageA(hwnd, LVM_GETNEXTITEM, -1, LVNI_FOCUSED);
    expect(-1, res);
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
    res = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM)&item);
    expect(0, res);
    /* retrieve with LVIF_NORECOMPUTE */
    item.mask  = LVIF_TEXT | LVIF_NORECOMPUTE;
    item.iItem = 0;
    item.pszText    = buff;
    item.cchTextMax = ARRAY_SIZE(buff);
    res = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM)&item);
    expect(TRUE, res);
    ok(lstrcmpA(buff, testA) == 0, "Expected (%s), got (%s)\n", testA, buff);

    item.mask = LVIF_TEXT;
    item.iItem = 1;
    item.pszText = LPSTR_TEXTCALLBACKA;
    res = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM)&item);
    expect(1, res);

    item.mask  = LVIF_TEXT | LVIF_NORECOMPUTE;
    item.iItem = 1;
    item.pszText    = buff;
    item.cchTextMax = ARRAY_SIZE(buff);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    res = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM)&item);
    expect(TRUE, res);
    ok(item.pszText == LPSTR_TEXTCALLBACKA, "Expected (%p), got (%p)\n",
       LPSTR_TEXTCALLBACKA, (VOID*)item.pszText);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq, "retrieve with LVIF_NORECOMPUTE seq", FALSE);

    DestroyWindow(hwnd);

    /* LVS_OWNERDATA */
    hwnd = create_listview_control(LVS_OWNERDATA | LVS_REPORT);
    ok(hwnd != NULL, "failed to create a listview window\n");

    item.mask = LVIF_STATE;
    item.stateMask = LVIS_SELECTED;
    item.state     = LVIS_SELECTED;
    item.iItem = 0;
    res = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM)&item);
    expect(0, res);

    item.mask  = LVIF_TEXT | LVIF_NORECOMPUTE;
    item.iItem = 0;
    item.pszText    = buff;
    item.cchTextMax = ARRAY_SIZE(buff);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    res = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM)&item);
    expect(TRUE, res);
    ok(item.pszText == LPSTR_TEXTCALLBACKA, "Expected (%p), got (%p)\n",
       LPSTR_TEXTCALLBACKA, (VOID*)item.pszText);
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

    style = GetWindowLongPtrA(header, GWL_STYLE);
    ok(style & HDS_BUTTONS, "expected header to have HDS_BUTTONS\n");

    style = GetWindowLongPtrA(hwnd, GWL_STYLE);
    SetWindowLongPtrA(hwnd, GWL_STYLE, style | LVS_NOSORTHEADER);
    /* HDS_BUTTONS retained */
    style = GetWindowLongPtrA(header, GWL_STYLE);
    ok(style & HDS_BUTTONS, "expected header to retain HDS_BUTTONS\n");

    DestroyWindow(hwnd);

    /* create with LVS_NOSORTHEADER */
    hwnd = create_listview_control(LVS_NOSORTHEADER | LVS_REPORT);
    ok(hwnd != NULL, "failed to create a listview window\n");

    header = (HWND)SendMessageA(hwnd, LVM_GETHEADER, 0, 0);
    ok(IsWindow(header), "header expected\n");

    style = GetWindowLongPtrA(header, GWL_STYLE);
    ok(!(style & HDS_BUTTONS), "expected header to have no HDS_BUTTONS\n");

    style = GetWindowLongPtrA(hwnd, GWL_STYLE);
    SetWindowLongPtrA(hwnd, GWL_STYLE, style & ~LVS_NOSORTHEADER);
    /* not changed here */
    style = GetWindowLongPtrA(header, GWL_STYLE);
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
    ret = SendMessageA(hwnd, WM_SETREDRAW, TRUE, 0);
    expect(0, ret);
    /* disable */
    style = GetWindowLongPtrA(hwnd, GWL_STYLE);
    ok(style & WS_VISIBLE, "Expected WS_VISIBLE to be set\n");
    ret = SendMessageA(hwnd, WM_SETREDRAW, FALSE, 0);
    expect(0, ret);
    style = GetWindowLongPtrA(hwnd, GWL_STYLE);
    ok(style & WS_VISIBLE, "Expected WS_VISIBLE to be set\n");
    ret = SendMessageA(hwnd, WM_SETREDRAW, TRUE, 0);
    expect(0, ret);

    /* check update rect after redrawing */
    ret = SendMessageA(hwnd, WM_SETREDRAW, FALSE, 0);
    expect(0, ret);
    InvalidateRect(hwnd, NULL, FALSE);
    RedrawWindow(hwnd, NULL, NULL, RDW_UPDATENOW);
    rect.right = rect.bottom = 1;
    GetUpdateRect(hwnd, &rect, FALSE);
    expect(0, rect.right);
    expect(0, rect.bottom);

    /* WM_ERASEBKGND */
    hdc = GetWindowDC(hwndparent);
    ret = SendMessageA(hwnd, WM_ERASEBKGND, (WPARAM)hdc, 0);
    expect(TRUE, ret);
    ret = SendMessageA(hwnd, WM_SETREDRAW, FALSE, 0);
    expect(0, ret);
    ret = SendMessageA(hwnd, WM_ERASEBKGND, (WPARAM)hdc, 0);
    expect(TRUE, ret);
    ret = SendMessageA(hwnd, WM_SETREDRAW, TRUE, 0);
    expect(0, ret);
    ReleaseDC(hwndparent, hdc);

    /* check notification messages to show that repainting is disabled */
    ret = SendMessageA(hwnd, LVM_SETITEMCOUNT, 1, 0);
    expect(TRUE, ret);
    ret = SendMessageA(hwnd, WM_SETREDRAW, FALSE, 0);
    expect(0, ret);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq,
                "redraw after WM_SETREDRAW (FALSE)", FALSE);

    ret = SendMessageA(hwnd, LVM_SETBKCOLOR, 0, CLR_NONE);
    expect(TRUE, ret);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq,
                "redraw after WM_SETREDRAW (FALSE) with CLR_NONE bkgnd", FALSE);

    /* message isn't forwarded to header */
    subclass_header(hwnd);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    ret = SendMessageA(hwnd, WM_SETREDRAW, FALSE, 0);
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
    r = SendMessageA(hwnd, LVM_SETITEMTEXTA, 0, (LPARAM)&item);
    expect(TRUE, r);

    r = SendMessageA(hwnd, LVM_SETCOLUMNWIDTH, 0, MAKELPARAM(100, 0));
    expect(TRUE, r);
    r = SendMessageA(hwnd, LVM_SETCOLUMNWIDTH, 1, MAKELPARAM(100, 0));
    expect(TRUE, r);

    SetRect(&bounds, LVIR_BOUNDS, 0, 0, 0);
    r = SendMessageA(hwnd, LVM_GETITEMRECT, 0, (LPARAM)&bounds);
    expect(1, r);
    ok(bounds.bottom - bounds.top > 0, "Expected non zero item height\n");
    ok(bounds.right - bounds.left > 0, "Expected non zero item width\n");
    r = SendMessageA(hwnd, LVM_GETITEMSPACING, TRUE, 0);
    vert = HIWORD(r);
    ok(bounds.bottom - bounds.top == vert,
        "Vertical spacing inconsistent (%d != %d)\n", bounds.bottom - bounds.top, vert);
    r = SendMessageA(hwnd, LVM_GETITEMPOSITION, 0, (LPARAM)&pos);
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
    SendMessageA(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);
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
    himl = pImageList_Create(16, 16, 0, 4, 4);
    ok(himl != NULL, "failed to create imagelist\n");
    hbmp = CreateBitmap(16, 16, 1, 1, NULL);
    ok(hbmp != NULL, "failed to create bitmap\n");
    r = pImageList_Add(himl, hbmp, 0);
    ok(r == 0, "should be zero\n");
    hbmp = CreateBitmap(16, 16, 1, 1, NULL);
    ok(hbmp != NULL, "failed to create bitmap\n");
    r = pImageList_Add(himl, hbmp, 0);
    ok(r == 1, "should be one\n");

    r = SendMessageA(hwnd, LVM_SETIMAGELIST, LVSIL_STATE, (LPARAM)himl);
    expect(0, r);

    item.mask = LVIF_IMAGE;
    item.iImage = 0;
    item.iItem = 0;
    item.iSubItem = 0;
    r = SendMessageA(hwnd, LVM_SETITEMA, 0, (LPARAM)&item);
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
    r = SendMessageA(hwnd, LVM_SETITEMA, 0, (LPARAM)&item);
    expect(TRUE, r);
    /* on state icon */
    x = pos.x + 8;
    y = pos.y + (bounds.bottom - bounds.top) / 2;
    test_lvm_hittest(hwnd, x, y, 0, LVHT_ONITEMSTATEICON, 0, FALSE, FALSE);
    test_lvm_subitemhittest(hwnd, x, y, 0, 0, LVHT_ONITEMSTATEICON, FALSE, FALSE, FALSE);
    y = (bounds.bottom - bounds.top) / 2;
    test_lvm_subitemhittest(hwnd, x, y, 0, 0, LVHT_ONITEMSTATEICON, FALSE, FALSE, FALSE);

    himl2 = (HIMAGELIST)SendMessageA(hwnd, LVM_SETIMAGELIST, LVSIL_STATE, 0);
    ok(himl2 == himl, "should return handle\n");

    r = SendMessageA(hwnd, LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)himl);
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
    r = SendMessageA(hwnd, LVM_GETVIEWRECT, 0, (LPARAM)&rect);
    expect(TRUE, r);

    insert_column(hwnd, 0);
    insert_column(hwnd, 1);

    memset(&item, 0, sizeof(item));
    item.iItem = 0;
    item.iSubItem = 0;
    r = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM)&item);
    ok(!r, "got %d\n", r);

    r = SendMessageA(hwnd, LVM_SETCOLUMNWIDTH, 0, MAKELPARAM(100, 0));
    expect(TRUE, r);
    r = SendMessageA(hwnd, LVM_SETCOLUMNWIDTH, 1, MAKELPARAM(120, 0));
    expect(TRUE, r);

    SetRect(&rect, -1, -1, -1, -1);
    r = SendMessageA(hwnd, LVM_GETVIEWRECT, 0, (LPARAM)&rect);
    expect(TRUE, r);
    /* left is set to (2e31-1) - XP SP2 */
    expect(0, rect.right);
    expect(0, rect.top);
    expect(0, rect.bottom);

    /* switch to LVS_ICON */
    SetWindowLongA(hwnd, GWL_STYLE, GetWindowLongA(hwnd, GWL_STYLE) & ~LVS_REPORT);

    SetRect(&rect, -1, -1, -1, -1);
    r = SendMessageA(hwnd, LVM_GETVIEWRECT, 0, (LPARAM)&rect);
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
    r = SendMessageA(hwnd, LVM_GETITEMPOSITION, 0, (LPARAM)&pt);
    expect(TRUE, r);
    ok_sequence(sequences, LISTVIEW_SEQ_INDEX, getitemposition_seq1, "get item position 1", FALSE);

    /* LVS_REPORT, single item, single column */
    insert_column(hwnd, 0);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    pt.x = pt.y = -1;
    r = SendMessageA(hwnd, LVM_GETITEMPOSITION, 0, (LPARAM)&pt);
    expect(TRUE, r);
    ok_sequence(sequences, LISTVIEW_SEQ_INDEX, getitemposition_seq2, "get item position 2", TRUE);

    SetRectEmpty(&rect);
    r = SendMessageA(header, HDM_GETITEMRECT, 0, (LPARAM)&rect);
    ok(r, "got %d\n", r);
    /* some padding? */
    expect(2, pt.x);
    /* offset by header height */
    expect(rect.bottom - rect.top, pt.y);

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

    /* rectangle isn't empty for empty text items */
    hwnd = create_listview_control(LVS_LIST);
    memset(&item, 0, sizeof(item));
    item.mask = 0;
    item.iItem = 0;
    r = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM)&item);
    expect(0, r);
    rect.left = LVIR_LABEL;
    r = SendMessageA(hwnd, LVM_GETITEMRECT, 0, (LPARAM)&rect);
    expect(TRUE, r);
    expect(0, rect.left);
    expect(0, rect.top);
    /* estimate it as width / height ratio */
todo_wine
    ok((rect.right / rect.bottom) >= 5, "got right %d, bottom %d\n", rect.right, rect.bottom);
    DestroyWindow(hwnd);

    hwnd = create_listview_control(LVS_REPORT);
    ok(hwnd != NULL, "failed to create a listview window\n");

    /* empty item */
    memset(&item, 0, sizeof(item));
    item.iItem = 0;
    item.iSubItem = 0;
    r = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM)&item);
    expect(0, r);

    SetRect(&rect, LVIR_BOUNDS, -1, -1, -1);
    r = SendMessageA(hwnd, LVM_GETITEMRECT, 0, (LPARAM)&rect);
    expect(TRUE, r);

    /* zero width rectangle with no padding */
    expect(0, rect.left);
    expect(0, rect.right);

    insert_column(hwnd, 0);
    insert_column(hwnd, 1);

    col.mask = LVCF_WIDTH;
    col.cx   = 50;
    r = SendMessageA(hwnd, LVM_SETCOLUMNA, 0, (LPARAM)&col);
    expect(TRUE, r);

    col.mask = LVCF_WIDTH;
    col.cx   = 100;
    r = SendMessageA(hwnd, LVM_SETCOLUMNA, 1, (LPARAM)&col);
    expect(TRUE, r);

    SetRect(&rect, LVIR_BOUNDS, -1, -1, -1);
    r = SendMessageA(hwnd, LVM_GETITEMRECT, 0, (LPARAM)&rect);
    expect(TRUE, r);

    /* still no left padding */
    expect(0, rect.left);
    expect(150, rect.right);

    SetRect(&rect, LVIR_SELECTBOUNDS, -1, -1, -1);
    r = SendMessageA(hwnd, LVM_GETITEMRECT, 0, (LPARAM)&rect);
    expect(TRUE, r);
    /* padding */
    expect(2, rect.left);

    SetRect(&rect, LVIR_LABEL, -1, -1, -1);
    r = SendMessageA(hwnd, LVM_GETITEMRECT, 0, (LPARAM)&rect);
    expect(TRUE, r);
    /* padding, column width */
    expect(2, rect.left);
    expect(50, rect.right);

    /* no icons attached */
    SetRect(&rect, LVIR_ICON, -1, -1, -1);
    r = SendMessageA(hwnd, LVM_GETITEMRECT, 0, (LPARAM)&rect);
    expect(TRUE, r);
    /* padding */
    expect(2, rect.left);
    expect(2, rect.right);

    /* change order */
    order[0] = 1; order[1] = 0;
    r = SendMessageA(hwnd, LVM_SETCOLUMNORDERARRAY, 2, (LPARAM)&order);
    expect(TRUE, r);
    pt.x = -1;
    r = SendMessageA(hwnd, LVM_GETITEMPOSITION, 0, (LPARAM)&pt);
    expect(TRUE, r);
    /* 1 indexed column width + padding */
    expect(102, pt.x);
    /* rect is at zero too */
    SetRect(&rect, LVIR_BOUNDS, -1, -1, -1);
    r = SendMessageA(hwnd, LVM_GETITEMRECT, 0, (LPARAM)&rect);
    expect(TRUE, r);
    expect(0, rect.left);
    /* just width sum */
    expect(150, rect.right);

    SetRect(&rect, LVIR_SELECTBOUNDS, -1, -1, -1);
    r = SendMessageA(hwnd, LVM_GETITEMRECT, 0, (LPARAM)&rect);
    expect(TRUE, r);
    /* column width + padding */
    expect(102, rect.left);

    /* back to initial order */
    order[0] = 0; order[1] = 1;
    r = SendMessageA(hwnd, LVM_SETCOLUMNORDERARRAY, 2, (LPARAM)&order);
    expect(TRUE, r);

    /* state icons */
    himl = pImageList_Create(16, 16, 0, 2, 2);
    ok(himl != NULL, "failed to create imagelist\n");
    hbm = CreateBitmap(16, 16, 1, 1, NULL);
    ok(hbm != NULL, "failed to create bitmap\n");
    r = pImageList_Add(himl, hbm, 0);
    expect(0, r);
    hbm = CreateBitmap(16, 16, 1, 1, NULL);
    ok(hbm != NULL, "failed to create bitmap\n");
    r = pImageList_Add(himl, hbm, 0);
    expect(1, r);

    r = SendMessageA(hwnd, LVM_SETIMAGELIST, LVSIL_STATE, (LPARAM)himl);
    expect(0, r);

    item.mask = LVIF_STATE;
    item.state = INDEXTOSTATEIMAGEMASK(1);
    item.stateMask = LVIS_STATEIMAGEMASK;
    item.iItem = 0;
    item.iSubItem = 0;
    r = SendMessageA(hwnd, LVM_SETITEMA, 0, (LPARAM)&item);
    expect(TRUE, r);

    /* icon bounds */
    SetRect(&rect, LVIR_ICON, -1, -1, -1);
    r = SendMessageA(hwnd, LVM_GETITEMRECT, 0, (LPARAM)&rect);
    expect(TRUE, r);
    /* padding + stateicon width */
    expect(18, rect.left);
    expect(18, rect.right);
    /* label bounds */
    SetRect(&rect, LVIR_LABEL, -1, -1, -1);
    r = SendMessageA(hwnd, LVM_GETITEMRECT, 0, (LPARAM)&rect);
    expect(TRUE, r);
    /* padding + stateicon width -> column width */
    expect(18, rect.left);
    expect(50, rect.right);

    himl_ret = (HIMAGELIST)SendMessageA(hwnd, LVM_SETIMAGELIST, LVSIL_STATE, 0);
    ok(himl_ret == himl, "got %p, expected %p\n", himl_ret, himl);

    r = SendMessageA(hwnd, LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)himl);
    expect(0, r);

    item.mask = LVIF_STATE | LVIF_IMAGE;
    item.iImage = 1;
    item.state = 0;
    item.stateMask = ~0;
    item.iItem = 0;
    item.iSubItem = 0;
    r = SendMessageA(hwnd, LVM_SETITEMA, 0, (LPARAM)&item);
    expect(TRUE, r);

    /* icon bounds */
    SetRect(&rect, LVIR_ICON, -1, -1, -1);
    r = SendMessageA(hwnd, LVM_GETITEMRECT, 0, (LPARAM)&rect);
    expect(TRUE, r);
    /* padding, icon width */
    expect(2, rect.left);
    expect(18, rect.right);
    /* label bounds */
    SetRect(&rect, LVIR_LABEL, -1, -1, -1);
    r = SendMessageA(hwnd, LVM_GETITEMRECT, 0, (LPARAM)&rect);
    expect(TRUE, r);
    /* padding + icon width -> column width */
    expect(18, rect.left);
    expect(50, rect.right);

    /* select bounds */
    SetRect(&rect, LVIR_SELECTBOUNDS, -1, -1, -1);
    r = SendMessageA(hwnd, LVM_GETITEMRECT, 0, (LPARAM)&rect);
    expect(TRUE, r);
    /* padding, column width */
    expect(2, rect.left);
    expect(50, rect.right);

    /* try with indentation */
    item.mask = LVIF_INDENT;
    item.iIndent = 1;
    item.iItem = 0;
    item.iSubItem = 0;
    r = SendMessageA(hwnd, LVM_SETITEMA, 0, (LPARAM)&item);
    expect(TRUE, r);

    /* bounds */
    SetRect(&rect, LVIR_BOUNDS, -1, -1, -1);
    r = SendMessageA(hwnd, LVM_GETITEMRECT, 0, (LPARAM)&rect);
    expect(TRUE, r);
    /* padding + 1 icon width, column width */
    expect(0, rect.left);
    expect(150, rect.right);

    /* select bounds */
    SetRect(&rect, LVIR_SELECTBOUNDS, -1, -1, -1);
    r = SendMessageA(hwnd, LVM_GETITEMRECT, 0, (LPARAM)&rect);
    expect(TRUE, r);
    /* padding + 1 icon width, column width */
    expect(2 + 16, rect.left);
    expect(50, rect.right);

    /* label bounds */
    SetRect(&rect, LVIR_LABEL, -1, -1, -1);
    r = SendMessageA(hwnd, LVM_GETITEMRECT, 0, (LPARAM)&rect);
    expect(TRUE, r);
    /* padding + 2 icon widths, column width */
    expect(2 + 16*2, rect.left);
    expect(50, rect.right);

    /* icon bounds */
    SetRect(&rect, LVIR_ICON, -1, -1, -1);
    r = SendMessageA(hwnd, LVM_GETITEMRECT, 0, (LPARAM)&rect);
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
    r = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM)&item);
    expect(0, r);

    /* test notifications without edit created */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    r = SendMessageA(hwnd, WM_COMMAND, MAKEWPARAM(0, EN_SETFOCUS), (LPARAM)0xdeadbeef);
    expect(0, r);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq,
                "edit box WM_COMMAND (EN_SETFOCUS), no edit created", FALSE);
    /* same thing but with valid window */
    hwndedit = CreateWindowA(WC_EDITA, "Test edit", WS_VISIBLE | WS_CHILD, 0, 0, 20,
                10, hwnd, (HMENU)1, (HINSTANCE)GetWindowLongPtrA(hwnd, GWLP_HINSTANCE), 0);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    r = SendMessageA(hwnd, WM_COMMAND, MAKEWPARAM(0, EN_SETFOCUS), (LPARAM)hwndedit);
    expect(0, r);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq,
                "edit box WM_COMMAND (EN_SETFOCUS), no edit created #2", FALSE);
    DestroyWindow(hwndedit);

    /* setting focus is necessary */
    SetFocus(hwnd);
    hwndedit = (HWND)SendMessageA(hwnd, LVM_EDITLABELA, 0, 0);
    ok(IsWindow(hwndedit), "Expected Edit window to be created\n");

    /* test children Z-order after Edit box created */
    header = (HWND)SendMessageA(hwnd, LVM_GETHEADER, 0, 0);
    ok(IsWindow(header), "Expected header to be created\n");
    ok(GetTopWindow(hwnd) == header, "Expected header to be on top\n");
    ok(GetNextWindow(header, GW_HWNDNEXT) == hwndedit, "got %p\n", GetNextWindow(header, GW_HWNDNEXT));

    /* modify initial string */
    r = SendMessageA(hwndedit, WM_SETTEXT, 0, (LPARAM)testitem1A);
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
    r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM)&item);
    expect(TRUE, r);

    ok(strcmp(buffer, testitem1A) == 0, "Expected item text to change\n");

    /* send LVM_EDITLABEL on already created edit */
    SetFocus(hwnd);
    hwndedit = (HWND)SendMessageA(hwnd, LVM_EDITLABELA, 0, 0);
    ok(IsWindow(hwndedit), "Expected Edit window to be created\n");
    /* focus will be set to edit */
    ok(GetFocus() == hwndedit, "Expected Edit window to be focused\n");
    hwndedit2 = (HWND)SendMessageA(hwnd, LVM_EDITLABELA, 0, 0);
    ok(IsWindow(hwndedit2), "Expected Edit window to be created\n");

    /* creating label disabled when control isn't focused */
    SetFocus(0);
    hwndedit = (HWND)SendMessageA(hwnd, LVM_EDITLABELA, 0, 0);
    todo_wine ok(hwndedit == NULL, "Expected Edit window not to be created\n");

    /* check EN_KILLFOCUS handling */
    memset(&item, 0, sizeof(item));
    item.pszText = testitemA;
    item.iItem = 0;
    item.iSubItem = 0;
    r = SendMessageA(hwnd, LVM_SETITEMTEXTA, 0, (LPARAM)&item);
    expect(TRUE, r);

    SetFocus(hwnd);
    hwndedit = (HWND)SendMessageA(hwnd, LVM_EDITLABELA, 0, 0);
    ok(IsWindow(hwndedit), "Expected Edit window to be created\n");
    /* modify edit and notify control that it lost focus */
    r = SendMessageA(hwndedit, WM_SETTEXT, 0, (LPARAM)testitem1A);
    expect(TRUE, r);
    g_editbox_disp_info.item.pszText = NULL;
    r = SendMessageA(hwnd, WM_COMMAND, MAKEWPARAM(0, EN_KILLFOCUS), (LPARAM)hwndedit);
    expect(0, r);
    ok(g_editbox_disp_info.item.pszText != NULL, "expected notification with not null text\n");

    memset(&item, 0, sizeof(item));
    item.pszText = buffer;
    item.cchTextMax = sizeof(buffer);
    item.iItem = 0;
    item.iSubItem = 0;
    r = SendMessageA(hwnd, LVM_GETITEMTEXTA, 0, (LPARAM)&item);
    expect(lstrlenA(item.pszText), r);
    ok(strcmp(buffer, testitem1A) == 0, "Expected item text to change\n");
    ok(!IsWindow(hwndedit), "Expected Edit window to be freed\n");

    /* change item name to differ in casing only */
    SetFocus(hwnd);
    hwndedit = (HWND)SendMessageA(hwnd, LVM_EDITLABELA, 0, 0);
    ok(IsWindow(hwndedit), "Expected Edit window to be created\n");
    /* modify edit and notify control that it lost focus */
    r = SendMessageA(hwndedit, WM_SETTEXT, 0, (LPARAM)testitem2A);
    expect(TRUE, r);
    g_editbox_disp_info.item.pszText = NULL;
    r = SendMessageA(hwnd, WM_COMMAND, MAKEWPARAM(0, EN_KILLFOCUS), (LPARAM)hwndedit);
    expect(0, r);
    ok(g_editbox_disp_info.item.pszText != NULL, "got %p\n", g_editbox_disp_info.item.pszText);

    memset(&item, 0, sizeof(item));
    item.pszText = buffer;
    item.cchTextMax = sizeof(buffer);
    item.iItem = 0;
    item.iSubItem = 0;
    r = SendMessageA(hwnd, LVM_GETITEMTEXTA, 0, (LPARAM)&item);
    expect(lstrlenA(item.pszText), r);
    ok(strcmp(buffer, testitem2A) == 0, "got %s, expected %s\n", buffer, testitem2A);
    ok(!IsWindow(hwndedit), "Expected Edit window to be freed\n");

    /* end edit without saving */
    SetFocus(hwnd);
    hwndedit = (HWND)SendMessageA(hwnd, LVM_EDITLABELA, 0, 0);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    r = SendMessageA(hwndedit, WM_KEYDOWN, VK_ESCAPE, 0);
    expect(0, r);
    ok_sequence(sequences, PARENT_SEQ_INDEX, edit_end_nochange,
                "edit box - end edit, no change, escape", TRUE);
    /* end edit with saving */
    SetFocus(hwnd);
    hwndedit = (HWND)SendMessageA(hwnd, LVM_EDITLABELA, 0, 0);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    r = SendMessageA(hwndedit, WM_KEYDOWN, VK_RETURN, 0);
    expect(0, r);
    ok_sequence(sequences, PARENT_SEQ_INDEX, edit_end_nochange,
                "edit box - end edit, no change, return", TRUE);

    memset(&item, 0, sizeof(item));
    item.pszText = buffer;
    item.cchTextMax = sizeof(buffer);
    item.iItem = 0;
    item.iSubItem = 0;
    r = SendMessageA(hwnd, LVM_GETITEMTEXTA, 0, (LPARAM)&item);
    expect(lstrlenA(item.pszText), r);
    ok(strcmp(buffer, testitem2A) == 0, "Expected item text to change\n");

    /* LVM_EDITLABEL with -1 destroys current edit */
    hwndedit = (HWND)SendMessageA(hwnd, LVM_GETEDITCONTROL, 0, 0);
    ok(hwndedit == NULL, "Expected Edit window not to be created\n");
    /* no edit present */
    hwndedit = (HWND)SendMessageA(hwnd, LVM_EDITLABELA, -1, 0);
    ok(hwndedit == NULL, "Expected Edit window not to be created\n");
    hwndedit = (HWND)SendMessageA(hwnd, LVM_EDITLABELA, 0, 0);
    ok(IsWindow(hwndedit), "Expected Edit window to be created\n");
    /* edit present */
    ok(GetFocus() == hwndedit, "Expected Edit to be focused\n");
    hwndedit2 = (HWND)SendMessageA(hwnd, LVM_EDITLABELA, -1, 0);
    ok(hwndedit2 == NULL, "Expected Edit window not to be created\n");
    ok(!IsWindow(hwndedit), "Expected Edit window to be destroyed\n");
    ok(GetFocus() == hwnd, "Expected List to be focused\n");
    /* check another negative value */
    hwndedit = (HWND)SendMessageA(hwnd, LVM_EDITLABELA, 0, 0);
    ok(IsWindow(hwndedit), "Expected Edit window to be created\n");
    ok(GetFocus() == hwndedit, "Expected Edit to be focused\n");
    hwndedit2 = (HWND)SendMessageA(hwnd, LVM_EDITLABELA, -2, 0);
    ok(hwndedit2 == NULL, "Expected Edit window not to be created\n");
    ok(!IsWindow(hwndedit), "Expected Edit window to be destroyed\n");
    ok(GetFocus() == hwnd, "Expected List to be focused\n");
    /* and value greater than max item index */
    hwndedit = (HWND)SendMessageA(hwnd, LVM_EDITLABELA, 0, 0);
    ok(IsWindow(hwndedit), "Expected Edit window to be created\n");
    ok(GetFocus() == hwndedit, "Expected Edit to be focused\n");
    r = SendMessageA(hwnd, LVM_GETITEMCOUNT, 0, 0);
    hwndedit2 = (HWND)SendMessageA(hwnd, LVM_EDITLABELA, r, 0);
    ok(hwndedit2 == NULL, "Expected Edit window not to be created\n");
    ok(!IsWindow(hwndedit), "Expected Edit window to be destroyed\n");
    ok(GetFocus() == hwnd, "Expected List to be focused\n");

    /* messaging tests */
    SetFocus(hwnd);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    blockEdit = FALSE;
    hwndedit = (HWND)SendMessageA(hwnd, LVM_EDITLABELA, 0, 0);
    ok(IsWindow(hwndedit), "Expected Edit window to be created\n");
    /* testing only sizing messages */
    ok_sequence(sequences, EDITBOX_SEQ_INDEX, editbox_create_pos,
                "edit box create - sizing", FALSE);

    /* WM_COMMAND with EN_KILLFOCUS isn't forwarded to parent */
    SetFocus(hwnd);
    hwndedit = (HWND)SendMessageA(hwnd, LVM_EDITLABELA, 0, 0);
    ok(IsWindow(hwndedit), "Expected Edit window to be created\n");
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    r = SendMessageA(hwnd, WM_COMMAND, MAKEWPARAM(0, EN_KILLFOCUS), (LPARAM)hwndedit);
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
    r = SendMessageA(hwnd, LVM_GETUNICODEFORMAT, 0, 0);
    expect(0, r);
    SendMessageA(hwnd, WM_NOTIFYFORMAT, 0, NF_QUERY);
    /* set */
    r = SendMessageA(hwnd, LVM_SETUNICODEFORMAT, 1, 0);
    expect(0, r);
    r = SendMessageA(hwnd, LVM_GETUNICODEFORMAT, 0, 0);
    ok(r == 1, "Unexpected return value %d.\n", r);
    r = SendMessageA(hwnd, LVM_SETUNICODEFORMAT, 0, 0);
    expect(1, r);
    r = SendMessageA(hwnd, LVM_GETUNICODEFORMAT, 0, 0);
    expect(0, r);

    DestroyWindow(hwnd);

    /* test failure in parent WM_NOTIFYFORMAT  */
    notifyFormat = 0;
    hwnd = create_listview_control(LVS_REPORT);
    ok(hwnd != NULL, "failed to create a listview window\n");
    header = (HWND)SendMessageA(hwnd, LVM_GETHEADER, 0, 0);
    ok(IsWindow(header), "expected header to be created\n");
    r = SendMessageA(hwnd, LVM_GETUNICODEFORMAT, 0, 0);
    expect(0, r);
    r = SendMessageA(header, HDM_GETUNICODEFORMAT, 0, 0);
    ok( r == 1, "Expected 1, got %d\n", r );
    r = SendMessageA(hwnd, WM_NOTIFYFORMAT, 0, NF_QUERY);
    ok(r != 0, "Expected valid format\n");

    notifyFormat = NFR_UNICODE;
    r = SendMessageA(hwnd, WM_NOTIFYFORMAT, 0, NF_REQUERY);
    expect(NFR_UNICODE, r);
    r = SendMessageA(hwnd, LVM_GETUNICODEFORMAT, 0, 0);
    expect(1, r);
    r = SendMessageA(header, HDM_GETUNICODEFORMAT, 0, 0);
    ok( r == 1, "Expected 1, got %d\n", r );

    notifyFormat = NFR_ANSI;
    r = SendMessageA(hwnd, WM_NOTIFYFORMAT, 0, NF_REQUERY);
    expect(NFR_ANSI, r);
    r = SendMessageA(hwnd, LVM_GETUNICODEFORMAT, 0, 0);
    expect(0, r);
    r = SendMessageA(header, HDM_GETUNICODEFORMAT, 0, 0);
    ok( r == 1, "Expected 1, got %d\n", r );

    DestroyWindow(hwnd);

    hwndparentW = create_parent_window(TRUE);
    ok(IsWindow(hwndparentW), "Unicode parent creation failed\n");
    if (!IsWindow(hwndparentW))  return;

    notifyFormat = -1;
    hwnd = create_listview_controlW(LVS_REPORT, hwndparentW);
    ok(hwnd != NULL, "failed to create a listview window\n");
    header = (HWND)SendMessageA(hwnd, LVM_GETHEADER, 0, 0);
    ok(IsWindow(header), "expected header to be created\n");
    r = SendMessageW(hwnd, LVM_GETUNICODEFORMAT, 0, 0);
    expect(1, r);
    r = SendMessageA(header, HDM_GETUNICODEFORMAT, 0, 0);
    expect(1, r);
    DestroyWindow(hwnd);
    /* receiving error code defaulting to ansi */
    notifyFormat = 0;
    hwnd = create_listview_controlW(LVS_REPORT, hwndparentW);
    ok(hwnd != NULL, "failed to create a listview window\n");
    header = (HWND)SendMessageA(hwnd, LVM_GETHEADER, 0, 0);
    ok(IsWindow(header), "expected header to be created\n");
    r = SendMessageW(hwnd, LVM_GETUNICODEFORMAT, 0, 0);
    expect(0, r);
    r = SendMessageA(header, HDM_GETUNICODEFORMAT, 0, 0);
    expect(1, r);
    DestroyWindow(hwnd);
    /* receiving ansi code from unicode window, use it */
    notifyFormat = NFR_ANSI;
    hwnd = create_listview_controlW(LVS_REPORT, hwndparentW);
    ok(hwnd != NULL, "failed to create a listview window\n");
    header = (HWND)SendMessageA(hwnd, LVM_GETHEADER, 0, 0);
    ok(IsWindow(header), "expected header to be created\n");
    r = SendMessageW(hwnd, LVM_GETUNICODEFORMAT, 0, 0);
    expect(0, r);
    r = SendMessageA(header, HDM_GETUNICODEFORMAT, 0, 0);
    expect(1, r);
    DestroyWindow(hwnd);
    /* unicode listview with ansi parent window */
    notifyFormat = -1;
    hwnd = create_listview_controlW(LVS_REPORT, hwndparent);
    ok(hwnd != NULL, "failed to create a listview window\n");
    header = (HWND)SendMessageA(hwnd, LVM_GETHEADER, 0, 0);
    ok(IsWindow(header), "expected header to be created\n");
    r = SendMessageW(hwnd, LVM_GETUNICODEFORMAT, 0, 0);
    expect(0, r);
    r = SendMessageA(header, HDM_GETUNICODEFORMAT, 0, 0);
    expect(1, r);
    DestroyWindow(hwnd);
    /* unicode listview with ansi parent window, return error code */
    notifyFormat = 0;
    hwnd = create_listview_controlW(LVS_REPORT, hwndparent);
    ok(hwnd != NULL, "failed to create a listview window\n");
    header = (HWND)SendMessageA(hwnd, LVM_GETHEADER, 0, 0);
    ok(IsWindow(header), "expected header to be created\n");
    r = SendMessageW(hwnd, LVM_GETUNICODEFORMAT, 0, 0);
    expect(0, r);
    r = SendMessageA(header, HDM_GETUNICODEFORMAT, 0, 0);
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
    r = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM)&item);
    expect(0, r);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    item.iItem = 0;
    item.mask = LVIF_INDENT;
    r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM)&item);
    expect(TRUE, r);

    ok_sequence(sequences, PARENT_SEQ_INDEX, single_getdispinfo_parent_seq,
                "get indent dispinfo", FALSE);

    /* Ask for iIndent with invalid subitem. */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    memset(&item, 0, sizeof(item));
    item.mask = LVIF_INDENT;
    item.iSubItem = 1;
    r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM)&item);
    ok(r, "Failed to get item.\n");

    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq, "get indent dispinfo 2", FALSE);

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

    ret = SendMessageA(hwnd, LVM_SORTITEMSEX, 0, (LPARAM)&DummyCompareEx);

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

    ret = SendMessageA(hwnd, LVM_GETVIEW, 0, 0);
    expect(LV_VIEW_DETAILS, ret);

    style = GetWindowLongPtrA(hwnd, GWL_STYLE);
    /* LVS_ICON == 0 */
    SetWindowLongPtrA(hwnd, GWL_STYLE, style & ~LVS_REPORT);
    ret = SendMessageA(hwnd, LVM_GETVIEW, 0, 0);
    expect(LV_VIEW_ICON, ret);

    style = GetWindowLongPtrA(hwnd, GWL_STYLE);
    SetWindowLongPtrA(hwnd, GWL_STYLE, style | LVS_SMALLICON);
    ret = SendMessageA(hwnd, LVM_GETVIEW, 0, 0);
    expect(LV_VIEW_SMALLICON, ret);

    style = GetWindowLongPtrA(hwnd, GWL_STYLE);
    SetWindowLongPtrA(hwnd, GWL_STYLE, (style & ~LVS_SMALLICON) | LVS_LIST);
    ret = SendMessageA(hwnd, LVM_GETVIEW, 0, 0);
    expect(LV_VIEW_LIST, ret);

    /* switching view doesn't touch window style */
    ret = SendMessageA(hwnd, LVM_SETVIEW, LV_VIEW_DETAILS, 0);
    expect(1, ret);
    style = GetWindowLongPtrA(hwnd, GWL_STYLE);
    ok(style & LVS_LIST, "Expected style to be preserved\n");
    ret = SendMessageA(hwnd, LVM_SETVIEW, LV_VIEW_ICON, 0);
    expect(1, ret);
    style = GetWindowLongPtrA(hwnd, GWL_STYLE);
    ok(style & LVS_LIST, "Expected style to be preserved\n");
    ret = SendMessageA(hwnd, LVM_SETVIEW, LV_VIEW_SMALLICON, 0);
    expect(1, ret);
    style = GetWindowLongPtrA(hwnd, GWL_STYLE);
    ok(style & LVS_LIST, "Expected style to be preserved\n");

    /* now change window style to see if view is remapped */
    style = GetWindowLongPtrA(hwnd, GWL_STYLE);
    SetWindowLongPtrA(hwnd, GWL_STYLE, style | LVS_SHOWSELALWAYS);
    ret = SendMessageA(hwnd, LVM_GETVIEW, 0, 0);
    expect(LV_VIEW_SMALLICON, ret);

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
    ret = SendMessageA(hwnd, LVM_CANCELEDITLABEL, 0, 0);
    expect(TRUE, ret);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq,
                "cancel edit label without edit", FALSE);

    /* cancel without data change */
    SetFocus(hwnd);
    hwndedit = (HWND)SendMessageA(hwnd, LVM_EDITLABELA, 0, 0);
    ok(IsWindow(hwndedit), "Expected edit control to be created\n");
    ret = SendMessageA(hwnd, LVM_CANCELEDITLABEL, 0, 0);
    expect(TRUE, ret);
    ok(!IsWindow(hwndedit), "Expected edit control to be destroyed\n");

    /* cancel after data change */
    memset(&itema, 0, sizeof(itema));
    itema.pszText = test;
    ret = SendMessageA(hwnd, LVM_SETITEMTEXTA, 0, (LPARAM)&itema);
    expect(TRUE, ret);
    SetFocus(hwnd);
    hwndedit = (HWND)SendMessageA(hwnd, LVM_EDITLABELA, 0, 0);
    ok(IsWindow(hwndedit), "Expected edit control to be created\n");
    ret = SetWindowTextA(hwndedit, test1);
    expect(1, ret);
    ret = SendMessageA(hwnd, LVM_CANCELEDITLABEL, 0, 0);
    expect(TRUE, ret);
    ok(!IsWindow(hwndedit), "Expected edit control to be destroyed\n");
    memset(&itema, 0, sizeof(itema));
    itema.pszText = buff;
    itema.cchTextMax = ARRAY_SIZE(buff);
    ret = SendMessageA(hwnd, LVM_GETITEMTEXTA, 0, (LPARAM)&itema);
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
    ret = SendMessageA(hwnd, LVM_MAPINDEXTOID, 0, 0);
    expect(-1, ret);
    DestroyWindow(hwnd);

    hwnd = create_listview_control(LVS_REPORT);
    ok(hwnd != NULL, "failed to create a listview window\n");

    /* LVM_MAPINDEXTOID with invalid index */
    ret = SendMessageA(hwnd, LVM_MAPINDEXTOID, 0, 0);
    expect(-1, ret);

    insert_item(hwnd, 0);
    insert_item(hwnd, 1);

    ret = SendMessageA(hwnd, LVM_MAPINDEXTOID, -1, 0);
    expect(-1, ret);
    ret = SendMessageA(hwnd, LVM_MAPINDEXTOID, 2, 0);
    expect(-1, ret);

    ret = SendMessageA(hwnd, LVM_MAPINDEXTOID, 0, 0);
    expect(0, ret);
    ret = SendMessageA(hwnd, LVM_MAPINDEXTOID, 1, 0);
    expect(1, ret);
    /* remove 0 indexed item, id retained */
    SendMessageA(hwnd, LVM_DELETEITEM, 0, 0);
    ret = SendMessageA(hwnd, LVM_MAPINDEXTOID, 0, 0);
    expect(1, ret);
    /* new id starts from previous value */
    insert_item(hwnd, 1);
    ret = SendMessageA(hwnd, LVM_MAPINDEXTOID, 1, 0);
    expect(2, ret);

    /* get index by id */
    ret = SendMessageA(hwnd, LVM_MAPIDTOINDEX, -1, 0);
    expect(-1, ret);
    ret = SendMessageA(hwnd, LVM_MAPIDTOINDEX, 0, 0);
    expect(-1, ret);
    ret = SendMessageA(hwnd, LVM_MAPIDTOINDEX, 1, 0);
    expect(0, ret);
    ret = SendMessageA(hwnd, LVM_MAPIDTOINDEX, 2, 0);
    expect(1, ret);

    DestroyWindow(hwnd);
}

static void test_getitemspacing(void)
{
    HWND hwnd;
    DWORD ret;
    INT cx, cy;
    HIMAGELIST himl40, himl80;

    cx = GetSystemMetrics(SM_CXICONSPACING) - GetSystemMetrics(SM_CXICON);
    cy = GetSystemMetrics(SM_CYICONSPACING) - GetSystemMetrics(SM_CYICON);

    /* LVS_ICON */
    hwnd = create_listview_control(LVS_ICON);
    ret = SendMessageA(hwnd, LVM_GETITEMSPACING, FALSE, 0);
    expect(cx, LOWORD(ret));
    expect(cy, HIWORD(ret));

    /* now try with icons */
    himl40 = pImageList_Create(40, 40, 0, 4, 4);
    ok(himl40 != NULL, "failed to create imagelist\n");
    himl80 = pImageList_Create(80, 80, 0, 4, 4);
    ok(himl80 != NULL, "failed to create imagelist\n");
    ret = SendMessageA(hwnd, LVM_SETIMAGELIST, LVSIL_NORMAL, (LPARAM)himl40);
    expect(0, ret);

    ret = SendMessageA(hwnd, LVM_GETITEMSPACING, FALSE, 0);
    /* spacing + icon size returned */
    expect(cx + 40, LOWORD(ret));
    expect(cy + 40, HIWORD(ret));
    /* try changing icon size */
    SendMessageA(hwnd, LVM_SETIMAGELIST, LVSIL_NORMAL, (LPARAM)himl80);

    ret = SendMessageA(hwnd, LVM_GETITEMSPACING, FALSE, 0);
    /* spacing + icon size returned */
    expect(cx + 80, LOWORD(ret));
    expect(cy + 80, HIWORD(ret));

    /* set own icon spacing */
    ret = SendMessageA(hwnd, LVM_SETICONSPACING, 0, MAKELPARAM(100, 100));
    expect(cx + 80, LOWORD(ret));
    expect(cy + 80, HIWORD(ret));

    ret = SendMessageA(hwnd, LVM_GETITEMSPACING, FALSE, 0);
    /* set size returned */
    expect(100, LOWORD(ret));
    expect(100, HIWORD(ret));

    /* now change image list - icon spacing should be unaffected */
    SendMessageA(hwnd, LVM_SETIMAGELIST, LVSIL_NORMAL, (LPARAM)himl40);

    ret = SendMessageA(hwnd, LVM_GETITEMSPACING, FALSE, 0);
    /* set size returned */
    expect(100, LOWORD(ret));
    expect(100, HIWORD(ret));

    /* spacing = 0 - keep previous value */
    ret = SendMessageA(hwnd, LVM_SETICONSPACING, 0, MAKELPARAM(0, -1));
    expect(100, LOWORD(ret));
    expect(100, HIWORD(ret));

    ret = SendMessageA(hwnd, LVM_GETITEMSPACING, FALSE, 0);
    expect(100, LOWORD(ret));

    expect(0xFFFF, HIWORD(ret));

    if (sizeof(void*) == 8)
    {
        /* NOTE: -1 is not treated the same as (DWORD)-1 by 64bit listview */
        ret = SendMessageA(hwnd, LVM_SETICONSPACING, 0, (DWORD)-1);
        expect(100, LOWORD(ret));
        expect(0xFFFF, HIWORD(ret));

        ret = SendMessageA(hwnd, LVM_SETICONSPACING, 0, -1);
        expect(0xFFFF, LOWORD(ret));
        expect(0xFFFF, HIWORD(ret));
    }
    else
    {
        ret = SendMessageA(hwnd, LVM_SETICONSPACING, 0, -1);
        expect(100, LOWORD(ret));
        expect(0xFFFF, HIWORD(ret));
    }
    ret = SendMessageA(hwnd, LVM_GETITEMSPACING, FALSE, 0);
    /* spacing + icon size returned */
    expect(cx + 40, LOWORD(ret));
    expect(cy + 40, HIWORD(ret));

    SendMessageA(hwnd, LVM_SETIMAGELIST, LVSIL_NORMAL, 0);
    pImageList_Destroy(himl80);
    DestroyWindow(hwnd);
    /* LVS_SMALLICON */
    hwnd = create_listview_control(LVS_SMALLICON);
    ret = SendMessageA(hwnd, LVM_GETITEMSPACING, FALSE, 0);
    expect(cx, LOWORD(ret));
    expect(cy, HIWORD(ret));

    /* spacing does not depend on selected view type */
    ret = SendMessageA(hwnd, LVM_SETIMAGELIST, LVSIL_NORMAL, (LPARAM)himl40);
    expect(0, ret);

    ret = SendMessageA(hwnd, LVM_GETITEMSPACING, FALSE, 0);
    /* spacing + icon size returned */
    expect(cx + 40, LOWORD(ret));
    expect(cy + 40, HIWORD(ret));

    SendMessageA(hwnd, LVM_SETIMAGELIST, LVSIL_NORMAL, 0);
    pImageList_Destroy(himl40);
    DestroyWindow(hwnd);
    /* LVS_REPORT */
    hwnd = create_listview_control(LVS_REPORT);
    ret = SendMessageA(hwnd, LVM_GETITEMSPACING, FALSE, 0);
    expect(cx, LOWORD(ret));
    expect(cy, HIWORD(ret));

    DestroyWindow(hwnd);
    /* LVS_LIST */
    hwnd = create_listview_control(LVS_LIST);
    ret = SendMessageA(hwnd, LVM_GETITEMSPACING, FALSE, 0);
    expect(cx, LOWORD(ret));
    expect(cy, HIWORD(ret));

    DestroyWindow(hwnd);
}

static INT get_current_font_height(HWND listview)
{
    TEXTMETRICA tm;
    HFONT hfont;
    HWND hwnd;
    HDC hdc;

    hwnd = (HWND)SendMessageA(listview, LVM_GETHEADER, 0, 0);
    if (!hwnd)
        hwnd = listview;

    hfont = (HFONT)SendMessageA(hwnd, WM_GETFONT, 0, 0);
    if (!hfont) {
        hdc = GetDC(hwnd);
        GetTextMetricsA(hdc, &tm);
        ReleaseDC(hwnd, hdc);
    }
    else {
        HFONT oldfont;

        hdc = GetDC(0);
        oldfont = SelectObject(hdc, hfont);
        GetTextMetricsA(hdc, &tm);
        SelectObject(hdc, oldfont);
        ReleaseDC(0, hdc);
    }

    return tm.tmHeight;
}

static void test_getcolumnwidth(void)
{
    HWND hwnd;
    INT ret;
    DWORD_PTR style;
    LVCOLUMNA col;
    LVITEMA itema;
    INT height;

    /* default column width */
    hwnd = create_listview_control(LVS_ICON);
    ret = SendMessageA(hwnd, LVM_GETCOLUMNWIDTH, 0, 0);
    expect(0, ret);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    SetWindowLongA(hwnd, GWL_STYLE, style | LVS_LIST);
    ret = SendMessageA(hwnd, LVM_GETCOLUMNWIDTH, 0, 0);
    todo_wine expect(8, ret);
    style = GetWindowLongA(hwnd, GWL_STYLE) & ~LVS_LIST;
    SetWindowLongA(hwnd, GWL_STYLE, style | LVS_REPORT);
    col.mask = 0;
    ret = SendMessageA(hwnd, LVM_INSERTCOLUMNA, 0, (LPARAM)&col);
    expect(0, ret);
    ret = SendMessageA(hwnd, LVM_GETCOLUMNWIDTH, 0, 0);
    expect(10, ret);
    DestroyWindow(hwnd);

    /* default column width with item added */
    hwnd = create_listview_control(LVS_LIST);
    memset(&itema, 0, sizeof(itema));
    ret = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM)&itema);
    ok(!ret, "got %d\n", ret);
    ret = SendMessageA(hwnd, LVM_GETCOLUMNWIDTH, 0, 0);
    height = get_current_font_height(hwnd);
    ok((ret / height) >= 6, "got width %d, height %d\n", ret, height);
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
    ret = SendMessageA(hwnd, LVM_SETCOLUMNWIDTH, 0, MAKELPARAM(100, 0));
    expect(TRUE, ret);
    ret = SendMessageA(hwnd, LVM_SETCOLUMNWIDTH, 1, MAKELPARAM(100, 0));
    expect(TRUE, ret);

    /* try with dummy call */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    ret = SendMessageA(hwnd, LVM_SCROLL, 0, 0);
    expect(TRUE, ret);
    ok_sequence(sequences, PARENT_SEQ_INDEX, scroll_parent_seq,
                "scroll notify 1", TRUE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    ret = SendMessageA(hwnd, LVM_SCROLL, 1, 0);
    expect(TRUE, ret);
    ok_sequence(sequences, PARENT_SEQ_INDEX, scroll_parent_seq,
                "scroll notify 2", TRUE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    ret = SendMessageA(hwnd, LVM_SCROLL, 1, 1);
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

    ret = SendMessageA(hwnd, LVM_SETBKCOLOR, 0, RGB(0, 0, 0));
    expect(TRUE, ret);

    SendMessageA(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_TRANSPARENTBKGND,
                                                    LVS_EX_TRANSPARENTBKGND);

    ret = SendMessageA(hwnd, LVM_GETBKCOLOR, 0, 0);
    if (ret != CLR_NONE)
    {
        win_skip("LVS_EX_TRANSPARENTBKGND unsupported\n");
        DestroyWindow(hwnd);
        return;
    }

    /* try to set some back color and check this style bit */
    ret = SendMessageA(hwnd, LVM_SETBKCOLOR, 0, RGB(0, 0, 0));
    expect(TRUE, ret);
    ret = SendMessageA(hwnd, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
    ok(!(ret & LVS_EX_TRANSPARENTBKGND), "Expected LVS_EX_TRANSPARENTBKGND to unset\n");

    /* now test what this style actually does */
    SendMessageA(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_TRANSPARENTBKGND,
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
    static CHAR test[] = "abracadabra, a very long item label";
    DWORD item_width, item_height, header_height;
    static CHAR column_header[] = "Header";
    unsigned const column_width = 100;
    DWORD ret, item_count;
    HIMAGELIST himl;
    LVITEMA itema;
    LVCOLUMNA col;
    HBITMAP hbmp;
    HWND hwnd;

    /* LVS_ICON */
    hwnd = create_listview_control(LVS_ICON);
    himl = pImageList_Create(40, 40, 0, 4, 4);
    ok(himl != NULL, "failed to create imagelist\n");
    hbmp = CreateBitmap(40, 40, 1, 1, NULL);
    ok(hbmp != NULL, "failed to create bitmap\n");
    ret = pImageList_Add(himl, hbmp, 0);
    expect(0, ret);
    ret = SendMessageA(hwnd, LVM_SETIMAGELIST, 0, (LPARAM)himl);
    expect(0, ret);

    itema.mask = LVIF_IMAGE;
    itema.iImage = 0;
    itema.iItem = 0;
    itema.iSubItem = 0;
    ret = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM)&itema);
    expect(0, ret);

    ret = SendMessageA(hwnd, LVM_SETICONSPACING, 0, MAKELPARAM(75, 75));
    ok(ret != 0, "Unexpected return value %#x.\n", ret);

    ret = SendMessageA(hwnd, LVM_APPROXIMATEVIEWRECT, 11, MAKELPARAM(100,100));
    expect(MAKELONG(77,827), ret);

    ret = SendMessageA(hwnd, LVM_SETICONSPACING, 0, MAKELPARAM(50, 50));
    ok(ret != 0, "got 0\n");

    ret = SendMessageA(hwnd, LVM_APPROXIMATEVIEWRECT, 11, MAKELPARAM(100,100));
    expect(MAKELONG(102,302), ret);

    ret = SendMessageA(hwnd, LVM_APPROXIMATEVIEWRECT, -1, MAKELPARAM(100,100));
    expect(MAKELONG(52,52), ret);

    itema.pszText = test;
    ret = SendMessageA(hwnd, LVM_SETITEMTEXTA, 0, (LPARAM)&itema);
    expect(TRUE, ret);
    ret = SendMessageA(hwnd, LVM_APPROXIMATEVIEWRECT, -1, MAKELPARAM(100,100));
    expect(MAKELONG(52,52), ret);

    ret = SendMessageA(hwnd, LVM_APPROXIMATEVIEWRECT, 0, MAKELPARAM(100,100));
    expect(MAKELONG(52,2), ret);
    ret = SendMessageA(hwnd, LVM_APPROXIMATEVIEWRECT, 1, MAKELPARAM(100,100));
    expect(MAKELONG(52,52), ret);
    ret = SendMessageA(hwnd, LVM_APPROXIMATEVIEWRECT, 2, MAKELPARAM(100,100));
    expect(MAKELONG(102,52), ret);
    ret = SendMessageA(hwnd, LVM_APPROXIMATEVIEWRECT, 3, MAKELPARAM(100,100));
    expect(MAKELONG(102,102), ret);
    ret = SendMessageA(hwnd, LVM_APPROXIMATEVIEWRECT, 4, MAKELPARAM(100,100));
    expect(MAKELONG(102,102), ret);
    ret = SendMessageA(hwnd, LVM_APPROXIMATEVIEWRECT, 5, MAKELPARAM(100,100));
    expect(MAKELONG(102,152), ret);
    ret = SendMessageA(hwnd, LVM_APPROXIMATEVIEWRECT, 6, MAKELPARAM(100,100));
    expect(MAKELONG(102,152), ret);
    ret = SendMessageA(hwnd, LVM_APPROXIMATEVIEWRECT, 7, MAKELPARAM(160,100));
    expect(MAKELONG(152,152), ret);

    DestroyWindow(hwnd);

    /* LVS_REPORT */
    hwnd = create_listview_control(LVS_REPORT);

    /* Empty control without columns */
    ret = SendMessageA(hwnd, LVM_APPROXIMATEVIEWRECT, 0, MAKELPARAM(100, 100));
todo_wine
    ok(LOWORD(ret) == 0, "Unexpected width %d.\n", LOWORD(ret));
    ok(HIWORD(ret) != 0, "Unexpected height %d.\n", HIWORD(ret));

    ret = SendMessageA(hwnd, LVM_APPROXIMATEVIEWRECT, 0, 0);
    ok(LOWORD(ret) == 0, "Unexpected width %d.\n", LOWORD(ret));
todo_wine
    ok(HIWORD(ret) != 0, "Unexpected height %d.\n", HIWORD(ret));

    header_height = HIWORD(ret);

    ret = SendMessageA(hwnd, LVM_APPROXIMATEVIEWRECT, 1, 0);
    ok(LOWORD(ret) == 0, "Unexpected width %d.\n", LOWORD(ret));
todo_wine
    ok(HIWORD(ret) > header_height, "Unexpected height %d.\n", HIWORD(ret));

    item_height = HIWORD(ret) - header_height;

    ret = SendMessageA(hwnd, LVM_APPROXIMATEVIEWRECT, -2, 0);
    ok(LOWORD(ret) == 0, "Unexpected width %d.\n", LOWORD(ret));
    ok(HIWORD(ret) == (header_height - 2 * item_height), "Unexpected height %d.\n", HIWORD(ret)) ;

    ret = SendMessageA(hwnd, LVM_APPROXIMATEVIEWRECT, -1, 0);
    ok(LOWORD(ret) == 0, "Unexpected width %d.\n", LOWORD(ret));
    ok(HIWORD(ret) == header_height, "Unexpected height.\n");
    ret = SendMessageA(hwnd, LVM_APPROXIMATEVIEWRECT, 2, 0);
    ok(LOWORD(ret) == 0, "Unexpected width %d.\n", LOWORD(ret));
    ok(HIWORD(ret) == header_height + 2 * item_height, "Unexpected height %d.\n", HIWORD(ret));

    /* Insert column */
    col.mask = LVCF_TEXT | LVCF_WIDTH;
    col.pszText = column_header;
    col.cx = column_width;
    ret = SendMessageA(hwnd, LVM_INSERTCOLUMNA, 0, (LPARAM)&col);
    ok(ret == 0, "Unexpected return value %d.\n", ret);

    /* Empty control with column */
    ret = SendMessageA(hwnd, LVM_APPROXIMATEVIEWRECT, 0, 0);
todo_wine {
    ok(LOWORD(ret) >= column_width, "Unexpected width %d.\n", LOWORD(ret));
    ok(HIWORD(ret) != 0, "Unexpected height %d.\n", HIWORD(ret));
}
    header_height = HIWORD(ret);
    item_width = LOWORD(ret);

    ret = SendMessageA(hwnd, LVM_APPROXIMATEVIEWRECT, 1, 0);
    ok(LOWORD(ret) == item_width, "Unexpected width %d.\n", LOWORD(ret));
todo_wine
    ok(HIWORD(ret) > header_height, "Unexpected height %d.\n", HIWORD(ret));

    item_height = HIWORD(ret) - header_height;

    ret = SendMessageA(hwnd, LVM_APPROXIMATEVIEWRECT, -2, 0);
    ok(LOWORD(ret) == item_width, "Unexpected width %d.\n", LOWORD(ret));
    ok(HIWORD(ret) == header_height - 2 * item_height, "Unexpected height %d.\n", HIWORD(ret));

    ret = SendMessageA(hwnd, LVM_APPROXIMATEVIEWRECT, -1, 0);
    ok(LOWORD(ret) == item_width, "Unexpected width %d.\n", LOWORD(ret));
    ok(HIWORD(ret) == header_height, "Unexpected height %d.\n", HIWORD(ret));

    ret = SendMessageA(hwnd, LVM_APPROXIMATEVIEWRECT, 2, 0);
    ok(LOWORD(ret) == item_width, "Unexpected width %d.\n", LOWORD(ret));
    ok(HIWORD(ret) == header_height + 2 * item_height, "Unexpected height %d.\n", HIWORD(ret));

    for (item_count = 1; item_count <= 2; ++item_count)
    {
        itema.mask = LVIF_TEXT;
        itema.iItem = 0;
        itema.iSubItem = 0;
        itema.pszText = test;
        ret = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM)&itema);
        ok(ret == 0, "Unexpected return value %d.\n", ret);

        ret = SendMessageA(hwnd, LVM_APPROXIMATEVIEWRECT, 0, 0);
        ok(LOWORD(ret) >= column_width, "Unexpected width %d.\n", LOWORD(ret));
    todo_wine
        ok(HIWORD(ret) != 0, "Unexpected height %d.\n", HIWORD(ret));

        header_height = HIWORD(ret);
        item_width = LOWORD(ret);

        ret = SendMessageA(hwnd, LVM_APPROXIMATEVIEWRECT, 1, 0);
        ok(LOWORD(ret) == item_width, "Unexpected width %d, item %d\n", LOWORD(ret), item_count - 1);
        ok(HIWORD(ret) > header_height, "Unexpected height %d. item %d.\n", HIWORD(ret),  item_count - 1);

        item_height = HIWORD(ret) - header_height;

        ret = SendMessageA(hwnd, LVM_APPROXIMATEVIEWRECT, -2, 0);
        ok(LOWORD(ret) == item_width, "Unexpected width %d.\n", LOWORD(ret));
    todo_wine
        ok(HIWORD(ret) == header_height - 2 * item_height, "Unexpected height %d.\n", HIWORD(ret));

        ret = SendMessageA(hwnd, LVM_APPROXIMATEVIEWRECT, -1, 0);
        ok(LOWORD(ret) == item_width, "Unexpected width %d.\n", LOWORD(ret));
        ok(HIWORD(ret) == header_height + item_count * item_height, "Unexpected height %d.\n", HIWORD(ret));

        ret = SendMessageA(hwnd, LVM_APPROXIMATEVIEWRECT, 2, 0);
        ok(LOWORD(ret) == item_width, "Unexpected width %d.\n", LOWORD(ret));
        ok(HIWORD(ret) == header_height + 2 * item_height, "Unexpected height %d.\n", HIWORD(ret));

        ret = SendMessageA(hwnd, LVM_APPROXIMATEVIEWRECT, 2, MAKELONG(item_width * 2, header_height + 3 * item_height));
        ok(LOWORD(ret) == item_width, "Unexpected width %d.\n", LOWORD(ret));
        ok(HIWORD(ret) == header_height + 2 * item_height, "Unexpected height %d.\n", HIWORD(ret));

        ret = SendMessageA(hwnd, LVM_APPROXIMATEVIEWRECT, -2, MAKELONG(item_width * 2, 0));
        ok(LOWORD(ret) == item_width, "Unexpected width %d.\n", LOWORD(ret));
    todo_wine
        ok(HIWORD(ret) == header_height - 2 * item_height, "Unexpected height %d.\n", HIWORD(ret));

        ret = SendMessageA(hwnd, LVM_APPROXIMATEVIEWRECT, -2, MAKELONG(-1, -1));
        ok(LOWORD(ret) == item_width, "Unexpected width %d.\n", LOWORD(ret));
    todo_wine
        ok(HIWORD(ret) == header_height - 2 * item_height, "Unexpected height %d.\n", HIWORD(ret));
    }

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
    r = SendMessageA(hwnd, LVM_FINDITEMA, -1, (LPARAM)&fi);
    expect(0, r);

    fi.flags = LVFI_STRING | LVFI_PARTIAL;
    r = SendMessageA(hwnd, LVM_FINDITEMA, -1, (LPARAM)&fi);
    expect(0, r);

    fi.flags = LVFI_PARTIAL;
    r = SendMessageA(hwnd, LVM_FINDITEMA, -1, (LPARAM)&fi);
    expect(0, r);

    /* partial string search, inserted text was "foo" */
    strcpy(f, "fo");
    fi.flags = LVFI_STRING | LVFI_PARTIAL;
    fi.psz = f;
    r = SendMessageA(hwnd, LVM_FINDITEMA, -1, (LPARAM)&fi);
    expect(0, r);

    fi.flags = LVFI_STRING;
    r = SendMessageA(hwnd, LVM_FINDITEMA, -1, (LPARAM)&fi);
    expect(-1, r);

    fi.flags = LVFI_PARTIAL;
    r = SendMessageA(hwnd, LVM_FINDITEMA, -1, (LPARAM)&fi);
    expect(0, r);

    /* partial string search, part after start char */
    strcpy(f, "oo");
    fi.flags = LVFI_STRING | LVFI_PARTIAL;
    fi.psz = f;
    r = SendMessageA(hwnd, LVM_FINDITEMA, -1, (LPARAM)&fi);
    expect(-1, r);

    /* try with LVFI_SUBSTRING */
    strcpy(f, "fo");
    fi.flags = LVFI_SUBSTRING;
    fi.psz = f;
    r = SendMessageA(hwnd, LVM_FINDITEMA, -1, (LPARAM)&fi);
    expect(0, r);
    strcpy(f, "f");
    fi.flags = LVFI_SUBSTRING;
    fi.psz = f;
    r = SendMessageA(hwnd, LVM_FINDITEMA, -1, (LPARAM)&fi);
    expect(0, r);
    strcpy(f, "o");
    fi.flags = LVFI_SUBSTRING;
    fi.psz = f;
    r = SendMessageA(hwnd, LVM_FINDITEMA, -1, (LPARAM)&fi);
    expect(-1, r);

    strcpy(f, "o");
    fi.flags = LVFI_SUBSTRING | LVFI_PARTIAL;
    fi.psz = f;
    r = SendMessageA(hwnd, LVM_FINDITEMA, -1, (LPARAM)&fi);
    expect(-1, r);

    strcpy(f, "f");
    fi.flags = LVFI_SUBSTRING | LVFI_STRING;
    fi.psz = f;
    r = SendMessageA(hwnd, LVM_FINDITEMA, -1, (LPARAM)&fi);
    expect(0, r);

    fi.flags = LVFI_SUBSTRING | LVFI_PARTIAL;
    r = SendMessageA(hwnd, LVM_FINDITEMA, -1, (LPARAM)&fi);
    expect(0, r);

    DestroyWindow(hwnd);
}

static void test_LVS_EX_HEADERINALLVIEWS(void)
{
    HWND hwnd, header;
    DWORD style;

    hwnd = create_listview_control(LVS_ICON);

    SendMessageA(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_HEADERINALLVIEWS,
                                                    LVS_EX_HEADERINALLVIEWS);

    header = (HWND)SendMessageA(hwnd, LVM_GETHEADER, 0, 0);
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
    SendMessageA(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_HEADERINALLVIEWS, 0);
    header = (HWND)SendMessageA(hwnd, LVM_GETHEADER, 0, 0);
    ok(IsWindow(header), "Expected header to be created\n");
    style = GetWindowLongA(header, GWL_STYLE);
    ok(!(style & HDS_HIDDEN), "HDS_HIDDEN not expected\n");

    DestroyWindow(hwnd);

    /* check other styles */
    hwnd = create_listview_control(LVS_LIST);
    SendMessageA(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_HEADERINALLVIEWS,
                                                    LVS_EX_HEADERINALLVIEWS);
    header = (HWND)SendMessageA(hwnd, LVM_GETHEADER, 0, 0);
    ok(IsWindow(header), "Expected header to be created\n");
    DestroyWindow(hwnd);

    hwnd = create_listview_control(LVS_SMALLICON);
    SendMessageA(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_HEADERINALLVIEWS,
                                                    LVS_EX_HEADERINALLVIEWS);
    header = (HWND)SendMessageA(hwnd, LVM_GETHEADER, 0, 0);
    ok(IsWindow(header), "Expected header to be created\n");
    DestroyWindow(hwnd);

    hwnd = create_listview_control(LVS_REPORT);
    SendMessageA(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_HEADERINALLVIEWS,
                                                    LVS_EX_HEADERINALLVIEWS);
    header = (HWND)SendMessageA(hwnd, LVM_GETHEADER, 0, 0);
    ok(IsWindow(header), "Expected header to be created\n");
    DestroyWindow(hwnd);
}

static void test_hover(void)
{
    HWND hwnd, fg;
    DWORD r;

    hwnd = create_listview_control(LVS_ICON);
    SetForegroundWindow(hwndparent);
    fg = GetForegroundWindow();
    if (fg != hwndparent)
    {
        skip("Window is not in the foreground. Skipping hover tests.\n");
        DestroyWindow(hwnd);
        return;
    }

    /* test WM_MOUSEHOVER forwarding */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    r = SendMessageA(hwnd, WM_MOUSEHOVER, 0, 0);
    expect(0, r);
    ok_sequence(sequences, PARENT_SEQ_INDEX, hover_parent, "NM_HOVER allow test", TRUE);
    g_block_hover = TRUE;
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    r = SendMessageA(hwnd, WM_MOUSEHOVER, 0, 0);
    expect(0, r);
    ok_sequence(sequences, PARENT_SEQ_INDEX, hover_parent, "NM_HOVER block test", TRUE);
    g_block_hover = FALSE;

    r = SendMessageA(hwnd, LVM_SETHOVERTIME, 0, 500);
    expect(HOVER_DEFAULT, r);
    r = SendMessageA(hwnd, LVM_GETHOVERTIME, 0, 0);
    expect(500, r);

    DestroyWindow(hwnd);
}

static void test_destroynotify(void)
{
    HWND hwnd;
    BOOL ret;

    hwnd = create_listview_control(LVS_REPORT);
    ok(hwnd != NULL, "failed to create listview window\n");

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    DestroyWindow(hwnd);
    ok_sequence(sequences, COMBINED_SEQ_INDEX, listview_destroy, "check destroy order", FALSE);

    /* same for ownerdata list */
    hwnd = create_listview_control(LVS_REPORT|LVS_OWNERDATA);
    ok(hwnd != NULL, "failed to create listview window\n");

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    DestroyWindow(hwnd);
    ok_sequence(sequences, COMBINED_SEQ_INDEX, listview_ownerdata_destroy, "check destroy order, ownerdata", FALSE);

    hwnd = create_listview_control(LVS_REPORT|LVS_OWNERDATA);
    ok(hwnd != NULL, "failed to create listview window\n");

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    ret = SendMessageA(hwnd, LVM_DELETEALLITEMS, 0, 0);
    ok(ret == TRUE, "got %d\n", ret);
    ok_sequence(sequences, COMBINED_SEQ_INDEX, listview_ownerdata_deleteall, "deleteall ownerdata", FALSE);
    DestroyWindow(hwnd);
}

static void test_header_notification(void)
{
    static char textA[] = "newtext";
    HWND list, header;
    HDITEMA item;
    NMHEADERA nmh;
    LVCOLUMNA col;
    DWORD ret;
    BOOL r;

    list = create_listview_control(LVS_REPORT);
    ok(list != NULL, "failed to create listview window\n");

    memset(&col, 0, sizeof(col));
    col.mask = LVCF_WIDTH;
    col.cx = 100;
    ret = SendMessageA(list, LVM_INSERTCOLUMNA, 0, (LPARAM)&col);
    expect(0, ret);

    /* check list parent notification after header item changed,
       this test should be placed before header subclassing to avoid
       Listview -> Header messages to be logged */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    col.mask = LVCF_TEXT;
    col.pszText = textA;
    r = SendMessageA(list, LVM_SETCOLUMNA, 0, (LPARAM)&col);
    expect(TRUE, r);

    ok_sequence(sequences, LISTVIEW_SEQ_INDEX, listview_header_changed_seq,
                "header notify, listview", FALSE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq,
                "header notify, parent", FALSE);

    header = subclass_header(list);

    ret = SendMessageA(header, HDM_GETITEMCOUNT, 0, 0);
    expect(1, ret);

    memset(&item, 0, sizeof(item));
    item.mask = HDI_WIDTH;
    ret = SendMessageA(header, HDM_GETITEMA, 0, (LPARAM)&item);
    expect(1, ret);
    expect(100, item.cxy);

    nmh.hdr.hwndFrom = header;
    nmh.hdr.idFrom = GetWindowLongPtrA(header, GWLP_ID);
    nmh.hdr.code = HDN_ITEMCHANGEDA;
    nmh.iItem = 0;
    nmh.iButton = 0;
    item.mask = HDI_WIDTH;
    item.cxy = 50;
    nmh.pitem = &item;
    ret = SendMessageA(list, WM_NOTIFY, 0, (LPARAM)&nmh);
    expect(0, ret);

    DestroyWindow(list);
}

static void test_header_notification2(void)
{
    static char textA[] = "newtext";
    HWND list, header;
    HDITEMW itemW;
    NMHEADERW nmhdr;
    LVCOLUMNA col;
    DWORD ret;
    WCHAR buffer[100];
    struct message parent_header_notify_seq[] = {
        { WM_NOTIFY, sent|id, 0, 0, 0 },
        { 0 }
    };

    list = create_listview_control(LVS_REPORT);
    ok(list != NULL, "failed to create listview window\n");

    memset(&col, 0, sizeof(col));
    col.mask = LVCF_WIDTH | LVCF_TEXT;
    col.cx = 100;
    col.pszText = textA;
    ret = SendMessageA(list, LVM_INSERTCOLUMNA, 0, (LPARAM)&col);
    expect(0, ret);

    header = (HWND)SendMessageA(list, LVM_GETHEADER, 0, 0);
    ok(header != 0, "No header\n");
    memset(&itemW, 0, sizeof(itemW));
    itemW.mask = HDI_WIDTH | HDI_ORDER | HDI_TEXT;
    itemW.pszText = buffer;
    itemW.cchTextMax = ARRAY_SIZE(buffer);
    ret = SendMessageW(header, HDM_GETITEMW, 0, (LPARAM)&itemW);
    expect(1, ret);

    nmhdr.hdr.hwndFrom = header;
    nmhdr.hdr.idFrom = GetWindowLongPtrW(header, GWLP_ID);
    nmhdr.iItem = 0;
    nmhdr.iButton = 0;
    nmhdr.pitem = &itemW;

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    nmhdr.hdr.code = HDN_ITEMCHANGINGW;
    ret = SendMessageW(list, WM_NOTIFY, 0, (LPARAM)&nmhdr);
    ok(ret == 0, "got %d\n", ret);
    parent_header_notify_seq[0].id = HDN_ITEMCHANGINGA;
    ok_sequence(sequences, PARENT_SEQ_INDEX, parent_header_notify_seq,
                "header notify, parent", TRUE);
    todo_wine
    ok(nmhdr.hdr.code == HDN_ITEMCHANGINGA, "Expected ANSI notification code\n");
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    nmhdr.hdr.code = HDN_ITEMCHANGEDW;
    ret = SendMessageW(list, WM_NOTIFY, 0, (LPARAM)&nmhdr);
    ok(ret == 0, "got %d\n", ret);
    parent_header_notify_seq[0].id = HDN_ITEMCHANGEDA;
    ok_sequence(sequences, PARENT_SEQ_INDEX, parent_header_notify_seq,
                "header notify, parent", TRUE);
    todo_wine
    ok(nmhdr.hdr.code == HDN_ITEMCHANGEDA, "Expected ANSI notification code\n");
    /* HDN_ITEMCLICK sets focus to list, which generates messages we don't want to check */
    SetFocus(list);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    nmhdr.hdr.code = HDN_ITEMCLICKW;
    ret = SendMessageW(list, WM_NOTIFY, 0, (LPARAM)&nmhdr);
    ok(ret == 0, "got %d\n", ret);
    ok_sequence(sequences, PARENT_SEQ_INDEX, parent_header_click_seq,
                "header notify, parent", FALSE);
    ok(nmhdr.hdr.code == HDN_ITEMCLICKA, "Expected ANSI notification code\n");
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    nmhdr.hdr.code = HDN_ITEMDBLCLICKW;
    ret = SendMessageW(list, WM_NOTIFY, 0, (LPARAM)&nmhdr);
    ok(ret == 0, "got %d\n", ret);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq,
                "header notify, parent", FALSE);
    ok(nmhdr.hdr.code == HDN_ITEMDBLCLICKW, "Expected Unicode notification code\n");
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    nmhdr.hdr.code = HDN_DIVIDERDBLCLICKW;
    ret = SendMessageW(list, WM_NOTIFY, 0, (LPARAM)&nmhdr);
    ok(ret == 0, "got %d\n", ret);
    ok_sequence(sequences, PARENT_SEQ_INDEX, parent_header_divider_dclick_seq,
                "header notify, parent", TRUE);
    ok(nmhdr.hdr.code == HDN_DIVIDERDBLCLICKA, "Expected ANSI notification code\n");
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    nmhdr.hdr.code = HDN_BEGINTRACKW;
    ret = SendMessageW(list, WM_NOTIFY, 0, (LPARAM)&nmhdr);
    ok(ret == 0, "got %d\n", ret);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq,
                "header notify, parent", FALSE);
    ok(nmhdr.hdr.code == HDN_BEGINTRACKW, "Expected Unicode notification code\n");
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    nmhdr.hdr.code = HDN_ENDTRACKW;
    ret = SendMessageW(list, WM_NOTIFY, 0, (LPARAM)&nmhdr);
    ok(ret == 0, "got %d\n", ret);
    parent_header_notify_seq[0].id = HDN_ENDTRACKA;
    ok_sequence(sequences, PARENT_SEQ_INDEX, parent_header_notify_seq,
                "header notify, parent", FALSE);
    ok(nmhdr.hdr.code == HDN_ENDTRACKA, "Expected ANSI notification code\n");
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    nmhdr.hdr.code = HDN_TRACKW;
    ret = SendMessageW(list, WM_NOTIFY, 0, (LPARAM)&nmhdr);
    ok(ret == 0, "got %d\n", ret);
    parent_header_notify_seq[0].id = HDN_TRACKA;
    ok_sequence(sequences, PARENT_SEQ_INDEX, parent_header_notify_seq,
                "header notify, parent", FALSE);
    ok(nmhdr.hdr.code == HDN_TRACKA, "Expected ANSI notification code\n");
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    nmhdr.hdr.code = HDN_BEGINDRAG;
    ret = SendMessageW(list, WM_NOTIFY, 0, (LPARAM)&nmhdr);
    ok(ret == 1, "got %d\n", ret);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq,
                "header notify, parent", FALSE);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    nmhdr.hdr.code = HDN_ENDDRAG;
    ret = SendMessageW(list, WM_NOTIFY, 0, (LPARAM)&nmhdr);
    ok(ret == 0, "got %d\n", ret);
    parent_header_notify_seq[0].id = HDN_ENDDRAG;
    ok_sequence(sequences, PARENT_SEQ_INDEX, parent_header_notify_seq,
                "header notify, parent", FALSE);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    nmhdr.hdr.code = HDN_FILTERCHANGE;
    ret = SendMessageW(list, WM_NOTIFY, 0, (LPARAM)&nmhdr);
    ok(ret == 0, "got %d\n", ret);
    parent_header_notify_seq[0].id = HDN_FILTERCHANGE;
    parent_header_notify_seq[0].flags |= optional; /* NT4 does not send this message */
    ok_sequence(sequences, PARENT_SEQ_INDEX, parent_header_notify_seq,
                "header notify, parent", FALSE);
    parent_header_notify_seq[0].flags &= ~optional;
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    nmhdr.hdr.code = HDN_BEGINFILTEREDIT;
    ret = SendMessageW(list, WM_NOTIFY, 0, (LPARAM)&nmhdr);
    ok(ret == 0, "got %d\n", ret);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq,
                "header notify, parent", FALSE);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    nmhdr.hdr.code = HDN_ENDFILTEREDIT;
    ret = SendMessageW(list, WM_NOTIFY, 0, (LPARAM)&nmhdr);
    ok(ret == 0, "got %d\n", ret);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq,
                "header notify, parent", FALSE);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    nmhdr.hdr.code = HDN_ITEMSTATEICONCLICK;
    ret = SendMessageW(list, WM_NOTIFY, 0, (LPARAM)&nmhdr);
    ok(ret == 0, "got %d\n", ret);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq,
                "header notify, parent", FALSE);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    nmhdr.hdr.code = HDN_ITEMKEYDOWN;
    ret = SendMessageW(list, WM_NOTIFY, 0, (LPARAM)&nmhdr);
    ok(ret == 0, "got %d\n", ret);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq,
                "header notify, parent", FALSE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

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
    pImageList_Destroy(himl);

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
    item.cchTextMax = ARRAY_SIZE(buff);
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
    ret = SendMessageA(hwnd, LVM_SETITEMTEXTA, 0, 0);
    expect(FALSE, ret);

    ret = SendMessageA(hwnd, LVM_SETITEMTEXTW, 0, 0);
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

static void test_LVM_REDRAWITEMS(void)
{
    HWND list;
    DWORD ret;

    list = create_listview_control(LVS_ICON);
    ok(list != NULL, "failed to create listview window\n");

    ret = SendMessageA(list, LVM_REDRAWITEMS, 0, 0);
    expect(TRUE, ret);

    insert_item(list, 0);

    ret = SendMessageA(list, LVM_REDRAWITEMS, -1, 0);
    expect(TRUE, ret);

    ret = SendMessageA(list, LVM_REDRAWITEMS, 0, -1);
    expect(TRUE, ret);

    ret = SendMessageA(list, LVM_REDRAWITEMS, 0, 0);
    expect(TRUE, ret);

    ret = SendMessageA(list, LVM_REDRAWITEMS, 0, 1);
    expect(TRUE, ret);

    ret = SendMessageA(list, LVM_REDRAWITEMS, 0, 2);
    expect(TRUE, ret);

    ret = SendMessageA(list, LVM_REDRAWITEMS, 1, 0);
    expect(TRUE, ret);

    ret = SendMessageA(list, LVM_REDRAWITEMS, 2, 3);
    expect(TRUE, ret);

    DestroyWindow(list);
}

static void test_imagelists(void)
{
    HWND hwnd, header;
    HIMAGELIST himl1, himl2, himl3;
    LRESULT ret;

    himl1 = pImageList_Create(40, 40, 0, 4, 4);
    himl2 = pImageList_Create(40, 40, 0, 4, 4);
    himl3 = pImageList_Create(40, 40, 0, 4, 4);
    ok(himl1 != NULL, "Failed to create imagelist\n");
    ok(himl2 != NULL, "Failed to create imagelist\n");
    ok(himl3 != NULL, "Failed to create imagelist\n");

    hwnd = create_listview_control(LVS_REPORT | LVS_SHAREIMAGELISTS);
    header = subclass_header(hwnd);

    ok(header != NULL, "Expected header\n");
    ret = SendMessageA(header, HDM_GETIMAGELIST, 0, 0);
    ok(ret == 0, "Expected no imagelist, got %p\n", (HIMAGELIST)ret);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    ret = SendMessageW(hwnd, LVM_SETIMAGELIST, LVSIL_NORMAL, (LPARAM)himl1);
    ok(ret == 0, "Expected no imagelist, got %p\n", (HIMAGELIST)ret);
    ok_sequence(sequences, LISTVIEW_SEQ_INDEX, listview_set_imagelist,
                "set normal image list", FALSE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    ret = SendMessageW(hwnd, LVM_SETIMAGELIST, LVSIL_STATE, (LPARAM)himl2);
    ok(ret == 0, "Expected no imagelist, got %p\n", (HIMAGELIST)ret);
    ok_sequence(sequences, LISTVIEW_SEQ_INDEX, listview_set_imagelist,
                "set state image list", TRUE);

    ret = SendMessageA(header, HDM_GETIMAGELIST, 0, 0);
    ok(ret == 0, "Expected no imagelist, got %p\n", (HIMAGELIST)ret);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    ret = SendMessageW(hwnd, LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)himl3);
    ok(ret == 0, "Expected no imagelist, got %p\n", (HIMAGELIST)ret);
    ok_sequence(sequences, LISTVIEW_SEQ_INDEX, listview_header_set_imagelist,
                "set small image list", FALSE);

    ret = SendMessageA(header, HDM_GETIMAGELIST, 0, 0);
    ok((HIMAGELIST)ret == himl3, "Expected imagelist %p, got %p\n", himl3, (HIMAGELIST)ret);
    DestroyWindow(hwnd);

    hwnd = create_listview_control(WS_VISIBLE | LVS_ICON);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    ret = SendMessageW(hwnd, LVM_SETIMAGELIST, LVSIL_NORMAL, (LPARAM)himl1);
    ok(ret == 0, "Expected no imagelist, got %p\n", (HIMAGELIST)ret);
    ok_sequence(sequences, LISTVIEW_SEQ_INDEX, listview_set_imagelist,
                "set normal image list", FALSE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    ret = SendMessageW(hwnd, LVM_SETIMAGELIST, LVSIL_STATE, (LPARAM)himl2);
    ok(ret == 0, "Expected no imagelist, got %p\n", (HIMAGELIST)ret);
    ok_sequence(sequences, LISTVIEW_SEQ_INDEX, listview_set_imagelist,
                "set state image list", FALSE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    ret = SendMessageW(hwnd, LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)himl3);
    ok(ret == 0, "Expected no imagelist, got %p\n", (HIMAGELIST)ret);
    ok_sequence(sequences, LISTVIEW_SEQ_INDEX, listview_set_imagelist,
                "set small image list", FALSE);

    header = (HWND)SendMessageA(hwnd, LVM_GETHEADER, 0, 0);
    ok(header == NULL, "Expected no header, got %p\n", header);

    SetWindowLongPtrA(hwnd, GWL_STYLE, GetWindowLongPtrA(hwnd, GWL_STYLE) | LVS_REPORT);

    header = (HWND)SendMessageA(hwnd, LVM_GETHEADER, 0, 0);
    ok(header != NULL, "Expected header, got NULL\n");

    ret = SendMessageA(header, HDM_GETIMAGELIST, 0, 0);
    ok((HIMAGELIST)ret == himl3, "Expected imagelist %p, got %p\n", himl3, (HIMAGELIST)ret);

    DestroyWindow(hwnd);
}

static void test_deleteitem(void)
{
    LVITEMA item;
    UINT state;
    HWND hwnd;
    BOOL ret;

    hwnd = create_listview_control(LVS_REPORT);

    insert_item(hwnd, 0);
    insert_item(hwnd, 0);
    insert_item(hwnd, 0);
    insert_item(hwnd, 0);
    insert_item(hwnd, 0);

    g_focus_test_LVN_DELETEITEM = TRUE;

    /* delete focused item (not the last index) */
    item.stateMask = LVIS_FOCUSED;
    item.state = LVIS_FOCUSED;
    ret = SendMessageA(hwnd, LVM_SETITEMSTATE, 2, (LPARAM)&item);
    ok(ret == TRUE, "got %d\n", ret);
    ret = SendMessageA(hwnd, LVM_DELETEITEM, 2, 0);
    ok(ret == TRUE, "got %d\n", ret);
    /* next item gets focus */
    state = SendMessageA(hwnd, LVM_GETITEMSTATE, 2, LVIS_FOCUSED);
    ok(state == LVIS_FOCUSED, "got %x\n", state);

    /* focus last item and delete it */
    item.stateMask = LVIS_FOCUSED;
    item.state = LVIS_FOCUSED;
    ret = SendMessageA(hwnd, LVM_SETITEMSTATE, 3, (LPARAM)&item);
    ok(ret == TRUE, "got %d\n", ret);
    ret = SendMessageA(hwnd, LVM_DELETEITEM, 3, 0);
    ok(ret == TRUE, "got %d\n", ret);
    /* new last item gets focus */
    state = SendMessageA(hwnd, LVM_GETITEMSTATE, 2, LVIS_FOCUSED);
    ok(state == LVIS_FOCUSED, "got %x\n", state);

    /* focus first item and delete it */
    item.stateMask = LVIS_FOCUSED;
    item.state = LVIS_FOCUSED;
    ret = SendMessageA(hwnd, LVM_SETITEMSTATE, 0, (LPARAM)&item);
    ok(ret == TRUE, "got %d\n", ret);
    ret = SendMessageA(hwnd, LVM_DELETEITEM, 0, 0);
    ok(ret == TRUE, "got %d\n", ret);
    /* new first item gets focus */
    state = SendMessageA(hwnd, LVM_GETITEMSTATE, 0, LVIS_FOCUSED);
    ok(state == LVIS_FOCUSED, "got %x\n", state);

    g_focus_test_LVN_DELETEITEM = FALSE;

    DestroyWindow(hwnd);
}

static void test_insertitem(void)
{
    LVITEMA item;
    UINT state;
    HWND hwnd;
    INT ret;

    hwnd = create_listview_control(LVS_REPORT);

    /* insert item 0 focused */
    item.mask = LVIF_STATE;
    item.state = LVIS_FOCUSED;
    item.stateMask = LVIS_FOCUSED;
    item.iItem = 0;
    item.iSubItem = 0;
    ret = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM)&item);
    ok(ret == 0, "got %d\n", ret);

    state = SendMessageA(hwnd, LVM_GETITEMSTATE, 0, LVIS_FOCUSED);
    ok(state == LVIS_FOCUSED, "got %x\n", state);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    /* insert item 1, focus shift */
    item.mask = LVIF_STATE;
    item.state = LVIS_FOCUSED;
    item.stateMask = LVIS_FOCUSED;
    item.iItem = 1;
    item.iSubItem = 0;
    ret = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM)&item);
    ok(ret == 1, "got %d\n", ret);

    ok_sequence(sequences, PARENT_SEQ_INDEX, parent_insert_focused_seq, "insert focused", TRUE);

    state = SendMessageA(hwnd, LVM_GETITEMSTATE, 1, LVIS_FOCUSED);
    ok(state == LVIS_FOCUSED, "got %x\n", state);

    /* insert item 2, no focus shift */
    item.mask = LVIF_STATE;
    item.state = 0;
    item.stateMask = LVIS_FOCUSED;
    item.iItem = 2;
    item.iSubItem = 0;
    ret = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM)&item);
    ok(ret == 2, "got %d\n", ret);

    state = SendMessageA(hwnd, LVM_GETITEMSTATE, 1, LVIS_FOCUSED);
    ok(state == LVIS_FOCUSED, "got %x\n", state);

    DestroyWindow(hwnd);
}

static void test_header_proc(void)
{
    HWND hwnd, header, hdr;
    WNDPROC proc1, proc2;

    hwnd = create_listview_control(LVS_REPORT);

    header = (HWND)SendMessageA(hwnd, LVM_GETHEADER, 0, 0);
    ok(header != NULL, "got %p\n", header);

    hdr = CreateWindowExA(0, WC_HEADERA, NULL,
			     WS_BORDER|WS_VISIBLE|HDS_BUTTONS|HDS_HORZ,
			     0, 0, 0, 0,
			     NULL, NULL, NULL, NULL);
    ok(hdr != NULL, "got %p\n", hdr);

    proc1 = (WNDPROC)GetWindowLongPtrW(header, GWLP_WNDPROC);
    proc2 = (WNDPROC)GetWindowLongPtrW(hdr, GWLP_WNDPROC);
    ok(proc1 == proc2, "got %p, expected %p\n", proc1, proc2);

    DestroyWindow(hdr);
    DestroyWindow(hwnd);
}

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

static void test_oneclickactivate(void)
{
    TRACKMOUSEEVENT track;
    char item1[] = "item1";
    LVITEMA item;
    HWND hwnd, fg;
    RECT rect;
    INT r;
    POINT orig_pos;

    hwnd = CreateWindowExA(0, WC_LISTVIEWA, "foo", WS_VISIBLE|WS_CHILD|LVS_LIST,
            10, 10, 100, 200, hwndparent, NULL, NULL, NULL);
    ok(hwnd != NULL, "failed to create listview window\n");
    r = SendMessageA(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_ONECLICKACTIVATE);
    ok(r == 0, "should return zero\n");

    SetForegroundWindow(hwndparent);
    flush_events();
    fg = GetForegroundWindow();
    if (fg != hwndparent)
    {
        skip("Window is not in the foreground. Skipping oneclickactivate tests.\n");
        DestroyWindow(hwnd);
        return;
    }

    item.mask = LVIF_TEXT;
    item.iItem = 0;
    item.iSubItem = 0;
    item.iImage = 0;
    item.pszText = item1;
    r = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &item);
    ok(r == 0, "should not fail\n");

    GetWindowRect(hwnd, &rect);
    GetCursorPos(&orig_pos);
    SetCursorPos(rect.left+5, rect.top+5);
    flush_events();
    r = SendMessageA(hwnd, WM_MOUSEMOVE, MAKELONG(1, 1), 0);
    expect(0, r);

    track.cbSize = sizeof(track);
    track.dwFlags = TME_QUERY;
    p_TrackMouseEvent(&track);
    ok(track.hwndTrack == hwnd, "hwndTrack != hwnd\n");
    ok(track.dwFlags == TME_LEAVE, "dwFlags = %x\n", track.dwFlags);

    r = SendMessageA(hwnd, LVM_GETSELECTEDCOUNT, 0, 0);
    expect(0, r);
    r = SendMessageA(hwnd, WM_MOUSEHOVER, MAKELONG(1, 1), 0);
    expect(0, r);
    r = SendMessageA(hwnd, LVM_GETSELECTEDCOUNT, 0, 0);
    expect(1, r);

    DestroyWindow(hwnd);
    SetCursorPos(orig_pos.x, orig_pos.y);
}

static void test_callback_mask(void)
{
    LVITEMA item;
    DWORD mask;
    HWND hwnd;
    BOOL ret;

    hwnd = create_listview_control(LVS_REPORT);

    ret = SendMessageA(hwnd, LVM_SETCALLBACKMASK, ~0u, 0);
    ok(ret, "got %d\n", ret);

    ret = SendMessageA(hwnd, LVM_SETCALLBACKMASK, ~0u, 1);
    ok(ret, "got %d\n", ret);

    mask = SendMessageA(hwnd, LVM_GETCALLBACKMASK, 0, 0);
    ok(mask == ~0u, "got 0x%08x\n", mask);

    /* Ask for state, invalid subitem. */
    insert_item(hwnd, 0);

    ret = SendMessageA(hwnd, LVM_SETCALLBACKMASK, LVIS_FOCUSED, 0);
    ok(ret, "Failed to set callback mask.\n");

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    memset(&item, 0, sizeof(item));
    item.iSubItem = 1;
    item.mask = LVIF_STATE;
    item.stateMask = LVIS_SELECTED;
    ret = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM)&item);
    ok(ret, "Failed to get item data.\n");

    memset(&item, 0, sizeof(item));
    item.mask = LVIF_STATE;
    item.stateMask = LVIS_SELECTED;
    ret = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM)&item);
    ok(ret, "Failed to get item data.\n");

    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq, "parent seq, callback mask/invalid subitem 1", TRUE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    memset(&item, 0, sizeof(item));
    memset(&g_itema, 0, sizeof(g_itema));
    item.iSubItem = 1;
    item.mask = LVIF_STATE;
    item.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
    ret = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM)&item);
    ok(ret, "Failed to get item data.\n");
    ok(g_itema.iSubItem == 1, "Unexpected LVN_DISPINFO subitem %d.\n", g_itema.iSubItem);
    ok(g_itema.stateMask == LVIS_FOCUSED, "Unexpected state mask %#x.\n", g_itema.stateMask);

    ok_sequence(sequences, PARENT_SEQ_INDEX, single_getdispinfo_parent_seq,
            "parent seq, callback mask/invalid subitem 2", FALSE);

    DestroyWindow(hwnd);

    /* LVS_OWNERDATA, mask LVIS_FOCUSED */
    hwnd = create_listview_control(LVS_REPORT | LVS_OWNERDATA);

    mask = SendMessageA(hwnd, LVM_GETCALLBACKMASK, 0, 0);
    ok(mask == 0, "Unexpected callback mask %#x.\n", mask);

    ret = SendMessageA(hwnd, LVM_SETCALLBACKMASK, LVIS_FOCUSED, 0);
    ok(ret, "Failed to set callback mask, %d\n", ret);

    mask = SendMessageA(hwnd, LVM_GETCALLBACKMASK, 0, 0);
    ok(mask == LVIS_FOCUSED, "Unexpected callback mask %#x.\n", mask);

    ret = SendMessageA(hwnd, LVM_SETITEMCOUNT, 1, 0);
    ok(ret, "Failed to set item count.\n");

    ret = SendMessageA(hwnd, LVM_GETSELECTIONMARK, 0, 0);
    ok(ret == -1, "Unexpected selection mark, %d\n", ret);

    item.stateMask = LVIS_FOCUSED;
    item.state = LVIS_FOCUSED;
    ret = SendMessageA(hwnd, LVM_SETITEMSTATE, 0, (LPARAM)&item);
    ok(ret, "Failed to set item state.\n");

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    ret = SendMessageA(hwnd, LVM_GETNEXTITEM, -1, LVNI_FOCUSED);
todo_wine
    ok(ret == 0, "Unexpected focused item, ret %d\n", ret);

    ret = SendMessageA(hwnd, LVM_GETSELECTIONMARK, 0, 0);
todo_wine
    ok(ret == 0, "Unexpected selection mark, %d\n", ret);

    ret = SendMessageA(hwnd, LVM_SETITEMCOUNT, 0, 0);
    ok(ret, "Failed to set item count.\n");

    ret = SendMessageA(hwnd, LVM_GETNEXTITEM, -1, LVNI_FOCUSED);
    ok(ret == -1, "Unexpected focused item, ret %d\n", ret);

    ret = SendMessageA(hwnd, LVM_GETSELECTIONMARK, 0, 0);
    ok(ret == -1, "Unexpected selection mark, %d\n", ret);

    ret = SendMessageA(hwnd, LVM_SETITEMCOUNT, 1, 0);
    ok(ret, "Failed to set item count.\n");

    ret = SendMessageA(hwnd, LVM_GETNEXTITEM, -1, LVNI_FOCUSED);
    ok(ret == -1, "Unexpected focused item, ret %d\n", ret);

    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq, "parent seq, owner data/focus 1", FALSE);

    /* LVS_OWNDERDATA, empty mask */
    ret = SendMessageA(hwnd, LVM_SETCALLBACKMASK, 0, 0);
    ok(ret, "Failed to set callback mask, %d\n", ret);

    ret = SendMessageA(hwnd, LVM_SETITEMCOUNT, 1, 0);
    ok(ret, "Failed to set item count.\n");

    ret = SendMessageA(hwnd, LVM_GETSELECTIONMARK, 0, 0);
    ok(ret == -1, "Unexpected selection mark, %d\n", ret);

    item.stateMask = LVIS_FOCUSED;
    item.state = LVIS_FOCUSED;
    ret = SendMessageA(hwnd, LVM_SETITEMSTATE, 0, (LPARAM)&item);
    ok(ret, "Failed to set item state.\n");

    ret = SendMessageA(hwnd, LVM_GETSELECTIONMARK, 0, 0);
    ok(ret == 0, "Unexpected selection mark, %d\n", ret);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    ret = SendMessageA(hwnd, LVM_GETNEXTITEM, -1, LVNI_FOCUSED);
    ok(ret == 0, "Unexpected focused item, ret %d\n", ret);

    ret = SendMessageA(hwnd, LVM_SETITEMCOUNT, 0, 0);
    ok(ret, "Failed to set item count.\n");

    ret = SendMessageA(hwnd, LVM_GETNEXTITEM, -1, LVNI_FOCUSED);
    ok(ret == -1, "Unexpected focused item, ret %d\n", ret);

    ret = SendMessageA(hwnd, LVM_GETSELECTIONMARK, 0, 0);
todo_wine
    ok(ret == -1, "Unexpected selection mark, %d\n", ret);

    ret = SendMessageA(hwnd, LVM_SETITEMCOUNT, 1, 0);
    ok(ret, "Failed to set item count.\n");

    ret = SendMessageA(hwnd, LVM_GETNEXTITEM, -1, LVNI_FOCUSED);
    ok(ret == -1, "Unexpected focused item, ret %d\n", ret);

    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq, "parent seq, owner data/focus 2", FALSE);

    /* 2 items, focus on index 0, reduce to 1 item. */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    ret = SendMessageA(hwnd, LVM_SETITEMCOUNT, 2, 0);
    ok(ret, "Failed to set item count.\n");

    ret = SendMessageA(hwnd, LVM_SETITEMSTATE, 0, (LPARAM)&item);
    ok(ret, "Failed to set item state.\n");

    ret = SendMessageA(hwnd, LVM_GETNEXTITEM, -1, LVNI_FOCUSED);
    ok(ret == 0, "Unexpected focused item, ret %d\n", ret);

    ret = SendMessageA(hwnd, LVM_SETITEMCOUNT, 1, 0);
    ok(ret, "Failed to set item count.\n");

    ret = SendMessageA(hwnd, LVM_GETNEXTITEM, -1, LVNI_FOCUSED);
    ok(ret == 0, "Unexpected focused item, ret %d\n", ret);

    ok_sequence(sequences, PARENT_SEQ_INDEX, parent_focus_change_ownerdata_seq,
        "parent seq, owner data/focus 3", TRUE);

    DestroyWindow(hwnd);
}

static void test_state_image(void)
{
    static const DWORD styles[] =
    {
        LVS_ICON,
        LVS_REPORT,
        LVS_SMALLICON,
        LVS_LIST,
    };
    int i;

    for (i = 0; i < ARRAY_SIZE(styles); i++)
    {
        static char text[] = "Item";
        static char subtext[] = "Subitem";
        char buff[16];
        LVITEMA item;
        HWND hwnd;
        int r;

        hwnd = create_listview_control(styles[i]);

        insert_column(hwnd, 0);
        insert_column(hwnd, 1);

        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = 0;
        item.iSubItem = 0;
        item.pszText = text;
        item.lParam = 123456;
        r = SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM)&item);
        ok(r == 0, "Failed to insert an item.\n");

        item.mask = LVIF_STATE;
        item.state = INDEXTOSTATEIMAGEMASK(1) | LVIS_SELECTED | LVIS_FOCUSED;
        item.stateMask = LVIS_STATEIMAGEMASK | LVIS_SELECTED | LVIS_FOCUSED;
        item.iItem = 0;
        item.iSubItem = 0;
        r = SendMessageA(hwnd, LVM_SETITEMA, 0, (LPARAM)&item);
        ok(r, "Failed to set item state.\n");

        item.mask = LVIF_TEXT;
        item.iItem = 0;
        item.iSubItem = 1;
        item.pszText = subtext;
        r = SendMessageA(hwnd, LVM_SETITEMA, 0, (LPARAM)&item);
        ok(r, "Failed to set subitem text.\n");

        item.mask = LVIF_STATE | LVIF_PARAM;
        item.stateMask = ~0u;
        item.state = 0;
        item.iItem = 0;
        item.iSubItem = 0;
        item.lParam = 0;
        r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM)&item);
        ok(r, "Failed to get item state.\n");
        ok(item.state == (INDEXTOSTATEIMAGEMASK(1) | LVIS_SELECTED | LVIS_FOCUSED),
            "Unexpected item state %#x.\n", item.state);
        ok(item.lParam == 123456, "Unexpected lParam %ld.\n", item.lParam);

        item.mask = 0;
        item.stateMask = ~0u;
        item.state = INDEXTOSTATEIMAGEMASK(2);
        item.iItem = 0;
        item.iSubItem = 1;
        r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM)&item);
        ok(r, "Failed to get subitem state.\n");
        ok(item.state == INDEXTOSTATEIMAGEMASK(2), "Unexpected state %#x.\n", item.state);

        item.mask = LVIF_STATE | LVIF_PARAM;
        item.stateMask = ~0u;
        item.state = INDEXTOSTATEIMAGEMASK(2);
        item.iItem = 0;
        item.iSubItem = 1;
        item.lParam = 0;
        r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM)&item);
        ok(r, "Failed to get subitem state.\n");
        ok(item.state == 0, "Unexpected state %#x.\n", item.state);
        ok(item.lParam == 123456, "Unexpected lParam %ld.\n", item.lParam);

        item.mask = LVIF_STATE;
        item.stateMask = LVIS_FOCUSED;
        item.state = 0;
        item.iItem = 0;
        item.iSubItem = 1;
        r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM)&item);
        ok(r, "Failed to get subitem state.\n");
        ok(item.state == 0, "Unexpected state %#x.\n", item.state);

        item.mask = LVIF_STATE;
        item.stateMask = ~0u;
        item.state = INDEXTOSTATEIMAGEMASK(2);
        item.iItem = 0;
        item.iSubItem = 2;
        r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM)&item);
        ok(r, "Failed to get subitem state.\n");
        ok(item.state == 0, "Unexpected state %#x.\n", item.state);

        item.mask = LVIF_TEXT;
        item.iItem = 0;
        item.iSubItem = 1;
        item.pszText = buff;
        item.cchTextMax = sizeof(buff);
        r = SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM)&item);
        ok(r, "Failed to get subitem text %d.\n", r);
        ok(!strcmp(buff, subtext), "Unexpected subitem text %s.\n", buff);

        DestroyWindow(hwnd);
    }
}

static void test_LVSCW_AUTOSIZE(void)
{
    int width, width2;
    HWND hwnd;
    BOOL ret;

    hwnd = create_listview_control(LVS_REPORT);
    ok(hwnd != NULL, "failed to create a listview window\n");

    insert_column(hwnd, 0);
    insert_column(hwnd, 1);
    insert_item(hwnd, 0);

    ret = SendMessageA(hwnd, LVM_SETCOLUMNWIDTH, 0, LVSCW_AUTOSIZE);
    ok(ret, "Failed to set column width.\n");

    width = SendMessageA(hwnd, LVM_GETCOLUMNWIDTH, 0, 0);
    ok(width > 0, "Unexpected column width %d.\n", width);

    /* Turn on checkboxes. */
    ret = SendMessageA(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);
    ok(ret == 0, "Unexpected previous extended style.\n");

    ret = SendMessageA(hwnd, LVM_SETCOLUMNWIDTH, 0, LVSCW_AUTOSIZE);
    ok(ret, "Failed to set column width.\n");

    width2 = SendMessageA(hwnd, LVM_GETCOLUMNWIDTH, 0, 0);
    ok(width2 > 0, "Unexpected column width %d.\n", width2);
    ok(width2 > width, "Expected increased column width.\n");

    /* Turn off checkboxes. */
    ret = SendMessageA(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_CHECKBOXES, 0);
    ok(ret == LVS_EX_CHECKBOXES, "Unexpected previous extended style.\n");

    ret = SendMessageA(hwnd, LVM_SETCOLUMNWIDTH, 0, LVSCW_AUTOSIZE);
    ok(ret, "Failed to set column width.\n");

    width = SendMessageA(hwnd, LVM_GETCOLUMNWIDTH, 0, 0);
    ok(width > 0, "Unexpected column width %d.\n", width2);
    ok(width2 > width, "Expected reduced column width.\n");

    DestroyWindow(hwnd);
}

static void test_LVN_ENDLABELEDIT(void)
{
    WCHAR text[] = {'l','a','l','a',0};
    HWND hwnd, hwndedit;
    LVITEMW item = {0};
    DWORD ret;

    hwnd = create_listview_control(LVS_REPORT | LVS_EDITLABELS);

    insert_column(hwnd, 0);

    item.mask = LVIF_TEXT;
    item.pszText = text;
    SendMessageW(hwnd, LVM_INSERTITEMW, 0, (LPARAM)&item);

    /* Test normal editing */
    SetFocus(hwnd);
    hwndedit = (HWND)SendMessageW(hwnd, LVM_EDITLABELW, 0, 0);
    ok(hwndedit != NULL, "Failed to get edit control.\n");

    ret = SendMessageA(hwndedit, WM_SETTEXT, 0, (LPARAM)"test");
    ok(ret, "Failed to set edit text.\n");

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    ret = SendMessageA(hwndedit, WM_KEYDOWN, VK_RETURN, 0);
    ok_sequence(sequences, PARENT_SEQ_INDEX, listview_end_label_edit, "Label edit", FALSE);

    /* Test editing with kill focus */
    SetFocus(hwnd);
    hwndedit = (HWND)SendMessageW(hwnd, LVM_EDITLABELW, 0, 0);
    ok(hwndedit != NULL, "Failed to get edit control.\n");

    ret = SendMessageA(hwndedit, WM_SETTEXT, 0, (LPARAM)"test2");
    ok(ret, "Failed to set edit text.\n");

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    g_WM_KILLFOCUS_on_LVN_ENDLABELEDIT = TRUE;
    ret = SendMessageA(hwndedit, WM_KEYDOWN, VK_RETURN, 0);
    g_WM_KILLFOCUS_on_LVN_ENDLABELEDIT = FALSE;

    ok_sequence(sequences, PARENT_SEQ_INDEX, listview_end_label_edit_kill_focus,
            "Label edit, kill focus", FALSE);
    ok(GetFocus() == hwnd, "Unexpected focused window.\n");

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    DestroyWindow(hwnd);
}

static LRESULT CALLBACK create_item_height_wndproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_CREATE)
        return 0;

    return CallWindowProcA(listviewWndProc, hwnd, msg, wParam, lParam);
}

static void test_LVM_GETCOUNTPERPAGE(void)
{
    static const DWORD styles[] = { LVS_ICON, LVS_LIST, LVS_REPORT, LVS_SMALLICON };
    unsigned int i, j;
    WNDCLASSEXA cls;
    ATOM class;
    HWND hwnd;
    BOOL ret;

    cls.cbSize = sizeof(WNDCLASSEXA);
    ret = GetClassInfoExA(GetModuleHandleA(NULL), WC_LISTVIEWA, &cls);
    ok(ret, "Failed to get class info.\n");
    listviewWndProc = cls.lpfnWndProc;
    cls.lpfnWndProc = create_item_height_wndproc;
    cls.lpszClassName = "CountPerPageClass";
    class = RegisterClassExA(&cls);
    ok(class, "Failed to register class.\n");

    for (i = 0; i < ARRAY_SIZE(styles); i++)
    {
        static char text[] = "item text";
        LVITEMA item = { 0 };
        UINT count, count2;

        hwnd = create_listview_control(styles[i]);
        ok(hwnd != NULL, "Failed to create listview window.\n");

        count = SendMessageA(hwnd, LVM_GETCOUNTPERPAGE, 0, 0);
        if (styles[i] == LVS_LIST || styles[i] == LVS_REPORT)
            ok(count > 0 || broken(styles[i] == LVS_LIST && count == 0), "%u: unexpected count %u.\n", i, count);
        else
            ok(count == 0, "%u: unexpected count %u.\n", i, count);

        for (j = 0; j < 10; j++)
        {
            item.mask = LVIF_TEXT;
            item.pszText = text;
            SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM)&item);
        }

        count2 = SendMessageA(hwnd, LVM_GETCOUNTPERPAGE, 0, 0);
        if (styles[i] == LVS_LIST || styles[i] == LVS_REPORT)
            ok(count == count2, "%u: unexpected count %u.\n", i, count2);
        else
            ok(count2 == 10, "%u: unexpected count %u.\n", i, count2);

        DestroyWindow(hwnd);

        hwnd = CreateWindowA("CountPerPageClass", "Test", WS_VISIBLE | styles[i], 0, 0, 100, 100, NULL, NULL,
            GetModuleHandleA(NULL), 0);
        ok(hwnd != NULL, "Failed to create a window.\n");

        count = SendMessageA(hwnd, LVM_GETCOUNTPERPAGE, 0, 0);
        ok(count == 0, "%u: unexpected count %u.\n", i, count);

        DestroyWindow(hwnd);
    }

    ret = UnregisterClassA("CountPerPageClass", NULL);
    ok(ret, "Failed to unregister test class.\n");
}

static void test_item_state_change(void)
{
    static const DWORD styles[] = { LVS_ICON, LVS_LIST, LVS_REPORT, LVS_SMALLICON };
    LVITEMA item;
    HWND hwnd;
    DWORD res;
    int i;

    for (i = 0; i < ARRAY_SIZE(styles); i++)
    {
        hwnd = create_listview_control(styles[i]);

        insert_item(hwnd, 0);

        /* LVM_SETITEMSTATE with mask */
        memset(&g_nmlistview, 0xcc, sizeof(g_nmlistview));
        memset(&item, 0, sizeof(item));
        item.mask = LVIF_STATE;
        item.stateMask = LVIS_SELECTED;
        item.state = LVIS_SELECTED;
        res = SendMessageA(hwnd, LVM_SETITEMSTATE, 0, (LPARAM)&item);
        ok(res, "Failed to set item state.\n");

        ok(g_nmlistview.iItem == item.iItem, "Unexpected item %d.\n", g_nmlistview.iItem);
        ok(g_nmlistview.iSubItem == item.iSubItem, "Unexpected subitem %d.\n", g_nmlistview.iSubItem);
        ok(g_nmlistview.lParam == item.lParam, "Unexpected lParam.\n");
        ok(g_nmlistview.uNewState == LVIS_SELECTED, "got new state 0x%08x\n", g_nmlistview.uNewState);
        ok(g_nmlistview.uOldState == 0, "got old state 0x%08x\n", g_nmlistview.uOldState);
        ok(g_nmlistview.uChanged == LVIF_STATE, "got changed 0x%08x\n", g_nmlistview.uChanged);

        /* LVM_SETITEMSTATE 0 mask */
        memset(&g_nmlistview, 0xcc, sizeof(g_nmlistview));
        memset(&item, 0, sizeof(item));
        item.stateMask = LVIS_SELECTED;
        item.state = 0;
        res = SendMessageA(hwnd, LVM_SETITEMSTATE, 0, (LPARAM)&item);
        ok(res, "Failed to set item state.\n");

        ok(g_nmlistview.iItem == item.iItem, "Unexpected item %d.\n", g_nmlistview.iItem);
        ok(g_nmlistview.iSubItem == item.iSubItem, "Unexpected subitem %d.\n", g_nmlistview.iSubItem);
        ok(g_nmlistview.lParam == item.lParam, "Unexpected lParam.\n");
        ok(g_nmlistview.uNewState == 0, "Unexpected new state %#x.\n", g_nmlistview.uNewState);
        ok(g_nmlistview.uOldState == LVIS_SELECTED, "Unexpected old state %#x.\n", g_nmlistview.uOldState);
        ok(g_nmlistview.uChanged == LVIF_STATE, "Unexpected change mask %#x.\n", g_nmlistview.uChanged);

        /* LVM_SETITEM changes state */
        memset(&g_nmlistview, 0xcc, sizeof(g_nmlistview));
        memset(&item, 0, sizeof(item));
        item.stateMask = LVIS_SELECTED;
        item.state = LVIS_SELECTED;
        item.mask = LVIF_STATE;
        res = SendMessageA(hwnd, LVM_SETITEMA, 0, (LPARAM)&item);
        ok(res, "Failed to set item.\n");

        ok(g_nmlistview.iItem == item.iItem, "Unexpected item %d.\n", g_nmlistview.iItem);
        ok(g_nmlistview.iSubItem == item.iSubItem, "Unexpected subitem %d.\n", g_nmlistview.iSubItem);
        ok(g_nmlistview.lParam == item.lParam, "Unexpected lParam.\n");
        ok(g_nmlistview.uNewState == LVIS_SELECTED, "Unexpected new state %#x.\n", g_nmlistview.uNewState);
        ok(g_nmlistview.uOldState == 0, "Unexpected old state %#x.\n", g_nmlistview.uOldState);
        ok(g_nmlistview.uChanged == LVIF_STATE, "Unexpected change mask %#x.\n", g_nmlistview.uChanged);

        /* LVM_SETITEM no state changes */
        memset(&g_nmlistview, 0xcc, sizeof(g_nmlistview));
        memset(&item, 0, sizeof(item));
        item.lParam = 11;
        item.mask = LVIF_PARAM;
        res = SendMessageA(hwnd, LVM_SETITEMA, 0, (LPARAM)&item);
        ok(res, "Failed to set item.\n");

        ok(g_nmlistview.iItem == item.iItem, "Unexpected item %d.\n", g_nmlistview.iItem);
        ok(g_nmlistview.iSubItem == item.iSubItem, "Unexpected subitem %d.\n", g_nmlistview.iSubItem);
        ok(g_nmlistview.lParam == item.lParam, "Unexpected lParam.\n");
        ok(g_nmlistview.uNewState == 0, "Unexpected new state %#x.\n", g_nmlistview.uNewState);
        ok(g_nmlistview.uOldState == 0, "Unexpected old state %#x.\n", g_nmlistview.uOldState);
        ok(g_nmlistview.uChanged == LVIF_PARAM, "Unexpected change mask %#x.\n", g_nmlistview.uChanged);

        DestroyWindow(hwnd);
    }
}

START_TEST(listview)
{
    ULONG_PTR ctx_cookie;
    HANDLE hCtx;

    init_functions();

    init_msg_sequences(sequences, NUM_MSG_SEQUENCES);

    hwndparent = create_parent_window(FALSE);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    g_is_below_5 = is_below_comctl_5();

    test_header_notification();
    test_header_notification2();
    test_images();
    test_checkboxes();
    test_items();
    test_create(FALSE);
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
    test_LVM_REDRAWITEMS();
    test_imagelists();
    test_deleteitem();
    test_insertitem();
    test_header_proc();
    test_oneclickactivate();
    test_callback_mask();
    test_state_image();
    test_LVSCW_AUTOSIZE();
    test_LVN_ENDLABELEDIT();
    test_LVM_GETCOUNTPERPAGE();
    test_item_state_change();

    if (!load_v6_module(&ctx_cookie, &hCtx))
    {
        DestroyWindow(hwndparent);
        return;
    }

    init_functions();

    /* comctl32 version 6 tests start here */
    test_get_set_view();
    test_canceleditlabel();
    test_mapidindex();
    test_scrollnotify();
    test_LVS_EX_TRANSPARENTBKGND();
    test_LVS_EX_HEADERINALLVIEWS();
    test_deleteitem();
    test_multiselect();
    test_insertitem();
    test_header_proc();
    test_images();
    test_checkboxes();
    test_items();
    test_create(TRUE);
    test_color();
    test_columns();
    test_sorting();
    test_ownerdata();
    test_norecompute();
    test_nosortheader();
    test_indentation();
    test_finditem();
    test_hover();
    test_destroynotify();
    test_createdragimage();
    test_dispinfo();
    test_LVM_SETITEMTEXT();
    test_LVM_REDRAWITEMS();
    test_oneclickactivate();
    test_state_image();
    test_LVSCW_AUTOSIZE();
    test_LVN_ENDLABELEDIT();
    test_LVM_GETCOUNTPERPAGE();
    test_item_state_change();

    unload_v6_module(ctx_cookie, hCtx);

    DestroyWindow(hwndparent);
}
