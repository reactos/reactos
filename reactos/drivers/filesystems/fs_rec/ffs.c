/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS File System Recognizer
 * FILE:             drivers/filesystems/fs_rec/ffs.c
 * PURPOSE:          FFS Recognizer
 * PROGRAMMER:       Peter Hater
 *                   Pierre Schweitzer (pierre@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include "fs_rec.h"
#include "ffs.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

BOOLEAN
NTAPI
FsRecIsFfsDiskLabel(IN PFFSD_DISKLABEL dl)
{
    return (dl->d_magic == DISKMAGIC);
}

BOOLEAN
NTAPI
FsRecIsFfs1Volume(IN PFFSD_SUPER_BLOCK sb)
{
    return (sb->fs_magic == FS_UFS1_MAGIC);
}

BOOLEAN
NTAPI
FsRecIsFfs2Volume(IN PFFSD_SUPER_BLOCK sb)
{
    return (sb->fs_magic == FS_UFS2_MAGIC);
}

NTSTATUS
NTAPI
FsRecFfsFsControl(IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp)
{
    PIO_STACK_LOCATION Stack;
    NTSTATUS Status;
    PDEVICE_OBJECT MountDevice;
    PFFSD_SUPER_BLOCK Spb = NULL;
    PFFSD_DISKLABEL DiskLabel = NULL;
    ULONGLONG FSOffset = 0;
    int i;
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
                /* Try to read the disklabel */
                Offset.QuadPart = LABELSECTOR*SectorSize;
                if (FsRecReadBlock(MountDevice,
                                   &Offset,
                                   SectorSize,
                                   SectorSize,
                                   (PVOID)&DiskLabel,
                                   &DeviceError))
                {
                    /* Check if it's an actual FFS disk label */
                    if (FsRecIsFfsDiskLabel(DiskLabel))
                    {
                        /* It is! */
	                    for (i = 0; i < MAXPARTITIONS; i++)
	                    {
		                    if (DiskLabel->d_partitions[i].p_fstype == FS_BSDFFS)
		                    {
			                    /* Important */
			                    FSOffset = DiskLabel->d_partitions[i].p_offset;
			                    FSOffset = FSOffset * SectorSize;
                                /* Try to read the superblock */
                                Offset.QuadPart = FSOffset+SBLOCK_UFS1;
                                if (FsRecReadBlock(MountDevice,
                                                   &Offset,
                                                   SBLOCKSIZE,
                                                   SectorSize,
                                                   (PVOID)&Spb,
                                                   &DeviceError))
                                {
                                    /* Check if it's an actual FFS volume */
                                    if (FsRecIsFfs1Volume(Spb))
                                    {
                                        /* It is! */
                                        Status = STATUS_FS_DRIVER_REQUIRED;
                                    }
                                    else
                                    {
                                        /* Free the boot sector if we have one */
                                        ExFreePool(Spb);
                                        Spb = NULL;

                                        Offset.QuadPart = FSOffset+SBLOCK_UFS2;
                                        if (FsRecReadBlock(MountDevice,
                                                           &Offset,
                                                           SBLOCKSIZE,
                                                           SectorSize,
                                                           (PVOID)&Spb,
                                                           &DeviceError))
                                        {
                                            /* Check if it's an actual FFS volume */
                                            if (FsRecIsFfs2Volume(Spb))
                                            {
                                                /* It is! */
                                                Status = STATUS_FS_DRIVER_REQUIRED;
                                            }
                                        }
                                    }
                                }

                                /* Free the boot sector if we have one */
                                ExFreePool(Spb);
                                Spb = NULL;
                            }
                        }
                    }
                    else
                    {
                        /* Try to read the superblock */
                        Offset.QuadPart = FSOffset+SBLOCK_UFS1;
                        if (FsRecReadBlock(MountDevice,
                                            &Offset,
                                            SBLOCKSIZE,
                                            SectorSize,
                                            (PVOID)&Spb,
                                            &DeviceError))
                        {
                            /* Check if it's an actual FFS volume */
                            if (FsRecIsFfs1Volume(Spb))
                            {
                                /* It is! */
                                Status = STATUS_FS_DRIVER_REQUIRED;
                            }
                            else
                            {
                                /* Free the boot sector if we have one */
                                ExFreePool(Spb);
                                Spb = NULL;

                                Offset.QuadPart = FSOffset+SBLOCK_UFS2;
                                if (FsRecReadBlock(MountDevice,
                                                    &Offset,
                                                    SBLOCKSIZE,
                                                    SectorSize,
                                                    (PVOID)&Spb,
                                                    &DeviceError))
                                {
                                    /* Check if it's an actual FFS volume */
                                    if (FsRecIsFfs2Volume(Spb))
                                    {
                                        /* It is! */
                                        Status = STATUS_FS_DRIVER_REQUIRED;
                                    }
                                }
                            }
                        }

                        /* Free the boot sector if we have one */
                        ExFreePool(Spb);
                        Spb = NULL;
                    }
                }

                /* Free the boot sector if we have one */
                ExFreePool(DiskLabel);
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
                                         L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\ffs");
            break;

        default:

            /* Invalid request */
            Status = STATUS_INVALID_DEVICE_REQUEST;
    }

    /* Return Status */
    return Status;
}
