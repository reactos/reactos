/* Unit tests for treeview.
 *
 * Copyright 2005 Krzysztof Foltman
 * Copyright 2007 Christopher James Peterson
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
#include "winreg.h"
#include "wine/commctrl.h" 

#include "wine/test.h"
#include "v6util.h"
#include "msg.h"

static BOOL (WINAPI *pInitCommonControlsEx)(const INITCOMMONCONTROLSEX*);
static const char *TEST_CALLBACK_TEXT = "callback_text";

static TVITEMA g_item_expanding, g_item_expanded;
static BOOL g_get_from_expand;
static BOOL g_get_rect_in_expand;
static BOOL g_disp_A_to_W;
static BOOL g_disp_set_stateimage;
static BOOL g_beginedit_alter_text;
static const char *g_endedit_overwrite_contents;
static char *g_endedit_overwrite_ptr;
static HFONT g_customdraw_font;
static BOOL g_v6;

#define NUM_MSG_SEQUENCES   3
#define TREEVIEW_SEQ_INDEX  0
#define PARENT_SEQ_INDEX    1
#define PARENT_CD_SEQ_INDEX 2

#define expect(expected, got) ok(got == expected, "Expected %d, got %d\n", expected, got)

static struct msg_sequence *sequences[NUM_MSG_SEQUENCES];
static struct msg_sequence *item_sequence[1];

static void flush_events(void)
{
    MSG msg;
    int diff = 200;
    int min_timeout = 100;
    DWORD time = GetTickCount() + diff;

    while (diff > 0)
    {
        if (MsgWaitForMultipleObjects(0, NULL, FALSE, min_timeout, QS_ALLINPUT) == WAIT_TIMEOUT) break;
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
        diff = time - GetTickCount();
    }
}

static const struct message FillRootSeq[] = {
    { TVM_INSERTITEMA, sent },
    { TVM_INSERTITEMA, sent },
    { 0 }
};

static const struct message rootnone_select_seq[] = {
    { TVM_SELECTITEM, sent|wparam, 9 },
    { TVM_SELECTITEM, sent|wparam, 9 },
    { TVM_SELECTITEM, sent|wparam, 9 },
    { TVM_SELECTITEM, sent|wparam, 9 },
    { TVM_SELECTITEM, sent|wparam, 9 },
    { TVM_SELECTITEM, sent|wparam, 9 },
    { 0 }
};

static const struct message rootchild_select_seq[] = {
    { TVM_SELECTITEM, sent|wparam, 9 },
    { TVM_SELECTITEM, sent|wparam, 9 },
    { TVM_SELECTITEM, sent|wparam, 9 },
    { TVM_SELECTITEM, sent|wparam, 9 },
    { TVM_SELECTITEM, sent|wparam, 9 },
    { TVM_SELECTITEM, sent|wparam, 9 },
    { 0 }
};

static const struct message getitemtext_seq[] = {
    { TVM_INSERTITEMA, sent },
    { TVM_GETITEMA, sent },
    { TVM_DELETEITEM, sent },
    { 0 }
};

static const struct message focus_seq[] = {
    { TVM_INSERTITEMA, sent },
    { TVM_INSERTITEMA, sent },
    { TVM_SELECTITEM, sent|wparam, 9 },
    /* The following end up out of order in wine */
    { WM_WINDOWPOSCHANGING, sent|defwinproc },
    { WM_NCCALCSIZE, sent|wparam|defwinproc, TRUE },
    { WM_WINDOWPOSCHANGED, sent|defwinproc },
    { WM_SIZE, sent|defwinproc },
    { WM_WINDOWPOSCHANGING, sent|defwinproc|optional },
    { WM_NCCALCSIZE, sent|wparam|defwinproc|optional, TRUE },
    { WM_WINDOWPOSCHANGED, sent|defwinproc|optional },
    { WM_SIZE, sent|defwinproc|optional },
    { WM_PAINT, sent|defwinproc },
    { WM_NCPAINT, sent|wparam|defwinproc, 1 },
    { WM_ERASEBKGND, sent|defwinproc },
    { TVM_EDITLABELA, sent },
    { WM_COMMAND, sent|wparam|defwinproc, MAKEWPARAM(0, EN_UPDATE) },
    { WM_COMMAND, sent|wparam|defwinproc, MAKEWPARAM(0, EN_CHANGE) },
    { WM_PARENTNOTIFY, sent|wparam|defwinproc, MAKEWPARAM(WM_CREATE, 0) },
    { WM_KILLFOCUS, sent|defwinproc },
    { WM_PAINT, sent|defwinproc },
    { WM_IME_SETCONTEXT, sent|defwinproc|optional },
    { WM_COMMAND, sent|wparam|defwinproc, MAKEWPARAM(0, EN_SETFOCUS) },
    { WM_ERASEBKGND, sent|defwinproc|optional },
    { WM_CTLCOLOREDIT, sent|defwinproc|optional },
    { WM_CTLCOLOREDIT, sent|defwinproc|optional },
    { 0 }
};

static const struct message test_get_set_bkcolor_seq[] = {
    { TVM_GETBKCOLOR, sent|wparam|lparam, 0, 0 },
    { TVM_SETBKCOLOR, sent|wparam|lparam, 0, 0 },
    { TVM_GETBKCOLOR, sent|wparam|lparam, 0, 0 },
    { TVM_SETBKCOLOR, sent|wparam|lparam, 0, 0x00ffffff },
    { TVM_GETBKCOLOR, sent|wparam|lparam, 0, 0 },
    { TVM_SETBKCOLOR, sent|wparam|lparam, 0, -1 },
    { 0 }
};

static const struct message test_get_set_imagelist_seq[] = {
    { TVM_SETIMAGELIST, sent|wparam|lparam, 0, 0 },
    { TVM_GETIMAGELIST, sent|wparam|lparam, 0, 0 },
    { 0 }
};

static const struct message test_get_set_indent_seq[] = {
    { TVM_SETINDENT, sent|wparam|lparam, 0, 0 },
    { TVM_GETINDENT, sent|wparam|lparam, 0, 0 },
    /* The actual amount to indent is dependent on the system for this message */
    { TVM_SETINDENT, sent },
    { TVM_GETINDENT, sent|wparam|lparam, 0, 0 },
    { 0 }
};

static const struct message test_get_set_insertmarkcolor_seq[] = {
    { TVM_SETINSERTMARKCOLOR, sent|wparam|lparam, 0, 0 },
    { TVM_GETINSERTMARKCOLOR, sent|wparam|lparam, 0, 0 },
    { 0 }
};

static const struct message test_get_set_item_seq[] = {
    { TVM_GETITEMA, sent },
    { TVM_SETITEMA, sent },
    { TVM_GETITEMA, sent },
    { TVM_SETITEMA, sent },
    { 0 }
};

static const struct message test_get_set_itemheight_seq[] = {
    { TVM_GETITEMHEIGHT, sent|wparam|lparam, 0, 0 },
    { TVM_SETITEMHEIGHT, sent|wparam|lparam, -1, 0 },
    { TVM_GETITEMHEIGHT, sent|wparam|lparam, 0, 0 },
    { TVM_SETITEMHEIGHT, sent|lparam, 0xcccccccc, 0 },
    { TVM_GETITEMHEIGHT, sent|wparam|lparam|optional, 0, 0 },
    { TVM_SETITEMHEIGHT, sent|wparam|lparam|optional, 9, 0 },
    { TVM_GETITEMHEIGHT, sent|wparam|lparam, 0, 0 },
    { 0 }
};

static const struct message test_get_set_scrolltime_seq[] = {
    { TVM_SETSCROLLTIME, sent|wparam|lparam, 20, 0 },
    { TVM_GETSCROLLTIME, sent|wparam|lparam, 0, 0 },
    { 0 }
};

static const struct message test_get_set_textcolor_seq[] = {
    { TVM_GETTEXTCOLOR, sent|wparam|lparam, 0, 0 },
    { TVM_SETTEXTCOLOR, sent|wparam|lparam, 0, 0 },
    { TVM_GETTEXTCOLOR, sent|wparam|lparam, 0, 0 },
    { TVM_SETTEXTCOLOR, sent|wparam|lparam, 0, RGB(255, 255, 255) },
    { TVM_GETTEXTCOLOR, sent|wparam|lparam, 0, 0 },
    { TVM_SETTEXTCOLOR, sent|wparam|lparam, 0, CLR_NONE },
    { 0 }
};

static const struct message test_get_set_tooltips_seq[] = {
    { WM_KILLFOCUS,    sent },
    { WM_IME_SETCONTEXT, sent|optional },
    { WM_IME_NOTIFY, sent|optional },
    { TVM_SETTOOLTIPS, sent|wparam|lparam, 0, 0 },
    { TVM_GETTOOLTIPS, sent|wparam|lparam, 0, 0 },
    { 0 }
};

static const struct message test_get_set_unicodeformat_seq[] = {
    { TVM_SETUNICODEFORMAT, sent|wparam|lparam, TRUE, 0 },
    { TVM_GETUNICODEFORMAT, sent|wparam|lparam, 0, 0 },
    { TVM_SETUNICODEFORMAT, sent|wparam|lparam, 0, 0 },
    { TVM_GETUNICODEFORMAT, sent|wparam|lparam, 0, 0 },
    { TVM_SETUNICODEFORMAT, sent|wparam|lparam, 0, 0 },
    { 0 }
};

static const struct message test_right_click_seq[] = {
    { WM_RBUTTONDOWN, sent|wparam, MK_RBUTTON },
    { WM_CAPTURECHANGED, sent|defwinproc },
    { TVM_GETNEXTITEM, sent|wparam|lparam|defwinproc, TVGN_CARET, 0 },
    { WM_NCHITTEST, sent|optional },
    { WM_SETCURSOR, sent|optional },
    { WM_MOUSEMOVE, sent|optional },
    { 0 }
};

static const struct message parent_expand_seq[] = {
    { WM_NOTIFY, sent|id, 0, 0, TVN_ITEMEXPANDINGA },
    { WM_NOTIFY, sent|id, 0, 0, TVN_ITEMEXPANDEDA },
    { 0 }
};

static const struct message parent_expand_kb_seq[] = {
    { WM_NOTIFY, sent|id, 0, 0, TVN_KEYDOWN },
    { WM_NOTIFY, sent|id, 0, 0, TVN_ITEMEXPANDINGA },
    { WM_NOTIFY, sent|id, 0, 0, TVN_ITEMEXPANDEDA },
    { WM_CHANGEUISTATE, sent|optional },
    { 0 }
};

static const struct message parent_collapse_2nd_kb_seq[] = {
    { WM_NOTIFY, sent|id|optional, 0, 0, TVN_KEYDOWN },
    { WM_NOTIFY, sent|id, 0, 0, TVN_ITEMEXPANDINGA },
    { WM_CHANGEUISTATE, sent|optional },
    { 0 }
};

static const struct message parent_expand_empty_kb_seq[] = {
    { WM_NOTIFY, sent|id, 0, 0, TVN_KEYDOWN },
    { WM_CHANGEUISTATE, sent|optional },
    { 0 }
};

static const struct message parent_singleexpand_seq0[] = {
    /* alpha expands */
    { WM_NOTIFY, sent|id, 0, 0, TVN_SELCHANGINGA },
    { WM_NOTIFY, sent|id, 0, 0, TVN_SELCHANGEDA },
    { WM_NOTIFY, sent|id, 0, 0, TVN_SINGLEEXPAND },
    { WM_NOTIFY, sent|id, 0, 0, TVN_ITEMEXPANDINGA },
    { WM_NOTIFY, sent|id, 0, 0, TVN_ITEMEXPANDEDA },
    { 0 }
};

static const struct message parent_singleexpand_seq1[] = {
    /* bravo expands */
    { WM_NOTIFY, sent|id, 0, 0, TVN_SELCHANGINGA },
    { WM_NOTIFY, sent|id, 0, 0, TVN_SELCHANGEDA },
    { WM_NOTIFY, sent|id, 0, 0, TVN_SINGLEEXPAND },
    { WM_NOTIFY, sent|id, 0, 0, TVN_ITEMEXPANDINGA },
    { WM_NOTIFY, sent|id, 0, 0, TVN_ITEMEXPANDEDA },
    { 0 }
};

static const struct message parent_singleexpand_seq2[] = {
    /* delta expands, bravo collapses */
    { WM_NOTIFY, sent|id, 0, 0, TVN_SELCHANGINGA },
    { WM_NOTIFY, sent|id, 0, 0, TVN_SELCHANGEDA },
    { WM_NOTIFY, sent|id, 0, 0, TVN_SINGLEEXPAND },
    { WM_NOTIFY, sent|id, 0, 0, TVN_ITEMEXPANDINGA },
    { WM_NOTIFY, sent|id, 0, 0, TVN_ITEMEXPANDEDA },
    { WM_NOTIFY, sent|id, 0, 0, TVN_ITEMEXPANDINGA },
    { WM_NOTIFY, sent|id, 0, 0, TVN_ITEMEXPANDEDA },
    { 0 }
};

static const struct message parent_singleexpand_seq3[] = {
    /* foxtrot expands, alpha and delta collapse */
    { WM_NOTIFY, sent|id, 0, 0, TVN_SELCHANGINGA },
    { WM_NOTIFY, sent|id, 0, 0, TVN_SELCHANGEDA },
    { WM_NOTIFY, sent|id, 0, 0, TVN_SINGLEEXPAND },
    { WM_NOTIFY, sent|id, 0, 0, TVN_ITEMEXPANDINGA },
    { WM_NOTIFY, sent|id, 0, 0, TVN_ITEMEXPANDEDA },
    { WM_NOTIFY, sent|id, 0, 0, TVN_ITEMEXPANDINGA },
    { WM_NOTIFY, sent|id, 0, 0, TVN_ITEMEXPANDEDA },
    { WM_NOTIFY, sent|id, 0, 0, TVN_ITEMEXPANDINGA },
    { WM_NOTIFY, sent|id, 0, 0, TVN_ITEMEXPANDEDA },
    { 0 }
};

static const struct message parent_singleexpand_seq4[] = {
    /* alpha expands, foxtrot collapses */
    { WM_NOTIFY, sent|id, 0, 0, TVN_SELCHANGINGA },
    { WM_NOTIFY, sent|id, 0, 0, TVN_SELCHANGEDA },
    { WM_NOTIFY, sent|id, 0, 0, TVN_SINGLEEXPAND },
    { WM_NOTIFY, sent|id, 0, 0, TVN_ITEMEXPANDINGA },
    { WM_NOTIFY, sent|id, 0, 0, TVN_ITEMEXPANDEDA },
    { WM_NOTIFY, sent|id, 0, 0, TVN_ITEMEXPANDINGA },
    { WM_NOTIFY, sent|id, 0, 0, TVN_ITEMEXPANDEDA },
    { 0 }
};

static const struct message parent_singleexpand_seq5[] = {
    /* foxtrot expands while golf is selected, then golf expands and alpha collapses */
    { WM_NOTIFY, sent|id, 0, 0, TVN_SELCHANGINGA },
    { WM_NOTIFY, sent|id, 0, 0, TVN_ITEMEXPANDINGA },
    { WM_NOTIFY, sent|id, 0, 0, TVN_ITEMEXPANDEDA },
    { WM_NOTIFY, sent|id, 0, 0, TVN_SELCHANGEDA },
    { WM_NOTIFY, sent|id, 0, 0, TVN_SINGLEEXPAND },
    { WM_NOTIFY, sent|id, 0, 0, TVN_ITEMEXPANDINGA },
    { WM_NOTIFY, sent|id, 0, 0, TVN_ITEMEXPANDEDA },
    { WM_NOTIFY, sent|id, 0, 0, TVN_ITEMEXPANDINGA },
    { WM_NOTIFY, sent|id, 0, 0, TVN_ITEMEXPANDEDA },
    { 0 }
};

static const struct message parent_singleexpand_seq6[] = {
    /* hotel does not expand and india does not collapse because they have no children */
    { WM_NOTIFY, sent|id, 0, 0, TVN_SELCHANGINGA },
    { WM_NOTIFY, sent|id, 0, 0, TVN_SELCHANGEDA },
    { WM_NOTIFY, sent|id, 0, 0, TVN_SINGLEEXPAND },
    { 0 }
};

static const struct message parent_singleexpand_seq7[] = {
    /* india does not expand and hotel does not collapse because they have no children */
    { WM_NOTIFY, sent|id, 0, 0, TVN_SELCHANGINGA },
    { WM_NOTIFY, sent|id, 0, 0, TVN_SELCHANGEDA },
    { WM_NOTIFY, sent|id, 0, 0, TVN_SINGLEEXPAND },
    { 0 }
};

static const struct message parent_get_dispinfo_seq[] = {
    { WM_NOTIFY, sent|id, 0, 0, TVN_GETDISPINFOA },
    { 0 }
};

