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
#include "winternl.h"

static NTSTATUS (WINAPI *pNtQueryObject)(HANDLE,OBJECT_INFORMATION_CLASS,PVOID,ULONG,PULONG);

static void init(void)
{
    HMODULE hntdll = GetModuleHandleA("ntdll.dll");
    pNtQueryObject = (void *)GetProcAddress(hntdll, "NtQueryObject");
}

#define TEST_GRANTED_ACCESS(a,b) test_granted_access(a,b,__LINE__)
static void test_granted_access(HANDLE handle, ACCESS_MASK access, int line)
{
    OBJECT_BASIC_INFORMATION obj_info;
    NTSTATUS status;

    status = pNtQueryObject(handle, ObjectBasicInformation, &obj_info,
                            sizeof(obj_info), NULL);
    ok_(__FILE__, line)(!status, "NtQueryObject with err: %08lx\n", status);
    ok_(__FILE__, line)(obj_info.GrantedAccess == access, "Granted access should "
        "be 0x%08lx, instead of 0x%08lx\n", access, obj_info.GrantedAccess);
}

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
    if (len_with_null == 0 && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("GetWindowsDirectoryW is not implemented\n");
        return;
    }
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
    if (len_with_null == 0 && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("GetSystemDirectoryW is not available\n");
        return;
    }
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
    WCHAR curdir[MAX_PATH];
    BOOL ret;

    ret = CreateDirectoryA(NULL, NULL);
    ok(ret == FALSE && (GetLastError() == ERROR_PATH_NOT_FOUND ||
                        GetLastError() == ERROR_INVALID_PARAMETER),
       "CreateDirectoryA(NULL): ret=%d err=%ld\n", ret, GetLastError());

    ret = CreateDirectoryA("", NULL);
    ok(ret == FALSE && (GetLastError() == ERROR_BAD_PATHNAME ||
                        GetLastError() == ERROR_PATH_NOT_FOUND),
       "CreateDirectoryA(%s): ret=%d err=%ld\n", tmpdir, ret, GetLastError());

    ret = GetSystemDirectoryA(tmpdir, MAX_PATH);
    ok(ret < MAX_PATH, "System directory should fit into MAX_PATH\n");

    GetCurrentDirectoryW(MAX_PATH, curdir);
    ret = SetCurrentDirectoryA(tmpdir);
    ok(ret == TRUE, "could not chdir to the System directory\n");

    ret = CreateDirectoryA(".", NULL);
    ok(ret == FALSE && GetLastError() == ERROR_ALREADY_EXISTS,
       "CreateDirectoryA(%s): ret=%d err=%ld\n", tmpdir, ret, GetLastError());


    ret = CreateDirectoryA("..", NULL);
    ok(ret == FALSE && GetLastError() == ERROR_ALREADY_EXISTS,
       "CreateDirectoryA(%s): ret=%d err=%ld\n", tmpdir, ret, GetLastError());

    GetTempPathA(MAX_PATH, tmpdir);
    tmpdir[3] = 0; /* truncate the path */
    ret = CreateDirectoryA(tmpdir, NULL);
    ok(ret == FALSE && (GetLastError() == ERROR_ALREADY_EXISTS ||
                        GetLastError() == ERROR_ACCESS_DENIED),
       "CreateDirectoryA(%s): ret=%d err=%ld\n", tmpdir, ret, GetLastError());

    GetTempPathA(MAX_PATH, tmpdir);
    lstrcatA(tmpdir, "Please Remove Me");
    ret = CreateDirectoryA(tmpdir, NULL);
    ok(ret == TRUE,       "CreateDirectoryA(%s) failed err=%ld\n", tmpdir, GetLastError());

    ret = CreateDirectoryA(tmpdir, NULL);
    ok(ret == FALSE && GetLastError() == ERROR_ALREADY_EXISTS,
       "CreateDirectoryA(%s): ret=%d err=%ld\n", tmpdir, ret, GetLastError());

    ret = RemoveDirectoryA(tmpdir);
    ok(ret == TRUE,
       "RemoveDirectoryA(%s) failed err=%ld\n", tmpdir, GetLastError());


    lstrcatA(tmpdir, "?");
    ret = CreateDirectoryA(tmpdir, NULL);
    ok(ret == FALSE && (GetLastError() == ERROR_INVALID_NAME ||
			GetLastError() == ERROR_PATH_NOT_FOUND),
       "CreateDirectoryA(%s): ret=%d err=%ld\n", tmpdir, ret, GetLastError());
    RemoveDirectoryA(tmpdir);

    tmpdir[lstrlenA(tmpdir) - 1] = '*';
    ret = CreateDirectoryA(tmpdir, NULL);
    ok(ret == FALSE && (GetLastError() == ERROR_INVALID_NAME ||
			GetLastError() == ERROR_PATH_NOT_FOUND),
       "CreateDirectoryA(%s): ret=%d err=%ld\n", tmpdir, ret, GetLastError());
    RemoveDirectoryA(tmpdir);

    GetTempPathA(MAX_PATH, tmpdir);
    lstrcatA(tmpdir, "Please Remove Me/Please Remove Me");
    ret = CreateDirectoryA(tmpdir, NULL);
    ok(ret == FALSE && GetLastError() == ERROR_PATH_NOT_FOUND, 
       "CreateDirectoryA(%s): ret=%d err=%ld\n", tmpdir, ret, GetLastError());
    RemoveDirectoryA(tmpdir);

    /* Test behavior with a trailing dot.
     * The directory should be created without the dot.
     */
    GetTempPathA(MAX_PATH, tmpdir);
    lstrcatA(tmpdir, "Please Remove Me.");
    ret = CreateDirectoryA(tmpdir, NULL);
    ok(ret == TRUE,
       "CreateDirectoryA(%s) failed err=%ld\n", tmpdir, GetLastError());

    lstrcatA(tmpdir, "/Please Remove Me");
    ret = CreateDirectoryA(tmpdir, NULL);
    ok(ret == TRUE,
       "CreateDirectoryA(%s) failed err=%ld\n", tmpdir, GetLastError());
    ret = RemoveDirectoryA(tmpdir);
    ok(ret == TRUE,
       "RemoveDirectoryA(%s) failed err=%ld\n", tmpdir, GetLastError());

    GetTempPathA(MAX_PATH, tmpdir);
    lstrcatA(tmpdir, "Please Remove Me");
    ret = RemoveDirectoryA(tmpdir);
    ok(ret == TRUE,
       "RemoveDirectoryA(%s) failed err=%ld\n", tmpdir, GetLastError());

    /* Test behavior with two trailing dots.
     * The directory should be created without the trailing dots.
     */
    GetTempPathA(MAX_PATH, tmpdir);
    lstrcatA(tmpdir, "Please Remove Me..");
    ret = CreateDirectoryA(tmpdir, NULL);
    ok(ret == TRUE,
       "CreateDirectoryA(%s) failed err=%ld\n", tmpdir, GetLastError());

    lstrcatA(tmpdir, "/Please Remove Me");
    ret = CreateDirectoryA(tmpdir, NULL);
    ok(ret == TRUE || /* On Win98 */
       (ret == FALSE && GetLastError() == ERROR_PATH_NOT_FOUND), /* On NT! */
       "CreateDirectoryA(%s): ret=%d err=%ld\n", tmpdir, ret, GetLastError());
    if (ret == TRUE)
    {
        ret = RemoveDirectoryA(tmpdir);
        ok(ret == TRUE,
           "RemoveDirectoryA(%s) failed err=%ld\n", tmpdir, GetLastError());
    }

    GetTempPathA(MAX_PATH, tmpdir);
    lstrcatA(tmpdir, "Please Remove Me");
    ret = RemoveDirectoryA(tmpdir);
    ok(ret == TRUE,
       "RemoveDirectoryA(%s) failed err=%ld\n", tmpdir, GetLastError());

    /* Test behavior with a trailing space.
     * The directory should be created without the trailing space.
     */
    GetTempPathA(MAX_PATH, tmpdir);
    lstrcatA(tmpdir, "Please Remove Me ");
    ret = CreateDirectoryA(tmpdir, NULL);
    ok(ret == TRUE,
       "CreateDirectoryA(%s) failed err=%ld\n", tmpdir, GetLastError());

    lstrcatA(tmpdir, "/Please Remove Me");
    ret = CreateDirectoryA(tmpdir, NULL);
    ok(ret == TRUE || /* On Win98 */
       (ret == FALSE && GetLastError() == ERROR_PATH_NOT_FOUND), /* On NT! */
       "CreateDirectoryA(%s): ret=%d err=%ld\n", tmpdir, ret, GetLastError());
    if (ret == TRUE)
    {
        ret = RemoveDirectoryA(tmpdir);
        ok(ret == TRUE,
           "RemoveDirectoryA(%s) failed err=%ld\n", tmpdir, GetLastError());
    }

    GetTempPathA(MAX_PATH, tmpdir);
    lstrcatA(tmpdir, "Please Remove Me");
    ret = RemoveDirectoryA(tmpdir);
    ok(ret == TRUE,
       "RemoveDirectoryA(%s) failed err=%ld\n", tmpdir, GetLastError());

    /* Test behavior with a trailing space.
     * The directory should be created without the trailing spaces.
     */
    GetTempPathA(MAX_PATH, tmpdir);
    lstrcatA(tmpdir, "Please Remove Me  ");
    ret = CreateDirectoryA(tmpdir, NULL);
    ok(ret == TRUE,
       "CreateDirectoryA(%s) failed err=%ld\n", tmpdir, GetLastError());

    lstrcatA(tmpdir, "/Please Remove Me");
    ret = CreateDirectoryA(tmpdir, NULL);
    ok(ret == TRUE || /* On Win98 */
       (ret == FALSE && GetLastError() == ERROR_PATH_NOT_FOUND), /* On NT! */
       "CreateDirectoryA(%s): ret=%d err=%ld\n", tmpdir, ret, GetLastError());
    if (ret == TRUE)
    {
        ret = RemoveDirectoryA(tmpdir);
        ok(ret == TRUE,
           "RemoveDirectoryA(%s) failed err=%ld\n", tmpdir, GetLastError());
    }

    GetTempPathA(MAX_PATH, tmpdir);
    lstrcatA(tmpdir, "Please Remove Me");
    ret = RemoveDirectoryA(tmpdir);
    ok(ret == TRUE,
       "RemoveDirectoryA(%s) failed err=%ld\n", tmpdir, GetLastError());
    SetCurrentDirectoryW(curdir);
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
    WCHAR curdir[MAX_PATH];

    ret = CreateDirectoryW(NULL, NULL);
    if (!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("CreateDirectoryW is not available\n");
        return;
    }
    ok(ret == FALSE && GetLastError() == ERROR_PATH_NOT_FOUND,
       "should not create NULL path ret %u err %lu\n", ret, GetLastError());

    ret = CreateDirectoryW(empty_strW, NULL);
    ok(ret == FALSE && GetLastError() == ERROR_PATH_NOT_FOUND,
       "should not create empty path ret %u err %lu\n", ret, GetLastError());

    ret = GetSystemDirectoryW(tmpdir, MAX_PATH);
    ok(ret < MAX_PATH, "System directory should fit into MAX_PATH\n");

    GetCurrentDirectoryW(MAX_PATH, curdir);
    ret = SetCurrentDirectoryW(tmpdir);
    ok(ret == TRUE, "could not chdir to the System directory ret %u err %lu\n", ret, GetLastError());

    ret = CreateDirectoryW(dotW, NULL);
    ok(ret == FALSE && GetLastError() == ERROR_ALREADY_EXISTS,
       "should not create existing path ret %u err %lu\n", ret, GetLastError());

    ret = CreateDirectoryW(dotdotW, NULL);
    ok(ret == FALSE && GetLastError() == ERROR_ALREADY_EXISTS,
       "should not create existing path ret %u err %lu\n", ret, GetLastError());

    GetTempPathW(MAX_PATH, tmpdir);
    tmpdir[3] = 0; /* truncate the path */
    ret = CreateDirectoryW(tmpdir, NULL);
    ok(ret == FALSE && (GetLastError() == ERROR_ACCESS_DENIED || GetLastError() == ERROR_ALREADY_EXISTS),
       "should deny access to the drive root ret %u err %lu\n", ret, GetLastError());

    GetTempPathW(MAX_PATH, tmpdir);
    lstrcatW(tmpdir, tmp_dir_name);
    ret = CreateDirectoryW(tmpdir, NULL);
    ok(ret == TRUE, "CreateDirectoryW should always succeed\n");

    ret = CreateDirectoryW(tmpdir, NULL);
    ok(ret == FALSE && GetLastError() == ERROR_ALREADY_EXISTS,
       "should not create existing path ret %u err %lu\n", ret, GetLastError());

    ret = RemoveDirectoryW(tmpdir);
    ok(ret == TRUE, "RemoveDirectoryW should always succeed\n");

    lstrcatW(tmpdir, questionW);
    ret = CreateDirectoryW(tmpdir, NULL);
    ok(ret == FALSE && GetLastError() == ERROR_INVALID_NAME,
       "CreateDirectoryW with ? wildcard name should fail with error 183, ret=%s error=%ld\n",
       ret ? " True" : "False", GetLastError());
    ret = RemoveDirectoryW(tmpdir);
    ok(ret == FALSE, "RemoveDirectoryW should have failed\n");

    tmpdir[lstrlenW(tmpdir) - 1] = '*';
    ret = CreateDirectoryW(tmpdir, NULL);
    ok(ret == FALSE && GetLastError() == ERROR_INVALID_NAME,
       "CreateDirectoryW with * wildcard name should fail with error 183, ret=%s error=%ld\n",
       ret ? " True" : "False", GetLastError());
    ret = RemoveDirectoryW(tmpdir);
    ok(ret == FALSE, "RemoveDirectoryW should have failed\n");
    
    GetTempPathW(MAX_PATH, tmpdir);
    lstrcatW(tmpdir, tmp_dir_name);
    lstrcatW(tmpdir, slashW);
    lstrcatW(tmpdir, tmp_dir_name);
    ret = CreateDirectoryW(tmpdir, NULL);
    ok(ret == FALSE && GetLastError() == ERROR_PATH_NOT_FOUND,
      "CreateDirectoryW with multiple nonexistent directories in path should fail ret %u err %lu\n",
       ret, GetLastError());
    ret = RemoveDirectoryW(tmpdir);
    ok(ret == FALSE, "RemoveDirectoryW should have failed\n");

    SetCurrentDirectoryW(curdir);
}

