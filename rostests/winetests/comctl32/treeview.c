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

#define NUM_MSG_SEQUENCES   1
#define LISTVIEW_SEQ_INDEX  0

static struct msg_sequence *MsgSequences[NUM_MSG_SEQUENCES];

static const struct message FillRootSeq[] = {
    { TVM_INSERTITEM, sent },
    { TVM_GETITEM, sent },
    { TVM_INSERTITEM, sent },
    { 0 }
};

static const struct message DoTest1Seq[] = {
    { TVM_SELECTITEM, sent|wparam, 0x00000009 },
    { TVM_SELECTITEM, sent|wparam, 0x00000009 },
    { TVM_SELECTITEM, sent|wparam, 0x00000009 },
    { TVM_SELECTITEM, sent|wparam, 0x00000009 },
    { TVM_SELECTITEM, sent|wparam, 0x00000009 },
    { TVM_SELECTITEM, sent|wparam, 0x00000009 },
    { 0 }
};

static const struct message DoTest2Seq[] = {
    { TVM_SELECTITEM, sent|wparam, 0x00000009 },
    { TVM_SELECTITEM, sent|wparam, 0x00000009 },
    { TVM_SELECTITEM, sent|wparam, 0x00000009 },
    { TVM_SELECTITEM, sent|wparam, 0x00000009 },
    { TVM_SELECTITEM, sent|wparam, 0x00000009 },
    { TVM_SELECTITEM, sent|wparam, 0x00000009 },
    { 0 }
};

static const struct message DoTest3Seq[] = {
    { TVM_INSERTITEM, sent },
    { TVM_GETITEM, sent },
    { TVM_DELETEITEM, sent },
    { 0 }
};

static const struct message DoFocusTestSeq[] = {
    { TVM_INSERTITEM, sent },
    { TVM_INSERTITEM, sent },
    { WM_WINDOWPOSCHANGING, sent|defwinproc },
    { WM_NCCALCSIZE, sent|wparam|defwinproc, 0x00000001 },
    { WM_WINDOWPOSCHANGED, sent|defwinproc },
    { WM_SIZE, sent|defwinproc },
    { WM_WINDOWPOSCHANGING, sent },
    { WM_NCCALCSIZE, sent|wparam, 0x00000001 },
    { WM_WINDOWPOSCHANGED, sent },
    { WM_SIZE, sent|defwinproc },
    { WM_WINDOWPOSCHANGING, sent|defwinproc|optional },
    { WM_NCCALCSIZE, sent|wparam|defwinproc|optional, 0x00000001 },
    { WM_WINDOWPOSCHANGED, sent|defwinproc|optional },
    { WM_SIZE, sent|defwinproc|optional },
    { TVM_SELECTITEM, sent|wparam, 0x00000009 },
    /* The following end up out of order in wine */
    { WM_PAINT, sent|defwinproc },
    { WM_NCPAINT, sent|wparam|defwinproc, 0x00000001 },
    { WM_ERASEBKGND, sent|defwinproc },
    { TVM_EDITLABEL, sent },
    { WM_COMMAND, sent|wparam|defwinproc, 0x04000000 },
    { WM_COMMAND, sent|wparam|defwinproc, 0x03000000 },
    { WM_PARENTNOTIFY, sent|wparam|defwinproc, 0x00000001 },
    { WM_KILLFOCUS, sent|defwinproc },
    { WM_PAINT, sent|defwinproc },
    { WM_IME_SETCONTEXT, sent|defwinproc|optional },
    { WM_COMMAND, sent|wparam|defwinproc, 0x01000000},
    { WM_ERASEBKGND, sent|defwinproc },
    { WM_CTLCOLOREDIT, sent|defwinproc|optional },
    { WM_CTLCOLOREDIT, sent|defwinproc|optional },
    { 0 }
};

