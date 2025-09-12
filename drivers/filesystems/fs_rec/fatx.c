/*
 * PROJECT:     ReactOS File System Recognizer
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     FATX Recognizer
 * COPYRIGHT:   Copyright 2022 Hervé Poussineau <hpoussin@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include "fs_rec.h"

#define NDEBUG
#include <debug.h>

/* TYPES ****************************************************************/

#include <pshpack1.h>
typedef struct _FATX_BOOT_SECTOR
{
    UCHAR SysType[4];
    ULONG VolumeId;
    ULONG SectorsPerCluster;
    USHORT FatCount;
    ULONG Reserved;
    UCHAR Unused[4078];
} FATX_BOOT_SECTOR, *PFATX_BOOT_SECTOR;
#include <poppack.h>

/* FUNCTIONS ****************************************************************/

BOOLEAN
NTAPI
FsRecIsFatxVolume(IN PFATX_BOOT_SECTOR BootSector)
{
    BOOLEAN Result = TRUE;

    PAGED_CODE();

    if (BootSector->SysType[0] != 'F' ||
        BootSector->SysType[1] != 'A' ||
        BootSector->SysType[2] != 'T' ||
        BootSector->SysType[3] != 'X')
    {
        /* Fail */
        Result = FALSE;
    }
    else if (BootSector->SectorsPerCluster != 1 &&
             BootSector->SectorsPerCluster != 2 &&
             BootSector->SectorsPerCluster != 4 &&
             BootSector->SectorsPerCluster != 8 &&
             BootSector->SectorsPerCluster != 16 &&
             BootSector->SectorsPerCluster != 32 &&
             BootSector->SectorsPerCluster != 64 &&
             BootSector->SectorsPerCluster != 128)
    {
        /* Fail */
        Result = FALSE;
    }

    /* Return the result */
    return Result;
}

NTSTATUS
NTAPI
FsRecFatxFsControl(IN PDEVICE_OBJECT DeviceObject,
                   IN PIRP Irp)
{
    PIO_STACK_LOCATION Stack;
    NTSTATUS Status;
    PDEVICE_OBJECT MountDevice;
    PFATX_BOOT_SECTOR Bpb = NULL;
    ULONG SectorSize;
    LARGE_INTEGER Offset = {{0, 0}};
    BOOLEAN DeviceError = FALSE;
    PAGED_CODE();

    /* Get the I/O Stack and check the function type */
    Stack = IoGetCurrentIrpStackLocation(Irp);
    switch (Stack->MinorFunction)
    {
        case IRP_MN_MOUNT_VOLUME:

            /* Assume failure */
            Status = STATUS_UNRECOGNIZED_VOLUME;

            /* Get the device object and request the sector size */
            MountDevice = Stack->Parameters.MountVolume.DeviceObject;
            if (FsRecGetDeviceSectorSize(MountDevice, &SectorSize))
            {
                /* Try to read the BPB */
                if (FsRecReadBlock(MountDevice,
                                   &Offset,
                                   512,
                                   SectorSize,
                                   (PVOID)&Bpb,
                                   &DeviceError))
                {
                    /* Check if it's an actual FAT volume */
                    if (FsRecIsFatxVolume(Bpb))
                    {
                        /* It is! */
                        Status = STATUS_FS_DRIVER_REQUIRED;
                    }
                }

                /* Free the boot sector if we have one */
                ExFreePool(Bpb);
            }

            break;

        case IRP_MN_LOAD_FILE_SYSTEM:

            /* Load the file system */
            Status = FsRecLoadFileSystem(DeviceObject,
                                         L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\vfatfs");
            break;

        default:

            /* Invalid request */
            Status = STATUS_INVALID_DEVICE_REQUEST;
    }

    /* Return Status */
    return Status;
}

/* EOF */
