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
#include <secext.h>
#include <objbase.h>

#include "wine/test.h"
#include "utils.h"

static BOOL is_wow64;

/* copied from dlls/msi/registry.c */
static BOOL squash_guid(LPCWSTR in, LPWSTR out)
{
    DWORD i,n=1;
    GUID guid;

    if (FAILED(CLSIDFromString((LPCOLESTR)in, &guid)))
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
    ok(hr == S_OK, "Expected S_OK, got %#lx\n", hr);

    size = StringFromGUID2(&guid, guidW, MAX_PATH);
    ok(size == 39, "Expected 39, got %#lx\n", hr);

    WideCharToMultiByte(CP_ACP, 0, guidW, size, prodcode, MAX_PATH, NULL, NULL);
    squash_guid(guidW, squashedW);
    WideCharToMultiByte(CP_ACP, 0, squashedW, -1, squashed, MAX_PATH, NULL, NULL);
}

static char *get_user_sid(void)
{
    HANDLE token;
    DWORD size = 0;
    TOKEN_USER *user;
    char *usersid = NULL;

    OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token);
    GetTokenInformation(token, TokenUser, NULL, size, &size);

    user = malloc(size);
    GetTokenInformation(token, TokenUser, user, size, &size);
    ConvertSidToStringSidA(user->User.Sid, &usersid);
    free(user);

    CloseHandle(token);
    return usersid;
}

static void check_reg_str(HKEY prodkey, LPCSTR name, LPCSTR expected, BOOL bcase, DWORD line)
{
    char val[MAX_PATH];
    DWORD size, type;
    LONG res;

    size = MAX_PATH;
    val[0] = '\0';
    res = RegQueryValueExA(prodkey, name, NULL, &type, (LPBYTE)val, &size);

    if (res != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ))
    {
        ok_(__FILE__, line)(FALSE, "Key doesn't exist or wrong type\n");
        return;
    }

    if (!expected)
        ok_(__FILE__, line)(!val[0], "Expected empty string, got %s\n", val);
    else
    {
        if (bcase)
            ok_(__FILE__, line)(!lstrcmpA(val, expected), "Expected %s, got %s\n", expected, val);
        else
            ok_(__FILE__, line)(!lstrcmpiA(val, expected), "Expected %s, got %s\n", expected, val);
    }
}

#define CHECK_REG_STR(prodkey, name, expected) \
    check_reg_str(prodkey, name, expected, TRUE, __LINE__);

static inline WCHAR *strdupAW( const char *str )
{
    int len;
    WCHAR *ret;
    len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
    if (!(ret = malloc( len * sizeof(WCHAR) ))) return NULL;
    MultiByteToWideChar( CP_ACP, 0, str, -1, ret, len );
    return ret;
}

