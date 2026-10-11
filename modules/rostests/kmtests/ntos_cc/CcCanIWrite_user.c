/*
 * PROJECT:     ReactOS kernel-mode tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Kernel-Mode Test Suite CcCanIWrite test user-mode part
 * COPYRIGHT:   Copyright 2026 Alex Mendoza <05alex.mendozaa@gmail.com>
 */

#include <kmt_test.h>

START_TEST(CcCanIWrite)
{
    HANDLE Handle;
    NTSTATUS Status;
    LARGE_INTEGER ByteOffset;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    CHAR Buffer = 0;
    UNICODE_STRING TestFile = RTL_CONSTANT_STRING(L"\\Device\\Kmtest-CcCanIWrite\\TestFile");
    DWORD Error;

    Error = KmtLoadAndOpenDriver(L"CcCanIWrite", FALSE);
    ok_eq_int(Error, ERROR_SUCCESS);
    if (Error)
        return;

    InitializeObjectAttributes(&ObjectAttributes, &TestFile, OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = NtOpenFile(&Handle, FILE_ALL_ACCESS, &ObjectAttributes, &IoStatusBlock, 0, FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
    ok_eq_hex(Status, STATUS_SUCCESS);

    if (NT_SUCCESS(Status))
    {
        ByteOffset.QuadPart = 0;
        Status = NtWriteFile(Handle, NULL, NULL, NULL, &IoStatusBlock, &Buffer, sizeof(Buffer), &ByteOffset, NULL);
        ok_eq_hex(Status, STATUS_SUCCESS);

        NtClose(Handle);
    }

    KmtCloseDriver();
    KmtUnloadDriver();
}