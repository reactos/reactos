/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for NtWriteFile
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include "precomp.h"

static
BOOL
Is64BitSystem(VOID)
{
#ifdef _WIN64
    return TRUE;
#else
    NTSTATUS Status;
    ULONG_PTR IsWow64;

    Status = NtQueryInformationProcess(NtCurrentProcess(),
                                       ProcessWow64Information,
                                       &IsWow64,
                                       sizeof(IsWow64),
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        return IsWow64 != 0;
    }

    return FALSE;
#endif
}

static
ULONG
SizeOfMdl(VOID)
{
    return Is64BitSystem() ? 48 : 28;
}

static
ULONG
SizeOfSector(VOID)
{
    BOOL Ret;
    ULONG SectorSize;

    /* FIXME: Would be better to actually open systemroot */
    Ret = GetDiskFreeSpaceW(NULL, NULL, &SectorSize, NULL, NULL);
    ok(Ret != FALSE, "GetDiskFreeSpaceW failed: %lx\n", GetLastError());
    if (!Ret)
    {
        SectorSize = 4096; /* On failure, assume max size */
    }

    return SectorSize;
}

START_TEST(NtWriteFile)
{
    NTSTATUS Status;
    HANDLE FileHandle;
    UNICODE_STRING FileName = RTL_CONSTANT_STRING(L"\\SystemRoot\\ntdll-apitest-NtWriteFile-test.bin");
    PVOID Buffer;
    SIZE_T BufferSize;
    LARGE_INTEGER ByteOffset;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatus;
    FILE_DISPOSITION_INFORMATION DispositionInfo;
    ULONG TooLargeDataSize = (MAXUSHORT + 1 - SizeOfMdl()) / sizeof(ULONG_PTR) * PAGE_SIZE; // 0x3FF9000 on x86
    ULONG LargeMdlMaxDataSize = TooLargeDataSize - PAGE_SIZE;

    trace("System is %d bits, Size of MDL: %lu\n", Is64BitSystem() ? 64 : 32, SizeOfMdl());
    trace("Max MDL data size: 0x%lx bytes\n", LargeMdlMaxDataSize);

    ByteOffset.QuadPart = 0;

    Buffer = NULL;
    BufferSize = TooLargeDataSize;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &Buffer,
                                     0,
                                     &BufferSize,
                                     MEM_RESERVE | MEM_COMMIT,
                                     PAGE_READONLY);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to allocate memory, status %lx\n", Status);
        return;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &FileName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtCreateFile(&FileHandle,
                          FILE_WRITE_DATA | DELETE | SYNCHRONIZE,
                          &ObjectAttributes,
                          &IoStatus,
                          NULL,
                          0,
                          0,
                          FILE_SUPERSEDE,
                          FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT |
                                                    FILE_NO_INTERMEDIATE_BUFFERING,
                          NULL,
                          0);
    ok_hex(Status, STATUS_SUCCESS);

    /* non-cached, max size -- succeeds */
    Status = NtWriteFile(FileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatus,
                         Buffer,
                         LargeMdlMaxDataSize - PAGE_SIZE,
                         &ByteOffset,
                         NULL);
    ok_hex(Status, STATUS_SUCCESS);

    /* non-cached, max size -- succeeds */
    Status = NtWriteFile(FileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatus,
                         Buffer,
                         LargeMdlMaxDataSize,
                         &ByteOffset,
                         NULL);
    ok_hex(Status, STATUS_SUCCESS);

    /* non-cached, too large -- fails to allocate MDL
     * Note: this returns STATUS_SUCCESS on Win7 -- higher MDL size limit */
    Status = NtWriteFile(FileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatus,
                         Buffer,
                         LargeMdlMaxDataSize + PAGE_SIZE,
                         &ByteOffset,
                         NULL);
    ok_hex(Status, STATUS_INSUFFICIENT_RESOURCES);

    /* non-cached, unaligned -- fails with invalid parameter */
    Status = NtWriteFile(FileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatus,
                         Buffer,
                         LargeMdlMaxDataSize + 1,
                         &ByteOffset,
                         NULL);
    ok_hex(Status, STATUS_INVALID_PARAMETER);

    DispositionInfo.DeleteFile = TRUE;
    Status = NtSetInformationFile(FileHandle,
                                  &IoStatus,
                                  &DispositionInfo,
                                  sizeof(DispositionInfo),
                                  FileDispositionInformation);
    ok_hex(Status, STATUS_SUCCESS);
    Status = NtClose(FileHandle);
    ok_hex(Status, STATUS_SUCCESS);

    Status = NtCreateFile(&FileHandle,
                          FILE_WRITE_DATA | DELETE | SYNCHRONIZE,
                          &ObjectAttributes,
                          &IoStatus,
                          NULL,
                          0,
                          0,
                          FILE_SUPERSEDE,
                          FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL,
                          0);
    ok_hex(Status, STATUS_SUCCESS);

    /* cached: succeeds with arbitrary length */
    Status = NtWriteFile(FileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatus,
                         Buffer,
                         LargeMdlMaxDataSize,
                         &ByteOffset,
                         NULL);
    ok_hex(Status, STATUS_SUCCESS);

    Status = NtWriteFile(FileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatus,
                         Buffer,
                         LargeMdlMaxDataSize + 1,
                         &ByteOffset,
                         NULL);
    ok_hex(Status, STATUS_SUCCESS);

    Status = NtWriteFile(FileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatus,
                         Buffer,
                         TooLargeDataSize,
                         &ByteOffset,
                         NULL);
    ok_hex(Status, STATUS_SUCCESS);

    DispositionInfo.DeleteFile = TRUE;
    Status = NtSetInformationFile(FileHandle,
                                  &IoStatus,
                                  &DispositionInfo,
                                  sizeof(DispositionInfo),
                                  FileDispositionInformation);
    ok_hex(Status, STATUS_SUCCESS);
    Status = NtClose(FileHandle);
    ok_hex(Status, STATUS_SUCCESS);

    Status = NtFreeVirtualMemory(NtCurrentProcess(),
                                 &Buffer,
                                 &BufferSize,
                                 MEM_RELEASE);
    ok_hex(Status, STATUS_SUCCESS);

    /* Now, testing aligned/non aligned writes */

    BufferSize = SizeOfSector();
    trace("Sector is %ld bytes\n", BufferSize);

    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &Buffer,
                                     0,
                                     &BufferSize,
                                     MEM_RESERVE | MEM_COMMIT,
                                     PAGE_READONLY);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to allocate memory, status %lx\n", Status);
        return;
    }

    Status = NtCreateFile(&FileHandle,
                          FILE_WRITE_DATA | DELETE | SYNCHRONIZE,
                          &ObjectAttributes,
                          &IoStatus,
                          NULL,
                          0,
                          0,
                          FILE_SUPERSEDE,
                          FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT |
                                                    FILE_NO_INTERMEDIATE_BUFFERING |
                                                    FILE_WRITE_THROUGH,
                          NULL,
                          0);
    ok_hex(Status, STATUS_SUCCESS);

    /* non-cached, broken length -- fails with invalid parameter */
    ByteOffset.QuadPart = 0;
    Status = NtWriteFile(FileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatus,
                         Buffer,
                         4,
                         &ByteOffset,
                         NULL);
    ok_hex(Status, STATUS_INVALID_PARAMETER);

    /* non-cached, broken offset -- fails with invalid parameter */
    ByteOffset.QuadPart = 4;
    Status = NtWriteFile(FileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatus,
                         Buffer,
                         BufferSize,
                         &ByteOffset,
                         NULL);
    ok_hex(Status, STATUS_INVALID_PARAMETER);

    /* non-cached, good length and offset -- succeeds */
    ByteOffset.QuadPart = 0;
    Status = NtWriteFile(FileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatus,
                         Buffer,
                         BufferSize,
                         &ByteOffset,
                         NULL);
    ok_hex(Status, STATUS_SUCCESS);

    DispositionInfo.DeleteFile = TRUE;
    Status = NtSetInformationFile(FileHandle,
                                  &IoStatus,
                                  &DispositionInfo,
                                  sizeof(DispositionInfo),
                                  FileDispositionInformation);
    ok_hex(Status, STATUS_SUCCESS);
    Status = NtClose(FileHandle);
    ok_hex(Status, STATUS_SUCCESS);

    Status = NtFreeVirtualMemory(NtCurrentProcess(),
                                 &Buffer,
                                 &BufferSize,
                                 MEM_RELEASE);
    ok_hex(Status, STATUS_SUCCESS);
}
