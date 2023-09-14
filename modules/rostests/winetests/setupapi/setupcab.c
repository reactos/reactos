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

static const struct
{
    const char *nameA;
    const WCHAR *nameW;
    DWORD size;
}
expected_files[] =
{
    {"tristram", L"tristram", 10},
    {"wine", L"wine", 14},
    {"shandy", L"shandy", 8},
};

static UINT CALLBACK simple_callbackA(void *context, UINT message, UINT_PTR param1, UINT_PTR param2)
{
    static int index;
    int *file_count = context;

    switch (message)
    {
    case SPFILENOTIFY_CABINETINFO:
    {
        const CABINET_INFO_A *info = (const CABINET_INFO_A *)param1;
        char temp[MAX_PATH];

        GetTempPathA(ARRAY_SIZE(temp), temp);
        ok(!strcmp(info->CabinetPath, temp), "Got path %s.\n", debugstr_a(info->CabinetPath));
        todo_wine ok(!info->CabinetFile[0], "Got file %s.\n", debugstr_a(info->CabinetFile));
        ok(!info->DiskName[0], "Got disk name %s.\n", debugstr_a(info->DiskName));
        ok(!info->SetId, "Got set ID %#x.\n", info->SetId);
        ok(!info->CabinetNumber, "Got cabinet number %u.\n", info->CabinetNumber);
        ok(!param2, "Got param2 %#Ix.\n", param2);
        return ERROR_SUCCESS;
    }

    case SPFILENOTIFY_FILEINCABINET:
    {
        FILE_IN_CABINET_INFO_A *info = (FILE_IN_CABINET_INFO_A *)param1;
        char temp[MAX_PATH], path[MAX_PATH];

        (*file_count)++;

        ok(index < ARRAY_SIZE(expected_files), "%u: Got unexpected file.\n", index);
        ok(!strcmp(info->NameInCabinet, expected_files[index].nameA),
                "%u: Got file name %s.\n", index, debugstr_a(info->NameInCabinet));
        ok(info->FileSize == expected_files[index].size, "%u: Got file size %u.\n", index, info->FileSize);
        ok(!info->Win32Error, "%u: Got error %u.\n", index, info->Win32Error);
        ok(info->DosDate == 14545, "%u: Got date %u.\n", index, info->DosDate);
        ok(info->DosTime == 18672, "%u: Got time %u.\n", index, info->DosTime);
        ok(info->DosAttribs == FILE_ATTRIBUTE_ARCHIVE, "%u: Got attributes %#x.\n", index, info->DosAttribs);

        GetTempPathA(ARRAY_SIZE(temp), temp);
        snprintf(path, ARRAY_SIZE(path), "%s/./testcab.cab", temp);
        todo_wine ok(!strcmp((const char *)param2, path), "%u: Got file name %s.\n",
                index, debugstr_a((const char *)param2));

        snprintf(info->FullTargetName, ARRAY_SIZE(info->FullTargetName),
                "%s\\%s", temp, expected_files[index].nameA);

        return FILEOP_DOIT;
    }

    case SPFILENOTIFY_FILEEXTRACTED:
    {
        const FILEPATHS_A *info = (const FILEPATHS_A *)param1;
        char temp[MAX_PATH], path[MAX_PATH];

        GetTempPathA(ARRAY_SIZE(temp), temp);
        ok(index < ARRAY_SIZE(expected_files), "%u: Got unexpected file.\n", index);
        snprintf(path, ARRAY_SIZE(path), "%s/./testcab.cab", temp);
        todo_wine ok(!strcmp(info->Source, path), "%u: Got source %s.\n", index, debugstr_a(info->Source));
        snprintf(path, ARRAY_SIZE(path), "%s\\%s", temp, expected_files[index].nameA);
        ok(!strcmp(info->Target, path), "%u: Got target %s.\n", index, debugstr_a(info->Target));
        ok(!info->Win32Error, "%u: Got error %u.\n", index, info->Win32Error);
        /* info->Flags seems to contain garbage. */

        ok(!param2, "Got param2 %#Ix.\n", param2);
        ++index;
        return ERROR_SUCCESS;
    }

    default:
        ok(0, "Unexpected message %#x.\n", message);
        return ERROR_CALL_NOT_IMPLEMENTED;
    }
}

static void test_simple_enumerationA(void)
{
    BOOL ret;
    char temp[MAX_PATH], path[MAX_PATH];
    unsigned int enum_count = 0, i;

    ret = SetupIterateCabinetA(NULL, 0, NULL, NULL);
    if (!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("SetupIterateCabinetW is not available\n");
        return;
    }

    GetTempPathA(ARRAY_SIZE(temp), temp);
    snprintf(path, ARRAY_SIZE(path), "%s/./testcab.cab", temp);

    create_source_fileA(path, comp_cab_zip_multi, sizeof(comp_cab_zip_multi));

    ret = SetupIterateCabinetA(path, 0, simple_callbackA, &enum_count);
    ok(ret == 1, "Expected SetupIterateCabinetW to return 1, got %d\n", ret);
    ok(enum_count == ARRAY_SIZE(expected_files), "Unexpectedly enumerated %d files\n", enum_count);

    for (i = 0; i < ARRAY_SIZE(expected_files); ++i)
    {
        snprintf(path, ARRAY_SIZE(path), "%s\\%s", temp, expected_files[i].nameA);
        ret = DeleteFileA(path);
        ok(ret, "Failed to delete %s, error %u.\n", debugstr_a(path), GetLastError());
    }

    snprintf(path, ARRAY_SIZE(path), "%s\\testcab.cab", temp);
    ret = DeleteFileA(path);
    ok(ret, "Failed to delete %s, error %u.\n", debugstr_a(path), GetLastError());
}

