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

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winreg.h"
#include "winperf.h"
#include "winsvc.h"
#include "winerror.h"
#include "aclapi.h"
#ifdef __REACTOS__
/* FIXME: Removing this hack requires fixing our incompatible wine/test.h and wine/debug.h. */
#ifndef wine_dbg_sprintf
static inline const char* wine_dbg_sprintf(const char* format, ...)
{
    static char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    return buffer;
}
#endif
#endif

#define IS_HKCR(hk) ((UINT_PTR)hk > 0 && ((UINT_PTR)hk & 3) == 2)

static HKEY hkey_main;
static DWORD GLE;

static const char * sTestpath1 = "%LONGSYSTEMVAR%\\subdir1";
static const char * sTestpath2 = "%FOO%\\subdir1";
static const DWORD ptr_size = 8 * sizeof(void*);

static DWORD (WINAPI *pRegGetValueA)(HKEY,LPCSTR,LPCSTR,DWORD,LPDWORD,PVOID,LPDWORD);
static DWORD (WINAPI *pRegGetValueW)(HKEY,LPCWSTR,LPCWSTR,DWORD,LPDWORD,PVOID,LPDWORD);
static LONG (WINAPI *pRegCopyTreeA)(HKEY,const char *,HKEY);
static LONG (WINAPI *pRegDeleteTreeA)(HKEY,const char *);
static DWORD (WINAPI *pRegDeleteKeyExA)(HKEY,LPCSTR,REGSAM,DWORD);
static BOOL (WINAPI *pIsWow64Process)(HANDLE,PBOOL);
static NTSTATUS (WINAPI * pNtDeleteKey)(HANDLE);
static NTSTATUS (WINAPI * pNtUnloadKey)(POBJECT_ATTRIBUTES);
static NTSTATUS (WINAPI * pRtlFormatCurrentUserKeyPath)(UNICODE_STRING*);
static NTSTATUS (WINAPI * pRtlFreeUnicodeString)(PUNICODE_STRING);
static NTSTATUS (WINAPI * pRtlInitUnicodeString)(PUNICODE_STRING,PCWSTR);
static LONG (WINAPI *pRegDeleteKeyValueA)(HKEY,LPCSTR,LPCSTR);
static LONG (WINAPI *pRegSetKeyValueW)(HKEY,LPCWSTR,LPCWSTR,DWORD,const void*,DWORD);
static LONG (WINAPI *pRegLoadMUIStringA)(HKEY,LPCSTR,LPSTR,DWORD,LPDWORD,DWORD,LPCSTR);
static LONG (WINAPI *pRegLoadMUIStringW)(HKEY,LPCWSTR,LPWSTR,DWORD,LPDWORD,DWORD,LPCWSTR);
static DWORD (WINAPI *pEnumDynamicTimeZoneInformation)(const DWORD,
                                                       DYNAMIC_TIME_ZONE_INFORMATION*);

static BOOL limited_user;
static const BOOL is_64bit = sizeof(void *) > sizeof(int);

static BOOL has_wow64(void)
{
    if (!is_64bit)
    {
        BOOL is_wow64;
        if (!pIsWow64Process || !pIsWow64Process( GetCurrentProcess(), &is_wow64 ) || !is_wow64)
            return FALSE;
    }
    return TRUE;
}

static const char *dbgstr_SYSTEMTIME(const SYSTEMTIME *st)
{
    return wine_dbg_sprintf("%02d-%02d-%04d %02d:%02d:%02d.%03d",
                            st->wMonth, st->wDay, st->wYear,
                            st->wHour, st->wMinute, st->wSecond, st->wMilliseconds);
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
    ADVAPI32_GET_PROC(RegGetValueW);
    ADVAPI32_GET_PROC(RegCopyTreeA);
    ADVAPI32_GET_PROC(RegDeleteTreeA);
    ADVAPI32_GET_PROC(RegDeleteKeyExA);
    ADVAPI32_GET_PROC(RegDeleteKeyValueA);
    ADVAPI32_GET_PROC(RegSetKeyValueW);
    ADVAPI32_GET_PROC(RegLoadMUIStringA);
    ADVAPI32_GET_PROC(RegLoadMUIStringW);
    ADVAPI32_GET_PROC(EnumDynamicTimeZoneInformation);

    pIsWow64Process = (void *)GetProcAddress( hkernel32, "IsWow64Process" );
    pRtlFormatCurrentUserKeyPath = (void *)GetProcAddress( hntdll, "RtlFormatCurrentUserKeyPath" );
    pRtlFreeUnicodeString = (void *)GetProcAddress(hntdll, "RtlFreeUnicodeString");
    pRtlInitUnicodeString = (void *)GetProcAddress(hntdll, "RtlInitUnicodeString");
    pNtDeleteKey = (void *)GetProcAddress( hntdll, "NtDeleteKey" );
    pNtUnloadKey = (void *)GetProcAddress( hntdll, "NtUnloadKey" );
}

static BOOL is_special_key(HKEY key)
{
    return !!((ULONG_PTR)key & 0x80000000);
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
    DWORD ret;

    if (!RegOpenKeyA( HKEY_CURRENT_USER, "Software\\Wine\\Test", &hkey_main )) delete_key( hkey_main );

    ret = RegCreateKeyA( HKEY_CURRENT_USER, "Software\\Wine\\Test", &hkey_main );
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);
}

static void check_user_privs(void)
{
    DWORD ret;
    HKEY hkey = (HKEY)0xdeadbeef;

    ret = RegOpenKeyExA( HKEY_LOCAL_MACHINE, "Software", 0, KEY_READ|KEY_WRITE, &hkey);
    ok(ret == ERROR_SUCCESS || ret == ERROR_ACCESS_DENIED, "expected success or access denied, got %li\n", ret);
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
    lok(ret == ERROR_SUCCESS, "RegQueryValueExA/1 failed: %ld, GLE=%ld\n", ret, GLE);
    /* It is wrong for the Ansi version to not be implemented */
    ok(GLE == 0xdeadbeef, "RegQueryValueExA set GLE = %lu\n", GLE);
    if(GLE == ERROR_CALL_NOT_IMPLEMENTED) return;

    str_byte_len = (string ? lstrlenA(string) : 0) + 1;
    lok(type == REG_SZ, "RegQueryValueExA/1 returned type %ld\n", type);
    lok(cbData == full_byte_len, "cbData=%ld instead of %ld or %ld\n", cbData, full_byte_len, str_byte_len);

    value = malloc(cbData+1);
    memset(value, 0xbd, cbData+1);
    type=0xdeadbeef;
    ret = RegQueryValueExA(hkey_main, name, NULL, &type, value, &cbData);
    GLE = GetLastError();
    lok(ret == ERROR_SUCCESS, "RegQueryValueExA/2 failed: %ld, GLE=%ld\n", ret, GLE);
    if (!string)
    {
        /* When cbData == 0, RegQueryValueExA() should not modify the buffer */
        lok(*value == 0xbd, "RegQueryValueExA overflowed: cbData=%lu *value=%02x\n", cbData, *value);
    }
    else
    {
        lok(memcmp(value, string, cbData) == 0, "RegQueryValueExA/2 failed: %s/%ld != %s/%ld\n",
           debugstr_an((char*)value, cbData), cbData,
           debugstr_an(string, full_byte_len), full_byte_len);
        lok(*(value+cbData) == 0xbd, "RegQueryValueExA/2 overflowed at offset %lu: %02x != bd\n", cbData, *(value+cbData));
    }
    free(value);
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
    lok(ret == ERROR_SUCCESS, "RegQueryValueExW/1 failed: %ld, GLE=%ld\n", ret, GLE);
    if(GLE == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("RegQueryValueExW() is not implemented\n");
        return;
    }

    lok(type == REG_SZ, "RegQueryValueExW/1 returned type %ld\n", type);
    lok(cbData == full_byte_len,
        "cbData=%ld instead of %ld\n", cbData, full_byte_len);

    /* Give enough space to overflow by one WCHAR */
    value = malloc(cbData+2);
    memset(value, 0xbd, cbData+2);
    type=0xdeadbeef;
    ret = RegQueryValueExW(hkey_main, name, NULL, &type, value, &cbData);
    GLE = GetLastError();
    lok(ret == ERROR_SUCCESS, "RegQueryValueExW/2 failed: %ld, GLE=%ld\n", ret, GLE);
    if (string)
    {
        lok(memcmp(value, string, cbData) == 0, "RegQueryValueExW failed: %s/%ld != %s/%ld\n",
           wine_dbgstr_wn((WCHAR*)value, cbData / sizeof(WCHAR)), cbData,
           wine_dbgstr_wn(string, full_byte_len / sizeof(WCHAR)), full_byte_len);
    }
    /* This implies that when cbData == 0, RegQueryValueExW() should not modify the buffer */
    lok(*(value+cbData) == 0xbd, "RegQueryValueExW/2 overflowed at %lu: %02x != bd\n", cbData, *(value+cbData));
    lok(*(value+cbData+1) == 0xbd, "RegQueryValueExW/2 overflowed at %lu+1: %02x != bd\n", cbData, *(value+cbData+1));
    free(value);
}