static const struct message empty_seq[] = {
    { 0 }
};

static const struct message parent_cd_seq[] = {
    { WM_NOTIFY, sent|id|custdraw, 0, 0, NM_CUSTOMDRAW, CDDS_PREPAINT },
    { WM_NOTIFY, sent|id|custdraw, 0, 0, NM_CUSTOMDRAW, CDDS_ITEMPREPAINT },
    { WM_NOTIFY, sent|id|custdraw, 0, 0, NM_CUSTOMDRAW, CDDS_ITEMPOSTPAINT },
    { WM_NOTIFY, sent|id|custdraw, 0, 0, NM_CUSTOMDRAW, CDDS_ITEMPREPAINT },
    { WM_NOTIFY, sent|id|custdraw, 0, 0, NM_CUSTOMDRAW, CDDS_ITEMPOSTPAINT },
    { WM_NOTIFY, sent|id|custdraw, 0, 0, NM_CUSTOMDRAW, CDDS_POSTPAINT },
    { 0 }
};

static const struct message parent_vk_return_seq[] = {
    { WM_NOTIFY, sent|id, 0, 0, TVN_KEYDOWN },
    { WM_NOTIFY, sent|id, 0, 0, NM_RETURN },
    { WM_CHANGEUISTATE, sent|optional },
    { 0 }
};

static const struct message parent_right_click_seq[] = {
    { WM_NOTIFY, sent|id, 0, 0, NM_RCLICK },
    { WM_CONTEXTMENU, sent },
    { WM_NOTIFY, sent|optional },
    { WM_SETCURSOR, sent|optional },
    { 0 }
};

static HWND hMainWnd;

static HTREEITEM hRoot, hChild;

static int pos = 0;
static char sequence[256];

static void Clear(void)
{
    pos = 0;
    sequence[0] = '\0';
}

static void AddItem(char ch)
{
    sequence[pos++] = ch;
    sequence[pos] = '\0';
}

static void IdentifyItem(HTREEITEM hItem)
{
    if (hItem == hRoot) {
        AddItem('R');
        return;
    }
    if (hItem == hChild) {
        AddItem('C');
        return;
    }
    if (hItem == NULL) {
        AddItem('n');
        return;
    }
    AddItem('?');
}

/* This function hooks in and records all messages to the treeview control */
static LRESULT WINAPI TreeviewWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static LONG defwndproc_counter = 0;
    LRESULT ret;
    WNDPROC lpOldProc = (WNDPROC)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    struct message msg = { 0 };

    msg.message = message;
    msg.flags = sent|wparam|lparam;
    if (defwndproc_counter) msg.flags |= defwinproc;
    msg.wParam = wParam;
    msg.lParam = lParam;
    add_message(sequences, TREEVIEW_SEQ_INDEX, &msg);

    defwndproc_counter++;
    ret = CallWindowProcA(lpOldProc, hwnd, message, wParam, lParam);
    defwndproc_counter--;

    return ret;
}

static HWND create_treeview_control(DWORD style)
{
    WNDPROC pOldWndProc;
    HWND hTree;

    hTree = CreateWindowExA(WS_EX_CLIENTEDGE, WC_TREEVIEWA, NULL, WS_CHILD|WS_VISIBLE|
            TVS_LINESATROOT|TVS_HASLINES|TVS_HASBUTTONS|TVS_EDITLABELS|style,
            0, 0, 120, 100, hMainWnd, (HMENU)100, GetModuleHandleA(0), 0);

    SetFocus(hTree);

    /* Record the old WNDPROC so we can call it after recording the messages */
    pOldWndProc = (WNDPROC)SetWindowLongPtrA(hTree, GWLP_WNDPROC, (LONG_PTR)TreeviewWndProc);
    SetWindowLongPtrA(hTree, GWLP_USERDATA, (LONG_PTR)pOldWndProc);

    return hTree;
}

static void fill_tree(HWND hTree)
{
    TVINSERTSTRUCTA ins;
    static CHAR root[]  = "Root",
                child[] = "Child";

    ins.hParent = TVI_ROOT;
    ins.hInsertAfter = TVI_ROOT;
    U(ins).item.mask = TVIF_TEXT;
    U(ins).item.pszText = root;
    hRoot = TreeView_InsertItemA(hTree, &ins);

    ins.hParent = hRoot;
    ins.hInsertAfter = TVI_FIRST;
    U(ins).item.mask = TVIF_TEXT;
    U(ins).item.pszText = child;
    hChild = TreeView_InsertItemA(hTree, &ins);
}

static void test_fillroot(void)
{
    TVITEMA tvi;
    HWND hTree;

    hTree = create_treeview_control(0);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    fill_tree(hTree);

    Clear();
    AddItem('A');
    ok(hRoot != NULL, "failed to set root\n");
    AddItem('B');
    ok(hChild != NULL, "failed to set child\n");
    AddItem('.');
    ok_sequence(sequences, TREEVIEW_SEQ_INDEX, FillRootSeq, "FillRoot", FALSE);
    ok(!strcmp(sequence, "AB."), "Item creation\n");

    /* UMLPad 1.15 depends on this being not -1 (I_IMAGECALLBACK) */
    tvi.hItem = hRoot;
    tvi.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    SendMessageA(hTree, TVM_GETITEMA, 0, (LPARAM)&tvi);
    ok(tvi.iImage == 0, "tvi.iImage=%d\n", tvi.iImage);
    ok(tvi.iSelectedImage == 0, "tvi.iSelectedImage=%d\n", tvi.iSelectedImage);

    DestroyWindow(hTree);
}

static void test_callback(void)
{
    HTREEITEM hRoot;
    HTREEITEM hItem1, hItem2;
    TVINSERTSTRUCTA ins;
    TVITEMA tvi;
    CHAR test_string[] = "Test_string";
    static const CHAR test2A[] = "TEST2";
    CHAR buf[128];
    HWND hTree;
    DWORD ret;

    hTree = create_treeview_control(0);

    ret = SendMessageA(hTree, TVM_DELETEITEM, 0, (LPARAM)TVI_ROOT);
    expect(TRUE, ret);
    ins.hParent = TVI_ROOT;
    ins.hInsertAfter = TVI_ROOT;
    U(ins).item.mask = TVIF_TEXT;
    U(ins).item.pszText = LPSTR_TEXTCALLBACKA;
    hRoot = TreeView_InsertItemA(hTree, &ins);
    ok(hRoot != NULL, "failed to set root\n");

    tvi.hItem = hRoot;
    tvi.mask = TVIF_TEXT;
    tvi.pszText = buf;
    tvi.cchTextMax = ARRAY_SIZE(buf);
    ret = TreeView_GetItemA(hTree, &tvi);
    expect(TRUE, ret);
    ok(strcmp(tvi.pszText, TEST_CALLBACK_TEXT) == 0, "Callback item text mismatch %s vs %s\n",
        tvi.pszText, TEST_CALLBACK_TEXT);

    ins.hParent = hRoot;
    ins.hInsertAfter = TVI_FIRST;
    U(ins).item.mask = TVIF_TEXT;
    U(ins).item.pszText = test_string;
    hItem1 = TreeView_InsertItemA(hTree, &ins);
    ok(hItem1 != NULL, "failed to set Item1\n");

    tvi.hItem = hItem1;
    ret = TreeView_GetItemA(hTree, &tvi);
    expect(TRUE, ret);
    ok(strcmp(tvi.pszText, test_string) == 0, "Item text mismatch %s vs %s\n",
        tvi.pszText, test_string);

    /* undocumented: pszText of NULL also means LPSTR_CALLBACK: */
    tvi.pszText = NULL;
    ret = TreeView_SetItemA(hTree, &tvi);
    expect(TRUE, ret);
    tvi.pszText = buf;
    ret = TreeView_GetItemA(hTree, &tvi);
    expect(TRUE, ret);
    ok(strcmp(tvi.pszText, TEST_CALLBACK_TEXT) == 0, "Item text mismatch %s vs %s\n",
        tvi.pszText, TEST_CALLBACK_TEXT);

    U(ins).item.pszText = NULL;
    hItem2 = TreeView_InsertItemA(hTree, &ins);
    ok(hItem2 != NULL, "failed to set Item2\n");
    tvi.hItem = hItem2;
    memset(buf, 0, sizeof(buf));
    ret = TreeView_GetItemA(hTree, &tvi);
    expect(TRUE, ret);
    ok(strcmp(tvi.pszText, TEST_CALLBACK_TEXT) == 0, "Item text mismatch %s vs %s\n",
        tvi.pszText, TEST_CALLBACK_TEXT);

    /* notification handler changed A->W */
    g_disp_A_to_W = TRUE;
    tvi.hItem = hItem2;
    memset(buf, 0, sizeof(buf));
    ret = TreeView_GetItemA(hTree, &tvi);
    expect(TRUE, ret);
    ok(strcmp(tvi.pszText, test2A) == 0, "got %s, expected %s\n",
        tvi.pszText, test2A);
    g_disp_A_to_W = FALSE;

    /* handler changes state image index */
    SetWindowLongA(hTree, GWL_STYLE, GetWindowLongA(hTree, GWL_STYLE) | TVS_CHECKBOXES);

    /* clear selection, handler will set selected state */
    ret = SendMessageA(hTree, TVM_SELECTITEM, TVGN_CARET, 0);
    expect(TRUE, ret);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    tvi.hItem = hRoot;
    tvi.mask = TVIF_STATE;
    tvi.state = TVIS_SELECTED;
    ret = TreeView_GetItemA(hTree, &tvi);
    expect(TRUE, ret);
    ok(tvi.state == INDEXTOSTATEIMAGEMASK(1), "got 0x%x\n", tvi.state);

    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq,
                "no TVN_GETDISPINFO for a state seq", FALSE);

    tvi.hItem     = hRoot;
    tvi.mask      = TVIF_IMAGE | TVIF_STATE;
    tvi.state     = TVIS_FOCUSED;
    tvi.stateMask = TVIS_FOCUSED;
    tvi.iImage    = I_IMAGECALLBACK;
    ret = TreeView_SetItemA(hTree, &tvi);
    expect(TRUE, ret);

    /* ask for item image index through callback - state is also set with state image index */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    tvi.hItem = hRoot;
    tvi.mask = TVIF_IMAGE;
    tvi.state = 0;
    ret = TreeView_GetItemA(hTree, &tvi);
    expect(TRUE, ret);
    ok(tvi.state == (INDEXTOSTATEIMAGEMASK(1) | TVIS_FOCUSED), "got 0x%x\n", tvi.state);

    ok_sequence(sequences, PARENT_SEQ_INDEX, parent_get_dispinfo_seq,
                "callback for state/overlay image index, noop seq", FALSE);

    /* ask for image again and overwrite state to some value in handler */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    g_disp_set_stateimage = TRUE;
    tvi.hItem = hRoot;
    tvi.mask = TVIF_IMAGE;
    tvi.state = INDEXTOSTATEIMAGEMASK(1);
    tvi.stateMask = 0;
    ret = TreeView_GetItemA(hTree, &tvi);
    expect(TRUE, ret);
    /* handler sets TVIS_SELECTED as well */
    ok(tvi.state == (TVIS_FOCUSED | TVIS_SELECTED | INDEXTOSTATEIMAGEMASK(2) | INDEXTOOVERLAYMASK(3)), "got 0x%x\n", tvi.state);
    g_disp_set_stateimage = FALSE;

    ok_sequence(sequences, PARENT_SEQ_INDEX, parent_get_dispinfo_seq,
                "callback for state/overlay image index seq", FALSE);

    DestroyWindow(hTree);
}

static void test_select(void)
{
    BOOL r;
    HWND hTree;

    hTree = create_treeview_control(0);
    fill_tree(hTree);

    /* root-none select tests */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    r = SendMessageA(hTree, TVM_SELECTITEM, TVGN_CARET, 0);
    expect(TRUE, r);
    Clear();
    AddItem('1');
    r = SendMessageA(hTree, TVM_SELECTITEM, TVGN_CARET, (LPARAM)hRoot);
    expect(TRUE, r);
    AddItem('2');
    r = SendMessageA(hTree, TVM_SELECTITEM, TVGN_CARET, (LPARAM)hRoot);
    expect(TRUE, r);
    AddItem('3');
    r = SendMessageA(hTree, TVM_SELECTITEM, TVGN_CARET, 0);
    expect(TRUE, r);
    AddItem('4');
    r = SendMessageA(hTree, TVM_SELECTITEM, TVGN_CARET, 0);
    expect(TRUE, r);
    AddItem('5');
    r = SendMessageA(hTree, TVM_SELECTITEM, TVGN_CARET, (LPARAM)hRoot);
    expect(TRUE, r);
    AddItem('.');
    ok(!strcmp(sequence, "1(nR)nR23(Rn)Rn45(nR)nR."), "root-none select test\n");
    ok_sequence(sequences, TREEVIEW_SEQ_INDEX, rootnone_select_seq,
                "root-none select seq", FALSE);

    /* root-child select tests */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    r = SendMessageA(hTree, TVM_SELECTITEM, TVGN_CARET, 0);
    expect(TRUE, r);

    Clear();
    AddItem('1');
    r = SendMessageA(hTree, TVM_SELECTITEM, TVGN_CARET, (LPARAM)hRoot);
    expect(TRUE, r);
    AddItem('2');
    r = SendMessageA(hTree, TVM_SELECTITEM, TVGN_CARET, (LPARAM)hRoot);
    expect(TRUE, r);
    AddItem('3');
    r = SendMessageA(hTree, TVM_SELECTITEM, TVGN_CARET, (LPARAM)hChild);
    expect(TRUE, r);
    AddItem('4');
    r = SendMessageA(hTree, TVM_SELECTITEM, TVGN_CARET, (LPARAM)hChild);
    expect(TRUE, r);
    AddItem('5');
    r = SendMessageA(hTree, TVM_SELECTITEM, TVGN_CARET, (LPARAM)hRoot);
    expect(TRUE, r);
    AddItem('.');
    ok(!strcmp(sequence, "1(nR)nR23(RC)RC45(CR)CR."), "root-child select test\n");
    ok_sequence(sequences, TREEVIEW_SEQ_INDEX, rootchild_select_seq,
                "root-child select seq", FALSE);

    DestroyWindow(hTree);
}

static void test_getitemtext(void)
{
    TVINSERTSTRUCTA ins;
    HTREEITEM hChild;
    TVITEMA tvi;
    HWND hTree;

    CHAR szBuffer[80] = "Blah";
    int nBufferSize = ARRAY_SIZE(szBuffer);

    hTree = create_treeview_control(0);
    fill_tree(hTree);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    /* add an item without TVIF_TEXT mask and pszText == NULL */
    ins.hParent = hRoot;
    ins.hInsertAfter = TVI_ROOT;
    U(ins).item.mask = 0;
    U(ins).item.pszText = NULL;
    U(ins).item.cchTextMax = 0;
    hChild = TreeView_InsertItemA(hTree, &ins);
    ok(hChild != NULL, "failed to set hChild\n");

    /* retrieve it with TVIF_TEXT mask */
    tvi.hItem = hChild;
    tvi.mask = TVIF_TEXT;
    tvi.cchTextMax = nBufferSize;
    tvi.pszText = szBuffer;

    SendMessageA( hTree, TVM_GETITEMA, 0, (LPARAM)&tvi );
    ok(!strcmp(szBuffer, ""), "szBuffer=\"%s\", expected \"\"\n", szBuffer);
    ok(SendMessageA(hTree, TVM_DELETEITEM, 0, (LPARAM)hChild), "DeleteItem failed\n");
    ok_sequence(sequences, TREEVIEW_SEQ_INDEX, getitemtext_seq, "get item text seq", FALSE);

    DestroyWindow(hTree);
}

static void test_focus(void)
{
    TVINSERTSTRUCTA ins;
    static CHAR child1[]  = "Edit",
                child2[]  = "A really long string";
    HTREEITEM hChild1, hChild2;
    HWND hTree;
    HWND hEdit;

    hTree = create_treeview_control(0);
    fill_tree(hTree);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    /* This test verifies that when a label is being edited, scrolling
     * the treeview does not cause the label to lose focus. To test
     * this, first some additional entries are added to generate
     * scrollbars.
     */
    ins.hParent = hRoot;
    ins.hInsertAfter = hChild;
    U(ins).item.mask = TVIF_TEXT;
    U(ins).item.pszText = child1;
    hChild1 = TreeView_InsertItemA(hTree, &ins);
    ok(hChild1 != NULL, "failed to set hChild1\n");
    ins.hInsertAfter = hChild1;
    U(ins).item.mask = TVIF_TEXT;
    U(ins).item.pszText = child2;
    hChild2 = TreeView_InsertItemA(hTree, &ins);
    ok(hChild2 != NULL, "failed to set hChild2\n");

    ShowWindow(hMainWnd,SW_SHOW);
    SendMessageA(hTree, TVM_SELECTITEM, TVGN_CARET, (LPARAM)hChild);
    hEdit = (HWND)SendMessageA(hTree, TVM_EDITLABELA, 0, (LPARAM)hChild);
    ScrollWindowEx(hTree, -10, 0, NULL, NULL, NULL, NULL, SW_SCROLLCHILDREN);
    ok(GetFocus() == hEdit, "Edit control should have focus\n");
    ok_sequence(sequences, TREEVIEW_SEQ_INDEX, focus_seq, "focus test", TRUE);

    DestroyWindow(hTree);
}

