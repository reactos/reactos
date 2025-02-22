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

#include "udfs.h"

/* FUNCTIONS ****************************************************************/

BOOLEAN
NTAPI
FsRecIsUdfsVolume(IN PDEVICE_OBJECT DeviceObject,
                  IN ULONG SectorSize)
{
    PVOLSTRUCTDESC VolumeStructDesc = NULL;
    LARGE_INTEGER Offset;
    BOOLEAN ret = FALSE;
    int i;
    PAGED_CODE();

    Offset.QuadPart = 16 * SectorSize;
    for (i = 0; i < 16; i++)
    {
        if (!FsRecReadBlock(DeviceObject,
                            &Offset,
                            SectorSize,
                            SectorSize,
                            (PVOID *)&VolumeStructDesc,
                            NULL))
        {
            break;
        }

        if (!strncmp((const char*)VolumeStructDesc->Ident,
                        VSD_STD_ID_BEA01,
                        VSD_STD_ID_LEN))
        {
            DPRINT("BEA01 found\n");
        }

        if (!strncmp((const char*)VolumeStructDesc->Ident,
                        VSD_STD_ID_NSR03,
                        VSD_STD_ID_LEN))
        {
            DPRINT("NSR03 found\n");
            ret = TRUE;
        }

        if (!strncmp((const char*)VolumeStructDesc->Ident,
                        VSD_STD_ID_NSR02,
                        VSD_STD_ID_LEN))
        {
            DPRINT("NSR02 found\n");
            ret = TRUE;
        }

        if (!strncmp((const char*)VolumeStructDesc->Ident,
                        VSD_STD_ID_TEA01,
                        VSD_STD_ID_LEN))
        {
            DPRINT("TEA01 found\n");
        }

        if (!strncmp((const char*)VolumeStructDesc->Ident,
                        VSD_STD_ID_CD001,
                        VSD_STD_ID_LEN))
        {
            DPRINT("CD001 found\n");
        }

        if (!strncmp((const char*)VolumeStructDesc->Ident,
                        VSD_STD_ID_CDW02,
                        VSD_STD_ID_LEN))
        {
            DPRINT("CDW02 found\n");
        }

        if (!strncmp((const char*)VolumeStructDesc->Ident,
                        VSD_STD_ID_BOOT2,
                        VSD_STD_ID_LEN))
        {
            DPRINT("BOOT2 found\n");
        }

        Offset.QuadPart += SectorSize;
    }

    if (VolumeStructDesc)
        ExFreePool(VolumeStructDesc);
    return ret;
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
                /* Check if it's an actual UDF volume */
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