static void test_set_value(void)
{
    DWORD ret;

    static const WCHAR name1W[] =   L"CleanSingleString";
    static const WCHAR name2W[] =   L"SomeIntraZeroedString";
    static const WCHAR emptyW[] =   L"";
    static const WCHAR string1W[] = L"ThisNeverBreaks";
    static const WCHAR string2W[] = L"This\0Breaks\0\0A\0\0\0Lot\0\0\0\0";
    static const WCHAR substring2W[] = L"This";

    static const char name1A[] =   "CleanSingleString";
    static const char name2A[] =   "SomeIntraZeroedString";
    static const char emptyA[] = "";
    static const char string1A[] = "ThisNeverBreaks";
    static const char string2A[] = "This\0Breaks\0\0A\0\0\0Lot\0\0\0\0";
    static const char substring2A[] = "This";

    ret = RegSetValueA(hkey_main, NULL, REG_SZ, NULL, 0);
    ok(ret == ERROR_INVALID_PARAMETER, "RegSetValueA should have failed with ERROR_INVALID_PARAMETER instead of %ld\n", ret);

    ret = RegSetValueA(hkey_main, NULL, REG_SZ, string1A, sizeof(string1A));
    ok(ret == ERROR_SUCCESS, "RegSetValueA failed: %ld, GLE=%ld\n", ret, GetLastError());
    test_hkey_main_Value_A(NULL, string1A, sizeof(string1A));
    test_hkey_main_Value_W(NULL, string1W, sizeof(string1W));

    /* RegSetValueA ignores the size passed in */
    ret = RegSetValueA(hkey_main, NULL, REG_SZ, string1A, 4);
    ok(ret == ERROR_SUCCESS, "RegSetValueA failed: %ld, GLE=%ld\n", ret, GetLastError());
    test_hkey_main_Value_A(NULL, string1A, sizeof(string1A));
    test_hkey_main_Value_W(NULL, string1W, sizeof(string1W));

    /* stops at first null */
    ret = RegSetValueA(hkey_main, NULL, REG_SZ, string2A, sizeof(string2A));
    ok(ret == ERROR_SUCCESS, "RegSetValueA failed: %ld, GLE=%ld\n", ret, GetLastError());
    test_hkey_main_Value_A(NULL, substring2A, sizeof(substring2A));
    test_hkey_main_Value_W(NULL, substring2W, sizeof(substring2W));

    /* only REG_SZ is supported on NT*/
    ret = RegSetValueA(hkey_main, NULL, REG_BINARY, string2A, sizeof(string2A));
    ok(ret == ERROR_INVALID_PARAMETER, "got %ld (expected ERROR_INVALID_PARAMETER)\n", ret);

    ret = RegSetValueA(hkey_main, NULL, REG_EXPAND_SZ, string2A, sizeof(string2A));
    ok(ret == ERROR_INVALID_PARAMETER, "got %ld (expected ERROR_INVALID_PARAMETER)\n", ret);

    ret = RegSetValueA(hkey_main, NULL, REG_MULTI_SZ, string2A, sizeof(string2A));
    ok(ret == ERROR_INVALID_PARAMETER, "got %ld (expected ERROR_INVALID_PARAMETER)\n", ret);

    /* Test RegSetValueExA with a 'zero-byte' string (as Office 2003 does).
     * Surprisingly enough we're supposed to get zero bytes out of it.
     */
    ret = RegSetValueExA(hkey_main, name1A, 0, REG_SZ, (const BYTE *)emptyA, 0);
    ok(ret == ERROR_SUCCESS, "RegSetValueExA failed: %ld, GLE=%ld\n", ret, GetLastError());
    test_hkey_main_Value_A(name1A, NULL, 0);
    test_hkey_main_Value_W(name1W, NULL, 0);

    /* test RegSetValueExA with an empty string */
    ret = RegSetValueExA(hkey_main, name1A, 0, REG_SZ, (const BYTE *)emptyA, sizeof(emptyA));
    ok(ret == ERROR_SUCCESS, "RegSetValueExA failed: %ld, GLE=%ld\n", ret, GetLastError());
    test_hkey_main_Value_A(name1A, emptyA, sizeof(emptyA));
    test_hkey_main_Value_W(name1W, emptyW, sizeof(emptyW));

    /* test RegSetValueExA with off-by-one size */
    ret = RegSetValueExA(hkey_main, name1A, 0, REG_SZ, (const BYTE *)string1A, sizeof(string1A)-sizeof(string1A[0]));
    ok(ret == ERROR_SUCCESS, "RegSetValueExA failed: %ld, GLE=%ld\n", ret, GetLastError());
    test_hkey_main_Value_A(name1A, string1A, sizeof(string1A));
    test_hkey_main_Value_W(name1W, string1W, sizeof(string1W));

    /* test RegSetValueExA with normal string */
    ret = RegSetValueExA(hkey_main, name1A, 0, REG_SZ, (const BYTE *)string1A, sizeof(string1A));
    ok(ret == ERROR_SUCCESS, "RegSetValueExA failed: %ld, GLE=%ld\n", ret, GetLastError());
    test_hkey_main_Value_A(name1A, string1A, sizeof(string1A));
    test_hkey_main_Value_W(name1W, string1W, sizeof(string1W));

    /* test RegSetValueExA with intrazeroed string */
    ret = RegSetValueExA(hkey_main, name2A, 0, REG_SZ, (const BYTE *)string2A, sizeof(string2A));
    ok(ret == ERROR_SUCCESS, "RegSetValueExA failed: %ld, GLE=%ld\n", ret, GetLastError());
    test_hkey_main_Value_A(name2A, string2A, sizeof(string2A));
    test_hkey_main_Value_W(name2W, string2W, sizeof(string2W));

    if (0)
    {
        ret = RegSetValueW(hkey_main, NULL, REG_SZ, NULL, 0);
        ok(ret == ERROR_INVALID_PARAMETER, "RegSetValueW should have failed with ERROR_INVALID_PARAMETER instead of %ld\n", ret);

        RegSetValueExA(hkey_main, name2A, 0, REG_SZ, (const BYTE *)1, 1);
        RegSetValueExA(hkey_main, name2A, 0, REG_DWORD, (const BYTE *)1, 1);
    }

    ret = RegSetValueW(hkey_main, NULL, REG_SZ, string1W, sizeof(string1W));
    ok(ret == ERROR_SUCCESS, "RegSetValueW failed: %ld, GLE=%ld\n", ret, GetLastError());
    test_hkey_main_Value_A(NULL, string1A, sizeof(string1A));
    test_hkey_main_Value_W(NULL, string1W, sizeof(string1W));

    ret = RegSetValueW(hkey_main, name1W, REG_SZ, string1W, sizeof(string1W));
    ok(ret == ERROR_SUCCESS, "RegSetValueW failed: %ld, GLE=%ld\n", ret, GetLastError());
    test_hkey_main_Value_A(name1A, string1A, sizeof(string1A));
    test_hkey_main_Value_W(name1W, string1W, sizeof(string1W));

    /* RegSetValueW ignores the size passed in */
    ret = RegSetValueW(hkey_main, NULL, REG_SZ, string1W, 4 * sizeof(string1W[0]));
    ok(ret == ERROR_SUCCESS, "RegSetValueW failed: %ld, GLE=%ld\n", ret, GetLastError());
    test_hkey_main_Value_A(NULL, string1A, sizeof(string1A));
    test_hkey_main_Value_W(NULL, string1W, sizeof(string1W));

    /* stops at first null */
    ret = RegSetValueW(hkey_main, NULL, REG_SZ, string2W, sizeof(string2W));
    ok(ret == ERROR_SUCCESS, "RegSetValueW failed: %ld, GLE=%ld\n", ret, GetLastError());
    test_hkey_main_Value_A(NULL, substring2A, sizeof(substring2A));
    test_hkey_main_Value_W(NULL, substring2W, sizeof(substring2W));

    /* only REG_SZ is supported */
    ret = RegSetValueW(hkey_main, NULL, REG_BINARY, string2W, sizeof(string2W));
    ok(ret == ERROR_INVALID_PARAMETER, "RegSetValueW should have returned ERROR_INVALID_PARAMETER instead of %ld\n", ret);
    ret = RegSetValueW(hkey_main, NULL, REG_EXPAND_SZ, string2W, sizeof(string2W));
    ok(ret == ERROR_INVALID_PARAMETER, "RegSetValueW should have returned ERROR_INVALID_PARAMETER instead of %ld\n", ret);
    ret = RegSetValueW(hkey_main, NULL, REG_MULTI_SZ, string2W, sizeof(string2W));
    ok(ret == ERROR_INVALID_PARAMETER, "RegSetValueW should have returned ERROR_INVALID_PARAMETER instead of %ld\n", ret);

    /* test RegSetValueExW with off-by-one size */
    ret = RegSetValueExW(hkey_main, name1W, 0, REG_SZ, (const BYTE *)string1W, sizeof(string1W)-sizeof(string1W[0]));
    ok(ret == ERROR_SUCCESS, "RegSetValueExW failed: %ld, GLE=%ld\n", ret, GetLastError());
    test_hkey_main_Value_A(name1A, string1A, sizeof(string1A));
    test_hkey_main_Value_W(name1W, string1W, sizeof(string1W));

    /* test RegSetValueExW with normal string */
    ret = RegSetValueExW(hkey_main, name1W, 0, REG_SZ, (const BYTE *)string1W, sizeof(string1W));
    ok(ret == ERROR_SUCCESS, "RegSetValueExW failed: %ld, GLE=%ld\n", ret, GetLastError());
    test_hkey_main_Value_A(name1A, string1A, sizeof(string1A));
    test_hkey_main_Value_W(name1W, string1W, sizeof(string1W));

    /* test RegSetValueExW with intrazeroed string */
    ret = RegSetValueExW(hkey_main, name2W, 0, REG_SZ, (const BYTE *)string2W, sizeof(string2W));
    ok(ret == ERROR_SUCCESS, "RegSetValueExW failed: %ld, GLE=%ld\n", ret, GetLastError());
    test_hkey_main_Value_A(name2A, string2A, sizeof(string2A));
    test_hkey_main_Value_W(name2W, string2W, sizeof(string2W));

    /* test RegSetValueExW with data = 1 */
    ret = RegSetValueExW(hkey_main, name2W, 0, REG_SZ, (const BYTE *)1, 1);
    ok(ret == ERROR_NOACCESS, "RegSetValueExW should have failed with ERROR_NOACCESS: %ld, GLE=%ld\n", ret, GetLastError());
    ret = RegSetValueExW(hkey_main, name2W, 0, REG_DWORD, (const BYTE *)1, 1);
    ok(ret == ERROR_NOACCESS, "RegSetValueExW should have failed with ERROR_NOACCESS: %ld, GLE=%ld\n", ret, GetLastError());

    if (pRegGetValueA) /* avoid a crash on Windows 2000 */
    {
        ret = RegSetValueExW(hkey_main, NULL, 0, REG_SZ, NULL, 4);
        ok(ret == ERROR_NOACCESS, "RegSetValueExW should have failed with ERROR_NOACCESS: %ld, GLE=%ld\n", ret, GetLastError());

        ret = RegSetValueExW(hkey_main, NULL, 0, REG_SZ, NULL, 0);
        ok(ret == ERROR_SUCCESS, "got %ld\n", ret);

        ret = RegSetValueExW(hkey_main, NULL, 0, REG_DWORD, NULL, 4);
        ok(ret == ERROR_NOACCESS, "RegSetValueExW should have failed with ERROR_NOACCESS: %ld, GLE=%ld\n", ret, GetLastError());

        ret = RegSetValueExW(hkey_main, NULL, 0, REG_DWORD, NULL, 0);
        ok(ret == ERROR_SUCCESS, "got %ld\n", ret);
    }

    /* RegSetKeyValue */
    if (!pRegSetKeyValueW)
        win_skip("RegSetKeyValue() is not supported.\n");
    else
    {
        DWORD len, type;
        HKEY subkey;

        ret = pRegSetKeyValueW(hkey_main, NULL, name1W, REG_SZ, (const BYTE*)string2W, sizeof(string2W));
        ok(ret == ERROR_SUCCESS, "got %ld\n", ret);
        test_hkey_main_Value_A(name1A, string2A, sizeof(string2A));
        test_hkey_main_Value_W(name1W, string2W, sizeof(string2W));

        ret = pRegSetKeyValueW(hkey_main, L"subkey", name1W, REG_SZ, string1W, sizeof(string1W));
        ok(ret == ERROR_SUCCESS, "got %ld\n", ret);

        ret = RegOpenKeyExW(hkey_main, L"subkey", 0, KEY_QUERY_VALUE, &subkey);
        ok(ret == ERROR_SUCCESS, "got %ld\n", ret);
        type = len = 0;
        ret = RegQueryValueExW(subkey, name1W, 0, &type, NULL, &len);
        ok(ret == ERROR_SUCCESS, "got %ld\n", ret);
        ok(len == sizeof(string1W), "got %ld\n", len);
        ok(type == REG_SZ, "got type %ld\n", type);

        ret = pRegSetKeyValueW(hkey_main, L"subkey", name1W, REG_SZ, NULL, 0);
        ok(ret == ERROR_SUCCESS, "got %ld\n", ret);

        ret = pRegSetKeyValueW(hkey_main, L"subkey", name1W, REG_SZ, NULL, 4);
        ok(ret == ERROR_NOACCESS, "got %ld\n", ret);

        ret = pRegSetKeyValueW(hkey_main, L"subkey", name1W, REG_DWORD, NULL, 4);
        ok(ret == ERROR_NOACCESS, "got %ld\n", ret);

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
    char value[20], data[30];
    WCHAR valueW[20], dataW[20];
    DWORD val_count, data_count, type;

    /* create the working key for new 'Test' value */
    res = RegCreateKeyA( hkey_main, "TestKey", &test_key );
    ok( res == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", res);

    /* check NULL data with zero length */
    res = RegSetValueExA( test_key, "Test", 0, REG_SZ, NULL, 0 );
    if (GetVersion() & 0x80000000)
        ok( res == ERROR_INVALID_PARAMETER, "RegSetValueExA returned %ld\n", res );
    else
        ok( !res, "RegSetValueExA returned %ld\n", res );
    res = RegSetValueExA( test_key, "Test", 0, REG_EXPAND_SZ, NULL, 0 );
    ok( ERROR_SUCCESS == res, "RegSetValueExA returned %ld\n", res );
    res = RegSetValueExA( test_key, "Test", 0, REG_BINARY, NULL, 0 );
    ok( ERROR_SUCCESS == res, "RegSetValueExA returned %ld\n", res );

    /* test reading the value and data without setting them */
    val_count = 20;
    data_count = 20;
    type = 1234;
    strcpy( value, "xxxxxxxxxx" );
    strcpy( data, "xxxxxxxxxx" );
    res = RegEnumValueA( test_key, 0, value, &val_count, NULL, &type, (LPBYTE)data, &data_count );
    ok( res == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", res );
    ok( val_count == 4, "val_count set to %ld instead of 4\n", val_count );
    ok( data_count == 0, "data_count set to %ld instead of 0\n", data_count );
    ok( type == REG_BINARY, "type %ld is not REG_BINARY\n", type );
    ok( !strcmp( value, "Test" ), "value is '%s' instead of Test\n", value );
    ok( !strcmp( data, "xxxxxxxxxx" ), "data is '%s' instead of xxxxxxxxxx\n", data );

    val_count = 20;
    data_count = 20;
    type = 1234;
    wcscpy( valueW, L"xxxxxxxx" );
    wcscpy( dataW, L"xxxxxxxx" );
    res = RegEnumValueW( test_key, 0, valueW, &val_count, NULL, &type, (BYTE*)dataW, &data_count );
    ok( res == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", res );
    ok( val_count == 4, "val_count set to %ld instead of 4\n", val_count );
    ok( data_count == 0, "data_count set to %ld instead of 0\n", data_count );
    ok( type == REG_BINARY, "type %ld is not REG_BINARY\n", type );
    ok( !wcscmp( valueW, L"Test" ), "value is not 'Test'\n" );
    ok( !wcscmp( dataW, L"xxxxxxxx" ), "data is not 'xxxxxxxx'\n" );

    res = RegSetValueExA( test_key, "Test", 0, REG_SZ, (const BYTE *)"foobar", 7 );
    ok( res == 0, "RegSetValueExA failed error %ld\n", res );

    /* overflow both name and data */
    val_count = 2;
    data_count = 2;
    type = 1234;
    strcpy( value, "xxxxxxxxxx" );
    strcpy( data, "xxxxxxxxxx" );
    res = RegEnumValueA( test_key, 0, value, &val_count, NULL, &type, (LPBYTE)data, &data_count );
    ok( res == ERROR_MORE_DATA, "expected ERROR_MORE_DATA, got %ld\n", res );
    ok( val_count == 2, "val_count set to %ld\n", val_count );
    /* Chinese, Japanese, and Korean editions of Windows 10 sometimes set data_count to a higher value */
    ok( data_count == 7 || broken( data_count > 7 ), "data_count set to %ld instead of 7\n", data_count );
    ok( type == REG_SZ, "type %ld is not REG_SZ\n", type );
    ok( !strcmp( value, "xxxxxxxxxx" ), "value set to '%s'\n", value );
    ok( !strcmp( data, "xxxxxxxxxx" ), "data set to '%s'\n", data );

    /* overflow name */
    val_count = 3;
    data_count = 16;
    type = 1234;
    strcpy( value, "xxxxxxxxxx" );
    memset( data, 'x', sizeof(data) );
    data[sizeof(data)-1] = '\0';
    res = RegEnumValueA( test_key, 0, value, &val_count, NULL, &type, (LPBYTE)data, &data_count );
    ok( res == ERROR_MORE_DATA, "expected ERROR_MORE_DATA, got %ld\n", res );
    ok( val_count == 3, "val_count set to %ld\n", val_count );
    /* In double-byte and UTF-8 locales Windows 10 may set data_count > 7,
     * potentially even more than the declared buffer size, in which case the
     * buffer is not NUL-terminated.
     */
    ok( data_count == 7 || broken( data_count > 7 ), "data_count set to %ld instead of 7\n", data_count );
    ok( type == REG_SZ, "type %ld is not REG_SZ\n", type );
    /* v5.1.2600.0 (XP Home and Professional) does not touch value or data in
     * this case. Neither does Windows 10 21H1 in UTF-8 locales.
     */
    ok( !strcmp( value, "Te" ) || !strcmp( value, "xxxxxxxxxx" ), 
        "value set to '%s' instead of 'Te' or 'xxxxxxxxxx'\n", value );
    ok( !strcmp( data, "foobar" ) || !strcmp( data, "xxxxxxx" ) ||
        broken( data_count > 7 && data_count < 16 &&
                strspn( data, "x" ) == data_count && data[data_count] == 0 ) ||
        broken( data_count >= 16 && strspn( data, "x" ) == sizeof(data) - 1 ),
        "data set to '%s' instead of 'foobar' or x's, data_count=%lu\n", data, data_count );

    /* overflow empty name */
    val_count = 0;
    data_count = 16;
    type = 1234;
    strcpy( value, "xxxxxxxxxx" );
    memset( data, 'x', sizeof(data) );
    data[sizeof(data)-1] = '\0';
    res = RegEnumValueA( test_key, 0, value, &val_count, NULL, &type, (LPBYTE)data, &data_count );
    ok( res == ERROR_MORE_DATA, "expected ERROR_MORE_DATA, got %ld\n", res );
    ok( val_count == 0, "val_count set to %ld\n", val_count );
    /* See comment in 'overflow name' section */
    ok( data_count == 7 || broken( data_count > 7 ), "data_count set to %ld instead of 7\n", data_count );
    ok( type == REG_SZ, "type %ld is not REG_SZ\n", type );
    ok( !strcmp( value, "xxxxxxxxxx" ), "value set to '%s'\n", value );
    /* See comment in 'overflow name' section */
    ok( !strcmp( data, "foobar" ) || !strcmp( data, "xxxxxxx" ) ||
        broken( data_count > 7 && data_count < 16 &&
                strspn( data, "x" ) == data_count && data[data_count] == 0 ) ||
        broken( data_count >= 16 && strspn( data, "x" ) == sizeof(data) - 1 ),
        "data set to '%s' instead of 'foobar' or x's, data_count=%lu\n", data, data_count );

    /* overflow data */
    val_count = 20;
    data_count = 2;
    type = 1234;
    strcpy( value, "xxxxxxxxxx" );
    strcpy( data, "xxxxxxxxxx" );
    res = RegEnumValueA( test_key, 0, value, &val_count, NULL, &type, (LPBYTE)data, &data_count );
    ok( res == ERROR_MORE_DATA, "expected ERROR_MORE_DATA, got %ld\n", res );
    ok( val_count == 20, "val_count set to %ld\n", val_count );
    ok( data_count == 7, "data_count set to %ld instead of 7\n", data_count );
    ok( type == REG_SZ, "type %ld is not REG_SZ\n", type );
    ok( !strcmp( value, "xxxxxxxxxx" ), "value set to '%s'\n", value );
    ok( !strcmp( data, "xxxxxxxxxx" ), "data set to '%s'\n", data );

    /* no overflow */
    val_count = 20;
    data_count = 20;
    type = 1234;
    strcpy( value, "xxxxxxxxxx" );
    strcpy( data, "xxxxxxxxxx" );
    res = RegEnumValueA( test_key, 0, value, &val_count, NULL, &type, (LPBYTE)data, &data_count );
    ok( res == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", res );
    ok( val_count == 4, "val_count set to %ld instead of 4\n", val_count );
    ok( data_count == 7, "data_count set to %ld instead of 7\n", data_count );
    ok( type == REG_SZ, "type %ld is not REG_SZ\n", type );
    ok( !strcmp( value, "Test" ), "value is '%s' instead of Test\n", value );
    ok( !strcmp( data, "foobar" ), "data is '%s' instead of foobar\n", data );

    if (pRegGetValueA) /* avoid a crash on Windows 2000 */
    {
        /* no value and no val_count parameter */
        data_count = 20;
        type = 1234;
        strcpy( data, "xxxxxxxxxx" );
        res = RegEnumValueA( test_key, 0, NULL, NULL, NULL, &type, (BYTE*)data, &data_count );
        ok( res == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %ld\n", res );

        /* no value parameter */
        val_count = 20;
        data_count = 20;
        type = 1234;
        strcpy( data, "xxxxxxxxxx" );
        res = RegEnumValueA( test_key, 0, NULL, &val_count, NULL, &type, (BYTE*)data, &data_count );
        ok( res == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %ld\n", res );

        /* no val_count parameter */
        data_count = 20;
        type = 1234;
        strcpy( value, "xxxxxxxxxx" );
        strcpy( data, "xxxxxxxxxx" );
        res = RegEnumValueA( test_key, 0, value, NULL, NULL, &type, (BYTE*)data, &data_count );
        ok( res == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %ld\n", res );
    }

    /* Unicode tests */

    SetLastError(0xdeadbeef);
    res = RegSetValueExW( test_key, L"Test", 0, REG_SZ, (const BYTE *)L"foobar", 7*sizeof(WCHAR) );
    if (res==0 && GetLastError()==ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("RegSetValueExW is not implemented\n");
        goto cleanup;
    }
    ok( res == 0, "RegSetValueExW failed error %ld\n", res );

    /* overflow both name and data */
    val_count = 2;
    data_count = 2;
    type = 1234;
    wcscpy( valueW, L"xxxxxxxx" );
    wcscpy( dataW, L"xxxxxxxx" );
    res = RegEnumValueW( test_key, 0, valueW, &val_count, NULL, &type, (BYTE*)dataW, &data_count );
    ok( res == ERROR_MORE_DATA, "expected ERROR_MORE_DATA, got %ld\n", res );
    ok( val_count == 2, "val_count set to %ld\n", val_count );
    ok( data_count == 7*sizeof(WCHAR), "data_count set to %ld instead of 7*sizeof(WCHAR)\n", data_count );
    ok( type == REG_SZ, "type %ld is not REG_SZ\n", type );
    ok( !wcscmp( valueW, L"xxxxxxxx" ), "value modified\n" );
    ok( !wcscmp( dataW, L"xxxxxxxx" ), "data modified\n" );

    /* overflow name */
    val_count = 3;
    data_count = 20;
    type = 1234;
    wcscpy( valueW, L"xxxxxxxx" );
    wcscpy( dataW, L"xxxxxxxx" );
    res = RegEnumValueW( test_key, 0, valueW, &val_count, NULL, &type, (BYTE*)dataW, &data_count );
    ok( res == ERROR_MORE_DATA, "expected ERROR_MORE_DATA, got %ld\n", res );
    ok( val_count == 3, "val_count set to %ld\n", val_count );
    ok( data_count == 7*sizeof(WCHAR), "data_count set to %ld instead of 7*sizeof(WCHAR)\n", data_count );
    ok( type == REG_SZ, "type %ld is not REG_SZ\n", type );
    ok( !wcscmp( valueW, L"xxxxxxxx" ), "value modified\n" );
    ok( !wcscmp( dataW, L"xxxxxxxx" ), "data modified\n" );

    /* overflow data */
    val_count = 20;
    data_count = 2;
    type = 1234;
    wcscpy( valueW, L"xxxxxxxx" );
    wcscpy( dataW, L"xxxxxxxx" );
    res = RegEnumValueW( test_key, 0, valueW, &val_count, NULL, &type, (BYTE*)dataW, &data_count );
    ok( res == ERROR_MORE_DATA, "expected ERROR_MORE_DATA, got %ld\n", res );
    ok( val_count == 4, "val_count set to %ld instead of 4\n", val_count );
    ok( data_count == 7*sizeof(WCHAR), "data_count set to %ld instead of 7*sizeof(WCHAR)\n", data_count );
    ok( type == REG_SZ, "type %ld is not REG_SZ\n", type );
    ok( !wcscmp( valueW, L"Test" ), "value is not 'Test'\n" );
    ok( !wcscmp( dataW, L"xxxxxxxx" ), "data modified\n" );

    /* no overflow */
    val_count = 20;
    data_count = 20;
    type = 1234;
    wcscpy( valueW, L"xxxxxxxx" );
    wcscpy( dataW, L"xxxxxxxx" );
    res = RegEnumValueW( test_key, 0, valueW, &val_count, NULL, &type, (BYTE*)dataW, &data_count );
    ok( res == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", res );
    ok( val_count == 4, "val_count set to %ld instead of 4\n", val_count );
    ok( data_count == 7*sizeof(WCHAR), "data_count set to %ld instead of 7*sizeof(WCHAR)\n", data_count );
    ok( type == REG_SZ, "type %ld is not REG_SZ\n", type );
    ok( !wcscmp( valueW, L"Test" ), "value is not 'Test'\n" );
    ok( !wcscmp( dataW, L"foobar" ), "data is not 'foobar'\n" );

    if (pRegGetValueA) /* avoid a crash on Windows 2000 */
    {
        /* no valueW and no val_count parameter */
        data_count = 20;
        type = 1234;
        wcscpy( dataW, L"xxxxxxxx" );
        res = RegEnumValueW( test_key, 0, NULL, NULL, NULL, &type, (BYTE*)dataW, &data_count );
        ok( res == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %ld\n", res );

        /* no valueW parameter */
        val_count = 20;
        data_count = 20;
        type = 1234;
        wcscpy( dataW, L"xxxxxxxx" );
        res = RegEnumValueW( test_key, 0, NULL, &val_count, NULL, &type, (BYTE*)dataW, &data_count );
        ok( res == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %ld\n", res );

        /* no val_count parameter */
        data_count = 20;
        type = 1234;
        wcscpy( valueW, L"xxxxxxxx" );
        wcscpy( dataW, L"xxxxxxxx" );
        res = RegEnumValueW( test_key, 0, valueW, NULL, NULL, &type, (BYTE*)dataW, &data_count );
        ok( res == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %ld\n", res );
    }

cleanup:
    RegDeleteKeyA(test_key, "");
    RegCloseKey(test_key);
}

static void test_query_value_ex(void)
{
    DWORD ret, size, type;
    BYTE buffer[10];

    size = sizeof(buffer);
    ret = RegQueryValueExA(hkey_main, "TP1_SZ", NULL, &type, NULL, &size);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);
    ok(size == strlen(sTestpath1) + 1, "(%ld,%ld)\n", (DWORD)strlen(sTestpath1) + 1, size);
    ok(type == REG_SZ, "type %ld is not REG_SZ\n", type);

    type = 0xdeadbeef;
    size = 0xdeadbeef;
    ret = RegQueryValueExA(HKEY_CLASSES_ROOT, "Nonexistent Value", NULL, &type, NULL, &size);
    ok(ret == ERROR_FILE_NOT_FOUND, "expected ERROR_FILE_NOT_FOUND, got %ld\n", ret);
    ok(size == 0, "size should have been set to 0 instead of %ld\n", size);

    size = sizeof(buffer);
    ret = RegQueryValueExA(HKEY_CLASSES_ROOT, "Nonexistent Value", NULL, &type, buffer, &size);
    ok(ret == ERROR_FILE_NOT_FOUND, "expected ERROR_FILE_NOT_FOUND, got %ld\n", ret);
    ok(size == sizeof(buffer), "size shouldn't have been changed to %ld\n", size);

    size = 4;
    ret = RegQueryValueExA(hkey_main, "BIN32", NULL, &size, buffer, &size);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);
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
    ok(ret == ERROR_INVALID_PARAMETER, "ret=%ld\n", ret);

    /* Query REG_DWORD using RRF_RT_REG_DWORD (ok) */
    size = type = dw = 0xdeadbeef;
    ret = pRegGetValueA(hkey_main, NULL, "DWORD", RRF_RT_REG_DWORD, &type, &dw, &size);
    ok(ret == ERROR_SUCCESS, "ret=%ld\n", ret);
    ok(size == 4, "size=%ld\n", size);
    ok(type == REG_DWORD, "type=%ld\n", type);
    ok(dw == 0x12345678, "dw=%ld\n", dw);

    /* Check RRF_SUBKEY_WOW64*KEY validation on a case without a subkey */
    ret = pRegGetValueA(hkey_main, NULL, "DWORD", RRF_RT_REG_DWORD | RRF_SUBKEY_WOW6464KEY | RRF_SUBKEY_WOW6432KEY, NULL, NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER || broken(ret == ERROR_SUCCESS), /* Before Win10 */
       "ret=%ld\n", ret);
    ret = pRegGetValueA(hkey_main, NULL, "DWORD", RRF_RT_REG_DWORD | RRF_SUBKEY_WOW6464KEY, NULL, NULL, NULL);
    ok(ret == ERROR_SUCCESS, "ret=%ld\n", ret);
    ret = pRegGetValueA(hkey_main, NULL, "DWORD", RRF_RT_REG_DWORD | RRF_SUBKEY_WOW6432KEY, NULL, NULL, NULL);
    ok(ret == ERROR_SUCCESS, "ret=%ld\n", ret);

    /* Query by subkey-name */
    ret = pRegGetValueA(HKEY_CURRENT_USER, "Software\\Wine\\Test", "DWORD", RRF_RT_REG_DWORD, NULL, NULL, NULL);
    ok(ret == ERROR_SUCCESS, "ret=%ld\n", ret);

    /* Check RRF_SUBKEY_WOW64*KEY validation on a case with a subkey */
    ret = pRegGetValueA(HKEY_CURRENT_USER, "Software\\Wine\\Test", "DWORD", RRF_RT_REG_DWORD | RRF_SUBKEY_WOW6464KEY | RRF_SUBKEY_WOW6432KEY, NULL, NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER || broken(ret == ERROR_SUCCESS), /* Before Win10 */
       "ret=%ld\n", ret);
    ret = pRegGetValueA(HKEY_CURRENT_USER, "Software\\Wine\\Test", "DWORD", RRF_RT_REG_DWORD | RRF_SUBKEY_WOW6464KEY, NULL, NULL, NULL);
    ok(ret == ERROR_SUCCESS, "ret=%ld\n", ret);
    ret = pRegGetValueA(HKEY_CURRENT_USER, "Software\\Wine\\Test", "DWORD", RRF_RT_REG_DWORD | RRF_SUBKEY_WOW6432KEY, NULL, NULL, NULL);
    ok(ret == ERROR_SUCCESS, "ret=%ld\n", ret);

    /* Query REG_DWORD using RRF_RT_REG_BINARY (restricted) */
    size = type = dw = 0xdeadbeef;
    ret = pRegGetValueA(hkey_main, NULL, "DWORD", RRF_RT_REG_BINARY, &type, &dw, &size);
    ok(ret == ERROR_UNSUPPORTED_TYPE, "ret=%ld\n", ret);
    /* Although the function failed all values are retrieved */
    ok(size == 4, "size=%ld\n", size);
    ok(type == REG_DWORD, "type=%ld\n", type);
    ok(dw == 0x12345678, "dw=%ld\n", dw);

    /* Test RRF_ZEROONFAILURE */
    type = dw = 0xdeadbeef; size = 4;
    ret = pRegGetValueA(hkey_main, NULL, "DWORD", RRF_RT_REG_SZ|RRF_ZEROONFAILURE, &type, &dw, &size);
    ok(ret == ERROR_UNSUPPORTED_TYPE, "ret=%ld\n", ret);
    /* Again all values are retrieved ... */
    ok(size == 4, "size=%ld\n", size);
    ok(type == REG_DWORD, "type=%ld\n", type);
    /* ... except the buffer, which is zeroed out */
    ok(dw == 0, "dw=%ld\n", dw);

    /* Test RRF_ZEROONFAILURE with a NULL buffer... */
    type = size = 0xbadbeef;
    ret = pRegGetValueA(hkey_main, NULL, "DWORD", RRF_RT_REG_SZ|RRF_ZEROONFAILURE, &type, NULL, &size);
    ok(ret == ERROR_UNSUPPORTED_TYPE, "ret=%ld\n", ret);
    ok(size == 4, "size=%ld\n", size);
    ok(type == REG_DWORD, "type=%ld\n", type);

    /* Query REG_DWORD using RRF_RT_DWORD (ok) */
    size = type = dw = 0xdeadbeef;
    ret = pRegGetValueA(hkey_main, NULL, "DWORD", RRF_RT_DWORD, &type, &dw, &size);
    ok(ret == ERROR_SUCCESS, "ret=%ld\n", ret);
    ok(size == 4, "size=%ld\n", size);
    ok(type == REG_DWORD, "type=%ld\n", type);
    ok(dw == 0x12345678, "dw=%ld\n", dw);

    /* Query 32-bit REG_BINARY using RRF_RT_DWORD (ok) */
    size = type = dw = 0xdeadbeef;
    ret = pRegGetValueA(hkey_main, NULL, "BIN32", RRF_RT_DWORD, &type, &dw, &size);
    ok(ret == ERROR_SUCCESS, "ret=%ld\n", ret);
    ok(size == 4, "size=%ld\n", size);
    ok(type == REG_BINARY, "type=%ld\n", type);
    ok(dw == 0x12345678, "dw=%ld\n", dw);
    
    /* Query 64-bit REG_BINARY using RRF_RT_DWORD (type mismatch) */
    qw[0] = qw[1] = size = type = 0xdeadbeef;
    ret = pRegGetValueA(hkey_main, NULL, "BIN64", RRF_RT_DWORD, &type, qw, &size);
    ok(ret == ERROR_DATATYPE_MISMATCH, "ret=%ld\n", ret);
    ok(size == 8, "size=%ld\n", size);
    ok(type == REG_BINARY, "type=%ld\n", type);
    ok(qw[0] == 0x12345678 && 
       qw[1] == 0x87654321, "qw={%ld,%ld}\n", qw[0], qw[1]);
    
    /* Query 64-bit REG_BINARY using 32-bit buffer (buffer too small) */
    type = dw = 0xdeadbeef; size = 4;
    ret = pRegGetValueA(hkey_main, NULL, "BIN64", RRF_RT_REG_BINARY, &type, &dw, &size);
    ok(ret == ERROR_MORE_DATA, "ret=%ld\n", ret);
    ok(dw == 0xdeadbeef, "dw=%ld\n", dw);
    ok(size == 8, "size=%ld\n", size);

    /* Query 64-bit REG_BINARY using RRF_RT_QWORD (ok) */
    qw[0] = qw[1] = size = type = 0xdeadbeef;
    ret = pRegGetValueA(hkey_main, NULL, "BIN64", RRF_RT_QWORD, &type, qw, &size);
    ok(ret == ERROR_SUCCESS, "ret=%ld\n", ret);
    ok(size == 8, "size=%ld\n", size);
    ok(type == REG_BINARY, "type=%ld\n", type);
    ok(qw[0] == 0x12345678 &&
       qw[1] == 0x87654321, "qw={%ld,%ld}\n", qw[0], qw[1]);

    /* Query REG_SZ using RRF_RT_REG_SZ (ok) */
    buf[0] = 0; type = 0xdeadbeef; size = sizeof(buf);
    ret = pRegGetValueA(hkey_main, NULL, "TP1_SZ", RRF_RT_REG_SZ, &type, buf, &size);
    ok(ret == ERROR_SUCCESS, "ret=%ld\n", ret);
    ok(size == strlen(sTestpath1)+1, "strlen(sTestpath1)=%d size=%ld\n", lstrlenA(sTestpath1), size);
    ok(type == REG_SZ, "type=%ld\n", type);
    ok(!strcmp(sTestpath1, buf), "sTestpath=\"%s\" buf=\"%s\"\n", sTestpath1, buf);

    /* Query REG_SZ using RRF_RT_REG_SZ and no buffer (ok) */
    type = 0xdeadbeef; size = 0;
    ret = pRegGetValueA(hkey_main, NULL, "TP1_SZ", RRF_RT_REG_SZ, &type, NULL, &size);
    ok(ret == ERROR_SUCCESS, "ret=%ld\n", ret);
    /* v5.2.3790.1830 (2003 SP1) returns sTestpath1 length + 2 here. */
    ok(size == strlen(sTestpath1)+1 || broken(size == strlen(sTestpath1)+2),
       "strlen(sTestpath1)=%d size=%ld\n", lstrlenA(sTestpath1), size);
    ok(type == REG_SZ, "type=%ld\n", type);

    /* Query REG_SZ using RRF_RT_REG_SZ on a zero-byte value (ok) */
    strcpy(buf, sTestpath1);
    type = 0xdeadbeef;
    size = 0;
    ret = pRegGetValueA(hkey_main, NULL, "TP1_ZB_SZ", RRF_RT_REG_SZ, &type, NULL, &size);
    ok(ret == ERROR_SUCCESS, "ret=%ld\n", ret);
    ok(size == 1, "size=%ld\n", size);
    ok(type == REG_SZ, "type=%ld\n", type);
    ret = pRegGetValueA(hkey_main, NULL, "TP1_ZB_SZ", RRF_RT_REG_SZ, &type, buf, &size);
    ok(ret == ERROR_SUCCESS, "ret=%ld\n", ret);
    ok(size == 1, "size=%ld\n", size);
    ok(type == REG_SZ, "type=%ld\n", type);
    ok(!strcmp(buf, ""), "Expected \"\", got \"%s\"\n", buf);

    /* Query REG_SZ using RRF_RT_REG_SZ|RRF_NOEXPAND (ok) */
    buf[0] = 0; type = 0xdeadbeef; size = sizeof(buf);
    ret = pRegGetValueA(hkey_main, NULL, "TP1_SZ", RRF_RT_REG_SZ|RRF_NOEXPAND, &type, buf, &size);
    ok(ret == ERROR_SUCCESS, "ret=%ld\n", ret);
    ok(size == strlen(sTestpath1)+1, "strlen(sTestpath1)=%d size=%ld\n", lstrlenA(sTestpath1), size);
    ok(type == REG_SZ, "type=%ld\n", type);
    ok(!strcmp(sTestpath1, buf), "sTestpath=\"%s\" buf=\"%s\"\n", sTestpath1, buf);

    /* Query REG_EXPAND_SZ using RRF_RT_REG_SZ and no buffer (ok, expands) */
    size = 0;
    ret = pRegGetValueA(hkey_main, NULL, "TP2_EXP_SZ", RRF_RT_REG_SZ, NULL, NULL, &size);
    ok(ret == ERROR_SUCCESS, "ret=%ld\n", ret);
    ok(size == strlen(expanded2)+2,
       "strlen(expanded2)=%d, strlen(sTestpath2)=%d, size=%ld\n", lstrlenA(expanded2), lstrlenA(sTestpath2), size);

    /* Query REG_EXPAND_SZ using RRF_RT_REG_SZ (ok, expands) */
    buf[0] = 0; type = 0xdeadbeef; size = sizeof(buf);
    ret = pRegGetValueA(hkey_main, NULL, "TP1_EXP_SZ", RRF_RT_REG_SZ, &type, buf, &size);
    ok(ret == ERROR_SUCCESS, "ret=%ld\n", ret);
    todo_wine ok(size == strlen(sTestpath1)+1,
       "strlen(expanded)=%d, strlen(sTestpath1)=%d, size=%ld\n", lstrlenA(expanded), lstrlenA(sTestpath1), size);
    ok(type == REG_SZ, "type=%ld\n", type);
    ok(!strcmp(expanded, buf), "expanded=\"%s\" buf=\"%s\"\n", expanded, buf);

    /* Query REG_EXPAND_SZ using RRF_RT_REG_SZ (ok, expands a lot) */
    buf[0] = 0; type = 0xdeadbeef; size = sizeof(buf);
    ret = pRegGetValueA(hkey_main, NULL, "TP2_EXP_SZ", RRF_RT_REG_SZ, &type, buf, &size);
    ok(ret == ERROR_SUCCESS, "ret=%ld\n", ret);
    ok(size == strlen(expanded2)+1,
        "strlen(expanded2)=%d, strlen(sTestpath1)=%d, size=%ld\n", lstrlenA(expanded2), lstrlenA(sTestpath2), size);
    ok(type == REG_SZ, "type=%ld\n", type);
    ok(!strcmp(expanded2, buf), "expanded2=\"%s\" buf=\"%s\"\n", expanded2, buf);

    /* Query REG_EXPAND_SZ using RRF_RT_REG_EXPAND_SZ|RRF_NOEXPAND (ok, doesn't expand) */
    buf[0] = 0; type = 0xdeadbeef; size = sizeof(buf);
    ret = pRegGetValueA(hkey_main, NULL, "TP1_EXP_SZ", RRF_RT_REG_EXPAND_SZ|RRF_NOEXPAND, &type, buf, &size);
    ok(ret == ERROR_SUCCESS, "ret=%ld\n", ret);
    ok(size == strlen(sTestpath1)+1, "strlen(sTestpath1)=%d size=%ld\n", lstrlenA(sTestpath1), size);
    ok(type == REG_EXPAND_SZ, "type=%ld\n", type);
    ok(!strcmp(sTestpath1, buf), "sTestpath=\"%s\" buf=\"%s\"\n", sTestpath1, buf);

    /* Query REG_EXPAND_SZ using RRF_RT_REG_EXPAND_SZ|RRF_NOEXPAND and no buffer (ok, doesn't expand) */
    size = 0xbadbeef;
    ret = pRegGetValueA(hkey_main, NULL, "TP1_EXP_SZ", RRF_RT_REG_EXPAND_SZ|RRF_NOEXPAND, NULL, NULL, &size);
    ok(ret == ERROR_SUCCESS, "ret=%ld\n", ret);
    todo_wine ok(size == strlen(sTestpath1)+2, "strlen(sTestpath1)=%d size=%ld\n", lstrlenA(sTestpath1), size);

    /* Query REG_EXPAND_SZ using RRF_RT_REG_SZ|RRF_NOEXPAND (type mismatch) */
    ret = pRegGetValueA(hkey_main, NULL, "TP1_EXP_SZ", RRF_RT_REG_SZ|RRF_NOEXPAND, NULL, NULL, NULL);
    ok(ret == ERROR_UNSUPPORTED_TYPE, "ret=%ld\n", ret);

    /* Query REG_EXPAND_SZ using RRF_RT_REG_EXPAND_SZ (not allowed without RRF_NOEXPAND) */
    ret = pRegGetValueA(hkey_main, NULL, "TP1_EXP_SZ", RRF_RT_REG_EXPAND_SZ, NULL, NULL, NULL);
    /* before win8: ERROR_INVALID_PARAMETER, win8: ERROR_UNSUPPORTED_TYPE */
    ok(ret == ERROR_INVALID_PARAMETER || ret == ERROR_UNSUPPORTED_TYPE, "ret=%ld\n", ret);

    /* Query REG_EXPAND_SZ using RRF_RT_ANY */
    buf[0] = 0; type = 0xdeadbeef; size = sizeof(buf);
    ret = pRegGetValueA(hkey_main, NULL, "TP1_EXP_SZ", RRF_RT_ANY, &type, buf, &size);
    ok(ret == ERROR_SUCCESS, "ret=%ld\n", ret);
    todo_wine ok(size == strlen(sTestpath1)+1,
        "strlen(expanded)=%d, strlen(sTestpath1)=%d, size=%ld\n", lstrlenA(expanded), lstrlenA(sTestpath1), size);
    ok(type == REG_SZ, "type=%ld\n", type);
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
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);
    ok(hkResult != NULL, "expected hkResult != NULL\n");
    hkPreserve = hkResult;

    /* open same key twice */
    ret = RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\Test", &hkResult);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);
    ok(hkResult != hkPreserve, "expected hkResult != hkPreserve\n");
    ok(hkResult != NULL, "hkResult != NULL\n");
    RegCloseKey(hkResult);

    /* trailing slashes */
    ret = RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\Test\\\\", &hkResult);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);
    RegCloseKey(hkResult);

    /* open nonexistent key
    * check that hkResult is set to NULL
    */
    hkResult = hkPreserve;
    ret = RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\Nonexistent", &hkResult);
    ok(ret == ERROR_FILE_NOT_FOUND, "expected ERROR_FILE_NOT_FOUND, got %ld\n", ret);
    ok(hkResult == NULL, "expected hkResult == NULL\n");

    /* open the same nonexistent key again to make sure the key wasn't created */
    hkResult = hkPreserve;
    ret = RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\Nonexistent", &hkResult);
    ok(ret == ERROR_FILE_NOT_FOUND, "expected ERROR_FILE_NOT_FOUND, got %ld\n", ret);
    ok(hkResult == NULL, "expected hkResult == NULL\n");

    /* send in NULL lpSubKey
    * check that hkResult receives the value of hKey
    */
    hkResult = hkPreserve;
    ret = RegOpenKeyA(HKEY_CURRENT_USER, NULL, &hkResult);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);
    ok(hkResult == HKEY_CURRENT_USER, "expected hkResult == HKEY_CURRENT_USER\n");

    /* send empty-string in lpSubKey */
    hkResult = hkPreserve;
    ret = RegOpenKeyA(HKEY_CURRENT_USER, "", &hkResult);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);
    ok(hkResult == HKEY_CURRENT_USER, "expected hkResult == HKEY_CURRENT_USER\n");

    /* send in NULL lpSubKey and NULL hKey
    * hkResult is set to NULL
    */
    hkResult = hkPreserve;
    ret = RegOpenKeyA(NULL, NULL, &hkResult);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);
    ok(hkResult == NULL, "expected hkResult == NULL\n");

    /* only send NULL hKey
     * the value of hkResult remains unchanged
     */
    hkResult = hkPreserve;
    ret = RegOpenKeyA(NULL, "Software\\Wine\\Test", &hkResult);
    ok(ret == ERROR_INVALID_HANDLE || ret == ERROR_BADKEY, /* Windows 95 returns BADKEY */
       "expected ERROR_INVALID_HANDLE or ERROR_BADKEY, got %ld\n", ret);
    ok(hkResult == hkPreserve, "expected hkResult == hkPreserve\n");

    /* send in NULL hkResult */
    ret = RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\Test", NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %ld\n", ret);

    ret = RegOpenKeyA(HKEY_CURRENT_USER, NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %ld\n", ret);

    ret = RegOpenKeyA(NULL, NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %ld\n", ret);

    /*  beginning backslash character */
    ret = RegOpenKeyA(HKEY_CURRENT_USER, "\\Software\\Wine\\Test", &hkResult);
    ok(ret == ERROR_BAD_PATHNAME || broken(ret == ERROR_SUCCESS),  /* wow64 */
       "expected ERROR_BAD_PATHNAME or ERROR_FILE_NOT_FOUND, got %ld\n", ret);
    if (!ret) RegCloseKey(hkResult);

    hkResult = NULL;
    ret = RegOpenKeyExA(HKEY_CLASSES_ROOT, "\\clsid", 0, KEY_QUERY_VALUE, &hkResult);
    ok(ret == ERROR_SUCCESS,
       "expected ERROR_SUCCESS, ERROR_BAD_PATHNAME or ERROR_FILE_NOT_FOUND, got %ld\n", ret);
    RegCloseKey(hkResult);

    /* NULL or empty subkey of special root */
    hkResult = NULL;
    ret = RegOpenKeyExA(HKEY_CLASSES_ROOT, NULL, 0, KEY_QUERY_VALUE, &hkResult);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);
    ok(hkResult == HKEY_CLASSES_ROOT, "expected hkResult == HKEY_CLASSES_ROOT\n");

    hkResult = NULL;
    ret = RegOpenKeyExA(HKEY_CLASSES_ROOT, "", 0, KEY_QUERY_VALUE, &hkResult);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);
    ok(hkResult == HKEY_CLASSES_ROOT, "expected hkResult == HKEY_CLASSES_ROOT\n");

    hkResult = NULL;
    ret = RegOpenKeyExA(HKEY_CLASSES_ROOT, "\\", 0, KEY_QUERY_VALUE, &hkResult);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);
    ok(hkResult != HKEY_CLASSES_ROOT, "expected hkResult to be a new key\n");
    ok(!RegCloseKey(hkResult), "got invalid hkey\n");

    /* empty subkey of existing handle */
    hkResult = hkPreserve;
    ret = RegOpenKeyExA(hkPreserve, "", 0, KEY_QUERY_VALUE, &hkResult);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);
    ok(hkResult != hkPreserve, "expected hkResult != hkPreserve\n");
    ok(!RegCloseKey(hkResult), "got invalid hkey\n");

    /* NULL subkey of existing handle */
    hkResult = hkPreserve;
    ret = RegOpenKeyExA(hkPreserve, NULL, 0, KEY_QUERY_VALUE, &hkResult);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);
    ok(hkResult != hkPreserve, "expected hkResult != hkPreserve\n");
    ok(!RegCloseKey(hkResult), "got invalid hkey\n");

    /* empty subkey of NULL */
    hkResult = hkPreserve;
    ret = RegOpenKeyExW(NULL, L"", 0, KEY_QUERY_VALUE, &hkResult);
    ok(ret == ERROR_INVALID_HANDLE, "expected ERROR_INVALID_HANDLE, got %ld\n", ret);
#ifdef __REACTOS__
    ok(hkResult == NULL || broken(GetNTVersion() <= _WIN32_WINNT_WS03), "expected hkResult == NULL\n");
#else
    ok(hkResult == NULL, "expected hkResult == NULL\n");
#endif

    hkResult = hkPreserve;
    ret = RegOpenKeyExA(NULL, "", 0, KEY_QUERY_VALUE, &hkResult);
    ok(ret == ERROR_INVALID_HANDLE, "expected ERROR_INVALID_HANDLE, got %ld\n", ret);
    ok(hkResult == hkPreserve, "expected hkResult == hkPreserve\n");

    RegCloseKey(hkPreserve);

    /* WOW64 flags */
    hkResult = NULL;
    ret = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software", 0, KEY_READ|KEY_WOW64_32KEY, &hkResult);
    ok(ret == ERROR_SUCCESS && hkResult, "RegOpenKeyEx with KEY_WOW64_32KEY failed (err=%lu)\n", ret);
    RegCloseKey(hkResult);

    hkResult = NULL;
    ret = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software", 0, KEY_READ|KEY_WOW64_64KEY, &hkResult);
    ok(ret == ERROR_SUCCESS && hkResult, "RegOpenKeyEx with KEY_WOW64_64KEY failed (err=%lu)\n", ret);
    RegCloseKey(hkResult);

    /* check special HKEYs on 64bit
     * only the lower 4 bytes of the supplied key are used
     */
    if (ptr_size == 64)
    {
        /* HKEY_CURRENT_USER */
        ret = RegOpenKeyA(UlongToHandle(HandleToUlong(HKEY_CURRENT_USER)), "Software", &hkResult);
        ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);
        ok(hkResult != NULL, "expected hkResult != NULL\n");
        RegCloseKey(hkResult);

        ret = RegOpenKeyA((HKEY)(HandleToUlong(HKEY_CURRENT_USER) | (ULONG64)1 << 32), "Software", &hkResult);
        ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);
        ok(hkResult != NULL, "expected hkResult != NULL\n");
        RegCloseKey(hkResult);

        ret = RegOpenKeyA((HKEY)(HandleToUlong(HKEY_CURRENT_USER) | (ULONG64)0xdeadbeef << 32), "Software", &hkResult);
        ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);
        ok(hkResult != NULL, "expected hkResult != NULL\n");
        RegCloseKey(hkResult);

        ret = RegOpenKeyA((HKEY)(HandleToUlong(HKEY_CURRENT_USER) | (ULONG64)0xffffffff << 32), "Software", &hkResult);
        ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);
        ok(hkResult != NULL, "expected hkResult != NULL\n");
        RegCloseKey(hkResult);

        /* HKEY_LOCAL_MACHINE */
        ret = RegOpenKeyA((HKEY)(HandleToUlong(HKEY_LOCAL_MACHINE) | (ULONG64)0xdeadbeef << 32), "Software", &hkResult);
        ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);
        ok(hkResult != NULL, "expected hkResult != NULL\n");
        RegCloseKey(hkResult);
    }

    /* Try using WOW64 flags when opening a key with a DACL set to verify that
     * the registry access check is performed correctly. Redirection isn't
     * being tested, so the tests don't care about whether the process is
     * running under WOW64. */
    if (!is_64bit)
    {
        skip("Not running WoW64 tests on 32-bit\n");
        return;
    }

    ret = RegCreateKeyExA(HKEY_LOCAL_MACHINE, "Software\\Wine", 0, NULL, 0,
                          KEY_WOW64_32KEY | KEY_ALL_ACCESS, NULL, &hkRoot32, NULL);
    ok(ret == ERROR_SUCCESS || ret == ERROR_ACCESS_DENIED,
       "RegCreateKeyEx with KEY_WOW64_32KEY failed (err=%lu)\n", ret);
    if (ret == ERROR_ACCESS_DENIED) return;
    ok(hkRoot32 != NULL, "hkRoot32 was set\n");

    ret = RegCreateKeyExA(HKEY_LOCAL_MACHINE, "Software\\Wine", 0, NULL, 0,
                          KEY_WOW64_64KEY | KEY_ALL_ACCESS, NULL, &hkRoot64, NULL);
    ok(ret == ERROR_SUCCESS || ret == ERROR_ACCESS_DENIED,
       "RegCreateKeyEx with KEY_WOW64_64KEY failed (err=%lu)\n", ret);
    if (ret == ERROR_ACCESS_DENIED)
    {
        RegDeleteKeyA(hkRoot32, "");
        RegCloseKey(hkRoot32);
        return;
    }
    ok(hkRoot64 != NULL, "hkRoot64 was set\n");

    bRet = AllocateAndInitializeSid(&sid_authority, 1, SECURITY_WORLD_RID,
                                    0, 0, 0, 0, 0, 0, 0, &world_sid);
    ok(bRet == TRUE,
       "Expected AllocateAndInitializeSid to return TRUE, got %d, last error %lu\n", bRet, GetLastError());

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
       "Expected SetEntriesInAclA to return ERROR_SUCCESS, got %lu, last error %lu\n", ret, GetLastError());

    sd = malloc(SECURITY_DESCRIPTOR_MIN_LENGTH);
    bRet = InitializeSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION);
    ok(bRet == TRUE,
       "Expected InitializeSecurityDescriptor to return TRUE, got %d, last error %lu\n", bRet, GetLastError());

    bRet = SetSecurityDescriptorDacl(sd, TRUE, key_acl, FALSE);
    ok(bRet == TRUE,
       "Expected SetSecurityDescriptorDacl to return TRUE, got %d, last error %lu\n", bRet, GetLastError());

    if (limited_user)
    {
        skip("not enough privileges to modify HKLM\n");
    }
    else
    {
        LONG error;

        error = RegSetKeySecurity(hkRoot64, DACL_SECURITY_INFORMATION, sd);
        ok(error == ERROR_SUCCESS,
           "Expected RegSetKeySecurity to return success, got error %lu\n", error);

        error = RegSetKeySecurity(hkRoot32, DACL_SECURITY_INFORMATION, sd);
        ok(error == ERROR_SUCCESS,
           "Expected RegSetKeySecurity to return success, got error %lu\n", error);

        hkResult = NULL;
        ret = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\Wine", 0, KEY_WOW64_64KEY | KEY_READ, &hkResult);
        ok(ret == ERROR_SUCCESS, "RegOpenKeyEx with KEY_WOW64_64KEY failed (err=%lu)\n", ret);
        ok(hkResult != NULL, "hkResult wasn't set\n");
        RegCloseKey(hkResult);

        hkResult = NULL;
        ret = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\Wine", 0, KEY_WOW64_32KEY | KEY_READ, &hkResult);
        ok(ret == ERROR_SUCCESS, "RegOpenKeyEx with KEY_WOW64_32KEY failed (err=%lu)\n", ret);
        ok(hkResult != NULL, "hkResult wasn't set\n");
        RegCloseKey(hkResult);
    }

    free(sd);
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

    /* NULL return key check */
    ret = RegCreateKeyA(hkey_main, "Subkey1", NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "Got unexpected ret %ld.\n", ret);

    ret = RegCreateKeyW(hkey_main, L"Subkey1", NULL);
