/* Unit test suite for header control.
 *
 * Copyright 2005 Vijay Kiran Kamuju
 * Copyright 2007 Shanren Zhou
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
#include <assert.h>

#include "wine/test.h"
#include "msg.h"

typedef struct tagEXPECTEDNOTIFY
{
    INT iCode;
    BOOL fUnicode;
    HDITEMA hdItem;
} EXPECTEDNOTIFY;

typedef LRESULT (*CUSTOMDRAWPROC)(int n, NMCUSTOMDRAW *nm);

static CUSTOMDRAWPROC g_CustomDrawProc;
static int g_CustomDrawCount;
static DRAWITEMSTRUCT g_DrawItem;
static BOOL g_DrawItemReceived;
static DWORD g_customheight;

static EXPECTEDNOTIFY expectedNotify[10];
static INT nExpectedNotify = 0;
static INT nReceivedNotify = 0;
static INT unexpectedNotify[10];
static INT nUnexpectedNotify = 0;

static HWND hHeaderParentWnd;
static HWND hWndHeader;
#define MAX_CHARS 100

#define compare(val, exp, fmt)  ok((val) == (exp), #val " value: " fmt ", expected: " fmt "\n", (val), (exp))

#define expect(expected, got) ok(expected == got, "expected %d, got %d\n", expected,got)

#define NUM_MSG_SEQUENCES    2
#define PARENT_SEQ_INDEX     0
#define HEADER_SEQ_INDEX     1

static struct msg_sequence *sequences[NUM_MSG_SEQUENCES];

static const struct message create_parent_wnd_seq[] = {
    { WM_GETMINMAXINFO, sent },
    { WM_NCCREATE, sent },
    { WM_NCCALCSIZE, sent|wparam, 0 },
    { WM_CREATE, sent },
    { 0 }
};

static const struct message add_header_to_parent_seq_interactive[] = {
    { WM_NOTIFYFORMAT, sent|lparam, 0, NF_QUERY },
    { WM_QUERYUISTATE, sent },
    { WM_PARENTNOTIFY, sent|wparam, 1 },
    { WM_SHOWWINDOW, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGING, sent|wparam, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam, 0 },
    { WM_ACTIVATEAPP, sent|wparam, 1 },
    { WM_NCACTIVATE, sent|wparam, 1 },
    { WM_ACTIVATE, sent|wparam, 1 },
    { WM_IME_SETCONTEXT, sent|defwinproc|wparam, 1 },
    { WM_IME_NOTIFY, sent|defwinproc|wparam, 2 },
    { WM_SETFOCUS, sent|defwinproc|wparam, 0 },
    { WM_WINDOWPOSCHANGED, sent|wparam, 0 },
    { WM_SIZE, sent|wparam, 0 },
    { WM_MOVE, sent|wparam, 0 },
    { 0 }
};

static const struct message add_header_to_parent_seq[] = {
    { WM_NOTIFYFORMAT, sent|lparam, 0, NF_QUERY },
    { WM_QUERYUISTATE, sent|optional },
    { WM_PARENTNOTIFY, sent },
    { 0 }
};

static const struct message insertItem_seq[] = {
    { HDM_INSERTITEM, sent|wparam, 0 },
    { HDM_INSERTITEM, sent|wparam, 1 },
    { HDM_INSERTITEM, sent|wparam, 2 },
    { HDM_INSERTITEM, sent|wparam, 3 },
    { 0 }
};

static const struct message getItem_seq[] = {
    { HDM_GETITEM, sent|wparam, 3 },
    { HDM_GETITEM, sent|wparam, 0 },
    { 0 }
};


static const struct message deleteItem_getItemCount_seq[] = {
    { HDM_DELETEITEM, sent|wparam, 3 },
    { HDM_GETITEMCOUNT, sent },
    { HDM_DELETEITEM, sent|wparam, 3 },
    { HDM_GETITEMCOUNT, sent },
    { HDM_DELETEITEM, sent|wparam, 2 },
    { HDM_GETITEMCOUNT, sent },
    { 0 }
};

static const struct message orderArray_seq[] = {
    { HDM_GETITEMCOUNT, sent },
    { HDM_SETORDERARRAY, sent|wparam, 2 },
    { HDM_GETORDERARRAY, sent|wparam, 2 },
    { 0 }
};

static const struct message setItem_seq[] = {
    { HDM_SETITEM, sent|wparam, 0 },
    { HDM_SETITEM, sent|wparam, 1 },
    { 0 }
};

static const struct message getItemRect_seq[] = {
    { HDM_GETITEMRECT, sent|wparam, 1 },
    { HDM_GETITEMRECT, sent|wparam, 0 },
    { HDM_GETITEMRECT, sent|wparam, 10 },
    { 0 }
};

static const struct message layout_seq[] = {
    { HDM_LAYOUT, sent },
    { 0 }
};

static const struct message orderToIndex_seq[] = {
    { HDM_ORDERTOINDEX, sent|wparam, 1 },
    { 0 }
};

static const struct message hittest_seq[] = {
    { HDM_HITTEST, sent },
    { HDM_HITTEST, sent },
    { HDM_HITTEST, sent },
    { 0 }
};

static const struct message setHotDivider_seq_interactive[] = {
    { HDM_SETHOTDIVIDER, sent|wparam, TRUE },
    { WM_PAINT, sent|defwinproc},
    { WM_NCPAINT, sent|defwinproc},
    { WM_ERASEBKGND, sent|defwinproc},
    { HDM_SETHOTDIVIDER, sent|wparam|lparam, FALSE, 100 },
    { WM_PAINT, sent|defwinproc},
    { HDM_SETHOTDIVIDER, sent|wparam|lparam, FALSE, 1},
    { WM_PAINT, sent|defwinproc},
    { 0 }
};

static const struct message setHotDivider_seq_noninteractive[] = {
    { HDM_SETHOTDIVIDER, sent|wparam, TRUE },
    { HDM_SETHOTDIVIDER, sent|wparam|lparam, FALSE, 100 },
    { HDM_SETHOTDIVIDER, sent|wparam|lparam, FALSE, 1},
    { 0 }
};

static const struct message imageMessages_seq[] = {
    { HDM_SETIMAGELIST, sent },
    { HDM_GETIMAGELIST, sent },
    { HDM_CREATEDRAGIMAGE, sent },
    { 0 }
};

static const struct message filterMessages_seq_interactive[] = {
    { HDM_SETFILTERCHANGETIMEOUT, sent|wparam|lparam, 1, 100 },
    { HDM_CLEARFILTER, sent|wparam|lparam, 0, 1 },
    { HDM_EDITFILTER,  sent|wparam|lparam, 1, 0 },
    { WM_PARENTNOTIFY, sent|wparam|defwinproc, WM_CREATE },
    { WM_CTLCOLOREDIT, sent|defwinproc },
    { WM_COMMAND, sent|defwinproc },
    { 0 }
};

static const struct message filterMessages_seq_noninteractive[] = {
    { HDM_SETFILTERCHANGETIMEOUT, sent|wparam|lparam, 1, 100 },
    { HDM_CLEARFILTER, sent|wparam|lparam, 0, 1 },
    { HDM_EDITFILTER,  sent|wparam|lparam, 1, 0 },
    { WM_PARENTNOTIFY, sent|wparam|defwinproc|optional, WM_CREATE },
    { WM_COMMAND, sent|defwinproc|optional },
    { 0 }
};

static const struct message unicodeformatMessages_seq[] = {
    { HDM_SETUNICODEFORMAT, sent|wparam, TRUE },
    { HDM_GETUNICODEFORMAT, sent },
    { 0 }
};

static const struct message bitmapmarginMessages_seq[] = {
    { HDM_GETBITMAPMARGIN, sent },
    { 0 }
};


static void expect_notify(INT iCode, BOOL fUnicode, HDITEMA *lpItem)
{
    assert(nExpectedNotify < 10);
    expectedNotify[nExpectedNotify].iCode = iCode;
    expectedNotify[nExpectedNotify].fUnicode = fUnicode;
    expectedNotify[nExpectedNotify].hdItem = *lpItem;
    nExpectedNotify++;
}

static void dont_expect_notify(INT iCode)
{
    assert(nUnexpectedNotify < 10);
    unexpectedNotify[nUnexpectedNotify++] = iCode;
}

static BOOL notifies_received(void)
{
    BOOL fRet = (nExpectedNotify == nReceivedNotify);
    nExpectedNotify = nReceivedNotify = 0;
    nUnexpectedNotify = 0;
    return fRet;
}

static LONG addItem(HWND hdex, int idx, LPSTR text)
{
    HDITEMA hdItem;
    hdItem.mask       = HDI_TEXT | HDI_WIDTH;
    hdItem.cxy        = 100;
    hdItem.pszText    = text;
    hdItem.cchTextMax = 0;
    return (LONG)SendMessage(hdex, HDM_INSERTITEMA, (WPARAM)idx, (LPARAM)&hdItem);
}

static LONG setItem(HWND hdex, int idx, LPSTR text, BOOL fCheckNotifies)
{
    LONG ret;
    HDITEMA hdexItem;
    hdexItem.mask       = HDI_TEXT;
    hdexItem.pszText    = text;
    hdexItem.cchTextMax = 0;
    if (fCheckNotifies)
    {
        expect_notify(HDN_ITEMCHANGINGA, FALSE, &hdexItem);
        expect_notify(HDN_ITEMCHANGEDA, FALSE, &hdexItem);
    }
    ret = (LONG)SendMessage(hdex, HDM_SETITEMA, (WPARAM)idx, (LPARAM)&hdexItem);
    if (fCheckNotifies)
        ok(notifies_received(), "setItem(): not all expected notifies were received\n");
    return ret;
}

static LONG setItemUnicodeNotify(HWND hdex, int idx, LPSTR text, LPWSTR wText)
{
    LONG ret;
    HDITEMA hdexItem;
    HDITEMW hdexNotify;
    hdexItem.mask       = HDI_TEXT;
    hdexItem.pszText    = text;
    hdexItem.cchTextMax = 0;
    
    hdexNotify.mask    = HDI_TEXT;
    hdexNotify.pszText = wText;
    
    expect_notify(HDN_ITEMCHANGINGW, TRUE, (HDITEMA*)&hdexNotify);
    expect_notify(HDN_ITEMCHANGEDW, TRUE, (HDITEMA*)&hdexNotify);
    ret = (LONG)SendMessage(hdex, HDM_SETITEMA, (WPARAM)idx, (LPARAM)&hdexItem);
    ok(notifies_received(), "setItemUnicodeNotify(): not all expected notifies were received\n");
    return ret;
}

static LONG delItem(HWND hdex, int idx)
{
    return (LONG)SendMessage(hdex, HDM_DELETEITEM, (WPARAM)idx, 0);
}

static LONG getItemCount(HWND hdex)
{
    return (LONG)SendMessage(hdex, HDM_GETITEMCOUNT, 0, 0);
}

static LONG getItem(HWND hdex, int idx, LPSTR textBuffer)
{
    HDITEMA hdItem;
    hdItem.mask         = HDI_TEXT;
    hdItem.pszText      = textBuffer;
    hdItem.cchTextMax   = MAX_CHARS;
    return (LONG)SendMessage(hdex, HDM_GETITEMA, (WPARAM)idx, (LPARAM)&hdItem);
}

static void addReadDelItem(HWND hdex, HDITEMA *phdiCreate, int maskRead, HDITEMA *phdiRead)
{
    ok(SendMessage(hdex, HDM_INSERTITEMA, 0, (LPARAM)phdiCreate)!=-1, "Adding item failed\n");
    ZeroMemory(phdiRead, sizeof(HDITEMA));
    phdiRead->mask = maskRead;
    ok(SendMessage(hdex, HDM_GETITEMA, 0, (LPARAM)phdiRead)!=0, "Getting item data failed\n");
    ok(SendMessage(hdex, HDM_DELETEITEM, 0, 0)!=0, "Deleting item failed\n");
}

static HWND create_header_control (void)
{
    HWND handle;
    HDLAYOUT hlayout;
    RECT rectwin;
    WINDOWPOS winpos;

    handle = CreateWindowEx(0, WC_HEADER, NULL,
			    WS_CHILD|WS_BORDER|WS_VISIBLE|HDS_BUTTONS|HDS_HORZ,
			    0, 0, 0, 0,
			    hHeaderParentWnd, NULL, NULL, NULL);
    assert(handle);

    if (winetest_interactive)
	ShowWindow (hHeaderParentWnd, SW_SHOW);

    GetClientRect(hHeaderParentWnd,&rectwin);
    hlayout.prc = &rectwin;
    hlayout.pwpos = &winpos;
    SendMessageA(handle,HDM_LAYOUT,0,(LPARAM) &hlayout);
    SetWindowPos(handle, winpos.hwndInsertAfter, winpos.x, winpos.y, 
                 winpos.cx, winpos.cy, 0);

    return handle;
}

static void compare_items(INT iCode, HDITEMA *hdi1, HDITEMA *hdi2, BOOL fUnicode)
{
    ok(hdi1->mask == hdi2->mask, "Notify %d mask mismatch (%08x != %08x)\n", iCode, hdi1->mask, hdi2->mask);
    if (hdi1->mask & HDI_WIDTH)
    {
        ok(hdi1->cxy == hdi2->cxy, "Notify %d cxy mismatch (%08x != %08x)\n", iCode, hdi1->cxy, hdi2->cxy);
    }
    if (hdi1->mask & HDI_TEXT)
    {
        if (hdi1->pszText == LPSTR_TEXTCALLBACKA)
        {
            ok(hdi1->pszText == LPSTR_TEXTCALLBACKA, "Notify %d - only one item is LPSTR_TEXTCALLBACK\n", iCode);
        }
        else
        if (fUnicode)
        {
            char buf1[260];
            char buf2[260];
            WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)hdi1->pszText, -1, buf1, 260, NULL, NULL);
            WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)hdi2->pszText, -1, buf2, 260, NULL, NULL);
            ok(lstrcmpW((LPWSTR)hdi1->pszText, (LPWSTR)hdi2->pszText)==0,
                "Notify %d text mismatch (L\"%s\" vs L\"%s\")\n",
                    iCode, buf1, buf2);
        }
        else
        {
            ok(strcmp(hdi1->pszText, hdi2->pszText)==0,
                "Notify %d text mismatch (\"%s\" vs \"%s\")\n",
                    iCode, hdi1->pszText, hdi2->pszText);
            }
    }
}

static char pszFirstItem[]      = "First Item";
static char pszSecondItem[]     = "Second Item";
static char pszThirdItem[]      = "Third Item";
static char pszFourthItem[]     = "Fourth Item";
static char pszReplaceItem[]    = "Replace Item";
static char pszOutOfRangeItem[] = "Out Of Range Item";

static char *str_items[] =
    {pszFirstItem, pszSecondItem, pszThirdItem, pszFourthItem, pszReplaceItem, pszOutOfRangeItem};
    
static char pszUniTestA[]  = "TST";
static WCHAR pszUniTestW[] = {'T','S','T',0};


#define TEST_GET_ITEM(i,c)\
{   res = getItem(hWndHeader, i, buffer);\
    ok(res != 0, "Getting item[%d] using valid index failed unexpectedly (%d)\n", i, res);\
    ok(strcmp(str_items[c], buffer) == 0, "Getting item[%d] returned \"%s\" expecting \"%s\"\n", i, buffer, str_items[c]);\
}

#define TEST_GET_ITEMCOUNT(i)\
{   res = getItemCount(hWndHeader);\
    ok(res == i, "Got Item Count as %d\n", res);\
}

struct subclass_info
{
    WNDPROC oldproc;
};

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
    add_message(sequences, HEADER_SEQ_INDEX, &msg);

    defwndproc_counter++;
    ret = CallWindowProcA(info->oldproc, hwnd, message, wParam, lParam);
    defwndproc_counter--;

    return ret;
}

static LRESULT WINAPI parent_wnd_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static LONG defwndproc_counter = 0;
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
    cls.hCursor = LoadCursorA(0, IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "Header test parent class";
    return RegisterClassA(&cls);
}

static HWND create_custom_parent_window(void)
{
    if (!register_parent_wnd_class())
        return NULL;

    return CreateWindowExA(0, "Header test parent class", "Header Message Sequence Testing",
                           WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                           672+2*GetSystemMetrics(SM_CXSIZEFRAME),
                           226+GetSystemMetrics(SM_CYCAPTION)+2*GetSystemMetrics(SM_CYSIZEFRAME),
                           NULL, NULL, GetModuleHandleA(NULL), 0);
}

static HWND create_custom_header_control(HWND hParent, BOOL preloadHeaderItems)
{
    struct subclass_info *info;
    HWND childHandle;
    HDLAYOUT hlayout;
    RECT rectwin;
    WINDOWPOS winpos;
    int retVal;
    int loopcnt;
    static char firstHeaderItem[] = "Name";
    static char secondHeaderItem[] = "Size";
    static char *items[] = {secondHeaderItem, firstHeaderItem};
    HDITEM hdItem;
    hdItem.mask = HDI_TEXT | HDI_WIDTH | HDI_FORMAT;
    hdItem.fmt = HDF_LEFT;
    hdItem.cxy = 80;
    hdItem.cchTextMax = 260;


    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    info = HeapAlloc(GetProcessHeap(), 0, sizeof(struct subclass_info));
    if (!info)
         return NULL;

    childHandle = CreateWindowEx(0, WC_HEADER, NULL,
                           WS_CHILD|WS_BORDER|WS_VISIBLE|HDS_BUTTONS|HDS_HORZ,
                           0, 0, 0, 0,
                           hParent, NULL, NULL, NULL);
    assert(childHandle);
    if (preloadHeaderItems)
    {
         for ( loopcnt = 0 ; loopcnt < 2 ; loopcnt++ )
         {
             hdItem.pszText = items[loopcnt];
             retVal = SendMessage(childHandle, HDM_INSERTITEM, loopcnt, (LPARAM) &hdItem);
             ok(retVal == loopcnt, "Adding item %d failed with return value %d\n", ( loopcnt + 1 ), retVal);
          }
    }

    if (winetest_interactive)
       ShowWindow (hParent, SW_SHOW);

    GetClientRect(hParent,&rectwin);
    hlayout.prc = &rectwin;
    hlayout.pwpos = &winpos;
    SendMessageA(childHandle,HDM_LAYOUT,0,(LPARAM) &hlayout);
    SetWindowPos(childHandle, winpos.hwndInsertAfter, winpos.x, winpos.y,
                 winpos.cx, winpos.cy, 0);

    info->oldproc = (WNDPROC)SetWindowLongPtrA(childHandle, GWLP_WNDPROC,
                                               (LONG_PTR)header_subclass_proc);
    SetWindowLongPtrA(childHandle, GWLP_USERDATA, (LONG_PTR)info);
    return childHandle;
}

static void check_auto_format(void)
{
    HDITEMA hdiCreate;
    HDITEMA hdiRead;
    static CHAR text[] = "Test";
    ZeroMemory(&hdiCreate, sizeof(HDITEMA));

    /* Windows implicitly sets some format bits in INSERTITEM */

    /* HDF_STRING is automatically set and cleared for no text */
    hdiCreate.mask = HDI_TEXT|HDI_WIDTH|HDI_FORMAT;
    hdiCreate.pszText = text;
    hdiCreate.cxy = 100;
    hdiCreate.fmt=HDF_CENTER;
    addReadDelItem(hWndHeader, &hdiCreate, HDI_FORMAT, &hdiRead);
    ok(hdiRead.fmt == (HDF_STRING|HDF_CENTER), "HDF_STRING not set automatically (fmt=%x)\n", hdiRead.fmt);

    hdiCreate.mask = HDI_WIDTH|HDI_FORMAT;
    hdiCreate.pszText = text;
    hdiCreate.fmt = HDF_CENTER|HDF_STRING;
    addReadDelItem(hWndHeader, &hdiCreate, HDI_FORMAT, &hdiRead);
    ok(hdiRead.fmt == (HDF_CENTER), "HDF_STRING should be automatically cleared (fmt=%x)\n", hdiRead.fmt);

    /* HDF_BITMAP is automatically set and cleared for a NULL bitmap or no bitmap */
    hdiCreate.mask = HDI_BITMAP|HDI_WIDTH|HDI_FORMAT;
    hdiCreate.hbm = CreateBitmap(16, 16, 1, 8, NULL);
    hdiCreate.fmt = HDF_CENTER;
    addReadDelItem(hWndHeader, &hdiCreate, HDI_FORMAT, &hdiRead);
    ok(hdiRead.fmt == (HDF_BITMAP|HDF_CENTER), "HDF_BITMAP not set automatically (fmt=%x)\n", hdiRead.fmt);
    DeleteObject(hdiCreate.hbm);

    hdiCreate.hbm = NULL;
    hdiCreate.fmt = HDF_CENTER|HDF_BITMAP;
    addReadDelItem(hWndHeader, &hdiCreate, HDI_FORMAT, &hdiRead);
    ok(hdiRead.fmt == HDF_CENTER, "HDF_BITMAP not cleared automatically for NULL bitmap (fmt=%x)\n", hdiRead.fmt);

    hdiCreate.mask = HDI_WIDTH|HDI_FORMAT;
    hdiCreate.fmt = HDF_CENTER|HDF_BITMAP;
    addReadDelItem(hWndHeader, &hdiCreate, HDI_FORMAT, &hdiRead);
    ok(hdiRead.fmt == HDF_CENTER, "HDF_BITMAP not cleared automatically for no bitmap (fmt=%x)\n", hdiRead.fmt);

    /* HDF_IMAGE is automatically set but not cleared */
    hdiCreate.mask = HDI_IMAGE|HDI_WIDTH|HDI_FORMAT;
    hdiCreate.iImage = 17;
    addReadDelItem(hWndHeader, &hdiCreate, HDI_FORMAT, &hdiRead);
    ok(hdiRead.fmt == (HDF_IMAGE|HDF_CENTER), "HDF_IMAGE not set automatically (fmt=%x)\n", hdiRead.fmt);

    hdiCreate.mask = HDI_WIDTH|HDI_FORMAT;
    hdiCreate.fmt = HDF_CENTER|HDF_IMAGE;
    hdiCreate.iImage = 0;
    addReadDelItem(hWndHeader, &hdiCreate, HDI_FORMAT, &hdiRead);
    ok(hdiRead.fmt == (HDF_CENTER|HDF_IMAGE), "HDF_IMAGE shouldn't be cleared automatically (fmt=%x)\n", hdiRead.fmt);
}

