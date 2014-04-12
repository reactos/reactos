/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite NPFS volume information test
 * PROGRAMMER:      Pierre Schweitzer <pierre@reactos.org>
 */

#include <kmt_test.h>
#include "npfs.h"

#define MAX_INSTANCES   1
#define IN_QUOTA        4096
#define OUT_QUOTA       4096

static
VOID
TestVolumeInfo(
    IN HANDLE ServerHandle)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    struct {
        FILE_FS_VOLUME_INFORMATION;
        WCHAR PartialName[2];
    } PartialInfo;
    struct {
        FILE_FS_VOLUME_INFORMATION;
        WCHAR PartialName[10];
    } CompleteInfo;

    Status = ZwQueryVolumeInformationFile(ServerHandle,
                                          &IoStatusBlock,
                                          &CompleteInfo,
                                          sizeof(CompleteInfo),
                                          FileFsVolumeInformation);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(IoStatusBlock.Status, STATUS_SUCCESS);
    ok_eq_long(CompleteInfo.VolumeCreationTime.LowPart, 0);
    ok_eq_long(CompleteInfo.VolumeCreationTime.HighPart, 0);
    ok_eq_ulong(CompleteInfo.VolumeSerialNumber, 0);
    ok_bool_false(CompleteInfo.SupportsObjects, "CompleteInfo.SupportsObjects");
    ok_eq_ulong(CompleteInfo.VolumeLabelLength, 18);
    ok_eq_ulong(IoStatusBlock.Information, 36);
    ok_eq_ulong(RtlCompareMemory(CompleteInfo.VolumeLabel, L"NamedPipe", 18), 18);

    Status = ZwQueryVolumeInformationFile(ServerHandle,
                                          &IoStatusBlock,
                                          &PartialInfo,
                                          sizeof(PartialInfo),
                                          FileFsVolumeInformation);
    ok_eq_hex(Status, STATUS_BUFFER_OVERFLOW);
    ok_eq_hex(IoStatusBlock.Status, STATUS_BUFFER_OVERFLOW);
    ok_eq_long(CompleteInfo.VolumeCreationTime.LowPart, 0);
    ok_eq_long(CompleteInfo.VolumeCreationTime.HighPart, 0);
    ok_eq_ulong(CompleteInfo.VolumeSerialNumber, 0);
    ok_bool_false(CompleteInfo.SupportsObjects, "CompleteInfo.SupportsObjects");
    ok_eq_ulong(CompleteInfo.VolumeLabelLength, 18);
    ok_eq_ulong(IoStatusBlock.Information, 32);
    ok_eq_ulong(RtlCompareMemory(CompleteInfo.VolumeLabel, L"Na", 4), 4);
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
                          DEVICE_NAMED_PIPE L"\\KmtestNpfsVolumeInfoTestPipe",
                          BYTE_STREAM, QUEUE, BYTE_STREAM, DUPLEX,
                          MAX_INSTANCES,
                          IN_QUOTA,
                          OUT_QUOTA);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok(ServerHandle != NULL && ServerHandle != INVALID_HANDLE_VALUE, "ServerHandle = %p\n", ServerHandle);
    if (!skip(NT_SUCCESS(Status) && ServerHandle != NULL && ServerHandle != INVALID_HANDLE_VALUE, "No pipe\n"))
    {
        TestVolumeInfo(ServerHandle);
        ObCloseHandle(ServerHandle, KernelMode);
    }
}

START_TEST(NpfsVolumeInfo)
{
    PKTHREAD Thread;

    Thread = KmtStartThread(RunTest, NULL);
    KmtFinishThread(Thread, NULL);
}
