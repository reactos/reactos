/*
 * Unit test suite for environment functions.
 *
 * Copyright 2002 Dmitry Timoshkov
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
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winnls.h"

static CHAR string[MAX_PATH];
#define ok_w(res, format, szString) \
\
    WideCharToMultiByte(CP_ACP, 0, szString, -1, string, MAX_PATH, NULL, NULL); \
    ok(res, format, string);

static BOOL (WINAPI *pGetComputerNameExA)(COMPUTER_NAME_FORMAT,LPSTR,LPDWORD);
static BOOL (WINAPI *pGetComputerNameExW)(COMPUTER_NAME_FORMAT,LPWSTR,LPDWORD);
static BOOL (WINAPI *pGetUserProfileDirectoryA)(HANDLE,LPSTR,LPDWORD);
static BOOL (WINAPI *pSetEnvironmentStringsW)(WCHAR *);

static void init_functionpointers(void)
{
    HMODULE hkernel32 = GetModuleHandleA("kernel32.dll");
    HMODULE huserenv = LoadLibraryA("userenv.dll");

    pGetComputerNameExA = (void *)GetProcAddress(hkernel32, "GetComputerNameExA");
    pGetComputerNameExW = (void *)GetProcAddress(hkernel32, "GetComputerNameExW");
    pSetEnvironmentStringsW = (void *)GetProcAddress(hkernel32, "SetEnvironmentStringsW");
    pGetUserProfileDirectoryA = (void *)GetProcAddress(huserenv,
                                                       "GetUserProfileDirectoryA");
}

static void test_Predefined(void)
{
    char Data[1024];
    DWORD DataSize;
    char Env[sizeof(Data)];
    DWORD EnvSize;
    HANDLE Token;
    BOOL NoErr;

    /*
     * Check value of %USERPROFILE%, should be same as GetUserProfileDirectory()
     * If this fails, your test environment is probably not set up
     */
    if (pGetUserProfileDirectoryA == NULL)
    {
        skip("Skipping USERPROFILE check\n");
        return;
    }
    NoErr = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &Token);
    ok(NoErr, "Failed to open token, error %lu\n", GetLastError());
    DataSize = sizeof(Data);
    NoErr = pGetUserProfileDirectoryA(Token, Data, &DataSize);
    ok(NoErr, "Failed to get user profile dir, error %lu\n", GetLastError());
    if (NoErr)
    {
        EnvSize = GetEnvironmentVariableA("USERPROFILE", Env, sizeof(Env));
        ok(EnvSize != 0 && EnvSize <= sizeof(Env),
           "Failed to retrieve environment variable USERPROFILE, error %lu\n",
           GetLastError());
        ok(strcmp(Data, Env) == 0,
           "USERPROFILE env var %s doesn't match GetUserProfileDirectory %s\n",
           Env, Data);
    }
    else
        skip("Skipping USERPROFILE check, can't get user profile dir\n");
    NoErr = CloseHandle(Token);
    ok(NoErr, "Failed to close token, error %lu\n", GetLastError());
}

static void test_GetSetEnvironmentVariableA(void)
{
    char buf[256];
    BOOL ret;
    DWORD ret_size;
    static const char name[] = "SomeWildName";
    static const char name_cased[] = "sOMEwILDnAME";
    static const char value[] = "SomeWildValue";

    ret = SetEnvironmentVariableA(name, value);
    ok(ret == TRUE,
       "unexpected error in SetEnvironmentVariableA, GetLastError=%ld\n",
       GetLastError());

    /* Try to retrieve the environment variable we just set */
    SetLastError(0xdeadbeef);
    ret_size = GetEnvironmentVariableA(name, NULL, 0);
    ok(ret_size == strlen(value) + 1,
       "should return length with terminating 0 ret_size=%ld\n", ret_size);
    ok(GetLastError() == 0xdeadbeef,
       "should not fail with zero size but GetLastError=%ld\n", GetLastError());

    lstrcpyA(buf, "foo");
    ret_size = GetEnvironmentVariableA(name, buf, lstrlenA(value));
    ok(lstrcmpA(buf, "foo") == 0, "should not touch the buffer\n");
    ok(ret_size == strlen(value) + 1,
       "should return length with terminating 0 ret_size=%ld\n", ret_size);

    lstrcpyA(buf, "foo");
    ret_size = GetEnvironmentVariableA(name, buf, lstrlenA(value) + 1);
    ok(lstrcmpA(buf, value) == 0, "should touch the buffer\n");
    ok(ret_size == strlen(value),
       "should return length without terminating 0 ret_size=%ld\n", ret_size);

    lstrcpyA(buf, "foo");
    ret_size = GetEnvironmentVariableA(name_cased, buf, lstrlenA(value) + 1);
    ok(lstrcmpA(buf, value) == 0, "should touch the buffer\n");
    ok(ret_size == strlen(value),
       "should return length without terminating 0 ret_size=%ld\n", ret_size);

    /* Remove that environment variable */
    ret = SetEnvironmentVariableA(name_cased, NULL);
    ok(ret == TRUE, "should erase existing variable\n");

    lstrcpyA(buf, "foo");
    SetLastError(0xdeadbeef);
    ret_size = GetEnvironmentVariableA(name, buf, lstrlenA(value) + 1);
    ok(lstrcmpA(buf, "foo") == 0, "should not touch the buffer\n");
    ok(ret_size == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND,
       "should not find variable but ret_size=%ld GetLastError=%ld\n",
       ret_size, GetLastError());

    /* Check behavior of SetEnvironmentVariableA(name, "") */
    ret = SetEnvironmentVariableA(name, value);
    ok(ret == TRUE,
       "unexpected error in SetEnvironmentVariableA, GetLastError=%ld\n",
       GetLastError());

    lstrcpyA(buf, "foo");
    ret_size = GetEnvironmentVariableA(name_cased, buf, lstrlenA(value) + 1);
    ok(lstrcmpA(buf, value) == 0, "should touch the buffer\n");
    ok(ret_size == strlen(value),
       "should return length without terminating 0 ret_size=%ld\n", ret_size);

    ret = SetEnvironmentVariableA(name_cased, "");
    ok(ret == TRUE,
       "should not fail with empty value but GetLastError=%ld\n", GetLastError());

    lstrcpyA(buf, "foo");
    SetLastError(0xdeadbeef);
    ret_size = GetEnvironmentVariableA(name, buf, lstrlenA(value) + 1);
    ok(ret_size == 0 &&
       ((GetLastError() == 0xdeadbeef && lstrcmpA(buf, "") == 0) ||
        (GetLastError() == ERROR_ENVVAR_NOT_FOUND)),
       "%s should be set to \"\" (NT) or removed (Win9x) but ret_size=%ld GetLastError=%ld and buf=%s\n",
       name, ret_size, GetLastError(), buf);

    SetLastError(0xdeadbeef);
    ret_size = GetEnvironmentVariableA(name, NULL, 0);
    ok(ret_size == 1 ||
       broken(ret_size == 0), /* XP */
       "should return 1 for empty string but ret_size=%ld GetLastError=%ld\n",
       ret_size, GetLastError());
    ok(GetLastError() == 0xdeadbeef ||
       broken(GetLastError() == ERROR_MORE_DATA), /* XP */
       "should not fail with zero size but GetLastError=%ld\n", GetLastError());

    /* Test the limits */
    SetLastError(0xdeadbeef);
    ret_size = GetEnvironmentVariableA(NULL, NULL, 0);
    ok(ret_size == 0 && (GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == ERROR_ENVVAR_NOT_FOUND),
       "should not find variable but ret_size=%ld GetLastError=%ld\n",
       ret_size, GetLastError());

    SetLastError(0xdeadbeef);
    ret_size = GetEnvironmentVariableA(NULL, buf, lstrlenA(value) + 1);
    ok(ret_size == 0 && (GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == ERROR_ENVVAR_NOT_FOUND),
       "should not find variable but ret_size=%ld GetLastError=%ld\n",
       ret_size, GetLastError());

    SetLastError(0xdeadbeef);
    ret_size = GetEnvironmentVariableA("", buf, lstrlenA(value) + 1);
    ok(ret_size == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND,
       "should not find variable but ret_size=%ld GetLastError=%ld\n",
       ret_size, GetLastError());
}

