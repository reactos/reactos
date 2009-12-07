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

static void init_functionpointers(void)
{
    HMODULE hkernel32 = GetModuleHandleA("kernel32.dll");

    pGetComputerNameExA = (void *)GetProcAddress(hkernel32, "GetComputerNameExA");
    pGetComputerNameExW = (void *)GetProcAddress(hkernel32, "GetComputerNameExW");
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
       "unexpected error in SetEnvironmentVariableA, GetLastError=%d\n",
       GetLastError());

    /* Try to retrieve the environment variable we just set */
    ret_size = GetEnvironmentVariableA(name, NULL, 0);
    ok(ret_size == strlen(value) + 1,
       "should return length with terminating 0 ret_size=%d\n", ret_size);

    lstrcpyA(buf, "foo");
    ret_size = GetEnvironmentVariableA(name, buf, lstrlenA(value));
    ok(lstrcmpA(buf, "foo") == 0, "should not touch the buffer\n");
    ok(ret_size == strlen(value) + 1,
       "should return length with terminating 0 ret_size=%d\n", ret_size);

    lstrcpyA(buf, "foo");
    ret_size = GetEnvironmentVariableA(name, buf, lstrlenA(value) + 1);
    ok(lstrcmpA(buf, value) == 0, "should touch the buffer\n");
    ok(ret_size == strlen(value),
       "should return length without terminating 0 ret_size=%d\n", ret_size);

    lstrcpyA(buf, "foo");
    ret_size = GetEnvironmentVariableA(name_cased, buf, lstrlenA(value) + 1);
    ok(lstrcmpA(buf, value) == 0, "should touch the buffer\n");
    ok(ret_size == strlen(value),
       "should return length without terminating 0 ret_size=%d\n", ret_size);

    /* Remove that environment variable */
    ret = SetEnvironmentVariableA(name_cased, NULL);
    ok(ret == TRUE, "should erase existing variable\n");

    lstrcpyA(buf, "foo");
    ret_size = GetEnvironmentVariableA(name, buf, lstrlenA(value) + 1);
    ok(lstrcmpA(buf, "foo") == 0, "should not touch the buffer\n");
    ok(ret_size == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND,
       "should not find variable but ret_size=%d GetLastError=%d\n",
       ret_size, GetLastError());

    /* Check behavior of SetEnvironmentVariableA(name, "") */
    ret = SetEnvironmentVariableA(name, value);
    ok(ret == TRUE,
       "unexpected error in SetEnvironmentVariableA, GetLastError=%d\n",
       GetLastError());

    lstrcpyA(buf, "foo");
    ret_size = GetEnvironmentVariableA(name_cased, buf, lstrlenA(value) + 1);
    ok(lstrcmpA(buf, value) == 0, "should touch the buffer\n");
    ok(ret_size == strlen(value),
       "should return length without terminating 0 ret_size=%d\n", ret_size);

    ret = SetEnvironmentVariableA(name_cased, "");
    ok(ret == TRUE,
       "should not fail with empty value but GetLastError=%d\n", GetLastError());

    lstrcpyA(buf, "foo");
    SetLastError(0);
    ret_size = GetEnvironmentVariableA(name, buf, lstrlenA(value) + 1);
    ok(ret_size == 0 &&
       ((GetLastError() == 0 && lstrcmpA(buf, "") == 0) ||
        (GetLastError() == ERROR_ENVVAR_NOT_FOUND)),
       "%s should be set to \"\" (NT) or removed (Win9x) but ret_size=%d GetLastError=%d and buf=%s\n",
       name, ret_size, GetLastError(), buf);

    /* Test the limits */
    ret_size = GetEnvironmentVariableA(NULL, NULL, 0);
    ok(ret_size == 0 && (GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == ERROR_ENVVAR_NOT_FOUND),
       "should not find variable but ret_size=%d GetLastError=%d\n",
       ret_size, GetLastError());

    ret_size = GetEnvironmentVariableA(NULL, buf, lstrlenA(value) + 1);
    ok(ret_size == 0 && (GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == ERROR_ENVVAR_NOT_FOUND),
       "should not find variable but ret_size=%d GetLastError=%d\n",
       ret_size, GetLastError());

    ret_size = GetEnvironmentVariableA("", buf, lstrlenA(value) + 1);
    ok(ret_size == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND,
       "should not find variable but ret_size=%d GetLastError=%d\n",
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
       "unexpected error in SetEnvironmentVariableW, GetLastError=%d\n",
       GetLastError());

    /* Try to retrieve the environment variable we just set */
    ret_size = GetEnvironmentVariableW(name, NULL, 0);
    ok(ret_size == lstrlenW(value) + 1,
       "should return length with terminating 0 ret_size=%d\n",
       ret_size);

    lstrcpyW(buf, fooW);
    ret_size = GetEnvironmentVariableW(name, buf, lstrlenW(value));
    ok_w(lstrcmpW(buf, fooW) == 0 ||
         lstrlenW(buf) == 0, /* Vista */
         "Expected untouched or empty buffer, got \"%s\"\n", buf);

    ok(ret_size == lstrlenW(value) + 1,
       "should return length with terminating 0 ret_size=%d\n", ret_size);

    lstrcpyW(buf, fooW);
    ret_size = GetEnvironmentVariableW(name, buf, lstrlenW(value) + 1);
    ok(lstrcmpW(buf, value) == 0, "should touch the buffer\n");
    ok(ret_size == lstrlenW(value),
       "should return length without terminating 0 ret_size=%d\n", ret_size);

    lstrcpyW(buf, fooW);
    ret_size = GetEnvironmentVariableW(name_cased, buf, lstrlenW(value) + 1);
    ok(lstrcmpW(buf, value) == 0, "should touch the buffer\n");
    ok(ret_size == lstrlenW(value),
       "should return length without terminating 0 ret_size=%d\n", ret_size);

    /* Remove that environment variable */
    ret = SetEnvironmentVariableW(name_cased, NULL);
    ok(ret == TRUE, "should erase existing variable\n");

    lstrcpyW(buf, fooW);
    ret_size = GetEnvironmentVariableW(name, buf, lstrlenW(value) + 1);
    ok(lstrcmpW(buf, fooW) == 0, "should not touch the buffer\n");
    ok(ret_size == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND,
       "should not find variable but ret_size=%d GetLastError=%d\n",
       ret_size, GetLastError());

    /* Check behavior of SetEnvironmentVariableW(name, "") */
    ret = SetEnvironmentVariableW(name, value);
    ok(ret == TRUE,
       "unexpected error in SetEnvironmentVariableW, GetLastError=%d\n",
       GetLastError());

    lstrcpyW(buf, fooW);
    ret_size = GetEnvironmentVariableW(name, buf, lstrlenW(value) + 1);
    ok(lstrcmpW(buf, value) == 0, "should touch the buffer\n");
    ok(ret_size == lstrlenW(value),
       "should return length without terminating 0 ret_size=%d\n", ret_size);

    ret = SetEnvironmentVariableW(name_cased, empty_strW);
    ok(ret == TRUE, "should not fail with empty value but GetLastError=%d\n", GetLastError());

    lstrcpyW(buf, fooW);
    ret_size = GetEnvironmentVariableW(name, buf, lstrlenW(value) + 1);
    ok(ret_size == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND,
       "should not find variable but ret_size=%d GetLastError=%d\n",
       ret_size, GetLastError());
    ok(lstrcmpW(buf, empty_strW) == 0, "should copy an empty string\n");

    /* Test the limits */
    ret_size = GetEnvironmentVariableW(NULL, NULL, 0);
    ok(ret_size == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND,
       "should not find variable but ret_size=%d GetLastError=%d\n",
       ret_size, GetLastError());

    if (0) /* Both tests crash on Vista */
    {
        ret_size = GetEnvironmentVariableW(NULL, buf, lstrlenW(value) + 1);
        ok(ret_size == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND,
           "should not find variable but ret_size=%d GetLastError=%d\n",
           ret_size, GetLastError());

        ret = SetEnvironmentVariableW(NULL, NULL);
        ok(ret == FALSE && (GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == ERROR_ENVVAR_NOT_FOUND),
           "should fail with NULL, NULL but ret=%d and GetLastError=%d\n",
           ret, GetLastError());
    }
}

