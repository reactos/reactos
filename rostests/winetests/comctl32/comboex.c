/* Unit test suite for comboex control.
 *
 * Copyright 2005 Jason Edmeades
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
//#include <windows.h>
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <commctrl.h>

//#include "wine/test.h"
#include "msg.h"

#define EDITBOX_SEQ_INDEX  0
#define NUM_MSG_SEQUENCES  1

#define EDITBOX_ID         0

#define expect(expected, got) ok(got == expected, "Expected %d, got %d\n", expected, got)

static struct msg_sequence *sequences[NUM_MSG_SEQUENCES];

static HWND hComboExParentWnd;
static HINSTANCE hMainHinst;
static const char ComboExTestClass[] = "ComboExTestClass";

static BOOL (WINAPI *pSetWindowSubclass)(HWND, SUBCLASSPROC, UINT_PTR, DWORD_PTR);

#define MAX_CHARS 100
static char *textBuffer = NULL;

static BOOL received_end_edit = FALSE;

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
    LRESULT ret;
    struct message msg;

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

static void test_comboboxex(void) {
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
    textBuffer = HeapAlloc(GetProcessHeap(), 0, MAX_CHARS);

    /* Basic comboboxex test */
    myHwnd = createComboEx(WS_BORDER | WS_VISIBLE | WS_CHILD | CBS_DROPDOWN);

    /* Add items onto the end of the combobox */
    res = addItem(myHwnd, -1, first_item);
    ok(res == 0, "Adding simple item failed (%d)\n", res);
    res = addItem(myHwnd, -1, second_item);
    ok(res == 1, "Adding simple item failed (%d)\n", res);
    res = addItem(myHwnd, 2, third_item);
    ok(res == 2, "Adding simple item failed (%d)\n", res);
    res = addItem(myHwnd, 1, middle_item);
    ok(res == 1, "Inserting simple item failed (%d)\n", res);

    /* Add an item completely out of range */
    res = addItem(myHwnd, 99, out_of_range_item);
    ok(res == -1, "Adding using out of range index worked unexpectedly (%d)\n", res);
    res = addItem(myHwnd, 5, out_of_range_item);
    ok(res == -1, "Adding using out of range index worked unexpectedly (%d)\n", res);
    /* Removed: Causes traps on Windows XP
       res = addItem(myHwnd, -2, "Out Of Range Item");
       ok(res == -1, "Adding out of range worked unexpectedly (%ld)\n", res);
     */

    /* Get an item completely out of range */ 
    res = getItem(myHwnd, 99, &cbexItem); 
    ok(res == 0, "Getting item using out of range index worked unexpectedly (%d, %s)\n", res, cbexItem.pszText);
    res = getItem(myHwnd, 4, &cbexItem); 
    ok(res == 0, "Getting item using out of range index worked unexpectedly (%d, %s)\n", res, cbexItem.pszText);
    res = getItem(myHwnd, -2, &cbexItem); 
    ok(res == 0, "Getting item using out of range index worked unexpectedly (%d, %s)\n", res, cbexItem.pszText);

    /* Get an item in range */ 
    res = getItem(myHwnd, 0, &cbexItem); 
    ok(res != 0, "Getting item using valid index failed unexpectedly (%d)\n", res);
    ok(strcmp(first_item, cbexItem.pszText) == 0, "Getting item returned wrong string (%s)\n", cbexItem.pszText);

    res = getItem(myHwnd, 1, &cbexItem); 
    ok(res != 0, "Getting item using valid index failed unexpectedly (%d)\n", res);
    ok(strcmp(middle_item, cbexItem.pszText) == 0, "Getting item returned wrong string (%s)\n", cbexItem.pszText);

    res = getItem(myHwnd, 2, &cbexItem); 
    ok(res != 0, "Getting item using valid index failed unexpectedly (%d)\n", res);
    ok(strcmp(second_item, cbexItem.pszText) == 0, "Getting item returned wrong string (%s)\n", cbexItem.pszText);

    res = getItem(myHwnd, 3, &cbexItem); 
    ok(res != 0, "Getting item using valid index failed unexpectedly (%d)\n", res);
    ok(strcmp(third_item, cbexItem.pszText) == 0, "Getting item returned wrong string (%s)\n", cbexItem.pszText);

    /* Set an item completely out of range */ 
    res = setItem(myHwnd, 99, replacement_item); 
    ok(res == 0, "Setting item using out of range index worked unexpectedly (%d)\n", res);
    res = setItem(myHwnd, 4, replacement_item); 
    ok(res == 0, "Setting item using out of range index worked unexpectedly (%d)\n", res);
    res = setItem(myHwnd, -2, replacement_item); 
    ok(res == 0, "Setting item using out of range index worked unexpectedly (%d)\n", res);

    /* Set an item in range */ 
    res = setItem(myHwnd, 0, replacement_item);
    ok(res != 0, "Setting first item failed (%d)\n", res);
    res = setItem(myHwnd, 3, replacement_item);
    ok(res != 0, "Setting last item failed (%d)\n", res);

    /* Remove items completely out of range (4 items in control at this point) */
    res = delItem(myHwnd, -1);
    ok(res == CB_ERR, "Deleting using out of range index worked unexpectedly (%d)\n", res);
    res = delItem(myHwnd, 4);
    ok(res == CB_ERR, "Deleting using out of range index worked unexpectedly (%d)\n", res);

    /* Remove items in range (4 items in control at this point) */
    res = delItem(myHwnd, 3);
    ok(res == 3, "Deleting using out of range index failed (%d)\n", res);
    res = delItem(myHwnd, 0);
    ok(res == 2, "Deleting using out of range index failed (%d)\n", res);
    res = delItem(myHwnd, 0);
    ok(res == 1, "Deleting using out of range index failed (%d)\n", res);
    res = delItem(myHwnd, 0);
    ok(res == 0, "Deleting using out of range index failed (%d)\n", res);

    /* Remove from an empty box */
    res = delItem(myHwnd, 0);
    ok(res == CB_ERR, "Deleting using out of range index worked unexpectedly (%d)\n", res);


    /* Cleanup */
    HeapFree(GetProcessHeap(), 0, textBuffer);
    DestroyWindow(myHwnd);
}

