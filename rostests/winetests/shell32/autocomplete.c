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

static void test_invalid_init(void)
{
    HRESULT hr;
    IAutoComplete *ac;
    IUnknown *acSource;
    HWND edit_control;

    /* AutoComplete instance */
    hr = CoCreateInstance(&CLSID_AutoComplete, NULL, CLSCTX_INPROC_SERVER,
                         &IID_IAutoComplete, (void **)&ac);
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip("CLSID_AutoComplete is not registered\n");
        return;
    }
    ok(hr == S_OK, "no IID_IAutoComplete (0x%08x)\n", hr);

    /* AutoComplete source */
    hr = CoCreateInstance(&CLSID_ACLMulti, NULL, CLSCTX_INPROC_SERVER,
                        &IID_IACList, (void **)&acSource);
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip("CLSID_ACLMulti is not registered\n");
        IAutoComplete_Release(ac);
        return;
    }
    ok(hr == S_OK, "no IID_IACList (0x%08x)\n", hr);

    edit_control = CreateWindowExA(0, "EDIT", "Some text", 0, 10, 10, 300, 300,
                       hMainWnd, NULL, hinst, NULL);
    ok(edit_control != NULL, "Can't create edit control\n");

    /* The refcount of acSource would be incremented on older Windows. */
    hr = IAutoComplete_Init(ac, NULL, acSource, NULL, NULL);
    ok(hr == E_INVALIDARG ||
       broken(hr == S_OK), /* Win2k/XP/Win2k3 */
       "Init returned 0x%08x\n", hr);
    if (hr == E_INVALIDARG)
    {
        LONG ref;

        IUnknown_AddRef(acSource);
        ref = IUnknown_Release(acSource);
        ok(ref == 1, "Expected AutoComplete source refcount to be 1, got %d\n", ref);
    }

if (0)
{
    /* Older Windows versions never check the window handle, while newer
     * versions only check for NULL. Subsequent attempts to initialize the
     * object after this call succeeds would fail, because initialization
     * state is determined by whether a non-NULL window handle is stored. */
    hr = IAutoComplete_Init(ac, (HWND)0xdeadbeef, acSource, NULL, NULL);
    ok(hr == S_OK, "Init returned 0x%08x\n", hr);

    /* Tests crash on older Windows. */
    hr = IAutoComplete_Init(ac, NULL, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "Init returned 0x%08x\n", hr);

    hr = IAutoComplete_Init(ac, edit_control, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "Init returned 0x%08x\n", hr);
}

    /* bind to edit control */
    hr = IAutoComplete_Init(ac, edit_control, acSource, NULL, NULL);
    ok(hr == S_OK, "Init returned 0x%08x\n", hr);

    /* try invalid parameters after successful initialization .*/
    hr = IAutoComplete_Init(ac, NULL, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG ||
       hr == E_FAIL, /* Win2k/XP/Win2k3 */
       "Init returned 0x%08x\n", hr);

    hr = IAutoComplete_Init(ac, NULL, acSource, NULL, NULL);
    ok(hr == E_INVALIDARG ||
       hr == E_FAIL, /* Win2k/XP/Win2k3 */
       "Init returned 0x%08x\n", hr);

    hr = IAutoComplete_Init(ac, edit_control, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG ||
       hr == E_FAIL, /* Win2k/XP/Win2k3 */
       "Init returned 0x%08x\n", hr);

    /* try initializing twice on the same control */
    hr = IAutoComplete_Init(ac, edit_control, acSource, NULL, NULL);
    ok(hr == E_FAIL, "Init returned 0x%08x\n", hr);

    /* try initializing with a different control */
    hr = IAutoComplete_Init(ac, hEdit, acSource, NULL, NULL);
    ok(hr == E_FAIL, "Init returned 0x%08x\n", hr);

    DestroyWindow(edit_control);

    /* try initializing with a different control after
     * destroying the original initialization control */
    hr = IAutoComplete_Init(ac, hEdit, acSource, NULL, NULL);
    ok(hr == E_UNEXPECTED ||
       hr == E_FAIL, /* Win2k/XP/Win2k3 */
       "Init returned 0x%08x\n", hr);

    IUnknown_Release(acSource);
    IAutoComplete_Release(ac);
}
static IAutoComplete *test_init(void)
{
    HRESULT r;
    IAutoComplete *ac;
    IUnknown *acSource;
    LONG_PTR user_data;

    /* AutoComplete instance */
    r = CoCreateInstance(&CLSID_AutoComplete, NULL, CLSCTX_INPROC_SERVER,
                         &IID_IAutoComplete, (LPVOID*)&ac);
    if (r == REGDB_E_CLASSNOTREG)
    {
        win_skip("CLSID_AutoComplete is not registered\n");
        return NULL;
    }
    ok(r == S_OK, "no IID_IAutoComplete (0x%08x)\n", r);

    /* AutoComplete source */
    r = CoCreateInstance(&CLSID_ACLMulti, NULL, CLSCTX_INPROC_SERVER,
                        &IID_IACList, (LPVOID*)&acSource);
    if (r == REGDB_E_CLASSNOTREG)
    {
        win_skip("CLSID_ACLMulti is not registered\n");
        IAutoComplete_Release(ac);
        return NULL;
    }
    ok(r == S_OK, "no IID_IACList (0x%08x)\n", r);

    user_data = GetWindowLongPtrA(hEdit, GWLP_USERDATA);
    ok(user_data == 0, "Expected the edit control user data to be zero\n");

    /* bind to edit control */
    r = IAutoComplete_Init(ac, hEdit, acSource, NULL, NULL);
    ok(r == S_OK, "Init returned 0x%08x\n", r);

    user_data = GetWindowLongPtrA(hEdit, GWLP_USERDATA);
    ok(user_data == 0, "Expected the edit control user data to be zero\n");

    IUnknown_Release(acSource);

    return ac;
}