static void check_auto_fields(void)
{
    HDITEMA hdiCreate;
    HDITEMA hdiRead;
    static CHAR text[] = "Test";
    LONG res;

    /* Windows stores the format, width, lparam even if they are not in the item's mask */
    ZeroMemory(&hdiCreate, sizeof(HDITEMA));
    hdiCreate.mask = HDI_TEXT;
    hdiCreate.cxy = 100;
    hdiCreate.pszText = text;
    addReadDelItem(hWndHeader, &hdiCreate, HDI_WIDTH, &hdiRead);
    TEST_GET_ITEMCOUNT(6);
    ok(hdiRead.cxy == hdiCreate.cxy, "cxy should be automatically set\n");

    ZeroMemory(&hdiCreate, sizeof(HDITEMA));
    hdiCreate.mask = HDI_TEXT;
    hdiCreate.pszText = text;
    hdiCreate.lParam = 0x12345678;
    addReadDelItem(hWndHeader, &hdiCreate, HDI_LPARAM, &hdiRead);
    TEST_GET_ITEMCOUNT(6);
    ok(hdiRead.lParam == hdiCreate.lParam, "lParam should be automatically set\n");

    ZeroMemory(&hdiCreate, sizeof(HDITEMA));
    hdiCreate.mask = HDI_TEXT;
    hdiCreate.pszText = text;
    hdiCreate.fmt = HDF_STRING|HDF_CENTER;
    addReadDelItem(hWndHeader, &hdiCreate, HDI_FORMAT, &hdiRead);
    TEST_GET_ITEMCOUNT(6);
    ok(hdiRead.fmt == hdiCreate.fmt, "fmt should be automatically set\n");

    /* others fields are not set */
    ZeroMemory(&hdiCreate, sizeof(HDITEMA));
    hdiCreate.mask = HDI_TEXT;
    hdiCreate.pszText = text;
    hdiCreate.hbm = CreateBitmap(16, 16, 1, 8, NULL);
    addReadDelItem(hWndHeader, &hdiCreate, HDI_BITMAP, &hdiRead);
    TEST_GET_ITEMCOUNT(6);
    ok(hdiRead.hbm == NULL, "hbm should not be automatically set\n");
    DeleteObject(hdiCreate.hbm);

    ZeroMemory(&hdiCreate, sizeof(HDITEMA));
    hdiCreate.mask = HDI_IMAGE;
    hdiCreate.iImage = 17;
    hdiCreate.pszText = text;
    addReadDelItem(hWndHeader, &hdiCreate, HDI_TEXT, &hdiRead);
    TEST_GET_ITEMCOUNT(6);
    ok(hdiRead.pszText==NULL, "pszText shouldn't be automatically set\n");

    /* field from comctl >4.0 not tested as the system probably won't touch them */
}