static UINT CALLBACK simple_callbackW(void *context, UINT message, UINT_PTR param1, UINT_PTR param2)
{
    static int index;
    int *file_count = context;

    switch (message)
    {
    case SPFILENOTIFY_CABINETINFO:
    {
        const CABINET_INFO_W *info = (const CABINET_INFO_W *)param1;
        WCHAR temp[MAX_PATH];

        GetTempPathW(ARRAY_SIZE(temp), temp);
        ok(!wcscmp(info->CabinetPath, temp), "Got path %s.\n", debugstr_w(info->CabinetPath));
        todo_wine ok(!info->CabinetFile[0], "Got file %s.\n", debugstr_w(info->CabinetFile));
        ok(!info->DiskName[0], "Got disk name %s.\n", debugstr_w(info->DiskName));
        ok(!info->SetId, "Got set ID %#x.\n", info->SetId);
        ok(!info->CabinetNumber, "Got cabinet number %u.\n", info->CabinetNumber);
        ok(!param2, "Got param2 %#Ix.\n", param2);
        return ERROR_SUCCESS;
    }

    case SPFILENOTIFY_FILEINCABINET:
    {
        FILE_IN_CABINET_INFO_W *info = (FILE_IN_CABINET_INFO_W *)param1;
        WCHAR temp[MAX_PATH], path[MAX_PATH];

        (*file_count)++;

        ok(index < ARRAY_SIZE(expected_files), "%u: Got unexpected file.\n", index);
        ok(!wcscmp(info->NameInCabinet, expected_files[index].nameW),
                "%u: Got file name %s.\n", index, debugstr_w(info->NameInCabinet));
        ok(info->FileSize == expected_files[index].size, "%u: Got file size %u.\n", index, info->FileSize);
        ok(!info->Win32Error, "%u: Got error %u.\n", index, info->Win32Error);
        ok(info->DosDate == 14545, "%u: Got date %u.\n", index, info->DosDate);
        ok(info->DosTime == 18672, "%u: Got time %u.\n", index, info->DosTime);
        ok(info->DosAttribs == FILE_ATTRIBUTE_ARCHIVE, "%u: Got attributes %#x.\n", index, info->DosAttribs);

        GetTempPathW(ARRAY_SIZE(temp), temp);
        swprintf(path, ARRAY_SIZE(path), L"%s/./testcab.cab", temp);
        todo_wine ok(!wcscmp((const WCHAR *)param2, path), "%u: Got file name %s.\n",
                index, debugstr_w((const WCHAR *)param2));

        swprintf(info->FullTargetName, ARRAY_SIZE(info->FullTargetName),
                L"%s\\%s", temp, expected_files[index].nameW);

        return FILEOP_DOIT;
    }

    case SPFILENOTIFY_FILEEXTRACTED:
    {
        const FILEPATHS_W *info = (const FILEPATHS_W *)param1;
        WCHAR temp[MAX_PATH], path[MAX_PATH];

        GetTempPathW(ARRAY_SIZE(temp), temp);
        ok(index < ARRAY_SIZE(expected_files), "%u: Got unexpected file.\n", index);
        swprintf(path, ARRAY_SIZE(path), L"%s/./testcab.cab", temp);
        todo_wine ok(!wcscmp(info->Source, path), "%u: Got source %s.\n", index, debugstr_w(info->Source));
        swprintf(path, ARRAY_SIZE(path), L"%s\\%s", temp, expected_files[index].nameW);
        ok(!wcscmp(info->Target, path), "%u: Got target %s.\n", index, debugstr_w(info->Target));
        ok(!info->Win32Error, "%u: Got error %u.\n", index, info->Win32Error);
        /* info->Flags seems to contain garbage. */

        ok(!param2, "Got param2 %#Ix.\n", param2);
        ++index;
        return ERROR_SUCCESS;
    }

    default:
        ok(0, "Unexpected message %#x.\n", message);
        return ERROR_CALL_NOT_IMPLEMENTED;
    }
}

static void test_simple_enumerationW(void)
{
    BOOL ret;
    WCHAR temp[MAX_PATH], path[MAX_PATH];
    unsigned int enum_count = 0, i;

    ret = SetupIterateCabinetW(NULL, 0, NULL, NULL);
    if (!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("SetupIterateCabinetW is not available\n");
        return;
    }

    GetTempPathW(ARRAY_SIZE(temp), temp);
    swprintf(path, ARRAY_SIZE(path), L"%s/./testcab.cab", temp);

    create_source_fileW(path, comp_cab_zip_multi, sizeof(comp_cab_zip_multi));

    ret = SetupIterateCabinetW(path, 0, simple_callbackW, &enum_count);
    ok(ret == 1, "Expected SetupIterateCabinetW to return 1, got %d\n", ret);
    ok(enum_count == ARRAY_SIZE(expected_files), "Unexpectedly enumerated %d files\n", enum_count);

    for (i = 0; i < ARRAY_SIZE(expected_files); ++i)
    {
        swprintf(path, ARRAY_SIZE(path), L"%s\\%s", temp, expected_files[i].nameW);
        ret = DeleteFileW(path);
        ok(ret, "Failed to delete %s, error %u.\n", debugstr_w(path), GetLastError());
    }

    swprintf(path, ARRAY_SIZE(path), L"%s\\testcab.cab", temp);
    ret = DeleteFileW(path);
    ok(ret, "Failed to delete %s, error %u.\n", debugstr_w(path), GetLastError());
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
