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

#include <assert.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "winreg.h"
#include "commctrl.h" 

#include "wine/test.h"
#include "msg.h"

const char *TEST_CALLBACK_TEXT = "callback_text";

#define NUM_MSG_SEQUENCES   1
#define TREEVIEW_SEQ_INDEX  0

#define expect(expected, got) ok(got == expected, "Expected %d, got %d\n", expected, got)

static struct msg_sequence *MsgSequences[NUM_MSG_SEQUENCES];

static const struct message FillRootSeq[] = {
    { TVM_INSERTITEM, sent },
    { TVM_INSERTITEM, sent },
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
    { TVM_INSERTITEM, sent },
    { TVM_GETITEM, sent },
    { TVM_DELETEITEM, sent },
    { 0 }
};

static const struct message focus_seq[] = {
    { TVM_INSERTITEM, sent },
    { TVM_INSERTITEM, sent },
    { TVM_SELECTITEM, sent|wparam, 9 },
    /* The following end up out of order in wine */
    { WM_WINDOWPOSCHANGING, sent|defwinproc },
    { WM_NCCALCSIZE, sent|wparam|defwinproc, TRUE },
    { WM_WINDOWPOSCHANGED, sent|defwinproc },
    { WM_SIZE, sent|defwinproc },
    { WM_PAINT, sent|defwinproc },
    { WM_NCPAINT, sent|wparam|defwinproc, 1 },
    { WM_ERASEBKGND, sent|defwinproc },
    { TVM_EDITLABEL, sent },
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
    { TVM_GETITEM, sent },
    { TVM_SETITEM, sent },
    { TVM_GETITEM, sent },
    { TVM_SETITEM, sent },
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
    struct message msg;
    WNDPROC lpOldProc = (WNDPROC)GetWindowLongPtrA(hwnd, GWLP_USERDATA);

    msg.message = message;
    msg.flags = sent|wparam|lparam;
    if (defwndproc_counter) msg.flags |= defwinproc;
    msg.wParam = wParam;
    msg.lParam = lParam;
    add_message(MsgSequences, TREEVIEW_SEQ_INDEX, &msg);

    defwndproc_counter++;
    ret = CallWindowProcA(lpOldProc, hwnd, message, wParam, lParam);
    defwndproc_counter--;

    return ret;
}

