/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS File System Recognizer
 * FILE:             drivers/filesystems/fs_rec/ext2.c
 * PURPOSE:          EXT2 Recognizer
 * PROGRAMMER:       Eric Kohl
 *                   Pierre Schweitzer 
 */

/* INCLUDES *****************************************************************/

#include "fs_rec.h"
#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

BOOLEAN
NTAPI
FsRecIsExt2Volume(IN pext2_super_block SuperBlock)
{
    BOOLEAN Result = TRUE;
    PAGED_CODE();

    if (SuperBlock->s_magic != EXT2_SUPER_MAGIC)
      Result = FALSE;

    return Result;
}

NTSTATUS
NTAPI
FsRecExt2FsControl(IN PDEVICE_OBJECT DeviceObject,
                   IN PIRP Irp)
{
    PIO_STACK_LOCATION Stack;
    NTSTATUS Status;
    PDEVICE_OBJECT MountDevice;
    PVOID SuperBlock = NULL;
    ULONG SectorSize;
    LARGE_INTEGER Offset = {{1024}};
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
                /* Try to read the Super Block */
                if (FsRecReadBlock(MountDevice,
                                   &Offset,
                                   1024,
                                   SectorSize,
                                   (PVOID)&SuperBlock,
                                   &DeviceError))
                {
                    /* Check if it's an actual EXT2 volume */
                    if (FsRecIsExt2Volume(SuperBlock))
                    {
                        /* It is! */
                        Status = STATUS_FS_DRIVER_REQUIRED;
                    }
                }

                /* Free the Super Block if we have one */
                ExFreePool(SuperBlock);
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
                                         L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Ext2fs");
            break;

        default:

            /* Invalid request */
            Status = STATUS_INVALID_DEVICE_REQUEST;
    }

    /* Return Status */
    return Status;
}

/* EOF */
