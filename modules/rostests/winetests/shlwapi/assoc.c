/* Unit test suite for SHLWAPI IQueryAssociations functions
 *
 * Copyright 2008 Google (Lei Zhang)
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

#include <stdarg.h>

#include "wine/test.h"
#include "shlwapi.h"
#include "shlguid.h"

#define expect(expected, got) ok ( expected == got, "Expected %ld, got %ld\n", expected, got)
#define expect_hr(expected, got) ok ( expected == got, "Expected %08lx, got %08lx\n", expected, got)

static HRESULT (WINAPI *pAssocQueryStringA)(ASSOCF,ASSOCSTR,LPCSTR,LPCSTR,LPSTR,LPDWORD) = NULL;
static HRESULT (WINAPI *pAssocQueryStringW)(ASSOCF,ASSOCSTR,LPCWSTR,LPCWSTR,LPWSTR,LPDWORD) = NULL;
static HRESULT (WINAPI *pAssocCreate)(CLSID, REFIID, void **) = NULL;

/* Should every version of Windows with IE have .html association? */

static void test_getstring_bad(void)
{
    static const WCHAR openwith[] = {'O','p','e','n','W','i','t','h','.','e','x','e',0};
    WCHAR buf[MAX_PATH];
    HRESULT hr;
    DWORD len;

    if (!pAssocQueryStringW)
    {
        win_skip("AssocQueryStringW() is missing\n");
        return;
    }

    len = 0xdeadbeef;
    hr = pAssocQueryStringW(0, ASSOCSTR_EXECUTABLE, NULL, L"open", NULL, &len);
    expect_hr(E_INVALIDARG, hr);
    ok(len == 0xdeadbeef, "got %lu\n", len);

    len = 0xdeadbeef;
    hr = pAssocQueryStringW(0, ASSOCSTR_EXECUTABLE, L"badbad", L"open", NULL, &len);
    ok(hr == E_FAIL ||
       hr == HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION), /* Win9x/WinMe/NT4/W2K/Vista/W2K8 */
       "Unexpected result : %08lx\n", hr);
    ok(len == 0xdeadbeef, "got %lu\n", len);

    len = ARRAY_SIZE(buf);
    hr = pAssocQueryStringW(0, ASSOCSTR_EXECUTABLE, L".bad", L"open", buf, &len);
    ok(hr == E_FAIL ||
       hr == HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION) /* Win9x/WinMe/NT4/W2K/Vista/W2K8 */ ||
       hr == S_OK /* Win8 */,
       "Unexpected result : %08lx\n", hr);
    if (hr == S_OK)
    {
        ok(len < ARRAY_SIZE(buf), "got %lu\n", len);
        ok(!lstrcmpiW(buf + len - ARRAY_SIZE(openwith), openwith), "wrong data\n");
    }

    len = 0xdeadbeef;
    hr = pAssocQueryStringW(0, ASSOCSTR_EXECUTABLE, L".html", L"invalid", NULL, &len);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) ||
       hr == HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION), /* Win9x/WinMe/NT4/W2K/Vista/W2K8 */
       "Unexpected result : %08lx\n", hr);
    ok(len == 0xdeadbeef, "got %lu\n", len);

    hr = pAssocQueryStringW(0, ASSOCSTR_EXECUTABLE, L".html", L"open", NULL, NULL);
    ok(hr == E_UNEXPECTED ||
       hr == E_INVALIDARG, /* Win9x/WinMe/NT4/W2K/Vista/W2K8 */
       "Unexpected result : %08lx\n", hr);

    len = 0xdeadbeef;
    hr = pAssocQueryStringW(0, ASSOCSTR_FRIENDLYAPPNAME, NULL, L"open", NULL, &len);
    expect_hr(E_INVALIDARG, hr);
    ok(len == 0xdeadbeef, "got %lu\n", len);

    len = 0xdeadbeef;
    hr = pAssocQueryStringW(0, ASSOCSTR_FRIENDLYAPPNAME, L"badbad", L"open", NULL, &len);
    ok(hr == E_FAIL ||
       hr == HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION), /* Win9x/WinMe/NT4/W2K/Vista/W2K8 */
       "Unexpected result : %08lx\n", hr);
    ok(len == 0xdeadbeef, "got %lu\n", len);

    len = 0xdeadbeef;
    hr = pAssocQueryStringW(0, ASSOCSTR_FRIENDLYAPPNAME, L".bad", L"open", NULL, &len);
    ok(hr == E_FAIL ||
       hr == HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION) /* Win9x/WinMe/NT4/W2K/Vista/W2K8 */ ||
       hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND) /* Win8 */ ||
       hr == S_FALSE, /* Win10 */
       "Unexpected result : %08lx\n", hr);
    ok((hr == S_FALSE && len < ARRAY_SIZE(buf)) || len == 0xdeadbeef,
       "got hr=%08lx and len=%lu\n", hr, len);

    len = 0xdeadbeef;
    hr = pAssocQueryStringW(0, ASSOCSTR_FRIENDLYAPPNAME, L".html", L"invalid", NULL, &len);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) ||
       hr == HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION) || /* W2K/Vista/W2K8 */
       hr == E_FAIL, /* Win9x/WinMe/NT4 */
       "Unexpected result : %08lx\n", hr);
    ok(len == 0xdeadbeef, "got %lu\n", len);

    hr = pAssocQueryStringW(0, ASSOCSTR_FRIENDLYAPPNAME, L".html", L"open", NULL, NULL);
    ok(hr == E_UNEXPECTED ||
       hr == E_INVALIDARG, /* Win9x/WinMe/NT4/W2K/Vista/W2K8 */
       "Unexpected result : %08lx\n", hr);
}

