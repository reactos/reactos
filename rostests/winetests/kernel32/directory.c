/*
 * Unit test suite for directory functions.
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

/* If you change something in these tests, please do the same
 * for GetSystemDirectory tests.
 */
static void test_GetWindowsDirectoryA(void)
{
    UINT len, len_with_null;
    char buf[MAX_PATH];

    len_with_null = GetWindowsDirectoryA(NULL, 0);
    ok(len_with_null <= MAX_PATH, "should fit into MAX_PATH\n");

    lstrcpyA(buf, "foo");
    len_with_null = GetWindowsDirectoryA(buf, 1);
    ok(lstrcmpA(buf, "foo") == 0, "should not touch the buffer\n");

    lstrcpyA(buf, "foo");
    len = GetWindowsDirectoryA(buf, len_with_null - 1);
    ok(lstrcmpA(buf, "foo") == 0, "should not touch the buffer\n");
    ok(len == len_with_null, "GetWindowsDirectoryW returned %d, expected %d\n",
       len, len_with_null);

    lstrcpyA(buf, "foo");
    len = GetWindowsDirectoryA(buf, len_with_null);
    ok(lstrcmpA(buf, "foo") != 0, "should touch the buffer\n");
    ok(len == strlen(buf), "returned length should be equal to the length of string\n");
    ok(len == len_with_null-1, "GetWindowsDirectoryA returned %d, expected %d\n",
       len, len_with_null-1);
}

static void test_GetWindowsDirectoryW(void)
{
    UINT len, len_with_null;
    WCHAR buf[MAX_PATH];
    static const WCHAR fooW[] = {'f','o','o',0};

    len_with_null = GetWindowsDirectoryW(NULL, 0);
    if (len_with_null==0 && GetLastError()==ERROR_CALL_NOT_IMPLEMENTED)
        return;
    ok(len_with_null <= MAX_PATH, "should fit into MAX_PATH\n");

    lstrcpyW(buf, fooW);
    len = GetWindowsDirectoryW(buf, 1);
    ok(lstrcmpW(buf, fooW) == 0, "should not touch the buffer\n");
    ok(len == len_with_null, "GetWindowsDirectoryW returned %d, expected %d\n",
       len, len_with_null);

    lstrcpyW(buf, fooW);
    len = GetWindowsDirectoryW(buf, len_with_null - 1);
    ok(lstrcmpW(buf, fooW) == 0, "should not touch the buffer\n");
    ok(len == len_with_null, "GetWindowsDirectoryW returned %d, expected %d\n",
       len, len_with_null);

    lstrcpyW(buf, fooW);
    len = GetWindowsDirectoryW(buf, len_with_null);
    ok(lstrcmpW(buf, fooW) != 0, "should touch the buffer\n");
    ok(len == lstrlenW(buf), "returned length should be equal to the length of string\n");
    ok(len == len_with_null-1, "GetWindowsDirectoryW returned %d, expected %d\n",
       len, len_with_null-1);
}


/* If you change something in these tests, please do the same
 * for GetWindowsDirectory tests.
 */
static void test_GetSystemDirectoryA(void)
{
    UINT len, len_with_null;
    char buf[MAX_PATH];

    len_with_null = GetSystemDirectoryA(NULL, 0);
    ok(len_with_null <= MAX_PATH, "should fit into MAX_PATH\n");

    lstrcpyA(buf, "foo");
    len = GetSystemDirectoryA(buf, 1);
    ok(lstrcmpA(buf, "foo") == 0, "should not touch the buffer\n");
    ok(len == len_with_null, "GetSystemDirectoryA returned %d, expected %d\n",
       len, len_with_null);

    lstrcpyA(buf, "foo");
    len = GetSystemDirectoryA(buf, len_with_null - 1);
    ok(lstrcmpA(buf, "foo") == 0, "should not touch the buffer\n");
    ok(len == len_with_null, "GetSystemDirectoryA returned %d, expected %d\n",
       len, len_with_null);

    lstrcpyA(buf, "foo");
    len = GetSystemDirectoryA(buf, len_with_null);
    ok(lstrcmpA(buf, "foo") != 0, "should touch the buffer\n");
    ok(len == strlen(buf), "returned length should be equal to the length of string\n");
    ok(len == len_with_null-1, "GetSystemDirectoryW returned %d, expected %d\n",
       len, len_with_null-1);
}

