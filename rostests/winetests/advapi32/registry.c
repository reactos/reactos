/*
 * Unit tests for registry functions
 *
 * Copyright (c) 2002 Alexandre Julliard
 * Copyright (c) 2010 André Hentschel
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

#define WIN32_NO_STATUS
#define WIN32_LEAN_AND_MEAN

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winreg.h"
#include "winsvc.h"
#include "winerror.h"
#include "aclapi.h"

#define IS_HKCR(hk) ((UINT_PTR)hk > 0 && ((UINT_PTR)hk & 3) == 2)

static HKEY hkey_main;
static DWORD GLE;

static const char * sTestpath1 = "%LONGSYSTEMVAR%\\subdir1";
static const char * sTestpath2 = "%FOO%\\subdir1";
static const DWORD ptr_size = 8 * sizeof(void*);

static DWORD (WINAPI *pRegGetValueA)(HKEY,LPCSTR,LPCSTR,DWORD,LPDWORD,PVOID,LPDWORD);
static DWORD (WINAPI *pRegDeleteTreeA)(HKEY,LPCSTR);
static DWORD (WINAPI *pRegDeleteKeyExA)(HKEY,LPCSTR,REGSAM,DWORD);
static BOOL (WINAPI *pIsWow64Process)(HANDLE,PBOOL);
static NTSTATUS (WINAPI * pNtDeleteKey)(HANDLE);
static NTSTATUS (WINAPI * pRtlFormatCurrentUserKeyPath)(UNICODE_STRING*);
static NTSTATUS (WINAPI * pRtlFreeUnicodeString)(PUNICODE_STRING);
static LONG (WINAPI *pRegDeleteKeyValueA)(HKEY,LPCSTR,LPCSTR);
static LONG (WINAPI *pRegSetKeyValueW)(HKEY,LPCWSTR,LPCWSTR,DWORD,const void*,DWORD);

static BOOL limited_user;


/* Debugging functions from wine/libs/wine/debug.c */

/* allocate some tmp string space */
/* FIXME: this is not 100% thread-safe */
static char *get_temp_buffer( int size )
{
    static char *list[32];
    static UINT pos;
    char *ret;
    UINT idx;

    idx = ++pos % (sizeof(list)/sizeof(list[0]));
    if (list[idx])
        ret = HeapReAlloc( GetProcessHeap(), 0, list[idx], size );
    else
        ret = HeapAlloc( GetProcessHeap(), 0, size );
    if (ret) list[idx] = ret;
    return ret;
}

static const char *wine_debugstr_an( const char *str, int n )
{
    static const char hex[16] = "0123456789abcdef";
    char *dst, *res;
    size_t size;

    if (!((ULONG_PTR)str >> 16))
    {
        if (!str) return "(null)";
        res = get_temp_buffer( 6 );
        sprintf( res, "#%04x", LOWORD(str) );
        return res;
    }
    if (n == -1) n = strlen(str);
    if (n < 0) n = 0;
    size = 10 + min( 300, n * 4 );
    dst = res = get_temp_buffer( size );
    *dst++ = '"';
    while (n-- > 0 && dst <= res + size - 9)
    {
        unsigned char c = *str++;
        switch (c)
        {
        case '\n': *dst++ = '\\'; *dst++ = 'n'; break;
        case '\r': *dst++ = '\\'; *dst++ = 'r'; break;
        case '\t': *dst++ = '\\'; *dst++ = 't'; break;
        case '"':  *dst++ = '\\'; *dst++ = '"'; break;
        case '\\': *dst++ = '\\'; *dst++ = '\\'; break;
        default:
            if (c >= ' ' && c <= 126)
                *dst++ = c;
            else
            {
                *dst++ = '\\';
                *dst++ = 'x';
                *dst++ = hex[(c >> 4) & 0x0f];
                *dst++ = hex[c & 0x0f];
            }
        }
    }
    *dst++ = '"';
    if (n > 0)
    {
        *dst++ = '.';
        *dst++ = '.';
        *dst++ = '.';
    }
    *dst++ = 0;
    return res;
}

#define ADVAPI32_GET_PROC(func) \
    p ## func = (void*)GetProcAddress(hadvapi32, #func)

static void InitFunctionPtrs(void)
{
    HMODULE hntdll = GetModuleHandleA("ntdll.dll");
    HMODULE hkernel32 = GetModuleHandleA("kernel32.dll");
    HMODULE hadvapi32 = GetModuleHandleA("advapi32.dll");

    /* This function was introduced with Windows 2003 SP1 */
    ADVAPI32_GET_PROC(RegGetValueA);
    ADVAPI32_GET_PROC(RegDeleteTreeA);
    ADVAPI32_GET_PROC(RegDeleteKeyExA);
    ADVAPI32_GET_PROC(RegDeleteKeyValueA);
    ADVAPI32_GET_PROC(RegSetKeyValueW);

    pIsWow64Process = (void *)GetProcAddress( hkernel32, "IsWow64Process" );
    pRtlFormatCurrentUserKeyPath = (void *)GetProcAddress( hntdll, "RtlFormatCurrentUserKeyPath" );
    pRtlFreeUnicodeString = (void *)GetProcAddress(hntdll, "RtlFreeUnicodeString");
    pNtDeleteKey = (void *)GetProcAddress( hntdll, "NtDeleteKey" );
}

/* delete key and all its subkeys */
static DWORD delete_key( HKEY hkey )
{
    char name[MAX_PATH];
    DWORD ret;

    if ((ret = RegOpenKeyExA( hkey, "", 0, KEY_ENUMERATE_SUB_KEYS, &hkey ))) return ret;
    while (!(ret = RegEnumKeyA(hkey, 0, name, sizeof(name))))
    {
        HKEY tmp;
        if (!(ret = RegOpenKeyExA( hkey, name, 0, KEY_ENUMERATE_SUB_KEYS, &tmp )))
        {
            ret = delete_key( tmp );
            RegCloseKey( tmp );
        }
        if (ret) break;
    }
    if (ret != ERROR_NO_MORE_ITEMS) return ret;
    RegDeleteKeyA( hkey, "" );
    RegCloseKey(hkey);
    return 0;
}

static void setup_main_key(void)
{
    if (!RegOpenKeyA( HKEY_CURRENT_USER, "Software\\Wine\\Test", &hkey_main )) delete_key( hkey_main );

    assert (!RegCreateKeyA( HKEY_CURRENT_USER, "Software\\Wine\\Test", &hkey_main ));
}

static void check_user_privs(void)
{
    DWORD ret;
    HKEY hkey = (HKEY)0xdeadbeef;

    ret = RegOpenKeyExA( HKEY_LOCAL_MACHINE, "Software", 0, KEY_READ|KEY_WRITE, &hkey);
    ok(ret == ERROR_SUCCESS || ret == ERROR_ACCESS_DENIED, "expected success or access denied, got %i\n", ret);
    if (ret == ERROR_SUCCESS)
    {
        ok(hkey != NULL, "RegOpenKeyExA succeeded but returned NULL hkey\n");
        RegCloseKey(hkey);
    }
    else
    {
        ok(hkey == NULL, "RegOpenKeyExA failed but returned hkey %p\n", hkey);
        limited_user = TRUE;
        trace("running as limited user\n");
    }
}

#define lok ok_(__FILE__, line)
#define test_hkey_main_Value_A(name, string, full_byte_len) _test_hkey_main_Value_A(__LINE__, name, string, full_byte_len)
static void _test_hkey_main_Value_A(int line, LPCSTR name, LPCSTR string,
                                   DWORD full_byte_len)
{
    DWORD ret, type, cbData;
    DWORD str_byte_len;
    BYTE* value;

    type=0xdeadbeef;
    cbData=0xdeadbeef;
    /* When successful RegQueryValueExA() leaves GLE as is,
     * so we must reset it to detect unimplemented functions.
     */
    SetLastError(0xdeadbeef);
    ret = RegQueryValueExA(hkey_main, name, NULL, &type, NULL, &cbData);
    GLE = GetLastError();
    lok(ret == ERROR_SUCCESS, "RegQueryValueExA/1 failed: %d, GLE=%d\n", ret, GLE);
    /* It is wrong for the Ansi version to not be implemented */
    ok(GLE == 0xdeadbeef, "RegQueryValueExA set GLE = %u\n", GLE);
    if(GLE == ERROR_CALL_NOT_IMPLEMENTED) return;

    str_byte_len = (string ? lstrlenA(string) : 0) + 1;
    lok(type == REG_SZ, "RegQueryValueExA/1 returned type %d\n", type);
    lok(cbData == full_byte_len, "cbData=%d instead of %d or %d\n", cbData, full_byte_len, str_byte_len);

    value = HeapAlloc(GetProcessHeap(), 0, cbData+1);
    memset(value, 0xbd, cbData+1);
    type=0xdeadbeef;
    ret = RegQueryValueExA(hkey_main, name, NULL, &type, value, &cbData);
    GLE = GetLastError();
    lok(ret == ERROR_SUCCESS, "RegQueryValueExA/2 failed: %d, GLE=%d\n", ret, GLE);
    if (!string)
    {
        /* When cbData == 0, RegQueryValueExA() should not modify the buffer */
        lok(*value == 0xbd, "RegQueryValueExA overflowed: cbData=%u *value=%02x\n", cbData, *value);
    }
    else
    {
        lok(memcmp(value, string, cbData) == 0, "RegQueryValueExA/2 failed: %s/%d != %s/%d\n",
           wine_debugstr_an((char*)value, cbData), cbData,
           wine_debugstr_an(string, full_byte_len), full_byte_len);
        lok(*(value+cbData) == 0xbd, "RegQueryValueExA/2 overflowed at offset %u: %02x != bd\n", cbData, *(value+cbData));
    }
    HeapFree(GetProcessHeap(), 0, value);
}

#define test_hkey_main_Value_W(name, string, full_byte_len) _test_hkey_main_Value_W(__LINE__, name, string, full_byte_len)
static void _test_hkey_main_Value_W(int line, LPCWSTR name, LPCWSTR string,
                                   DWORD full_byte_len)
{
    DWORD ret, type, cbData;
    BYTE* value;

    type=0xdeadbeef;
    cbData=0xdeadbeef;
    /* When successful RegQueryValueExW() leaves GLE as is,
     * so we must reset it to detect unimplemented functions.
     */
    SetLastError(0xdeadbeef);
    ret = RegQueryValueExW(hkey_main, name, NULL, &type, NULL, &cbData);
    GLE = GetLastError();
    lok(ret == ERROR_SUCCESS, "RegQueryValueExW/1 failed: %d, GLE=%d\n", ret, GLE);
    if(GLE == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("RegQueryValueExW() is not implemented\n");
        return;
    }

    lok(type == REG_SZ, "RegQueryValueExW/1 returned type %d\n", type);
    lok(cbData == full_byte_len,
        "cbData=%d instead of %d\n", cbData, full_byte_len);

    /* Give enough space to overflow by one WCHAR */
    value = HeapAlloc(GetProcessHeap(), 0, cbData+2);
    memset(value, 0xbd, cbData+2);
    type=0xdeadbeef;
    ret = RegQueryValueExW(hkey_main, name, NULL, &type, value, &cbData);
    GLE = GetLastError();
    lok(ret == ERROR_SUCCESS, "RegQueryValueExW/2 failed: %d, GLE=%d\n", ret, GLE);
    if (string)
    {
        lok(memcmp(value, string, cbData) == 0, "RegQueryValueExW failed: %s/%d != %s/%d\n",
           wine_dbgstr_wn((WCHAR*)value, cbData / sizeof(WCHAR)), cbData,
           wine_dbgstr_wn(string, full_byte_len / sizeof(WCHAR)), full_byte_len);
    }
    /* This implies that when cbData == 0, RegQueryValueExW() should not modify the buffer */
    lok(*(value+cbData) == 0xbd, "RegQueryValueExW/2 overflowed at %u: %02x != bd\n", cbData, *(value+cbData));
    lok(*(value+cbData+1) == 0xbd, "RegQueryValueExW/2 overflowed at %u+1: %02x != bd\n", cbData, *(value+cbData+1));
    HeapFree(GetProcessHeap(), 0, value);
}

static void test_set_value(void)
{
    DWORD ret;

    static const WCHAR name1W[] =   {'C','l','e','a','n','S','i','n','g','l','e','S','t','r','i','n','g', 0};
    static const WCHAR name2W[] =   {'S','o','m','e','I','n','t','r','a','Z','e','r','o','e','d','S','t','r','i','n','g', 0};
    static const WCHAR emptyW[] = {0};
    static const WCHAR string1W[] = {'T','h','i','s','N','e','v','e','r','B','r','e','a','k','s', 0};
    static const WCHAR string2W[] = {'T','h','i','s', 0 ,'B','r','e','a','k','s', 0 , 0 ,'A', 0 , 0 , 0 , 'L','o','t', 0 , 0 , 0 , 0, 0};
    static const WCHAR substring2W[] = {'T','h','i','s',0};

    static const char name1A[] =   "CleanSingleString";
    static const char name2A[] =   "SomeIntraZeroedString";
    static const char emptyA[] = "";
    static const char string1A[] = "ThisNeverBreaks";
    static const char string2A[] = "This\0Breaks\0\0A\0\0\0Lot\0\0\0\0";
    static const char substring2A[] = "This";

    if (0)
    {
        /* Crashes on NT4, Windows 2000 and XP SP1 */
        ret = RegSetValueA(hkey_main, NULL, REG_SZ, NULL, 0);
        ok(ret == ERROR_INVALID_PARAMETER, "RegSetValueA should have failed with ERROR_INVALID_PARAMETER instead of %d\n", ret);
    }

    ret = RegSetValueA(hkey_main, NULL, REG_SZ, string1A, sizeof(string1A));
    ok(ret == ERROR_SUCCESS, "RegSetValueA failed: %d, GLE=%d\n", ret, GetLastError());
    test_hkey_main_Value_A(NULL, string1A, sizeof(string1A));
    test_hkey_main_Value_W(NULL, string1W, sizeof(string1W));

    /* RegSetValueA ignores the size passed in */
    ret = RegSetValueA(hkey_main, NULL, REG_SZ, string1A, 4);
    ok(ret == ERROR_SUCCESS, "RegSetValueA failed: %d, GLE=%d\n", ret, GetLastError());
    test_hkey_main_Value_A(NULL, string1A, sizeof(string1A));
    test_hkey_main_Value_W(NULL, string1W, sizeof(string1W));

    /* stops at first null */
    ret = RegSetValueA(hkey_main, NULL, REG_SZ, string2A, sizeof(string2A));
    ok(ret == ERROR_SUCCESS, "RegSetValueA failed: %d, GLE=%d\n", ret, GetLastError());
    test_hkey_main_Value_A(NULL, substring2A, sizeof(substring2A));
    test_hkey_main_Value_W(NULL, substring2W, sizeof(substring2W));

    /* only REG_SZ is supported on NT*/
    ret = RegSetValueA(hkey_main, NULL, REG_BINARY, string2A, sizeof(string2A));
    ok(ret == ERROR_INVALID_PARAMETER, "got %d (expected ERROR_INVALID_PARAMETER)\n", ret);

    ret = RegSetValueA(hkey_main, NULL, REG_EXPAND_SZ, string2A, sizeof(string2A));
    ok(ret == ERROR_INVALID_PARAMETER, "got %d (expected ERROR_INVALID_PARAMETER)\n", ret);

    ret = RegSetValueA(hkey_main, NULL, REG_MULTI_SZ, string2A, sizeof(string2A));
    ok(ret == ERROR_INVALID_PARAMETER, "got %d (expected ERROR_INVALID_PARAMETER)\n", ret);

    /* Test RegSetValueExA with a 'zero-byte' string (as Office 2003 does).
     * Surprisingly enough we're supposed to get zero bytes out of it.
     */
    ret = RegSetValueExA(hkey_main, name1A, 0, REG_SZ, (const BYTE *)emptyA, 0);
    ok(ret == ERROR_SUCCESS, "RegSetValueExA failed: %d, GLE=%d\n", ret, GetLastError());
    test_hkey_main_Value_A(name1A, NULL, 0);
    test_hkey_main_Value_W(name1W, NULL, 0);

    /* test RegSetValueExA with an empty string */
    ret = RegSetValueExA(hkey_main, name1A, 0, REG_SZ, (const BYTE *)emptyA, sizeof(emptyA));
    ok(ret == ERROR_SUCCESS, "RegSetValueExA failed: %d, GLE=%d\n", ret, GetLastError());
    test_hkey_main_Value_A(name1A, emptyA, sizeof(emptyA));
    test_hkey_main_Value_W(name1W, emptyW, sizeof(emptyW));

    /* test RegSetValueExA with off-by-one size */
    ret = RegSetValueExA(hkey_main, name1A, 0, REG_SZ, (const BYTE *)string1A, sizeof(string1A)-sizeof(string1A[0]));
    ok(ret == ERROR_SUCCESS, "RegSetValueExA failed: %d, GLE=%d\n", ret, GetLastError());
    test_hkey_main_Value_A(name1A, string1A, sizeof(string1A));
    test_hkey_main_Value_W(name1W, string1W, sizeof(string1W));

    /* test RegSetValueExA with normal string */
    ret = RegSetValueExA(hkey_main, name1A, 0, REG_SZ, (const BYTE *)string1A, sizeof(string1A));
    ok(ret == ERROR_SUCCESS, "RegSetValueExA failed: %d, GLE=%d\n", ret, GetLastError());
    test_hkey_main_Value_A(name1A, string1A, sizeof(string1A));
    test_hkey_main_Value_W(name1W, string1W, sizeof(string1W));

    /* test RegSetValueExA with intrazeroed string */
    ret = RegSetValueExA(hkey_main, name2A, 0, REG_SZ, (const BYTE *)string2A, sizeof(string2A));
    ok(ret == ERROR_SUCCESS, "RegSetValueExA failed: %d, GLE=%d\n", ret, GetLastError());
    test_hkey_main_Value_A(name2A, string2A, sizeof(string2A));
    test_hkey_main_Value_W(name2W, string2W, sizeof(string2W));

    if (0)
    {
        /* Crashes on NT4, Windows 2000 and XP SP1 */
        ret = RegSetValueW(hkey_main, NULL, REG_SZ, NULL, 0);
        ok(ret == ERROR_INVALID_PARAMETER, "RegSetValueW should have failed with ERROR_INVALID_PARAMETER instead of %d\n", ret);

        RegSetValueExA(hkey_main, name2A, 0, REG_SZ, (const BYTE *)1, 1);
        RegSetValueExA(hkey_main, name2A, 0, REG_DWORD, (const BYTE *)1, 1);
    }

    ret = RegSetValueW(hkey_main, NULL, REG_SZ, string1W, sizeof(string1W));
    ok(ret == ERROR_SUCCESS, "RegSetValueW failed: %d, GLE=%d\n", ret, GetLastError());
    test_hkey_main_Value_A(NULL, string1A, sizeof(string1A));
    test_hkey_main_Value_W(NULL, string1W, sizeof(string1W));

    ret = RegSetValueW(hkey_main, name1W, REG_SZ, string1W, sizeof(string1W));
    ok(ret == ERROR_SUCCESS, "RegSetValueW failed: %d, GLE=%d\n", ret, GetLastError());
    test_hkey_main_Value_A(name1A, string1A, sizeof(string1A));
    test_hkey_main_Value_W(name1W, string1W, sizeof(string1W));

    /* RegSetValueW ignores the size passed in */
    ret = RegSetValueW(hkey_main, NULL, REG_SZ, string1W, 4 * sizeof(string1W[0]));
    ok(ret == ERROR_SUCCESS, "RegSetValueW failed: %d, GLE=%d\n", ret, GetLastError());
    test_hkey_main_Value_A(NULL, string1A, sizeof(string1A));
    test_hkey_main_Value_W(NULL, string1W, sizeof(string1W));

    /* stops at first null */
    ret = RegSetValueW(hkey_main, NULL, REG_SZ, string2W, sizeof(string2W));
    ok(ret == ERROR_SUCCESS, "RegSetValueW failed: %d, GLE=%d\n", ret, GetLastError());
    test_hkey_main_Value_A(NULL, substring2A, sizeof(substring2A));
    test_hkey_main_Value_W(NULL, substring2W, sizeof(substring2W));

    /* only REG_SZ is supported */
    ret = RegSetValueW(hkey_main, NULL, REG_BINARY, string2W, sizeof(string2W));
    ok(ret == ERROR_INVALID_PARAMETER, "RegSetValueW should have returned ERROR_INVALID_PARAMETER instead of %d\n", ret);
    ret = RegSetValueW(hkey_main, NULL, REG_EXPAND_SZ, string2W, sizeof(string2W));
    ok(ret == ERROR_INVALID_PARAMETER, "RegSetValueW should have returned ERROR_INVALID_PARAMETER instead of %d\n", ret);
    ret = RegSetValueW(hkey_main, NULL, REG_MULTI_SZ, string2W, sizeof(string2W));
    ok(ret == ERROR_INVALID_PARAMETER, "RegSetValueW should have returned ERROR_INVALID_PARAMETER instead of %d\n", ret);

    /* test RegSetValueExW with off-by-one size */
    ret = RegSetValueExW(hkey_main, name1W, 0, REG_SZ, (const BYTE *)string1W, sizeof(string1W)-sizeof(string1W[0]));
    ok(ret == ERROR_SUCCESS, "RegSetValueExW failed: %d, GLE=%d\n", ret, GetLastError());
    test_hkey_main_Value_A(name1A, string1A, sizeof(string1A));
    test_hkey_main_Value_W(name1W, string1W, sizeof(string1W));

    /* test RegSetValueExW with normal string */
    ret = RegSetValueExW(hkey_main, name1W, 0, REG_SZ, (const BYTE *)string1W, sizeof(string1W));
    ok(ret == ERROR_SUCCESS, "RegSetValueExW failed: %d, GLE=%d\n", ret, GetLastError());
    test_hkey_main_Value_A(name1A, string1A, sizeof(string1A));
    test_hkey_main_Value_W(name1W, string1W, sizeof(string1W));

    /* test RegSetValueExW with intrazeroed string */
    ret = RegSetValueExW(hkey_main, name2W, 0, REG_SZ, (const BYTE *)string2W, sizeof(string2W));
    ok(ret == ERROR_SUCCESS, "RegSetValueExW failed: %d, GLE=%d\n", ret, GetLastError());
    test_hkey_main_Value_A(name2A, string2A, sizeof(string2A));
    test_hkey_main_Value_W(name2W, string2W, sizeof(string2W));

    /* test RegSetValueExW with data = 1 */
    ret = RegSetValueExW(hkey_main, name2W, 0, REG_SZ, (const BYTE *)1, 1);
    ok(ret == ERROR_NOACCESS, "RegSetValueExW should have failed with ERROR_NOACCESS: %d, GLE=%d\n", ret, GetLastError());
    ret = RegSetValueExW(hkey_main, name2W, 0, REG_DWORD, (const BYTE *)1, 1);
    ok(ret == ERROR_NOACCESS, "RegSetValueExW should have failed with ERROR_NOACCESS: %d, GLE=%d\n", ret, GetLastError());

    if (pRegGetValueA) /* avoid a crash on Windows 2000 */
    {
        ret = RegSetValueExW(hkey_main, NULL, 0, REG_SZ, NULL, 4);
        ok(ret == ERROR_NOACCESS, "RegSetValueExW should have failed with ERROR_NOACCESS: %d, GLE=%d\n", ret, GetLastError());

        ret = RegSetValueExW(hkey_main, NULL, 0, REG_SZ, NULL, 0);
        ok(ret == ERROR_SUCCESS, "got %d\n", ret);

        ret = RegSetValueExW(hkey_main, NULL, 0, REG_DWORD, NULL, 4);
        ok(ret == ERROR_NOACCESS, "RegSetValueExW should have failed with ERROR_NOACCESS: %d, GLE=%d\n", ret, GetLastError());

        ret = RegSetValueExW(hkey_main, NULL, 0, REG_DWORD, NULL, 0);
        ok(ret == ERROR_SUCCESS, "got %d\n", ret);
    }

    /* RegSetKeyValue */
    if (!pRegSetKeyValueW)
        win_skip("RegSetKeyValue() is not supported.\n");
    else
    {
        static const WCHAR subkeyW[] = {'s','u','b','k','e','y',0};
        DWORD len, type;
        HKEY subkey;

        ret = pRegSetKeyValueW(hkey_main, NULL, name1W, REG_SZ, (const BYTE*)string2W, sizeof(string2W));
        ok(ret == ERROR_SUCCESS, "got %d\n", ret);
        test_hkey_main_Value_A(name1A, string2A, sizeof(string2A));
        test_hkey_main_Value_W(name1W, string2W, sizeof(string2W));

        ret = pRegSetKeyValueW(hkey_main, subkeyW, name1W, REG_SZ, string1W, sizeof(string1W));
        ok(ret == ERROR_SUCCESS, "got %d\n", ret);

        ret = RegOpenKeyExW(hkey_main, subkeyW, 0, KEY_QUERY_VALUE, &subkey);
        ok(ret == ERROR_SUCCESS, "got %d\n", ret);
        type = len = 0;
        ret = RegQueryValueExW(subkey, name1W, 0, &type, NULL, &len);
        ok(ret == ERROR_SUCCESS, "got %d\n", ret);
        ok(len == sizeof(string1W), "got %d\n", len);
        ok(type == REG_SZ, "got type %d\n", type);

        ret = pRegSetKeyValueW(hkey_main, subkeyW, name1W, REG_SZ, NULL, 0);
        ok(ret == ERROR_SUCCESS, "got %d\n", ret);

        ret = pRegSetKeyValueW(hkey_main, subkeyW, name1W, REG_SZ, NULL, 4);
        ok(ret == ERROR_NOACCESS, "got %d\n", ret);

        ret = pRegSetKeyValueW(hkey_main, subkeyW, name1W, REG_DWORD, NULL, 4);
        ok(ret == ERROR_NOACCESS, "got %d\n", ret);

        RegCloseKey(subkey);
    }
}

