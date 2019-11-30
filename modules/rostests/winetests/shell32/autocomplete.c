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
#include "shldisp.h"
#include "shlobj.h"

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
    IAutoComplete *ac, *ac2;
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

    /* bind a different object to the same edit control */
    r = CoCreateInstance(&CLSID_AutoComplete, NULL, CLSCTX_INPROC_SERVER,
                         &IID_IAutoComplete, (LPVOID*)&ac2);
    ok(r == S_OK, "no IID_IAutoComplete (0x%08x)\n", r);

    r = IAutoComplete_Init(ac2, hEdit, acSource, NULL, NULL);
    ok(r == S_OK, "Init returned 0x%08x\n", r);
    IAutoComplete_Release(ac2);

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

static WNDPROC HijackerWndProc_prev;
static const WCHAR HijackerWndProc_txt[] = {'H','i','j','a','c','k','e','d',0};
static LRESULT CALLBACK HijackerWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg) {
    case WM_GETTEXT:
    {
        size_t len = min(wParam, ARRAY_SIZE(HijackerWndProc_txt));
        memcpy((void*)lParam, HijackerWndProc_txt, len * sizeof(WCHAR));
        return len;
    }
    case WM_GETTEXTLENGTH:
        return ARRAY_SIZE(HijackerWndProc_txt) - 1;
    }
    return CallWindowProcW(HijackerWndProc_prev, hWnd, msg, wParam, lParam);
}

static LRESULT CALLBACK HijackerWndProc2(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg) {
    case EM_SETSEL:
        lParam = wParam;
        break;
    case WM_SETTEXT:
        lParam = (LPARAM)HijackerWndProc_txt;
        break;
    }
    return CallWindowProcW(HijackerWndProc_prev, hWnd, msg, wParam, lParam);
}

struct string_enumerator
{
    IEnumString IEnumString_iface;
    IACList IACList_iface;
    LONG ref;
    WCHAR **data;
    int data_len;
    int cur;
    UINT num_resets;
    UINT num_expand;
    WCHAR last_expand[32];
};

static struct string_enumerator *impl_from_IEnumString(IEnumString *iface)
{
    return CONTAINING_RECORD(iface, struct string_enumerator, IEnumString_iface);
}

