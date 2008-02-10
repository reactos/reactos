/*
 * Tests for MSI Source functions
 *
 * Copyright (C) 2006 James Hawkins
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

#define _WIN32_MSI 300

#include <stdio.h>

#include <windows.h>
#include <msiquery.h>
#include <msidefs.h>
#include <msi.h>
#include <sddl.h>

#include "wine/test.h"

static BOOL (WINAPI *pConvertSidToStringSidA)(PSID, LPSTR*);
static UINT (WINAPI *pMsiSourceListGetInfoA)
    (LPCSTR, LPCSTR, MSIINSTALLCONTEXT, DWORD, LPCSTR, LPSTR, LPDWORD);
static UINT (WINAPI *pMsiSourceListAddSourceExA)
    (LPCSTR, LPCSTR, MSIINSTALLCONTEXT, DWORD, LPCSTR, DWORD);

static void init_functionpointers(void)
{
    HMODULE hmsi = GetModuleHandleA("msi.dll");
    HMODULE hadvapi32 = GetModuleHandleA("advapi32.dll");

#define GET_PROC(dll, func) \
    p ## func = (void *)GetProcAddress(dll, #func); \
    if(!p ## func) \
      trace("GetProcAddress(%s) failed\n", #func);

    GET_PROC(hmsi, MsiSourceListAddSourceExA)
    GET_PROC(hmsi, MsiSourceListGetInfoA)

    GET_PROC(hadvapi32, ConvertSidToStringSidA)

#undef GET_PROC
}

/* copied from dlls/msi/registry.c */
static BOOL squash_guid(LPCWSTR in, LPWSTR out)
{
    DWORD i,n=1;
    GUID guid;

    if (FAILED(CLSIDFromString((LPOLESTR)in, &guid)))
        return FALSE;

    for(i=0; i<8; i++)
        out[7-i] = in[n++];
    n++;
    for(i=0; i<4; i++)
        out[11-i] = in[n++];
    n++;
    for(i=0; i<4; i++)
        out[15-i] = in[n++];
    n++;
    for(i=0; i<2; i++)
    {
        out[17+i*2] = in[n++];
        out[16+i*2] = in[n++];
    }
    n++;
    for( ; i<8; i++)
    {
        out[17+i*2] = in[n++];
        out[16+i*2] = in[n++];
    }
    out[32]=0;
    return TRUE;
}

static void create_test_guid(LPSTR prodcode, LPSTR squashed)
{
    WCHAR guidW[MAX_PATH];
    WCHAR squashedW[MAX_PATH];
    GUID guid;
    HRESULT hr;
    int size;

    hr = CoCreateGuid(&guid);
    ok(hr == S_OK, "Expected S_OK, got %d\n", hr);

    size = StringFromGUID2(&guid, (LPOLESTR)guidW, MAX_PATH);
    ok(size == 39, "Expected 39, got %d\n", hr);

    WideCharToMultiByte(CP_ACP, 0, guidW, size, prodcode, MAX_PATH, NULL, NULL);
    squash_guid(guidW, squashedW);
    WideCharToMultiByte(CP_ACP, 0, squashedW, -1, squashed, MAX_PATH, NULL, NULL);
}

static void get_user_sid(LPSTR *usersid)
{
    HANDLE token;
    BYTE buf[1024];
    DWORD size;
    PTOKEN_USER user;

    OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token);
    size = sizeof(buf);
    GetTokenInformation(token, TokenUser, (void *)buf, size, &size);
    user = (PTOKEN_USER)buf;
    pConvertSidToStringSidA(user->User.Sid, usersid);
}