static void test_GetSetEnvironmentVariableW(void)
{
    WCHAR buf[256];
    BOOL ret;
    DWORD ret_size;
    static const WCHAR name[] = {'S','o','m','e','W','i','l','d','N','a','m','e',0};
    static const WCHAR value[] = {'S','o','m','e','W','i','l','d','V','a','l','u','e',0};
    static const WCHAR name_cased[] = {'s','O','M','E','w','I','L','D','n','A','M','E',0};
    static const WCHAR empty_strW[] = { 0 };
    static const WCHAR fooW[] = {'f','o','o',0};

    ret = SetEnvironmentVariableW(name, value);
    if (ret == FALSE && GetLastError()==ERROR_CALL_NOT_IMPLEMENTED)
    {
        /* Must be Win9x which doesn't support the Unicode functions */
        win_skip("SetEnvironmentVariableW is not implemented\n");
        return;
    }
    ok(ret == TRUE,
       "unexpected error in SetEnvironmentVariableW, GetLastError=%ld\n",
       GetLastError());

    /* Try to retrieve the environment variable we just set */
    SetLastError(0xdeadbeef);
    ret_size = GetEnvironmentVariableW(name, NULL, 0);
    ok(ret_size == lstrlenW(value) + 1,
       "should return length with terminating 0 ret_size=%ld\n",
       ret_size);
    ok(GetLastError() == 0xdeadbeef,
       "should not fail with zero size but GetLastError=%ld\n", GetLastError());

    lstrcpyW(buf, fooW);
    ret_size = GetEnvironmentVariableW(name, buf, lstrlenW(value));
    ok_w(lstrcmpW(buf, fooW) == 0 ||
         lstrlenW(buf) == 0, /* Vista */
         "Expected untouched or empty buffer, got \"%s\"\n", buf);

    ok(ret_size == lstrlenW(value) + 1,
       "should return length with terminating 0 ret_size=%ld\n", ret_size);

    lstrcpyW(buf, fooW);
    ret_size = GetEnvironmentVariableW(name, buf, lstrlenW(value) + 1);
    ok(lstrcmpW(buf, value) == 0, "should touch the buffer\n");
    ok(ret_size == lstrlenW(value),
       "should return length without terminating 0 ret_size=%ld\n", ret_size);

    lstrcpyW(buf, fooW);
    ret_size = GetEnvironmentVariableW(name_cased, buf, lstrlenW(value) + 1);
    ok(lstrcmpW(buf, value) == 0, "should touch the buffer\n");
    ok(ret_size == lstrlenW(value),
       "should return length without terminating 0 ret_size=%ld\n", ret_size);

    /* Remove that environment variable */
    ret = SetEnvironmentVariableW(name_cased, NULL);
    ok(ret == TRUE, "should erase existing variable\n");

    lstrcpyW(buf, fooW);
    SetLastError(0xdeadbeef);
    ret_size = GetEnvironmentVariableW(name, buf, lstrlenW(value) + 1);
    ok(lstrcmpW(buf, fooW) == 0, "should not touch the buffer\n");
    ok(ret_size == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND,
       "should not find variable but ret_size=%ld GetLastError=%ld\n",
       ret_size, GetLastError());

    /* Check behavior of SetEnvironmentVariableW(name, "") */
    ret = SetEnvironmentVariableW(name, value);
    ok(ret == TRUE,
       "unexpected error in SetEnvironmentVariableW, GetLastError=%ld\n",
       GetLastError());

    lstrcpyW(buf, fooW);
    ret_size = GetEnvironmentVariableW(name, buf, lstrlenW(value) + 1);
    ok(lstrcmpW(buf, value) == 0, "should touch the buffer\n");
    ok(ret_size == lstrlenW(value),
       "should return length without terminating 0 ret_size=%ld\n", ret_size);

    ret = SetEnvironmentVariableW(name_cased, empty_strW);
    ok(ret == TRUE, "should not fail with empty value but GetLastError=%ld\n", GetLastError());

    lstrcpyW(buf, fooW);
    SetLastError(0xdeadbeef);
    ret_size = GetEnvironmentVariableW(name, buf, lstrlenW(value) + 1);
    ok(ret_size == 0 &&
       ((GetLastError() == 0xdeadbeef && lstrcmpW(buf, empty_strW) == 0) ||
        (GetLastError() == ERROR_ENVVAR_NOT_FOUND)),
       "should be set to \"\" (NT) or removed (Win9x) but ret_size=%ld GetLastError=%ld\n",
       ret_size, GetLastError());
    ok(lstrcmpW(buf, empty_strW) == 0, "should copy an empty string\n");

    SetLastError(0xdeadbeef);
    ret_size = GetEnvironmentVariableW(name, NULL, 0);
    ok(ret_size == 1 ||
       broken(ret_size == 0), /* XP */
       "should return 1 for empty string but ret_size=%ld GetLastError=%ld\n",
       ret_size, GetLastError());
    ok(GetLastError() == 0xdeadbeef ||
       broken(GetLastError() == ERROR_MORE_DATA), /* XP */
       "should not fail with zero size but GetLastError=%ld\n", GetLastError());

    /* Test the limits */
    SetLastError(0xdeadbeef);
    ret_size = GetEnvironmentVariableW(NULL, NULL, 0);
    ok(ret_size == 0 && (GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == ERROR_ENVVAR_NOT_FOUND),
       "should not find variable but ret_size=%ld GetLastError=%ld\n",
       ret_size, GetLastError());

    if (0) /* Both tests crash on Vista */
    {
        SetLastError(0xdeadbeef);
        ret_size = GetEnvironmentVariableW(NULL, buf, lstrlenW(value) + 1);
        ok(ret_size == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND,
           "should not find variable but ret_size=%ld GetLastError=%ld\n",
           ret_size, GetLastError());

        SetLastError(0xdeadbeef);
        ret = SetEnvironmentVariableW(NULL, NULL);
        ok(ret == FALSE && (GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == ERROR_ENVVAR_NOT_FOUND),
           "should fail with NULL, NULL but ret=%d and GetLastError=%ld\n",
           ret, GetLastError());
    }
}