static void test_GetSystemDirectoryW(void)
{
    UINT len, len_with_null;
    WCHAR buf[MAX_PATH];
    static const WCHAR fooW[] = {'f','o','o',0};

    len_with_null = GetSystemDirectoryW(NULL, 0);
    if (len_with_null==0 && GetLastError()==ERROR_CALL_NOT_IMPLEMENTED)
        return;
    ok(len_with_null <= MAX_PATH, "should fit into MAX_PATH\n");

    lstrcpyW(buf, fooW);
    len = GetSystemDirectoryW(buf, 1);
    ok(lstrcmpW(buf, fooW) == 0, "should not touch the buffer\n");
    ok(len == len_with_null, "GetSystemDirectoryW returned %d, expected %d\n",
       len, len_with_null);

    lstrcpyW(buf, fooW);
    len = GetSystemDirectoryW(buf, len_with_null - 1);
    ok(lstrcmpW(buf, fooW) == 0, "should not touch the buffer\n");
    ok(len == len_with_null, "GetSystemDirectoryW returned %d, expected %d\n",
       len, len_with_null);

    lstrcpyW(buf, fooW);
    len = GetSystemDirectoryW(buf, len_with_null);
    ok(lstrcmpW(buf, fooW) != 0, "should touch the buffer\n");
    ok(len == lstrlenW(buf), "returned length should be equal to the length of string\n");
    ok(len == len_with_null-1, "GetSystemDirectoryW returned %d, expected %d\n",
       len, len_with_null-1);
}