static const struct message TestGetSetBkColorSeq[] = {
    { TVM_GETBKCOLOR, sent|wparam|lparam, 0x00000000, 0x00000000 },
    { TVM_SETBKCOLOR, sent|wparam|lparam, 0x00000000, 0x00000000 },
    { TVM_GETBKCOLOR, sent|wparam|lparam, 0x00000000, 0x00000000 },
    { TVM_SETBKCOLOR, sent|wparam|lparam, 0x00000000, 0x00ffffff },
    { TVM_GETBKCOLOR, sent|wparam|lparam, 0x00000000, 0x00000000 },
    { TVM_SETBKCOLOR, sent|wparam|lparam, 0x00000000, -1 },
    { 0 }
};

static const struct message TestGetSetImageListSeq[] = {
    { TVM_SETIMAGELIST, sent|wparam|lparam, 0x00000000, 0x00000000 },
    { TVM_GETIMAGELIST, sent|wparam|lparam, 0x00000000, 0x00000000 },
    { 0 }
};

static const struct message TestGetSetIndentSeq[] = {
    { TVM_SETINDENT, sent|wparam|lparam, 0x00000000, 0x00000000 },
    { TVM_GETINDENT, sent|wparam|lparam, 0x00000000, 0x00000000 },
    /* The actual amount to indent is dependent on the system for this message */
    { TVM_SETINDENT, sent },
    { TVM_GETINDENT, sent|wparam|lparam, 0x00000000, 0x00000000 },
    { 0 }
};

static const struct message TestGetSetInsertMarkColorSeq[] = {
    { TVM_SETINSERTMARKCOLOR, sent|wparam|lparam, 0x00000000, 0x00000000 },
    { TVM_GETINSERTMARKCOLOR, sent|wparam|lparam, 0x00000000, 0x00000000 },
    { 0 }
};

static const struct message TestGetSetItemSeq[] = {
    { TVM_GETITEM, sent },
    { TVM_SETITEM, sent },
    { TVM_GETITEM, sent },
    { TVM_SETITEM, sent },
    { 0 }
};

static const struct message TestGetSetItemHeightSeq[] = {
    { TVM_GETITEMHEIGHT, sent|wparam|lparam, 0x00000000, 0x00000000 },
    { TVM_SETITEMHEIGHT, sent|wparam|lparam, -1, 0x00000000 },
    { TVM_GETITEMHEIGHT, sent|wparam|lparam, 0x00000000, 0x00000000 },
    { TVM_SETITEMHEIGHT, sent|lparam, 0xcccccccc, 0x00000000 },
    { TVM_GETITEMHEIGHT, sent|wparam|lparam|optional, 0x00000000, 0x00000000 },
    { TVM_SETITEMHEIGHT, sent|wparam|lparam|optional, 0x00000009, 0x00000000 },
    { WM_WINDOWPOSCHANGING, sent|defwinproc },
    { WM_NCCALCSIZE, sent|wparam|defwinproc, 0x00000001 },
    { WM_WINDOWPOSCHANGED, sent|defwinproc },
    { WM_SIZE, sent|defwinproc },
    { TVM_GETITEMHEIGHT, sent|wparam|lparam, 0x00000000, 0x00000000 },
    { 0 }
};

static const struct message TestGetSetScrollTimeSeq[] = {
    { TVM_SETSCROLLTIME, sent|wparam|lparam, 0x00000014, 0x00000000 },
    { TVM_GETSCROLLTIME, sent|wparam|lparam, 0x00000000, 0x00000000 },
    { 0 }
};

static const struct message TestGetSetTextColorSeq[] = {
    { TVM_GETTEXTCOLOR, sent|wparam|lparam, 0x00000000, 0x00000000 },
    { TVM_SETTEXTCOLOR, sent|wparam|lparam, 0x00000000, 0x00000000 },
    { TVM_GETTEXTCOLOR, sent|wparam|lparam, 0x00000000, 0x00000000 },
    { TVM_SETTEXTCOLOR, sent|wparam|lparam, 0x00000000, 0x00ffffff },
    { TVM_GETTEXTCOLOR, sent|wparam|lparam, 0x00000000, 0x00000000 },
    { TVM_SETTEXTCOLOR, sent|wparam|lparam, 0x00000000, -1 },
    { 0 }
};

