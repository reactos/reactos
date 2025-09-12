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
 *                   Trevor Thompson
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

    DPRINT("NtfsReadFile(%p, %p, %p, %lu, %lu, %lx, %p)\n", DeviceExt, FileObject, Buffer, Length, ReadOffset, IrpFlags, LengthRead);

    *LengthRead = 0;

    if (Length == 0)
    {
        DPRINT1("Null read!\n");
        return STATUS_SUCCESS;
    }

    Fcb = (PNTFS_FCB)FileObject->FsContext;

    if (NtfsFCBIsCompressed(Fcb))
    {
        DPRINT1("Compressed file!\n");
        UNIMPLEMENTED;
        return STATUS_NOT_IMPLEMENTED;
    }

    if (NtfsFCBIsEncrypted(Fcb))
    {
        DPRINT1("Encrypted file!\n");
        UNIMPLEMENTED;
        return STATUS_NOT_IMPLEMENTED;
    }

    FileRecord = ExAllocateFromNPagedLookasideList(&DeviceExt->FileRecLookasideList);
    if (FileRecord == NULL)
    {
        DPRINT1("Not enough memory!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = ReadFileRecord(DeviceExt, Fcb->MFTIndex, FileRecord);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Can't find record!\n");
        ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, FileRecord);
        return Status;
    }


    Status = FindAttribute(DeviceExt, FileRecord, AttributeData, Fcb->Stream, wcslen(Fcb->Stream), &DataContext, NULL);
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
        ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, FileRecord);
        return Status;
    }

    StreamSize = AttributeDataLength(DataContext->pRecord);
    if (ReadOffset >= StreamSize)
    {
        DPRINT1("Reading beyond stream end!\n");
        ReleaseAttributeContext(DataContext);
        ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, FileRecord);
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
        /* do we need to extend RealLength by one sector? */
        if (RealLength + RealReadOffset < ReadOffset + Length)
        {
            if (RealReadOffset + RealLength + DeviceExt->NtfsInfo.BytesPerSector <= AttributeAllocatedLength(DataContext->pRecord))
                RealLength += DeviceExt->NtfsInfo.BytesPerSector;
        }


        ReadBuffer = ExAllocatePoolWithTag(NonPagedPool, RealLength, TAG_NTFS);
        if (ReadBuffer == NULL)
        {
            DPRINT1("Not enough memory!\n");
            ReleaseAttributeContext(DataContext);
            ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, FileRecord);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        AllocatedBuffer = TRUE;
    }

    DPRINT("Effective read: %lu at %lu for stream '%S'\n", RealLength, RealReadOffset, Fcb->Stream);
    RealLengthRead = ReadAttribute(DeviceExt, DataContext, RealReadOffset, (PCHAR)ReadBuffer, RealLength);
    if (RealLengthRead == 0)
    {
        DPRINT1("Read failure!\n");
        ReleaseAttributeContext(DataContext);
        ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, FileRecord);
        if (AllocatedBuffer)
        {
            ExFreePoolWithTag(ReadBuffer, TAG_NTFS);
        }
        return Status;
    }

    ReleaseAttributeContext(DataContext);
    ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, FileRecord);

    *LengthRead = ToRead;

    DPRINT("%lu got read\n", *LengthRead);

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

