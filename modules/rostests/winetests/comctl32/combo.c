/* Unit test suite for ComboBox and ComboBoxEx32 controls.
 *
 * Copyright 2005 Jason Edmeades
 * Copyright 2007 Mikolaj Zalewski
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

#include <limits.h>
#include <stdio.h>
#include <windows.h>
#include <commctrl.h>

#include "wine/test.h"
#include "v6util.h"
#include "msg.h"

enum message_seq_index
{
    EDITBOX_SEQ_INDEX = 0,
    PARENT_SEQ_INDEX,
    NUM_MSG_SEQUENCES,
};

#define EDITBOX_ID         0
#define COMBO_ID           1995

#define expect_rect(r, _left, _top, _right, _bottom) ok(r.left == _left && r.top == _top && \
    r.bottom == _bottom && r.right == _right, "Invalid rect %s vs (%d,%d)-(%d,%d)\n", \
    wine_dbgstr_rect(&r), _left, _top, _right, _bottom);


static struct msg_sequence *sequences[NUM_MSG_SEQUENCES];

static HWND hComboExParentWnd, hMainWnd;
static HINSTANCE hMainHinst;
static const char ComboExTestClass[] = "ComboExTestClass";

static HBRUSH brush_red;

static BOOL (WINAPI *pSetWindowSubclass)(HWND, SUBCLASSPROC, UINT_PTR, DWORD_PTR);

#define MAX_CHARS 100
static char *textBuffer = NULL;

static BOOL received_end_edit = FALSE;

static void get_combobox_info(HWND hwnd, COMBOBOXINFO *info)
{
    BOOL ret;

    info->cbSize = sizeof(*info);
    ret = GetComboBoxInfo(hwnd, info);
    ok(ret, "Failed to get combobox info structure, error %ld\n", GetLastError());
}

static HWND createComboEx(DWORD style) {
   return CreateWindowExA(0, WC_COMBOBOXEXA, NULL, style, 0, 0, 300, 300,
            hComboExParentWnd, NULL, hMainHinst, NULL);
}

static LONG addItem(HWND cbex, int idx, const char *text) {
    COMBOBOXEXITEMA cbexItem;
    memset(&cbexItem, 0x00, sizeof(cbexItem));
    cbexItem.mask = CBEIF_TEXT;
    cbexItem.iItem = idx;
    cbexItem.pszText    = (char*)text;
    cbexItem.cchTextMax = 0;
    return SendMessageA(cbex, CBEM_INSERTITEMA, 0, (LPARAM)&cbexItem);
}

static LONG setItem(HWND cbex, int idx, const char *text) {
    COMBOBOXEXITEMA cbexItem;
    memset(&cbexItem, 0x00, sizeof(cbexItem));
    cbexItem.mask = CBEIF_TEXT;
    cbexItem.iItem = idx;
    cbexItem.pszText    = (char*)text;
    cbexItem.cchTextMax = 0;
    return SendMessageA(cbex, CBEM_SETITEMA, 0, (LPARAM)&cbexItem);
}

static LONG delItem(HWND cbex, int idx) {
    return SendMessageA(cbex, CBEM_DELETEITEM, idx, 0);
}

static LONG getItem(HWND cbex, int idx, COMBOBOXEXITEMA *cbItem) {
    memset(cbItem, 0x00, sizeof(COMBOBOXEXITEMA));
    cbItem->mask = CBEIF_TEXT;
    cbItem->pszText      = textBuffer;
    cbItem->iItem        = idx;
    cbItem->cchTextMax   = 100;
    return SendMessageA(cbex, CBEM_GETITEMA, 0, (LPARAM)cbItem);
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
    msg.id     = EDITBOX_ID;

    if (message != WM_PAINT &&
        message != WM_ERASEBKGND &&
        message != WM_NCPAINT &&
        message != WM_NCHITTEST &&
        message != WM_GETTEXT &&
        message != WM_GETICON &&
        message != WM_DEVICECHANGE)
    {
        add_message(sequences, EDITBOX_SEQ_INDEX, &msg);
    }

    defwndproc_counter++;
    ret = CallWindowProcA(oldproc, hwnd, message, wParam, lParam);
    defwndproc_counter--;
    return ret;
}

static HWND subclass_editbox(HWND hwndComboEx)
{
    WNDPROC oldproc;
    HWND hwnd;

    hwnd = (HWND)SendMessageA(hwndComboEx, CBEM_GETEDITCONTROL, 0, 0);
    oldproc = (WNDPROC)SetWindowLongPtrA(hwnd, GWLP_WNDPROC,
                                         (LONG_PTR)editbox_subclass_proc);
    SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)oldproc);

    return hwnd;
}

static void test_comboex(void)
{
    HWND myHwnd = 0;
    LONG res;
    COMBOBOXEXITEMA cbexItem;
    static const char *first_item  = "First Item",
                *second_item = "Second Item",
                *third_item  = "Third Item",
                *middle_item = "Between First and Second Items",
                *replacement_item = "Between First and Second Items",
                *out_of_range_item = "Out of Range Item";

    /* Allocate space for result */
    textBuffer = malloc(MAX_CHARS);

    /* Basic comboboxex test */
    myHwnd = createComboEx(WS_BORDER | WS_VISIBLE | WS_CHILD | CBS_DROPDOWN);

    /* Add items onto the end of the combobox */
    res = addItem(myHwnd, -1, first_item);
    ok(res == 0, "Adding simple item failed (%ld)\n", res);
    res = addItem(myHwnd, -1, second_item);
    ok(res == 1, "Adding simple item failed (%ld)\n", res);
    res = addItem(myHwnd, 2, third_item);
    ok(res == 2, "Adding simple item failed (%ld)\n", res);
    res = addItem(myHwnd, 1, middle_item);
    ok(res == 1, "Inserting simple item failed (%ld)\n", res);

    /* Add an item completely out of range */
    res = addItem(myHwnd, 99, out_of_range_item);
    ok(res == -1, "Adding using out of range index worked unexpectedly (%ld)\n", res);
    res = addItem(myHwnd, 5, out_of_range_item);
    ok(res == -1, "Adding using out of range index worked unexpectedly (%ld)\n", res);
    /* Removed: Causes traps on Windows XP
       res = addItem(myHwnd, -2, "Out Of Range Item");
       ok(res == -1, "Adding out of range worked unexpectedly (%ld)\n", res);
     */

    /* Get an item completely out of range */ 
    res = getItem(myHwnd, 99, &cbexItem); 
    ok(res == 0, "Getting item using out of range index worked unexpectedly (%ld, %s)\n", res, cbexItem.pszText);
    res = getItem(myHwnd, 4, &cbexItem); 
    ok(res == 0, "Getting item using out of range index worked unexpectedly (%ld, %s)\n", res, cbexItem.pszText);
    res = getItem(myHwnd, -2, &cbexItem); 
    ok(res == 0, "Getting item using out of range index worked unexpectedly (%ld, %s)\n", res, cbexItem.pszText);

    /* Get an item in range */ 
    res = getItem(myHwnd, 0, &cbexItem); 
    ok(res != 0, "Getting item using valid index failed unexpectedly (%ld)\n", res);
    ok(strcmp(first_item, cbexItem.pszText) == 0, "Getting item returned wrong string (%s)\n", cbexItem.pszText);

    res = getItem(myHwnd, 1, &cbexItem); 
    ok(res != 0, "Getting item using valid index failed unexpectedly (%ld)\n", res);
    ok(strcmp(middle_item, cbexItem.pszText) == 0, "Getting item returned wrong string (%s)\n", cbexItem.pszText);

    res = getItem(myHwnd, 2, &cbexItem); 
    ok(res != 0, "Getting item using valid index failed unexpectedly (%ld)\n", res);
    ok(strcmp(second_item, cbexItem.pszText) == 0, "Getting item returned wrong string (%s)\n", cbexItem.pszText);

    res = getItem(myHwnd, 3, &cbexItem); 
    ok(res != 0, "Getting item using valid index failed unexpectedly (%ld)\n", res);
    ok(strcmp(third_item, cbexItem.pszText) == 0, "Getting item returned wrong string (%s)\n", cbexItem.pszText);

    /* Set an item completely out of range */ 
    res = setItem(myHwnd, 99, replacement_item); 
    ok(res == 0, "Setting item using out of range index worked unexpectedly (%ld)\n", res);
    res = setItem(myHwnd, 4, replacement_item); 
    ok(res == 0, "Setting item using out of range index worked unexpectedly (%ld)\n", res);
    res = setItem(myHwnd, -2, replacement_item); 
    ok(res == 0, "Setting item using out of range index worked unexpectedly (%ld)\n", res);

    /* Set an item in range */ 
    res = setItem(myHwnd, 0, replacement_item);
    ok(res != 0, "Setting first item failed (%ld)\n", res);
    res = setItem(myHwnd, 3, replacement_item);
    ok(res != 0, "Setting last item failed (%ld)\n", res);

    /* Remove items completely out of range (4 items in control at this point) */
    res = delItem(myHwnd, -1);
    ok(res == CB_ERR, "Deleting using out of range index worked unexpectedly (%ld)\n", res);
    res = delItem(myHwnd, 4);
    ok(res == CB_ERR, "Deleting using out of range index worked unexpectedly (%ld)\n", res);

    /* Remove items in range (4 items in control at this point) */
    res = delItem(myHwnd, 3);
    ok(res == 3, "Deleting using out of range index failed (%ld)\n", res);
    res = delItem(myHwnd, 0);
    ok(res == 2, "Deleting using out of range index failed (%ld)\n", res);
    res = delItem(myHwnd, 0);
    ok(res == 1, "Deleting using out of range index failed (%ld)\n", res);
    res = delItem(myHwnd, 0);
    ok(res == 0, "Deleting using out of range index failed (%ld)\n", res);

    /* Remove from an empty box */
    res = delItem(myHwnd, 0);
    ok(res == CB_ERR, "Deleting using out of range index worked unexpectedly (%ld)\n", res);


    /* Cleanup */
    free(textBuffer);
    DestroyWindow(myHwnd);
}

