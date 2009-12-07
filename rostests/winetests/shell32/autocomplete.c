/*
 * Tests for autocomplete
 *
 * Copyright 2008 Jan de Mooij
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

#define COBJMACROS

#include <wine/test.h>
#include <stdarg.h>

#include "windows.h"
#include "shobjidl.h"
#include "shlguid.h"
#include "initguid.h"
#include "shldisp.h"

static HWND hMainWnd, hEdit;
static HINSTANCE hinst;
static int killfocus_count;

static BOOL test_init(void) {
    HRESULT r;
    IAutoComplete* ac;
    IUnknown *acSource;

    /* AutoComplete instance */
    r = CoCreateInstance(&CLSID_AutoComplete, NULL, CLSCTX_INPROC_SERVER,
                         &IID_IAutoComplete, (LPVOID*)&ac);
    if (r == REGDB_E_CLASSNOTREG)
    {
        win_skip("CLSID_AutoComplete is not registered\n");
        return FALSE;
    }
    ok(SUCCEEDED(r), "no IID_IAutoComplete (0x%08x)\n", r);

    /* AutoComplete source */
    r = CoCreateInstance(&CLSID_ACLMulti, NULL, CLSCTX_INPROC_SERVER,
                        &IID_IACList, (LPVOID*)&acSource);
    if (r == REGDB_E_CLASSNOTREG)
    {
        win_skip("CLSID_ACLMulti is not registered\n");
        return FALSE;
    }
    ok(SUCCEEDED(r), "no IID_IACList (0x%08x)\n", r);

    /* bind to edit control */
    r = IAutoComplete_Init(ac, hEdit, acSource, NULL, NULL);
    ok(SUCCEEDED(r), "Init failed (0x%08x)\n", r);

    return TRUE;
}
static void test_killfocus(void) {
    /* Test if WM_KILLFOCUS messages are handled properly by checking if
     * the parent receives an EN_KILLFOCUS message. */
    SetFocus(hEdit);
    killfocus_count = 0;
    SetFocus(0);
    ok(killfocus_count == 1, "Expected one EN_KILLFOCUS message, got: %d\n", killfocus_count);
}
static LRESULT CALLBACK MyWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
    case WM_CREATE:
        /* create edit control */
        hEdit = CreateWindowEx(0, "EDIT", "Some text", 0, 10, 10, 300, 300,
                    hWnd, NULL, hinst, NULL);
        ok(hEdit != NULL, "Can't create edit control\n");
        break;
    case WM_COMMAND:
        if(HIWORD(wParam) == EN_KILLFOCUS)
            killfocus_count++;
        break;
    }
    return DefWindowProcA(hWnd, msg, wParam, lParam);
}
static void createMainWnd(void) {
    WNDCLASSA wc;
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
}
START_TEST(autocomplete) {
    HRESULT r;
    MSG msg;

    r = CoInitialize(NULL);
    ok(SUCCEEDED(r), "CoInitialize failed (0x%08x). Tests aborted.\n", r);
    if (FAILED(r))
        return;

    createMainWnd();

    if(!ok(hMainWnd != NULL, "Failed to create parent window. Tests aborted.\n"))
        return;

    if (!test_init())
        goto cleanup;
    test_killfocus();

    PostQuitMessage(0);
    while(GetMessageA(&msg,0,0,0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

cleanup:
    DestroyWindow(hEdit);
    DestroyWindow(hMainWnd);

    CoUninitialize();
}