static void test_MsiSourceListGetInfo(void)
{
    char prodcode[MAX_PATH], prod_squashed[MAX_PATH], keypath[MAX_PATH * 2], value[MAX_PATH], *usersid;
    WCHAR valueW[MAX_PATH], *usersidW, *prodcodeW;
    const char *data;
    LONG res;
    UINT r;
    HKEY userkey, hkey, media;
    DWORD size;

    create_test_guid(prodcode, prod_squashed);
    if (!(usersid = get_user_sid()))
    {
        skip("User SID not available -> skipping MsiSourceListGetInfoA tests\n");
        return;
    }

    /* NULL szProductCodeOrPatchCode */
    r = MsiSourceListGetInfoA(NULL, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAMEA, NULL, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* empty szProductCodeOrPatchCode */
    r = MsiSourceListGetInfoA("", usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAMEA, NULL, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* garbage szProductCodeOrPatchCode */
    r = MsiSourceListGetInfoA("garbage", usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAMEA, NULL, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* guid without brackets */
    r = MsiSourceListGetInfoA("51CD2AD5-0482-4C46-8DDD-0ED1022AA1AA", usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAMEA, NULL, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* guid with brackets */
    r = MsiSourceListGetInfoA("{51CD2AD5-0482-4C46-8DDD-0ED1022AA1AA}", usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAMEA, NULL, NULL);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);

    /* same length as guid, but random */
    r = MsiSourceListGetInfoA("ADKD-2KSDFF2-DKK1KNFJASD9GLKWME-1I3KAD", usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAMEA, NULL, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* invalid context */
    r = MsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_NONE,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAMEA, NULL, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* another invalid context */
    r = MsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_ALLUSERMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAMEA, NULL, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* yet another invalid context */
    r = MsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_ALL,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAMEA, NULL, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* mix two valid contexts */
    r = MsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED | MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAMEA, NULL, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* invalid option */
    r = MsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              4, INSTALLPROPERTY_PACKAGENAMEA, NULL, NULL);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);

    /* NULL property */
    r = MsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, NULL, NULL, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* empty property */
    r = MsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, "", NULL, NULL);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);

    /* value is non-NULL while size is NULL */
    r = MsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAMEA, value, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* size is non-NULL while value is NULL */
    size = MAX_PATH;
    r = MsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAMEA, NULL, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT || r == ERROR_INVALID_PARAMETER,
      "Expected ERROR_UNKNOWN_PRODUCT or ERROR_INVALID_PARAMETER, got %d\n", r);

    lstrcpyA(keypath, "Software\\Microsoft\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_CURRENT_USER, keypath, &userkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* user product key exists */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAMEA, value, &size);
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected \"aaa\", got \"%s\"\n", value);

    res = RegCreateKeyA(userkey, "SourceList", &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* SourceList key exists */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAMEA, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(size == 0, "Expected 0, got %lu\n", size);
    ok(!lstrcmpA(value, ""), "Expected \"\", got \"%s\"\n", value);

    data = "msitest.msi";
    res = RegSetValueExA(hkey, "PackageName", 0, REG_SZ, (const BYTE *)data, lstrlenA(data) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* PackageName value exists */
    size = 0xdeadbeef;
    r = MsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAMEA, NULL, &size);
    ok(r == ERROR_SUCCESS || r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_SUCCESS or ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(size == 11 || r != ERROR_SUCCESS, "Expected 11, got %lu\n", size);

    /* read the value, don't change size */
    size = 11;
    lstrcpyA(value, "aaa");
    r = MsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAMEA, value, &size);
    ok(r == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected 'aaa', got %s\n", value);
    ok(size == 11, "Expected 11, got %lu\n", size);

    /* read the value, fix size */
    size++;
    r = MsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAMEA, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, "msitest.msi"), "Expected 'msitest.msi', got %s\n", value);
    ok(size == 11, "Expected 11, got %lu\n", size);

    /* empty property now that product key exists */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, "", value, &size);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(size == MAX_PATH, "Expected %d, got %lu\n", MAX_PATH, size);
    ok(!lstrcmpA(value, "aaa"), "Expected \"aaa\", got \"%s\"\n", value);

    /* nonexistent property now that product key exists */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, "nonexistent", value, &size);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(size == MAX_PATH, "Expected %d, got %lu\n", MAX_PATH, size);
    ok(!lstrcmpA(value, "aaa"), "Expected \"aaa\", got \"%s\"\n", value);

    data = "tester";
    res = RegSetValueExA(hkey, "nonexistent", 0, REG_SZ, (const BYTE *)data, lstrlenA(data) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* nonexistent property now that nonexistent value exists */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, "nonexistent", value, &size);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(size == MAX_PATH, "Expected %d, got %lu\n", MAX_PATH, size);
    ok(!lstrcmpA(value, "aaa"), "Expected \"aaa\", got \"%s\"\n", value);

    /* invalid option now that product key exists */
    size = MAX_PATH;
    r = MsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              4, INSTALLPROPERTY_PACKAGENAMEA, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(size == 11, "Expected 11, got %lu\n", size);

    /* INSTALLPROPERTY_MEDIAPACKAGEPATH, media key does not exist */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_MEDIAPACKAGEPATHA, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, ""), "Expected \"\", got \"%s\"\n", value);
    ok(size == 0, "Expected 0, got %lu\n", size);

    res = RegCreateKeyA(hkey, "Media", &media);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    data = "path";
    res = RegSetValueExA(media, "MediaPackage", 0, REG_SZ,
                         (const BYTE *)data, lstrlenA(data) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* INSTALLPROPERTY_MEDIAPACKAGEPATH */
    size = MAX_PATH;
    r = MsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_MEDIAPACKAGEPATHA, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, "path"), "Expected \"path\", got \"%s\"\n", value);
    ok(size == 4, "Expected 4, got %lu\n", size);

    /* INSTALLPROPERTY_DISKPROMPT */
    data = "prompt";
    res = RegSetValueExA(media, "DiskPrompt", 0, REG_SZ,
                         (const BYTE *)data, lstrlenA(data) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    size = MAX_PATH;
    r = MsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_DISKPROMPTA, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, "prompt"), "Expected \"prompt\", got \"%s\"\n", value);
    ok(size == 6, "Expected 6, got %lu\n", size);

    /* LastUsedSource value doesn't exist */
    RegDeleteValueA(hkey, "LastUsedSource");
    size = MAX_PATH;
    memset(value, 0x55, sizeof(value));
    r = MsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_LASTUSEDSOURCEA, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(!lstrcmpA(value, ""), "Expected \"\", got \"%s\"\n", value);
    ok(size == 0, "Expected 0, got %lu\n", size);

    size = MAX_PATH;
    usersidW = strdupAW(usersid);
    prodcodeW = strdupAW(prodcode);
    memset(valueW, 0x55, sizeof(valueW));
    r = MsiSourceListGetInfoW(prodcodeW, usersidW, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_LASTUSEDSOURCEW, valueW, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(!valueW[0], "Expected \"\"");
    ok(size == 0, "Expected 0, got %lu\n", size);
    free(usersidW);
    free(prodcodeW);

    data = "";
    res = RegSetValueExA(hkey, "LastUsedSource", 0, REG_SZ,
                         (const BYTE *)data, lstrlenA(data) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* INSTALLPROPERTY_LASTUSEDSOURCE, source is empty */
    size = MAX_PATH;
    r = MsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_LASTUSEDSOURCEA, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, ""), "Expected \"\", got \"%s\"\n", value);
    ok(size == 0, "Expected 0, got %lu\n", size);

    data = "source";
    res = RegSetValueExA(hkey, "LastUsedSource", 0, REG_SZ,
                         (const BYTE *)data, lstrlenA(data) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* INSTALLPROPERTY_LASTUSEDSOURCE */
    size = MAX_PATH;
    r = MsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_LASTUSEDSOURCEA, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, "source"), "Expected \"source\", got \"%s\"\n", value);
    ok(size == 6, "Expected 6, got %lu\n", size);

    /* INSTALLPROPERTY_LASTUSEDSOURCE, size is too short */
    size = 4;
    lstrcpyA(value, "aaa");
    r = MsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_LASTUSEDSOURCEA, value, &size);
    ok(r == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got \"%s\"\n", value);
    ok(size == 6, "Expected 6, got %lu\n", size);

    /* INSTALLPROPERTY_LASTUSEDSOURCE, size is exactly 6 */
    size = 6;
    lstrcpyA(value, "aaa");
    r = MsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_LASTUSEDSOURCEA, value, &size);
    ok(r == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got \"%s\"\n", value);
    ok(size == 6, "Expected 6, got %lu\n", size);

    data = "a;source";
    res = RegSetValueExA(hkey, "LastUsedSource", 0, REG_SZ, (const BYTE *)data, lstrlenA(data) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* INSTALLPROPERTY_LASTUSEDSOURCE, one semi-colon */
    size = MAX_PATH;
    r = MsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_LASTUSEDSOURCEA, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, "source"), "Expected \"source\", got \"%s\"\n", value);
    ok(size == 6, "Expected 6, got %lu\n", size);

    data = "a:source";
    res = RegSetValueExA(hkey, "LastUsedSource", 0, REG_SZ, (const BYTE *)data, lstrlenA(data) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* INSTALLPROPERTY_LASTUSEDSOURCE, one colon */
    size = MAX_PATH;
    r = MsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_LASTUSEDSOURCEA, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, "a:source"), "Expected \"a:source\", got \"%s\"\n", value);
    ok(size == 8, "Expected 8, got %lu\n", size);

    /* INSTALLPROPERTY_LASTUSEDTYPE, invalid source format */
    size = MAX_PATH;
    r = MsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_LASTUSEDTYPEA, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, ""), "Expected \"\", got \"%s\"\n", value);
    ok(size == 0, "Expected 0, got %lu\n", size);

    data = "x;y;z";
    res = RegSetValueExA(hkey, "LastUsedSource", 0, REG_SZ, (const BYTE *)data, lstrlenA(data) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* INSTALLPROPERTY_LASTUSEDTYPE, invalid source format */
    size = MAX_PATH;
    r = MsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_LASTUSEDTYPEA, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, ""), "Expected \"\", got \"%s\"\n", value);
    ok(size == 0, "Expected 0, got %lu\n", size);

    data = "n;y;z";
    res = RegSetValueExA(hkey, "LastUsedSource", 0, REG_SZ, (const BYTE *)data, lstrlenA(data) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* INSTALLPROPERTY_LASTUSEDTYPE */
    size = MAX_PATH;
    r = MsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_LASTUSEDTYPEA, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, "n"), "Expected \"n\", got \"%s\"\n", value);
    ok(size == 1, "Expected 1, got %lu\n", size);

    data = "negatory";
    res = RegSetValueExA(hkey, "LastUsedSource", 0, REG_SZ, (const BYTE *)data, lstrlenA(data) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* INSTALLPROPERTY_LASTUSEDTYPE */
    size = MAX_PATH;
    r = MsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_LASTUSEDTYPEA, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, "n"), "Expected \"n\", got \"%s\"\n", value);
    ok(size == 1, "Expected 1, got %lu\n", size);

    data = "megatron";
    res = RegSetValueExA(hkey, "LastUsedSource", 0, REG_SZ, (const BYTE *)data, lstrlenA(data) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* INSTALLPROPERTY_LASTUSEDTYPE */
    size = MAX_PATH;
    r = MsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_LASTUSEDTYPEA, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, "m"), "Expected \"m\", got \"%s\"\n", value);
    ok(size == 1, "Expected 1, got %lu\n", size);

    data = "useless";
    res = RegSetValueExA(hkey, "LastUsedSource", 0, REG_SZ, (const BYTE *)data, lstrlenA(data) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* INSTALLPROPERTY_LASTUSEDTYPE */
    size = MAX_PATH;
    r = MsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_LASTUSEDTYPEA, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, "u"), "Expected \"u\", got \"%s\"\n", value);
    ok(size == 1, "Expected 1, got %lu\n", size);

    RegDeleteValueA(media, "MediaPackage");
    RegDeleteValueA(media, "DiskPrompt");
    RegDeleteKeyA(media, "");
    RegDeleteValueA(hkey, "LastUsedSource");
    RegDeleteValueA(hkey, "nonexistent");
    RegDeleteValueA(hkey, "PackageName");
    RegDeleteKeyA(hkey, "");
    RegDeleteKeyA(userkey, "");
    RegCloseKey(hkey);
    RegCloseKey(userkey);

    /* try a patch */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PATCH, INSTALLPROPERTY_PACKAGENAMEA, value, &size);
    ok(r == ERROR_UNKNOWN_PATCH, "Expected ERROR_UNKNOWN_PATCH, got %d\n", r);
    ok(size == MAX_PATH, "Expected %d, got %lu\n", MAX_PATH, size);
    ok(!lstrcmpA(value, "aaa"), "Expected \"aaa\", got \"%s\"\n", value);

    lstrcpyA(keypath, "Software\\Microsoft\\Installer\\Patches\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_CURRENT_USER, keypath, &userkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* patch key exists
     * NOTE: using prodcode guid, but it really doesn't matter
     */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PATCH, INSTALLPROPERTY_PACKAGENAMEA, value, &size);
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);
    ok(size == MAX_PATH, "Expected %d, got %lu\n", MAX_PATH, size);
    ok(!lstrcmpA(value, "aaa"), "Expected \"aaa\", got \"%s\"\n", value);

    res = RegCreateKeyA(userkey, "SourceList", &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* SourceList key exists */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PATCH, INSTALLPROPERTY_PACKAGENAMEA, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, ""), "Expected \"\", got \"%s\"\n", value);
    ok(size == 0, "Expected 0, got %lu\n", size);

    RegDeleteKeyA(hkey, "");
    RegDeleteKeyA(userkey, "");
    RegCloseKey(hkey);
    RegCloseKey(userkey);
    LocalFree(usersid);
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
    HKEY prodkey, userkey, hkey, url, net;
    DWORD size;
    REGSAM access = KEY_ALL_ACCESS;

    if (!is_process_elevated())
    {
        skip("process is limited\n");
        return;
    }

    create_test_guid(prodcode, prod_squashed);
    if (!(usersid = get_user_sid()))
    {
        skip("User SID not available -> skipping MsiSourceListAddSourceExA tests\n");
        return;
    }

    if (is_wow64)
        access |= KEY_WOW64_64KEY;

    /* GetLastError is not set by the function */

    /* NULL szProductCodeOrPatchCode */
    r = MsiSourceListAddSourceExA(NULL, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "C:\\source", 0);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* empty szProductCodeOrPatchCode */
    r = MsiSourceListAddSourceExA("", usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "C:\\source", 0);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* garbage szProductCodeOrPatchCode */
    r = MsiSourceListAddSourceExA("garbage", usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "C:\\source", 0);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* guid without brackets */
    r = MsiSourceListAddSourceExA("51CD2AD5-0482-4C46-8DDD-0ED1022AA1AA", usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "C:\\source", 0);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* guid with brackets */
    r = MsiSourceListAddSourceExA("{51CD2AD5-0482-4C46-8DDD-0ED1022AA1AA}", usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "C:\\source", 0);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);

    /* MSIINSTALLCONTEXT_USERUNMANAGED */

    r = MsiSourceListAddSourceExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "C:\\source", 0);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);

    lstrcpyA(keypath, "Software\\Microsoft\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_CURRENT_USER, keypath, &userkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* user product key exists */
    r = MsiSourceListAddSourceExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "C:\\source", 0);
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);

    res = RegCreateKeyA(userkey, "SourceList", &url);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    RegCloseKey(url);

    /* SourceList key exists */
    r = MsiSourceListAddSourceExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "C:\\source", 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    res = RegOpenKeyA(userkey, "SourceList\\URL", &url);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "1", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    ok(!lstrcmpA(value, "C:\\source/"), "Expected 'C:\\source/', got %s\n", value);
    ok(size == 11, "Expected 11, got %lu\n", size);

    /* add another source, index 0 */
    r = MsiSourceListAddSourceExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "another", 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "1", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    ok(!lstrcmpA(value, "C:\\source/"), "Expected 'C:\\source/', got %s\n", value);
    ok(size == 11, "Expected 11, got %lu\n", size);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "2", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    ok(!lstrcmpA(value, "another/"), "Expected 'another/', got %s\n", value);
    ok(size == 9, "Expected 9, got %lu\n", size);

    /* add another source, index 1 */
    r = MsiSourceListAddSourceExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "third/", 1);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "1", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    ok(!lstrcmpA(value, "third/"), "Expected 'third/', got %s\n", value);
    ok(size == 7, "Expected 7, got %lu\n", size);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "2", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    ok(!lstrcmpA(value, "C:\\source/"), "Expected 'C:\\source/', got %s\n", value);
    ok(size == 11, "Expected 11, got %lu\n", size);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "3", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    ok(!lstrcmpA(value, "another/"), "Expected 'another/', got %s\n", value);
    ok(size == 9, "Expected 9, got %lu\n", size);

    /* add another source, index > N */
    r = MsiSourceListAddSourceExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "last/", 5);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "1", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    ok(!lstrcmpA(value, "third/"), "Expected 'third/', got %s\n", value);
    ok(size == 7, "Expected 7, got %lu\n", size);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "2", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    ok(!lstrcmpA(value, "C:\\source/"), "Expected 'C:\\source/', got %s\n", value);
    ok(size == 11, "Expected 11, got %lu\n", size);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "3", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    ok(!lstrcmpA(value, "another/"), "Expected 'another/', got %s\n", value);
    ok(size == 9, "Expected 9, got %lu\n", size);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "4", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    ok(!lstrcmpA(value, "last/"), "Expected 'last/', got %s\n", value);
    ok(size == 6, "Expected 6, got %lu\n", size);

    /* just MSISOURCETYPE_NETWORK */
    r = MsiSourceListAddSourceExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSISOURCETYPE_NETWORK, "source", 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    res = RegOpenKeyA(userkey, "SourceList\\Net", &net);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    size = MAX_PATH;
    res = RegQueryValueExA(net, "1", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    ok(!lstrcmpA(value, "source\\"), "Expected 'source\\', got %s\n", value);
    ok(size == 8, "Expected 8, got %lu\n", size);

    /* just MSISOURCETYPE_URL */
    r = MsiSourceListAddSourceExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSISOURCETYPE_URL, "source", 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "1", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    ok(!lstrcmpA(value, "third/"), "Expected 'third/', got %s\n", value);
    ok(size == 7, "Expected 7, got %lu\n", size);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "2", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    ok(!lstrcmpA(value, "C:\\source/"), "Expected 'C:\\source/', got %s\n", value);
    ok(size == 11, "Expected 11, got %lu\n", size);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "3", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    ok(!lstrcmpA(value, "another/"), "Expected 'another/', got %s\n", value);
    ok(size == 9, "Expected 9, got %lu\n", size);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "4", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    ok(!lstrcmpA(value, "last/"), "Expected 'last/', got %s\n", value);
    ok(size == 6, "Expected 6, got %lu\n", size);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "5", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    ok(!lstrcmpA(value, "source/"), "Expected 'source/', got %s\n", value);
    ok(size == 8, "Expected 8, got %lu\n", size);

    /* NULL szUserSid */
    r = MsiSourceListAddSourceExA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSISOURCETYPE_NETWORK, "nousersid", 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    res = RegQueryValueExA(net, "1", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    ok(!lstrcmpA(value, "source\\"), "Expected 'source\\', got %s\n", value);
    ok(size == 8, "Expected 8, got %lu\n", size);

    size = MAX_PATH;
    res = RegQueryValueExA(net, "2", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    ok(!lstrcmpA(value, "nousersid\\"), "Expected 'nousersid\\', got %s\n", value);
    ok(size == 11, "Expected 11, got %lu\n", size);

    /* invalid options, must have source type */
    r = MsiSourceListAddSourceExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT, "source", 0);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    r = MsiSourceListAddSourceExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PATCH, "source", 0);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* NULL szSource */
    r = MsiSourceListAddSourceExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSISOURCETYPE_URL, NULL, 1);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* empty szSource */
    r = MsiSourceListAddSourceExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSISOURCETYPE_URL, "", 1);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* MSIINSTALLCONTEXT_USERMANAGED, non-NULL szUserSid */

    r = MsiSourceListAddSourceExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "C:\\source", 0);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Managed\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &prodkey, NULL);
    if (res != ERROR_SUCCESS)
    {
        skip("Product key creation failed with error code %ld\n", res);
        goto machine_tests;
    }

    /* product key exists */
    r = MsiSourceListAddSourceExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "C:\\source", 0);
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);

    res = RegCreateKeyExA(prodkey, "SourceList", 0, NULL, 0, access, NULL, &hkey, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    RegCloseKey(hkey);

    /* SourceList exists */
    r = MsiSourceListAddSourceExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "C:\\source", 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    res = RegOpenKeyExA(prodkey, "SourceList\\URL", 0, access, &url);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "1", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    ok(!lstrcmpA(value, "C:\\source/"), "Expected 'C:\\source/', got %s\n", value);
    ok(size == 11, "Expected 11, got %lu\n", size);

    RegCloseKey(url);

    /* MSIINSTALLCONTEXT_USERMANAGED, NULL szUserSid */

    r = MsiSourceListAddSourceExA(prodcode, NULL, MSIINSTALLCONTEXT_USERMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "another", 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    res = RegOpenKeyExA(prodkey, "SourceList\\URL", 0, access, &url);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "1", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    ok(!lstrcmpA(value, "C:\\source/"), "Expected 'C:\\source/', got %s\n", value);
    ok(size == 11, "Expected 11, got %lu\n", size);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "2", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    ok(!lstrcmpA(value, "another/"), "Expected 'another/', got %s\n", value);
    ok(size == 9, "Expected 9, got %lu\n", size);

    RegCloseKey(url);
    RegCloseKey(prodkey);

    /* MSIINSTALLCONTEXT_MACHINE */

machine_tests:
    /* szUserSid must be NULL for MSIINSTALLCONTEXT_MACHINE */
    r = MsiSourceListAddSourceExA(prodcode, usersid, MSIINSTALLCONTEXT_MACHINE,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "C:\\source", 0);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    r = MsiSourceListAddSourceExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "C:\\source", 0);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);

    lstrcpyA(keypath, "Software\\Classes\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &prodkey, NULL);
    if (res != ERROR_SUCCESS)
    {
        skip("Product key creation failed with error code %ld\n", res);
        LocalFree(usersid);
        return;
    }

    /* product key exists */
    r = MsiSourceListAddSourceExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "C:\\source", 0);
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);

    res = RegCreateKeyExA(prodkey, "SourceList", 0, NULL, 0, access, NULL, &hkey, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    RegCloseKey(hkey);

    /* SourceList exists */
    r = MsiSourceListAddSourceExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "C:\\source", 0);
    if (r == ERROR_ACCESS_DENIED)
        skip("MsiSourceListAddSourceEx (insufficient privileges)\n");
    else
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

        res = RegOpenKeyExA(prodkey, "SourceList\\URL", 0, access, &url);
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

        size = MAX_PATH;
        res = RegQueryValueExA(url, "1", NULL, NULL, (LPBYTE)value, &size);
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
        ok(!lstrcmpA(value, "C:\\source/"), "Expected 'C:\\source/', got %s\n", value);
        ok(size == 11, "Expected 11, got %lu\n", size);

        RegCloseKey(url);
        RegCloseKey(prodkey);
    }
    LocalFree(usersid);
}