static void test_comboex_WM_LBUTTONDOWN(void)
{
    HWND hComboEx, hCombo, hEdit, hList;
    COMBOBOXINFO cbInfo;
    UINT x, y, item_height;
    LRESULT result;
    UINT i;
    int idx;
    RECT rect;
    WCHAR buffer[3];
    static const UINT choices[] = {8,9,10,11,12,14,16,18,20,22,24,26,28,36,48,72};

    hComboEx = CreateWindowExA(0, WC_COMBOBOXEXA, NULL,
            WS_VISIBLE|WS_CHILD|CBS_DROPDOWN, 0, 0, 200, 150,
            hComboExParentWnd, NULL, hMainHinst, NULL);

    for (i = 0; i < ARRAY_SIZE(choices); i++)
    {
        COMBOBOXEXITEMW cbexItem;
        wsprintfW(buffer, L"%2d", choices[i]);

        memset(&cbexItem, 0x00, sizeof(cbexItem));
        cbexItem.mask = CBEIF_TEXT;
        cbexItem.iItem = i;
        cbexItem.pszText = buffer;
        cbexItem.cchTextMax = 0;
        ok(SendMessageW(hComboEx, CBEM_INSERTITEMW, 0, (LPARAM)&cbexItem) >= 0,
           "Failed to add item %d\n", i);
    }

    hCombo = (HWND)SendMessageA(hComboEx, CBEM_GETCOMBOCONTROL, 0, 0);
    hEdit = (HWND)SendMessageA(hComboEx, CBEM_GETEDITCONTROL, 0, 0);

    get_combobox_info(hCombo, &cbInfo);
    hList = cbInfo.hwndList;

    ok(GetFocus() == hComboExParentWnd,
       "Focus not on Main Window, instead on %p\n", GetFocus());

    /* Click on the button to drop down the list */
    x = cbInfo.rcButton.left + (cbInfo.rcButton.right-cbInfo.rcButton.left)/2;
    y = cbInfo.rcButton.top + (cbInfo.rcButton.bottom-cbInfo.rcButton.top)/2;
    result = SendMessageA(hCombo, WM_LBUTTONDOWN, 0, MAKELPARAM(x, y));
    ok(result, "WM_LBUTTONDOWN was not processed. LastError=%ld\n",
       GetLastError());
    ok(GetFocus() == hCombo ||
       broken(GetFocus() != hCombo), /* win98 */
       "Focus not on ComboBoxEx's ComboBox Control, instead on %p\n",
       GetFocus());
    ok(SendMessageA(hComboEx, CB_GETDROPPEDSTATE, 0, 0),
       "The dropdown list should have appeared after clicking the button.\n");
    idx = SendMessageA(hCombo, CB_GETTOPINDEX, 0, 0);
    ok(idx == 0, "For TopIndex expected %d, got %d\n", 0, idx);

    result = SendMessageA(hCombo, WM_LBUTTONUP, 0, MAKELPARAM(x, y));
    ok(result, "WM_LBUTTONUP was not processed. LastError=%ld\n",
       GetLastError());
    ok(GetFocus() == hCombo ||
       broken(GetFocus() != hCombo), /* win98 */
       "Focus not on ComboBoxEx's ComboBox Control, instead on %p\n",
       GetFocus());

    /* Click on the 5th item in the list */
    item_height = SendMessageA(hCombo, CB_GETITEMHEIGHT, 0, 0);
    ok(GetClientRect(hList, &rect), "Failed to get list's client rect.\n");
    x = rect.left + (rect.right-rect.left)/2;
    y = item_height/2 + item_height*4;
    result = SendMessageA(hList, WM_MOUSEMOVE, 0, MAKELPARAM(x, y));
    ok(!result, "WM_MOUSEMOVE was not processed. LastError=%ld\n",
       GetLastError());
    ok(GetFocus() == hCombo ||
       broken(GetFocus() != hCombo), /* win98 */
       "Focus not on ComboBoxEx's ComboBox Control, instead on %p\n",
       GetFocus());

    result = SendMessageA(hList, WM_LBUTTONDOWN, 0, MAKELPARAM(x, y));
    ok(!result, "WM_LBUTTONDOWN was not processed. LastError=%ld\n",
       GetLastError());
    ok(GetFocus() == hCombo ||
       broken(GetFocus() != hCombo), /* win98 */
       "Focus not on ComboBoxEx's ComboBox Control, instead on %p\n",
       GetFocus());
    ok(SendMessageA(hComboEx, CB_GETDROPPEDSTATE, 0, 0),
       "The dropdown list should still be visible.\n");

    result = SendMessageA(hList, WM_LBUTTONUP, 0, MAKELPARAM(x, y));
    ok(!result, "WM_LBUTTONUP was not processed. LastError=%ld\n",
       GetLastError());
    todo_wine ok(GetFocus() == hEdit ||
       broken(GetFocus() == hCombo), /* win98 */
       "Focus not on ComboBoxEx's Edit Control, instead on %p\n",
       GetFocus());

    result = SendMessageA(hCombo, CB_GETDROPPEDSTATE, 0, 0);
    ok(!result ||
       broken(result != 0), /* win98 */
       "The dropdown list should have been rolled up.\n");
    idx = SendMessageA(hComboEx, CB_GETCURSEL, 0, 0);
    ok(idx == 4 ||
       broken(idx == -1), /* win98 */
       "Current Selection: expected %d, got %d\n", 4, idx);
    ok(received_end_edit, "Expected to receive a CBEN_ENDEDIT message\n");

    SetFocus( hComboExParentWnd );
    ok( GetFocus() == hComboExParentWnd, "got %p\n", GetFocus() );
    SetFocus( hComboEx );
    ok( GetFocus() == hEdit, "got %p\n", GetFocus() );

    DestroyWindow(hComboEx);
}

static void test_comboex_CB_GETLBTEXT(void)
{
    HWND hCombo;
    CHAR buff[1];
    COMBOBOXEXITEMA item;
    LRESULT ret;

    hCombo = createComboEx(WS_BORDER | WS_VISIBLE | WS_CHILD | CBS_DROPDOWN);

    /* set text to null */
    addItem(hCombo, 0, NULL);

    buff[0] = 'a';
    item.mask = CBEIF_TEXT;
    item.iItem = 0;
    item.pszText = buff;
    item.cchTextMax = 1;
    ret = SendMessageA(hCombo, CBEM_GETITEMA, 0, (LPARAM)&item);
    ok(ret != 0, "CBEM_GETITEM failed\n");
    ok(buff[0] == 0, "\n");

    ret = SendMessageA(hCombo, CB_GETLBTEXTLEN, 0, 0);
    ok(ret == 0, "Expected zero length\n");

    ret = SendMessageA(hCombo, CB_GETLBTEXTLEN, 0, 0);
    ok(ret == 0, "Expected zero length\n");

    buff[0] = 'a';
    ret = SendMessageA(hCombo, CB_GETLBTEXT, 0, (LPARAM)buff);
    ok(ret == 0, "Expected zero length\n");
    ok(buff[0] == 0, "Expected null terminator as a string, got %s\n", buff);

    DestroyWindow(hCombo);
}

