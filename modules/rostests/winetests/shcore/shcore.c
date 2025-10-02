/*
 * Copyright 2018 Nikolay Sivov for CodeWeavers
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

#include <windows.h>
#include "initguid.h"
#include "objidl.h"
#include "shlwapi.h"

#include "wine/test.h"

static HRESULT (WINAPI *pGetProcessReference)(IUnknown **);
static void (WINAPI *pSetProcessReference)(IUnknown *);
static HRESULT (WINAPI *pSHGetInstanceExplorer)(IUnknown **);
static int (WINAPI *pSHUnicodeToAnsi)(const WCHAR *, char *, int);
static int (WINAPI *pSHAnsiToUnicode)(const char *, WCHAR *, int);
static int (WINAPI *pSHAnsiToAnsi)(const char *, char *, int);
static int (WINAPI *pSHUnicodeToUnicode)(const WCHAR *, WCHAR *, int);
static HKEY (WINAPI *pSHRegDuplicateHKey)(HKEY);
static DWORD (WINAPI *pSHDeleteKeyA)(HKEY, const char *);
static DWORD (WINAPI *pSHGetValueA)(HKEY, const char *, const char *, DWORD *, void *, DWORD *);
static LSTATUS (WINAPI *pSHRegGetValueA)(HKEY, const char *, const char *, SRRF, DWORD *, void *, DWORD *);
static DWORD (WINAPI *pSHQueryValueExA)(HKEY, const char *, DWORD *, DWORD *, void *buff, DWORD *buff_len);
static DWORD (WINAPI *pSHRegGetPathA)(HKEY, const char *, const char *, char *, DWORD);
static DWORD (WINAPI *pSHCopyKeyA)(HKEY, const char *, HKEY, DWORD);
static HRESULT (WINAPI *pSHCreateStreamOnFileA)(const char *path, DWORD mode, IStream **stream);
static HRESULT (WINAPI *pIStream_Size)(IStream *stream, ULARGE_INTEGER *size);

/* Keys used for testing */
#define REG_TEST_KEY        "Software\\Wine\\Test"
#define REG_CURRENT_VERSION "Software\\Microsoft\\Windows\\CurrentVersion\\explorer"

static const char test_path1[] = "%LONGSYSTEMVAR%\\subdir1";
static const char test_path2[] = "%FOO%\\subdir1";

static const char * test_envvar1 = "bar";
static const char * test_envvar2 = "ImARatherLongButIndeedNeededString";
static char test_exp_path1[MAX_PATH];
static char test_exp_path2[MAX_PATH];
static DWORD exp_len1;
static DWORD exp_len2;
static const char * initial_buffer ="0123456789";

static void init(HMODULE hshcore)
{
#define X(f) p##f = (void*)GetProcAddress(hshcore, #f)
    X(GetProcessReference);
    X(SetProcessReference);
    X(SHUnicodeToAnsi);
    X(SHAnsiToUnicode);
    X(SHAnsiToAnsi);
    X(SHUnicodeToUnicode);
    X(SHRegDuplicateHKey);
    X(SHDeleteKeyA);
    X(SHGetValueA);
    X(SHRegGetValueA);
    X(SHQueryValueExA);
    X(SHRegGetPathA);
    X(SHCopyKeyA);
    X(SHCreateStreamOnFileA);
    X(IStream_Size);
#undef X
}

static HRESULT WINAPI unk_QI(IUnknown *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

struct test_unk
{
    IUnknown IUnknown_iface;
    LONG refcount;
};

static struct test_unk *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct test_unk, IUnknown_iface);
}

static ULONG WINAPI unk_AddRef(IUnknown *iface)
{
    struct test_unk *obj = impl_from_IUnknown(iface);
    return InterlockedIncrement(&obj->refcount);
}

static ULONG WINAPI unk_Release(IUnknown *iface)
{
    struct test_unk *obj = impl_from_IUnknown(iface);
    return InterlockedDecrement(&obj->refcount);
}

static const IUnknownVtbl testunkvtbl =
{
    unk_QI,
    unk_AddRef,
    unk_Release,
};

static void test_unk_init(struct test_unk *testunk)
{
    testunk->IUnknown_iface.lpVtbl = &testunkvtbl;
    testunk->refcount = 1;
}

