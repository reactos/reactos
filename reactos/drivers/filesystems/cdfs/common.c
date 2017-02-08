/*
 *  ReactOS kernel
 *  Copyright (C) 2002, 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/filesystems/cdfs/common.c
 * PURPOSE:          CDROM (ISO 9660) filesystem driver
 * PROGRAMMER:       Art Yerkes
 *                   Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include "cdfs.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

NTSTATUS
CdfsReadSectors(IN PDEVICE_OBJECT DeviceObject,
                IN ULONG DiskSector,
                IN ULONG SectorCount,
                IN OUT PUCHAR Buffer,
                IN BOOLEAN Override)
{
    PIO_STACK_LOCATION Stack;
    IO_STATUS_BLOCK IoStatus;
    LARGE_INTEGER Offset;
    ULONG BlockSize;
    KEVENT Event;
    PIRP Irp;
    NTSTATUS Status;
    BOOLEAN LastChance = FALSE;

again:
    KeInitializeEvent(&Event,
        NotificationEvent,
        FALSE);

    Offset.u.LowPart = DiskSector << 11;
    Offset.u.HighPart = DiskSector >> 21;

    BlockSize = BLOCKSIZE * SectorCount;

    DPRINT("CdfsReadSectors(DeviceObject %p, DiskSector %u, Buffer %p)\n",
        DeviceObject, DiskSector, Buffer);
    DPRINT("Offset %I64x BlockSize %u\n",
        Offset.QuadPart,
        BlockSize);

    DPRINT("Building synchronous FSD Request...\n");
    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,
        DeviceObject,
        Buffer,
        BlockSize,
        &Offset,
        &Event,
        &IoStatus);
    if (Irp == NULL)
    {
        DPRINT("IoBuildSynchronousFsdRequest failed\n");
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    if (Override)
    {
        Stack = IoGetNextIrpStackLocation(Irp);
        Stack->Flags |= SL_OVERRIDE_VERIFY_VOLUME;
    }

    DPRINT("Calling IO Driver... with irp %p\n", Irp);
    Status = IoCallDriver(DeviceObject, Irp);

    DPRINT("Waiting for IO Operation for %p\n", Irp);
    if (Status == STATUS_PENDING)
    {
        DPRINT("Operation pending\n");
        KeWaitForSingleObject(&Event, Suspended, KernelMode, FALSE, NULL);
        DPRINT("Getting IO Status... for %p\n", Irp);
        Status = IoStatus.Status;
    }

    if (!NT_SUCCESS(Status))
    {
        if (Status == STATUS_VERIFY_REQUIRED)
        {
            PDEVICE_OBJECT DeviceToVerify;

            DeviceToVerify = IoGetDeviceToVerify(PsGetCurrentThread());
            IoSetDeviceToVerify(PsGetCurrentThread(), NULL);

            Status = IoVerifyVolume(DeviceToVerify, FALSE);

            if (NT_SUCCESS(Status) && !LastChance)
            {
                DPRINT("Volume verify succeeded; trying request again\n");
                LastChance = TRUE;
                goto again;
            }
            else if (NT_SUCCESS(Status))
            {
                DPRINT1("Failed to read after successful verify, aborting\n");
                Status = STATUS_DEVICE_NOT_READY;
            }
            else
            {
                DPRINT1("IoVerifyVolume() failed (Status %lx)\n", Status);
            }
        }

        DPRINT("CdfsReadSectors() failed (Status %x)\n", Status);
        DPRINT("(DeviceObject %p, DiskSector %u, Buffer %p, Offset 0x%I64x)\n",
            DeviceObject, DiskSector, Buffer,
            Offset.QuadPart);
        return(Status);
    }

    DPRINT("Block request succeeded for %p\n", Irp);

    return(STATUS_SUCCESS);
}


NTSTATUS
CdfsDeviceIoControl (IN PDEVICE_OBJECT DeviceObject,
                     IN ULONG CtlCode,
                     IN PVOID InputBuffer,
                     IN ULONG InputBufferSize,
                     IN OUT PVOID OutputBuffer,
                     IN OUT PULONG OutputBufferSize,
                     IN BOOLEAN Override)
{
    PIO_STACK_LOCATION Stack;
    IO_STATUS_BLOCK IoStatus;
    KEVENT Event;
    PIRP Irp;
    NTSTATUS Status;
    BOOLEAN LastChance = FALSE;

    DPRINT("CdfsDeviceIoControl(DeviceObject %p, CtlCode %u, "
        "InputBuffer %p, InputBufferSize %u, OutputBuffer %p, "
        "POutputBufferSize %p (%x)\n", DeviceObject, CtlCode,
        InputBuffer, InputBufferSize, OutputBuffer, OutputBufferSize,
        OutputBufferSize ? *OutputBufferSize : 0);

again:
    KeInitializeEvent (&Event, NotificationEvent, FALSE);

    DPRINT("Building device I/O control request ...\n");
    Irp = IoBuildDeviceIoControlRequest(CtlCode,
        DeviceObject,
        InputBuffer,
        InputBufferSize,
        OutputBuffer,
        (OutputBufferSize != NULL) ? *OutputBufferSize : 0,
        FALSE,
        &Event,
        &IoStatus);
    if (Irp == NULL)
    {
        DPRINT("IoBuildDeviceIoControlRequest failed\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (Override)
    {
        Stack = IoGetNextIrpStackLocation(Irp);
        Stack->Flags |= SL_OVERRIDE_VERIFY_VOLUME;
    }

    DPRINT ("Calling IO Driver... with irp %p\n", Irp);
    Status = IoCallDriver(DeviceObject, Irp);

    DPRINT ("Waiting for IO Operation for %p\n", Irp);
    if (Status == STATUS_PENDING)
    {
        DPRINT ("Operation pending\n");
        KeWaitForSingleObject (&Event, Suspended, KernelMode, FALSE, NULL);
        DPRINT ("Getting IO Status... for %p\n", Irp);

        Status = IoStatus.Status;
    }

    if (OutputBufferSize != NULL)
    {
        *OutputBufferSize = IoStatus.Information;
    }

    if (Status == STATUS_VERIFY_REQUIRED)
    {
        PDEVICE_OBJECT DeviceToVerify;

        DeviceToVerify = IoGetDeviceToVerify(PsGetCurrentThread());
        IoSetDeviceToVerify(PsGetCurrentThread(), NULL);

        if (DeviceToVerify)
        {
            Status = IoVerifyVolume(DeviceToVerify, FALSE);
            DPRINT("IoVerifyVolume() returned (Status %lx)\n", Status);
        }

        if (NT_SUCCESS(Status) && !LastChance)
        {
            DPRINT1("Volume verify succeeded; trying request again\n");
            LastChance = TRUE;
            goto again;
        }
        else if (NT_SUCCESS(Status))
        {
            DPRINT1("Failed to read after successful verify, aborting\n");
            Status = STATUS_DEVICE_NOT_READY;
        }
        else
        {
            DPRINT1("IoVerifyVolume() failed (Status %lx)\n", Status);
        }
    }

    DPRINT("Returning Status %x\n", Status);

    return Status;
}

/* EOF */