static void test_get_set_bkcolor(void)
{
    COLORREF crColor;
    HWND hTree;

    hTree = create_treeview_control(0);
    fill_tree(hTree);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    /* If the value is -1, the control is using the system color for the background color. */
    crColor = SendMessageA(hTree, TVM_GETBKCOLOR, 0, 0);
    ok(crColor == ~0u, "Default background color reported as 0x%.8x\n", crColor);

    /* Test for black background */
    SendMessageA(hTree, TVM_SETBKCOLOR, 0, RGB(0,0,0));
    crColor = SendMessageA(hTree, TVM_GETBKCOLOR, 0, 0);
    ok(crColor == RGB(0,0,0), "Black background color reported as 0x%.8x\n", crColor);

    /* Test for white background */
    SendMessageA(hTree, TVM_SETBKCOLOR, 0, RGB(255,255,255));
    crColor = SendMessageA(hTree, TVM_GETBKCOLOR, 0, 0);
    ok(crColor == RGB(255,255,255), "White background color reported as 0x%.8x\n", crColor);

    /* Reset the default background */
    SendMessageA(hTree, TVM_SETBKCOLOR, 0, -1);

    ok_sequence(sequences, TREEVIEW_SEQ_INDEX, test_get_set_bkcolor_seq,
        "test get set bkcolor", FALSE);

    DestroyWindow(hTree);
}

static void test_get_set_imagelist(void)
{
    HIMAGELIST himl;
    HWND hTree;

    hTree = create_treeview_control(0);
    fill_tree(hTree);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    /* Test a NULL HIMAGELIST */
    SendMessageA(hTree, TVM_SETIMAGELIST, TVSIL_NORMAL, 0);
    himl = (HIMAGELIST)SendMessageA(hTree, TVM_GETIMAGELIST, TVSIL_NORMAL, 0);
    ok(himl == NULL, "NULL image list, reported as %p, expected 0.\n", himl);

    /* TODO: Test an actual image list */

    ok_sequence(sequences, TREEVIEW_SEQ_INDEX, test_get_set_imagelist_seq,
        "test get imagelist", FALSE);

    DestroyWindow(hTree);
}

static void test_get_set_indent(void)
{
    int ulIndent;
    int ulMinIndent;
    int ulMoreThanTwiceMin;
    HWND hTree;

    hTree = create_treeview_control(0);
    fill_tree(hTree);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    /* Finding the minimum indent */
    SendMessageA(hTree, TVM_SETINDENT, 0, 0);
    ulMinIndent = SendMessageA(hTree, TVM_GETINDENT, 0, 0);

    /* Checking an indent that is more than twice the default indent */
    ulMoreThanTwiceMin = 2*ulMinIndent+1;
    SendMessageA(hTree, TVM_SETINDENT, ulMoreThanTwiceMin, 0);
    ulIndent = SendMessageA(hTree, TVM_GETINDENT, 0, 0);
    ok(ulIndent == ulMoreThanTwiceMin, "Indent reported as %d, expected %d\n", ulIndent, ulMoreThanTwiceMin);

    ok_sequence(sequences, TREEVIEW_SEQ_INDEX, test_get_set_indent_seq,
        "test get set indent", FALSE);

    DestroyWindow(hTree);
}

static void test_get_set_insertmark(void)
{
    COLORREF crColor = RGB(0,0,0);
    HWND hTree;

    hTree = create_treeview_control(0);
    fill_tree(hTree);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    SendMessageA(hTree, TVM_SETINSERTMARKCOLOR, 0, crColor);
    crColor = SendMessageA(hTree, TVM_GETINSERTMARKCOLOR, 0, 0);
    ok(crColor == RGB(0,0,0), "Insert mark color reported as 0x%.8x, expected 0x00000000\n", crColor);

    ok_sequence(sequences, TREEVIEW_SEQ_INDEX, test_get_set_insertmarkcolor_seq,
        "test get set insertmark color", FALSE);

    DestroyWindow(hTree);
}

static void test_get_set_item(void)
{
    TVITEMA tviRoot = {0};
    int nBufferSize = 80;
    char szBuffer[80] = {0};
    HWND hTree, hTree2;
    DWORD ret;

    hTree = create_treeview_control(0);
    fill_tree(hTree);

    tviRoot.hItem = hRoot;
    tviRoot.mask  = TVIF_STATE;
    tviRoot.state = TVIS_FOCUSED;
    tviRoot.stateMask = TVIS_FOCUSED;
    ret = SendMessageA(hTree, TVM_SETITEMA, 0, (LPARAM)&tviRoot);
    expect(TRUE, ret);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    /* Test the root item, state is set even when not requested */
    tviRoot.hItem = hRoot;
    tviRoot.mask = TVIF_TEXT;
    tviRoot.state = 0;
    tviRoot.stateMask = 0;
    tviRoot.cchTextMax = nBufferSize;
    tviRoot.pszText = szBuffer;
    ret = SendMessageA(hTree, TVM_GETITEMA, 0, (LPARAM)&tviRoot);
    expect(TRUE, ret);
    ok(!strcmp("Root", szBuffer), "GetItem: szBuffer=\"%s\", expected \"Root\"\n", szBuffer);
    ok(tviRoot.state == TVIS_FOCUSED, "got 0x%0x\n", tviRoot.state);

    /* Change the root text */
    lstrcpynA(szBuffer, "Testing123", nBufferSize);
    ret = SendMessageA(hTree, TVM_SETITEMA, 0, (LPARAM)&tviRoot);
    expect(TRUE, ret);
    memset(szBuffer, 0, nBufferSize);
    ret = SendMessageA(hTree, TVM_GETITEMA, 0, (LPARAM)&tviRoot);
    expect(TRUE, ret);
    ok(!strcmp("Testing123", szBuffer), "GetItem: szBuffer=\"%s\", expected \"Testing123\"\n", szBuffer);

    /* Reset the root text */
    memset(szBuffer, 0, nBufferSize);
    lstrcpynA(szBuffer, "Root", nBufferSize);
    ret = SendMessageA(hTree, TVM_SETITEMA, 0, (LPARAM)&tviRoot);
    expect(TRUE, ret);

    ok_sequence(sequences, TREEVIEW_SEQ_INDEX, test_get_set_item_seq,
        "test get set item", FALSE);

    /* get item from a different tree */
    hTree2 = create_treeview_control(0);

    tviRoot.hItem = hRoot;
    tviRoot.mask = TVIF_STATE;
    tviRoot.state = 0;
    ret = SendMessageA(hTree2, TVM_GETITEMA, 0, (LPARAM)&tviRoot);
    expect(TRUE, ret);
    ok(tviRoot.state == TVIS_FOCUSED, "got state 0x%0x\n", tviRoot.state);

    /* invalid item pointer, nt4 crashes here but later versions just return 0 */
    tviRoot.hItem = (HTREEITEM)0xdeadbeef;
    tviRoot.mask = TVIF_STATE;
    tviRoot.state = 0;
    ret = SendMessageA(hTree2, TVM_GETITEMA, 0, (LPARAM)&tviRoot);
    expect(FALSE, ret);

    DestroyWindow(hTree);
    DestroyWindow(hTree2);
}

static void test_get_set_itemheight(void)
{
    int ulOldHeight = 0;
    int ulNewHeight = 0;
    HWND hTree;

    hTree = create_treeview_control(0);
    fill_tree(hTree);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    /* Assuming default height to begin with */
    ulOldHeight = SendMessageA(hTree, TVM_GETITEMHEIGHT, 0, 0);

    /* Explicitly setting and getting the default height */
    SendMessageA(hTree, TVM_SETITEMHEIGHT, -1, 0);
    ulNewHeight = SendMessageA(hTree, TVM_GETITEMHEIGHT, 0, 0);
    ok(ulNewHeight == ulOldHeight, "Default height not set properly, reported %d, expected %d\n", ulNewHeight, ulOldHeight);

    /* Explicitly setting and getting the height of twice the normal */
    SendMessageA(hTree, TVM_SETITEMHEIGHT, 2*ulOldHeight, 0);
    ulNewHeight = SendMessageA(hTree, TVM_GETITEMHEIGHT, 0, 0);
    ok(ulNewHeight == 2*ulOldHeight, "New height not set properly, reported %d, expected %d\n", ulNewHeight, 2*ulOldHeight);

    /* Assuming tree doesn't have TVS_NONEVENHEIGHT set, so a set of 9 will round down to 8 */
    SendMessageA(hTree, TVM_SETITEMHEIGHT, 9, 0);
    ulNewHeight = SendMessageA(hTree, TVM_GETITEMHEIGHT, 0, 0);
    ok(ulNewHeight == 8, "Uneven height not set properly, reported %d, expected %d\n", ulNewHeight, 8);

    ok_sequence(sequences, TREEVIEW_SEQ_INDEX, test_get_set_itemheight_seq,
        "test get set item height", FALSE);

    /* without TVS_NONEVENHEIGHT */
    SetWindowLongA(hTree, GWL_STYLE, GetWindowLongA(hTree, GWL_STYLE) & ~TVS_NONEVENHEIGHT);
    /* odd value */
    ulOldHeight = SendMessageA(hTree, TVM_SETITEMHEIGHT, 3, 0);
    ok(ulOldHeight == 8, "got %d, expected %d\n", ulOldHeight, 8);
    ulNewHeight = SendMessageA(hTree, TVM_GETITEMHEIGHT, 0, 0);
    ok(ulNewHeight == 2, "got %d, expected %d\n", ulNewHeight, 2);

    ulOldHeight = SendMessageA(hTree, TVM_SETITEMHEIGHT, 4, 0);
    ok(ulOldHeight == 2, "got %d, expected %d\n", ulOldHeight, 2);
    ulNewHeight = SendMessageA(hTree, TVM_GETITEMHEIGHT, 0, 0);
    ok(ulNewHeight == 4, "got %d, expected %d\n", ulNewHeight, 4);

    /* with TVS_NONEVENHEIGHT */
    SetWindowLongA(hTree, GWL_STYLE, GetWindowLongA(hTree, GWL_STYLE) | TVS_NONEVENHEIGHT);
    /* odd value */
    ulOldHeight = SendMessageA(hTree, TVM_SETITEMHEIGHT, 3, 0);
    ok(ulOldHeight == 4, "got %d, expected %d\n", ulOldHeight, 4);
    ulNewHeight = SendMessageA(hTree, TVM_GETITEMHEIGHT, 0, 0);
    ok(ulNewHeight == 3, "got %d, expected %d\n", ulNewHeight, 3);
    /* even value */
    ulOldHeight = SendMessageA(hTree, TVM_SETITEMHEIGHT, 10, 0);
    ok(ulOldHeight == 3, "got %d, expected %d\n", ulOldHeight, 3);
    ulNewHeight = SendMessageA(hTree, TVM_GETITEMHEIGHT, 0, 0);
    ok(ulNewHeight == 10, "got %d, expected %d\n", ulNewHeight, 10);

    DestroyWindow(hTree);
}

static void test_get_set_scrolltime(void)
{
    int ulExpectedTime = 20;
    int ulTime = 0;
    HWND hTree;

    hTree = create_treeview_control(0);
    fill_tree(hTree);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    SendMessageA(hTree, TVM_SETSCROLLTIME, ulExpectedTime, 0);
    ulTime = SendMessageA(hTree, TVM_GETSCROLLTIME, 0, 0);
    ok(ulTime == ulExpectedTime, "Scroll time reported as %d, expected %d\n", ulTime, ulExpectedTime);

    ok_sequence(sequences, TREEVIEW_SEQ_INDEX, test_get_set_scrolltime_seq,
        "test get set scroll time", FALSE);

    DestroyWindow(hTree);
}

static void test_get_set_textcolor(void)
{
    /* If the value is -1, the control is using the system color for the text color. */
    COLORREF crColor;
    HWND hTree;

    hTree = create_treeview_control(0);
    fill_tree(hTree);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    crColor = SendMessageA(hTree, TVM_GETTEXTCOLOR, 0, 0);
    ok(crColor == ~0u, "Default text color reported as 0x%.8x\n", crColor);

    /* Test for black text */
    SendMessageA(hTree, TVM_SETTEXTCOLOR, 0, RGB(0,0,0));
    crColor = SendMessageA(hTree, TVM_GETTEXTCOLOR, 0, 0);
    ok(crColor == RGB(0,0,0), "Black text color reported as 0x%.8x\n", crColor);

    /* Test for white text */
    SendMessageA(hTree, TVM_SETTEXTCOLOR, 0, RGB(255,255,255));
    crColor = SendMessageA(hTree, TVM_GETTEXTCOLOR, 0, 0);
    ok(crColor == RGB(255,255,255), "White text color reported as 0x%.8x\n", crColor);

    /* Reset the default text color */
    SendMessageA(hTree, TVM_SETTEXTCOLOR, 0, CLR_NONE);

    ok_sequence(sequences, TREEVIEW_SEQ_INDEX, test_get_set_textcolor_seq,
        "test get set text color", FALSE);

    DestroyWindow(hTree);
}