static void test_process_reference(void)
{
    struct test_unk test_unk, test_unk2;
    IUnknown *obj;
    HMODULE hmod;
    HRESULT hr;

    obj = (void *)0xdeadbeef;
    hr = pGetProcessReference(&obj);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok(obj == NULL, "Unexpected pointer.\n");

    test_unk_init(&test_unk);
    test_unk_init(&test_unk2);

    pSetProcessReference(&test_unk.IUnknown_iface);
    ok(test_unk.refcount == 1, "Unexpected refcount %lu.\n", test_unk.refcount);
    pSetProcessReference(&test_unk2.IUnknown_iface);
    ok(test_unk.refcount == 1, "Unexpected refcount %lu.\n", test_unk.refcount);
    ok(test_unk2.refcount == 1, "Unexpected refcount %lu.\n", test_unk2.refcount);

    hr = pGetProcessReference(&obj);
    ok(hr == S_OK, "Failed to get reference, hr %#lx.\n", hr);
    ok(obj == &test_unk2.IUnknown_iface, "Unexpected pointer.\n");
    ok(test_unk2.refcount == 2, "Unexpected refcount %lu.\n", test_unk2.refcount);

    hmod = LoadLibraryA("shell32.dll");

    pSHGetInstanceExplorer = (void *)GetProcAddress(hmod, "SHGetInstanceExplorer");
    hr = pSHGetInstanceExplorer(&obj);
    ok(hr == S_OK, "Failed to get reference, hr %#lx.\n", hr);
    ok(obj == &test_unk2.IUnknown_iface, "Unexpected pointer.\n");
    ok(test_unk2.refcount == 3, "Unexpected refcount %lu.\n", test_unk2.refcount);
}

static void test_SHUnicodeToAnsi(void)
{
    char buff[16];
    int ret;

    ret = pSHUnicodeToAnsi(NULL, NULL, 0);
    ok(ret == 0, "Unexpected return value %d.\n", ret);

    strcpy(buff, "abc");
    ret = pSHUnicodeToAnsi(NULL, buff, 2);
    ok(ret == 1, "Unexpected return value %d.\n", ret);
    ok(buff[0] == 0 && buff[1] == 'b', "Unexpected buffer contents.\n");

    buff[0] = 1;
    ret = pSHUnicodeToAnsi(NULL, buff, 0);
    ok(ret == 0, "Unexpected return value %d.\n", ret);
    ok(buff[0] == 1, "Unexpected buffer contents.\n");

    buff[0] = 1;
    strcpy(buff, "test");
    ret = pSHUnicodeToAnsi(L"", buff, 1);
    ok(ret == 1, "Unexpected return value %d.\n", ret);
    ok(*buff == 0, "Unexpected buffer contents.\n");

    buff[0] = 1;
    ret = pSHUnicodeToAnsi(L"test", buff, 0);
    ok(ret == 0, "Unexpected return value %d.\n", ret);
    ok(buff[0] == 1, "Unexpected buffer contents.\n");

    buff[0] = 1;
    ret = pSHUnicodeToAnsi(L"test", buff, 1);
    ok(ret == 1, "Unexpected return value %d.\n", ret);
    ok(*buff == 0, "Unexpected buffer contents.\n");

    ret = pSHUnicodeToAnsi(L"test", buff, 16);
    ok(ret == 5, "Unexpected return value %d.\n", ret);
    ok(!strcmp(buff, "test"), "Unexpected buffer contents.\n");

    ret = pSHUnicodeToAnsi(L"test", buff, 2);
    ok(ret == 2, "Unexpected return value %d.\n", ret);
    ok(!strcmp(buff, "t"), "Unexpected buffer contents.\n");
}

static void test_SHAnsiToUnicode(void)
{
    WCHAR buffW[16];
    int ret;

    ret = pSHAnsiToUnicode(NULL, NULL, 0);
    ok(ret == 0, "Unexpected return value %d.\n", ret);

    buffW[0] = 1;
    buffW[1] = 2;
    ret = pSHAnsiToUnicode(NULL, buffW, 2);
    ok(ret == 1, "Unexpected return value %d.\n", ret);
    ok(buffW[0] == 0 && buffW[1] == 2, "Unexpected buffer contents.\n");

    buffW[0] = 1;
    ret = pSHAnsiToUnicode(NULL, buffW, 0);
    ok(ret == 0, "Unexpected return value %d.\n", ret);
    ok(buffW[0] == 1, "Unexpected buffer contents.\n");

    buffW[0] = 1;
    ret = pSHAnsiToUnicode("", buffW, 1);
    ok(ret == 1, "Unexpected return value %d.\n", ret);
    ok(*buffW == 0, "Unexpected buffer contents.\n");

    buffW[0] = 1;
    ret = pSHAnsiToUnicode("test", buffW, 0);
    ok(ret == 0, "Unexpected return value %d.\n", ret);
    ok(buffW[0] == 1, "Unexpected buffer contents.\n");

    buffW[0] = 1;
    ret = pSHAnsiToUnicode("test", buffW, 1);
    ok(ret == 1, "Unexpected return value %d.\n", ret);
    ok(*buffW == 0, "Unexpected buffer contents.\n");

    ret = pSHAnsiToUnicode("test", buffW, 16);
    ok(ret == 5, "Unexpected return value %d.\n", ret);
    ok(!lstrcmpW(buffW, L"test"), "Unexpected buffer contents.\n");

    ret = pSHAnsiToUnicode("test", buffW, 2);
    ok(ret == 2, "Unexpected return value %d.\n", ret);
    ok(buffW[0] == 't' && buffW[1] == 0, "Unexpected buffer contents.\n");
}