static void test_CreateDirectoryA(void)
{
    char tmpdir[MAX_PATH];
    BOOL ret;

    ret = CreateDirectoryA(NULL, NULL);
    ok(ret == FALSE && (GetLastError() == ERROR_PATH_NOT_FOUND ||
                        GetLastError() == ERROR_INVALID_PARAMETER),
       "CreateDirectoryA(NULL): ret=%d err=%d\n", ret, GetLastError());

    ret = CreateDirectoryA("", NULL);
    ok(ret == FALSE && (GetLastError() == ERROR_BAD_PATHNAME ||
                        GetLastError() == ERROR_PATH_NOT_FOUND),
       "CreateDirectoryA(%s): ret=%d err=%d\n", tmpdir, ret, GetLastError());

    ret = GetSystemDirectoryA(tmpdir, MAX_PATH);
    ok(ret < MAX_PATH, "System directory should fit into MAX_PATH\n");

    ret = SetCurrentDirectoryA(tmpdir);
    ok(ret == TRUE, "could not chdir to the System directory\n");

    ret = CreateDirectoryA(".", NULL);
    ok(ret == FALSE && GetLastError() == ERROR_ALREADY_EXISTS,
       "CreateDirectoryA(%s): ret=%d err=%d\n", tmpdir, ret, GetLastError());


    ret = CreateDirectoryA("..", NULL);
    ok(ret == FALSE && GetLastError() == ERROR_ALREADY_EXISTS,
       "CreateDirectoryA(%s): ret=%d err=%d\n", tmpdir, ret, GetLastError());

    GetTempPathA(MAX_PATH, tmpdir);
    tmpdir[3] = 0; /* truncate the path */
    ret = CreateDirectoryA(tmpdir, NULL);
    ok(ret == FALSE && (GetLastError() == ERROR_ALREADY_EXISTS ||
                        GetLastError() == ERROR_ACCESS_DENIED),
       "CreateDirectoryA(%s): ret=%d err=%d\n", tmpdir, ret, GetLastError());

    GetTempPathA(MAX_PATH, tmpdir);
    lstrcatA(tmpdir, "Please Remove Me");
    ret = CreateDirectoryA(tmpdir, NULL);
    ok(ret == TRUE,       "CreateDirectoryA(%s) failed err=%d\n", tmpdir, GetLastError());

    ret = CreateDirectoryA(tmpdir, NULL);
    ok(ret == FALSE && GetLastError() == ERROR_ALREADY_EXISTS,
       "CreateDirectoryA(%s): ret=%d err=%d\n", tmpdir, ret, GetLastError());

    ret = RemoveDirectoryA(tmpdir);
    ok(ret == TRUE,
       "RemoveDirectoryA(%s) failed err=%d\n", tmpdir, GetLastError());


    lstrcatA(tmpdir, "?");
    ret = CreateDirectoryA(tmpdir, NULL);
    ok(ret == FALSE && (GetLastError() == ERROR_INVALID_NAME ||
			GetLastError() == ERROR_PATH_NOT_FOUND),
       "CreateDirectoryA(%s): ret=%d err=%d\n", tmpdir, ret, GetLastError());
    RemoveDirectoryA(tmpdir);

    tmpdir[lstrlenA(tmpdir) - 1] = '*';
    ret = CreateDirectoryA(tmpdir, NULL);
    ok(ret == FALSE && (GetLastError() == ERROR_INVALID_NAME ||
			GetLastError() == ERROR_PATH_NOT_FOUND),
       "CreateDirectoryA(%s): ret=%d err=%d\n", tmpdir, ret, GetLastError());
    RemoveDirectoryA(tmpdir);

    GetTempPathA(MAX_PATH, tmpdir);
    lstrcatA(tmpdir, "Please Remove Me/Please Remove Me");
    ret = CreateDirectoryA(tmpdir, NULL);
    ok(ret == FALSE && GetLastError() == ERROR_PATH_NOT_FOUND, 
       "CreateDirectoryA(%s): ret=%d err=%d\n", tmpdir, ret, GetLastError());
    RemoveDirectoryA(tmpdir);

    /* Test behavior with a trailing dot.
     * The directory should be created without the dot.
     */
    GetTempPathA(MAX_PATH, tmpdir);
    lstrcatA(tmpdir, "Please Remove Me.");
    ret = CreateDirectoryA(tmpdir, NULL);
    ok(ret == TRUE,
       "CreateDirectoryA(%s) failed err=%d\n", tmpdir, GetLastError());

    lstrcatA(tmpdir, "/Please Remove Me");
    ret = CreateDirectoryA(tmpdir, NULL);
    ok(ret == TRUE,
       "CreateDirectoryA(%s) failed err=%d\n", tmpdir, GetLastError());
    ret = RemoveDirectoryA(tmpdir);
    ok(ret == TRUE,
       "RemoveDirectoryA(%s) failed err=%d\n", tmpdir, GetLastError());

    GetTempPathA(MAX_PATH, tmpdir);
    lstrcatA(tmpdir, "Please Remove Me");
    ret = RemoveDirectoryA(tmpdir);
    ok(ret == TRUE,
       "RemoveDirectoryA(%s) failed err=%d\n", tmpdir, GetLastError());

    /* Test behavior with two trailing dots.
     * The directory should be created without the trailing dots.
     */
    GetTempPathA(MAX_PATH, tmpdir);
    lstrcatA(tmpdir, "Please Remove Me..");
    ret = CreateDirectoryA(tmpdir, NULL);
    ok(ret == TRUE,
       "CreateDirectoryA(%s) failed err=%d\n", tmpdir, GetLastError());

    lstrcatA(tmpdir, "/Please Remove Me");
    ret = CreateDirectoryA(tmpdir, NULL);
    ok(ret == TRUE || /* On Win98 */
       (ret == FALSE && GetLastError() == ERROR_PATH_NOT_FOUND), /* On NT! */
       "CreateDirectoryA(%s): ret=%d err=%d\n", tmpdir, ret, GetLastError());
    if (ret == TRUE)
    {
        ret = RemoveDirectoryA(tmpdir);
        ok(ret == TRUE,
           "RemoveDirectoryA(%s) failed err=%d\n", tmpdir, GetLastError());
    }

    GetTempPathA(MAX_PATH, tmpdir);
    lstrcatA(tmpdir, "Please Remove Me");
    ret = RemoveDirectoryA(tmpdir);
    ok(ret == TRUE,
       "RemoveDirectoryA(%s) failed err=%d\n", tmpdir, GetLastError());

    /* Test behavior with a trailing space.
     * The directory should be created without the trailing space.
     */
    GetTempPathA(MAX_PATH, tmpdir);
    lstrcatA(tmpdir, "Please Remove Me ");
    ret = CreateDirectoryA(tmpdir, NULL);
    ok(ret == TRUE,
       "CreateDirectoryA(%s) failed err=%d\n", tmpdir, GetLastError());

    lstrcatA(tmpdir, "/Please Remove Me");
    ret = CreateDirectoryA(tmpdir, NULL);
    ok(ret == TRUE || /* On Win98 */
       (ret == FALSE && GetLastError() == ERROR_PATH_NOT_FOUND), /* On NT! */
       "CreateDirectoryA(%s): ret=%d err=%d\n", tmpdir, ret, GetLastError());
    if (ret == TRUE)
    {
        ret = RemoveDirectoryA(tmpdir);
        ok(ret == TRUE,
           "RemoveDirectoryA(%s) failed err=%d\n", tmpdir, GetLastError());
    }

    GetTempPathA(MAX_PATH, tmpdir);
    lstrcatA(tmpdir, "Please Remove Me");
    ret = RemoveDirectoryA(tmpdir);
    ok(ret == TRUE,
       "RemoveDirectoryA(%s) failed err=%d\n", tmpdir, GetLastError());

    /* Test behavior with a trailing space.
     * The directory should be created without the trailing spaces.
     */
    GetTempPathA(MAX_PATH, tmpdir);
    lstrcatA(tmpdir, "Please Remove Me  ");
    ret = CreateDirectoryA(tmpdir, NULL);
    ok(ret == TRUE,
       "CreateDirectoryA(%s) failed err=%d\n", tmpdir, GetLastError());

    lstrcatA(tmpdir, "/Please Remove Me");
    ret = CreateDirectoryA(tmpdir, NULL);
    ok(ret == TRUE || /* On Win98 */
       (ret == FALSE && GetLastError() == ERROR_PATH_NOT_FOUND), /* On NT! */
       "CreateDirectoryA(%s): ret=%d err=%d\n", tmpdir, ret, GetLastError());
    if (ret == TRUE)
    {
        ret = RemoveDirectoryA(tmpdir);
        ok(ret == TRUE,
           "RemoveDirectoryA(%s) failed err=%d\n", tmpdir, GetLastError());
    }

    GetTempPathA(MAX_PATH, tmpdir);
    lstrcatA(tmpdir, "Please Remove Me");
    ret = RemoveDirectoryA(tmpdir);
    ok(ret == TRUE,
       "RemoveDirectoryA(%s) failed err=%d\n", tmpdir, GetLastError());
}