static void test_get_set_tooltips(void)
{
    HWND hTree, tooltips, hwnd;
    DWORD style;
    int i;

    /* TVS_NOTOOLTIPS */
    hTree = create_treeview_control(TVS_NOTOOLTIPS);

    tooltips = (HWND)SendMessageA(hTree, TVM_GETTOOLTIPS, 0, 0);
    ok(tooltips == NULL, "Unexpected tooltip window %p.\n", tooltips);

    tooltips = (HWND)SendMessageA(hTree, TVM_SETTOOLTIPS, 0, 0);
    ok(tooltips == NULL, "Unexpected ret value %p.\n", tooltips);

    /* Toggle style */
    style = GetWindowLongA(hTree, GWL_STYLE);
    SetWindowLongA(hTree, GWL_STYLE, style & ~TVS_NOTOOLTIPS);

    tooltips = (HWND)SendMessageA(hTree, TVM_GETTOOLTIPS, 0, 0);
    ok(IsWindow(tooltips), "Unexpected tooltip window %p.\n", tooltips);

    style = GetWindowLongA(hTree, GWL_STYLE);
    SetWindowLongA(hTree, GWL_STYLE, style | TVS_NOTOOLTIPS);

    tooltips = (HWND)SendMessageA(hTree, TVM_GETTOOLTIPS, 0, 0);
    ok(tooltips == NULL, "Unexpected tooltip window %p.\n", tooltips);

    DestroyWindow(hTree);

    /* Set some valid window, does not have to be tooltips class. */
    hTree = create_treeview_control(TVS_NOTOOLTIPS);

    hwnd = CreateWindowA(WC_STATICA, "Test", WS_VISIBLE|WS_CHILD, 5, 5, 100, 100, hMainWnd, NULL, NULL, 0);
    ok(hwnd != NULL, "Failed to create child window.\n");

    tooltips = (HWND)SendMessageA(hTree, TVM_SETTOOLTIPS, (WPARAM)hwnd, 0);
    ok(tooltips == NULL, "Unexpected ret value %p.\n", tooltips);

    tooltips = (HWND)SendMessageA(hTree, TVM_GETTOOLTIPS, 0, 0);
    ok(tooltips == hwnd, "Unexpected tooltip window %p.\n", tooltips);

    /* Externally set tooltips window, disable style. */
    style = GetWindowLongA(hTree, GWL_STYLE);
    SetWindowLongA(hTree, GWL_STYLE, style & ~TVS_NOTOOLTIPS);

    tooltips = (HWND)SendMessageA(hTree, TVM_GETTOOLTIPS, 0, 0);
    ok(IsWindow(tooltips) && tooltips != hwnd, "Unexpected tooltip window %p.\n", tooltips);
    ok(IsWindow(hwnd), "Expected valid window.\n");

    style = GetWindowLongA(hTree, GWL_STYLE);
    SetWindowLongA(hTree, GWL_STYLE, style | TVS_NOTOOLTIPS);
    ok(!IsWindow(tooltips), "Unexpected tooltip window %p.\n", tooltips);

    tooltips = (HWND)SendMessageA(hTree, TVM_GETTOOLTIPS, 0, 0);
    ok(tooltips == NULL, "Unexpected tooltip window %p.\n", tooltips);
    ok(IsWindow(hwnd), "Expected valid window.\n");

    DestroyWindow(hTree);
    ok(IsWindow(hwnd), "Expected valid window.\n");

    /* Set window, disable tooltips. */
    hTree = create_treeview_control(0);

    tooltips = (HWND)SendMessageA(hTree, TVM_SETTOOLTIPS, (WPARAM)hwnd, 0);
    ok(IsWindow(tooltips), "Unexpected ret value %p.\n", tooltips);

    style = GetWindowLongA(hTree, GWL_STYLE);
    SetWindowLongA(hTree, GWL_STYLE, style | TVS_NOTOOLTIPS);
    ok(!IsWindow(hwnd), "Unexpected tooltip window %p.\n", tooltips);
    ok(IsWindow(tooltips), "Expected valid window %p.\n", tooltips);

    DestroyWindow(hTree);
    ok(IsWindow(tooltips), "Expected valid window %p.\n", tooltips);
    DestroyWindow(tooltips);
    DestroyWindow(hwnd);

    for (i = 0; i < 2; i++)
    {
        DWORD style = i == 0 ? 0 : TVS_NOTOOLTIPS;

        hwnd = CreateWindowA(WC_STATICA, "Test", WS_VISIBLE|WS_CHILD, 5, 5, 100, 100, hMainWnd, NULL, NULL, 0);
        ok(hwnd != NULL, "Failed to create child window.\n");

        hTree = create_treeview_control(style);

        tooltips = (HWND)SendMessageA(hTree, TVM_SETTOOLTIPS, (WPARAM)hwnd, 0);
        ok(style & TVS_NOTOOLTIPS ? tooltips == NULL : IsWindow(tooltips), "Unexpected ret value %p.\n", tooltips);
        DestroyWindow(tooltips);

        tooltips = (HWND)SendMessageA(hTree, TVM_GETTOOLTIPS, 0, 0);
        ok(tooltips == hwnd, "Unexpected tooltip window %p.\n", tooltips);

        /* TreeView is destroyed, check if set window is still around. */
        DestroyWindow(hTree);
        ok(!IsWindow(hwnd), "Unexpected window.\n");
    }

    hTree = create_treeview_control(0);
    fill_tree(hTree);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    /* show even WS_POPUP treeview don't send NM_TOOLTIPSCREATED */
    hwnd = CreateWindowA(WC_TREEVIEWA, NULL, WS_POPUP|WS_VISIBLE, 0, 0, 100, 100,
            hMainWnd, NULL, NULL, NULL);
    DestroyWindow(hwnd);

    /* Testing setting a NULL ToolTip */
    tooltips = (HWND)SendMessageA(hTree, TVM_SETTOOLTIPS, 0, 0);
    ok(IsWindow(tooltips), "Unexpected ret value %p.\n", tooltips);

    hwnd = (HWND)SendMessageA(hTree, TVM_GETTOOLTIPS, 0, 0);
    ok(hwnd == NULL, "Unexpected tooltip window %p.\n", hwnd);

    ok_sequence(sequences, TREEVIEW_SEQ_INDEX, test_get_set_tooltips_seq,
        "test get set tooltips", TRUE);

    DestroyWindow(hTree);
    ok(IsWindow(tooltips), "Expected valid window.\n");
    DestroyWindow(tooltips);
}

static void test_get_set_unicodeformat(void)
{
    BOOL bPreviousSetting;
    BOOL bNewSetting;
    HWND hTree;

    hTree = create_treeview_control(0);
    fill_tree(hTree);

    /* Check that an invalid format returned by NF_QUERY defaults to ANSI */
    bPreviousSetting = SendMessageA(hTree, TVM_GETUNICODEFORMAT, 0, 0);
    ok(bPreviousSetting == FALSE, "Format should be ANSI.\n");

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    /* Set to Unicode */
    bPreviousSetting = SendMessageA(hTree, TVM_SETUNICODEFORMAT, 1, 0);
    bNewSetting = SendMessageA(hTree, TVM_GETUNICODEFORMAT, 0, 0);
    ok(bNewSetting == TRUE, "Unicode setting did not work.\n");

    /* Set to ANSI */
    SendMessageA(hTree, TVM_SETUNICODEFORMAT, 0, 0);
    bNewSetting = SendMessageA(hTree, TVM_GETUNICODEFORMAT, 0, 0);
    ok(bNewSetting == FALSE, "ANSI setting did not work.\n");

    /* Revert to original setting */
    SendMessageA(hTree, TVM_SETUNICODEFORMAT, bPreviousSetting, 0);

    ok_sequence(sequences, TREEVIEW_SEQ_INDEX, test_get_set_unicodeformat_seq,
        "test get set unicode format", FALSE);

    DestroyWindow(hTree);
}

static LRESULT CALLBACK parent_wnd_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static LONG defwndproc_counter = 0;
    struct message msg = { 0 };
    LRESULT ret;
    RECT rect;
    HTREEITEM visibleItem;

    msg.message = message;
    msg.flags = sent|wparam|lparam;
    if (defwndproc_counter) msg.flags |= defwinproc;
    msg.wParam = wParam;
    msg.lParam = lParam;
    if (message == WM_NOTIFY && lParam)
        msg.id = ((NMHDR*)lParam)->code;

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
    }

    switch(message) {
    case WM_NOTIFYFORMAT:
    {
        /* Make NF_QUERY return an invalid format to show that it defaults to ANSI */
        if (lParam == NF_QUERY) return 0;
        break;
    }

    case WM_NOTIFY:
    {
        NMHDR *pHdr = (NMHDR *)lParam;

        ok(pHdr->code != NM_TOOLTIPSCREATED, "Treeview should not send NM_TOOLTIPSCREATED\n");
        if (pHdr->idFrom == 100)
        {
            NMTREEVIEWA *pTreeView = (LPNMTREEVIEWA) lParam;
            switch(pHdr->code)
            {
            case TVN_SELCHANGINGA:
                AddItem('(');
                IdentifyItem(pTreeView->itemOld.hItem);
                IdentifyItem(pTreeView->itemNew.hItem);
                break;
            case TVN_SELCHANGEDA:
                AddItem(')');
                IdentifyItem(pTreeView->itemOld.hItem);
                IdentifyItem(pTreeView->itemNew.hItem);
                break;
            case TVN_GETDISPINFOA: {
                NMTVDISPINFOA *disp = (NMTVDISPINFOA *)lParam;
                if (disp->item.mask & TVIF_TEXT) {
                    lstrcpynA(disp->item.pszText, TEST_CALLBACK_TEXT, disp->item.cchTextMax);
                }

                if (g_disp_A_to_W && (disp->item.mask & TVIF_TEXT)) {
                    static const WCHAR testW[] = {'T','E','S','T','2',0};

                    disp->hdr.code = TVN_GETDISPINFOW;
                    memcpy(disp->item.pszText, testW, sizeof(testW));
                }

                if (g_disp_set_stateimage)
                {
                    ok(disp->item.mask == TVIF_IMAGE, "got %x\n", disp->item.mask);
                    /* both masks set here are necessary to change state bits */
                    disp->item.mask |= TVIF_STATE;
                    disp->item.state = TVIS_SELECTED | INDEXTOSTATEIMAGEMASK(2) | INDEXTOOVERLAYMASK(3);
                    disp->item.stateMask = TVIS_SELECTED | TVIS_OVERLAYMASK | TVIS_STATEIMAGEMASK;
                }

                break;
              }
            case TVN_BEGINLABELEDITA:
              {
                if (g_beginedit_alter_text)
                {
                    static const char* textA = "<edittextaltered>";
                    HWND edit;

                    edit = (HWND)SendMessageA(pHdr->hwndFrom, TVM_GETEDITCONTROL, 0, 0);
                    ok(IsWindow(edit), "failed to get edit handle\n");
                    SetWindowTextA(edit, textA);
                }

                break;
              }

            case TVN_ENDLABELEDITA:
              {
                NMTVDISPINFOA *disp = (NMTVDISPINFOA *)lParam;
                if (disp->item.mask & TVIF_TEXT)
                {
                    ok(disp->item.cchTextMax == MAX_PATH, "cchTextMax is %d\n", disp->item.cchTextMax);
                    if (g_endedit_overwrite_contents)
                        strcpy(disp->item.pszText, g_endedit_overwrite_contents);
                    if (g_endedit_overwrite_ptr)
                        disp->item.pszText = g_endedit_overwrite_ptr;
                }
                return TRUE;
              }
            case TVN_ITEMEXPANDINGA:
              {
                UINT newmask = pTreeView->itemNew.mask & ~TVIF_CHILDREN;
                ok(newmask ==
                   (TVIF_HANDLE | TVIF_SELECTEDIMAGE | TVIF_IMAGE | TVIF_PARAM | TVIF_STATE),
                   "got wrong mask %x\n", pTreeView->itemNew.mask);
                ok(pTreeView->itemOld.mask == 0,
                   "got wrong mask %x\n", pTreeView->itemOld.mask);

                if (g_get_from_expand)
                {
                  g_item_expanding.mask = TVIF_STATE;
                  g_item_expanding.hItem = hRoot;
                  ret = SendMessageA(pHdr->hwndFrom, TVM_GETITEMA, 0, (LPARAM)&g_item_expanding);
                  ok(ret == TRUE, "got %lu\n", ret);
                }
                break;
              }
            case TVN_ITEMEXPANDEDA:
                ok(pTreeView->itemNew.mask & TVIF_STATE, "got wrong mask %x\n", pTreeView->itemNew.mask);
                ok(pTreeView->itemNew.state & (TVIS_EXPANDED|TVIS_EXPANDEDONCE),
                   "got wrong mask %x\n", pTreeView->itemNew.mask);
                ok(pTreeView->itemOld.mask == 0,
                   "got wrong mask %x\n", pTreeView->itemOld.mask);

                if (g_get_from_expand)
                {
                  g_item_expanded.mask = TVIF_STATE;
                  g_item_expanded.hItem = hRoot;
                  ret = SendMessageA(pHdr->hwndFrom, TVM_GETITEMA, 0, (LPARAM)&g_item_expanded);
                  ok(ret == TRUE, "got %lu\n", ret);
                }
                if (g_get_rect_in_expand)
                {
                  visibleItem = (HTREEITEM)SendMessageA(pHdr->hwndFrom, TVM_GETNEXTITEM,
                          TVGN_FIRSTVISIBLE, 0);
                  ok(pTreeView->itemNew.hItem == visibleItem, "expanded item == first visible item\n");
                  *(HTREEITEM*)&rect = visibleItem;
                  ok(SendMessageA(pHdr->hwndFrom, TVM_GETITEMRECT, TRUE, (LPARAM)&rect),
                          "Failed to get rect for first visible item.\n");
                  visibleItem = (HTREEITEM)SendMessageA(pHdr->hwndFrom, TVM_GETNEXTITEM,
                          TVGN_NEXTVISIBLE, (LPARAM)visibleItem);
                  *(HTREEITEM*)&rect = visibleItem;
                  ok(visibleItem != NULL, "There must be a visible item after the first one.\n");
                  ok(SendMessageA(pHdr->hwndFrom, TVM_GETITEMRECT, TRUE, (LPARAM)&rect),
                          "Failed to get rect for second visible item.\n");
                }
                break;
            case TVN_DELETEITEMA:
            {
                struct message item;

                ok(pTreeView->itemNew.mask == 0, "got wrong mask 0x%x\n", pTreeView->itemNew.mask);

                ok(pTreeView->itemOld.mask == (TVIF_HANDLE | TVIF_PARAM), "got wrong mask 0x%x\n", pTreeView->itemOld.mask);
                ok(pTreeView->itemOld.hItem != NULL, "got %p\n", pTreeView->itemOld.hItem);

                memset(&item, 0, sizeof(item));
                item.lParam = (LPARAM)pTreeView->itemOld.hItem;
                add_message(item_sequence, 0, &item);

                break;
            }
            case NM_CUSTOMDRAW:
            {
                NMTVCUSTOMDRAW *nmcd = (NMTVCUSTOMDRAW*)lParam;
                COLORREF c0ffee = RGB(0xc0,0xff,0xee), cafe = RGB(0xca,0xfe,0x00);
                COLORREF text = GetTextColor(nmcd->nmcd.hdc), bkgnd = GetBkColor(nmcd->nmcd.hdc);

                msg.flags |= custdraw;
                msg.stage = nmcd->nmcd.dwDrawStage;
                add_message(sequences, PARENT_CD_SEQ_INDEX, &msg);

                switch (msg.stage)
                {
                case CDDS_PREPAINT:
                    return CDRF_NOTIFYITEMDRAW|CDRF_NOTIFYITEMERASE|CDRF_NOTIFYPOSTPAINT;
                case CDDS_ITEMPREPAINT:
                    ok(text == nmcd->clrText || (g_v6 && nmcd->clrText == 0xffffffff),
                       "got %08x vs %08x\n", text, nmcd->clrText);
                    ok(bkgnd == nmcd->clrTextBk || (g_v6 && nmcd->clrTextBk == 0xffffffff),
                       "got %08x vs %08x\n", bkgnd, nmcd->clrTextBk);
                    nmcd->clrText = cafe;
                    nmcd->clrTextBk = c0ffee;
                    SetTextColor(nmcd->nmcd.hdc, c0ffee);
                    SetBkColor(nmcd->nmcd.hdc, cafe);
                    if (g_customdraw_font)
                        SelectObject(nmcd->nmcd.hdc, g_customdraw_font);
                    return CDRF_NOTIFYPOSTPAINT|CDRF_NEWFONT;
                case CDDS_ITEMPOSTPAINT:
                    /* at the point of post paint notification colors are already restored */
                    ok(nmcd->clrText == cafe, "got 0%x\n", nmcd->clrText);
                    ok(nmcd->clrTextBk == c0ffee, "got 0%x\n", nmcd->clrTextBk);
                    ok(text != cafe, "got 0%x\n", text);
                    ok(bkgnd != c0ffee, "got 0%x\n", bkgnd);
                    if (g_customdraw_font)
                        ok(GetCurrentObject(nmcd->nmcd.hdc, OBJ_FONT) != g_customdraw_font, "got %p\n",
                           GetCurrentObject(nmcd->nmcd.hdc, OBJ_FONT));
                    break;
                default:
                    ;
                }
                break;
            }
            case NM_RCLICK:
            {
                HTREEITEM selected = (HTREEITEM)SendMessageA(((NMHDR *)lParam)->hwndFrom,
                                                             TVM_GETNEXTITEM, TVGN_CARET, 0);
                ok(selected == hChild, "child item should still be selected\n");
                break;
            }
            }
        }
        break;
    }

    }

    defwndproc_counter++;
    ret = DefWindowProcA(hWnd, message, wParam, lParam);
    defwndproc_counter--;

    return ret;
}