static void check_mask(void)
{
    HDITEMA hdi;
    static CHAR text[] = "ABC";
    LRESULT ret;

    /* don't create items if the mask is zero */
    ZeroMemory(&hdi, sizeof(hdi));
    hdi.mask = 0;
    hdi.cxy = 200;
    hdi.pszText = text;
    hdi.fmt = 0;
    hdi.iOrder = 0;
    hdi.lParam = 17;
    hdi.cchTextMax = 260;
    ret = SendMessage(hWndHeader, HDM_INSERTITEM, 0, (LPARAM)&hdi);
    ok(ret == -1, "Creating an item with a zero mask should have failed\n");
    if (ret != -1) SendMessage(hWndHeader, HDM_DELETEITEM, 0, 0);

    /* with a non-zero mask creation will succeed */
    ZeroMemory(&hdi, sizeof(hdi));
    hdi.mask = HDI_LPARAM;
    ret = SendMessage(hWndHeader, HDM_INSERTITEM, 0, (LPARAM)&hdi);
    ok(ret != -1, "Adding item with non-zero mask failed\n");
    if (ret != -1)
        SendMessage(hWndHeader, HDM_DELETEITEM, 0, 0);

    /* in SETITEM if the mask contains a unknown bit, it is ignored */
    ZeroMemory(&hdi, sizeof(hdi));
    hdi.mask = 0x08000000 | HDI_LPARAM | HDI_IMAGE;
    hdi.lParam = 133;
    hdi.iImage = 17;
    ret = SendMessage(hWndHeader, HDM_INSERTITEM, 0, (LPARAM)&hdi);
    ok(ret != -1, "Adding item failed\n");

    if (ret != -1)
    {
        /* check result */
        ZeroMemory(&hdi, sizeof(hdi));
        hdi.mask = HDI_LPARAM | HDI_IMAGE;
        SendMessage(hWndHeader, HDM_GETITEM, 0, (LPARAM)&hdi);
        ok(hdi.lParam == 133, "comctl32 4.0 field not set\n");
        ok(hdi.iImage == 17, "comctl32 >4.0 field not set\n");

        /* but in GETITEM if an unknown bit is set, comctl32 uses only version 4.0 fields */
        ZeroMemory(&hdi, sizeof(hdi));
        hdi.mask = 0x08000000 | HDI_LPARAM | HDI_IMAGE;
        SendMessage(hWndHeader, HDM_GETITEM, 0, (LPARAM)&hdi);
        ok(hdi.lParam == 133, "comctl32 4.0 field not read\n");
        ok(hdi.iImage == 0, "comctl32 >4.0 field shouldn't be read\n");

        SendMessage(hWndHeader, HDM_DELETEITEM, 0, 0);
    }
}

