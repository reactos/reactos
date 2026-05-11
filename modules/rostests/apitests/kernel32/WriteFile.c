/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Test for WriteFile
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "precomp.h"

static void Test_WriteFile_sync(HANDLE hFile)
{
    UCHAR Buffer[128];
    DWORD dwBytesWritten;
    BOOL bResult;

    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

    /* Test NULL File */
    SetLastError(0xdeadbeef);
    dwBytesWritten = 0xdeadbeef;
    bResult = WriteFile(NULL, Buffer, sizeof(Buffer), &dwBytesWritten, NULL);
    ok_eq_bool(bResult, FALSE);
    ok_eq_ulong(dwBytesWritten, 0);
    ok_eq_ulong(GetLastError(), ERROR_INVALID_HANDLE);

    /* Test INVALID_HANDLE_VALUE */
    SetLastError(0xdeadbeef);
    dwBytesWritten = 0xdeadbeef;
    bResult = WriteFile(INVALID_HANDLE_VALUE, Buffer, sizeof(Buffer), &dwBytesWritten, NULL);
    ok_eq_bool(bResult, FALSE);
    ok_eq_ulong(dwBytesWritten, 0);
    ok_eq_ulong(GetLastError(), ERROR_INVALID_HANDLE);

    /* Write some bytes */
    SetLastError(0xdeadbeef);
    dwBytesWritten = 0xdeadbeef;
    bResult = WriteFile(hFile, Buffer, sizeof(Buffer), &dwBytesWritten, NULL);
    ok_eq_bool(bResult, TRUE);
    ok_eq_ulong(dwBytesWritten, sizeof(Buffer));
    ok_eq_ulong(GetLastError(), 0xdeadbeef);

    /* Test with NULL lpBuffer */
    SetLastError(0xdeadbeef);
    dwBytesWritten = 0xdeadbeef;
    StartSeh()
        bResult = WriteFile(hFile, NULL, sizeof(Buffer), &dwBytesWritten, NULL);
        ok_eq_bool(bResult, FALSE);
        ok_eq_ulong(dwBytesWritten, 0);
        ok_eq_ulong(GetLastError(), ERROR_INVALID_USER_BUFFER);
    EndSeh(STATUS_SUCCESS);

    /* Test with NULL lpBuffer and 0 nNumberOfBytesToWrite */
    SetLastError(0xdeadbeef);
    dwBytesWritten = 0xdeadbeef;
    StartSeh()
        bResult = WriteFile(hFile, NULL, 0, &dwBytesWritten, NULL);
        ok_eq_bool(bResult, TRUE);
        ok_eq_ulong(dwBytesWritten, 0);
        ok_eq_ulong(GetLastError(), 0xdeadbeef);
    EndSeh(STATUS_SUCCESS);

    /* Test with NULL lpNumberOfBytesWritten */
    SetLastError(0xdeadbeef);
    StartSeh()
        bResult = WriteFile(hFile, Buffer, sizeof(Buffer), NULL, NULL);
        ok_eq_bool(bResult, TRUE);
        ok_eq_ulong(GetLastError(), 0xdeadbeef);
    EndSeh((GetNTVersion() >= _WIN32_WINNT_WIN8) ? STATUS_SUCCESS : EXCEPTION_ACCESS_VIOLATION);
}

