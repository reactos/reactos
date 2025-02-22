/*
 *  ReactOS kernel
 *  Copyright (C) 2002 ReactOS Team
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/filesystem/ntfs/blockdev.c
 * PURPOSE:          NTFS filesystem driver
 * PROGRAMMERS:      Eric Kohl
 *                   Trevor Thompson
 */

/* INCLUDES *****************************************************************/

#include "ntfs.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

NTSTATUS
NtfsReadDisk(IN PDEVICE_OBJECT DeviceObject,
             IN LONGLONG StartingOffset,
             IN ULONG Length,
             IN ULONG SectorSize,
             IN OUT PUCHAR Buffer,
             IN BOOLEAN Override)
{
    PIO_STACK_LOCATION Stack;
    IO_STATUS_BLOCK IoStatus;
    LARGE_INTEGER Offset;
    KEVENT Event;
    PIRP Irp;
    NTSTATUS Status;
    ULONGLONG RealReadOffset;
    ULONG RealLength;
    BOOLEAN AllocatedBuffer = FALSE;
    PUCHAR ReadBuffer = Buffer;

    DPRINT("NtfsReadDisk(%p, %I64x, %lu, %lu, %p, %d)\n", DeviceObject, StartingOffset, Length, SectorSize, Buffer, Override);

    KeInitializeEvent(&Event,
                      NotificationEvent,
                      FALSE);

    RealReadOffset = (ULONGLONG)StartingOffset;
    RealLength = Length;

    if ((RealReadOffset % SectorSize) != 0 || (RealLength % SectorSize) != 0)
    {
        RealReadOffset = ROUND_DOWN(StartingOffset, SectorSize);
        RealLength = ROUND_UP(Length, SectorSize);

        ReadBuffer = ExAllocatePoolWithTag(NonPagedPool, RealLength + SectorSize, TAG_NTFS);
        if (ReadBuffer == NULL)
        {
            DPRINT1("Not enough memory!\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        AllocatedBuffer = TRUE;
    }

    Offset.QuadPart = RealReadOffset;

    DPRINT("Building synchronous FSD Request...\n");
    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,
                                       DeviceObject,
                                       ReadBuffer,
                                       RealLength,
                                       &Offset,
                                       &Event,
                                       &IoStatus);
    if (Irp == NULL)
    {
        DPRINT("IoBuildSynchronousFsdRequest failed\n");

        if (AllocatedBuffer)
        {
            ExFreePoolWithTag(ReadBuffer, TAG_NTFS);
        }

        return STATUS_INSUFFICIENT_RESOURCES;
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

    if (AllocatedBuffer)
    {
        if (NT_SUCCESS(Status))
        {
            RtlCopyMemory(Buffer, ReadBuffer + (StartingOffset - RealReadOffset), Length);
        }

        ExFreePoolWithTag(ReadBuffer, TAG_NTFS);
    }

    DPRINT("NtfsReadDisk() done (Status %x)\n", Status);

    return Status;
}

/**
* @name NtfsWriteDisk
* @implemented
*
* Writes data from the given buffer to the given DeviceObject.
*
* @param DeviceObject
* Device to write to
*
* @param StartingOffset
* Offset, in bytes, from the start of the device object where the data will be written
*
* @param Length
* How much data will be written, in bytes
*
* @param SectorSize
* Size of the sector on the disk that the write must be aligned to
*
* @param Buffer
* The data that's being written to the device
*
* @return
* STATUS_SUCCESS in case of success, STATUS_INSUFFICIENT_RESOURCES if a memory allocation failed,
* or whatever status IoCallDriver() sets.
*
* @remarks Called by NtfsWriteFile(). May perform a read-modify-write operation if the
* requested write is not sector-aligned.
*
*/
NTSTATUS
NtfsWriteDisk(IN PDEVICE_OBJECT DeviceObject,
              IN LONGLONG StartingOffset,
              IN ULONG Length,
              IN ULONG SectorSize,
              IN const PUCHAR Buffer)
{
    IO_STATUS_BLOCK IoStatus;
    LARGE_INTEGER Offset;
    KEVENT Event;
    PIRP Irp;
    NTSTATUS Status;
    ULONGLONG RealWriteOffset;
    ULONG RealLength;
    BOOLEAN AllocatedBuffer = FALSE;
    PUCHAR TempBuffer = NULL;

    DPRINT("NtfsWriteDisk(%p, %I64x, %lu, %lu, %p)\n", DeviceObject, StartingOffset, Length, SectorSize, Buffer);

    if (Length == 0)
        return STATUS_SUCCESS;

    RealWriteOffset = (ULONGLONG)StartingOffset;
    RealLength = Length;

    // Does the write need to be adjusted to be sector-aligned?
    if ((RealWriteOffset % SectorSize) != 0 || (RealLength % SectorSize) != 0)
    {
        ULONGLONG relativeOffset;

        // We need to do a read-modify-write. We'll start be copying the entire
        // contents of every sector that will be overwritten.
        // TODO: Optimize (read no more than necessary)

        RealWriteOffset = ROUND_DOWN(StartingOffset, SectorSize);
        RealLength = ROUND_UP(Length, SectorSize);

        // Would the end of our sector-aligned write fall short of the requested write?
        if (RealWriteOffset + RealLength < StartingOffset + Length)
        {
            RealLength += SectorSize;
        }

        // Did we underestimate the memory required somehow?
        if (RealLength + RealWriteOffset < StartingOffset + Length)
        {
            DPRINT1("\a\t\t\t\t\tFIXME: calculated less memory than needed!\n");
            DPRINT1("StartingOffset: %lu\tLength: %lu\tRealWriteOffset: %lu\tRealLength: %lu\n",
                    StartingOffset, Length, RealWriteOffset, RealLength);

            RealLength += SectorSize;
        }

        // Allocate a buffer to copy the existing data to
        TempBuffer = ExAllocatePoolWithTag(NonPagedPool, RealLength, TAG_NTFS);

        // Did we fail to allocate it?
        if (TempBuffer == NULL)
        {
            DPRINT1("Not enough memory!\n");

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        // Read the sectors we'll be overwriting into TempBuffer
        Status = NtfsReadDisk(DeviceObject, RealWriteOffset, RealLength, SectorSize, TempBuffer, FALSE);

        // Did we fail the read?
        if (!NT_SUCCESS(Status))
        {
            RtlSecureZeroMemory(TempBuffer, RealLength);
            ExFreePoolWithTag(TempBuffer, TAG_NTFS);
            return Status;
        }

        // Calculate where the new data should be written to, relative to the start of TempBuffer
        relativeOffset = StartingOffset - RealWriteOffset;

        // Modify the tempbuffer with the data being read
        RtlCopyMemory(TempBuffer + relativeOffset, Buffer, Length);

        AllocatedBuffer = TRUE;
    }

    // set the destination offset
    Offset.QuadPart = RealWriteOffset;

    // setup the notification event for the write
    KeInitializeEvent(&Event,
                      NotificationEvent,
                      FALSE);

    DPRINT("Building synchronous FSD Request...\n");

    // Build an IRP requesting the lower-level [disk] driver to perform the write
    // TODO: Forward the existing IRP instead
    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_WRITE,
                                       DeviceObject,
                                       // if we allocated a temp buffer, use that instead of the Buffer parameter
                                       ((AllocatedBuffer) ? TempBuffer : Buffer),
                                       RealLength,
                                       &Offset,
                                       &Event,
                                       &IoStatus);
    // Did we fail to build the IRP?
    if (Irp == NULL)
    {
        DPRINT1("IoBuildSynchronousFsdRequest failed\n");

        if (AllocatedBuffer)
        {
            RtlSecureZeroMemory(TempBuffer, RealLength);
            ExFreePoolWithTag(TempBuffer, TAG_NTFS);
        }

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Call the next-lower driver to perform the write
    DPRINT("Calling IO Driver with irp %p\n", Irp);
    Status = IoCallDriver(DeviceObject, Irp);

    // Wait until the next-lower driver has completed the IRP
    DPRINT("Waiting for IO Operation for %p\n", Irp);
    if (Status == STATUS_PENDING)
    {
        DPRINT("Operation pending\n");
        KeWaitForSingleObject(&Event, Suspended, KernelMode, FALSE, NULL);
        DPRINT("Getting IO Status... for %p\n", Irp);
        Status = IoStatus.Status;
    }

    if (AllocatedBuffer)
    {
        // zero the buffer before freeing it, so private user data can't be snooped
        RtlSecureZeroMemory(TempBuffer, RealLength);

        ExFreePoolWithTag(TempBuffer, TAG_NTFS);
    }

    DPRINT("NtfsWriteDisk() done (Status %x)\n", Status);

    return Status;
}

NTSTATUS
NtfsReadSectors(IN PDEVICE_OBJECT DeviceObject,
                IN ULONG DiskSector,
                IN ULONG SectorCount,
                IN ULONG SectorSize,
                IN OUT PUCHAR Buffer,
                IN BOOLEAN Override)
{
    LONGLONG Offset;
    ULONG BlockSize;

    Offset = (LONGLONG)DiskSector * (LONGLONG)SectorSize;
    BlockSize = SectorCount * SectorSize;

    return NtfsReadDisk(DeviceObject, Offset, BlockSize, SectorSize, Buffer, Override);
}


NTSTATUS
NtfsDeviceIoControl(IN PDEVICE_OBJECT DeviceObject,
                    IN ULONG ControlCode,
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

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    DPRINT("Building device I/O control request ...\n");
    Irp = IoBuildDeviceIoControlRequest(ControlCode,
                                        DeviceObject,
                                        InputBuffer,
                                        InputBufferSize,
                                        OutputBuffer,
                                        (OutputBufferSize) ? *OutputBufferSize : 0,
                                        FALSE,
                                        &Event,
                                        &IoStatus);
    if (Irp == NULL)
    {
        DPRINT("IoBuildDeviceIoControlRequest() failed\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (Override)
    {
        Stack = IoGetNextIrpStackLocation(Irp);
        Stack->Flags |= SL_OVERRIDE_VERIFY_VOLUME;
    }

    DPRINT("Calling IO Driver... with irp %p\n", Irp);
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Suspended, KernelMode, FALSE, NULL);
        Status = IoStatus.Status;
    }

    if (OutputBufferSize)
    {
        *OutputBufferSize = IoStatus.Information;
    }

    return Status;
}

/* EOF */