static HWND create_treeview_control(void)
{
    WNDPROC pOldWndProc;
    HWND hTree;

    hTree = CreateWindowExA(WS_EX_CLIENTEDGE, WC_TREEVIEWA, NULL, WS_CHILD|WS_VISIBLE|
            TVS_LINESATROOT|TVS_HASLINES|TVS_HASBUTTONS|TVS_EDITLABELS,
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
    hRoot = TreeView_InsertItem(hTree, &ins);

    ins.hParent = hRoot;
    ins.hInsertAfter = TVI_FIRST;
    U(ins).item.mask = TVIF_TEXT;
    U(ins).item.pszText = child;
    hChild = TreeView_InsertItem(hTree, &ins);
}

static void test_fillroot(void)
{
    TVITEM tvi;
    HWND hTree;

    hTree = create_treeview_control();

    flush_sequences(MsgSequences, NUM_MSG_SEQUENCES);

    fill_tree(hTree);

    Clear();
    AddItem('A');
    assert(hRoot);
    AddItem('B');
    assert(hChild);
    AddItem('.');
    ok_sequence(MsgSequences, TREEVIEW_SEQ_INDEX, FillRootSeq, "FillRoot", FALSE);
    ok(!strcmp(sequence, "AB."), "Item creation\n");

    /* UMLPad 1.15 depends on this being not -1 (I_IMAGECALLBACK) */
    tvi.hItem = hRoot;
    tvi.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    SendMessage( hTree, TVM_GETITEM, 0, (LPARAM)&tvi );
    ok(tvi.iImage == 0, "tvi.iImage=%d\n", tvi.iImage);
    ok(tvi.iSelectedImage == 0, "tvi.iSelectedImage=%d\n", tvi.iSelectedImage);

    DestroyWindow(hTree);
}

static void test_callback(void)
{
    HTREEITEM hRoot;
    HTREEITEM hItem1, hItem2;
    TVINSERTSTRUCTA ins;
    TVITEM tvi;
    CHAR test_string[] = "Test_string";
    CHAR buf[128];
    LRESULT ret;
    HWND hTree;

    hTree = create_treeview_control();
    fill_tree(hTree);

    ret = TreeView_DeleteAllItems(hTree);
    ok(ret == TRUE, "ret\n");
    ins.hParent = TVI_ROOT;
    ins.hInsertAfter = TVI_ROOT;
    U(ins).item.mask = TVIF_TEXT;
    U(ins).item.pszText = LPSTR_TEXTCALLBACK;
    hRoot = TreeView_InsertItem(hTree, &ins);
    assert(hRoot);

    tvi.hItem = hRoot;
    tvi.mask = TVIF_TEXT;
    tvi.pszText = buf;
    tvi.cchTextMax = sizeof(buf)/sizeof(buf[0]);
    ret = TreeView_GetItem(hTree, &tvi);
    ok(ret == 1, "ret\n");
    ok(strcmp(tvi.pszText, TEST_CALLBACK_TEXT) == 0, "Callback item text mismatch %s vs %s\n",
        tvi.pszText, TEST_CALLBACK_TEXT);

    ins.hParent = hRoot;
    ins.hInsertAfter = TVI_FIRST;
    U(ins).item.mask = TVIF_TEXT;
    U(ins).item.pszText = test_string;
    hItem1 = TreeView_InsertItem(hTree, &ins);
    assert(hItem1);

    tvi.hItem = hItem1;
    ret = TreeView_GetItem(hTree, &tvi);
    ok(ret == TRUE, "ret\n");
    ok(strcmp(tvi.pszText, test_string) == 0, "Item text mismatch %s vs %s\n",
        tvi.pszText, test_string);

    /* undocumented: pszText of NULL also means LPSTR_CALLBACK: */
    tvi.pszText = NULL;
    ret = TreeView_SetItem(hTree, &tvi);
    ok(ret == 1, "Expected SetItem return 1, got %ld\n", ret);
    tvi.pszText = buf;
    ret = TreeView_GetItem(hTree, &tvi);
    ok(ret == TRUE, "Expected GetItem return TRUE, got %ld\n", ret);
    ok(strcmp(tvi.pszText, TEST_CALLBACK_TEXT) == 0, "Item text mismatch %s vs %s\n",
        tvi.pszText, TEST_CALLBACK_TEXT);

    U(ins).item.pszText = NULL;
    hItem2 = TreeView_InsertItem(hTree, &ins);
    assert(hItem2);
    tvi.hItem = hItem2;
    memset(buf, 0, sizeof(buf));
    ret = TreeView_GetItem(hTree, &tvi);
    ok(ret == TRUE, "Expected GetItem return TRUE, got %ld\n", ret);
    ok(strcmp(tvi.pszText, TEST_CALLBACK_TEXT) == 0, "Item text mismatch %s vs %s\n",
        tvi.pszText, TEST_CALLBACK_TEXT);

    DestroyWindow(hTree);
}

static void test_select(void)
{
    BOOL r;
    HWND hTree;

    hTree = create_treeview_control();
    fill_tree(hTree);

    /* root-none select tests */
    flush_sequences(MsgSequences, NUM_MSG_SEQUENCES);
    r = TreeView_SelectItem(hTree, NULL);
    expect(TRUE, r);
    Clear();
    AddItem('1');
    r = TreeView_SelectItem(hTree, hRoot);
    expect(TRUE, r);
    AddItem('2');
    r = TreeView_SelectItem(hTree, hRoot);
    expect(TRUE, r);
    AddItem('3');
    r = TreeView_SelectItem(hTree, NULL);
    expect(TRUE, r);
    AddItem('4');
    r = TreeView_SelectItem(hTree, NULL);
    expect(TRUE, r);
    AddItem('5');
    r = TreeView_SelectItem(hTree, hRoot);
    expect(TRUE, r);
    AddItem('.');
    ok(!strcmp(sequence, "1(nR)nR23(Rn)Rn45(nR)nR."), "root-none select test\n");
    ok_sequence(MsgSequences, TREEVIEW_SEQ_INDEX, rootnone_select_seq,
                "root-none select seq", FALSE);

    /* root-child select tests */
    flush_sequences(MsgSequences, NUM_MSG_SEQUENCES);
    r = TreeView_SelectItem(hTree, NULL);
    expect(TRUE, r);

    Clear();
    AddItem('1');
    r = TreeView_SelectItem(hTree, hRoot);
    expect(TRUE, r);
    AddItem('2');
    r = TreeView_SelectItem(hTree, hRoot);
    expect(TRUE, r);
    AddItem('3');
    r = TreeView_SelectItem(hTree, hChild);
    expect(TRUE, r);
    AddItem('4');
    r = TreeView_SelectItem(hTree, hChild);
    expect(TRUE, r);
    AddItem('5');
    r = TreeView_SelectItem(hTree, hRoot);
    expect(TRUE, r);
    AddItem('.');
    ok(!strcmp(sequence, "1(nR)nR23(RC)RC45(CR)CR."), "root-child select test\n");
    ok_sequence(MsgSequences, TREEVIEW_SEQ_INDEX, rootchild_select_seq,
                "root-child select seq", FALSE);

    DestroyWindow(hTree);
}

static void test_getitemtext(void)
{
    TVINSERTSTRUCTA ins;
    HTREEITEM hChild;
    TVITEM tvi;
    HWND hTree;

    CHAR szBuffer[80] = "Blah";
    int nBufferSize = sizeof(szBuffer)/sizeof(CHAR);

    hTree = create_treeview_control();
    fill_tree(hTree);

    flush_sequences(MsgSequences, NUM_MSG_SEQUENCES);

    /* add an item without TVIF_TEXT mask and pszText == NULL */
    ins.hParent = hRoot;
    ins.hInsertAfter = TVI_ROOT;
    U(ins).item.mask = 0;
    U(ins).item.pszText = NULL;
    U(ins).item.cchTextMax = 0;
    hChild = TreeView_InsertItem(hTree, &ins);
    assert(hChild);

    /* retrieve it with TVIF_TEXT mask */
    tvi.hItem = hChild;
    tvi.mask = TVIF_TEXT;
    tvi.cchTextMax = nBufferSize;
    tvi.pszText = szBuffer;

    SendMessageA( hTree, TVM_GETITEM, 0, (LPARAM)&tvi );
    ok(!strcmp(szBuffer, ""), "szBuffer=\"%s\", expected \"\"\n", szBuffer);
    ok(SendMessageA(hTree, TVM_DELETEITEM, 0, (LPARAM)hChild), "DeleteItem failed\n");
    ok_sequence(MsgSequences, TREEVIEW_SEQ_INDEX, getitemtext_seq, "get item text seq", FALSE);

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

    hTree = create_treeview_control();
    fill_tree(hTree);

    flush_sequences(MsgSequences, NUM_MSG_SEQUENCES);

    /* This test verifies that when a label is being edited, scrolling
     * the treeview does not cause the label to lose focus. To test
     * this, first some additional entries are added to generate
     * scrollbars.
     */
    ins.hParent = hRoot;
    ins.hInsertAfter = hChild;
    U(ins).item.mask = TVIF_TEXT;
    U(ins).item.pszText = child1;
    hChild1 = TreeView_InsertItem(hTree, &ins);
    assert(hChild1);
    ins.hInsertAfter = hChild1;
    U(ins).item.mask = TVIF_TEXT;
    U(ins).item.pszText = child2;
    hChild2 = TreeView_InsertItem(hTree, &ins);
    assert(hChild2);

    ShowWindow(hMainWnd,SW_SHOW);
    SendMessageA(hTree, TVM_SELECTITEM, TVGN_CARET, (LPARAM)hChild);
    hEdit = TreeView_EditLabel(hTree, hChild);
    ScrollWindowEx(hTree, -10, 0, NULL, NULL, NULL, NULL, SW_SCROLLCHILDREN);
    ok(GetFocus() == hEdit, "Edit control should have focus\n");
    ok_sequence(MsgSequences, TREEVIEW_SEQ_INDEX, focus_seq, "focus test", TRUE);

    DestroyWindow(hTree);
}

static void test_get_set_bkcolor(void)
{
    COLORREF crColor = RGB(0,0,0);
    HWND hTree;

    hTree = create_treeview_control();
    fill_tree(hTree);

    flush_sequences(MsgSequences, NUM_MSG_SEQUENCES);

    /* If the value is -1, the control is using the system color for the background color. */
    crColor = (COLORREF)SendMessage( hTree, TVM_GETBKCOLOR, 0, 0 );
    ok(crColor == -1, "Default background color reported as 0x%.8x\n", crColor);

    /* Test for black background */
    SendMessage( hTree, TVM_SETBKCOLOR, 0, RGB(0,0,0) );
    crColor = (COLORREF)SendMessage( hTree, TVM_GETBKCOLOR, 0, 0 );
    ok(crColor == RGB(0,0,0), "Black background color reported as 0x%.8x\n", crColor);

    /* Test for white background */
    SendMessage( hTree, TVM_SETBKCOLOR, 0, RGB(255,255,255) );
    crColor = (COLORREF)SendMessage( hTree, TVM_GETBKCOLOR, 0, 0 );
    ok(crColor == RGB(255,255,255), "White background color reported as 0x%.8x\n", crColor);

    /* Reset the default background */
    SendMessage( hTree, TVM_SETBKCOLOR, 0, -1 );

    ok_sequence(MsgSequences, TREEVIEW_SEQ_INDEX, test_get_set_bkcolor_seq,
        "test get set bkcolor", FALSE);

    DestroyWindow(hTree);
}

static void test_get_set_imagelist(void)
{
    HIMAGELIST hImageList = NULL;
    HWND hTree;

    hTree = create_treeview_control();
    fill_tree(hTree);

    flush_sequences(MsgSequences, NUM_MSG_SEQUENCES);

    /* Test a NULL HIMAGELIST */
    SendMessage( hTree, TVM_SETIMAGELIST, TVSIL_NORMAL, (LPARAM)hImageList );
    hImageList = (HIMAGELIST)SendMessage( hTree, TVM_GETIMAGELIST, TVSIL_NORMAL, 0 );
    ok(hImageList == NULL, "NULL image list, reported as 0x%p, expected 0.\n", hImageList);

    /* TODO: Test an actual image list */

    ok_sequence(MsgSequences, TREEVIEW_SEQ_INDEX, test_get_set_imagelist_seq,
        "test get imagelist", FALSE);

    DestroyWindow(hTree);
}

static void test_get_set_indent(void)
{
    int ulIndent = -1;
    int ulMinIndent = -1;
    int ulMoreThanTwiceMin = -1;
    HWND hTree;

    hTree = create_treeview_control();
    fill_tree(hTree);

    flush_sequences(MsgSequences, NUM_MSG_SEQUENCES);

    /* Finding the minimum indent */
    SendMessage( hTree, TVM_SETINDENT, 0, 0 );
    ulMinIndent = (int)SendMessage( hTree, TVM_GETINDENT, 0, 0 );

    /* Checking an indent that is more than twice the default indent */
    ulMoreThanTwiceMin = 2*ulMinIndent+1;
    SendMessage( hTree, TVM_SETINDENT, ulMoreThanTwiceMin, 0 );
    ulIndent = (DWORD)SendMessage( hTree, TVM_GETINDENT, 0, 0 );
    ok(ulIndent == ulMoreThanTwiceMin, "Indent reported as %d, expected %d\n", ulIndent, ulMoreThanTwiceMin);

    ok_sequence(MsgSequences, TREEVIEW_SEQ_INDEX, test_get_set_indent_seq,
        "test get set indent", FALSE);

    DestroyWindow(hTree);
}

static void test_get_set_insertmark(void)
{
    COLORREF crColor = RGB(0,0,0);
    HWND hTree;

    hTree = create_treeview_control();
    fill_tree(hTree);

    flush_sequences(MsgSequences, NUM_MSG_SEQUENCES);

    SendMessage( hTree, TVM_SETINSERTMARKCOLOR, 0, crColor );
    crColor = (COLORREF)SendMessage( hTree, TVM_GETINSERTMARKCOLOR, 0, 0 );
    ok(crColor == RGB(0,0,0), "Insert mark color reported as 0x%.8x, expected 0x00000000\n", crColor);

    ok_sequence(MsgSequences, TREEVIEW_SEQ_INDEX, test_get_set_insertmarkcolor_seq,
        "test get set insertmark color", FALSE);

    DestroyWindow(hTree);
}

static void test_get_set_item(void)
{
    TVITEM tviRoot = {0};
    int nBufferSize = 80;
    char szBuffer[80] = {0};
    HWND hTree;

    hTree = create_treeview_control();
    fill_tree(hTree);

    flush_sequences(MsgSequences, NUM_MSG_SEQUENCES);

    /* Test the root item */
    tviRoot.hItem = hRoot;
    tviRoot.mask = TVIF_TEXT;
    tviRoot.cchTextMax = nBufferSize;
    tviRoot.pszText = szBuffer;
    SendMessage( hTree, TVM_GETITEM, 0, (LPARAM)&tviRoot );
    ok(!strcmp("Root", szBuffer), "GetItem: szBuffer=\"%s\", expected \"Root\"\n", szBuffer);

    /* Change the root text */
    strncpy(szBuffer, "Testing123", nBufferSize);
    SendMessage( hTree, TVM_SETITEM, 0, (LPARAM)&tviRoot );
    memset(szBuffer, 0, nBufferSize);
    SendMessage( hTree, TVM_GETITEM, 0, (LPARAM)&tviRoot );
    ok(!strcmp("Testing123", szBuffer), "GetItem: szBuffer=\"%s\", expected \"Testing123\"\n", szBuffer);

    /* Reset the root text */
    memset(szBuffer, 0, nBufferSize);
    strncpy(szBuffer, "Root", nBufferSize);
    SendMessage( hTree, TVM_SETITEM, 0, (LPARAM)&tviRoot );

    ok_sequence(MsgSequences, TREEVIEW_SEQ_INDEX, test_get_set_item_seq,
        "test get set item", FALSE);

    DestroyWindow(hTree);
}

static void test_get_set_itemheight(void)
{
    int ulOldHeight = 0;
    int ulNewHeight = 0;
    HWND hTree;

    hTree = create_treeview_control();
    fill_tree(hTree);

    flush_sequences(MsgSequences, NUM_MSG_SEQUENCES);

    /* Assuming default height to begin with */
    ulOldHeight = (int) SendMessage( hTree, TVM_GETITEMHEIGHT, 0, 0 );

    /* Explicitly setting and getting the default height */
    SendMessage( hTree, TVM_SETITEMHEIGHT, -1, 0 );
    ulNewHeight = (int) SendMessage( hTree, TVM_GETITEMHEIGHT, 0, 0 );
    ok(ulNewHeight == ulOldHeight, "Default height not set properly, reported %d, expected %d\n", ulNewHeight, ulOldHeight);

    /* Explicitly setting and getting the height of twice the normal */
    SendMessage( hTree, TVM_SETITEMHEIGHT, 2*ulOldHeight, 0 );
    ulNewHeight = (int) SendMessage( hTree, TVM_GETITEMHEIGHT, 0, 0 );
    ok(ulNewHeight == 2*ulOldHeight, "New height not set properly, reported %d, expected %d\n", ulNewHeight, 2*ulOldHeight);

    /* Assuming tree doesn't have TVS_NONEVENHEIGHT set, so a set of 9 will round down to 8 */
    SendMessage( hTree, TVM_SETITEMHEIGHT, 9, 0 );
    ulNewHeight = (int) SendMessage( hTree, TVM_GETITEMHEIGHT, 0, 0 );
    ok(ulNewHeight == 8, "Uneven height not set properly, reported %d, expected %d\n", ulNewHeight, 8);

    ok_sequence(MsgSequences, TREEVIEW_SEQ_INDEX, test_get_set_itemheight_seq,
        "test get set item height", FALSE);

    DestroyWindow(hTree);
}

static void test_get_set_scrolltime(void)
{
    int ulExpectedTime = 20;
    int ulTime = 0;
    HWND hTree;

    hTree = create_treeview_control();
    fill_tree(hTree);

    flush_sequences(MsgSequences, NUM_MSG_SEQUENCES);

    SendMessage( hTree, TVM_SETSCROLLTIME, ulExpectedTime, 0 );
    ulTime = (int)SendMessage( hTree, TVM_GETSCROLLTIME, 0, 0 );
    ok(ulTime == ulExpectedTime, "Scroll time reported as %d, expected %d\n", ulTime, ulExpectedTime);

    ok_sequence(MsgSequences, TREEVIEW_SEQ_INDEX, test_get_set_scrolltime_seq,
        "test get set scroll time", FALSE);

    DestroyWindow(hTree);
}

static void test_get_set_textcolor(void)
{
    /* If the value is -1, the control is using the system color for the text color. */
    COLORREF crColor = RGB(0,0,0);
    HWND hTree;

    hTree = create_treeview_control();
    fill_tree(hTree);

    flush_sequences(MsgSequences, NUM_MSG_SEQUENCES);

    crColor = (COLORREF)SendMessage( hTree, TVM_GETTEXTCOLOR, 0, 0 );
    ok(crColor == -1, "Default text color reported as 0x%.8x\n", crColor);

    /* Test for black text */
    SendMessage( hTree, TVM_SETTEXTCOLOR, 0, RGB(0,0,0) );
    crColor = (COLORREF)SendMessage( hTree, TVM_GETTEXTCOLOR, 0, 0 );
    ok(crColor == RGB(0,0,0), "Black text color reported as 0x%.8x\n", crColor);

    /* Test for white text */
    SendMessage( hTree, TVM_SETTEXTCOLOR, 0, RGB(255,255,255) );
    crColor = (COLORREF)SendMessage( hTree, TVM_GETTEXTCOLOR, 0, 0 );
    ok(crColor == RGB(255,255,255), "White text color reported as 0x%.8x\n", crColor);

    /* Reset the default text color */
    SendMessage( hTree, TVM_SETTEXTCOLOR, 0, CLR_NONE );

    ok_sequence(MsgSequences, TREEVIEW_SEQ_INDEX, test_get_set_textcolor_seq,
        "test get set text color", FALSE);

    DestroyWindow(hTree);
}

static void test_get_set_tooltips(void)
{
    HWND hwndLastToolTip = NULL;
    HWND hPopupTreeView;
    HWND hTree;

    hTree = create_treeview_control();
    fill_tree(hTree);

    flush_sequences(MsgSequences, NUM_MSG_SEQUENCES);

    /* show even WS_POPUP treeview don't send NM_TOOLTIPSCREATED */
    hPopupTreeView = CreateWindow(WC_TREEVIEW, NULL, WS_POPUP|WS_VISIBLE, 0, 0, 100, 100, hMainWnd, NULL, NULL, NULL);
    DestroyWindow(hPopupTreeView);

    /* Testing setting a NULL ToolTip */
    SendMessage( hTree, TVM_SETTOOLTIPS, 0, 0 );
    hwndLastToolTip = (HWND)SendMessage( hTree, TVM_GETTOOLTIPS, 0, 0 );
    ok(hwndLastToolTip == NULL, "NULL tool tip, reported as 0x%p, expected 0.\n", hwndLastToolTip);

    ok_sequence(MsgSequences, TREEVIEW_SEQ_INDEX, test_get_set_tooltips_seq,
        "test get set tooltips", TRUE);

    /* TODO: Add a test of an actual tooltip */
    DestroyWindow(hTree);
}

static void test_get_set_unicodeformat(void)
{
    BOOL bPreviousSetting = 0;
    BOOL bNewSetting = 0;
    HWND hTree;

    hTree = create_treeview_control();
    fill_tree(hTree);

    flush_sequences(MsgSequences, NUM_MSG_SEQUENCES);

    /* Set to Unicode */
    bPreviousSetting = (BOOL)SendMessage( hTree, TVM_SETUNICODEFORMAT, 1, 0 );
    bNewSetting = (BOOL)SendMessage( hTree, TVM_GETUNICODEFORMAT, 0, 0 );
    ok(bNewSetting == 1, "Unicode setting did not work.\n");

    /* Set to ANSI */
    SendMessage( hTree, TVM_SETUNICODEFORMAT, 0, 0 );
    bNewSetting = (BOOL)SendMessage( hTree, TVM_GETUNICODEFORMAT, 0, 0 );
    ok(bNewSetting == 0, "ANSI setting did not work.\n");

    /* Revert to original setting */
    SendMessage( hTree, TVM_SETUNICODEFORMAT, bPreviousSetting, 0 );

    ok_sequence(MsgSequences, TREEVIEW_SEQ_INDEX, test_get_set_unicodeformat_seq,
        "test get set unicode format", FALSE);

    DestroyWindow(hTree);
}

static LRESULT CALLBACK MyWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg) {
    case WM_NOTIFY:
    {
        NMHDR *pHdr = (NMHDR *)lParam;
    
        ok(pHdr->code != NM_FIRST - 19, "Treeview should not send NM_TOOLTIPSCREATED\n");
        if (pHdr->idFrom == 100) {
            NMTREEVIEWA *pTreeView = (LPNMTREEVIEWA) lParam;
            switch(pHdr->code) {
            case TVN_SELCHANGINGA:
                AddItem('(');
                IdentifyItem(pTreeView->itemOld.hItem);
                IdentifyItem(pTreeView->itemNew.hItem);
                return 0;
            case TVN_SELCHANGEDA:
                AddItem(')');
                IdentifyItem(pTreeView->itemOld.hItem);
                IdentifyItem(pTreeView->itemNew.hItem);
                return 0;
            case TVN_GETDISPINFOA: {
                NMTVDISPINFOA *disp = (NMTVDISPINFOA *)lParam;
                if (disp->item.mask & TVIF_TEXT) {
                    lstrcpyn(disp->item.pszText, TEST_CALLBACK_TEXT, disp->item.cchTextMax);
                }
                return 0;
              }
            case TVN_ENDLABELEDIT: return TRUE;
            }
        }
        return 0;
    }
  
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
  
    default:
        return DefWindowProcA(hWnd, msg, wParam, lParam);
    }
    return 0L;
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

    hTree = create_treeview_control();

    /* The test builds the following tree and expands then node 1, while node 0 is collapsed.
     *
     * 0
     * |- 1
     * |  |- 2
     * |  |- 3
     * |- 4
     *
     */

    ret = TreeView_DeleteAllItems(hTree);
    ok(ret == TRUE, "ret\n");
    ins.hParent = TVI_ROOT;
    ins.hInsertAfter = TVI_ROOT;
    U(ins).item.mask = TVIF_TEXT;
    U(ins).item.pszText = nodeText[0];
    node[0] = TreeView_InsertItem(hTree, &ins);
    assert(node[0]);

    ins.hInsertAfter = TVI_LAST;
    U(ins).item.mask = TVIF_TEXT;
    ins.hParent = node[0];

    U(ins).item.pszText = nodeText[1];
    node[1] = TreeView_InsertItem(hTree, &ins);
    assert(node[1]);
    U(ins).item.pszText = nodeText[4];
    node[4] = TreeView_InsertItem(hTree, &ins);
    assert(node[4]);

    ins.hParent = node[1];

    U(ins).item.pszText = nodeText[2];
    node[2] = TreeView_InsertItem(hTree, &ins);
    assert(node[2]);
    U(ins).item.pszText = nodeText[3];
    node[3] = TreeView_InsertItem(hTree, &ins);
    assert(node[3]);


    nodeVisible = TreeView_GetItemRect(hTree, node[1], &dummyRect, FALSE);
    ok(!nodeVisible, "Node 1 should not be visible.\n");
    nodeVisible = TreeView_GetItemRect(hTree, node[2], &dummyRect, FALSE);
    ok(!nodeVisible, "Node 2 should not be visible.\n");
    nodeVisible = TreeView_GetItemRect(hTree, node[3], &dummyRect, FALSE);
    ok(!nodeVisible, "Node 3 should not be visible.\n");
    nodeVisible = TreeView_GetItemRect(hTree, node[4], &dummyRect, FALSE);
    ok(!nodeVisible, "Node 4 should not be visible.\n");

    ok(TreeView_Expand(hTree, node[1], TVE_EXPAND), "Expand of node 1 failed.\n");

    nodeVisible = TreeView_GetItemRect(hTree, node[1], &dummyRect, FALSE);
    ok(!nodeVisible, "Node 1 should not be visible.\n");
    nodeVisible = TreeView_GetItemRect(hTree, node[2], &dummyRect, FALSE);
    ok(!nodeVisible, "Node 2 should not be visible.\n");
    nodeVisible = TreeView_GetItemRect(hTree, node[3], &dummyRect, FALSE);
    ok(!nodeVisible, "Node 3 should not be visible.\n");
    nodeVisible = TreeView_GetItemRect(hTree, node[4], &dummyRect, FALSE);
    ok(!nodeVisible, "Node 4 should not be visible.\n");

    DestroyWindow(hTree);
}