/**
* @name NtfsWriteFile
* @implemented
*
* Writes a file to the disk. It presently borrows a lot of code from NtfsReadFile() and
* VFatWriteFileData(). It needs some more work before it will be complete; it won't handle
* page files, asnyc io, cached writes, etc.
*
* @param DeviceExt
* Points to the target disk's DEVICE_EXTENSION
*
* @param FileObject
* Pointer to a FILE_OBJECT describing the target file
*
* @param Buffer
* The data that's being written to the file
*
* @Param Length
* The size of the data buffer being written, in bytes
*
* @param WriteOffset
* Offset, in bytes, from the beginning of the file. Indicates where to start
* writing data.
*
* @param IrpFlags
* TODO: flags are presently ignored in code.
*
* @param CaseSensitive
* Boolean indicating if the function should operate in case-sensitive mode. This will be TRUE
* if an application opened the file with the FILE_FLAG_POSIX_SEMANTICS flag.
*
* @param LengthWritten
* Pointer to a ULONG. This ULONG will be set to the number of bytes successfully written.
*
* @return
* STATUS_SUCCESS if successful, STATUS_NOT_IMPLEMENTED if a required feature isn't implemented,
* STATUS_INSUFFICIENT_RESOURCES if an allocation failed, STATUS_ACCESS_DENIED if the write itself fails,
* STATUS_PARTIAL_COPY or STATUS_UNSUCCESSFUL if ReadFileRecord() fails, or
* STATUS_OBJECT_NAME_NOT_FOUND if the file's data stream could not be found.
*
* @remarks Called by NtfsWrite(). It may perform a read-modify-write operation if the requested write is
* not sector-aligned. LengthWritten only refers to how much of the requested data has been written;
* extra data that needs to be written to make the write sector-aligned will not affect it.
*
*/
NTSTATUS NtfsWriteFile(PDEVICE_EXTENSION DeviceExt,
                       PFILE_OBJECT FileObject,
                       const PUCHAR Buffer,
                       ULONG Length,
                       ULONG WriteOffset,
                       ULONG IrpFlags,
                       BOOLEAN CaseSensitive,
                       PULONG LengthWritten)
{
    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
    PNTFS_FCB Fcb;
    PFILE_RECORD_HEADER FileRecord;
    PNTFS_ATTR_CONTEXT DataContext;
    ULONG AttributeOffset;
    ULONGLONG StreamSize;

    DPRINT("NtfsWriteFile(%p, %p, %p, %lu, %lu, %x, %s, %p)\n",
           DeviceExt,
           FileObject,
           Buffer,
           Length,
           WriteOffset,
           IrpFlags,
           (CaseSensitive ? "TRUE" : "FALSE"),
           LengthWritten);

    *LengthWritten = 0;

    ASSERT(DeviceExt);

    if (Length == 0)
    {
        if (Buffer == NULL)
            return STATUS_SUCCESS;
        else
            return STATUS_INVALID_PARAMETER;
    }

    // get the File control block
    Fcb = (PNTFS_FCB)FileObject->FsContext;
    ASSERT(Fcb);

    DPRINT("Fcb->PathName: %wS\n", Fcb->PathName);
    DPRINT("Fcb->ObjectName: %wS\n", Fcb->ObjectName);

    // we don't yet handle compression
    if (NtfsFCBIsCompressed(Fcb))
    {
        DPRINT("Compressed file!\n");
        UNIMPLEMENTED;
        return STATUS_NOT_IMPLEMENTED;
    }

    // allocate non-paged memory for the FILE_RECORD_HEADER
    FileRecord = ExAllocateFromNPagedLookasideList(&DeviceExt->FileRecLookasideList);
    if (FileRecord == NULL)
    {
        DPRINT1("Not enough memory! Can't write %wS!\n", Fcb->PathName);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // read the FILE_RECORD_HEADER from the drive (or cache)
    DPRINT("Reading file record...\n");
    Status = ReadFileRecord(DeviceExt, Fcb->MFTIndex, FileRecord);
    if (!NT_SUCCESS(Status))
    {
        // We couldn't get the file's record. Free the memory and return the error
        DPRINT1("Can't find record for %wS!\n", Fcb->ObjectName);
        ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, FileRecord);
        return Status;
    }

    DPRINT("Found record for %wS\n", Fcb->ObjectName);

    // Find the attribute with the data stream for our file
    DPRINT("Finding Data Attribute...\n");
    Status = FindAttribute(DeviceExt, FileRecord, AttributeData, Fcb->Stream, wcslen(Fcb->Stream), &DataContext,
                           &AttributeOffset);

    // Did we fail to find the attribute?
    if (!NT_SUCCESS(Status))
    {
        NTSTATUS BrowseStatus;
        FIND_ATTR_CONTXT Context;
        PNTFS_ATTR_RECORD Attribute;

        DPRINT1("No '%S' data stream associated with file!\n", Fcb->Stream);

        // Couldn't find the requested data stream; print a list of streams available
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
        ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, FileRecord);
        return Status;
    }

    // Get the size of the stream on disk
    StreamSize = AttributeDataLength(DataContext->pRecord);

    DPRINT("WriteOffset: %lu\tStreamSize: %I64u\n", WriteOffset, StreamSize);

    // Are we trying to write beyond the end of the stream?
    if (WriteOffset + Length > StreamSize)
    {
        // is increasing the stream size allowed?
        if (!(Fcb->Flags & FCB_IS_VOLUME) &&
            !(IrpFlags & IRP_PAGING_IO))
        {
            LARGE_INTEGER DataSize;
            ULONGLONG AllocationSize;
            PFILENAME_ATTRIBUTE fileNameAttribute;
            ULONGLONG ParentMFTId;
            UNICODE_STRING filename;

            DataSize.QuadPart = WriteOffset + Length;

            // set the attribute data length
            Status = SetAttributeDataLength(FileObject, Fcb, DataContext, AttributeOffset, FileRecord, &DataSize);
            if (!NT_SUCCESS(Status))
            {
                ReleaseAttributeContext(DataContext);
                ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, FileRecord);
                *LengthWritten = 0;
                return Status;
            }

            AllocationSize = AttributeAllocatedLength(DataContext->pRecord);

            // now we need to update this file's size in every directory index entry that references it
            // TODO: put this code in its own function and adapt it to work with every filename / hardlink
            // stored in the file record.
            fileNameAttribute = GetBestFileNameFromRecord(Fcb->Vcb, FileRecord);
            ASSERT(fileNameAttribute);

            ParentMFTId = fileNameAttribute->DirectoryFileReferenceNumber & NTFS_MFT_MASK;

            filename.Buffer = fileNameAttribute->Name;
            filename.Length = fileNameAttribute->NameLength * sizeof(WCHAR);
            filename.MaximumLength = filename.Length;

            Status = UpdateFileNameRecord(Fcb->Vcb,
                                          ParentMFTId,
                                          &filename,
                                          FALSE,
                                          DataSize.QuadPart,
                                          AllocationSize,
                                          CaseSensitive);

        }
        else
        {
            // TODO - just fail for now
            ReleaseAttributeContext(DataContext);
            ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, FileRecord);
            *LengthWritten = 0;
            return STATUS_ACCESS_DENIED;
        }
    }

    DPRINT("Length: %lu\tWriteOffset: %lu\tStreamSize: %I64u\n", Length, WriteOffset, StreamSize);

    // Write the data to the attribute
    Status = WriteAttribute(DeviceExt, DataContext, WriteOffset, Buffer, Length, LengthWritten, FileRecord);

    // Did the write fail?
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Write failure!\n");
        ReleaseAttributeContext(DataContext);
        ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, FileRecord);

        return Status;
    }

    // This should never happen:
    if (*LengthWritten != Length)
    {
        DPRINT1("\a\tNTFS DRIVER ERROR: length written (%lu) differs from requested (%lu), but no error was indicated!\n",
            *LengthWritten, Length);
        Status = STATUS_UNEXPECTED_IO_ERROR;
    }

    ReleaseAttributeContext(DataContext);
    ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, FileRecord);

    return Status;
}