static void test_MsiSourceListEnumSources(void)
{
    CHAR prodcode[MAX_PATH];
    CHAR prod_squashed[MAX_PATH];
    CHAR keypath[MAX_PATH*2];
    CHAR value[MAX_PATH];
    LPSTR usersid;
    LONG res;
    UINT r;
    HKEY prodkey, userkey;
    HKEY url, net, source;
    DWORD size;
    REGSAM access = KEY_ALL_ACCESS;

    create_test_guid(prodcode, prod_squashed);
    if (!(usersid = get_user_sid()))
    {
        skip("User SID not available -> skipping MsiSourceListEnumSourcesA tests\n");
        return;
    }

    if (is_wow64)
        access |= KEY_WOW64_64KEY;

    /* GetLastError is not set by the function */

    /* NULL szProductCodeOrPatchCode */
    size = 0xdeadbeef;
    r = MsiSourceListEnumSourcesA(NULL, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_NETWORK, 0, value, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(size == 0xdeadbeef, "Expected 0xdeadbeef, got %lu\n", size);

    /* empty szProductCodeOrPatchCode */
    size = 0xdeadbeef;
    r = MsiSourceListEnumSourcesA("", usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_NETWORK, 0, value, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(size == 0xdeadbeef, "Expected 0xdeadbeef, got %lu\n", size);

    /* garbage szProductCodeOrPatchCode */
    size = 0xdeadbeef;
    r = MsiSourceListEnumSourcesA("garbage", usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_NETWORK, 0, value, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(size == 0xdeadbeef, "Expected 0xdeadbeef, got %lu\n", size);

    /* guid without brackets */
    size = 0xdeadbeef;
    r = MsiSourceListEnumSourcesA("51CD2AD5-0482-4C46-8DDD-0ED1022AA1AA",
                                  usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_NETWORK, 0, value, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(size == 0xdeadbeef, "Expected 0xdeadbeef, got %lu\n", size);

    /* guid with brackets */
    size = 0xdeadbeef;
    r = MsiSourceListEnumSourcesA("{51CD2AD5-0482-4C46-8DDD-0ED1022AA1AA}",
                                  usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_NETWORK, 0, value, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(size == 0xdeadbeef, "Expected 0xdeadbeef, got %lu\n", size);

    /* MSIINSTALLCONTEXT_USERUNMANAGED */

    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %lu\n", size);

    lstrcpyA(keypath, "Software\\Microsoft\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_CURRENT_USER, keypath, &userkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* user product key exists */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %lu\n", size);

    res = RegCreateKeyA(userkey, "SourceList", &source);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* SourceList key exists */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %lu\n", size);

    res = RegCreateKeyA(source, "URL", &url);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* URL key exists */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %lu\n", size);

    res = RegSetValueExA(url, "1", 0, REG_SZ, (LPBYTE)"first", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    res = RegSetValueExA(url, "2", 0, REG_SZ, (LPBYTE)"second", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    res = RegSetValueExA(url, "4", 0, REG_SZ, (LPBYTE)"fourth", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* sources exist */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, "first"), "Expected \"first\", got %s\n", value);
    ok(size == 5, "Expected 5, got %lu\n", size);

    /* try index 0 again */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, "first"), "Expected \"first\", got %s\n", value);
    ok(size == 5, "Expected 5, got %lu\n", size);

    /* both szSource and pcchSource are NULL, index 0 */
    r = MsiSourceListEnumSourcesA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, NULL, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* both szSource and pcchSource are NULL, index 1 */
    r = MsiSourceListEnumSourcesA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 1, NULL, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* size is exactly 5 */
    size = 5;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected \"aaa\", got %s\n", value);
    ok(size == 5, "Expected 5, got %lu\n", size);

    /* szSource is non-NULL while pcchSource is NULL */
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected \"aaa\", got %s\n", value);

    /* try index 1 after failure */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 1, value, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected \"aaa\", got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %lu\n", size);

    /* reset the enumeration */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, "first"), "Expected \"first\", got %s\n", value);
    ok(size == 5, "Expected 5, got %lu\n", size);

    /* try index 1 */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 1, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, "second"), "Expected \"second\", got %s\n", value);
    ok(size == 6, "Expected 6, got %lu\n", size);

    /* try index 1 again */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 1, value, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %lu\n", size);

    /* try index 2 */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 2, value, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %lu\n", size);

    /* try index < 0 */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, -1, value, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %lu\n", size);

    /* NULL szUserSid */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, "first"), "Expected \"first\", got %s\n", value);
    ok(size == 5, "Expected 5, got %lu\n", size);

    /* invalid dwOptions, must be one of MSICODE_ and MSISOURCETYPE_ */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT, 0, value, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %lu\n", size);

    /* invalid dwOptions, must be one of MSICODE_ and MSISOURCETYPE_ */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PATCH, 0, value, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %lu\n", size);

    /* invalid dwOptions, must be one of MSICODE_ and MSISOURCETYPE_ */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSICODE_PATCH | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_UNKNOWN_PATCH, "Expected ERROR_UNKNOWN_PATCH, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %lu\n", size);

    /* invalid dwOptions, must be one of MSICODE_ and MSISOURCETYPE_ */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_NETWORK | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %lu\n", size);

    RegDeleteValueA(url, "1");
    RegDeleteValueA(url, "2");
    RegDeleteValueA(url, "4");
    RegDeleteKeyA(url, "");
    RegCloseKey(url);

    /* SourceList key exists */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_NETWORK, 0, value, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %lu\n", size);

    res = RegCreateKeyA(source, "Net", &net);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Net key exists */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_NETWORK, 0, value, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %lu\n", size);

    res = RegSetValueExA(net, "1", 0, REG_SZ, (LPBYTE)"first", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* sources exist */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_NETWORK, 0, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, "first"), "Expected \"first\", got %s\n", value);
    ok(size == 5, "Expected 5, got %lu\n", size);

    RegDeleteValueA(net, "1");
    RegDeleteKeyA(net, "");
    RegCloseKey(net);
    RegDeleteKeyA(source, "");
    RegCloseKey(source);
    RegDeleteKeyA(userkey, "");
    RegCloseKey(userkey);

    /* MSIINSTALLCONTEXT_USERMANAGED */

    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %lu\n", size);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Managed\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &userkey, NULL);
    if (res != ERROR_SUCCESS)
    {
        skip("Product key creation failed with error code %ld\n", res);
        goto machine_tests;
    }

    /* user product key exists */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %lu\n", size);

    res = RegCreateKeyExA(userkey, "SourceList", 0, NULL, 0, access, NULL, &source, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* SourceList key exists */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %lu\n", size);

    res = RegCreateKeyExA(source, "URL", 0, NULL, 0, access, NULL, &url, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* URL key exists */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %lu\n", size);

    res = RegSetValueExA(url, "1", 0, REG_SZ, (LPBYTE)"first", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* sources exist */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, "first"), "Expected \"first\", got %s\n", value);
    ok(size == 5, "Expected 5, got %lu\n", size);

    /* NULL szUserSid */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, NULL, MSIINSTALLCONTEXT_USERMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, "first"), "Expected \"first\", got %s\n", value);
    ok(size == 5, "Expected 5, got %lu\n", size);

    RegDeleteValueA(url, "1");
    RegDeleteKeyExA(url, "", access, 0);
    RegCloseKey(url);

    /* SourceList key exists */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_NETWORK, 0, value, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %lu\n", size);

    res = RegCreateKeyExA(source, "Net", 0, NULL, 0, access, NULL, &net, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Net key exists */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_NETWORK, 0, value, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %lu\n", size);

    res = RegSetValueExA(net, "1", 0, REG_SZ, (LPBYTE)"first", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* sources exist */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_NETWORK, 0, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, "first"), "Expected \"first\", got %s\n", value);
    ok(size == 5, "Expected 5, got %lu\n", size);

    RegDeleteValueA(net, "1");
    RegDeleteKeyExA(net, "", access, 0);
    RegCloseKey(net);
    RegDeleteKeyExA(source, "", access, 0);
    RegCloseKey(source);
    RegDeleteKeyExA(userkey, "", access, 0);
    RegCloseKey(userkey);

    /* MSIINSTALLCONTEXT_MACHINE */

machine_tests:
    /* szUserSid is non-NULL */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid, MSIINSTALLCONTEXT_MACHINE,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %lu\n", size);

    /* szUserSid is NULL */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %lu\n", size);

    lstrcpyA(keypath, "Software\\Classes\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &prodkey, NULL);
    if (res != ERROR_SUCCESS)
    {
        skip("Product key creation failed with error code %ld\n", res);
        LocalFree(usersid);
        return;
    }

    /* user product key exists */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %lu\n", size);

    res = RegCreateKeyExA(prodkey, "SourceList", 0, NULL, 0, access, NULL, &source, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* SourceList key exists */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %lu\n", size);

    res = RegCreateKeyExA(source, "URL", 0, NULL, 0, access, NULL, &url, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* URL key exists */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %lu\n", size);

    res = RegSetValueExA(url, "1", 0, REG_SZ, (LPBYTE)"first", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* sources exist */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, "first"), "Expected \"first\", got %s\n", value);
    ok(size == 5, "Expected 5, got %lu\n", size);

    /* NULL szUserSid */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, "first"), "Expected \"first\", got %s\n", value);
    ok(size == 5, "Expected 5, got %lu\n", size);

    RegDeleteValueA(url, "1");
    RegDeleteKeyExA(url, "", access, 0);
    RegCloseKey(url);

    /* SourceList key exists */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE,
                                  MSICODE_PRODUCT | MSISOURCETYPE_NETWORK, 0, value, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %lu\n", size);

    res = RegCreateKeyExA(source, "Net", 0, NULL, 0, access, NULL, &net, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Net key exists */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE,
                                  MSICODE_PRODUCT | MSISOURCETYPE_NETWORK, 0, value, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %lu\n", size);

    res = RegSetValueExA(net, "1", 0, REG_SZ, (LPBYTE)"first", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* sources exist */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE,
                                  MSICODE_PRODUCT | MSISOURCETYPE_NETWORK, 0, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, "first"), "Expected \"first\", got %s\n", value);
    ok(size == 5, "Expected 5, got %lu\n", size);

    RegDeleteValueA(net, "1");
    RegDeleteKeyExA(net, "", access, 0);
    RegCloseKey(net);
    RegDeleteKeyExA(source, "", access, 0);
    RegCloseKey(source);
    RegDeleteKeyExA(prodkey, "", access, 0);
    RegCloseKey(prodkey);
    LocalFree(usersid);
}