static void test_RemoveDirectoryA(void)
{
    char curdir[MAX_PATH];
    char tmpdir[MAX_PATH];
    BOOL ret;

    GetTempPathA(MAX_PATH, tmpdir);
    lstrcatA(tmpdir, "Please Remove Me");
    ret = CreateDirectoryA(tmpdir, NULL);
    ok(ret == TRUE, "CreateDirectoryA should always succeed\n");

    GetCurrentDirectoryA(MAX_PATH, curdir);
    ok(SetCurrentDirectoryA(tmpdir), "SetCurrentDirectoryA failed\n");

    SetLastError(0xdeadbeef);
    ok(!RemoveDirectoryA(tmpdir), "RemoveDirectoryA succeeded\n");
    ok(GetLastError() == ERROR_SHARING_VIOLATION,
       "Expected ERROR_SHARING_VIOLATION, got %lu\n", GetLastError());

    TEST_GRANTED_ACCESS(NtCurrentTeb()->Peb->ProcessParameters->CurrentDirectory.Handle,
                        FILE_TRAVERSE | SYNCHRONIZE);

    SetCurrentDirectoryA(curdir);
    ret = RemoveDirectoryA(tmpdir);
    ok(ret == TRUE, "RemoveDirectoryA should always succeed\n");

    lstrcatA(tmpdir, "?");
    ret = RemoveDirectoryA(tmpdir);
    ok(ret == FALSE && (GetLastError() == ERROR_INVALID_NAME ||
			GetLastError() == ERROR_PATH_NOT_FOUND),
       "RemoveDirectoryA with ? wildcard name should fail, ret=%s error=%ld\n",
       ret ? " True" : "False", GetLastError());

    tmpdir[lstrlenA(tmpdir) - 1] = '*';
    ret = RemoveDirectoryA(tmpdir);
    ok(ret == FALSE && (GetLastError() == ERROR_INVALID_NAME ||
			GetLastError() == ERROR_PATH_NOT_FOUND),
       "RemoveDirectoryA with * wildcard name should fail, ret=%s error=%ld\n",
       ret ? " True" : "False", GetLastError());
}