/**
* @name NtfsWrite
* @implemented
*
* Handles IRP_MJ_WRITE I/O Request Packets for NTFS. This code borrows a lot from
* VfatWrite, and needs a lot of cleaning up. It also needs a lot more of the code
* from VfatWrite integrated.
*
* @param IrpContext
* Points to an NTFS_IRP_CONTEXT which describes the write
*
* @return
* STATUS_SUCCESS if successful,
* STATUS_INSUFFICIENT_RESOURCES if an allocation failed,
* STATUS_INVALID_DEVICE_REQUEST if called on the main device object,
* STATUS_NOT_IMPLEMENTED or STATUS_ACCESS_DENIED if a required feature isn't implemented.
* STATUS_PARTIAL_COPY, STATUS_UNSUCCESSFUL, or STATUS_OBJECT_NAME_NOT_FOUND if NtfsWriteFile() fails.
*
* @remarks Called by NtfsDispatch() in response to an IRP_MJ_WRITE request. Page files are not implemented.
* Support for large files (>4gb) is not implemented. Cached writes, file locks, transactions, etc - not implemented.
*
*/
NTSTATUS
NtfsWrite(PNTFS_IRP_CONTEXT IrpContext)
{
    PNTFS_FCB Fcb;
    PERESOURCE Resource = NULL;
    LARGE_INTEGER ByteOffset;
    PUCHAR Buffer;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Length = 0;
    ULONG ReturnedWriteLength = 0;
    PDEVICE_OBJECT DeviceObject = NULL;
    PDEVICE_EXTENSION DeviceExt = NULL;
    PFILE_OBJECT FileObject = NULL;
    PIRP Irp = NULL;
    ULONG BytesPerSector;

    DPRINT("NtfsWrite(IrpContext %p)\n", IrpContext);
    ASSERT(IrpContext);

    // get the I/O request packet
    Irp = IrpContext->Irp;

    // This request is not allowed on the main device object
    if (IrpContext->DeviceObject == NtfsGlobalData->DeviceObject)
    {
        DPRINT1("\t\t\t\tNtfsWrite is called with the main device object.\n");

        Irp->IoStatus.Information = 0;
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    // get the File control block
    Fcb = (PNTFS_FCB)IrpContext->FileObject->FsContext;
    ASSERT(Fcb);

    DPRINT("About to write %wS\n", Fcb->ObjectName);
    DPRINT("NTFS Version: %d.%d\n", Fcb->Vcb->NtfsInfo.MajorVersion, Fcb->Vcb->NtfsInfo.MinorVersion);

    // setup some more locals
    FileObject = IrpContext->FileObject;
    DeviceObject = IrpContext->DeviceObject;
    DeviceExt = DeviceObject->DeviceExtension;
    BytesPerSector = DeviceExt->StorageDevice->SectorSize;
    Length = IrpContext->Stack->Parameters.Write.Length;

    // get the file offset we'll be writing to
    ByteOffset = IrpContext->Stack->Parameters.Write.ByteOffset;
    if (ByteOffset.u.LowPart == FILE_WRITE_TO_END_OF_FILE &&
        ByteOffset.u.HighPart == -1)
    {
        ByteOffset.QuadPart = Fcb->RFCB.FileSize.QuadPart;
    }

    DPRINT("ByteOffset: %I64u\tLength: %lu\tBytes per sector: %lu\n", ByteOffset.QuadPart,
        Length, BytesPerSector);

    if (ByteOffset.u.HighPart && !(Fcb->Flags & FCB_IS_VOLUME))
    {
        // TODO: Support large files
        DPRINT1("FIXME: Writing to large files is not yet supported at this time.\n");
        return STATUS_INVALID_PARAMETER;
    }

    // Is this a non-cached write? A non-buffered write?
    if (IrpContext->Irp->Flags & (IRP_PAGING_IO | IRP_NOCACHE) || (Fcb->Flags & FCB_IS_VOLUME) ||
        IrpContext->FileObject->Flags & FILE_NO_INTERMEDIATE_BUFFERING)
    {
        // non-cached and non-buffered writes must be sector aligned
        if (ByteOffset.u.LowPart % BytesPerSector != 0 || Length % BytesPerSector != 0)
        {
            DPRINT1("Non-cached writes and non-buffered writes must be sector aligned!\n");
            return STATUS_INVALID_PARAMETER;
        }
    }

    if (Length == 0)
    {
        DPRINT1("Null write!\n");

        IrpContext->Irp->IoStatus.Information = 0;

        // FIXME: Doesn't accurately detect when a user passes NULL to WriteFile() for the buffer
        if (Irp->UserBuffer == NULL && Irp->MdlAddress == NULL)
        {
            // FIXME: Update last write time
            return STATUS_SUCCESS;
        }

        return STATUS_INVALID_PARAMETER;
    }

    // get the Resource
    if (Fcb->Flags & FCB_IS_VOLUME)
    {
        Resource = &DeviceExt->DirResource;
    }
    else if (IrpContext->Irp->Flags & IRP_PAGING_IO)
    {
        Resource = &Fcb->PagingIoResource;
    }
    else
    {
        Resource = &Fcb->MainResource;
    }

    // acquire exclusive access to the Resource
    if (!ExAcquireResourceExclusiveLite(Resource, BooleanFlagOn(IrpContext->Flags, IRPCONTEXT_CANWAIT)))
    {
        return STATUS_CANT_WAIT;
    }

    /* From VfatWrite(). Todo: Handle file locks
    if (!(IrpContext->Irp->Flags & IRP_PAGING_IO) &&
    FsRtlAreThereCurrentFileLocks(&Fcb->FileLock))
    {
    if (!FsRtlCheckLockForWriteAccess(&Fcb->FileLock, IrpContext->Irp))
    {
    Status = STATUS_FILE_LOCK_CONFLICT;
    goto ByeBye;
    }
    }*/

    // Is this an async request to a file?
    if (!(IrpContext->Flags & IRPCONTEXT_CANWAIT) && !(Fcb->Flags & FCB_IS_VOLUME))
    {
        DPRINT1("FIXME: Async writes not supported in NTFS!\n");

        ExReleaseResourceLite(Resource);
        return STATUS_NOT_IMPLEMENTED;
    }

    // get the buffer of data the user is trying to write
    Buffer = NtfsGetUserBuffer(Irp, BooleanFlagOn(Irp->Flags, IRP_PAGING_IO));
    ASSERT(Buffer);

    // lock the buffer
    Status = NtfsLockUserBuffer(Irp, Length, IoReadAccess);

    // were we unable to lock the buffer?
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Unable to lock user buffer!\n");

        ExReleaseResourceLite(Resource);
        return Status;
    }

    DPRINT("Existing File Size(Fcb->RFCB.FileSize.QuadPart): %I64u\n", Fcb->RFCB.FileSize.QuadPart);
    DPRINT("About to write the data. Length: %lu\n", Length);

    // TODO: handle HighPart of ByteOffset (large files)

    // write the file
    Status = NtfsWriteFile(DeviceExt,
                           FileObject,
                           Buffer,
                           Length,
                           ByteOffset.LowPart,
                           Irp->Flags,
                           BooleanFlagOn(IrpContext->Stack->Flags, SL_CASE_SENSITIVE),
                           &ReturnedWriteLength);

    IrpContext->Irp->IoStatus.Status = Status;

    // was the write successful?
    if (NT_SUCCESS(Status))
    {
        // TODO: Update timestamps

        if (FileObject->Flags & FO_SYNCHRONOUS_IO)
        {
            // advance the file pointer
            FileObject->CurrentByteOffset.QuadPart = ByteOffset.QuadPart + ReturnedWriteLength;
        }

        IrpContext->PriorityBoost = IO_DISK_INCREMENT;
    }
    else
    {
        DPRINT1("Write not Succesful!\tReturned length: %lu\n", ReturnedWriteLength);
    }

    Irp->IoStatus.Information = ReturnedWriteLength;

    // Note: We leave the user buffer that we locked alone, it's up to the I/O manager to unlock and free it

    ExReleaseResourceLite(Resource);

    return Status;
}

/* EOF */