static void test_SHAnsiToAnsi(void)
{
    char buff[16];
    int ret;

    ret = pSHAnsiToAnsi(NULL, NULL, 0);
    ok(ret == 0, "Unexpected return value %d.\n", ret);

    strcpy(buff, "abcdefghijklm");
    ret = pSHAnsiToAnsi("test", buff, 3);
    ok(ret == 0, "Unexpected return value %d.\n", ret);
    ok(!strcmp(buff, "te"), "Unexpected buffer contents.\n");
    ok(buff[3] == 'd', "Unexpected buffer contents.\n");

    strcpy(buff, "abcdefghijklm");
    ret = pSHAnsiToAnsi("", buff, 3);
    ok(ret == 1, "Unexpected return value %d.\n", ret);
    ok(!*buff, "Unexpected buffer contents.\n");
    ok(buff[3] == 'd', "Unexpected buffer contents.\n");

    strcpy(buff, "abcdefghijklm");
    ret = pSHAnsiToAnsi("test", buff, 4);
    ok(ret == 0, "Unexpected return value %d.\n", ret);
    ok(!strcmp(buff, "tes"), "Unexpected buffer contents.\n");
    ok(buff[4] == 'e', "Unexpected buffer contents.\n");

    strcpy(buff, "abcdefghijklm");
    ret = pSHAnsiToAnsi("test", buff, 5);
    ok(ret == 5, "Unexpected return value %d.\n", ret);
    ok(!strcmp(buff, "test"), "Unexpected buffer contents.\n");
    ok(buff[5] == 'f', "Unexpected buffer contents.\n");

    strcpy(buff, "abcdefghijklm");
    ret = pSHAnsiToAnsi("test", buff, 6);
    ok(ret == 5, "Unexpected return value %d.\n", ret);
    ok(!strcmp(buff, "test"), "Unexpected buffer contents.\n");
    ok(buff[5] == 'f', "Unexpected buffer contents.\n");
}

static void test_SHUnicodeToUnicode(void)
{
    WCHAR buff[16];
    int ret;

    ret = pSHUnicodeToUnicode(NULL, NULL, 0);
    ok(ret == 0, "Unexpected return value %d.\n", ret);

    lstrcpyW(buff, L"abcdefghiklm");
    ret = pSHUnicodeToUnicode(L"test", buff, 3);
    ok(ret == 0, "Unexpected return value %d.\n", ret);
    ok(!memcmp(buff, L"test", 2 * sizeof(WCHAR)) && !buff[2], "Unexpected buffer contents.\n");
    ok(buff[3] == 'd', "Unexpected buffer contents.\n");

    lstrcpyW(buff, L"abcdefghiklm");
    ret = pSHUnicodeToUnicode(L"", buff, 3);
    ok(ret == 1, "Unexpected return value %d.\n", ret);
    ok(!*buff, "Unexpected buffer contents.\n");
    ok(buff[3] == 'd', "Unexpected buffer contents.\n");

    lstrcpyW(buff, L"abcdefghiklm");
    ret = pSHUnicodeToUnicode(L"test", buff, 4);
    ok(ret == 0, "Unexpected return value %d.\n", ret);
    ok(!memcmp(buff, L"test", 3 * sizeof(WCHAR)) && !buff[3], "Unexpected buffer contents.\n");
    ok(buff[4] == 'e', "Unexpected buffer contents.\n");

    lstrcpyW(buff, L"abcdefghiklm");
    ret = pSHUnicodeToUnicode(L"test", buff, 5);
    ok(ret == 5, "Unexpected return value %d.\n", ret);
    ok(!lstrcmpW(buff, L"test"), "Unexpected buffer contents.\n");
    ok(buff[5] == 'f', "Unexpected buffer contents.\n");

    lstrcpyW(buff, L"abcdefghiklm");
    ret = pSHUnicodeToUnicode(L"test", buff, 6);
    ok(ret == 5, "Unexpected return value %d.\n", ret);
    ok(!lstrcmpW(buff, L"test"), "Unexpected buffer contents.\n");
    ok(buff[5] == 'f', "Unexpected buffer contents.\n");
}

