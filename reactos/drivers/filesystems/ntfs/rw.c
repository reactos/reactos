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
    ULONGLONG StreamSize;

    DPRINT1("NtfsReadFile(%p, %p, %p, %u, %u, %x, %p)\n", DeviceExt, FileObject, Buffer, Length, ReadOffset, IrpFlags, LengthRead);

    *LengthRead = 0;

    if (Length == 0)
    {
        DPRINT1("Null read!\n");
        return STATUS_SUCCESS;
    }

    Fcb = (PNTFS_FCB)FileObject->FsContext;

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


    Status = FindAttribute(DeviceExt, FileRecord, AttributeData, Fcb->Stream, wcslen(Fcb->Stream), &DataContext);
    if (!NT_SUCCESS(Status))
    {
        NTSTATUS BrowseStatus;
        FIND_ATTR_CONTXT Context;
        PNTFS_ATTR_RECORD Attribute;

        DPRINT1("No '%S' data stream associated with file!\n", Fcb->Stream);

        BrowseStatus = FindFirstAttribute(&Context, DeviceExt, FileRecord, FALSE, &Attribute);
        while (NT_SUCCESS(BrowseStatus))
        {
            if (Attribute->Type == AttributeData)
            {
                UNICODE_STRING Name;

                Name.Length = Attribute->NameLength * sizeof(WCHAR);
                Name.MaximumLength = Name.Length;
                Name.Buffer = (PWCHAR)((ULONG_PTR)Attribute + Attribute->NameOffset);
                DPRINT1("Data stream: '%wZ' available\n", &Name);
            }

            BrowseStatus = FindNextAttribute(&Context, &Attribute);
        }
        FindCloseAttribute(&Context);

        ReleaseAttributeContext(DataContext);
        ExFreePoolWithTag(FileRecord, TAG_NTFS);
        return Status;
    }

    StreamSize = AttributeDataLength(&DataContext->Record);
    if (ReadOffset >= StreamSize)
    {
        DPRINT1("Reading beyond stream end!\n");
        ReleaseAttributeContext(DataContext);
        ExFreePoolWithTag(FileRecord, TAG_NTFS);
        return STATUS_END_OF_FILE;
    }

    ToRead = Length;
    if (ReadOffset + Length > StreamSize)
        ToRead = StreamSize - ReadOffset;

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
            ReleaseAttributeContext(DataContext);
            ExFreePoolWithTag(FileRecord, TAG_NTFS);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        AllocatedBuffer = TRUE;
    }

    DPRINT1("Effective read: %lu at %lu for stream '%S'\n", RealLength, RealReadOffset, Fcb->Stream);
    RealLengthRead = ReadAttribute(DeviceExt, DataContext, RealReadOffset, (PCHAR)ReadBuffer, RealLength);
    if (RealLengthRead == 0)
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
    Buffer = NtfsGetUserBuffer(Irp, BooleanFlagOn(Irp->Flags, IRP_PAGING_IO));

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