static HRESULT WINAPI string_enumerator_QueryInterface(IEnumString *iface, REFIID riid, void **ppv)
{
    struct string_enumerator *this = impl_from_IEnumString(iface);
    if (IsEqualGUID(riid, &IID_IEnumString) || IsEqualGUID(riid, &IID_IUnknown))
        *ppv = &this->IEnumString_iface;
    else if (IsEqualGUID(riid, &IID_IACList))
        *ppv = &this->IACList_iface;
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef(&this->IEnumString_iface);
    return S_OK;
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
    this->num_resets++;

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

static IEnumStringVtbl string_enumerator_vtbl =
{
    string_enumerator_QueryInterface,
    string_enumerator_AddRef,
    string_enumerator_Release,
    string_enumerator_Next,
    string_enumerator_Skip,
    string_enumerator_Reset,
    string_enumerator_Clone
};

static struct string_enumerator *impl_from_IACList(IACList *iface)
{
    return CONTAINING_RECORD(iface, struct string_enumerator, IACList_iface);
}

static HRESULT WINAPI aclist_QueryInterface(IACList *iface, REFIID riid, void **ppv)
{
    return string_enumerator_QueryInterface(&impl_from_IACList(iface)->IEnumString_iface, riid, ppv);
}

static ULONG WINAPI aclist_AddRef(IACList *iface)
{
    return string_enumerator_AddRef(&impl_from_IACList(iface)->IEnumString_iface);
}

static ULONG WINAPI aclist_Release(IACList *iface)
{
    return string_enumerator_Release(&impl_from_IACList(iface)->IEnumString_iface);
}

static HRESULT WINAPI aclist_Expand(IACList *iface, const WCHAR *expand)
{
    struct string_enumerator *this = impl_from_IACList(iface);

    /* see what we get called with and how many times,
       don't actually do any expansion of the strings */
    memcpy(this->last_expand, expand, min((lstrlenW(expand) + 1)*sizeof(WCHAR), sizeof(this->last_expand)));
    this->last_expand[ARRAY_SIZE(this->last_expand) - 1] = '\0';
    this->num_expand++;

    return S_OK;
}

static IACListVtbl aclist_vtbl =
{
    aclist_QueryInterface,
    aclist_AddRef,
    aclist_Release,
    aclist_Expand
};

static HRESULT string_enumerator_create(void **ppv, WCHAR **suggestions, int count)
{
    struct string_enumerator *object;

    object = heap_alloc_zero(sizeof(*object));
    object->IEnumString_iface.lpVtbl = &string_enumerator_vtbl;
    object->IACList_iface.lpVtbl = &aclist_vtbl;
    object->ref = 1;
    object->data = suggestions;
    object->data_len = count;
    object->cur = 0;

    *ppv = &object->IEnumString_iface;

    return S_OK;
}

static void dispatch_messages(void)
{
    MSG msg;
    Sleep(33);
    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}

#define check_dropdown(acdropdown, hwnd_edit, list, list_num) check_dropdown_(__FILE__, __LINE__, acdropdown, hwnd_edit, list, list_num)
static void check_dropdown_(const char *file, UINT line, IAutoCompleteDropDown *acdropdown, HWND hwnd_edit, WCHAR **list, UINT list_num)
{
    UINT i;
    DWORD flags = 0;
    LPWSTR str;
    HRESULT hr;

    hr = IAutoCompleteDropDown_GetDropDownStatus(acdropdown, &flags, &str);
    ok_(file, line)(hr == S_OK, "IAutoCompleteDropDown_GetDropDownStatus failed: %x\n", hr);
    if (hr != S_OK) return;
    if (list_num) ok_(file, line)(flags & ACDD_VISIBLE, "AutoComplete DropDown not visible\n");
    else
    {
        ok_(file, line)(!(flags & ACDD_VISIBLE), "AutoComplete DropDown visible\n");
        return;
    }
    ok_(file, line)(str == NULL, "Expected (null), got %s\n", wine_dbgstr_w(str));
    if (str)
    {
        CoTaskMemFree(str);
        return;
    }

    for (i = 0; i <= list_num; i++)
    {
        flags = 0;
        SendMessageW(hwnd_edit, WM_KEYDOWN, VK_DOWN, 0);
        SendMessageW(hwnd_edit, WM_KEYUP, VK_DOWN, 0xc0000000);
        hr = IAutoCompleteDropDown_GetDropDownStatus(acdropdown, &flags, &str);
        ok_(file, line)(hr == S_OK, "IAutoCompleteDropDown_GetDropDownStatus failed: %x\n", hr);
        ok_(file, line)(flags & ACDD_VISIBLE, "AutoComplete DropDown not visible\n");
        if (hr == S_OK)
        {
            if (i < list_num)
                ok_(file, line)(str && !lstrcmpW(list[i], str), "Expected %s, got %s\n",
                                wine_dbgstr_w(list[i]), wine_dbgstr_w(str));
            else
                ok_(file, line)(str == NULL, "Expected (null), got %s\n", wine_dbgstr_w(str));
        }
        CoTaskMemFree(str);
    }
}

static void test_aclist_expand(HWND hwnd_edit, void *enumerator, IAutoCompleteDropDown *acdropdown)
{
    struct string_enumerator *obj = (struct string_enumerator*)enumerator;
    static WCHAR str1[] = {'t','e','s','t',0};
    static WCHAR str1a[] = {'t','e','s','t','\\',0};
    static WCHAR str2[] = {'t','e','s','t','\\','f','o','o','\\','b','a','r','\\','b','a',0};
    static WCHAR str2a[] = {'t','e','s','t','\\','f','o','o','\\','b','a','r','\\',0};
    static WCHAR str2b[] = {'t','e','s','t','\\','f','o','o','\\','b','a','r','\\','b','a','z','_','b','b','q','\\',0};
    HRESULT hr;
    obj->num_resets = 0;

    ok(obj->num_expand == 0, "Expected 0 expansions, got %u\n", obj->num_expand);
    SendMessageW(hwnd_edit, WM_SETTEXT, 0, (LPARAM)str1);
    SendMessageW(hwnd_edit, EM_SETSEL, ARRAY_SIZE(str1) - 1, ARRAY_SIZE(str1) - 1);
    SendMessageW(hwnd_edit, WM_CHAR, '\\', 1);
    dispatch_messages();
    ok(obj->num_expand == 1, "Expected 1 expansion, got %u\n", obj->num_expand);
    ok(lstrcmpW(obj->last_expand, str1a) == 0, "Expected %s, got %s\n", wine_dbgstr_w(str1a), wine_dbgstr_w(obj->last_expand));
    ok(obj->num_resets == 1, "Expected 1 reset, got %u\n", obj->num_resets);
    SendMessageW(hwnd_edit, WM_SETTEXT, 0, (LPARAM)str2);
    SendMessageW(hwnd_edit, EM_SETSEL, ARRAY_SIZE(str2) - 1, ARRAY_SIZE(str2) - 1);
    SendMessageW(hwnd_edit, WM_CHAR, 'z', 1);
    dispatch_messages();
    ok(obj->num_expand == 2, "Expected 2 expansions, got %u\n", obj->num_expand);
    ok(lstrcmpW(obj->last_expand, str2a) == 0, "Expected %s, got %s\n", wine_dbgstr_w(str2a), wine_dbgstr_w(obj->last_expand));
    ok(obj->num_resets == 2, "Expected 2 resets, got %u\n", obj->num_resets);
    SetFocus(hwnd_edit);
    SendMessageW(hwnd_edit, WM_CHAR, '_', 1);
    SendMessageW(hwnd_edit, WM_CHAR, 'b', 1);
    SetFocus(0);
    SetFocus(hwnd_edit);
    SendMessageW(hwnd_edit, WM_CHAR, 'b', 1);
    SendMessageW(hwnd_edit, WM_CHAR, 'q', 1);
    dispatch_messages();
    ok(obj->num_expand == 2, "Expected 2 expansions, got %u\n", obj->num_expand);
    ok(obj->num_resets == 2, "Expected 2 resets, got %u\n", obj->num_resets);
    SendMessageW(hwnd_edit, WM_CHAR, '\\', 1);
    dispatch_messages();
    ok(obj->num_expand == 3, "Expected 3 expansions, got %u\n", obj->num_expand);
    ok(lstrcmpW(obj->last_expand, str2b) == 0, "Expected %s, got %s\n", wine_dbgstr_w(str2b), wine_dbgstr_w(obj->last_expand));
    ok(obj->num_resets == 3, "Expected 3 resets, got %u\n", obj->num_resets);
    SendMessageW(hwnd_edit, EM_SETSEL, ARRAY_SIZE(str1a) - 1, -1);
    SendMessageW(hwnd_edit, WM_CHAR, 'x', 1);
    SendMessageW(hwnd_edit, WM_CHAR, 'y', 1);
    dispatch_messages();
    ok(obj->num_expand == 4, "Expected 4 expansions, got %u\n", obj->num_expand);
    ok(lstrcmpW(obj->last_expand, str1a) == 0, "Expected %s, got %s\n", wine_dbgstr_w(str1a), wine_dbgstr_w(obj->last_expand));
    ok(obj->num_resets == 4, "Expected 4 resets, got %u\n", obj->num_resets);
    SendMessageW(hwnd_edit, EM_SETSEL, ARRAY_SIZE(str1) - 1, -1);
    SendMessageW(hwnd_edit, WM_CHAR, 'x', 1);
    dispatch_messages();
    ok(obj->num_expand == 4, "Expected 4 expansions, got %u\n", obj->num_expand);
    ok(obj->num_resets == 5, "Expected 5 resets, got %u\n", obj->num_resets);
    SendMessageW(hwnd_edit, WM_SETTEXT, 0, (LPARAM)str1a);
    SendMessageW(hwnd_edit, EM_SETSEL, ARRAY_SIZE(str1a) - 1, ARRAY_SIZE(str1a) - 1);
    SendMessageW(hwnd_edit, WM_CHAR, 'f', 1);
    dispatch_messages();
    ok(obj->num_expand == 5, "Expected 5 expansions, got %u\n", obj->num_expand);
    ok(lstrcmpW(obj->last_expand, str1a) == 0, "Expected %s, got %s\n", wine_dbgstr_w(str1a), wine_dbgstr_w(obj->last_expand));
    ok(obj->num_resets == 6, "Expected 6 resets, got %u\n", obj->num_resets);
    hr = IAutoCompleteDropDown_ResetEnumerator(acdropdown);
    ok(hr == S_OK, "IAutoCompleteDropDown_ResetEnumerator failed: %x\n", hr);
    SendMessageW(hwnd_edit, WM_CHAR, 'o', 1);
    dispatch_messages();
    ok(obj->num_expand == 6, "Expected 6 expansions, got %u\n", obj->num_expand);
    ok(lstrcmpW(obj->last_expand, str1a) == 0, "Expected %s, got %s\n", wine_dbgstr_w(str1a), wine_dbgstr_w(obj->last_expand));
    ok(obj->num_resets == 7, "Expected 7 resets, got %u\n", obj->num_resets);
}

static void test_prefix_filtering(HWND hwnd_edit)
{
    static WCHAR htt[]  = {'h','t','t',0};
    static WCHAR www[]  = {'w','w','w','.',0};
    static WCHAR str0[] = {'w','w','w','.','a','x',0};
    static WCHAR str1[] = {'h','t','t','p','s',':','/','/','w','w','w','.','a','c',0};
    static WCHAR str2[] = {'a','a',0};
    static WCHAR str3[] = {'a','b',0};
    static WCHAR str4[] = {'h','t','t','p',':','/','/','a','0',0};
    static WCHAR str5[] = {'h','t','t','p','s',':','/','/','h','t','a',0};
    static WCHAR str6[] = {'h','f','o','o',0};
    static WCHAR str7[] = {'h','t','t','p',':','/','/','w','w','w','.','a','d','d',0};
    static WCHAR str8[] = {'w','w','w','.','w','w','w','.','?',0};
    static WCHAR str9[] = {'h','t','t','p',':','/','/','a','b','c','.','a','a','.','c','o','m',0};
    static WCHAR str10[]= {'f','t','p',':','/','/','a','b','c',0};
    static WCHAR str11[]= {'f','i','l','e',':','/','/','a','a',0};
    static WCHAR str12[]= {'f','t','p',':','/','/','w','w','w','.','a','a',0};
    static WCHAR *suggestions[] = { str0, str1, str2, str3, str4, str5, str6, str7, str8, str9, str10, str11, str12 };
    static WCHAR *sorted1[] = { str4, str2, str3, str9, str1, str7, str0 };
    static WCHAR *sorted2[] = { str3, str9 };
    static WCHAR *sorted3[] = { str1, str7, str0 };
    static WCHAR *sorted4[] = { str6, str5 };
    static WCHAR *sorted5[] = { str5 };
    static WCHAR *sorted6[] = { str4, str9 };
    static WCHAR *sorted7[] = { str11, str10, str12 };
    IUnknown *enumerator;
    IAutoComplete2 *autocomplete;
    IAutoCompleteDropDown *acdropdown;
    WCHAR buffer[20];
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_AutoComplete, NULL, CLSCTX_INPROC_SERVER, &IID_IAutoComplete2, (void**)&autocomplete);
    ok(hr == S_OK, "CoCreateInstance failed: %x\n", hr);

    hr = IAutoComplete2_QueryInterface(autocomplete, &IID_IAutoCompleteDropDown, (LPVOID*)&acdropdown);
    ok(hr == S_OK, "No IAutoCompleteDropDown interface: %x\n", hr);

    string_enumerator_create((void**)&enumerator, suggestions, ARRAY_SIZE(suggestions));

    hr = IAutoComplete2_SetOptions(autocomplete, ACO_FILTERPREFIXES | ACO_AUTOSUGGEST | ACO_AUTOAPPEND);
    ok(hr == S_OK, "IAutoComplete2_SetOptions failed: %x\n", hr);
    hr = IAutoComplete2_Init(autocomplete, hwnd_edit, enumerator, NULL, NULL);
    ok(hr == S_OK, "IAutoComplete_Init failed: %x\n", hr);

    SendMessageW(hwnd_edit, EM_SETSEL, 0, -1);
    SendMessageW(hwnd_edit, WM_CHAR, 'a', 1);
    dispatch_messages();
    SendMessageW(hwnd_edit, WM_GETTEXT, ARRAY_SIZE(buffer), (LPARAM)buffer);
    ok(lstrcmpW(str4 + 7, buffer) == 0, "Expected %s, got %s\n", wine_dbgstr_w(str4 + 7), wine_dbgstr_w(buffer));
    check_dropdown(acdropdown, hwnd_edit, sorted1, ARRAY_SIZE(sorted1));

    SendMessageW(hwnd_edit, EM_SETSEL, 0, -1);
    SendMessageW(hwnd_edit, WM_CHAR, 'a', 1);
    SendMessageW(hwnd_edit, WM_CHAR, 'b', 1);
    dispatch_messages();
    SendMessageW(hwnd_edit, WM_GETTEXT, ARRAY_SIZE(buffer), (LPARAM)buffer);
    ok(lstrcmpW(str3, buffer) == 0, "Expected %s, got %s\n", wine_dbgstr_w(str3), wine_dbgstr_w(buffer));
    check_dropdown(acdropdown, hwnd_edit, sorted2, ARRAY_SIZE(sorted2));
    SendMessageW(hwnd_edit, EM_SETSEL, 0, -1);
    SendMessageW(hwnd_edit, WM_CHAR, 'a', 1);
    SendMessageW(hwnd_edit, WM_CHAR, 'b', 1);
    SendMessageW(hwnd_edit, WM_CHAR, 'c', 1);
    dispatch_messages();
    SendMessageW(hwnd_edit, WM_GETTEXT, ARRAY_SIZE(buffer), (LPARAM)buffer);
    ok(lstrcmpW(str9 + 7, buffer) == 0, "Expected %s, got %s\n", wine_dbgstr_w(str9 + 7), wine_dbgstr_w(buffer));

    SendMessageW(hwnd_edit, WM_SETTEXT, 0, (LPARAM)www);
    SendMessageW(hwnd_edit, EM_SETSEL, ARRAY_SIZE(www) - 1, ARRAY_SIZE(www) - 1);
    SendMessageW(hwnd_edit, WM_CHAR, 'a', 1);
    dispatch_messages();
    SendMessageW(hwnd_edit, WM_GETTEXT, ARRAY_SIZE(buffer), (LPARAM)buffer);
    ok(lstrcmpW(str1 + 8, buffer) == 0, "Expected %s, got %s\n", wine_dbgstr_w(str1 + 8), wine_dbgstr_w(buffer));
    check_dropdown(acdropdown, hwnd_edit, sorted3, ARRAY_SIZE(sorted3));
    SendMessageW(hwnd_edit, WM_SETTEXT, 0, (LPARAM)www);
    SendMessageW(hwnd_edit, EM_SETSEL, ARRAY_SIZE(www) - 1, ARRAY_SIZE(www) - 1);
    SendMessageW(hwnd_edit, WM_CHAR, 'w', 1);
    dispatch_messages();
    SendMessageW(hwnd_edit, WM_GETTEXT, ARRAY_SIZE(buffer), (LPARAM)buffer);
    ok(lstrcmpW(str8, buffer) == 0, "Expected %s, got %s\n", wine_dbgstr_w(str8), wine_dbgstr_w(buffer));

    SendMessageW(hwnd_edit, EM_SETSEL, 0, -1);
    SendMessageW(hwnd_edit, WM_CHAR, 'h', 1);
    dispatch_messages();
    SendMessageW(hwnd_edit, WM_GETTEXT, ARRAY_SIZE(buffer), (LPARAM)buffer);
    ok(lstrcmpW(str6, buffer) == 0, "Expected %s, got %s\n", wine_dbgstr_w(str6), wine_dbgstr_w(buffer));
    check_dropdown(acdropdown, hwnd_edit, sorted4, ARRAY_SIZE(sorted4));
    SendMessageW(hwnd_edit, WM_CHAR, 't', 1);
    SendMessageW(hwnd_edit, WM_GETTEXT, ARRAY_SIZE(buffer), (LPARAM)buffer);
    ok(lstrcmpW(str5 + 8, buffer) == 0, "Expected %s, got %s\n", wine_dbgstr_w(str5 + 8), wine_dbgstr_w(buffer));
    check_dropdown(acdropdown, hwnd_edit, sorted5, ARRAY_SIZE(sorted5));
    SendMessageW(hwnd_edit, WM_CHAR, 't', 1);
    SendMessageW(hwnd_edit, WM_GETTEXT, ARRAY_SIZE(buffer), (LPARAM)buffer);
    ok(lstrcmpW(htt, buffer) == 0, "Expected %s, got %s\n", wine_dbgstr_w(htt), wine_dbgstr_w(buffer));
    check_dropdown(acdropdown, hwnd_edit, NULL, 0);
    SendMessageW(hwnd_edit, WM_CHAR, 'p', 1);
    SendMessageW(hwnd_edit, WM_CHAR, ':', 1);
    SendMessageW(hwnd_edit, WM_CHAR, '/', 1);
    SendMessageW(hwnd_edit, WM_CHAR, '/', 1);
    SendMessageW(hwnd_edit, WM_CHAR, 'a', 1);
    dispatch_messages();
    SendMessageW(hwnd_edit, WM_GETTEXT, ARRAY_SIZE(buffer), (LPARAM)buffer);
    ok(lstrcmpW(str4, buffer) == 0, "Expected %s, got %s\n", wine_dbgstr_w(str4), wine_dbgstr_w(buffer));
    check_dropdown(acdropdown, hwnd_edit, sorted6, ARRAY_SIZE(sorted6));
    SendMessageW(hwnd_edit, EM_SETSEL, 0, 2);
    SendMessageW(hwnd_edit, WM_CHAR, 'H', 1);
    dispatch_messages();
    check_dropdown(acdropdown, hwnd_edit, NULL, 0);
    SendMessageW(hwnd_edit, WM_CHAR, 't', 1);
    dispatch_messages();
    check_dropdown(acdropdown, hwnd_edit, sorted6, ARRAY_SIZE(sorted6));

    SendMessageW(hwnd_edit, EM_SETSEL, 0, -1);
    SendMessageW(hwnd_edit, WM_CHAR, 'F', 1);
    dispatch_messages();
    SendMessageW(hwnd_edit, WM_GETTEXT, ARRAY_SIZE(buffer), (LPARAM)buffer);
    check_dropdown(acdropdown, hwnd_edit, sorted7, ARRAY_SIZE(sorted7));
    SendMessageW(hwnd_edit, WM_CHAR, 'i', 1);
    SendMessageW(hwnd_edit, WM_CHAR, 'L', 1);
    SendMessageW(hwnd_edit, WM_GETTEXT, ARRAY_SIZE(buffer), (LPARAM)buffer);
    check_dropdown(acdropdown, hwnd_edit, sorted7, 1);

    IAutoCompleteDropDown_Release(acdropdown);
    IAutoComplete2_Release(autocomplete);
    IUnknown_Release(enumerator);
}