static void test_header_control (void)
{
    LONG res;
    static char buffer[MAX_CHARS];
    int i;

    hWndHeader = create_header_control ();

    for (i = 3; i >= 0; i--)
    {
        TEST_GET_ITEMCOUNT(3-i);
        res = addItem(hWndHeader, 0, str_items[i]);
        ok(res == 0, "Adding simple item failed (%d)\n", res);
    }

    TEST_GET_ITEMCOUNT(4);
    res = addItem(hWndHeader, 99, str_items[i+1]);
    ok(res != -1, "Adding Out of Range item should fail with -1 got (%d)\n", res);
    TEST_GET_ITEMCOUNT(5);
    res = addItem(hWndHeader, 5, str_items[i+1]);
    ok(res != -1, "Adding Out of Range item should fail with -1 got (%d)\n", res);
    TEST_GET_ITEMCOUNT(6);

    for (i = 0; i < 4; i++) { TEST_GET_ITEM(i,i); TEST_GET_ITEMCOUNT(6); }

    res=getItem(hWndHeader, 99, buffer);
    ok(res == 0, "Getting Out of Range item should fail with 0 (%d), got %s\n", res,buffer);
    res=getItem(hWndHeader, 5, buffer);
    ok(res == 1, "Getting Out of Range item should fail with 1 (%d), got %s\n", res,buffer);
    res=getItem(hWndHeader, -2, buffer);
    ok(res == 0, "Getting Out of Range item should fail with 0 (%d), got %s\n", res,buffer);

    if (winetest_interactive)
    {
        UpdateWindow(hHeaderParentWnd);
        UpdateWindow(hWndHeader);
    }

    TEST_GET_ITEMCOUNT(6);
    res=setItem(hWndHeader, 99, str_items[5], FALSE);
    ok(res == 0, "Setting Out of Range item should fail with 0 (%d)\n", res);
    res=setItem(hWndHeader, 5, str_items[5], TRUE);
    ok(res == 1, "Setting Out of Range item should fail with 1 (%d)\n", res);
    res=setItem(hWndHeader, -2, str_items[5], FALSE);
    ok(res == 0, "Setting Out of Range item should fail with 0 (%d)\n", res);
    TEST_GET_ITEMCOUNT(6);

    for (i = 0; i < 4; i++)
    {
        res = setItem(hWndHeader, i, str_items[4], TRUE);
        ok(res != 0, "Setting %d item failed (%d)\n", i+1, res);
        TEST_GET_ITEM(i, 4);
        TEST_GET_ITEMCOUNT(6);
    }
    
    SendMessageA(hWndHeader, HDM_SETUNICODEFORMAT, (WPARAM)TRUE, 0);
    setItemUnicodeNotify(hWndHeader, 3, pszUniTestA, pszUniTestW);
    SendMessageA(hWndHeader, WM_NOTIFYFORMAT, (WPARAM)hHeaderParentWnd, (LPARAM)NF_REQUERY);
    setItem(hWndHeader, 3, str_items[4], TRUE);
    
    dont_expect_notify(HDN_GETDISPINFOA);
    dont_expect_notify(HDN_GETDISPINFOW);
    addItem(hWndHeader, 0, LPSTR_TEXTCALLBACKA);
    setItem(hWndHeader, 0, str_items[4], TRUE);
    /* unexpected notifies cleared by notifies_received in setItem */
    dont_expect_notify(HDN_GETDISPINFOA);
    dont_expect_notify(HDN_GETDISPINFOW);
    setItem(hWndHeader, 0, LPSTR_TEXTCALLBACKA, TRUE);
    /* unexpected notifies cleared by notifies_received in setItem */
    delItem(hWndHeader, 0);

    check_auto_format();
    TEST_GET_ITEMCOUNT(6);
    check_auto_fields();
    TEST_GET_ITEMCOUNT(6);
    check_mask();
    TEST_GET_ITEMCOUNT(6);

    res = delItem(hWndHeader, 5);
    ok(res == 1, "Deleting Out of Range item should fail with 1 (%d)\n", res);
    res = delItem(hWndHeader, -2);
    ok(res == 0, "Deleting Out of Range item should fail with 0 (%d)\n", res);
    TEST_GET_ITEMCOUNT(5);

    res = delItem(hWndHeader, 3);
    ok(res != 0, "Deleting using out of range index failed (%d)\n", res);
    TEST_GET_ITEMCOUNT(4);
    res = delItem(hWndHeader, 0);
    ok(res != 0, "Deleting using out of range index failed (%d)\n", res);
    TEST_GET_ITEMCOUNT(3);
    res = delItem(hWndHeader, 0);
    ok(res != 0, "Deleting using out of range index failed (%d)\n", res);
    TEST_GET_ITEMCOUNT(2);
    res = delItem(hWndHeader, 0);
    ok(res != 0, "Deleting using out of range index failed (%d)\n", res);
    TEST_GET_ITEMCOUNT(1);

    DestroyWindow(hWndHeader);
}

