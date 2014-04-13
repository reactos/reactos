/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite NPFS file information test
 * PROGRAMMER:      Pierre Schweitzer <pierre@reactos.org>
 */

#include <kmt_test.h>
#include "npfs.h"

#define MAX_INSTANCES   1
#define IN_QUOTA        4096
#define OUT_QUOTA       4096
#define PIPE_NAME       L"\\KmtestNpfsFileInfoTestPipe"

static
VOID
TestFileInfo(
    IN HANDLE ServerHandle)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;

    struct {
        FILE_ALL_INFORMATION;
        WCHAR PartialName[50];
    } FileAllInfo;

    RtlFillMemory(&FileAllInfo, sizeof(FileAllInfo), 0xFF);
    Status = ZwQueryInformationFile(ServerHandle,
                                    &IoStatusBlock,
                                    &FileAllInfo,
                                    sizeof(FileAllInfo),
                                    FileAllInformation);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(IoStatusBlock.Status, STATUS_SUCCESS);
    ok_eq_longlong(FileAllInfo.BasicInformation.CreationTime.QuadPart, 0);
    ok_eq_longlong(FileAllInfo.BasicInformation.LastAccessTime.QuadPart, 0);
    ok_eq_longlong(FileAllInfo.BasicInformation.LastWriteTime.QuadPart, 0);
    ok_eq_longlong(FileAllInfo.BasicInformation.ChangeTime.QuadPart, 0);
    ok_eq_ulong(FileAllInfo.BasicInformation.FileAttributes, FILE_ATTRIBUTE_NORMAL);
    ok_eq_longlong(FileAllInfo.StandardInformation.AllocationSize.QuadPart, 8192);
    ok_eq_longlong(FileAllInfo.StandardInformation.EndOfFile.QuadPart, 0);
    ok_eq_ulong(FileAllInfo.StandardInformation.NumberOfLinks, 1);
    ok_bool_true(FileAllInfo.StandardInformation.DeletePending, "DeletePending");
    ok_bool_false(FileAllInfo.StandardInformation.Directory, "Directory");
    ok(FileAllInfo.InternalInformation.IndexNumber.QuadPart != 0xFFFFFFFFFFFFFFFF, "FileAllInfo.InternalInformation.IndexNumber = 0xFFFFFFFFFFFFFFFF, whereas it shouldn't\n");
    ok(FileAllInfo.InternalInformation.IndexNumber.QuadPart != 0, "FileAllInfo.InternalInformation.IndexNumber = 0, whereas it shouldn't\n");
    ok_eq_ulong(FileAllInfo.EaInformation.EaSize, 0);
    ok_eq_ulong(FileAllInfo.AccessInformation.AccessFlags, (FILE_GENERIC_READ | FILE_GENERIC_WRITE));
    ok_eq_longlong(FileAllInfo.PositionInformation.CurrentByteOffset.QuadPart, 0);
    ok_eq_ulong(FileAllInfo.ModeInformation.Mode, FILE_SYNCHRONOUS_IO_NONALERT);
    ok_eq_ulong(FileAllInfo.AlignmentInformation.AlignmentRequirement, 0);
    ok_eq_ulong(FileAllInfo.NameInformation.FileNameLength, sizeof(PIPE_NAME) - sizeof(WCHAR));
    ok_eq_size(RtlCompareMemory(FileAllInfo.NameInformation.FileName, PIPE_NAME, sizeof(PIPE_NAME) - sizeof(WCHAR)), (sizeof(PIPE_NAME) - sizeof(WCHAR)));
    ok_eq_wchar(FileAllInfo.NameInformation.FileName[sizeof(PIPE_NAME) / sizeof(WCHAR) - 1], 0xFFFF);
    ok_eq_ulong(IoStatusBlock.Information, (FIELD_OFFSET(FILE_ALL_INFORMATION, NameInformation.FileName) + sizeof(PIPE_NAME) - sizeof(WCHAR)));

    RtlFillMemory(&FileAllInfo, sizeof(FileAllInfo), 0xFF);
    Status = ZwQueryInformationFile(ServerHandle,
                                    &IoStatusBlock,
                                    &FileAllInfo,
                                    sizeof(FILE_ALL_INFORMATION) + 4 * sizeof(WCHAR),
                                    FileAllInformation);
    ok_eq_hex(Status, STATUS_BUFFER_OVERFLOW);
    ok_eq_hex(IoStatusBlock.Status, STATUS_BUFFER_OVERFLOW);
    ok_eq_longlong(FileAllInfo.BasicInformation.CreationTime.QuadPart, 0);
    ok_eq_longlong(FileAllInfo.BasicInformation.LastAccessTime.QuadPart, 0);
    ok_eq_longlong(FileAllInfo.BasicInformation.LastWriteTime.QuadPart, 0);
    ok_eq_longlong(FileAllInfo.BasicInformation.ChangeTime.QuadPart, 0);
    ok_eq_ulong(FileAllInfo.BasicInformation.FileAttributes, FILE_ATTRIBUTE_NORMAL);
    ok_eq_longlong(FileAllInfo.StandardInformation.AllocationSize.QuadPart, 8192);
    ok_eq_longlong(FileAllInfo.StandardInformation.EndOfFile.QuadPart, 0);
    ok_eq_ulong(FileAllInfo.StandardInformation.NumberOfLinks, 1);
    ok_bool_true(FileAllInfo.StandardInformation.DeletePending, "DeletePending");
    ok_bool_false(FileAllInfo.StandardInformation.Directory, "Directory");
    ok(FileAllInfo.InternalInformation.IndexNumber.QuadPart != 0xFFFFFFFFFFFFFFFF, "FileAllInfo.InternalInformation.IndexNumber = 0xFFFFFFFFFFFFFFFF, whereas it shouldn't\n");
    ok(FileAllInfo.InternalInformation.IndexNumber.QuadPart != 0, "FileAllInfo.InternalInformation.IndexNumber = 0, whereas it shouldn't\n");
    ok_eq_ulong(FileAllInfo.EaInformation.EaSize, 0);
    ok_eq_ulong(FileAllInfo.AccessInformation.AccessFlags, (FILE_GENERIC_READ | FILE_GENERIC_WRITE));
    ok_eq_longlong(FileAllInfo.PositionInformation.CurrentByteOffset.QuadPart, 0);
    ok_eq_ulong(FileAllInfo.ModeInformation.Mode, FILE_SYNCHRONOUS_IO_NONALERT);
    ok_eq_ulong(FileAllInfo.AlignmentInformation.AlignmentRequirement, 0);
    ok_eq_ulong(FileAllInfo.NameInformation.FileNameLength, sizeof(PIPE_NAME) - sizeof(WCHAR));
    ok_eq_size(RtlCompareMemory(FileAllInfo.NameInformation.FileName, PIPE_NAME, 6 * sizeof(WCHAR)), (6 * sizeof(WCHAR)));
    ok_eq_wchar(FileAllInfo.NameInformation.FileName[6], 0xFFFF);
    ok_eq_ulong(IoStatusBlock.Information, (FIELD_OFFSET(FILE_ALL_INFORMATION, NameInformation.FileName) + 6 * sizeof(WCHAR)));

    RtlFillMemory(&FileAllInfo, sizeof(FileAllInfo), 0xFF);
    Status = ZwQueryInformationFile(ServerHandle,
                                    &IoStatusBlock,
                                    &FileAllInfo,
                                    sizeof(FILE_ALL_INFORMATION) - 4,
                                    FileAllInformation);
    ok_eq_hex(Status, STATUS_BUFFER_OVERFLOW);
    ok_eq_hex(IoStatusBlock.Status, STATUS_BUFFER_OVERFLOW);
    ok_eq_longlong(FileAllInfo.BasicInformation.CreationTime.QuadPart, 0);
    ok_eq_longlong(FileAllInfo.BasicInformation.LastAccessTime.QuadPart, 0);
    ok_eq_longlong(FileAllInfo.BasicInformation.LastWriteTime.QuadPart, 0);
    ok_eq_longlong(FileAllInfo.BasicInformation.ChangeTime.QuadPart, 0);
    ok_eq_ulong(FileAllInfo.BasicInformation.FileAttributes, FILE_ATTRIBUTE_NORMAL);
    ok_eq_longlong(FileAllInfo.StandardInformation.AllocationSize.QuadPart, 8192);
    ok_eq_longlong(FileAllInfo.StandardInformation.EndOfFile.QuadPart, 0);
    ok_eq_ulong(FileAllInfo.StandardInformation.NumberOfLinks, 1);
    ok_bool_true(FileAllInfo.StandardInformation.DeletePending, "DeletePending");
    ok_bool_false(FileAllInfo.StandardInformation.Directory, "Directory");
    ok(FileAllInfo.InternalInformation.IndexNumber.QuadPart != 0xFFFFFFFFFFFFFFFF, "FileAllInfo.InternalInformation.IndexNumber = 0xFFFFFFFFFFFFFFFF, whereas it shouldn't\n");
    ok(FileAllInfo.InternalInformation.IndexNumber.QuadPart != 0, "FileAllInfo.InternalInformation.IndexNumber = 0, whereas it shouldn't\n");
    ok_eq_ulong(FileAllInfo.EaInformation.EaSize, 0);
    ok_eq_ulong(FileAllInfo.AccessInformation.AccessFlags, (FILE_GENERIC_READ | FILE_GENERIC_WRITE));
    ok_eq_longlong(FileAllInfo.PositionInformation.CurrentByteOffset.QuadPart, 0);
    ok_eq_ulong(FileAllInfo.ModeInformation.Mode, FILE_SYNCHRONOUS_IO_NONALERT);
    ok_eq_ulong(FileAllInfo.AlignmentInformation.AlignmentRequirement, 0);
    ok_eq_ulong(FileAllInfo.NameInformation.FileNameLength, sizeof(PIPE_NAME) - sizeof(WCHAR));
    ok_eq_wchar(FileAllInfo.NameInformation.FileName[0], 0xFFFF);
    ok_eq_ulong(IoStatusBlock.Information, (sizeof(FILE_ALL_INFORMATION) - 4));
}

static KSTART_ROUTINE RunTest;
static
VOID
NTAPI
RunTest(
    IN PVOID Context)
{
    NTSTATUS Status;
    HANDLE ServerHandle;

    UNREFERENCED_PARAMETER(Context);

    ServerHandle = INVALID_HANDLE_VALUE;
    Status = NpCreatePipe(&ServerHandle,
                          DEVICE_NAMED_PIPE PIPE_NAME,
                          BYTE_STREAM, QUEUE, BYTE_STREAM, DUPLEX,
                          MAX_INSTANCES,
                          IN_QUOTA,
                          OUT_QUOTA);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok(ServerHandle != NULL && ServerHandle != INVALID_HANDLE_VALUE, "ServerHandle = %p\n", ServerHandle);
    if (!skip(NT_SUCCESS(Status) && ServerHandle != NULL && ServerHandle != INVALID_HANDLE_VALUE, "No pipe\n"))
    {
        TestFileInfo(ServerHandle);
        ObCloseHandle(ServerHandle, KernelMode);
    }
}

START_TEST(NpfsFileInfo)
{
    PKTHREAD Thread;

    Thread = KmtStartThread(RunTest, NULL);
    KmtFinishThread(Thread, NULL);
}
