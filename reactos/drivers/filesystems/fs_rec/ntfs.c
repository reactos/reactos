/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS File System Recognizer
 * FILE:             drivers/filesystems/fs_rec/ntfs.c
 * PURPOSE:          NTFS Recognizer
 * PROGRAMMER:       Alex Ionescu (alex.ionescu@reactos.org)
 *                   Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include "fs_rec.h"
#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

BOOLEAN
NTAPI
FsRecIsNtfsVolume(IN PPACKED_BOOT_SECTOR BootSector,
                  IN ULONG BytesPerSector,
                  IN PLARGE_INTEGER  NumberOfSectors)
{
    PAGED_CODE();
    BOOLEAN Result;

    /* Assume success */
    Result = TRUE;

    if ((BootSector->Oem[0] == 'N') &&
        (BootSector->Oem[1] == 'T') &&
        (BootSector->Oem[2] == 'F') &&
        (BootSector->Oem[3] == 'S') &&
        (BootSector->Oem[4] == ' ') &&
        (BootSector->Oem[5] == ' ') &&
        (BootSector->Oem[6] == ' ') &&
        (BootSector->Oem[7] == ' '))
    {
        /* Fail */
        Result = FALSE;
    }

    /* Return the result */
    return Result;
}

NTSTATUS
NTAPI
FsRecNtfsFsControl(IN PDEVICE_OBJECT DeviceObject,
                   IN PIRP Irp)
{
    PIO_STACK_LOCATION Stack;
    NTSTATUS Status;
    PDEVICE_OBJECT MountDevice;
    PPACKED_BOOT_SECTOR Bpb = NULL;
    ULONG SectorSize;
    LARGE_INTEGER Offset = {{0}}, Offset2, Offset3, SectorCount;
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
            if ((FsRecGetDeviceSectorSize(MountDevice, &SectorSize)) &&
                (FsRecGetDeviceSectors(MountDevice, SectorSize, &SectorCount)))
            {
                /* Setup other offsets to try */
                Offset2.QuadPart = SectorCount.QuadPart >> 1;
                Offset2.QuadPart *= SectorSize;
                Offset3.QuadPart = (SectorCount.QuadPart - 1) * SectorSize;

                /* Try to read the BPB */
                if (FsRecReadBlock(MountDevice,
                                   &Offset,
                                   512,
                                   SectorSize,
                                   (PVOID)&Bpb,
                                   NULL))
                {
                    /* Check if it's an actual FAT volume */
                    if (FsRecIsNtfsVolume(Bpb, SectorSize, &SectorCount))
                    {
                        /* It is! */
                        Status = STATUS_FS_DRIVER_REQUIRED;
                    }
                }
                else if (FsRecReadBlock(MountDevice,
                                        &Offset2,
                                        512,
                                        SectorSize,
                                        (PVOID)&Bpb,
                                        NULL))
                {
                    /* Check if it's an actual FAT volume */
                    if (FsRecIsNtfsVolume(Bpb, SectorSize, &SectorCount))
                    {
                        /* It is! */
                        Status = STATUS_FS_DRIVER_REQUIRED;
                    }
                }
                else if (FsRecReadBlock(MountDevice,
                                        &Offset3,
                                        512,
                                        SectorSize,
                                        (PVOID)&Bpb,
                                        NULL))
                {
                    /* Check if it's an actual FAT volume */
                    if (FsRecIsNtfsVolume(Bpb, SectorSize, &SectorCount))
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
                                         L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Ntfs");
            break;

        default:

            /* Invalid request */
            Status = STATUS_INVALID_DEVICE_REQUEST;
    }

    /* Return Status */
    return Status;
}

/* EOF */