static void test_custom_source(void)
{
    static WCHAR str_alpha[] = {'t','e','s','t','1',0};
    static WCHAR str_alpha2[] = {'t','e','s','t','2',0};
    static WCHAR str_beta[] = {'a','u','t','o',' ','c','o','m','p','l','e','t','e',0};
    static WCHAR str_au[] = {'a','u',0};
    static WCHAR str_aut[] = {'a','u','t',0};
    static WCHAR *suggestions[] = { str_alpha, str_alpha2, str_beta };
    struct string_enumerator *obj;
    IUnknown *enumerator;
    IAutoComplete2 *autocomplete;
    IAutoCompleteDropDown *acdropdown;
    HWND hwnd_edit;
    DWORD flags = 0;
    WCHAR buffer[20];
    HRESULT hr;

    ShowWindow(hMainWnd, SW_SHOW);

    hwnd_edit = CreateWindowA("Edit", "", WS_OVERLAPPED | WS_VISIBLE | WS_CHILD | WS_BORDER, 50, 5, 200, 20, hMainWnd, 0, NULL, 0);

    hr = CoCreateInstance(&CLSID_AutoComplete, NULL, CLSCTX_INPROC_SERVER, &IID_IAutoComplete2, (void**)&autocomplete);
    ok(hr == S_OK, "CoCreateInstance failed: %x\n", hr);

    hr = IAutoComplete2_QueryInterface(autocomplete, &IID_IAutoCompleteDropDown, (LPVOID*)&acdropdown);
    ok(hr == S_OK, "No IAutoCompleteDropDown interface: %x\n", hr);

    string_enumerator_create((void**)&enumerator, suggestions, ARRAY_SIZE(suggestions));
    obj = (struct string_enumerator*)enumerator;

    hr = IAutoComplete2_SetOptions(autocomplete, ACO_AUTOSUGGEST | ACO_AUTOAPPEND);
    ok(hr == S_OK, "IAutoComplete2_SetOptions failed: %x\n", hr);
    hr = IAutoCompleteDropDown_ResetEnumerator(acdropdown);
    ok(hr == S_OK, "IAutoCompleteDropDown_ResetEnumerator failed: %x\n", hr);
    hr = IAutoComplete2_Init(autocomplete, hwnd_edit, enumerator, NULL, NULL);
    ok(hr == S_OK, "IAutoComplete_Init failed: %x\n", hr);

    SetFocus(hwnd_edit);
    SendMessageW(hwnd_edit, WM_CHAR, 'a', 1);
    SendMessageW(hwnd_edit, WM_CHAR, 'u', 1);
    dispatch_messages();
    SendMessageW(hwnd_edit, WM_GETTEXT, ARRAY_SIZE(buffer), (LPARAM)buffer);
    ok(lstrcmpW(str_beta, buffer) == 0, "Expected %s, got %s\n", wine_dbgstr_w(str_beta), wine_dbgstr_w(buffer));
    ok(obj->num_resets == 1, "Expected 1 reset, got %u\n", obj->num_resets);
    SendMessageW(hwnd_edit, EM_SETSEL, 0, -1);
    SendMessageW(hwnd_edit, WM_CHAR, '\b', 1);
    dispatch_messages();
    SendMessageW(hwnd_edit, WM_GETTEXT, ARRAY_SIZE(buffer), (LPARAM)buffer);
    ok(buffer[0] == '\0', "Expected empty string, got %s\n", wine_dbgstr_w(buffer));
    ok(obj->num_resets == 1, "Expected 1 reset, got %u\n", obj->num_resets);
    hr = IAutoCompleteDropDown_ResetEnumerator(acdropdown);
    ok(hr == S_OK, "IAutoCompleteDropDown_ResetEnumerator failed: %x\n", hr);
    ok(obj->num_resets == 1, "Expected 1 reset, got %u\n", obj->num_resets);
    obj->num_resets = 0;

    /* hijack the window procedure */
    HijackerWndProc_prev = (WNDPROC)SetWindowLongPtrW(hwnd_edit, GWLP_WNDPROC, (LONG_PTR)HijackerWndProc);
    SendMessageW(hwnd_edit, WM_GETTEXT, ARRAY_SIZE(buffer), (LPARAM)buffer);
    ok(lstrcmpW(HijackerWndProc_txt, buffer) == 0, "Expected %s, got %s\n", wine_dbgstr_w(HijackerWndProc_txt), wine_dbgstr_w(buffer));

    SendMessageW(hwnd_edit, WM_CHAR, 'a', 1);
    SendMessageW(hwnd_edit, WM_CHAR, 'u', 1);
    SetWindowLongPtrW(hwnd_edit, GWLP_WNDPROC, (LONG_PTR)HijackerWndProc_prev);
    dispatch_messages();
    SendMessageW(hwnd_edit, WM_GETTEXT, ARRAY_SIZE(buffer), (LPARAM)buffer);
    ok(lstrcmpW(str_au, buffer) == 0, "Expected %s, got %s\n", wine_dbgstr_w(str_au), wine_dbgstr_w(buffer));
    ok(obj->num_resets == 1, "Expected 1 reset, got %u\n", obj->num_resets);
    SendMessageW(hwnd_edit, EM_SETSEL, 0, -1);
    SendMessageW(hwnd_edit, WM_CHAR, '\b', 1);
    dispatch_messages();
    SendMessageW(hwnd_edit, WM_GETTEXT, ARRAY_SIZE(buffer), (LPARAM)buffer);
    ok(buffer[0] == '\0', "Expected empty string, got %s\n", wine_dbgstr_w(buffer));
    hr = IAutoCompleteDropDown_ResetEnumerator(acdropdown);
    ok(hr == S_OK, "IAutoCompleteDropDown_ResetEnumerator failed: %x\n", hr);

    HijackerWndProc_prev = (WNDPROC)SetWindowLongPtrW(hwnd_edit, GWLP_WNDPROC, (LONG_PTR)HijackerWndProc2);
    SendMessageW(hwnd_edit, WM_CHAR, 'a', 1);
    SendMessageW(hwnd_edit, WM_CHAR, 'u', 1);
    SetWindowLongPtrW(hwnd_edit, GWLP_WNDPROC, (LONG_PTR)HijackerWndProc_prev);
    dispatch_messages();
    SendMessageW(hwnd_edit, WM_GETTEXT, ARRAY_SIZE(buffer), (LPARAM)buffer);
    ok(lstrcmpW(str_beta, buffer) == 0, "Expected %s, got %s\n", wine_dbgstr_w(str_beta), wine_dbgstr_w(buffer));
    ok(obj->num_resets == 2, "Expected 2 resets, got %u\n", obj->num_resets);
    /* end of hijacks */

    hr = IAutoCompleteDropDown_GetDropDownStatus(acdropdown, &flags, NULL);
    ok(hr == S_OK, "IAutoCompleteDropDown_GetDropDownStatus failed: %x\n", hr);
    ok(flags & ACDD_VISIBLE, "AutoComplete DropDown should be visible\n");
    SendMessageW(hwnd_edit, WM_SETTEXT, 0, (LPARAM)str_au);
    dispatch_messages();
    hr = IAutoCompleteDropDown_GetDropDownStatus(acdropdown, &flags, NULL);
    ok(hr == S_OK, "IAutoCompleteDropDown_GetDropDownStatus failed: %x\n", hr);
    ok(!(flags & ACDD_VISIBLE), "AutoComplete DropDown should have been hidden\n");
    SendMessageW(hwnd_edit, WM_SETTEXT, 0, (LPARAM)str_aut);
    dispatch_messages();
    hr = IAutoCompleteDropDown_GetDropDownStatus(acdropdown, &flags, NULL);
    ok(hr == S_OK, "IAutoCompleteDropDown_GetDropDownStatus failed: %x\n", hr);
    ok(!(flags & ACDD_VISIBLE), "AutoComplete DropDown should be hidden\n");
    SendMessageW(hwnd_edit, WM_GETTEXT, ARRAY_SIZE(buffer), (LPARAM)buffer);
    ok(lstrcmpW(str_aut, buffer) == 0, "Expected %s, got %s\n", wine_dbgstr_w(str_aut), wine_dbgstr_w(buffer));

    test_aclist_expand(hwnd_edit, enumerator, acdropdown);
    obj->num_resets = 0;

    hr = IAutoCompleteDropDown_ResetEnumerator(acdropdown);
    ok(hr == S_OK, "IAutoCompleteDropDown_ResetEnumerator failed: %x\n", hr);
    SendMessageW(hwnd_edit, WM_CHAR, 'x', 1);
    dispatch_messages();
    ok(obj->num_resets == 1, "Expected 1 reset, got %u\n", obj->num_resets);
    SendMessageW(hwnd_edit, WM_CHAR, 'x', 1);
    dispatch_messages();
    ok(obj->num_resets == 1, "Expected 1 reset, got %u\n", obj->num_resets);

    IAutoCompleteDropDown_Release(acdropdown);
    IAutoComplete2_Release(autocomplete);
    IUnknown_Release(enumerator);

    test_prefix_filtering(hwnd_edit);

    ShowWindow(hMainWnd, SW_HIDE);
    DestroyWindow(hwnd_edit);
}

START_TEST(autocomplete)
{
    HRESULT r;
    MSG msg;
    IAutoComplete* ac;
    RECT win_rect;
    POINT orig_pos;

    r = CoInitialize(NULL);
    ok(r == S_OK, "CoInitialize failed (0x%08x). Tests aborted.\n", r);
    if (r != S_OK)
        return;

    createMainWnd();
    ok(hMainWnd != NULL, "Failed to create parent window. Tests aborted.\n");
    if (!hMainWnd) return;

    /* Move the cursor away from the dropdown listbox */
    GetWindowRect(hMainWnd, &win_rect);
    GetCursorPos(&orig_pos);
    SetCursorPos(win_rect.left, win_rect.top);

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
    SetCursorPos(orig_pos.x, orig_pos.y);
    DestroyWindow(hEdit);
    DestroyWindow(hMainWnd);

    CoUninitialize();
}