static const struct message TestGetSetToolTipsSeq[] = {
    { WM_COMMAND,       sent|wparam,            0x02000000 },
    { WM_PARENTNOTIFY,  sent|wparam|defwinproc, 0x00020002 },
    { TVM_SETTOOLTIPS, sent|wparam|lparam, 0x00000000, 0x00000000 },
    { TVM_GETTOOLTIPS, sent|wparam|lparam, 0x00000000, 0x00000000 },
    { 0 }
};

static const struct message TestGetSetUnicodeFormatSeq[] = {
    { TVM_SETUNICODEFORMAT, sent|wparam|lparam, 0x00000001, 0x00000000 },
    { TVM_GETUNICODEFORMAT, sent|wparam|lparam, 0x00000000, 0x00000000 },
    { TVM_SETUNICODEFORMAT, sent|wparam|lparam, 0x00000000, 0x00000000 },
    { TVM_GETUNICODEFORMAT, sent|wparam|lparam, 0x00000000, 0x00000000 },
    { TVM_SETUNICODEFORMAT, sent|wparam|lparam, 0x00000000, 0x00000000 },
    { 0 }
};

static HWND hMainWnd;

static HWND hTree, hEdit;
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

static void FillRoot(void)
{
    TVINSERTSTRUCTA ins;
    TVITEM tvi;
    static CHAR root[]  = "Root",
                child[] = "Child";

    Clear();
    AddItem('A');
    ins.hParent = TVI_ROOT;
    ins.hInsertAfter = TVI_ROOT;
    U(ins).item.mask = TVIF_TEXT;
    U(ins).item.pszText = root;
    hRoot = TreeView_InsertItem(hTree, &ins);
    assert(hRoot);

    /* UMLPad 1.15 depends on this being not -1 (I_IMAGECALLBACK) */
    tvi.hItem = hRoot;
    tvi.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    SendMessage( hTree, TVM_GETITEM, 0, (LPARAM)&tvi );
    ok(tvi.iImage == 0, "tvi.iImage=%d\n", tvi.iImage);
    ok(tvi.iSelectedImage == 0, "tvi.iSelectedImage=%d\n", tvi.iSelectedImage);

    AddItem('B');
    ins.hParent = hRoot;
    ins.hInsertAfter = TVI_FIRST;
    U(ins).item.mask = TVIF_TEXT;
    U(ins).item.pszText = child;
    hChild = TreeView_InsertItem(hTree, &ins);
    assert(hChild);
    AddItem('.');

    ok(!strcmp(sequence, "AB."), "Item creation\n");
}

static void DoTest1(void)
{
    BOOL r;
    r = TreeView_SelectItem(hTree, NULL);
    Clear();
    AddItem('1');
    r = TreeView_SelectItem(hTree, hRoot);
    AddItem('2');
    r = TreeView_SelectItem(hTree, hRoot);
    AddItem('3');
    r = TreeView_SelectItem(hTree, NULL);
    AddItem('4');
    r = TreeView_SelectItem(hTree, NULL);
    AddItem('5');
    r = TreeView_SelectItem(hTree, hRoot);
    AddItem('.');
    ok(!strcmp(sequence, "1(nR)nR23(Rn)Rn45(nR)nR."), "root-none select test\n");
}