#ifdef __REACTOS__
    ok(ret == ERROR_INVALID_PARAMETER || broken(ERROR_BADKEY) /* WS03 */, "Got unexpected ret %ld.\n", ret);
#else
    ok(ret == ERROR_INVALID_PARAMETER, "Got unexpected ret %ld.\n", ret);
#endif

    ret = RegCreateKeyExA(hkey_main, "Subkey1", 0, NULL, 0, KEY_NOTIFY, NULL, NULL, NULL);
    ok(ret == ERROR_BADKEY, "Got unexpected ret %ld.\n", ret);

    ret = RegCreateKeyExW(hkey_main, L"Subkey1", 0, NULL, 0, KEY_NOTIFY, NULL, NULL, NULL);
    ok(ret == ERROR_BADKEY, "Got unexpected ret %ld.\n", ret);

    ret = RegCreateKeyExA(hkey_main, "Subkey1", 0, NULL, 0, KEY_NOTIFY, NULL, &hkey1, NULL);
    ok(!ret, "RegCreateKeyExA failed with error %ld\n", ret);
    /* should succeed: all versions of Windows ignore the access rights
     * to the parent handle */
    ret = RegCreateKeyExA(hkey1, "Subkey2", 0, NULL, 0, KEY_SET_VALUE, NULL, &hkey2, NULL);
    ok(!ret, "RegCreateKeyExA failed with error %ld\n", ret);

    /* clean up */
    RegDeleteKeyA(hkey2, "");
    RegDeleteKeyA(hkey1, "");
    RegCloseKey(hkey2);
    RegCloseKey(hkey1);

    /* test creation of volatile keys */
    ret = RegCreateKeyExA(hkey_main, "Volatile", 0, NULL, REG_OPTION_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey1, NULL);
    ok(!ret, "RegCreateKeyExA failed with error %ld\n", ret);
    ret = RegCreateKeyExA(hkey1, "Subkey2", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hkey2, NULL);
    ok(ret == ERROR_CHILD_MUST_BE_VOLATILE, "RegCreateKeyExA failed with error %ld\n", ret);
    if (!ret) RegCloseKey( hkey2 );
    ret = RegCreateKeyExA(hkey1, "Subkey2", 0, NULL, REG_OPTION_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey2, NULL);
    ok(!ret, "RegCreateKeyExA failed with error %ld\n", ret);
    RegCloseKey(hkey2);
    /* should succeed if the key already exists */
    ret = RegCreateKeyExA(hkey1, "Subkey2", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hkey2, NULL);
    ok(!ret, "RegCreateKeyExA failed with error %ld\n", ret);

    /* clean up */
    RegDeleteKeyA(hkey2, "");
    RegDeleteKeyA(hkey1, "");
    RegCloseKey(hkey2);
    RegCloseKey(hkey1);

    /*  beginning backslash character */
    ret = RegCreateKeyExA(hkey_main, "\\Subkey3", 0, NULL, 0, KEY_NOTIFY, NULL, &hkey1, NULL);
    ok(ret == ERROR_BAD_PATHNAME, "expected ERROR_BAD_PATHNAME, got %ld\n", ret);

    /* trailing backslash characters */
    ret = RegCreateKeyExA(hkey_main, "Subkey4\\\\", 0, NULL, 0, KEY_NOTIFY, NULL, &hkey1, NULL);
    ok(ret == ERROR_SUCCESS, "RegCreateKeyExA failed with error %ld\n", ret);
    RegDeleteKeyA(hkey1, "");
    RegCloseKey(hkey1);

    /* WOW64 flags - open an existing key */
    hkey1 = NULL;
    ret = RegCreateKeyExA(HKEY_LOCAL_MACHINE, "Software", 0, NULL, 0, KEY_READ|KEY_WOW64_32KEY, NULL, &hkey1, NULL);
    ok(ret == ERROR_SUCCESS && hkey1 != NULL,
        "RegOpenKeyEx with KEY_WOW64_32KEY failed (err=%lu)\n", ret);
    RegCloseKey(hkey1);

    hkey1 = NULL;
    ret = RegCreateKeyExA(HKEY_LOCAL_MACHINE, "Software", 0, NULL, 0, KEY_READ|KEY_WOW64_64KEY, NULL, &hkey1, NULL);
    ok(ret == ERROR_SUCCESS && hkey1 != NULL,
        "RegOpenKeyEx with KEY_WOW64_64KEY failed (err=%lu)\n", ret);
    RegCloseKey(hkey1);

    /* Try using WOW64 flags when opening a key with a DACL set to verify that
     * the registry access check is performed correctly. Redirection isn't
     * being tested, so the tests don't care about whether the process is
     * running under WOW64. */
    if (!has_wow64())
    {
        skip("WOW64 flags are not recognized\n");
        return;
    }

    ret = RegCreateKeyExA(HKEY_LOCAL_MACHINE, "Software\\Wine", 0, NULL, 0,
                          KEY_WOW64_32KEY | KEY_ALL_ACCESS, NULL, &hkRoot32, NULL);
    if (limited_user)
        ok(ret == ERROR_ACCESS_DENIED && hkRoot32 == NULL,
           "RegCreateKeyEx with KEY_WOW64_32KEY failed (err=%ld)\n", ret);
    else
        ok(ret == ERROR_SUCCESS && hkRoot32 != NULL,
           "RegCreateKeyEx with KEY_WOW64_32KEY failed (err=%ld)\n", ret);

    ret = RegCreateKeyExA(HKEY_LOCAL_MACHINE, "Software\\Wine", 0, NULL, 0,
                          KEY_WOW64_64KEY | KEY_ALL_ACCESS, NULL, &hkRoot64, NULL);
    if (limited_user)
        ok(ret == ERROR_ACCESS_DENIED && hkRoot64 == NULL,
           "RegCreateKeyEx with KEY_WOW64_64KEY failed (err=%ld)\n", ret);
    else
        ok(ret == ERROR_SUCCESS && hkRoot64 != NULL,
           "RegCreateKeyEx with KEY_WOW64_64KEY failed (err=%ld)\n", ret);

    bRet = AllocateAndInitializeSid(&sid_authority, 1, SECURITY_WORLD_RID,
                                    0, 0, 0, 0, 0, 0, 0, &world_sid);
    ok(bRet == TRUE,
       "Expected AllocateAndInitializeSid to return TRUE, got %d, last error %lu\n", bRet, GetLastError());

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
       "Expected SetEntriesInAclA to return ERROR_SUCCESS, got %lu, last error %lu\n", dwRet, GetLastError());

    sd = malloc(SECURITY_DESCRIPTOR_MIN_LENGTH);
    bRet = InitializeSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION);
    ok(bRet == TRUE,
       "Expected InitializeSecurityDescriptor to return TRUE, got %d, last error %lu\n", bRet, GetLastError());

    bRet = SetSecurityDescriptorDacl(sd, TRUE, key_acl, FALSE);
    ok(bRet == TRUE,
       "Expected SetSecurityDescriptorDacl to return TRUE, got %d, last error %lu\n", bRet, GetLastError());

    if (limited_user)
    {
        skip("not enough privileges to modify HKLM\n");
    }
    else
    {
        ret = RegSetKeySecurity(hkRoot64, DACL_SECURITY_INFORMATION, sd);
        ok(ret == ERROR_SUCCESS,
           "Expected RegSetKeySecurity to return success, got error %lu\n", ret);

        ret = RegSetKeySecurity(hkRoot32, DACL_SECURITY_INFORMATION, sd);
        ok(ret == ERROR_SUCCESS,
           "Expected RegSetKeySecurity to return success, got error %lu\n", ret);

        hkey1 = NULL;
        ret = RegCreateKeyExA(HKEY_LOCAL_MACHINE, "Software\\Wine", 0, NULL, 0,
                              KEY_WOW64_64KEY | KEY_READ, NULL, &hkey1, NULL);
        ok(ret == ERROR_SUCCESS && hkey1 != NULL,
           "RegOpenKeyEx with KEY_WOW64_64KEY failed (err=%lu)\n", ret);
        RegCloseKey(hkey1);

        hkey1 = NULL;
        ret = RegCreateKeyExA(HKEY_LOCAL_MACHINE, "Software\\Wine", 0, NULL, 0,
                              KEY_WOW64_32KEY | KEY_READ, NULL, &hkey1, NULL);
        ok(ret == ERROR_SUCCESS && hkey1 != NULL,
           "RegOpenKeyEx with KEY_WOW64_32KEY failed (err=%lu)\n", ret);
        RegCloseKey(hkey1);
    }

    free(sd);
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
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);
    ret = RegCloseKey(hkHandle);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);

    /* try to close the key twice */
    ret = RegCloseKey(hkHandle); /* Windows 95 doesn't mind. */
    ok(ret == ERROR_INVALID_HANDLE || ret == ERROR_SUCCESS,
       "expected ERROR_INVALID_HANDLE or ERROR_SUCCESS, got %ld\n", ret);
    
    /* try to close a NULL handle */
    ret = RegCloseKey(NULL);
    ok(ret == ERROR_INVALID_HANDLE || ret == ERROR_BADKEY, /* Windows 95 returns BADKEY */
       "expected ERROR_INVALID_HANDLE or ERROR_BADKEY, got %ld\n", ret);

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
    ok(ret == ERROR_INVALID_PARAMETER, "got %ld\n", ret);

    ret = RegCreateKeyA(hkey_main, "deleteme", &key);
    ok(ret == ERROR_SUCCESS, "Could not create key, got %ld\n", ret);
    ret = RegDeleteKeyA(key, "");
    ok(ret == ERROR_SUCCESS, "RegDeleteKeyA failed, got %ld\n", ret);
    RegCloseKey(key);
    ret = RegOpenKeyA(hkey_main, "deleteme", &key);
    ok(ret == ERROR_FILE_NOT_FOUND, "Key was not deleted, got %ld\n", ret);
    RegCloseKey(key);

    /* Test deleting 32-bit keys */
    ret = RegCreateKeyExA(hkey_main, "deleteme", 0, NULL, 0, KEY_ALL_ACCESS | KEY_WOW64_32KEY, NULL, &key, NULL);
    ok(ret == ERROR_SUCCESS, "Could not create key, got %ld\n", ret);
    RegCloseKey(key);

    ret = RegOpenKeyExA(hkey_main, "deleteme", 0, KEY_READ | KEY_WOW64_32KEY, &key);
    ok(ret == ERROR_SUCCESS, "Could not open key, got %ld\n", ret);

    ret = RegDeleteKeyExA(key, "", KEY_WOW64_32KEY, 0);
    ok(ret == ERROR_SUCCESS, "RegDeleteKeyExA failed, got %ld\n", ret);
    RegCloseKey(key);

    ret = RegOpenKeyExA(hkey_main, "deleteme", 0, KEY_READ | KEY_WOW64_32KEY, &key);
    ok(ret == ERROR_FILE_NOT_FOUND, "Key was not deleted, got %ld\n", ret);
    RegCloseKey(key);
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

static void delete_dir(const char *path)
{
    char file[2 * MAX_PATH], *p;
    WIN32_FIND_DATAA fd;
    HANDLE hfind;
    BOOL r;

    strcpy(file, path);
    p = file + strlen(file);
    p[0] = '\\';
    p[1] = '*';
    p[2] = 0;
    hfind = FindFirstFileA(file, &fd);
    if (hfind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!strcmp(fd.cFileName, ".") || !strcmp(fd.cFileName, ".."))
                continue;

            strcpy(p + 1, fd.cFileName);
            r = DeleteFileA(file);
            ok(r, "DeleteFile failed on %s: %ld\n", debugstr_a(file), GetLastError());
        } while(FindNextFileA(hfind, &fd));
        FindClose(hfind);
    }

    r = RemoveDirectoryA(path);
    ok(r, "RemoveDirectory failed: %ld\n", GetLastError());
}

static void test_reg_load_key(void)
{
    char saved_key[2 * MAX_PATH], buf[16], *p;
    UNICODE_STRING key_name;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;
    DWORD ret, size;
    HKEY key;

    if (!set_privileges(SE_RESTORE_NAME, TRUE) ||
        !set_privileges(SE_BACKUP_NAME, TRUE))
    {
        win_skip("Failed to set SE_RESTORE_NAME privileges, skipping tests\n");
        return;
    }

    GetTempPathA(MAX_PATH, saved_key);
    strcat(saved_key, "\\wine_reg_test");
    CreateDirectoryA(saved_key, NULL);
    strcat(saved_key, "\\saved_key");

    ret = RegSaveKeyA(hkey_main, saved_key, NULL);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);

    ret = RegLoadKeyA(HKEY_LOCAL_MACHINE, "Test", saved_key);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);

    ret = RegOpenKeyA(HKEY_LOCAL_MACHINE, "Test", &key);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);

    ret = RegSetValueExA(key, "test", 0, REG_SZ, (BYTE *)"value", 6);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);

    /* try to unload though the key handle is live */
    pRtlInitUnicodeString(&key_name, L"\\REGISTRY\\Machine\\Test");
    InitializeObjectAttributes(&attr, &key_name, OBJ_CASE_INSENSITIVE, NULL, NULL);
    status = pNtUnloadKey(&attr);
    ok(status == STATUS_CANNOT_DELETE, "expected STATUS_CANNOT_DELETE, got %08lx\n", status);

    RegCloseKey(key);

    ret = RegUnLoadKeyA(HKEY_LOCAL_MACHINE, "Test");
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);

    ret = RegLoadKeyA(HKEY_LOCAL_MACHINE, "Test", "");
    ok(ret == ERROR_INVALID_PARAMETER, "expected INVALID_PARAMETER, got %ld\n", ret);

    /* check if modifications are saved */
    ret = RegLoadKeyA(HKEY_LOCAL_MACHINE, "Test", saved_key);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);

    ret = RegOpenKeyA(HKEY_LOCAL_MACHINE, "Test", &key);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);

    size = sizeof(buf);
    ret = RegGetValueA(key, NULL, "test", RRF_RT_REG_SZ, NULL, buf, &size);
    todo_wine ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);
    if (ret == ERROR_SUCCESS)
    {
        ok(size == 6, "size = %ld\n", size);
        ok(!strcmp(buf, "value"), "buf = %s\n", buf);
    }

    RegCloseKey(key);

    ret = RegUnLoadKeyA(HKEY_LOCAL_MACHINE, "Test");
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);

    pRtlInitUnicodeString(&key_name, L"\\REGISTRY\\User\\.Default");
    InitializeObjectAttributes(&attr, &key_name, OBJ_CASE_INSENSITIVE, NULL, NULL);
    status = pNtUnloadKey(&attr);
#ifdef __REACTOS__
    ok(status == STATUS_ACCESS_DENIED || broken(status == STATUS_CANNOT_DELETE) /* WS03 */, "expected STATUS_ACCESS_DENIED, got %08lx\n", status);
#else
    ok(status == STATUS_ACCESS_DENIED, "expected STATUS_ACCESS_DENIED, got %08lx\n", status);
#endif

    ret = RegUnLoadKeyA(HKEY_USERS, ".Default");
    ok(ret == ERROR_ACCESS_DENIED, "expected ERROR_ACCESS_DENIED, got %ld\n", ret);

    set_privileges(SE_RESTORE_NAME, FALSE);
    set_privileges(SE_BACKUP_NAME, FALSE);

    p = strrchr(saved_key, '\\');
    *p = 0;
    delete_dir(saved_key);
}

/* Helper function to wait for a file blocked by the registry to be available */
static void wait_file_available(char *path)
{
    int attempts = 0;
    HANDLE file = NULL;

    while (((file = CreateFileA(path, GENERIC_READ, 0, NULL,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
            && attempts++ < 10)
    {
        Sleep(200);
    }
    ok(file != INVALID_HANDLE_VALUE, "couldn't open file %s after 10 attempts error %ld.\n", path, GetLastError());
    CloseHandle(file);
}

static void test_reg_load_app_key(void)
{
#if defined(__REACTOS__) && DLL_EXPORT_VERSION < 0x600
    skip("test_reg_load_app_key() can't be built unless DLL_EXPORT_VERSION >= 0x600\n");
#else
    DWORD ret, size;
    char hivefilepath[2 * MAX_PATH], *p;
    const BYTE test_data[] = "Hello World";
    BYTE output[sizeof(test_data)];
    HKEY appkey = NULL;

    if (!set_privileges(SE_BACKUP_NAME, TRUE))
    {
        win_skip("Failed to set SE_BACKUP_NAME privileges, skipping tests\n");
        return;
    }

    GetTempPathA(MAX_PATH, hivefilepath);
    strcat(hivefilepath, "\\wine_reg_test");
    CreateDirectoryA(hivefilepath, NULL);
    strcat(hivefilepath, "\\saved_key");

    ret = RegSaveKeyA(hkey_main, hivefilepath, NULL);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);

    set_privileges(SE_BACKUP_NAME, FALSE);

    /* Test simple key load */
    /* Check if the changes are saved */
    ret = RegLoadAppKeyA(hivefilepath, &appkey, KEY_READ | KEY_WRITE, 0, 0);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);
    ok(appkey != NULL, "got a null key\n");

    ret = RegSetValueExA(appkey, "testkey", 0, REG_BINARY, test_data, sizeof(test_data));
    todo_wine ok(ret == ERROR_SUCCESS, "couldn't set key value %lx\n", ret);
    RegCloseKey(appkey);

    wait_file_available(hivefilepath);

    appkey = NULL;
    ret = RegLoadAppKeyA(hivefilepath, &appkey, KEY_READ, 0, 0);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);
    ok(appkey != NULL, "got a null key\n");

    size = sizeof(test_data);
    memset(output, 0xff, sizeof(output));
    ret = RegGetValueA(appkey, NULL, "testkey", RRF_RT_REG_BINARY, NULL, output, &size);
    todo_wine ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);
    ok(size == sizeof(test_data), "size doesn't match %ld != %ld\n", size, (DWORD)sizeof(test_data));
    todo_wine ok(!memcmp(test_data, output, sizeof(test_data)), "output is not what expected\n");

    RegCloseKey(appkey);

    wait_file_available(hivefilepath);

    p = strrchr(hivefilepath, '\\');
    *p = 0;
    delete_dir(hivefilepath);
#endif
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
    ok( ret, "GetComputerName failed err = %ld\n", GetLastError());
    if( !ret) return;

    lstrcpyA(netwName, "\\\\");
    lstrcpynA(netwName+2, compName, MAX_COMPUTERNAME_LENGTH + 1);

    retl = RegConnectRegistryA( compName, HKEY_LOCAL_MACHINE, &hkey);
    ok( !retl, "RegConnectRegistryA failed err = %ld\n", retl);
    if( !retl) RegCloseKey( hkey);

    retl = RegConnectRegistryA( netwName, HKEY_LOCAL_MACHINE, &hkey);
    ok( !retl, "RegConnectRegistryA failed err = %ld\n", retl);
    if( !retl) RegCloseKey( hkey);

    SetLastError(0xdeadbeef);
    schnd = OpenSCManagerA( compName, NULL, GENERIC_READ); 
    if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("OpenSCManagerA is not implemented\n");
        return;
    }

    ok( schnd != NULL, "OpenSCManagerA failed err = %ld\n", GetLastError());
    CloseServiceHandle( schnd);

    SetLastError(0xdeadbeef);
    schnd = OpenSCManagerA( netwName, NULL, GENERIC_READ); 
    ok( schnd != NULL, "OpenSCManagerA failed err = %ld\n", GetLastError());
    CloseServiceHandle( schnd);

}

static void test_reg_query_value(void)
{
    HKEY subkey;
    CHAR val[MAX_PATH];
    WCHAR valW[5];
    LONG size, ret;

    ret = RegCreateKeyA(hkey_main, "subkey", &subkey);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);

    ret = RegSetValueA(subkey, NULL, REG_SZ, "data", 4);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);

    /* try an invalid hkey */
    SetLastError(0xdeadbeef);
    size = MAX_PATH;
    ret = RegQueryValueA((HKEY)0xcafebabe, "subkey", val, &size);
    ok(ret == ERROR_INVALID_HANDLE ||
       ret == ERROR_BADKEY || /* Windows 98 returns BADKEY */
       ret == ERROR_ACCESS_DENIED, /* non-admin winxp */
       "Expected ERROR_INVALID_HANDLE or ERROR_BADKEY, got %ld\n", ret);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* try a NULL hkey */
    SetLastError(0xdeadbeef);
    size = MAX_PATH;
    ret = RegQueryValueA(NULL, "subkey", val, &size);
    ok(ret == ERROR_INVALID_HANDLE || ret == ERROR_BADKEY, /* Windows 98 returns BADKEY */
       "Expected ERROR_INVALID_HANDLE or ERROR_BADKEY, got %ld\n", ret);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* try a NULL value */
    size = MAX_PATH;
    ret = RegQueryValueA(hkey_main, "subkey", NULL, &size);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);
    ok(size == 5, "Expected 5, got %ld\n", size);

    /* try a NULL size */
    SetLastError(0xdeadbeef);
    val[0] = '\0';
    ret = RegQueryValueA(hkey_main, "subkey", val, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", ret);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());
    ok(!val[0], "Expected val to be untouched, got %s\n", val);

    /* try a NULL value and size */
    ret = RegQueryValueA(hkey_main, "subkey", NULL, NULL);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);

    /* try a size too small */
    SetLastError(0xdeadbeef);
    val[0] = '\0';
    size = 1;
    ret = RegQueryValueA(hkey_main, "subkey", val, &size);
    ok(ret == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %ld\n", ret);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());
    ok(!val[0], "Expected val to be untouched, got %s\n", val);
    ok(size == 5, "Expected 5, got %ld\n", size);

    /* successfully read the value using 'subkey' */
    size = MAX_PATH;
    ret = RegQueryValueA(hkey_main, "subkey", val, &size);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);
    ok(!lstrcmpA(val, "data"), "Expected 'data', got '%s'\n", val);
    ok(size == 5, "Expected 5, got %ld\n", size);

    /* successfully read the value using the subkey key */
    size = MAX_PATH;
    ret = RegQueryValueA(subkey, NULL, val, &size);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);
    ok(!lstrcmpA(val, "data"), "Expected 'data', got '%s'\n", val);
    ok(size == 5, "Expected 5, got %ld\n", size);

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
    ok(ret == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %ld\n", ret);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());
    ok(!valW[0], "Expected valW to be untouched\n");
    ok(size == 10, "Got wrong size: %ld\n", size);

    /* unicode - try size in WCHARS */
    SetLastError(0xdeadbeef);
    size = ARRAY_SIZE(valW);
    ret = RegQueryValueW(subkey, NULL, valW, &size);
    ok(ret == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %ld\n", ret);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());
    ok(!valW[0], "Expected valW to be untouched\n");
    ok(size == 10, "Got wrong size: %ld\n", size);

    /* unicode - successfully read the value */
    size = sizeof(valW);
    ret = RegQueryValueW(subkey, NULL, valW, &size);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);
    ok(!lstrcmpW(valW, L"data"), "Got wrong value\n");
    ok(size == 10, "Got wrong size: %ld\n", size);

    /* unicode - set the value without a NULL terminator */
    ret = RegSetValueW(subkey, NULL, REG_SZ, L"data", 8);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);

    /* unicode - read the unterminated value, value is terminated for us */
    memset(valW, 'a', sizeof(valW));
    size = sizeof(valW);
    ret = RegQueryValueW(subkey, NULL, valW, &size);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);
    ok(!lstrcmpW(valW, L"data"), "Got wrong value\n");
    ok(size == 10, "Got wrong size: %ld\n", size);

cleanup:
    RegDeleteKeyA(subkey, "");
    RegCloseKey(subkey);
}

