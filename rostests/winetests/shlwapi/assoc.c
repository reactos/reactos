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

#define expect(expected, got) ok ( expected == got, "Expected %d, got %d\n", expected, got)
#define expect_hr(expected, got) ok ( expected == got, "Expected %08x, got %08x\n", expected, got)

static HRESULT (WINAPI *pAssocQueryStringA)(ASSOCF,ASSOCSTR,LPCSTR,LPCSTR,LPSTR,LPDWORD) = NULL;
static HRESULT (WINAPI *pAssocQueryStringW)(ASSOCF,ASSOCSTR,LPCWSTR,LPCWSTR,LPWSTR,LPDWORD) = NULL;

/* Every version of Windows with IE should have this association? */
static const WCHAR dotHtml[] = { '.','h','t','m','l',0 };
static const WCHAR badBad[] = { 'b','a','d','b','a','d',0 };
static const WCHAR dotBad[] = { '.','b','a','d',0 };
static const WCHAR open[] = { 'o','p','e','n',0 };
static const WCHAR invalid[] = { 'i','n','v','a','l','i','d',0 };

static void test_getstring_bad(void)
{
    HRESULT hr;
    DWORD len;

    if (!pAssocQueryStringW)
    {
        win_skip("AssocQueryStringW() is missing\n");
        return;
    }

    hr = pAssocQueryStringW(0, ASSOCSTR_EXECUTABLE, NULL, open, NULL, &len);
    expect_hr(E_INVALIDARG, hr);
    hr = pAssocQueryStringW(0, ASSOCSTR_EXECUTABLE, badBad, open, NULL, &len);
    ok(hr == E_FAIL ||
       hr == HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION), /* Win9x/WinMe/NT4/W2K/Vista/W2K8 */
       "Unexpected result : %08x\n", hr);
    hr = pAssocQueryStringW(0, ASSOCSTR_EXECUTABLE, dotBad, open, NULL, &len);
    ok(hr == E_FAIL ||
       hr == HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION), /* Win9x/WinMe/NT4/W2K/Vista/W2K8 */
       "Unexpected result : %08x\n", hr);
    hr = pAssocQueryStringW(0, ASSOCSTR_EXECUTABLE, dotHtml, invalid, NULL,
                           &len);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) ||
       hr == HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION), /* Win9x/WinMe/NT4/W2K/Vista/W2K8 */
       "Unexpected result : %08x\n", hr);
    hr = pAssocQueryStringW(0, ASSOCSTR_EXECUTABLE, dotHtml, open, NULL, NULL);
    ok(hr == E_UNEXPECTED ||
       hr == E_INVALIDARG, /* Win9x/WinMe/NT4/W2K/Vista/W2K8 */
       "Unexpected result : %08x\n", hr);

    hr = pAssocQueryStringW(0, ASSOCSTR_FRIENDLYAPPNAME, NULL, open, NULL, &len);
    expect_hr(E_INVALIDARG, hr);
    hr = pAssocQueryStringW(0, ASSOCSTR_FRIENDLYAPPNAME, badBad, open, NULL,
                           &len);
    ok(hr == E_FAIL ||
       hr == HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION), /* Win9x/WinMe/NT4/W2K/Vista/W2K8 */
       "Unexpected result : %08x\n", hr);
    hr = pAssocQueryStringW(0, ASSOCSTR_FRIENDLYAPPNAME, dotBad, open, NULL,
                           &len);
    ok(hr == E_FAIL ||
       hr == HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION), /* Win9x/WinMe/NT4/W2K/Vista/W2K8 */
       "Unexpected result : %08x\n", hr);
    hr = pAssocQueryStringW(0, ASSOCSTR_FRIENDLYAPPNAME, dotHtml, invalid, NULL,
                           &len);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) ||
       hr == HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION) || /* W2K/Vista/W2K8 */
       hr == E_FAIL, /* Win9x/WinMe/NT4 */
       "Unexpected result : %08x\n", hr);
    hr = pAssocQueryStringW(0, ASSOCSTR_FRIENDLYAPPNAME, dotHtml, open, NULL,
                           NULL);
    ok(hr == E_UNEXPECTED ||
       hr == E_INVALIDARG, /* Win9x/WinMe/NT4/W2K/Vista/W2K8 */
       "Unexpected result : %08x\n", hr);
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

    hr = pAssocQueryStringW(0, ASSOCSTR_EXECUTABLE, dotHtml, open, NULL, &len);
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
    hr = pAssocQueryStringW(0, ASSOCSTR_EXECUTABLE, dotHtml, open,
                           executableName, &len2);
    expect_hr(S_OK, hr);
    slen = lstrlenW(executableName) + 1;
    expect(len, len2);
    expect(len, slen);

    hr = pAssocQueryStringW(0, ASSOCSTR_FRIENDLYAPPNAME, dotHtml, open, NULL,
                           &len);
    ok(hr == S_FALSE ||
       hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), /* Win9x/NT4 */
       "Unexpected result : %08x\n", hr);
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
    hr = pAssocQueryStringW(0, ASSOCSTR_FRIENDLYAPPNAME, dotHtml, open,
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
       "Unexpected result : %08x\n", hr);
    hr = pAssocQueryStringA(0, ASSOCSTR_EXECUTABLE, dotWinetest, "foo", buf, &len);
    expect_hr(S_OK, hr);
    ok(strstr(buf, action) != NULL,
        "got '%s' (Expected result to include 'notepad.exe')\n", buf);

cleanup:
    SHDeleteKeyA(HKEY_CLASSES_ROOT, dotWinetest);
    SHDeleteKeyA(HKEY_CLASSES_ROOT, winetestfile);

}

START_TEST(assoc)
{
    HMODULE hshlwapi;
    hshlwapi = GetModuleHandleA("shlwapi.dll");
    pAssocQueryStringA = (void*)GetProcAddress(hshlwapi, "AssocQueryStringA");
    pAssocQueryStringW = (void*)GetProcAddress(hshlwapi, "AssocQueryStringW");

    test_getstring_bad();
    test_getstring_basic();
    test_getstring_no_extra();
}
