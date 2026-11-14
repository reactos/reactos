/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Test for DeleteFile and truncation on files that are mapped
 * COPYRIGHT:   Copyright 2026 Mark Jansen <mark.jansen@reactos.org>
 */

#include "precomp.h"

static BOOL CreateTestFile(LPCSTR FileName)
{
    HANDLE hFile;
    CHAR Buffer[0x2000];
    DWORD dwWritten;
    BOOL bResult;

    hFile = CreateFileA(FileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return FALSE;

    FillMemory(Buffer, sizeof(Buffer), 'A');
    bResult = WriteFile(hFile, Buffer, sizeof(Buffer), &dwWritten, NULL);
    CloseHandle(hFile);
    return bResult && dwWritten == sizeof(Buffer);
}

static HANDLE CreateTestMapping(LPCSTR FileName, DWORD Protect)
{
    HANDLE hFile, hMapping;
    DWORD Access = (Protect == PAGE_READWRITE) ? (GENERIC_READ | GENERIC_WRITE) : GENERIC_READ;

    hFile = CreateFileA(FileName, Access, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(hFile != INVALID_HANDLE_VALUE, "CreateFileA failed with %lu\n", GetLastError());
    if (hFile == INVALID_HANDLE_VALUE)
        return NULL;

    hMapping = CreateFileMappingA(hFile, NULL, Protect, 0, 0, NULL);
    ok(hMapping != NULL, "CreateFileMappingA failed with %lu\n", GetLastError());

    /* The file handle is not what keeps the file alive */
    CloseHandle(hFile);
    return hMapping;
}

static void Test_DeleteMappedView(LPCSTR FileName, DWORD Protect, DWORD MapAccess)
{
    HANDLE hMapping;
    PVOID View;
    BOOL bResult;

    if (!CreateTestFile(FileName))
    {
        skip("Unable to create '%s' (%lu)\n", FileName, GetLastError());
        return;
    }

    hMapping = CreateTestMapping(FileName, Protect);
    if (!hMapping)
    {
        DeleteFileA(FileName);
        return;
    }

    View = MapViewOfFile(hMapping, MapAccess, 0, 0, 0);
    ok(View != NULL, "MapViewOfFile failed with %lu\n", GetLastError());
    CloseHandle(hMapping);

    /* Only the view is left */
    SetLastError(0xdeadbeef);
    bResult = DeleteFileA(FileName);
    ok_eq_bool(bResult, FALSE);
    ok_eq_ulong(GetLastError(), ERROR_ACCESS_DENIED);
    ok(GetFileAttributesA(FileName) != INVALID_FILE_ATTRIBUTES, "File is gone\n");

    if (View)
        UnmapViewOfFile(View);

    SetLastError(0xdeadbeef);
    bResult = DeleteFileA(FileName);
    ok_eq_bool(bResult, TRUE);
    ok_eq_ulong(GetLastError(), 0xdeadbeef);
    ok(GetFileAttributesA(FileName) == INVALID_FILE_ATTRIBUTES, "File still exists\n");
}

static void Test_OverwriteMappedView(LPCSTR FileName)
{
    HANDLE hMapping, hFile;
    PVOID View;
    BOOL bResult;

    if (!CreateTestFile(FileName))
    {
        skip("Unable to create '%s' (%lu)\n", FileName, GetLastError());
        return;
    }

    hMapping = CreateTestMapping(FileName, PAGE_READONLY);
    if (!hMapping)
    {
        DeleteFileA(FileName);
        return;
    }

    View = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
    ok(View != NULL, "MapViewOfFile failed with %lu\n", GetLastError());
    CloseHandle(hMapping);

    /* Overwriting truncates the file */
    SetLastError(0xdeadbeef);
    hFile = CreateFileA(FileName, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(hFile == INVALID_HANDLE_VALUE, "CreateFileA succeeded\n");
    ok_eq_ulong(GetLastError(), ERROR_USER_MAPPED_FILE);
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);

    hFile = CreateFileA(FileName, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(hFile != INVALID_HANDLE_VALUE, "CreateFileA failed with %lu\n", GetLastError());
    if (hFile != INVALID_HANDLE_VALUE)
    {
        /* Shrinking is not allowed */
        SetFilePointer(hFile, 0x1000, NULL, FILE_BEGIN);
        SetLastError(0xdeadbeef);
        bResult = SetEndOfFile(hFile);
        ok_eq_bool(bResult, FALSE);
        ok_eq_ulong(GetLastError(), ERROR_USER_MAPPED_FILE);

        /* Extending is */
        SetFilePointer(hFile, 0x3000, NULL, FILE_BEGIN);
        bResult = SetEndOfFile(hFile);
        ok(bResult == TRUE, "SetEndOfFile failed with %lu\n", GetLastError());
        CloseHandle(hFile);
    }

    if (View)
        UnmapViewOfFile(View);

    hFile = CreateFileA(FileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(hFile != INVALID_HANDLE_VALUE, "CreateFileA failed with %lu\n", GetLastError());
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);

    bResult = DeleteFileA(FileName);
    ok(bResult == TRUE, "DeleteFileA failed with %lu\n", GetLastError());
}

static void Test_DeleteMappingNoView(LPCSTR FileName)
{
    HANDLE hMapping;
    BOOL bResult;

    if (!CreateTestFile(FileName))
    {
        skip("Unable to create '%s' (%lu)\n", FileName, GetLastError());
        return;
    }

    hMapping = CreateTestMapping(FileName, PAGE_READONLY);
    if (!hMapping)
    {
        DeleteFileA(FileName);
        return;
    }

    /* Only the section is left */
    SetLastError(0xdeadbeef);
    bResult = DeleteFileA(FileName);
    ok_eq_bool(bResult, FALSE);
    ok_eq_ulong(GetLastError(), ERROR_ACCESS_DENIED);

    CloseHandle(hMapping);

    SetLastError(0xdeadbeef);
    bResult = DeleteFileA(FileName);
    ok_eq_bool(bResult, TRUE);
    ok_eq_ulong(GetLastError(), 0xdeadbeef);
}

static void Test_DeleteCachedFile(LPCSTR FileName)
{
    HANDLE hFile;
    CHAR Buffer[0x100];
    DWORD dwRead;
    BOOL bResult;

    if (!CreateTestFile(FileName))
    {
        skip("Unable to create '%s' (%lu)\n", FileName, GetLastError());
        return;
    }

    /* Cached I/O creates a Cc section, which must not block delete */
    hFile = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE,
                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(hFile != INVALID_HANDLE_VALUE, "CreateFileA failed with %lu\n", GetLastError());
    bResult = ReadFile(hFile, Buffer, sizeof(Buffer), &dwRead, NULL);
    ok_eq_bool(bResult, TRUE);

    SetLastError(0xdeadbeef);
    bResult = DeleteFileA(FileName);
    ok_eq_bool(bResult, TRUE);
    ok_eq_ulong(GetLastError(), 0xdeadbeef);

    CloseHandle(hFile);
    ok(GetFileAttributesA(FileName) == INVALID_FILE_ATTRIBUTES, "File still exists\n");
}

START_TEST(DeleteFile)
{
    CHAR TempPath[MAX_PATH], FileName[MAX_PATH];

    GetTempPathA(_countof(TempPath), TempPath);
    if (!GetTempFileNameA(TempPath, "del", 0, FileName))
    {
        skip("GetTempFileNameA failed with error %lu\n", GetLastError());
        return;
    }

    Test_DeleteMappedView(FileName, PAGE_READONLY, FILE_MAP_READ);
    Test_DeleteMappedView(FileName, PAGE_READWRITE, FILE_MAP_WRITE);
    Test_DeleteMappedView(FileName, PAGE_WRITECOPY, FILE_MAP_COPY);
    Test_OverwriteMappedView(FileName);
    Test_DeleteMappingNoView(FileName);
    Test_DeleteCachedFile(FileName);

    DeleteFileA(FileName);
}
