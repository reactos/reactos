/*
 * Unit tests for SetupIterateCabinet
 *
 * Copyright 2007 Hans Leidekker
 * Copyright 2010 Andrew Nguyen
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

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "setupapi.h"
#include "wine/test.h"

static const BYTE comp_cab_zip_multi[] = {
    0x4d, 0x53, 0x43, 0x46, 0x00, 0x00, 0x00, 0x00, 0x9c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x2c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x01, 0x01, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x71, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x0a, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd1, 0x38, 0xf0, 0x48, 0x20, 0x00, 0x74, 0x72, 0x69, 0x73,
    0x74, 0x72, 0x61, 0x6d, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd1,
    0x38, 0xf0, 0x48, 0x20, 0x00, 0x77, 0x69, 0x6e, 0x65, 0x00, 0x08, 0x00, 0x00, 0x00, 0x18, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xd1, 0x38, 0xf0, 0x48, 0x20, 0x00, 0x73, 0x68, 0x61, 0x6e, 0x64, 0x79,
    0x00, 0x67, 0x2c, 0x03, 0x85, 0x23, 0x00, 0x20, 0x00, 0x43, 0x4b, 0xcb, 0x49, 0x2c, 0x2d, 0x4a,
    0xcd, 0x4b, 0x4e, 0xe5, 0xe5, 0x2a, 0xcd, 0x4b, 0xce, 0xcf, 0x2d, 0x28, 0x4a, 0x2d, 0x2e, 0x4e,
    0x4d, 0xe1, 0xe5, 0x2a, 0x2e, 0x49, 0x2d, 0xca, 0x03, 0x8a, 0x02, 0x00
};

static const WCHAR docW[] = {'d','o','c',0};

static void create_source_fileA(LPSTR filename, const BYTE *data, DWORD size)
{
    HANDLE handle;
    DWORD written;

    handle = CreateFileA(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL, NULL);
    WriteFile(handle, data, size, &written, NULL);
    CloseHandle(handle);
}

static void create_source_fileW(LPWSTR filename, const BYTE *data, DWORD size)
{
    HANDLE handle;
    DWORD written;

    handle = CreateFileW(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL, NULL);
    WriteFile(handle, data, size, &written, NULL);
    CloseHandle(handle);
}

static UINT CALLBACK dummy_callbackA(PVOID Context, UINT Notification,
                                     UINT_PTR Param1, UINT_PTR Param2)
{
    ok(0, "Received unexpected notification (%p, %u, %lu, %lu)\n", Context,
       Notification, Param1, Param2);
    return 0;
}

static UINT CALLBACK dummy_callbackW(PVOID Context, UINT Notification,
                                     UINT_PTR Param1, UINT_PTR Param2)
{
    ok(0, "Received unexpected notification (%p, %u, %lu, %lu)\n", Context,
       Notification, Param1, Param2);
    return 0;
}

static void test_invalid_parametersA(void)
{
    BOOL ret;
    char source[MAX_PATH], temp[MAX_PATH];
    int i;

    const struct
    {
        PCSTR CabinetFile;
        PSP_FILE_CALLBACK_A MsgHandler;
        DWORD expected_lasterror;
        int todo_lasterror;
    } invalid_parameters[] =
    {
        {NULL,                  NULL,            ERROR_INVALID_PARAMETER},
        {NULL,                  dummy_callbackA, ERROR_INVALID_PARAMETER},
        {"c:\\nonexistent.cab", NULL,            ERROR_FILE_NOT_FOUND},
        {"c:\\nonexistent.cab", dummy_callbackA, ERROR_FILE_NOT_FOUND},
        {source,                NULL,            ERROR_INVALID_DATA, 1},
        {source,                dummy_callbackA, ERROR_INVALID_DATA, 1},
    };

    GetTempPathA(sizeof(temp), temp);
    GetTempFileNameA(temp, "doc", 0, source);

    create_source_fileA(source, NULL, 0);

    for (i = 0; i < ARRAY_SIZE(invalid_parameters); i++)
    {
        SetLastError(0xdeadbeef);
        ret = SetupIterateCabinetA(invalid_parameters[i].CabinetFile, 0,
                                   invalid_parameters[i].MsgHandler, NULL);
        ok(!ret, "[%d] Expected SetupIterateCabinetA to return 0, got %d\n", i, ret);
        todo_wine_if (invalid_parameters[i].todo_lasterror)
            ok(GetLastError() == invalid_parameters[i].expected_lasterror,
               "[%d] Expected GetLastError() to return %u, got %u\n",
               i, invalid_parameters[i].expected_lasterror, GetLastError());
    }

    SetLastError(0xdeadbeef);
    ret = SetupIterateCabinetA("", 0, NULL, NULL);
    ok(!ret, "Expected SetupIterateCabinetA to return 0, got %d\n", ret);
    ok(GetLastError() == ERROR_NOT_ENOUGH_MEMORY ||
       GetLastError() == ERROR_FILE_NOT_FOUND, /* Win9x/NT4/Win2k */
       "Expected GetLastError() to return ERROR_NOT_ENOUGH_MEMORY, got %u\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupIterateCabinetA("", 0, dummy_callbackA, NULL);
    ok(!ret, "Expected SetupIterateCabinetA to return 0, got %d\n", ret);
    ok(GetLastError() == ERROR_NOT_ENOUGH_MEMORY ||
       GetLastError() == ERROR_FILE_NOT_FOUND, /* Win9x/NT4/Win2k */
       "Expected GetLastError() to return ERROR_NOT_ENOUGH_MEMORY, got %u\n",
       GetLastError());

    DeleteFileA(source);
}