static void DoTest2(void)
{
    BOOL r;
    r = TreeView_SelectItem(hTree, NULL);
    Clear();
    AddItem('1');
    r = TreeView_SelectItem(hTree, hRoot);
    AddItem('2');
    r = TreeView_SelectItem(hTree, hRoot);
    AddItem('3');
    r = TreeView_SelectItem(hTree, hChild);
    AddItem('4');
    r = TreeView_SelectItem(hTree, hChild);
    AddItem('5');
    r = TreeView_SelectItem(hTree, hRoot);
    AddItem('.');
    ok(!strcmp(sequence, "1(nR)nR23(RC)RC45(CR)CR."), "root-child select test\n");
}

static void DoTest3(void)
{
    TVINSERTSTRUCTA ins;
    HTREEITEM hChild;
    TVITEM tvi;

    int nBufferSize = 80;
    CHAR szBuffer[80] = "Blah";

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
}

static void DoFocusTest(void)
{
    TVINSERTSTRUCTA ins;
    static CHAR child1[]  = "Edit",
                child2[]  = "A really long string";
    HTREEITEM hChild1, hChild2;

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
    /* Using SendMessageA since Win98 doesn't have default unicode support */
    SendMessageA(hTree, TVM_SELECTITEM, TVGN_CARET, (LPARAM)hChild);
    hEdit = TreeView_EditLabel(hTree, hChild);
    ScrollWindowEx(hTree, -10, 0, NULL, NULL, NULL, NULL, SW_SCROLLCHILDREN);
    ok(GetFocus() == hEdit, "Edit control should have focus\n");
}

static void TestGetSetBkColor(void)
{
    COLORREF crColor = RGB(0,0,0);

    todo_wine{
        /* If the value is -1, the control is using the system color for the background color. */
        crColor = (COLORREF)SendMessage( hTree, TVM_GETBKCOLOR, 0, 0 );
        ok(crColor == -1, "Default background color reported as 0x%.8x\n", crColor);
    }

    /* Test for black background */
    SendMessage( hTree, TVM_SETBKCOLOR, 0, (LPARAM)RGB(0,0,0) );
    crColor = (COLORREF)SendMessage( hTree, TVM_GETBKCOLOR, 0, 0 );
    ok(crColor == RGB(0,0,0), "Black background color reported as 0x%.8x\n", crColor);

    /* Test for white background */
    SendMessage( hTree, TVM_SETBKCOLOR, 0, (LPARAM)RGB(255,255,255) );
    crColor = (COLORREF)SendMessage( hTree, TVM_GETBKCOLOR, 0, 0 );
    ok(crColor == RGB(255,255,255), "White background color reported as 0x%.8x\n", crColor);

    /* Reset the default background */
    SendMessage( hTree, TVM_SETBKCOLOR, 0, -1 );
}

static void TestGetSetImageList(void)
{
    HIMAGELIST hImageList = NULL;

    /* Test a NULL HIMAGELIST */
    SendMessage( hTree, TVM_SETIMAGELIST, TVSIL_NORMAL, (LPARAM)hImageList );
    hImageList = (HIMAGELIST)SendMessage( hTree, TVM_GETIMAGELIST, TVSIL_NORMAL, 0 );
    ok(hImageList == NULL, "NULL image list, reported as 0x%p, expected 0.\n", hImageList);

    /* TODO: Test an actual image list */
}

static void TestGetSetIndent(void)
{
    int ulIndent = -1;
    int ulMinIndent = -1;
    int ulMoreThanTwiceMin = -1;

    /* Finding the minimum indent */
    SendMessage( hTree, TVM_SETINDENT, 0, 0 );
    ulMinIndent = (int)SendMessage( hTree, TVM_GETINDENT, 0, 0 );

    /* Checking an indent that is more than twice the default indent */
    ulMoreThanTwiceMin = 2*ulMinIndent+1;
    SendMessage( hTree, TVM_SETINDENT, ulMoreThanTwiceMin, 0 );
    ulIndent = (DWORD)SendMessage( hTree, TVM_GETINDENT, 0, 0 );
    ok(ulIndent == ulMoreThanTwiceMin, "Indent reported as %d, expected %d\n", ulIndent, ulMoreThanTwiceMin);
}