static void create_test_entries(void)
{
    static const DWORD qw[2] = { 0x12345678, 0x87654321 };

    SetEnvironmentVariableA("LONGSYSTEMVAR", "bar");
    SetEnvironmentVariableA("FOO", "ImARatherLongButIndeedNeededString");

    ok(!RegSetValueExA(hkey_main,"TP1_EXP_SZ",0,REG_EXPAND_SZ, (const BYTE *)sTestpath1, strlen(sTestpath1)+1), 
        "RegSetValueExA failed\n");
    ok(!RegSetValueExA(hkey_main,"TP1_SZ",0,REG_SZ, (const BYTE *)sTestpath1, strlen(sTestpath1)+1), 
        "RegSetValueExA failed\n");
    ok(!RegSetValueExA(hkey_main,"TP1_ZB_SZ",0,REG_SZ, (const BYTE *)"", 0),
       "RegSetValueExA failed\n");
    ok(!RegSetValueExA(hkey_main,"TP2_EXP_SZ",0,REG_EXPAND_SZ, (const BYTE *)sTestpath2, strlen(sTestpath2)+1), 
        "RegSetValueExA failed\n");
    ok(!RegSetValueExA(hkey_main,"DWORD",0,REG_DWORD, (const BYTE *)qw, 4),
        "RegSetValueExA failed\n");
    ok(!RegSetValueExA(hkey_main,"BIN32",0,REG_BINARY, (const BYTE *)qw, 4),
        "RegSetValueExA failed\n");
    ok(!RegSetValueExA(hkey_main,"BIN64",0,REG_BINARY, (const BYTE *)qw, 8),
        "RegSetValueExA failed\n");
}
        