static void test_CreateDirectoryW(void)
{
    WCHAR tmpdir[MAX_PATH];
    BOOL ret;
    static const WCHAR empty_strW[] = { 0 };
    static const WCHAR tmp_dir_name[] = {'P','l','e','a','s','e',' ','R','e','m','o','v','e',' ','M','e',0};
    static const WCHAR dotW[] = {'.',0};
    static const WCHAR slashW[] = {'/',0};
    static const WCHAR dotdotW[] = {'.','.',0};
    static const WCHAR questionW[] = {'?',0};

    ret = CreateDirectoryW(NULL, NULL);
    if (!ret && GetLastError()==ERROR_CALL_NOT_IMPLEMENTED)
        return;
    ok(ret == FALSE && GetLastError() == ERROR_PATH_NOT_FOUND,
       "should not create NULL path ret %u err %u\n", ret, GetLastError());

    ret = CreateDirectoryW(empty_strW, NULL);
    ok(ret == FALSE && GetLastError() == ERROR_PATH_NOT_FOUND,
       "should not create empty path ret %u err %u\n", ret, GetLastError());

    ret = GetSystemDirectoryW(tmpdir, MAX_PATH);
    ok(ret < MAX_PATH, "System directory should fit into MAX_PATH\n");

    ret = SetCurrentDirectoryW(tmpdir);
    ok(ret == TRUE, "could not chdir to the System directory ret %u err %u\n", ret, GetLastError());

    ret = CreateDirectoryW(dotW, NULL);
    ok(ret == FALSE && GetLastError() == ERROR_ALREADY_EXISTS,
       "should not create existing path ret %u err %u\n", ret, GetLastError());

    ret = CreateDirectoryW(dotdotW, NULL);
    ok(ret == FALSE && GetLastError() == ERROR_ALREADY_EXISTS,
       "should not create existing path ret %u err %u\n", ret, GetLastError());

    GetTempPathW(MAX_PATH, tmpdir);
    tmpdir[3] = 0; /* truncate the path */
    ret = CreateDirectoryW(tmpdir, NULL);
    ok(ret == FALSE && (GetLastError() == ERROR_ACCESS_DENIED || GetLastError() == ERROR_ALREADY_EXISTS),
       "should deny access to the drive root ret %u err %u\n", ret, GetLastError());

    GetTempPathW(MAX_PATH, tmpdir);
    lstrcatW(tmpdir, tmp_dir_name);
    ret = CreateDirectoryW(tmpdir, NULL);
    ok(ret == TRUE, "CreateDirectoryW should always succeed\n");

    ret = CreateDirectoryW(tmpdir, NULL);
    ok(ret == FALSE && GetLastError() == ERROR_ALREADY_EXISTS,
       "should not create existing path ret %u err %u\n", ret, GetLastError());

    ret = RemoveDirectoryW(tmpdir);
    ok(ret == TRUE, "RemoveDirectoryW should always succeed\n");

    lstrcatW(tmpdir, questionW);
    ret = CreateDirectoryW(tmpdir, NULL);
    ok(ret == FALSE && GetLastError() == ERROR_INVALID_NAME,
       "CreateDirectoryW with ? wildcard name should fail with error 183, ret=%s error=%d\n",
       ret ? " True" : "False", GetLastError());
    ret = RemoveDirectoryW(tmpdir);

    tmpdir[lstrlenW(tmpdir) - 1] = '*';
    ret = CreateDirectoryW(tmpdir, NULL);
    ok(ret == FALSE && GetLastError() == ERROR_INVALID_NAME,
       "CreateDirectoryW with * wildcard name should fail with error 183, ret=%s error=%d\n",
       ret ? " True" : "False", GetLastError());
    ret = RemoveDirectoryW(tmpdir);
    
    GetTempPathW(MAX_PATH, tmpdir);
    lstrcatW(tmpdir, tmp_dir_name);
    lstrcatW(tmpdir, slashW);
    lstrcatW(tmpdir, tmp_dir_name);
    ret = CreateDirectoryW(tmpdir, NULL);
    ok(ret == FALSE && GetLastError() == ERROR_PATH_NOT_FOUND,
      "CreateDirectoryW with multiple nonexistent directories in path should fail ret %u err %u\n",
       ret, GetLastError());
    ret = RemoveDirectoryW(tmpdir);
}