static void test_ExpandEnvironmentStringsA(void)
{
    const char* value="Long long value";
    const char* not_an_env_var="%NotAnEnvVar%";
    char buf[256], buf1[256], buf2[0x8000];
    DWORD ret_size, ret_size1;

    SetEnvironmentVariableA("EnvVar", value);

    ret_size = ExpandEnvironmentStringsA(NULL, buf1, sizeof(buf1));
    ok(ret_size == 1 || ret_size == 0 /* Win9x */ || ret_size == 2 /* NT4 */,
       "ExpandEnvironmentStrings returned %d\n", ret_size);

    /* Try to get the required buffer size 'the natural way' */
    strcpy(buf, "%EnvVar%");
    ret_size = ExpandEnvironmentStringsA(buf, NULL, 0);
    ok(ret_size == strlen(value)+1 || /* win98 */
       ret_size == (strlen(value)+1)*2 || /* NT4 */
       ret_size == strlen(value)+2 || /* win2k, XP, win2k3 */
       ret_size == 0 /* Win95 */,
       "ExpandEnvironmentStrings returned %d instead of %d, %d or %d\n",
       ret_size, lstrlenA(value)+1, lstrlenA(value)+2, 0);

    /* Again, side-stepping the Win95 bug */
    ret_size = ExpandEnvironmentStringsA(buf, buf1, 0);
    /* v5.1.2600.2945 (XP SP2) returns len + 2 here! */
    ok(ret_size == strlen(value)+1 || ret_size == strlen(value)+2 ||
       ret_size == (strlen(value)+1)*2 /* NT4 */,
       "ExpandEnvironmentStrings returned %d instead of %d\n",
       ret_size, lstrlenA(value)+1);

    /* Try with a buffer that's too small */
    ret_size = ExpandEnvironmentStringsA(buf, buf1, 12);
    /* v5.1.2600.2945 (XP SP2) returns len + 2 here! */
    ok(ret_size == strlen(value)+1 || ret_size == strlen(value)+2 ||
       ret_size == (strlen(value)+1)*2 /* NT4 */,
       "ExpandEnvironmentStrings returned %d instead of %d\n",
       ret_size, lstrlenA(value)+1);

    /* Try with a buffer of just the right size */
    /* v5.1.2600.2945 (XP SP2) needs and returns len + 2 here! */
    ret_size = ExpandEnvironmentStringsA(buf, buf1, ret_size);
    ok(ret_size == strlen(value)+1 || ret_size == strlen(value)+2 ||
       ret_size == (strlen(value)+1)*2 /* NT4 */,
       "ExpandEnvironmentStrings returned %d instead of %d\n",
       ret_size, lstrlenA(value)+1);
    ok(!strcmp(buf1, value), "ExpandEnvironmentStrings returned [%s]\n", buf1);

    /* Try with an unset environment variable */
    strcpy(buf, not_an_env_var);
    ret_size = ExpandEnvironmentStringsA(buf, buf1, sizeof(buf1));
    ok(ret_size == strlen(not_an_env_var)+1 ||
       ret_size == (strlen(not_an_env_var)+1)*2 /* NT4 */,
       "ExpandEnvironmentStrings returned %d instead of %d\n", ret_size, lstrlenA(not_an_env_var)+1);
    ok(!strcmp(buf1, not_an_env_var), "ExpandEnvironmentStrings returned [%s]\n", buf1);

    /* test a large destination size */
    strcpy(buf, "12345");
    ret_size = ExpandEnvironmentStringsA(buf, buf2, sizeof(buf2));
    ok(!strcmp(buf, buf2), "ExpandEnvironmentStrings failed %s vs %s. ret_size = %d\n", buf, buf2, ret_size);

    ret_size1 = GetWindowsDirectoryA(buf1,256);
    ok ((ret_size1 >0) && (ret_size1<256), "GetWindowsDirectory Failed\n");
    ret_size = ExpandEnvironmentStringsA("%SystemRoot%",buf,sizeof(buf));
    if (ERROR_ENVVAR_NOT_FOUND != GetLastError())
    {
        ok(!strcmp(buf, buf1), "ExpandEnvironmentStrings failed %s vs %s. ret_size = %d\n", buf, buf1, ret_size);
    }

    /* Try with a variable that references another */
    SetEnvironmentVariableA("IndirectVar", "Foo%EnvVar%Bar");
    strcpy(buf, "Indirect-%IndirectVar%-Indirect");
    strcpy(buf2, "Indirect-Foo%EnvVar%Bar-Indirect");
    ret_size = ExpandEnvironmentStringsA(buf, buf1, sizeof(buf1));
    ok(ret_size == strlen(buf2)+1 ||
       ret_size == (strlen(buf2)+1)*2 /* NT4 */,
       "ExpandEnvironmentStrings returned %d instead of %d\n", ret_size, lstrlen(buf2)+1);
    ok(!strcmp(buf1, buf2), "ExpandEnvironmentStrings returned [%s]\n", buf1);
    SetEnvironmentVariableA("IndirectVar", NULL);

    SetEnvironmentVariableA("EnvVar", NULL);
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
    ret = GetComputerNameA((LPSTR)0xdeadbeef, &size);
    error = GetLastError();
    todo_wine
    ok(!ret && error == ERROR_BUFFER_OVERFLOW, "GetComputerNameA should have failed with ERROR_BUFFER_OVERFLOW instead of %d\n", error);

    /* Only Vista returns the computer name length as documented in the MSDN */
    if (size != 0)
    {
        size++; /* nul terminating character */
        name = HeapAlloc(GetProcessHeap(), 0, size * sizeof(name[0]));
        ok(name != NULL, "HeapAlloc failed with error %d\n", GetLastError());
        ret = GetComputerNameA(name, &size);
        ok(ret, "GetComputerNameA failed with error %d\n", GetLastError());
        HeapFree(GetProcessHeap(), 0, name);
    }

    size = MAX_COMPUTERNAME_LENGTH + 1;
    name = HeapAlloc(GetProcessHeap(), 0, size * sizeof(name[0]));
    ok(name != NULL, "HeapAlloc failed with error %d\n", GetLastError());
    ret = GetComputerNameA(name, &size);
    ok(ret, "GetComputerNameA failed with error %d\n", GetLastError());
    trace("computer name is \"%s\"\n", name);
    name_len = strlen(name);
    ok(size == name_len, "size should be same as length, name_len=%d, size=%d\n", name_len, size);
    HeapFree(GetProcessHeap(), 0, name);

    size = 0;
    SetLastError(0xdeadbeef);
    ret = GetComputerNameW((LPWSTR)0xdeadbeef, &size);
    error = GetLastError();
    if (error == ERROR_CALL_NOT_IMPLEMENTED)
        win_skip("GetComputerNameW is not implemented\n");
    else
    {
        todo_wine
        ok(!ret && error == ERROR_BUFFER_OVERFLOW, "GetComputerNameW should have failed with ERROR_BUFFER_OVERFLOW instead of %d\n", error);
        size++; /* nul terminating character */
        nameW = HeapAlloc(GetProcessHeap(), 0, size * sizeof(nameW[0]));
        ok(nameW != NULL, "HeapAlloc failed with error %d\n", GetLastError());
        ret = GetComputerNameW(nameW, &size);
        ok(ret, "GetComputerNameW failed with error %d\n", GetLastError());
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
    ret = pGetComputerNameExA(ComputerNameDnsDomain, (LPSTR)0xdeadbeef, &size);
    error = GetLastError();
    ok(ret == 0, "Expected 0, got %d\n", ret);
    ok(error == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", error);

    /* size is not set in win2k */
    size = MAX_COMP_NAME;
    name = HeapAlloc(GetProcessHeap(), 0, size * sizeof(name[0]));
    ok(name != NULL, "HeapAlloc failed with error %d\n", GetLastError());
    ret = pGetComputerNameExA(ComputerNameDnsDomain, name, &size);
    ok(ret, "GetComputerNameExA(ComputerNameDnsDomain) failed with error %d\n", GetLastError());
    trace("domain name is \"%s\"\n", name);
    HeapFree(GetProcessHeap(), 0, name);

    size = 0;
    ret = pGetComputerNameExA(ComputerNameDnsFullyQualified, (LPSTR)0xdeadbeef, &size);
    error = GetLastError();
    ok(ret == 0, "Expected 0, got %d\n", ret);
    ok(error == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", error);

    /* size is not set in win2k */
    size = MAX_COMP_NAME;
    name = HeapAlloc(GetProcessHeap(), 0, size * sizeof(name[0]));
    ok(name != NULL, "HeapAlloc failed with error %d\n", GetLastError());
    ret = pGetComputerNameExA(ComputerNameDnsFullyQualified, name, &size);
    ok(ret, "GetComputerNameExA(ComputerNameDnsFullyQualified) failed with error %d\n", GetLastError());
    trace("fully qualified hostname is \"%s\"\n", name);
    HeapFree(GetProcessHeap(), 0, name);

    size = 0;
    ret = pGetComputerNameExA(ComputerNameDnsHostname, (LPSTR)0xdeadbeef, &size);
    error = GetLastError();
    ok(ret == 0, "Expected 0, got %d\n", ret);
    ok(error == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", error);

    /* size is not set in win2k */
    size = MAX_COMP_NAME;
    name = HeapAlloc(GetProcessHeap(), 0, size * sizeof(name[0]));
    ok(name != NULL, "HeapAlloc failed with error %d\n", GetLastError());
    ret = pGetComputerNameExA(ComputerNameDnsHostname, name, &size);
    ok(ret, "GetComputerNameExA(ComputerNameDnsHostname) failed with error %d\n", GetLastError());
    trace("hostname is \"%s\"\n", name);
    HeapFree(GetProcessHeap(), 0, name);

    size = 0;
    ret = pGetComputerNameExA(ComputerNameNetBIOS, (LPSTR)0xdeadbeef, &size);
    error = GetLastError();
    ok(ret == 0, "Expected 0, got %d\n", ret);
    ok(error == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", error);

    /* size is not set in win2k */
    size = MAX_COMP_NAME;
    name = HeapAlloc(GetProcessHeap(), 0, size * sizeof(name[0]));
    ok(name != NULL, "HeapAlloc failed with error %d\n", GetLastError());
    ret = pGetComputerNameExA(ComputerNameNetBIOS, name, &size);
    ok(ret, "GetComputerNameExA(ComputerNameNetBIOS) failed with error %d\n", GetLastError());
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
    ret = pGetComputerNameExW(ComputerNameDnsDomain, (LPWSTR)0xdeadbeef, &size);
    error = GetLastError();
    ok(!ret && error == ERROR_MORE_DATA, "GetComputerNameExW should have failed with ERROR_MORE_DATA instead of %d\n", error);
    nameW = HeapAlloc(GetProcessHeap(), 0, size * sizeof(nameW[0]));
    ok(nameW != NULL, "HeapAlloc failed with error %d\n", GetLastError());
    ret = pGetComputerNameExW(ComputerNameDnsDomain, nameW, &size);
    ok(ret, "GetComputerNameExW(ComputerNameDnsDomain) failed with error %d\n", GetLastError());
    HeapFree(GetProcessHeap(), 0, nameW);

    size = 0;
    ret = pGetComputerNameExW(ComputerNameDnsFullyQualified, (LPWSTR)0xdeadbeef, &size);
    error = GetLastError();
    ok(!ret && error == ERROR_MORE_DATA, "GetComputerNameExW should have failed with ERROR_MORE_DATA instead of %d\n", error);
    nameW = HeapAlloc(GetProcessHeap(), 0, size * sizeof(nameW[0]));
    ok(nameW != NULL, "HeapAlloc failed with error %d\n", GetLastError());
    ret = pGetComputerNameExW(ComputerNameDnsFullyQualified, nameW, &size);
    ok(ret, "GetComputerNameExW(ComputerNameDnsFullyQualified) failed with error %d\n", GetLastError());
    HeapFree(GetProcessHeap(), 0, nameW);

    size = 0;
    ret = pGetComputerNameExW(ComputerNameDnsHostname, (LPWSTR)0xdeadbeef, &size);
    error = GetLastError();
    ok(!ret && error == ERROR_MORE_DATA, "GetComputerNameExW should have failed with ERROR_MORE_DATA instead of %d\n", error);
    nameW = HeapAlloc(GetProcessHeap(), 0, size * sizeof(nameW[0]));
    ok(nameW != NULL, "HeapAlloc failed with error %d\n", GetLastError());
    ret = pGetComputerNameExW(ComputerNameDnsHostname, nameW, &size);
    ok(ret, "GetComputerNameExW(ComputerNameDnsHostname) failed with error %d\n", GetLastError());
    HeapFree(GetProcessHeap(), 0, nameW);

    size = 0;
    ret = pGetComputerNameExW(ComputerNameNetBIOS, (LPWSTR)0xdeadbeef, &size);
    error = GetLastError();
    ok(!ret && error == ERROR_MORE_DATA, "GetComputerNameExW should have failed with ERROR_MORE_DATA instead of %d\n", error);
    nameW = HeapAlloc(GetProcessHeap(), 0, size * sizeof(nameW[0]));
    ok(nameW != NULL, "HeapAlloc failed with error %d\n", GetLastError());
    ret = pGetComputerNameExW(ComputerNameNetBIOS, nameW, &size);
    ok(ret, "GetComputerNameExW(ComputerNameNetBIOS) failed with error %d\n", GetLastError());
    HeapFree(GetProcessHeap(), 0, nameW);
}

START_TEST(environ)
{
    init_functionpointers();

    test_GetSetEnvironmentVariableA();
    test_GetSetEnvironmentVariableW();
    test_ExpandEnvironmentStringsA();
    test_GetComputerName();
    test_GetComputerNameExA();
    test_GetComputerNameExW();
}
