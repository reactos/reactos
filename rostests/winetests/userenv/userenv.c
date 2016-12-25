/*
 * Unit test suite for userenv functions
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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"

#include "userenv.h"

#include "wine/test.h"

#define expect(EXPECTED,GOT) ok((GOT)==(EXPECTED), "Expected %d, got %d\n", (EXPECTED), (GOT))
#define expect_env(EXPECTED,GOT,VAR) ok((GOT)==(EXPECTED), "Expected %d, got %d for %s (%d)\n", (EXPECTED), (GOT), (VAR), j)
#define expect_gle(EXPECTED) ok(GetLastError() == (EXPECTED), "Expected %d, got %d\n", (EXPECTED), GetLastError())

static BOOL (WINAPI *pIsWow64Process)(HANDLE,PBOOL);

struct profile_item
{
    const char * name;
};

/* Helper function for retrieving environment variables */
static BOOL get_env(const WCHAR * env, const char * var, char ** result)
{
    const WCHAR * p = env;
    int envlen, varlen, buflen;
    char buf[256];

    if (!env || !var || !result) return FALSE;

    varlen = strlen(var);
    do
    {
        if (!WideCharToMultiByte( CP_ACP, 0, p, -1, buf, sizeof(buf), NULL, NULL )) buf[sizeof(buf)-1] = 0;
        envlen = strlen(buf);
        if (CompareStringA(GetThreadLocale(), NORM_IGNORECASE|LOCALE_USE_CP_ACP, buf, min(envlen, varlen), var, varlen) == CSTR_EQUAL)
        {
            if (buf[varlen] == '=')
            {
                buflen = strlen(buf);
                *result = HeapAlloc(GetProcessHeap(), 0, buflen + 1);
                if (!*result) return FALSE;
                memcpy(*result, buf, buflen + 1);
                return TRUE;
            }
        }
        while (*p) p++;
        p++;
    } while (*p);
    return FALSE;
}

static void test_create_env(void)
{
    BOOL r, is_wow64 = FALSE;
    HANDLE htok;
    WCHAR * env[4];
    char * st, systemroot[100];
    int i, j;

    static const struct profile_item common_vars[] = {
        { "ComSpec" },
        { "COMPUTERNAME" },
        { "NUMBER_OF_PROCESSORS" },
        { "OS" },
        { "PROCESSOR_ARCHITECTURE" },
        { "PROCESSOR_IDENTIFIER" },
        { "PROCESSOR_LEVEL" },
        { "PROCESSOR_REVISION" },
        { "SystemDrive" },
        { "SystemRoot" },
        { "windir" }
    };
    static const struct profile_item common_post_nt4_vars[] = {
        { "ALLUSERSPROFILE" },
        { "TEMP" },
        { "TMP" },
        { "CommonProgramFiles" },
        { "ProgramFiles" },
        { "PATH" },
        { "USERPROFILE" }
    };
    static const struct profile_item common_win64_vars[] = {
        { "ProgramW6432" },
        { "CommonProgramW6432" }
    };

    r = SetEnvironmentVariableA("WINE_XYZZY", "ZZYZX");
    expect(TRUE, r);

    r = GetEnvironmentVariableA("SystemRoot", systemroot, sizeof(systemroot));
    ok(r != 0, "GetEnvironmentVariable failed (%d)\n", GetLastError());

    r = SetEnvironmentVariableA("SystemRoot", "overwrite");
    expect(TRUE, r);

    if (0)
    {
        /* Crashes on NT4 */
        r = CreateEnvironmentBlock(NULL, NULL, FALSE);
        expect(FALSE, r);
    }

    r = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY|TOKEN_DUPLICATE, &htok);
    expect(TRUE, r);

    if (0)
    {
        /* Crashes on NT4 */
        r = CreateEnvironmentBlock(NULL, htok, FALSE);
        expect(FALSE, r);
    }

    r = CreateEnvironmentBlock((LPVOID) &env[0], NULL, FALSE);
    expect(TRUE, r);

    r = CreateEnvironmentBlock((LPVOID) &env[1], htok, FALSE);
    expect(TRUE, r);

    r = CreateEnvironmentBlock((LPVOID) &env[2], NULL, TRUE);
    expect(TRUE, r);

    r = CreateEnvironmentBlock((LPVOID) &env[3], htok, TRUE);
    expect(TRUE, r);

    r = SetEnvironmentVariableA("SystemRoot", systemroot);
    expect(TRUE, r);

    for(i=0; i<4; i++)
    {
        r = get_env(env[i], "SystemRoot", &st);
        ok(!strcmp(st, "SystemRoot=overwrite"), "%s\n", st);
        expect(TRUE, r);
        HeapFree(GetProcessHeap(), 0, st);
    }

    /* Test for common environment variables (NT4 and higher) */
    for (i = 0; i < sizeof(common_vars)/sizeof(common_vars[0]); i++)
    {
        for (j = 0; j < 4; j++)
        {
            r = get_env(env[j], common_vars[i].name, &st);
            expect_env(TRUE, r, common_vars[i].name);
            if (r) HeapFree(GetProcessHeap(), 0, st);
        }
    }

    /* Test for common environment variables (post NT4) */
    if (!GetEnvironmentVariableA("ALLUSERSPROFILE", NULL, 0))
    {
        win_skip("Some environment variables are not present on NT4\n");
    }
    else
    {
        for (i = 0; i < sizeof(common_post_nt4_vars)/sizeof(common_post_nt4_vars[0]); i++)
        {
            for (j = 0; j < 4; j++)
            {
                r = get_env(env[j], common_post_nt4_vars[i].name, &st);
                expect_env(TRUE, r, common_post_nt4_vars[i].name);
                if (r) HeapFree(GetProcessHeap(), 0, st);
            }
        }
    }

    if(pIsWow64Process)
        pIsWow64Process(GetCurrentProcess(), &is_wow64);
    if (sizeof(void*)==8 || is_wow64)
    {
        for (i = 0; i < sizeof(common_win64_vars)/sizeof(common_win64_vars[0]); i++)
        {
            for (j=0; j<4; j++)
            {
                r = get_env(env[j], common_win64_vars[i].name, &st);
                ok(r || broken(!r)/* Vista,2k3,XP */, "Expected 1, got 0 for %s\n", common_win64_vars[i].name);
                if (r) HeapFree(GetProcessHeap(), 0, st);
            }
        }
    }

    r = get_env(env[0], "WINE_XYZZY", &st);
    expect(FALSE, r);

    r = get_env(env[1], "WINE_XYZZY", &st);
    expect(FALSE, r);

    r = get_env(env[2], "WINE_XYZZY", &st);
    expect(TRUE, r);
    if (r) HeapFree(GetProcessHeap(), 0, st);

    r = get_env(env[3], "WINE_XYZZY", &st);
    expect(TRUE, r);
    if (r) HeapFree(GetProcessHeap(), 0, st);

    for (i = 0; i < sizeof(env) / sizeof(env[0]); i++)
    {
        r = DestroyEnvironmentBlock(env[i]);
        expect(TRUE, r);
    }
}