static void test_MsiSourceListSetInfo(void)
{
    CHAR prodcode[MAX_PATH];
    CHAR prod_squashed[MAX_PATH];
    CHAR keypath[MAX_PATH*2];
    HKEY prodkey, userkey;
    HKEY net, url, media, source;
    LPSTR usersid;
    LONG res;
    UINT r;
    REGSAM access = KEY_ALL_ACCESS;

    if (!is_process_elevated())
    {
        skip("process is limited\n");
        return;
    }

    create_test_guid(prodcode, prod_squashed);
    if (!(usersid = get_user_sid()))
    {
        skip("User SID not available -> skipping MsiSourceListSetInfoA tests\n");
        return;
    }

    if (is_wow64)
        access |= KEY_WOW64_64KEY;

    /* GetLastError is not set by the function */

    /* NULL szProductCodeOrPatchCode */
    r = MsiSourceListSetInfoA(NULL, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT | MSISOURCETYPE_NETWORK,
                              INSTALLPROPERTY_MEDIAPACKAGEPATHA, "path");
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* empty szProductCodeOrPatchCode */
    r = MsiSourceListSetInfoA("", usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT | MSISOURCETYPE_NETWORK,
                              INSTALLPROPERTY_MEDIAPACKAGEPATHA, "path");
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* garbage szProductCodeOrPatchCode */
    r = MsiSourceListSetInfoA("garbage", usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT | MSISOURCETYPE_NETWORK,
                              INSTALLPROPERTY_MEDIAPACKAGEPATHA, "path");
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* guid without brackets */
    r = MsiSourceListSetInfoA("51CD2AD5-0482-4C46-8DDD-0ED1022AA1AA",
                              usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT | MSISOURCETYPE_NETWORK,
                              INSTALLPROPERTY_MEDIAPACKAGEPATHA, "path");
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* guid with brackets */
    r = MsiSourceListSetInfoA("{51CD2AD5-0482-4C46-8DDD-0ED1022AA1AA}",
                              usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT | MSISOURCETYPE_NETWORK,
                              INSTALLPROPERTY_MEDIAPACKAGEPATHA, "path");
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);

    /* dwOptions is MSICODE_PRODUCT */
    r = MsiSourceListSetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, MSICODE_PRODUCT,
                              INSTALLPROPERTY_MEDIAPACKAGEPATHA, "path");
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);

    /* dwOptions is MSICODE_PATCH */
    r = MsiSourceListSetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, MSICODE_PATCH,
                              INSTALLPROPERTY_MEDIAPACKAGEPATHA, "path");
    ok(r == ERROR_UNKNOWN_PATCH, "Expected ERROR_UNKNOWN_PATCH, got %d\n", r);

    /* dwOptions is both MSICODE_PRODUCT and MSICODE_PATCH */
    r = MsiSourceListSetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT | MSICODE_PATCH | MSISOURCETYPE_URL,
                              INSTALLPROPERTY_MEDIAPACKAGEPATHA, "path");
    ok(r == ERROR_UNKNOWN_PATCH, "Expected ERROR_UNKNOWN_PATCH, got %d\n", r);

    /* dwOptions has both MSISOURCETYPE_NETWORK and MSISOURCETYPE_URL */
    r = MsiSourceListSetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT | MSISOURCETYPE_NETWORK | MSISOURCETYPE_URL,
                              INSTALLPROPERTY_MEDIAPACKAGEPATHA, "path");
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);

    /* LastUsedSource and dwOptions has both
     * MSISOURCETYPE_NETWORK and MSISOURCETYPE_URL
     */
    r = MsiSourceListSetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT | MSISOURCETYPE_NETWORK | MSISOURCETYPE_URL,
                              INSTALLPROPERTY_LASTUSEDSOURCEA, "path");
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);

    /* LastUsedSource and dwOptions has no source type */
    r = MsiSourceListSetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED, MSICODE_PRODUCT,
                              INSTALLPROPERTY_LASTUSEDSOURCEA, "path");
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);

    /* MSIINSTALLCONTEXT_USERUNMANAGED */

    lstrcpyA(keypath, "Software\\Microsoft\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_CURRENT_USER, keypath, &userkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* user product key exists */
    r = MsiSourceListSetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED, MSICODE_PRODUCT,
                              INSTALLPROPERTY_MEDIAPACKAGEPATHA, "path");
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);

    res = RegCreateKeyA(userkey, "SourceList", &source);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* SourceList key exists, no source type */
    r = MsiSourceListSetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED, MSICODE_PRODUCT,
                              INSTALLPROPERTY_MEDIAPACKAGEPATHA, "path");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* Media key is created by MsiSourceListSetInfo */
    res = RegOpenKeyA(source, "Media", &media);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    CHECK_REG_STR(media, "MediaPackage", "path");

    /* set the info again */
    r = MsiSourceListSetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED, MSICODE_PRODUCT,
                              INSTALLPROPERTY_MEDIAPACKAGEPATHA, "path2");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    CHECK_REG_STR(media, "MediaPackage", "path2");

    /* NULL szProperty */
    r = MsiSourceListSetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED, MSICODE_PRODUCT,
                              NULL, "path");
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* empty szProperty */
    r = MsiSourceListSetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED, MSICODE_PRODUCT,
                              "", "path");
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);

    /* NULL szValue */
    r = MsiSourceListSetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED, MSICODE_PRODUCT,
                              INSTALLPROPERTY_MEDIAPACKAGEPATHA, NULL);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);

    /* empty szValue */
    r = MsiSourceListSetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED, MSICODE_PRODUCT,
                              INSTALLPROPERTY_MEDIAPACKAGEPATHA, "");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    CHECK_REG_STR(media, "MediaPackage", "");

    /* INSTALLPROPERTY_MEDIAPACKAGEPATH, MSISOURCETYPE_NETWORK */
    r = MsiSourceListSetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT | MSISOURCETYPE_NETWORK,
                              INSTALLPROPERTY_MEDIAPACKAGEPATHA, "path");
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* INSTALLPROPERTY_MEDIAPACKAGEPATH, MSISOURCETYPE_URL */
    r = MsiSourceListSetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT | MSISOURCETYPE_URL,
                              INSTALLPROPERTY_MEDIAPACKAGEPATHA, "path");
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* INSTALLPROPERTY_DISKPROMPT */
    r = MsiSourceListSetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED, MSICODE_PRODUCT,
                              INSTALLPROPERTY_DISKPROMPTA, "prompt");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    CHECK_REG_STR(media, "DiskPrompt", "prompt");

    /* INSTALLPROPERTY_DISKPROMPT, MSISOURCETYPE_NETWORK */
    r = MsiSourceListSetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT | MSISOURCETYPE_NETWORK,
                              INSTALLPROPERTY_DISKPROMPTA, "prompt");
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* INSTALLPROPERTY_DISKPROMPT, MSISOURCETYPE_URL */
    r = MsiSourceListSetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT | MSISOURCETYPE_URL,
                              INSTALLPROPERTY_DISKPROMPTA, "prompt");
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* INSTALLPROPERTY_LASTUSEDSOURCE */
    r = MsiSourceListSetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED, MSICODE_PRODUCT,
                              INSTALLPROPERTY_LASTUSEDSOURCEA, "source");
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* INSTALLPROPERTY_LASTUSEDSOURCE, MSISOURCETYPE_NETWORK */
    r = MsiSourceListSetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT | MSISOURCETYPE_NETWORK,
                              INSTALLPROPERTY_LASTUSEDSOURCEA, "source");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* Net key is created by MsiSourceListSetInfo */
    res = RegOpenKeyA(source, "Net", &net);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    CHECK_REG_STR(net, "1", "source\\")
    CHECK_REG_STR(source, "LastUsedSource", "n;1;source");

    /* source has forward slash */
    r = MsiSourceListSetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT | MSISOURCETYPE_NETWORK,
                              INSTALLPROPERTY_LASTUSEDSOURCEA, "source/");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    CHECK_REG_STR(net, "1", "source\\");
    CHECK_REG_STR(net, "2", "source/\\");
    CHECK_REG_STR(source, "LastUsedSource", "n;2;source/");

    /* INSTALLPROPERTY_LASTUSEDSOURCE, MSISOURCETYPE_URL */
    r = MsiSourceListSetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT | MSISOURCETYPE_URL,
                              INSTALLPROPERTY_LASTUSEDSOURCEA, "source");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* URL key is created by MsiSourceListSetInfo */
    res = RegOpenKeyA(source, "URL", &url);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    CHECK_REG_STR(url, "1", "source/");
    CHECK_REG_STR(source, "LastUsedSource", "u;1;source");

    /* source has backslash */
    r = MsiSourceListSetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT | MSISOURCETYPE_URL,
                              INSTALLPROPERTY_LASTUSEDSOURCEA, "source\\");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    CHECK_REG_STR(url, "1", "source/");
    CHECK_REG_STR(url, "2", "source\\/");
    CHECK_REG_STR(source, "LastUsedSource", "u;2;source\\");

    /* INSTALLPROPERTY_LASTUSEDSOURCE, MSISOURCETYPE_MEDIA */
    r = MsiSourceListSetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT | MSISOURCETYPE_MEDIA,
                              INSTALLPROPERTY_LASTUSEDSOURCEA, "source");
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* INSTALLPROPERTY_PACKAGENAME */
    r = MsiSourceListSetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED, MSICODE_PRODUCT,
                              INSTALLPROPERTY_PACKAGENAMEA, "name");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    CHECK_REG_STR(source, "PackageName", "name");

    /* INSTALLPROPERTY_PACKAGENAME, MSISOURCETYPE_NETWORK */
    r = MsiSourceListSetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT | MSISOURCETYPE_NETWORK,
                              INSTALLPROPERTY_PACKAGENAMEA, "name");
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* INSTALLPROPERTY_PACKAGENAME, MSISOURCETYPE_URL */
    r = MsiSourceListSetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT | MSISOURCETYPE_URL,
                              INSTALLPROPERTY_PACKAGENAMEA, "name");
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* INSTALLPROPERTY_LASTUSEDTYPE */
    r = MsiSourceListSetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED, MSICODE_PRODUCT,
                              INSTALLPROPERTY_LASTUSEDTYPEA, "type");
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);

    /* definitely unknown property */
    r = MsiSourceListSetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED, MSICODE_PRODUCT,
                              "unknown", "val");
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);

    RegDeleteValueA(net, "1");
    RegDeleteKeyA(net, "");
    RegCloseKey(net);
    RegDeleteValueA(url, "1");
    RegDeleteKeyA(url, "");
    RegCloseKey(url);
    RegDeleteValueA(media, "MediaPackage");
    RegDeleteValueA(media, "DiskPrompt");
    RegDeleteKeyA(media, "");
    RegCloseKey(media);
    RegDeleteValueA(source, "PackageName");
    RegDeleteKeyA(source, "");
    RegCloseKey(source);
    RegDeleteKeyA(userkey, "");
    RegCloseKey(userkey);

    /* MSIINSTALLCONTEXT_USERMANAGED */

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Managed\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &userkey, NULL);
    if (res != ERROR_SUCCESS)
    {
        skip("Product key creation failed with error code %ld\n", res);
        goto machine_tests;
    }

    /* user product key exists */
    r = MsiSourceListSetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERMANAGED, MSICODE_PRODUCT,
                              INSTALLPROPERTY_MEDIAPACKAGEPATHA, "path");
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);

    res = RegCreateKeyExA(userkey, "SourceList", 0, NULL, 0, access, NULL, &source, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* SourceList key exists, no source type */
    r = MsiSourceListSetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERMANAGED, MSICODE_PRODUCT,
                              INSTALLPROPERTY_MEDIAPACKAGEPATHA, "path");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* Media key is created by MsiSourceListSetInfo */
    res = RegOpenKeyExA(source, "Media", 0, access, &media);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    CHECK_REG_STR(media, "MediaPackage", "path");

    RegDeleteValueA(media, "MediaPackage");
    RegDeleteKeyExA(media, "", access, 0);
    RegCloseKey(media);
    RegDeleteKeyExA(source, "", access, 0);
    RegCloseKey(source);
    RegDeleteKeyExA(userkey, "", access, 0);
    RegCloseKey(userkey);

    /* MSIINSTALLCONTEXT_MACHINE */

machine_tests:
    lstrcpyA(keypath, "Software\\Classes\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &prodkey, NULL);
    if (res != ERROR_SUCCESS)
    {
        skip("Product key creation failed with error code %ld\n", res);
        LocalFree(usersid);
        return;
    }

    /* user product key exists */
    r = MsiSourceListSetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, MSICODE_PRODUCT,
                              INSTALLPROPERTY_MEDIAPACKAGEPATHA, "path");
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);

    res = RegCreateKeyExA(prodkey, "SourceList", 0, NULL, 0, access, NULL, &source, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* SourceList key exists, no source type */
    r = MsiSourceListSetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, MSICODE_PRODUCT,
                              INSTALLPROPERTY_MEDIAPACKAGEPATHA, "path");
    if (r == ERROR_ACCESS_DENIED)
    {
        skip("MsiSourceListSetInfo (insufficient privileges)\n");
        goto done;
    }
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* Media key is created by MsiSourceListSetInfo */
    res = RegOpenKeyExA(source, "Media", 0, access, &media);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    CHECK_REG_STR(media, "MediaPackage", "path");

    /* szUserSid is non-NULL */
    r = MsiSourceListSetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_MACHINE, MSICODE_PRODUCT,
                              INSTALLPROPERTY_MEDIAPACKAGEPATHA, "path");
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    RegDeleteValueA(media, "MediaPackage");
    RegDeleteKeyExA(media, "", access, 0);
    RegCloseKey(media);

done:
    RegDeleteKeyExA(source, "", access, 0);
    RegCloseKey(source);
    RegDeleteKeyExA(prodkey, "", access, 0);
    RegCloseKey(prodkey);
    LocalFree(usersid);
}