static void test_invalid_parametersW(void)
{
    static const WCHAR nonexistentW[] = {'c',':','\\','n','o','n','e','x','i','s','t','e','n','t','.','c','a','b',0};
    static const WCHAR emptyW[] = {0};

    BOOL ret;
    WCHAR source[MAX_PATH], temp[MAX_PATH];
    int i;

    const struct
    {
        PCWSTR CabinetFile;
        PSP_FILE_CALLBACK_W MsgHandler;
        DWORD expected_lasterror;
        int todo_lasterror;
    } invalid_parameters[] =
    {
        {nonexistentW, NULL,            ERROR_FILE_NOT_FOUND},
        {nonexistentW, dummy_callbackW, ERROR_FILE_NOT_FOUND},
        {source,       NULL,            ERROR_INVALID_DATA, 1},
        {source,       dummy_callbackW, ERROR_INVALID_DATA, 1},
    };

    ret = SetupIterateCabinetW(NULL, 0, NULL, NULL);
    if (!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("SetupIterateCabinetW is not available\n");
        return;
    }

    GetTempPathW(ARRAY_SIZE(temp), temp);
    GetTempFileNameW(temp, docW, 0, source);

    create_source_fileW(source, NULL, 0);

    for (i = 0; i < ARRAY_SIZE(invalid_parameters); i++)
    {
        SetLastError(0xdeadbeef);
        ret = SetupIterateCabinetW(invalid_parameters[i].CabinetFile, 0,
                                   invalid_parameters[i].MsgHandler, NULL);
        ok(!ret, "[%d] Expected SetupIterateCabinetW to return 0, got %d\n", i, ret);
        todo_wine_if (invalid_parameters[i].todo_lasterror)
            ok(GetLastError() == invalid_parameters[i].expected_lasterror,
               "[%d] Expected GetLastError() to return %u, got %u\n",
               i, invalid_parameters[i].expected_lasterror, GetLastError());
    }

    SetLastError(0xdeadbeef);
    ret = SetupIterateCabinetW(NULL, 0, NULL, NULL);
    ok(!ret, "Expected SetupIterateCabinetW to return 0, got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER ||
       GetLastError() == ERROR_NOT_ENOUGH_MEMORY, /* Vista/Win2k8 */
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %u\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupIterateCabinetW(NULL, 0, dummy_callbackW, NULL);
    ok(!ret, "Expected SetupIterateCabinetW to return 0, got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER ||
       GetLastError() == ERROR_NOT_ENOUGH_MEMORY, /* Vista/Win2k8 */
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %u\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupIterateCabinetW(emptyW, 0, NULL, NULL);
    ok(!ret, "Expected SetupIterateCabinetW to return 0, got %d\n", ret);
    ok(GetLastError() == ERROR_NOT_ENOUGH_MEMORY ||
       GetLastError() == ERROR_FILE_NOT_FOUND, /* NT4/Win2k */
       "Expected GetLastError() to return ERROR_NOT_ENOUGH_MEMORY, got %u\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupIterateCabinetW(emptyW, 0, dummy_callbackW, NULL);
    ok(!ret, "Expected SetupIterateCabinetW to return 0, got %d\n", ret);
    ok(GetLastError() == ERROR_NOT_ENOUGH_MEMORY ||
       GetLastError() == ERROR_FILE_NOT_FOUND, /* NT4/Win2k */
       "Expected GetLastError() to return ERROR_NOT_ENOUGH_MEMORY, got %u\n",
       GetLastError());

    DeleteFileW(source);
}

static UINT CALLBACK crash_callbackA(PVOID Context, UINT Notification,
                                     UINT_PTR Param1, UINT_PTR Param2)
{
    *(volatile char*)0 = 2;
    return 0;
}

static UINT CALLBACK crash_callbackW(PVOID Context, UINT Notification,
                                     UINT_PTR Param1, UINT_PTR Param2)
{
    *(volatile char*)0 = 2;
    return 0;
}

static void test_invalid_callbackA(void)
{
    BOOL ret;
    char source[MAX_PATH], temp[MAX_PATH];

    GetTempPathA(sizeof(temp), temp);
    GetTempFileNameA(temp, "doc", 0, source);

    create_source_fileA(source, comp_cab_zip_multi, sizeof(comp_cab_zip_multi));

    SetLastError(0xdeadbeef);
    ret = SetupIterateCabinetA(source, 0, NULL, NULL);
    ok(!ret, "Expected SetupIterateCabinetA to return 0, got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_DATA,
       "Expected GetLastError() to return ERROR_INVALID_DATA, got %u\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupIterateCabinetA(source, 0, crash_callbackA, NULL);
    ok(!ret, "Expected SetupIterateCabinetA to return 0, got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_DATA,
       "Expected GetLastError() to return ERROR_INVALID_DATA, got %u\n",
       GetLastError());

    DeleteFileA(source);
}

static void test_invalid_callbackW(void)
{
    BOOL ret;
    WCHAR source[MAX_PATH], temp[MAX_PATH];

    ret = SetupIterateCabinetW(NULL, 0, NULL, NULL);
    if (!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("SetupIterateCabinetW is not available\n");
        return;
    }

    GetTempPathW(ARRAY_SIZE(temp), temp);
    GetTempFileNameW(temp, docW, 0, source);

    create_source_fileW(source, comp_cab_zip_multi, sizeof(comp_cab_zip_multi));

    SetLastError(0xdeadbeef);
    ret = SetupIterateCabinetW(source, 0, NULL, NULL);
    ok(!ret, "Expected SetupIterateCabinetW to return 0, got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_DATA,
       "Expected GetLastError() to return ERROR_INVALID_DATA, got %u\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupIterateCabinetW(source, 0, crash_callbackW, NULL);
    ok(!ret, "Expected SetupIterateCabinetW to return 0, got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_DATA,
       "Expected GetLastError() to return ERROR_INVALID_DATA, got %u\n",
       GetLastError());

    DeleteFileW(source);
}

static const char *expected_files[] = {"tristram", "wine", "shandy"};

static UINT CALLBACK simple_callbackA(PVOID Context, UINT Notification,
                                      UINT_PTR Param1, UINT_PTR Param2)
{
    static int index;
    int *file_count = Context;

    switch (Notification)
    {
    case SPFILENOTIFY_CABINETINFO:
        index = 0;
        return NO_ERROR;
    case SPFILENOTIFY_FILEINCABINET:
    {
        FILE_IN_CABINET_INFO_A *info = (FILE_IN_CABINET_INFO_A *)Param1;

        (*file_count)++;

        if (index < ARRAY_SIZE(expected_files))
        {
            ok(!strcmp(expected_files[index], info->NameInCabinet),
               "[%d] Expected file \"%s\", got \"%s\"\n",
               index, expected_files[index], info->NameInCabinet);
            index++;
            return FILEOP_SKIP;
        }
        else
        {
            ok(0, "Unexpectedly enumerated more than number of files in cabinet, index = %d\n", index);
            return FILEOP_ABORT;
        }
    }
    default:
        return NO_ERROR;
    }
}

static void test_simple_enumerationA(void)
{
    BOOL ret;
    char source[MAX_PATH], temp[MAX_PATH];
    int enum_count = 0;

    GetTempPathA(sizeof(temp), temp);
    GetTempFileNameA(temp, "doc", 0, source);

    create_source_fileA(source, comp_cab_zip_multi, sizeof(comp_cab_zip_multi));

    ret = SetupIterateCabinetA(source, 0, simple_callbackA, &enum_count);
    ok(ret == 1, "Expected SetupIterateCabinetA to return 1, got %d\n", ret);
    ok(enum_count == ARRAY_SIZE(expected_files), "Unexpectedly enumerated %d files\n", enum_count);

    DeleteFileA(source);
}

static const WCHAR tristramW[] = {'t','r','i','s','t','r','a','m',0};
static const WCHAR wineW[] = {'w','i','n','e',0};
static const WCHAR shandyW[] = {'s','h','a','n','d','y',0};
static const WCHAR *expected_filesW[] = {tristramW, wineW, shandyW};

static UINT CALLBACK simple_callbackW(PVOID Context, UINT Notification,
                                      UINT_PTR Param1, UINT_PTR Param2)
{
    static int index;
    int *file_count = Context;

    switch (Notification)
    {
    case SPFILENOTIFY_CABINETINFO:
        index = 0;
        return NO_ERROR;
    case SPFILENOTIFY_FILEINCABINET:
    {
        FILE_IN_CABINET_INFO_W *info = (FILE_IN_CABINET_INFO_W *)Param1;

        (*file_count)++;

        if (index < ARRAY_SIZE(expected_filesW))
        {
            ok(!lstrcmpW(expected_filesW[index], info->NameInCabinet),
               "[%d] Expected file %s, got %s\n",
               index, wine_dbgstr_w(expected_filesW[index]), wine_dbgstr_w(info->NameInCabinet));
            index++;
            return FILEOP_SKIP;
        }
        else
        {
            ok(0, "Unexpectedly enumerated more than number of files in cabinet, index = %d\n", index);
            return FILEOP_ABORT;
        }
    }
    default:
        return NO_ERROR;
    }
}

static void test_simple_enumerationW(void)
{
    BOOL ret;
    WCHAR source[MAX_PATH], temp[MAX_PATH];
    int enum_count = 0;

    ret = SetupIterateCabinetW(NULL, 0, NULL, NULL);
    if (!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("SetupIterateCabinetW is not available\n");
        return;
    }

    GetTempPathW(ARRAY_SIZE(temp), temp);
    GetTempFileNameW(temp, docW, 0, source);

    create_source_fileW(source, comp_cab_zip_multi, sizeof(comp_cab_zip_multi));

    ret = SetupIterateCabinetW(source, 0, simple_callbackW, &enum_count);
    ok(ret == 1, "Expected SetupIterateCabinetW to return 1, got %d\n", ret);
    ok(enum_count == ARRAY_SIZE(expected_files), "Unexpectedly enumerated %d files\n", enum_count);

    DeleteFileW(source);
}

START_TEST(setupcab)
{
    test_invalid_parametersA();
    test_invalid_parametersW();

    /* Tests crash on NT4/Win9x/Win2k and Wine. */
    if (0)
    {
        test_invalid_callbackA();
        test_invalid_callbackW();
    }

    test_simple_enumerationA();
    test_simple_enumerationW();
}
