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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"

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
    ret_size = GetEnvironmentVariableA(name, NULL, 0);
    ok(ret_size == strlen(value) + 1,
       "should return length with terminating 0 ret_size=%ld\n", ret_size);

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
    SetLastError(0);
    ret_size = GetEnvironmentVariableA(name, buf, lstrlenA(value) + 1);
    ok(ret_size == 0 &&
       ((GetLastError() == 0 && lstrcmpA(buf, "") == 0) ||
        (GetLastError() == ERROR_ENVVAR_NOT_FOUND)),
       "%s should be set to \"\" (NT) or removed (Win9x) but ret_size=%ld GetLastError=%ld and buf=%s\n",
       name, ret_size, GetLastError(), buf);

    /* Test the limits */
    ret_size = GetEnvironmentVariableA(NULL, NULL, 0);
    ok(ret_size == 0 && (GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == ERROR_ENVVAR_NOT_FOUND),
       "should not find variable but ret_size=%ld GetLastError=%ld\n",
       ret_size, GetLastError());

    ret_size = GetEnvironmentVariableA(NULL, buf, lstrlenA(value) + 1);
    ok(ret_size == 0 && (GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == ERROR_ENVVAR_NOT_FOUND),
       "should not find variable but ret_size=%ld GetLastError=%ld\n",
       ret_size, GetLastError());

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
        return;
    }
    ok(ret == TRUE,
       "unexpected error in SetEnvironmentVariableW, GetLastError=%ld\n",
       GetLastError());

    /* Try to retrieve the environment variable we just set */
    ret_size = GetEnvironmentVariableW(name, NULL, 0);
    ok(ret_size == lstrlenW(value) + 1,
       "should return length with terminating 0 ret_size=%ld\n",
       ret_size);

    lstrcpyW(buf, fooW);
    ret_size = GetEnvironmentVariableW(name, buf, lstrlenW(value));
    ok(lstrcmpW(buf, fooW) == 0, "should not touch the buffer\n");

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
    ret_size = GetEnvironmentVariableW(name, buf, lstrlenW(value) + 1);
    ok(ret_size == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND,
       "should not find variable but ret_size=%ld GetLastError=%ld\n",
       ret_size, GetLastError());
    ok(lstrcmpW(buf, empty_strW) == 0, "should copy an empty string\n");

    /* Test the limits */
    ret_size = GetEnvironmentVariableW(NULL, NULL, 0);
    ok(ret_size == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND,
       "should not find variable but ret_size=%ld GetLastError=%ld\n",
       ret_size, GetLastError());

    ret_size = GetEnvironmentVariableW(NULL, buf, lstrlenW(value) + 1);
    ok(ret_size == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND,
       "should not find variable but ret_size=%ld GetLastError=%ld\n",
       ret_size, GetLastError());

    ret = SetEnvironmentVariableW(NULL, NULL);
    ok(ret == FALSE && (GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == ERROR_ENVVAR_NOT_FOUND),
       "should fail with NULL, NULL but ret=%d and GetLastError=%ld\n",
       ret, GetLastError());
}

START_TEST(environ)
{
    test_GetSetEnvironmentVariableA();
    test_GetSetEnvironmentVariableW();
}
