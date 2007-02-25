/* Unit test suite for header control.
 *
 * Copyright 2005 Vijay Kiran Kamuju 
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

typedef struct tagEXPECTEDNOTIFY
{
    INT iCode;
    BOOL fUnicode;
    HDITEMA hdItem;
} EXPECTEDNOTIFY;

EXPECTEDNOTIFY expectedNotify[10];
INT nExpectedNotify = 0;
INT nReceivedNotify = 0;
INT unexpectedNotify[10];
INT nUnexpectedNotify = 0;

static HWND hHeaderParentWnd;
static HWND hWndHeader;
#define MAX_CHARS 100

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

static LONG addItem(HWND hdex, int idx, LPCSTR text)
{
    HDITEMA hdItem;
    hdItem.mask       = HDI_TEXT | HDI_WIDTH;
    hdItem.cxy        = 100;
    hdItem.pszText    = (LPSTR)text;
    hdItem.cchTextMax = 0;
    return (LONG)SendMessage(hdex, HDM_INSERTITEMA, (WPARAM)idx, (LPARAM)&hdItem);
}

static LONG setItem(HWND hdex, int idx, LPCSTR text, BOOL fCheckNotifies)
{
    LONG ret;
    HDITEMA hdexItem;
    hdexItem.mask       = HDI_TEXT;
    hdexItem.pszText    = (LPSTR)text;
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

static LONG setItemUnicodeNotify(HWND hdex, int idx, LPCSTR text, LPCWSTR wText)
{
    LONG ret;
    HDITEMA hdexItem;
    HDITEMW hdexNotify;
    hdexItem.mask       = HDI_TEXT;
    hdexItem.pszText    = (LPSTR)text;
    hdexItem.cchTextMax = 0;
    
    hdexNotify.mask    = HDI_TEXT;
    hdexNotify.pszText = (LPWSTR)wText;
    
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
    ok(SendMessage(hdex, HDM_INSERTITEMA, (WPARAM)0, (LPARAM)phdiCreate)!=-1, "Adding item failed\n");
    ZeroMemory(phdiRead, sizeof(HDITEMA));
    phdiRead->mask = maskRead;
    ok(SendMessage(hdex, HDM_GETITEMA, (WPARAM)0, (LPARAM)phdiRead)!=0, "Getting item data failed\n");
    ok(SendMessage(hdex, HDM_DELETEITEM, (WPARAM)0, (LPARAM)0)!=0, "Deleteing item failed\n");
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

static const char *str_items[] =
    {"First Item", "Second Item", "Third Item", "Fourth Item", "Replace Item", "Out Of Range Item"};
    
static const char pszUniTestA[] = "TST";
static const WCHAR pszUniTestW[] = {'T','S','T',0};


#define TEST_GET_ITEM(i,c)\
{   res = getItem(hWndHeader, i, buffer);\
    ok(res != 0, "Getting item[%d] using valid index failed unexpectedly (%ld)\n", i, res);\
    ok(strcmp(str_items[c], buffer) == 0, "Getting item[%d] returned \"%s\" expecting \"%s\"\n", i, buffer, str_items[c]);\
}

#define TEST_GET_ITEMCOUNT(i)\
{   res = getItemCount(hWndHeader);\
    ok(res == i, "Got Item Count as %ld\n", res);\
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
    LRESULT res;

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
    ret = SendMessage(hWndHeader, HDM_INSERTITEM, (WPARAM)0, (LPARAM)&hdi);
    ok(ret == -1, "Creating an item with a zero mask should have failed\n");
    if (ret != -1) SendMessage(hWndHeader, HDM_DELETEITEM, (WPARAM)0, (LPARAM)0);

    /* with a non-zero mask creation will succeed */
    ZeroMemory(&hdi, sizeof(hdi));
    hdi.mask = HDI_LPARAM;
    ret = SendMessage(hWndHeader, HDM_INSERTITEM, (WPARAM)0, (LPARAM)&hdi);
    ok(ret != -1, "Adding item with non-zero mask failed\n");
    if (ret != -1)
        SendMessage(hWndHeader, HDM_DELETEITEM, (WPARAM)0, (LPARAM)0);

    /* in SETITEM if the mask contains a unknown bit, it is ignored */
    ZeroMemory(&hdi, sizeof(hdi));
    hdi.mask = 0x08000000 | HDI_LPARAM | HDI_IMAGE;
    hdi.lParam = 133;
    hdi.iImage = 17;
    ret = SendMessage(hWndHeader, HDM_INSERTITEM, (WPARAM)0, (LPARAM)&hdi);
    ok(ret != -1, "Adding item failed\n");

    if (ret != -1)
    {
        /* check result */
        ZeroMemory(&hdi, sizeof(hdi));
        hdi.mask = HDI_LPARAM | HDI_IMAGE;
        SendMessage(hWndHeader, HDM_GETITEM, (WPARAM)0, (LPARAM)&hdi);
        ok(hdi.lParam == 133, "comctl32 4.0 field not set\n");
        ok(hdi.iImage == 17, "comctl32 >4.0 field not set\n");

        /* but in GETITEM if an unknown bit is set, comctl32 uses only version 4.0 fields */
        ZeroMemory(&hdi, sizeof(hdi));
        hdi.mask = 0x08000000 | HDI_LPARAM | HDI_IMAGE;
        SendMessage(hWndHeader, HDM_GETITEM, (WPARAM)0, (LPARAM)&hdi);
        ok(hdi.lParam == 133, "comctl32 4.0 field not read\n");
        ok(hdi.iImage == 0, "comctl32 >4.0 field shouldn't be read\n");

        SendMessage(hWndHeader, HDM_DELETEITEM, (WPARAM)0, (LPARAM)0);
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
        ok(res == 0, "Adding simple item failed (%ld)\n", res);
    }

    TEST_GET_ITEMCOUNT(4);
    res = addItem(hWndHeader, 99, str_items[i+1]);
    ok(res != -1, "Adding Out of Range item should fail with -1 got (%ld)\n", res);
    TEST_GET_ITEMCOUNT(5);
    res = addItem(hWndHeader, 5, str_items[i+1]);
    ok(res != -1, "Adding Out of Range item should fail with -1 got (%ld)\n", res);
    TEST_GET_ITEMCOUNT(6);

    for (i = 0; i < 4; i++) { TEST_GET_ITEM(i,i); TEST_GET_ITEMCOUNT(6); }

    res=getItem(hWndHeader, 99, buffer);
    ok(res == 0, "Getting Out of Range item should fail with 0 (%ld), got %s\n", res,buffer);
    res=getItem(hWndHeader, 5, buffer);
    ok(res == 1, "Getting Out of Range item should fail with 1 (%ld), got %s\n", res,buffer);
    res=getItem(hWndHeader, -2, buffer);
    ok(res == 0, "Getting Out of Range item should fail with 0 (%ld), got %s\n", res,buffer);

    if (winetest_interactive)
    {
        UpdateWindow(hHeaderParentWnd);
        UpdateWindow(hWndHeader);
    }

    TEST_GET_ITEMCOUNT(6);
    res=setItem(hWndHeader, 99, str_items[5], FALSE);
    ok(res == 0, "Setting Out of Range item should fail with 0 (%ld)\n", res);
    res=setItem(hWndHeader, 5, str_items[5], TRUE);
    ok(res == 1, "Setting Out of Range item should fail with 1 (%ld)\n", res);
    res=setItem(hWndHeader, -2, str_items[5], FALSE);
    ok(res == 0, "Setting Out of Range item should fail with 0 (%ld)\n", res);
    TEST_GET_ITEMCOUNT(6);

    for (i = 0; i < 4; i++)
    {
        res = setItem(hWndHeader, i, str_items[4], TRUE);
        ok(res != 0, "Setting %d item failed (%ld)\n", i+1, res);
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
    ok(res == 1, "Deleting Out of Range item should fail with 1 (%ld)\n", res);
    res = delItem(hWndHeader, -2);
    ok(res == 0, "Deleting Out of Range item should fail with 0 (%ld)\n", res);
    TEST_GET_ITEMCOUNT(5);

    res = delItem(hWndHeader, 3);
    ok(res != 0, "Deleting using out of range index failed (%ld)\n", res);
    TEST_GET_ITEMCOUNT(4);
    res = delItem(hWndHeader, 0);
    ok(res != 0, "Deleting using out of range index failed (%ld)\n", res);
    TEST_GET_ITEMCOUNT(3);
    res = delItem(hWndHeader, 0);
    ok(res != 0, "Deleting using out of range index failed (%ld)\n", res);
    TEST_GET_ITEMCOUNT(2);
    res = delItem(hWndHeader, 0);
    ok(res != 0, "Deleting using out of range index failed (%ld)\n", res);
    TEST_GET_ITEMCOUNT(1);

    DestroyWindow(hWndHeader);
}