static void test_comboex_WM_WINDOWPOSCHANGING(void)
{
    HWND hCombo;
    WINDOWPOS wp;
    RECT rect;
    int combo_height;
    int ret;

    hCombo = createComboEx(WS_BORDER | WS_VISIBLE | WS_CHILD | CBS_DROPDOWN);
    ok(hCombo != NULL, "createComboEx failed\n");
    ret = GetWindowRect(hCombo, &rect);
    ok(ret, "GetWindowRect failed\n");
    combo_height = rect.bottom - rect.top;
    ok(combo_height > 0, "wrong combo height\n");

    /* Test height > combo_height */
    wp.x = rect.left;
    wp.y = rect.top;
    wp.cx = (rect.right - rect.left);
    wp.cy = combo_height * 2;
    wp.flags = 0;
    wp.hwnd = hCombo;
    wp.hwndInsertAfter = NULL;

    ret = SendMessageA(hCombo, WM_WINDOWPOSCHANGING, 0, (LPARAM)&wp);
    ok(ret == 0, "expected 0, got %x\n", ret);
    ok(wp.cy == combo_height,
            "Expected height %d, got %d\n", combo_height, wp.cy);

    /* Test height < combo_height */
    wp.x = rect.left;
    wp.y = rect.top;
    wp.cx = (rect.right - rect.left);
    wp.cy = combo_height / 2;
    wp.flags = 0;
    wp.hwnd = hCombo;
    wp.hwndInsertAfter = NULL;

    ret = SendMessageA(hCombo, WM_WINDOWPOSCHANGING, 0, (LPARAM)&wp);
    ok(ret == 0, "expected 0, got %x\n", ret);
    ok(wp.cy == combo_height,
            "Expected height %d, got %d\n", combo_height, wp.cy);

    ret = DestroyWindow(hCombo);
    ok(ret, "DestroyWindow failed\n");
}

struct di_context
{
    unsigned int mask;
    BOOL set_CBEIF_DI_SETITEM;
};

static struct di_context di_context;

static LRESULT ComboExTestOnNotify(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    struct message msg;
    NMHDR *hdr = (NMHDR*)lParam;

    msg.message = message;
    msg.flags = sent|wparam|lparam;
    msg.wParam = wParam;
    msg.lParam = lParam;
    if (hdr) msg.id = hdr->code;

    add_message(sequences, PARENT_SEQ_INDEX, &msg);

    switch (hdr->code)
    {
    case CBEN_ENDEDITA:
        {
            NMCBEENDEDITA *edit_info = (NMCBEENDEDITA*)hdr;
            if(edit_info->iWhy==CBENF_DROPDOWN){
                received_end_edit = TRUE;
            }
            break;
        }
    case CBEN_ENDEDITW:
        {
            NMCBEENDEDITW *edit_info = (NMCBEENDEDITW*)hdr;
            if(edit_info->iWhy==CBENF_DROPDOWN){
                received_end_edit = TRUE;
            }
            break;
        }
    case CBEN_GETDISPINFOA:
    case CBEN_GETDISPINFOW:
        {
            NMCOMBOBOXEXA *item = (NMCOMBOBOXEXA *)hdr;

            di_context.mask = item->ceItem.mask;

            if (item->ceItem.mask & CBEIF_IMAGE)
            {
                ok(item->ceItem.iImage == I_IMAGECALLBACK, "Unexpected iImage %d.\n", item->ceItem.iImage);
                item->ceItem.iImage = 123;
            }

            if (item->ceItem.mask & CBEIF_TEXT)
            {
                ok(item->ceItem.pszText && item->ceItem.pszText != LPSTR_TEXTCALLBACKA,
                        "Unexpected pszText %p.\n", item->ceItem.pszText);
                ok(item->ceItem.cchTextMax == 0, "Unexpected cchTextMax %d.\n", item->ceItem.cchTextMax);
            }

            if (item->ceItem.mask & CBEIF_SELECTEDIMAGE)
                ok(item->ceItem.iSelectedImage == I_IMAGECALLBACK, "Unexpected iSelectedImage %d.\n",
                        item->ceItem.iSelectedImage);

            if (item->ceItem.mask & CBEIF_OVERLAY)
                ok(item->ceItem.iOverlay == I_IMAGECALLBACK, "Unexpected iOverlay %d.\n",
                        item->ceItem.iOverlay);

            if (item->ceItem.mask & CBEIF_INDENT)
                ok(item->ceItem.iIndent == 0, "Unexpected iIndent %d.\n", item->ceItem.iIndent);

            if (di_context.set_CBEIF_DI_SETITEM)
                item->ceItem.mask |= CBEIF_DI_SETITEM;

            break;
        }
    }
    return 0;
}

static LRESULT CALLBACK ComboExTestWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg) {

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_NOTIFY:
        return ComboExTestOnNotify(hWnd,msg,wParam,lParam);
    default:
        return DefWindowProcA(hWnd, msg, wParam, lParam);
    }
    
    return 0L;
}

static void init_functions(void)
{
    HMODULE hComCtl32 = LoadLibraryA("comctl32.dll");

#define X(f) p##f = (void*)GetProcAddress(hComCtl32, #f);
#define X2(f, ord) p##f = (void*)GetProcAddress(hComCtl32, (const char *)ord);
    X2(SetWindowSubclass, 410);
#undef X
#undef X2
}

static BOOL init(void)
{
    WNDCLASSA wc;

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.hIcon = NULL;
    wc.hCursor = LoadCursorA(NULL, (LPCSTR)IDC_ARROW);
    wc.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = ComboExTestClass;
    wc.lpfnWndProc = ComboExTestWndProc;
    RegisterClassA(&wc);

    brush_red = CreateSolidBrush(RGB(255, 0, 0));

    hMainWnd = CreateWindowA(WC_STATICA, "Test", WS_OVERLAPPEDWINDOW, 10, 10, 300, 300, NULL, NULL, NULL, 0);
    ShowWindow(hMainWnd, SW_SHOW);

    hComboExParentWnd = CreateWindowExA(0, ComboExTestClass, "ComboEx test", WS_OVERLAPPEDWINDOW|WS_VISIBLE,
      CW_USEDEFAULT, CW_USEDEFAULT, 680, 260, NULL, NULL, GetModuleHandleA(NULL), 0);
    ok(hComboExParentWnd != NULL, "failed to create parent window\n");

    hMainHinst = GetModuleHandleA(NULL);

    return hComboExParentWnd != NULL;
}