static void test_itemedit(void)
{
    DWORD r;
    HWND edit;
    TVITEMA item;
    CHAR buff[2];
    HWND hTree;

    hTree = create_treeview_control();
    fill_tree(hTree);

    /* try with null item */
    edit = (HWND)SendMessage(hTree, TVM_EDITLABEL, 0, 0);
    ok(!IsWindow(edit), "Expected valid handle\n");

    /* trigger edit */
    edit = (HWND)SendMessage(hTree, TVM_EDITLABEL, 0, (LPARAM)hRoot);
    ok(IsWindow(edit), "Expected valid handle\n");
    /* item shouldn't be selected automatically after TVM_EDITLABEL */
    r = SendMessage(hTree, TVM_GETITEMSTATE, (WPARAM)hRoot, TVIS_SELECTED);
    expect(0, r);
    /* try to cancel with wrong edit handle */
    r = SendMessage(hTree, WM_COMMAND, MAKEWPARAM(0, EN_KILLFOCUS), 0);
    expect(0, r);
    ok(IsWindow(edit), "Expected edit control to be valid\n");
    r = SendMessage(hTree, WM_COMMAND, MAKEWPARAM(0, EN_KILLFOCUS), (LPARAM)edit);
    expect(0, r);
    ok(!IsWindow(edit), "Expected edit control to be destroyed\n");
    /* try to cancel without creating edit */
    r = SendMessage(hTree, WM_COMMAND, MAKEWPARAM(0, EN_KILLFOCUS), 0);
    expect(0, r);

    /* try to cancel with wrong (not null) handle */
    edit = (HWND)SendMessage(hTree, TVM_EDITLABEL, 0, (LPARAM)hRoot);
    ok(IsWindow(edit), "Expected valid handle\n");
    r = SendMessage(hTree, WM_COMMAND, MAKEWPARAM(0, EN_KILLFOCUS), (LPARAM)hTree);
    expect(0, r);
    ok(IsWindow(edit), "Expected edit control to be valid\n");
    r = SendMessage(hTree, WM_COMMAND, MAKEWPARAM(0, EN_KILLFOCUS), (LPARAM)edit);
    expect(0, r);

    /* remove selection after starting edit */
    r = TreeView_SelectItem(hTree, hRoot);
    expect(TRUE, r);
    edit = (HWND)SendMessage(hTree, TVM_EDITLABEL, 0, (LPARAM)hRoot);
    ok(IsWindow(edit), "Expected valid handle\n");
    r = TreeView_SelectItem(hTree, NULL);
    expect(TRUE, r);
    /* alter text */
    strncpy(buff, "x", sizeof(buff)/sizeof(CHAR));
    r = SendMessage(edit, WM_SETTEXT, 0, (LPARAM)buff);
    expect(TRUE, r);
    r = SendMessage(hTree, WM_COMMAND, MAKEWPARAM(0, EN_KILLFOCUS), (LPARAM)edit);
    expect(0, r);
    ok(!IsWindow(edit), "Expected edit control to be destroyed\n");
    /* check that text is saved */
    item.mask = TVIF_TEXT;
    item.hItem = hRoot;
    item.pszText = buff;
    item.cchTextMax = sizeof(buff)/sizeof(CHAR);
    r = SendMessage(hTree, TVM_GETITEM, 0, (LPARAM)&item);
    expect(TRUE, r);
    ok(!strcmp("x", buff), "Expected item text to change\n");

    DestroyWindow(hTree);
}