static void test_get_profiles_dir(void)
{
    static const char ProfileListA[] = "Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList";
    static const char ProfilesDirectory[] = "ProfilesDirectory";
    BOOL r;
    DWORD cch, profiles_len;
    LONG l;
    HKEY key;
    char *profiles_dir, *buf, small_buf[1];

    l = RegOpenKeyExA(HKEY_LOCAL_MACHINE, ProfileListA, 0, KEY_READ, &key);
    ok(!l, "RegOpenKeyExA failed: %d\n", GetLastError());

    l = RegQueryValueExA(key, ProfilesDirectory, NULL, NULL, NULL, &cch);
    if (l)
    {
        win_skip("No ProfilesDirectory value (NT4), skipping tests\n");
        return;
    }
    buf = HeapAlloc(GetProcessHeap(), 0, cch);
    RegQueryValueExA(key, ProfilesDirectory, NULL, NULL, (BYTE *)buf, &cch);
    RegCloseKey(key);
    profiles_len = ExpandEnvironmentStringsA(buf, NULL, 0);
    profiles_dir = HeapAlloc(GetProcessHeap(), 0, profiles_len);
    ExpandEnvironmentStringsA(buf, profiles_dir, profiles_len);
    HeapFree(GetProcessHeap(), 0, buf);

    SetLastError(0xdeadbeef);
    r = GetProfilesDirectoryA(NULL, NULL);
    expect(FALSE, r);
    expect_gle(ERROR_INVALID_PARAMETER);
    SetLastError(0xdeadbeef);
    r = GetProfilesDirectoryA(NULL, &cch);
    expect(FALSE, r);
    expect_gle(ERROR_INVALID_PARAMETER);
    SetLastError(0xdeadbeef);
    cch = 1;
    r = GetProfilesDirectoryA(small_buf, &cch);
    expect(FALSE, r);
    expect_gle(ERROR_INSUFFICIENT_BUFFER);
    /* MSDN claims the returned character count includes the NULL terminator
     * when the buffer is too small, but that's not in fact what gets returned.
     */
    ok(cch == profiles_len - 1, "expected %d, got %d\n", profiles_len - 1, cch);
    /* Allocate one more character than the return value to prevent a buffer
     * overrun.
     */
    buf = HeapAlloc(GetProcessHeap(), 0, cch + 1);
    r = GetProfilesDirectoryA(buf, &cch);
    /* Rather than a BOOL, the return value is also the number of characters
     * stored in the buffer.
     */
    expect(profiles_len - 1, r);
    ok(!strcmp(buf, profiles_dir), "expected %s, got %s\n", profiles_dir, buf);

    HeapFree(GetProcessHeap(), 0, buf);
    HeapFree(GetProcessHeap(), 0, profiles_dir);

    SetLastError(0xdeadbeef);
    r = GetProfilesDirectoryW(NULL, NULL);
    expect(FALSE, r);
    expect_gle(ERROR_INVALID_PARAMETER);

    cch = 0;
    SetLastError(0xdeadbeef);
    r = GetProfilesDirectoryW(NULL, &cch);
    expect(FALSE, r);
    expect_gle(ERROR_INSUFFICIENT_BUFFER);
    ok(cch, "expected cch > 0\n");

    SetLastError(0xdeadbeef);
    r = GetProfilesDirectoryW(NULL, &cch);
    expect(FALSE, r);
    expect_gle(ERROR_INSUFFICIENT_BUFFER);
}