static void TestGetSetInsertMarkColor(void)
{
    COLORREF crColor = RGB(0,0,0);
    SendMessage( hTree, TVM_SETINSERTMARKCOLOR, 0, crColor );
    crColor = (COLORREF)SendMessage( hTree, TVM_GETINSERTMARKCOLOR, 0, 0 );
    ok(crColor == RGB(0,0,0), "Insert mark color reported as 0x%.8x, expected 0x00000000\n", crColor);
}

static void TestGetSetItem(void)
{
    TVITEM tviRoot = {0};
    int nBufferSize = 80;
    char szBuffer[80] = {0};

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
}

static void TestGetSetItemHeight(void)
{
    int ulOldHeight = 0;
    int ulNewHeight = 0;

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
}

static void TestGetSetScrollTime(void)
{
    int ulExpectedTime = 20;
    int ulTime = 0;
    SendMessage( hTree, TVM_SETSCROLLTIME, ulExpectedTime, 0 );
    ulTime = (int)SendMessage( hTree, TVM_GETSCROLLTIME, 0, 0 );
    ok(ulTime == ulExpectedTime, "Scroll time reported as %d, expected %d\n", ulTime, ulExpectedTime);
}

static void TestGetSetTextColor(void)
{
    /* If the value is -1, the control is using the system color for the text color. */
    COLORREF crColor = RGB(0,0,0);
    crColor = (COLORREF)SendMessage( hTree, TVM_GETTEXTCOLOR, 0, 0 );
    ok(crColor == -1, "Default text color reported as 0x%.8x\n", crColor);

    /* Test for black text */
    SendMessage( hTree, TVM_SETTEXTCOLOR, 0, (LPARAM)RGB(0,0,0) );
    crColor = (COLORREF)SendMessage( hTree, TVM_GETTEXTCOLOR, 0, 0 );
    ok(crColor == RGB(0,0,0), "Black text color reported as 0x%.8x\n", crColor);

    /* Test for white text */
    SendMessage( hTree, TVM_SETTEXTCOLOR, 0, (LPARAM)RGB(255,255,255) );
    crColor = (COLORREF)SendMessage( hTree, TVM_GETTEXTCOLOR, 0, 0 );
    ok(crColor == RGB(255,255,255), "White text color reported as 0x%.8x\n", crColor);

    /* Reset the default text color */
    SendMessage( hTree, TVM_SETTEXTCOLOR, 0, -1 );
}

static void TestGetSetToolTips(void)
{
    HWND hwndLastToolTip = NULL;
    HWND hPopupTreeView;

    /* show even WS_POPUP treeview don't send NM_TOOLTIPSCREATED */
    hPopupTreeView = CreateWindow(WC_TREEVIEW, NULL, WS_POPUP|WS_VISIBLE, 0, 0, 100, 100, hMainWnd, NULL, NULL, NULL);
    DestroyWindow(hPopupTreeView);

    /* Testing setting a NULL ToolTip */
    SendMessage( hTree, TVM_SETTOOLTIPS, 0, 0 );
    hwndLastToolTip = (HWND)SendMessage( hTree, TVM_GETTOOLTIPS, 0, 0 );
    ok(hwndLastToolTip == NULL, "NULL tool tip, reported as 0x%p, expected 0.\n", hwndLastToolTip);

    /* TODO: Add a test of an actual tooltip */
}