static void cleanup(void)
{
    MSG msg;
    
    PostMessageA(hComboExParentWnd, WM_CLOSE, 0, 0);
    while (GetMessageA(&msg,0,0,0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    
    DestroyWindow(hComboExParentWnd);
    UnregisterClassA(ComboExTestClass, GetModuleHandleA(NULL));

    DestroyWindow(hMainWnd);
    DeleteObject(brush_red);
}

static void test_comboex_subclass(void)
{
    HWND hComboEx, hCombo, hEdit;

    hComboEx = createComboEx(WS_BORDER | WS_VISIBLE | WS_CHILD | CBS_DROPDOWN);

    hCombo = (HWND)SendMessageA(hComboEx, CBEM_GETCOMBOCONTROL, 0, 0);
    ok(hCombo != NULL, "Failed to get internal combo\n");
    hEdit = (HWND)SendMessageA(hComboEx, CBEM_GETEDITCONTROL, 0, 0);
    ok(hEdit != NULL, "Failed to get internal edit\n");

    if (pSetWindowSubclass)
    {
        ok(GetPropA(hCombo, "CC32SubclassInfo") != NULL, "Expected CC32SubclassInfo property\n");
        ok(GetPropA(hEdit, "CC32SubclassInfo") != NULL, "Expected CC32SubclassInfo property\n");
    }

    DestroyWindow(hComboEx);
}

static const struct message test_setitem_edit_seq[] = {
    { WM_SETTEXT, sent|id, 0, 0, EDITBOX_ID },
    { EM_SETSEL, sent|id|wparam|lparam, 0,  0, EDITBOX_ID },
    { EM_SETSEL, sent|id|wparam|lparam, 0, -1, EDITBOX_ID },
    { 0 }
};

static void test_comboex_get_set_item(void)
{
    char textA[] = "test";
    HWND hComboEx;
    COMBOBOXEXITEMA item;
    DWORD ret;

    hComboEx = createComboEx(WS_BORDER | WS_VISIBLE | WS_CHILD | CBS_DROPDOWN);

    subclass_editbox(hComboEx);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    memset(&item, 0, sizeof(item));
    item.mask = CBEIF_TEXT;
    item.pszText = textA;
    item.iItem = -1;
    ret = SendMessageA(hComboEx, CBEM_SETITEMA, 0, (LPARAM)&item);
    ok(ret == 1, "Unexpected return value %ld.\n", ret);

    ok_sequence(sequences, EDITBOX_SEQ_INDEX, test_setitem_edit_seq, "set item data for edit", FALSE);

    /* get/set lParam */
    item.mask = CBEIF_LPARAM;
    item.iItem = -1;
    item.lParam = 0xdeadbeef;
    ret = SendMessageA(hComboEx, CBEM_GETITEMA, 0, (LPARAM)&item);
    ok(ret == 1, "Unexpected return value %ld.\n", ret);
    ok(item.lParam == 0, "Expected zero, got %Ix\n", item.lParam);

    item.lParam = 0x1abe11ed;
    ret = SendMessageA(hComboEx, CBEM_SETITEMA, 0, (LPARAM)&item);
    ok(ret == 1, "Unexpected return value %ld.\n", ret);

    item.lParam = 0;
    ret = SendMessageA(hComboEx, CBEM_GETITEMA, 0, (LPARAM)&item);
    ok(ret == 1, "Unexpected return value %ld.\n", ret);
    ok(item.lParam == 0x1abe11ed, "Expected 0x1abe11ed, got %Ix\n", item.lParam);

    DestroyWindow(hComboEx);
}

static HWND create_combobox(DWORD style)
{
    return CreateWindowA(WC_COMBOBOXA, "Combo", WS_VISIBLE|WS_CHILD|style, 5, 5, 100, 100, hMainWnd, (HMENU)COMBO_ID, NULL, 0);
}

static int get_font_height(HFONT hFont)
{
    TEXTMETRICA tm;
    HFONT hFontOld;
    HDC hDC;

    hDC = CreateCompatibleDC(NULL);
    hFontOld = SelectObject(hDC, hFont);
    GetTextMetricsA(hDC, &tm);
    SelectObject(hDC, hFontOld);
    DeleteDC(hDC);

    return tm.tmHeight;
}

static void test_combo_setitemheight(DWORD style)
{
    HWND hCombo = create_combobox(style);
    int i, font_height, height;
    HFONT hFont;
    RECT r;

    GetClientRect(hCombo, &r);
    expect_rect(r, 0, 0, 100, get_font_height(GetStockObject(SYSTEM_FONT)) + 8);
    SendMessageA(hCombo, CB_GETDROPPEDCONTROLRECT, 0, (LPARAM)&r);
    MapWindowPoints(HWND_DESKTOP, hMainWnd, (LPPOINT)&r, 2);
    todo_wine expect_rect(r, 5, 5, 105, 105);

    for (i = 1; i < 30; i++)
    {
        SendMessageA(hCombo, CB_SETITEMHEIGHT, -1, i);
        GetClientRect(hCombo, &r);
        ok((r.bottom - r.top) == (i + 6), "Unexpected client rect height.\n");
    }

    DestroyWindow(hCombo);

    /* Set item height below text height, force resize. */
    hCombo = create_combobox(style);

    hFont = (HFONT)SendMessageA(hCombo, WM_GETFONT, 0, 0);
    font_height = get_font_height(hFont);
    SendMessageA(hCombo, CB_SETITEMHEIGHT, -1, font_height / 2);
    height = SendMessageA(hCombo, CB_GETITEMHEIGHT, -1, 0);
    todo_wine
    ok(height == font_height / 2, "Unexpected item height %d, expected %d.\n", height, font_height / 2);

    SetWindowPos(hCombo, NULL, 10, 10, 150, 5 * font_height, SWP_SHOWWINDOW);
    height = SendMessageA(hCombo, CB_GETITEMHEIGHT, -1, 0);
    ok(height > font_height, "Unexpected item height %d, font height %d.\n", height, font_height);

    DestroyWindow(hCombo);
}

static void test_combo_setfont(DWORD style)
{
    HFONT hFont1, hFont2;
    HWND hCombo;
    RECT r;
    int i;

    hCombo = create_combobox(style);
    hFont1 = CreateFontA(10, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, SYMBOL_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH|FF_DONTCARE, "Marlett");
    hFont2 = CreateFontA(8, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, SYMBOL_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH|FF_DONTCARE, "Marlett");

    GetClientRect(hCombo, &r);
    expect_rect(r, 0, 0, 100, get_font_height(GetStockObject(SYSTEM_FONT)) + 8);
    SendMessageA(hCombo, CB_GETDROPPEDCONTROLRECT, 0, (LPARAM)&r);
    MapWindowPoints(HWND_DESKTOP, hMainWnd, (LPPOINT)&r, 2);
    todo_wine expect_rect(r, 5, 5, 105, 105);

    /* The size of the dropped control is initially equal to the size
       of the window when it was created.  The size of the calculated
       dropped area changes only by how much the selection area
       changes, not by how much the list area changes.  */
    if (get_font_height(hFont1) == 10 && get_font_height(hFont2) == 8)
    {
        SendMessageA(hCombo, WM_SETFONT, (WPARAM)hFont1, FALSE);
        GetClientRect(hCombo, &r);
        expect_rect(r, 0, 0, 100, 18);
        SendMessageA(hCombo, CB_GETDROPPEDCONTROLRECT, 0, (LPARAM)&r);
        MapWindowPoints(HWND_DESKTOP, hMainWnd, (LPPOINT)&r, 2);
        todo_wine expect_rect(r, 5, 5, 105, 105 - (get_font_height(GetStockObject(SYSTEM_FONT)) - get_font_height(hFont1)));

        SendMessageA(hCombo, WM_SETFONT, (WPARAM)hFont2, FALSE);
        GetClientRect(hCombo, &r);
        expect_rect(r, 0, 0, 100, 16);
        SendMessageA(hCombo, CB_GETDROPPEDCONTROLRECT, 0, (LPARAM)&r);
        MapWindowPoints(HWND_DESKTOP, hMainWnd, (LPPOINT)&r, 2);
        todo_wine expect_rect(r, 5, 5, 105, 105 - (get_font_height(GetStockObject(SYSTEM_FONT)) - get_font_height(hFont2)));

        SendMessageA(hCombo, WM_SETFONT, (WPARAM)hFont1, FALSE);
        GetClientRect(hCombo, &r);
        expect_rect(r, 0, 0, 100, 18);
        SendMessageA(hCombo, CB_GETDROPPEDCONTROLRECT, 0, (LPARAM)&r);
        MapWindowPoints(HWND_DESKTOP, hMainWnd, (LPPOINT)&r, 2);
        todo_wine expect_rect(r, 5, 5, 105, 105 - (get_font_height(GetStockObject(SYSTEM_FONT)) - get_font_height(hFont1)));
    }
    else
    {
        ok(0, "Expected Marlett font heights 10/8, got %d/%d\n",
           get_font_height(hFont1), get_font_height(hFont2));
    }

    for (i = 1; i < 30; i++)
    {
        HFONT hFont = CreateFontA(i, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, SYMBOL_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH|FF_DONTCARE, "Marlett");
        int height = get_font_height(hFont);

        SendMessageA(hCombo, WM_SETFONT, (WPARAM)hFont, FALSE);
        GetClientRect(hCombo, &r);
        ok((r.bottom - r.top) == (height + 8), "Unexpected client rect height.\n");
        SendMessageA(hCombo, WM_SETFONT, 0, FALSE);
        DeleteObject(hFont);
    }

    DestroyWindow(hCombo);
    DeleteObject(hFont1);
    DeleteObject(hFont2);
}

static LRESULT (CALLBACK *old_parent_proc)(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
static LPCSTR expected_edit_text;
static LPCSTR expected_list_text;
static BOOL selchange_fired;
static HWND lparam_for_WM_CTLCOLOR;

static LRESULT CALLBACK parent_wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_COMMAND:
        switch (wparam)
        {
            case MAKEWPARAM(COMBO_ID, CBN_SELCHANGE):
            {
                HWND hCombo = (HWND)lparam;
                char list[20], edit[20];
                int idx;

                memset(list, 0, sizeof(list));
                memset(edit, 0, sizeof(edit));

                idx = SendMessageA(hCombo, CB_GETCURSEL, 0, 0);
                SendMessageA(hCombo, CB_GETLBTEXT, idx, (LPARAM)list);
                SendMessageA(hCombo, WM_GETTEXT, sizeof(edit), (LPARAM)edit);

                ok(!strcmp(edit, expected_edit_text), "edit: got %s, expected %s\n",
                    edit, expected_edit_text);
                ok(!strcmp(list, expected_list_text), "list: got %s, expected %s\n",
                    list, expected_list_text);

                selchange_fired = TRUE;
            }
            break;
        }
        break;
    case WM_CTLCOLOR:
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
        if (lparam_for_WM_CTLCOLOR)
        {
            ok(lparam_for_WM_CTLCOLOR == (HWND)lparam, "Expected %p, got %p\n", lparam_for_WM_CTLCOLOR, (HWND)lparam);
            return (LRESULT) brush_red;
        }
        break;
    }

    return CallWindowProcA(old_parent_proc, hwnd, msg, wparam, lparam);
}

static void test_selection(DWORD style, const char * const text[], const int *edit, const int *list)
{
    HWND hCombo;
    INT idx;

    hCombo = create_combobox(style);

    SendMessageA(hCombo, CB_ADDSTRING, 0, (LPARAM)text[0]);
    SendMessageA(hCombo, CB_ADDSTRING, 0, (LPARAM)text[1]);
    SendMessageA(hCombo, CB_SETCURSEL, -1, 0);

    old_parent_proc = (void *)SetWindowLongPtrA(hMainWnd, GWLP_WNDPROC, (ULONG_PTR)parent_wnd_proc);

    idx = SendMessageA(hCombo, CB_GETCURSEL, 0, 0);
    ok(idx == -1, "expected selection -1, got %d\n", idx);

    /* keyboard navigation */

    expected_list_text = text[list[0]];
    expected_edit_text = text[edit[0]];
    selchange_fired = FALSE;
    SendMessageA(hCombo, WM_KEYDOWN, VK_DOWN, 0);
    ok(selchange_fired, "CBN_SELCHANGE not sent!\n");

    expected_list_text = text[list[1]];
    expected_edit_text = text[edit[1]];
    selchange_fired = FALSE;
    SendMessageA(hCombo, WM_KEYDOWN, VK_DOWN, 0);
    ok(selchange_fired, "CBN_SELCHANGE not sent!\n");

    expected_list_text = text[list[2]];
    expected_edit_text = text[edit[2]];
    selchange_fired = FALSE;
    SendMessageA(hCombo, WM_KEYDOWN, VK_UP, 0);
    ok(selchange_fired, "CBN_SELCHANGE not sent!\n");

    /* programmatic navigation */

    expected_list_text = text[list[3]];
    expected_edit_text = text[edit[3]];
    selchange_fired = FALSE;
    SendMessageA(hCombo, CB_SETCURSEL, list[3], 0);
    ok(!selchange_fired, "CBN_SELCHANGE sent!\n");

    expected_list_text = text[list[4]];
    expected_edit_text = text[edit[4]];
    selchange_fired = FALSE;
    SendMessageA(hCombo, CB_SETCURSEL, list[4], 0);
    ok(!selchange_fired, "CBN_SELCHANGE sent!\n");

    SetWindowLongPtrA(hMainWnd, GWLP_WNDPROC, (ULONG_PTR)old_parent_proc);
    DestroyWindow(hCombo);
}

static void test_combo_CBN_SELCHANGE(void)
{
    static const char * const text[] = { "alpha", "beta", "" };
    static const int sel_1[] = { 2, 0, 1, 0, 1 };
    static const int sel_2[] = { 0, 1, 0, 0, 1 };

    test_selection(CBS_SIMPLE, text, sel_1, sel_2);
    test_selection(CBS_DROPDOWN, text, sel_1, sel_2);
    test_selection(CBS_DROPDOWNLIST, text, sel_2, sel_2);
}

static void test_combo_changesize(DWORD style)
{
    INT ddheight, clheight, ddwidth, clwidth;
    HWND hCombo;
    RECT rc;

    hCombo = create_combobox(style);

    /* get initial measurements */
    GetClientRect( hCombo, &rc);
    clheight = rc.bottom - rc.top;
    clwidth = rc.right - rc.left;
    SendMessageA(hCombo, CB_GETDROPPEDCONTROLRECT, 0, (LPARAM)&rc);
    ddheight = rc.bottom - rc.top;
    ddwidth = rc.right - rc.left;
    /* use MoveWindow to move & resize the combo */
    /* first make it slightly smaller */
    MoveWindow( hCombo, 10, 10, clwidth - 2, clheight - 2, TRUE);
    GetClientRect( hCombo, &rc);
    ok( rc.right - rc.left == clwidth - 2, "clientrect width is %ld vs %d\n",
            rc.right - rc.left, clwidth - 2);
    ok( rc.bottom - rc.top == clheight, "clientrect height is %ld vs %d\n",
                rc.bottom - rc.top, clheight);
    SendMessageA(hCombo, CB_GETDROPPEDCONTROLRECT, 0, (LPARAM)&rc);
    ok( rc.right - rc.left == clwidth - 2, "drop-down rect width is %ld vs %d\n",
            rc.right - rc.left, clwidth - 2);
    ok( rc.bottom - rc.top == ddheight, "drop-down rect height is %ld vs %d\n",
            rc.bottom - rc.top, ddheight);
    ok( rc.right - rc.left == ddwidth -2, "drop-down rect width is %ld vs %d\n",
            rc.right - rc.left, ddwidth - 2);
    /* new cx, cy is slightly bigger than the initial values */
    MoveWindow( hCombo, 10, 10, clwidth + 2, clheight + 2, TRUE);
    GetClientRect( hCombo, &rc);
    ok( rc.right - rc.left == clwidth + 2, "clientrect width is %ld vs %d\n",
            rc.right - rc.left, clwidth + 2);
    ok( rc.bottom - rc.top == clheight, "clientrect height is %ld vs %d\n",
            rc.bottom - rc.top, clheight);
    SendMessageA(hCombo, CB_GETDROPPEDCONTROLRECT, 0, (LPARAM)&rc);
    ok( rc.right - rc.left == clwidth + 2, "drop-down rect width is %ld vs %d\n",
            rc.right - rc.left, clwidth + 2);
    todo_wine {
        ok( rc.bottom - rc.top == clheight + 2, "drop-down rect height is %ld vs %d\n",
                rc.bottom - rc.top, clheight + 2);
    }

    ddwidth = SendMessageA(hCombo, CB_SETDROPPEDWIDTH, -1, 0);
    ok( ddwidth == clwidth + 2, "drop-width is %d vs %d\n", ddwidth, clwidth + 2);
    ddwidth = SendMessageA(hCombo, CB_GETDROPPEDWIDTH, 0, 0);
    ok( ddwidth == clwidth + 2, "drop-width is %d vs %d\n", ddwidth, clwidth + 2);

    ddwidth = SendMessageA(hCombo, CB_SETDROPPEDWIDTH, 0, 0);
    ok( ddwidth == clwidth + 2, "drop-width is %d vs %d\n", ddwidth, clwidth + 2);
    ddwidth = SendMessageA(hCombo, CB_GETDROPPEDWIDTH, 0, 0);
    ok( ddwidth == clwidth + 2, "drop-width is %d vs %d\n", ddwidth, clwidth + 2);

    ddwidth = SendMessageA(hCombo, CB_SETDROPPEDWIDTH, clwidth - 1, 0);
    ok( ddwidth == clwidth + 2, "drop-width is %d vs %d\n", ddwidth, clwidth + 2);
    ddwidth = SendMessageA(hCombo, CB_GETDROPPEDWIDTH, 0, 0);
    ok( ddwidth == clwidth + 2, "drop-width is %d vs %d\n", ddwidth, clwidth + 2);

    ddwidth = SendMessageA(hCombo, CB_SETDROPPEDWIDTH, clwidth << 1, 0);
    ok( ddwidth == (clwidth << 1), "drop-width is %d vs %d\n", ddwidth, clwidth << 1);
    ddwidth = SendMessageA(hCombo, CB_GETDROPPEDWIDTH, 0, 0);
    ok( ddwidth == (clwidth << 1), "drop-width is %d vs %d\n", ddwidth, clwidth << 1);

    ddwidth = SendMessageA(hCombo, CB_SETDROPPEDWIDTH, 0, 0);
    ok( ddwidth == (clwidth << 1), "drop-width is %d vs %d\n", ddwidth, clwidth << 1);
    ddwidth = SendMessageA(hCombo, CB_GETDROPPEDWIDTH, 0, 0);
    ok( ddwidth == (clwidth << 1), "drop-width is %d vs %d\n", ddwidth, clwidth << 1);

    ddwidth = SendMessageA(hCombo, CB_SETDROPPEDWIDTH, 1, 0);
    ok( ddwidth == clwidth + 2, "drop-width is %d vs %d\n", ddwidth, clwidth + 2);
    ddwidth = SendMessageA(hCombo, CB_GETDROPPEDWIDTH, 0, 0);
    ok( ddwidth == clwidth + 2, "drop-width is %d vs %d\n", ddwidth, clwidth + 2);

    DestroyWindow(hCombo);
}

static void test_combo_editselection(void)
{
    COMBOBOXINFO cbInfo;
    INT start, end;
    char edit[20];
    HWND hCombo;
    HWND hEdit;
    DWORD len;

    /* Build a combo */
    hCombo = create_combobox(CBS_SIMPLE);

    get_combobox_info(hCombo, &cbInfo);
    hEdit = cbInfo.hwndItem;

    /* Initially combo selection is empty*/
    len = SendMessageA(hCombo, CB_GETEDITSEL, 0,0);
    ok(LOWORD(len)==0, "Unexpected start position for selection %d\n", LOWORD(len));
    ok(HIWORD(len)==0, "Unexpected end position for selection %d\n", HIWORD(len));

    /* Set some text, and press a key to replace it */
    edit[0] = 0x00;
    SendMessageA(hCombo, WM_SETTEXT, 0, (LPARAM)"Jason1");
    SendMessageA(hCombo, WM_GETTEXT, sizeof(edit), (LPARAM)edit);
    ok(strcmp(edit, "Jason1")==0, "Unexpected text retrieved %s\n", edit);

    /* Now what is the selection - still empty */
    SendMessageA(hCombo, CB_GETEDITSEL, (WPARAM)&start, (WPARAM)&end);
    ok(start==0, "Unexpected start position for selection %d\n", start);
    ok(end==0, "Unexpected end position for selection %d\n", end);
    len = SendMessageA(hCombo, CB_GETEDITSEL, 0,0);
    ok(LOWORD(len)==0, "Unexpected start position for selection %d\n", LOWORD(len));
    ok(HIWORD(len)==0, "Unexpected end position for selection %d\n", HIWORD(len));

    /* Give it focus, and it gets selected */
    SendMessageA(hCombo, WM_SETFOCUS, 0, (LPARAM)hEdit);
    SendMessageA(hCombo, CB_GETEDITSEL, (WPARAM)&start, (WPARAM)&end);
    ok(start==0, "Unexpected start position for selection %d\n", start);
    ok(end==6, "Unexpected end position for selection %d\n", end);
    len = SendMessageA(hCombo, CB_GETEDITSEL, 0,0);
    ok(LOWORD(len)==0, "Unexpected start position for selection %d\n", LOWORD(len));
    ok(HIWORD(len)==6, "Unexpected end position for selection %d\n", HIWORD(len));

    /* Now emulate a key press */
    edit[0] = 0x00;
    SendMessageA(hCombo, WM_CHAR, 'A', 0x1c0001);
    SendMessageA(hCombo, WM_GETTEXT, sizeof(edit), (LPARAM)edit);
    ok(strcmp(edit, "A")==0, "Unexpected text retrieved %s\n", edit);

    len = SendMessageA(hCombo, CB_GETEDITSEL, 0,0);
    ok(LOWORD(len)==1, "Unexpected start position for selection %d\n", LOWORD(len));
    ok(HIWORD(len)==1, "Unexpected end position for selection %d\n", HIWORD(len));

    /* Now what happens when it gets more focus a second time - it doesn't reselect */
    SendMessageA(hCombo, WM_SETFOCUS, 0, (LPARAM)hEdit);
    len = SendMessageA(hCombo, CB_GETEDITSEL, 0,0);
    ok(LOWORD(len)==1, "Unexpected start position for selection %d\n", LOWORD(len));
    ok(HIWORD(len)==1, "Unexpected end position for selection %d\n", HIWORD(len));
    DestroyWindow(hCombo);

    /* Start again - Build a combo */
    hCombo = create_combobox(CBS_SIMPLE);
    get_combobox_info(hCombo, &cbInfo);
    hEdit = cbInfo.hwndItem;

    /* Set some text and give focus so it gets selected */
    edit[0] = 0x00;
    SendMessageA(hCombo, WM_SETTEXT, 0, (LPARAM)"Jason2");
    SendMessageA(hCombo, WM_GETTEXT, sizeof(edit), (LPARAM)edit);
    ok(strcmp(edit, "Jason2")==0, "Unexpected text retrieved %s\n", edit);

    SendMessageA(hCombo, WM_SETFOCUS, 0, (LPARAM)hEdit);

    /* Now what is the selection */
    SendMessageA(hCombo, CB_GETEDITSEL, (WPARAM)&start, (WPARAM)&end);
    ok(start==0, "Unexpected start position for selection %d\n", start);
    ok(end==6, "Unexpected end position for selection %d\n", end);
    len = SendMessageA(hCombo, CB_GETEDITSEL, 0,0);
    ok(LOWORD(len)==0, "Unexpected start position for selection %d\n", LOWORD(len));
    ok(HIWORD(len)==6, "Unexpected end position for selection %d\n", HIWORD(len));

    /* Now change the selection to the apparently invalid start -1, end -1 and
       show it means no selection (ie start -1) but cursor at end              */
    SendMessageA(hCombo, CB_SETEDITSEL, 0, -1);
    edit[0] = 0x00;
    SendMessageA(hCombo, WM_CHAR, 'A', 0x1c0001);
    SendMessageA(hCombo, WM_GETTEXT, sizeof(edit), (LPARAM)edit);
    ok(strcmp(edit, "Jason2A")==0, "Unexpected text retrieved %s\n", edit);
    DestroyWindow(hCombo);
}

static WNDPROC edit_window_proc;
static long setsel_start = 1, setsel_end = 1;
static HWND hCBN_SetFocus, hCBN_KillFocus;

static LRESULT CALLBACK combobox_subclass_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == EM_SETSEL)
    {
        setsel_start = wParam;
        setsel_end = lParam;
    }
    return CallWindowProcA(edit_window_proc, hwnd, msg, wParam, lParam);
}