static void test_getstring_basic(void)
{
    HRESULT hr;
    WCHAR * friendlyName;
    WCHAR * executableName;
    DWORD len, len2, slen;

    if (!pAssocQueryStringW)
    {
        win_skip("AssocQueryStringW() is missing\n");
        return;
    }

    hr = pAssocQueryStringW(0, ASSOCSTR_EXECUTABLE, L".html", L"open", NULL, &len);
    expect_hr(S_FALSE, hr);
    if (hr != S_FALSE)
    {
        skip("failed to get initial len\n");
        return;
    }

    executableName = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                               len * sizeof(WCHAR));
    if (!executableName)
    {
        skip("failed to allocate memory\n");
        return;
    }

    len2 = len;
    hr = pAssocQueryStringW(0, ASSOCSTR_EXECUTABLE, L".html", L"open",
                           executableName, &len2);
    expect_hr(S_OK, hr);
    slen = lstrlenW(executableName) + 1;
    expect(len, len2);
    expect(len, slen);

    hr = pAssocQueryStringW(0, ASSOCSTR_FRIENDLYAPPNAME, L".html", L"open", NULL,
                           &len);
    ok(hr == S_FALSE ||
       hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) /* Win9x/NT4 */ ||
       hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND), /* Win8 */
       "Unexpected result : %08lx\n", hr);
    if (hr != S_FALSE)
    {
        HeapFree(GetProcessHeap(), 0, executableName);
        skip("failed to get initial len\n");
        return;
    }

    friendlyName = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                               len * sizeof(WCHAR));
    if (!friendlyName)
    {
        HeapFree(GetProcessHeap(), 0, executableName);
        skip("failed to allocate memory\n");
        return;
    }

    len2 = len;
    hr = pAssocQueryStringW(0, ASSOCSTR_FRIENDLYAPPNAME, L".html", L"open",
                           friendlyName, &len2);
    expect_hr(S_OK, hr);
    slen = lstrlenW(friendlyName) + 1;
    expect(len, len2);
    expect(len, slen);

    HeapFree(GetProcessHeap(), 0, executableName);
    HeapFree(GetProcessHeap(), 0, friendlyName);
}

