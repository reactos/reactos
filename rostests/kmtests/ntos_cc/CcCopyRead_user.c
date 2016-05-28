/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite CcCopyRead test user-mode part
 * PROGRAMMER:      Pierre Schweitzer <pierre@reactos.org>
 */

#include <kmt_test.h>

START_TEST(CcCopyRead)
{
    HANDLE Handle;
    NTSTATUS Status;
    LARGE_INTEGER ByteOffset;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PVOID Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, 1024);
    UNICODE_STRING BigAlignmentTest = RTL_CONSTANT_STRING(L"\\Device\\Kmtest-CcCopyRead\\BigAlignmentTest");
    UNICODE_STRING SmallAlignmentTest = RTL_CONSTANT_STRING(L"\\Device\\Kmtest-CcCopyRead\\SmallAlignmentTest");
    
    KmtLoadDriver(L"CcCopyRead", FALSE);
    KmtOpenDriver();

    InitializeObjectAttributes(&ObjectAttributes, &SmallAlignmentTest, OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = NtOpenFile(&Handle, FILE_ALL_ACCESS, &ObjectAttributes, &IoStatusBlock, 0, FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
    ok_eq_hex(Status, STATUS_SUCCESS);

    ByteOffset.QuadPart = 3;
    Status = NtReadFile(Handle, NULL, NULL, NULL, &IoStatusBlock, Buffer, 3, &ByteOffset, NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);

    ByteOffset.QuadPart = 514;
    Status = NtReadFile(Handle, NULL, NULL, NULL, &IoStatusBlock, Buffer, 514, &ByteOffset, NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);

    NtClose(Handle);

    InitializeObjectAttributes(&ObjectAttributes, &BigAlignmentTest, OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = NtOpenFile(&Handle, FILE_ALL_ACCESS, &ObjectAttributes, &IoStatusBlock, 0, FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
    ok_eq_hex(Status, STATUS_SUCCESS);

    ByteOffset.QuadPart = 3;
    Status = NtReadFile(Handle, NULL, NULL, NULL, &IoStatusBlock, Buffer, 3, &ByteOffset, NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);

    ByteOffset.QuadPart = 514;
    Status = NtReadFile(Handle, NULL, NULL, NULL, &IoStatusBlock, Buffer, 514, &ByteOffset, NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);

    ByteOffset.QuadPart = 300000;
    Status = NtReadFile(Handle, NULL, NULL, NULL, &IoStatusBlock, Buffer, 10, &ByteOffset, NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);

    NtClose(Handle);

    RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);
    KmtCloseDriver();
    KmtUnloadDriver();
}