static void test_RemoveDirectoryW(void)
{
    WCHAR curdir[MAX_PATH];
    WCHAR tmpdir[MAX_PATH];
    BOOL ret;
    static const WCHAR tmp_dir_name[] = {'P','l','e','a','s','e',' ','R','e','m','o','v','e',' ','M','e',0};
    static const WCHAR questionW[] = {'?',0};

    GetTempPathW(MAX_PATH, tmpdir);
    lstrcatW(tmpdir, tmp_dir_name);
    ret = CreateDirectoryW(tmpdir, NULL);
    if (!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("CreateDirectoryW is not available\n");
        return;
    }

    ok(ret == TRUE, "CreateDirectoryW should always succeed\n");

    GetCurrentDirectoryW(MAX_PATH, curdir);
    ok(SetCurrentDirectoryW(tmpdir), "SetCurrentDirectoryW failed\n");

    SetLastError(0xdeadbeef);
    ok(!RemoveDirectoryW(tmpdir), "RemoveDirectoryW succeeded\n");
    ok(GetLastError() == ERROR_SHARING_VIOLATION,
       "Expected ERROR_SHARING_VIOLATION, got %lu\n", GetLastError());

    TEST_GRANTED_ACCESS(NtCurrentTeb()->Peb->ProcessParameters->CurrentDirectory.Handle,
                        FILE_TRAVERSE | SYNCHRONIZE);

    SetCurrentDirectoryW(curdir);
    ret = RemoveDirectoryW(tmpdir);
    ok(ret == TRUE, "RemoveDirectoryW should always succeed\n");

    lstrcatW(tmpdir, questionW);
    ret = RemoveDirectoryW(tmpdir);
    ok(ret == FALSE && GetLastError() == ERROR_INVALID_NAME,
       "RemoveDirectoryW with wildcard should fail with error 183, ret=%s error=%ld\n",
       ret ? " True" : "False", GetLastError());

    tmpdir[lstrlenW(tmpdir) - 1] = '*';
    ret = RemoveDirectoryW(tmpdir);
    ok(ret == FALSE && GetLastError() == ERROR_INVALID_NAME,
       "RemoveDirectoryW with * wildcard name should fail with error 183, ret=%s error=%ld\n",
       ret ? " True" : "False", GetLastError());
}