static void test_reg_query_info(void)
{
    HKEY subkey;
    HKEY subsubkey;
    LONG ret;
    char classbuffer[32];
    WCHAR classbufferW[32];
    char expectbuffer[32];
    WCHAR expectbufferW[32];
    char subkey_class[] = "subkey class";
    WCHAR subkey_classW[] = L"subkey class";
    char subsubkey_class[] = "subsubkey class";
    DWORD classlen;
    DWORD subkeys, maxsubkeylen, maxclasslen;
    DWORD values, maxvaluenamelen, maxvaluelen;
    DWORD sdlen;
    FILETIME lastwrite;

    ret = RegCreateKeyExA(hkey_main, "subkey", 0, subkey_class, 0, KEY_ALL_ACCESS, NULL, &subkey, NULL);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);

    /* all parameters NULL */
    ret = RegQueryInfoKeyA(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ok(ret == ERROR_INVALID_HANDLE, "ret = %ld\n", ret);

    ret = RegQueryInfoKeyW(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ok(ret == ERROR_INVALID_HANDLE, "ret = %ld\n", ret);

    /* not requesting any information */
    ret = RegQueryInfoKeyA(subkey, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ok(ret == ERROR_SUCCESS, "ret = %ld\n", ret);

    ret = RegQueryInfoKeyW(subkey, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ok(ret == ERROR_SUCCESS, "ret = %ld\n", ret);

    /* class without length is invalid */
    memset(classbuffer, 0x55, sizeof(classbuffer));
    ret = RegQueryInfoKeyA(subkey, classbuffer, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "ret = %ld\n", ret);
    ok(classbuffer[0] == 0x55, "classbuffer[0] = 0x%x\n", classbuffer[0]);

    memset(classbufferW, 0x55, sizeof(classbufferW));
    ret = RegQueryInfoKeyW(subkey, classbufferW, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "ret = %ld\n", ret);
    ok(classbufferW[0] == 0x5555, "classbufferW[0] = 0x%x\n", classbufferW[0]);

    /* empty key */
    sdlen = classlen =0;
    ret = RegQueryInfoKeyA(subkey, NULL, &classlen, NULL, &subkeys, &maxsubkeylen, &maxclasslen, &values, &maxvaluenamelen, &maxvaluelen, &sdlen, &lastwrite);
    ok(ret == ERROR_SUCCESS, "ret = %ld\n", ret);
    ok(classlen == strlen(subkey_class), "classlen = %lu\n", classlen);
    ok(subkeys == 0, "subkeys = %lu\n", subkeys);
    ok(maxsubkeylen == 0, "maxsubkeylen = %lu\n", maxsubkeylen);
    ok(maxclasslen == 0, "maxclasslen = %lu\n", maxclasslen);
    ok(values == 0, "values = %lu\n", values);
    ok(maxvaluenamelen == 0, "maxvaluenamelen = %lu\n", maxvaluenamelen);
    ok(maxvaluelen == 0, "maxvaluelen = %lu\n", maxvaluelen);
    todo_wine ok(sdlen != 0, "sdlen = %lu\n", sdlen);
    ok(lastwrite.dwLowDateTime != 0, "lastwrite.dwLowDateTime = %lu\n", lastwrite.dwLowDateTime);
    ok(lastwrite.dwHighDateTime != 0, "lastwrite.dwHighDateTime = %lu\n", lastwrite.dwHighDateTime);

    sdlen = classlen = 0;
    ret = RegQueryInfoKeyW(subkey, NULL, &classlen, NULL, &subkeys, &maxsubkeylen, &maxclasslen, &values, &maxvaluenamelen, &maxvaluelen, &sdlen, &lastwrite);
    ok(ret == ERROR_SUCCESS, "ret = %ld\n", ret);
    ok(classlen == strlen(subkey_class), "classlen = %lu\n", classlen);
    ok(subkeys == 0, "subkeys = %lu\n", subkeys);
    ok(maxsubkeylen == 0, "maxsubkeylen = %lu\n", maxsubkeylen);
    ok(maxclasslen == 0, "maxclasslen = %lu\n", maxclasslen);
    ok(values == 0, "values = %lu\n", values);
    ok(maxvaluenamelen == 0, "maxvaluenamelen = %lu\n", maxvaluenamelen);
    ok(maxvaluelen == 0, "maxvaluelen = %lu\n", maxvaluelen);
    todo_wine ok(sdlen != 0, "sdlen = %lu\n", sdlen);
    ok(lastwrite.dwLowDateTime != 0, "lastwrite.dwLowDateTime = %lu\n", lastwrite.dwLowDateTime);
    ok(lastwrite.dwHighDateTime != 0, "lastwrite.dwHighDateTime = %lu\n", lastwrite.dwHighDateTime);

    ret = RegCreateKeyExA(subkey, "subsubkey", 0, subsubkey_class, 0, KEY_ALL_ACCESS, NULL, &subsubkey, NULL);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);

    ret = RegSetValueExA(subkey, NULL, 0, REG_SZ, (const BYTE*)"data", 5);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);

    /* with subkey & default value */
    sdlen = classlen = 0;
    ret = RegQueryInfoKeyA(subkey, NULL, &classlen, NULL, &subkeys, &maxsubkeylen, &maxclasslen, &values, &maxvaluenamelen, &maxvaluelen, &sdlen, &lastwrite);
    ok(ret == ERROR_SUCCESS, "ret = %ld\n", ret);
    ok(classlen == strlen(subkey_class), "classlen = %lu\n", classlen);
    ok(subkeys == 1, "subkeys = %lu\n", subkeys);
    ok(maxsubkeylen == strlen("subsubkey"), "maxsubkeylen = %lu\n", maxsubkeylen);
    ok(maxclasslen == strlen(subsubkey_class), "maxclasslen = %lu\n", maxclasslen);
    ok(values == 1, "values = %lu\n", values);
    ok(maxvaluenamelen == 0, "maxvaluenamelen = %lu\n", maxvaluenamelen);
    ok(maxvaluelen == sizeof("data") * sizeof(WCHAR), "maxvaluelen = %lu\n", maxvaluelen);
    todo_wine ok(sdlen != 0, "sdlen = %lu\n", sdlen);
    ok(lastwrite.dwLowDateTime != 0, "lastwrite.dwLowDateTime = %lu\n", lastwrite.dwLowDateTime);
    ok(lastwrite.dwHighDateTime != 0, "lastwrite.dwHighDateTime = %lu\n", lastwrite.dwHighDateTime);

    sdlen = classlen = 0;
    ret = RegQueryInfoKeyW(subkey, NULL, &classlen, NULL, &subkeys, &maxsubkeylen, &maxclasslen, &values, &maxvaluenamelen, &maxvaluelen, &sdlen, &lastwrite);
    ok(ret == ERROR_SUCCESS, "ret = %ld\n", ret);
    ok(classlen == strlen(subkey_class), "classlen = %lu\n", classlen);
    ok(subkeys == 1, "subkeys = %lu\n", subkeys);
    ok(maxsubkeylen == strlen("subsubkey"), "maxsubkeylen = %lu\n", maxsubkeylen);
    ok(maxclasslen == strlen(subsubkey_class), "maxclasslen = %lu\n", maxclasslen);
    ok(values == 1, "values = %lu\n", values);
    ok(maxvaluenamelen == 0, "maxvaluenamelen = %lu\n", maxvaluenamelen);
    ok(maxvaluelen == sizeof("data") * sizeof(WCHAR), "maxvaluelen = %lu\n", maxvaluelen);
    todo_wine ok(sdlen != 0, "sdlen = %lu\n", sdlen);
    ok(lastwrite.dwLowDateTime != 0, "lastwrite.dwLowDateTime = %lu\n", lastwrite.dwLowDateTime);
    ok(lastwrite.dwHighDateTime != 0, "lastwrite.dwHighDateTime = %lu\n", lastwrite.dwHighDateTime);

    ret = RegSetValueExA(subkey, "value one", 0, REG_SZ, (const BYTE*)"first value data", 17);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);

    ret = RegSetValueExA(subkey, "value 2", 0, REG_SZ, (const BYTE*)"second value data", 18);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);

    /* with named value */
    classlen = 0;
    ret = RegQueryInfoKeyA(subkey, NULL, &classlen, NULL, &subkeys, &maxsubkeylen, &maxclasslen, &values, &maxvaluenamelen, &maxvaluelen, &sdlen, &lastwrite);
    ok(ret == ERROR_SUCCESS, "ret = %ld\n", ret);
    ok(values == 3, "values = %lu\n", values);
    ok(maxvaluenamelen == strlen("value one"), "maxvaluenamelen = %lu\n", maxvaluenamelen);
    ok(maxvaluelen == sizeof("second value data") * sizeof(WCHAR), "maxvaluelen = %lu\n", maxvaluelen);

    classlen = 0;
    ret = RegQueryInfoKeyW(subkey, NULL, &classlen, NULL, &subkeys, &maxsubkeylen, &maxclasslen, &values, &maxvaluenamelen, &maxvaluelen, &sdlen, &lastwrite);
    ok(ret == ERROR_SUCCESS, "ret = %ld\n", ret);
    ok(values == 3, "values = %lu\n", values);
    ok(maxvaluenamelen == strlen("value one"), "maxvaluenamelen = %lu\n", maxvaluenamelen);
    ok(maxvaluelen == sizeof("second value data") * sizeof(WCHAR), "maxvaluelen = %lu\n", maxvaluelen);

    /* class name with zero size buffer */
    memset(classbuffer, 0x55, sizeof(classbuffer));
    classlen = 0;
    ret = RegQueryInfoKeyA(subkey, classbuffer, &classlen, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ok(ret == ERROR_SUCCESS, "ret = %ld\n", ret);
    todo_wine ok(classlen == 0, "classlen = %lu\n", classlen);
    memset(expectbuffer, 0x55, sizeof(expectbuffer));
    ok(!memcmp(classbuffer, expectbuffer, sizeof(classbuffer)), "classbuffer was modified\n");

    memset(classbufferW, 0x55, sizeof(classbufferW));
    classlen = 0;
    ret = RegQueryInfoKeyW(subkey, classbufferW, &classlen, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ok(ret == ERROR_SUCCESS, "ret = %ld\n", ret);
    todo_wine ok(classlen == 0, "classlen = %lu\n", classlen);
    memset(expectbufferW, 0x55, sizeof(expectbufferW));
    ok(!memcmp(classbufferW, expectbufferW, sizeof(classbufferW)), "classbufferW was modified\n");

    /* class name with one char buffer */
    memset(classbuffer, 0x55, sizeof(classbuffer));
    classlen = 1;
    ret = RegQueryInfoKeyA(subkey, classbuffer, &classlen, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ok(ret == ERROR_MORE_DATA, "ret = %ld\n", ret);
    ok(classlen == 0, "classlen = %lu\n", classlen);
    memset(expectbuffer, 0x55, sizeof(expectbuffer));
    expectbuffer[0] = 0;
    ok(!memcmp(classbuffer, expectbuffer, sizeof(classbuffer)), "classbuffer was modified\n");

    memset(classbufferW, 0x55, sizeof(classbufferW));
    classlen = 1;
    ret = RegQueryInfoKeyW(subkey, classbufferW, &classlen, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    /* failure-code changed to ERROR_MORE_DATA in recent win10  */
    ok((ret == ERROR_INSUFFICIENT_BUFFER) || (ret == ERROR_MORE_DATA), "ret = %ld\n", ret);
    ok(classlen == 0 /* win8 */ ||
       classlen == strlen(subkey_class), "classlen = %lu\n", classlen);
    memset(expectbufferW, 0x55, sizeof(expectbufferW));
    ok(!memcmp(classbufferW, expectbufferW, sizeof(classbufferW)), "classbufferW was modified\n");

    /* class name with buffer one char too small */
    memset(classbuffer, 0x55, sizeof(classbuffer));
    classlen = sizeof(subkey_class) - 1;
    ret = RegQueryInfoKeyA(subkey, classbuffer, &classlen, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ok(ret == ERROR_MORE_DATA, "ret = %ld\n", ret);
    ok(classlen == sizeof(subkey_class) - 2, "classlen = %lu\n", classlen);
    memset(expectbuffer, 0x55, sizeof(expectbuffer));
    strcpy(expectbuffer, subkey_class);
    expectbuffer[sizeof(subkey_class) - 2] = 0;
    expectbuffer[sizeof(subkey_class) - 1] = 0x55;
    ok(!memcmp(classbuffer, expectbuffer, sizeof(classbuffer)),
       "classbuffer = %.*s, expected %s\n",
       (int)sizeof(classbuffer), classbuffer, expectbuffer);

    memset(classbufferW, 0x55, sizeof(classbufferW));
    classlen = sizeof(subkey_class) - 1;
    ret = RegQueryInfoKeyW(subkey, classbufferW, &classlen, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ok(ret == ERROR_INSUFFICIENT_BUFFER, "ret = %ld\n", ret);
    ok(classlen == sizeof(subkey_class) - 2 /* win8 */ ||
       classlen == strlen(subkey_class), "classlen = %lu\n", classlen);
    memset(expectbufferW, 0x55, sizeof(expectbufferW));
    ok(!memcmp(classbufferW, expectbufferW, sizeof(classbufferW)), "classbufferW was modified\n");

    /* class name with large enough buffer */
    memset(classbuffer, 0x55, sizeof(classbuffer));
    classlen = sizeof(subkey_class);
    ret = RegQueryInfoKeyA(subkey, classbuffer, &classlen, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ok(ret == ERROR_SUCCESS, "ret = %ld\n", ret);
    ok(classlen == sizeof(subkey_class) - 1, "classlen = %lu\n", classlen);
    memset(expectbuffer, 0x55, sizeof(expectbuffer));
    strcpy(expectbuffer, subkey_class);
    ok(!memcmp(classbuffer, expectbuffer, sizeof(classbuffer)),
       "classbuffer = \"%.*s\", expected %s\n",
       (int)sizeof(classbuffer), classbuffer, expectbuffer);

    memset(classbuffer, 0x55, sizeof(classbuffer));
    classlen = 0xdeadbeef;
    ret = RegQueryInfoKeyA(subkey, classbuffer, &classlen, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ok(ret == ERROR_SUCCESS, "ret = %ld\n", ret);
    ok(classlen == sizeof(subkey_class) - 1, "classlen = %lu\n", classlen);
    memset(expectbuffer, 0x55, sizeof(expectbuffer));
    strcpy(expectbuffer, subkey_class);
    ok(!memcmp(classbuffer, expectbuffer, sizeof(classbuffer)),
       "classbuffer = \"%.*s\", expected %s\n",
       (int)sizeof(classbuffer), classbuffer, expectbuffer);

    memset(classbufferW, 0x55, sizeof(classbufferW));
    classlen = sizeof(subkey_class);
    ret = RegQueryInfoKeyW(subkey, classbufferW, &classlen, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ok(ret == ERROR_SUCCESS, "ret = %ld\n", ret);
    ok(classlen == sizeof(subkey_class) - 1, "classlen = %lu\n", classlen);
    memset(expectbufferW, 0x55, sizeof(expectbufferW));
    lstrcpyW(expectbufferW, subkey_classW);
    ok(!memcmp(classbufferW, expectbufferW, sizeof(classbufferW)),
       "classbufferW = %s, expected %s\n",
       wine_dbgstr_wn(classbufferW, ARRAY_SIZE(classbufferW)), wine_dbgstr_w(expectbufferW));

    memset(classbufferW, 0x55, sizeof(classbufferW));
    classlen = 0xdeadbeef;
    ret = RegQueryInfoKeyW(subkey, classbufferW, &classlen, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ok(ret == ERROR_SUCCESS, "ret = %ld\n", ret);
    ok(classlen == sizeof(subkey_class) - 1, "classlen = %lu\n", classlen);
    memset(expectbufferW, 0x55, sizeof(expectbufferW));
    lstrcpyW(expectbufferW, subkey_classW);
    ok(!memcmp(classbufferW, expectbufferW, sizeof(classbufferW)),
       "classbufferW = %s, expected %s\n",
       wine_dbgstr_wn(classbufferW, ARRAY_SIZE(classbufferW)), wine_dbgstr_w(expectbufferW));

    RegDeleteKeyA(subsubkey, "");
    RegCloseKey(subsubkey);
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
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);

    /* Off-by-one RegSetValueExA -> adds a trailing '\0'! */
    insize=sizeof(string)-1;
    ret = RegSetValueExA(subkey, "stringtest", 0, REG_SZ, (BYTE*)string, insize);
    ok(ret == ERROR_SUCCESS, "RegSetValueExA failed: %ld\n", ret);
    outsize=insize;
    ret = RegQueryValueExA(subkey, "stringtest", NULL, NULL, buffer, &outsize);
    ok(ret == ERROR_MORE_DATA, "RegQueryValueExA returned: %ld\n", ret);

    /* Off-by-two RegSetValueExA -> no trailing '\0' */
    insize=sizeof(string)-2;
    ret = RegSetValueExA(subkey, "stringtest", 0, REG_SZ, (BYTE*)string, insize);
    ok(ret == ERROR_SUCCESS, "RegSetValueExA failed: %ld\n", ret);
    outsize=0;
    ret = RegQueryValueExA(subkey, "stringtest", NULL, NULL, NULL, &outsize);
    ok(ret == ERROR_SUCCESS, "RegQueryValueExA failed: %ld\n", ret);
    ok(outsize == insize, "wrong size %lu != %lu\n", outsize, insize);

    /* RegQueryValueExA may return a string with no trailing '\0' */
    outsize=insize;
    memset(buffer, 0xbd, sizeof(buffer));
    ret = RegQueryValueExA(subkey, "stringtest", NULL, NULL, buffer, &outsize);
    ok(ret == ERROR_SUCCESS, "RegQueryValueExA failed: %ld\n", ret);
    ok(outsize == insize, "wrong size: %lu != %lu\n", outsize, insize);
    ok(memcmp(buffer, string, outsize) == 0, "bad string: %s/%lu != %s\n",
       debugstr_an((char*)buffer, outsize), outsize, string);
    ok(buffer[insize] == 0xbd, "buffer overflow at %lu %02x\n", insize, buffer[insize]);

    /* RegQueryValueExA adds a trailing '\0' if there is room */
    outsize=insize+1;
    memset(buffer, 0xbd, sizeof(buffer));
    ret = RegQueryValueExA(subkey, "stringtest", NULL, NULL, buffer, &outsize);
    ok(ret == ERROR_SUCCESS, "RegQueryValueExA failed: %ld\n", ret);
    ok(outsize == insize, "wrong size: %lu != %lu\n", outsize, insize);
    ok(memcmp(buffer, string, outsize) == 0, "bad string: %s/%lu != %s\n",
       debugstr_an((char*)buffer, outsize), outsize, string);
    ok(buffer[insize] == 0, "buffer overflow at %lu %02x\n", insize, buffer[insize]);

    /* RegEnumValueA may return a string with no trailing '\0' */
    outsize=insize;
    memset(buffer, 0xbd, sizeof(buffer));
    nsize=sizeof(name);
    ret = RegEnumValueA(subkey, 0, name, &nsize, NULL, NULL, buffer, &outsize);
    ok(ret == ERROR_SUCCESS, "RegEnumValueA failed: %ld\n", ret);
    ok(strcmp(name, "stringtest") == 0, "wrong name: %s\n", name);
    ok(outsize == insize, "wrong size: %lu != %lu\n", outsize, insize);
    ok(memcmp(buffer, string, outsize) == 0, "bad string: %s/%lu != %s\n",
       debugstr_an((char*)buffer, outsize), outsize, string);
    ok(buffer[insize] == 0xbd, "buffer overflow at %lu %02x\n", insize, buffer[insize]);

    /* RegEnumValueA adds a trailing '\0' if there is room */
    outsize=insize+1;
    memset(buffer, 0xbd, sizeof(buffer));
    nsize=sizeof(name);
    ret = RegEnumValueA(subkey, 0, name, &nsize, NULL, NULL, buffer, &outsize);
    ok(ret == ERROR_SUCCESS, "RegEnumValueA failed: %ld\n", ret);
    ok(strcmp(name, "stringtest") == 0, "wrong name: %s\n", name);
    ok(outsize == insize, "wrong size: %lu != %lu\n", outsize, insize);
    ok(memcmp(buffer, string, outsize) == 0, "bad string: %s/%lu != %s\n",
       debugstr_an((char*)buffer, outsize), outsize, string);
    ok(buffer[insize] == 0, "buffer overflow at %lu %02x\n", insize, buffer[insize]);

    /* RegGetValueA always adds the trailing '\0' */
    if (pRegGetValueA)
    {
        outsize = insize;
        ret = pRegGetValueA(subkey, NULL, "stringtest", RRF_RT_REG_SZ, NULL, buffer, &outsize);
        ok(ret == ERROR_MORE_DATA, "RegGetValueA returned: %ld\n", ret);
        ok(outsize == insize + 1, "wrong size: %lu != %lu\n", outsize, insize + 1);
        memset(buffer, 0xbd, sizeof(buffer));
        ret = pRegGetValueA(subkey, NULL, "stringtest", RRF_RT_REG_SZ, NULL, buffer, &outsize);
        ok(ret == ERROR_SUCCESS, "RegGetValueA returned: %ld\n", ret);
        ok(outsize == insize + 1, "wrong size: %lu != %lu\n", outsize, insize + 1);
        ok(memcmp(buffer, string, insize) == 0, "bad string: %s/%lu != %s\n",
           debugstr_an((char*)buffer, insize), insize, string);
        ok(buffer[insize] == 0, "buffer overflow at %lu %02x\n", insize, buffer[insize]);
    }

    RegDeleteKeyA(subkey, "");
    RegCloseKey(subkey);
}

static void test_multistring_termination(void)
{
    HKEY subkey;
    LSTATUS ret;
    static const char multistring[] = "Aa\0Bb\0Cc\0";
    char name[sizeof("multistringtest")];
    BYTE buffer[sizeof(multistring)];
    DWORD insize, outsize, nsize;

    ret = RegCreateKeyA(hkey_main, "multistring_termination", &subkey);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);

    /* Off-by-one RegSetValueExA -> only one trailing '\0' */
    insize = sizeof(multistring) - 1;
    ret = RegSetValueExA(subkey, "multistringtest", 0, REG_SZ, (BYTE*)multistring, insize);
    ok(ret == ERROR_SUCCESS, "RegSetValueExA failed: %ld\n", ret);
    outsize = 0;
    ret = RegQueryValueExA(subkey, "multistringtest", NULL, NULL, NULL, &outsize);
    ok(ret == ERROR_SUCCESS, "RegQueryValueExA failed: %ld\n", ret);
    ok(outsize == insize, "wrong size %lu != %lu\n", outsize, insize);

    /* Off-by-two RegSetValueExA -> adds a trailing '\0'! */
    insize = sizeof(multistring) - 2;
    ret = RegSetValueExA(subkey, "multistringtest", 0, REG_SZ, (BYTE*)multistring, insize);
    ok(ret == ERROR_SUCCESS, "RegSetValueExA failed: %ld\n", ret);
    outsize = insize;
    ret = RegQueryValueExA(subkey, "multistringtest", NULL, NULL, buffer, &outsize);
    ok(ret == ERROR_MORE_DATA, "RegQueryValueExA returned: %ld\n", ret);

    /* Off-by-three RegSetValueExA -> no trailing '\0' */
    insize = sizeof(multistring) - 3;
    ret = RegSetValueExA(subkey, "multistringtest", 0, REG_SZ, (BYTE*)multistring, insize);
    ok(ret == ERROR_SUCCESS, "RegSetValueExA failed: %ld\n", ret);
    outsize = 0;
    ret = RegQueryValueExA(subkey, "multistringtest", NULL, NULL, NULL, &outsize);
    ok(ret == ERROR_SUCCESS, "RegQueryValueExA failed: %ld\n", ret);
    ok(outsize == insize, "wrong size %lu != %lu\n", outsize, insize);

    /* RegQueryValueExA may return a multistring with no trailing '\0' */
    outsize = insize;
    memset(buffer, 0xbd, sizeof(buffer));
    ret = RegQueryValueExA(subkey, "multistringtest", NULL, NULL, buffer, &outsize);
    ok(ret == ERROR_SUCCESS, "RegQueryValueExA failed: %ld\n", ret);
    ok(outsize == insize, "wrong size: %lu != %lu\n", outsize, insize);
    ok(memcmp(buffer, multistring, outsize) == 0, "bad multistring: %s/%lu != %s\n",
       debugstr_an((char*)buffer, outsize), outsize, multistring);
    ok(buffer[insize] == 0xbd, "buffer overflow at %lu %02x\n", insize, buffer[insize]);

    /* RegQueryValueExA adds one trailing '\0' if there is room */
    outsize = insize + 1;
    memset(buffer, 0xbd, sizeof(buffer));
    ret = RegQueryValueExA(subkey, "multistringtest", NULL, NULL, buffer, &outsize);
    ok(ret == ERROR_SUCCESS, "RegQueryValueExA failed: %ld\n", ret);
    ok(outsize == insize, "wrong size: %lu != %lu\n", outsize, insize);
    ok(memcmp(buffer, multistring, outsize) == 0, "bad multistring: %s/%lu != %s\n",
       debugstr_an((char*)buffer, outsize), outsize, multistring);
    ok(buffer[insize] == 0, "buffer overflow at %lu %02x\n", insize, buffer[insize]);

    /* RegQueryValueExA doesn't add a second trailing '\0' even if there is room */
    outsize = insize + 2;
    memset(buffer, 0xbd, sizeof(buffer));
    ret = RegQueryValueExA(subkey, "multistringtest", NULL, NULL, buffer, &outsize);
    ok(ret == ERROR_SUCCESS, "RegQueryValueExA failed: %ld\n", ret);
    ok(outsize == insize, "wrong size: %lu != %lu\n", outsize, insize);
    ok(memcmp(buffer, multistring, outsize) == 0, "bad multistring: %s/%lu != %s\n",
       debugstr_an((char*)buffer, outsize), outsize, multistring);
    ok(buffer[insize + 1] == 0xbd, "buffer overflow at %lu %02x\n", insize, buffer[insize + 1]);

    /* RegEnumValueA may return a multistring with no trailing '\0' */
    outsize = insize;
    memset(buffer, 0xbd, sizeof(buffer));
    nsize = sizeof(name);
    ret = RegEnumValueA(subkey, 0, name, &nsize, NULL, NULL, buffer, &outsize);
    ok(ret == ERROR_SUCCESS, "RegEnumValueA failed: %ld\n", ret);
    ok(strcmp(name, "multistringtest") == 0, "wrong name: %s\n", name);
    ok(outsize == insize, "wrong size: %lu != %lu\n", outsize, insize);
    ok(memcmp(buffer, multistring, outsize) == 0, "bad multistring: %s/%lu != %s\n",
       debugstr_an((char*)buffer, outsize), outsize, multistring);
    ok(buffer[insize] == 0xbd, "buffer overflow at %lu %02x\n", insize, buffer[insize]);

    /* RegEnumValueA adds one trailing '\0' even if there's room for two */
    outsize = insize + 2;
    memset(buffer, 0xbd, sizeof(buffer));
    nsize = sizeof(name);
    ret = RegEnumValueA(subkey, 0, name, &nsize, NULL, NULL, buffer, &outsize);
    ok(ret == ERROR_SUCCESS, "RegEnumValueA failed: %ld\n", ret);
    ok(strcmp(name, "multistringtest") == 0, "wrong name: %s\n", name);
    ok(outsize == insize, "wrong size: %lu != %lu\n", outsize, insize);
    ok(memcmp(buffer, multistring, outsize) == 0, "bad multistring: %s/%lu != %s\n",
       debugstr_an((char*)buffer, outsize), outsize, multistring);
    ok(buffer[insize] == 0, "buffer overflow at %lu %02x\n", insize, buffer[insize]);
    ok(buffer[insize + 1] == 0xbd, "buffer overflow at %lu %02x\n", insize, buffer[insize]);

    /* RegGetValueA always adds one trailing '\0' even if there's room for two */
    if (pRegGetValueA)
    {
        outsize = insize;
        ret = pRegGetValueA(subkey, NULL, "multistringtest", RRF_RT_REG_SZ, NULL, buffer, &outsize);
        ok(ret == ERROR_MORE_DATA, "RegGetValueA returned: %ld\n", ret);
        ok(outsize == insize + 1, "wrong size: %lu != %lu\n", outsize, insize + 1);
        outsize = insize + 2;
        memset(buffer, 0xbd, sizeof(buffer));
        ret = pRegGetValueA(subkey, NULL, "multistringtest", RRF_RT_REG_SZ, NULL, buffer, &outsize);
        ok(ret == ERROR_SUCCESS, "RegGetValueA returned: %ld\n", ret);
        ok(outsize == insize + 1, "wrong size: %lu != %lu\n", outsize, insize + 1);
        ok(buffer[insize] == 0, "buffer overflow at %lu %02x\n", insize, buffer[insize + 1]);
        ok(buffer[insize + 1] == 0xbd, "buffer overflow at %lu %02x\n", insize, buffer[insize + 1]);
    }

    RegDeleteKeyA(subkey, "");
    RegCloseKey(subkey);
}

static void test_reg_copy_tree(void)
{
    HKEY src, dst, subkey;
    CHAR buffer[MAX_PATH];
    DWORD dwsize, type;
    LONG size, ret;

    if (!pRegCopyTreeA)
    {
        win_skip("Skipping RegCopyTreeA tests, function not present\n");
        return;
    }

    ret = RegCreateKeyA(hkey_main, "src", &src);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);
    ret = RegCreateKeyA(hkey_main, "dst", &dst);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);

    /* Copy nonexistent subkey */
    ret = pRegCopyTreeA(src, "nonexistent_subkey", dst);
    ok(ret == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", ret);

    /*  Create test keys and values */
    ret = RegSetValueA(src, NULL, REG_SZ, "data", 4);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);
    ret = RegSetValueExA(src, "value", 0, REG_SZ, (const BYTE *)"data2", 5);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);

    ret = RegCreateKeyA(src, "subkey2", &subkey);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);
    ret = RegSetValueA(subkey, NULL, REG_SZ, "data3", 5);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);
    ret = RegSetValueExA(subkey, "value", 0, REG_SZ, (const BYTE *)"data4", 5);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);
    ret = RegCloseKey(subkey);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);

    ret = RegCreateKeyA(src, "subkey3", &subkey);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);
    ret = RegCloseKey(subkey);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);

    /* Copy subkey */
    ret = pRegCopyTreeA(src, "subkey2", dst);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);

    size = MAX_PATH;
    ret = RegQueryValueA(dst, NULL, buffer, &size);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);
    ok(!strcmp(buffer, "data3"), "Expected 'data3', got '%s'\n", buffer);

    dwsize = MAX_PATH;
    ret = RegQueryValueExA(dst, "value", NULL, &type, (BYTE *)buffer, &dwsize);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);
    ok(type == REG_SZ, "Expected REG_SZ, got %lu\n", type);
    ok(!strcmp(buffer, "data4"), "Expected 'data4', got '%s'\n", buffer);

    /* Copy full tree */
    ret = pRegCopyTreeA(src, NULL, dst);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);

    size = MAX_PATH;
    ret = RegQueryValueA(dst, NULL, buffer, &size);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);
    ok(!strcmp(buffer, "data"), "Expected 'data', got '%s'\n", buffer);

    dwsize = MAX_PATH;
    ret = RegQueryValueExA(dst, "value", NULL, &type, (BYTE *)buffer, &dwsize);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);
    ok(type == REG_SZ, "Expected REG_SZ, got %lu\n", type);
    ok(!strcmp(buffer, "data2"), "Expected 'data2', got '%s'\n", buffer);

    ret = RegOpenKeyA(dst, "subkey2", &subkey);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);
    size = MAX_PATH;
    ret = RegQueryValueA(subkey, NULL, buffer, &size);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);
    ok(!strcmp(buffer, "data3"), "Expected 'data3', got '%s'\n", buffer);
    dwsize = MAX_PATH;
    ret = RegQueryValueExA(subkey, "value", NULL, &type, (BYTE *)buffer, &dwsize);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);
    ok(type == REG_SZ, "Expected REG_SZ, got %lu\n", type);
    ok(!strcmp(buffer, "data4"), "Expected 'data4', got '%s'\n", buffer);
    ret = RegCloseKey(subkey);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);

    ret = RegOpenKeyA(dst, "subkey3", &subkey);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);
    ret = RegCloseKey(subkey);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);

    delete_key(src);
    delete_key(dst);
}

static void test_reg_delete_tree(void)
{
    CHAR buffer[MAX_PATH];
    HKEY subkey, subkey2;
    DWORD dwsize, type;
    LONG size, ret;

    if(!pRegDeleteTreeA) {
        win_skip("Skipping RegDeleteTreeA tests, function not present\n");
        return;
    }

    ret = RegCreateKeyA(hkey_main, "subkey", &subkey);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);
    ret = RegCreateKeyA(subkey, "subkey2", &subkey2);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);
    ret = RegSetValueA(subkey, NULL, REG_SZ, "data", 4);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);
    ret = RegSetValueA(subkey2, NULL, REG_SZ, "data2", 5);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);
    ret = RegCloseKey(subkey2);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);

    ret = pRegDeleteTreeA(subkey, "subkey2");
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);
    ok(RegOpenKeyA(subkey, "subkey2", &subkey2),
        "subkey2 was not deleted\n");
    size = MAX_PATH;
    ok(!RegQueryValueA(subkey, NULL, buffer, &size),
        "Default value of subkey no longer present\n");

    ret = RegCreateKeyA(subkey, "subkey2", &subkey2);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);
    ret = RegCloseKey(subkey2);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);
    ret = pRegDeleteTreeA(hkey_main, "subkey\\subkey2");
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);
    ok(RegOpenKeyA(subkey, "subkey2", &subkey2),
        "subkey2 was not deleted\n");
    ok(!RegQueryValueA(subkey, NULL, buffer, &size),
        "Default value of subkey no longer present\n");

    ret = RegCreateKeyA(subkey, "subkey2", &subkey2);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);
    ret = RegCloseKey(subkey2);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);
    ret = RegCreateKeyA(subkey, "subkey3", &subkey2);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);
    ret = RegCloseKey(subkey2);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);
    ret = RegSetValueA(subkey, NULL, REG_SZ, "data", 4);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);
    ret = RegSetValueExA(subkey, "value", 0, REG_SZ, (const BYTE *)"data2", 5);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);
    ret = pRegDeleteTreeA(subkey, NULL);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);
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
    dwsize = MAX_PATH;
    ok(RegQueryValueExA(subkey, "value", NULL, &type, (BYTE *)buffer, &dwsize),
        "Value is still present\n");
    ret = RegCloseKey(subkey);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);

    ret = RegOpenKeyA(hkey_main, "subkey", &subkey);
    ok(ret == ERROR_SUCCESS, "subkey was deleted\n");
    ret = pRegDeleteTreeA(subkey, "");
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);
    ret = RegCloseKey(subkey);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);

    ret = RegOpenKeyA(hkey_main, "subkey", &subkey);
    ok(ret == ERROR_SUCCESS, "subkey was deleted\n");
    ret = RegCloseKey(subkey);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", ret);

    ret = pRegDeleteTreeA(hkey_main, "not-here");
    ok(ret == ERROR_FILE_NOT_FOUND,
        "Expected ERROR_FILE_NOT_FOUND, got %ld\n", ret);
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
    ok(values == 4, "Expected 4 values, got %lu\n", values);

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
    static const WCHAR targetW[] = L"\\Software\\Wine\\Test\\target";
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
    target = malloc( target_len );
    memcpy( target, target_str.Buffer, target_str.Length );
    memcpy( target + target_str.Length/sizeof(WCHAR), targetW, sizeof(targetW) );

    err = RegCreateKeyExA( hkey_main, "link", 0, NULL, REG_OPTION_CREATE_LINK,
                           KEY_ALL_ACCESS, NULL, &link, NULL );
    ok( err == ERROR_SUCCESS, "RegCreateKeyEx failed: %lu\n", err );

    /* REG_SZ is not allowed */
    err = RegSetValueExA( link, "SymbolicLinkValue", 0, REG_SZ, (BYTE *)"foobar", sizeof("foobar") );
    ok( err == ERROR_ACCESS_DENIED, "RegSetValueEx wrong error %lu\n", err );
    err = RegSetValueExA( link, "SymbolicLinkValue", 0, REG_LINK,
                          (BYTE *)target, target_len - sizeof(WCHAR) );
    ok( err == ERROR_SUCCESS, "RegSetValueEx failed error %lu\n", err );
    /* other values are not allowed */
    err = RegSetValueExA( link, "link", 0, REG_LINK, (BYTE *)target, target_len - sizeof(WCHAR) );
    ok( err == ERROR_ACCESS_DENIED, "RegSetValueEx wrong error %lu\n", err );

    /* try opening the target through the link */

    err = RegOpenKeyA( hkey_main, "link", &key );
    ok( err == ERROR_FILE_NOT_FOUND, "RegOpenKey wrong error %lu\n", err );

    err = RegCreateKeyExA( hkey_main, "target", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &key, NULL );
    ok( err == ERROR_SUCCESS, "RegCreateKeyEx failed error %lu\n", err );

    dw = 0xbeef;
    err = RegSetValueExA( key, "value", 0, REG_DWORD, (BYTE *)&dw, sizeof(dw) );
    ok( err == ERROR_SUCCESS, "RegSetValueEx failed error %lu\n", err );
    RegCloseKey( key );

    err = RegOpenKeyA( hkey_main, "link", &key );
    ok( err == ERROR_SUCCESS, "RegOpenKey failed error %lu\n", err );

    len = sizeof(buffer);
    err = RegQueryValueExA( key, "value", NULL, &type, buffer, &len );
    ok( err == ERROR_SUCCESS, "RegOpenKey failed error %lu\n", err );
    ok( len == sizeof(DWORD), "wrong len %lu\n", len );

    len = sizeof(buffer);
    err = RegQueryValueExA( key, "SymbolicLinkValue", NULL, &type, buffer, &len );
    ok( err == ERROR_FILE_NOT_FOUND, "RegQueryValueEx wrong error %lu\n", err );

    /* REG_LINK can be created in non-link keys */
    err = RegSetValueExA( key, "SymbolicLinkValue", 0, REG_LINK,
                          (BYTE *)target, target_len - sizeof(WCHAR) );
    ok( err == ERROR_SUCCESS, "RegSetValueEx failed error %lu\n", err );
    len = sizeof(buffer);
    err = RegQueryValueExA( key, "SymbolicLinkValue", NULL, &type, buffer, &len );
    ok( err == ERROR_SUCCESS, "RegQueryValueEx failed error %lu\n", err );
    ok( len == target_len - sizeof(WCHAR), "wrong len %lu\n", len );
    err = RegDeleteValueA( key, "SymbolicLinkValue" );
    ok( err == ERROR_SUCCESS, "RegDeleteValue failed error %lu\n", err );

    RegCloseKey( key );

    err = RegCreateKeyExA( hkey_main, "link", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &key, NULL );
    ok( err == ERROR_SUCCESS, "RegCreateKeyEx failed error %lu\n", err );

    len = sizeof(buffer);
    err = RegQueryValueExA( key, "value", NULL, &type, buffer, &len );
    ok( err == ERROR_SUCCESS, "RegQueryValueEx failed error %lu\n", err );
    ok( len == sizeof(DWORD), "wrong len %lu\n", len );

    err = RegQueryValueExA( key, "SymbolicLinkValue", NULL, &type, buffer, &len );
    ok( err == ERROR_FILE_NOT_FOUND, "RegQueryValueEx wrong error %lu\n", err );
    RegCloseKey( key );

    /* now open the symlink itself */

    err = RegOpenKeyExA( hkey_main, "link", REG_OPTION_OPEN_LINK, KEY_ALL_ACCESS, &key );
    ok( err == ERROR_SUCCESS, "RegOpenKeyEx failed error %lu\n", err );
    len = sizeof(buffer);
    err = RegQueryValueExA( key, "SymbolicLinkValue", NULL, &type, buffer, &len );
    ok( err == ERROR_SUCCESS, "RegQueryValueEx failed error %lu\n", err );
    ok( len == target_len - sizeof(WCHAR), "wrong len %lu\n", len );
    RegCloseKey( key );

    err = RegCreateKeyExA( hkey_main, "link", 0, NULL, REG_OPTION_OPEN_LINK,
                           KEY_ALL_ACCESS, NULL, &key, NULL );
    ok( err == ERROR_SUCCESS, "RegCreateKeyEx failed error %lu\n", err );
    len = sizeof(buffer);
    err = RegQueryValueExA( key, "SymbolicLinkValue", NULL, &type, buffer, &len );
    ok( err == ERROR_SUCCESS, "RegQueryValueEx failed error %lu\n", err );
    ok( len == target_len - sizeof(WCHAR), "wrong len %lu\n", len );
    RegCloseKey( key );

    err = RegCreateKeyExA( hkey_main, "link", 0, NULL, REG_OPTION_CREATE_LINK,
                           KEY_ALL_ACCESS, NULL, &key, NULL );
    ok( err == ERROR_ALREADY_EXISTS, "RegCreateKeyEx wrong error %lu\n", err );

    err = RegCreateKeyExA( hkey_main, "link", 0, NULL, REG_OPTION_CREATE_LINK | REG_OPTION_OPEN_LINK,
                           KEY_ALL_ACCESS, NULL, &key, NULL );
    ok( err == ERROR_ALREADY_EXISTS, "RegCreateKeyEx wrong error %lu\n", err );

    err = RegDeleteKeyA( hkey_main, "target" );
    ok( err == ERROR_SUCCESS, "RegDeleteKey failed error %lu\n", err );

    err = RegDeleteKeyA( hkey_main, "link" );
    ok( err == ERROR_FILE_NOT_FOUND, "RegDeleteKey wrong error %lu\n", err );

    status = pNtDeleteKey( link );
    ok( !status, "NtDeleteKey failed: 0x%08lx\n", status );
    RegCloseKey( link );

    free( target );
    pRtlFreeUnicodeString( &target_str );
}

