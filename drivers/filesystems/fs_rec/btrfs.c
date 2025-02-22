/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS File System Recognizer
 * FILE:             drivers/filesystems/fs_rec/btrfs.c
 * PURPOSE:          Btrfs Recognizer
 * PROGRAMMER:       Peter Hater
 *                   Pierre Schweitzer (pierre@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include "fs_rec.h"
#include "btrfs.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

BOOLEAN
NTAPI
FsRecIsBtrfsVolume(IN PBTRFS_SUPER_BLOCK SuperBlock)
{
    /* Just check for magic */
    return (SuperBlock->magic == BTRFS_MAGIC);
}

NTSTATUS
NTAPI
FsRecBtrfsFsControl(IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp)
{
    PIO_STACK_LOCATION Stack;
    NTSTATUS Status;
    PDEVICE_OBJECT MountDevice;
    PBTRFS_SUPER_BLOCK Spb = NULL;
    ULONG SectorSize;
    LARGE_INTEGER Offset;
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
                /* Try to read the superblock */
                Offset.QuadPart = BTRFS_SB_OFFSET;
                if (FsRecReadBlock(MountDevice,
                                   &Offset,
                                   BTRFS_SB_SIZE,
                                   SectorSize,
                                   (PVOID)&Spb,
                                   &DeviceError))
                {
                    /* Check if it's an actual BTRFS volume */
                    if (FsRecIsBtrfsVolume(Spb))
                    {
                        /* It is! */
                        Status = STATUS_FS_DRIVER_REQUIRED;
                    }
                }

                /* Free the boot sector if we have one */
                ExFreePool(Spb);
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
                                         L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\btrfs");
            break;

        default:

            /* Invalid request */
            Status = STATUS_INVALID_DEVICE_REQUEST;
    }

    /* Return Status */
    return Status;
}