static void test_get_user_profile_dir(void)
{
    BOOL ret;
    DWORD error, len;
    HANDLE token;
    char *dirA;
    WCHAR *dirW;

    if (!GetEnvironmentVariableA( "ALLUSERSPROFILE", NULL, 0 ))
    {
        win_skip("Skipping tests on NT4\n");
        return;
    }

    ret = OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &token );
    ok(ret, "expected success %u\n", GetLastError());

    SetLastError( 0xdeadbeef );
    ret = GetUserProfileDirectoryA( NULL, NULL, NULL );
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %u\n", error);

    SetLastError( 0xdeadbeef );
    ret = GetUserProfileDirectoryA( token, NULL, NULL );
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %u\n", error);

    dirA = HeapAlloc( GetProcessHeap(), 0, 32 );
    SetLastError( 0xdeadbeef );
    ret = GetUserProfileDirectoryA( token, dirA, NULL );
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %u\n", error);
    HeapFree( GetProcessHeap(), 0, dirA );

    len = 0;
    SetLastError( 0xdeadbeef );
    ret = GetUserProfileDirectoryA( token, NULL, &len );
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %u\n", error);
    ok(!len, "expected 0, got %u\n", len);

    len = 0;
    dirA = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, 32 );
    SetLastError( 0xdeadbeef );
    ret = GetUserProfileDirectoryA( token, dirA, &len );
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_INSUFFICIENT_BUFFER, "expected ERROR_INSUFFICIENT_BUFFER, got %u\n", error);
    ok(len, "expected len > 0\n");
    HeapFree( GetProcessHeap(), 0, dirA );

    dirA = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, len );
    SetLastError( 0xdeadbeef );
    ret = GetUserProfileDirectoryA( token, dirA, &len );
    ok(ret, "expected success %u\n", GetLastError());
    ok(len, "expected len > 0\n");
    ok(lstrlenA( dirA ) == len - 1, "length mismatch %d != %d - 1\n", lstrlenA( dirA ), len );
    trace("%s\n", dirA);
    HeapFree( GetProcessHeap(), 0, dirA );

    SetLastError( 0xdeadbeef );
    ret = GetUserProfileDirectoryW( NULL, NULL, NULL );
    error = GetLastError();
    ok(!ret, "expected failure\n");
    todo_wine ok(error == ERROR_INVALID_HANDLE, "expected ERROR_INVALID_HANDLE, got %u\n", error);

    SetLastError( 0xdeadbeef );
    ret = GetUserProfileDirectoryW( token, NULL, NULL );
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %u\n", error);

    dirW = HeapAlloc( GetProcessHeap(), 0, 32 );
    SetLastError( 0xdeadbeef );
    ret = GetUserProfileDirectoryW( token, dirW, NULL );
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %u\n", error);
    HeapFree( GetProcessHeap(), 0, dirW );

    len = 0;
    SetLastError( 0xdeadbeef );
    ret = GetUserProfileDirectoryW( token, NULL, &len );
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_INSUFFICIENT_BUFFER, "expected ERROR_INSUFFICIENT_BUFFER, got %u\n", error);
    ok(len, "expected len > 0\n");

    dirW = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, len * sizeof(WCHAR) );
    SetLastError( 0xdeadbeef );
    ret = GetUserProfileDirectoryW( token, dirW, &len );
    ok(ret, "expected success %u\n", GetLastError());
    ok(len, "expected len > 0\n");
    ok(lstrlenW( dirW ) == len - 1, "length mismatch %d != %d - 1\n", lstrlenW( dirW ), len );
    HeapFree( GetProcessHeap(), 0, dirW );

    CloseHandle( token );
}

START_TEST(userenv)
{
    pIsWow64Process = (void*)GetProcAddress(GetModuleHandleA("kernel32.dll"), "IsWow64Process");

    test_create_env();
    test_get_profiles_dir();
    test_get_user_profile_dir();
}