static DWORD get_key_value( HKEY root, const char *name, DWORD flags )
{
    HKEY key;
    DWORD err, type, dw = 1, len = sizeof(dw);

    err = RegOpenKeyExA( root, name, 0, flags | KEY_ALL_ACCESS, &key );
    if (err == ERROR_FILE_NOT_FOUND) return 0;
    ok( err == ERROR_SUCCESS, "%08lx: RegOpenKeyEx failed: %lu\n", flags, err );

    err = RegQueryValueExA( key, "value", NULL, &type, (BYTE *)&dw, &len );
    if (err == ERROR_FILE_NOT_FOUND)
        dw = 0;
    else
        ok( err == ERROR_SUCCESS, "%08lx: RegQueryValueEx failed: %lu\n", flags, err );
    RegCloseKey( key );
    return dw;
}

static void _check_key_value( int line, HANDLE root, const char *name, DWORD flags, DWORD expect )
{
    DWORD dw = get_key_value( root, name, flags );
    ok_(__FILE__,line)( dw == expect, "%08lx: wrong value %lu/%lu\n", flags, dw, expect );
}
#define check_key_value(root,name,flags,expect) _check_key_value( __LINE__, root, name, flags, expect )

static void _check_enum_value( int line, const char *name, DWORD flags, DWORD subkeys_in, BOOL found_in)
{
    char buffer[1024];
    DWORD err, i, subkeys;
    BOOL found;
    HKEY key;

    err = RegOpenKeyExA( HKEY_LOCAL_MACHINE, name, 0, flags, &key );
    ok_( __FILE__, line )( err == ERROR_SUCCESS, "RegOpenKeyExA failed: %lu\n", err );

    err = RegQueryInfoKeyA( key, NULL, NULL, NULL, &subkeys,
                            NULL, NULL, NULL, NULL, NULL, NULL, NULL );
    ok_( __FILE__, line )( err == ERROR_SUCCESS, "RegQueryInfoKeyA failed: %lu\n", err );
    ok_( __FILE__, line )( subkeys == subkeys_in, "wrong number of subkeys: %lu\n", subkeys );

    found = FALSE;
    for (i = 0; i < subkeys; i++)
    {
        err = RegEnumKeyA( key, i, buffer, sizeof(buffer) );
        ok_( __FILE__, line )( err == ERROR_SUCCESS, "RegEnumKeyA failed: %lu\n", err );

        if (!strcmp(buffer, "Wine"))
            found = TRUE;
    }
    ok_( __FILE__, line )( found == found_in, "found equals %d\n", found );
    RegCloseKey( key );
}
#define check_enum_value(name, flags, subkeys, found) _check_enum_value( __LINE__, name, flags, subkeys, found )

