/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS File System Recognizer
 * FILE:             drivers/filesystems/fs_rec/udfs.c
 * PURPOSE:          USFS Recognizer
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
FsRecIsUdfsVolume(IN PDEVICE_OBJECT DeviceObject,
                  IN ULONG SectorSize)
{
    PUCHAR Buffer = NULL;
    LARGE_INTEGER Offset;
    ULONG State = 0;

    Offset.QuadPart = UDFS_VRS_START_SECTOR;
    while (TRUE)
    {
        if (!FsRecReadBlock(DeviceObject,
                            &Offset,
                            512,
                            SectorSize,
                            (PVOID)&Buffer,
                            NULL))
        {
            break;
        }

        switch (State)
        {
            case 0:

                if ((Offset.QuadPart == UDFS_VRS_START_SECTOR) &&
                    (Buffer[1] == 'B') &&
                    (Buffer[2] == 'E') &&
                    (Buffer[3] == 'A') &&
                    (Buffer[4] == '0') &&
                    (Buffer[5] == '1'))
                {
                    State = 1;
                }
                else
                {
                    ExFreePool(Buffer);
                    return FALSE;
                }
                break;

            case 1:

                if ((Buffer[1] == 'N') &&
                    (Buffer[2] == 'S') &&
                    (Buffer[3] == 'R') &&
                    (Buffer[4] == '0') &&
                    ((Buffer[5] == '2') || (Buffer[5] == '3')))
                {
                    State = 2;
                }
                break;

            case 2:

                if ((Buffer[1] == 'T') &&
                    (Buffer[2] == 'E') &&
                    (Buffer[3] == 'A') &&
                    (Buffer[4] == '0') &&
                    (Buffer[5] == '1'))
                {
                    ExFreePool(Buffer);
                    return TRUE;
                }
                break;
        }

        Offset.QuadPart++;
        if (Offset.QuadPart == UDFS_AVDP_SECTOR)
        {
            ExFreePool(Buffer);
            return FALSE;
        }
    }

    ExFreePool(Buffer);
    return TRUE;
}

NTSTATUS
NTAPI
FsRecUdfsFsControl(IN PDEVICE_OBJECT DeviceObject,
                   IN PIRP Irp)
{
    PIO_STACK_LOCATION Stack;
    NTSTATUS Status;
    PDEVICE_OBJECT MountDevice;
    ULONG SectorSize;
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
                /* Check if it's an actual FAT volume */
                if (FsRecIsUdfsVolume(MountDevice, SectorSize))
                {
                    /* It is! */
                    Status = STATUS_FS_DRIVER_REQUIRED;
                }
            }

            break;

        case IRP_MN_LOAD_FILE_SYSTEM:

            /* Load the file system */
            Status = FsRecLoadFileSystem(DeviceObject,
                                         L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Udfs");
            break;

        default:

            /* Invalid request */
            Status = STATUS_INVALID_DEVICE_REQUEST;
    }

    /* Return Status */
    return Status;
}

/* EOF */