static void test_SHRegDuplicateHKey(void)
{
    HKEY hkey, hkey2;
    DWORD ret;

    ret = RegCreateKeyA(HKEY_CURRENT_USER, "Software\\Wine\\Test", &hkey);
    ok(!ret, "Failed to create test key, ret %ld.\n", ret);

    hkey2 = pSHRegDuplicateHKey(hkey);
    ok(hkey2 != NULL && hkey2 != hkey, "Unexpected duplicate key.\n");

    RegCloseKey(hkey2);
    RegCloseKey(hkey);

    RegDeleteKeyA(HKEY_CURRENT_USER, "Software\\Wine\\Test");
}

static void test_SHDeleteKey(void)
{
    HKEY hkey, hkey2;
    DWORD ret;

    ret = RegCreateKeyA(HKEY_CURRENT_USER, "Software\\Wine\\Test", &hkey);
    ok(!ret, "Failed to create test key, %ld.\n", ret);

    ret = RegCreateKeyA(hkey, "delete_key", &hkey2);
    ok(!ret, "Failed to create test key, %ld.\n", ret);
    RegCloseKey(hkey2);

    ret = RegDeleteKeyA(HKEY_CURRENT_USER, "Software\\Wine\\Test");
    ok(ret == ERROR_ACCESS_DENIED, "Unexpected return value %ld.\n", ret);

    ret = pSHDeleteKeyA(HKEY_CURRENT_USER, "Software\\Wine\\Test");
    ok(!ret, "Unexpected retval %lu.\n", ret);

    ret = RegCloseKey(hkey);
    ok(!ret, "Unexpected retval %lu.\n", ret);
}

static HKEY create_test_entries(void)
{
    HKEY hKey;
    DWORD ret;
    DWORD nExpectedLen1, nExpectedLen2;

    SetEnvironmentVariableA("LONGSYSTEMVAR", test_envvar1);
    SetEnvironmentVariableA("FOO", test_envvar2);

    ret = RegCreateKeyA(HKEY_CURRENT_USER, REG_TEST_KEY, &hKey);
    ok(!ret, "Unexpected retval %lu.\n", ret);

    if (hKey)
    {
        ok(!RegSetValueExA(hKey, "Test1", 0, REG_EXPAND_SZ, (BYTE *)test_path1, strlen(test_path1)+1), "RegSetValueExA failed\n");
        ok(!RegSetValueExA(hKey, "Test2", 0, REG_SZ, (BYTE *)test_path1, strlen(test_path1)+1), "RegSetValueExA failed\n");
        ok(!RegSetValueExA(hKey, "Test3", 0, REG_EXPAND_SZ, (BYTE *)test_path2, strlen(test_path2)+1), "RegSetValueExA failed\n");
    }

    exp_len1 = ExpandEnvironmentStringsA(test_path1, test_exp_path1, sizeof(test_exp_path1));
    exp_len2 = ExpandEnvironmentStringsA(test_path2, test_exp_path2, sizeof(test_exp_path2));

    nExpectedLen1 = strlen(test_path1) - strlen("%LONGSYSTEMVAR%") + strlen(test_envvar1) + 1;
    nExpectedLen2 = strlen(test_path2) - strlen("%FOO%") + strlen(test_envvar2) + 1;

    /* Make sure we carry on with correct values */
    exp_len1 = nExpectedLen1;
    exp_len2 = nExpectedLen2;

    return hKey;
}

/* delete key and all its subkeys */
static DWORD delete_key( HKEY hkey, LPCSTR parent, LPCSTR keyname )
{
    HKEY parentKey;
    DWORD ret;

    RegCloseKey(hkey);

    /* open the parent of the key to close */
    ret = RegOpenKeyExA( HKEY_CURRENT_USER, parent, 0, KEY_ALL_ACCESS, &parentKey);
    if (ret != ERROR_SUCCESS)
        return ret;

    ret = pSHDeleteKeyA( parentKey, keyname );
    RegCloseKey(parentKey);

    return ret;
}