static void test_RemoveDirectoryA(void)
{
    char tmpdir[MAX_PATH];
    BOOL ret;

    GetTempPathA(MAX_PATH, tmpdir);
    lstrcatA(tmpdir, "Please Remove Me");
    ret = CreateDirectoryA(tmpdir, NULL);
    ok(ret == TRUE, "CreateDirectoryA should always succeed\n");

    ret = RemoveDirectoryA(tmpdir);
    ok(ret == TRUE, "RemoveDirectoryA should always succeed\n");

    lstrcatA(tmpdir, "?");
    ret = RemoveDirectoryA(tmpdir);
    ok(ret == FALSE && (GetLastError() == ERROR_INVALID_NAME ||
			GetLastError() == ERROR_PATH_NOT_FOUND),
       "RemoveDirectoryA with ? wildcard name should fail, ret=%s error=%d\n",
       ret ? " True" : "False", GetLastError());

    tmpdir[lstrlenA(tmpdir) - 1] = '*';
    ret = RemoveDirectoryA(tmpdir);
    ok(ret == FALSE && (GetLastError() == ERROR_INVALID_NAME ||
			GetLastError() == ERROR_PATH_NOT_FOUND),
       "RemoveDirectoryA with * wildcard name should fail, ret=%s error=%d\n",
       ret ? " True" : "False", GetLastError());
}