static void test_SetCurrentDirectoryA(void)
{
    SetLastError(0);
    ok( !SetCurrentDirectoryA( "\\some_dummy_dir" ), "SetCurrentDirectoryA succeeded\n" );
    ok( GetLastError() == ERROR_FILE_NOT_FOUND, "wrong error %ld\n", GetLastError() );
    ok( !SetCurrentDirectoryA( "\\some_dummy\\subdir" ), "SetCurrentDirectoryA succeeded\n" );
    ok( GetLastError() == ERROR_PATH_NOT_FOUND, "wrong error %ld\n", GetLastError() );
}

static void test_CreateDirectory_root(void)
{
    static const WCHAR drive_c_root[] = { 'C',':','\\',0 };
    static const WCHAR drive_c[] = { 'C',':',0 };
    char curdir[MAX_PATH];
    BOOL ret;

    GetCurrentDirectoryA(sizeof(curdir), curdir);

    ret = SetCurrentDirectoryA("C:\\");
    ok(ret, "SetCurrentDirectory error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = CreateDirectoryA("C:\\", NULL);
    ok(!ret, "CreateDirectory should fail\n");
    if (GetLastError() == ERROR_ACCESS_DENIED)
    {
        win_skip("not an administrator\n");
        SetCurrentDirectoryA(curdir);
        return;
    }
    ok(GetLastError() == ERROR_ALREADY_EXISTS, "got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = CreateDirectoryA("C:", NULL);
    ok(!ret, "CreateDirectory should fail\n");
    ok(GetLastError() == ERROR_ALREADY_EXISTS, "got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = CreateDirectoryW(drive_c_root, NULL);
    ok(!ret, "CreateDirectory should fail\n");
    ok(GetLastError() == ERROR_ALREADY_EXISTS, "got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = CreateDirectoryW(drive_c, NULL);
    ok(!ret, "CreateDirectory should fail\n");
    ok(GetLastError() == ERROR_ALREADY_EXISTS, "got %lu\n", GetLastError());

    SetCurrentDirectoryA(curdir);
}

START_TEST(directory)
{
    init();

    test_CreateDirectory_root();

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