static void test_redirection(void)
{
    DWORD err, type, dw, len;
    HKEY key, key32, key64, root, root32, root64;
    DWORD subkeys, subkeys32, subkeys64;

    if (!has_wow64())
    {
        skip( "Not on Wow64, no redirection\n" );
        return;
    }

    if (limited_user)
    {
        skip("not enough privileges to modify HKLM\n");
        return;
    }
#if defined(__REACTOS__) && defined(_WIN64)
    if (GetNTVersion() == _WIN32_WINNT_VISTA) {
        skip("test_redirection() invalid for Vista x64 and hangs.\n");
        return;
    }
#endif

    err = RegCreateKeyExA( HKEY_LOCAL_MACHINE, "Software\\Wine", 0, NULL, 0,
                           KEY_WOW64_64KEY | KEY_ALL_ACCESS, NULL, &root64, NULL );
    ok( err == ERROR_SUCCESS, "RegCreateKeyExA failed: %lu\n", err );

    err = RegCreateKeyExA( HKEY_LOCAL_MACHINE, "Software\\Wine", 0, NULL, 0,
                           KEY_WOW64_32KEY | KEY_ALL_ACCESS, NULL, &root32, NULL );
    ok( err == ERROR_SUCCESS, "RegCreateKeyExA failed: %lu\n", err );

    err = RegCreateKeyExA( HKEY_LOCAL_MACHINE, "Software\\Wine\\Winetest", 0, NULL, 0,
                           KEY_WOW64_64KEY | KEY_ALL_ACCESS, NULL, &key64, NULL );
    ok( err == ERROR_SUCCESS, "RegCreateKeyExA failed: %lu\n", err );

    err = RegCreateKeyExA( HKEY_LOCAL_MACHINE, "Software\\Wine\\Winetest", 0, NULL, 0,
                           KEY_WOW64_32KEY | KEY_ALL_ACCESS, NULL, &key32, NULL );
    ok( err == ERROR_SUCCESS, "RegCreateKeyExA failed: %lu\n", err );

    dw = 64;
    err = RegSetValueExA( key64, "value", 0, REG_DWORD, (BYTE *)&dw, sizeof(dw) );
    ok( err == ERROR_SUCCESS, "RegSetValueExA failed: %lu\n", err );

    dw = 32;
    err = RegSetValueExA( key32, "value", 0, REG_DWORD, (BYTE *)&dw, sizeof(dw) );
    ok( err == ERROR_SUCCESS, "RegSetValueExA failed: %lu\n", err );

    dw = 0;
    len = sizeof(dw);
    err = RegQueryValueExA( key32, "value", NULL, &type, (BYTE *)&dw, &len );
    ok( err == ERROR_SUCCESS, "RegQueryValueExA failed: %lu\n", err );
    ok( dw == 32, "wrong value %lu\n", dw );

    dw = 0;
    len = sizeof(dw);
    err = RegQueryValueExA( key64, "value", NULL, &type, (BYTE *)&dw, &len );
    ok( err == ERROR_SUCCESS, "RegQueryValueExA failed: %lu\n", err );
    ok( dw == 64, "wrong value %lu\n", dw );

    err = RegCreateKeyExA( HKEY_LOCAL_MACHINE, "Software", 0, NULL, 0,
                           KEY_ALL_ACCESS, NULL, &key, NULL );
    ok( err == ERROR_SUCCESS, "RegCreateKeyExA failed: %lu\n", err );
    check_key_value( key, "Wine\\Winetest", 0, ptr_size );
    check_key_value( key, "Wine\\Winetest", KEY_WOW64_64KEY, ptr_size );
    check_key_value( key, "Wine\\Winetest", KEY_WOW64_32KEY, 32 );
    check_key_value( key, "Wow6432Node\\Wine\\Winetest", 0, ptr_size == 32 ? 0 : 32 );
    check_key_value( key, "Wow6432Node\\Wine\\Winetest", KEY_WOW64_64KEY, ptr_size == 32 ? 0 : 32 );
    check_key_value( key, "Wow6432Node\\Wine\\Winetest", KEY_WOW64_32KEY, ptr_size == 32 ? 0 : 32 );
    RegCloseKey( key );

    err = RegCreateKeyExA( HKEY_LOCAL_MACHINE, "Software", 0, NULL, 0,
                           KEY_WOW64_64KEY | KEY_ALL_ACCESS, NULL, &key, NULL );
    ok( err == ERROR_SUCCESS, "RegCreateKeyExA failed: %lu\n", err );
    dw = get_key_value( key, "Wine\\Winetest", 0 );
    ok( dw == 64 || broken(dw == 32) /* win7 */, "wrong value %lu\n", dw );
    check_key_value( key, "Wine\\Winetest", KEY_WOW64_64KEY, 64 );
    check_key_value( key, "Wine\\Winetest", KEY_WOW64_32KEY, 32 );
    check_key_value( key, "Wow6432Node\\Wine\\Winetest", 0, 32 );
    check_key_value( key, "Wow6432Node\\Wine\\Winetest", KEY_WOW64_64KEY, 32 );
    check_key_value( key, "Wow6432Node\\Wine\\Winetest", KEY_WOW64_32KEY, 32 );
    RegCloseKey( key );

    err = RegCreateKeyExA( HKEY_LOCAL_MACHINE, "Software", 0, NULL, 0,
                           KEY_WOW64_32KEY | KEY_ALL_ACCESS, NULL, &key, NULL );
    ok( err == ERROR_SUCCESS, "RegCreateKeyExA failed: %lu\n", err );
    check_key_value( key, "Wine\\Winetest", 0, 32 );
    check_key_value( key, "Wine\\Winetest", KEY_WOW64_64KEY, 32 );
    check_key_value( key, "Wine\\Winetest", KEY_WOW64_32KEY, 32 );
    check_key_value( key, "Wow6432Node\\Wine\\Winetest", 0, 0 );
    check_key_value( key, "Wow6432Node\\Wine\\Winetest", KEY_WOW64_64KEY, 0 );
    check_key_value( key, "Wow6432Node\\Wine\\Winetest", KEY_WOW64_32KEY, 0 );
    RegCloseKey( key );

    check_key_value( HKEY_LOCAL_MACHINE, "Software\\Wine\\Winetest", 0, ptr_size );
    check_key_value( HKEY_LOCAL_MACHINE, "Software\\Wow6432Node\\Wine\\Winetest", 0, 32 );
    check_key_value( HKEY_LOCAL_MACHINE, "Software\\Wine\\Winetest", KEY_WOW64_64KEY, 64 );
    check_key_value( HKEY_LOCAL_MACHINE, "Software\\Wine\\Winetest", KEY_WOW64_32KEY, 32 );
    check_key_value( HKEY_LOCAL_MACHINE, "Software\\Wow6432Node\\Wine\\Winetest", KEY_WOW64_64KEY, 32 );
    check_key_value( HKEY_LOCAL_MACHINE, "Software\\Wow6432Node\\Wine\\Winetest", KEY_WOW64_32KEY, 32 );

    err = RegCreateKeyExA( HKEY_LOCAL_MACHINE, "Software\\Wow6432Node", 0, NULL, 0,
                           KEY_ALL_ACCESS, NULL, &key, NULL );
    ok( err == ERROR_SUCCESS, "RegCreateKeyExA failed: %lu\n", err );
    check_key_value( key, "Wine\\Winetest", 0, 32 );
    check_key_value( key, "Wine\\Winetest", KEY_WOW64_64KEY, 32 );
    check_key_value( key, "Wine\\Winetest", KEY_WOW64_32KEY, 32 );
    RegCloseKey( key );

    err = RegCreateKeyExA( HKEY_LOCAL_MACHINE, "Software\\Wow6432Node", 0, NULL, 0,
                           KEY_WOW64_64KEY | KEY_ALL_ACCESS, NULL, &key, NULL );
    ok( err == ERROR_SUCCESS, "RegCreateKeyExA failed: %lu\n", err );
    check_key_value( key, "Wine\\Winetest", 0, 32 );
    check_key_value( key, "Wine\\Winetest", KEY_WOW64_64KEY, 32 );
    check_key_value( key, "Wine\\Winetest", KEY_WOW64_32KEY, 32 );
    RegCloseKey( key );

    err = RegCreateKeyExA( HKEY_LOCAL_MACHINE, "Software\\Wow6432Node", 0, NULL, 0,
                           KEY_WOW64_32KEY | KEY_ALL_ACCESS, NULL, &key, NULL );
    ok( err == ERROR_SUCCESS, "RegCreateKeyExA failed: %lu\n", err );
    check_key_value( key, "Wine\\Winetest", 0, 32 );
    check_key_value( key, "Wine\\Winetest", KEY_WOW64_64KEY, 32 );
    check_key_value( key, "Wine\\Winetest", KEY_WOW64_32KEY, 32 );
    RegCloseKey( key );

    err = RegCreateKeyExA( HKEY_LOCAL_MACHINE, "Software\\Wow6432Node\\Wine", 0, NULL, 0,
                           KEY_ALL_ACCESS, NULL, &key, NULL );
    ok( err == ERROR_SUCCESS, "RegCreateKeyExA failed: %lu\n", err );
    check_key_value( key, "Winetest", 0, 32 );
    check_key_value( key, "Winetest", KEY_WOW64_64KEY, 32 );
    check_key_value( key, "Winetest", KEY_WOW64_32KEY, 32 );
    RegCloseKey( key );

    err = RegCreateKeyExA( HKEY_LOCAL_MACHINE, "Software\\Wow6432Node\\Wine", 0, NULL, 0,
                           KEY_WOW64_64KEY | KEY_ALL_ACCESS, NULL, &key, NULL );
    ok( err == ERROR_SUCCESS, "RegCreateKeyExA failed: %lu\n", err );
    check_key_value( key, "Winetest", 0, 32 );
    check_key_value( key, "Winetest", KEY_WOW64_64KEY, 32 );
    check_key_value( key, "Winetest", KEY_WOW64_32KEY, 32 );
    RegCloseKey( key );

    err = RegCreateKeyExA( HKEY_LOCAL_MACHINE, "Software\\Wow6432Node\\Wine", 0, NULL, 0,
                           KEY_WOW64_32KEY | KEY_ALL_ACCESS, NULL, &key, NULL );
    ok( err == ERROR_SUCCESS, "RegCreateKeyExA failed: %lu\n", err );
    check_key_value( key, "Winetest", 0, 32 );
    check_key_value( key, "Winetest", KEY_WOW64_64KEY, 32 );
    check_key_value( key, "Winetest", KEY_WOW64_32KEY, 32 );
    RegCloseKey( key );

    err = RegCreateKeyExA( HKEY_LOCAL_MACHINE, "Software\\Wine", 0, NULL, 0,
                           KEY_ALL_ACCESS, NULL, &key, NULL );
    ok( err == ERROR_SUCCESS, "RegCreateKeyExA failed: %lu\n", err );
    check_key_value( key, "Winetest", 0, ptr_size );
    check_key_value( key, "Winetest", KEY_WOW64_64KEY, ptr_size );
    check_key_value( key, "Winetest", KEY_WOW64_32KEY, 32 );
    RegCloseKey( key );

    err = RegCreateKeyExA( HKEY_LOCAL_MACHINE, "Software\\Wine", 0, NULL, 0,
                           KEY_WOW64_64KEY | KEY_ALL_ACCESS, NULL, &key, NULL );
    ok( err == ERROR_SUCCESS, "RegCreateKeyExA failed: %lu\n", err );
    check_key_value( key, "Winetest", 0, 64 );
    check_key_value( key, "Winetest", KEY_WOW64_64KEY, 64 );
    check_key_value( key, "Winetest", KEY_WOW64_32KEY, 32 );
    RegCloseKey( key );

    err = RegCreateKeyExA( HKEY_LOCAL_MACHINE, "Software\\Wine", 0, NULL, 0,
                           KEY_WOW64_32KEY | KEY_ALL_ACCESS, NULL, &key, NULL );
    ok( err == ERROR_SUCCESS, "RegCreateKeyExA failed: %lu\n", err );
    check_key_value( key, "Winetest", 0, 32 );
    check_key_value( key, "Winetest", KEY_WOW64_64KEY, 32 );
    check_key_value( key, "Winetest", KEY_WOW64_32KEY, 32 );
    RegCloseKey( key );

    if (pRegDeleteKeyExA)
    {
        err = pRegDeleteKeyExA( key32, "", KEY_WOW64_32KEY, 0 );
        ok( err == ERROR_SUCCESS, "RegDeleteKey failed: %lu\n", err );
        err = pRegDeleteKeyExA( key64, "", KEY_WOW64_64KEY, 0 );
        ok( err == ERROR_SUCCESS, "RegDeleteKey failed: %lu\n", err );
        pRegDeleteKeyExA( key64, "", KEY_WOW64_64KEY, 0 );
        pRegDeleteKeyExA( root64, "", KEY_WOW64_64KEY, 0 );
    }
    else
    {
        err = RegDeleteKeyA( key32, "" );
        ok( err == ERROR_SUCCESS, "RegDeleteKey failed: %lu\n", err );
        err = RegDeleteKeyA( key64, "" );
        ok( err == ERROR_SUCCESS, "RegDeleteKey failed: %lu\n", err );
        RegDeleteKeyA( key64, "" );
        RegDeleteKeyA( root64, "" );
    }
    RegCloseKey( key32 );
    RegCloseKey( key64 );
    RegCloseKey( root32 );
    RegCloseKey( root64 );

    err = RegCreateKeyExW( HKEY_LOCAL_MACHINE, L"Software\\WOW6432Node\\test1\\test2", 0, NULL, 0,
                              KEY_WRITE | KEY_WOW64_32KEY, NULL, &key, NULL );
    ok(!err, "got %#lx.\n", err);
    RegCloseKey(key);

    err = RegCreateKeyExW( HKEY_LOCAL_MACHINE, L"Software\\test1\\test2", 0, NULL, 0, KEY_WRITE | KEY_WOW64_32KEY,
                              NULL, &key, NULL );
    ok(!err, "got %#lx.\n", err);
    RegCloseKey(key);

    err = RegOpenKeyExW( HKEY_LOCAL_MACHINE, L"Software\\test1\\test2", 0, KEY_WRITE | KEY_WOW64_32KEY, &key );
    ok(!err, "got %#lx.\n", err);
    RegCloseKey(key);

    if (pRegDeleteTreeA)
    {
        err = pRegDeleteTreeA(HKEY_LOCAL_MACHINE, "Software\\WOW6432Node\\test1");
        ok(!err, "got %#lx.\n", err);
        err = pRegDeleteTreeA(HKEY_LOCAL_MACHINE, "Software\\test1");
        ok(err == ERROR_FILE_NOT_FOUND, "got %#lx.\n", err);
    }

    /* Software\Classes is shared/reflected so behavior is different */

    err = RegCreateKeyExA( HKEY_LOCAL_MACHINE, "Software\\Classes\\Wine",
                           0, NULL, 0, KEY_WOW64_64KEY | KEY_ALL_ACCESS, NULL, &key64, NULL);
    if (err == ERROR_ACCESS_DENIED)
    {
        skip("Not authorized to modify the Classes key\n");
        return;
    }
    ok( err == ERROR_SUCCESS, "RegCreateKeyA failed: %lu\n", err );

    err = RegOpenKeyExA( HKEY_LOCAL_MACHINE, "Software\\Classes\\Wow6432Node\\Wine",
                         0, KEY_WOW64_32KEY | KEY_ALL_ACCESS, &key );
    ok( err == ERROR_SUCCESS, "RegOpenKeyExA failed: %lu\n", err );
    RegCloseKey( key );

    err = RegOpenKeyExA( HKEY_LOCAL_MACHINE, "Software\\Classes\\Wow6432Node\\Wine",
                         0, KEY_ALL_ACCESS, &key );
    ok( err == (ptr_size == 64 ? ERROR_FILE_NOT_FOUND : ERROR_SUCCESS),
        "RegOpenKeyExA failed: %lu\n", err );
    if (!err) RegCloseKey( key );

    err = RegCreateKeyExA( HKEY_LOCAL_MACHINE, "Software\\Classes\\Wow6432Node\\Wine",
                           0, NULL, 0, KEY_WOW64_32KEY | KEY_ALL_ACCESS, NULL, &key32, NULL);
    ok( err == ERROR_SUCCESS, "RegCreateKeyA failed: %lu\n", err );

    dw = 32;
    err = RegSetValueExA( key32, "value", 0, REG_DWORD, (BYTE *)&dw, sizeof(dw) );
    ok( err == ERROR_SUCCESS, "RegSetValueExA failed: %lu\n", err );

    dw = 64;
    err = RegSetValueExA( key64, "value", 0, REG_DWORD, (BYTE *)&dw, sizeof(dw) );
    ok( err == ERROR_SUCCESS, "RegSetValueExA failed: %lu\n", err );

    check_key_value( HKEY_LOCAL_MACHINE, "Software\\Classes\\Wine", 0, 64 );
    check_key_value( HKEY_LOCAL_MACHINE, "Software\\Classes\\Wine", KEY_WOW64_64KEY, 64 );
    check_key_value( HKEY_LOCAL_MACHINE, "Software\\Classes\\Wine", KEY_WOW64_32KEY, 64 );
    check_key_value( HKEY_LOCAL_MACHINE, "Software\\Classes\\Wow6432Node\\Wine", 0, ptr_size == 64 ? 0 : 64 );
    check_key_value( HKEY_LOCAL_MACHINE, "Software\\Classes\\Wow6432Node\\Wine", KEY_WOW64_64KEY, 0 );
    check_key_value( HKEY_LOCAL_MACHINE, "Software\\Classes\\Wow6432Node\\Wine", KEY_WOW64_32KEY, 64 );

    RegDeleteKeyA( key32, "" );
    RegCloseKey( key32 );

    check_key_value( HKEY_LOCAL_MACHINE, "Software\\Classes\\Wine", 0, 0 );
    check_key_value( HKEY_LOCAL_MACHINE, "Software\\Classes\\Wine", KEY_WOW64_64KEY, 0 );
    check_key_value( HKEY_LOCAL_MACHINE, "Software\\Classes\\Wine", KEY_WOW64_32KEY, 0 );
    check_key_value( HKEY_LOCAL_MACHINE, "Software\\Classes\\Wow6432Node\\Wine", 0, 0 );
    check_key_value( HKEY_LOCAL_MACHINE, "Software\\Classes\\Wow6432Node\\Wine", KEY_WOW64_64KEY, 0 );
    check_key_value( HKEY_LOCAL_MACHINE, "Software\\Classes\\Wow6432Node\\Wine", KEY_WOW64_32KEY, 0 );

    RegDeleteKeyA( key64, "" );
    RegCloseKey( key64 );

    err = RegOpenKeyExA( HKEY_LOCAL_MACHINE, "Software\\Classes", 0, KEY_WOW64_64KEY | KEY_ALL_ACCESS, &root64 );
    ok( err == ERROR_SUCCESS, "RegOpenKeyExA failed: %lu\n", err );

    err = RegOpenKeyExA( HKEY_LOCAL_MACHINE, "Software\\Classes", 0, KEY_WOW64_32KEY | KEY_ALL_ACCESS, &root32 );
    ok( err == ERROR_SUCCESS, "RegOpenKeyExA failed: %lu\n", err );

    err = RegCreateKeyExA( root64, "Wine", 0, NULL, 0,
                           KEY_ALL_ACCESS, NULL, &key64, NULL);
    ok( err == ERROR_SUCCESS, "RegCreateKeyA failed: %lu\n", err );

    err = RegCreateKeyExA( key64, "Wine", 0, NULL, 0,
                           KEY_WOW64_32KEY | KEY_ALL_ACCESS, NULL, &key, NULL);
    ok( err == ERROR_SUCCESS, "RegCreateKeyA failed: %lu\n", err );
    RegDeleteKeyA( key, "" );
    RegCloseKey( key );

    err = RegOpenKeyExA( root32, "Wine", 0, KEY_WOW64_32KEY | KEY_ALL_ACCESS, &key );
    ok( err == ERROR_SUCCESS, "RegOpenKeyExA failed: %lu\n", err );
    RegCloseKey( key );

    err = RegOpenKeyExA( root32, "Wine", 0, KEY_ALL_ACCESS, &key );
    ok( err == ERROR_SUCCESS, "RegOpenKeyExA failed: %lu\n", err );
    RegCloseKey( key );

    err = RegCreateKeyExA( root32, "Wine", 0, NULL, 0,
                           KEY_ALL_ACCESS, NULL, &key32, NULL);
    ok( err == ERROR_SUCCESS, "RegCreateKeyA failed: %lu\n", err );

    dw = 32;
    err = RegSetValueExA( key32, "value", 0, REG_DWORD, (BYTE *)&dw, sizeof(dw) );
    ok( err == ERROR_SUCCESS, "RegSetValueExA failed: %lu\n", err );

    dw = 64;
    err = RegSetValueExA( key64, "value", 0, REG_DWORD, (BYTE *)&dw, sizeof(dw) );
    ok( err == ERROR_SUCCESS, "RegSetValueExA failed: %lu\n", err );

    check_key_value( root64, "Wine", 0, 64 );
    check_key_value( root64, "Wine", KEY_WOW64_64KEY, 64 );
    check_key_value( root64, "Wine", KEY_WOW64_32KEY, 64 );
    check_key_value( root32, "Wine", 0, 64 );
    check_key_value( root32, "Wine", KEY_WOW64_64KEY, 64 );
    check_key_value( root32, "Wine", KEY_WOW64_32KEY, 64 );

    RegDeleteKeyA( key32, "" );
    RegCloseKey( key32 );

    check_key_value( root64, "Wine", 0, 0 );
    check_key_value( root64, "Wine", KEY_WOW64_64KEY, 0 );
    check_key_value( root64, "Wine", KEY_WOW64_32KEY, 0 );
    check_key_value( root32, "Wine", 0, 0 );
    check_key_value( root32, "Wine", KEY_WOW64_64KEY, 0 );
    check_key_value( root32, "Wine", KEY_WOW64_32KEY, 0 );

    RegDeleteKeyA( key64, "" );
    RegCloseKey( key64 );

    err = RegCreateKeyExA( root32, "Wine", 0, NULL, 0,
                           KEY_ALL_ACCESS, NULL, &key32, NULL);
    ok( err == ERROR_SUCCESS, "RegCreateKeyA failed: %lu\n", err );

    dw = 32;
    err = RegSetValueExA( key32, "value", 0, REG_DWORD, (BYTE *)&dw, sizeof(dw) );
    ok( err == ERROR_SUCCESS, "RegSetValueExA failed: %lu\n", err );

    check_key_value( root64, "Wine", 0, 32 );
    check_key_value( root64, "Wine", KEY_WOW64_64KEY, 32 );
    check_key_value( root64, "Wine", KEY_WOW64_32KEY, 32 );
    check_key_value( root32, "Wine", 0, 32 );
    check_key_value( root32, "Wine", KEY_WOW64_64KEY, 32 );
    check_key_value( root32, "Wine", KEY_WOW64_32KEY, 32 );

    RegDeleteKeyA( key32, "" );
    RegCloseKey( key32 );

    RegCloseKey( root64 );
    RegCloseKey( root32 );

    err = RegOpenKeyExA( HKEY_CLASSES_ROOT, "Interface",
                         0, KEY_WOW64_64KEY | KEY_ALL_ACCESS, &root64 );
    ok( err == ERROR_SUCCESS, "RegOpenKeyExA failed: %lu\n", err );

    err = RegOpenKeyExA( HKEY_CLASSES_ROOT, "Interface",
                         0, KEY_WOW64_32KEY | KEY_ALL_ACCESS, &root32 );
    ok( err == ERROR_SUCCESS, "RegOpenKeyExA failed: %lu\n", err );

    err = RegOpenKeyExA( HKEY_CLASSES_ROOT, "Interface",
                         0, KEY_ALL_ACCESS, &root );
    ok( err == ERROR_SUCCESS, "RegOpenKeyExA failed: %lu\n", err );

    err = RegCreateKeyExA( root32, "Wine", 0, NULL, 0,
                           KEY_WOW64_32KEY | KEY_ALL_ACCESS, NULL, &key32, NULL);
    ok( err == ERROR_SUCCESS, "RegCreateKeyA failed: %lu\n", err );

    err = RegOpenKeyExA( root, "Wine", 0, KEY_ALL_ACCESS, &key );
    ok( err == (ptr_size == 64 ? ERROR_FILE_NOT_FOUND : ERROR_SUCCESS),
        "RegOpenKeyExA failed: %lu\n", err );
    if (!err) RegCloseKey( key );

    RegDeleteKeyA( key32, "" );
    RegCloseKey( key32 );

    err = RegCreateKeyExA( root64, "Wine", 0, NULL, 0,
                           KEY_WOW64_64KEY | KEY_ALL_ACCESS, NULL, &key64, NULL);
    ok( err == ERROR_SUCCESS, "RegCreateKeyA failed: %lu\n", err );

    err = RegOpenKeyExA( root, "Wine", 0, KEY_ALL_ACCESS, &key );
    ok( err == (ptr_size == 32 ? ERROR_FILE_NOT_FOUND : ERROR_SUCCESS),
        "RegOpenKeyExA failed: %lu\n", err );
    if (!err) RegCloseKey( key );

    RegDeleteKeyA( key64, "" );
    RegCloseKey( key64 );

    RegDeleteKeyA( root64, "" );
    RegDeleteKeyA( root32, "" );
    RegDeleteKeyA( root, "" );

    RegCloseKey( root64 );
    RegCloseKey( root32 );
    RegCloseKey( root );

    err = RegOpenKeyExA( HKEY_LOCAL_MACHINE, "Software\\Classes",
                         0, KEY_WOW64_64KEY | KEY_ALL_ACCESS, &root64 );
    ok( err == ERROR_SUCCESS, "RegOpenKeyExA failed: %lu\n", err );

    err = RegOpenKeyExA( HKEY_LOCAL_MACHINE, "Software\\Classes",
                         0, KEY_WOW64_32KEY | KEY_ALL_ACCESS, &root32 );
    ok( err == ERROR_SUCCESS, "RegOpenKeyExA failed: %lu\n", err );

    err = RegOpenKeyExA( HKEY_LOCAL_MACHINE, "Software\\Classes",
                         0, KEY_ALL_ACCESS, &root );
    ok( err == ERROR_SUCCESS, "RegOpenKeyExA failed: %lu\n", err );

    err = RegOpenKeyExA( root64, "Interface",
                         0, KEY_WOW64_64KEY | KEY_ALL_ACCESS, &key64 );
    ok( err == ERROR_SUCCESS, "RegOpenKeyExA failed: %lu\n", err );

    err = RegOpenKeyExA( root32, "Interface",
                         0, KEY_WOW64_32KEY | KEY_ALL_ACCESS, &key32 );
    ok( err == ERROR_SUCCESS, "RegOpenKeyExA failed: %lu\n", err );

    err = RegOpenKeyExA( root, "Interface",
                         0, KEY_ALL_ACCESS, &key );
    ok( err == ERROR_SUCCESS, "RegOpenKeyExA failed: %lu\n", err );

    RegCloseKey( root64 );
    RegCloseKey( root32 );
    RegCloseKey( root );

    root64 = key64;
    root32 = key32;
    root = key;

    err = RegCreateKeyExA( root32, "Wine", 0, NULL, 0,
                           KEY_WOW64_32KEY | KEY_ALL_ACCESS, NULL, &key32, NULL);
    ok( err == ERROR_SUCCESS, "RegCreateKeyA failed: %lu\n", err );

    err = RegOpenKeyExA( root, "Wine", 0, KEY_ALL_ACCESS, &key );
    ok( err == (ptr_size == 64 ? ERROR_FILE_NOT_FOUND : ERROR_SUCCESS),
        "RegOpenKeyExA failed: %lu\n", err );
    if (!err) RegCloseKey( key );

    RegDeleteKeyA( key32, "" );
    RegCloseKey( key32 );

    err = RegCreateKeyExA( root64, "Wine", 0, NULL, 0,
                           KEY_WOW64_64KEY | KEY_ALL_ACCESS, NULL, &key64, NULL);
    ok( err == ERROR_SUCCESS, "RegCreateKeyA failed: %lu\n", err );

    err = RegOpenKeyExA( root, "Wine", 0, KEY_ALL_ACCESS, &key );
    ok( err == (ptr_size == 32 ? ERROR_FILE_NOT_FOUND : ERROR_SUCCESS),
        "RegOpenKeyExA failed: %lu\n", err );
    if (!err) RegCloseKey( key );

    RegDeleteKeyA( key64, "" );
    RegCloseKey( key64 );

    RegDeleteKeyA( root, "" );
    RegCloseKey( root );

    err = RegCreateKeyExA( root64, "Wine", 0, NULL, 0,
                           KEY_ALL_ACCESS, NULL, &key64, NULL);
    ok( err == ERROR_SUCCESS, "RegCreateKeyA failed: %lu\n", err );

    err = RegOpenKeyExA( root32, "Wine", 0, KEY_WOW64_32KEY | KEY_ALL_ACCESS, &key );
    ok( err == ERROR_FILE_NOT_FOUND, "RegOpenKeyExA failed: %lu\n", err );

    err = RegOpenKeyExA( root32, "Wine", 0, KEY_ALL_ACCESS, &key );
    ok( err == ERROR_FILE_NOT_FOUND, "RegOpenKeyExA failed: %lu\n", err );

    err = RegCreateKeyExA( root32, "Wine", 0, NULL, 0,
                           KEY_ALL_ACCESS, NULL, &key32, NULL);
    ok( err == ERROR_SUCCESS, "RegCreateKeyA failed: %lu\n", err );

    dw = 32;
    err = RegSetValueExA( key32, "value", 0, REG_DWORD, (BYTE *)&dw, sizeof(dw) );
    ok( err == ERROR_SUCCESS, "RegSetValueExA failed: %lu\n", err );

    dw = 64;
    err = RegSetValueExA( key64, "value", 0, REG_DWORD, (BYTE *)&dw, sizeof(dw) );
    ok( err == ERROR_SUCCESS, "RegSetValueExA failed: %lu\n", err );

    check_key_value( root64, "Wine", 0, 64 );
    check_key_value( root64, "Wine", KEY_WOW64_64KEY, 64 );
    check_key_value( root64, "Wine", KEY_WOW64_32KEY, 32 );
    check_key_value( root32, "Wine", 0, 32 );
    check_key_value( root32, "Wine", KEY_WOW64_64KEY, 32 );
    check_key_value( root32, "Wine", KEY_WOW64_32KEY, 32 );

    err = RegOpenKeyExA( HKEY_LOCAL_MACHINE, "Software\\Classes\\Interface",
                         0, KEY_WOW64_64KEY | KEY_ALL_ACCESS, &key );
    ok( err == ERROR_SUCCESS, "RegOpenKeyExA failed: %lu\n", err );
    check_key_value( key, "Wine", 0, 64 );
    check_key_value( key, "Wine", KEY_WOW64_64KEY, 64 );
    check_key_value( key, "Wine", KEY_WOW64_32KEY, 32 );
    RegCloseKey( key );

    err = RegOpenKeyExA( HKEY_LOCAL_MACHINE, "Software\\Classes\\Interface",
                         0, KEY_WOW64_32KEY | KEY_ALL_ACCESS, &key );
    ok( err == ERROR_SUCCESS, "RegOpenKeyExA failed: %lu\n", err );
    check_key_value( key, "Wine", 0, 32 );
    check_key_value( key, "Wine", KEY_WOW64_64KEY, 32 );
    check_key_value( key, "Wine", KEY_WOW64_32KEY, 32 );
    RegCloseKey( key );

    check_key_value( HKEY_LOCAL_MACHINE, "Software\\Classes\\Interface\\Wine", 0, ptr_size );
    check_key_value( HKEY_LOCAL_MACHINE, "Software\\Classes\\Interface\\Wine", KEY_WOW64_64KEY, 64 );
    check_key_value( HKEY_LOCAL_MACHINE, "Software\\Classes\\Interface\\Wine", KEY_WOW64_32KEY, 32 );

    RegDeleteKeyA( key32, "" );
    RegCloseKey( key32 );

    check_key_value( root64, "Wine", 0, 64 );
    check_key_value( root64, "Wine", KEY_WOW64_64KEY, 64 );
    check_key_value( root64, "Wine", KEY_WOW64_32KEY, 0 );
    check_key_value( root32, "Wine", 0, 0 );
    check_key_value( root32, "Wine", KEY_WOW64_64KEY, 0 );
    check_key_value( root32, "Wine", KEY_WOW64_32KEY, 0 );

    check_key_value( HKEY_LOCAL_MACHINE, "Software\\Classes\\Interface\\Wine", 0, ptr_size == 64 ? 64 : 0 );
    check_key_value( HKEY_LOCAL_MACHINE, "Software\\Classes\\Interface\\Wine", KEY_WOW64_64KEY, 64 );
    check_key_value( HKEY_LOCAL_MACHINE, "Software\\Classes\\Interface\\Wine", KEY_WOW64_32KEY, 0 );

    RegDeleteKeyA( key64, "" );
    RegCloseKey( key64 );

    err = RegCreateKeyExA( root32, "Wine", 0, NULL, 0,
                           KEY_ALL_ACCESS, NULL, &key32, NULL);
    ok( err == ERROR_SUCCESS, "RegCreateKeyA failed: %lu\n", err );

    dw = 32;
    err = RegSetValueExA( key32, "value", 0, REG_DWORD, (BYTE *)&dw, sizeof(dw) );
    ok( err == ERROR_SUCCESS, "RegSetValueExA failed: %lu\n", err );

    check_key_value( root64, "Wine", 0, 0 );
    check_key_value( root64, "Wine", KEY_WOW64_64KEY, 0 );
    check_key_value( root64, "Wine", KEY_WOW64_32KEY, 32 );
    check_key_value( root32, "Wine", 0, 32 );
    check_key_value( root32, "Wine", KEY_WOW64_64KEY, 32 );
    check_key_value( root32, "Wine", KEY_WOW64_32KEY, 32 );

    RegDeleteKeyA( key32, "" );
    RegCloseKey( key32 );

    RegCloseKey( root64 );
    RegCloseKey( root32 );

    err = RegCreateKeyExA( HKEY_LOCAL_MACHINE, "Software\\Classes\\Wow6432Node\\Wine",
                           0, NULL, 0, KEY_ALL_ACCESS, NULL, &key32, NULL);
    ok( err == ERROR_SUCCESS, "RegCreateKeyA failed: %lu\n", err );

    err = RegOpenKeyExA( HKEY_LOCAL_MACHINE, "Software\\Classes\\Wow6432Node\\Wine",
                         0, KEY_ALL_ACCESS, &key );
    ok( err == ERROR_SUCCESS, "RegOpenKeyExA failed: %lu\n", err );
    RegCloseKey( key );

    err = RegOpenKeyExA( HKEY_LOCAL_MACHINE, "Software\\Classes\\Wow6432Node\\Wine",
                         0, KEY_WOW64_32KEY | KEY_ALL_ACCESS, &key );
    todo_wine_if(ptr_size == 64) ok( err == (ptr_size == 64 ? ERROR_FILE_NOT_FOUND : ERROR_SUCCESS),
        "RegOpenKeyExA failed: %lu\n", err );
    if (!err) RegCloseKey( key );

    err = RegOpenKeyExA( HKEY_LOCAL_MACHINE, "Software\\Classes\\Wow6432Node\\Wine",
                         0, KEY_WOW64_64KEY | KEY_ALL_ACCESS, &key );
    ok( err == (ptr_size == 32 ? ERROR_FILE_NOT_FOUND : ERROR_SUCCESS),
        "RegOpenKeyExA failed: %lu\n", err );
    if (!err) RegCloseKey( key );

    err = RegOpenKeyExA( HKEY_LOCAL_MACHINE, "Software\\Classes\\Wine",
                         0, KEY_ALL_ACCESS, &key );
    ok( err == (ptr_size == 64 ? ERROR_FILE_NOT_FOUND : ERROR_SUCCESS),
        "RegOpenKeyExA failed: %lu\n", err );
    if (!err) RegCloseKey( key );

    err = RegOpenKeyExA( HKEY_LOCAL_MACHINE, "Software\\Classes\\Wine",
                         0, KEY_WOW64_32KEY | KEY_ALL_ACCESS, &key );
    todo_wine_if(ptr_size == 64) ok( err == (ptr_size == 64 ? ERROR_FILE_NOT_FOUND : ERROR_SUCCESS),
        "RegOpenKeyExA failed: %lu\n", err );
    if (!err) RegCloseKey( key );

    err = RegOpenKeyExA( HKEY_LOCAL_MACHINE, "Software\\Classes\\Wine",
                         0, KEY_WOW64_64KEY | KEY_ALL_ACCESS, &key );
    ok( err == (ptr_size == 64 ? ERROR_FILE_NOT_FOUND : ERROR_SUCCESS),
        "RegOpenKeyExA failed: %lu\n", err );
    if (!err) RegCloseKey( key );

    err = RegOpenKeyExA( HKEY_LOCAL_MACHINE, "Software\\Classes\\Wow6432Node",
                         0, KEY_WOW64_32KEY | KEY_ALL_ACCESS, &root32 );
    ok( err == ERROR_SUCCESS, "RegOpenKeyExA failed: %lu\n", err );

    err = RegQueryInfoKeyA(root32, NULL, NULL, NULL, &subkeys,
                           NULL, NULL, NULL, NULL, NULL, NULL, NULL );
    ok( err == ERROR_SUCCESS, "RegQueryInfoKeyA failed: %lu\n", err );
    ok( subkeys > 0, "wrong number of subkeys: %lu\n", subkeys );
    subkeys32 = subkeys;
    RegCloseKey( root32 );

    err = RegOpenKeyExA( HKEY_LOCAL_MACHINE, "Software\\Classes",
                         0, KEY_WOW64_64KEY | KEY_ALL_ACCESS, &root64 );
    ok( err == ERROR_SUCCESS, "RegOpenKeyExA failed: %lu\n", err );

    err = RegQueryInfoKeyA(root64, NULL, NULL, NULL, &subkeys,
                           NULL, NULL, NULL, NULL, NULL, NULL, NULL );
    ok( err == ERROR_SUCCESS, "RegQueryInfoKeyA failed: %lu\n", err );
    ok( subkeys > subkeys32, "wrong number of subkeys: %lu\n", subkeys );
    subkeys64 = subkeys;
    RegCloseKey( root64 );

    check_enum_value( "Software\\Classes",
                      KEY_WOW64_32KEY | KEY_ALL_ACCESS, subkeys64, ptr_size == 32 );
    check_enum_value( "Software\\Classes",
                      KEY_WOW64_64KEY | KEY_ALL_ACCESS, subkeys64, ptr_size == 32 );
    check_enum_value( "Software\\Classes",
                      KEY_ALL_ACCESS, subkeys64, ptr_size == 32 );
    check_enum_value( "Software\\Classes\\Wow6432Node",
                      KEY_WOW64_32KEY | KEY_ALL_ACCESS, subkeys32, ptr_size == 64 );
    check_enum_value( "Software\\Classes\\Wow6432Node",
                      KEY_WOW64_64KEY | KEY_ALL_ACCESS, subkeys32, ptr_size == 64 );
    check_enum_value( "Software\\Classes\\Wow6432Node",
                      KEY_ALL_ACCESS, subkeys32, ptr_size == 64 );
    check_enum_value( "Software\\Wow6432Node\\Classes",
                      KEY_WOW64_32KEY | KEY_ALL_ACCESS, subkeys64, ptr_size == 32 );
    check_enum_value( "Software\\Wow6432Node\\Classes",
                      KEY_WOW64_64KEY | KEY_ALL_ACCESS, subkeys32, ptr_size == 64 );
    check_enum_value( "Software\\Wow6432Node\\Classes",
                      KEY_ALL_ACCESS, ptr_size == 32 ? subkeys64 : subkeys32, TRUE );

    RegDeleteKeyA( key32, "" );
    RegCloseKey( key32 );
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
                 "test key not found in hkcr: %ld\n", res);
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
    ok(res == ERROR_SUCCESS, "RegSetValueExA failed: %ld, GLE=%lx\n", res, GetLastError());

    /* try to find the value in hkcr */
    res = RegQueryValueExA(hkcr, "val1", NULL, &type, (LPBYTE)buffer, &size);
    ok(res == ERROR_SUCCESS, "RegQueryValueExA failed: %ld\n", res);
    ok(!strcmp( buffer, "user" ), "value set to '%s'\n", buffer );

    /* modify the value in hkcr */
    res = RegSetValueExA(hkcr, "val1", 0, REG_SZ, (const BYTE *)"hkcr", sizeof("hkcr"));
    ok(res == ERROR_SUCCESS, "RegSetValueExA failed: %ld, GLE=%lx\n", res, GetLastError());

    /* check if the value is also modified in user's classes */
    res = RegQueryValueExA(hkey, "val1", NULL, &type, (LPBYTE)buffer, &size);
    ok(res == ERROR_SUCCESS, "RegQueryValueExA failed: %ld, GLE=%lx\n", res, GetLastError());
    ok(!strcmp( buffer, "hkcr" ), "value set to '%s'\n", buffer );

    /* set a value in hkcr */
    res = RegSetValueExA(hkcr, "val0", 0, REG_SZ, (const BYTE *)"hkcr", sizeof("hkcr"));
    ok(res == ERROR_SUCCESS, "RegSetValueExA failed: %ld, GLE=%lx\n", res, GetLastError());

    /* try to find the value in user's classes */
    res = RegQueryValueExA(hkey, "val0", NULL, &type, (LPBYTE)buffer, &size);
    ok(res == ERROR_SUCCESS, "RegQueryValueExA failed: %ld\n", res);
    ok(!strcmp( buffer, "hkcr" ), "value set to '%s'\n", buffer );

    /* modify the value in user's classes */
    res = RegSetValueExA(hkey, "val0", 0, REG_SZ, (const BYTE *)"user", sizeof("user"));
    ok(res == ERROR_SUCCESS, "RegSetValueExA failed: %ld, GLE=%lx\n", res, GetLastError());

    /* check if the value is also modified in hkcr */
    res = RegQueryValueExA(hkcr, "val0", NULL, &type, (LPBYTE)buffer, &size);
    ok(res == ERROR_SUCCESS, "RegQueryValueExA failed: %ld, GLE=%lx\n", res, GetLastError());
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
       "test key not found in hkcr: %ld\n", res);
    ok(IS_HKCR(hkcr), "hkcr mask not set in %p\n", hkcr);
    if (res)
    {
        delete_key( hklm );
        RegCloseKey( hklm );
        return;
    }

    /* set a value in hklm classes */
    res = RegSetValueExA(hklm, "val2", 0, REG_SZ, (const BYTE *)"hklm", sizeof("hklm"));
    ok(res == ERROR_SUCCESS, "RegSetValueExA failed: %ld, GLE=%lx\n", res, GetLastError());

    /* try to find the value in hkcr */
    res = RegQueryValueExA(hkcr, "val2", NULL, &type, (LPBYTE)buffer, &size);
    ok(res == ERROR_SUCCESS, "RegQueryValueExA failed: %ld\n", res);
    ok(!strcmp( buffer, "hklm" ), "value set to '%s'\n", buffer );

    /* modify the value in hkcr */
    res = RegSetValueExA(hkcr, "val2", 0, REG_SZ, (const BYTE *)"hkcr", sizeof("hkcr"));
    ok(res == ERROR_SUCCESS, "RegSetValueExA failed: %ld, GLE=%lx\n", res, GetLastError());

    /* check that the value is modified in hklm classes */
    res = RegQueryValueExA(hklm, "val2", NULL, &type, (LPBYTE)buffer, &size);
    ok(res == ERROR_SUCCESS, "RegQueryValueExA failed: %ld, GLE=%lx\n", res, GetLastError());
    ok(!strcmp( buffer, "hkcr" ), "value set to '%s'\n", buffer );

    if (RegCreateKeyExA( HKEY_CURRENT_USER, "Software\\Classes\\WineTestCls", 0, NULL, 0,
                         KEY_QUERY_VALUE|KEY_SET_VALUE, NULL, &hkey, NULL )) return;
    ok(!IS_HKCR(hkey), "hkcr mask set in %p\n", hkey);

    /* try to open that key in hkcr */
    res = RegOpenKeyExA( HKEY_CLASSES_ROOT, "WineTestCls", 0,
                         KEY_QUERY_VALUE|KEY_SET_VALUE, &hkcr );
    ok(res == ERROR_SUCCESS,
       "test key not found in hkcr: %ld\n", res);
    ok(IS_HKCR(hkcr), "hkcr mask not set in %p\n", hkcr);

    /* set a value in user's classes */
    res = RegSetValueExA(hkey, "val2", 0, REG_SZ, (const BYTE *)"user", sizeof("user"));
    ok(res == ERROR_SUCCESS, "RegSetValueExA failed: %ld, GLE=%lx\n", res, GetLastError());

    /* try to find the value in hkcr */
    res = RegQueryValueExA(hkcr, "val2", NULL, &type, (LPBYTE)buffer, &size);
    ok(res == ERROR_SUCCESS, "RegQueryValueExA failed: %ld\n", res);
    ok(!strcmp( buffer, "user" ), "value set to '%s'\n", buffer );

    /* modify the value in hklm */
    res = RegSetValueExA(hklm, "val2", 0, REG_SZ, (const BYTE *)"hklm", sizeof("hklm"));
    ok(res == ERROR_SUCCESS, "RegSetValueExA failed: %ld, GLE=%lx\n", res, GetLastError());

    /* check that the value is not overwritten in hkcr or user's classes */
    res = RegQueryValueExA(hkcr, "val2", NULL, &type, (LPBYTE)buffer, &size);
    ok(res == ERROR_SUCCESS, "RegQueryValueExA failed: %ld\n", res);
    ok(!strcmp( buffer, "user" ), "value set to '%s'\n", buffer );
    res = RegQueryValueExA(hkey, "val2", NULL, &type, (LPBYTE)buffer, &size);
    ok(res == ERROR_SUCCESS, "RegQueryValueExA failed: %ld, GLE=%lx\n", res, GetLastError());
    ok(!strcmp( buffer, "user" ), "value set to '%s'\n", buffer );

    /* modify the value in hkcr */
    res = RegSetValueExA(hkcr, "val2", 0, REG_SZ, (const BYTE *)"hkcr", sizeof("hkcr"));
    ok(res == ERROR_SUCCESS, "RegSetValueExA failed: %ld, GLE=%lx\n", res, GetLastError());

    /* check that the value is overwritten in hklm and user's classes */
    res = RegQueryValueExA(hkcr, "val2", NULL, &type, (LPBYTE)buffer, &size);
    ok(res == ERROR_SUCCESS, "RegQueryValueExA failed: %ld\n", res);
    ok(!strcmp( buffer, "hkcr" ), "value set to '%s'\n", buffer );
    res = RegQueryValueExA(hkey, "val2", NULL, &type, (LPBYTE)buffer, &size);
    ok(res == ERROR_SUCCESS, "RegQueryValueExA failed: %ld, GLE=%lx\n", res, GetLastError());
    ok(!strcmp( buffer, "hkcr" ), "value set to '%s'\n", buffer );

    /* create a subkey in hklm */
    if (RegCreateKeyExA( hklm, "subkey1", 0, NULL, 0,
                         KEY_QUERY_VALUE|KEY_SET_VALUE, NULL, &hklmsub1, NULL )) return;
    ok(!IS_HKCR(hklmsub1), "hkcr mask set in %p\n", hklmsub1);
    /* try to open that subkey in hkcr */
    res = RegOpenKeyExA( hkcr, "subkey1", 0, KEY_QUERY_VALUE|KEY_SET_VALUE, &hkcrsub1 );
    ok(res == ERROR_SUCCESS, "test key not found in hkcr: %ld\n", res);
    ok(IS_HKCR(hkcrsub1), "hkcr mask not set in %p\n", hkcrsub1);

    /* set a value in hklm classes */
    res = RegSetValueExA(hklmsub1, "subval1", 0, REG_SZ, (const BYTE *)"hklm", sizeof("hklm"));
    ok(res == ERROR_SUCCESS, "RegSetValueExA failed: %ld, GLE=%lx\n", res, GetLastError());

    /* try to find the value in hkcr */
    res = RegQueryValueExA(hkcrsub1, "subval1", NULL, &type, (LPBYTE)buffer, &size);
    ok(res == ERROR_SUCCESS, "RegQueryValueExA failed: %ld\n", res);
    ok(!strcmp( buffer, "hklm" ), "value set to '%s'\n", buffer );

    /* modify the value in hkcr */
    res = RegSetValueExA(hkcrsub1, "subval1", 0, REG_SZ, (const BYTE *)"hkcr", sizeof("hkcr"));
    ok(res == ERROR_SUCCESS, "RegSetValueExA failed: %ld, GLE=%lx\n", res, GetLastError());

    /* check that the value is modified in hklm classes */
    res = RegQueryValueExA(hklmsub1, "subval1", NULL, &type, (LPBYTE)buffer, &size);
    ok(res == ERROR_SUCCESS, "RegQueryValueExA failed: %ld, GLE=%lx\n", res, GetLastError());
    ok(!strcmp( buffer, "hkcr" ), "value set to '%s'\n", buffer );

    /* create a subkey in user's classes */
    if (RegCreateKeyExA( hkey, "subkey1", 0, NULL, 0,
                         KEY_QUERY_VALUE|KEY_SET_VALUE, NULL, &hkeysub1, NULL )) return;
    ok(!IS_HKCR(hkeysub1), "hkcr mask set in %p\n", hkeysub1);

    /* set a value in user's classes */
    res = RegSetValueExA(hkeysub1, "subval1", 0, REG_SZ, (const BYTE *)"user", sizeof("user"));
    ok(res == ERROR_SUCCESS, "RegSetValueExA failed: %ld, GLE=%lx\n", res, GetLastError());

    /* try to find the value in hkcr */
    res = RegQueryValueExA(hkcrsub1, "subval1", NULL, &type, (LPBYTE)buffer, &size);
    ok(res == ERROR_SUCCESS, "RegQueryValueExA failed: %ld\n", res);
    ok(!strcmp( buffer, "user" ), "value set to '%s'\n", buffer );

    /* modify the value in hklm */
    res = RegSetValueExA(hklmsub1, "subval1", 0, REG_SZ, (const BYTE *)"hklm", sizeof("hklm"));
    ok(res == ERROR_SUCCESS, "RegSetValueExA failed: %ld, GLE=%lx\n", res, GetLastError());

    /* check that the value is not overwritten in hkcr or user's classes */
    res = RegQueryValueExA(hkcrsub1, "subval1", NULL, &type, (LPBYTE)buffer, &size);
    ok(res == ERROR_SUCCESS, "RegQueryValueExA failed: %ld\n", res);
    ok(!strcmp( buffer, "user" ), "value set to '%s'\n", buffer );
    res = RegQueryValueExA(hkeysub1, "subval1", NULL, &type, (LPBYTE)buffer, &size);
    ok(res == ERROR_SUCCESS, "RegQueryValueExA failed: %ld, GLE=%lx\n", res, GetLastError());
    ok(!strcmp( buffer, "user" ), "value set to '%s'\n", buffer );

    /* modify the value in hkcr */
    res = RegSetValueExA(hkcrsub1, "subval1", 0, REG_SZ, (const BYTE *)"hkcr", sizeof("hkcr"));
    ok(res == ERROR_SUCCESS, "RegSetValueExA failed: %ld, GLE=%lx\n", res, GetLastError());

    /* check that the value is not overwritten in hklm, but in user's classes */
    res = RegQueryValueExA(hklmsub1, "subval1", NULL, &type, (LPBYTE)buffer, &size);
    ok(res == ERROR_SUCCESS, "RegQueryValueExA failed: %ld\n", res);
    ok(!strcmp( buffer, "hklm" ), "value set to '%s'\n", buffer );
    res = RegQueryValueExA(hkeysub1, "subval1", NULL, &type, (LPBYTE)buffer, &size);
    ok(res == ERROR_SUCCESS, "RegQueryValueExA failed: %ld, GLE=%lx\n", res, GetLastError());
    ok(!strcmp( buffer, "hkcr" ), "value set to '%s'\n", buffer );

    /* new subkey in hkcr */
    if (RegCreateKeyExA( hkcr, "subkey2", 0, NULL, 0,
                         KEY_QUERY_VALUE|KEY_SET_VALUE, NULL, &hkcrsub2, NULL )) return;
    ok(IS_HKCR(hkcrsub2), "hkcr mask not set in %p\n", hkcrsub2);
    res = RegSetValueExA(hkcrsub2, "subval1", 0, REG_SZ, (const BYTE *)"hkcr", sizeof("hkcr"));
    ok(res == ERROR_SUCCESS, "RegSetValueExA failed: %ld, GLE=%lx\n", res, GetLastError());

    /* try to open that new subkey in user's classes and hklm */
    res = RegOpenKeyExA( hkey, "subkey2", 0, KEY_QUERY_VALUE|KEY_SET_VALUE, &hklmsub2 );
    ok(res != ERROR_SUCCESS, "test key found in user's classes: %ld\n", res);
    hklmsub2 = 0;
    res = RegOpenKeyExA( hklm, "subkey2", 0, KEY_QUERY_VALUE|KEY_SET_VALUE, &hklmsub2 );
    ok(res == ERROR_SUCCESS, "test key not found in hklm: %ld\n", res);
    ok(!IS_HKCR(hklmsub2), "hkcr mask set in %p\n", hklmsub2);

    /* check that the value is present in hklm */
    res = RegQueryValueExA(hklmsub2, "subval1", NULL, &type, (LPBYTE)buffer, &size);
    ok(res == ERROR_SUCCESS, "RegQueryValueExA failed: %ld\n", res);
    ok(!strcmp( buffer, "hkcr" ), "value set to '%s'\n", buffer );

    /* cleanup */
    RegCloseKey( hkeysub1 );
    RegCloseKey( hklmsub1 );

    /* delete subkey1 from hkcr (should point at user's classes) */
    res = RegDeleteKeyA(hkcr, "subkey1");
    ok(res == ERROR_SUCCESS, "RegDeleteKey failed: %ld\n", res);

    /* confirm key was removed in hkey but not hklm */
    res = RegOpenKeyExA(hkey, "subkey1", 0, KEY_READ, &hkeysub1);
    ok(res == ERROR_FILE_NOT_FOUND, "test key found in user's classes: %ld\n", res);
    res = RegOpenKeyExA(hklm, "subkey1", 0, KEY_READ, &hklmsub1);
    ok(res == ERROR_SUCCESS, "test key not found in hklm: %ld\n", res);
    ok(!IS_HKCR(hklmsub1), "hkcr mask set in %p\n", hklmsub1);

    /* delete subkey1 from hkcr again (which should now point at hklm) */
    res = RegDeleteKeyA(hkcr, "subkey1");
    ok(res == ERROR_SUCCESS, "RegDeleteKey failed: %ld\n", res);

    /* confirm hkey was removed in hklm */
    RegCloseKey( hklmsub1 );
    res = RegOpenKeyExA(hklm, "subkey1", 0, KEY_READ, &hklmsub1);
    ok(res == ERROR_FILE_NOT_FOUND, "test key found in hklm: %ld\n", res);

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
                 "test key not found in hkcr: %ld\n", res);
    if (res)
    {
        skip("HKCR key merging not supported\n");
        delete_key( hkcu );
        RegCloseKey( hkcu );
        return;
    }

    res = RegSetValueExA( hkcu, "X", 0, REG_SZ, (const BYTE *) "AA", 3 );
    ok(res == ERROR_SUCCESS, "RegSetValueExA failed: %ld\n", res);
    res = RegSetValueExA( hkcu, "Y", 0, REG_SZ, (const BYTE *) "B", 2 );
    ok(res == ERROR_SUCCESS, "RegSetValueExA failed: %ld\n", res);
    res = RegCreateKeyA( hkcu, "A", &hkcusub[0] );
    ok(res == ERROR_SUCCESS, "RegCreateKeyA failed: %ld\n", res);
    res = RegCreateKeyA( hkcu, "B", &hkcusub[1] );
    ok(res == ERROR_SUCCESS, "RegCreateKeyA failed: %ld\n", res);

    /* test on values in HKCU */
    size = sizeof(buffer);
    res = RegEnumValueA( hkcr, 0, buffer, &size, NULL, NULL, NULL, NULL );
    ok(res == ERROR_SUCCESS, "RegEnumValueA failed: %ld\n", res );
    ok(!strcmp( buffer, "X" ), "expected 'X', got '%s'\n", buffer);
    size = sizeof(buffer);
    res = RegEnumValueA( hkcr, 1, buffer, &size, NULL, NULL, NULL, NULL );
    ok(res == ERROR_SUCCESS, "RegEnumValueA failed: %ld\n", res );
    ok(!strcmp( buffer, "Y" ), "expected 'Y', got '%s'\n", buffer);
    size = sizeof(buffer);
    res = RegEnumValueA( hkcr, 2, buffer, &size, NULL, NULL, NULL, NULL );
    ok(res == ERROR_NO_MORE_ITEMS, "expected ERROR_NO_MORE_ITEMS, got %ld\n", res );

    res = RegEnumKeyA( hkcr, 0, buffer, size );
    ok(res == ERROR_SUCCESS, "RegEnumKey failed: %ld\n", res );
    ok(!strcmp( buffer, "A" ), "expected 'A', got '%s'\n", buffer);
    res = RegEnumKeyA( hkcr, 1, buffer, size );
    ok(res == ERROR_SUCCESS, "RegEnumKey failed: %ld\n", res );
    ok(!strcmp( buffer, "B" ), "expected 'B', got '%s'\n", buffer);
    res = RegEnumKeyA( hkcr, 2, buffer, size );
    ok(res == ERROR_NO_MORE_ITEMS, "expected ERROR_NO_MORE_ITEMS, got %ld\n", res );

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
    ok(res == ERROR_SUCCESS, "RegSetValueExA failed: %ld\n", res);
    res = RegSetValueExA( hklm, "Z", 0, REG_SZ, (const BYTE *) "C", 2 );
    ok(res == ERROR_SUCCESS, "RegSetValueExA failed: %ld\n", res);
    res = RegCreateKeyA( hklm, "A", &hklmsub[0] );
    ok(res == ERROR_SUCCESS, "RegCreateKeyA failed: %ld\n", res);
    res = RegCreateKeyA( hklm, "C", &hklmsub[1] );
    ok(res == ERROR_SUCCESS, "RegCreateKeyA failed: %ld\n", res);

    /* test on values/keys in both HKCU and HKLM */
    size = sizeof(buffer);
    res = RegEnumValueA( hkcr, 0, buffer, &size, NULL, NULL, NULL, NULL );
    ok(res == ERROR_SUCCESS, "RegEnumValueA failed: %ld\n", res );
    ok(!strcmp( buffer, "X" ), "expected 'X', got '%s'\n", buffer);
    size = sizeof(buffer);
    res = RegEnumValueA( hkcr, 1, buffer, &size, NULL, NULL, NULL, NULL );
    ok(res == ERROR_SUCCESS, "RegEnumValueA failed: %ld\n", res );
    ok(!strcmp( buffer, "Y" ), "expected 'Y', got '%s'\n", buffer);
    size = sizeof(buffer);
    res = RegEnumValueA( hkcr, 2, buffer, &size, NULL, NULL, NULL, NULL );
    ok(res == ERROR_SUCCESS, "RegEnumValueA failed: %ld\n", res );
    ok(!strcmp( buffer, "Z" ), "expected 'Z', got '%s'\n", buffer);
    size = sizeof(buffer);
    res = RegEnumValueA( hkcr, 3, buffer, &size, NULL, NULL, NULL, NULL );
    ok(res == ERROR_NO_MORE_ITEMS, "expected ERROR_NO_MORE_ITEMS, got %ld\n", res );

    res = RegEnumKeyA( hkcr, 0, buffer, size );
    ok(res == ERROR_SUCCESS, "RegEnumKey failed: %ld\n", res );
    ok(!strcmp( buffer, "A" ), "expected 'A', got '%s'\n", buffer);
    res = RegEnumKeyA( hkcr, 1, buffer, size );
    ok(res == ERROR_SUCCESS, "RegEnumKey failed: %ld\n", res );
    ok(!strcmp( buffer, "B" ), "expected 'B', got '%s'\n", buffer);
    res = RegEnumKeyA( hkcr, 2, buffer, size );
    ok(res == ERROR_SUCCESS, "RegEnumKey failed: %ld\n", res );
    ok(!strcmp( buffer, "C" ), "expected 'C', got '%s'\n", buffer);
    res = RegEnumKeyA( hkcr, 3, buffer, size );
    ok(res == ERROR_NO_MORE_ITEMS, "expected ERROR_NO_MORE_ITEMS, got %ld\n", res );

    /* delete values/keys from HKCU to test only on HKLM */
    RegCloseKey( hkcusub[0] );
    RegCloseKey( hkcusub[1] );
    delete_key( hkcu );
    RegCloseKey( hkcu );

    size = sizeof(buffer);
    res = RegEnumValueA( hkcr, 0, buffer, &size, NULL, NULL, NULL, NULL );
    ok(res == ERROR_KEY_DELETED ||
       res == ERROR_NO_SYSTEM_RESOURCES, /* Windows XP */
       "expected ERROR_KEY_DELETED, got %ld\n", res);
    size = sizeof(buffer);
    res = RegEnumKeyA( hkcr, 0, buffer, size );
    ok(res == ERROR_KEY_DELETED ||
       res == ERROR_NO_SYSTEM_RESOURCES, /* Windows XP */
       "expected ERROR_KEY_DELETED, got %ld\n", res);

    /* reopen HKCR handle */
    RegCloseKey( hkcr );
    res = RegOpenKeyA( HKEY_CLASSES_ROOT, "WineTestCls", &hkcr );
    ok(res == ERROR_SUCCESS, "test key not found in hkcr: %ld\n", res);
    if (res) goto cleanup;

    /* test on values/keys in HKLM */
    size = sizeof(buffer);
    res = RegEnumValueA( hkcr, 0, buffer, &size, NULL, NULL, NULL, NULL );
    ok(res == ERROR_SUCCESS, "RegEnumValueA failed: %ld\n", res );
    ok(!strcmp( buffer, "X" ), "expected 'X', got '%s'\n", buffer);
    size = sizeof(buffer);
    res = RegEnumValueA( hkcr, 1, buffer, &size, NULL, NULL, NULL, NULL );
    ok(res == ERROR_SUCCESS, "RegEnumValueA failed: %ld\n", res );
    ok(!strcmp( buffer, "Z" ), "expected 'Z', got '%s'\n", buffer);
    size = sizeof(buffer);
    res = RegEnumValueA( hkcr, 2, buffer, &size, NULL, NULL, NULL, NULL );
    ok(res == ERROR_NO_MORE_ITEMS, "expected ERROR_NO_MORE_ITEMS, got %ld\n", res );

    res = RegEnumKeyA( hkcr, 0, buffer, size );
    ok(res == ERROR_SUCCESS, "RegEnumKey failed: %ld\n", res );
    ok(!strcmp( buffer, "A" ), "expected 'A', got '%s'\n", buffer);
    res = RegEnumKeyA( hkcr, 1, buffer, size );
    ok(res == ERROR_SUCCESS, "RegEnumKey failed: %ld\n", res );
    ok(!strcmp( buffer, "C" ), "expected 'C', got '%s'\n", buffer);
    res = RegEnumKeyA( hkcr, 2, buffer, size );
    ok(res == ERROR_NO_MORE_ITEMS, "expected ERROR_NO_MORE_ITEMS, got %ld\n", res );

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
    ok(res == ERROR_SUCCESS, "RegOpenKeyA failed: %ld\n", res);
    todo_wine ok(IS_HKCR(hkey) || broken(!IS_HKCR(hkey)) /* WinNT */,
                 "hkcr mask not set in %p\n", hkey);
    RegCloseKey( hkey );

    res = RegOpenKeyA( HKEY_CURRENT_USER, "Software", &hkey );
    ok(res == ERROR_SUCCESS, "RegOpenKeyA failed: %ld\n", res);
    ok(!IS_HKCR(hkey), "hkcr mask set in %p\n", hkey);
    RegCloseKey( hkey );

    res = RegOpenKeyA( HKEY_LOCAL_MACHINE, "Software", &hkey );
    ok(res == ERROR_SUCCESS, "RegOpenKeyA failed: %ld\n", res);
    ok(!IS_HKCR(hkey), "hkcr mask set in %p\n", hkey);
    RegCloseKey( hkey );

    res = RegOpenKeyA( HKEY_USERS, ".Default", &hkey );
    ok(res == ERROR_SUCCESS, "RegOpenKeyA failed: %ld\n", res);
    ok(!IS_HKCR(hkey), "hkcr mask set in %p\n", hkey);
    RegCloseKey( hkey );

    res = RegOpenKeyA( HKEY_CURRENT_CONFIG, "Software", &hkey );
    ok(res == ERROR_SUCCESS, "RegOpenKeyA failed: %ld\n", res);
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
    ok(res == ERROR_KEY_DELETED, "expect ERROR_KEY_DELETED, got %li\n", res);

    res = RegEnumKeyA( hkey, 0, value, sizeof(value) );
    ok(res == ERROR_KEY_DELETED, "expect ERROR_KEY_DELETED, got %li\n", res);

    val_count = sizeof(value);
    type = 0;
    res = RegQueryValueExA( hkey, "test", NULL, &type, (BYTE *)value, &val_count );
    ok(res == ERROR_KEY_DELETED, "expect ERROR_KEY_DELETED, got %li\n", res);

    res = RegSetValueExA( hkey, "test", 0, REG_SZ, (const BYTE*)"value", 6);
    ok(res == ERROR_KEY_DELETED, "expect ERROR_KEY_DELETED, got %li\n", res);

    res = RegOpenKeyA( hkey, "test", &hkey2 );
    ok(res == ERROR_KEY_DELETED, "expect ERROR_KEY_DELETED, got %li\n", res);
    if (res == 0)
        RegCloseKey( hkey2 );

    res = RegCreateKeyA( hkey, "test", &hkey2 );
    ok(res == ERROR_KEY_DELETED, "expect ERROR_KEY_DELETED, got %li\n", res);
    if (res == 0)
        RegCloseKey( hkey2 );

    res = RegFlushKey( hkey );
    ok(res == ERROR_KEY_DELETED, "expect ERROR_KEY_DELETED, got %li\n", res);

    RegCloseKey( hkey );

    setup_main_key();
}

