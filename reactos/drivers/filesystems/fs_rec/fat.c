/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS File System Recognizer
 * FILE:             drivers/filesystems/fs_rec/fat.c
 * PURPOSE:          FAT Recognizer
 * PROGRAMMER:       Alex Ionescu (alex.ionescu@reactos.org)
 *                   Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include "fs_rec.h"
#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

NTSTATUS
NTAPI
FsRecIsFatVolume(IN PPACKED_BOOT_SECTOR PackedBootSector)
{
    BIOS_PARAMETER_BLOCK Bpb;
    BOOLEAN Result = TRUE;
    PAGED_CODE();

    RtlZeroMemory(&Bpb, sizeof(BIOS_PARAMETER_BLOCK));

    /* Unpack the BPB and do a small fix up */
    FatUnpackBios(&Bpb, &PackedBootSector->PackedBpb);
    if (Bpb.Sectors) Bpb.LargeSectors = 0;

    /* Recognize jump */
    if ((PackedBootSector->Jump[0] != 0x49) &&
        (PackedBootSector->Jump[0] != 0xE9) &&
        (PackedBootSector->Jump[0] != 0xEB))
    {
        /* Fail */
        Result = FALSE;
    }
    else if ((Bpb.BytesPerSector != 128) &&
             (Bpb.BytesPerSector != 256) &&
             (Bpb.BytesPerSector != 512) &&
             (Bpb.BytesPerSector != 1024) &&
             (Bpb.BytesPerSector != 2048) &&
             (Bpb.BytesPerSector != 4096))
    {
        /* Fail */
        Result = FALSE;
    }
    else if ((Bpb.SectorsPerCluster != 1) &&
             (Bpb.SectorsPerCluster != 2) &&
             (Bpb.SectorsPerCluster != 4) &&
             (Bpb.SectorsPerCluster != 8) &&
             (Bpb.SectorsPerCluster != 16) &&
             (Bpb.SectorsPerCluster != 32) &&
             (Bpb.SectorsPerCluster != 64) &&
             (Bpb.SectorsPerCluster != 128))
    {
        /* Fail */
        Result = FALSE;
    }
    else if (!Bpb.ReservedSectors)
    {
        /* Fail */
        Result = FALSE;
    }
    else if (!(Bpb.Sectors) && !(Bpb.LargeSectors))
    {
        /* Fail */
        Result = FALSE;
    }
    else if ((Bpb.Media != 0x00) &&
             (Bpb.Media != 0x01) &&
             (Bpb.Media != 0xf0) &&
             (Bpb.Media != 0xf8) &&
             (Bpb.Media != 0xf9) &&
             (Bpb.Media != 0xfa) &&
             (Bpb.Media != 0xfb) &&
             (Bpb.Media != 0xfc) &&
             (Bpb.Media != 0xfd) &&
             (Bpb.Media != 0xfe) &&
             (Bpb.Media != 0xff))
    {
        /* Fail */
        Result = FALSE;
    }
    else if ((Bpb.SectorsPerFat) && !(Bpb.RootEntries))
    {
        /* Fail */
        Result = FALSE;
    }

    /* Return the result */
    return Result;
}

NTSTATUS
NTAPI
FsRecVfatFsControl(IN PDEVICE_OBJECT DeviceObject,
                   IN PIRP Irp)
{
    PIO_STACK_LOCATION Stack;
    NTSTATUS Status;
    PDEVICE_OBJECT MountDevice;
    PPACKED_BOOT_SECTOR Bpb = NULL;
    ULONG SectorSize;
    LARGE_INTEGER Offset = {{0}};
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
                    if (FsRecIsFatVolume(Bpb))
                    {
                        /* It is! */
                        Status = STATUS_FS_DRIVER_REQUIRED;
                    }
                }

                /* Free the boot sector if we have one */
                ExFreePool(Bpb);
            }
            else
            {
                /* We have some sort of failure in the storage stack */
                DeviceError = TRUE;
            }

            /* Check if we have an error on the stack */
            if (DeviceError)
            {
                /* Was this because of a floppy? */
                if (MountDevice->Characteristics & FILE_FLOPPY_DISKETTE)
                {
                    /* Let the FS try anyway */
                    Status = STATUS_FS_DRIVER_REQUIRED;
                }
            }

            break;

        case IRP_MN_LOAD_FILE_SYSTEM:

            /* Load the file system */
            Status = FsRecLoadFileSystem(DeviceObject,
                                         L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Vfatfs");
            break;

        default:

            /* Invalid request */
            Status = STATUS_INVALID_DEVICE_REQUEST;
    }

    /* Return Status */
    return Status;
}

/* EOF */
