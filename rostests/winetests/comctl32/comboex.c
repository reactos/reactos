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
#include <windows.h>
#include <commctrl.h>

#include "wine/test.h"

static HWND hComboExParentWnd;
static HINSTANCE hMainHinst;
static const char ComboExTestClass[] = "ComboExTestClass";

#define MAX_CHARS 100
static char *textBuffer = NULL;

static HWND createComboEx(DWORD style) {
   return CreateWindowExA(0, WC_COMBOBOXEXA, NULL, style, 0, 0, 300, 300,
            hComboExParentWnd, NULL, hMainHinst, NULL);
}

static LONG addItem(HWND cbex, int idx, LPTSTR text) {
    COMBOBOXEXITEM cbexItem;
    memset(&cbexItem, 0x00, sizeof(cbexItem));
    cbexItem.mask = CBEIF_TEXT;
    cbexItem.iItem = idx;
    cbexItem.pszText    = text;
    cbexItem.cchTextMax = 0;
    return (LONG)SendMessage(cbex, CBEM_INSERTITEM, 0,(LPARAM)&cbexItem);
}

static LONG setItem(HWND cbex, int idx, LPTSTR text) {
    COMBOBOXEXITEM cbexItem;
    memset(&cbexItem, 0x00, sizeof(cbexItem));
    cbexItem.mask = CBEIF_TEXT;
    cbexItem.iItem = idx;
    cbexItem.pszText    = text;
    cbexItem.cchTextMax = 0;
    return (LONG)SendMessage(cbex, CBEM_SETITEM, 0,(LPARAM)&cbexItem);
}

static LONG delItem(HWND cbex, int idx) {
    return (LONG)SendMessage(cbex, CBEM_DELETEITEM, (LPARAM)idx, 0);
}

static LONG getItem(HWND cbex, int idx, COMBOBOXEXITEM *cbItem) {
    memset(cbItem, 0x00, sizeof(COMBOBOXEXITEM));
    cbItem->mask = CBEIF_TEXT;
    cbItem->pszText      = textBuffer;
    cbItem->iItem        = idx;
    cbItem->cchTextMax   = 100;
    return (LONG)SendMessage(cbex, CBEM_GETITEM, 0, (LPARAM)cbItem);
}