static void test_hdm_getitemrect(HWND hParent)
{

    HWND hChild;
    RECT rect;
    int retVal;

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    hChild = create_custom_header_control(hParent, TRUE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, add_header_to_parent_seq,
                                    "adder header control to parent", FALSE);

    retVal = SendMessage(hChild, HDM_GETITEMRECT, 1, (LPARAM) &rect);
    ok(retVal == TRUE, "Getting item rect should TRUE, got %d\n", retVal);
    /* check bounding rectangle information of 2nd header item */
    expect(80, rect.left);
    expect(0, rect.top);
    expect(160, rect.right);
    todo_wine
    {
      expect(g_customheight, rect.bottom);
    }
    retVal = SendMessage(hChild, HDM_GETITEMRECT, 0, (LPARAM) &rect);

    ok(retVal == TRUE, "Getting item rect should TRUE, got %d\n", retVal);
    /* check bounding rectangle information of 1st header item */
    expect(0, rect.left);
    expect(0, rect.top);

    expect(80, rect.right);
    todo_wine
    {
      expect(g_customheight, rect.bottom);
    }
    retVal = SendMessage(hChild, HDM_GETITEMRECT, 10, (LPARAM) &rect);
    ok(retVal == 0, "Getting rect of nonexistent item should return 0, got %d\n", retVal);

    ok_sequence(sequences, HEADER_SEQ_INDEX, getItemRect_seq, "getItemRect sequence testing", FALSE);
    DestroyWindow(hChild);
}

