/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Test for LdrLoadDll
 * COPYRIGHT:   Copyright 2017 Thomas Faber <thomas.faber@reactos.org>
 *              Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

/*

NTSTATUS
NTAPI
LdrLoadDll(
    IN PWSTR SearchPath OPTIONAL,
    IN PULONG DllCharacteristics OPTIONAL,
    IN PUNICODE_STRING DllName,
    OUT PVOID *BaseAddress
);

*/

START_TEST(LdrLoadDll)
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING DllName;
    PVOID BaseAddress, BaseAddress2;
    WCHAR szWinDir[MAX_PATH], szSysDir[MAX_PATH], szPath[MAX_PATH];
    WCHAR *pch;

    RtlInitEmptyUnicodeString(&DllName, NULL, 0);
    GetWindowsDirectoryW(szWinDir, _countof(szWinDir));
    GetSystemDirectoryW(szSysDir, _countof(szSysDir));

    Status = 0xDEADFACE;
    StartSeh()
        Status = LdrLoadDll(NULL, NULL, &DllName, NULL);
    EndSeh(STATUS_ACCESS_VIOLATION);
    ok_ntstatus(Status, 0xDEADFACE);

    Status = 0xDEADFACE;
    BaseAddress = InvalidPointer;
    StartSeh()
        Status = LdrLoadDll(NULL, NULL, &DllName, &BaseAddress);
    EndSeh(STATUS_ACCESS_VIOLATION);
    ok_ntstatus(Status, 0xDEADFACE);
    ok_ptr(BaseAddress, InvalidPointer);

    Status = 0xDEADFACE;
    StartSeh()
        Status = LdrLoadDll(L"", NULL, &DllName, NULL);
    EndSeh(STATUS_ACCESS_VIOLATION);
    ok_ntstatus(Status, 0xDEADFACE);

    Status = 0xDEADFACE;
    BaseAddress = InvalidPointer;
    StartSeh()
        Status = LdrLoadDll(L"", NULL, &DllName, &BaseAddress);
    EndSeh(STATUS_ACCESS_VIOLATION);
    ok_ntstatus(Status, 0xDEADFACE);
    ok_ptr(BaseAddress, InvalidPointer);

    RtlInitUnicodeString(&DllName, L"advapi32.dll");

    Status = 0xDEADFACE;
    BaseAddress = InvalidPointer;
    StartSeh()
        Status = LdrLoadDll(NULL, NULL, &DllName, NULL);
        if (NT_SUCCESS(Status))
            LdrUnloadDll(BaseAddress);
    EndSeh(STATUS_ACCESS_VIOLATION);
    ok_ntstatus(Status, 0xDEADFACE);
    ok_ptr(BaseAddress, InvalidPointer);

    RtlInitUnicodeString(&DllName, L"advapi32.dll");
    Status = 0xDEADFACE;
    BaseAddress = InvalidPointer;
    StartSeh()
        BaseAddress = InvalidPointer;
        Status = LdrLoadDll(NULL, NULL, &DllName, &BaseAddress);
        ok_ntstatus(Status, STATUS_SUCCESS);
        ok(BaseAddress != NULL && BaseAddress != InvalidPointer, "BaseAddress = %p\n", BaseAddress);
        if (NT_SUCCESS(Status))
        {
            BaseAddress2 = InvalidPointer;
            Status = LdrLoadDll(NULL, NULL, &DllName, &BaseAddress2);
            ok_ntstatus(Status, STATUS_SUCCESS);
            ok_ptr(BaseAddress2, BaseAddress);
            LdrUnloadDll(BaseAddress);
        }
    EndSeh(STATUS_SUCCESS);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok(BaseAddress != NULL, "BaseAddress was NULL\n");

    RtlInitUnicodeString(&DllName, L"advapi32.dll");
    Status = 0xDEADFACE;
    BaseAddress = InvalidPointer;
    StartSeh()
        Status = LdrLoadDll(L"\\SystemRoot\\System32", NULL, &DllName, NULL);
    EndSeh(STATUS_ACCESS_VIOLATION);
    ok_ntstatus(Status, 0xDEADFACE);
    ok_ptr(BaseAddress, InvalidPointer);

    /* Test with only backslashes in path; no file extension */
    StringCchPrintfW(szPath, _countof(szPath), L"%s\\advapi32", szSysDir);
    RtlInitUnicodeString(&DllName, szPath);
    Status = 0xDEADFACE;
    BaseAddress = InvalidPointer;
    StartSeh()
        Status = LdrLoadDll(szWinDir, NULL, &DllName, &BaseAddress);
        ok_ntstatus(Status, STATUS_SUCCESS);
        if (NT_SUCCESS(Status))
            LdrUnloadDll(BaseAddress);
    EndSeh(STATUS_SUCCESS);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok(BaseAddress != NULL, "BaseAddress was NULL\n");

    /* Test with only backslashes in path; with file extension */
    StringCchPrintfW(szPath, _countof(szPath), L"%s\\advapi32.dll", szSysDir);
    RtlInitUnicodeString(&DllName, szPath);
    Status = 0xDEADFACE;
    BaseAddress = InvalidPointer;
    StartSeh()
        Status = LdrLoadDll(szWinDir, NULL, &DllName, &BaseAddress);
        ok_ntstatus(Status, STATUS_SUCCESS);
        if (NT_SUCCESS(Status))
            LdrUnloadDll(BaseAddress);
    EndSeh(STATUS_SUCCESS);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok(BaseAddress != NULL, "BaseAddress was NULL\n");

    /* Test with one forward slash in path; no file extension */
    StringCchPrintfW(szPath, _countof(szPath), L"%s/advapi32", szSysDir);
    RtlInitUnicodeString(&DllName, szPath);
    Status = 0xDEADFACE;
    BaseAddress = InvalidPointer;
    StartSeh()
        Status = LdrLoadDll(szWinDir, NULL, &DllName, &BaseAddress);
        ok_ntstatus(Status, STATUS_SUCCESS);
        if (NT_SUCCESS(Status))
            LdrUnloadDll(BaseAddress);
    EndSeh(STATUS_SUCCESS);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok(BaseAddress != NULL, "BaseAddress was NULL\n");

    /* Test with one forward slash in path; with file extension */
    StringCchPrintfW(szPath, _countof(szPath), L"%s/advapi32.dll", szSysDir);
    RtlInitUnicodeString(&DllName, szPath);
    Status = 0xDEADFACE;
    BaseAddress = InvalidPointer;
    StartSeh()
        Status = LdrLoadDll(szWinDir, NULL, &DllName, &BaseAddress);
        ok_ntstatus(Status, STATUS_SUCCESS);
        if (NT_SUCCESS(Status))
            LdrUnloadDll(BaseAddress);
    EndSeh(STATUS_SUCCESS);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok(BaseAddress != NULL, "BaseAddress was NULL\n");

    /* Test with only forward slashes in path; no file extension */
    StringCchPrintfW(szPath, _countof(szPath), L"%s\\advapi32", szSysDir);
    for (pch = szPath; *pch != UNICODE_NULL; ++pch)
    {
        if (*pch == L'\\')
            *pch = L'/';
    }

    RtlInitUnicodeString(&DllName, szPath);
    Status = 0xDEADFACE;
    BaseAddress = InvalidPointer;
    StartSeh()
        Status = LdrLoadDll(szWinDir, NULL, &DllName, &BaseAddress);
        ok_ntstatus(Status, STATUS_SUCCESS);
        if (NT_SUCCESS(Status))
            LdrUnloadDll(BaseAddress);
    EndSeh(STATUS_SUCCESS);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok(BaseAddress != NULL, "BaseAddress was NULL\n");

    /* Test with only forward slashes in path; with file extension */
    /* Test with only forward slashes in path; with file extension */
    StringCchPrintfW(szPath, _countof(szPath), L"%s\\advapi32.dll", szSysDir);
    for (pch = szPath; *pch != UNICODE_NULL; ++pch)
    {
        if (*pch == L'\\')
            *pch = L'/';
    }

    RtlInitUnicodeString(&DllName, szPath);
    Status = 0xDEADFACE;
    BaseAddress = InvalidPointer;
    StartSeh()
        Status = LdrLoadDll(szWinDir, NULL, &DllName, &BaseAddress);
        ok_ntstatus(Status, STATUS_SUCCESS);
        if (NT_SUCCESS(Status))
            LdrUnloadDll(BaseAddress);
    EndSeh(STATUS_SUCCESS);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok(BaseAddress != NULL, "BaseAddress was NULL\n");
}