static void test_MsiSourceListAddMediaDisk(void)
{
    CHAR prodcode[MAX_PATH];
    CHAR prod_squashed[MAX_PATH];
    CHAR keypath[MAX_PATH*2];
    HKEY prodkey, userkey;
    HKEY media, source;
    LPSTR usersid;
    LONG res;
    UINT r;
    REGSAM access = KEY_ALL_ACCESS;

    if (!is_process_elevated())
    {
        skip("process is limited\n");
        return;
    }

    create_test_guid(prodcode, prod_squashed);
    if (!(usersid = get_user_sid()))
    {
        skip("User SID not available -> skipping MsiSourceListAddMediaDiskA tests\n");
        return;
    }

    if (is_wow64)
        access |= KEY_WOW64_64KEY;

    /* GetLastError is not set by the function */

    /* NULL szProductCodeOrPatchCode */
    r = MsiSourceListAddMediaDiskA(NULL, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                   MSICODE_PRODUCT, 1, "label", "prompt");
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* empty szProductCodeOrPatchCode */
    r = MsiSourceListAddMediaDiskA("", usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                   MSICODE_PRODUCT, 1, "label", "prompt");
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* garbage szProductCodeOrPatchCode */
    r = MsiSourceListAddMediaDiskA("garbage", usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                   MSICODE_PRODUCT, 1, "label", "prompt");
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* guid without brackets */
    r = MsiSourceListAddMediaDiskA("51CD2AD5-0482-4C46-8DDD-0ED1022AA1AA",
                                   usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                   MSICODE_PRODUCT, 1, "label", "prompt");
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* guid with brackets */
    r = MsiSourceListAddMediaDiskA("{51CD2AD5-0482-4C46-8DDD-0ED1022AA1AA}",
                                   usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                   MSICODE_PRODUCT, 1, "label", "prompt");
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);

    /* dwOptions has MSISOURCETYPE_NETWORK */
    r = MsiSourceListAddMediaDiskA("{51CD2AD5-0482-4C46-8DDD-0ED1022AA1AA}",
                                   usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                   MSICODE_PRODUCT | MSISOURCETYPE_NETWORK,
                                   1, "label", "prompt");
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* dwOptions has MSISOURCETYPE_URL */
    r = MsiSourceListAddMediaDiskA("{51CD2AD5-0482-4C46-8DDD-0ED1022AA1AA}",
                                   usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                   MSICODE_PRODUCT | MSISOURCETYPE_URL,
                                   1, "label", "prompt");
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* dwOptions has MSISOURCETYPE_MEDIA */
    r = MsiSourceListAddMediaDiskA("{51CD2AD5-0482-4C46-8DDD-0ED1022AA1AA}",
                                   usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                   MSICODE_PRODUCT | MSISOURCETYPE_MEDIA,
                                   1, "label", "prompt");
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* MSIINSTALLCONTEXT_USERUNMANAGED */

    lstrcpyA(keypath, "Software\\Microsoft\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_CURRENT_USER, keypath, &userkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* user product key exists */
    r = MsiSourceListAddMediaDiskA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                   MSICODE_PRODUCT, 1, "label", "prompt");
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);

    res = RegCreateKeyA(userkey, "SourceList", &source);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* SourceList key exists */
    r = MsiSourceListAddMediaDiskA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                   MSICODE_PRODUCT, 1, "label", "prompt");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* Media subkey is created by MsiSourceListAddMediaDisk */
    res = RegOpenKeyA(source, "Media", &media);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    CHECK_REG_STR(media, "1", "label;prompt");

    /* dwDiskId is random */
    r = MsiSourceListAddMediaDiskA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                   MSICODE_PRODUCT, 42, "label42", "prompt42");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    CHECK_REG_STR(media, "1", "label;prompt");
    CHECK_REG_STR(media, "42", "label42;prompt42");

    /* dwDiskId is 0 */
    r = MsiSourceListAddMediaDiskA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                   MSICODE_PRODUCT, 0, "label0", "prompt0");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    CHECK_REG_STR(media, "0", "label0;prompt0");
    CHECK_REG_STR(media, "1", "label;prompt");
    CHECK_REG_STR(media, "42", "label42;prompt42");

    /* dwDiskId is < 0 */
    r = MsiSourceListAddMediaDiskA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                   MSICODE_PRODUCT, -1, "label-1", "prompt-1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    CHECK_REG_STR(media, "-1", "label-1;prompt-1");
    CHECK_REG_STR(media, "0", "label0;prompt0");
    CHECK_REG_STR(media, "1", "label;prompt");
    CHECK_REG_STR(media, "42", "label42;prompt42");

    /* update dwDiskId 1 */
    r = MsiSourceListAddMediaDiskA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                   MSICODE_PRODUCT, 1, "newlabel", "newprompt");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    CHECK_REG_STR(media, "-1", "label-1;prompt-1");
    CHECK_REG_STR(media, "0", "label0;prompt0");
    CHECK_REG_STR(media, "1", "newlabel;newprompt");
    CHECK_REG_STR(media, "42", "label42;prompt42");

    /* update dwDiskId 1, szPrompt is NULL */
    r = MsiSourceListAddMediaDiskA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                   MSICODE_PRODUCT, 1, "etiqueta", NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    CHECK_REG_STR(media, "-1", "label-1;prompt-1");
    CHECK_REG_STR(media, "0", "label0;prompt0");
    CHECK_REG_STR(media, "1", "etiqueta;");
    CHECK_REG_STR(media, "42", "label42;prompt42");

    /* update dwDiskId 1, szPrompt is empty */
    r = MsiSourceListAddMediaDiskA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                   MSICODE_PRODUCT, 1, "etikett", "");
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* update dwDiskId 1, szVolumeLabel is NULL */
    r = MsiSourceListAddMediaDiskA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                   MSICODE_PRODUCT, 1, NULL, "provocar");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    CHECK_REG_STR(media, "-1", "label-1;prompt-1");
    CHECK_REG_STR(media, "0", "label0;prompt0");
    CHECK_REG_STR(media, "1", ";provocar");
    CHECK_REG_STR(media, "42", "label42;prompt42");

    /* update dwDiskId 1, szVolumeLabel is empty */
    r = MsiSourceListAddMediaDiskA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                   MSICODE_PRODUCT, 1, "", "provoquer");
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* szUserSid is NULL */
    r = MsiSourceListAddMediaDiskA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                                   MSICODE_PRODUCT, 1, NULL, "provoquer");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    CHECK_REG_STR(media, "-1", "label-1;prompt-1");
    CHECK_REG_STR(media, "0", "label0;prompt0");
    CHECK_REG_STR(media, "1", ";provoquer");
    CHECK_REG_STR(media, "42", "label42;prompt42");

    RegDeleteValueA(media, "-1");
    RegDeleteValueA(media, "0");
    RegDeleteValueA(media, "1");
    RegDeleteValueA(media, "42");
    RegDeleteKeyA(media, "");
    RegCloseKey(media);
    RegDeleteKeyA(source, "");
    RegCloseKey(source);
    RegDeleteKeyA(userkey, "");
    RegCloseKey(userkey);

    /* MSIINSTALLCONTEXT_USERMANAGED */

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Managed\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &userkey, NULL);
    if (res != ERROR_SUCCESS)
    {
        skip("Product key creation failed with error code %ld\n", res);
        goto machine_tests;
    }

    /* user product key exists */
    r = MsiSourceListAddMediaDiskA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED,
                                   MSICODE_PRODUCT, 1, "label", "prompt");
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);

    res = RegCreateKeyExA(userkey, "SourceList", 0, NULL, 0, access, NULL, &source, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* SourceList key exists */
    r = MsiSourceListAddMediaDiskA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED,
                                   MSICODE_PRODUCT, 1, "label", "prompt");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* Media subkey is created by MsiSourceListAddMediaDisk */
    res = RegOpenKeyExA(source, "Media", 0, access, &media);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    CHECK_REG_STR(media, "1", "label;prompt");

    RegDeleteValueA(media, "1");
    RegDeleteKeyExA(media, "", access, 0);
    RegCloseKey(media);
    RegDeleteKeyExA(source, "", access, 0);
    RegCloseKey(source);
    RegDeleteKeyExA(userkey, "", access, 0);
    RegCloseKey(userkey);

    /* MSIINSTALLCONTEXT_MACHINE */

machine_tests:
    lstrcpyA(keypath, "Software\\Classes\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &prodkey, NULL);
    if (res != ERROR_SUCCESS)
    {
        skip("Product key creation failed with error code %ld\n", res);
        LocalFree(usersid);
        return;
    }

    /* machine product key exists */
    r = MsiSourceListAddMediaDiskA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE,
                                   MSICODE_PRODUCT, 1, "label", "prompt");
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);

    res = RegCreateKeyExA(prodkey, "SourceList", 0, NULL, 0, access, NULL, &source, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* SourceList key exists */
    r = MsiSourceListAddMediaDiskA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE,
                                   MSICODE_PRODUCT, 1, "label", "prompt");
    if (r == ERROR_ACCESS_DENIED)
    {
        skip("MsiSourceListAddMediaDisk (insufficient privileges)\n");
        goto done;
    }
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* Media subkey is created by MsiSourceListAddMediaDisk */
    res = RegOpenKeyExA(source, "Media", 0, access, &media);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    CHECK_REG_STR(media, "1", "label;prompt");

    /* szUserSid is non-NULL */
    r = MsiSourceListAddMediaDiskA(prodcode, usersid, MSIINSTALLCONTEXT_MACHINE,
                                   MSICODE_PRODUCT, 1, "label", "prompt");
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    RegDeleteValueA(media, "1");
    RegDeleteKeyExA(media, "", access, 0);
    RegCloseKey(media);

done:
    RegDeleteKeyExA(source, "", access, 0);
    RegCloseKey(source);
    RegDeleteKeyExA(prodkey, "", access, 0);
    RegCloseKey(prodkey);
    LocalFree(usersid);
}