static void TestGetSetUnicodeFormat(void)
{
    BOOL bPreviousSetting = 0;
    BOOL bNewSetting = 0;

    /* Set to Unicode */
    bPreviousSetting = (BOOL)SendMessage( hTree, TVM_SETUNICODEFORMAT, 1, 0 );
    bNewSetting = (BOOL)SendMessage( hTree, TVM_GETUNICODEFORMAT, 0, 0 );
    ok(bNewSetting == 1, "Unicode setting did not work.\n");

    /* Set to ANSI */
    SendMessage( hTree, TVM_SETUNICODEFORMAT, 0, 0 );
    bNewSetting = (BOOL)SendMessage( hTree, TVM_GETUNICODEFORMAT, 0, 0 );
    ok(bNewSetting == 0, "ANSI setting did not work.\n");

    /* Revert to original setting */
    SendMessage( hTree, TVM_SETUNICODEFORMAT, (LPARAM)bPreviousSetting, 0 );
}

static void TestGetSet(void)
{
    /* TVM_GETBKCOLOR and TVM_SETBKCOLOR */
    flush_sequences(MsgSequences, NUM_MSG_SEQUENCES);
    TestGetSetBkColor();
    ok_sequence(MsgSequences, LISTVIEW_SEQ_INDEX, TestGetSetBkColorSeq,
        "TestGetSetBkColor", FALSE);

    /* TVM_GETIMAGELIST and TVM_SETIMAGELIST */
    flush_sequences(MsgSequences, NUM_MSG_SEQUENCES);
    TestGetSetImageList();
    ok_sequence(MsgSequences, LISTVIEW_SEQ_INDEX, TestGetSetImageListSeq,
        "TestGetImageList", FALSE);

    /* TVM_SETINDENT and TVM_GETINDENT */
    flush_sequences(MsgSequences, NUM_MSG_SEQUENCES);
    TestGetSetIndent();
    ok_sequence(MsgSequences, LISTVIEW_SEQ_INDEX, TestGetSetIndentSeq,
        "TestGetSetIndent", FALSE);

    /* TVM_GETINSERTMARKCOLOR and TVM_GETINSERTMARKCOLOR */
    flush_sequences(MsgSequences, NUM_MSG_SEQUENCES);
    TestGetSetInsertMarkColor();
    ok_sequence(MsgSequences, LISTVIEW_SEQ_INDEX, TestGetSetInsertMarkColorSeq,
        "TestGetSetInsertMarkColor", FALSE);

    /* TVM_GETITEM and TVM_SETITEM */
    flush_sequences(MsgSequences, NUM_MSG_SEQUENCES);
    TestGetSetItem();
    ok_sequence(MsgSequences, LISTVIEW_SEQ_INDEX, TestGetSetItemSeq,
        "TestGetSetItem", FALSE);

    /* TVM_GETITEMHEIGHT and TVM_SETITEMHEIGHT */
    flush_sequences(MsgSequences, NUM_MSG_SEQUENCES);
    TestGetSetItemHeight();
    ok_sequence(MsgSequences, LISTVIEW_SEQ_INDEX, TestGetSetItemHeightSeq,
        "TestGetSetItemHeight", FALSE);

    /* TVM_GETSCROLLTIME and TVM_SETSCROLLTIME */
    flush_sequences(MsgSequences, NUM_MSG_SEQUENCES);
    TestGetSetScrollTime();
    ok_sequence(MsgSequences, LISTVIEW_SEQ_INDEX, TestGetSetScrollTimeSeq,
        "TestGetSetScrollTime", FALSE);

    /* TVM_GETTEXTCOLOR and TVM_SETTEXTCOLOR */
    flush_sequences(MsgSequences, NUM_MSG_SEQUENCES);
    TestGetSetTextColor();
    ok_sequence(MsgSequences, LISTVIEW_SEQ_INDEX, TestGetSetTextColorSeq,
        "TestGetSetTextColor", FALSE);

    /* TVM_GETTOOLTIPS and TVM_SETTOOLTIPS */
    flush_sequences(MsgSequences, NUM_MSG_SEQUENCES);
    TestGetSetToolTips();
    ok_sequence(MsgSequences, LISTVIEW_SEQ_INDEX, TestGetSetToolTipsSeq,
        "TestGetSetToolTips", TRUE);

    /* TVM_GETUNICODEFORMAT and TVM_SETUNICODEFORMAT */
    flush_sequences(MsgSequences, NUM_MSG_SEQUENCES);
    TestGetSetUnicodeFormat();
    ok_sequence(MsgSequences, LISTVIEW_SEQ_INDEX, TestGetSetUnicodeFormatSeq,
        "TestGetSetUnicodeFormat", FALSE);
}