static void test_SHGetValue(void)
{
    DWORD size;
    DWORD type;
    DWORD ret;
    char buf[MAX_PATH];

    HKEY hkey = create_test_entries();

    strcpy(buf, initial_buffer);
    size = MAX_PATH;
    type = -1;
    ret = pSHGetValueA(HKEY_CURRENT_USER, REG_TEST_KEY, "Test1", &type, buf, &size);
    ok(!ret, "Failed to get value, ret %lu.\n", ret);

    ok(!strcmp(test_exp_path1, buf), "Unexpected value %s.\n", buf);
    ok(type == REG_SZ, "Unexpected type %ld.\n", type);

    strcpy(buf, initial_buffer);
    size = MAX_PATH;
    type = -1;
    ret = pSHGetValueA(HKEY_CURRENT_USER, REG_TEST_KEY, "Test2", &type, buf, &size);
    ok(!ret, "Failed to get value, ret %lu.\n", ret);
    ok(!strcmp(test_path1, buf), "Unexpected value %s.\n", buf);
    ok(type == REG_SZ, "Unexpected type %ld.\n", type);

    delete_key(hkey, "Software\\Wine", "Test");
}

static void test_SHRegGetValue(void)
{
    LSTATUS ret;
    DWORD size, type;
    char data[MAX_PATH];

    HKEY hkey = create_test_entries();

    size = MAX_PATH;
    ret = pSHRegGetValueA(HKEY_CURRENT_USER, REG_TEST_KEY, "Test1", SRRF_RT_REG_EXPAND_SZ, &type, data, &size);
    ok(ret == ERROR_INVALID_PARAMETER, "Unexpected retval %lu.\n", ret);

    size = MAX_PATH;
    ret = pSHRegGetValueA(HKEY_CURRENT_USER, REG_TEST_KEY, "Test1", SRRF_RT_REG_SZ, &type, data, &size);
    ok(!ret, "Unexpected retval %lu.\n", ret);
    ok(!strcmp(data, test_exp_path1), "data = %s, expected %s\n", data, test_exp_path1);
    ok(type == REG_SZ, "Unexpected type %ld.\n", type);

    size = MAX_PATH;
    ret = pSHRegGetValueA(HKEY_CURRENT_USER, REG_TEST_KEY, "Test1", SRRF_RT_REG_DWORD, &type, data, &size);
    ok(ret == ERROR_UNSUPPORTED_TYPE, "Unexpected retval %lu.\n", ret);

    size = MAX_PATH;
    ret = pSHRegGetValueA(HKEY_CURRENT_USER, REG_TEST_KEY, "Test2", SRRF_RT_REG_EXPAND_SZ, &type, data, &size);
    ok(ret == ERROR_INVALID_PARAMETER, "Unexpected retval %lu.\n", ret);

    size = MAX_PATH;
    ret = pSHRegGetValueA(HKEY_CURRENT_USER, REG_TEST_KEY, "Test2", SRRF_RT_REG_SZ, &type, data, &size);
    ok(!ret, "Unexpected retval %lu.\n", ret);
    ok(!strcmp(data, test_path1), "data = %s, expected %s\n", data, test_path1);
    ok(type == REG_SZ, "Unexpected type %ld.\n", type);

    size = MAX_PATH;
    ret = pSHRegGetValueA(HKEY_CURRENT_USER, REG_TEST_KEY, "Test2", SRRF_RT_REG_QWORD, &type, data, &size);
    ok(ret == ERROR_UNSUPPORTED_TYPE, "Unexpected retval %lu.\n", ret);

    delete_key(hkey, "Software\\Wine", "Test");
}