static LRESULT CALLBACK test_window_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_COMMAND:
        switch (HIWORD(wParam))
        {
        case CBN_SETFOCUS:
            hCBN_SetFocus = (HWND)lParam;
            break;
        case CBN_KILLFOCUS:
            hCBN_KillFocus = (HWND)lParam;
            break;
        }
        break;
    case WM_NEXTDLGCTL:
        SetFocus((HWND)wParam);
        break;
    }
    return CallWindowProcA(old_parent_proc, hwnd, msg, wParam, lParam);
}

static void test_combo_editselection_focus(DWORD style)
{
    static const char wine_test[] = "Wine Test";
    HWND hCombo, hEdit, hButton;
    char buffer[16] = {0};
    COMBOBOXINFO cbInfo;
    DWORD len;

    hCombo = create_combobox(style);
    get_combobox_info(hCombo, &cbInfo);
    hEdit = cbInfo.hwndItem;

    hButton = CreateWindowA(WC_BUTTONA, "OK", WS_VISIBLE|WS_CHILD|BS_DEFPUSHBUTTON,
                            5, 50, 100, 20, hMainWnd, NULL,
                            (HINSTANCE)GetWindowLongPtrA(hMainWnd, GWLP_HINSTANCE), NULL);

    old_parent_proc = (WNDPROC)SetWindowLongPtrA(hMainWnd, GWLP_WNDPROC, (ULONG_PTR)test_window_proc);
    edit_window_proc = (WNDPROC)SetWindowLongPtrA(hEdit, GWLP_WNDPROC, (ULONG_PTR)combobox_subclass_proc);

    SendMessageA(hCombo, WM_SETFOCUS, 0, (LPARAM)hEdit);
    ok(setsel_start == 0, "Unexpected EM_SETSEL start value; got %ld\n", setsel_start);
    todo_wine ok(setsel_end == INT_MAX, "Unexpected EM_SETSEL end value; got %ld\n", setsel_end);
    ok(hCBN_SetFocus == hCombo, "Wrong handle set by CBN_SETFOCUS; got %p\n", hCBN_SetFocus);
    ok(GetFocus() == hEdit, "hEdit should have keyboard focus\n");

    SendMessageA(hMainWnd, WM_NEXTDLGCTL, (WPARAM)hButton, TRUE);
    ok(setsel_start == 0, "Unexpected EM_SETSEL start value; got %ld\n", setsel_start);
    todo_wine ok(setsel_end == 0, "Unexpected EM_SETSEL end value; got %ld\n", setsel_end);
    ok(hCBN_KillFocus == hCombo, "Wrong handle set by CBN_KILLFOCUS; got %p\n", hCBN_KillFocus);
    ok(GetFocus() == hButton, "hButton should have keyboard focus\n");

    SendMessageA(hCombo, WM_SETTEXT, 0, (LPARAM)wine_test);
    SendMessageA(hMainWnd, WM_NEXTDLGCTL, (WPARAM)hCombo, TRUE);
    ok(setsel_start == 0, "Unexpected EM_SETSEL start value; got %ld\n", setsel_start);
    todo_wine ok(setsel_end == INT_MAX, "Unexpected EM_SETSEL end value; got %ld\n", setsel_end);
    ok(hCBN_SetFocus == hCombo, "Wrong handle set by CBN_SETFOCUS; got %p\n", hCBN_SetFocus);
    ok(GetFocus() == hEdit, "hEdit should have keyboard focus\n");
    SendMessageA(hCombo, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);
    ok(!strcmp(buffer, wine_test), "Unexpected text in edit control; got '%s'\n", buffer);

    SendMessageA(hMainWnd, WM_NEXTDLGCTL, (WPARAM)hButton, TRUE);
    ok(setsel_start == 0, "Unexpected EM_SETSEL start value; got %ld\n", setsel_start);
    todo_wine ok(setsel_end == 0, "Unexpected EM_SETSEL end value; got %ld\n", setsel_end);
    ok(hCBN_KillFocus == hCombo, "Wrong handle set by CBN_KILLFOCUS; got %p\n", hCBN_KillFocus);
    ok(GetFocus() == hButton, "hButton should have keyboard focus\n");
    len = SendMessageA(hCombo, CB_GETEDITSEL, 0, 0);
    ok(len == 0, "Unexpected text selection; start: %u, end: %u\n", LOWORD(len), HIWORD(len));

    SetWindowLongPtrA(hMainWnd, GWLP_WNDPROC, (ULONG_PTR)old_parent_proc);
    DestroyWindow(hButton);
    DestroyWindow(hCombo);
}