/* This function hooks in and records all messages to the treeview control */
static LRESULT WINAPI TreeviewWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static long defwndproc_counter = 0;
    LRESULT ret;
    struct message msg;
    WNDPROC lpOldProc = (WNDPROC)GetWindowLongPtrA(hwnd, GWLP_USERDATA);

    msg.message = message;
    msg.flags = sent|wparam|lparam;
    if (defwndproc_counter) msg.flags |= defwinproc;
    msg.wParam = wParam;
    msg.lParam = lParam;
    add_message(MsgSequences, LISTVIEW_SEQ_INDEX, &msg);

    defwndproc_counter++;
    ret = CallWindowProcA(lpOldProc, hwnd, message, wParam, lParam);
    defwndproc_counter--;

    return ret;
}

static LRESULT CALLBACK MyWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WNDPROC pOldWndProc;

    switch(msg) {

    case WM_CREATE:
    {
        hTree = CreateWindowExA(WS_EX_CLIENTEDGE, WC_TREEVIEWA, NULL, WS_CHILD|WS_VISIBLE|
            TVS_LINESATROOT|TVS_HASLINES|TVS_HASBUTTONS|TVS_EDITLABELS,
            0, 0, 120, 100, hWnd, (HMENU)100, GetModuleHandleA(0), 0);

        SetFocus(hTree);

        /* Record the old WNDPROC so we can call it after recording the messages */
        pOldWndProc = (WNDPROC)SetWindowLongPtrA(hTree, GWLP_WNDPROC, (LONG_PTR)TreeviewWndProc);
        SetWindowLongPtrA(hTree, GWLP_USERDATA, (LONG_PTR)pOldWndProc);

        return 0;
    }
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
            }
        }
        return 0;
    }
  
    case WM_SIZE:
        MoveWindow(hTree, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
        break;
      
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
  
    default:
        return DefWindowProcA(hWnd, msg, wParam, lParam);
    }
    return 0L;
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

    if ( !ok(hMainWnd != NULL, "Failed to create parent window. Tests aborted.\n") )
        return;

    flush_sequences(MsgSequences, NUM_MSG_SEQUENCES);
    FillRoot();
    ok_sequence(MsgSequences, LISTVIEW_SEQ_INDEX, FillRootSeq, "FillRoot", FALSE);

    flush_sequences(MsgSequences, NUM_MSG_SEQUENCES);
    DoTest1();
    ok_sequence(MsgSequences, LISTVIEW_SEQ_INDEX, DoTest1Seq, "DoTest1", FALSE);

    flush_sequences(MsgSequences, NUM_MSG_SEQUENCES);
    DoTest2();
    ok_sequence(MsgSequences, LISTVIEW_SEQ_INDEX, DoTest2Seq, "DoTest2", FALSE);

    flush_sequences(MsgSequences, NUM_MSG_SEQUENCES);
    DoTest3();
    ok_sequence(MsgSequences, LISTVIEW_SEQ_INDEX, DoTest3Seq, "DoTest3", FALSE);

    flush_sequences(MsgSequences, NUM_MSG_SEQUENCES);
    DoFocusTest();
    ok_sequence(MsgSequences, LISTVIEW_SEQ_INDEX, DoFocusTestSeq, "DoFocusTest", TRUE);

    /* Sequences tested inside due to number */
    TestGetSet();

    PostMessageA(hMainWnd, WM_CLOSE, 0, 0);
    while(GetMessageA(&msg,0,0,0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}