LRESULT CALLBACK HeaderTestWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg) {

    case WM_NOTIFY:
    {
        NMHEADERA *hdr = (NMHEADER *)lParam;
        EXPECTEDNOTIFY *expected;
        int i;
        
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
    
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
  
    default:
        return DefWindowProcA(hWnd, msg, wParam, lParam);
    }
    
    return 0L;
}

static void init(void) {
    WNDCLASSA wc;
    INITCOMMONCONTROLSEX icex;

    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC  = ICC_USEREX_CLASSES;
    InitCommonControlsEx(&icex);

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.hIcon = NULL;
    wc.hCursor = LoadCursorA(NULL, MAKEINTRESOURCEA(IDC_ARROW));
    wc.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "HeaderTestClass";
    wc.lpfnWndProc = HeaderTestWndProc;
    RegisterClassA(&wc);

    hHeaderParentWnd = CreateWindowExA(0, "HeaderTestClass", "Header test", WS_OVERLAPPEDWINDOW, 
      CW_USEDEFAULT, CW_USEDEFAULT, 680, 260, NULL, NULL, GetModuleHandleA(NULL), 0);
    assert(hHeaderParentWnd != NULL);
}

START_TEST(header)
{
    init();

    test_header_control();

    DestroyWindow(hHeaderParentWnd);
}