static void test_RemoveDirectoryW(void)
{
    WCHAR tmpdir[MAX_PATH];
    BOOL ret;
    static const WCHAR tmp_dir_name[] = {'P','l','e','a','s','e',' ','R','e','m','o','v','e',' ','M','e',0};
    static const WCHAR questionW[] = {'?',0};

    GetTempPathW(MAX_PATH, tmpdir);
    lstrcatW(tmpdir, tmp_dir_name);
    ret = CreateDirectoryW(tmpdir, NULL);
    if (!ret && GetLastError()==ERROR_CALL_NOT_IMPLEMENTED)
      return;

    ok(ret == TRUE, "CreateDirectoryW should always succeed\n");

    ret = RemoveDirectoryW(tmpdir);
    ok(ret == TRUE, "RemoveDirectoryW should always succeed\n");

    lstrcatW(tmpdir, questionW);
    ret = RemoveDirectoryW(tmpdir);
    ok(ret == FALSE && GetLastError() == ERROR_INVALID_NAME,
       "RemoveDirectoryW with wildcard should fail with error 183, ret=%s error=%d\n",
       ret ? " True" : "False", GetLastError());

    tmpdir[lstrlenW(tmpdir) - 1] = '*';
    ret = RemoveDirectoryW(tmpdir);
    ok(ret == FALSE && GetLastError() == ERROR_INVALID_NAME,
       "RemoveDirectoryW with * wildcard name should fail with error 183, ret=%s error=%d\n",
       ret ? " True" : "False", GetLastError());
}

static void test_SetCurrentDirectoryA(void)
{
    SetLastError(0);
    ok( !SetCurrentDirectoryA( "\\some_dummy_dir" ), "SetCurrentDirectoryA succeeded\n" );
    ok( GetLastError() == ERROR_FILE_NOT_FOUND, "wrong error %d\n", GetLastError() );
    ok( !SetCurrentDirectoryA( "\\some_dummy\\subdir" ), "SetCurrentDirectoryA succeeded\n" );
    ok( GetLastError() == ERROR_PATH_NOT_FOUND, "wrong error %d\n", GetLastError() );
}

START_TEST(directory)
{
    test_GetWindowsDirectoryA();
    test_GetWindowsDirectoryW();

    test_GetSystemDirectoryA();
    test_GetSystemDirectoryW();

    test_CreateDirectoryA();
    test_CreateDirectoryW();

    test_RemoveDirectoryA();
    test_RemoveDirectoryW();

    test_SetCurrentDirectoryA();
}