static void test_enum_value(void)
{
    DWORD res;
    HKEY test_key;
    char value[20], data[20];
    WCHAR valueW[20], dataW[20];
    DWORD val_count, data_count, type;
    static const WCHAR foobarW[] = {'f','o','o','b','a','r',0};
    static const WCHAR testW[] = {'T','e','s','t',0};
    static const WCHAR xxxW[] = {'x','x','x','x','x','x','x','x',0};

    /* create the working key for new 'Test' value */
    res = RegCreateKeyA( hkey_main, "TestKey", &test_key );
    ok( res == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %d\n", res);

    /* check NULL data with zero length */
    res = RegSetValueExA( test_key, "Test", 0, REG_SZ, NULL, 0 );
    if (GetVersion() & 0x80000000)
        ok( res == ERROR_INVALID_PARAMETER, "RegSetValueExA returned %d\n", res );
    else
        ok( !res, "RegSetValueExA returned %d\n", res );
    res = RegSetValueExA( test_key, "Test", 0, REG_EXPAND_SZ, NULL, 0 );
    ok( ERROR_SUCCESS == res || ERROR_INVALID_PARAMETER == res, "RegSetValueExA returned %d\n", res );
    res = RegSetValueExA( test_key, "Test", 0, REG_BINARY, NULL, 0 );
    ok( ERROR_SUCCESS == res || ERROR_INVALID_PARAMETER == res, "RegSetValueExA returned %d\n", res );

    /* test reading the value and data without setting them */
    val_count = 20;
    data_count = 20;
    type = 1234;
    strcpy( value, "xxxxxxxxxx" );
    strcpy( data, "xxxxxxxxxx" );
    res = RegEnumValueA( test_key, 0, value, &val_count, NULL, &type, (LPBYTE)data, &data_count );
    ok( res == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %d\n", res );
    ok( val_count == 4, "val_count set to %d instead of 4\n", val_count );
    ok( data_count == 0, "data_count set to %d instead of 0\n", data_count );
    ok( type == REG_BINARY, "type %d is not REG_BINARY\n", type );
    ok( !strcmp( value, "Test" ), "value is '%s' instead of Test\n", value );
    ok( !strcmp( data, "xxxxxxxxxx" ), "data is '%s' instead of xxxxxxxxxx\n", data );

    val_count = 20;
    data_count = 20;
    type = 1234;
    memcpy( valueW, xxxW, sizeof(xxxW) );
    memcpy( dataW, xxxW, sizeof(xxxW) );
    res = RegEnumValueW( test_key, 0, valueW, &val_count, NULL, &type, (BYTE*)dataW, &data_count );
    ok( res == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %d\n", res );
    ok( val_count == 4, "val_count set to %d instead of 4\n", val_count );
    ok( data_count == 0, "data_count set to %d instead of 0\n", data_count );
    ok( type == REG_BINARY, "type %d is not REG_BINARY\n", type );
    ok( !memcmp( valueW, testW, sizeof(testW) ), "value is not 'Test'\n" );
    ok( !memcmp( dataW, xxxW, sizeof(xxxW) ), "data is not 'xxxxxxxxxx'\n" );

    res = RegSetValueExA( test_key, "Test", 0, REG_SZ, (const BYTE *)"foobar", 7 );
    ok( res == 0, "RegSetValueExA failed error %d\n", res );

    /* overflow both name and data */
    val_count = 2;
    data_count = 2;
    type = 1234;
    strcpy( value, "xxxxxxxxxx" );
    strcpy( data, "xxxxxxxxxx" );
    res = RegEnumValueA( test_key, 0, value, &val_count, NULL, &type, (LPBYTE)data, &data_count );
    ok( res == ERROR_MORE_DATA, "expected ERROR_MORE_DATA, got %d\n", res );
    ok( val_count == 2, "val_count set to %d\n", val_count );
    ok( data_count == 7 || broken( data_count == 8 ), "data_count set to %d instead of 7\n", data_count );
    ok( type == REG_SZ, "type %d is not REG_SZ\n", type );
    ok( !strcmp( value, "xxxxxxxxxx" ), "value set to '%s'\n", value );
    ok( !strcmp( data, "xxxxxxxxxx" ), "data set to '%s'\n", data );

    /* overflow name */
    val_count = 3;
    data_count = 20;
    type = 1234;
    strcpy( value, "xxxxxxxxxx" );
    strcpy( data, "xxxxxxxxxx" );
    res = RegEnumValueA( test_key, 0, value, &val_count, NULL, &type, (LPBYTE)data, &data_count );
    ok( res == ERROR_MORE_DATA, "expected ERROR_MORE_DATA, got %d\n", res );
    ok( val_count == 3, "val_count set to %d\n", val_count );
    ok( data_count == 7 || broken( data_count == 8 ), "data_count set to %d instead of 7\n", data_count );
    ok( type == REG_SZ, "type %d is not REG_SZ\n", type );
    /* v5.1.2600.0 (XP Home and Professional) does not touch value or data in this case */
    ok( !strcmp( value, "Te" ) || !strcmp( value, "xxxxxxxxxx" ), 
        "value set to '%s' instead of 'Te' or 'xxxxxxxxxx'\n", value );
    ok( !strcmp( data, "foobar" ) || !strcmp( data, "xxxxxxx" ) || broken( !strcmp( data, "xxxxxxxx" ) && data_count == 8 ),
        "data set to '%s' instead of 'foobar' or 'xxxxxxx'\n", data );

    /* overflow empty name */
    val_count = 0;
    data_count = 20;
    type = 1234;
    strcpy( value, "xxxxxxxxxx" );
    strcpy( data, "xxxxxxxxxx" );
    res = RegEnumValueA( test_key, 0, value, &val_count, NULL, &type, (LPBYTE)data, &data_count );
    ok( res == ERROR_MORE_DATA, "expected ERROR_MORE_DATA, got %d\n", res );
    ok( val_count == 0, "val_count set to %d\n", val_count );
    ok( data_count == 7 || broken( data_count == 8 ), "data_count set to %d instead of 7\n", data_count );
    ok( type == REG_SZ, "type %d is not REG_SZ\n", type );
    ok( !strcmp( value, "xxxxxxxxxx" ), "value set to '%s'\n", value );
    /* v5.1.2600.0 (XP Home and Professional) does not touch data in this case */
    ok( !strcmp( data, "foobar" ) || !strcmp( data, "xxxxxxx" ) || broken( !strcmp( data, "xxxxxxxx" ) && data_count == 8 ),
        "data set to '%s' instead of 'foobar' or 'xxxxxxx'\n", data );

    /* overflow data */
    val_count = 20;
    data_count = 2;
    type = 1234;
    strcpy( value, "xxxxxxxxxx" );
    strcpy( data, "xxxxxxxxxx" );
    res = RegEnumValueA( test_key, 0, value, &val_count, NULL, &type, (LPBYTE)data, &data_count );
    ok( res == ERROR_MORE_DATA, "expected ERROR_MORE_DATA, got %d\n", res );
    ok( val_count == 20, "val_count set to %d\n", val_count );
    ok( data_count == 7, "data_count set to %d instead of 7\n", data_count );
    ok( type == REG_SZ, "type %d is not REG_SZ\n", type );
    ok( !strcmp( value, "xxxxxxxxxx" ), "value set to '%s'\n", value );
    ok( !strcmp( data, "xxxxxxxxxx" ), "data set to '%s'\n", data );

    /* no overflow */
    val_count = 20;
    data_count = 20;
    type = 1234;
    strcpy( value, "xxxxxxxxxx" );
    strcpy( data, "xxxxxxxxxx" );
    res = RegEnumValueA( test_key, 0, value, &val_count, NULL, &type, (LPBYTE)data, &data_count );
    ok( res == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %d\n", res );
    ok( val_count == 4, "val_count set to %d instead of 4\n", val_count );
    ok( data_count == 7, "data_count set to %d instead of 7\n", data_count );
    ok( type == REG_SZ, "type %d is not REG_SZ\n", type );
    ok( !strcmp( value, "Test" ), "value is '%s' instead of Test\n", value );
    ok( !strcmp( data, "foobar" ), "data is '%s' instead of foobar\n", data );

    /* Unicode tests */

    SetLastError(0xdeadbeef);
    res = RegSetValueExW( test_key, testW, 0, REG_SZ, (const BYTE *)foobarW, 7*sizeof(WCHAR) );
    if (res==0 && GetLastError()==ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("RegSetValueExW is not implemented\n");
        goto cleanup;
    }
    ok( res == 0, "RegSetValueExW failed error %d\n", res );

    /* overflow both name and data */
    val_count = 2;
    data_count = 2;
    type = 1234;
    memcpy( valueW, xxxW, sizeof(xxxW) );
    memcpy( dataW, xxxW, sizeof(xxxW) );
    res = RegEnumValueW( test_key, 0, valueW, &val_count, NULL, &type, (BYTE*)dataW, &data_count );
    ok( res == ERROR_MORE_DATA, "expected ERROR_MORE_DATA, got %d\n", res );
    ok( val_count == 2, "val_count set to %d\n", val_count );
    ok( data_count == 7*sizeof(WCHAR), "data_count set to %d instead of 7*sizeof(WCHAR)\n", data_count );
    ok( type == REG_SZ, "type %d is not REG_SZ\n", type );
    ok( !memcmp( valueW, xxxW, sizeof(xxxW) ), "value modified\n" );
    ok( !memcmp( dataW, xxxW, sizeof(xxxW) ), "data modified\n" );

    /* overflow name */
    val_count = 3;
    data_count = 20;
    type = 1234;
    memcpy( valueW, xxxW, sizeof(xxxW) );
    memcpy( dataW, xxxW, sizeof(xxxW) );
    res = RegEnumValueW( test_key, 0, valueW, &val_count, NULL, &type, (BYTE*)dataW, &data_count );
    ok( res == ERROR_MORE_DATA, "expected ERROR_MORE_DATA, got %d\n", res );
    ok( val_count == 3, "val_count set to %d\n", val_count );
    ok( data_count == 7*sizeof(WCHAR), "data_count set to %d instead of 7*sizeof(WCHAR)\n", data_count );
    ok( type == REG_SZ, "type %d is not REG_SZ\n", type );
    ok( !memcmp( valueW, xxxW, sizeof(xxxW) ), "value modified\n" );
    ok( !memcmp( dataW, xxxW, sizeof(xxxW) ), "data modified\n" );

    /* overflow data */
    val_count = 20;
    data_count = 2;
    type = 1234;
    memcpy( valueW, xxxW, sizeof(xxxW) );
    memcpy( dataW, xxxW, sizeof(xxxW) );
    res = RegEnumValueW( test_key, 0, valueW, &val_count, NULL, &type, (BYTE*)dataW, &data_count );
    ok( res == ERROR_MORE_DATA, "expected ERROR_MORE_DATA, got %d\n", res );
    ok( val_count == 4, "val_count set to %d instead of 4\n", val_count );
    ok( data_count == 7*sizeof(WCHAR), "data_count set to %d instead of 7*sizeof(WCHAR)\n", data_count );
    ok( type == REG_SZ, "type %d is not REG_SZ\n", type );
    ok( !memcmp( valueW, testW, sizeof(testW) ), "value is not 'Test'\n" );
    ok( !memcmp( dataW, xxxW, sizeof(xxxW) ), "data modified\n" );

    /* no overflow */
    val_count = 20;
    data_count = 20;
    type = 1234;
    memcpy( valueW, xxxW, sizeof(xxxW) );
    memcpy( dataW, xxxW, sizeof(xxxW) );
    res = RegEnumValueW( test_key, 0, valueW, &val_count, NULL, &type, (BYTE*)dataW, &data_count );
    ok( res == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %d\n", res );
    ok( val_count == 4, "val_count set to %d instead of 4\n", val_count );
    ok( data_count == 7*sizeof(WCHAR), "data_count set to %d instead of 7*sizeof(WCHAR)\n", data_count );
    ok( type == REG_SZ, "type %d is not REG_SZ\n", type );
    ok( !memcmp( valueW, testW, sizeof(testW) ), "value is not 'Test'\n" );
    ok( !memcmp( dataW, foobarW, sizeof(foobarW) ), "data is not 'foobar'\n" );

cleanup:
    RegDeleteKeyA(test_key, "");
    RegCloseKey(test_key);
}

static void test_query_value_ex(void)
{
    DWORD ret;
    DWORD size;
    DWORD type;
    BYTE buffer[10];
    
    ret = RegQueryValueExA(hkey_main, "TP1_SZ", NULL, &type, NULL, &size);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %d\n", ret);
    ok(size == strlen(sTestpath1) + 1, "(%d,%d)\n", (DWORD)strlen(sTestpath1) + 1, size);
    ok(type == REG_SZ, "type %d is not REG_SZ\n", type);

    type = 0xdeadbeef;
    size = 0xdeadbeef;
    ret = RegQueryValueExA(HKEY_CLASSES_ROOT, "Nonexistent Value", NULL, &type, NULL, &size);
    ok(ret == ERROR_FILE_NOT_FOUND, "expected ERROR_FILE_NOT_FOUND, got %d\n", ret);
    ok(size == 0, "size should have been set to 0 instead of %d\n", size);

    size = sizeof(buffer);
    ret = RegQueryValueExA(HKEY_CLASSES_ROOT, "Nonexistent Value", NULL, &type, buffer, &size);
    ok(ret == ERROR_FILE_NOT_FOUND, "expected ERROR_FILE_NOT_FOUND, got %d\n", ret);
    ok(size == sizeof(buffer), "size shouldn't have been changed to %d\n", size);

    size = 4;
    ret = RegQueryValueExA(hkey_main, "BIN32", NULL, &size, buffer, &size);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %d\n", ret);
}

static void test_get_value(void)
{
    DWORD ret;
    DWORD size;
    DWORD type;
    DWORD dw, qw[2];
    CHAR buf[80];
    CHAR expanded[] = "bar\\subdir1";
    CHAR expanded2[] = "ImARatherLongButIndeedNeededString\\subdir1";
   
    if(!pRegGetValueA)
    {
        win_skip("RegGetValue not available on this platform\n");
        return;
    }

    /* Invalid parameter */
    ret = pRegGetValueA(hkey_main, NULL, "DWORD", RRF_RT_REG_DWORD, &type, &dw, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "ret=%d\n", ret);

    /* Query REG_DWORD using RRF_RT_REG_DWORD (ok) */
    size = type = dw = 0xdeadbeef;
    ret = pRegGetValueA(hkey_main, NULL, "DWORD", RRF_RT_REG_DWORD, &type, &dw, &size);
    ok(ret == ERROR_SUCCESS, "ret=%d\n", ret);
    ok(size == 4, "size=%d\n", size);
    ok(type == REG_DWORD, "type=%d\n", type);
    ok(dw == 0x12345678, "dw=%d\n", dw);

    /* Query by subkey-name */
    ret = pRegGetValueA(HKEY_CURRENT_USER, "Software\\Wine\\Test", "DWORD", RRF_RT_REG_DWORD, NULL, NULL, NULL);
    ok(ret == ERROR_SUCCESS, "ret=%d\n", ret);

    /* Query REG_DWORD using RRF_RT_REG_BINARY (restricted) */
    size = type = dw = 0xdeadbeef;
    ret = pRegGetValueA(hkey_main, NULL, "DWORD", RRF_RT_REG_BINARY, &type, &dw, &size);
    ok(ret == ERROR_UNSUPPORTED_TYPE, "ret=%d\n", ret);
    /* Although the function failed all values are retrieved */
    ok(size == 4, "size=%d\n", size);
    ok(type == REG_DWORD, "type=%d\n", type);
    ok(dw == 0x12345678, "dw=%d\n", dw);

    /* Test RRF_ZEROONFAILURE */
    type = dw = 0xdeadbeef; size = 4;
    ret = pRegGetValueA(hkey_main, NULL, "DWORD", RRF_RT_REG_SZ|RRF_ZEROONFAILURE, &type, &dw, &size);
    ok(ret == ERROR_UNSUPPORTED_TYPE, "ret=%d\n", ret);
    /* Again all values are retrieved ... */
    ok(size == 4, "size=%d\n", size);
    ok(type == REG_DWORD, "type=%d\n", type);
    /* ... except the buffer, which is zeroed out */
    ok(dw == 0, "dw=%d\n", dw);

    /* Test RRF_ZEROONFAILURE with a NULL buffer... */
    type = size = 0xbadbeef;
    ret = pRegGetValueA(hkey_main, NULL, "DWORD", RRF_RT_REG_SZ|RRF_ZEROONFAILURE, &type, NULL, &size);
    ok(ret == ERROR_UNSUPPORTED_TYPE, "ret=%d\n", ret);
    ok(size == 4, "size=%d\n", size);
    ok(type == REG_DWORD, "type=%d\n", type);

    /* Query REG_DWORD using RRF_RT_DWORD (ok) */
    size = type = dw = 0xdeadbeef;
    ret = pRegGetValueA(hkey_main, NULL, "DWORD", RRF_RT_DWORD, &type, &dw, &size);
    ok(ret == ERROR_SUCCESS, "ret=%d\n", ret);
    ok(size == 4, "size=%d\n", size);
    ok(type == REG_DWORD, "type=%d\n", type);
    ok(dw == 0x12345678, "dw=%d\n", dw);

    /* Query 32-bit REG_BINARY using RRF_RT_DWORD (ok) */
    size = type = dw = 0xdeadbeef;
    ret = pRegGetValueA(hkey_main, NULL, "BIN32", RRF_RT_DWORD, &type, &dw, &size);
    ok(ret == ERROR_SUCCESS, "ret=%d\n", ret);
    ok(size == 4, "size=%d\n", size);
    ok(type == REG_BINARY, "type=%d\n", type);
    ok(dw == 0x12345678, "dw=%d\n", dw);
    
    /* Query 64-bit REG_BINARY using RRF_RT_DWORD (type mismatch) */
    qw[0] = qw[1] = size = type = 0xdeadbeef;
    ret = pRegGetValueA(hkey_main, NULL, "BIN64", RRF_RT_DWORD, &type, qw, &size);
    ok(ret == ERROR_DATATYPE_MISMATCH, "ret=%d\n", ret);
    ok(size == 8, "size=%d\n", size);
    ok(type == REG_BINARY, "type=%d\n", type);
    ok(qw[0] == 0x12345678 && 
       qw[1] == 0x87654321, "qw={%d,%d}\n", qw[0], qw[1]);
    
    /* Query 64-bit REG_BINARY using 32-bit buffer (buffer too small) */
    type = dw = 0xdeadbeef; size = 4;
    ret = pRegGetValueA(hkey_main, NULL, "BIN64", RRF_RT_REG_BINARY, &type, &dw, &size);
    ok(ret == ERROR_MORE_DATA, "ret=%d\n", ret);
    ok(dw == 0xdeadbeef, "dw=%d\n", dw);
    ok(size == 8, "size=%d\n", size);

    /* Query 64-bit REG_BINARY using RRF_RT_QWORD (ok) */
    qw[0] = qw[1] = size = type = 0xdeadbeef;
    ret = pRegGetValueA(hkey_main, NULL, "BIN64", RRF_RT_QWORD, &type, qw, &size);
    ok(ret == ERROR_SUCCESS, "ret=%d\n", ret);
    ok(size == 8, "size=%d\n", size);
    ok(type == REG_BINARY, "type=%d\n", type);
    ok(qw[0] == 0x12345678 &&
       qw[1] == 0x87654321, "qw={%d,%d}\n", qw[0], qw[1]);

    /* Query REG_SZ using RRF_RT_REG_SZ (ok) */
    buf[0] = 0; type = 0xdeadbeef; size = sizeof(buf);
    ret = pRegGetValueA(hkey_main, NULL, "TP1_SZ", RRF_RT_REG_SZ, &type, buf, &size);
    ok(ret == ERROR_SUCCESS, "ret=%d\n", ret);
    ok(size == strlen(sTestpath1)+1, "strlen(sTestpath1)=%d size=%d\n", lstrlenA(sTestpath1), size);
    ok(type == REG_SZ, "type=%d\n", type);
    ok(!strcmp(sTestpath1, buf), "sTestpath=\"%s\" buf=\"%s\"\n", sTestpath1, buf);

    /* Query REG_SZ using RRF_RT_REG_SZ and no buffer (ok) */
    type = 0xdeadbeef; size = 0;
    ret = pRegGetValueA(hkey_main, NULL, "TP1_SZ", RRF_RT_REG_SZ, &type, NULL, &size);
    ok(ret == ERROR_SUCCESS, "ret=%d\n", ret);
    /* v5.2.3790.1830 (2003 SP1) returns sTestpath1 length + 2 here. */
    ok(size == strlen(sTestpath1)+1 || broken(size == strlen(sTestpath1)+2),
       "strlen(sTestpath1)=%d size=%d\n", lstrlenA(sTestpath1), size);
    ok(type == REG_SZ, "type=%d\n", type);

    /* Query REG_SZ using RRF_RT_REG_SZ on a zero-byte value (ok) */
    strcpy(buf, sTestpath1);
    type = 0xdeadbeef;
    size = sizeof(buf);
    ret = pRegGetValueA(hkey_main, NULL, "TP1_ZB_SZ", RRF_RT_REG_SZ, &type, buf, &size);
    ok(ret == ERROR_SUCCESS, "ret=%d\n", ret);
    /* v5.2.3790.1830 (2003 SP1) returns sTestpath1 length + 2 here. */
    ok(size == 0 ||
       size == 1, /* win2k3 */
       "size=%d\n", size);
    ok(type == REG_SZ, "type=%d\n", type);
    ok(!strcmp(sTestpath1, buf) ||
       !strcmp(buf, ""),
       "Expected \"%s\" or \"\", got \"%s\"\n", sTestpath1, buf);

    /* Query REG_SZ using RRF_RT_REG_SZ|RRF_NOEXPAND (ok) */
    buf[0] = 0; type = 0xdeadbeef; size = sizeof(buf);
    ret = pRegGetValueA(hkey_main, NULL, "TP1_SZ", RRF_RT_REG_SZ|RRF_NOEXPAND, &type, buf, &size);
    ok(ret == ERROR_SUCCESS, "ret=%d\n", ret);
    ok(size == strlen(sTestpath1)+1, "strlen(sTestpath1)=%d size=%d\n", lstrlenA(sTestpath1), size);
    ok(type == REG_SZ, "type=%d\n", type);
    ok(!strcmp(sTestpath1, buf), "sTestpath=\"%s\" buf=\"%s\"\n", sTestpath1, buf);

    /* Query REG_EXPAND_SZ using RRF_RT_REG_SZ and no buffer (ok, expands) */
    size = 0;
    ret = pRegGetValueA(hkey_main, NULL, "TP2_EXP_SZ", RRF_RT_REG_SZ, NULL, NULL, &size);
    ok(ret == ERROR_SUCCESS, "ret=%d\n", ret);
    ok((size == strlen(expanded2)+1) || /* win2k3 SP1 */
       (size == strlen(expanded2)+2) || /* win2k3 SP2 */
       (size == strlen(sTestpath2)+1),
        "strlen(expanded2)=%d, strlen(sTestpath2)=%d, size=%d\n", lstrlenA(expanded2), lstrlenA(sTestpath2), size);

    /* Query REG_EXPAND_SZ using RRF_RT_REG_SZ (ok, expands) */
    buf[0] = 0; type = 0xdeadbeef; size = sizeof(buf);
    ret = pRegGetValueA(hkey_main, NULL, "TP1_EXP_SZ", RRF_RT_REG_SZ, &type, buf, &size);
    ok(ret == ERROR_SUCCESS, "ret=%d\n", ret);
    /* At least v5.2.3790.1830 (2003 SP1) returns the unexpanded sTestpath1 length + 1 here. */
    ok(size == strlen(expanded)+1 || broken(size == strlen(sTestpath1)+1),
        "strlen(expanded)=%d, strlen(sTestpath1)=%d, size=%d\n", lstrlenA(expanded), lstrlenA(sTestpath1), size);
    ok(type == REG_SZ, "type=%d\n", type);
    ok(!strcmp(expanded, buf), "expanded=\"%s\" buf=\"%s\"\n", expanded, buf);

    /* Query REG_EXPAND_SZ using RRF_RT_REG_SZ (ok, expands a lot) */
    buf[0] = 0; type = 0xdeadbeef; size = sizeof(buf);
    ret = pRegGetValueA(hkey_main, NULL, "TP2_EXP_SZ", RRF_RT_REG_SZ, &type, buf, &size);
    ok(ret == ERROR_SUCCESS, "ret=%d\n", ret);
    /* At least v5.2.3790.1830 (2003 SP1) returns the unexpanded sTestpath2 length + 1 here. */
    ok(size == strlen(expanded2)+1 || broken(size == strlen(sTestpath2)+1),
        "strlen(expanded2)=%d, strlen(sTestpath1)=%d, size=%d\n", lstrlenA(expanded2), lstrlenA(sTestpath2), size);
    ok(type == REG_SZ, "type=%d\n", type);
    ok(!strcmp(expanded2, buf), "expanded2=\"%s\" buf=\"%s\"\n", expanded2, buf);

    /* Query REG_EXPAND_SZ using RRF_RT_REG_EXPAND_SZ|RRF_NOEXPAND (ok, doesn't expand) */
    buf[0] = 0; type = 0xdeadbeef; size = sizeof(buf);
    ret = pRegGetValueA(hkey_main, NULL, "TP1_EXP_SZ", RRF_RT_REG_EXPAND_SZ|RRF_NOEXPAND, &type, buf, &size);
    ok(ret == ERROR_SUCCESS, "ret=%d\n", ret);
    ok(size == strlen(sTestpath1)+1, "strlen(sTestpath1)=%d size=%d\n", lstrlenA(sTestpath1), size);
    ok(type == REG_EXPAND_SZ, "type=%d\n", type);
    ok(!strcmp(sTestpath1, buf), "sTestpath=\"%s\" buf=\"%s\"\n", sTestpath1, buf);

    /* Query REG_EXPAND_SZ using RRF_RT_REG_EXPAND_SZ|RRF_NOEXPAND and no buffer (ok, doesn't expand) */
    size = 0xbadbeef;
    ret = pRegGetValueA(hkey_main, NULL, "TP1_EXP_SZ", RRF_RT_REG_EXPAND_SZ|RRF_NOEXPAND, NULL, NULL, &size);
    ok(ret == ERROR_SUCCESS, "ret=%d\n", ret);
    /* v5.2.3790.1830 (2003 SP1) returns sTestpath1 length + 2 here. */
    ok(size == strlen(sTestpath1)+1 || broken(size == strlen(sTestpath1)+2),
       "strlen(sTestpath1)=%d size=%d\n", lstrlenA(sTestpath1), size);

    /* Query REG_EXPAND_SZ using RRF_RT_REG_SZ|RRF_NOEXPAND (type mismatch) */
    ret = pRegGetValueA(hkey_main, NULL, "TP1_EXP_SZ", RRF_RT_REG_SZ|RRF_NOEXPAND, NULL, NULL, NULL);
    ok(ret == ERROR_UNSUPPORTED_TYPE, "ret=%d\n", ret);

    /* Query REG_EXPAND_SZ using RRF_RT_REG_EXPAND_SZ (not allowed without RRF_NOEXPAND) */
    ret = pRegGetValueA(hkey_main, NULL, "TP1_EXP_SZ", RRF_RT_REG_EXPAND_SZ, NULL, NULL, NULL);
    /* before win8: ERROR_INVALID_PARAMETER, win8: ERROR_UNSUPPORTED_TYPE */
    ok(ret == ERROR_INVALID_PARAMETER || ret == ERROR_UNSUPPORTED_TYPE, "ret=%d\n", ret);

    /* Query REG_EXPAND_SZ using RRF_RT_ANY */
    buf[0] = 0; type = 0xdeadbeef; size = sizeof(buf);
    ret = pRegGetValueA(hkey_main, NULL, "TP1_EXP_SZ", RRF_RT_ANY, &type, buf, &size);
    ok(ret == ERROR_SUCCESS, "ret=%d\n", ret);
    /* At least v5.2.3790.1830 (2003 SP1) returns the unexpanded sTestpath1 length + 1 here. */
    ok(size == strlen(expanded)+1 || broken(size == strlen(sTestpath1)+1),
        "strlen(expanded)=%d, strlen(sTestpath1)=%d, size=%d\n", lstrlenA(expanded), lstrlenA(sTestpath1), size);
    ok(type == REG_SZ, "type=%d\n", type);
    ok(!strcmp(expanded, buf), "expanded=\"%s\" buf=\"%s\"\n", expanded, buf);
} 

static void test_reg_open_key(void)
{
    DWORD ret = 0;
    HKEY hkResult = NULL;
    HKEY hkPreserve = NULL;
    HKEY hkRoot64 = NULL;
    HKEY hkRoot32 = NULL;
    BOOL bRet;
    SID_IDENTIFIER_AUTHORITY sid_authority = {SECURITY_WORLD_SID_AUTHORITY};
    PSID world_sid;
    EXPLICIT_ACCESSA access;
    PACL key_acl;
    SECURITY_DESCRIPTOR *sd;

    /* successful open */
    ret = RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\Test", &hkResult);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %d\n", ret);
    ok(hkResult != NULL, "expected hkResult != NULL\n");
    hkPreserve = hkResult;

    /* open same key twice */
    ret = RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\Test", &hkResult);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %d\n", ret);
    ok(hkResult != hkPreserve, "expected hkResult != hkPreserve\n");
    ok(hkResult != NULL, "hkResult != NULL\n");
    RegCloseKey(hkResult);

    /* trailing slashes */
    ret = RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\Test\\\\", &hkResult);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %d\n", ret);
    RegCloseKey(hkResult);

    /* open nonexistent key
    * check that hkResult is set to NULL
    */
    hkResult = hkPreserve;
    ret = RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\Nonexistent", &hkResult);
    ok(ret == ERROR_FILE_NOT_FOUND, "expected ERROR_FILE_NOT_FOUND, got %d\n", ret);
    ok(hkResult == NULL, "expected hkResult == NULL\n");

    /* open the same nonexistent key again to make sure the key wasn't created */
    hkResult = hkPreserve;
    ret = RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\Nonexistent", &hkResult);
    ok(ret == ERROR_FILE_NOT_FOUND, "expected ERROR_FILE_NOT_FOUND, got %d\n", ret);
    ok(hkResult == NULL, "expected hkResult == NULL\n");

    /* send in NULL lpSubKey
    * check that hkResult receives the value of hKey
    */
    hkResult = hkPreserve;
    ret = RegOpenKeyA(HKEY_CURRENT_USER, NULL, &hkResult);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %d\n", ret);
    ok(hkResult == HKEY_CURRENT_USER, "expected hkResult == HKEY_CURRENT_USER\n");

    /* send empty-string in lpSubKey */
    hkResult = hkPreserve;
    ret = RegOpenKeyA(HKEY_CURRENT_USER, "", &hkResult);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %d\n", ret);
    ok(hkResult == HKEY_CURRENT_USER, "expected hkResult == HKEY_CURRENT_USER\n");

    /* send in NULL lpSubKey and NULL hKey
    * hkResult is set to NULL
    */
    hkResult = hkPreserve;
    ret = RegOpenKeyA(NULL, NULL, &hkResult);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %d\n", ret);
    ok(hkResult == NULL, "expected hkResult == NULL\n");

    /* only send NULL hKey
     * the value of hkResult remains unchanged
     */
    hkResult = hkPreserve;
    ret = RegOpenKeyA(NULL, "Software\\Wine\\Test", &hkResult);
    ok(ret == ERROR_INVALID_HANDLE || ret == ERROR_BADKEY, /* Windows 95 returns BADKEY */
       "expected ERROR_INVALID_HANDLE or ERROR_BADKEY, got %d\n", ret);
    ok(hkResult == hkPreserve, "expected hkResult == hkPreserve\n");
    RegCloseKey(hkResult);

    /* send in NULL hkResult */
    ret = RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\Test", NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %d\n", ret);

    ret = RegOpenKeyA(HKEY_CURRENT_USER, NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %d\n", ret);

    ret = RegOpenKeyA(NULL, NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %d\n", ret);

    /*  beginning backslash character */
    ret = RegOpenKeyA(HKEY_CURRENT_USER, "\\Software\\Wine\\Test", &hkResult);
    ok(ret == ERROR_BAD_PATHNAME || /* NT/2k/XP */
       broken(ret == ERROR_SUCCESS),  /* wow64 */
       "expected ERROR_BAD_PATHNAME or ERROR_FILE_NOT_FOUND, got %d\n", ret);
    if (!ret) RegCloseKey(hkResult);

    hkResult = NULL;
    ret = RegOpenKeyExA(HKEY_CLASSES_ROOT, "\\clsid", 0, KEY_QUERY_VALUE, &hkResult);
    ok(ret == ERROR_SUCCESS || /* 2k/XP */
       ret == ERROR_BAD_PATHNAME, /* NT */
       "expected ERROR_SUCCESS, ERROR_BAD_PATHNAME or ERROR_FILE_NOT_FOUND, got %d\n", ret);
    RegCloseKey(hkResult);

    /* WOW64 flags */
    hkResult = NULL;
    ret = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software", 0, KEY_READ|KEY_WOW64_32KEY, &hkResult);
    ok((ret == ERROR_SUCCESS && hkResult != NULL) || broken(ret == ERROR_ACCESS_DENIED /* NT4, win2k */),
        "RegOpenKeyEx with KEY_WOW64_32KEY failed (err=%u)\n", ret);
    RegCloseKey(hkResult);

    hkResult = NULL;
    ret = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software", 0, KEY_READ|KEY_WOW64_64KEY, &hkResult);
    ok((ret == ERROR_SUCCESS && hkResult != NULL) || broken(ret == ERROR_ACCESS_DENIED /* NT4, win2k */),
        "RegOpenKeyEx with KEY_WOW64_64KEY failed (err=%u)\n", ret);
    RegCloseKey(hkResult);

    /* check special HKEYs on 64bit
     * only the lower 4 bytes of the supplied key are used
     */
    if (ptr_size == 64)
    {
        /* HKEY_CURRENT_USER */
        ret = RegOpenKeyA(UlongToHandle(HandleToUlong(HKEY_CURRENT_USER)), "Software", &hkResult);
        ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %d\n", ret);
        ok(hkResult != NULL, "expected hkResult != NULL\n");
        RegCloseKey(hkResult);

        ret = RegOpenKeyA((HKEY)(HandleToUlong(HKEY_CURRENT_USER) | (ULONG64)1 << 32), "Software", &hkResult);
        ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %d\n", ret);
        ok(hkResult != NULL, "expected hkResult != NULL\n");
        RegCloseKey(hkResult);

        ret = RegOpenKeyA((HKEY)(HandleToUlong(HKEY_CURRENT_USER) | (ULONG64)0xdeadbeef << 32), "Software", &hkResult);
        ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %d\n", ret);
        ok(hkResult != NULL, "expected hkResult != NULL\n");
        RegCloseKey(hkResult);

        ret = RegOpenKeyA((HKEY)(HandleToUlong(HKEY_CURRENT_USER) | (ULONG64)0xffffffff << 32), "Software", &hkResult);
        ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %d\n", ret);
        ok(hkResult != NULL, "expected hkResult != NULL\n");
        RegCloseKey(hkResult);

        /* HKEY_LOCAL_MACHINE */
        ret = RegOpenKeyA((HKEY)(HandleToUlong(HKEY_LOCAL_MACHINE) | (ULONG64)0xdeadbeef << 32), "Software", &hkResult);
        ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %d\n", ret);
        ok(hkResult != NULL, "expected hkResult != NULL\n");
        RegCloseKey(hkResult);
    }

    /* Try using WOW64 flags when opening a key with a DACL set to verify that
     * the registry access check is performed correctly. Redirection isn't
     * being tested, so the tests don't care about whether the process is
     * running under WOW64. */
    if (!pIsWow64Process)
    {
        win_skip("WOW64 flags are not recognized\n");
        return;
    }

    ret = RegCreateKeyExA(HKEY_LOCAL_MACHINE, "Software\\Wine", 0, NULL, 0,
                          KEY_WOW64_32KEY | KEY_ALL_ACCESS, NULL, &hkRoot32, NULL);
    if (limited_user)
        ok(ret == ERROR_ACCESS_DENIED && hkRoot32 == NULL,
           "RegCreateKeyEx with KEY_WOW64_32KEY failed (err=%u)\n", ret);
    else
        ok(ret == ERROR_SUCCESS && hkRoot32 != NULL,
           "RegCreateKeyEx with KEY_WOW64_32KEY failed (err=%u)\n", ret);

    ret = RegCreateKeyExA(HKEY_LOCAL_MACHINE, "Software\\Wine", 0, NULL, 0,
                          KEY_WOW64_64KEY | KEY_ALL_ACCESS, NULL, &hkRoot64, NULL);
    if (limited_user)
        ok(ret == ERROR_ACCESS_DENIED && hkRoot64 == NULL,
           "RegCreateKeyEx with KEY_WOW64_64KEY failed (err=%u)\n", ret);
    else
        ok(ret == ERROR_SUCCESS && hkRoot64 != NULL,
           "RegCreateKeyEx with KEY_WOW64_64KEY failed (err=%u)\n", ret);

    bRet = AllocateAndInitializeSid(&sid_authority, 1, SECURITY_WORLD_RID,
                                    0, 0, 0, 0, 0, 0, 0, &world_sid);
    ok(bRet == TRUE,
       "Expected AllocateAndInitializeSid to return TRUE, got %d, last error %u\n", bRet, GetLastError());

    access.grfAccessPermissions = GENERIC_ALL | STANDARD_RIGHTS_ALL;
    access.grfAccessMode = SET_ACCESS;
    access.grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    access.Trustee.pMultipleTrustee = NULL;
    access.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    access.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    access.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    access.Trustee.ptstrName = (char *)world_sid;

    ret = SetEntriesInAclA(1, &access, NULL, &key_acl);
    ok(ret == ERROR_SUCCESS,
       "Expected SetEntriesInAclA to return ERROR_SUCCESS, got %u, last error %u\n", ret, GetLastError());

    sd = HeapAlloc(GetProcessHeap(), 0, SECURITY_DESCRIPTOR_MIN_LENGTH);
    bRet = InitializeSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION);
    ok(bRet == TRUE,
       "Expected InitializeSecurityDescriptor to return TRUE, got %d, last error %u\n", bRet, GetLastError());

    bRet = SetSecurityDescriptorDacl(sd, TRUE, key_acl, FALSE);
    ok(bRet == TRUE,
       "Expected SetSecurityDescriptorDacl to return TRUE, got %d, last error %u\n", bRet, GetLastError());

    if (limited_user)
    {
        skip("not enough privileges to modify HKLM\n");
    }
    else
    {
        LONG error;

        error = RegSetKeySecurity(hkRoot64, DACL_SECURITY_INFORMATION, sd);
        ok(error == ERROR_SUCCESS,
           "Expected RegSetKeySecurity to return success, got error %u\n", error);

        error = RegSetKeySecurity(hkRoot32, DACL_SECURITY_INFORMATION, sd);
        ok(error == ERROR_SUCCESS,
           "Expected RegSetKeySecurity to return success, got error %u\n", error);

        hkResult = NULL;
        ret = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\Wine", 0, KEY_WOW64_64KEY | KEY_READ, &hkResult);
        ok(ret == ERROR_SUCCESS && hkResult != NULL,
           "RegOpenKeyEx with KEY_WOW64_64KEY failed (err=%u)\n", ret);
        RegCloseKey(hkResult);

        hkResult = NULL;
        ret = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\Wine", 0, KEY_WOW64_32KEY | KEY_READ, &hkResult);
        ok(ret == ERROR_SUCCESS && hkResult != NULL,
           "RegOpenKeyEx with KEY_WOW64_32KEY failed (err=%u)\n", ret);
        RegCloseKey(hkResult);
    }

    HeapFree(GetProcessHeap(), 0, sd);
    LocalFree(key_acl);
    FreeSid(world_sid);
    RegDeleteKeyA(hkRoot64, "");
    RegCloseKey(hkRoot64);
    RegDeleteKeyA(hkRoot32, "");
    RegCloseKey(hkRoot32);
}

static void test_reg_create_key(void)
{
    LONG ret;
    HKEY hkey1, hkey2;
    HKEY hkRoot64 = NULL;
    HKEY hkRoot32 = NULL;
    DWORD dwRet;
    BOOL bRet;
    SID_IDENTIFIER_AUTHORITY sid_authority = {SECURITY_WORLD_SID_AUTHORITY};
    PSID world_sid;
    EXPLICIT_ACCESSA access;
    PACL key_acl;
    SECURITY_DESCRIPTOR *sd;

    ret = RegCreateKeyExA(hkey_main, "Subkey1", 0, NULL, 0, KEY_NOTIFY, NULL, &hkey1, NULL);
    ok(!ret, "RegCreateKeyExA failed with error %d\n", ret);
    /* should succeed: all versions of Windows ignore the access rights
     * to the parent handle */
    ret = RegCreateKeyExA(hkey1, "Subkey2", 0, NULL, 0, KEY_SET_VALUE, NULL, &hkey2, NULL);
    ok(!ret, "RegCreateKeyExA failed with error %d\n", ret);

    /* clean up */
    RegDeleteKeyA(hkey2, "");
    RegDeleteKeyA(hkey1, "");
    RegCloseKey(hkey2);
    RegCloseKey(hkey1);

    /* test creation of volatile keys */
    ret = RegCreateKeyExA(hkey_main, "Volatile", 0, NULL, REG_OPTION_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey1, NULL);
    ok(!ret, "RegCreateKeyExA failed with error %d\n", ret);
    ret = RegCreateKeyExA(hkey1, "Subkey2", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hkey2, NULL);
    ok(ret == ERROR_CHILD_MUST_BE_VOLATILE, "RegCreateKeyExA failed with error %d\n", ret);
    if (!ret) RegCloseKey( hkey2 );
    ret = RegCreateKeyExA(hkey1, "Subkey2", 0, NULL, REG_OPTION_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey2, NULL);
    ok(!ret, "RegCreateKeyExA failed with error %d\n", ret);
    RegCloseKey(hkey2);
    /* should succeed if the key already exists */
    ret = RegCreateKeyExA(hkey1, "Subkey2", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hkey2, NULL);
    ok(!ret, "RegCreateKeyExA failed with error %d\n", ret);

    /* clean up */
    RegDeleteKeyA(hkey2, "");
    RegDeleteKeyA(hkey1, "");
    RegCloseKey(hkey2);
    RegCloseKey(hkey1);

    /*  beginning backslash character */
    ret = RegCreateKeyExA(hkey_main, "\\Subkey3", 0, NULL, 0, KEY_NOTIFY, NULL, &hkey1, NULL);
    if (!(GetVersion() & 0x80000000))
        ok(ret == ERROR_BAD_PATHNAME, "expected ERROR_BAD_PATHNAME, got %d\n", ret);
    else {
        ok(!ret, "RegCreateKeyExA failed with error %d\n", ret);
        RegDeleteKeyA(hkey1, "");
        RegCloseKey(hkey1);
    }

    /* trailing backslash characters */
    ret = RegCreateKeyExA(hkey_main, "Subkey4\\\\", 0, NULL, 0, KEY_NOTIFY, NULL, &hkey1, NULL);
    ok(ret == ERROR_SUCCESS, "RegCreateKeyExA failed with error %d\n", ret);
    RegDeleteKeyA(hkey1, "");
    RegCloseKey(hkey1);

    /* WOW64 flags - open an existing key */
    hkey1 = NULL;
    ret = RegCreateKeyExA(HKEY_LOCAL_MACHINE, "Software", 0, NULL, 0, KEY_READ|KEY_WOW64_32KEY, NULL, &hkey1, NULL);
    ok((ret == ERROR_SUCCESS && hkey1 != NULL) || broken(ret == ERROR_ACCESS_DENIED /* NT4, win2k */),
        "RegOpenKeyEx with KEY_WOW64_32KEY failed (err=%u)\n", ret);
    RegCloseKey(hkey1);

    hkey1 = NULL;
    ret = RegCreateKeyExA(HKEY_LOCAL_MACHINE, "Software", 0, NULL, 0, KEY_READ|KEY_WOW64_64KEY, NULL, &hkey1, NULL);
    ok((ret == ERROR_SUCCESS && hkey1 != NULL) || broken(ret == ERROR_ACCESS_DENIED /* NT4, win2k */),
        "RegOpenKeyEx with KEY_WOW64_64KEY failed (err=%u)\n", ret);
    RegCloseKey(hkey1);

    /* Try using WOW64 flags when opening a key with a DACL set to verify that
     * the registry access check is performed correctly. Redirection isn't
     * being tested, so the tests don't care about whether the process is
     * running under WOW64. */
    if (!pIsWow64Process)
    {
        win_skip("WOW64 flags are not recognized\n");
        return;
    }

    ret = RegCreateKeyExA(HKEY_LOCAL_MACHINE, "Software\\Wine", 0, NULL, 0,
                          KEY_WOW64_32KEY | KEY_ALL_ACCESS, NULL, &hkRoot32, NULL);
    if (limited_user)
        ok(ret == ERROR_ACCESS_DENIED && hkRoot32 == NULL,
           "RegCreateKeyEx with KEY_WOW64_32KEY failed (err=%d)\n", ret);
    else
        ok(ret == ERROR_SUCCESS && hkRoot32 != NULL,
           "RegCreateKeyEx with KEY_WOW64_32KEY failed (err=%d)\n", ret);

    ret = RegCreateKeyExA(HKEY_LOCAL_MACHINE, "Software\\Wine", 0, NULL, 0,
                          KEY_WOW64_64KEY | KEY_ALL_ACCESS, NULL, &hkRoot64, NULL);
    if (limited_user)
        ok(ret == ERROR_ACCESS_DENIED && hkRoot64 == NULL,
           "RegCreateKeyEx with KEY_WOW64_64KEY failed (err=%d)\n", ret);
    else
        ok(ret == ERROR_SUCCESS && hkRoot64 != NULL,
           "RegCreateKeyEx with KEY_WOW64_64KEY failed (err=%d)\n", ret);

    bRet = AllocateAndInitializeSid(&sid_authority, 1, SECURITY_WORLD_RID,
                                    0, 0, 0, 0, 0, 0, 0, &world_sid);
    ok(bRet == TRUE,
       "Expected AllocateAndInitializeSid to return TRUE, got %d, last error %u\n", bRet, GetLastError());

    access.grfAccessPermissions = GENERIC_ALL | STANDARD_RIGHTS_ALL;
    access.grfAccessMode = SET_ACCESS;
    access.grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    access.Trustee.pMultipleTrustee = NULL;
    access.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    access.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    access.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    access.Trustee.ptstrName = (char *)world_sid;

    dwRet = SetEntriesInAclA(1, &access, NULL, &key_acl);
    ok(dwRet == ERROR_SUCCESS,
       "Expected SetEntriesInAclA to return ERROR_SUCCESS, got %u, last error %u\n", dwRet, GetLastError());

    sd = HeapAlloc(GetProcessHeap(), 0, SECURITY_DESCRIPTOR_MIN_LENGTH);
    bRet = InitializeSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION);
    ok(bRet == TRUE,
       "Expected InitializeSecurityDescriptor to return TRUE, got %d, last error %u\n", bRet, GetLastError());

    bRet = SetSecurityDescriptorDacl(sd, TRUE, key_acl, FALSE);
    ok(bRet == TRUE,
       "Expected SetSecurityDescriptorDacl to return TRUE, got %d, last error %u\n", bRet, GetLastError());

    if (limited_user)
    {
        skip("not enough privileges to modify HKLM\n");
    }
    else
    {
        ret = RegSetKeySecurity(hkRoot64, DACL_SECURITY_INFORMATION, sd);
        ok(ret == ERROR_SUCCESS,
           "Expected RegSetKeySecurity to return success, got error %u\n", ret);

        ret = RegSetKeySecurity(hkRoot32, DACL_SECURITY_INFORMATION, sd);
        ok(ret == ERROR_SUCCESS,
           "Expected RegSetKeySecurity to return success, got error %u\n", ret);

        hkey1 = NULL;
        ret = RegCreateKeyExA(HKEY_LOCAL_MACHINE, "Software\\Wine", 0, NULL, 0,
                              KEY_WOW64_64KEY | KEY_READ, NULL, &hkey1, NULL);
        ok(ret == ERROR_SUCCESS && hkey1 != NULL,
           "RegOpenKeyEx with KEY_WOW64_64KEY failed (err=%u)\n", ret);
        RegCloseKey(hkey1);

        hkey1 = NULL;
        ret = RegCreateKeyExA(HKEY_LOCAL_MACHINE, "Software\\Wine", 0, NULL, 0,
                              KEY_WOW64_32KEY | KEY_READ, NULL, &hkey1, NULL);
        ok(ret == ERROR_SUCCESS && hkey1 != NULL,
           "RegOpenKeyEx with KEY_WOW64_32KEY failed (err=%u)\n", ret);
        RegCloseKey(hkey1);
    }

    HeapFree(GetProcessHeap(), 0, sd);
    LocalFree(key_acl);
    FreeSid(world_sid);
    RegDeleteKeyA(hkRoot64, "");
    RegCloseKey(hkRoot64);
    RegDeleteKeyA(hkRoot32, "");
    RegCloseKey(hkRoot32);
}

static void test_reg_close_key(void)
{
    DWORD ret = 0;
    HKEY hkHandle;

    /* successfully close key
     * hkHandle remains changed after call to RegCloseKey
     */
    ret = RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\Test", &hkHandle);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %d\n", ret);
    ret = RegCloseKey(hkHandle);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %d\n", ret);

    /* try to close the key twice */
    ret = RegCloseKey(hkHandle); /* Windows 95 doesn't mind. */
    ok(ret == ERROR_INVALID_HANDLE || ret == ERROR_SUCCESS,
       "expected ERROR_INVALID_HANDLE or ERROR_SUCCESS, got %d\n", ret);
    
    /* try to close a NULL handle */
    ret = RegCloseKey(NULL);
    ok(ret == ERROR_INVALID_HANDLE || ret == ERROR_BADKEY, /* Windows 95 returns BADKEY */
       "expected ERROR_INVALID_HANDLE or ERROR_BADKEY, got %d\n", ret);

    /* Check to see if we didn't potentially close our main handle, which could happen on win98 as
     * win98 doesn't give a new handle when the same key is opened.
     * Not re-opening will make some next tests fail.
     */
    if (hkey_main == hkHandle)
    {
        trace("The main handle is most likely closed, so re-opening\n");
        RegOpenKeyA( HKEY_CURRENT_USER, "Software\\Wine\\Test", &hkey_main );
    }
}

