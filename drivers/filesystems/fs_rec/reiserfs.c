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
#include "reiserfs.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

BOOLEAN
NTAPI
FsRecIsReiserfsVolume(IN PRFSD_SUPER_BLOCK sb)
{
    UCHAR sz_MagicKey[] = REISER2FS_SUPER_MAGIC_STRING;
    UCHAR currentChar;
    int   i;

    // If any characters read from disk don't match the expected magic key, we don't have a ReiserFS volume.
    for (i = 0; i < MAGIC_KEY_LENGTH; i++)
    {
        currentChar = sb->s_magic[i];
        if (currentChar != sz_MagicKey[i])
        {
            return FALSE;
        }
    }

    return TRUE;
}

NTSTATUS
NTAPI
FsRecReiserfsFsControl(IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp)
{
    PIO_STACK_LOCATION Stack;
    NTSTATUS Status;
    PDEVICE_OBJECT MountDevice;
    PRFSD_SUPER_BLOCK Spb = NULL;
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
                Offset.QuadPart = REISERFS_DISK_OFFSET_IN_BYTES;
                if (FsRecReadBlock(MountDevice,
                                   &Offset,
                                   SectorSize,
                                   SectorSize,
                                   (PVOID)&Spb,
                                   &DeviceError))
                {
                    /* Check if it's an actual BTRFS volume */
                    if (FsRecIsReiserfsVolume(Spb))
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
                                         L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\reiserfs");
            break;

        default:

            /* Invalid request */
            Status = STATUS_INVALID_DEVICE_REQUEST;
    }

    /* Return Status */
    return Status;
}
