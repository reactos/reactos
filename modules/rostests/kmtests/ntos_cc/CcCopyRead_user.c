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
    UNICODE_STRING ReallySmallAlignmentTest = RTL_CONSTANT_STRING(L"\\Device\\Kmtest-CcCopyRead\\ReallySmallAlignmentTest");
    UNICODE_STRING FileBig = RTL_CONSTANT_STRING(L"\\Device\\Kmtest-CcCopyRead\\FileBig");
    
    KmtLoadDriver(L"CcCopyRead", FALSE);
    KmtOpenDriver();

    InitializeObjectAttributes(&ObjectAttributes, &SmallAlignmentTest, OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = NtOpenFile(&Handle, FILE_ALL_ACCESS, &ObjectAttributes, &IoStatusBlock, 0, FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
    ok_eq_hex(Status, STATUS_SUCCESS);

    ByteOffset.QuadPart = 3;
    Status = NtReadFile(Handle, NULL, NULL, NULL, &IoStatusBlock, Buffer, 3, &ByteOffset, NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(((USHORT *)Buffer)[0], 0xBABA);

    ByteOffset.QuadPart = 514;
    Status = NtReadFile(Handle, NULL, NULL, NULL, &IoStatusBlock, Buffer, 514, &ByteOffset, NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(((USHORT *)Buffer)[242], 0xBABA);
    ok_eq_hex(((USHORT *)Buffer)[243], 0xFFFF);

    ByteOffset.QuadPart = 1000;
    Status = NtReadFile(Handle, NULL, NULL, NULL, &IoStatusBlock, Buffer, 2, &ByteOffset, NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(((USHORT *)Buffer)[0], 0xFFFF);
    ok_eq_hex(((USHORT *)Buffer)[1], 0xBABA);

    NtClose(Handle);

    InitializeObjectAttributes(&ObjectAttributes, &BigAlignmentTest, OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = NtOpenFile(&Handle, FILE_ALL_ACCESS, &ObjectAttributes, &IoStatusBlock, 0, FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
    ok_eq_hex(Status, STATUS_SUCCESS);

    ByteOffset.QuadPart = 3;
    Status = NtReadFile(Handle, NULL, NULL, NULL, &IoStatusBlock, Buffer, 3, &ByteOffset, NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(((USHORT *)Buffer)[0], 0xBABA);

    ByteOffset.QuadPart = 514;
    Status = NtReadFile(Handle, NULL, NULL, NULL, &IoStatusBlock, Buffer, 514, &ByteOffset, NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(((USHORT *)Buffer)[242], 0xBABA);
    ok_eq_hex(((USHORT *)Buffer)[243], 0xFFFF);

    ByteOffset.QuadPart = 300000;
    Status = NtReadFile(Handle, NULL, NULL, NULL, &IoStatusBlock, Buffer, 10, &ByteOffset, NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(((USHORT *)Buffer)[0], 0xBABA);

    ByteOffset.QuadPart = 999990;
    Status = NtReadFile(Handle, NULL, NULL, NULL, &IoStatusBlock, Buffer, 10, &ByteOffset, NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(((USHORT *)Buffer)[0], 0xBABA);

    ByteOffset.QuadPart = 1000;
    Status = NtReadFile(Handle, NULL, NULL, NULL, &IoStatusBlock, Buffer, 2, &ByteOffset, NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(((USHORT *)Buffer)[0], 0xFFFF);
    ok_eq_hex(((USHORT *)Buffer)[1], 0xBABA);

    NtClose(Handle);

    InitializeObjectAttributes(&ObjectAttributes, &ReallySmallAlignmentTest, OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = NtOpenFile(&Handle, FILE_ALL_ACCESS, &ObjectAttributes, &IoStatusBlock, 0, FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
    ok_eq_hex(Status, STATUS_SUCCESS);

    ByteOffset.QuadPart = 1;
    Status = NtReadFile(Handle, NULL, NULL, NULL, &IoStatusBlock, Buffer, 61, &ByteOffset, NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(((USHORT *)Buffer)[0], 0xBABA);

    NtClose(Handle);

    InitializeObjectAttributes(&ObjectAttributes, &FileBig, OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = NtOpenFile(&Handle, FILE_ALL_ACCESS, &ObjectAttributes, &IoStatusBlock, 0, FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
    ok_eq_hex(Status, STATUS_SUCCESS);

    ByteOffset.QuadPart = 0;
    Status = NtReadFile(Handle, NULL, NULL, NULL, &IoStatusBlock, Buffer, 1024, &ByteOffset, NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(((USHORT *)Buffer)[0], 0xBABA);

    NtClose(Handle);

    RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);
    KmtCloseDriver();
    KmtUnloadDriver();
}
