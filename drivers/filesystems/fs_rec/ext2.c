/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS File System Recognizer
 * FILE:             drivers/filesystems/fs_rec/ext2.c
 * PURPOSE:          EXT2 Recognizer
 * PROGRAMMER:       Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include "fs_rec.h"
#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

BOOLEAN
NTAPI
FsRecIsExt2Volume(IN PVOID PackedBootSector)
{
    /* For now, always return failure... */
    return FALSE;
}

NTSTATUS
NTAPI
FsRecExt2FsControl(IN PDEVICE_OBJECT DeviceObject,
                   IN PIRP Irp)
{
    PIO_STACK_LOCATION Stack;
    NTSTATUS Status;
    PDEVICE_OBJECT MountDevice;
    PVOID Bpb = NULL;
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
                    /* Check if it's an actual EXT2 volume */
                    if (FsRecIsExt2Volume(Bpb))
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
                                         L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Ext2");
            break;

        default:

            /* Invalid request */
            Status = STATUS_INVALID_DEVICE_REQUEST;
    }

    /* Return Status */
    return Status;
}

/* EOF */
