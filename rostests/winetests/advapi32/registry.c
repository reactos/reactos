/*
 * Unit tests for registry functions
 *
 * Copyright (c) 2002 Alexandre Julliard
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
#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winsvc.h"
#include "winerror.h"

static HKEY hkey_main;
static DWORD GLE;

static const char * sTestpath1 = "%LONGSYSTEMVAR%\\subdir1";
static const char * sTestpath2 = "%FOO%\\subdir1";

static HMODULE hadvapi32;
static DWORD (WINAPI *pRegGetValueA)(HKEY,LPCSTR,LPCSTR,DWORD,LPDWORD,PVOID,LPDWORD);
static DWORD (WINAPI *pRegDeleteTreeA)(HKEY,LPCSTR);



/* Debugging functions from wine/libs/wine/debug.c */

/* allocate some tmp string space */
/* FIXME: this is not 100% thread-safe */
static char *get_temp_buffer( int size )
{
    static char *list[32];
    static long pos;
    char *ret;
    int idx;

    idx = ++pos % (sizeof(list)/sizeof(list[0]));
    if ((ret = realloc( list[idx], size ))) list[idx] = ret;
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

static const char *wine_debugstr_wn( const WCHAR *str, int n )
{
    char *dst, *res;
    size_t size;

    if (!HIWORD(str))
    {
        if (!str) return "(null)";
        res = get_temp_buffer( 6 );
        sprintf( res, "#%04x", LOWORD(str) );
        return res;
    }
    if (n == -1) n = lstrlenW(str);
    if (n < 0) n = 0;
    size = 12 + min( 300, n * 5);
    dst = res = get_temp_buffer( n * 5 + 7 );
    *dst++ = 'L';
    *dst++ = '"';
    while (n-- > 0 && dst <= res + size - 10)
    {
        WCHAR c = *str++;
        switch (c)
        {
        case '\n': *dst++ = '\\'; *dst++ = 'n'; break;
        case '\r': *dst++ = '\\'; *dst++ = 'r'; break;
        case '\t': *dst++ = '\\'; *dst++ = 't'; break;
        case '"':  *dst++ = '\\'; *dst++ = '"'; break;
        case '\\': *dst++ = '\\'; *dst++ = '\\'; break;
        default:
            if (c >= ' ' && c <= 126)
                *dst++ = (char)c;
            else
            {
                *dst++ = '\\';
                sprintf(dst,"%04x",c);
                dst+=4;
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
    *dst = 0;
    return res;
}


#define ADVAPI32_GET_PROC(func) \
    p ## func = (void*)GetProcAddress(hadvapi32, #func);


static void InitFunctionPtrs(void)
{
    hadvapi32 = GetModuleHandleA("advapi32.dll");

    /* This function was introduced with Windows 2003 SP1 */
    ADVAPI32_GET_PROC(RegGetValueA)
    ADVAPI32_GET_PROC(RegDeleteTreeA)
}

/* delete key and all its subkeys */
static DWORD delete_key( HKEY hkey )
{
    char name[MAX_PATH];
    DWORD ret;

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
    return 0;
}

static void setup_main_key(void)
{
    if (!RegOpenKeyA( HKEY_CURRENT_USER, "Software\\Wine\\Test", &hkey_main )) delete_key( hkey_main );

    assert (!RegCreateKeyA( HKEY_CURRENT_USER, "Software\\Wine\\Test", &hkey_main ));
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
    lok(cbData == full_byte_len || cbData == str_byte_len /* Win9x */,
        "cbData=%d instead of %d or %d\n", cbData, full_byte_len, str_byte_len);

    value = HeapAlloc(GetProcessHeap(), 0, cbData+1);
    memset(value, 0xbd, cbData+1);
    type=0xdeadbeef;
    ret = RegQueryValueExA(hkey_main, name, NULL, &type, value, &cbData);
    GLE = GetLastError();
    lok(ret == ERROR_SUCCESS, "RegQueryValueExA/2 failed: %d, GLE=%d\n", ret, GLE);
    if (!string)
    {
        /* When cbData == 0, RegQueryValueExA() should not modify the buffer */
        lok(*value == 0xbd || (cbData == 1 && *value == '\0') /* Win9x */,
           "RegQueryValueExA overflowed: cbData=%u *value=%02x\n", cbData, *value);
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
           wine_debugstr_wn((WCHAR*)value, cbData / sizeof(WCHAR)), cbData,
           wine_debugstr_wn(string, full_byte_len / sizeof(WCHAR)), full_byte_len);
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
    /* NT: ERROR_INVALID_PARAMETER, 9x: ERROR_SUCCESS */
    ok(ret == ERROR_INVALID_PARAMETER || broken(ret == ERROR_SUCCESS),
        "got %d (expected ERROR_INVALID_PARAMETER or ERROR_SUCCESS)\n", ret);

    ret = RegSetValueA(hkey_main, NULL, REG_EXPAND_SZ, string2A, sizeof(string2A));
    /* NT: ERROR_INVALID_PARAMETER, 9x: ERROR_SUCCESS */
    ok(ret == ERROR_INVALID_PARAMETER || broken(ret == ERROR_SUCCESS),
        "got %d (expected ERROR_INVALID_PARAMETER or ERROR_SUCCESS)\n", ret);

    ret = RegSetValueA(hkey_main, NULL, REG_MULTI_SZ, string2A, sizeof(string2A));
    /* NT: ERROR_INVALID_PARAMETER, 9x: ERROR_SUCCESS */
    ok(ret == ERROR_INVALID_PARAMETER || broken(ret == ERROR_SUCCESS),
        "got %d (expected ERROR_INVALID_PARAMETER or ERROR_SUCCESS)\n", ret);

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

    /* 9x doesn't support W-calls, so don't test them then */
    if(GLE == ERROR_CALL_NOT_IMPLEMENTED) return; 

    if (0)
    {
        /* Crashes on NT4, Windows 2000 and XP SP1 */
        ret = RegSetValueW(hkey_main, NULL, REG_SZ, NULL, 0);
        ok(ret == ERROR_INVALID_PARAMETER, "RegSetValueW should have failed with ERROR_INVALID_PARAMETER instead of %d\n", ret);
    }

    ret = RegSetValueW(hkey_main, NULL, REG_SZ, string1W, sizeof(string1W));
    ok(ret == ERROR_SUCCESS, "RegSetValueW failed: %d, GLE=%d\n", ret, GetLastError());
    test_hkey_main_Value_A(NULL, string1A, sizeof(string1A));
    test_hkey_main_Value_W(NULL, string1W, sizeof(string1W));

    /* RegSetValueA ignores the size passed in */
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
    ok( data_count == 7, "data_count set to %d instead of 7\n", data_count );
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
    /* Win9x returns 2 as specified by MSDN but NT returns 3... */
    ok( val_count == 2 || val_count == 3, "val_count set to %d\n", val_count );
    ok( data_count == 7, "data_count set to %d instead of 7\n", data_count );
    ok( type == REG_SZ, "type %d is not REG_SZ\n", type );
    /* v5.1.2600.0 (XP Home and Professional) does not touch value or data in this case */
    ok( !strcmp( value, "Te" ) || !strcmp( value, "xxxxxxxxxx" ), 
        "value set to '%s' instead of 'Te' or 'xxxxxxxxxx'\n", value );
    ok( !strcmp( data, "foobar" ) || !strcmp( data, "xxxxxxx" ), 
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
    ok( data_count == 7, "data_count set to %d instead of 7\n", data_count );
    ok( type == REG_SZ, "type %d is not REG_SZ\n", type );
    ok( !strcmp( value, "xxxxxxxxxx" ), "value set to '%s'\n", value );
    /* v5.1.2600.0 (XP Home and Professional) does not touch data in this case */
    ok( !strcmp( data, "foobar" ) || !strcmp( data, "xxxxxxx" ), 
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
    /* the type parameter is cleared on Win9x, but is set to a random value on
     * NT, so don't do that test there. The size parameter is left untouched on Win9x
     * but cleared on NT+, this can be tested on all platforms.
     */
    if (GetVersion() & 0x80000000)
    {
        ok(type == 0, "type should have been set to 0 instead of 0x%x\n", type);
        ok(size == 0xdeadbeef, "size should have been left untouched (0xdeadbeef)\n");
    }
    else
    {
        trace("test_query_value_ex: type set to: 0x%08x\n", type);
        ok(size == 0, "size should have been set to 0 instead of %d\n", size);
    }

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
    ok(ret == ERROR_INVALID_PARAMETER, "ret=%d\n", ret);

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

    /* successful open */
    ret = RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\Test", &hkResult);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %d\n", ret);
    ok(hkResult != NULL, "expected hkResult != NULL\n");
    hkPreserve = hkResult;

    /* these tests fail on Win9x, but we want to be compatible with NT, so
     * run them if we can */
    if (!(GetVersion() & 0x80000000))
    {
        /* open same key twice */
        ret = RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\Test", &hkResult);
        ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %d\n", ret);
        ok(hkResult != hkPreserve, "epxected hkResult != hkPreserve\n");
        ok(hkResult != NULL, "hkResult != NULL\n");
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
    }

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

    /*  beginning backslash character */
    ret = RegOpenKeyA(HKEY_CURRENT_USER, "\\Software\\Wine\\Test", &hkResult);
       ok(ret == ERROR_BAD_PATHNAME || /* NT/2k/XP */
           ret == ERROR_FILE_NOT_FOUND /* Win9x,ME */
           , "expected ERROR_BAD_PATHNAME or ERROR_FILE_NOT_FOUND, got %d\n", ret);

    hkResult = NULL;
    ret = RegOpenKeyExA(HKEY_CLASSES_ROOT, "\\clsid", 0, KEY_QUERY_VALUE, &hkResult);
    ok(ret == ERROR_SUCCESS || /* 2k/XP */
       ret == ERROR_BAD_PATHNAME || /* NT */
       ret == ERROR_FILE_NOT_FOUND /* Win9x,ME */
       , "expected ERROR_SUCCESS, ERROR_BAD_PATHNAME or ERROR_FILE_NOT_FOUND, got %d\n", ret);
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
}

static void test_reg_create_key(void)
{
    LONG ret;
    HKEY hkey1, hkey2;
    ret = RegCreateKeyExA(hkey_main, "Subkey1", 0, NULL, 0, KEY_NOTIFY, NULL, &hkey1, NULL);
    ok(!ret, "RegCreateKeyExA failed with error %d\n", ret);
    /* should succeed: all versions of Windows ignore the access rights
     * to the parent handle */
    ret = RegCreateKeyExA(hkey1, "Subkey2", 0, NULL, 0, KEY_SET_VALUE, NULL, &hkey2, NULL);
    ok(!ret, "RegCreateKeyExA failed with error %d\n", ret);

    /* clean up */
    RegDeleteKey(hkey2, "");
    RegDeleteKey(hkey1, "");

    /*  beginning backslash character */
    ret = RegCreateKeyExA(hkey_main, "\\Subkey3", 0, NULL, 0, KEY_NOTIFY, NULL, &hkey1, NULL);
    if (!(GetVersion() & 0x80000000))
        ok(ret == ERROR_BAD_PATHNAME, "expected ERROR_BAD_PATHNAME, got %d\n", ret);
    else {
        ok(!ret, "RegCreateKeyExA failed with error %d\n", ret);
        RegDeleteKey(hkey1, NULL);
    }

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
}

static void test_reg_close_key(void)
{
    DWORD ret = 0;
    HKEY hkHandle;

    /* successfully close key
     * hkHandle remains changed after call to RegCloseKey
     */
    ret = RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\Test", &hkHandle);
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

    ret = RegDeleteKey(hkey_main, NULL);

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
}

static void test_reg_save_key(void)
{
    DWORD ret;

    ret = RegSaveKey(hkey_main, "saved_key", NULL);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %d\n", ret);
}

static void test_reg_load_key(void)
{
    DWORD ret;
    HKEY hkHandle;

    ret = RegLoadKey(HKEY_LOCAL_MACHINE, "Test", "saved_key");
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %d\n", ret);

    ret = RegOpenKey(HKEY_LOCAL_MACHINE, "Test", &hkHandle);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %d\n", ret);

    RegCloseKey(hkHandle);
}

static void test_reg_unload_key(void)
{
    DWORD ret;

    ret = RegUnLoadKey(HKEY_LOCAL_MACHINE, "Test");
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %d\n", ret);

    DeleteFile("saved_key");
    DeleteFile("saved_key.LOG");
}

static BOOL set_privileges(LPCSTR privilege, BOOL set)
{
    TOKEN_PRIVILEGES tp;
    HANDLE hToken;
    LUID luid;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
        return FALSE;

    if(!LookupPrivilegeValue(NULL, privilege, &luid))
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
    ok(lstrlenA(val) == 0, "Expected val to be untouched, got %s\n", val);

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
    ok(lstrlenA(val) == 0, "Expected val to be untouched, got %s\n", val);
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
    ok(lstrlenW(valW) == 0, "Expected valW to be untouched\n");
    ok(size == sizeof(expected), "Got wrong size: %d\n", size);

    /* unicode - try size in WCHARS */
    SetLastError(0xdeadbeef);
    size = sizeof(valW) / sizeof(WCHAR);
    ret = RegQueryValueW(subkey, NULL, valW, &size);
    ok(ret == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", ret);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());
    ok(lstrlenW(valW) == 0, "Expected valW to be untouched\n");
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

    /* Off-by-two RegSetValueExA -> no trailing '\0', except on Win9x */
    insize=sizeof(string)-2;
    ret = RegSetValueExA(subkey, "stringtest", 0, REG_SZ, (BYTE*)string, insize);
    ok(ret == ERROR_SUCCESS, "RegSetValueExA failed: %d\n", ret);
    outsize=0;
    ret = RegQueryValueExA(subkey, "stringtest", NULL, NULL, NULL, &outsize);
    ok(ret == ERROR_SUCCESS, "RegQueryValueExA failed: %d\n", ret);
    ok(outsize == insize || broken(outsize == sizeof(string)) /* Win9x */,
       "wrong size %u != %u\n", outsize, insize);

    if (outsize == insize)
    {
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
    }

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
    ok(!lstrlenA(buffer),
        "Expected length 0 got length %u(%s)\n", lstrlenA(buffer), buffer);
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
    static char keyname[] = "test_rw_order";
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

    ok(!RegDeleteKey(HKEY_CURRENT_USER, keyname), "Failed to delete key\n");
}

START_TEST(registry)
{
    /* Load pointers for functions that are not available in all Windows versions */
    InitFunctionPtrs();

    setup_main_key();
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

    /* cleanup */
    delete_key( hkey_main );
    
    test_regconnectregistry();
}