static void test_comboboxex(void) {
    HWND myHwnd = 0;
    LONG res = -1;
    COMBOBOXEXITEM cbexItem;
    static TCHAR first_item[]        = {'F','i','r','s','t',' ','I','t','e','m',0},
                 second_item[]       = {'S','e','c','o','n','d',' ','I','t','e','m',0},
                 third_item[]        = {'T','h','i','r','d',' ','I','t','e','m',0},
                 middle_item[]       = {'B','e','t','w','e','e','n',' ','F','i','r','s','t',' ','a','n','d',' ',
                                        'S','e','c','o','n','d',' ','I','t','e','m','s',0},
                 replacement_item[]  = {'B','e','t','w','e','e','n',' ','F','i','r','s','t',' ','a','n','d',' ',
                                        'S','e','c','o','n','d',' ','I','t','e','m','s',0},
                 out_of_range_item[] = {'O','u','t',' ','o','f',' ','R','a','n','g','e',' ','I','t','e','m',0};

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
    int i, idx;
    RECT rect;
    WCHAR buffer[3];
    static const UINT choices[] = {8,9,10,11,12,14,16,18,20,22,24,26,28,36,48,72};
    static const WCHAR stringFormat[] = {'%','2','d','\0'};
    BOOL (WINAPI *pGetComboBoxInfo)(HWND, PCOMBOBOXINFO);

    pGetComboBoxInfo = (void*)GetProcAddress(GetModuleHandleA("user32.dll"), "GetComboBoxInfo");
    if (!pGetComboBoxInfo){
        skip("GetComboBoxInfo is not available\n");
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

    hCombo = (HWND)SendMessage(hComboEx, CBEM_GETCOMBOCONTROL, 0, 0);
    hEdit = (HWND)SendMessage(hComboEx, CBEM_GETEDITCONTROL, 0, 0);

    cbInfo.cbSize = sizeof(COMBOBOXINFO);
    result = pGetComboBoxInfo(hCombo, &cbInfo);
    ok(result, "Failed to get combobox info structure. LastError=%d\n",
       GetLastError());
    hList = cbInfo.hwndList;

    trace("hWnd=%p, hComboEx=%p, hCombo=%p, hList=%p, hEdit=%p\n",
         hComboExParentWnd, hComboEx, hCombo, hList, hEdit);
    ok(GetFocus() == hComboExParentWnd,
       "Focus not on Main Window, instead on %p\n", GetFocus());

    /* Click on the button to drop down the list */
    x = cbInfo.rcButton.left + (cbInfo.rcButton.right-cbInfo.rcButton.left)/2;
    y = cbInfo.rcButton.top + (cbInfo.rcButton.bottom-cbInfo.rcButton.top)/2;
    result = SendMessage(hCombo, WM_LBUTTONDOWN, 0, MAKELPARAM(x, y));
    ok(result, "WM_LBUTTONDOWN was not processed. LastError=%d\n",
       GetLastError());
    ok(GetFocus() == hCombo ||
       broken(GetFocus() != hCombo), /* win98 */
       "Focus not on ComboBoxEx's ComboBox Control, instead on %p\n",
       GetFocus());
    ok(SendMessage(hComboEx, CB_GETDROPPEDSTATE, 0, 0),
       "The dropdown list should have appeared after clicking the button.\n");
    idx = SendMessage(hCombo, CB_GETTOPINDEX, 0, 0);
    ok(idx == 0, "For TopIndex expected %d, got %d\n", 0, idx);

    result = SendMessage(hCombo, WM_LBUTTONUP, 0, MAKELPARAM(x, y));
    ok(result, "WM_LBUTTONUP was not processed. LastError=%d\n",
       GetLastError());
    ok(GetFocus() == hCombo ||
       broken(GetFocus() != hCombo), /* win98 */
       "Focus not on ComboBoxEx's ComboBox Control, instead on %p\n",
       GetFocus());

    /* Click on the 5th item in the list */
    item_height = SendMessage(hCombo, CB_GETITEMHEIGHT, 0, 0);
    ok(GetClientRect(hList, &rect), "Failed to get list's client rect.\n");
    x = rect.left + (rect.right-rect.left)/2;
    y = item_height/2 + item_height*4;
    result = SendMessage(hList, WM_MOUSEMOVE, 0, MAKELPARAM(x, y));
    ok(!result, "WM_MOUSEMOVE was not processed. LastError=%d\n",
       GetLastError());
    ok(GetFocus() == hCombo ||
       broken(GetFocus() != hCombo), /* win98 */
       "Focus not on ComboBoxEx's ComboBox Control, instead on %p\n",
       GetFocus());

    result = SendMessage(hList, WM_LBUTTONDOWN, 0, MAKELPARAM(x, y));
    ok(!result, "WM_LBUTTONDOWN was not processed. LastError=%d\n",
       GetLastError());
    ok(GetFocus() == hCombo ||
       broken(GetFocus() != hCombo), /* win98 */
       "Focus not on ComboBoxEx's ComboBox Control, instead on %p\n",
       GetFocus());
    ok(SendMessage(hComboEx, CB_GETDROPPEDSTATE, 0, 0),
       "The dropdown list should still be visible.\n");

    result = SendMessage(hList, WM_LBUTTONUP, 0, MAKELPARAM(x, y));
    ok(!result, "WM_LBUTTONUP was not processed. LastError=%d\n",
       GetLastError());
    todo_wine ok(GetFocus() == hEdit ||
       broken(GetFocus() == hCombo), /* win98 */
       "Focus not on ComboBoxEx's Edit Control, instead on %p\n",
       GetFocus());

    result = SendMessage(hCombo, CB_GETDROPPEDSTATE, 0, 0);
    ok(!result ||
       broken(result != 0), /* win98 */
       "The dropdown list should have been rolled up.\n");
    idx = SendMessage(hComboEx, CB_GETCURSEL, 0, 0);
    ok(idx == 4 ||
       broken(idx == -1), /* win98 */
       "Current Selection: expected %d, got %d\n", 4, idx);

    DestroyWindow(hComboEx);
}

static LRESULT CALLBACK ComboExTestWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg) {

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
    wc.lpszClassName = ComboExTestClass;
    wc.lpfnWndProc = ComboExTestWndProc;
    RegisterClassA(&wc);

    hComboExParentWnd = CreateWindowExA(0, ComboExTestClass, "ComboEx test", WS_OVERLAPPEDWINDOW|WS_VISIBLE,
      CW_USEDEFAULT, CW_USEDEFAULT, 680, 260, NULL, NULL, GetModuleHandleA(NULL), 0);
    assert(hComboExParentWnd != NULL);

    hMainHinst = GetModuleHandleA(NULL);
    return 1;
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

START_TEST(comboex)
{
    if (!init())
        return;

    test_comboboxex();
    test_WM_LBUTTONDOWN();

    cleanup();
}