static void test_SHQueryValueEx(void)
{
    DWORD buffer_len1,buffer_len2;
    DWORD ret, type, size;
    char buf[MAX_PATH];
    HKEY hKey, testkey;

    testkey = create_test_entries();

    ret = RegOpenKeyExA(HKEY_CURRENT_USER, REG_TEST_KEY, 0,  KEY_QUERY_VALUE, &hKey);
    ok(!ret, "Failed to open a key, ret %lu.\n", ret);

    /****** SHQueryValueExA ******/

    buffer_len1 = max(strlen(test_exp_path1)+1, strlen(test_path1)+1);
    buffer_len2 = max(strlen(test_exp_path2)+1, strlen(test_path2)+1);

    /*
     * Case 1.1 All arguments are NULL
     */
    ret = pSHQueryValueExA( hKey, "Test1", NULL, NULL, NULL, NULL);
    ok(!ret, "Failed to query value, ret %lu.\n", ret);

    /*
     * Case 1.2 dwType is set
     */
    type = -1;
    ret = pSHQueryValueExA( hKey, "Test1", NULL, &type, NULL, NULL);
    ok(!ret, "Failed to query value, ret %lu.\n", ret);
    ok(type == REG_SZ, "Unexpected type %ld.\n", type);

    size = 6;
    ret = pSHQueryValueExA( hKey, "Test1", NULL, NULL, NULL, &size);
    ok(!ret, "Failed to query value, ret %lu.\n", ret);
    ok(size == buffer_len1, "Buffer sizes %lu and %lu are not equal\n", size, buffer_len1);

    /*
     * Expanded > unexpanded
     */
    size = 6;
    ret = pSHQueryValueExA( hKey, "Test3", NULL, NULL, NULL, &size);
    ok(!ret, "Failed to query value, ret %lu.\n", ret);
    ok(size >= buffer_len2, "Buffer size %lu should be >= %lu.\n", size, buffer_len2);

    /*
     * Case 1 string shrinks during expanding
     */
    strcpy(buf, initial_buffer);
    size = 6;
    type = -1;
    ret = pSHQueryValueExA( hKey, "Test1", NULL, &type, buf, &size);
    ok(ret == ERROR_MORE_DATA, "Unexpected retval %ld.\n", ret);
    ok(!strcmp(initial_buffer, buf), "Comparing (%s) with (%s) failed\n", buf, initial_buffer);
    ok(size == buffer_len1, "Buffer sizes %lu and %lu are not equal\n", size, buffer_len1);
    ok(type == REG_SZ, "Unexpected type %ld.\n", type);

    /*
    * string grows during expanding
    * dwSize is smaller than the size of the unexpanded string
    */
    strcpy(buf, initial_buffer);
    size = 6;
    type = -1;
    ret = pSHQueryValueExA( hKey, "Test3", NULL, &type, buf, &size);
    ok(ret == ERROR_MORE_DATA, "Unexpected retval %ld.\n", ret);
    ok(!strcmp(initial_buffer, buf), "Comparing (%s) with (%s) failed\n", buf, initial_buffer);
    ok(size >= buffer_len2, "Buffer size %lu should be >= %lu.\n", size, buffer_len2);
    ok(type == REG_SZ, "Unexpected type %ld.\n", type);

    /*
    * string grows during expanding
    * dwSize is larger than the size of the unexpanded string, but
    * smaller than the part before the backslash. If the unexpanded
    * string fits into the buffer, it can get cut when expanded.
    */
    strcpy(buf, initial_buffer);
    size = strlen(test_envvar2) - 2;
    type = -1;
    ret = pSHQueryValueExA(hKey, "Test3", NULL, &type, buf, &size);
    ok(ret == ERROR_MORE_DATA, "Unexpected retval %ld.\n", ret);

    ok(!strcmp("", buf), "Unexpanded string %s.\n", buf);

    ok(size >= buffer_len2, "Buffer size %lu should be >= %lu.\n", size, buffer_len2);
    ok(type == REG_SZ, "Unexpected type %ld.\n", type);

    /*
    * string grows during expanding
    * dwSize is larger than the size of the part before the backslash,
    * but smaller than the expanded string. If the unexpanded string fits
    * into the buffer, it can get cut when expanded.
    */
    strcpy(buf, initial_buffer);
    size = exp_len2 - 4;
    type = -1;
    ret = pSHQueryValueExA( hKey, "Test3", NULL, &type, buf, &size);
    ok(ret == ERROR_MORE_DATA, "Unexpected retval %ld.\n", ret);

    ok( !strcmp("", buf) || !strcmp(test_envvar2, buf),
    "Expected empty or first part of the string \"%s\", got \"%s\"\n", test_envvar2, buf);

    ok(size >= buffer_len2, "Buffer size %lu should be >= %lu.\n", size, buffer_len2);
    ok(type == REG_SZ, "Unexpected type %ld.\n", type);

    /*
    * The buffer is NULL but the size is set
    */
    strcpy(buf, initial_buffer);
    size = 6;
    type = -1;
    ret = pSHQueryValueExA( hKey, "Test3", NULL, &type, NULL, &size);
    ok(!ret, "Failed to query value, ret %lu.\n", ret);
    ok(size >= buffer_len2, "Buffer size %lu should be >= %lu.\n", size, buffer_len2);
    ok(type == REG_SZ, "Unexpected type %ld.\n", type);

    RegCloseKey(hKey);

    delete_key(testkey, "Software\\Wine", "Test");
}