static void test_GetSetEnvironmentVariableAW(void)
{
    static const WCHAR nameW[] = {0x540D, 0x524D, 0};
    static const char name[] = "\x96\xBC\x91\x4F";
    static const WCHAR valueW[] = {0x5024, 0};
    static const char value[] = "\x92\x6C";
    WCHAR bufW[256];
    char buf[256];
    DWORD ret_size;
    BOOL ret;

    if (GetACP() != 932)
    {
        skip("GetACP() == %d, need 932 for A/W tests\n", GetACP());
        return;
    }

    /* Write W, read A */
    ret = SetEnvironmentVariableW(nameW, valueW);
    ok(ret == TRUE, "SetEnvironmentVariableW failed, last error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret_size = GetEnvironmentVariableA(name, NULL, 0);
    todo_wine ok(ret_size == lstrlenA(value) + 1, "expected ret_size %d, got %ld\n", lstrlenA(value) + 1, ret_size);
    ok(GetLastError() == 0xdeadbeef, "expected last error 0xdeadbeef, got %ld\n", GetLastError());

    lstrcpyA(buf, "foo");
    SetLastError(0xdeadbeef);
    ret_size = GetEnvironmentVariableA(name, buf, lstrlenA(value) + 1);
    todo_wine ok(lstrcmpA(buf, value) == 0, "expected %s, got %s\n", debugstr_a(value), debugstr_a(buf));
    todo_wine ok(ret_size == lstrlenA(value), "expected ret_size %d, got %ld\n", lstrlenA(value), ret_size);
    ok(GetLastError() == 0xdeadbeef, "expected last error 0xdeadbeef, got %ld\n", GetLastError());

    /* Write A, read A/W */
    ret = SetEnvironmentVariableA(name, value);
    ok(ret == TRUE, "SetEnvironmentVariableW failed, last error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret_size = GetEnvironmentVariableA(name, NULL, 0);
    todo_wine ok(ret_size == lstrlenA(value) + 1, "expected ret_size %d, got %ld\n", lstrlenA(value) + 1, ret_size);
    ok(GetLastError() == 0xdeadbeef, "expected last error 0xdeadbeef, got %ld\n", GetLastError());

    lstrcpyA(buf, "foo");
    SetLastError(0xdeadbeef);
    ret_size = GetEnvironmentVariableA(name, buf, lstrlenA(value) + 1);
    todo_wine ok(lstrcmpA(buf, value) == 0, "expected %s, got %s\n", debugstr_a(value), debugstr_a(buf));
    todo_wine ok(ret_size == lstrlenA(value), "expected ret_size %d, got %ld\n", lstrlenA(value), ret_size);
    ok(GetLastError() == 0xdeadbeef, "expected last error 0xdeadbeef, got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret_size = GetEnvironmentVariableW(nameW, NULL, 0);
    ok(ret_size == lstrlenW(valueW) + 1, "expected ret_size %d, got %ld\n", lstrlenW(valueW) + 1, ret_size);
    ok(GetLastError() == 0xdeadbeef, "expected last error 0xdeadbeef, got %ld\n", GetLastError());

    lstrcpyW(bufW, L"foo");
    SetLastError(0xdeadbeef);
    ret_size = GetEnvironmentVariableW(nameW, bufW, lstrlenW(valueW) + 1);
    ok(ret_size == lstrlenW(valueW), "expected ret_size %d, got %ld\n", lstrlenW(valueW), ret_size);
    ok(GetLastError() == 0xdeadbeef, "expected last error 0xdeadbeef, got %ld\n", GetLastError());
    ok(lstrcmpW(bufW, valueW) == 0, "expected %s, got %s\n", debugstr_w(valueW), debugstr_w(bufW));
}

static void test_ExpandEnvironmentStringsW(void)
{
    int i;
    WCHAR buf[256];
    DWORD ret;
    BOOL success;
    static const struct test_info
    {
        const WCHAR *input;
        const WCHAR *expected_str;
        int count_in;
        int expected_count_out;
    } tests[] =
    {
        /*  0 */ { L"Long long value",       L"abcdefghijklmnopqrstuv",  0, 16 },
        /*  1 */ { L"Long long value",       L"abcdefghijklmnopqrstuv",  1, 16 },
        /*  2 */ { L"Long long value",       L"Lbcdefghijklmnopqrstuv",  2, 16 },
        /*  3 */ { L"Long long value",       L"Locdefghijklmnopqrstuv",  3, 16 },
        /*  4 */ { L"Long long value",       L"Long long valuopqrstuv", 15, 16 },
        /*  5 */ { L"Long long value",       L"Long long value",        16, 16 },
        /*  6 */ { L"Long long value",       L"Long long value",        17, 16 },
        /*  7 */ { L"%TVAR% long long",      L"abcdefghijklmnopqrstuv",  0, 15 },
        /*  8 */ { L"%TVAR% long long",      L"",                        1, 15 },
        /*  9 */ { L"%TVAR% long long",      L"",                        2, 15 },
        /* 10 */ { L"%TVAR% long long",      L"",                        4, 15 },
        /* 11 */ { L"%TVAR% long long",      L"WINE",                    5, 15 },
        /* 12 */ { L"%TVAR% long long",      L"WINE fghijklmnopqrstuv",  6, 15 },
        /* 13 */ { L"%TVAR% long long",      L"WINE lghijklmnopqrstuv",  7, 15 },
        /* 14 */ { L"%TVAR% long long",      L"WINE long long",         15, 15 },
        /* 15 */ { L"%TVAR% long long",      L"WINE long long",         16, 15 },
        /* 16 */ { L"%TVAR%%TVAR% long",     L"",                        4, 14 },
        /* 17 */ { L"%TVAR%%TVAR% long",     L"WINE",                    5, 14 },
        /* 18 */ { L"%TVAR%%TVAR% long",     L"WINE",                    6, 14 },
        /* 19 */ { L"%TVAR%%TVAR% long",     L"WINE",                    8, 14 },
        /* 20 */ { L"%TVAR%%TVAR% long",     L"WINEWINE",                9, 14 },
        /* 21 */ { L"%TVAR%%TVAR% long",     L"WINEWINE jklmnopqrstuv", 10, 14 },
        /* 22 */ { L"%TVAR%%TVAR% long",     L"WINEWINE long",          14, 14 },
        /* 23 */ { L"%TVAR%%TVAR% long",     L"WINEWINE long",          15, 14 },
        /* 24 */ { L"%TVAR% %TVAR% long",    L"WINE",                    5, 15 },
        /* 25 */ { L"%TVAR% %TVAR% long",    L"WINE ",                   6, 15 },
        /* 26 */ { L"%TVAR% %TVAR% long",    L"WINE ",                   8, 15 },
        /* 27 */ { L"%TVAR% %TVAR% long",    L"WINE ",                   9, 15 },
        /* 28 */ { L"%TVAR% %TVAR% long",    L"WINE WINE",              10, 15 },
        /* 29 */ { L"%TVAR% %TVAR% long",    L"WINE WINE klmnopqrstuv", 11, 15 },
        /* 30 */ { L"%TVAR% %TVAR% long",    L"WINE WINE llmnopqrstuv", 12, 15 },
        /* 31 */ { L"%TVAR% %TVAR% long",    L"WINE WINE lonnopqrstuv", 14, 15 },
        /* 32 */ { L"%TVAR% %TVAR% long",    L"WINE WINE long",         15, 15 },
        /* 33 */ { L"%TVAR% %TVAR% long",    L"WINE WINE long",         16, 15 },
        /* 34 */ { L"%TVAR2% long long",     L"abcdefghijklmnopqrstuv",  1, 18 },
        /* 35 */ { L"%TVAR2% long long",     L"%bcdefghijklmnopqrstuv",  2, 18 },
        /* 36 */ { L"%TVAR2% long long",     L"%TVdefghijklmnopqrstuv",  4, 18 },
        /* 37 */ { L"%TVAR2% long long",     L"%TVAR2ghijklmnopqrstuv",  7, 18 },
        /* 38 */ { L"%TVAR2% long long",     L"%TVAR2%hijklmnopqrstuv",  8, 18 },
        /* 39 */ { L"%TVAR2% long long",     L"%TVAR2% ijklmnopqrstuv",  9, 18 },
        /* 40 */ { L"%TVAR2% long long",     L"%TVAR2% ljklmnopqrstuv", 10, 18 },
        /* 41 */ { L"%TVAR2% long long",     L"%TVAR2% long long",      18, 18 },
        /* 42 */ { L"%TVAR2% long long",     L"%TVAR2% long long",      19, 18 },
        /* 43 */ { L"%TVAR long long",       L"abcdefghijklmnopqrstuv",  1, 16 },
        /* 44 */ { L"%TVAR long long",       L"%bcdefghijklmnopqrstuv",  2, 16 },
        /* 45 */ { L"%TVAR long long",       L"%Tcdefghijklmnopqrstuv",  3, 16 },
        /* 46 */ { L"%TVAR long long",       L"%TVAR long lonopqrstuv", 15, 16 },
        /* 47 */ { L"%TVAR long long",       L"%TVAR long long",        16, 16 },
        /* 48 */ { L"%TVAR long long",       L"%TVAR long long",        17, 16 },
    };

    success = SetEnvironmentVariableW(L"TVAR", L"WINE");
    ok(success, "SetEnvironmentVariableW failed with %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = ExpandEnvironmentStringsW(L"Long long value", NULL, 0);
    ok(ret == 16, "got %lu\n", ret);
    ok(GetLastError() == 0xdeadbeef, "got last error %ld\n", GetLastError());

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        const struct test_info *test = &tests[i];

        SetLastError(0xdeadbeef);
        wcscpy(buf, L"abcdefghijklmnopqrstuv");
        ret = ExpandEnvironmentStringsW(test->input, buf, test->count_in);
        ok(ret == test->expected_count_out, "Test %d: got %lu\n", i, ret);
        ok(GetLastError() == 0xdeadbeef, "Test %d: got last error %ld\n", i, GetLastError());
        ok(!wcscmp(buf, test->expected_str), "Test %d: got %s\n", i, debugstr_w(buf));
    }

    success = SetEnvironmentVariableW(L"TVAR", NULL);
    ok(success, "SetEnvironmentVariableW failed with %ld\n", GetLastError());
}

static void test_ExpandEnvironmentStringsA(void)
{
    const char* value="Long long value";
    const char* not_an_env_var="%NotAnEnvVar%";
    char buf[256], buf1[256], buf2[0x8000];
    DWORD ret_size, ret_size1;
    int i;
    DWORD ret;
    BOOL success;
    static const struct test_info
    {
    const char *input;
    const char *expected_str;
    int count_in;
    int expected_count_out;
    } tests[] =
    {
        /*  0 */ { "Long long value",       "",                   0, 17 },
        /*  1 */ { "Long long value",       "",                   1, 17 },
        /*  2 */ { "Long long value",       "",                   2, 17 },
        /*  3 */ { "Long long value",       "",                   3, 17 },
        /*  4 */ { "Long long value",       "",                  15, 17 },
        /*  5 */ { "Long long value",       "",                  16, 17 },
        /*  6 */ { "Long long value",       "Long long value",   17, 16 },
        /*  7 */ { "%TVAR% long long",      "",                   0, 16 },
        /*  8 */ { "%TVAR% long long",      "",                   1, 16 },
        /*  9 */ { "%TVAR% long long",      "",                   2, 16 },
        /* 10 */ { "%TVAR% long long",      "",                   4, 16 },
        /* 11 */ { "%TVAR% long long",      "",                   5, 16 },
        /* 12 */ { "%TVAR% long long",      "",                   6, 16 },
        /* 13 */ { "%TVAR% long long",      "",                   7, 16 },
        /* 14 */ { "%TVAR% long long",      "",                  15, 16 },
        /* 15 */ { "%TVAR% long long",      "WINE long long",    16, 15 },
        /* 16 */ { "%TVAR%%TVAR% long",     "",                   4, 15 },
        /* 17 */ { "%TVAR%%TVAR% long",     "",                   5, 15 },
        /* 18 */ { "%TVAR%%TVAR% long",     "",                   6, 15 },
        /* 19 */ { "%TVAR%%TVAR% long",     "",                   8, 15 },
        /* 20 */ { "%TVAR%%TVAR% long",     "",                   9, 15 },
        /* 21 */ { "%TVAR%%TVAR% long",     "",                  10, 15 },
        /* 22 */ { "%TVAR%%TVAR% long",     "",                  14, 15 },
        /* 23 */ { "%TVAR%%TVAR% long",     "WINEWINE long",     15, 14 },
        /* 24 */ { "%TVAR% %TVAR% long",    "",                   5, 16 },
        /* 25 */ { "%TVAR% %TVAR% long",    "",                   6, 16 },
        /* 26 */ { "%TVAR% %TVAR% long",    "",                   8, 16 },
        /* 27 */ { "%TVAR% %TVAR% long",    "",                   9, 16 },
        /* 28 */ { "%TVAR% %TVAR% long",    "",                  10, 16 },
        /* 29 */ { "%TVAR% %TVAR% long",    "",                  11, 16 },
        /* 30 */ { "%TVAR% %TVAR% long",    "",                  12, 16 },
        /* 31 */ { "%TVAR% %TVAR% long",    "",                  14, 16 },
        /* 32 */ { "%TVAR% %TVAR% long",    "",                  15, 16 },
        /* 33 */ { "%TVAR% %TVAR% long",    "WINE WINE long",    16, 15 },
        /* 34 */ { "%TVAR2% long long",     "",                   1, 19 },
        /* 35 */ { "%TVAR2% long long",     "",                   2, 19 },
        /* 36 */ { "%TVAR2% long long",     "",                   4, 19 },
        /* 37 */ { "%TVAR2% long long",     "",                   7, 19 },
        /* 38 */ { "%TVAR2% long long",     "",                   8, 19 },
        /* 39 */ { "%TVAR2% long long",     "",                   9, 19 },
        /* 40 */ { "%TVAR2% long long",     "",                  10, 19 },
        /* 41 */ { "%TVAR2% long long",     "",                  18, 19 },
        /* 42 */ { "%TVAR2% long long",     "%TVAR2% long long", 19, 18 },
        /* 43 */ { "%TVAR long long",       "",                   1, 17 },
        /* 44 */ { "%TVAR long long",       "",                   2, 17 },
        /* 45 */ { "%TVAR long long",       "",                   3, 17 },
        /* 46 */ { "%TVAR long long",       "",                  15, 17 },
        /* 47 */ { "%TVAR long long",       "",                  16, 17 },
        /* 48 */ { "%TVAR long long",       "%TVAR long long",   17, 16 },
    };

    SetEnvironmentVariableA("EnvVar", value);

    ret_size = ExpandEnvironmentStringsA(NULL, buf1, sizeof(buf1));
    ok(ret_size == 1 || ret_size == 0 /* Win9x */ || ret_size == 2 /* NT4 */,
       "ExpandEnvironmentStrings returned %ld\n", ret_size);

    /* Try to get the required buffer size 'the natural way' */
    strcpy(buf, "%EnvVar%");
    ret_size = ExpandEnvironmentStringsA(buf, NULL, 0);
    ok(ret_size == strlen(value)+1 || /* win98 */
       ret_size == (strlen(value)+1)*2 || /* NT4 */
       ret_size == strlen(value)+2 || /* win2k, XP, win2k3 */
       ret_size == 0 /* Win95 */,
       "ExpandEnvironmentStrings returned %ld instead of %d, %d or %d\n",
       ret_size, lstrlenA(value)+1, lstrlenA(value)+2, 0);

    /* Again, side-stepping the Win95 bug */
    ret_size = ExpandEnvironmentStringsA(buf, buf1, 0);
    /* v5.1.2600.2945 (XP SP2) returns len + 2 here! */
    ok(ret_size == strlen(value)+1 || ret_size == strlen(value)+2 ||
       ret_size == (strlen(value)+1)*2 /* NT4 */,
       "ExpandEnvironmentStrings returned %ld instead of %d\n",
       ret_size, lstrlenA(value)+1);

    /* Try with a buffer that's too small */
    ret_size = ExpandEnvironmentStringsA(buf, buf1, 12);
    /* v5.1.2600.2945 (XP SP2) returns len + 2 here! */
    ok(ret_size == strlen(value)+1 || ret_size == strlen(value)+2 ||
       ret_size == (strlen(value)+1)*2 /* NT4 */,
       "ExpandEnvironmentStrings returned %ld instead of %d\n",
       ret_size, lstrlenA(value)+1);

    /* Try with a buffer of just the right size */
    /* v5.1.2600.2945 (XP SP2) needs and returns len + 2 here! */
    ret_size = ExpandEnvironmentStringsA(buf, buf1, ret_size);
    ok(ret_size == strlen(value)+1 || ret_size == strlen(value)+2 ||
       ret_size == (strlen(value)+1)*2 /* NT4 */,
       "ExpandEnvironmentStrings returned %ld instead of %d\n",
       ret_size, lstrlenA(value)+1);
    ok(!strcmp(buf1, value), "ExpandEnvironmentStrings returned [%s]\n", buf1);

    /* Try with an unset environment variable */
    strcpy(buf, not_an_env_var);
    ret_size = ExpandEnvironmentStringsA(buf, buf1, sizeof(buf1));
    ok(ret_size == strlen(not_an_env_var)+1 ||
       ret_size == (strlen(not_an_env_var)+1)*2 /* NT4 */,
       "ExpandEnvironmentStrings returned %ld instead of %d\n", ret_size, lstrlenA(not_an_env_var)+1);
    ok(!strcmp(buf1, not_an_env_var), "ExpandEnvironmentStrings returned [%s]\n", buf1);

    /* test a large destination size */
    strcpy(buf, "12345");
    ret_size = ExpandEnvironmentStringsA(buf, buf2, sizeof(buf2));
    ok(!strcmp(buf, buf2), "ExpandEnvironmentStrings failed %s vs %s. ret_size = %ld\n", buf, buf2, ret_size);

    SetLastError(0xdeadbeef);
    ret_size1 = GetWindowsDirectoryA(buf1,256);
    ok ((ret_size1 >0) && (ret_size1<256), "GetWindowsDirectory Failed\n");
    ret_size = ExpandEnvironmentStringsA("%SystemRoot%",buf,sizeof(buf));
    if (ERROR_ENVVAR_NOT_FOUND != GetLastError())
    {
        ok(!strcmp(buf, buf1), "ExpandEnvironmentStrings failed %s vs %s. ret_size = %ld\n", buf, buf1, ret_size);
    }

    /* Try with a variable that references another */
    SetEnvironmentVariableA("IndirectVar", "Foo%EnvVar%Bar");
    strcpy(buf, "Indirect-%IndirectVar%-Indirect");
    strcpy(buf2, "Indirect-Foo%EnvVar%Bar-Indirect");
    ret_size = ExpandEnvironmentStringsA(buf, buf1, sizeof(buf1));
    ok(ret_size == strlen(buf2)+1 ||
       ret_size == (strlen(buf2)+1)*2 /* NT4 */,
       "ExpandEnvironmentStrings returned %ld instead of %d\n", ret_size, lstrlenA(buf2)+1);
    ok(!strcmp(buf1, buf2), "ExpandEnvironmentStrings returned [%s]\n", buf1);
    SetEnvironmentVariableA("IndirectVar", NULL);

    SetEnvironmentVariableA("EnvVar", NULL);

    success = SetEnvironmentVariableA("TVAR", "WINE");
    ok(success, "SetEnvironmentVariableA failed with %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = ExpandEnvironmentStringsW(L"Long long value", NULL, 0);
    ok(ret == 16, "got %lu\n", ret);
    ok(GetLastError() == 0xdeadbeef, "got last error %ld\n", GetLastError());

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        const struct test_info *test = &tests[i];

        SetLastError(0xdeadbeef);
        strcpy(buf, "abcdefghijklmnopqrstuv");
        ret = ExpandEnvironmentStringsA(test->input, buf, test->count_in);
        ok(ret == test->expected_count_out, "Test %d: got %lu\n", i, ret);
        ok(GetLastError() == 0xdeadbeef, "Test %d: got last error %ld\n", i, GetLastError());
        ok(!strcmp(buf, test->expected_str), "Test %d: got %s\n", i, debugstr_a(buf));
    }

    success = SetEnvironmentVariableA("TVAR", NULL);
    ok(success, "SetEnvironmentVariableA failed with %ld\n", GetLastError());

    if (GetACP() == 932) /* shift-jis */
    {
        const char *japanese_test = "\x88\xEA\x93\xF1\x8E\x4F\x8E\x6C\x8C\xDC\x98\x5A\x8E\xB5\x94\xAA\x8B\xE3"; /* Japanese Kanji for "one" to "nine", in shift-jis */
        int japanese_len = strlen(japanese_test);

        SetLastError(0xdeadbeef);
        ret_size = ExpandEnvironmentStringsA(japanese_test, NULL, 0);
        ok(GetLastError() == 0xdeadbeef, "got last error %ld\n", GetLastError());
        ok(ret_size >= japanese_len, "Needed at least %d, got %lu\n", japanese_len, ret_size);

        SetLastError(0xdeadbeef);
        ret_size = ExpandEnvironmentStringsA(japanese_test, buf, ret_size);
        ok(GetLastError() == 0xdeadbeef, "got last error %ld\n", GetLastError());
        ok(ret_size >= japanese_len, "Needed at least %d, got %lu\n", japanese_len, ret_size);
        ok(!strcmp(buf, japanese_test), "Got %s\n", debugstr_a(buf));

        SetLastError(0xdeadbeef);
        ret_size = ExpandEnvironmentStringsA(japanese_test, buf, japanese_len / 2); /* Buffer too small */
        ok(GetLastError() == 0xdeadbeef, "got last error %ld\n", GetLastError());
        ok(ret_size >= japanese_len, "Needed at least %d, got %lu\n", japanese_len, ret_size);
    }
    else
    {
        skip("Skipping japanese language tests\n");
    }
}

static void test_GetComputerName(void)
{
    DWORD size;
    BOOL ret;
    LPSTR name;
    LPWSTR nameW;
    DWORD error;
    int name_len;

    size = 0;
    SetLastError(0xdeadbeef);
    ret = GetComputerNameA((LPSTR)0xdeadbeef, &size);
    error = GetLastError();
    ok(!ret && error == ERROR_BUFFER_OVERFLOW, "GetComputerNameA should have failed with ERROR_BUFFER_OVERFLOW instead of %ld\n", error);

    /* Only Vista returns the computer name length as documented in the MSDN */
    if (size != 0)
    {
        size++; /* nul terminating character */
        name = HeapAlloc(GetProcessHeap(), 0, size * sizeof(name[0]));
        ok(name != NULL, "HeapAlloc failed with error %ld\n", GetLastError());
        ret = GetComputerNameA(name, &size);
        ok(ret, "GetComputerNameA failed with error %ld\n", GetLastError());
        HeapFree(GetProcessHeap(), 0, name);
    }

    size = MAX_COMPUTERNAME_LENGTH + 1;
    name = HeapAlloc(GetProcessHeap(), 0, size * sizeof(name[0]));
    ok(name != NULL, "HeapAlloc failed with error %ld\n", GetLastError());
    ret = GetComputerNameA(name, &size);
    ok(ret, "GetComputerNameA failed with error %ld\n", GetLastError());
    trace("computer name is \"%s\"\n", name);
    name_len = strlen(name);
    ok(size == name_len, "size should be same as length, name_len=%d, size=%ld\n", name_len, size);
    HeapFree(GetProcessHeap(), 0, name);

    size = 0;
    SetLastError(0xdeadbeef);
    ret = GetComputerNameW((LPWSTR)0xdeadbeef, &size);
    error = GetLastError();
    if (error == ERROR_CALL_NOT_IMPLEMENTED)
        win_skip("GetComputerNameW is not implemented\n");
    else
    {
        ok(!ret && error == ERROR_BUFFER_OVERFLOW, "GetComputerNameW should have failed with ERROR_BUFFER_OVERFLOW instead of %ld\n", error);
        size++; /* nul terminating character */
        nameW = HeapAlloc(GetProcessHeap(), 0, size * sizeof(nameW[0]));
        ok(nameW != NULL, "HeapAlloc failed with error %ld\n", GetLastError());
        ret = GetComputerNameW(nameW, &size);
        ok(ret, "GetComputerNameW failed with error %ld\n", GetLastError());
        HeapFree(GetProcessHeap(), 0, nameW);
    }
}

static void test_GetComputerNameExA(void)
{
    DWORD size;
    BOOL ret;
    LPSTR name;
    DWORD error;

    static const int MAX_COMP_NAME = 32767;

    if (!pGetComputerNameExA)
    {
        win_skip("GetComputerNameExA function not implemented\n");
        return;
    }

    size = 0;
    SetLastError(0xdeadbeef);
    ret = pGetComputerNameExA(ComputerNameDnsDomain, (LPSTR)0xdeadbeef, &size);
    error = GetLastError();
    ok(ret == 0, "Expected 0, got %d\n", ret);
    ok(error == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %ld\n", error);

    /* size is not set in win2k */
    if (size == 0)
    {
        win_skip("Win2k doesn't set the size\n");
        size = MAX_COMP_NAME;
    }
    name = HeapAlloc(GetProcessHeap(), 0, size * sizeof(name[0]));
    ok(name != NULL, "HeapAlloc failed with error %ld\n", GetLastError());
    ret = pGetComputerNameExA(ComputerNameDnsDomain, name, &size);
    ok(ret, "GetComputerNameExA(ComputerNameDnsDomain) failed with error %ld\n", GetLastError());
    trace("domain name is \"%s\"\n", name);
    HeapFree(GetProcessHeap(), 0, name);

    size = 0;
    SetLastError(0xdeadbeef);
    ret = pGetComputerNameExA(ComputerNameDnsFullyQualified, (LPSTR)0xdeadbeef, &size);
    error = GetLastError();
    ok(ret == 0, "Expected 0, got %d\n", ret);
    ok(error == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %ld\n", error);

    /* size is not set in win2k */
    if (size == 0)
        size = MAX_COMP_NAME;
    name = HeapAlloc(GetProcessHeap(), 0, size * sizeof(name[0]));
    ok(name != NULL, "HeapAlloc failed with error %ld\n", GetLastError());
    ret = pGetComputerNameExA(ComputerNameDnsFullyQualified, name, &size);
    ok(ret, "GetComputerNameExA(ComputerNameDnsFullyQualified) failed with error %ld\n", GetLastError());
    trace("fully qualified hostname is \"%s\"\n", name);
    HeapFree(GetProcessHeap(), 0, name);

    size = 0;
    SetLastError(0xdeadbeef);
    ret = pGetComputerNameExA(ComputerNameDnsHostname, (LPSTR)0xdeadbeef, &size);
    error = GetLastError();
    ok(ret == 0, "Expected 0, got %d\n", ret);
    ok(error == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %ld\n", error);

    /* size is not set in win2k */
    if (size == 0)
        size = MAX_COMP_NAME;
    name = HeapAlloc(GetProcessHeap(), 0, size * sizeof(name[0]));
    ok(name != NULL, "HeapAlloc failed with error %ld\n", GetLastError());
    ret = pGetComputerNameExA(ComputerNameDnsHostname, name, &size);
    ok(ret, "GetComputerNameExA(ComputerNameDnsHostname) failed with error %ld\n", GetLastError());
    trace("hostname is \"%s\"\n", name);
    HeapFree(GetProcessHeap(), 0, name);

    size = 0;
    SetLastError(0xdeadbeef);
    ret = pGetComputerNameExA(ComputerNameNetBIOS, (LPSTR)0xdeadbeef, &size);
    error = GetLastError();
    ok(ret == 0, "Expected 0, got %d\n", ret);
    ok(error == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %ld\n", error);

    size = 0;
    SetLastError(0xdeadbeef);
    ret = pGetComputerNameExA(ComputerNameNetBIOS, NULL, &size);
    error = GetLastError();
    ok(ret == 0, "Expected 0, got %d\n", ret);
    ok(error == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %ld\n", error);

    /* size is not set in win2k */
    if (size == 0)
        size = MAX_COMP_NAME;
    name = HeapAlloc(GetProcessHeap(), 0, size * sizeof(name[0]));
    ok(name != NULL, "HeapAlloc failed with error %ld\n", GetLastError());
    ret = pGetComputerNameExA(ComputerNameNetBIOS, name, &size);
    ok(ret, "GetComputerNameExA(ComputerNameNetBIOS) failed with error %ld\n", GetLastError());
    trace("NetBIOS name is \"%s\"\n", name);
    HeapFree(GetProcessHeap(), 0, name);
}

static void test_GetComputerNameExW(void)
{
    DWORD size;
    BOOL ret;
    LPWSTR nameW;
    DWORD error;

    if (!pGetComputerNameExW)
    {
        win_skip("GetComputerNameExW function not implemented\n");
        return;
    }

    size = 0;
    SetLastError(0xdeadbeef);
    ret = pGetComputerNameExW(ComputerNameDnsDomain, (LPWSTR)0xdeadbeef, &size);
    error = GetLastError();
    ok(!ret && error == ERROR_MORE_DATA, "GetComputerNameExW should have failed with ERROR_MORE_DATA instead of %ld\n", error);
    nameW = HeapAlloc(GetProcessHeap(), 0, size * sizeof(nameW[0]));
    ok(nameW != NULL, "HeapAlloc failed with error %ld\n", GetLastError());
    ret = pGetComputerNameExW(ComputerNameDnsDomain, nameW, &size);
    ok(ret, "GetComputerNameExW(ComputerNameDnsDomain) failed with error %ld\n", GetLastError());
    HeapFree(GetProcessHeap(), 0, nameW);

    size = 0;
    SetLastError(0xdeadbeef);
    ret = pGetComputerNameExW(ComputerNameDnsFullyQualified, (LPWSTR)0xdeadbeef, &size);
    error = GetLastError();
    ok(!ret && error == ERROR_MORE_DATA, "GetComputerNameExW should have failed with ERROR_MORE_DATA instead of %ld\n", error);
    nameW = HeapAlloc(GetProcessHeap(), 0, size * sizeof(nameW[0]));
    ok(nameW != NULL, "HeapAlloc failed with error %ld\n", GetLastError());
    ret = pGetComputerNameExW(ComputerNameDnsFullyQualified, nameW, &size);
    ok(ret, "GetComputerNameExW(ComputerNameDnsFullyQualified) failed with error %ld\n", GetLastError());
    HeapFree(GetProcessHeap(), 0, nameW);

    size = 0;
    SetLastError(0xdeadbeef);
    ret = pGetComputerNameExW(ComputerNameDnsHostname, (LPWSTR)0xdeadbeef, &size);
    error = GetLastError();
    ok(!ret && error == ERROR_MORE_DATA, "GetComputerNameExW should have failed with ERROR_MORE_DATA instead of %ld\n", error);
    nameW = HeapAlloc(GetProcessHeap(), 0, size * sizeof(nameW[0]));
    ok(nameW != NULL, "HeapAlloc failed with error %ld\n", GetLastError());
    ret = pGetComputerNameExW(ComputerNameDnsHostname, nameW, &size);
    ok(ret, "GetComputerNameExW(ComputerNameDnsHostname) failed with error %ld\n", GetLastError());
    HeapFree(GetProcessHeap(), 0, nameW);

    size = 0;
    SetLastError(0xdeadbeef);
    ret = pGetComputerNameExW(ComputerNameNetBIOS, (LPWSTR)0xdeadbeef, &size);
    error = GetLastError();
    ok(!ret && error == ERROR_MORE_DATA, "GetComputerNameExW should have failed with ERROR_MORE_DATA instead of %ld\n", error);
    nameW = HeapAlloc(GetProcessHeap(), 0, size * sizeof(nameW[0]));
    ok(nameW != NULL, "HeapAlloc failed with error %ld\n", GetLastError());
    ret = pGetComputerNameExW(ComputerNameNetBIOS, nameW, &size);
    ok(ret, "GetComputerNameExW(ComputerNameNetBIOS) failed with error %ld\n", GetLastError());
    HeapFree(GetProcessHeap(), 0, nameW);

    size = 0;
    SetLastError(0xdeadbeef);
    ret = pGetComputerNameExW(ComputerNameNetBIOS, NULL, &size);
    error = GetLastError();
    ok(!ret && error == ERROR_MORE_DATA, "GetComputerNameExW should have failed with ERROR_MORE_DATA instead of %ld\n", error);
}

static void test_GetEnvironmentStringsW(void)
{
    PWCHAR env1;
    PWCHAR env2;

    env1 = GetEnvironmentStringsW();
    env2 = GetEnvironmentStringsW();
    ok(env1 != env2 ||
       broken(env1 == env2), /* NT <= 5.1 */
       "should return different copies\n");
    FreeEnvironmentStringsW(env1);
    FreeEnvironmentStringsW(env2);
}

#define copy_string(dst, src) memcpy(dst, src, sizeof(src))

static void check_env_var_(int line, const char *var, const char *value)
{
    char buffer[20];
    DWORD size = GetEnvironmentVariableA(var, buffer, sizeof(buffer));
    if (value)
    {
        ok_(__FILE__, line)(size == strlen(value), "wrong size %lu\n", size);
        ok_(__FILE__, line)(!strcmp(buffer, value), "wrong value %s\n", debugstr_a(buffer));
    }
    else
    {
        ok_(__FILE__, line)(!size, "wrong size %lu\n", size);
        ok_(__FILE__, line)(GetLastError() == ERROR_ENVVAR_NOT_FOUND, "got error %lu\n", GetLastError());
    }
}
#define check_env_var(a, b) check_env_var_(__LINE__, a, b)

static void test_SetEnvironmentStrings(void)
{
    static const WCHAR testenv[] = L"testenv1=unus\0testenv3=tres\0";
    WCHAR env[200];
    WCHAR *old_env;
    BOOL ret;

    if (!pSetEnvironmentStringsW)
    {
        win_skip("SetEnvironmentStringsW() is not available\n");
        return;
    }

    ret = SetEnvironmentVariableA("testenv1", "heis");
    ok(ret, "got error %lu\n", GetLastError());
    ret = SetEnvironmentVariableA("testenv2", "dyo");
    ok(ret, "got error %lu\n", GetLastError());

    old_env = GetEnvironmentStringsW();

    memcpy(env, testenv, sizeof(testenv));
    ret = pSetEnvironmentStringsW(env);
    ok(ret, "got error %lu\n", GetLastError());
    ok(!memcmp(env, testenv, sizeof(testenv)), "input parameter should not be changed\n");

    check_env_var("testenv1", "unus");
    check_env_var("testenv2", NULL);
    check_env_var("testenv3", "tres");
    check_env_var("PATH", NULL);

    ret = pSetEnvironmentStringsW(old_env);
    ok(ret, "got error %lu\n", GetLastError());

    check_env_var("testenv1", "heis");
    check_env_var("testenv2", "dyo");
    check_env_var("testenv3", NULL);

    SetEnvironmentVariableA("testenv1", NULL);
    SetEnvironmentVariableA("testenv2", NULL);

    copy_string(env, L"testenv\0");
    SetLastError(0xdeadbeef);
    ret = pSetEnvironmentStringsW(env);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "got error %lu\n", GetLastError());

    copy_string(env, L"=unus\0");
    SetLastError(0xdeadbeef);
    ret = pSetEnvironmentStringsW(env);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "got error %lu\n", GetLastError());

    copy_string(env, L"one=two=three four=five\0");
    ret = pSetEnvironmentStringsW(env);
    ok(ret, "got error %lu\n", GetLastError());

    check_env_var("one", "two=three four=five");

    ret = pSetEnvironmentStringsW(old_env);
    ok(ret, "got error %lu\n", GetLastError());
    ret = FreeEnvironmentStringsW(old_env);
    ok(ret, "got error %lu\n", GetLastError());
}

START_TEST(environ)
{
    init_functionpointers();

    test_Predefined();
    test_GetSetEnvironmentVariableA();
    test_GetSetEnvironmentVariableW();
    test_GetSetEnvironmentVariableAW();
    test_ExpandEnvironmentStringsW();
    test_ExpandEnvironmentStringsA();
    test_GetComputerName();
    test_GetComputerNameExA();
    test_GetComputerNameExW();
    test_GetEnvironmentStringsW();
    test_SetEnvironmentStrings();
}
