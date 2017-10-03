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
    FILE_FS_SIZE_INFORMATION FileFsSizeInfo;
    FILE_FS_DEVICE_INFORMATION FileFsDeviceInfo;
    FILE_FS_FULL_SIZE_INFORMATION FileFsFullSizeInfo;

    struct {
        FILE_FS_VOLUME_INFORMATION;
        WCHAR PartialName[10];
    } VolumeInfo;

    struct {
        FILE_FS_ATTRIBUTE_INFORMATION;
        WCHAR PartialName[6];
    } AttributeInfo;

    RtlFillMemory(&VolumeInfo, sizeof(VolumeInfo), 0xFF);
    Status = ZwQueryVolumeInformationFile(ServerHandle,
                                          &IoStatusBlock,
                                          &VolumeInfo,
                                          sizeof(VolumeInfo),
                                          FileFsVolumeInformation);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(IoStatusBlock.Status, STATUS_SUCCESS);
    ok_eq_long(VolumeInfo.VolumeCreationTime.LowPart, 0);
    ok_eq_long(VolumeInfo.VolumeCreationTime.HighPart, 0);
    ok_eq_ulong(VolumeInfo.VolumeSerialNumber, 0);
    ok_bool_false(VolumeInfo.SupportsObjects, "VolumeInfo.SupportsObjects");
    ok_eq_ulong(VolumeInfo.VolumeLabelLength, 18);
    ok_eq_size(RtlCompareMemory(VolumeInfo.VolumeLabel, L"NamedPipe", 18), 18);
    ok_eq_wchar(VolumeInfo.VolumeLabel[9], 0xFFFF);
    ok_eq_ulong(IoStatusBlock.Information, (FIELD_OFFSET(FILE_FS_VOLUME_INFORMATION, VolumeLabel) + 9 * sizeof(WCHAR)));

    RtlFillMemory(&VolumeInfo, sizeof(VolumeInfo), 0xFF);
    Status = ZwQueryVolumeInformationFile(ServerHandle,
                                          &IoStatusBlock,
                                          &VolumeInfo,
                                          sizeof(FILE_FS_VOLUME_INFORMATION) + 2 * sizeof(WCHAR),
                                          FileFsVolumeInformation);
    ok_eq_hex(Status, STATUS_BUFFER_OVERFLOW);
    ok_eq_hex(IoStatusBlock.Status, STATUS_BUFFER_OVERFLOW);
    ok_eq_long(VolumeInfo.VolumeCreationTime.LowPart, 0);
    ok_eq_long(VolumeInfo.VolumeCreationTime.HighPart, 0);
    ok_eq_ulong(VolumeInfo.VolumeSerialNumber, 0);
    ok_bool_false(VolumeInfo.SupportsObjects, "VolumeInfo.SupportsObjects");
    ok_eq_ulong(VolumeInfo.VolumeLabelLength, 18);
    ok_eq_size(RtlCompareMemory(VolumeInfo.VolumeLabel, L"NamedP", 10), 10);
    ok_eq_wchar(VolumeInfo.VolumeLabel[5], 0xFFFF);
    ok_eq_ulong(IoStatusBlock.Information, (FIELD_OFFSET(FILE_FS_VOLUME_INFORMATION, VolumeLabel) + 5 * sizeof(WCHAR)));

    RtlFillMemory(&FileFsSizeInfo, sizeof(FileFsSizeInfo), 0xFF);
    Status = ZwQueryVolumeInformationFile(ServerHandle,
                                          &IoStatusBlock,
                                          &FileFsSizeInfo,
                                          sizeof(FileFsSizeInfo),
                                          FileFsSizeInformation);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(IoStatusBlock.Status, STATUS_SUCCESS);
    ok_eq_longlong(FileFsSizeInfo.TotalAllocationUnits.QuadPart, 0);
    ok_eq_longlong(FileFsSizeInfo.AvailableAllocationUnits.QuadPart, 0);
    ok_eq_ulong(FileFsSizeInfo.SectorsPerAllocationUnit, 1);
    ok_eq_ulong(FileFsSizeInfo.BytesPerSector, 1);
    ok_eq_ulong(IoStatusBlock.Information, sizeof(FileFsSizeInfo));

    RtlFillMemory(&FileFsDeviceInfo, sizeof(FileFsDeviceInfo), 0xFF);
    Status = ZwQueryVolumeInformationFile(ServerHandle,
                                          &IoStatusBlock,
                                          &FileFsDeviceInfo,
                                          sizeof(FileFsDeviceInfo),
                                          FileFsDeviceInformation);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(IoStatusBlock.Status, STATUS_SUCCESS);
    ok_eq_ulong(FileFsDeviceInfo.Characteristics, 0);
    ok_eq_ulong(FileFsDeviceInfo.DeviceType, FILE_DEVICE_NAMED_PIPE);
    ok_eq_ulong(IoStatusBlock.Information, sizeof(FileFsDeviceInfo));

    RtlFillMemory(&AttributeInfo, sizeof(AttributeInfo), 0xFF);
    Status = ZwQueryVolumeInformationFile(ServerHandle,
                                          &IoStatusBlock,
                                          &AttributeInfo,
                                          sizeof(AttributeInfo),
                                          FileFsAttributeInformation);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(IoStatusBlock.Status, STATUS_SUCCESS);
    ok_eq_ulong(AttributeInfo.FileSystemAttributes, FILE_CASE_PRESERVED_NAMES);
    ok_eq_long(AttributeInfo.MaximumComponentNameLength, 0xFFFFFFFF);
    ok_eq_ulong(AttributeInfo.FileSystemNameLength, 8);
    ok_eq_size(RtlCompareMemory(AttributeInfo.FileSystemName, L"NPFS", 8), 8);
    ok_eq_wchar(AttributeInfo.FileSystemName[4], 0xFFFF);
    ok_eq_ulong(IoStatusBlock.Information, 20);

    RtlFillMemory(&AttributeInfo, sizeof(AttributeInfo), 0xFF);
    Status = ZwQueryVolumeInformationFile(ServerHandle,
                                          &IoStatusBlock,
                                          &AttributeInfo,
                                          sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + 2 * sizeof(WCHAR),
                                          FileFsAttributeInformation);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(IoStatusBlock.Status, STATUS_SUCCESS);
    ok_eq_ulong(AttributeInfo.FileSystemAttributes, FILE_CASE_PRESERVED_NAMES);
    ok_eq_long(AttributeInfo.MaximumComponentNameLength, 0xFFFFFFFF);
    ok_eq_ulong(AttributeInfo.FileSystemNameLength, 8);
    ok_eq_size(RtlCompareMemory(AttributeInfo.FileSystemName, L"NPFS", 8), 8);
    ok_eq_wchar(AttributeInfo.FileSystemName[4], 0xFFFF);
    ok_eq_ulong(IoStatusBlock.Information, 20);

    RtlFillMemory(&FileFsFullSizeInfo, sizeof(FileFsFullSizeInfo), 0xFF);
    Status = ZwQueryVolumeInformationFile(ServerHandle,
                                          &IoStatusBlock,
                                          &FileFsFullSizeInfo,
                                          sizeof(FileFsFullSizeInfo),
                                          FileFsFullSizeInformation);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(IoStatusBlock.Status, STATUS_SUCCESS);
    ok_eq_longlong(FileFsFullSizeInfo.TotalAllocationUnits.QuadPart, 0);
    ok_eq_longlong(FileFsFullSizeInfo.CallerAvailableAllocationUnits.QuadPart, 0);
    ok_eq_longlong(FileFsFullSizeInfo.ActualAvailableAllocationUnits.QuadPart, 0);
    ok_eq_ulong(FileFsFullSizeInfo.SectorsPerAllocationUnit, 0);
    ok_eq_ulong(FileFsFullSizeInfo.BytesPerSector, 0);
    ok_eq_ulong(IoStatusBlock.Information, sizeof(FileFsFullSizeInfo));
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