static void Test_WriteFile_async(HANDLE hFile)
{
    BOOL bResult;
    UCHAR Buffer[128];
    DWORD dwBytesWritten;
    OVERLAPPED ol = { 0 };

    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

    /* Test NULL File */
    SetLastError(0xdeadbeef);
    dwBytesWritten = 0xdeadbeef;
    bResult = WriteFile(NULL, Buffer, sizeof(Buffer), &dwBytesWritten, &ol);
    ok_eq_bool(bResult, FALSE);
    ok_eq_ulong(dwBytesWritten, 0);
    ok_eq_ulong(GetLastError(), ERROR_INVALID_HANDLE);

    /* Test INVALID_HANDLE_VALUE */
    SetLastError(0xdeadbeef);
    dwBytesWritten = 0xdeadbeef;
    bResult = WriteFile(INVALID_HANDLE_VALUE, Buffer, sizeof(Buffer), &dwBytesWritten, &ol);
    ok_eq_bool(bResult, FALSE);
    ok_eq_ulong(dwBytesWritten, 0);
    ok_eq_ulong(GetLastError(), ERROR_INVALID_HANDLE);

    /* Write some bytes */
    SetLastError(0xdeadbeef);
    dwBytesWritten = 0xdeadbeef;
    bResult = WriteFile(hFile, Buffer, sizeof(Buffer), &dwBytesWritten, &ol);
    ok_eq_bool(bResult, TRUE);
    dwBytesWritten = 0xdeadbeef;
    bResult = GetOverlappedResult(hFile, &ol, &dwBytesWritten, TRUE);
    ok_eq_bool(bResult, TRUE);
    ok_eq_ulong(dwBytesWritten, sizeof(Buffer));
    ok_eq_ulong(GetLastError(), (GetNTVersion() >= _WIN32_WINNT_WIN10) ? 0 : 0xdeadbeef);

    /* Test with NULL lpBuffer */
    SetLastError(0xdeadbeef);
    dwBytesWritten = 0xdeadbeef;
    StartSeh()
        bResult = WriteFile(hFile, NULL, sizeof(Buffer), &dwBytesWritten, &ol);
        ok_eq_bool(bResult, FALSE);
        ok_eq_ulong(GetLastError(), ERROR_INVALID_USER_BUFFER);
    EndSeh(STATUS_SUCCESS);

    /* Test with NULL lpBuffer and 0 nNumberOfBytesToWrite */
    SetLastError(0xdeadbeef);
    dwBytesWritten = 0xdeadbeef;
    StartSeh()
        bResult = WriteFile(hFile, NULL, 0, &dwBytesWritten, &ol);
        ok_eq_bool(bResult, TRUE);
        ok_eq_ulong(GetLastError(), 0xdeadbeef);
        bResult = GetOverlappedResult(hFile, &ol, &dwBytesWritten, TRUE);
        ok_eq_bool(bResult, TRUE);
        ok_eq_ulong(dwBytesWritten, 0);
        ok_eq_ulong(GetLastError(), (GetNTVersion() >= _WIN32_WINNT_WIN10) ? 0 : 0xdeadbeef);
    EndSeh(STATUS_SUCCESS);

    /* Test with NULL lpNumberOfBytesWritten */
    SetLastError(0xdeadbeef);
    bResult = WriteFile(hFile, Buffer, sizeof(Buffer), NULL, &ol);
    ok_eq_bool(bResult, TRUE);
    ok_eq_ulong(GetLastError(), 0xdeadbeef);
    bResult = GetOverlappedResult(hFile, &ol, &dwBytesWritten, TRUE);
    ok_eq_bool(bResult, TRUE);
    ok_eq_ulong(dwBytesWritten, sizeof(Buffer));
    ok_eq_ulong(GetLastError(), (GetNTVersion() >= _WIN32_WINNT_WIN10) ? 0 : 0xdeadbeef);
}

START_TEST(WriteFile)
{
    HANDLE hFile;

    /* Create a temp file name */
    CHAR tempFileName[MAX_PATH];
    if (!GetTempFileNameA(".", "writetest", 0, tempFileName))
    {
        skip("GetTempFileNameA failed with error %lu\n", GetLastError());
        return;
    }

    /* Create a temp file to write to */
    hFile = CreateFileA(tempFileName,
                        GENERIC_WRITE,
                        0,
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE,
                        NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        skip("CreateFileA failed with error %lu\n", GetLastError());
        return;
    }

    Test_WriteFile_sync(hFile);
    Test_WriteFile_async(hFile);

    CloseHandle(hFile);
}