static void test_SHRegGetPath(void)
{
    char buf[MAX_PATH];
    DWORD ret;
    HKEY hkey;

    hkey = create_test_entries();

    strcpy(buf, initial_buffer);
    ret = pSHRegGetPathA(HKEY_CURRENT_USER, REG_TEST_KEY, "Test1", buf, 0);
    ok(!ret, "Failed to get path, ret %lu.\n", ret);
    ok(!strcmp(test_exp_path1, buf), "Unexpected path %s.\n", buf);

    delete_key(hkey, "Software\\Wine", "Test");
}

static void test_SHCopyKey(void)
{
    HKEY hKeySrc, hKeyDst;
    DWORD ret;

    HKEY hkey = create_test_entries();

    /* Delete existing destination sub keys */
    hKeyDst = NULL;
    if (!RegOpenKeyA(HKEY_CURRENT_USER, REG_TEST_KEY "\\CopyDestination", &hKeyDst) && hKeyDst)
    {
        pSHDeleteKeyA(hKeyDst, NULL);
        RegCloseKey(hKeyDst);
    }

    hKeyDst = NULL;
    ret = RegCreateKeyA(HKEY_CURRENT_USER, REG_TEST_KEY "\\CopyDestination", &hKeyDst);
    ok(!ret, "Failed to create a test key, ret %ld.\n", ret);

    hKeySrc = NULL;
    ret = RegOpenKeyA(HKEY_LOCAL_MACHINE, REG_CURRENT_VERSION, &hKeySrc);
    ok(!ret, "Failed to open a test key, ret %ld.\n", ret);

    ret = pSHCopyKeyA(hKeySrc, NULL, hKeyDst, 0);
    ok(!ret, "Copy failed, ret %lu.\n", ret);

    RegCloseKey(hKeySrc);
    RegCloseKey(hKeyDst);

    /* Check we copied the sub keys, i.e. something that's on every windows system (including Wine) */
    hKeyDst = NULL;
    ret = RegOpenKeyA(HKEY_CURRENT_USER, REG_TEST_KEY "\\CopyDestination\\Shell Folders", &hKeyDst);
    ok(!ret, "Failed to open a test key, ret %ld.\n", ret);

    /* And the we copied the values too */
    ok(!pSHQueryValueExA(hKeyDst, "Common AppData", NULL, NULL, NULL, NULL), "SHQueryValueExA failed\n");

    RegCloseKey(hKeyDst);
    delete_key( hkey, "Software\\Wine", "Test" );
}

#define CHECK_FILE_SIZE(filename,exp_size) _check_file_size(filename, exp_size, __LINE__)
static void _check_file_size(const CHAR *filename, LONG exp_size, int line)
{
    HANDLE handle;
    DWORD file_size = 0xdeadbeef;
    handle = CreateFileA(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0);
    file_size = GetFileSize(handle, NULL);
    ok_(__FILE__,line)(file_size == exp_size, "got wrong file size: %ld.\n", file_size);
    CloseHandle(handle);
}

#define CHECK_STREAM_SIZE(obj,exp_size) _check_stream_size(obj, exp_size, __LINE__)
static void _check_stream_size(IStream *obj, LONG exp_size, int line)
{
    ULARGE_INTEGER stream_size;
    STATSTG stat;
    HRESULT hr;
    stream_size.QuadPart = 0xdeadbeef;
    hr = pIStream_Size(obj, &stream_size);
    ok_(__FILE__,line)(hr == S_OK, "IStream_Size failed: hr %#lx.\n", hr);
    ok_(__FILE__,line)(stream_size.QuadPart == exp_size, "Size(): got wrong size of stream: %s.\n",
                       wine_dbgstr_longlong(stream_size.QuadPart));
    hr = IStream_Stat(obj, &stat, STATFLAG_NONAME);
    ok_(__FILE__,line)(hr == S_OK, "IStream_Stat failed: hr %#lx.\n", hr);
    ok_(__FILE__,line)(stat.cbSize.QuadPart == exp_size, "Stat(): got wrong size of stream: %s.\n",
                       wine_dbgstr_longlong(stat.cbSize.QuadPart));
}