static void test_combo_listbox_styles(DWORD cb_style)
{
    DWORD style, exstyle, expect_style, expect_exstyle;
    COMBOBOXINFO info;
    HWND combo;

    expect_style = WS_CHILD|WS_CLIPSIBLINGS|LBS_COMBOBOX|LBS_HASSTRINGS|LBS_NOTIFY;
    if (cb_style == CBS_SIMPLE)
    {
        expect_style |= WS_VISIBLE;
        expect_exstyle = WS_EX_CLIENTEDGE;
    }
    else
    {
        expect_style |= WS_BORDER;
        expect_exstyle = WS_EX_TOOLWINDOW;
    }

    combo = create_combobox(cb_style);
    get_combobox_info(combo, &info);

    style = GetWindowLongW( info.hwndList, GWL_STYLE );
    exstyle = GetWindowLongW( info.hwndList, GWL_EXSTYLE );
    ok(style == expect_style, "%08lx: got %08lx\n", cb_style, style);
    ok(exstyle == expect_exstyle, "%08lx: got %08lx\n", cb_style, exstyle);

    if (cb_style != CBS_SIMPLE)
        expect_exstyle |= WS_EX_TOPMOST;

    SendMessageW(combo, CB_SHOWDROPDOWN, TRUE, 0 );
    style = GetWindowLongW( info.hwndList, GWL_STYLE );
    exstyle = GetWindowLongW( info.hwndList, GWL_EXSTYLE );
    ok(style == (expect_style | WS_VISIBLE), "%08lx: got %08lx\n", cb_style, style);
    ok(exstyle == expect_exstyle, "%08lx: got %08lx\n", cb_style, exstyle);

    SendMessageW(combo, CB_SHOWDROPDOWN, FALSE, 0 );
    style = GetWindowLongW( info.hwndList, GWL_STYLE );
    exstyle = GetWindowLongW( info.hwndList, GWL_EXSTYLE );
    ok(style == expect_style, "%08lx: got %08lx\n", cb_style, style);
    ok(exstyle == expect_exstyle, "%08lx: got %08lx\n", cb_style, exstyle);

    DestroyWindow(combo);
}