static void test_MsiSourceListGetInfo(void)
{
    CHAR prodcode[MAX_PATH];
    CHAR prod_squashed[MAX_PATH];
    CHAR keypath[MAX_PATH*2];
    CHAR value[MAX_PATH];
    LPSTR usersid;
    LPCSTR data;
    LONG res;
    UINT r;
    HKEY userkey, hkey;
    DWORD size;

    if (!pMsiSourceListGetInfoA)
    {
        skip("Skipping MsiSourceListGetInfoA tests\n");
        return;
    }

    create_test_guid(prodcode, prod_squashed);
    get_user_sid(&usersid);

    /* NULL szProductCodeOrPatchCode */
    r = pMsiSourceListGetInfoA(NULL, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAME, NULL, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* empty szProductCodeOrPatchCode */
    r = pMsiSourceListGetInfoA("", usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAME, NULL, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* garbage szProductCodeOrPatchCode */
    r = pMsiSourceListGetInfoA("garbage", usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAME, NULL, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /*  szProductCodeOrPatchCode */
    r = pMsiSourceListGetInfoA("garbage", usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAME, NULL, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* guid without brackets */
    r = pMsiSourceListGetInfoA("51CD2AD5-0482-4C46-8DDD-0ED1022AA1AA", usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAME, NULL, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* guid with brackets */
    r = pMsiSourceListGetInfoA("{51CD2AD5-0482-4C46-8DDD-0ED1022AA1AA}", usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAME, NULL, NULL);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);

    /* same length as guid, but random */
    r = pMsiSourceListGetInfoA("ADKD-2KSDFF2-DKK1KNFJASD9GLKWME-1I3KAD", usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAME, NULL, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* invalid context */
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_NONE,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAME, NULL, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* another invalid context */
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_ALLUSERMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAME, NULL, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* yet another invalid context */
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_ALL,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAME, NULL, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* mix two valid contexts */
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED | MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAME, NULL, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* invalid option */
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              4, INSTALLPROPERTY_PACKAGENAME, NULL, NULL);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);

    /* NULL property */
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, NULL, NULL, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* empty property */
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, "", NULL, NULL);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);

    /* value is non-NULL while size is NULL */
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAME, value, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* size is non-NULL while value is NULL */
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAME, NULL, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);

    lstrcpyA(keypath, "Software\\Microsoft\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_CURRENT_USER, keypath, &userkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* user product key exists */
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAME, NULL, &size);
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);

    res = RegCreateKeyA(userkey, "SourceList", &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* SourceList key exists */
    size = 0xdeadbeef;
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAME, NULL, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(size == 0, "Expected 0, got %d\n", size);

    data = "msitest.msi";
    res = RegSetValueExA(hkey, "PackageName", 0, REG_SZ, (const BYTE *)data, lstrlenA(data) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* PackageName value exists */
    size = 0xdeadbeef;
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAME, NULL, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(size == 11, "Expected 11, got %d\n", size);

    /* read the value, don't change size */
    lstrcpyA(value, "aaa");
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAME, value, &size);
    ok(r == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected 'aaa', got %s\n", value);
    ok(size == 11, "Expected 11, got %d\n", size);

    /* read the value, fix size */
    size++;
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAME, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, "msitest.msi"), "Expected 'msitest.msi', got %s\n", value);
    ok(size == 11, "Expected 11, got %d\n", size);

    /* empty property now that product key exists */
    size = 0xdeadbeef;
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, "", NULL, &size);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(size == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", size);

    /* nonexistent property now that product key exists */
    size = 0xdeadbeef;
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, "nonexistent", NULL, &size);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(size == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", size);

    data = "tester";
    res = RegSetValueExA(hkey, "nonexistent", 0, REG_SZ, (const BYTE *)data, lstrlenA(data) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* nonexistent property now that nonexistent value exists */
    size = 0xdeadbeef;
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, "nonexistent", NULL, &size);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(size == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", size);

    /* invalid option now that product key exists */
    size = 0xdeadbeef;
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              4, INSTALLPROPERTY_PACKAGENAME, NULL, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(size == 11, "Expected 11, got %d\n", size);

    RegDeleteValueA(hkey, "nonexistent");
    RegDeleteValueA(hkey, "PackageName");
    RegDeleteKeyA(hkey, "");
    RegDeleteKeyA(userkey, "");
    RegCloseKey(hkey);
    RegCloseKey(userkey);

    /* try a patch */
    size = 0xdeadbeef;
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PATCH, INSTALLPROPERTY_PACKAGENAME, NULL, &size);
    ok(r == ERROR_UNKNOWN_PATCH, "Expected ERROR_UNKNOWN_PATCH, got %d\n", r);
    ok(size == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", size);

    lstrcpyA(keypath, "Software\\Microsoft\\Installer\\Patches\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_CURRENT_USER, keypath, &userkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* patch key exists
     * NOTE: using prodcode guid, but it really doesn't matter
     */
    size = 0xdeadbeef;
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PATCH, INSTALLPROPERTY_PACKAGENAME, NULL, &size);
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);
    ok(size == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", size);

    res = RegCreateKeyA(userkey, "SourceList", &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* SourceList key exists */
    size = 0xdeadbeef;
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PATCH, INSTALLPROPERTY_PACKAGENAME, NULL, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(size == 0, "Expected 0, got %d\n", size);

    RegDeleteKeyA(hkey, "");
    RegDeleteKeyA(userkey, "");
    RegCloseKey(hkey);
    RegCloseKey(userkey);
}

static void test_MsiSourceListAddSourceEx(void)
{
    CHAR prodcode[MAX_PATH];
    CHAR prod_squashed[MAX_PATH];
    CHAR keypath[MAX_PATH*2];
    CHAR value[MAX_PATH];
    LPSTR usersid;
    LONG res;
    UINT r;
    HKEY prodkey, userkey, hkey;
    HKEY url, net;
    DWORD size;

    if (!pMsiSourceListAddSourceExA)
    {
        skip("Skipping MsiSourceListAddSourceExA tests\n");
        return;
    }

    create_test_guid(prodcode, prod_squashed);
    get_user_sid(&usersid);

    /* GetLastError is not set by the function */

    /* NULL szProductCodeOrPatchCode */
    r = pMsiSourceListAddSourceExA(NULL, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "C:\\source", 0);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* empty szProductCodeOrPatchCode */
    r = pMsiSourceListAddSourceExA("", usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "C:\\source", 0);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* garbage szProductCodeOrPatchCode */
    r = pMsiSourceListAddSourceExA("garbage", usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "C:\\source", 0);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* guid without brackets */
    r = pMsiSourceListAddSourceExA("51CD2AD5-0482-4C46-8DDD-0ED1022AA1AA", usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "C:\\source", 0);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* guid with brackets */
    r = pMsiSourceListAddSourceExA("{51CD2AD5-0482-4C46-8DDD-0ED1022AA1AA}", usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "C:\\source", 0);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);

    /* MSIINSTALLCONTEXT_USERUNMANAGED */

    r = pMsiSourceListAddSourceExA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "C:\\source", 0);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);

    lstrcpyA(keypath, "Software\\Microsoft\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_CURRENT_USER, keypath, &userkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* user product key exists */
    r = pMsiSourceListAddSourceExA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "C:\\source", 0);
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);

    res = RegCreateKeyA(userkey, "SourceList", &url);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    RegCloseKey(url);

    /* SourceList key exists */
    r = pMsiSourceListAddSourceExA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "C:\\source", 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    res = RegOpenKeyA(userkey, "SourceList\\URL", &url);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "1", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(value, "C:\\source/"), "Expected 'C:\\source/', got %s\n", value);
    ok(size == 11, "Expected 11, got %d\n", size);

    /* add another source, index 0 */
    r = pMsiSourceListAddSourceExA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "another", 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "1", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(value, "C:\\source/"), "Expected 'C:\\source/', got %s\n", value);
    ok(size == 11, "Expected 11, got %d\n", size);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "2", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(value, "another/"), "Expected 'another/', got %s\n", value);
    ok(size == 9, "Expected 9, got %d\n", size);

    /* add another source, index 1 */
    r = pMsiSourceListAddSourceExA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "third/", 1);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "1", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(value, "third/"), "Expected 'third/', got %s\n", value);
    ok(size == 7, "Expected 7, got %d\n", size);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "2", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(value, "C:\\source/"), "Expected 'C:\\source/', got %s\n", value);
    ok(size == 11, "Expected 11, got %d\n", size);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "3", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(value, "another/"), "Expected 'another/', got %s\n", value);
    ok(size == 9, "Expected 9, got %d\n", size);

    /* add another source, index > N */
    r = pMsiSourceListAddSourceExA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "last/", 5);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "1", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(value, "third/"), "Expected 'third/', got %s\n", value);
    ok(size == 7, "Expected 7, got %d\n", size);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "2", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(value, "C:\\source/"), "Expected 'C:\\source/', got %s\n", value);
    ok(size == 11, "Expected 11, got %d\n", size);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "3", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(value, "another/"), "Expected 'another/', got %s\n", value);
    ok(size == 9, "Expected 9, got %d\n", size);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "4", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(value, "last/"), "Expected 'last/', got %s\n", value);
    ok(size == 6, "Expected 6, got %d\n", size);

    /* just MSISOURCETYPE_NETWORK */
    r = pMsiSourceListAddSourceExA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSISOURCETYPE_NETWORK, "source", 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    res = RegOpenKeyA(userkey, "SourceList\\Net", &net);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    size = MAX_PATH;
    res = RegQueryValueExA(net, "1", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(value, "source\\"), "Expected 'source\\', got %s\n", value);
    ok(size == 8, "Expected 8, got %d\n", size);

    /* just MSISOURCETYPE_URL */
    r = pMsiSourceListAddSourceExA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSISOURCETYPE_URL, "source", 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "1", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(value, "third/"), "Expected 'third/', got %s\n", value);
    ok(size == 7, "Expected 7, got %d\n", size);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "2", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(value, "C:\\source/"), "Expected 'C:\\source/', got %s\n", value);
    ok(size == 11, "Expected 11, got %d\n", size);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "3", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(value, "another/"), "Expected 'another/', got %s\n", value);
    ok(size == 9, "Expected 9, got %d\n", size);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "4", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(value, "last/"), "Expected 'last/', got %s\n", value);
    ok(size == 6, "Expected 6, got %d\n", size);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "5", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(value, "source/"), "Expected 'source/', got %s\n", value);
    ok(size == 8, "Expected 8, got %d\n", size);

    /* NULL szUserSid */
    r = pMsiSourceListAddSourceExA(prodcode, NULL,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSISOURCETYPE_NETWORK, "nousersid", 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    res = RegQueryValueExA(net, "1", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(value, "source\\"), "Expected 'source\\', got %s\n", value);
    ok(size == 8, "Expected 8, got %d\n", size);

    size = MAX_PATH;
    res = RegQueryValueExA(net, "2", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(value, "nousersid\\"), "Expected 'nousersid\\', got %s\n", value);
    ok(size == 11, "Expected 11, got %d\n", size);

    /* invalid options, must have source type */
    r = pMsiSourceListAddSourceExA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT, "source", 0);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    r = pMsiSourceListAddSourceExA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PATCH, "source", 0);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* NULL szSource */
    r = pMsiSourceListAddSourceExA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSISOURCETYPE_URL, NULL, 1);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* empty szSource */
    r = pMsiSourceListAddSourceExA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSISOURCETYPE_URL, "", 1);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* MSIINSTALLCONTEXT_USERMANAGED, non-NULL szUserSid */

    r = pMsiSourceListAddSourceExA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "C:\\source", 0);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Managed\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* product key exists */
    r = pMsiSourceListAddSourceExA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "C:\\source", 0);
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);

    res = RegCreateKeyA(prodkey, "SourceList", &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    RegCloseKey(hkey);

    /* SourceList exists */
    r = pMsiSourceListAddSourceExA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "C:\\source", 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    res = RegOpenKeyA(prodkey, "SourceList\\URL", &url);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "1", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(value, "C:\\source/"), "Expected 'C:\\source/', got %s\n", value);
    ok(size == 11, "Expected 11, got %d\n", size);

    RegCloseKey(url);

    /* MSIINSTALLCONTEXT_USERMANAGED, NULL szUserSid */

    r = pMsiSourceListAddSourceExA(prodcode, NULL,
                                  MSIINSTALLCONTEXT_USERMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "another", 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    res = RegOpenKeyA(prodkey, "SourceList\\URL", &url);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "1", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(value, "C:\\source/"), "Expected 'C:\\source/', got %s\n", value);
    ok(size == 11, "Expected 11, got %d\n", size);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "2", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(value, "another/"), "Expected 'another/', got %s\n", value);
    ok(size == 9, "Expected 9, got %d\n", size);

    RegCloseKey(url);
    RegCloseKey(prodkey);

    /* MSIINSTALLCONTEXT_MACHINE */

    /* szUserSid must be NULL for MSIINSTALLCONTEXT_MACHINE */
    r = pMsiSourceListAddSourceExA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_MACHINE,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "C:\\source", 0);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    r = pMsiSourceListAddSourceExA(prodcode, NULL,
                                  MSIINSTALLCONTEXT_MACHINE,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "C:\\source", 0);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    lstrcpyA(keypath, "Software\\Classes\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* product key exists */
    r = pMsiSourceListAddSourceExA(prodcode, NULL,
                                  MSIINSTALLCONTEXT_MACHINE,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "C:\\source", 0);
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);

    res = RegCreateKeyA(prodkey, "SourceList", &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    RegCloseKey(hkey);

    /* SourceList exists */
    r = pMsiSourceListAddSourceExA(prodcode, NULL,
                                  MSIINSTALLCONTEXT_MACHINE,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "C:\\source", 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    res = RegOpenKeyA(prodkey, "SourceList\\URL", &url);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "1", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(value, "C:\\source/"), "Expected 'C:\\source/', got %s\n", value);
    ok(size == 11, "Expected 11, got %d\n", size);

    RegCloseKey(url);
    RegCloseKey(prodkey);
    HeapFree(GetProcessHeap(), 0, usersid);
}

START_TEST(source)
{
    init_functionpointers();

    test_MsiSourceListGetInfo();
    test_MsiSourceListAddSourceEx();
}