static void test_MsiSourceListEnumMediaDisks(void)
{
    CHAR prodcode[MAX_PATH];
    CHAR prod_squashed[MAX_PATH];
    CHAR keypath[MAX_PATH*2];
    CHAR label[MAX_PATH];
    CHAR prompt[MAX_PATH];
    HKEY prodkey, userkey, media, source;
    DWORD labelsz, promptsz, val, id;
    LPSTR usersid;
    LONG res;
    UINT r;
    REGSAM access = KEY_ALL_ACCESS;

    create_test_guid(prodcode, prod_squashed);
    if (!(usersid = get_user_sid()))
    {
        skip("User SID not available -> skipping MsiSourceListEnumMediaDisksA tests\n");
        return;
    }

    if (is_wow64)
        access |= KEY_WOW64_64KEY;

    /* GetLastError is not set by the function */

    /* NULL szProductCodeOrPatchCode */
    labelsz = sizeof(label);
    promptsz = sizeof(prompt);
    r = MsiSourceListEnumMediaDisksA(NULL, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                     MSICODE_PRODUCT, 0, &id, label, &labelsz, prompt, &promptsz);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* empty szProductCodeOrPatchCode */
    labelsz = sizeof(label);
    promptsz = sizeof(prompt);
    r = MsiSourceListEnumMediaDisksA("", usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                     MSICODE_PRODUCT, 0, &id, label, &labelsz, prompt, &promptsz);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* garbage szProductCodeOrPatchCode */
    labelsz = sizeof(label);
    promptsz = sizeof(prompt);
    r = MsiSourceListEnumMediaDisksA("garbage", usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                     MSICODE_PRODUCT, 0, &id, label, &labelsz, prompt, &promptsz);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* guid without brackets */
    labelsz = sizeof(label);
    promptsz = sizeof(prompt);
    r = MsiSourceListEnumMediaDisksA("51CD2AD5-0482-4C46-8DDD-0ED1022AA1AA",
                                     usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                     MSICODE_PRODUCT, 0, &id, label, &labelsz, prompt, &promptsz);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* guid with brackets */
    labelsz = sizeof(label);
    promptsz = sizeof(prompt);
    r = MsiSourceListEnumMediaDisksA("{51CD2AD5-0482-4C46-8DDD-0ED1022AA1AA}",
                                     usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                     MSICODE_PRODUCT, 0, &id, label, &labelsz, prompt, &promptsz);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);

    /* dwOptions has MSISOURCETYPE_NETWORK */
    labelsz = sizeof(label);
    promptsz = sizeof(prompt);
    r = MsiSourceListEnumMediaDisksA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                     MSICODE_PRODUCT | MSISOURCETYPE_NETWORK,
                                     0, &id, label, &labelsz, prompt, &promptsz);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* dwOptions has MSISOURCETYPE_URL */
    labelsz = sizeof(label);
    promptsz = sizeof(prompt);
    r = MsiSourceListEnumMediaDisksA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                     MSICODE_PRODUCT | MSISOURCETYPE_URL,
                                     0, &id, label, &labelsz, prompt, &promptsz);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* dwIndex is non-zero */
    labelsz = sizeof(label);
    promptsz = sizeof(prompt);
    r = MsiSourceListEnumMediaDisksA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                     MSICODE_PRODUCT, 1, &id, label, &labelsz, prompt, &promptsz);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* MSIINSTALLCONTEXT_USERUNMANAGED */

    lstrcpyA(keypath, "Software\\Microsoft\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_CURRENT_USER, keypath, &userkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* user product key exists */
    labelsz = sizeof(label);
    promptsz = sizeof(prompt);
    r = MsiSourceListEnumMediaDisksA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                     MSICODE_PRODUCT, 0, &id, label, &labelsz, prompt, &promptsz);
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);

    res = RegCreateKeyA(userkey, "SourceList", &source);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* SourceList key exists */
    id = 0xbeef;
    lstrcpyA(label, "aaa");
    labelsz = 0xdeadbeef;
    lstrcpyA(prompt, "bbb");
    promptsz = 0xdeadbeef;
    r = MsiSourceListEnumMediaDisksA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                     MSICODE_PRODUCT, 0, &id, label, &labelsz, prompt, &promptsz);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(id == 0xbeef, "Expected 0xbeef, got %lu\n", id);
    ok(!lstrcmpA(label, "aaa"), "Expected \"aaa\", got \"%s\"\n", label);
    ok(labelsz == 0xdeadbeef, "Expected 0xdeadbeef, got %lu\n", labelsz);
    ok(!lstrcmpA(prompt, "bbb"), "Expected \"bbb\", got \"%s\"\n", prompt);
    ok(promptsz == 0xdeadbeef, "Expected 0xdeadbeef, got %lu\n", promptsz);

    res = RegCreateKeyA(source, "Media", &media);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Media key exists */
    id = 0xbeef;
    lstrcpyA(label, "aaa");
    labelsz = 0xdeadbeef;
    lstrcpyA(prompt, "bbb");
    promptsz = 0xdeadbeef;
    r = MsiSourceListEnumMediaDisksA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                     MSICODE_PRODUCT, 0, &id, label, &labelsz, prompt, &promptsz);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(id == 0xbeef, "Expected 0xbeef, got %lu\n", id);
    ok(!lstrcmpA(label, "aaa"), "Expected \"aaa\", got \"%s\"\n", label);
    ok(labelsz == 0xdeadbeef, "Expected 0xdeadbeef, got %lu\n", labelsz);
    ok(!lstrcmpA(prompt, "bbb"), "Expected \"bbb\", got \"%s\"\n", prompt);
    ok(promptsz == 0xdeadbeef, "Expected 0xdeadbeef, got %lu\n", promptsz);

    res = RegSetValueExA(media, "1", 0, REG_SZ, (LPBYTE)"label;prompt", 13);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* disk exists */
    id = 0;
    lstrcpyA(label, "aaa");
    labelsz = MAX_PATH;
    lstrcpyA(prompt, "bbb");
    promptsz = MAX_PATH;
    r = MsiSourceListEnumMediaDisksA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                     MSICODE_PRODUCT, 0, &id, label, &labelsz, prompt, &promptsz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(id == 1, "Expected 1, got %lu\n", id);
    ok(!lstrcmpA(label, "label"), "Expected \"label\", got \"%s\"\n", label);
    ok(labelsz == 5, "Expected 5, got %lu\n", labelsz);
    ok(!lstrcmpA(prompt, "prompt"), "Expected \"prompt\", got \"%s\"\n", prompt);
    ok(promptsz == 6, "Expected 6, got %lu\n", promptsz);

    res = RegSetValueExA(media, "2", 0, REG_SZ, (LPBYTE)"one;two", 8);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* now disk 2 exists, get the sizes */
    id = 0;
    labelsz = MAX_PATH;
    promptsz = MAX_PATH;
    r = MsiSourceListEnumMediaDisksA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                     MSICODE_PRODUCT, 1, &id, NULL, &labelsz, NULL, &promptsz);
    ok(r == ERROR_SUCCESS || r == ERROR_INVALID_PARAMETER,
      "Expected ERROR_SUCCESS or ERROR_INVALID_PARAMETER, got %d\n", r);
    if (r == ERROR_SUCCESS)
    {
        ok(id == 2, "Expected 2, got %lu\n", id);
        ok(labelsz == 3, "Expected 3, got %lu\n", labelsz);
        ok(promptsz == 3, "Expected 3, got %lu\n", promptsz);
    }

    /* now fill in the values */
    id = 0xbeef;
    lstrcpyA(label, "aaa");
    labelsz = MAX_PATH;
    lstrcpyA(prompt, "bbb");
    promptsz = MAX_PATH;
    r = MsiSourceListEnumMediaDisksA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                     MSICODE_PRODUCT, 1, &id, label, &labelsz, prompt, &promptsz);
    ok(r == ERROR_SUCCESS || r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_SUCCESS or ERROR_INVALID_PARAMETER, got %d\n", r);
    if (r == ERROR_SUCCESS)
    {
        ok(id == 2, "Expected 2, got %lu\n", id);
        ok(!lstrcmpA(label, "one"), "Expected \"one\", got \"%s\"\n", label);
        ok(labelsz == 3, "Expected 3, got %lu\n", labelsz);
        ok(!lstrcmpA(prompt, "two"), "Expected \"two\", got \"%s\"\n", prompt);
        ok(promptsz == 3, "Expected 3, got %lu\n", promptsz);
    }
    else if (r == ERROR_INVALID_PARAMETER)
    {
        ok(id == 0xbeef, "Expected 0xbeef, got %lu\n", id);
        ok(!lstrcmpA(label, "aaa"), "Expected \"aaa\", got \"%s\"\n", label);
        ok(labelsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", labelsz);
        ok(!lstrcmpA(prompt, "bbb"), "Expected \"bbb\", got \"%s\"\n", prompt);
        ok(promptsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", promptsz);
    }

    res = RegSetValueExA(media, "4", 0, REG_SZ, (LPBYTE)"three;four", 11);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* disks 1, 2, 4 exist, reset the enumeration */
    id = 0;
    lstrcpyA(label, "aaa");
    labelsz = MAX_PATH;
    lstrcpyA(prompt, "bbb");
    promptsz = MAX_PATH;
    r = MsiSourceListEnumMediaDisksA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                     MSICODE_PRODUCT, 0, &id, label, &labelsz, prompt, &promptsz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(id == 1, "Expected 1, got %lu\n", id);
    ok(!lstrcmpA(label, "label"), "Expected \"label\", got \"%s\"\n", label);
    ok(labelsz == 5, "Expected 5, got %lu\n", labelsz);
    ok(!lstrcmpA(prompt, "prompt"), "Expected \"prompt\", got \"%s\"\n", prompt);
    ok(promptsz == 6, "Expected 6, got %lu\n", promptsz);

    /* disks 1, 2, 4 exist, index 1 */
    id = 0;
    lstrcpyA(label, "aaa");
    labelsz = MAX_PATH;
    lstrcpyA(prompt, "bbb");
    promptsz = MAX_PATH;
    r = MsiSourceListEnumMediaDisksA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                     MSICODE_PRODUCT, 1, &id, label, &labelsz, prompt, &promptsz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(id == 2, "Expected 2, got %lu\n", id);
    ok(!lstrcmpA(label, "one"), "Expected \"one\", got \"%s\"\n", label);
    ok(labelsz == 3, "Expected 3, got %lu\n", labelsz);
    ok(!lstrcmpA(prompt, "two"), "Expected \"two\", got \"%s\"\n", prompt);
    ok(promptsz == 3, "Expected 3, got %lu\n", promptsz);

    /* disks 1, 2, 4 exist, index 2 */
    id = 0;
    lstrcpyA(label, "aaa");
    labelsz = MAX_PATH;
    lstrcpyA(prompt, "bbb");
    promptsz = MAX_PATH;
    r = MsiSourceListEnumMediaDisksA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                     MSICODE_PRODUCT, 2, &id, label, &labelsz, prompt, &promptsz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(id == 4, "Expected 4, got %lu\n", id);
    ok(!lstrcmpA(label, "three"), "Expected \"three\", got \"%s\"\n", label);
    ok(labelsz == 5, "Expected 5, got %lu\n", labelsz);
    ok(!lstrcmpA(prompt, "four"), "Expected \"four\", got \"%s\"\n", prompt);
    ok(promptsz == 4, "Expected 4, got %lu\n", promptsz);

    /* disks 1, 2, 4 exist, index 3, invalid */
    id = 0xbeef;
    lstrcpyA(label, "aaa");
    labelsz = MAX_PATH;
    lstrcpyA(prompt, "bbb");
    promptsz = MAX_PATH;
    r = MsiSourceListEnumMediaDisksA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                     MSICODE_PRODUCT, 3, &id, label, &labelsz, prompt, &promptsz);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(id == 0xbeef, "Expected 0xbeef, got %lu\n", id);
    ok(!lstrcmpA(label, "aaa"), "Expected \"aaa\", got \"%s\"\n", label);
    ok(labelsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", labelsz);
    ok(!lstrcmpA(prompt, "bbb"), "Expected \"bbb\", got \"%s\"\n", prompt);
    ok(promptsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", promptsz);

    /* disks 1, 2, 4 exist, reset the enumeration */
    id = 0;
    lstrcpyA(label, "aaa");
    labelsz = MAX_PATH;
    lstrcpyA(prompt, "bbb");
    promptsz = MAX_PATH;
    r = MsiSourceListEnumMediaDisksA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                     MSICODE_PRODUCT, 0, &id, label, &labelsz, prompt, &promptsz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(id == 1, "Expected 1, got %lu\n", id);
    ok(!lstrcmpA(label, "label"), "Expected \"label\", got \"%s\"\n", label);
    ok(labelsz == 5, "Expected 5, got %lu\n", labelsz);
    ok(!lstrcmpA(prompt, "prompt"), "Expected \"prompt\", got \"%s\"\n", prompt);
    ok(promptsz == 6, "Expected 6, got %lu\n", promptsz);

    /* try index 0 again */
    id = 0;
    lstrcpyA(label, "aaa");
    labelsz = MAX_PATH;
    lstrcpyA(prompt, "bbb");
    promptsz = MAX_PATH;
    r = MsiSourceListEnumMediaDisksA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                     MSICODE_PRODUCT, 0, &id, label, &labelsz, prompt, &promptsz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(id == 1, "Expected 1, got %lu\n", id);
    ok(!lstrcmpA(label, "label"), "Expected \"label\", got \"%s\"\n", label);
    ok(labelsz == 5, "Expected 5, got %lu\n", labelsz);
    ok(!lstrcmpA(prompt, "prompt"), "Expected \"prompt\", got \"%s\"\n", prompt);
    ok(promptsz == 6, "Expected 6, got %lu\n", promptsz);

    /* jump to index 2 */
    id = 0xbeef;
    lstrcpyA(label, "aaa");
    labelsz = MAX_PATH;
    lstrcpyA(prompt, "bbb");
    promptsz = MAX_PATH;
    r = MsiSourceListEnumMediaDisksA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                     MSICODE_PRODUCT, 2, &id, label, &labelsz, prompt, &promptsz);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(id == 0xbeef, "Expected 0xbeef, got %lu\n", id);
    ok(!lstrcmpA(label, "aaa"), "Expected \"aaa\", got \"%s\"\n", label);
    ok(labelsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", labelsz);
    ok(!lstrcmpA(prompt, "bbb"), "Expected \"bbb\", got \"%s\"\n", prompt);
    ok(promptsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", promptsz);

    /* after error, try index 1 */
    id = 0xbeef;
    lstrcpyA(label, "aaa");
    labelsz = MAX_PATH;
    lstrcpyA(prompt, "bbb");
    promptsz = MAX_PATH;
    r = MsiSourceListEnumMediaDisksA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                     MSICODE_PRODUCT, 1, &id, label, &labelsz, prompt, &promptsz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(id == 2, "Expected 2, got %lu\n", id);
    ok(!lstrcmpA(label, "one"), "Expected \"one\", got \"%s\"\n", label);
    ok(labelsz == 3, "Expected 3, got %lu\n", labelsz);
    ok(!lstrcmpA(prompt, "two"), "Expected \"two\", got \"%s\"\n", prompt);
    ok(promptsz == 3, "Expected 3, got %lu\n", promptsz);

    /* try index 1 again */
    id = 0xbeef;
    lstrcpyA(label, "aaa");
    labelsz = MAX_PATH;
    lstrcpyA(prompt, "bbb");
    promptsz = MAX_PATH;
    r = MsiSourceListEnumMediaDisksA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                     MSICODE_PRODUCT, 1, &id, label, &labelsz, prompt, &promptsz);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(id == 0xbeef, "Expected 0xbeef, got %lu\n", id);
    ok(!lstrcmpA(label, "aaa"), "Expected \"aaa\", got \"%s\"\n", label);
    ok(labelsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", labelsz);
    ok(!lstrcmpA(prompt, "bbb"), "Expected \"bbb\", got \"%s\"\n", prompt);
    ok(promptsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", promptsz);

    /* NULL pdwDiskId */
    lstrcpyA(label, "aaa");
    labelsz = MAX_PATH;
    lstrcpyA(prompt, "bbb");
    promptsz = MAX_PATH;
    r = MsiSourceListEnumMediaDisksA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                     MSICODE_PRODUCT, 0, NULL, label, &labelsz, prompt, &promptsz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(label, "label"), "Expected \"label\", got \"%s\"\n", label);
    ok(labelsz == 5, "Expected 5, got %lu\n", labelsz);
    ok(!lstrcmpA(prompt, "prompt"), "Expected \"prompt\", got \"%s\"\n", prompt);
    ok(promptsz == 6, "Expected 6, got %lu\n", promptsz);

    /* szVolumeLabel is NULL */
    id = 0;
    labelsz = MAX_PATH;
    lstrcpyA(prompt, "bbb");
    promptsz = MAX_PATH;
    r = MsiSourceListEnumMediaDisksA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                     MSICODE_PRODUCT, 0, &id, NULL, &labelsz, prompt, &promptsz);
    ok(r == ERROR_SUCCESS || r == ERROR_INVALID_PARAMETER,
      "Expected ERROR_SUCCESS or ERROR_INVALID_PARAMETER, got %d\n", r);
    if (r == ERROR_SUCCESS)
    {
        ok(id == 1, "Expected 1, got %lu\n", id);
        ok(labelsz == 5, "Expected 5, got %lu\n", labelsz);
        ok(!lstrcmpA(prompt, "prompt"), "Expected \"prompt\", got \"%s\"\n", prompt);
        ok(promptsz == 6, "Expected 6, got %lu\n", promptsz);
    }

    /* szVolumeLabel and pcchVolumeLabel are NULL */
    id = 0;
    lstrcpyA(prompt, "bbb");
    promptsz = MAX_PATH;
    r = MsiSourceListEnumMediaDisksA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                     MSICODE_PRODUCT, 0, &id, NULL, NULL, prompt, &promptsz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(id == 1, "Expected 1, got %lu\n", id);
    ok(!lstrcmpA(prompt, "prompt"), "Expected \"prompt\", got \"%s\"\n", prompt);
    ok(promptsz == 6, "Expected 6, got %lu\n", promptsz);

    /* szVolumeLabel is non-NULL while pcchVolumeLabel is NULL */
    id = 0xbeef;
    lstrcpyA(label, "aaa");
    lstrcpyA(prompt, "bbb");
    promptsz = MAX_PATH;
    r = MsiSourceListEnumMediaDisksA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                     MSICODE_PRODUCT, 0, &id, label, NULL, prompt, &promptsz);
    ok(r == ERROR_SUCCESS || r == ERROR_INVALID_PARAMETER,
      "Expected ERROR_SUCCESS or ERROR_INVALID_PARAMETER, got %d\n", r);
    if (r == ERROR_SUCCESS)
    {
        ok(id == 1, "Expected 1, got %lu\n", id);
        ok(!lstrcmpA(label, "aaa"), "Expected \"aaa\", got \"%s\"\n", label);
        ok(!lstrcmpA(prompt, "prompt"), "Expected \"prompt\", got \"%s\"\n", prompt);
        ok(promptsz == 6, "Expected 6, got %lu\n", promptsz);
    }

    /* szDiskPrompt is NULL */
    id = 0;
    lstrcpyA(label, "aaa");
    labelsz = MAX_PATH;
    promptsz = MAX_PATH;
    r = MsiSourceListEnumMediaDisksA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                     MSICODE_PRODUCT, 0, &id, label, &labelsz, NULL, &promptsz);
    ok(r == ERROR_SUCCESS || r == ERROR_INVALID_PARAMETER, "Expected ERROR_SUCCESS, got %d\n", r);
    if (r == ERROR_SUCCESS)
    {
        ok(id == 1, "Expected 1, got %lu\n", id);
        ok(!lstrcmpA(label, "label"), "Expected \"label\", got \"%s\"\n", label);
        ok(labelsz == 5, "Expected 5, got %lu\n", labelsz);
        ok(promptsz == 6, "Expected 6, got %lu\n", promptsz);
    }

    /* szDiskPrompt and pcchDiskPrompt are NULL */
    id = 0;
    lstrcpyA(label, "aaa");
    labelsz = MAX_PATH;
    r = MsiSourceListEnumMediaDisksA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                     MSICODE_PRODUCT, 0, &id, label, &labelsz, NULL, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(id == 1, "Expected 1, got %lu\n", id);
    ok(!lstrcmpA(label, "label"), "Expected \"label\", got \"%s\"\n", label);
    ok(labelsz == 5, "Expected 5, got %lu\n", labelsz);

    /* szDiskPrompt is non-NULL while pcchDiskPrompt is NULL */
    id = 0xbeef;
    lstrcpyA(label, "aaa");
    labelsz = MAX_PATH;
    lstrcpyA(prompt, "bbb");
    r = MsiSourceListEnumMediaDisksA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                     MSICODE_PRODUCT, 0, &id, label, &labelsz, prompt, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(id == 0xbeef, "Expected 0xbeef, got %lu\n", id);
    ok(!lstrcmpA(label, "aaa"), "Expected \"aaa\", got \"%s\"\n", label);
    ok(labelsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", labelsz);
    ok(!lstrcmpA(prompt, "bbb"), "Expected \"bbb\", got \"%s\"\n", prompt);

    /* pcchVolumeLabel, szDiskPrompt and pcchDiskPrompt are NULL */
    id = 0;
    lstrcpyA(label, "aaa");
    r = MsiSourceListEnumMediaDisksA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                     MSICODE_PRODUCT, 0, &id, label, NULL, NULL, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(label, "aaa"), "Expected \"aaa\", got \"%s\"\n", label);
    ok(id == 1, "Expected 1, got %lu\n", id);

    /* szVolumeLabel, pcchVolumeLabel, szDiskPrompt and pcchDiskPrompt are NULL */
    id = 0;
    r = MsiSourceListEnumMediaDisksA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                     MSICODE_PRODUCT, 0, &id, NULL, NULL, NULL, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(id == 1, "Expected 1, got %lu\n", id);

    /* pcchVolumeLabel is exactly 5 */
    lstrcpyA(label, "aaa");
    labelsz = 5;
    lstrcpyA(prompt, "bbb");
    promptsz = MAX_PATH;
    r = MsiSourceListEnumMediaDisksA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                     MSICODE_PRODUCT, 0, NULL, label, &labelsz, prompt, &promptsz);
    ok(r == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", r);
    ok(!lstrcmpA(label, "aaa"), "Expected \"aaa\", got \"%s\"\n", label);
    ok(labelsz == 5, "Expected 5, got %lu\n", labelsz);
    ok(!lstrcmpA(prompt, "bbb"), "Expected \"bbb\", got \"%s\"\n", prompt);
    ok(promptsz == 6, "Expected 6, got %lu\n", promptsz);

    /* pcchDiskPrompt is exactly 6 */
    lstrcpyA(label, "aaa");
    labelsz = MAX_PATH;
    lstrcpyA(prompt, "bbb");
    promptsz = 6;
    r = MsiSourceListEnumMediaDisksA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                     MSICODE_PRODUCT, 0, NULL, label, &labelsz, prompt, &promptsz);
    ok(r == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", r);
    ok(!lstrcmpA(label, "aaa"), "Expected \"aaa\", got \"%s\"\n", label);
    ok(labelsz == 5, "Expected 5, got %lu\n", labelsz);
    ok(!lstrcmpA(prompt, "bbb"), "Expected \"bbb\", got \"%s\"\n", prompt);
    ok(promptsz == 6, "Expected 6, got %lu\n", promptsz);

    res = RegSetValueExA(media, "1", 0, REG_SZ, (LPBYTE)"label", 13);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* no semicolon */
    id = 0;
    lstrcpyA(label, "aaa");
    labelsz = MAX_PATH;
    lstrcpyA(prompt, "bbb");
    promptsz = MAX_PATH;
    r = MsiSourceListEnumMediaDisksA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                     MSICODE_PRODUCT, 0, &id, label, &labelsz, prompt, &promptsz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(id == 1, "Expected 1, got %lu\n", id);
    ok(!lstrcmpA(label, "label"), "Expected \"label\", got \"%s\"\n", label);
    ok(labelsz == 5, "Expected 5, got %lu\n", labelsz);
    ok(!lstrcmpA(prompt, "label"), "Expected \"label\", got \"%s\"\n", prompt);
    ok(promptsz == 5, "Expected 5, got %lu\n", promptsz);

    res = RegSetValueExA(media, "1", 0, REG_SZ, (LPBYTE)"label;", 13);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* semicolon, no disk prompt */
    id = 0;
    lstrcpyA(label, "aaa");
    labelsz = MAX_PATH;
    lstrcpyA(prompt, "bbb");
    promptsz = MAX_PATH;
    r = MsiSourceListEnumMediaDisksA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                     MSICODE_PRODUCT, 0, &id, label, &labelsz, prompt, &promptsz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(id == 1, "Expected 1, got %lu\n", id);
    ok(!lstrcmpA(label, "label"), "Expected \"label\", got \"%s\"\n", label);
    ok(labelsz == 5, "Expected 5, got %lu\n", labelsz);
    ok(!lstrcmpA(prompt, ""), "Expected \"\", got \"%s\"\n", prompt);
    ok(promptsz == 0, "Expected 0, got %lu\n", promptsz);

    res = RegSetValueExA(media, "1", 0, REG_SZ, (LPBYTE)";prompt", 13);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* semicolon, label doesn't exist */
    id = 0;
    lstrcpyA(label, "aaa");
    labelsz = MAX_PATH;
    lstrcpyA(prompt, "bbb");
    promptsz = MAX_PATH;
    r = MsiSourceListEnumMediaDisksA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                     MSICODE_PRODUCT, 0, &id, label, &labelsz, prompt, &promptsz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(id == 1, "Expected 1, got %lu\n", id);
    ok(!lstrcmpA(label, ""), "Expected \"\", got \"%s\"\n", label);
    ok(labelsz == 0, "Expected 0, got %lu\n", labelsz);
    ok(!lstrcmpA(prompt, "prompt"), "Expected \"prompt\", got \"%s\"\n", prompt);
    ok(promptsz == 6, "Expected 6, got %lu\n", promptsz);

    res = RegSetValueExA(media, "1", 0, REG_SZ, (LPBYTE)";", 13);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* semicolon, neither label nor disk prompt exist */
    id = 0;
    lstrcpyA(label, "aaa");
    labelsz = MAX_PATH;
    lstrcpyA(prompt, "bbb");
    promptsz = MAX_PATH;
    r = MsiSourceListEnumMediaDisksA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                     MSICODE_PRODUCT, 0, &id, label, &labelsz, prompt, &promptsz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(id == 1, "Expected 1, got %lu\n", id);
    ok(!lstrcmpA(label, ""), "Expected \"\", got \"%s\"\n", label);
    ok(labelsz == 0, "Expected 0, got %lu\n", labelsz);
    ok(!lstrcmpA(prompt, ""), "Expected \"\", got \"%s\"\n", prompt);
    ok(promptsz == 0, "Expected 0, got %lu\n", promptsz);

    val = 42;
    res = RegSetValueExA(media, "1", 0, REG_DWORD, (LPBYTE)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* type is REG_DWORD */
    id = 0;
    lstrcpyA(label, "aaa");
    labelsz = MAX_PATH;
    lstrcpyA(prompt, "bbb");
    promptsz = MAX_PATH;
    r = MsiSourceListEnumMediaDisksA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                     MSICODE_PRODUCT, 0, &id, label, &labelsz, prompt, &promptsz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(id == 1, "Expected 1, got %lu\n", id);
    ok(!lstrcmpA(label, "#42"), "Expected \"#42\", got \"%s\"\n", label);
    ok(labelsz == 3, "Expected 3, got %lu\n", labelsz);
    ok(!lstrcmpA(prompt, "#42"), "Expected \"#42\", got \"%s\"\n", prompt);
    ok(promptsz == 3, "Expected 3, got %lu\n", promptsz);

    RegDeleteValueA(media, "1");
    RegDeleteValueA(media, "2");
    RegDeleteValueA(media, "4");
    RegDeleteKeyA(media, "");
    RegCloseKey(media);
    RegDeleteKeyA(source, "");
    RegCloseKey(source);
    RegDeleteKeyA(userkey, "");
    RegCloseKey(userkey);

    /* MSIINSTALLCONTEXT_USERMANAGED */

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Managed\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &userkey, NULL);
    if (res != ERROR_SUCCESS)
    {
        skip("Product key creation failed with error code %ld\n", res);
        goto machine_tests;
    }

    /* user product key exists */
    r = MsiSourceListEnumMediaDisksA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED,
                                     MSICODE_PRODUCT, 0, &id, label, &labelsz, prompt, &promptsz);
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);

    res = RegCreateKeyExA(userkey, "SourceList", 0, NULL, 0, access, NULL, &source, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* SourceList key exists */
    id = 0xbeef;
    lstrcpyA(label, "aaa");
    labelsz = 0xdeadbeef;
    lstrcpyA(prompt, "bbb");
    promptsz = 0xdeadbeef;
    r = MsiSourceListEnumMediaDisksA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED,
                                     MSICODE_PRODUCT, 0, &id, label, &labelsz, prompt, &promptsz);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(id == 0xbeef, "Expected 0xbeef, got %lu\n", id);
    ok(!lstrcmpA(label, "aaa"), "Expected \"aaa\", got \"%s\"\n", label);
    ok(labelsz == 0xdeadbeef, "Expected 0xdeadbeef, got %lu\n", labelsz);
    ok(!lstrcmpA(prompt, "bbb"), "Expected \"bbb\", got \"%s\"\n", prompt);
    ok(promptsz == 0xdeadbeef, "Expected 0xdeadbeef, got %lu\n", promptsz);

    res = RegCreateKeyExA(source, "Media", 0, NULL, 0, access, NULL, &media, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Media key exists */
    id = 0xbeef;
    lstrcpyA(label, "aaa");
    labelsz = 0xdeadbeef;
    lstrcpyA(prompt, "bbb");
    promptsz = 0xdeadbeef;
    r = MsiSourceListEnumMediaDisksA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED,
                                     MSICODE_PRODUCT, 0, &id, label, &labelsz, prompt, &promptsz);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(id == 0xbeef, "Expected 0xbeef, got %lu\n", id);
    ok(!lstrcmpA(label, "aaa"), "Expected \"aaa\", got \"%s\"\n", label);
    ok(labelsz == 0xdeadbeef, "Expected 0xdeadbeef, got %lu\n", labelsz);
    ok(!lstrcmpA(prompt, "bbb"), "Expected \"bbb\", got \"%s\"\n", prompt);
    ok(promptsz == 0xdeadbeef, "Expected 0xdeadbeef, got %lu\n", promptsz);

    res = RegSetValueExA(media, "2", 0, REG_SZ, (LPBYTE)"label;prompt", 13);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* disk exists, but no id 1 */
    id = 0;
    lstrcpyA(label, "aaa");
    labelsz = MAX_PATH;
    lstrcpyA(prompt, "bbb");
    promptsz = MAX_PATH;
    r = MsiSourceListEnumMediaDisksA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED,
                                     MSICODE_PRODUCT, 0, &id, label, &labelsz, prompt, &promptsz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(id == 2, "Expected 2, got %lu\n", id);
    ok(!lstrcmpA(label, "label"), "Expected \"label\", got \"%s\"\n", label);
    ok(labelsz == 5, "Expected 5, got %lu\n", labelsz);
    ok(!lstrcmpA(prompt, "prompt"), "Expected \"prompt\", got \"%s\"\n", prompt);
    ok(promptsz == 6, "Expected 6, got %lu\n", promptsz);

    RegDeleteValueA(media, "2");
    RegDeleteKeyExA(media, "", access, 0);
    RegCloseKey(media);
    RegDeleteKeyExA(source, "", access, 0);
    RegCloseKey(source);
    RegDeleteKeyExA(userkey, "", access, 0);
    RegCloseKey(userkey);

    /* MSIINSTALLCONTEXT_MACHINE */