static void test_combo_WS_VSCROLL(void)
{
    HWND hCombo, hList;
    COMBOBOXINFO info;
    DWORD style;
    int i;

    hCombo = create_combobox(CBS_DROPDOWNLIST);

    get_combobox_info(hCombo, &info);
    hList = info.hwndList;

    for (i = 0; i < 3; i++)
    {
        char buffer[2];
        sprintf(buffer, "%d", i);
        SendMessageA(hCombo, CB_ADDSTRING, 0, (LPARAM)buffer);
    }

    style = GetWindowLongA(info.hwndList, GWL_STYLE);
    SetWindowLongA(hList, GWL_STYLE, style | WS_VSCROLL);

    SendMessageA(hCombo, CB_SHOWDROPDOWN, TRUE, 0);
    SendMessageA(hCombo, CB_SHOWDROPDOWN, FALSE, 0);

    style = GetWindowLongA(hList, GWL_STYLE);
    ok((style & WS_VSCROLL) != 0, "Style does not include WS_VSCROLL\n");

    DestroyWindow(hCombo);
}

static void test_combo_dropdown_size(DWORD style)
{
    static const char wine_test[] = "Wine Test";
    HWND hCombo, hList;
    COMBOBOXINFO cbInfo;
    int i, test, ret;

    static const struct list_size_info
    {
        int num_items;
        int height_combo;
        int limit;
    } info_height[] = {
        {33, 50, -1},
        {35, 100, 40},
        {15, 50, 3},
    };

    for (test = 0; test < ARRAY_SIZE(info_height); test++)
    {
        const struct list_size_info *info_test = &info_height[test];
        int height_item; /* Height of a list item */
        int height_list; /* Height of the list we got */
        int expected_height_list;
        RECT rect_list_client;
        int min_visible_expected;

        winetest_push_context("Test %d", test);
        hCombo = CreateWindowA(WC_COMBOBOXA, "Combo", CBS_DROPDOWN | WS_VISIBLE | WS_CHILD | style, 5, 5, 100,
                info_test->height_combo, hMainWnd, (HMENU)COMBO_ID, NULL, 0);

        min_visible_expected = SendMessageA(hCombo, CB_GETMINVISIBLE, 0, 0);
        ok(min_visible_expected == 30, "Unexpected number of items %d.\n", min_visible_expected);

        cbInfo.cbSize = sizeof(COMBOBOXINFO);
        ret = SendMessageA(hCombo, CB_GETCOMBOBOXINFO, 0, (LPARAM)&cbInfo);
        ok(ret, "Failed to get combo info, %d\n", ret);

        hList = cbInfo.hwndList;
        for (i = 0; i < info_test->num_items; i++)
        {
            ret = SendMessageA(hCombo, CB_ADDSTRING, 0, (LPARAM) wine_test);
            ok(ret == i, "Failed to add string %d, returned %d.\n", i, ret);
        }

        if (info_test->limit != -1)
        {
            int min_visible_actual;
            min_visible_expected = info_test->limit;

            ret = SendMessageA(hCombo, CB_SETMINVISIBLE, min_visible_expected, 0);
            ok(ret, "Failed to set visible limit.\n");
            min_visible_actual = SendMessageA(hCombo, CB_GETMINVISIBLE, 0, 0);
            ok(min_visible_expected == min_visible_actual, "unexpected number of items %d.\n",
                    min_visible_actual);
        }

        ret = SendMessageA(hCombo, CB_SHOWDROPDOWN, TRUE,0);
        ok(ret, "Failed to show dropdown.\n");
        ret = SendMessageA(hCombo, CB_GETDROPPEDSTATE, 0, 0);
        ok(ret, "Unexpected dropped state.\n");

        GetClientRect(hList, &rect_list_client);
        height_list = rect_list_client.bottom - rect_list_client.top;
        height_item = (int)SendMessageA(hList, LB_GETITEMHEIGHT, 0, 0);

        if (style & CBS_NOINTEGRALHEIGHT)
        {
            RECT rect_list_complete;
            int list_height_nonclient;
            int list_height_calculated;
            int edit_padding_size = cbInfo.rcItem.top; /* edit client rect top is the padding it has to its parent
                                                          We assume it's the same on the bottom */

            GetWindowRect(hList, &rect_list_complete);

            list_height_nonclient = (rect_list_complete.bottom - rect_list_complete.top)
                                    - (rect_list_client.bottom - rect_list_client.top);

            /* Calculate the expected client size of the listbox popup from the size of the combobox. */
            list_height_calculated = info_test->height_combo      /* Take height we created combobox with */
                    - (cbInfo.rcItem.bottom - cbInfo.rcItem.top)  /* Subtract size of edit control */
                    - list_height_nonclient                       /* Subtract list nonclient area */
                    - edit_padding_size * 2;                      /* subtract space around the edit control */

            expected_height_list = min(list_height_calculated, height_item * info_test->num_items);
            if (expected_height_list < 0)
                expected_height_list = 0;

            ok(expected_height_list == height_list, "expected list height to be %d, got %d\n",
                    expected_height_list, height_list);
        }
        else
        {
            expected_height_list = min(info_test->num_items, min_visible_expected) * height_item;

            ok(expected_height_list == height_list, "expected list height to be %d, got %d\n",
                    expected_height_list, height_list);
        }

        DestroyWindow(hCombo);
        winetest_pop_context();
    }
}

