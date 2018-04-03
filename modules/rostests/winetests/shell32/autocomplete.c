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

#include <stdarg.h>

#include "windows.h"
#include "shobjidl.h"
#include "shlguid.h"
#include "initguid.h"
#include "shldisp.h"

#include "wine/heap.h"
#include "wine/test.h"

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

struct string_enumerator
{
    IEnumString IEnumString_iface;
    LONG ref;
    WCHAR **data;
    int data_len;
    int cur;
};

static struct string_enumerator *impl_from_IEnumString(IEnumString *iface)
{
    return CONTAINING_RECORD(iface, struct string_enumerator, IEnumString_iface);
}

static HRESULT WINAPI string_enumerator_QueryInterface(IEnumString *iface, REFIID riid, void **ppv)
{
    if (IsEqualGUID(riid, &IID_IEnumString) || IsEqualGUID(riid, &IID_IUnknown))
    {
        IUnknown_AddRef(iface);
        *ppv = iface;
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI string_enumerator_AddRef(IEnumString *iface)
{
    struct string_enumerator *this = impl_from_IEnumString(iface);

    ULONG ref = InterlockedIncrement(&this->ref);

    return ref;
}

static ULONG WINAPI string_enumerator_Release(IEnumString *iface)
{
    struct string_enumerator *this = impl_from_IEnumString(iface);

    ULONG ref = InterlockedDecrement(&this->ref);

    if (!ref)
        heap_free(this);

    return ref;
}

static HRESULT WINAPI string_enumerator_Next(IEnumString *iface, ULONG num, LPOLESTR *strings, ULONG *num_returned)
{
    struct string_enumerator *this = impl_from_IEnumString(iface);
    int i, len;

    *num_returned = 0;
    for (i = 0; i < num; i++)
    {
        if (this->cur >= this->data_len)
            return S_FALSE;

        len = lstrlenW(this->data[this->cur]) + 1;

        strings[i] = CoTaskMemAlloc(len * sizeof(WCHAR));
        memcpy(strings[i], this->data[this->cur], len * sizeof(WCHAR));

        (*num_returned)++;
        this->cur++;
    }

    return S_OK;
}

static HRESULT WINAPI string_enumerator_Reset(IEnumString *iface)
{
    struct string_enumerator *this = impl_from_IEnumString(iface);

    this->cur = 0;

    return S_OK;
}

static HRESULT WINAPI string_enumerator_Skip(IEnumString *iface, ULONG num)
{
    struct string_enumerator *this = impl_from_IEnumString(iface);

    this->cur += num;

    return S_OK;
}

static HRESULT WINAPI string_enumerator_Clone(IEnumString *iface, IEnumString **out)
{
    *out = NULL;
    return E_NOTIMPL;
}

static IEnumStringVtbl string_enumerator_vtlb =
{
    string_enumerator_QueryInterface,
    string_enumerator_AddRef,
    string_enumerator_Release,
    string_enumerator_Next,
    string_enumerator_Skip,
    string_enumerator_Reset,
    string_enumerator_Clone
};

static HRESULT string_enumerator_create(void **ppv, WCHAR **suggestions, int count)
{
    struct string_enumerator *object;

    object = heap_alloc_zero(sizeof(*object));
    object->IEnumString_iface.lpVtbl = &string_enumerator_vtlb;
    object->ref = 1;
    object->data = suggestions;
    object->data_len = count;
    object->cur = 0;

    *ppv = &object->IEnumString_iface;

    return S_OK;
}

static void test_custom_source(void)
{
    static WCHAR str_alpha[] = {'t','e','s','t','1',0};
    static WCHAR str_alpha2[] = {'t','e','s','t','2',0};
    static WCHAR str_beta[] = {'a','u','t','o',' ','c','o','m','p','l','e','t','e',0};
    static WCHAR *suggestions[] = { str_alpha, str_alpha2, str_beta };
    IUnknown *enumerator;
    IAutoComplete2 *autocomplete;
    HWND hwnd_edit;
    WCHAR buffer[20];
    HRESULT hr;
    MSG msg;

    ShowWindow(hMainWnd, SW_SHOW);

    hwnd_edit = CreateWindowA("Edit", "", WS_OVERLAPPED | WS_VISIBLE | WS_CHILD | WS_BORDER, 50, 5, 200, 20, hMainWnd, 0, NULL, 0);

    hr = CoCreateInstance(&CLSID_AutoComplete, NULL, CLSCTX_INPROC_SERVER, &IID_IAutoComplete2, (void**)&autocomplete);
    ok(hr == S_OK, "CoCreateInstance failed: %x\n", hr);

    string_enumerator_create((void**)&enumerator, suggestions, sizeof(suggestions) / sizeof(*suggestions));

    hr = IAutoComplete2_SetOptions(autocomplete, ACO_AUTOSUGGEST | ACO_AUTOAPPEND);
    ok(hr == S_OK, "IAutoComplete2_SetOptions failed: %x\n", hr);
    hr = IAutoComplete2_Init(autocomplete, hwnd_edit, enumerator, NULL, NULL);
    ok(hr == S_OK, "IAutoComplete_Init failed: %x\n", hr);

    SendMessageW(hwnd_edit, WM_CHAR, 'a', 1);
    /* Send a keyup message since wine doesn't handle WM_CHAR yet */
    SendMessageW(hwnd_edit, WM_KEYUP, 'u', 1);
    Sleep(100);
    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    SendMessageW(hwnd_edit, WM_GETTEXT, sizeof(buffer) / sizeof(*buffer), (LPARAM)buffer);
    ok(lstrcmpW(str_beta, buffer) == 0, "Expected %s, got %s\n", wine_dbgstr_w(str_beta), wine_dbgstr_w(buffer));

    ShowWindow(hMainWnd, SW_HIDE);
    DestroyWindow(hwnd_edit);
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

    test_custom_source();

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
