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

}

LRESULT CALLBACK ComboExTestWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
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

static void init(void) {
    WNDCLASSA wc;
    INITCOMMONCONTROLSEX icex;

    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC   = ICC_USEREX_CLASSES;
    InitCommonControlsEx(&icex);

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.hIcon = NULL;
    wc.hCursor = LoadCursorA(NULL, MAKEINTRESOURCEA(IDC_ARROW));
    wc.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = ComboExTestClass;
    wc.lpfnWndProc = ComboExTestWndProc;
    RegisterClassA(&wc);

    hComboExParentWnd = CreateWindowExA(0, ComboExTestClass, "ComboEx test", WS_OVERLAPPEDWINDOW, 
      CW_USEDEFAULT, CW_USEDEFAULT, 680, 260, NULL, NULL, GetModuleHandleA(NULL), 0);
    assert(hComboExParentWnd != NULL);

    hMainHinst = GetModuleHandleA(NULL);

}

static void cleanup(void)
{
    MSG msg;
    
    PostMessageA(hComboExParentWnd, WM_CLOSE, 0, 0);
    while (GetMessageA(&msg,0,0,0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    
    UnregisterClassA(ComboExTestClass, GetModuleHandleA(NULL));
}

START_TEST(comboex)
{
    init();

    test_comboboxex();

    cleanup();
}