static void test_expandinvisible(void)
{
    static CHAR nodeText[][5] = {"0", "1", "2", "3", "4"};
    TVINSERTSTRUCTA ins;
    HTREEITEM node[5];
    RECT dummyRect;
    BOOL nodeVisible;
    LRESULT ret;
    HWND hTree;

    hTree = create_treeview_control(0);

    /* The test builds the following tree and expands node 1, while node 0 is collapsed.
     *
     * 0
     * |- 1
     * |  |- 2
     * |  |- 3
     * |- 4
     *
     */

    ret = SendMessageA(hTree, TVM_DELETEITEM, 0, (LPARAM)TVI_ROOT);
    ok(ret == TRUE, "ret\n");
    ins.hParent = TVI_ROOT;
    ins.hInsertAfter = TVI_ROOT;
    U(ins).item.mask = TVIF_TEXT;
    U(ins).item.pszText = nodeText[0];
    node[0] = TreeView_InsertItemA(hTree, &ins);
    ok(node[0] != NULL, "failed to set node[0]\n");

    ins.hInsertAfter = TVI_LAST;
    U(ins).item.mask = TVIF_TEXT;
    ins.hParent = node[0];

    U(ins).item.pszText = nodeText[1];
    node[1] = TreeView_InsertItemA(hTree, &ins);
    ok(node[1] != NULL, "failed to set node[1]\n");
    U(ins).item.pszText = nodeText[4];
    node[4] = TreeView_InsertItemA(hTree, &ins);
    ok(node[4] != NULL, "failed to set node[4]\n");

    ins.hParent = node[1];

    U(ins).item.pszText = nodeText[2];
    node[2] = TreeView_InsertItemA(hTree, &ins);
    ok(node[2] != NULL, "failed to set node[2]\n");
    U(ins).item.pszText = nodeText[3];
    node[3] = TreeView_InsertItemA(hTree, &ins);
    ok(node[3] != NULL, "failed to set node[3]\n");

    *(HTREEITEM *)&dummyRect = node[1];
    nodeVisible = SendMessageA(hTree, TVM_GETITEMRECT, FALSE, (LPARAM)&dummyRect);
    ok(!nodeVisible, "Node 1 should not be visible.\n");
    *(HTREEITEM *)&dummyRect = node[2];
    nodeVisible = SendMessageA(hTree, TVM_GETITEMRECT, FALSE, (LPARAM)&dummyRect);
    ok(!nodeVisible, "Node 2 should not be visible.\n");
    *(HTREEITEM *)&dummyRect = node[3];
    nodeVisible = SendMessageA(hTree, TVM_GETITEMRECT, FALSE, (LPARAM)&dummyRect);
    ok(!nodeVisible, "Node 3 should not be visible.\n");
    *(HTREEITEM *)&dummyRect = node[4];
    nodeVisible = SendMessageA(hTree, TVM_GETITEMRECT, FALSE, (LPARAM)&dummyRect);
    ok(!nodeVisible, "Node 4 should not be visible.\n");

    ok(SendMessageA(hTree, TVM_EXPAND, TVE_EXPAND, (LPARAM)node[1]), "Expand of node 1 failed.\n");

    *(HTREEITEM *)&dummyRect = node[1];
    nodeVisible = SendMessageA(hTree, TVM_GETITEMRECT, FALSE, (LPARAM)&dummyRect);
    ok(!nodeVisible, "Node 1 should not be visible.\n");
    *(HTREEITEM *)&dummyRect = node[2];
    nodeVisible = SendMessageA(hTree, TVM_GETITEMRECT, FALSE, (LPARAM)&dummyRect);
    ok(!nodeVisible, "Node 2 should not be visible.\n");
    *(HTREEITEM *)&dummyRect = node[3];
    nodeVisible = SendMessageA(hTree, TVM_GETITEMRECT, FALSE, (LPARAM)&dummyRect);
    ok(!nodeVisible, "Node 3 should not be visible.\n");
    *(HTREEITEM *)&dummyRect = node[4];
    nodeVisible = SendMessageA(hTree, TVM_GETITEMRECT, FALSE, (LPARAM)&dummyRect);
    ok(!nodeVisible, "Node 4 should not be visible.\n");

    DestroyWindow(hTree);
}

static void test_expand(void)
{
    HTREEITEM first, second, last, child;
    TVINSERTSTRUCTA ins;
    BOOL visible;
    RECT rect;
    HWND tv;
    int i;

    tv = create_treeview_control(0);

    ins.hParent = TVI_ROOT;
    ins.hInsertAfter = TVI_LAST;
    U(ins).item.mask = 0;
    first = TreeView_InsertItemA(tv, &ins);
    ok(first != NULL, "failed to insert first node\n");
    second = TreeView_InsertItemA(tv, &ins);
    ok(second != NULL, "failed to insert second node\n");
    for (i=0; i<100; i++)
    {
        last = TreeView_InsertItemA(tv, &ins);
        ok(last != NULL, "failed to insert %d node\n", i);
    }

    ins.hParent = second;
    child = TreeView_InsertItemA(tv, &ins);
    ok(child != NULL, "failed to insert child node\n");

    ok(SendMessageA(tv, TVM_SELECTITEM, TVGN_CARET, (LPARAM)last), "last node selection failed\n");
    ok(SendMessageA(tv, TVM_EXPAND, TVE_EXPAND, (LPARAM)second), "expand of second node failed\n");
    ok(SendMessageA(tv, TVM_SELECTITEM, TVGN_CARET, (LPARAM)first), "first node selection failed\n");

    *(HTREEITEM *)&rect = first;
    visible = SendMessageA(tv, TVM_GETITEMRECT, FALSE, (LPARAM)&rect);
    ok(visible, "first node should be visible\n");
    ok(!rect.left, "rect.left = %d\n", rect.left);
    ok(!rect.top, "rect.top = %d\n", rect.top);
    ok(rect.right, "rect.right = 0\n");
    ok(rect.bottom, "rect.bottom = 0\n");

    DestroyWindow(tv);
}

static void test_itemedit(void)
{
    DWORD r;
    HWND edit;
    TVITEMA item;
    CHAR buffA[500];
    HWND hTree;

    hTree = create_treeview_control(0);
    fill_tree(hTree);

    /* try with null item */
    edit = (HWND)SendMessageA(hTree, TVM_EDITLABELA, 0, 0);
    ok(!IsWindow(edit), "Expected valid handle\n");

    /* trigger edit */
    edit = (HWND)SendMessageA(hTree, TVM_EDITLABELA, 0, (LPARAM)hRoot);
    ok(IsWindow(edit), "Expected valid handle\n");
    /* item shouldn't be selected automatically after TVM_EDITLABELA */
    r = SendMessageA(hTree, TVM_GETITEMSTATE, (WPARAM)hRoot, TVIS_SELECTED);
    expect(0, r);
    /* try to cancel with wrong edit handle */
    r = SendMessageA(hTree, WM_COMMAND, MAKEWPARAM(0, EN_KILLFOCUS), 0);
    expect(0, r);
    ok(IsWindow(edit), "Expected edit control to be valid\n");
    r = SendMessageA(hTree, WM_COMMAND, MAKEWPARAM(0, EN_KILLFOCUS), (LPARAM)edit);
    expect(0, r);
    ok(!IsWindow(edit), "Expected edit control to be destroyed\n");
    /* try to cancel without creating edit */
    r = SendMessageA(hTree, WM_COMMAND, MAKEWPARAM(0, EN_KILLFOCUS), 0);
    expect(0, r);

    /* try to cancel with wrong (not null) handle */
    edit = (HWND)SendMessageA(hTree, TVM_EDITLABELA, 0, (LPARAM)hRoot);
    ok(IsWindow(edit), "Expected valid handle\n");
    r = SendMessageA(hTree, WM_COMMAND, MAKEWPARAM(0, EN_KILLFOCUS), (LPARAM)hTree);
    expect(0, r);
    ok(IsWindow(edit), "Expected edit control to be valid\n");
    r = SendMessageA(hTree, WM_COMMAND, MAKEWPARAM(0, EN_KILLFOCUS), (LPARAM)edit);
    expect(0, r);

    /* remove selection after starting edit */
    r = SendMessageA(hTree, TVM_SELECTITEM, TVGN_CARET, (LPARAM)hRoot);
    expect(TRUE, r);
    edit = (HWND)SendMessageA(hTree, TVM_EDITLABELA, 0, (LPARAM)hRoot);
    ok(IsWindow(edit), "Expected valid handle\n");
    r = SendMessageA(hTree, TVM_SELECTITEM, TVGN_CARET, 0);
    expect(TRUE, r);
    /* alter text */
    strcpy(buffA, "x");
    r = SendMessageA(edit, WM_SETTEXT, 0, (LPARAM)buffA);
    expect(TRUE, r);
    r = SendMessageA(hTree, WM_COMMAND, MAKEWPARAM(0, EN_KILLFOCUS), (LPARAM)edit);
    expect(0, r);
    ok(!IsWindow(edit), "Expected edit control to be destroyed\n");
    /* check that text is saved */
    item.mask = TVIF_TEXT;
    item.hItem = hRoot;
    item.pszText = buffA;
    item.cchTextMax = ARRAY_SIZE(buffA);
    r = SendMessageA(hTree, TVM_GETITEMA, 0, (LPARAM)&item);
    expect(TRUE, r);
    ok(!strcmp("x", buffA), "Expected item text to change\n");

    /* try A/W messages */
    edit = (HWND)SendMessageA(hTree, TVM_EDITLABELA, 0, (LPARAM)hRoot);
    ok(IsWindow(edit), "Expected valid handle\n");
    ok(IsWindowUnicode(edit), "got ansi window\n");
    r = SendMessageA(hTree, WM_COMMAND, MAKEWPARAM(0, EN_KILLFOCUS), (LPARAM)edit);
    expect(0, r);
    ok(!IsWindow(edit), "expected invalid handle\n");

    edit = (HWND)SendMessageA(hTree, TVM_EDITLABELW, 0, (LPARAM)hRoot);
    ok(IsWindow(edit), "Expected valid handle\n");
    ok(IsWindowUnicode(edit), "got ansi window\n");
    r = SendMessageA(hTree, WM_COMMAND, MAKEWPARAM(0, EN_KILLFOCUS), (LPARAM)edit);
    expect(0, r);

    /* alter text during TVM_BEGINLABELEDIT, check that it's preserved */
    strcpy(buffA, "<root>");

    item.mask = TVIF_TEXT;
    item.hItem = hRoot;
    item.pszText = buffA;
    item.cchTextMax = 0;
    r = SendMessageA(hTree, TVM_SETITEMA, 0, (LPARAM)&item);
    expect(TRUE, r);

    g_beginedit_alter_text = TRUE;
    edit = (HWND)SendMessageA(hTree, TVM_EDITLABELA, 0, (LPARAM)hRoot);
    ok(IsWindow(edit), "Expected valid handle\n");
    g_beginedit_alter_text = FALSE;

    GetWindowTextA(edit, buffA, ARRAY_SIZE(buffA));
    ok(!strcmp(buffA, "<edittextaltered>"), "got string %s\n", buffA);

    r = SendMessageA(hTree, WM_COMMAND, MAKEWPARAM(0, EN_KILLFOCUS), (LPARAM)edit);
    expect(0, r);

    /* How much text can be typed? */
    edit = (HWND)SendMessageA(hTree, TVM_EDITLABELA, 0, (LPARAM)hRoot);
    ok(IsWindow(edit), "Expected valid handle\n");
    r = SendMessageA(edit, EM_GETLIMITTEXT, 0, 0);
    expect(MAX_PATH - 1, r);
    /* WM_SETTEXT can set more... */
    memset(buffA, 'a', ARRAY_SIZE(buffA));
    buffA[ARRAY_SIZE(buffA)-1] = 0;
    r = SetWindowTextA(edit, buffA);
    expect(TRUE, r);
    r = GetWindowTextA(edit, buffA, ARRAY_SIZE(buffA));
    ok( r == ARRAY_SIZE(buffA) - 1, "got %d\n", r );
    /* ...but it's trimmed to MAX_PATH chars when editing ends */
    r = SendMessageA(hTree, WM_COMMAND, MAKEWPARAM(0, EN_KILLFOCUS), (LPARAM)edit);
    expect(0, r);
    item.mask = TVIF_TEXT;
    item.hItem = hRoot;
    item.pszText = buffA;
    item.cchTextMax = ARRAY_SIZE(buffA);
    r = SendMessageA(hTree, TVM_GETITEMA, 0, (LPARAM)&item);
    expect(TRUE, r);
    expect(MAX_PATH - 1, lstrlenA(item.pszText));

    /* We can't get around that MAX_PATH limit by increasing EM_SETLIMITTEXT */
    edit = (HWND)SendMessageA(hTree, TVM_EDITLABELA, 0, (LPARAM)hRoot);
    ok(IsWindow(edit), "Expected valid handle\n");
    SendMessageA(edit, EM_SETLIMITTEXT, ARRAY_SIZE(buffA)-1, 0);
    memset(buffA, 'a', ARRAY_SIZE(buffA));
    buffA[ARRAY_SIZE(buffA)-1] = 0;
    r = SetWindowTextA(edit, buffA);
    expect(TRUE, r);
    r = SendMessageA(hTree, WM_COMMAND, MAKEWPARAM(0, EN_KILLFOCUS), (LPARAM)edit);
    expect(0, r);
    item.mask = TVIF_TEXT;
    item.hItem = hRoot;
    item.pszText = buffA;
    item.cchTextMax = ARRAY_SIZE(buffA);
    r = SendMessageA(hTree, TVM_GETITEMA, 0, (LPARAM)&item);
    expect(TRUE, r);
    expect(MAX_PATH - 1, lstrlenA(item.pszText));

    /* Overwriting of pszText contents in TVN_ENDLABELEDIT */
    edit = (HWND)SendMessageA(hTree, TVM_EDITLABELA, 0, (LPARAM)hRoot);
    ok(IsWindow(edit), "Expected valid handle\n");
    r = SetWindowTextA(edit, "old");
    expect(TRUE, r);
    g_endedit_overwrite_contents = "<new_contents>";
    r = SendMessageA(hTree, WM_COMMAND, MAKEWPARAM(0, EN_KILLFOCUS), (LPARAM)edit);
    expect(0, r);
    g_endedit_overwrite_contents = NULL;
    item.mask = TVIF_TEXT;
    item.hItem = hRoot;
    item.pszText = buffA;
    item.cchTextMax = ARRAY_SIZE(buffA);
    r = SendMessageA(hTree, TVM_GETITEMA, 0, (LPARAM)&item);
    expect(TRUE, r);
    expect(0, strcmp(item.pszText, "<new_contents>"));

    /* Overwriting of pszText pointer in TVN_ENDLABELEDIT */
    edit = (HWND)SendMessageA(hTree, TVM_EDITLABELA, 0, (LPARAM)hRoot);
    ok(IsWindow(edit), "Expected valid handle\n");
    r = SetWindowTextA(edit, "old");
    expect(TRUE, r);
    g_endedit_overwrite_ptr = (char*) "<new_ptr>";
    r = SendMessageA(hTree, WM_COMMAND, MAKEWPARAM(0, EN_KILLFOCUS), (LPARAM)edit);
    expect(0, r);
    g_endedit_overwrite_ptr = NULL;
    item.mask = TVIF_TEXT;
    item.hItem = hRoot;
    item.pszText = buffA;
    item.cchTextMax = ARRAY_SIZE(buffA);
    r = SendMessageA(hTree, TVM_GETITEMA, 0, (LPARAM)&item);
    expect(TRUE, r);
    expect(0, strcmp(item.pszText, "<new_ptr>"));

    DestroyWindow(hTree);
}

static void test_treeview_classinfo(void)
{
    WNDCLASSA cls;

    memset(&cls, 0, sizeof(cls));
    GetClassInfoA(GetModuleHandleA("comctl32.dll"), WC_TREEVIEWA, &cls);
    ok(cls.hbrBackground == NULL, "Expected NULL background brush, got %p\n", cls.hbrBackground);
    ok(cls.style == (CS_GLOBALCLASS | CS_DBLCLKS), "Expected got %x\n", cls.style);
    expect(0, cls.cbClsExtra);
}

static void test_get_linecolor(void)
{
    COLORREF clr;
    HWND hTree;

    hTree = create_treeview_control(0);

    /* newly created control has default color */
    clr = SendMessageA(hTree, TVM_GETLINECOLOR, 0, 0);
    if (clr == 0)
        win_skip("TVM_GETLINECOLOR is not supported on comctl32 < 5.80\n");
    else
        expect(CLR_DEFAULT, clr);

    DestroyWindow(hTree);
}

static void test_get_insertmarkcolor(void)
{
    COLORREF clr;
    HWND hTree;

    hTree = create_treeview_control(0);

    /* newly created control has default color */
    clr = SendMessageA(hTree, TVM_GETINSERTMARKCOLOR, 0, 0);
    if (clr == 0)
        win_skip("TVM_GETINSERTMARKCOLOR is not supported on comctl32 < 5.80\n");
    else
        expect(CLR_DEFAULT, clr);

    DestroyWindow(hTree);
}

