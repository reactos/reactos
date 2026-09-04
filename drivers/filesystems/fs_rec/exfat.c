/*
 * PROJECT:     ReactOS File System Recognizer
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     exFAT recognizer
 * COPYRIGHT:   Copyright 2026 Ahmed ARIF <arif.ing@outlook.com>
 */

#include "fs_rec.h"

#define NDEBUG
#include <debug.h>

#define EXFAT_NAME_OFFSET           3
#define EXFAT_NAME_LENGTH           8
#define EXFAT_SECTOR_SHIFT_OFFSET   108
#define EXFAT_BOOT_SIGNATURE_OFFSET 510

static BOOLEAN
FsRecIsExFatVolume(
    PUCHAR BootSector)
{
    UCHAR SectorShift;

    PAGED_CODE();

    SectorShift = BootSector[EXFAT_SECTOR_SHIFT_OFFSET];
    return RtlCompareMemory(&BootSector[EXFAT_NAME_OFFSET],
                            "EXFAT   ",
                            EXFAT_NAME_LENGTH) == EXFAT_NAME_LENGTH &&
           BootSector[EXFAT_BOOT_SIGNATURE_OFFSET] == 0x55 &&
           BootSector[EXFAT_BOOT_SIGNATURE_OFFSET + 1] == 0xAA &&
           SectorShift >= 9 && SectorShift <= 12;
}

NTSTATUS
NTAPI
FsRecExFatFsControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION Stack;
    PDEVICE_OBJECT MountDevice;
    PUCHAR BootSector = NULL;
    ULONG SectorSize;
    LARGE_INTEGER Offset = {{0, 0}};
    NTSTATUS Status;

    PAGED_CODE();

    Stack = IoGetCurrentIrpStackLocation(Irp);
    switch (Stack->MinorFunction)
    {
        case IRP_MN_MOUNT_VOLUME:
            Status = STATUS_UNRECOGNIZED_VOLUME;
            MountDevice = Stack->Parameters.MountVolume.DeviceObject;
            if (FsRecGetDeviceSectorSize(MountDevice, &SectorSize) &&
                FsRecReadBlock(MountDevice,
                               &Offset,
                               512,
                               SectorSize,
                               (PVOID*)&BootSector,
                               NULL))
            {
                if (FsRecIsExFatVolume(BootSector))
                    Status = STATUS_FS_DRIVER_REQUIRED;
            }
            if (BootSector)
                ExFreePool(BootSector);
            break;

        case IRP_MN_LOAD_FILE_SYSTEM:
            Status = FsRecLoadFileSystem(DeviceObject,
                                         L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\ExFat");
            break;

        default:
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    return Status;
}