static void test_reg_delete_key(void)
{
    DWORD ret;
    HKEY key;

    ret = RegDeleteKeyA(hkey_main, NULL);

    /* There is a bug in NT4 and W2K that doesn't check if the subkey is NULL. If
     * there are also no subkeys available it will delete the key pointed to by hkey_main.
     * Not re-creating will make some next tests fail.
     */
    if (ret == ERROR_SUCCESS)
    {
        trace("We are probably running on NT4 or W2K as the main key is deleted,"
            " re-creating the main key\n");
        setup_main_key();
    }
    else
        ok(ret == ERROR_INVALID_PARAMETER ||
           ret == ERROR_ACCESS_DENIED ||
           ret == ERROR_BADKEY, /* Win95 */
           "ret=%d\n", ret);

    ret = RegCreateKeyA(hkey_main, "deleteme", &key);
    ok(ret == ERROR_SUCCESS, "Could not create key, got %d\n", ret);
    ret = RegDeleteKeyA(key, "");
    ok(ret == ERROR_SUCCESS, "RegDeleteKeyA failed, got %d\n", ret);
    RegCloseKey(key);
    ret = RegOpenKeyA(hkey_main, "deleteme", &key);
    ok(ret == ERROR_FILE_NOT_FOUND, "Key was not deleted, got %d\n", ret);
    RegCloseKey(key);
}

static void test_reg_save_key(void)
{
    DWORD ret;

    ret = RegSaveKeyA(hkey_main, "saved_key", NULL);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %d\n", ret);
}

static void test_reg_load_key(void)
{
    DWORD ret;
    HKEY hkHandle;

    ret = RegLoadKeyA(HKEY_LOCAL_MACHINE, "Test", "saved_key");
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %d\n", ret);

    ret = RegOpenKeyA(HKEY_LOCAL_MACHINE, "Test", &hkHandle);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %d\n", ret);

    RegCloseKey(hkHandle);
}

static void test_reg_unload_key(void)
{
    DWORD ret;

    ret = RegUnLoadKeyA(HKEY_LOCAL_MACHINE, "Test");
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %d\n", ret);

    DeleteFileA("saved_key");
    DeleteFileA("saved_key.LOG");
}