static void test_expandnotify(void)
{
    HTREEITEM hitem;
    HWND hTree;
    BOOL ret;
    TVITEMA item;

    hTree = create_treeview_control(0);
    fill_tree(hTree);

    item.hItem = hRoot;
    item.mask = TVIF_STATE;

    item.state = TVIS_EXPANDED;
    ret = SendMessageA(hTree, TVM_GETITEMA, 0, (LPARAM)&item);
    expect(TRUE, ret);
    ok((item.state & TVIS_EXPANDED) == 0, "expected collapsed\n");

    /* preselect root node here */
    ret = SendMessageA(hTree, TVM_SELECTITEM, TVGN_CARET, (LPARAM)hRoot);
    expect(TRUE, ret);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    ret = SendMessageA(hTree, TVM_EXPAND, TVE_COLLAPSE, (LPARAM)hRoot);
    expect(FALSE, ret);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq, "no collapse notifications", FALSE);

    g_get_from_expand = TRUE;
    /* expand */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    g_item_expanding.state = 0xdeadbeef;
    g_item_expanded.state = 0xdeadbeef;
    ret = SendMessageA(hTree, TVM_EXPAND, TVE_EXPAND, (LPARAM)hRoot);
    expect(TRUE, ret);
    ok(g_item_expanding.state == TVIS_SELECTED, "got state on TVN_ITEMEXPANDING 0x%08x\n",
       g_item_expanding.state);
    ok(g_item_expanded.state == (TVIS_SELECTED|TVIS_EXPANDED), "got state on TVN_ITEMEXPANDED 0x%08x\n",
       g_item_expanded.state);
    ok_sequence(sequences, PARENT_SEQ_INDEX, parent_expand_seq, "expand notifications", FALSE);
    g_get_from_expand = FALSE;

    /* check that it's expanded */
    item.state = TVIS_EXPANDED;
    ret = SendMessageA(hTree, TVM_GETITEMA, 0, (LPARAM)&item);
    expect(TRUE, ret);
    ok((item.state & TVIS_EXPANDED) == TVIS_EXPANDED, "expected expanded\n");

    /* collapse */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    ret = SendMessageA(hTree, TVM_EXPAND, TVE_COLLAPSE, (LPARAM)hRoot);
    expect(TRUE, ret);
    item.state = TVIS_EXPANDED;
    ret = SendMessageA(hTree, TVM_GETITEMA, 0, (LPARAM)&item);
    expect(TRUE, ret);
    ok((item.state & TVIS_EXPANDED) == 0, "expected collapsed\n");
    /* all further collapse/expand attempts won't produce any notifications,
       the only way is to reset with all children removed */
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq, "collapse after expand notifications", FALSE);

    /* try to toggle child that doesn't have children itself */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    ret = SendMessageA(hTree, TVM_EXPAND, TVE_TOGGLE, (LPARAM)hChild);
    expect(FALSE, ret);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq, "toggle node without children", FALSE);

    DestroyWindow(hTree);

    /* test TVM_GETITEMRECT inside TVN_ITEMEXPANDED notification */
    hTree = create_treeview_control(0);
    fill_tree(hTree);
    g_get_rect_in_expand = TRUE;
    ret = SendMessageA(hTree, TVM_SELECTITEM, TVGN_CARET, (LPARAM)hChild);
    expect(TRUE, ret);
    g_get_rect_in_expand = FALSE;

    DestroyWindow(hTree);

    /* TVE_TOGGLE acts as any other TVM_EXPAND */
    hTree = create_treeview_control(0);
    fill_tree(hTree);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    ret = SendMessageA(hTree, TVM_EXPAND, TVE_TOGGLE, (LPARAM)hRoot);
    expect(TRUE, ret);
    ok_sequence(sequences, PARENT_SEQ_INDEX, parent_expand_seq, "toggle node (expand)", FALSE);

    /* toggle again - no notifications */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    ret = SendMessageA(hTree, TVM_EXPAND, TVE_TOGGLE, (LPARAM)hRoot);
    expect(TRUE, ret);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq, "toggle node (collapse)", FALSE);

    DestroyWindow(hTree);

    /* some keyboard events are also translated to expand */
    hTree = create_treeview_control(0);
    fill_tree(hTree);

    /* preselect root node here */
    ret = SendMessageA(hTree, TVM_SELECTITEM, TVGN_CARET, (LPARAM)hRoot);
    expect(TRUE, ret);

    g_get_from_expand = TRUE;
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    ret = SendMessageA(hTree, WM_KEYDOWN, VK_ADD, 0);
    expect(FALSE, ret);
    ok_sequence(sequences, PARENT_SEQ_INDEX, parent_expand_kb_seq, "expand node", FALSE);
    ok(g_item_expanding.state == TVIS_SELECTED, "got state on TVN_ITEMEXPANDING 0x%08x\n",
       g_item_expanding.state);
    ok(g_item_expanded.state == (TVIS_SELECTED|TVIS_EXPANDED), "got state on TVN_ITEMEXPANDED 0x%08x\n",
       g_item_expanded.state);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    ret = SendMessageA(hTree, WM_KEYDOWN, VK_ADD, 0);
    expect(FALSE, ret);
    ok_sequence(sequences, PARENT_SEQ_INDEX, parent_expand_kb_seq, "expand node again", FALSE);
    ok(g_item_expanding.state == (TVIS_SELECTED|TVIS_EXPANDED|TVIS_EXPANDEDONCE), "got state on TVN_ITEMEXPANDING 0x%08x\n",
       g_item_expanding.state);
    ok(g_item_expanded.state == (TVIS_SELECTED|TVIS_EXPANDED|TVIS_EXPANDEDONCE), "got state on TVN_ITEMEXPANDED 0x%08x\n",
       g_item_expanded.state);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    ret = SendMessageA(hTree, WM_KEYDOWN, VK_SUBTRACT, 0);
    expect(FALSE, ret);
    ok_sequence(sequences, PARENT_SEQ_INDEX, parent_expand_kb_seq, "collapse node", FALSE);
    ok(g_item_expanding.state == (TVIS_SELECTED|TVIS_EXPANDED|TVIS_EXPANDEDONCE), "got state on TVN_ITEMEXPANDING 0x%08x\n",
       g_item_expanding.state);
    ok(g_item_expanded.state == (TVIS_SELECTED|TVIS_EXPANDEDONCE), "got state on TVN_ITEMEXPANDED 0x%08x\n",
       g_item_expanded.state);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    ret = SendMessageA(hTree, WM_KEYDOWN, VK_SUBTRACT, 0);
    expect(FALSE, ret);
    ok_sequence(sequences, PARENT_SEQ_INDEX, parent_collapse_2nd_kb_seq, "collapse node again", FALSE);
    ok(g_item_expanding.state == (TVIS_SELECTED|TVIS_EXPANDEDONCE), "got state on TVN_ITEMEXPANDING 0x%08x\n",
       g_item_expanding.state);
    g_get_from_expand = FALSE;

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    ret = SendMessageA(hTree, WM_KEYDOWN, VK_ADD, 0);
    expect(FALSE, ret);
    ok_sequence(sequences, PARENT_SEQ_INDEX, parent_expand_kb_seq, "expand node", FALSE);

    /* go to child */
    ret = SendMessageA(hTree, WM_KEYDOWN, VK_RIGHT, 0);
    expect(FALSE, ret);

    /* try to expand child that doesn't have children itself */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    ret = SendMessageA(hTree, WM_KEYDOWN, VK_ADD, 0);
    expect(FALSE, ret);
    ok_sequence(sequences, PARENT_SEQ_INDEX, parent_expand_empty_kb_seq, "expand node with no children", FALSE);

    /* stay on current selection and set non-zero children count */
    hitem = (HTREEITEM)SendMessageA(hTree, TVM_GETNEXTITEM, TVGN_CARET, 0);
    ok(hitem != NULL, "got %p\n", hitem);

    item.hItem = hitem;
    item.mask = TVIF_CHILDREN;
    item.cChildren = 0x80000000;

    ret = SendMessageA(hTree, TVM_SETITEMA, 0, (LPARAM)&item);
    expect(TRUE, ret);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    ret = SendMessageA(hTree, WM_KEYDOWN, VK_ADD, 0);
    expect(FALSE, ret);
    ok_sequence(sequences, PARENT_SEQ_INDEX, parent_collapse_2nd_kb_seq, "expand node with children", FALSE);

    DestroyWindow(hTree);
}

static void test_expandedimage(void)
{
    TVITEMEXA item;
    HWND hTree;
    BOOL ret;

    hTree = create_treeview_control(0);
    fill_tree(hTree);

    item.mask = TVIF_EXPANDEDIMAGE;
    item.iExpandedImage = 1;
    item.hItem = hRoot;
    ret = SendMessageA(hTree, TVM_SETITEMA, 0, (LPARAM)&item);
    ok(ret, "got %d\n", ret);

    item.mask = TVIF_EXPANDEDIMAGE;
    item.iExpandedImage = -1;
    item.hItem = hRoot;
    ret = SendMessageA(hTree, TVM_GETITEMA, 0, (LPARAM)&item);
    ok(ret, "got %d\n", ret);

    if (item.iExpandedImage != 1)
    {
        win_skip("TVIF_EXPANDEDIMAGE not supported\n");
        DestroyWindow(hTree);
        return;
    }

    /* test for default iExpandedImage value */
    item.mask = TVIF_EXPANDEDIMAGE;
    item.iExpandedImage = -1;
    item.hItem = hChild;
    ret = SendMessageA(hTree, TVM_GETITEMA, 0, (LPARAM)&item);
    ok(ret, "got %d\n", ret);
    ok(item.iExpandedImage == (WORD)I_IMAGENONE, "got %d\n", item.iExpandedImage);

    DestroyWindow(hTree);
}

static void test_TVS_SINGLEEXPAND(void)
{
    HWND hTree;
    HTREEITEM alpha, bravo, charlie, delta, echo, foxtrot, golf, hotel, india, juliet;
    TVINSERTSTRUCTA ins;
    char foo[] = "foo";
    char context[32];
    int i;
    BOOL ret;

    /* build a fairly complex tree
     * - TVI_ROOT
     *   - alpha
     *     - bravo
     *       - charlie
     *     - delta
     *       - echo
     *   - foxtrot
     *     - golf
     *       - hotel
     *       - india
     *     - juliet
     */
    struct
    {
        HTREEITEM *handle;
        HTREEITEM *parent;
        UINT final_state;
    }
    items[] =
    {
        { &alpha,    NULL,      TVIS_EXPANDEDONCE               },
        { &bravo,    &alpha,    TVIS_EXPANDEDONCE               },
        { &charlie,  &bravo,    0                               },
        { &delta,    &alpha,    TVIS_EXPANDEDONCE               },
        { &echo,     &delta,    0                               },
        { &foxtrot,  NULL,      TVIS_EXPANDEDONCE|TVIS_EXPANDED },
        { &golf,     &foxtrot,  TVIS_EXPANDEDONCE|TVIS_EXPANDED },
        { &hotel,    &golf,     0                               },
        { &india,    &golf,     TVIS_SELECTED                   },
        { &juliet,   &foxtrot,  0                               }
    };

    struct
    {
        HTREEITEM *select;
        const struct message *sequence;
    }
    sequence_tests[] =
    {
        { &alpha,    parent_singleexpand_seq0 },
        { &bravo,    parent_singleexpand_seq1 },
        { &delta,    parent_singleexpand_seq2 },
        { &foxtrot,  parent_singleexpand_seq3 },
        { &alpha,    parent_singleexpand_seq4 },
        { &golf,     parent_singleexpand_seq5 },
        { &hotel,    parent_singleexpand_seq6 },
        { &india,    parent_singleexpand_seq7 },
        { &india,    empty_seq }
    };

    hTree = create_treeview_control(0);
    SetWindowLongA(hTree, GWL_STYLE, GetWindowLongA(hTree, GWL_STYLE) | TVS_SINGLEEXPAND);
    /* to avoid painting related notifications */
    ShowWindow(hTree, SW_HIDE);
    for (i = 0; i < ARRAY_SIZE(items); i++)
    {
        ins.hParent = items[i].parent ? *items[i].parent : TVI_ROOT;
        ins.hInsertAfter = TVI_FIRST;
        U(ins).item.mask = TVIF_TEXT;
        U(ins).item.pszText = foo;
        *items[i].handle = TreeView_InsertItemA(hTree, &ins);
    }

    for (i = 0; i < ARRAY_SIZE(sequence_tests); i++)
    {
        flush_sequences(sequences, NUM_MSG_SEQUENCES);
        ret = SendMessageA(hTree, TVM_SELECTITEM, TVGN_CARET, (LPARAM)(*sequence_tests[i].select));
        ok(ret, "got %d\n", ret);
        sprintf(context, "singleexpand notifications %d", i);
        ok_sequence(sequences, PARENT_SEQ_INDEX, sequence_tests[i].sequence, context, FALSE);
    }

    for (i = 0; i < ARRAY_SIZE(items); i++)
    {
        ret = SendMessageA(hTree, TVM_GETITEMSTATE, (WPARAM)(*items[i].handle), 0xFFFF);
        ok(ret == items[i].final_state, "singleexpand items[%d]: expected state 0x%x got 0x%x\n",
           i, items[i].final_state, ret);
    }

    /* a workaround for NT4 that sends expand notifications when nothing is about to expand */
    ret = SendMessageA(hTree, TVM_DELETEITEM, 0, (LPARAM)TVI_ROOT);
    ok(ret, "got %d\n", ret);
    fill_tree(hTree);
    ret = SendMessageA(hTree, TVM_SELECTITEM, TVGN_CARET, 0);
    ok(ret, "got %d\n", ret);

    DestroyWindow(hTree);
}

static void test_WM_PAINT(void)
{
    HWND hTree;
    COLORREF clr;
    LONG ret;
    RECT rc;
    HDC hdc;

    hTree = create_treeview_control(0);

    clr = SendMessageA(hTree, TVM_SETBKCOLOR, 0, RGB(255, 0, 0));
    ok(clr == ~0u, "got %d, expected -1\n", clr);

    hdc = GetDC(hMainWnd);

    GetClientRect(hMainWnd, &rc);
    FillRect(hdc, &rc, GetStockObject(BLACK_BRUSH));

    clr = GetPixel(hdc, 1, 1);
    ok(clr == RGB(0, 0, 0), "got 0x%x\n", clr);

    ret = SendMessageA(hTree, WM_PAINT, (WPARAM)hdc, 0);
    ok(ret == 0, "got %d\n", ret);

    clr = GetPixel(hdc, 1, 1);
    ok(clr == RGB(255, 0, 0) || broken(clr == RGB(0, 0, 0)) /* win98 */,
        "got 0x%x\n", clr);

    ReleaseDC(hMainWnd, hdc);

    DestroyWindow(hTree);
}

static void test_delete_items(void)
{
    const struct message *msg;
    HWND hTree;
    HTREEITEM hItem1, hItem2;
    TVINSERTSTRUCTA ins;
    INT ret;

    static CHAR item1[] = "Item 1";
    static CHAR item2[] = "Item 2";

    hTree = create_treeview_control(0);
    fill_tree(hTree);

    /* check delete order */
    flush_sequences(item_sequence, 1);
    ret = SendMessageA(hTree, TVM_DELETEITEM, 0, 0);
    ok(ret == TRUE, "got %d\n", ret);

    msg = item_sequence[0]->sequence;
    ok(item_sequence[0]->count == 2, "expected 2 items, got %d\n", item_sequence[0]->count);

    if (item_sequence[0]->count == 2)
    {
      ok(msg[0].lParam == (LPARAM)hChild, "expected %p, got 0x%lx\n", hChild, msg[0].lParam);
      ok(msg[1].lParam == (LPARAM)hRoot, "expected %p, got 0x%lx\n", hRoot, msg[1].lParam);
    }

    ret = SendMessageA(hTree, TVM_GETCOUNT, 0, 0);
    ok(ret == 0, "got %d\n", ret);

    DestroyWindow(hTree);

    /* Regression test for a crash when deleting the first visible item while bRedraw == false. */
    hTree = create_treeview_control(0);

    ret = SendMessageA(hTree, WM_SETREDRAW, FALSE, 0);
    ok(ret == 0, "got %d\n", ret);

    ins.hParent = TVI_ROOT;
    ins.hInsertAfter = TVI_ROOT;
    U(ins).item.mask = TVIF_TEXT;
    U(ins).item.pszText = item1;
    hItem1 = TreeView_InsertItemA(hTree, &ins);
    ok(hItem1 != NULL, "InsertItem failed\n");

    ins.hParent = TVI_ROOT;
    ins.hInsertAfter = hItem1;
    U(ins).item.mask = TVIF_TEXT;
    U(ins).item.pszText = item2;
    hItem2 = TreeView_InsertItemA(hTree, &ins);
    ok(hItem2 != NULL, "InsertItem failed\n");

    ret = SendMessageA(hTree, TVM_DELETEITEM, 0, (LPARAM)hItem1);
    ok(ret == TRUE, "got %d\n", ret);

    ret = SendMessageA(hTree, WM_SETREDRAW, TRUE, 0);
    ok(ret == 0, "got %d\n", ret);

    DestroyWindow(hTree);
}

static void test_cchildren(void)
{
    HWND hTree;
    INT ret;
    TVITEMA item;

    hTree = create_treeview_control(0);
    fill_tree(hTree);

    ret = SendMessageA(hTree, TVM_DELETEITEM, 0, (LPARAM)hChild);
    expect(TRUE, ret);

    /* check cChildren - automatic mode */
    item.hItem = hRoot;
    item.mask = TVIF_CHILDREN;

    ret = SendMessageA(hTree, TVM_GETITEMA, 0, (LPARAM)&item);
    expect(TRUE, ret);
    expect(0, item.cChildren);

    DestroyWindow(hTree);

    /* start over */
    hTree = create_treeview_control(0);
    fill_tree(hTree);

    /* turn off automatic mode by setting cChildren explicitly */
    item.hItem = hRoot;
    item.mask = TVIF_CHILDREN;

    ret = SendMessageA(hTree, TVM_GETITEMA, 0, (LPARAM)&item);
    expect(TRUE, ret);
    expect(1, item.cChildren);

    ret = SendMessageA(hTree, TVM_SETITEMA, 0, (LPARAM)&item);
    expect(TRUE, ret);

    ret = SendMessageA(hTree, TVM_DELETEITEM, 0, (LPARAM)hChild);
    expect(TRUE, ret);

    /* check cChildren */
    ret = SendMessageA(hTree, TVM_GETITEMA, 0, (LPARAM)&item);
    expect(TRUE, ret);
todo_wine
    expect(1, item.cChildren);

    DestroyWindow(hTree);
}