static void test_treeview_classinfo(void)
{
    WNDCLASSA cls;

    memset(&cls, 0, sizeof(cls));
    GetClassInfo(GetModuleHandleA("comctl32.dll"), WC_TREEVIEWA, &cls);
    ok(cls.hbrBackground == NULL, "Expected NULL background brush, got %p\n", cls.hbrBackground);
    ok(cls.style == (CS_GLOBALCLASS | CS_DBLCLKS), "Expected got %x\n", cls.style);
    expect(0, cls.cbClsExtra);
}

static void test_get_linecolor(void)
{
    COLORREF clr;
    HWND hTree;

    hTree = create_treeview_control();

    /* newly created control has default color */
    clr = (COLORREF)SendMessage(hTree, TVM_GETLINECOLOR, 0, 0);
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

    hTree = create_treeview_control();

    /* newly created control has default color */
    clr = (COLORREF)SendMessage(hTree, TVM_GETINSERTMARKCOLOR, 0, 0);
    if (clr == 0)
        win_skip("TVM_GETINSERTMARKCOLOR is not supported on comctl32 < 5.80\n");
    else
        expect(CLR_DEFAULT, clr);

    DestroyWindow(hTree);
}

START_TEST(treeview)
{
    HMODULE hComctl32;
    BOOL (WINAPI *pInitCommonControlsEx)(const INITCOMMONCONTROLSEX*);
    WNDCLASSA wc;
    MSG msg;
  
    hComctl32 = GetModuleHandleA("comctl32.dll");
    pInitCommonControlsEx = (void*)GetProcAddress(hComctl32, "InitCommonControlsEx");
    if (pInitCommonControlsEx)
    {
        INITCOMMONCONTROLSEX iccex;
        iccex.dwSize = sizeof(iccex);
        iccex.dwICC  = ICC_TREEVIEW_CLASSES;
        pInitCommonControlsEx(&iccex);
    }
    else
        InitCommonControls();

    init_msg_sequences(MsgSequences, NUM_MSG_SEQUENCES);
  
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.hIcon = NULL;
    wc.hCursor = LoadCursorA(NULL, IDC_IBEAM);
    wc.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "MyTestWnd";
    wc.lpfnWndProc = MyWndProc;
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

    PostMessageA(hMainWnd, WM_CLOSE, 0, 0);
    while(GetMessageA(&msg,0,0,0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}