static BOOL set_privileges(LPCSTR privilege, BOOL set)
{
    TOKEN_PRIVILEGES tp;
    HANDLE hToken;
    LUID luid;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
        return FALSE;

    if(!LookupPrivilegeValueA(NULL, privilege, &luid))
    {
        CloseHandle(hToken);
        return FALSE;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    
    if (set)
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    else
        tp.Privileges[0].Attributes = 0;

    AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
    if (GetLastError() != ERROR_SUCCESS)
    {
        CloseHandle(hToken);
        return FALSE;
    }

    CloseHandle(hToken);
    return TRUE;
}

/* tests that show that RegConnectRegistry and 
   OpenSCManager accept computer names without the
   \\ prefix (what MSDN says).   */
static void test_regconnectregistry( void)
{
    CHAR compName[MAX_COMPUTERNAME_LENGTH + 1];
    CHAR netwName[MAX_COMPUTERNAME_LENGTH + 3]; /* 2 chars for double backslash */
    DWORD len = sizeof(compName) ;
    BOOL ret;
    LONG retl;
    HKEY hkey;
    SC_HANDLE schnd;

    SetLastError(0xdeadbeef);
    ret = GetComputerNameA(compName, &len);
    ok( ret, "GetComputerName failed err = %d\n", GetLastError());
    if( !ret) return;

    lstrcpyA(netwName, "\\\\");
    lstrcpynA(netwName+2, compName, MAX_COMPUTERNAME_LENGTH + 1);

    retl = RegConnectRegistryA( compName, HKEY_LOCAL_MACHINE, &hkey);
    ok( !retl ||
        retl == ERROR_DLL_INIT_FAILED ||
        retl == ERROR_BAD_NETPATH, /* some win2k */
        "RegConnectRegistryA failed err = %d\n", retl);
    if( !retl) RegCloseKey( hkey);

    retl = RegConnectRegistryA( netwName, HKEY_LOCAL_MACHINE, &hkey);
    ok( !retl ||
        retl == ERROR_DLL_INIT_FAILED ||
        retl == ERROR_BAD_NETPATH, /* some win2k */
        "RegConnectRegistryA failed err = %d\n", retl);
    if( !retl) RegCloseKey( hkey);

    SetLastError(0xdeadbeef);
    schnd = OpenSCManagerA( compName, NULL, GENERIC_READ); 
    if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("OpenSCManagerA is not implemented\n");
        return;
    }

    ok( schnd != NULL, "OpenSCManagerA failed err = %d\n", GetLastError());
    CloseServiceHandle( schnd);

    SetLastError(0xdeadbeef);
    schnd = OpenSCManagerA( netwName, NULL, GENERIC_READ); 
    ok( schnd != NULL, "OpenSCManagerA failed err = %d\n", GetLastError());
    CloseServiceHandle( schnd);

}