static void test_WM_LBUTTONDOWN(void)
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
    static const WCHAR stringFormat[] = {'%','2','d','\0'};
    BOOL (WINAPI *pGetComboBoxInfo)(HWND, PCOMBOBOXINFO);

    pGetComboBoxInfo = (void*)GetProcAddress(GetModuleHandleA("user32.dll"), "GetComboBoxInfo");
    if (!pGetComboBoxInfo){
        win_skip("GetComboBoxInfo is not available\n");
        return;
    }

    hComboEx = CreateWindowExA(0, WC_COMBOBOXEXA, NULL,
            WS_VISIBLE|WS_CHILD|CBS_DROPDOWN, 0, 0, 200, 150,
            hComboExParentWnd, NULL, hMainHinst, NULL);

    for (i = 0; i < sizeof(choices)/sizeof(UINT); i++){
        COMBOBOXEXITEMW cbexItem;
        wsprintfW(buffer, stringFormat, choices[i]);

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

    cbInfo.cbSize = sizeof(COMBOBOXINFO);
    result = pGetComboBoxInfo(hCombo, &cbInfo);
    ok(result, "Failed to get combobox info structure. LastError=%d\n",
       GetLastError());
    hList = cbInfo.hwndList;

    ok(GetFocus() == hComboExParentWnd,
       "Focus not on Main Window, instead on %p\n", GetFocus());

    /* Click on the button to drop down the list */
    x = cbInfo.rcButton.left + (cbInfo.rcButton.right-cbInfo.rcButton.left)/2;
    y = cbInfo.rcButton.top + (cbInfo.rcButton.bottom-cbInfo.rcButton.top)/2;
    result = SendMessageA(hCombo, WM_LBUTTONDOWN, 0, MAKELPARAM(x, y));
    ok(result, "WM_LBUTTONDOWN was not processed. LastError=%d\n",
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
    ok(result, "WM_LBUTTONUP was not processed. LastError=%d\n",
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
    ok(!result, "WM_MOUSEMOVE was not processed. LastError=%d\n",
       GetLastError());
    ok(GetFocus() == hCombo ||
       broken(GetFocus() != hCombo), /* win98 */
       "Focus not on ComboBoxEx's ComboBox Control, instead on %p\n",
       GetFocus());

    result = SendMessageA(hList, WM_LBUTTONDOWN, 0, MAKELPARAM(x, y));
    ok(!result, "WM_LBUTTONDOWN was not processed. LastError=%d\n",
       GetLastError());
    ok(GetFocus() == hCombo ||
       broken(GetFocus() != hCombo), /* win98 */
       "Focus not on ComboBoxEx's ComboBox Control, instead on %p\n",
       GetFocus());
    ok(SendMessageA(hComboEx, CB_GETDROPPEDSTATE, 0, 0),
       "The dropdown list should still be visible.\n");

    result = SendMessageA(hList, WM_LBUTTONUP, 0, MAKELPARAM(x, y));
    ok(!result, "WM_LBUTTONUP was not processed. LastError=%d\n",
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

static void test_CB_GETLBTEXT(void)
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

static void test_WM_WINDOWPOSCHANGING(void)
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

static LRESULT ComboExTestOnNotify(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    NMHDR *hdr = (NMHDR*)lParam;
    switch(hdr->code){
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

static BOOL init(void)
{
    HMODULE hComctl32;
    BOOL (WINAPI *pInitCommonControlsEx)(const INITCOMMONCONTROLSEX*);
    WNDCLASSA wc;
    INITCOMMONCONTROLSEX iccex;

    hComctl32 = GetModuleHandleA("comctl32.dll");
    pInitCommonControlsEx = (void*)GetProcAddress(hComctl32, "InitCommonControlsEx");
    if (!pInitCommonControlsEx)
    {
        win_skip("InitCommonControlsEx() is missing. Skipping the tests\n");
        return FALSE;
    }
    iccex.dwSize = sizeof(iccex);
    iccex.dwICC  = ICC_USEREX_CLASSES;
    pInitCommonControlsEx(&iccex);

    pSetWindowSubclass = (void*)GetProcAddress(hComctl32, (LPSTR)410);

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
}

static void test_comboboxex_subclass(void)
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

static void test_get_set_item(void)
{
    char textA[] = "test";
    HWND hComboEx;
    COMBOBOXEXITEMA item;
    BOOL ret;

    hComboEx = createComboEx(WS_BORDER | WS_VISIBLE | WS_CHILD | CBS_DROPDOWN);

    subclass_editbox(hComboEx);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    memset(&item, 0, sizeof(item));
    item.mask = CBEIF_TEXT;
    item.pszText = textA;
    item.iItem = -1;
    ret = SendMessageA(hComboEx, CBEM_SETITEMA, 0, (LPARAM)&item);
    expect(TRUE, ret);

    ok_sequence(sequences, EDITBOX_SEQ_INDEX, test_setitem_edit_seq, "set item data for edit", FALSE);

    /* get/set lParam */
    item.mask = CBEIF_LPARAM;
    item.iItem = -1;
    item.lParam = 0xdeadbeef;
    ret = SendMessageA(hComboEx, CBEM_GETITEMA, 0, (LPARAM)&item);
    expect(TRUE, ret);
    ok(item.lParam == 0, "Expected zero, got %lx\n", item.lParam);

    item.lParam = 0x1abe11ed;
    ret = SendMessageA(hComboEx, CBEM_SETITEMA, 0, (LPARAM)&item);
    expect(TRUE, ret);

    item.lParam = 0;
    ret = SendMessageA(hComboEx, CBEM_GETITEMA, 0, (LPARAM)&item);
    expect(TRUE, ret);
    ok(item.lParam == 0x1abe11ed, "Expected 0x1abe11ed, got %lx\n", item.lParam);

    DestroyWindow(hComboEx);
}

START_TEST(comboex)
{
    if (!init())
        return;

    init_msg_sequences(sequences, NUM_MSG_SEQUENCES);

    test_comboboxex();
    test_WM_LBUTTONDOWN();
    test_CB_GETLBTEXT();
    test_WM_WINDOWPOSCHANGING();
    test_comboboxex_subclass();
    test_get_set_item();

    cleanup();
}