static void test_delete_value(void)
{
    LONG res;
    char longname[401];

    res = RegSetValueExA( hkey_main, "test", 0, REG_SZ, (const BYTE*)"value", 6 );
    ok(res == ERROR_SUCCESS, "expect ERROR_SUCCESS, got %li\n", res);

    res = RegQueryValueExA( hkey_main, "test", NULL, NULL, NULL, NULL);
    ok(res == ERROR_SUCCESS, "expect ERROR_SUCCESS, got %li\n", res);

    res = RegDeleteValueA( hkey_main, "test" );
    ok(res == ERROR_SUCCESS, "expect ERROR_SUCCESS, got %li\n", res);

    res = RegQueryValueExA( hkey_main, "test", NULL, NULL, NULL, NULL);
    ok(res == ERROR_FILE_NOT_FOUND, "expect ERROR_FILE_NOT_FOUND, got %li\n", res);

    res = RegDeleteValueA( hkey_main, "test" );
    ok(res == ERROR_FILE_NOT_FOUND, "expect ERROR_FILE_NOT_FOUND, got %li\n", res);

    memset(longname, 'a', 400);
    longname[400] = 0;
    res = RegDeleteValueA( hkey_main, longname );
    ok(res == ERROR_FILE_NOT_FOUND, "expect ERROR_FILE_NOT_FOUND, got %li\n", res);

    /* Default registry value */
    res = RegSetValueExA(hkey_main, "", 0, REG_SZ, (const BYTE *)"value", 6);
    ok(res == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", res);

    res = RegQueryValueExA(hkey_main, "", NULL, NULL, NULL, NULL);
    ok(res == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", res);

    res = RegDeleteValueA(hkey_main, "" );
    ok(res == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", res);

    res = RegQueryValueExA(hkey_main, "", NULL, NULL, NULL, NULL);
    ok(res == ERROR_FILE_NOT_FOUND, "expected ERROR_FILE_NOT_FOUND, got %ld\n", res);
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
    ok(ret == ERROR_INVALID_HANDLE, "got %ld\n", ret);

    ret = pRegDeleteKeyValueA(hkey_main, NULL, NULL);
    ok(ret == ERROR_FILE_NOT_FOUND, "got %ld\n", ret);

    ret = RegSetValueExA(hkey_main, "test", 0, REG_SZ, (const BYTE*)"value", 6);
    ok(ret == ERROR_SUCCESS, "got %ld\n", ret);

    ret = RegQueryValueExA(hkey_main, "test", NULL, NULL, NULL, NULL);
    ok(ret == ERROR_SUCCESS, "got %ld\n", ret);

    /* NULL subkey name means delete from open key */
    ret = pRegDeleteKeyValueA(hkey_main, NULL, "test");
    ok(ret == ERROR_SUCCESS, "got %ld\n", ret);

    ret = RegQueryValueExA(hkey_main, "test", NULL, NULL, NULL, NULL);
    ok(ret == ERROR_FILE_NOT_FOUND, "got %ld\n", ret);

    /* now with real subkey */
    ret = RegCreateKeyExA(hkey_main, "Subkey1", 0, NULL, 0, KEY_WRITE|KEY_READ, NULL, &subkey, NULL);
    ok(!ret, "failed with error %ld\n", ret);

    ret = RegSetValueExA(subkey, "test", 0, REG_SZ, (const BYTE*)"value", 6);
    ok(ret == ERROR_SUCCESS, "got %ld\n", ret);

    ret = RegQueryValueExA(subkey, "test", NULL, NULL, NULL, NULL);
    ok(ret == ERROR_SUCCESS, "got %ld\n", ret);

    ret = pRegDeleteKeyValueA(hkey_main, "Subkey1", "test");
    ok(ret == ERROR_SUCCESS, "got %ld\n", ret);

    ret = RegQueryValueExA(subkey, "test", NULL, NULL, NULL, NULL);
    ok(ret == ERROR_FILE_NOT_FOUND, "got %ld\n", ret);

    /* Default registry value */
    ret = RegSetValueExA(subkey, "", 0, REG_SZ, (const BYTE *)"value", 6);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);

    ret = RegQueryValueExA(subkey, "", NULL, NULL, NULL, NULL);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);

    ret = pRegDeleteKeyValueA(hkey_main, "Subkey1", "" );
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);

    ret = RegQueryValueExA(subkey, "", NULL, NULL, NULL, NULL);
    ok(ret == ERROR_FILE_NOT_FOUND, "expected ERROR_FILE_NOT_FOUND, got %ld\n", ret);

    RegDeleteKeyA(subkey, "");
    RegCloseKey(subkey);
}

static void test_RegOpenCurrentUser(void)
{
    HKEY key;
    LONG ret;

    key = HKEY_CURRENT_USER;
    ret = RegOpenCurrentUser(KEY_READ, &key);
    ok(!ret, "got %ld, error %ld\n", ret, GetLastError());
    ok(key != HKEY_CURRENT_USER, "got %p\n", key);
    RegCloseKey(key);
}

struct notify_data {
    HKEY key;
    DWORD flags;
    HANDLE event;
};

static DWORD WINAPI notify_change_thread(void *arg)
{
    struct notify_data *data = arg;
    LONG ret;

    ret = RegNotifyChangeKeyValue(data->key, TRUE,
            REG_NOTIFY_CHANGE_NAME|data->flags, data->event, TRUE);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);
    return 0;
}

static void test_RegNotifyChangeKeyValue(void)
{
    struct notify_data data;
    HKEY key, subkey, subsubkey;
    HANDLE thread;
    HANDLE event;
    DWORD dwret;
    LONG ret;

    event = CreateEventW(NULL, FALSE, TRUE, NULL);
    ok(event != NULL, "CreateEvent failed, error %lu\n", GetLastError());
    ret = RegCreateKeyA(hkey_main, "TestKey", &key);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);

    ret = RegNotifyChangeKeyValue(key, TRUE, REG_NOTIFY_CHANGE_NAME|REG_NOTIFY_CHANGE_LAST_SET, event, TRUE);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);
    dwret = WaitForSingleObject(event, 0);
    ok(dwret == WAIT_TIMEOUT, "expected WAIT_TIMEOUT, got %lu\n", dwret);

    ret = RegCreateKeyA(key, "SubKey", &subkey);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);
    dwret = WaitForSingleObject(event, 0);
    ok(dwret == WAIT_OBJECT_0, "expected WAIT_OBJECT_0, got %lu\n", dwret);

    /* watching deeper keys */
    ret = RegNotifyChangeKeyValue(key, TRUE, REG_NOTIFY_CHANGE_NAME|REG_NOTIFY_CHANGE_LAST_SET, event, TRUE);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);
    dwret = WaitForSingleObject(event, 0);
    ok(dwret == WAIT_TIMEOUT, "expected WAIT_TIMEOUT, got %lu\n", dwret);

    ret = RegCreateKeyA(subkey, "SubKey", &subsubkey);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);
    dwret = WaitForSingleObject(event, 0);
    ok(dwret == WAIT_OBJECT_0, "expected WAIT_OBJECT_0, got %lu\n", dwret);

    /* watching deeper values */
    ret = RegNotifyChangeKeyValue(key, TRUE, REG_NOTIFY_CHANGE_NAME|REG_NOTIFY_CHANGE_LAST_SET, event, TRUE);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);
    dwret = WaitForSingleObject(event, 0);
    ok(dwret == WAIT_TIMEOUT, "expected WAIT_TIMEOUT, got %lu\n", dwret);

    ret = RegSetValueA(subsubkey, NULL, REG_SZ, "SubSubKeyValue", 0);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);
    dwret = WaitForSingleObject(event, 0);
    ok(dwret == WAIT_OBJECT_0, "expected WAIT_OBJECT_0, got %lu\n", dwret);

    /* don't watch deeper values */
    RegCloseKey(key);
    ret = RegOpenKeyA(hkey_main, "TestKey", &key);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);

    ret = RegNotifyChangeKeyValue(key, FALSE, REG_NOTIFY_CHANGE_NAME|REG_NOTIFY_CHANGE_LAST_SET, event, TRUE);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);
    dwret = WaitForSingleObject(event, 0);
    ok(dwret == WAIT_TIMEOUT, "expected WAIT_TIMEOUT, got %lu\n", dwret);

    ret = RegSetValueA(subsubkey, NULL, REG_SZ, "SubSubKeyValueNEW", 0);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);
    dwret = WaitForSingleObject(event, 0);
    ok(dwret == WAIT_TIMEOUT, "expected WAIT_TIMEOUT, got %lu\n", dwret);

    RegDeleteKeyA(subkey, "SubKey");
    RegDeleteKeyA(key, "SubKey");
    RegCloseKey(subsubkey);
    RegCloseKey(subkey);
    RegCloseKey(key);

    /* test same thread with REG_NOTIFY_THREAD_AGNOSTIC */
    ret = RegOpenKeyA(hkey_main, "TestKey", &key);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);
    ret = RegNotifyChangeKeyValue(key, TRUE, REG_NOTIFY_CHANGE_NAME|REG_NOTIFY_THREAD_AGNOSTIC,
            event, TRUE);
    if (ret == ERROR_INVALID_PARAMETER)
    {
        win_skip("REG_NOTIFY_THREAD_AGNOSTIC is not supported\n");
        RegCloseKey(key);
        CloseHandle(event);
        return;
    }

    ret = RegCreateKeyA(key, "SubKey", &subkey);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);
    dwret = WaitForSingleObject(event, 0);
    ok(dwret == WAIT_OBJECT_0, "expected WAIT_OBJECT_0, got %lu\n", dwret);

    RegDeleteKeyA(key, "SubKey");
    RegCloseKey(subkey);
    RegCloseKey(key);

    /* test different thread without REG_NOTIFY_THREAD_AGNOSTIC */
    ret = RegOpenKeyA(hkey_main, "TestKey", &key);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);

    data.key = key;
    data.flags = 0;
    data.event = event;
    thread = CreateThread(NULL, 0, notify_change_thread, &data, 0, NULL);
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);

    /* the thread exiting causes event to signal on Windows
       this is worked around on Windows using REG_NOTIFY_THREAD_AGNOSTIC
       Wine already behaves as if the flag is set */
    dwret = WaitForSingleObject(event, 0);
    todo_wine
    ok(dwret == WAIT_OBJECT_0, "expected WAIT_OBJECT_0, got %lu\n", dwret);
    RegCloseKey(key);

    /* test different thread with REG_NOTIFY_THREAD_AGNOSTIC */
    ret = RegOpenKeyA(hkey_main, "TestKey", &key);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);

    data.key = key;
    data.flags = REG_NOTIFY_THREAD_AGNOSTIC;
    thread = CreateThread(NULL, 0, notify_change_thread, &data, 0, NULL);
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);

    dwret = WaitForSingleObject(event, 0);
    ok(dwret == WAIT_TIMEOUT, "expected WAIT_TIMEOUT, got %lu\n", dwret);

    ret = RegCreateKeyA(key, "SubKey", &subkey);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);

    dwret = WaitForSingleObject(event, 0);
    ok(dwret == WAIT_OBJECT_0, "expected WAIT_OBJECT_0, got %lu\n", dwret);

    RegDeleteKeyA(key, "SubKey");
    RegDeleteKeyA(key, "");
    RegCloseKey(subkey);
    RegCloseKey(key);
    CloseHandle(event);
}

static const char *find_counter_value(const char *text, const char *index)
{
    const char *p = text;

    while (*p)
    {
        if (!strcmp(p, index))
            return p + strlen(p) + 1;

        p += strlen(p) + 1;
        p += strlen(p) + 1;
    }

    return NULL;
}

static void test_counter_values(const char *text, HKEY key)
{
    const char *p = text;
    const char *name;

    ok(!strcmp(p, "1"), "got first index %s\n", debugstr_a(p));
    p += strlen(p) + 1;
    ok(!strcmp(p, "1847"), "got first name %s\n", debugstr_a(p));
    p += strlen(p) + 1;

    while (*p)
    {
        unsigned int index = atoi(p);

        ok(index > 0, "expected nonzero index\n");

        p += strlen(p) + 1;
        ok(*p, "name missing for %u\n", index);
        p += strlen(p) + 1;
    }

    name = find_counter_value(text, "1846");
    ok(name != NULL, "did not find name\n");
    if (key != HKEY_PERFORMANCE_NLSTEXT)
        ok(!strcmp(name, "End Marker"), "got name %s\n", debugstr_a(name));
}

static void test_help_values(const char *text, HKEY key)
{
    const char *p = text;
    const char *name;

    while (*p)
    {
        unsigned int index = atoi(p);

        ok(index > 0, "expected nonzero index\n");

        p += strlen(p) + 1;
        p += strlen(p) + 1;
    }

    name = find_counter_value(text, "1847");
    ok(name != NULL, "did not find name\n");
    if (key != HKEY_PERFORMANCE_NLSTEXT)
        ok(!strcmp(name, "End Marker"), "got name %s\n", debugstr_a(name));
}

static void test_performance_keys(void)
{
    static const HKEY keys[] = {HKEY_PERFORMANCE_DATA, HKEY_PERFORMANCE_TEXT, HKEY_PERFORMANCE_NLSTEXT};
    static const char *const names[] = {NULL, "", "Global", "2", "invalid counter name", "System"};
    DWORD size, type, sysname_len, expect_size, key_count, value_count;
    LARGE_INTEGER perftime1, perftime2, systime1, systime2, freq;
    WCHAR sysname[MAX_COMPUTERNAME_LENGTH + 1];
    unsigned int buffer_size = 1024 * 1024;
    void *buffer, *bufferW;
    PERF_DATA_BLOCK *data;
    union
    {
        FILETIME f;
        LONGLONG l;
    } file_time;
    unsigned int i, j;
    HKEY key;
    LONG ret;

#ifdef __REACTOS__
#ifdef _WIN64
    if (GetNTVersion() <= _WIN32_WINNT_VISTA) {
#else
    if (GetNTVersion() <= _WIN32_WINNT_WS03) {
#endif
        skip("test_performance_keys() is invalid for Vista x64 and WS03\n");
        return;
    }
#endif
    buffer = malloc(buffer_size);

    sysname_len = ARRAY_SIZE(sysname);
    GetComputerNameW(sysname, &sysname_len);

    for (i = 0; i < ARRAY_SIZE(keys); ++i)
    {
        winetest_push_context("key %p", keys[i]);

        for (j = 0; j < ARRAY_SIZE(names); ++j)
        {
            winetest_push_context("value %s", debugstr_a(names[j]));

            QueryPerformanceFrequency(&freq);

            size = 0;
            ret = RegQueryValueExA(keys[i], names[j], NULL, NULL, NULL, &size);
            ok(ret == ERROR_MORE_DATA, "got %lu\n", ret);
            ok(!size, "got size %lu\n", size);

            size = 10;
            ret = RegQueryValueExA(keys[i], names[j], NULL, NULL, buffer, &size);
            ok(ret == ERROR_MORE_DATA, "got %lu\n", ret);
            todo_wine
                ok(size == 10, "got size %lu\n", size);

            size = buffer_size;
            ret = RegQueryValueExA(keys[i], names[j], NULL, NULL, NULL, &size);
            ok(ret == ERROR_MORE_DATA, "got %lu\n", ret);

            QueryPerformanceCounter(&perftime1);
            NtQuerySystemTime(&systime1);

            size = buffer_size;
            type = 0xdeadbeef;
            ret = RegQueryValueExA(keys[i], names[j], NULL, &type, buffer, &size);
            ok(!ret, "got %lu\n", ret);
            ok(type == REG_BINARY, "got type %lu\n", type);
            ok(size >= sizeof(PERF_DATA_BLOCK) && size < buffer_size, "got size %lu\n", size);

            QueryPerformanceCounter(&perftime2);
            NtQuerySystemTime(&systime2);

            data = buffer;
            ok(!wcsncmp(data->Signature, L"PERF", 4), "got signature %s\n",
                    debugstr_wn(data->Signature, 4));
            ok(data->LittleEndian == 1, "got endianness %lu\n", data->LittleEndian);
            ok(data->Version == 1, "got version %lu\n", data->Version);
            ok(data->Revision == 1, "got version %lu\n", data->Revision);
            ok(data->TotalByteLength == size, "expected size %lu, got %lu\n",
                    size, data->TotalByteLength);

            expect_size = sizeof(PERF_DATA_BLOCK) + data->SystemNameLength;
            expect_size = (expect_size + 7) & ~7;

            ok(data->HeaderLength == expect_size, "expected header size %lu, got %lu\n",
                    expect_size, data->HeaderLength);
            /* todo: test objects... */
            todo_wine ok(data->DefaultObject == 238, "got default object %lu\n", data->DefaultObject);

            ok(data->PerfTime.QuadPart >= perftime1.QuadPart
                    && data->PerfTime.QuadPart <= perftime2.QuadPart,
                    "got times %I64d, %I64d, %I64d\n",
                    perftime1.QuadPart, data->PerfTime.QuadPart, perftime2.QuadPart);
            ok(data->PerfFreq.QuadPart == freq.QuadPart, "expected frequency %I64d, got %I64d\n",
                    freq.QuadPart, data->PerfFreq.QuadPart);
            ok(data->PerfTime100nSec.QuadPart >= systime1.QuadPart
                    && data->PerfTime100nSec.QuadPart <= systime2.QuadPart,
                    "got times %I64d, %I64d, %I64d\n",
                    systime1.QuadPart, data->PerfTime100nSec.QuadPart, systime2.QuadPart);
            SystemTimeToFileTime(&data->SystemTime, &file_time.f);
            /* SYSTEMTIME has a granularity of 1 ms */
            ok(file_time.l >= systime1.QuadPart - 10000 && file_time.l <= systime2.QuadPart,
                    "got times %I64d, %I64d, %I64d\n", systime1.QuadPart, file_time.l, systime2.QuadPart);

            ok(data->SystemNameLength == (sysname_len + 1) * sizeof(WCHAR), "got %lu\n", data->SystemNameLength);
            ok(data->SystemNameOffset == sizeof(PERF_DATA_BLOCK),
                    "got name offset %lu\n", data->SystemNameOffset);
            ok(!wcscmp(sysname, (const WCHAR *)(data + 1)), "expected name %s, got %s\n",
                    debugstr_w(sysname), debugstr_w((const WCHAR *)(data + 1)));

            winetest_pop_context();
        }

        /* test the "Counter" value */

        size = 0xdeadbeef;
        ret = RegQueryValueExA(keys[i], "cOuNtEr", NULL, NULL, NULL, &size);
        ok(!ret, "got %lu\n", ret);
        ok(size > 0 && size < 0xdeadbeef, "got size %lu\n", size);

        type = 0xdeadbeef;
        size = 0;
        ret = RegQueryValueExA(keys[i], "cOuNtEr", NULL, &type, buffer, &size);
        ok(ret == ERROR_MORE_DATA, "got %lu\n", ret);
        ok(size > 0, "got size %lu\n", size);

        type = 0xdeadbeef;
        size = buffer_size;
        ret = RegQueryValueExA(keys[i], "cOuNtEr", NULL, &type, buffer, &size);
        ok(!ret, "got %lu\n", ret);
        ok(type == REG_MULTI_SZ, "got type %lu\n", type);
        test_counter_values(buffer, keys[i]);

        type = 0xdeadbeef;
        size = buffer_size;
        ret = RegQueryValueExA(keys[i], "cOuNtErwine", NULL, &type, buffer, &size);
        ok(!ret, "got %lu\n", ret);
        ok(type == REG_MULTI_SZ, "got type %lu\n", type);
        test_counter_values(buffer, keys[i]);

        size = 0;
        ret = RegQueryValueExW(keys[i], L"cOuNtEr", NULL, NULL, NULL, &size);
        ok(!ret, "got %lu\n", ret);
        ok(size > 0, "got size %lu\n", size);

        bufferW = malloc(size);

        type = 0xdeadbeef;
        ret = RegQueryValueExW(keys[i], L"cOuNtEr", NULL, &type, bufferW, &size);
        ok(!ret, "got %lu\n", ret);
        ok(type == REG_MULTI_SZ, "got type %lu\n", type);
        WideCharToMultiByte(CP_ACP, 0, bufferW, size / sizeof(WCHAR), buffer, buffer_size, NULL, NULL);
        test_counter_values(buffer, keys[i]);

        /* test the "Help" value */

        size = 0xdeadbeef;
        ret = RegQueryValueExA(keys[i], "hElP", NULL, NULL, NULL, &size);
        ok(!ret, "got %lu\n", ret);
        ok(size > 0 && size < 0xdeadbeef, "got size %lu\n", size);

        type = 0xdeadbeef;
        size = 0;
        ret = RegQueryValueExA(keys[i], "hElP", NULL, &type, buffer, &size);
        ok(ret == ERROR_MORE_DATA, "got %lu\n", ret);
        ok(size > 0, "got size %lu\n", size);

        type = 0xdeadbeef;
        size = buffer_size;
        ret = RegQueryValueExA(keys[i], "hElP", NULL, &type, buffer, &size);
        test_help_values(buffer, keys[i]);

        type = 0xdeadbeef;
        size = buffer_size;
        ret = RegQueryValueExA(keys[i], "hElPwine", NULL, &type, buffer, &size);
        ok(!ret, "got %lu\n", ret);
        ok(type == REG_MULTI_SZ, "got type %lu\n", type);
        test_help_values(buffer, keys[i]);

        size = 0;
        ret = RegQueryValueExW(keys[i], L"hElP", NULL, NULL, NULL, &size);
        ok(!ret, "got %lu\n", ret);
        ok(size > 0, "got size %lu\n", size);

        bufferW = malloc(size);

        type = 0xdeadbeef;
        ret = RegQueryValueExW(keys[i], L"hElP", NULL, &type, bufferW, &size);
        ok(!ret, "got %lu\n", ret);
        ok(type == REG_MULTI_SZ, "got type %lu\n", type);
        WideCharToMultiByte(CP_ACP, 0, bufferW, size / sizeof(WCHAR), buffer, buffer_size, NULL, NULL);
        test_help_values(buffer, keys[i]);

        /* test other registry APIs */

        ret = RegOpenKeyA(keys[i], NULL, &key);
        todo_wine ok(ret == ERROR_INVALID_HANDLE, "got %lu\n", ret);

        ret = RegOpenKeyA(keys[i], "Global", &key);
        ok(ret == ERROR_INVALID_HANDLE, "got %lu\n", ret);

        ret = RegOpenKeyExA(keys[i], "Global", 0, KEY_READ, &key);
        ok(ret == ERROR_INVALID_HANDLE, "got %lu\n", ret);

        size = 0;
        ret = RegQueryValueA(keys[i], "Global", NULL, (LONG *)&size);
        ok(ret == ERROR_INVALID_HANDLE, "got %lu\n", ret);

        ret = RegSetValueA(keys[i], "Global", REG_SZ, "dummy", 5);
        ok(ret == ERROR_INVALID_HANDLE, "got %lu\n", ret);

        key_count = 0x900ddeed;
        ret = RegQueryInfoKeyA(keys[i], NULL, NULL, NULL, &key_count, NULL,
                NULL, &value_count, NULL, NULL, NULL, NULL);
        todo_wine ok(!ret, "got %lu\n", ret);
        todo_wine ok(!key_count, "got %lu subkeys\n", key_count);
        todo_wine ok(value_count == 2, "got %lu values\n", value_count);

        size = buffer_size;
        ret = RegEnumValueA(keys[i], 0, buffer, &size, NULL, NULL, NULL, NULL);
        todo_wine ok(ret == ERROR_MORE_DATA, "got %lu\n", ret);
        ok(size == buffer_size, "got size %lu\n", size);

        ret = RegCloseKey(keys[i]);
        ok(!ret, "got %lu\n", ret);

        ret = RegCloseKey(keys[i]);
        ok(!ret, "got %lu\n", ret);

        winetest_pop_context();
    }

    ret = RegSetValueExA(HKEY_PERFORMANCE_DATA, "Global", 0, REG_SZ, (const BYTE *)"dummy", 5);
    ok(ret == ERROR_INVALID_HANDLE, "got %lu\n", ret);

    ret = RegSetValueExA(HKEY_PERFORMANCE_TEXT, "Global", 0, REG_SZ, (const BYTE *)"dummy", 5);
    todo_wine ok(ret == ERROR_BADKEY, "got %lu\n", ret);

    ret = RegSetValueExA(HKEY_PERFORMANCE_NLSTEXT, "Global", 0, REG_SZ, (const BYTE *)"dummy", 5);
    todo_wine ok(ret == ERROR_BADKEY, "got %lu\n", ret);

    if (pRegSetKeyValueW)
    {
        ret = pRegSetKeyValueW(HKEY_PERFORMANCE_DATA, NULL, L"Global", REG_SZ, L"dummy", 10);
        ok(ret == ERROR_INVALID_HANDLE, "got %lu\n", ret);

        ret = pRegSetKeyValueW(HKEY_PERFORMANCE_TEXT, NULL, L"Global", REG_SZ, L"dummy", 10);
        todo_wine ok(ret == ERROR_BADKEY, "got %lu\n", ret);

        ret = pRegSetKeyValueW(HKEY_PERFORMANCE_NLSTEXT, NULL, L"Global", REG_SZ, L"dummy", 10);
        todo_wine ok(ret == ERROR_BADKEY, "got %lu\n", ret);
    }

    ret = RegEnumKeyA(HKEY_PERFORMANCE_DATA, 0, buffer, buffer_size);
    ok(ret == ERROR_INVALID_HANDLE, "got %lu\n", ret);

    ret = RegEnumKeyA(HKEY_PERFORMANCE_TEXT, 0, buffer, buffer_size);
    todo_wine ok(ret == ERROR_NO_MORE_ITEMS, "got %lu\n", ret);

    ret = RegEnumKeyA(HKEY_PERFORMANCE_NLSTEXT, 0, buffer, buffer_size);
    todo_wine ok(ret == ERROR_NO_MORE_ITEMS, "got %lu\n", ret);

    free(buffer);
}

static void test_perflib_key(void)
{
    unsigned int primary_lang = PRIMARYLANGID(GetUserDefaultLangID());
    unsigned int buffer_size = 1024 * 1024;
    OBJECT_NAME_INFORMATION *name_info;
    HKEY perflib_key, key, key2;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING string;
    char lang_name[5];
    const char *knames[2] = {"009", "CurrentLanguage"};
    char *buffer;
    DWORD size;
    LONG ret, l;

    ret = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
            "Software\\Microsoft\\Windows NT\\CurrentVersion\\Perflib", 0, KEY_READ, &perflib_key);
    ok(!ret, "got %lu\n", ret);

    ret = RegOpenKeyExA(perflib_key, "009", 0, KEY_READ, &key);
    ok(!ret, "got %lu\n", ret);
    /* English always returns TEXT; most other languages return NLSTEXT, but
     * some (e.g. Hindi) return TEXT */
    ok(key == HKEY_PERFORMANCE_TEXT || key == HKEY_PERFORMANCE_NLSTEXT, "got key %p\n", key);

    ret = RegCloseKey(key);
    ok(!ret, "got %lu\n", ret);

    RtlInitUnicodeString(&string, L"009");
    InitializeObjectAttributes(&attr, &string, OBJ_CASE_INSENSITIVE, perflib_key, NULL);
    ret = NtOpenKey((HANDLE *)&key, KEY_ALL_ACCESS, &attr);
    ok(ret == STATUS_PREDEFINED_HANDLE || ret == STATUS_ACCESS_DENIED
            || ret == STATUS_SUCCESS /* Win < 7 */, "got %#lx\n", ret);
    if (ret == STATUS_PREDEFINED_HANDLE)
        ok(!is_special_key(key), "expected a normal handle, got %p\n", key);
    else if (ret == STATUS_SUCCESS)
        ok(key == HKEY_PERFORMANCE_TEXT, "got key %p\n", key);
    else
    {
        skip("Not enough permissions to test the perflib key.\n");
        RegCloseKey(perflib_key);
        return;
    }

    buffer = malloc(buffer_size);

    ret = NtQueryKey(key, KeyFullInformation, buffer, buffer_size, &size);
    ok(ret == STATUS_INVALID_HANDLE, "got %#lx\n", ret);

    ret = NtEnumerateKey(key, 0, KeyFullInformation, buffer, buffer_size, &size);
    ok(ret == STATUS_INVALID_HANDLE, "got %#lx\n", ret);

    RtlInitUnicodeString(&string, L"counter");
    ret = NtQueryValueKey(key, &string, KeyValuePartialInformation, buffer, buffer_size, &size);
    ok(ret == STATUS_INVALID_HANDLE, "got %#lx\n", ret);

    ret = NtEnumerateValueKey(key, 0, KeyValuePartialInformation, buffer, buffer_size, &size);
    ok(ret == STATUS_INVALID_HANDLE, "got %#lx\n", ret);

    ret = NtSetValueKey(key, &string, 0, REG_SZ, "test", 5);
    ok(ret == STATUS_INVALID_HANDLE, "got %#lx\n", ret);

    ret = NtDeleteValueKey(key, &string);
    ok(ret == STATUS_INVALID_HANDLE, "got %#lx\n", ret);

    ret = NtDeleteKey(key);
    ok(ret == STATUS_INVALID_HANDLE, "got %#lx\n", ret);

    RtlInitUnicodeString(&string, L"subkey");
    InitializeObjectAttributes(&attr, &string, OBJ_CASE_INSENSITIVE, key, NULL);
    ret = NtOpenKey((HANDLE *)&key2, KEY_READ, &attr);
    if (is_special_key(key))
        ok(ret == STATUS_INVALID_HANDLE, "got %#lx\n", ret);
    else
        ok(ret == STATUS_OBJECT_NAME_NOT_FOUND
                || broken(ret == STATUS_INVALID_HANDLE) /* WoW64 */, "got %#lx\n", ret);

    ret = NtCreateKey((HANDLE *)&key2, KEY_ALL_ACCESS, &attr, 0, NULL, 0, NULL);
    if (is_special_key(key))
        ok(ret == STATUS_INVALID_HANDLE, "got %#lx\n", ret);
    else
        ok(!ret || broken(ret == STATUS_ACCESS_DENIED) /* w8adm */
                || broken(ret == STATUS_INVALID_HANDLE) /* WoW64 */, "got %#lx\n", ret);
    if (!ret)
    {
        NtDeleteKey(key2);
        NtClose(key2);
    }

    /* it's a real handle, though */
    ret = NtQueryObject(key, ObjectNameInformation, buffer, buffer_size, &size);
    if (is_special_key(key))
        ok(ret == STATUS_INVALID_HANDLE, "got %#lx\n", ret);
    else
        ok(!ret, "got %#lx\n", ret);
    if (!ret)
    {
        name_info = (OBJECT_NAME_INFORMATION *)buffer;
        ok(!wcsicmp(name_info->Name.Buffer, L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\Windows NT"
                "\\CurrentVersion\\Perflib\\009"), "got name %s\n", debugstr_w(name_info->Name.Buffer));
    }

    ret = NtClose(key);
    if (is_special_key(key))
        ok(ret == STATUS_INVALID_HANDLE, "got %#lx\n", ret);
    else
        ok(!ret, "got %#lx\n", ret);

    for (l = 0; l < ARRAY_SIZE(knames); l++)
    {
        winetest_push_context("%ld", l);
      todo_wine_if(l == 1) {
        ret = RegOpenKeyExA(perflib_key, knames[l], 0, KEY_READ, &key);
#ifdef __REACTOS__
        if (ret == ERROR_FILE_NOT_FOUND) /* WS03, Vista */
            continue;
#endif
        ok(!ret, "got %lu\n", ret);
        if (is_special_key(key))
        {
            size = buffer_size;
            ret = RegQueryValueExA(key, "counter", NULL, NULL, (BYTE *)buffer, &size);
            ok(!ret, "got %lu\n", ret);
            if (!ret)
            {
                char *str;
                int c = 0;
                for (str = buffer; *str; str += strlen(str) + 1)
                    c++;
                /* Note that the two keys may not have the same number of
                 * entries if they are in different languages.
                 */
                ok(c >= 2 && (c % 2) == 0, "%d is not a valid number of entries in %s\n", c, knames[l]);
                trace("%s has %d entries\n", knames[l], c);
            }
        }
        else
        {
            /* Windows 7 does not always return a special key for 009
             * when running without elevated privileges.
             */
            ok(broken(l == 0), "expected a special handle, got %p\n", key);
        }

        ret = RegCloseKey(key);
        ok(!ret, "got %lu\n", ret);
      }
        winetest_pop_context();
    }

    /* multilingual support was not really completely thought through */
    switch (primary_lang)
    {
    case LANG_PORTUGUESE:
    case LANG_CHINESE:
        sprintf(lang_name, "%04x", GetUserDefaultLangID());
        break;
    default:
        sprintf(lang_name, "%03x", primary_lang);
        break;
    }
    if (primary_lang != LANG_ENGLISH &&
        !RegOpenKeyExA(perflib_key, lang_name, 0, KEY_READ, &key))
    {
        ok(!is_special_key(key), "expected a normal handle, got %p\n", key);

        size = buffer_size;
        ret = RegQueryValueExA(key, "counter", NULL, NULL, (BYTE *)buffer, &size);
        todo_wine ok(ret == ERROR_FILE_NOT_FOUND, "got %lu\n", ret);

        ret = RegCloseKey(key);
        todo_wine ok(!ret, "got %lu\n", ret);
    }
    /* else some languages don't have their own key. The keys are not really
     * usable anyway so assume it does not really matter.
     */

    ret = RegCloseKey(perflib_key);
    ok(!ret, "got %lu\n", ret);

    RtlInitUnicodeString(&string, L"\\Registry\\PerfData");
    InitializeObjectAttributes(&attr, &string, OBJ_CASE_INSENSITIVE, NULL, NULL);
    ret = NtOpenKey((HANDLE *)&key, KEY_READ, &attr);
    ok(ret == STATUS_OBJECT_NAME_NOT_FOUND, "got %#lx\n", ret);

    free(buffer);
}