struct _ITEM_DATA
{
    HTREEITEM  parent; /* for root value of parent field is unidetified */
    HTREEITEM  nextsibling;
    HTREEITEM  firstchild;
    void      *unk[2];
    DWORD      unk2;
    WORD       pad;
    WORD       width;
};

struct _ITEM_DATA_V6
{
    HTREEITEM  parent; /* for root value of parent field is unidetified */
    HTREEITEM  nextsibling;
    HTREEITEM  firstchild;
    void      *unk[3];
    DWORD      unk2[2];
    WORD       pad;
    WORD       width;
};

static void _check_item(HWND hwnd, HTREEITEM item, BOOL is_version_6, int line)
{
    struct _ITEM_DATA *data = (struct _ITEM_DATA *)item;
    HTREEITEM parent, nextsibling, firstchild, root;
    RECT rect;
    BOOL ret;

    root = (HTREEITEM)SendMessageA(hwnd, TVM_GETNEXTITEM, TVGN_ROOT, (LPARAM)item);
    parent = (HTREEITEM)SendMessageA(hwnd, TVM_GETNEXTITEM, TVGN_PARENT, (LPARAM)item);
    nextsibling = (HTREEITEM)SendMessageA(hwnd, TVM_GETNEXTITEM, TVGN_NEXT, (LPARAM)item);
    firstchild = (HTREEITEM)SendMessageA(hwnd, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM)item);

    *(HTREEITEM*)&rect = item;
    ret = SendMessageA(hwnd, TVM_GETITEMRECT, TRUE, (LPARAM)&rect);

    ok_(__FILE__, line)(item == root ? data->parent != NULL : data->parent == parent,
            "Unexpected parent item %p, got %p, %p\n", parent, data->parent, hwnd);
    ok_(__FILE__, line)(data->nextsibling == nextsibling, "Unexpected sibling %p, got %p\n",
            nextsibling, data->nextsibling);
    ok_(__FILE__, line)(data->firstchild == firstchild, "Unexpected first child %p, got %p\n",
            firstchild, data->firstchild);
    if (ret)
    {
        WORD width;

        if (is_version_6)
        {
            struct _ITEM_DATA_V6 *data_v6 = (struct _ITEM_DATA_V6 *)item;
            width = data_v6->width;
        }
        else
            width = data->width;
    todo_wine
        ok_(__FILE__, line)(width == (rect.right - rect.left) || broken(is_version_6 && width == 0) /* XP */,
                "Width %d, rect width %d.\n", width, rect.right - rect.left);
    }
}

#define CHECK_ITEM(a, b) _check_item(a, b, is_version_6, __LINE__)

static void test_htreeitem_layout(BOOL is_version_6)
{
    TVINSERTSTRUCTA ins;
    HTREEITEM item1, item2;
    HWND hTree;

    hTree = create_treeview_control(0);
    fill_tree(hTree);

    /* root has some special pointer in parent field */
    CHECK_ITEM(hTree, hRoot);
    CHECK_ITEM(hTree, hChild);

    ins.hParent = hChild;
    ins.hInsertAfter = TVI_FIRST;
    U(ins).item.mask = 0;
    item1 = TreeView_InsertItemA(hTree, &ins);

    CHECK_ITEM(hTree, item1);

    ins.hParent = hRoot;
    ins.hInsertAfter = TVI_FIRST;
    U(ins).item.mask = 0;
    item2 = TreeView_InsertItemA(hTree, &ins);

    CHECK_ITEM(hTree, item2);

    SendMessageA(hTree, TVM_DELETEITEM, 0, (LPARAM)hChild);

    /* without children now */
    CHECK_ITEM(hTree, hRoot);

    DestroyWindow(hTree);
}

static void test_TVS_CHECKBOXES(void)
{
    HIMAGELIST himl, himl2;
    HWND hTree, hTree2;
    TVITEMA item;
    DWORD ret;
    MSG msg;

    hTree = create_treeview_control(0);
    fill_tree(hTree);

    himl = (HIMAGELIST)SendMessageA(hTree, TVM_GETIMAGELIST, TVSIL_STATE, 0);
    ok(himl == NULL, "got %p\n", himl);

    item.hItem = hRoot;
    item.mask = TVIF_STATE;
    item.state = INDEXTOSTATEIMAGEMASK(1);
    item.stateMask = TVIS_STATEIMAGEMASK;
    ret = SendMessageA(hTree, TVM_GETITEMA, 0, (LPARAM)&item);
    expect(TRUE, ret);
    ok(item.state == 0, "got 0x%x\n", item.state);

    /* set some index for a child */
    item.hItem = hChild;
    item.mask = TVIF_STATE;
    item.state = INDEXTOSTATEIMAGEMASK(4);
    item.stateMask = TVIS_STATEIMAGEMASK;
    ret = SendMessageA(hTree, TVM_SETITEMA, 0, (LPARAM)&item);
    expect(TRUE, ret);

    /* enabling check boxes set all items to 1 state image index */
    SetWindowLongA(hTree, GWL_STYLE, GetWindowLongA(hTree, GWL_STYLE) | TVS_CHECKBOXES);
    himl = (HIMAGELIST)SendMessageA(hTree, TVM_GETIMAGELIST, TVSIL_STATE, 0);
    ok(himl != NULL, "got %p\n", himl);

    himl2 = (HIMAGELIST)SendMessageA(hTree, TVM_GETIMAGELIST, TVSIL_STATE, 0);
    ok(himl2 != NULL, "got %p\n", himl2);
    ok(himl2 == himl, "got %p, expected %p\n", himl2, himl);

    item.hItem = hRoot;
    item.mask = TVIF_STATE;
    item.state = 0;
    item.stateMask = TVIS_STATEIMAGEMASK;
    ret = SendMessageA(hTree, TVM_GETITEMA, 0, (LPARAM)&item);
    expect(TRUE, ret);
    ok(item.state == INDEXTOSTATEIMAGEMASK(1), "got 0x%x\n", item.state);

    item.hItem = hChild;
    item.mask = TVIF_STATE;
    item.state = 0;
    item.stateMask = TVIS_STATEIMAGEMASK;
    ret = SendMessageA(hTree, TVM_GETITEMA, 0, (LPARAM)&item);
    expect(TRUE, ret);
    ok(item.state == INDEXTOSTATEIMAGEMASK(1), "got 0x%x\n", item.state);

    /* create another control and check its checkbox list */
    hTree2 = create_treeview_control(0);
    fill_tree(hTree2);

    /* set some index for a child */
    item.hItem = hChild;
    item.mask = TVIF_STATE;
    item.state = INDEXTOSTATEIMAGEMASK(4);
    item.stateMask = TVIS_STATEIMAGEMASK;
    ret = SendMessageA(hTree2, TVM_SETITEMA, 0, (LPARAM)&item);
    expect(TRUE, ret);

    /* enabling check boxes set all items to 1 state image index */
    SetWindowLongA(hTree2, GWL_STYLE, GetWindowLongA(hTree, GWL_STYLE) | TVS_CHECKBOXES);
    himl2 = (HIMAGELIST)SendMessageA(hTree2, TVM_GETIMAGELIST, TVSIL_STATE, 0);
    ok(himl2 != NULL, "got %p\n", himl2);
    ok(himl != himl2, "got %p, expected %p\n", himl2, himl);

    DestroyWindow(hTree2);
    DestroyWindow(hTree);

    /* the same, but initially created with TVS_CHECKBOXES */
    hTree = create_treeview_control(TVS_CHECKBOXES);
    fill_tree(hTree);
    himl = (HIMAGELIST)SendMessageA(hTree, TVM_GETIMAGELIST, TVSIL_STATE, 0);
    ok(himl == NULL, "got %p\n", himl);

    item.hItem = hRoot;
    item.mask = TVIF_STATE;
    item.state = 0;
    item.stateMask = TVIS_STATEIMAGEMASK;
    ret = SendMessageA(hTree, TVM_GETITEMA, 0, (LPARAM)&item);
    expect(TRUE, ret);
    ok(item.state == INDEXTOSTATEIMAGEMASK(1), "got 0x%x\n", item.state);

    item.hItem = hChild;
    item.mask = TVIF_STATE;
    item.state = 0;
    item.stateMask = TVIS_STATEIMAGEMASK;
    ret = SendMessageA(hTree, TVM_GETITEMA, 0, (LPARAM)&item);
    expect(TRUE, ret);
    ok(item.state == INDEXTOSTATEIMAGEMASK(1), "got 0x%x\n", item.state);

    item.hItem = hChild;
    item.mask = TVIF_STATE;
    item.state = INDEXTOSTATEIMAGEMASK(2);
    item.stateMask = TVIS_STATEIMAGEMASK;
    ret = SendMessageA(hTree, TVM_SETITEMA, 0, (LPARAM)&item);
    expect(TRUE, ret);

    item.hItem = hChild;
    item.mask = TVIF_STATE;
    item.state = 0;
    ret = SendMessageA(hTree, TVM_GETITEMA, 0, (LPARAM)&item);
    expect(TRUE, ret);
    ok(item.state == INDEXTOSTATEIMAGEMASK(2), "got 0x%x\n", item.state);

    while(GetMessageA(&msg, 0, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);

        if((msg.hwnd == hTree) && (msg.message == WM_PAINT))
            break;
    }

    item.hItem = hChild;
    item.mask = TVIF_STATE;
    item.state = 0;
    ret = SendMessageA(hTree, TVM_GETITEMA, 0, (LPARAM)&item);
    expect(TRUE, ret);
    ok(item.state == INDEXTOSTATEIMAGEMASK(1), "got 0x%x\n", item.state);

    himl = (HIMAGELIST)SendMessageA(hTree, TVM_GETIMAGELIST, TVSIL_STATE, 0);
    ok(himl != NULL, "got %p\n", himl);

    DestroyWindow(hTree);

    /* check what happens if TVSIL_STATE image list is removed */
    hTree = create_treeview_control(0);
    fill_tree(hTree);
    himl = (HIMAGELIST)SendMessageA(hTree, TVM_GETIMAGELIST, TVSIL_STATE, 0);
    ok(himl == NULL, "got %p\n", himl);

    SetWindowLongA(hTree, GWL_STYLE, GetWindowLongA(hTree, GWL_STYLE) | TVS_CHECKBOXES);
    himl = (HIMAGELIST)SendMessageA(hTree, TVM_GETIMAGELIST, TVSIL_STATE, 0);
    ok(himl != NULL, "got %p\n", himl);

    himl2 = (HIMAGELIST)SendMessageA(hTree, TVM_SETIMAGELIST, TVSIL_STATE, 0);
    ok(himl2 == himl, "got %p\n", himl2);

    himl2 = (HIMAGELIST)SendMessageA(hTree, TVM_GETIMAGELIST, TVSIL_STATE, 0);
    ok(himl2 == NULL, "got %p\n", himl2);

    item.hItem = hChild;
    item.mask = TVIF_STATE;
    item.state = INDEXTOSTATEIMAGEMASK(2);
    item.stateMask = TVIS_STATEIMAGEMASK;
    ret = SendMessageA(hTree, TVM_SETITEMA, 0, (LPARAM)&item);
    expect(TRUE, ret);

    item.hItem = hChild;
    item.mask = TVIF_STATE;
    item.state = 0;
    ret = SendMessageA(hTree, TVM_GETITEMA, 0, (LPARAM)&item);
    expect(TRUE, ret);
    ok(item.state == INDEXTOSTATEIMAGEMASK(2), "got 0x%x\n", item.state);

    while(GetMessageA(&msg, 0, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);

        if((msg.hwnd == hTree) && (msg.message == WM_PAINT))
            break;
    }

    item.hItem = hChild;
    item.mask = TVIF_STATE;
    item.state = 0;
    ret = SendMessageA(hTree, TVM_GETITEMA, 0, (LPARAM)&item);
    expect(TRUE, ret);
    ok(item.state == INDEXTOSTATEIMAGEMASK(1), "got 0x%x\n", item.state);

    himl = (HIMAGELIST)SendMessageA(hTree, TVM_GETIMAGELIST, TVSIL_STATE, 0);
    ok(himl != NULL, "got %p\n", himl);

    DestroyWindow(hTree);
}

static void test_TVM_GETNEXTITEM(void)
{
    HTREEITEM item;
    HWND hTree;

    hTree = create_treeview_control(0);
    fill_tree(hTree);

    item = (HTREEITEM)SendMessageA(hTree, TVM_GETNEXTITEM, TVGN_ROOT, 0);
    ok(item == hRoot, "got %p, expected %p\n", item, hRoot);

    item = (HTREEITEM)SendMessageA(hTree, TVM_GETNEXTITEM, TVGN_ROOT, (LPARAM)TVI_ROOT);
    ok(item == hRoot, "got %p, expected %p\n", item, hRoot);

    item = (HTREEITEM)SendMessageA(hTree, TVM_GETNEXTITEM, TVGN_ROOT, (LPARAM)hRoot);
    ok(item == hRoot, "got %p, expected %p\n", item, hRoot);

    item = (HTREEITEM)SendMessageA(hTree, TVM_GETNEXTITEM, TVGN_ROOT, (LPARAM)hChild);
    ok(item == hRoot, "got %p, expected %p\n", item, hRoot);

    item = (HTREEITEM)SendMessageA(hTree, TVM_GETNEXTITEM, TVGN_CHILD, 0);
    ok(item == hRoot, "got %p, expected %p\n", item, hRoot);

    item = (HTREEITEM)SendMessageA(hTree, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM)hRoot);
    ok(item == hChild, "got %p, expected %p\n", item, hChild);

    item = (HTREEITEM)SendMessageA(hTree, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM)TVI_ROOT);
    ok(item == hRoot, "got %p, expected %p\n", item, hRoot);

    item = (HTREEITEM)SendMessageA(hTree, TVM_GETNEXTITEM, TVGN_PARENT, 0);
    ok(item == NULL, "got %p\n", item);

    item = (HTREEITEM)SendMessageA(hTree, TVM_GETNEXTITEM, TVGN_PARENT, (LPARAM)hChild);
    ok(item == hRoot, "got %p, expected %p\n", item, hRoot);

    DestroyWindow(hTree);
}

static void test_TVM_HITTEST(void)
{
    HWND hTree;
    LRESULT ret;
    RECT rc;
    TVHITTESTINFO ht;

    hTree = create_treeview_control(0);
    fill_tree(hTree);

    *(HTREEITEM*)&rc = hRoot;
    ret = SendMessageA(hTree, TVM_GETITEMRECT, TRUE, (LPARAM)&rc);
    expect(TRUE, (BOOL)ret);

    ht.pt.x = rc.left-1;
    ht.pt.y = rc.top;

    ret = SendMessageA(hTree, TVM_HITTEST, 0, (LPARAM)&ht);
    ok((HTREEITEM)ret == hRoot, "got %p, expected %p\n", (HTREEITEM)ret, hRoot);
    ok(ht.hItem == hRoot, "got %p, expected %p\n", ht.hItem, hRoot);
    ok(ht.flags == TVHT_ONITEMBUTTON, "got %d, expected %d\n", ht.flags, TVHT_ONITEMBUTTON);

    ret = SendMessageA(hTree, TVM_EXPAND, TVE_EXPAND, (LPARAM)hRoot);
    expect(TRUE, (BOOL)ret);

    *(HTREEITEM*)&rc = hChild;
    ret = SendMessageA(hTree, TVM_GETITEMRECT, TRUE, (LPARAM)&rc);
    expect(TRUE, (BOOL)ret);

    ht.pt.x = rc.left-1;
    ht.pt.y = rc.top;

    ret = SendMessageA(hTree, TVM_HITTEST, 0, (LPARAM)&ht);
    ok((HTREEITEM)ret == hChild, "got %p, expected %p\n", (HTREEITEM)ret, hChild);
    ok(ht.hItem == hChild, "got %p, expected %p\n", ht.hItem, hChild);
    /* Wine returns item button here, but this item has no button */
    todo_wine ok(ht.flags == TVHT_ONITEMINDENT, "got %d, expected %d\n", ht.flags, TVHT_ONITEMINDENT);

    DestroyWindow(hTree);
}

static void test_WM_GETDLGCODE(void)
{
    DWORD code;
    HWND hTree;

    hTree = create_treeview_control(0);

    code = SendMessageA(hTree, WM_GETDLGCODE, VK_TAB, 0);
    ok(code == (DLGC_WANTCHARS | DLGC_WANTARROWS), "0x%08x\n", code);

    DestroyWindow(hTree);
}