machine_tests:
    lstrcpyA(keypath, "Software\\Classes\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &prodkey, NULL);
    if (res != ERROR_SUCCESS)
    {
        skip("Product key creation failed with error code %ld\n", res);
        LocalFree(usersid);
        return;
    }

    /* machine product key exists */
    r = MsiSourceListEnumMediaDisksA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE,
                                     MSICODE_PRODUCT, 0, &id, label, &labelsz, prompt, &promptsz);
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);

    res = RegCreateKeyExA(prodkey, "SourceList", 0, NULL, 0, access, NULL, &source, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* SourceList key exists */
    id = 0xbeef;
    lstrcpyA(label, "aaa");
    labelsz = 0xdeadbeef;
    lstrcpyA(prompt, "bbb");
    promptsz = 0xdeadbeef;
    r = MsiSourceListEnumMediaDisksA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE,
                                     MSICODE_PRODUCT, 0, &id, label, &labelsz, prompt, &promptsz);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(id == 0xbeef, "Expected 0xbeef, got %lu\n", id);
    ok(!lstrcmpA(label, "aaa"), "Expected \"aaa\", got \"%s\"\n", label);
    ok(labelsz == 0xdeadbeef, "Expected 0xdeadbeef, got %lu\n", labelsz);
    ok(!lstrcmpA(prompt, "bbb"), "Expected \"bbb\", got \"%s\"\n", prompt);
    ok(promptsz == 0xdeadbeef, "Expected 0xdeadbeef, got %lu\n", promptsz);

    res = RegCreateKeyExA(source, "Media", 0, NULL, 0, access, NULL, &media, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Media key exists */
    id = 0xbeef;
    lstrcpyA(label, "aaa");
    labelsz = 0xdeadbeef;
    lstrcpyA(prompt, "bbb");
    promptsz = 0xdeadbeef;
    r = MsiSourceListEnumMediaDisksA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE,
                                     MSICODE_PRODUCT, 0, &id, label, &labelsz, prompt, &promptsz);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(id == 0xbeef, "Expected 0xbeef, got %lu\n", id);
    ok(!lstrcmpA(label, "aaa"), "Expected \"aaa\", got \"%s\"\n", label);
    ok(labelsz == 0xdeadbeef, "Expected 0xdeadbeef, got %lu\n", labelsz);
    ok(!lstrcmpA(prompt, "bbb"), "Expected \"bbb\", got \"%s\"\n", prompt);
    ok(promptsz == 0xdeadbeef, "Expected 0xdeadbeef, got %lu\n", promptsz);

    res = RegSetValueExA(media, "2", 0, REG_SZ, (LPBYTE)"label;prompt", 13);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* disk exists, but no id 1 */
    id = 0;
    lstrcpyA(label, "aaa");
    labelsz = MAX_PATH;
    lstrcpyA(prompt, "bbb");
    promptsz = MAX_PATH;
    r = MsiSourceListEnumMediaDisksA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE,
                                     MSICODE_PRODUCT, 0, &id, label, &labelsz, prompt, &promptsz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(id == 2, "Expected 2, got %lu\n", id);
    ok(!lstrcmpA(label, "label"), "Expected \"label\", got \"%s\"\n", label);
    ok(labelsz == 5, "Expected 5, got %lu\n", labelsz);
    ok(!lstrcmpA(prompt, "prompt"), "Expected \"prompt\", got \"%s\"\n", prompt);
    ok(promptsz == 6, "Expected 6, got %lu\n", promptsz);

    /* szUserSid is non-NULL */
    id = 0xbeef;
    lstrcpyA(label, "aaa");
    labelsz = MAX_PATH;
    lstrcpyA(prompt, "bbb");
    promptsz = MAX_PATH;
    r = MsiSourceListEnumMediaDisksA(prodcode, usersid, MSIINSTALLCONTEXT_MACHINE,
                                     MSICODE_PRODUCT, 0, &id, label, &labelsz, prompt, &promptsz);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(id == 0xbeef, "Expected 0xbeef, got %lu\n", id);
    ok(!lstrcmpA(label, "aaa"), "Expected \"aaa\", got \"%s\"\n", label);
    ok(labelsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", labelsz);
    ok(!lstrcmpA(prompt, "bbb"), "Expected \"bbb\", got \"%s\"\n", prompt);
    ok(promptsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", promptsz);

    RegDeleteValueA(media, "2");
    RegDeleteKeyExA(media, "", access, 0);
    RegCloseKey(media);
    RegDeleteKeyExA(source, "", access, 0);
    RegCloseKey(source);
    RegDeleteKeyExA(prodkey, "", access, 0);
    RegCloseKey(prodkey);
    LocalFree(usersid);
}

static void test_MsiSourceListAddSource(void)
{
    CHAR prodcode[MAX_PATH];
    CHAR prod_squashed[MAX_PATH];
    CHAR keypath[MAX_PATH*2];
    CHAR username[MAX_PATH];
    LPSTR usersid;
    LONG res;
    UINT r;
    HKEY prodkey, userkey, net, source;
    DWORD size;
    REGSAM access = KEY_ALL_ACCESS;

    if (!is_process_elevated())
    {
        skip("process is limited\n");
        return;
    }

    create_test_guid(prodcode, prod_squashed);
    if (!(usersid = get_user_sid()))
    {
        skip("User SID not available -> skipping MsiSourceListAddSourceA tests\n");
        return;
    }

    /* MACHINENAME\username */
    size = MAX_PATH;
    GetUserNameExA(NameSamCompatible, username, &size);
    trace("username: %s\n", username);

    if (is_wow64)
        access |= KEY_WOW64_64KEY;

    /* GetLastError is not set by the function */

    /* NULL szProduct */
    r = MsiSourceListAddSourceA(NULL, username, 0, "source");
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* empty szProduct */
    r = MsiSourceListAddSourceA("", username, 0, "source");
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* garbage szProduct */
    r = MsiSourceListAddSourceA("garbage", username, 0, "source");
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* guid without brackets */
    r = MsiSourceListAddSourceA("51CD2AD5-0482-4C46-8DDD-0ED1022AA1AA", username, 0, "source");
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* guid with brackets */
    r = MsiSourceListAddSourceA("{51CD2AD5-0482-4C46-8DDD-0ED1022AA1AA}", username, 0, "source");
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);

    /* dwReserved is not 0 */
    r = MsiSourceListAddSourceA(prodcode, username, 42, "source");
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* szSource is NULL */
    r = MsiSourceListAddSourceA(prodcode, username, 0, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* szSource is empty */
    r = MsiSourceListAddSourceA(prodcode, username, 0, "");
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* MSIINSTALLCONTEXT_USERMANAGED */

    r = MsiSourceListAddSourceA(prodcode, username, 0, "source");
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Managed\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &userkey, NULL);
    if (res != ERROR_SUCCESS)
    {
        skip("Product key creation failed with error code %ld\n", res);
        goto userunmanaged_tests;
    }

    /* user product key exists */
    r = MsiSourceListAddSourceA(prodcode, username, 0, "source");
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);

    res = RegCreateKeyExA(userkey, "SourceList", 0, NULL, 0, access, NULL, &source, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* SourceList key exists */
    r = MsiSourceListAddSourceA(prodcode, username, 0, "source");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* Net key is created */
    res = RegOpenKeyExA(source, "Net", 0, access, &net);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* LastUsedSource does not exist and it is not created */
    res = RegQueryValueExA(source, "LastUsedSource", 0, NULL, NULL, NULL);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", res);

    CHECK_REG_STR(net, "1", "source\\");

    RegDeleteValueA(net, "1");
    RegDeleteKeyExA(net, "", access, 0);
    RegCloseKey(net);

    res = RegSetValueExA(source, "LastUsedSource", 0, REG_SZ, (LPBYTE)"blah", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* LastUsedSource value exists */
    r = MsiSourceListAddSourceA(prodcode, username, 0, "source");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* Net key is created */
    res = RegOpenKeyExA(source, "Net", 0, access, &net);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    CHECK_REG_STR(source, "LastUsedSource", "blah");
    CHECK_REG_STR(net, "1", "source\\");

    RegDeleteValueA(net, "1");
    RegDeleteKeyExA(net, "", access, 0);
    RegCloseKey(net);

    res = RegSetValueExA(source, "LastUsedSource", 0, REG_SZ, (LPBYTE)"5", 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* LastUsedSource is an integer */
    r = MsiSourceListAddSourceA(prodcode, username, 0, "source");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* Net key is created */
    res = RegOpenKeyExA(source, "Net", 0, access, &net);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    CHECK_REG_STR(source, "LastUsedSource", "5");
    CHECK_REG_STR(net, "1", "source\\");

    /* Add a second source, has trailing backslash */
    r = MsiSourceListAddSourceA(prodcode, username, 0, "another\\");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    CHECK_REG_STR(source, "LastUsedSource", "5");
    CHECK_REG_STR(net, "1", "source\\");
    CHECK_REG_STR(net, "2", "another\\");

    res = RegSetValueExA(source, "LastUsedSource", 0, REG_SZ, (LPBYTE)"2", 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* LastUsedSource is in the source list */
    r = MsiSourceListAddSourceA(prodcode, username, 0, "third/");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    CHECK_REG_STR(source, "LastUsedSource", "2");
    CHECK_REG_STR(net, "1", "source\\");
    CHECK_REG_STR(net, "2", "another\\");
    CHECK_REG_STR(net, "3", "third/\\");

    RegDeleteValueA(net, "1");
    RegDeleteValueA(net, "2");
    RegDeleteValueA(net, "3");
    RegDeleteKeyExA(net, "", access, 0);
    RegCloseKey(net);
    RegDeleteKeyExA(source, "", access, 0);
    RegCloseKey(source);
    RegDeleteKeyExA(userkey, "", access, 0);
    RegCloseKey(userkey);

    /* MSIINSTALLCONTEXT_USERUNMANAGED */

userunmanaged_tests:
    r = MsiSourceListAddSourceA(prodcode, username, 0, "source");
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);

    lstrcpyA(keypath, "Software\\Microsoft\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_CURRENT_USER, keypath, &userkey);
    if (res != ERROR_SUCCESS)
    {
        skip("Product key creation failed with error code %ld\n", res);
        goto machine_tests;
    }

    /* user product key exists */
    r = MsiSourceListAddSourceA(prodcode, username, 0, "source");
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);

    res = RegCreateKeyA(userkey, "SourceList", &source);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* SourceList key exists */
    r = MsiSourceListAddSourceA(prodcode, username, 0, "source");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* Net key is created */
    res = RegOpenKeyA(source, "Net", &net);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    CHECK_REG_STR(net, "1", "source\\");

    RegDeleteValueA(net, "1");
    RegDeleteKeyA(net, "");
    RegCloseKey(net);
    RegDeleteKeyA(source, "");
    RegCloseKey(source);
    RegDeleteKeyA(userkey, "");
    RegCloseKey(userkey);

    /* MSIINSTALLCONTEXT_MACHINE */

machine_tests:
    r = MsiSourceListAddSourceA(prodcode, NULL, 0, "source");
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);

    lstrcpyA(keypath, "Software\\Classes\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &prodkey, NULL);
    if (res != ERROR_SUCCESS)
    {
        skip("Product key creation failed with error code %ld\n", res);
        LocalFree(usersid);
        return;
    }

    /* machine product key exists */
    r = MsiSourceListAddSourceA(prodcode, NULL, 0, "source");
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);

    res = RegCreateKeyExA(prodkey, "SourceList", 0, NULL, 0, access, NULL, &source, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* SourceList key exists */
    r = MsiSourceListAddSourceA(prodcode, NULL, 0, "source");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* Net key is created */
    res = RegOpenKeyExA(source, "Net", 0, access, &net);
    if (res == ERROR_ACCESS_DENIED)
    {
        skip("MsiSourceListAddSource (insufficient privileges)\n");
        goto done;
    }
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    CHECK_REG_STR(net, "1", "source\\");

    /* empty szUserName */
    r = MsiSourceListAddSourceA(prodcode, "", 0, "another");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    CHECK_REG_STR(net, "1", "source\\");
    CHECK_REG_STR(net, "2", "another\\");

    RegDeleteValueA(net, "2");
    RegDeleteValueA(net, "1");
    RegDeleteKeyExA(net, "", access, 0);
    RegCloseKey(net);

done:
    RegDeleteKeyExA(source, "", access, 0);
    RegCloseKey(source);
    RegDeleteKeyExA(prodkey, "", access, 0);
    RegCloseKey(prodkey);
    LocalFree(usersid);
}

START_TEST(source)
{
    if (!is_process_elevated()) restart_as_admin_elevated();

    IsWow64Process(GetCurrentProcess(), &is_wow64);

    test_MsiSourceListGetInfo();
    test_MsiSourceListAddSourceEx();
    test_MsiSourceListEnumSources();
    test_MsiSourceListSetInfo();
    test_MsiSourceListAddMediaDisk();
    test_MsiSourceListEnumMediaDisks();
    test_MsiSourceListAddSource();
}