static void test_RegLoadMUIString(void)
{
    HMODULE hUser32, hResDll, hFile;
    int (WINAPI *pLoadStringW)(HMODULE, UINT, WCHAR *, int);
    LONG ret;
    HKEY hkey;
    DWORD type, size, text_size;
    UINT i;
    char buf[64], *p, sysdir[MAX_PATH];
    char with_env_var[128], filename[MAX_PATH], tmp_path[MAX_PATH];
    WCHAR textW[64], bufW[64];
    WCHAR curdirW[MAX_PATH], sysdirW[MAX_PATH];
    const static char tz_value[] = "MUI_Std";
    const static WCHAR tz_valueW[] = L"MUI_Std";
    struct {
        const char* value;
        DWORD type;
        BOOL use_sysdir;
        DWORD expected;
        DWORD broken_ret;
    } test_case[] = {
        /* 0 */
        { "",                  REG_SZ,        FALSE, ERROR_INVALID_DATA },
        { "not a MUI string",  REG_SZ,        FALSE, ERROR_INVALID_DATA },
        { "@unknown.dll",      REG_SZ,        TRUE,  ERROR_INVALID_DATA },
        { "@unknown.dll,-10",  REG_SZ,        TRUE,  ERROR_FILE_NOT_FOUND },
        /*  4 */
        { with_env_var,        REG_SZ,        FALSE, ERROR_SUCCESS },
        { with_env_var,        REG_EXPAND_SZ, FALSE, ERROR_SUCCESS },
        { "%WineMuiTest1%",    REG_EXPAND_SZ, TRUE,  ERROR_INVALID_DATA },
        { "@%WineMuiTest2%",   REG_EXPAND_SZ, TRUE,  ERROR_SUCCESS },
        /*  8 */
        { "@%WineMuiExe%,a",   REG_SZ,        FALSE, ERROR_INVALID_DATA },
        { "@%WineMuiExe%,-4",  REG_SZ,        FALSE, ERROR_NOT_FOUND, ERROR_FILE_NOT_FOUND },
#ifdef __REACTOS__
        { "@%WineMuiExe%,-39", REG_SZ,        FALSE, ERROR_RESOURCE_NAME_NOT_FOUND, ERROR_RESOURCE_DATA_NOT_FOUND },
#else
        { "@%WineMuiExe%,-39", REG_SZ,        FALSE, ERROR_RESOURCE_NAME_NOT_FOUND },
#endif
        { "@%WineMuiDat%,-16", REG_EXPAND_SZ, FALSE, ERROR_BAD_EXE_FORMAT, ERROR_FILE_NOT_FOUND },
    };

    if (!pRegLoadMUIStringA || !pRegLoadMUIStringW)
    {
        win_skip("RegLoadMUIString is not available\n");
        return;
    }
#if defined(__REACTOS__) && defined(_WIN64)
    if (GetNTVersion() == _WIN32_WINNT_VISTA) {
        skip("test_RegLoadMUIString() spams the console with garbage and crashes on Vista x64.\n");
        return;
    }
#endif

    hUser32 = LoadLibraryA("user32.dll");
    ok(hUser32 != NULL, "cannot load user32.dll\n");
    pLoadStringW = (void *)GetProcAddress(hUser32, "LoadStringW");
    ok(pLoadStringW != NULL, "failed to get LoadStringW address\n");

    ret = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                        "Software\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones\\UTC", 0,
                        KEY_READ, &hkey);
    ok(ret == ERROR_SUCCESS, "got %ld\n", ret);

    size = ARRAY_SIZE(buf);
    ret = RegQueryValueExA(hkey, tz_value, NULL, &type, (BYTE *)buf, &size);
    ok(ret == ERROR_SUCCESS, "got %ld\n", ret);
    ok(buf[0] == '@', "got %s\n", buf);

    /* setup MUI string for tests */
    strcpy(with_env_var, "@%windir%\\system32\\");
    strcat(with_env_var, &buf[1]);
    SetEnvironmentVariableA("WineMuiTest1", buf);
    SetEnvironmentVariableA("WineMuiTest2", &buf[1]);

    /* load expecting text */
    p = strrchr(buf, ',');
    *p = '\0';
    i = atoi(p + 2); /* skip ',-' */
    hResDll = LoadLibraryExA(&buf[1], NULL, LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE);
    memset(textW, 0xaa, sizeof(textW));
    ret = pLoadStringW(hResDll, i, textW, ARRAY_SIZE(textW));
    ok(ret > 0, "failed to load string resource\n");
    text_size = (ret + 1) * sizeof(WCHAR);
    FreeLibrary(hResDll);
    FreeLibrary(hUser32);

    ret = GetSystemDirectoryW(sysdirW, ARRAY_SIZE(sysdirW));
    ok(ret > 0, "GetSystemDirectoryW failed\n");
    ret = GetSystemDirectoryA(sysdir, ARRAY_SIZE(sysdir));
    ok(ret > 0, "GetSystemDirectoryA failed\n");

    /* change the current directory to system32 */
    GetCurrentDirectoryW(ARRAY_SIZE(curdirW), curdirW);
    SetCurrentDirectoryW(sysdirW);

    size = 0xdeadbeef;
    ret = pRegLoadMUIStringW(hkey, tz_valueW, NULL, 0, &size, 0, NULL);
    ok(ret == ERROR_MORE_DATA, "got %ld, expected ERROR_MORE_DATA\n", ret);
#ifdef __REACTOS__
    ok(size == text_size || broken(size == text_size + sizeof(WCHAR)) /* Vista */, "got %lu, expected %lu\n", size, text_size);
#else
    ok(size == text_size, "got %lu, expected %lu\n", size, text_size);
#endif

    memset(bufW, 0xff, sizeof(bufW));
    ret = pRegLoadMUIStringW(hkey, tz_valueW, bufW, sizeof(WCHAR)+1, &size, 0, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "got %ld, expected ERROR_INVALID_PARAMETER\n", ret);
    ok(bufW[0] == 0xffff, "got 0x%04x, expected 0xffff\n", bufW[0]);

    size = 0xdeadbeef;
    memset(bufW, 0xff, sizeof(bufW));
    ret = pRegLoadMUIStringW(hkey, tz_valueW, bufW, sizeof(WCHAR)*2, &size, 0, NULL);
    ok(ret == ERROR_MORE_DATA, "got %ld, expected ERROR_MORE_DATA\n", ret);
    ok(size == text_size || broken(size == text_size + sizeof(WCHAR) /* vista */),
       "got %lu, expected %lu\n", size, text_size);
    ok(bufW[0] == 0xffff, "got 0x%04x, expected 0xffff\n", bufW[0]);

    memset(bufW, 0xff, sizeof(bufW));
    ret = pRegLoadMUIStringW(hkey, tz_valueW, bufW, sizeof(WCHAR)*2, &size, REG_MUI_STRING_TRUNCATE, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "got %ld, expected ERROR_INVALID_PARAMETER\n", ret);
    ok(bufW[0] == 0xffff, "got 0x%04x, expected 0xffff\n", bufW[0]);

    memset(bufW, 0xff, sizeof(bufW));
    ret = pRegLoadMUIStringW(hkey, tz_valueW, bufW, sizeof(WCHAR)*2, NULL, 0xdeadbeef, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "got %ld, expected ERROR_INVALID_PARAMETER\n", ret);
    ok(bufW[0] == 0xffff, "got 0x%04x, expected 0xffff\n", bufW[0]);

    memset(bufW, 0xff, sizeof(bufW));
    ret = pRegLoadMUIStringW(hkey, tz_valueW, bufW, sizeof(WCHAR)*2, NULL, REG_MUI_STRING_TRUNCATE, NULL);
    ok(ret == ERROR_SUCCESS, "got %ld, expected ERROR_SUCCESS\n", ret);
    ok(bufW[0] == textW[0], "got 0x%04x, expected 0x%04x\n", bufW[0], textW[0]);
    ok(bufW[1] == 0, "got 0x%04x, expected nul\n", bufW[1]);

    size = 0xdeadbeef;
    memset(bufW, 0xff, sizeof(bufW));
    ret = pRegLoadMUIStringW(hkey, tz_valueW, bufW, ARRAY_SIZE(bufW), &size, 0, NULL);
    ok(ret == ERROR_SUCCESS, "got %ld, expected ERROR_SUCCESS\n", ret);
    ok(size == text_size, "got %lu, expected %lu\n", size, text_size);
    ok(!memcmp(textW, bufW, size), "got %s, expected %s\n",
       wine_dbgstr_wn(bufW, size / sizeof(WCHAR)), wine_dbgstr_wn(textW, text_size / sizeof(WCHAR)));

    ret = pRegLoadMUIStringA(hkey, tz_value, buf, ARRAY_SIZE(buf), &size, 0, NULL);
    ok(ret == ERROR_CALL_NOT_IMPLEMENTED, "got %ld, expected ERROR_CALL_NOT_IMPLEMENTED\n", ret);

    /* change the current directory to other than system32 directory */
    SetCurrentDirectoryA("\\");

    size = 0xdeadbeef;
    memset(bufW, 0xff, sizeof(bufW));
    ret = pRegLoadMUIStringW(hkey, tz_valueW, bufW, ARRAY_SIZE(bufW), &size, 0, sysdirW);
    ok(ret == ERROR_SUCCESS, "got %ld, expected ERROR_SUCCESS\n", ret);
    ok(size == text_size, "got %lu, expected %lu\n", size, text_size);
    ok(!memcmp(textW, bufW, size), "got %s, expected %s\n",
       wine_dbgstr_wn(bufW, size / sizeof(WCHAR)), wine_dbgstr_wn(textW, text_size / sizeof(WCHAR)));

    ret = pRegLoadMUIStringA(hkey, tz_value, buf, ARRAY_SIZE(buf), &size, 0, sysdir);
    ok(ret == ERROR_CALL_NOT_IMPLEMENTED, "got %ld, expected ERROR_CALL_NOT_IMPLEMENTED\n", ret);

    ret = pRegLoadMUIStringW(hkey, tz_valueW, bufW, ARRAY_SIZE(bufW), &size, 0, NULL);
    ok(ret == ERROR_FILE_NOT_FOUND, "got %ld, expected ERROR_FILE_NOT_FOUND\n", ret);

    ret = pRegLoadMUIStringA(hkey, tz_value, buf, ARRAY_SIZE(buf), &size, 0, NULL);
    ok(ret == ERROR_CALL_NOT_IMPLEMENTED, "got %ld, expected ERROR_CALL_NOT_IMPLEMENTED\n", ret);

    RegCloseKey(hkey);

    GetModuleFileNameA(NULL, filename, ARRAY_SIZE(filename));
    SetEnvironmentVariableA("WineMuiExe", filename);

    GetTempPathA(ARRAY_SIZE(tmp_path), tmp_path);
    GetTempFileNameA(tmp_path, "mui", 0, filename);
    SetEnvironmentVariableA("WineMuiDat", filename);

    /* write dummy data to the file, i.e. it's not a PE file. */
    hFile = CreateFileA(filename, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    ok(hFile != INVALID_HANDLE_VALUE, "can't open %s\n", filename);
    WriteFile(hFile, filename, strlen(filename), &size, NULL);
    CloseHandle(hFile);

    for (i = 0; i < ARRAY_SIZE(test_case); i++)
    {
        size = test_case[i].value ? strlen(test_case[i].value) + 1 : 0;
        ret = RegSetValueExA(hkey_main, tz_value, 0, test_case[i].type,
                             (const BYTE *)test_case[i].value, size);
        ok(ret == ERROR_SUCCESS, "[%2u] got %ld\n", i, ret);

        size = 0xdeadbeef;
        memset(bufW, 0xff, sizeof(bufW));
        ret = pRegLoadMUIStringW(hkey_main, tz_valueW, bufW, ARRAY_SIZE(bufW),
                                 &size, 0,
                                 test_case[i].use_sysdir ? sysdirW : NULL);
        ok(ret == test_case[i].expected ||
#ifdef __REACTOS__
           broken(i == 9 && ret == ERROR_RESOURCE_DATA_NOT_FOUND /* Vista */) ||
#endif
           broken(test_case[i].value[0] == '%' && ret == ERROR_SUCCESS /* vista */) ||
           broken(test_case[i].broken_ret && ret == test_case[i].broken_ret /* vista */),
           "[%2u] expected %ld, got %ld\n", i, test_case[i].expected, ret);
        if (ret == ERROR_SUCCESS && test_case[i].expected == ERROR_SUCCESS)
        {
            ok(size == text_size, "[%2u] got %lu, expected %lu\n", i, size, text_size);
            ok(!memcmp(bufW, textW, size), "[%2u] got %s, expected %s\n", i,
               wine_dbgstr_wn(bufW, size/sizeof(WCHAR)),
               wine_dbgstr_wn(textW, text_size/sizeof(WCHAR)));
        }
    }

    SetCurrentDirectoryW(curdirW);
    DeleteFileA(filename);
    SetEnvironmentVariableA("WineMuiTest1", NULL);
    SetEnvironmentVariableA("WineMuiTest2", NULL);
    SetEnvironmentVariableA("WineMuiExe", NULL);
    SetEnvironmentVariableA("WineMuiDat", NULL);
}

static void test_EnumDynamicTimeZoneInformation(void)
{
    LSTATUS status;
    HKEY key, subkey;
    WCHAR name[128];
    WCHAR keyname[128];
    WCHAR sysdir[MAX_PATH];
    DWORD index, ret, gle, size;
    DYNAMIC_TIME_ZONE_INFORMATION bogus_dtzi, dtzi;
    struct tz_reg_data
    {
        LONG bias;
        LONG std_bias;
        LONG dlt_bias;
        SYSTEMTIME std_date;
        SYSTEMTIME dlt_date;
    } tz_data;

    if (!pEnumDynamicTimeZoneInformation)
    {
        win_skip("EnumDynamicTimeZoneInformation is not supported.\n");
        return;
    }

    if (pRegLoadMUIStringW)
        GetSystemDirectoryW(sysdir, ARRAY_SIZE(sysdir));

    SetLastError(0xdeadbeef);
    ret = pEnumDynamicTimeZoneInformation(0, NULL);
    gle = GetLastError();
    ok(gle == 0xdeadbeef, "got 0x%lx\n", gle);
    ok(ret == ERROR_INVALID_PARAMETER, "got %ld\n", ret);

    memset(&bogus_dtzi, 0xcc, sizeof(bogus_dtzi));
    memset(&dtzi, 0xcc, sizeof(dtzi));
    SetLastError(0xdeadbeef);
    ret = pEnumDynamicTimeZoneInformation(-1, &dtzi);
    gle = GetLastError();
    ok(gle == 0xdeadbeef, "got 0x%lx\n", gle);
    ok(ret == ERROR_NO_MORE_ITEMS, "got %ld\n", ret);
    ok(!memcmp(&dtzi, &bogus_dtzi, sizeof(dtzi)), "mismatch\n");

    status = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
            "Software\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones", 0,
            KEY_ENUMERATE_SUB_KEYS|KEY_QUERY_VALUE, &key);
    ok(status == ERROR_SUCCESS, "got %ld\n", status);
    index = 0;
    while (!(status = RegEnumKeyW(key, index, keyname, ARRAY_SIZE(keyname))))
    {
        winetest_push_context("%s" , wine_dbgstr_w(keyname));
        subkey = NULL;
        status = RegOpenKeyExW(key, keyname, 0, KEY_QUERY_VALUE, &subkey);
        ok(status == ERROR_SUCCESS, "got %ld\n", status);

        memset(&dtzi, 0xcc, sizeof(dtzi));
        SetLastError(0xdeadbeef);
        ret = pEnumDynamicTimeZoneInformation(index, &dtzi);
        gle = GetLastError();
        /* recently added time zones may not have MUI strings */
        ok(gle == ERROR_SUCCESS ||
           gle == ERROR_RESOURCE_TYPE_NOT_FOUND /* Win10 1809 32-bit */ ||
           gle == ERROR_MUI_FILE_NOT_FOUND /* Win10 1809 64-bit */,
            "got 0x%lx\n", gle);
        ok(ret == ERROR_SUCCESS, "got %ld\n", ret);
        ok(!lstrcmpW(dtzi.TimeZoneKeyName, keyname), "expected %s, got %s\n",
            wine_dbgstr_w(keyname), wine_dbgstr_w(dtzi.TimeZoneKeyName));

        if (gle == ERROR_SUCCESS)
        {
            size = sizeof(name);
            memset(name, 0, sizeof(name));
            status = pRegGetValueW(subkey, NULL, L"Std", RRF_RT_REG_SZ, NULL, name, &size);
            ok(status == ERROR_SUCCESS, "status %ld Std %s\n", status,
               wine_dbgstr_w(name));
            ok(*name, "Std name is empty\n");
            if (pRegLoadMUIStringW)
            {
                size = sizeof(name);
                memset(name, 0, sizeof(name));
                status = pRegLoadMUIStringW(subkey, L"MUI_Std", name, size, &size, 0, sysdir);
                ok(status == ERROR_SUCCESS, "status %ld MUI_Std %s\n",
                   status, wine_dbgstr_w(name));
                ok(*name, "MUI_Std name is empty\n");
            }
            ok(!memcmp(&dtzi.StandardName, name, size), "expected %s, got %s\n",
               wine_dbgstr_w(name), wine_dbgstr_w(dtzi.StandardName));

            size = sizeof(name);
            memset(name, 0, sizeof(name));
            status = pRegGetValueW(subkey, NULL, L"Dlt", RRF_RT_REG_SZ, NULL, name, &size);
            ok(status == ERROR_SUCCESS, "status %ld %s Dlt %s\n",
               status, wine_dbgstr_w(keyname), wine_dbgstr_w(name));
            ok(*name, "Dlt name is empty\n");
            if (pRegLoadMUIStringW)
            {
                size = sizeof(name);
                memset(name, 0, sizeof(name));
                status = pRegLoadMUIStringW(subkey, L"MUI_Dlt", name, size, &size, 0, sysdir);
                ok(status == ERROR_SUCCESS, "status %ld %s MUI_Dlt %s\n",
                   status, wine_dbgstr_w(keyname), wine_dbgstr_w(name));
                ok(*name, "MUI_Dlt name is empty\n");
            }
            ok(!memcmp(&dtzi.DaylightName, name, size), "expected %s, got %s\n",
               wine_dbgstr_w(name), wine_dbgstr_w(dtzi.DaylightName));

            size = sizeof(name);
            memset(name, 0, sizeof(name));
            status = pRegGetValueW(subkey, NULL, L"Display", RRF_RT_REG_SZ, NULL, name, &size);
            ok(status == ERROR_SUCCESS, "status %ld %s Display %s\n",
               status, wine_dbgstr_w(keyname), wine_dbgstr_w(name));
            ok(*name, "Display name is empty\n");
            if (pRegLoadMUIStringW)
            {
                size = sizeof(name);
                memset(name, 0, sizeof(name));
                status = pRegLoadMUIStringW(subkey, L"MUI_Display", name, size, &size, 0, sysdir);
                /* recently added time zones may not have MUI strings */
                ok((status == ERROR_SUCCESS && *name) ||
                   broken(status == ERROR_RESOURCE_TYPE_NOT_FOUND) /* Win10 1809 32-bit */ ||
                   broken(status == ERROR_MUI_FILE_NOT_FOUND) /* Win10 1809 64-bit */,
                   "status %ld MUI_Display %s\n", status, wine_dbgstr_w(name));
            }
        }
        else
        {
            ok(!dtzi.StandardName[0], "expected empty StandardName\n");
            ok(!dtzi.DaylightName[0], "expected empty DaylightName\n");
        }

        ok(!dtzi.DynamicDaylightTimeDisabled, "got %d\n", dtzi.DynamicDaylightTimeDisabled);

        size = sizeof(tz_data);
        status = pRegGetValueW(key, keyname, L"TZI", RRF_RT_REG_BINARY, NULL, &tz_data, &size);
        ok(status == ERROR_SUCCESS, "got %ld\n", status);

        ok(dtzi.Bias == tz_data.bias, "expected %ld, got %ld\n",
            tz_data.bias, dtzi.Bias);
        ok(dtzi.StandardBias == tz_data.std_bias, "expected %ld, got %ld\n",
            tz_data.std_bias, dtzi.StandardBias);
        ok(dtzi.DaylightBias == tz_data.dlt_bias, "expected %ld, got %ld\n",
            tz_data.dlt_bias, dtzi.DaylightBias);

        ok(!memcmp(&dtzi.StandardDate, &tz_data.std_date, sizeof(dtzi.StandardDate)),
            "expected %s, got %s\n",
            dbgstr_SYSTEMTIME(&tz_data.std_date), dbgstr_SYSTEMTIME(&dtzi.StandardDate));

        ok(!memcmp(&dtzi.DaylightDate, &tz_data.dlt_date, sizeof(dtzi.DaylightDate)),
            "expected %s, got %s\n",
            dbgstr_SYSTEMTIME(&tz_data.dlt_date), dbgstr_SYSTEMTIME(&dtzi.DaylightDate));

        winetest_pop_context();
        RegCloseKey(subkey);
        index++;
    }
    ok(status == ERROR_NO_MORE_ITEMS, "got %ld\n", status);

    memset(&dtzi, 0xcc, sizeof(dtzi));
    SetLastError(0xdeadbeef);
    ret = pEnumDynamicTimeZoneInformation(index, &dtzi);
    gle = GetLastError();
    ok(gle == 0xdeadbeef, "got 0x%lx\n", gle);
    ok(ret == ERROR_NO_MORE_ITEMS, "got %ld\n", ret);
    ok(!memcmp(&dtzi, &bogus_dtzi, sizeof(dtzi)), "mismatch\n");

    RegCloseKey(key);
}

static void test_RegRenameKey(void)
{
#if defined(__REACTOS__) && DLL_EXPORT_VERSION < 0x600
    skip("test_RegRenameKey() can't be built unless DLL_EXPORT_VERSION >= 0x600\n");
#else
    HKEY key, key2;
    LSTATUS ret;

    ret = RegRenameKey(NULL, NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "Unexpected return value %ld.\n", ret);
    ret = RegRenameKey(NULL, NULL, L"newname");
    ok(ret == ERROR_INVALID_HANDLE, "Unexpected return value %ld.\n", ret);
    ret = RegRenameKey(NULL, L"oldname", NULL);
    ok(ret == ERROR_INVALID_HANDLE, "Unexpected return value %ld.\n", ret);
    ret = RegRenameKey(NULL, L"oldname", L"newname");
    ok(ret == ERROR_INVALID_HANDLE, "Unexpected return value %ld.\n", ret);

    ret = RegCreateKeyExA(hkey_main, "TestRenameKey", 0, NULL, 0, KEY_READ, NULL, &key, NULL);
    ok(!ret, "Unexpected return value %ld.\n", ret);
    ret = RegRenameKey(key, NULL, L"TestNewRenameKey");
    ok(ret == ERROR_ACCESS_DENIED, "Unexpected return value %ld.\n", ret);
    RegCloseKey(key);

    /* Rename itself. */
    ret = RegCreateKeyExA(hkey_main, "TestRenameKey", 0, NULL, 0, KEY_WRITE, NULL, &key, NULL);
    ok(!ret, "Unexpected return value %ld.\n", ret);
    ret = RegRenameKey(key, NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "Unexpected return value %ld.\n", ret);
    ret = RegRenameKey(key, NULL, L"TestNewRenameKey");
    ok(!ret, "Unexpected return value %ld.\n", ret);
    RegCloseKey(key);

    ret = RegDeleteKeyA(hkey_main, "TestNewRenameKey");
    ok(!ret, "Unexpected return value %ld.\n", ret);
    ret = RegDeleteKeyA(hkey_main, "TestRenameKey");
    ok(ret, "Unexpected return value %ld.\n", ret);

    /* Subkey does not exist. */
    ret = RegCreateKeyExA(hkey_main, "TestRenameKey", 0, NULL, 0, KEY_WRITE, NULL, &key, NULL);
    ok(!ret, "Unexpected return value %ld.\n", ret);
    ret = RegRenameKey(key, L"unknown_subkey", NULL);
    ok(ret == ERROR_FILE_NOT_FOUND, "Unexpected return value %ld.\n", ret);
    ret = RegRenameKey(key, L"unknown_subkey", L"known_subkey");
    ok(ret == ERROR_FILE_NOT_FOUND, "Unexpected return value %ld.\n", ret);

    /* Rename existing subkey. */
    ret = RegCreateKeyExA(key, "known_subkey", 0, NULL, 0, KEY_WRITE, NULL, &key2, NULL);
    ok(!ret, "Unexpected return value %ld.\n", ret);
    RegCloseKey(key2);

    ret = RegRenameKey(key, L"known_subkey", L"renamed_subkey");
    ok(!ret, "Unexpected return value %ld.\n", ret);

    ret = RegDeleteKeyA(key, "renamed_subkey");
    ok(!ret, "Unexpected return value %ld.\n", ret);
    ret = RegDeleteKeyA(key, "known_subkey");
    ok(ret, "Unexpected return value %ld.\n", ret);

    RegCloseKey(key);
#endif
}

static BOOL check_cs_number( const WCHAR *str )
{
    if (str[0] < '0' || str[0] > '9' || str[1] < '0' || str[1] > '9' || str[2] < '0' || str[2] > '9')
        return FALSE;
    if (str[0] == '0' && str[1] == '0' && str[2] == '0')
        return FALSE;
    return TRUE;
}

static void test_control_set_symlink(void)
{
    static const WCHAR target_pfxW[] = L"\\REGISTRY\\Machine\\System\\ControlSet";
    DWORD target_len, type, len, err;
    BYTE buffer[1024];
    HKEY key;

    target_len = sizeof(target_pfxW) + 3 * sizeof(WCHAR);

    err = RegOpenKeyExA( HKEY_LOCAL_MACHINE, "System\\CurrentControlSet", REG_OPTION_OPEN_LINK, KEY_QUERY_VALUE, &key );
    ok( err == ERROR_SUCCESS, "RegOpenKeyEx failed error %lu\n", err );
    len = sizeof(buffer);
    err = RegQueryValueExA( key, "SymbolicLinkValue", NULL, &type, buffer, &len );
    ok( err == ERROR_SUCCESS, "RegQueryValueEx failed error %lu\n", err );
    ok( len == target_len - sizeof(WCHAR), "wrong len %lu\n", len );
    ok( !wcsnicmp( (WCHAR*)buffer, target_pfxW, ARRAY_SIZE(target_pfxW) - 1 ) &&
        check_cs_number( (WCHAR*)buffer + ARRAY_SIZE(target_pfxW) - 1 ),
        "wrong link target\n" );
    RegCloseKey( key );
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
    test_reg_query_info();
    test_string_termination();
    test_multistring_termination();
    test_symlinks();
    test_redirection();
    test_classesroot();
    test_classesroot_enum();
    test_classesroot_mask();
    test_reg_load_key();
    test_reg_load_app_key();
    test_reg_copy_tree();
    test_reg_delete_tree();
    test_rw_order();
    test_deleted_key();
    test_delete_value();
    test_delete_key_value();
    test_RegOpenCurrentUser();
    test_RegNotifyChangeKeyValue();
    test_performance_keys();
    test_RegLoadMUIString();
    test_EnumDynamicTimeZoneInformation();
    test_perflib_key();
    test_RegRenameKey();
    test_control_set_symlink();

    /* cleanup */
    delete_key( hkey_main );

    test_regconnectregistry();
}