#define CHECK_STREAM_POS(obj,exp_pos) _check_stream_pos(obj, exp_pos, __LINE__)
static void _check_stream_pos(IStream *obj, LONG exp_pos, int line)
{
    LARGE_INTEGER move;
    ULARGE_INTEGER pos;
    HRESULT hr;
    move.QuadPart = 0;
    pos.QuadPart = 0xdeadbeef;
    hr = IStream_Seek(obj, move, STREAM_SEEK_CUR, &pos);
    ok_(__FILE__,line)(hr == S_OK, "IStream_Seek failed: hr %#lx.\n", hr);
    ok_(__FILE__,line)(pos.QuadPart == exp_pos, "got wrong position: %s.\n",
                       wine_dbgstr_longlong(pos.QuadPart));
}

static void test_stream_size(void)
{
    static const byte test_data[] = {0x1,0x2,0x3,0x4,0x5,0x6};
    static const CHAR filename[] = "test_file";
    IStream *stream, *stream2;
    HANDLE handle;
    DWORD written = 0;
    ULARGE_INTEGER stream_size;
    HRESULT hr;

    handle = CreateFileA(filename, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(handle != INVALID_HANDLE_VALUE, "File creation failed: %lu.\n", GetLastError());
    WriteFile(handle, test_data, sizeof(test_data), &written, NULL);
    ok(written == sizeof(test_data), "Failed to write data into file.\n");
    CloseHandle(handle);

    /* in read-only mode, SetSize() will success but it has no effect on Size() and the file */
    hr = pSHCreateStreamOnFileA(filename, STGM_FAILIFTHERE|STGM_READ, &stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    CHECK_STREAM_SIZE(stream, sizeof(test_data));
    stream_size.QuadPart = 0;
    hr = IStream_SetSize(stream, stream_size);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    CHECK_STREAM_SIZE(stream, sizeof(test_data));
    CHECK_STREAM_POS(stream, 0);
    stream_size.QuadPart = 100;
    hr = IStream_SetSize(stream, stream_size);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    CHECK_STREAM_SIZE(stream, sizeof(test_data));
    CHECK_STREAM_POS(stream, 100);
    IStream_Release(stream);
    CHECK_FILE_SIZE(filename, sizeof(test_data));

    hr = pSHCreateStreamOnFileA(filename, STGM_FAILIFTHERE|STGM_WRITE, &stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = pSHCreateStreamOnFileA(filename, STGM_FAILIFTHERE|STGM_READ, &stream2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    CHECK_STREAM_SIZE(stream, sizeof(test_data));
    CHECK_STREAM_SIZE(stream2, sizeof(test_data));
    CHECK_STREAM_POS(stream, 0);
    CHECK_STREAM_POS(stream2, 0);

    stream_size.QuadPart = 0;
    hr = IStream_SetSize(stream, stream_size);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    CHECK_STREAM_SIZE(stream, 0);
    CHECK_STREAM_SIZE(stream2, 0);
    CHECK_STREAM_POS(stream, 0);
    CHECK_STREAM_POS(stream2, 0);

    stream_size.QuadPart = 100;
    hr = IStream_SetSize(stream, stream_size);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    CHECK_STREAM_SIZE(stream, 100);
    CHECK_STREAM_SIZE(stream2, 100);
    CHECK_STREAM_POS(stream, 0);
    CHECK_STREAM_POS(stream2, 0);

    stream_size.QuadPart = 90;
    hr = IStream_SetSize(stream2, stream_size);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    CHECK_STREAM_SIZE(stream, 100);
    CHECK_STREAM_SIZE(stream2, 100);
    CHECK_STREAM_POS(stream, 0);
    CHECK_STREAM_POS(stream2, 90);
    IStream_Release(stream);
    IStream_Release(stream2);
    CHECK_FILE_SIZE(filename, 100);

    DeleteFileA(filename);
}

START_TEST(shcore)
{
    HMODULE hshcore = LoadLibraryA("shcore.dll");

    if (!hshcore)
    {
        win_skip("Shcore.dll is not available.\n");
        return;
    }

    init(hshcore);

    test_process_reference();
    test_SHUnicodeToAnsi();
    test_SHAnsiToUnicode();
    test_SHAnsiToAnsi();
    test_SHUnicodeToUnicode();
    test_SHRegDuplicateHKey();
    test_SHDeleteKey();
    test_SHGetValue();
    test_SHRegGetValue();
    test_SHQueryValueEx();
    test_SHRegGetPath();
    test_SHCopyKey();
    test_stream_size();
}