static void test_reg_query_value(void)
{
    HKEY subkey;
    CHAR val[MAX_PATH];
    WCHAR valW[5];
    LONG size, ret;

    static const WCHAR expected[] = {'d','a','t','a',0};

    ret = RegCreateKeyA(hkey_main, "subkey", &subkey);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", ret);

    ret = RegSetValueA(subkey, NULL, REG_SZ, "data", 4);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", ret);

    /* try an invalid hkey */
    SetLastError(0xdeadbeef);
    size = MAX_PATH;
    ret = RegQueryValueA((HKEY)0xcafebabe, "subkey", val, &size);
    ok(ret == ERROR_INVALID_HANDLE ||
       ret == ERROR_BADKEY || /* Windows 98 returns BADKEY */
       ret == ERROR_ACCESS_DENIED, /* non-admin winxp */
       "Expected ERROR_INVALID_HANDLE or ERROR_BADKEY, got %d\n", ret);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* try a NULL hkey */
    SetLastError(0xdeadbeef);
    size = MAX_PATH;
    ret = RegQueryValueA(NULL, "subkey", val, &size);
    ok(ret == ERROR_INVALID_HANDLE || ret == ERROR_BADKEY, /* Windows 98 returns BADKEY */
       "Expected ERROR_INVALID_HANDLE or ERROR_BADKEY, got %d\n", ret);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* try a NULL value */
    size = MAX_PATH;
    ret = RegQueryValueA(hkey_main, "subkey", NULL, &size);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", ret);
    ok(size == 5, "Expected 5, got %d\n", size);

    /* try a NULL size */
    SetLastError(0xdeadbeef);
    val[0] = '\0';
    ret = RegQueryValueA(hkey_main, "subkey", val, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", ret);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());
    ok(!val[0], "Expected val to be untouched, got %s\n", val);

    /* try a NULL value and size */
    ret = RegQueryValueA(hkey_main, "subkey", NULL, NULL);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", ret);

    /* try a size too small */
    SetLastError(0xdeadbeef);
    val[0] = '\0';
    size = 1;
    ret = RegQueryValueA(hkey_main, "subkey", val, &size);
    ok(ret == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", ret);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());
    ok(!val[0], "Expected val to be untouched, got %s\n", val);
    ok(size == 5, "Expected 5, got %d\n", size);

    /* successfully read the value using 'subkey' */
    size = MAX_PATH;
    ret = RegQueryValueA(hkey_main, "subkey", val, &size);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", ret);
    ok(!lstrcmpA(val, "data"), "Expected 'data', got '%s'\n", val);
    ok(size == 5, "Expected 5, got %d\n", size);

    /* successfully read the value using the subkey key */
    size = MAX_PATH;
    ret = RegQueryValueA(subkey, NULL, val, &size);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", ret);
    ok(!lstrcmpA(val, "data"), "Expected 'data', got '%s'\n", val);
    ok(size == 5, "Expected 5, got %d\n", size);

    /* unicode - try size too small */
    SetLastError(0xdeadbeef);
    valW[0] = '\0';
    size = 0;
    ret = RegQueryValueW(subkey, NULL, valW, &size);
    if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("RegQueryValueW is not implemented\n");
        goto cleanup;
    }
    ok(ret == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", ret);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());
    ok(!valW[0], "Expected valW to be untouched\n");
    ok(size == sizeof(expected), "Got wrong size: %d\n", size);

    /* unicode - try size in WCHARS */
    SetLastError(0xdeadbeef);
    size = sizeof(valW) / sizeof(WCHAR);
    ret = RegQueryValueW(subkey, NULL, valW, &size);
    ok(ret == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", ret);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());
    ok(!valW[0], "Expected valW to be untouched\n");
    ok(size == sizeof(expected), "Got wrong size: %d\n", size);

    /* unicode - successfully read the value */
    size = sizeof(valW);
    ret = RegQueryValueW(subkey, NULL, valW, &size);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", ret);
    ok(!lstrcmpW(valW, expected), "Got wrong value\n");
    ok(size == sizeof(expected), "Got wrong size: %d\n", size);

    /* unicode - set the value without a NULL terminator */
    ret = RegSetValueW(subkey, NULL, REG_SZ, expected, sizeof(expected)-sizeof(WCHAR));
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", ret);

    /* unicode - read the unterminated value, value is terminated for us */
    memset(valW, 'a', sizeof(valW));
    size = sizeof(valW);
    ret = RegQueryValueW(subkey, NULL, valW, &size);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", ret);
    ok(!lstrcmpW(valW, expected), "Got wrong value\n");
    ok(size == sizeof(expected), "Got wrong size: %d\n", size);

cleanup:
    RegDeleteKeyA(subkey, "");
    RegCloseKey(subkey);
}

static void test_string_termination(void)
{
    HKEY subkey;
    LSTATUS ret;
    static const char string[] = "FullString";
    char name[11];
    BYTE buffer[11];
    DWORD insize, outsize, nsize;

    ret = RegCreateKeyA(hkey_main, "string_termination", &subkey);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", ret);

    /* Off-by-one RegSetValueExA -> adds a trailing '\0'! */
    insize=sizeof(string)-1;
    ret = RegSetValueExA(subkey, "stringtest", 0, REG_SZ, (BYTE*)string, insize);
    ok(ret == ERROR_SUCCESS, "RegSetValueExA failed: %d\n", ret);
    outsize=insize;
    ret = RegQueryValueExA(subkey, "stringtest", NULL, NULL, buffer, &outsize);
    ok(ret == ERROR_MORE_DATA, "RegQueryValueExA returned: %d\n", ret);

    /* Off-by-two RegSetValueExA -> no trailing '\0' */
    insize=sizeof(string)-2;
    ret = RegSetValueExA(subkey, "stringtest", 0, REG_SZ, (BYTE*)string, insize);
    ok(ret == ERROR_SUCCESS, "RegSetValueExA failed: %d\n", ret);
    outsize=0;
    ret = RegQueryValueExA(subkey, "stringtest", NULL, NULL, NULL, &outsize);
    ok(ret == ERROR_SUCCESS, "RegQueryValueExA failed: %d\n", ret);
    ok(outsize == insize, "wrong size %u != %u\n", outsize, insize);

    /* RegQueryValueExA may return a string with no trailing '\0' */
    outsize=insize;
    memset(buffer, 0xbd, sizeof(buffer));
    ret = RegQueryValueExA(subkey, "stringtest", NULL, NULL, buffer, &outsize);
    ok(ret == ERROR_SUCCESS, "RegQueryValueExA failed: %d\n", ret);
    ok(outsize == insize, "wrong size: %u != %u\n", outsize, insize);
    ok(memcmp(buffer, string, outsize) == 0, "bad string: %s/%u != %s\n",
       wine_debugstr_an((char*)buffer, outsize), outsize, string);
    ok(buffer[insize] == 0xbd, "buffer overflow at %u %02x\n", insize, buffer[insize]);

    /* RegQueryValueExA adds a trailing '\0' if there is room */
    outsize=insize+1;
    memset(buffer, 0xbd, sizeof(buffer));
    ret = RegQueryValueExA(subkey, "stringtest", NULL, NULL, buffer, &outsize);
    ok(ret == ERROR_SUCCESS, "RegQueryValueExA failed: %d\n", ret);
    ok(outsize == insize, "wrong size: %u != %u\n", outsize, insize);
    ok(memcmp(buffer, string, outsize) == 0, "bad string: %s/%u != %s\n",
       wine_debugstr_an((char*)buffer, outsize), outsize, string);
    ok(buffer[insize] == 0, "buffer overflow at %u %02x\n", insize, buffer[insize]);

    /* RegEnumValueA may return a string with no trailing '\0' */
    outsize=insize;
    memset(buffer, 0xbd, sizeof(buffer));
    nsize=sizeof(name);
    ret = RegEnumValueA(subkey, 0, name, &nsize, NULL, NULL, buffer, &outsize);
    ok(ret == ERROR_SUCCESS, "RegEnumValueA failed: %d\n", ret);
    ok(strcmp(name, "stringtest") == 0, "wrong name: %s\n", name);
    ok(outsize == insize, "wrong size: %u != %u\n", outsize, insize);
    ok(memcmp(buffer, string, outsize) == 0, "bad string: %s/%u != %s\n",
       wine_debugstr_an((char*)buffer, outsize), outsize, string);
    ok(buffer[insize] == 0xbd, "buffer overflow at %u %02x\n", insize, buffer[insize]);

    /* RegEnumValueA adds a trailing '\0' if there is room */
    outsize=insize+1;
    memset(buffer, 0xbd, sizeof(buffer));
    nsize=sizeof(name);
    ret = RegEnumValueA(subkey, 0, name, &nsize, NULL, NULL, buffer, &outsize);
    ok(ret == ERROR_SUCCESS, "RegEnumValueA failed: %d\n", ret);
    ok(strcmp(name, "stringtest") == 0, "wrong name: %s\n", name);
    ok(outsize == insize, "wrong size: %u != %u\n", outsize, insize);
    ok(memcmp(buffer, string, outsize) == 0, "bad string: %s/%u != %s\n",
       wine_debugstr_an((char*)buffer, outsize), outsize, string);
    ok(buffer[insize] == 0, "buffer overflow at %u %02x\n", insize, buffer[insize]);

    RegDeleteKeyA(subkey, "");
    RegCloseKey(subkey);
}

static void test_reg_delete_tree(void)
{
    CHAR buffer[MAX_PATH];
    HKEY subkey, subkey2;
    LONG size, ret;

    if(!pRegDeleteTreeA) {
        win_skip("Skipping RegDeleteTreeA tests, function not present\n");
        return;
    }

    ret = RegCreateKeyA(hkey_main, "subkey", &subkey);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", ret);
    ret = RegCreateKeyA(subkey, "subkey2", &subkey2);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", ret);
    ret = RegSetValueA(subkey, NULL, REG_SZ, "data", 4);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", ret);
    ret = RegSetValueA(subkey2, NULL, REG_SZ, "data2", 5);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", ret);
    ret = RegCloseKey(subkey2);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", ret);

    ret = pRegDeleteTreeA(subkey, "subkey2");
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", ret);
    ok(RegOpenKeyA(subkey, "subkey2", &subkey2),
        "subkey2 was not deleted\n");
    size = MAX_PATH;
    ok(!RegQueryValueA(subkey, NULL, buffer, &size),
        "Default value of subkey not longer present\n");

    ret = RegCreateKeyA(subkey, "subkey2", &subkey2);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", ret);
    ret = RegCloseKey(subkey2);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", ret);
    ret = pRegDeleteTreeA(hkey_main, "subkey\\subkey2");
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", ret);
    ok(RegOpenKeyA(subkey, "subkey2", &subkey2),
        "subkey2 was not deleted\n");
    ok(!RegQueryValueA(subkey, NULL, buffer, &size),
        "Default value of subkey not longer present\n");

    ret = RegCreateKeyA(subkey, "subkey2", &subkey2);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", ret);
    ret = RegCloseKey(subkey2);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", ret);
    ret = RegCreateKeyA(subkey, "subkey3", &subkey2);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", ret);
    ret = RegCloseKey(subkey2);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", ret);
    ret = RegSetValueA(subkey, "value", REG_SZ, "data2", 5);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", ret);
    ret = pRegDeleteTreeA(subkey, NULL);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", ret);
    ok(!RegOpenKeyA(hkey_main, "subkey", &subkey),
        "subkey was deleted\n");
    ok(RegOpenKeyA(subkey, "subkey2", &subkey2),
        "subkey2 was not deleted\n");
    ok(RegOpenKeyA(subkey, "subkey3", &subkey2),
        "subkey3 was not deleted\n");
    size = MAX_PATH;
    ret = RegQueryValueA(subkey, NULL, buffer, &size);
    ok(ret == ERROR_SUCCESS,
        "Default value of subkey is not present\n");
    ok(!buffer[0], "Expected length 0 got length %u(%s)\n", lstrlenA(buffer), buffer);
    size = MAX_PATH;
    ok(RegQueryValueA(subkey, "value", buffer, &size),
        "Value is still present\n");

    ret = pRegDeleteTreeA(hkey_main, "not-here");
    ok(ret == ERROR_FILE_NOT_FOUND,
        "Expected ERROR_FILE_NOT_FOUND, got %d\n", ret);
}

static void test_rw_order(void)
{
    HKEY hKey;
    DWORD dw = 0;
    static const char keyname[] = "test_rw_order";
    char value_buf[2];
    DWORD values, value_len, value_name_max_len;
    LSTATUS ret;

    RegDeleteKeyA(HKEY_CURRENT_USER, keyname);
    ret = RegCreateKeyA(HKEY_CURRENT_USER, keyname, &hKey);
    if(ret != ERROR_SUCCESS) {
        skip("Couldn't create key. Skipping.\n");
        return;
    }

    ok(!RegSetValueExA(hKey, "A", 0, REG_DWORD, (LPBYTE)&dw, sizeof(dw)),
       "RegSetValueExA for value \"A\" failed\n");
    ok(!RegSetValueExA(hKey, "C", 0, REG_DWORD, (LPBYTE)&dw, sizeof(dw)),
       "RegSetValueExA for value \"C\" failed\n");
    ok(!RegSetValueExA(hKey, "D", 0, REG_DWORD, (LPBYTE)&dw, sizeof(dw)),
       "RegSetValueExA for value \"D\" failed\n");
    ok(!RegSetValueExA(hKey, "B", 0, REG_DWORD, (LPBYTE)&dw, sizeof(dw)),
       "RegSetValueExA for value \"B\" failed\n");

    ok(!RegQueryInfoKeyA(hKey, NULL, NULL, NULL, NULL, NULL, NULL, &values,
       &value_name_max_len, NULL, NULL, NULL), "RegQueryInfoKeyA failed\n");
    ok(values == 4, "Expected 4 values, got %u\n", values);

    /* Value enumeration preserves RegSetValueEx call order */
    value_len = 2;
    ok(!RegEnumValueA(hKey, 0, value_buf, &value_len, NULL, NULL, NULL, NULL), "RegEnumValueA failed\n");
    ok(strcmp(value_buf, "A") == 0, "Expected name \"A\", got %s\n", value_buf);
    value_len = 2;
    ok(!RegEnumValueA(hKey, 1, value_buf, &value_len, NULL, NULL, NULL, NULL), "RegEnumValueA failed\n");
    todo_wine ok(strcmp(value_buf, "C") == 0, "Expected name \"C\", got %s\n", value_buf);
    value_len = 2;
    ok(!RegEnumValueA(hKey, 2, value_buf, &value_len, NULL, NULL, NULL, NULL), "RegEnumValueA failed\n");
    todo_wine ok(strcmp(value_buf, "D") == 0, "Expected name \"D\", got %s\n", value_buf);
    value_len = 2;
    ok(!RegEnumValueA(hKey, 3, value_buf, &value_len, NULL, NULL, NULL, NULL), "RegEnumValueA failed\n");
    todo_wine ok(strcmp(value_buf, "B") == 0, "Expected name \"B\", got %s\n", value_buf);

    ok(!RegDeleteKeyA(HKEY_CURRENT_USER, keyname), "Failed to delete key\n");
}

static void test_symlinks(void)
{
    static const WCHAR targetW[] = {'\\','S','o','f','t','w','a','r','e','\\','W','i','n','e',
                                    '\\','T','e','s','t','\\','t','a','r','g','e','t',0};
    BYTE buffer[1024];
    UNICODE_STRING target_str;
    WCHAR *target;
    HKEY key, link;
    NTSTATUS status;
    DWORD target_len, type, len, dw, err;

    if (!pRtlFormatCurrentUserKeyPath || !pNtDeleteKey)
    {
        win_skip( "Can't perform symlink tests\n" );
        return;
    }

    pRtlFormatCurrentUserKeyPath( &target_str );

    target_len = target_str.Length + sizeof(targetW);
    target = HeapAlloc( GetProcessHeap(), 0, target_len );
    memcpy( target, target_str.Buffer, target_str.Length );
    memcpy( target + target_str.Length/sizeof(WCHAR), targetW, sizeof(targetW) );

    err = RegCreateKeyExA( hkey_main, "link", 0, NULL, REG_OPTION_CREATE_LINK,
                           KEY_ALL_ACCESS, NULL, &link, NULL );
    ok( err == ERROR_SUCCESS, "RegCreateKeyEx failed: %u\n", err );

    /* REG_SZ is not allowed */
    err = RegSetValueExA( link, "SymbolicLinkValue", 0, REG_SZ, (BYTE *)"foobar", sizeof("foobar") );
    ok( err == ERROR_ACCESS_DENIED, "RegSetValueEx wrong error %u\n", err );
    err = RegSetValueExA( link, "SymbolicLinkValue", 0, REG_LINK,
                          (BYTE *)target, target_len - sizeof(WCHAR) );
    ok( err == ERROR_SUCCESS, "RegSetValueEx failed error %u\n", err );
    /* other values are not allowed */
    err = RegSetValueExA( link, "link", 0, REG_LINK, (BYTE *)target, target_len - sizeof(WCHAR) );
    ok( err == ERROR_ACCESS_DENIED, "RegSetValueEx wrong error %u\n", err );

    /* try opening the target through the link */

    err = RegOpenKeyA( hkey_main, "link", &key );
    ok( err == ERROR_FILE_NOT_FOUND, "RegOpenKey wrong error %u\n", err );

    err = RegCreateKeyExA( hkey_main, "target", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &key, NULL );
    ok( err == ERROR_SUCCESS, "RegCreateKeyEx failed error %u\n", err );

    dw = 0xbeef;
    err = RegSetValueExA( key, "value", 0, REG_DWORD, (BYTE *)&dw, sizeof(dw) );
    ok( err == ERROR_SUCCESS, "RegSetValueEx failed error %u\n", err );
    RegCloseKey( key );

    err = RegOpenKeyA( hkey_main, "link", &key );
    ok( err == ERROR_SUCCESS, "RegOpenKey failed error %u\n", err );

    len = sizeof(buffer);
    err = RegQueryValueExA( key, "value", NULL, &type, buffer, &len );
    ok( err == ERROR_SUCCESS, "RegOpenKey failed error %u\n", err );
    ok( len == sizeof(DWORD), "wrong len %u\n", len );

    len = sizeof(buffer);
    err = RegQueryValueExA( key, "SymbolicLinkValue", NULL, &type, buffer, &len );
    ok( err == ERROR_FILE_NOT_FOUND, "RegQueryValueEx wrong error %u\n", err );

    /* REG_LINK can be created in non-link keys */
    err = RegSetValueExA( key, "SymbolicLinkValue", 0, REG_LINK,
                          (BYTE *)target, target_len - sizeof(WCHAR) );
    ok( err == ERROR_SUCCESS, "RegSetValueEx failed error %u\n", err );
    len = sizeof(buffer);
    err = RegQueryValueExA( key, "SymbolicLinkValue", NULL, &type, buffer, &len );
    ok( err == ERROR_SUCCESS, "RegQueryValueEx failed error %u\n", err );
    ok( len == target_len - sizeof(WCHAR), "wrong len %u\n", len );
    err = RegDeleteValueA( key, "SymbolicLinkValue" );
    ok( err == ERROR_SUCCESS, "RegDeleteValue failed error %u\n", err );

    RegCloseKey( key );

    err = RegCreateKeyExA( hkey_main, "link", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &key, NULL );
    ok( err == ERROR_SUCCESS, "RegCreateKeyEx failed error %u\n", err );

    len = sizeof(buffer);
    err = RegQueryValueExA( key, "value", NULL, &type, buffer, &len );
    ok( err == ERROR_SUCCESS, "RegQueryValueEx failed error %u\n", err );
    ok( len == sizeof(DWORD), "wrong len %u\n", len );

    err = RegQueryValueExA( key, "SymbolicLinkValue", NULL, &type, buffer, &len );
    ok( err == ERROR_FILE_NOT_FOUND, "RegQueryValueEx wrong error %u\n", err );
    RegCloseKey( key );

    /* now open the symlink itself */

    err = RegOpenKeyExA( hkey_main, "link", REG_OPTION_OPEN_LINK, KEY_ALL_ACCESS, &key );
    ok( err == ERROR_SUCCESS, "RegOpenKeyEx failed error %u\n", err );
    len = sizeof(buffer);
    err = RegQueryValueExA( key, "SymbolicLinkValue", NULL, &type, buffer, &len );
    ok( err == ERROR_SUCCESS, "RegQueryValueEx failed error %u\n", err );
    ok( len == target_len - sizeof(WCHAR), "wrong len %u\n", len );
    RegCloseKey( key );

    err = RegCreateKeyExA( hkey_main, "link", 0, NULL, REG_OPTION_OPEN_LINK,
                           KEY_ALL_ACCESS, NULL, &key, NULL );
    ok( err == ERROR_SUCCESS, "RegCreateKeyEx failed error %u\n", err );
    len = sizeof(buffer);
    err = RegQueryValueExA( key, "SymbolicLinkValue", NULL, &type, buffer, &len );
    ok( err == ERROR_SUCCESS, "RegQueryValueEx failed error %u\n", err );
    ok( len == target_len - sizeof(WCHAR), "wrong len %u\n", len );
    RegCloseKey( key );

    err = RegCreateKeyExA( hkey_main, "link", 0, NULL, REG_OPTION_CREATE_LINK,
                           KEY_ALL_ACCESS, NULL, &key, NULL );
    ok( err == ERROR_ALREADY_EXISTS, "RegCreateKeyEx wrong error %u\n", err );

    err = RegCreateKeyExA( hkey_main, "link", 0, NULL, REG_OPTION_CREATE_LINK | REG_OPTION_OPEN_LINK,
                           KEY_ALL_ACCESS, NULL, &key, NULL );
    ok( err == ERROR_ALREADY_EXISTS, "RegCreateKeyEx wrong error %u\n", err );

    err = RegDeleteKeyA( hkey_main, "target" );
    ok( err == ERROR_SUCCESS, "RegDeleteKey failed error %u\n", err );

    err = RegDeleteKeyA( hkey_main, "link" );
    ok( err == ERROR_FILE_NOT_FOUND, "RegDeleteKey wrong error %u\n", err );

    status = pNtDeleteKey( link );
    ok( !status, "NtDeleteKey failed: 0x%08x\n", status );
    RegCloseKey( link );

    HeapFree( GetProcessHeap(), 0, target );
    pRtlFreeUnicodeString( &target_str );
}

static DWORD get_key_value( HKEY root, const char *name, DWORD flags )
{
    HKEY key;
    DWORD err, type, dw, len = sizeof(dw);

    err = RegCreateKeyExA( root, name, 0, NULL, 0, flags | KEY_ALL_ACCESS, NULL, &key, NULL );
    if (err == ERROR_FILE_NOT_FOUND) return 0;
    ok( err == ERROR_SUCCESS, "%08x: RegCreateKeyEx failed: %u\n", flags, err );

    err = RegQueryValueExA( key, "value", NULL, &type, (BYTE *)&dw, &len );
    if (err == ERROR_FILE_NOT_FOUND)
        dw = 0;
    else
        ok( err == ERROR_SUCCESS, "%08x: RegQueryValueEx failed: %u\n", flags, err );
    RegCloseKey( key );
    return dw;
}

static void _check_key_value( int line, HANDLE root, const char *name, DWORD flags, DWORD expect )
{
    DWORD dw = get_key_value( root, name, flags );
    ok_(__FILE__,line)( dw == expect, "%08x: wrong value %u/%u\n", flags, dw, expect );
}
#define check_key_value(root,name,flags,expect) _check_key_value( __LINE__, root, name, flags, expect )

static void test_redirection(void)
{
    DWORD err, type, dw, len;
    HKEY key, root32, root64, key32, key64, native, op_key;
    BOOL is_vista = FALSE;
    REGSAM opposite = (sizeof(void*) == 8 ? KEY_WOW64_32KEY : KEY_WOW64_64KEY);

    if (ptr_size != 64)
    {
        BOOL is_wow64;
        if (!pIsWow64Process || !pIsWow64Process( GetCurrentProcess(), &is_wow64 ) || !is_wow64)
        {
            skip( "Not on Wow64, no redirection\n" );
            return;
        }
    }

    err = RegCreateKeyExA( HKEY_LOCAL_MACHINE, "Software\\Wine", 0, NULL, 0,
                           KEY_WOW64_64KEY | KEY_ALL_ACCESS, NULL, &root64, NULL );
    ok( err == ERROR_SUCCESS, "RegCreateKeyExA failed: %u\n", err );

    err = RegCreateKeyExA( HKEY_LOCAL_MACHINE, "Software\\Wine", 0, NULL, 0,
                           KEY_WOW64_32KEY | KEY_ALL_ACCESS, NULL, &root32, NULL );
    ok( err == ERROR_SUCCESS, "RegCreateKeyExA failed: %u\n", err );

    err = RegCreateKeyExA( HKEY_LOCAL_MACHINE, "Software\\Wine\\Winetest", 0, NULL, 0,
                           KEY_WOW64_64KEY | KEY_ALL_ACCESS, NULL, &key64, NULL );
    ok( err == ERROR_SUCCESS, "RegCreateKeyExA failed: %u\n", err );

    err = RegCreateKeyExA( HKEY_LOCAL_MACHINE, "Software\\Wine\\Winetest", 0, NULL, 0,
                           KEY_WOW64_32KEY | KEY_ALL_ACCESS, NULL, &key32, NULL );
    ok( err == ERROR_SUCCESS, "RegCreateKeyExA failed: %u\n", err );

    dw = 64;
    err = RegSetValueExA( key64, "value", 0, REG_DWORD, (BYTE *)&dw, sizeof(dw) );
    ok( err == ERROR_SUCCESS, "RegSetValueExA failed: %u\n", err );

    dw = 32;
    err = RegSetValueExA( key32, "value", 0, REG_DWORD, (BYTE *)&dw, sizeof(dw) );
    ok( err == ERROR_SUCCESS, "RegSetValueExA failed: %u\n", err );

    dw = 0;
    len = sizeof(dw);
    err = RegQueryValueExA( key32, "value", NULL, &type, (BYTE *)&dw, &len );
    ok( err == ERROR_SUCCESS, "RegQueryValueExA failed: %u\n", err );
    ok( dw == 32, "wrong value %u\n", dw );

    dw = 0;
    len = sizeof(dw);
    err = RegQueryValueExA( key64, "value", NULL, &type, (BYTE *)&dw, &len );
    ok( err == ERROR_SUCCESS, "RegQueryValueExA failed: %u\n", err );
    ok( dw == 64, "wrong value %u\n", dw );

    err = RegCreateKeyExA( HKEY_LOCAL_MACHINE, "Software", 0, NULL, 0,
                           KEY_ALL_ACCESS, NULL, &key, NULL );
    ok( err == ERROR_SUCCESS, "RegCreateKeyExA failed: %u\n", err );

    if (ptr_size == 32)
    {
        /* the Vista mechanism allows opening Wow6432Node from a 32-bit key too */
        /* the new (and simpler) Win7 mechanism doesn't */
        if (get_key_value( key, "Wow6432Node\\Wine\\Winetest", 0 ) == 32)
        {
            trace( "using Vista-style Wow6432Node handling\n" );
            is_vista = TRUE;
        }
        check_key_value( key, "Wine\\Winetest", 0, 32 );
        check_key_value( key, "Wine\\Winetest", KEY_WOW64_64KEY, is_vista ? 64 : 32 );
        check_key_value( key, "Wine\\Winetest", KEY_WOW64_32KEY, 32 );
        check_key_value( key, "Wow6432Node\\Wine\\Winetest", 0, is_vista ? 32 : 0 );
        check_key_value( key, "Wow6432Node\\Wine\\Winetest", KEY_WOW64_64KEY, is_vista ? 64 : 0 );
        check_key_value( key, "Wow6432Node\\Wine\\Winetest", KEY_WOW64_32KEY, is_vista ? 32 : 0 );
    }
    else
    {
        if (get_key_value( HKEY_LOCAL_MACHINE, "Software\\Wow6432Node\\Wine\\Winetest", KEY_WOW64_64KEY ) == 64)
        {
            trace( "using Vista-style Wow6432Node handling\n" );
            is_vista = TRUE;
        }
        check_key_value( key, "Wine\\Winetest", 0, 64 );
        check_key_value( key, "Wow6432Node\\Wine\\Winetest", 0, 32 );
    }
    RegCloseKey( key );

    if (ptr_size == 32)
    {
        err = RegCreateKeyExA( HKEY_LOCAL_MACHINE, "Software", 0, NULL, 0,
                               KEY_WOW64_64KEY | KEY_ALL_ACCESS, NULL, &key, NULL );
        ok( err == ERROR_SUCCESS, "RegCreateKeyExA failed: %u\n", err );
        dw = get_key_value( key, "Wine\\Winetest", 0 );
        ok( dw == 64 || broken(dw == 32) /* xp64 */, "wrong value %u\n", dw );
        check_key_value( key, "Wine\\Winetest", KEY_WOW64_64KEY, 64 );
        check_key_value( key, "Wine\\Winetest", KEY_WOW64_32KEY, 32 );
        check_key_value( key, "Wow6432Node\\Wine\\Winetest", 0, 32 );
        dw = get_key_value( key, "Wow6432Node\\Wine\\Winetest", KEY_WOW64_64KEY );
        ok( dw == 32 || broken(dw == 64) /* xp64 */, "wrong value %u\n", dw );
        check_key_value( key, "Wow6432Node\\Wine\\Winetest", KEY_WOW64_32KEY, 32 );
        RegCloseKey( key );

        err = RegCreateKeyExA( HKEY_LOCAL_MACHINE, "Software", 0, NULL, 0,
                               KEY_WOW64_32KEY | KEY_ALL_ACCESS, NULL, &key, NULL );
        ok( err == ERROR_SUCCESS, "RegCreateKeyExA failed: %u\n", err );
        check_key_value( key, "Wine\\Winetest", 0, 32 );
        check_key_value( key, "Wine\\Winetest", KEY_WOW64_64KEY, is_vista ? 64 : 32 );
        check_key_value( key, "Wine\\Winetest", KEY_WOW64_32KEY, 32 );
        check_key_value( key, "Wow6432Node\\Wine\\Winetest", 0, is_vista ? 32 : 0 );
        check_key_value( key, "Wow6432Node\\Wine\\Winetest", KEY_WOW64_64KEY, is_vista ? 64 : 0 );
        check_key_value( key, "Wow6432Node\\Wine\\Winetest", KEY_WOW64_32KEY, is_vista ? 32 : 0 );
        RegCloseKey( key );
    }
    else
    {
        err = RegCreateKeyExA( HKEY_LOCAL_MACHINE, "Software", 0, NULL, 0,
                               KEY_WOW64_64KEY | KEY_ALL_ACCESS, NULL, &key, NULL );
        ok( err == ERROR_SUCCESS, "RegCreateKeyExA failed: %u\n", err );
        check_key_value( key, "Wine\\Winetest", 0, 64 );
        check_key_value( key, "Wine\\Winetest", KEY_WOW64_64KEY, 64 );
        dw = get_key_value( key, "Wine\\Winetest", KEY_WOW64_32KEY );
        todo_wine ok( dw == 32, "wrong value %u\n", dw );
        check_key_value( key, "Wow6432Node\\Wine\\Winetest", 0, 32 );
        RegCloseKey( key );

        err = RegCreateKeyExA( HKEY_LOCAL_MACHINE, "Software", 0, NULL, 0,
                               KEY_WOW64_32KEY | KEY_ALL_ACCESS, NULL, &key, NULL );
        ok( err == ERROR_SUCCESS, "RegCreateKeyExA failed: %u\n", err );
        check_key_value( key, "Wine\\Winetest", 0, 32 );
        dw = get_key_value( key, "Wine\\Winetest", KEY_WOW64_64KEY );
        ok( dw == 32 || broken(dw == 64) /* vista */, "wrong value %u\n", dw );
        check_key_value( key, "Wine\\Winetest", KEY_WOW64_32KEY, 32 );
        RegCloseKey( key );
    }

    check_key_value( HKEY_LOCAL_MACHINE, "Software\\Wine\\Winetest", 0, ptr_size );
    check_key_value( HKEY_LOCAL_MACHINE, "Software\\Wow6432Node\\Wine\\Winetest", 0, 32 );
    if (ptr_size == 64)
    {
        /* KEY_WOW64 flags have no effect on 64-bit */
        check_key_value( HKEY_LOCAL_MACHINE, "Software\\Wine\\Winetest", KEY_WOW64_64KEY, 64 );
        check_key_value( HKEY_LOCAL_MACHINE, "Software\\Wine\\Winetest", KEY_WOW64_32KEY, 32 );
        check_key_value( HKEY_LOCAL_MACHINE, "Software\\Wow6432Node\\Wine\\Winetest", KEY_WOW64_64KEY, is_vista ? 64 : 32 );
        check_key_value( HKEY_LOCAL_MACHINE, "Software\\Wow6432Node\\Wine\\Winetest", KEY_WOW64_32KEY, 32 );
    }
    else
    {
        check_key_value( HKEY_LOCAL_MACHINE, "Software\\Wine\\Winetest", KEY_WOW64_64KEY, 64 );
        check_key_value( HKEY_LOCAL_MACHINE, "Software\\Wine\\Winetest", KEY_WOW64_32KEY, 32 );
        check_key_value( HKEY_LOCAL_MACHINE, "Software\\Wow6432Node\\Wine\\Winetest", KEY_WOW64_64KEY, is_vista ? 64 : 32 );
        check_key_value( HKEY_LOCAL_MACHINE, "Software\\Wow6432Node\\Wine\\Winetest", KEY_WOW64_32KEY, 32 );
    }

    err = RegCreateKeyExA( HKEY_LOCAL_MACHINE, "Software\\Wow6432Node", 0, NULL, 0,
                           KEY_ALL_ACCESS, NULL, &key, NULL );
    ok( err == ERROR_SUCCESS, "RegCreateKeyExA failed: %u\n", err );
    check_key_value( key, "Wine\\Winetest", 0, 32 );
    check_key_value( key, "Wine\\Winetest", KEY_WOW64_64KEY, is_vista ? 64 : 32 );
    check_key_value( key, "Wine\\Winetest", KEY_WOW64_32KEY, 32 );
    RegCloseKey( key );

    if (ptr_size == 32)
    {
        err = RegCreateKeyExA( HKEY_LOCAL_MACHINE, "Software\\Wow6432Node", 0, NULL, 0,
                               KEY_WOW64_64KEY | KEY_ALL_ACCESS, NULL, &key, NULL );
        ok( err == ERROR_SUCCESS, "RegCreateKeyExA failed: %u\n", err );
        dw = get_key_value( key, "Wine\\Winetest", 0 );
        ok( dw == (is_vista ? 64 : 32) || broken(dw == 32) /* xp64 */, "wrong value %u\n", dw );
        check_key_value( key, "Wine\\Winetest", KEY_WOW64_64KEY, is_vista ? 64 : 32 );
        check_key_value( key, "Wine\\Winetest", KEY_WOW64_32KEY, 32 );
        RegCloseKey( key );

        err = RegCreateKeyExA( HKEY_LOCAL_MACHINE, "Software\\Wow6432Node", 0, NULL, 0,
                               KEY_WOW64_32KEY | KEY_ALL_ACCESS, NULL, &key, NULL );
        ok( err == ERROR_SUCCESS, "RegCreateKeyExA failed: %u\n", err );
        check_key_value( key, "Wine\\Winetest", 0, 32 );
        check_key_value( key, "Wine\\Winetest", KEY_WOW64_64KEY, is_vista ? 64 : 32 );
        check_key_value( key, "Wine\\Winetest", KEY_WOW64_32KEY, 32 );
        RegCloseKey( key );
    }

    err = RegCreateKeyExA( HKEY_LOCAL_MACHINE, "Software\\Wow6432Node\\Wine", 0, NULL, 0,
                           KEY_ALL_ACCESS, NULL, &key, NULL );
    ok( err == ERROR_SUCCESS, "RegCreateKeyExA failed: %u\n", err );
    check_key_value( key, "Winetest", 0, 32 );
    check_key_value( key, "Winetest", KEY_WOW64_64KEY, is_vista ? 64 : 32 );
    check_key_value( key, "Winetest", KEY_WOW64_32KEY, 32 );
    RegCloseKey( key );

    if (ptr_size == 32)
    {
        err = RegCreateKeyExA( HKEY_LOCAL_MACHINE, "Software\\Wow6432Node\\Wine", 0, NULL, 0,
                               KEY_WOW64_64KEY | KEY_ALL_ACCESS, NULL, &key, NULL );
        ok( err == ERROR_SUCCESS, "RegCreateKeyExA failed: %u\n", err );
        dw = get_key_value( key, "Winetest", 0 );
        ok( dw == 32 || (is_vista && dw == 64), "wrong value %u\n", dw );
        check_key_value( key, "Winetest", KEY_WOW64_64KEY, is_vista ? 64 : 32 );
        check_key_value( key, "Winetest", KEY_WOW64_32KEY, 32 );
        RegCloseKey( key );

        err = RegCreateKeyExA( HKEY_LOCAL_MACHINE, "Software\\Wow6432Node\\Wine", 0, NULL, 0,
                               KEY_WOW64_32KEY | KEY_ALL_ACCESS, NULL, &key, NULL );
        ok( err == ERROR_SUCCESS, "RegCreateKeyExA failed: %u\n", err );
        check_key_value( key, "Winetest", 0, 32 );
        check_key_value( key, "Winetest", KEY_WOW64_64KEY, is_vista ? 64 : 32 );
        check_key_value( key, "Winetest", KEY_WOW64_32KEY, 32 );
        RegCloseKey( key );
    }

    err = RegCreateKeyExA( HKEY_LOCAL_MACHINE, "Software\\Wine", 0, NULL, 0,
                           KEY_ALL_ACCESS, NULL, &key, NULL );
    ok( err == ERROR_SUCCESS, "RegCreateKeyExA failed: %u\n", err );
    check_key_value( key, "Winetest", 0, ptr_size );
    check_key_value( key, "Winetest", KEY_WOW64_64KEY, is_vista ? 64 : ptr_size );
    dw = get_key_value( key, "Winetest", KEY_WOW64_32KEY );
    if (ptr_size == 32) ok( dw == 32, "wrong value %u\n", dw );
    else todo_wine ok( dw == 32, "wrong value %u\n", dw );
    RegCloseKey( key );

    if (ptr_size == 32)
    {
        err = RegCreateKeyExA( HKEY_LOCAL_MACHINE, "Software\\Wine", 0, NULL, 0,
                               KEY_WOW64_64KEY | KEY_ALL_ACCESS, NULL, &key, NULL );
        ok( err == ERROR_SUCCESS, "RegCreateKeyExA failed: %u\n", err );
        dw = get_key_value( key, "Winetest", 0 );
        ok( dw == 64 || broken(dw == 32) /* xp64 */, "wrong value %u\n", dw );
        check_key_value( key, "Winetest", KEY_WOW64_64KEY, 64 );
        dw = get_key_value( key, "Winetest", KEY_WOW64_32KEY );
        todo_wine ok( dw == 32, "wrong value %u\n", dw );
        RegCloseKey( key );

        err = RegCreateKeyExA( HKEY_LOCAL_MACHINE, "Software\\Wine", 0, NULL, 0,
                               KEY_WOW64_32KEY | KEY_ALL_ACCESS, NULL, &key, NULL );
        ok( err == ERROR_SUCCESS, "RegCreateKeyExA failed: %u\n", err );
        check_key_value( key, "Winetest", 0, 32 );
        check_key_value( key, "Winetest", KEY_WOW64_64KEY, is_vista ? 64 : 32 );
        check_key_value( key, "Winetest", KEY_WOW64_32KEY, 32 );
        RegCloseKey( key );
    }

    if (pRegDeleteKeyExA)
    {
        err = pRegDeleteKeyExA( key32, "", KEY_WOW64_32KEY, 0 );
        ok( err == ERROR_SUCCESS, "RegDeleteKey failed: %u\n", err );
        err = pRegDeleteKeyExA( key64, "", KEY_WOW64_64KEY, 0 );
        ok( err == ERROR_SUCCESS, "RegDeleteKey failed: %u\n", err );
        pRegDeleteKeyExA( key64, "", KEY_WOW64_64KEY, 0 );
        pRegDeleteKeyExA( root64, "", KEY_WOW64_64KEY, 0 );
    }
    else
    {
        err = RegDeleteKeyA( key32, "" );
        ok( err == ERROR_SUCCESS, "RegDeleteKey failed: %u\n", err );
        err = RegDeleteKeyA( key64, "" );
        ok( err == ERROR_SUCCESS, "RegDeleteKey failed: %u\n", err );
        RegDeleteKeyA( key64, "" );
        RegDeleteKeyA( root64, "" );
    }
    RegCloseKey( key32 );
    RegCloseKey( key64 );
    RegCloseKey( root32 );
    RegCloseKey( root64 );

    /* open key in native bit mode */
    err = RegOpenKeyExA(HKEY_CLASSES_ROOT, "Interface", 0, KEY_ALL_ACCESS, &native);
    ok(err == ERROR_SUCCESS, "got %i\n", err);

    pRegDeleteKeyExA(native, "AWineTest", 0, 0);

    /* write subkey in opposite bit mode */
    err = RegOpenKeyExA(HKEY_CLASSES_ROOT, "Interface", 0, KEY_ALL_ACCESS | opposite, &op_key);
    ok(err == ERROR_SUCCESS, "got %i\n", err);

    err = RegCreateKeyExA(op_key, "AWineTest", 0, NULL, 0, KEY_ALL_ACCESS | opposite,
            NULL, &key, NULL);
    ok(err == ERROR_SUCCESS || err == ERROR_ACCESS_DENIED, "got %i\n", err);
    if(err != ERROR_SUCCESS){
        win_skip("Can't write to registry\n");
        RegCloseKey(op_key);
        RegCloseKey(native);
        return;
    }
    RegCloseKey(key);

    /* verify subkey is not present in native mode */
    err = RegOpenKeyExA(native, "AWineTest", 0, KEY_ALL_ACCESS, &key);
    ok(err == ERROR_FILE_NOT_FOUND ||
            broken(err == ERROR_SUCCESS), /* before Win7, HKCR is reflected instead of redirected */
            "got %i\n", err);

    err = pRegDeleteKeyExA(op_key, "AWineTest", opposite, 0);
    ok(err == ERROR_SUCCESS, "got %i\n", err);

    RegCloseKey(op_key);
    RegCloseKey(native);
}

static void test_classesroot(void)
{
    HKEY hkey, hklm, hkcr, hkeysub1, hklmsub1, hkcrsub1, hklmsub2, hkcrsub2;
    DWORD size = 8;
    DWORD type = REG_SZ;
    static CHAR buffer[8];
    LONG res;

    /* create a key in the user's classes */
    if (!RegOpenKeyA( HKEY_CURRENT_USER, "Software\\Classes\\WineTestCls", &hkey ))
    {
        delete_key( hkey );
        RegCloseKey( hkey );
    }
    res = RegCreateKeyExA( HKEY_CURRENT_USER, "Software\\Classes\\WineTestCls", 0, NULL, 0,
                           KEY_QUERY_VALUE|KEY_SET_VALUE, NULL, &hkey, NULL );
    if (res == ERROR_ACCESS_DENIED)
    {
        skip("not enough privileges to add a user class\n");
        return;
    }
    ok(!IS_HKCR(hkey), "hkcr mask set in %p\n", hkey);

    /* try to open that key in hkcr */
    res = RegOpenKeyExA( HKEY_CLASSES_ROOT, "WineTestCls", 0,
                         KEY_QUERY_VALUE|KEY_SET_VALUE, &hkcr );
    todo_wine ok(res == ERROR_SUCCESS ||
                 broken(res == ERROR_FILE_NOT_FOUND /* WinNT */),
                 "test key not found in hkcr: %d\n", res);
    if (res)
    {
        skip("HKCR key merging not supported\n");
        delete_key( hkey );
        RegCloseKey( hkey );
        return;
    }

    todo_wine ok(IS_HKCR(hkcr), "hkcr mask not set in %p\n", hkcr);

    /* set a value in user's classes */
    res = RegSetValueExA(hkey, "val1", 0, REG_SZ, (const BYTE *)"user", sizeof("user"));
    ok(res == ERROR_SUCCESS, "RegSetValueExA failed: %d, GLE=%x\n", res, GetLastError());

    /* try to find the value in hkcr */
    res = RegQueryValueExA(hkcr, "val1", NULL, &type, (LPBYTE)buffer, &size);
    ok(res == ERROR_SUCCESS, "RegQueryValueExA failed: %d\n", res);
    ok(!strcmp( buffer, "user" ), "value set to '%s'\n", buffer );

    /* modify the value in hkcr */
    res = RegSetValueExA(hkcr, "val1", 0, REG_SZ, (const BYTE *)"hkcr", sizeof("hkcr"));
    ok(res == ERROR_SUCCESS, "RegSetValueExA failed: %d, GLE=%x\n", res, GetLastError());

    /* check if the value is also modified in user's classes */
    res = RegQueryValueExA(hkey, "val1", NULL, &type, (LPBYTE)buffer, &size);
    ok(res == ERROR_SUCCESS, "RegQueryValueExA failed: %d, GLE=%x\n", res, GetLastError());
    ok(!strcmp( buffer, "hkcr" ), "value set to '%s'\n", buffer );

    /* set a value in hkcr */
    res = RegSetValueExA(hkcr, "val0", 0, REG_SZ, (const BYTE *)"hkcr", sizeof("hkcr"));
    ok(res == ERROR_SUCCESS, "RegSetValueExA failed: %d, GLE=%x\n", res, GetLastError());

    /* try to find the value in user's classes */
    res = RegQueryValueExA(hkey, "val0", NULL, &type, (LPBYTE)buffer, &size);
    ok(res == ERROR_SUCCESS, "RegQueryValueExA failed: %d\n", res);
    ok(!strcmp( buffer, "hkcr" ), "value set to '%s'\n", buffer );

    /* modify the value in user's classes */
    res = RegSetValueExA(hkey, "val0", 0, REG_SZ, (const BYTE *)"user", sizeof("user"));
    ok(res == ERROR_SUCCESS, "RegSetValueExA failed: %d, GLE=%x\n", res, GetLastError());

    /* check if the value is also modified in hkcr */
    res = RegQueryValueExA(hkcr, "val0", NULL, &type, (LPBYTE)buffer, &size);
    ok(res == ERROR_SUCCESS, "RegQueryValueExA failed: %d, GLE=%x\n", res, GetLastError());
    ok(!strcmp( buffer, "user" ), "value set to '%s'\n", buffer );

    /* cleanup */
    delete_key( hkey );
    delete_key( hkcr );
    RegCloseKey( hkey );
    RegCloseKey( hkcr );

    /* create a key in the hklm classes */
    if (!RegOpenKeyA( HKEY_LOCAL_MACHINE, "Software\\Classes\\WineTestCls", &hklm ))
    {
        delete_key( hklm );
        RegCloseKey( hklm );
    }
    res = RegCreateKeyExA( HKEY_LOCAL_MACHINE, "Software\\Classes\\WineTestCls", 0, NULL, REG_OPTION_NON_VOLATILE,
                           KEY_ALL_ACCESS, NULL, &hklm, NULL );
    if (res == ERROR_ACCESS_DENIED)
    {
        skip("not enough privileges to add a system class\n");
        return;
    }
    ok(!IS_HKCR(hklm), "hkcr mask set in %p\n", hklm);

    /* try to open that key in hkcr */
    res = RegOpenKeyExA( HKEY_CLASSES_ROOT, "WineTestCls", 0,
                         KEY_QUERY_VALUE|KEY_SET_VALUE, &hkcr );
    ok(res == ERROR_SUCCESS,
       "test key not found in hkcr: %d\n", res);
    ok(IS_HKCR(hkcr), "hkcr mask not set in %p\n", hkcr);
    if (res)
    {
        delete_key( hklm );
        RegCloseKey( hklm );
        return;
    }

    /* set a value in hklm classes */
    res = RegSetValueExA(hklm, "val2", 0, REG_SZ, (const BYTE *)"hklm", sizeof("hklm"));
    ok(res == ERROR_SUCCESS, "RegSetValueExA failed: %d, GLE=%x\n", res, GetLastError());

    /* try to find the value in hkcr */
    res = RegQueryValueExA(hkcr, "val2", NULL, &type, (LPBYTE)buffer, &size);
    ok(res == ERROR_SUCCESS, "RegQueryValueExA failed: %d\n", res);
    ok(!strcmp( buffer, "hklm" ), "value set to '%s'\n", buffer );

    /* modify the value in hkcr */
    res = RegSetValueExA(hkcr, "val2", 0, REG_SZ, (const BYTE *)"hkcr", sizeof("hkcr"));
    ok(res == ERROR_SUCCESS, "RegSetValueExA failed: %d, GLE=%x\n", res, GetLastError());

    /* check that the value is modified in hklm classes */
    res = RegQueryValueExA(hklm, "val2", NULL, &type, (LPBYTE)buffer, &size);
    ok(res == ERROR_SUCCESS, "RegQueryValueExA failed: %d, GLE=%x\n", res, GetLastError());
    ok(!strcmp( buffer, "hkcr" ), "value set to '%s'\n", buffer );

    if (RegCreateKeyExA( HKEY_CURRENT_USER, "Software\\Classes\\WineTestCls", 0, NULL, 0,
                         KEY_QUERY_VALUE|KEY_SET_VALUE, NULL, &hkey, NULL )) return;
    ok(!IS_HKCR(hkey), "hkcr mask set in %p\n", hkey);

    /* try to open that key in hkcr */
    res = RegOpenKeyExA( HKEY_CLASSES_ROOT, "WineTestCls", 0,
                         KEY_QUERY_VALUE|KEY_SET_VALUE, &hkcr );
    ok(res == ERROR_SUCCESS,
       "test key not found in hkcr: %d\n", res);
    ok(IS_HKCR(hkcr), "hkcr mask not set in %p\n", hkcr);

    /* set a value in user's classes */
    res = RegSetValueExA(hkey, "val2", 0, REG_SZ, (const BYTE *)"user", sizeof("user"));
    ok(res == ERROR_SUCCESS, "RegSetValueExA failed: %d, GLE=%x\n", res, GetLastError());

    /* try to find the value in hkcr */
    res = RegQueryValueExA(hkcr, "val2", NULL, &type, (LPBYTE)buffer, &size);
    ok(res == ERROR_SUCCESS, "RegQueryValueExA failed: %d\n", res);
    ok(!strcmp( buffer, "user" ), "value set to '%s'\n", buffer );

    /* modify the value in hklm */
    res = RegSetValueExA(hklm, "val2", 0, REG_SZ, (const BYTE *)"hklm", sizeof("hklm"));
    ok(res == ERROR_SUCCESS, "RegSetValueExA failed: %d, GLE=%x\n", res, GetLastError());

    /* check that the value is not overwritten in hkcr or user's classes */
    res = RegQueryValueExA(hkcr, "val2", NULL, &type, (LPBYTE)buffer, &size);
    ok(res == ERROR_SUCCESS, "RegQueryValueExA failed: %d\n", res);
    ok(!strcmp( buffer, "user" ), "value set to '%s'\n", buffer );
    res = RegQueryValueExA(hkey, "val2", NULL, &type, (LPBYTE)buffer, &size);
    ok(res == ERROR_SUCCESS, "RegQueryValueExA failed: %d, GLE=%x\n", res, GetLastError());
    ok(!strcmp( buffer, "user" ), "value set to '%s'\n", buffer );

    /* modify the value in hkcr */
    res = RegSetValueExA(hkcr, "val2", 0, REG_SZ, (const BYTE *)"hkcr", sizeof("hkcr"));
    ok(res == ERROR_SUCCESS, "RegSetValueExA failed: %d, GLE=%x\n", res, GetLastError());

    /* check that the value is overwritten in hklm and user's classes */
    res = RegQueryValueExA(hkcr, "val2", NULL, &type, (LPBYTE)buffer, &size);
    ok(res == ERROR_SUCCESS, "RegQueryValueExA failed: %d\n", res);
    ok(!strcmp( buffer, "hkcr" ), "value set to '%s'\n", buffer );
    res = RegQueryValueExA(hkey, "val2", NULL, &type, (LPBYTE)buffer, &size);
    ok(res == ERROR_SUCCESS, "RegQueryValueExA failed: %d, GLE=%x\n", res, GetLastError());
    ok(!strcmp( buffer, "hkcr" ), "value set to '%s'\n", buffer );

    /* create a subkey in hklm */
    if (RegCreateKeyExA( hklm, "subkey1", 0, NULL, 0,
                         KEY_QUERY_VALUE|KEY_SET_VALUE, NULL, &hklmsub1, NULL )) return;
    ok(!IS_HKCR(hklmsub1), "hkcr mask set in %p\n", hklmsub1);
    /* try to open that subkey in hkcr */
    res = RegOpenKeyExA( hkcr, "subkey1", 0, KEY_QUERY_VALUE|KEY_SET_VALUE, &hkcrsub1 );
    ok(res == ERROR_SUCCESS, "test key not found in hkcr: %d\n", res);
    ok(IS_HKCR(hkcrsub1), "hkcr mask not set in %p\n", hkcrsub1);

    /* set a value in hklm classes */
    res = RegSetValueExA(hklmsub1, "subval1", 0, REG_SZ, (const BYTE *)"hklm", sizeof("hklm"));
    ok(res == ERROR_SUCCESS, "RegSetValueExA failed: %d, GLE=%x\n", res, GetLastError());

    /* try to find the value in hkcr */
    res = RegQueryValueExA(hkcrsub1, "subval1", NULL, &type, (LPBYTE)buffer, &size);
    ok(res == ERROR_SUCCESS, "RegQueryValueExA failed: %d\n", res);
    ok(!strcmp( buffer, "hklm" ), "value set to '%s'\n", buffer );

    /* modify the value in hkcr */
    res = RegSetValueExA(hkcrsub1, "subval1", 0, REG_SZ, (const BYTE *)"hkcr", sizeof("hkcr"));
    ok(res == ERROR_SUCCESS, "RegSetValueExA failed: %d, GLE=%x\n", res, GetLastError());

    /* check that the value is modified in hklm classes */
    res = RegQueryValueExA(hklmsub1, "subval1", NULL, &type, (LPBYTE)buffer, &size);
    ok(res == ERROR_SUCCESS, "RegQueryValueExA failed: %d, GLE=%x\n", res, GetLastError());
    ok(!strcmp( buffer, "hkcr" ), "value set to '%s'\n", buffer );

    /* create a subkey in user's classes */
    if (RegCreateKeyExA( hkey, "subkey1", 0, NULL, 0,
                         KEY_QUERY_VALUE|KEY_SET_VALUE, NULL, &hkeysub1, NULL )) return;
    ok(!IS_HKCR(hkeysub1), "hkcr mask set in %p\n", hkeysub1);

    /* set a value in user's classes */
    res = RegSetValueExA(hkeysub1, "subval1", 0, REG_SZ, (const BYTE *)"user", sizeof("user"));
    ok(res == ERROR_SUCCESS, "RegSetValueExA failed: %d, GLE=%x\n", res, GetLastError());

    /* try to find the value in hkcr */
    res = RegQueryValueExA(hkcrsub1, "subval1", NULL, &type, (LPBYTE)buffer, &size);
    ok(res == ERROR_SUCCESS, "RegQueryValueExA failed: %d\n", res);
    ok(!strcmp( buffer, "user" ), "value set to '%s'\n", buffer );

    /* modify the value in hklm */
    res = RegSetValueExA(hklmsub1, "subval1", 0, REG_SZ, (const BYTE *)"hklm", sizeof("hklm"));
    ok(res == ERROR_SUCCESS, "RegSetValueExA failed: %d, GLE=%x\n", res, GetLastError());

    /* check that the value is not overwritten in hkcr or user's classes */
    res = RegQueryValueExA(hkcrsub1, "subval1", NULL, &type, (LPBYTE)buffer, &size);
    ok(res == ERROR_SUCCESS, "RegQueryValueExA failed: %d\n", res);
    ok(!strcmp( buffer, "user" ), "value set to '%s'\n", buffer );
    res = RegQueryValueExA(hkeysub1, "subval1", NULL, &type, (LPBYTE)buffer, &size);
    ok(res == ERROR_SUCCESS, "RegQueryValueExA failed: %d, GLE=%x\n", res, GetLastError());
    ok(!strcmp( buffer, "user" ), "value set to '%s'\n", buffer );

    /* modify the value in hkcr */
    res = RegSetValueExA(hkcrsub1, "subval1", 0, REG_SZ, (const BYTE *)"hkcr", sizeof("hkcr"));
    ok(res == ERROR_SUCCESS, "RegSetValueExA failed: %d, GLE=%x\n", res, GetLastError());

    /* check that the value is not overwritten in hklm, but in user's classes */
    res = RegQueryValueExA(hklmsub1, "subval1", NULL, &type, (LPBYTE)buffer, &size);
    ok(res == ERROR_SUCCESS, "RegQueryValueExA failed: %d\n", res);
    ok(!strcmp( buffer, "hklm" ), "value set to '%s'\n", buffer );
    res = RegQueryValueExA(hkeysub1, "subval1", NULL, &type, (LPBYTE)buffer, &size);
    ok(res == ERROR_SUCCESS, "RegQueryValueExA failed: %d, GLE=%x\n", res, GetLastError());
    ok(!strcmp( buffer, "hkcr" ), "value set to '%s'\n", buffer );

    /* new subkey in hkcr */
    if (RegCreateKeyExA( hkcr, "subkey2", 0, NULL, 0,
                         KEY_QUERY_VALUE|KEY_SET_VALUE, NULL, &hkcrsub2, NULL )) return;
    ok(IS_HKCR(hkcrsub2), "hkcr mask not set in %p\n", hkcrsub2);
    res = RegSetValueExA(hkcrsub2, "subval1", 0, REG_SZ, (const BYTE *)"hkcr", sizeof("hkcr"));
    ok(res == ERROR_SUCCESS, "RegSetValueExA failed: %d, GLE=%x\n", res, GetLastError());

    /* try to open that new subkey in user's classes and hklm */
    res = RegOpenKeyExA( hkey, "subkey2", 0, KEY_QUERY_VALUE|KEY_SET_VALUE, &hklmsub2 );
    ok(res != ERROR_SUCCESS, "test key found in user's classes: %d\n", res);
    hklmsub2 = 0;
    res = RegOpenKeyExA( hklm, "subkey2", 0, KEY_QUERY_VALUE|KEY_SET_VALUE, &hklmsub2 );
    ok(res == ERROR_SUCCESS, "test key not found in hklm: %d\n", res);
    ok(!IS_HKCR(hklmsub2), "hkcr mask set in %p\n", hklmsub2);

    /* check that the value is present in hklm */
    res = RegQueryValueExA(hklmsub2, "subval1", NULL, &type, (LPBYTE)buffer, &size);
    ok(res == ERROR_SUCCESS, "RegQueryValueExA failed: %d\n", res);
    ok(!strcmp( buffer, "hkcr" ), "value set to '%s'\n", buffer );

    /* cleanup */
    RegCloseKey( hkeysub1 );
    RegCloseKey( hklmsub1 );

    /* delete subkey1 from hkcr (should point at user's classes) */
    res = RegDeleteKeyA(hkcr, "subkey1");
    ok(res == ERROR_SUCCESS, "RegDeleteKey failed: %d\n", res);

    /* confirm key was removed in hkey but not hklm */
    res = RegOpenKeyExA(hkey, "subkey1", 0, KEY_READ, &hkeysub1);
    ok(res == ERROR_FILE_NOT_FOUND, "test key found in user's classes: %d\n", res);
    res = RegOpenKeyExA(hklm, "subkey1", 0, KEY_READ, &hklmsub1);
    ok(res == ERROR_SUCCESS, "test key not found in hklm: %d\n", res);
    ok(!IS_HKCR(hklmsub1), "hkcr mask set in %p\n", hklmsub1);

    /* delete subkey1 from hkcr again (which should now point at hklm) */
    res = RegDeleteKeyA(hkcr, "subkey1");
    ok(res == ERROR_SUCCESS, "RegDeleteKey failed: %d\n", res);

    /* confirm hkey was removed in hklm */
    RegCloseKey( hklmsub1 );
    res = RegOpenKeyExA(hklm, "subkey1", 0, KEY_READ, &hklmsub1);
    ok(res == ERROR_FILE_NOT_FOUND, "test key found in hklm: %d\n", res);

    /* final cleanup */
    delete_key( hkey );
    delete_key( hklm );
    delete_key( hkcr );
    delete_key( hkeysub1 );
    delete_key( hklmsub1 );
    delete_key( hkcrsub1 );
    delete_key( hklmsub2 );
    delete_key( hkcrsub2 );
    RegCloseKey( hkey );
    RegCloseKey( hklm );
    RegCloseKey( hkcr );
    RegCloseKey( hkeysub1 );
    RegCloseKey( hklmsub1 );
    RegCloseKey( hkcrsub1 );
    RegCloseKey( hklmsub2 );
    RegCloseKey( hkcrsub2 );
}

static void test_classesroot_enum(void)
{
    HKEY hkcu=0, hklm=0, hkcr=0, hkcusub[2]={0}, hklmsub[2]={0};
    DWORD size;
    static CHAR buffer[2];
    LONG res;

    /* prepare initial testing env in HKCU */
    if (!RegOpenKeyA( HKEY_CURRENT_USER, "Software\\Classes\\WineTestCls", &hkcu ))
    {
        delete_key( hkcu );
        RegCloseKey( hkcu );
    }
    res = RegCreateKeyExA( HKEY_CURRENT_USER, "Software\\Classes\\WineTestCls", 0, NULL, 0,
                            KEY_SET_VALUE|KEY_ENUMERATE_SUB_KEYS, NULL, &hkcu, NULL );

    if (res != ERROR_SUCCESS)
    {
        skip("failed to add a user class\n");
        return;
    }

    res = RegOpenKeyA( HKEY_CLASSES_ROOT, "WineTestCls", &hkcr );
    todo_wine ok(res == ERROR_SUCCESS ||
                 broken(res == ERROR_FILE_NOT_FOUND /* WinNT */),
                 "test key not found in hkcr: %d\n", res);
    if (res)
    {
        skip("HKCR key merging not supported\n");
        delete_key( hkcu );
        RegCloseKey( hkcu );
        return;
    }

    res = RegSetValueExA( hkcu, "X", 0, REG_SZ, (const BYTE *) "AA", 3 );
    ok(res == ERROR_SUCCESS, "RegSetValueExA failed: %d\n", res);
    res = RegSetValueExA( hkcu, "Y", 0, REG_SZ, (const BYTE *) "B", 2 );
    ok(res == ERROR_SUCCESS, "RegSetValueExA failed: %d\n", res);
    res = RegCreateKeyA( hkcu, "A", &hkcusub[0] );
    ok(res == ERROR_SUCCESS, "RegCreateKeyA failed: %d\n", res);
    res = RegCreateKeyA( hkcu, "B", &hkcusub[1] );
    ok(res == ERROR_SUCCESS, "RegCreateKeyA failed: %d\n", res);

    /* test on values in HKCU */
    size = sizeof(buffer);
    res = RegEnumValueA( hkcr, 0, buffer, &size, NULL, NULL, NULL, NULL );
    ok(res == ERROR_SUCCESS, "RegEnumValueA failed: %d\n", res );
    ok(!strcmp( buffer, "X" ), "expected 'X', got '%s'\n", buffer);
    size = sizeof(buffer);
    res = RegEnumValueA( hkcr, 1, buffer, &size, NULL, NULL, NULL, NULL );
    ok(res == ERROR_SUCCESS, "RegEnumValueA failed: %d\n", res );
    ok(!strcmp( buffer, "Y" ), "expected 'Y', got '%s'\n", buffer);
    size = sizeof(buffer);
    res = RegEnumValueA( hkcr, 2, buffer, &size, NULL, NULL, NULL, NULL );
    ok(res == ERROR_NO_MORE_ITEMS, "expected ERROR_NO_MORE_ITEMS, got %d\n", res );

    res = RegEnumKeyA( hkcr, 0, buffer, size );
    ok(res == ERROR_SUCCESS, "RegEnumKey failed: %d\n", res );
    ok(!strcmp( buffer, "A" ), "expected 'A', got '%s'\n", buffer);
    res = RegEnumKeyA( hkcr, 1, buffer, size );
    ok(res == ERROR_SUCCESS, "RegEnumKey failed: %d\n", res );
    ok(!strcmp( buffer, "B" ), "expected 'B', got '%s'\n", buffer);
    res = RegEnumKeyA( hkcr, 2, buffer, size );
    ok(res == ERROR_NO_MORE_ITEMS, "expected ERROR_NO_MORE_ITEMS, got %d\n", res );

    /* prepare test env in HKLM */
    if (!RegOpenKeyA( HKEY_LOCAL_MACHINE, "Software\\Classes\\WineTestCls", &hklm ))
    {
        delete_key( hklm );
        RegCloseKey( hklm );
    }

    res = RegCreateKeyExA( HKEY_LOCAL_MACHINE, "Software\\Classes\\WineTestCls", 0, NULL, 0,
                            KEY_SET_VALUE|KEY_ENUMERATE_SUB_KEYS, NULL, &hklm, NULL );

    if (res == ERROR_ACCESS_DENIED)
    {
        RegCloseKey( hkcusub[0] );
        RegCloseKey( hkcusub[1] );
        delete_key( hkcu );
        RegCloseKey( hkcu );
        RegCloseKey( hkcr );
        skip("not enough privileges to add a system class\n");
        return;
    }

    res = RegSetValueExA( hklm, "X", 0, REG_SZ, (const BYTE *) "AB", 3 );
    ok(res == ERROR_SUCCESS, "RegSetValueExA failed: %d\n", res);
    res = RegSetValueExA( hklm, "Z", 0, REG_SZ, (const BYTE *) "C", 2 );
    ok(res == ERROR_SUCCESS, "RegSetValueExA failed: %d\n", res);
    res = RegCreateKeyA( hklm, "A", &hklmsub[0] );
    ok(res == ERROR_SUCCESS, "RegCreateKeyA failed: %d\n", res);
    res = RegCreateKeyA( hklm, "C", &hklmsub[1] );
    ok(res == ERROR_SUCCESS, "RegCreateKeyA failed: %d\n", res);

    /* test on values/keys in both HKCU and HKLM */
    size = sizeof(buffer);
    res = RegEnumValueA( hkcr, 0, buffer, &size, NULL, NULL, NULL, NULL );
    ok(res == ERROR_SUCCESS, "RegEnumValueA failed: %d\n", res );
    ok(!strcmp( buffer, "X" ), "expected 'X', got '%s'\n", buffer);
    size = sizeof(buffer);
    res = RegEnumValueA( hkcr, 1, buffer, &size, NULL, NULL, NULL, NULL );
    ok(res == ERROR_SUCCESS, "RegEnumValueA failed: %d\n", res );
    ok(!strcmp( buffer, "Y" ), "expected 'Y', got '%s'\n", buffer);
    size = sizeof(buffer);
    res = RegEnumValueA( hkcr, 2, buffer, &size, NULL, NULL, NULL, NULL );
    ok(res == ERROR_SUCCESS, "RegEnumValueA failed: %d\n", res );
    ok(!strcmp( buffer, "Z" ), "expected 'Z', got '%s'\n", buffer);
    size = sizeof(buffer);
    res = RegEnumValueA( hkcr, 3, buffer, &size, NULL, NULL, NULL, NULL );
    ok(res == ERROR_NO_MORE_ITEMS, "expected ERROR_NO_MORE_ITEMS, got %d\n", res );

    res = RegEnumKeyA( hkcr, 0, buffer, size );
    ok(res == ERROR_SUCCESS, "RegEnumKey failed: %d\n", res );
    ok(!strcmp( buffer, "A" ), "expected 'A', got '%s'\n", buffer);
    res = RegEnumKeyA( hkcr, 1, buffer, size );
    ok(res == ERROR_SUCCESS, "RegEnumKey failed: %d\n", res );
    ok(!strcmp( buffer, "B" ), "expected 'B', got '%s'\n", buffer);
    res = RegEnumKeyA( hkcr, 2, buffer, size );
    ok(res == ERROR_SUCCESS, "RegEnumKey failed: %d\n", res );
    ok(!strcmp( buffer, "C" ), "expected 'C', got '%s'\n", buffer);
    res = RegEnumKeyA( hkcr, 3, buffer, size );
    ok(res == ERROR_NO_MORE_ITEMS, "expected ERROR_NO_MORE_ITEMS, got %d\n", res );

    /* delete values/keys from HKCU to test only on HKLM */
    RegCloseKey( hkcusub[0] );
    RegCloseKey( hkcusub[1] );
    delete_key( hkcu );
    RegCloseKey( hkcu );

    size = sizeof(buffer);
    res = RegEnumValueA( hkcr, 0, buffer, &size, NULL, NULL, NULL, NULL );
    ok(res == ERROR_KEY_DELETED ||
       res == ERROR_NO_SYSTEM_RESOURCES, /* Windows XP */
       "expected ERROR_KEY_DELETED, got %d\n", res);
    size = sizeof(buffer);
    res = RegEnumKeyA( hkcr, 0, buffer, size );
    ok(res == ERROR_KEY_DELETED ||
       res == ERROR_NO_SYSTEM_RESOURCES, /* Windows XP */
       "expected ERROR_KEY_DELETED, got %d\n", res);

    /* reopen HKCR handle */
    RegCloseKey( hkcr );
    res = RegOpenKeyA( HKEY_CLASSES_ROOT, "WineTestCls", &hkcr );
    ok(res == ERROR_SUCCESS, "test key not found in hkcr: %d\n", res);
    if (res) goto cleanup;

    /* test on values/keys in HKLM */
    size = sizeof(buffer);
    res = RegEnumValueA( hkcr, 0, buffer, &size, NULL, NULL, NULL, NULL );
    ok(res == ERROR_SUCCESS, "RegEnumValueA failed: %d\n", res );
    ok(!strcmp( buffer, "X" ), "expected 'X', got '%s'\n", buffer);
    size = sizeof(buffer);
    res = RegEnumValueA( hkcr, 1, buffer, &size, NULL, NULL, NULL, NULL );
    ok(res == ERROR_SUCCESS, "RegEnumValueA failed: %d\n", res );
    ok(!strcmp( buffer, "Z" ), "expected 'Z', got '%s'\n", buffer);
    size = sizeof(buffer);
    res = RegEnumValueA( hkcr, 2, buffer, &size, NULL, NULL, NULL, NULL );
    ok(res == ERROR_NO_MORE_ITEMS, "expected ERROR_NO_MORE_ITEMS, got %d\n", res );

    res = RegEnumKeyA( hkcr, 0, buffer, size );
    ok(res == ERROR_SUCCESS, "RegEnumKey failed: %d\n", res );
    ok(!strcmp( buffer, "A" ), "expected 'A', got '%s'\n", buffer);
    res = RegEnumKeyA( hkcr, 1, buffer, size );
    ok(res == ERROR_SUCCESS, "RegEnumKey failed: %d\n", res );
    ok(!strcmp( buffer, "C" ), "expected 'C', got '%s'\n", buffer);
    res = RegEnumKeyA( hkcr, 2, buffer, size );
    ok(res == ERROR_NO_MORE_ITEMS, "expected ERROR_NO_MORE_ITEMS, got %d\n", res );

cleanup:
    RegCloseKey( hklmsub[0] );
    RegCloseKey( hklmsub[1] );
    delete_key( hklm );
    RegCloseKey( hklm );
    RegCloseKey( hkcr );
}

static void test_classesroot_mask(void)
{
    HKEY hkey;
    LSTATUS res;

    res = RegOpenKeyA( HKEY_CLASSES_ROOT, "CLSID", &hkey );
    ok(res == ERROR_SUCCESS, "RegOpenKeyA failed: %d\n", res);
    todo_wine ok(IS_HKCR(hkey) || broken(!IS_HKCR(hkey)) /* WinNT */,
                 "hkcr mask not set in %p\n", hkey);
    RegCloseKey( hkey );

    res = RegOpenKeyA( HKEY_CURRENT_USER, "Software", &hkey );
    ok(res == ERROR_SUCCESS, "RegOpenKeyA failed: %d\n", res);
    ok(!IS_HKCR(hkey), "hkcr mask set in %p\n", hkey);
    RegCloseKey( hkey );

    res = RegOpenKeyA( HKEY_LOCAL_MACHINE, "Software", &hkey );
    ok(res == ERROR_SUCCESS, "RegOpenKeyA failed: %d\n", res);
    ok(!IS_HKCR(hkey), "hkcr mask set in %p\n", hkey);
    RegCloseKey( hkey );

    res = RegOpenKeyA( HKEY_USERS, ".Default", &hkey );
    ok(res == ERROR_SUCCESS, "RegOpenKeyA failed: %d\n", res);
    ok(!IS_HKCR(hkey), "hkcr mask set in %p\n", hkey);
    RegCloseKey( hkey );

    res = RegOpenKeyA( HKEY_CURRENT_CONFIG, "Software", &hkey );
    ok(res == ERROR_SUCCESS, "RegOpenKeyA failed: %d\n", res);
    ok(!IS_HKCR(hkey), "hkcr mask set in %p\n", hkey);
    RegCloseKey( hkey );
}

static void test_deleted_key(void)
{
    HKEY hkey, hkey2;
    char value[20];
    DWORD val_count, type;
    LONG res;

    /* Open the test key, then delete it while it's open */
    RegOpenKeyA( HKEY_CURRENT_USER, "Software\\Wine\\Test", &hkey );

    delete_key( hkey_main );

    val_count = sizeof(value);
    type = 0;
    res = RegEnumValueA( hkey, 0, value, &val_count, NULL, &type, 0, 0 );
    ok(res == ERROR_KEY_DELETED, "expect ERROR_KEY_DELETED, got %i\n", res);

    res = RegEnumKeyA( hkey, 0, value, sizeof(value) );
    ok(res == ERROR_KEY_DELETED, "expect ERROR_KEY_DELETED, got %i\n", res);

    val_count = sizeof(value);
    type = 0;
    res = RegQueryValueExA( hkey, "test", NULL, &type, (BYTE *)value, &val_count );
    ok(res == ERROR_KEY_DELETED, "expect ERROR_KEY_DELETED, got %i\n", res);

    res = RegSetValueExA( hkey, "test", 0, REG_SZ, (const BYTE*)"value", 6);
    ok(res == ERROR_KEY_DELETED, "expect ERROR_KEY_DELETED, got %i\n", res);

    res = RegOpenKeyA( hkey, "test", &hkey2 );
    ok(res == ERROR_KEY_DELETED, "expect ERROR_KEY_DELETED, got %i\n", res);
    if (res == 0)
        RegCloseKey( hkey2 );

    res = RegCreateKeyA( hkey, "test", &hkey2 );
    ok(res == ERROR_KEY_DELETED, "expect ERROR_KEY_DELETED, got %i\n", res);
    if (res == 0)
        RegCloseKey( hkey2 );

    res = RegFlushKey( hkey );
    ok(res == ERROR_KEY_DELETED, "expect ERROR_KEY_DELETED, got %i\n", res);

    RegCloseKey( hkey );

    setup_main_key();
}

static void test_delete_value(void)
{
    LONG res;
    char longname[401];

    res = RegSetValueExA( hkey_main, "test", 0, REG_SZ, (const BYTE*)"value", 6 );
    ok(res == ERROR_SUCCESS, "expect ERROR_SUCCESS, got %i\n", res);

    res = RegQueryValueExA( hkey_main, "test", NULL, NULL, NULL, NULL);
    ok(res == ERROR_SUCCESS, "expect ERROR_SUCCESS, got %i\n", res);

    res = RegDeleteValueA( hkey_main, "test" );
    ok(res == ERROR_SUCCESS, "expect ERROR_SUCCESS, got %i\n", res);

    res = RegQueryValueExA( hkey_main, "test", NULL, NULL, NULL, NULL);
    ok(res == ERROR_FILE_NOT_FOUND, "expect ERROR_FILE_NOT_FOUND, got %i\n", res);

    res = RegDeleteValueA( hkey_main, "test" );
    ok(res == ERROR_FILE_NOT_FOUND, "expect ERROR_FILE_NOT_FOUND, got %i\n", res);

    memset(longname, 'a', 400);
    longname[400] = 0;
    res = RegDeleteValueA( hkey_main, longname );
    ok(res == ERROR_FILE_NOT_FOUND || broken(res == ERROR_MORE_DATA), /* nt4, win2k */
       "expect ERROR_FILE_NOT_FOUND, got %i\n", res);
}

static void test_delete_key_value(void)
{
    HKEY subkey;
    LONG ret;

    if (!pRegDeleteKeyValueA)
    {
        win_skip("RegDeleteKeyValue is not available.\n");
        return;
    }

    ret = pRegDeleteKeyValueA(NULL, NULL, NULL);
    ok(ret == ERROR_INVALID_HANDLE, "got %d\n", ret);

    ret = pRegDeleteKeyValueA(hkey_main, NULL, NULL);
    ok(ret == ERROR_FILE_NOT_FOUND, "got %d\n", ret);

    ret = RegSetValueExA(hkey_main, "test", 0, REG_SZ, (const BYTE*)"value", 6);
    ok(ret == ERROR_SUCCESS, "got %d\n", ret);

    ret = RegQueryValueExA(hkey_main, "test", NULL, NULL, NULL, NULL);
    ok(ret == ERROR_SUCCESS, "got %d\n", ret);

    /* NULL subkey name means delete from open key */
    ret = pRegDeleteKeyValueA(hkey_main, NULL, "test");
    ok(ret == ERROR_SUCCESS, "got %d\n", ret);

    ret = RegQueryValueExA(hkey_main, "test", NULL, NULL, NULL, NULL);
    ok(ret == ERROR_FILE_NOT_FOUND, "got %d\n", ret);

    /* now with real subkey */
    ret = RegCreateKeyExA(hkey_main, "Subkey1", 0, NULL, 0, KEY_WRITE|KEY_READ, NULL, &subkey, NULL);
    ok(!ret, "failed with error %d\n", ret);

    ret = RegSetValueExA(subkey, "test", 0, REG_SZ, (const BYTE*)"value", 6);
    ok(ret == ERROR_SUCCESS, "got %d\n", ret);

    ret = RegQueryValueExA(subkey, "test", NULL, NULL, NULL, NULL);
    ok(ret == ERROR_SUCCESS, "got %d\n", ret);

    ret = pRegDeleteKeyValueA(hkey_main, "Subkey1", "test");
    ok(ret == ERROR_SUCCESS, "got %d\n", ret);

    ret = RegQueryValueExA(subkey, "test", NULL, NULL, NULL, NULL);
    ok(ret == ERROR_FILE_NOT_FOUND, "got %d\n", ret);

    RegDeleteKeyA(subkey, "");
    RegCloseKey(subkey);
}

START_TEST(registry)
{
    /* Load pointers for functions that are not available in all Windows versions */
    InitFunctionPtrs();

    setup_main_key();
    check_user_privs();
    test_set_value();
    create_test_entries();
    test_enum_value();
    test_query_value_ex();
    test_get_value();
    test_reg_open_key();
    test_reg_create_key();
    test_reg_close_key();
    test_reg_delete_key();
    test_reg_query_value();
    test_string_termination();
    test_symlinks();
    test_redirection();
    test_classesroot();
    test_classesroot_enum();
    test_classesroot_mask();

    /* SaveKey/LoadKey require the SE_BACKUP_NAME privilege to be set */
    if (set_privileges(SE_BACKUP_NAME, TRUE) &&
        set_privileges(SE_RESTORE_NAME, TRUE))
    {
        test_reg_save_key();
        test_reg_load_key();
        test_reg_unload_key();

        set_privileges(SE_BACKUP_NAME, FALSE);
        set_privileges(SE_RESTORE_NAME, FALSE);
    }

    test_reg_delete_tree();
    test_rw_order();
    test_deleted_key();
    test_delete_value();
    test_delete_key_value();

    /* cleanup */
    delete_key( hkey_main );
    
    test_regconnectregistry();
}