static void test_getstring_no_extra(void)
{
    LONG ret;
    HKEY hkey;
    HRESULT hr;
    static const CHAR dotWinetest[] = {
        '.','w','i','n','e','t','e','s','t',0
    };
    static const CHAR winetestfile[] = {
        'w','i','n','e','t','e','s','t', 'f','i','l','e',0
    };
    static const CHAR winetestfileAction[] = {
        'w','i','n','e','t','e','s','t','f','i','l','e',
        '\\','s','h','e','l','l',
        '\\','f','o','o',
        '\\','c','o','m','m','a','n','d',0
    };
    static const CHAR action[] = {
        'n','o','t','e','p','a','d','.','e','x','e',0
    };
    CHAR buf[MAX_PATH];
    DWORD len = MAX_PATH;

    if (!pAssocQueryStringA)
    {
        win_skip("AssocQueryStringA() is missing\n");
        return;
    }

    buf[0] = '\0';
    ret = RegCreateKeyA(HKEY_CLASSES_ROOT, dotWinetest, &hkey);
    if (ret != ERROR_SUCCESS) {
        skip("failed to create dotWinetest key\n");
        return;
    }

    ret = RegSetValueA(hkey, NULL, REG_SZ, winetestfile, lstrlenA(winetestfile));
    RegCloseKey(hkey);
    if (ret != ERROR_SUCCESS)
    {
        skip("failed to set dotWinetest key\n");
        goto cleanup;
    }

    ret = RegCreateKeyA(HKEY_CLASSES_ROOT, winetestfileAction, &hkey);
    if (ret != ERROR_SUCCESS)
    {
        skip("failed to create winetestfileAction key\n");
        goto cleanup;
    }

    ret = RegSetValueA(hkey, NULL, REG_SZ, action, lstrlenA(action));
    RegCloseKey(hkey);
    if (ret != ERROR_SUCCESS)
    {
        skip("failed to set winetestfileAction key\n");
        goto cleanup;
    }

    hr = pAssocQueryStringA(0, ASSOCSTR_EXECUTABLE, dotWinetest, NULL, buf, &len);
    ok(hr == S_OK ||
       hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), /* XP and W2K3 */
       "Unexpected result : %08lx\n", hr);
    hr = pAssocQueryStringA(0, ASSOCSTR_EXECUTABLE, dotWinetest, "foo", buf, &len);
    expect_hr(S_OK, hr);
    ok(strstr(buf, action) != NULL,
        "got '%s' (Expected result to include 'notepad.exe')\n", buf);

cleanup:
    SHDeleteKeyA(HKEY_CLASSES_ROOT, dotWinetest);
    SHDeleteKeyA(HKEY_CLASSES_ROOT, winetestfile);

}

static void test_assoc_create(void)
{
    HRESULT hr;
    IQueryAssociations *pqa;

    if (!pAssocCreate)
    {
        win_skip("AssocCreate() is missing\n");
        return;
    }

    hr = pAssocCreate(IID_NULL, &IID_NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected result : %08lx\n", hr);

    hr = pAssocCreate(CLSID_QueryAssociations, &IID_NULL, (LPVOID*)&pqa);
    ok(hr == CLASS_E_CLASSNOTAVAILABLE || hr == E_NOTIMPL || hr == E_NOINTERFACE
        , "Unexpected result : %08lx\n", hr);

    hr = pAssocCreate(IID_NULL, &IID_IQueryAssociations, (LPVOID*)&pqa);
    ok(hr == CLASS_E_CLASSNOTAVAILABLE || hr == E_NOTIMPL || hr == E_INVALIDARG
        , "Unexpected result : %08lx\n", hr);

    hr = pAssocCreate(CLSID_QueryAssociations, &IID_IQueryAssociations, (LPVOID*)&pqa);
    ok(hr == S_OK  || hr == E_NOTIMPL /* win98 */
        , "Unexpected result : %08lx\n", hr);
    if(hr == S_OK)
    {
        IQueryAssociations_Release(pqa);
    }

    hr = pAssocCreate(CLSID_QueryAssociations, &IID_IUnknown, (LPVOID*)&pqa);
    ok(hr == S_OK  || hr == E_NOTIMPL /* win98 */
        , "Unexpected result : %08lx\n", hr);
    if(hr == S_OK)
    {
        IQueryAssociations_Release(pqa);
    }
}

START_TEST(assoc)
{
    HMODULE hshlwapi;
    hshlwapi = GetModuleHandleA("shlwapi.dll");
    pAssocQueryStringA = (void*)GetProcAddress(hshlwapi, "AssocQueryStringA");
    pAssocQueryStringW = (void*)GetProcAddress(hshlwapi, "AssocQueryStringW");
    pAssocCreate       = (void*)GetProcAddress(hshlwapi, "AssocCreate");

    test_getstring_bad();
    test_getstring_basic();
    test_getstring_no_extra();
    test_assoc_create();
}