static void test_combo_ctlcolor(void)
{
    static const int messages[] =
    {
        WM_CTLCOLOR,
        WM_CTLCOLORMSGBOX,
        WM_CTLCOLOREDIT,
        WM_CTLCOLORLISTBOX,
        WM_CTLCOLORBTN,
        WM_CTLCOLORDLG,
        WM_CTLCOLORSCROLLBAR,
        WM_CTLCOLORSTATIC,
    };

    HBRUSH brush, global_brush;
    COMBOBOXINFO info;
    unsigned int i;
    HWND combo;

    combo = create_combobox(CBS_DROPDOWN);
    ok(!!combo, "Failed to create combo window.\n");

    old_parent_proc = (void *)SetWindowLongPtrA(hMainWnd, GWLP_WNDPROC, (ULONG_PTR)parent_wnd_proc);

    get_combobox_info(combo, &info);

    lparam_for_WM_CTLCOLOR = info.hwndItem;

    /* Parent returns valid brush handle. */
    for (i = 0; i < ARRAY_SIZE(messages); ++i)
    {
        brush = (HBRUSH)SendMessageA(combo, messages[i], 0, (LPARAM)info.hwndItem);
        ok(brush == brush_red, "%u: unexpected brush %p, expected got %p.\n", i, brush, brush_red);
    }

    /* Parent returns NULL brush. */
    global_brush = brush_red;
    brush_red = NULL;

    for (i = 0; i < ARRAY_SIZE(messages); ++i)
    {
        brush = (HBRUSH)SendMessageA(combo, messages[i], 0, (LPARAM)info.hwndItem);
        ok(!brush, "%u: unexpected brush %p.\n", i, brush);
    }

    brush_red = global_brush;

    lparam_for_WM_CTLCOLOR = 0;

    /* Parent does default processing. */
    for (i = 0; i < ARRAY_SIZE(messages); ++i)
    {
        brush = (HBRUSH)SendMessageA(combo, messages[i], 0, (LPARAM)info.hwndItem);
        ok(!!brush && brush != brush_red, "%u: unexpected brush %p.\n", i, brush);
    }

    SetWindowLongPtrA(hMainWnd, GWLP_WNDPROC, (ULONG_PTR)old_parent_proc);
    DestroyWindow(combo);

    /* Combo without a parent. */
    combo = CreateWindowA(WC_COMBOBOXA, "Combo", CBS_DROPDOWN, 5, 5, 100, 100, NULL, NULL, NULL, 0);
    ok(!!combo, "Failed to create combo window.\n");

    get_combobox_info(combo, &info);

    for (i = 0; i < ARRAY_SIZE(messages); ++i)
    {
        brush = (HBRUSH)SendMessageA(combo, messages[i], 0, (LPARAM)info.hwndItem);
        ok(!brush, "%u: unexpected brush %p.\n", i, brush);
    }

    DestroyWindow(combo);
}

static const struct message getdisp_parent_seq[] =
{
    { WM_NOTIFY, sent|id, 0, 0, CBEN_GETDISPINFOA },
    { 0 }
};

static const struct message empty_seq[] =
{
    { 0 }
};

static void test_comboex_CBEN_GETDISPINFO(void)
{
    static const unsigned int test_masks[] =
    {
        CBEIF_TEXT,
        CBEIF_IMAGE,
        CBEIF_INDENT,
        CBEIF_OVERLAY,
        CBEIF_SELECTEDIMAGE,
        CBEIF_IMAGE | CBEIF_INDENT,
    };
    COMBOBOXEXITEMA item;
    unsigned int i;
    HWND combo;
    DWORD res;

    combo = createComboEx(WS_BORDER | WS_VISIBLE | WS_CHILD | CBS_DROPDOWN);
    ok(!!combo, "Failed to create control window.\n");

    /* All possible callback fields. */
    memset(&item, 0, sizeof(item));
    item.mask = CBEIF_TEXT | CBEIF_IMAGE | CBEIF_INDENT | CBEIF_OVERLAY | CBEIF_SELECTEDIMAGE;
    item.pszText = LPSTR_TEXTCALLBACKA;
    item.iImage = I_IMAGECALLBACK;
    item.iSelectedImage = I_IMAGECALLBACK;
    item.iOverlay = I_IMAGECALLBACK;
    item.iIndent = I_INDENTCALLBACK;

    res = SendMessageA(combo, CBEM_INSERTITEMA, 0, (LPARAM)&item);
    ok(!res, "Unexpected return value %lu.\n", res);

    for (i = 0; i < ARRAY_SIZE(test_masks); ++i)
    {
        flush_sequences(sequences, NUM_MSG_SEQUENCES);

        memset(&item, 0, sizeof(item));
        item.mask = test_masks[i];
        res = SendMessageA(combo, CBEM_GETITEMA, 0, (LPARAM)&item);
        ok(res == 1, "Unexpected return value %lu.\n", res);

        ok_sequence(sequences, PARENT_SEQ_INDEX, getdisp_parent_seq, "Get disp mask seq", TRUE);
    }

    di_context.set_CBEIF_DI_SETITEM = TRUE;

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    memset(&item, 0, sizeof(item));
    item.mask = CBEIF_IMAGE;
    di_context.mask = 0;
    res = SendMessageA(combo, CBEM_GETITEMA, 0, (LPARAM)&item);
    ok(res == 1, "Unexpected return value %lu.\n", res);
    todo_wine
    ok(di_context.mask == CBEIF_IMAGE, "Unexpected mask %#x.\n", di_context.mask);

    ok_sequence(sequences, PARENT_SEQ_INDEX, getdisp_parent_seq, "Get disp DI_SETITEM seq", TRUE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    memset(&item, 0, sizeof(item));
    item.mask = CBEIF_IMAGE;
    res = SendMessageA(combo, CBEM_GETITEMA, 0, (LPARAM)&item);
    ok(res == 1, "Unexpected return value %lu.\n", res);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq, "Get disp after DI_SETITEM seq", FALSE);

    /* Request two fields, one was set. */
    memset(&item, 0, sizeof(item));
    item.mask = CBEIF_IMAGE | CBEIF_INDENT;
    di_context.mask = 0;
    res = SendMessageA(combo, CBEM_GETITEMA, 0, (LPARAM)&item);
    ok(res == 1, "Unexpected return value %lu.\n", res);
    todo_wine
    ok(di_context.mask == CBEIF_INDENT, "Unexpected mask %#x.\n", di_context.mask);

    di_context.set_CBEIF_DI_SETITEM = FALSE;

    DestroyWindow(combo);
}

START_TEST(combo)
{
    ULONG_PTR ctx_cookie;
    HANDLE hCtx;

    init_functions();

    if (!init())
        return;

    init_msg_sequences(sequences, NUM_MSG_SEQUENCES);

    /* ComboBoxEx32 tests. */
    test_comboex();
    test_comboex_WM_LBUTTONDOWN();
    test_comboex_CB_GETLBTEXT();
    test_comboex_WM_WINDOWPOSCHANGING();
    test_comboex_subclass();
    test_comboex_get_set_item();
    test_comboex_CBEN_GETDISPINFO();

    if (!load_v6_module(&ctx_cookie, &hCtx))
    {
        cleanup();
        return;
    }

    test_comboex();
    test_comboex_CB_GETLBTEXT();
    test_comboex_WM_WINDOWPOSCHANGING();
    test_comboex_get_set_item();
    test_comboex_CBEN_GETDISPINFO();

    /* ComboBox control tests. */
    test_combo_WS_VSCROLL();
    test_combo_setfont(CBS_DROPDOWN);
    test_combo_setfont(CBS_DROPDOWNLIST);
    test_combo_setitemheight(CBS_DROPDOWN);
    test_combo_setitemheight(CBS_DROPDOWNLIST);
    test_combo_CBN_SELCHANGE();
    test_combo_changesize(CBS_DROPDOWN);
    test_combo_changesize(CBS_DROPDOWNLIST);
    test_combo_editselection();
    test_combo_editselection_focus(CBS_SIMPLE);
    test_combo_editselection_focus(CBS_DROPDOWN);
    test_combo_listbox_styles(CBS_SIMPLE);
    test_combo_listbox_styles(CBS_DROPDOWN);
    test_combo_listbox_styles(CBS_DROPDOWNLIST);
    test_combo_dropdown_size(0);
    test_combo_dropdown_size(CBS_NOINTEGRALHEIGHT);
    test_combo_ctlcolor();

    cleanup();
    unload_v6_module(ctx_cookie, hCtx);
}
