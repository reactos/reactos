/*
 *  ReactOS kernel
 *  Copyright (C) 2002, 2014 ReactOS Team
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
 * FILE:             drivers/filesystem/ntfs/rw.c
 * PURPOSE:          NTFS filesystem driver
 * PROGRAMMERS:      Art Yerkes
 *                   Pierre Schweitzer (pierre@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntddk.h>
#include "ntfs.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

/*
 * FUNCTION: Reads data from a file
 */
static
NTSTATUS
NtfsReadFile(PDEVICE_EXTENSION DeviceExt,
             PFILE_OBJECT FileObject,
             PUCHAR Buffer,
             ULONG Length,
             ULONG ReadOffset,
             ULONG IrpFlags,
             PULONG LengthRead)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PNTFS_FCB Fcb;
    PFILE_RECORD_HEADER FileRecord;
    PNTFS_ATTR_CONTEXT DataContext;
    ULONG RealLength;
    ULONG RealReadOffset;
    ULONG RealLengthRead;
    ULONG ToRead;
    BOOLEAN AllocatedBuffer = FALSE;
    PCHAR ReadBuffer = (PCHAR)Buffer;

    DPRINT1("NtfsReadFile(%p, %p, %p, %u, %u, %x, %p)\n", DeviceExt, FileObject, Buffer, Length, ReadOffset, IrpFlags, LengthRead);

    *LengthRead = 0;

    if (Length == 0)
    {
        DPRINT1("Null read!\n");
        return STATUS_SUCCESS;
    }

    Fcb = (PNTFS_FCB)FileObject->FsContext;

    if (ReadOffset >= Fcb->Entry.AllocatedSize)
    {
        DPRINT1("Reading beyond file end!\n");
        return STATUS_END_OF_FILE;
    }

    ToRead = Length;
    if (ReadOffset + Length > Fcb->Entry.AllocatedSize)
        ToRead = Fcb->Entry.AllocatedSize - ReadOffset;

    RealReadOffset = ReadOffset;
    RealLength = ToRead;

    if ((ReadOffset % DeviceExt->NtfsInfo.BytesPerSector) != 0 || (ToRead % DeviceExt->NtfsInfo.BytesPerSector) != 0)
    {
        RealReadOffset = ROUND_DOWN(ReadOffset, DeviceExt->NtfsInfo.BytesPerSector);
        RealLength = ROUND_UP(ToRead, DeviceExt->NtfsInfo.BytesPerSector);

        ReadBuffer = ExAllocatePoolWithTag(NonPagedPool, RealLength + DeviceExt->NtfsInfo.BytesPerSector, TAG_NTFS);
        if (ReadBuffer == NULL)
        {
            DPRINT1("Not enough memory!\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        AllocatedBuffer = TRUE;
    }

    FileRecord = ExAllocatePoolWithTag(NonPagedPool, DeviceExt->NtfsInfo.BytesPerFileRecord, TAG_NTFS);
    if (FileRecord == NULL)
    {
        DPRINT1("Not enough memory!\n");
        if (AllocatedBuffer)
        {
            ExFreePoolWithTag(ReadBuffer, TAG_NTFS);
        }
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = ReadFileRecord(DeviceExt, Fcb->MFTIndex, FileRecord);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Can't find record!\n");
        ExFreePoolWithTag(FileRecord, TAG_NTFS);
        if (AllocatedBuffer)
        {
            ExFreePoolWithTag(ReadBuffer, TAG_NTFS);
        }
        return Status;
    }

    Status = FindAttribute(DeviceExt, FileRecord, AttributeData, L"", 0, &DataContext);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("No data associated with file!\n");
        ExFreePoolWithTag(FileRecord, TAG_NTFS);
        if (AllocatedBuffer)
        {
            ExFreePoolWithTag(ReadBuffer, TAG_NTFS);
        }
        return Status;
    }

    DPRINT1("Effective read: %lu at %lu\n", RealLength, RealReadOffset);
    RealLengthRead = ReadAttribute(DeviceExt, DataContext, RealReadOffset, (PCHAR)ReadBuffer, RealLength);
    if (RealLengthRead != RealLength)
    {
        DPRINT1("Read failure!\n");
        ReleaseAttributeContext(DataContext);
        ExFreePoolWithTag(FileRecord, TAG_NTFS);
        if (AllocatedBuffer)
        {
            ExFreePoolWithTag(ReadBuffer, TAG_NTFS);
        }
        return Status;
    }

    ReleaseAttributeContext(DataContext);
    ExFreePoolWithTag(FileRecord, TAG_NTFS);

    *LengthRead = ToRead;

    DPRINT1("%lu got read\n", *LengthRead);

    if (AllocatedBuffer)
    {
        RtlCopyMemory(Buffer, ReadBuffer + (ReadOffset - RealReadOffset), ToRead);
    }

    if (ToRead != Length)
    {
        RtlZeroMemory(Buffer + ToRead, Length - ToRead);
    }

    if (AllocatedBuffer)
    {
        ExFreePoolWithTag(ReadBuffer, TAG_NTFS);
    }

    return STATUS_SUCCESS;
}


NTSTATUS
NtfsRead(PNTFS_IRP_CONTEXT IrpContext)
{
    PDEVICE_EXTENSION DeviceExt;
    PIO_STACK_LOCATION Stack;
    PFILE_OBJECT FileObject;
    PVOID Buffer;
    ULONG ReadLength;
    LARGE_INTEGER ReadOffset;
    ULONG ReturnedReadLength = 0;
    NTSTATUS Status = STATUS_SUCCESS;
    PIRP Irp;
    PDEVICE_OBJECT DeviceObject;

    DPRINT("NtfsRead(IrpContext %p)\n", IrpContext);

    DeviceObject = IrpContext->DeviceObject;
    Irp = IrpContext->Irp;
    Stack = IrpContext->Stack;
    FileObject = IrpContext->FileObject;

    DeviceExt = DeviceObject->DeviceExtension;
    ReadLength = Stack->Parameters.Read.Length;
    ReadOffset = Stack->Parameters.Read.ByteOffset;
    Buffer = NtfsGetUserBuffer(Irp, Irp->Flags & IRP_PAGING_IO);

    Status = NtfsReadFile(DeviceExt,
                          FileObject,
                          Buffer,
                          ReadLength,
                          ReadOffset.u.LowPart,
                          Irp->Flags,
                          &ReturnedReadLength);
    if (NT_SUCCESS(Status))
    {
        if (FileObject->Flags & FO_SYNCHRONOUS_IO)
        {
            FileObject->CurrentByteOffset.QuadPart =
                ReadOffset.QuadPart + ReturnedReadLength;
        }

        Irp->IoStatus.Information = ReturnedReadLength;
    }
    else
    {
        Irp->IoStatus.Information = 0;
    }

    return Status;
}


NTSTATUS
NtfsWrite(PNTFS_IRP_CONTEXT IrpContext)
{
    DPRINT("NtfsWrite(IrpContext %p)\n",IrpContext);

    IrpContext->Irp->IoStatus.Information = 0;
    return STATUS_NOT_SUPPORTED;
}

/* EOF */