static void test_hdm_layout(HWND hParent)
{
    HWND hChild;
    int retVal;
    RECT rect;
    HDLAYOUT hdLayout;
    WINDOWPOS windowPos;
    hdLayout.prc = &rect;
    hdLayout.pwpos = &windowPos;

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    hChild = create_custom_header_control(hParent, TRUE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, add_header_to_parent_seq,
                                    "adder header control to parent", FALSE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    retVal = SendMessage(hChild, HDM_LAYOUT, 0, (LPARAM) &hdLayout);
    expect(TRUE, retVal);

    ok_sequence(sequences, HEADER_SEQ_INDEX, layout_seq, "layout sequence testing", FALSE);

    DestroyWindow(hChild);
}

static void test_hdm_ordertoindex(HWND hParent)
{
    HWND hChild;
    int retVal;

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    hChild = create_custom_header_control(hParent, TRUE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, add_header_to_parent_seq,
                                    "adder header control to parent", FALSE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    retVal = SendMessage(hChild, HDM_ORDERTOINDEX, 1, 0);
    expect(1, retVal);

    ok_sequence(sequences, HEADER_SEQ_INDEX, orderToIndex_seq, "orderToIndex sequence testing", FALSE);
    DestroyWindow(hChild);
}

static void test_hdm_hittest(HWND hParent)
{
    HWND hChild;
    int retVal;
    POINT pt;
    HDHITTESTINFO hdHitTestInfo;
    const int firstItemRightBoundary = 80;
    const int secondItemRightBoundary = 160;
    const int bottomBoundary = g_customheight;

    pt.x = firstItemRightBoundary - 1;
    pt.y = bottomBoundary - 1;
    hdHitTestInfo.pt = pt;


    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    hChild = create_custom_header_control(hParent, TRUE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, add_header_to_parent_seq,
                                    "adder header control to parent", FALSE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    retVal = SendMessage(hChild, HDM_HITTEST, 0, (LPARAM) &hdHitTestInfo);
    todo_wine
    {
      expect(0, retVal);
      expect(0, hdHitTestInfo.iItem);
    }

    pt.x = secondItemRightBoundary - 1;
    pt.y = bottomBoundary - 1;
    hdHitTestInfo.pt = pt;
    retVal = SendMessage(hChild, HDM_HITTEST, 1, (LPARAM) &hdHitTestInfo);
    todo_wine
    {
      expect(1, retVal);
    }
    expect(1, hdHitTestInfo.iItem);

    pt.x = secondItemRightBoundary;
    pt.y = bottomBoundary + 1;
    hdHitTestInfo.pt = pt;
    todo_wine
    {
     retVal = SendMessage(hChild, HDM_HITTEST, 0, (LPARAM) &hdHitTestInfo);
     expect(-1, retVal);
    }

    ok_sequence(sequences, HEADER_SEQ_INDEX, hittest_seq, "hittest sequence testing", FALSE);

    DestroyWindow(hChild);
}

static void test_hdm_sethotdivider(HWND hParent)
{
    HWND hChild;
    int retVal;
    /*  low word: x coordinate = 5
     *  high word:  y coordinate = 5
     */

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    hChild = create_custom_header_control(hParent, TRUE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, add_header_to_parent_seq,
                                    "adder header control to parent", FALSE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    todo_wine
    {
        retVal = SendMessage(hChild, HDM_SETHOTDIVIDER, TRUE, 0X00050005);
        expect(0, retVal);
    }
    retVal = SendMessage(hChild, HDM_SETHOTDIVIDER, FALSE, 100);
    expect(100, retVal);
    retVal = SendMessage(hChild, HDM_SETHOTDIVIDER, FALSE, 1);
    expect(1, retVal);
    if (winetest_interactive)
       ok_sequence(sequences, HEADER_SEQ_INDEX, setHotDivider_seq_interactive,
                   "setHotDivider sequence testing", TRUE);
    else
       ok_sequence(sequences, HEADER_SEQ_INDEX, setHotDivider_seq_noninteractive,
                   "setHotDivider sequence testing", FALSE);

    DestroyWindow(hChild);
}

static void test_hdm_imageMessages(HWND hParent)
{
    HIMAGELIST hImageList = ImageList_Create (4, 4, 0, 1, 0);
    HIMAGELIST hImageListRetVal;
    HWND hChild;

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    hChild = create_custom_header_control(hParent, TRUE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, add_header_to_parent_seq,
                                    "adder header control to parent", FALSE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    hImageListRetVal = (HIMAGELIST) SendMessage(hChild, HDM_SETIMAGELIST, 0, (LPARAM) hImageList);
    ok(hImageListRetVal == NULL, "Expected NULL, got %p\n", hImageListRetVal);

    hImageListRetVal = (HIMAGELIST) SendMessage(hChild, HDM_GETIMAGELIST, 0, 0);
    ok(hImageListRetVal != NULL, "Expected non-NULL handle, got %p\n", hImageListRetVal);

    hImageListRetVal = (HIMAGELIST) SendMessage(hChild, HDM_CREATEDRAGIMAGE, 0, 0);
    ok(hImageListRetVal != NULL, "Expected non-NULL handle, got %p\n", hImageListRetVal);

    ok_sequence(sequences, HEADER_SEQ_INDEX, imageMessages_seq, "imageMessages sequence testing", FALSE);

    DestroyWindow(hChild);
}

static void test_hdm_filterMessages(HWND hParent)
{
    HWND hChild;
    int retVal, timeout;

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    hChild = create_custom_header_control(hParent, TRUE);
    assert(hChild);
    ok_sequence(sequences, PARENT_SEQ_INDEX, add_header_to_parent_seq,
                                    "adder header control to parent", FALSE);

    timeout = SendMessage(hChild, HDM_SETFILTERCHANGETIMEOUT, 1, 100);
    SendMessage(hChild, HDM_SETFILTERCHANGETIMEOUT, 1, timeout);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    /* msdn incorrectly states that return value
     * is the index of the filter control being
     * modified. The sendMessage here should
     * return previous filter timeout value
     */

    retVal = SendMessage(hChild, HDM_SETFILTERCHANGETIMEOUT, 1, 100);
    expect(timeout, retVal);

    todo_wine
    {
        retVal = SendMessage(hChild, HDM_CLEARFILTER, 0, 1);
        expect(1, retVal);
        retVal = SendMessage(hChild, HDM_EDITFILTER, 1, 0);
        expect(1, retVal);
    }
    if (winetest_interactive)
         ok_sequence(sequences, HEADER_SEQ_INDEX, filterMessages_seq_interactive,
                     "filterMessages sequence testing", TRUE);
    else
         ok_sequence(sequences, HEADER_SEQ_INDEX, filterMessages_seq_noninteractive,
                     "filterMessages sequence testing", FALSE);
    /* Some Win9x versions don't send a WM_KILLFOCUS.
     * Set the focus explicitly to the parent to avoid a crash.
     */
    SetFocus(hParent);
    DestroyWindow(hChild);

}

static void test_hdm_unicodeformatMessages(HWND hParent)
{
    HWND hChild;
    int retVal;

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    hChild = create_custom_header_control(hParent, TRUE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, add_header_to_parent_seq,
                                    "adder header control to parent", FALSE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    retVal = SendMessage(hChild, HDM_SETUNICODEFORMAT, TRUE, 0);
    expect(0, retVal);
    retVal = SendMessage(hChild, HDM_GETUNICODEFORMAT, 0, 0);
    expect(1, retVal);

    ok_sequence(sequences, HEADER_SEQ_INDEX, unicodeformatMessages_seq,
                     "unicodeformatMessages sequence testing", FALSE);
    DestroyWindow(hChild);
}

static void test_hdm_bitmapmarginMessages(HWND hParent)
{
    HWND hChild;
    int retVal;

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    hChild = create_custom_header_control(hParent, TRUE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, add_header_to_parent_seq,
                                    "adder header control to parent", FALSE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    retVal = SendMessage(hChild, HDM_GETBITMAPMARGIN, 0, 0);
    expect(6, retVal);

    ok_sequence(sequences, HEADER_SEQ_INDEX, bitmapmarginMessages_seq,
                      "bitmapmarginMessages sequence testing", FALSE);
    DestroyWindow(hChild);
}

static void test_hdm_index_messages(HWND hParent)
{

    HWND hChild;
    int retVal;
    int loopcnt;
    int strcmpResult;
    int iSize;
    static const int lpiarray[2] = {1, 0};
    static int lpiarrayReceived[2];
    static char firstHeaderItem[] = "Name";
    static char secondHeaderItem[] = "Size";
    static char thirdHeaderItem[] = "Type";
    static char fourthHeaderItem[] = "Date Modified";
    static char *items[] = {firstHeaderItem, secondHeaderItem, thirdHeaderItem, fourthHeaderItem};
    HDITEM hdItem;
    hdItem.mask = HDI_TEXT | HDI_WIDTH | HDI_FORMAT;
    hdItem.fmt = HDF_LEFT;
    hdItem.cxy = 80;
    hdItem.cchTextMax = 260;

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    hChild = create_custom_header_control(hParent, FALSE);
    if (winetest_interactive)
         ok_sequence(sequences, PARENT_SEQ_INDEX, add_header_to_parent_seq_interactive,
                                              "adder header control to parent", TRUE);
    else
         ok_sequence(sequences, PARENT_SEQ_INDEX, add_header_to_parent_seq,
                                     "adder header control to parent", FALSE);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    for ( loopcnt = 0 ; loopcnt < 4 ; loopcnt++ )
    {
      hdItem.pszText = items[loopcnt];
      retVal = SendMessage(hChild, HDM_INSERTITEM, loopcnt, (LPARAM) &hdItem);
      ok(retVal == loopcnt, "Adding item %d failed with return value %d\n", ( loopcnt + 1 ), retVal);
    }
    ok_sequence(sequences, HEADER_SEQ_INDEX, insertItem_seq, "insertItem sequence testing", FALSE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    retVal = SendMessage(hChild, HDM_DELETEITEM, 3, (LPARAM) &hdItem);
    ok(retVal == TRUE, "Deleting item 3 should return TRUE, got %d\n", retVal);
    retVal = SendMessage(hChild, HDM_GETITEMCOUNT, 0, (LPARAM) &hdItem);
    ok(retVal == 3, "Getting item count should return 3, got %d\n", retVal);

    retVal = SendMessage(hChild, HDM_DELETEITEM, 3, (LPARAM) &hdItem);
    ok(retVal == FALSE, "Deleting already-deleted item should return FALSE, got %d\n", retVal);
    retVal = SendMessage(hChild, HDM_GETITEMCOUNT, 0, (LPARAM) &hdItem);
    ok(retVal == 3, "Getting item count should return 3, got %d\n", retVal);

    retVal = SendMessage(hChild, HDM_DELETEITEM, 2, (LPARAM) &hdItem);
    ok(retVal == TRUE, "Deleting item 2 should return TRUE, got %d\n", retVal);
    retVal = SendMessage(hChild, HDM_GETITEMCOUNT, 0, (LPARAM) &hdItem);
    ok(retVal == 2, "Getting item count should return 2, got %d\n", retVal);

    ok_sequence(sequences, HEADER_SEQ_INDEX, deleteItem_getItemCount_seq,
                         "deleteItem_getItemCount sequence testing", FALSE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    retVal = SendMessage(hChild, HDM_GETITEM, 3, (LPARAM) &hdItem);
    ok(retVal == FALSE, "Getting already-deleted item should return FALSE, got %d\n", retVal);

    retVal = SendMessage(hChild, HDM_GETITEM, 0, (LPARAM) &hdItem);
    ok(retVal == TRUE, "Getting the 1st header item should return TRUE, got %d\n", retVal);

    ok_sequence(sequences, HEADER_SEQ_INDEX, getItem_seq, "getItem sequence testing", FALSE);

    /* check if the item is the right one */
    strcmpResult =  strcmp(hdItem.pszText, firstHeaderItem);
    expect(0, strcmpResult);
    expect(80, hdItem.cxy);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    iSize = SendMessage(hChild, HDM_GETITEMCOUNT, 0, (LPARAM) &hdItem);
    retVal = SendMessage(hChild, HDM_SETORDERARRAY, iSize, (LPARAM) lpiarray);
    ok(retVal == TRUE, "Setting header items order should return TRUE, got %d\n", retVal);

    retVal = SendMessage(hChild, HDM_GETORDERARRAY, iSize, (LPARAM) lpiarrayReceived);
    ok(retVal == TRUE, "Getting header items order should return TRUE, got %d\n", retVal);

    ok_sequence(sequences, HEADER_SEQ_INDEX, orderArray_seq, "set_get_orderArray sequence testing", FALSE);

    /* check if the array order is set correctly and the size of the array is correct. */
    expect(2, iSize);
    expect(lpiarray[0], lpiarrayReceived[0]);
    expect(lpiarray[1], lpiarrayReceived[1]);

    hdItem.mask = HDI_FORMAT;
    hdItem.fmt = HDF_CENTER | HDF_STRING;

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    retVal = SendMessage(hChild, HDM_SETITEM, 0, (LPARAM) &hdItem);
    ok(retVal == TRUE, "Aligning 1st header item to center should return TRUE, got %d\n", retVal);
    hdItem.fmt = HDF_RIGHT | HDF_STRING;
    retVal = SendMessage(hChild, HDM_SETITEM, 1, (LPARAM) &hdItem);
    ok(retVal == TRUE, "Aligning 2nd header item to right should return TRUE, got %d\n", retVal);

    ok_sequence(sequences, HEADER_SEQ_INDEX, setItem_seq, "setItem sequence testing", FALSE);
    DestroyWindow(hChild);
}

#define TEST_NMCUSTOMDRAW(draw_stage, item_spec, lparam, _left, _top, _right, _bottom) \
    ok(nm->dwDrawStage == draw_stage, "Invalid dwDrawStage %d vs %d\n", draw_stage, nm->dwDrawStage); \
    if (item_spec != -1) \
        ok(nm->dwItemSpec == item_spec, "Invalid dwItemSpec %d vs %ld\n", item_spec, nm->dwItemSpec); \
    ok(nm->lItemlParam == lparam, "Invalid lItemlParam %d vs %ld\n", lparam, nm->lItemlParam); \
    ok(nm->rc.top == _top && nm->rc.bottom == _bottom && nm->rc.left == _left && nm->rc.right == _right, \
        "Invalid rect (%d,%d) (%d,%d) vs (%d,%d) (%d,%d)\n", _left, _top, _right, _bottom, \
        nm->rc.left, nm->rc.top, nm->rc.right, nm->rc.bottom);

static LRESULT customdraw_1(int n, NMCUSTOMDRAW *nm)
{
    if (nm == NULL) {  /* test ended */
        ok(n==1, "NM_CUSTOMDRAW messages: %d, expected: 1\n", n);
        return 0;
    }

    switch (n)
    {
    case 0:
        /* don't test dwItemSpec - it's 0 no comctl5 but 1308756 on comctl6 */
        TEST_NMCUSTOMDRAW(CDDS_PREPAINT, -1, 0, 0, 0, 670, g_customheight);
        return 0;
    }

    ok(FALSE, "To many custom draw messages (n=%d, nm->dwDrawStage=%d)\n", n, nm->dwDrawStage);
    return -1;
}

static LRESULT customdraw_2(int n, NMCUSTOMDRAW *nm)
{
    if (nm == NULL) {  /* test ended */
        ok(n==4, "NM_CUSTOMDRAW messages: %d, expected: 4\n", n);
        return 0;
    }

    switch (n)
    {
    case 0:
        TEST_NMCUSTOMDRAW(CDDS_PREPAINT, -1, 0, 0, 0, 670, g_customheight);
        return CDRF_NOTIFYITEMDRAW;
    case 1:
        TEST_NMCUSTOMDRAW(CDDS_ITEMPREPAINT, 0, 0, 0, 0, 50, g_customheight);
        return 0;
    case 2:
        TEST_NMCUSTOMDRAW(CDDS_ITEMPREPAINT, 1, 5, 50, 0, 150, g_customheight);
        return 0;
    case 3:
        TEST_NMCUSTOMDRAW(CDDS_ITEMPREPAINT, 2, 10, 150, 0, 300, g_customheight);
        return 0;
    }

    ok(FALSE, "To many custom draw messages (n=%d, nm->dwDrawStage=%d)\n", n, nm->dwDrawStage);
    return 0;
}

static LRESULT customdraw_3(int n, NMCUSTOMDRAW *nm)
{
    if (nm == NULL) {  /* test ended */
        ok(n==5, "NM_CUSTOMDRAW messages: %d, expected: 5\n", n);
        return 0;
    }

    switch (n)
    {
    case 0:
        TEST_NMCUSTOMDRAW(CDDS_PREPAINT, -1, 0, 0, 0, 670, g_customheight);
        return CDRF_NOTIFYITEMDRAW|CDRF_NOTIFYPOSTERASE|CDRF_NOTIFYPOSTPAINT|CDRF_SKIPDEFAULT;
    case 1:
        TEST_NMCUSTOMDRAW(CDDS_ITEMPREPAINT, 0, 0, 0, 0, 50, g_customheight);
        return 0;
    case 2:
        TEST_NMCUSTOMDRAW(CDDS_ITEMPREPAINT, 1, 5, 50, 0, 150, g_customheight);
        return 0;
    case 3:
        TEST_NMCUSTOMDRAW(CDDS_ITEMPREPAINT, 2, 10, 150, 0, 300, g_customheight);
        return 0;
    case 4:
        TEST_NMCUSTOMDRAW(CDDS_POSTPAINT, -1, 0, 0, 0, 670, g_customheight);
        return 0;
    }

    ok(FALSE, "To many custom draw messages (n=%d, nm->dwDrawStage=%d)\n", n, nm->dwDrawStage);
    return 0;
}


static LRESULT customdraw_4(int n, NMCUSTOMDRAW *nm)
{
    if (nm == NULL) {  /* test ended */
        ok(n==4, "NM_CUSTOMDRAW messages: %d, expected: 4\n", n);
        return 0;
    }

    switch (n)
    {
    case 0:
        TEST_NMCUSTOMDRAW(CDDS_PREPAINT, -1, 0, 0, 0, 670, g_customheight);
        return CDRF_NOTIFYITEMDRAW|CDRF_NOTIFYPOSTPAINT;
    case 1:
        TEST_NMCUSTOMDRAW(CDDS_ITEMPREPAINT, 0, 0, 0, 0, 50, g_customheight);
        return 0;
    case 2:
        TEST_NMCUSTOMDRAW(CDDS_ITEMPREPAINT, 2, 10, 150, 0, 300, g_customheight);
        return 0;
    case 3:
        TEST_NMCUSTOMDRAW(CDDS_POSTPAINT, -1, 0, 0, 0, 670, g_customheight);
        return 0;
    }

    ok(FALSE, "To many custom draw messages (n=%d, nm->dwDrawStage=%d)\n", n, nm->dwDrawStage);
    return 0;
}

static void run_customdraw_scenario(CUSTOMDRAWPROC proc)
{
    g_CustomDrawProc = proc;
    g_CustomDrawCount = 0;
    InvalidateRect(hWndHeader, NULL, TRUE);
    UpdateWindow(hWndHeader);
    proc(g_CustomDrawCount, NULL);
    g_CustomDrawProc = NULL;
}

static void test_customdraw(void)
{
    int i;
    HDITEM item;
    RECT rect;
    CHAR name[] = "Test";
    hWndHeader = create_header_control();
    GetClientRect(hWndHeader, &rect);
    ok(rect.right - rect.left == 670 && rect.bottom - rect.top == g_customheight,
        "Tests will fail as header size is %dx%d instead of 670x%d\n",
        rect.right - rect.left, rect.bottom - rect.top, g_customheight);

    for (i = 0; i < 3; i++)
    {
        ZeroMemory(&item, sizeof(item));
        item.mask = HDI_TEXT|HDI_WIDTH;
        item.cxy = 50*(i+1);
        item.pszText = name;
        item.lParam = i*5;
        SendMessage(hWndHeader, HDM_INSERTITEM, i, (LPARAM)&item);
    }

    run_customdraw_scenario(customdraw_1);
    run_customdraw_scenario(customdraw_2);
    run_customdraw_scenario(customdraw_3);

    ZeroMemory(&item, sizeof(item));
    item.mask = HDI_FORMAT;
    item.fmt = HDF_OWNERDRAW;
    SendMessage(hWndHeader, HDM_SETITEM, 1, (LPARAM)&item);
    g_DrawItem.CtlID = 0;
    g_DrawItem.CtlType = ODT_HEADER;
    g_DrawItem.hwndItem = hWndHeader;
    g_DrawItem.itemID = 1;
    g_DrawItem.itemState = 0;
    SendMessage(hWndHeader, HDM_GETITEMRECT, 1, (LPARAM)&g_DrawItem.rcItem);
    run_customdraw_scenario(customdraw_4);
    ok(g_DrawItemReceived, "WM_DRAWITEM not received\n");
    DestroyWindow(hWndHeader);
    hWndHeader = NULL;
    g_DrawItem.CtlType = 0;
    g_DrawItemReceived = FALSE;
}

static void check_order(const int expected_id[], const int expected_order[],
                        int count, const char *type)
{
    int i;
    HDITEMA hdi;

    ok(getItemCount(hWndHeader) == count, "Invalid item count in order tests\n");
    for (i = 0; i < count; i++)
    {
        hdi.mask = HDI_LPARAM|HDI_ORDER;
        SendMessage(hWndHeader, HDM_GETITEMA, i, (LPARAM)&hdi);
        ok(hdi.lParam == expected_id[i],
            "Invalid item ids after '%s'- item %d has lParam %d\n", type, i, (int)hdi.lParam);
        ok(hdi.iOrder == expected_order[i],
            "Invalid item order after '%s'- item %d has iOrder %d\n", type, i, hdi.iOrder);
    }
}

static void test_header_order (void)
{
    const int rand1[] = {0, 1, 1, 0, 4};
    const int rand2[] = {4, 5, 6, 7, 4};
    const int rand3[] = {5, 5, 1, 6, 1};
    const int rand4[] = {1, 5, 2, 7, 6, 1, 4, 2, 3, 2};
    const int rand5[] = {7, 8, 5, 6, 7, 2, 1, 9, 10, 10};
    const int rand6[] = {2, 8, 3, 4, 0};

    const int ids1[] = {3, 0, 2, 1, 4};
    const int ord1[] = {0, 1, 2, 3, 4};
    const int ids2[] = {3, 9, 7, 0, 2, 1, 4, 8, 6, 5};
    const int ord2[] = {0, 4, 7, 1, 2, 3, 9, 8, 6, 5};
    const int ord3[] = {0, 3, 9, 2, 1, 8, 7, 6, 5, 4};
    const int ids4[] = {9, 0, 1, 8, 6};
    const int ord4[] = {1, 0, 4, 3, 2};

    char buffer[20];
    HDITEMA hdi;
    int i;

    hWndHeader = create_header_control();

    ZeroMemory(&hdi, sizeof(HDITEMA));
    hdi.mask = HDI_TEXT | HDI_LPARAM;
    hdi.pszText = buffer;
    strcpy(buffer, "test");

    for (i = 0; i < 5; i++)
    {
        hdi.lParam = i;
        SendMessage(hWndHeader, HDM_INSERTITEMA, rand1[i], (LPARAM)&hdi);
        rand();
    }
    check_order(ids1, ord1, 5, "insert without iOrder");

    hdi.mask |= HDI_ORDER;
    for (i = 0; i < 5; i++)
    {
        hdi.lParam = i + 5;
        hdi.iOrder = rand2[i];
        SendMessage(hWndHeader, HDM_INSERTITEMA, rand3[i], (LPARAM)&hdi);
        rand(); rand();
    }
    check_order(ids2, ord2, 10, "insert with order");

    hdi.mask = HDI_ORDER;
    for (i=0; i<10; i++)
    {
        hdi.iOrder = rand5[i];
        SendMessage(hWndHeader, HDM_SETITEMA, rand4[i], (LPARAM)&hdi);
        rand(); rand();
    }
    check_order(ids2, ord3, 10, "setitems changing order");

    for (i=0; i<5; i++)
        SendMessage(hWndHeader, HDM_DELETEITEM, rand6[i], 0);
    check_order(ids4, ord4, 5, "deleteitem");

    DestroyWindow(hWndHeader);
}

static LRESULT CALLBACK HeaderTestWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    DRAWITEMSTRUCT *di;
    switch(msg) {

    case WM_NOTIFY:
    {
        NMHEADERA *hdr = (NMHEADER *)lParam;
        EXPECTEDNOTIFY *expected;
        int i;

        if (hdr->hdr.code == NM_CUSTOMDRAW)
            if (g_CustomDrawProc)
                return g_CustomDrawProc(g_CustomDrawCount++, (NMCUSTOMDRAW*)hdr);

        for (i=0; i<nUnexpectedNotify; i++)
            ok(hdr->hdr.code != unexpectedNotify[i], "Received invalid notify %d\n", hdr->hdr.code);
        
        if (nReceivedNotify >= nExpectedNotify || hdr->hdr.hwndFrom != hWndHeader )
            break;

        expected = &expectedNotify[nReceivedNotify];
        if (hdr->hdr.code != expected->iCode)
            break;
        
        nReceivedNotify++;
        compare_items(hdr->hdr.code, &expected->hdItem, hdr->pitem, expected->fUnicode);
        break;
    }

    case WM_DRAWITEM:
        di = (DRAWITEMSTRUCT *)lParam;
        ok(g_DrawItem.CtlType != 0, "Unexpected WM_DRAWITEM\n");
        if (g_DrawItem.CtlType == 0) return 0;
        g_DrawItemReceived = TRUE;
        compare(di->CtlType,   g_DrawItem.CtlType, "%d");
        compare(di->CtlID,     g_DrawItem.CtlID, "%d");
        compare(di->hwndItem,  g_DrawItem.hwndItem, "%p");
        compare(di->itemID,    g_DrawItem.itemID, "%d");
        compare(di->itemState, g_DrawItem.itemState, "%d");
        compare(di->rcItem.left,   g_DrawItem.rcItem.left, "%d");
        compare(di->rcItem.top,    g_DrawItem.rcItem.top, "%d");
        compare(di->rcItem.right,  g_DrawItem.rcItem.right, "%d");
        compare(di->rcItem.bottom, g_DrawItem.rcItem.bottom, "%d");
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
  
    default:
        return DefWindowProcA(hWnd, msg, wParam, lParam);
    }
    
    return 0L;
}

static int init(void)
{
    HMODULE hComctl32;
    BOOL (WINAPI *pInitCommonControlsEx)(const INITCOMMONCONTROLSEX*);
    WNDCLASSA wc;
    INITCOMMONCONTROLSEX iccex;
    TEXTMETRICA tm;
    HFONT hOldFont;
    HDC hdc;

    hComctl32 = GetModuleHandleA("comctl32.dll");
    pInitCommonControlsEx = (void*)GetProcAddress(hComctl32, "InitCommonControlsEx");
    if (!pInitCommonControlsEx)
    {
        skip("InitCommonControlsEx() is missing. Skipping the tests\n");
        return 0;
    }

    iccex.dwSize = sizeof(iccex);
    iccex.dwICC  = ICC_USEREX_CLASSES;
    pInitCommonControlsEx(&iccex);

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.hIcon = NULL;
    wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
    wc.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "HeaderTestClass";
    wc.lpfnWndProc = HeaderTestWndProc;
    RegisterClassA(&wc);

    /* The height of the header control depends on the height of the system font.
       The height of the system font is dpi dependent */
    hdc = GetDC(0);
    hOldFont = SelectObject(hdc, GetStockObject(SYSTEM_FONT));
    GetTextMetricsA(hdc, &tm);
    /* 2 dot extra space are needed for the border */
    g_customheight = tm.tmHeight + 2;
    trace("customdraw height: %d (dpi: %d)\n", g_customheight, GetDeviceCaps(hdc, LOGPIXELSY));
    SelectObject(hdc, hOldFont);
    ReleaseDC(0, hdc);

    hHeaderParentWnd = CreateWindowExA(0, "HeaderTestClass", "Header test", WS_OVERLAPPEDWINDOW, 
      CW_USEDEFAULT, CW_USEDEFAULT, 672+2*GetSystemMetrics(SM_CXSIZEFRAME),
      226+GetSystemMetrics(SM_CYCAPTION)+2*GetSystemMetrics(SM_CYSIZEFRAME),
      NULL, NULL, GetModuleHandleA(NULL), 0);
    assert(hHeaderParentWnd != NULL);
    ShowWindow(hHeaderParentWnd, SW_SHOW);
    return 1;
}

START_TEST(header)
{
    HWND parent_hwnd;

    if (!init())
        return;

    test_header_control();
    test_header_order();
    test_customdraw();

    DestroyWindow(hHeaderParentWnd);

    init_msg_sequences(sequences, NUM_MSG_SEQUENCES);
    parent_hwnd = create_custom_parent_window();
    ok_sequence(sequences, PARENT_SEQ_INDEX, create_parent_wnd_seq, "create parent windows", FALSE);

    test_hdm_index_messages(parent_hwnd);
    test_hdm_getitemrect(parent_hwnd);
    test_hdm_hittest(parent_hwnd);
    test_hdm_layout(parent_hwnd);
    test_hdm_ordertoindex(parent_hwnd);
    test_hdm_sethotdivider(parent_hwnd);
    test_hdm_imageMessages(parent_hwnd);
    test_hdm_filterMessages(parent_hwnd);
    test_hdm_unicodeformatMessages(parent_hwnd);
    test_hdm_bitmapmarginMessages(parent_hwnd);

    DestroyWindow(parent_hwnd);

}
