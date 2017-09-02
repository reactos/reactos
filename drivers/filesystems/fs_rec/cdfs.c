/*
 * PROJECT:     ReactOS File System Recognizer
 * LICENSE:     GPL-2.0 (https://spdx.org/licenses/GPL-2.0)
 * PURPOSE:     CDFS Recognizer
 * COPYRIGHT:   Copyright 2002 Eric Kohl
 *              Copyright 2007 Alex Ionescu <alex.ionescu@reactos.org>
 *              Copyright 2017 Colin Finck <colin@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include "fs_rec.h"

#define NDEBUG
#include <debug.h>

#include "cdfs.h"

/* FUNCTIONS ****************************************************************/

BOOLEAN
NTAPI
FsRecIsCdfsVolume(IN PDEVICE_OBJECT DeviceObject,
                  IN ULONG SectorSize)
{
    BOOLEAN bReturnValue = FALSE;
    LARGE_INTEGER Offset;
    PVD_HEADER pVdHeader = NULL;
    PAGED_CODE();

    // Read the Primary Volume Descriptor.
    Offset.QuadPart = VD_HEADER_OFFSET;
    if (!FsRecReadBlock(DeviceObject, &Offset, sizeof(VD_HEADER), SectorSize, (PVOID*)&pVdHeader, NULL))
    {
        // We cannot read this block, so no reason to let the CDFS driver try it.
        goto Cleanup;
    }

    // Verify the fields.
    if (pVdHeader->Type != VD_TYPE_PRIMARY)
        goto Cleanup;

    DPRINT("pVdHeader->Type verified!\n");

    if (RtlCompareMemory(pVdHeader->Identifier, VD_IDENTIFIER, VD_IDENTIFIER_LENGTH) != VD_IDENTIFIER_LENGTH)
        goto Cleanup;

    DPRINT("pVdHeader->Identifier verified!\n");

    if (pVdHeader->Version != VD_VERSION)
        goto Cleanup;

    DPRINT("pVdHeader->Version verified! This is a CDFS volume!\n");

    bReturnValue = TRUE;

Cleanup:
    if (pVdHeader)
        ExFreePool(pVdHeader);

    return bReturnValue;
}

NTSTATUS
NTAPI
FsRecCdfsFsControl(IN PDEVICE_OBJECT DeviceObject,
                   IN PIRP Irp)
{
    PDEVICE_OBJECT MountDevice;
    ULONG SectorSize;
    PIO_STACK_LOCATION Stack;
    NTSTATUS Status;
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
                /* Check if it's an actual CDFS (ISO-9660) volume */
                if (FsRecIsCdfsVolume(MountDevice, SectorSize))
                {
                    /* It is! */
                    Status = STATUS_FS_DRIVER_REQUIRED;
                }
            }

            break;

        case IRP_MN_LOAD_FILE_SYSTEM:

            /* Load the file system */
            Status = FsRecLoadFileSystem(DeviceObject,
                                         L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Cdfs");
            break;

        default:

            /* Invalid request */
            Status = STATUS_INVALID_DEVICE_REQUEST;
    }

    /* Return Status */
    return Status;
}

/* EOF */