static void test_killfocus(void)
{
    /* Test if WM_KILLFOCUS messages are handled properly by checking if
     * the parent receives an EN_KILLFOCUS message. */
    SetFocus(hEdit);
    killfocus_count = 0;
    SetFocus(0);
    ok(killfocus_count == 1, "Expected one EN_KILLFOCUS message, got: %d\n", killfocus_count);
}

static LRESULT CALLBACK MyWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg) {
    case WM_CREATE:
        /* create edit control */
        hEdit = CreateWindowExA(0, "EDIT", "Some text", 0, 10, 10, 300, 300,
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

static void createMainWnd(void)
{
    WNDCLASSA wc;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.hIcon = NULL;
    wc.hCursor = LoadCursorA(NULL, (LPSTR)IDC_IBEAM);
    wc.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "MyTestWnd";
    wc.lpfnWndProc = MyWndProc;
    RegisterClassA(&wc);

    hMainWnd = CreateWindowExA(0, "MyTestWnd", "Blah", WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT, 130, 105, NULL, NULL, GetModuleHandleA(NULL), 0);
}

START_TEST(autocomplete)
{
    HRESULT r;
    MSG msg;
    IAutoComplete* ac;

    r = CoInitialize(NULL);
    ok(r == S_OK, "CoInitialize failed (0x%08x). Tests aborted.\n", r);
    if (r != S_OK)
        return;

    createMainWnd();
    ok(hMainWnd != NULL, "Failed to create parent window. Tests aborted.\n");
    if (!hMainWnd) return;

    test_invalid_init();
    ac = test_init();
    if (!ac)
        goto cleanup;
    test_killfocus();

    PostQuitMessage(0);
    while(GetMessageA(&msg,0,0,0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    IAutoComplete_Release(ac);

cleanup:
    DestroyWindow(hEdit);
    DestroyWindow(hMainWnd);

    CoUninitialize();
}
