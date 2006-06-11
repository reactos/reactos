/* Unit tests for treeview.
 *
 * Copyright 2005 Krzysztof Foltman
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
 
static void MakeButton(TBBUTTON *p, int idCommand, int fsStyle, int nString) {
  p->iBitmap = -2;
  p->idCommand = idCommand;
  p->fsState = TBSTATE_ENABLED;
  p->fsStyle = fsStyle;
  p->iString = nString;
}

static LRESULT CALLBACK MyWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg) {
                  
    case WM_CREATE:
    {
        TBBUTTON buttons[9];
        int i;
        HWND hToolbar;
        for (i=0; i<9; i++)
            MakeButton(buttons+i, 1000+i, TBSTYLE_CHECKGROUP, 0);
        MakeButton(buttons+3, 1003, TBSTYLE_SEP|TBSTYLE_GROUP, 0);
        MakeButton(buttons+6, 1006, TBSTYLE_SEP, 0);

        hToolbar = CreateToolbarEx(hWnd,
            WS_VISIBLE | WS_CLIPCHILDREN | CCS_TOP |
            WS_CHILD | TBSTYLE_LIST,
            100,
            0, NULL, (UINT)0,
            buttons, sizeof(buttons)/sizeof(buttons[0]),
            0, 0, 20, 16, sizeof(TBBUTTON));
        ok(hToolbar != NULL, "Toolbar creation\n");

        SendMessage(hToolbar, TB_ADDSTRINGA, 0, (LPARAM)"test\000");

        /* test for exclusion working inside a separator-separated :-) group */
        SendMessage(hToolbar, TB_CHECKBUTTON, 1000, 1); /* press A1 */
        ok(SendMessage(hToolbar, TB_ISBUTTONCHECKED, 1000, 0), "A1 pressed\n");
        ok(!SendMessage(hToolbar, TB_ISBUTTONCHECKED, 1001, 0), "A2 not pressed\n");

        SendMessage(hToolbar, TB_CHECKBUTTON, 1004, 1); /* press A5, release A1 */
        ok(SendMessage(hToolbar, TB_ISBUTTONCHECKED, 1004, 0), "A5 pressed\n");
        ok(!SendMessage(hToolbar, TB_ISBUTTONCHECKED, 1000, 0), "A1 not pressed anymore\n");

        SendMessage(hToolbar, TB_CHECKBUTTON, 1005, 1); /* press A6, release A5 */
        ok(SendMessage(hToolbar, TB_ISBUTTONCHECKED, 1005, 0), "A6 pressed\n");
        ok(!SendMessage(hToolbar, TB_ISBUTTONCHECKED, 1004, 0), "A5 not pressed anymore\n");

        /* test for inter-group crosstalk, ie. two radio groups interfering with each other */
        SendMessage(hToolbar, TB_CHECKBUTTON, 1007, 1); /* press B2 */
        ok(SendMessage(hToolbar, TB_ISBUTTONCHECKED, 1005, 0), "A6 still pressed, no inter-group crosstalk\n");
        ok(!SendMessage(hToolbar, TB_ISBUTTONCHECKED, 1000, 0), "A1 still not pressed\n");
        ok(SendMessage(hToolbar, TB_ISBUTTONCHECKED, 1007, 0), "B2 pressed\n");

        SendMessage(hToolbar, TB_CHECKBUTTON, 1000, 1); /* press A1 and ensure B group didn't suffer */
        ok(!SendMessage(hToolbar, TB_ISBUTTONCHECKED, 1005, 0), "A6 not pressed anymore\n");
        ok(SendMessage(hToolbar, TB_ISBUTTONCHECKED, 1000, 0), "A1 pressed\n");
        ok(SendMessage(hToolbar, TB_ISBUTTONCHECKED, 1007, 0), "B2 still pressed\n");

        SendMessage(hToolbar, TB_CHECKBUTTON, 1008, 1); /* press B3, and ensure A group didn't suffer */
        ok(!SendMessage(hToolbar, TB_ISBUTTONCHECKED, 1005, 0), "A6 pressed\n");
        ok(SendMessage(hToolbar, TB_ISBUTTONCHECKED, 1000, 0), "A1 pressed\n");
        ok(!SendMessage(hToolbar, TB_ISBUTTONCHECKED, 1007, 0), "B2 not pressed\n");
        ok(SendMessage(hToolbar, TB_ISBUTTONCHECKED, 1008, 0), "B3 pressed\n");
        PostMessage(hWnd, WM_CLOSE, 0, 0);
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

START_TEST(toolbar)
{
    WNDCLASSA wc;
    MSG msg;
    RECT rc;
    HWND hMainWnd;
  
    InitCommonControls();
  
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.hIcon = NULL;
    wc.hCursor = LoadCursorA(NULL, MAKEINTRESOURCEA(IDC_IBEAM));
    wc.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "MyTestWnd";
    wc.lpfnWndProc = MyWndProc;
    RegisterClassA(&wc);
    
    hMainWnd = CreateWindowExA(0, "MyTestWnd", "Blah", WS_OVERLAPPEDWINDOW, 
      CW_USEDEFAULT, CW_USEDEFAULT, 680, 260, NULL, NULL, GetModuleHandleA(NULL), 0);
    GetClientRect(hMainWnd, &rc);

    while(GetMessageA(&msg,0,0,0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}