static void test_customdraw(void)
{
    LOGFONTA lf;
    HWND hwnd;

    hwnd = create_treeview_control(0);
    fill_tree(hwnd);
    SendMessageA(hwnd, TVM_EXPAND, TVE_EXPAND, (WPARAM)hRoot);

    /* create additional font, custom draw handler will select it */
    SystemParametersInfoA(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, 0);
    lf.lfHeight *= 2;
    g_customdraw_font = CreateFontIndirectA(&lf);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);
    ok_sequence(sequences, PARENT_CD_SEQ_INDEX, parent_cd_seq, "custom draw notifications", FALSE);
    DeleteObject(g_customdraw_font);
    g_customdraw_font = NULL;

    DestroyWindow(hwnd);
}

static void test_WM_KEYDOWN(void)
{
    static const char *rootA = "root";
    TVINSERTSTRUCTA ins;
    HTREEITEM hRoot;
    HWND hwnd;

    hwnd = create_treeview_control(0);

    ins.hParent = TVI_ROOT;
    ins.hInsertAfter = TVI_ROOT;
    U(ins).item.mask = TVIF_TEXT;
    U(ins).item.pszText = (char*)rootA;
    hRoot = TreeView_InsertItemA(hwnd, &ins);
    ok(hRoot != NULL, "got %p\n", hRoot);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    SendMessageA(hwnd, WM_KEYDOWN, VK_RETURN, 0);
    ok_sequence(sequences, PARENT_SEQ_INDEX, parent_vk_return_seq, "WM_KEYDOWN/VK_RETURN parent notification", TRUE);

    DestroyWindow(hwnd);
}

static void test_TVS_FULLROWSELECT(void)
{
    DWORD style;
    HWND hwnd;

    /* try to create both with TVS_HASLINES and TVS_FULLROWSELECT */
    hwnd = create_treeview_control(TVS_FULLROWSELECT);

    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok((style & (TVS_FULLROWSELECT | TVS_HASLINES)) == (TVS_FULLROWSELECT | TVS_HASLINES), "got style 0x%08x\n", style);

    DestroyWindow(hwnd);

    /* create just with TVS_HASLINES, try to enable TVS_FULLROWSELECT later */
    hwnd = create_treeview_control(0);

    style = GetWindowLongA(hwnd, GWL_STYLE);
    SetWindowLongA(hwnd, GWL_STYLE, style | TVS_FULLROWSELECT);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(style & TVS_FULLROWSELECT, "got style 0x%08x\n", style);

    DestroyWindow(hwnd);
}

static void get_item_names_string(HWND hwnd, HTREEITEM item, char *str)
{
    TVITEMA tvitem = { 0 };
    HTREEITEM child;
    char name[16];

    if (!item)
    {
        item = (HTREEITEM)SendMessageA(hwnd, TVM_GETNEXTITEM, TVGN_ROOT, 0);
        str[0] = 0;
    }

    child = (HTREEITEM)SendMessageA(hwnd, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM)item);

    tvitem.mask = TVIF_TEXT;
    tvitem.hItem = item;
    tvitem.pszText = name;
    tvitem.cchTextMax = sizeof(name);
    SendMessageA(hwnd, TVM_GETITEMA, 0, (LPARAM)&tvitem);
    strcat(str, tvitem.pszText);

    while (child != NULL)
    {
        get_item_names_string(hwnd, child, str);
        child = (HTREEITEM)SendMessageA(hwnd, TVM_GETNEXTITEM, TVGN_NEXT, (LPARAM)child);
    }
}

static void fill_treeview_sort_test(HWND hwnd)
{
    static const char *itemnames[] =
    {
        "root", "Wasp", "Caribou", "Vacuum",
        "Ocelot", "Newspaper", "Litter bin"
    };

    HTREEITEM root, children[2];
    TVINSERTSTRUCTA ins;
    unsigned i = 0;

    SendMessageA(hwnd, TVM_DELETEITEM, 0, 0);

    /* root, two children, with two children each */
    ins.hParent = TVI_ROOT;
    ins.hInsertAfter = TVI_ROOT;
    U(ins).item.mask = TVIF_TEXT;
    U(ins).item.pszText = (char *)itemnames[i++];
    root = (HTREEITEM)SendMessageA(hwnd, TVM_INSERTITEMA, 0, (LPARAM)&ins);

    ins.hParent = root;
    ins.hInsertAfter = TVI_LAST;
    U(ins).item.mask = TVIF_TEXT;
    U(ins).item.pszText = (char *)itemnames[i++];
    children[0] = (HTREEITEM)SendMessageA(hwnd, TVM_INSERTITEMA, 0, (LPARAM)&ins);

    U(ins).item.pszText = (char *)itemnames[i++];
    children[1] = (HTREEITEM)SendMessageA(hwnd, TVM_INSERTITEMA, 0, (LPARAM)&ins);

    ins.hParent = children[0];
    U(ins).item.pszText = (char *)itemnames[i++];
    SendMessageA(hwnd, TVM_INSERTITEMA, 0, (LPARAM)&ins);

    U(ins).item.pszText = (char *)itemnames[i++];
    SendMessageA(hwnd, TVM_INSERTITEMA, 0, (LPARAM)&ins);

    ins.hParent = children[1];
    U(ins).item.pszText = (char *)itemnames[i++];
    SendMessageA(hwnd, TVM_INSERTITEMA, 0, (LPARAM)&ins);

    U(ins).item.pszText = (char *)itemnames[i++];
    SendMessageA(hwnd, TVM_INSERTITEMA, 0, (LPARAM)&ins);
}

static void test_TVM_SORTCHILDREN(void)
{
    static const char *initial_order = "rootWaspVacuumOcelotCaribouNewspaperLitter bin";
    static const char *sorted_order = "rootCaribouNewspaperLitter binWaspVacuumOcelot";
    TVINSERTSTRUCTA ins;
    char buff[256];
    HTREEITEM root;
    HWND hwnd;
    BOOL ret;

    hwnd = create_treeview_control(0);

    /* call on empty tree */
    ret = SendMessageA(hwnd, TVM_SORTCHILDREN, 0, 0);
    ok(!ret, "Unexpected ret value %d\n", ret);

    ret = SendMessageA(hwnd, TVM_SORTCHILDREN, 0, (LPARAM)TVI_ROOT);
    ok(!ret, "Unexpected ret value %d\n", ret);

    /* add only root, sort from it */
    ins.hParent = TVI_ROOT;
    ins.hInsertAfter = TVI_ROOT;
    U(ins).item.mask = TVIF_TEXT;
    U(ins).item.pszText = (char *)"root";
    root = (HTREEITEM)SendMessageA(hwnd, TVM_INSERTITEMA, 0, (LPARAM)&ins);
    ok(root != NULL, "Expected root node\n");

    ret = SendMessageA(hwnd, TVM_SORTCHILDREN, 0, (LPARAM)root);
    ok(!ret, "Unexpected ret value %d\n", ret);

    ret = SendMessageA(hwnd, TVM_SORTCHILDREN, TRUE, (LPARAM)root);
    ok(!ret, "Unexpected ret value %d\n", ret);

    /* root, two children, with two children each */
    fill_treeview_sort_test(hwnd);
    get_item_names_string(hwnd, NULL, buff);
    ok(!strcmp(buff, initial_order), "Wrong initial order %s, expected %s\n", buff, initial_order);

    /* with NULL item nothing is sorted */
    fill_treeview_sort_test(hwnd);
    ret = SendMessageA(hwnd, TVM_SORTCHILDREN, 0, 0);
todo_wine
    ok(ret, "Unexpected ret value %d\n", ret);
    get_item_names_string(hwnd, NULL, buff);
    ok(!strcmp(buff, initial_order), "Wrong sorted order %s, expected %s\n", buff, initial_order);

    /* TVI_ROOT as item */
    fill_treeview_sort_test(hwnd);
    ret = SendMessageA(hwnd, TVM_SORTCHILDREN, 0, (LPARAM)TVI_ROOT);
todo_wine
    ok(ret, "Unexpected ret value %d\n", ret);
    get_item_names_string(hwnd, NULL, buff);
    ok(!strcmp(buff, initial_order), "Wrong sorted order %s, expected %s\n", buff, initial_order);

    /* zero WPARAM, item is specified */
    fill_treeview_sort_test(hwnd);
    root = (HTREEITEM)SendMessageA(hwnd, TVM_GETNEXTITEM, TVGN_ROOT, 0);
    ok(root != NULL, "Failed to get root item\n");
    ret = SendMessageA(hwnd, TVM_SORTCHILDREN, 0, (LPARAM)root);
    ok(ret, "Unexpected ret value %d\n", ret);
    get_item_names_string(hwnd, NULL, buff);
    ok(!strcmp(buff, sorted_order), "Wrong sorted order %s, expected %s\n", buff, sorted_order);

    /* non-zero WPARAM, NULL item */
    fill_treeview_sort_test(hwnd);
    ret = SendMessageA(hwnd, TVM_SORTCHILDREN, TRUE, 0);
todo_wine
    ok(ret, "Unexpected ret value %d\n", ret);
    get_item_names_string(hwnd, NULL, buff);
    ok(!strcmp(buff, initial_order), "Wrong sorted order %s, expected %s\n", buff, sorted_order);

    /* TVI_ROOT as item */
    fill_treeview_sort_test(hwnd);
    ret = SendMessageA(hwnd, TVM_SORTCHILDREN, TRUE, (LPARAM)TVI_ROOT);
todo_wine
    ok(ret, "Unexpected ret value %d\n", ret);
    get_item_names_string(hwnd, NULL, buff);
    ok(!strcmp(buff, initial_order), "Wrong sorted order %s, expected %s\n", buff, sorted_order);

    /* non-zero WPARAM, item is specified */
    fill_treeview_sort_test(hwnd);
    root = (HTREEITEM)SendMessageA(hwnd, TVM_GETNEXTITEM, TVGN_ROOT, 0);
    ok(root != NULL, "Failed to get root item\n");
    ret = SendMessageA(hwnd, TVM_SORTCHILDREN, TRUE, (LPARAM)root);
    ok(ret, "Unexpected ret value %d\n", ret);
    get_item_names_string(hwnd, NULL, buff);
    ok(!strcmp(buff, sorted_order), "Wrong sorted order %s, expected %s\n", buff, sorted_order);

    /* case insensitive comparison */
    SendMessageA(hwnd, TVM_DELETEITEM, 0, 0);

    ins.hParent = TVI_ROOT;
    ins.hInsertAfter = TVI_ROOT;
    U(ins).item.mask = TVIF_TEXT;
    U(ins).item.pszText = (char *)"root";
    root = (HTREEITEM)SendMessageA(hwnd, TVM_INSERTITEMA, 0, (LPARAM)&ins);
    ok(root != NULL, "Expected root node\n");

    ins.hParent = root;
    ins.hInsertAfter = TVI_LAST;
    U(ins).item.pszText = (char *)"I1";
    SendMessageA(hwnd, TVM_INSERTITEMA, 0, (LPARAM)&ins);

    ins.hParent = root;
    ins.hInsertAfter = TVI_LAST;
    U(ins).item.pszText = (char *)"i1";
    SendMessageA(hwnd, TVM_INSERTITEMA, 0, (LPARAM)&ins);

    ret = SendMessageA(hwnd, TVM_SORTCHILDREN, TRUE, (LPARAM)root);
    ok(ret, "Unexpected ret value %d\n", ret);
    get_item_names_string(hwnd, NULL, buff);
    ok(!strcmp(buff, "rootI1i1"), "Wrong sorted order %s\n", buff);

    DestroyWindow(hwnd);
}

static void test_right_click(void)
{
    HWND hTree;
    HTREEITEM selected;
    RECT rc;
    LRESULT result;
    POINT pt, orig_pos;

    hTree = create_treeview_control(0);
    fill_tree(hTree);

    SendMessageA(hTree, TVM_ENSUREVISIBLE, 0, (LPARAM)hChild);
    SendMessageA(hTree, TVM_SELECTITEM, TVGN_CARET, (LPARAM)hChild);
    selected = (HTREEITEM)SendMessageA(hTree, TVM_GETNEXTITEM, TVGN_CARET, 0);
    ok(selected == hChild, "child item not selected\n");

    *(HTREEITEM *)&rc = hRoot;
    result = SendMessageA(hTree, TVM_GETITEMRECT, TRUE, (LPARAM)&rc);
    ok(result, "TVM_GETITEMRECT failed\n");

    flush_events();

    pt.x = (rc.left + rc.right) / 2;
    pt.y = (rc.top + rc.bottom) / 2;
    ClientToScreen(hMainWnd, &pt);
    GetCursorPos(&orig_pos);
    SetCursorPos(pt.x, pt.y);

    flush_events();
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    PostMessageA(hTree, WM_RBUTTONDOWN, MK_RBUTTON, MAKELPARAM(pt.x, pt.y));
    PostMessageA(hTree, WM_RBUTTONUP, 0, MAKELPARAM(pt.x, pt.y));

    flush_events();

    ok_sequence(sequences, TREEVIEW_SEQ_INDEX, test_right_click_seq, "right click sequence", FALSE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, parent_right_click_seq, "parent right click sequence", FALSE);

    selected = (HTREEITEM)SendMessageA(hTree, TVM_GETNEXTITEM, TVGN_CARET, 0);
    ok(selected == hChild, "child item should still be selected\n");

    SetCursorPos(orig_pos.x, orig_pos.y);
    DestroyWindow(hTree);
}

static void init_functions(void)
{
    HMODULE hComCtl32 = LoadLibraryA("comctl32.dll");

#define X(f) p##f = (void*)GetProcAddress(hComCtl32, #f);
    X(InitCommonControlsEx);
#undef X
}

START_TEST(treeview)
{
    INITCOMMONCONTROLSEX iccex;
    ULONG_PTR ctx_cookie;
    HANDLE hCtx;
    WNDCLASSA wc;

    init_functions();

    iccex.dwSize = sizeof(iccex);
    iccex.dwICC  = ICC_TREEVIEW_CLASSES;
    pInitCommonControlsEx(&iccex);

    init_msg_sequences(sequences, NUM_MSG_SEQUENCES);
    init_msg_sequences(item_sequence, 1);
  
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.hIcon = NULL;
    wc.hCursor = LoadCursorA(NULL, (LPCSTR)IDC_IBEAM);
    wc.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "MyTestWnd";
    wc.lpfnWndProc = parent_wnd_proc;
    RegisterClassA(&wc);

    hMainWnd = CreateWindowExA(0, "MyTestWnd", "Blah", WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT, 130, 105, NULL, NULL, GetModuleHandleA(NULL), 0);

    ok(hMainWnd != NULL, "Failed to create parent window. Tests aborted.\n");
    if (!hMainWnd) return;

    test_fillroot();
    test_select();
    test_getitemtext();
    test_focus();
    test_get_set_bkcolor();
    test_get_set_imagelist();
    test_get_set_indent();
    test_get_set_insertmark();
    test_get_set_item();
    test_get_set_itemheight();
    test_get_set_scrolltime();
    test_get_set_textcolor();
    test_get_linecolor();
    test_get_insertmarkcolor();
    test_get_set_tooltips();
    test_get_set_unicodeformat();
    test_callback();
    test_expandinvisible();
    test_itemedit();
    test_treeview_classinfo();
    test_expandnotify();
    test_TVS_SINGLEEXPAND();
    test_WM_PAINT();
    test_delete_items();
    test_cchildren();
    test_htreeitem_layout(FALSE);
    test_TVS_CHECKBOXES();
    test_TVM_GETNEXTITEM();
    test_TVM_HITTEST();
    test_WM_GETDLGCODE();
    test_customdraw();
    test_WM_KEYDOWN();
    test_TVS_FULLROWSELECT();
    test_TVM_SORTCHILDREN();
    test_right_click();

    if (!load_v6_module(&ctx_cookie, &hCtx))
    {
        DestroyWindow(hMainWnd);
        return;
    }

    /* comctl32 version 6 tests start here */
    g_v6 = TRUE;

    test_fillroot();
    test_getitemtext();
    test_get_set_insertmark();
    test_get_set_item();
    test_get_set_scrolltime();
    test_get_set_textcolor();
    test_get_linecolor();
    test_get_insertmarkcolor();
    test_expandedimage();
    test_get_set_tooltips();
    test_get_set_unicodeformat();
    test_expandinvisible();
    test_expand();
    test_itemedit();
    test_treeview_classinfo();
    test_delete_items();
    test_cchildren();
    test_htreeitem_layout(TRUE);
    test_TVM_GETNEXTITEM();
    test_TVM_HITTEST();
    test_WM_GETDLGCODE();
    test_customdraw();
    test_WM_KEYDOWN();
    test_TVS_FULLROWSELECT();
    test_TVM_SORTCHILDREN();

    unload_v6_module(ctx_cookie, hCtx);
}
