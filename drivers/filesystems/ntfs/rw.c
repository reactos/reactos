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

/* Bounds on how far a growing file is allocated past its end */
#define NTFS_GROW_AHEAD_MIN (64 * 1024)
#define NTFS_GROW_AHEAD_MAX (4 * 1024 * 1024)

/* FUNCTIONS ****************************************************************/

/**
* @name NtfsReadWriteVolume
* @implemented
*
* Serves a read or write made through a handle on the volume itself.
*
* @param DeviceExt
* Points to the target disk's DEVICE_EXTENSION
*
* @param MajorFunction
* IRP_MJ_READ or IRP_MJ_WRITE
*
* @param Buffer
* System-space buffer to transfer to or from
*
* @param Length
* How much to transfer, in bytes
*
* @param Offset
* Byte offset from the start of the volume
*
* @param Irp
* The request being served, so that its buffer can be used directly
*
* @param LengthTransferred
* Receives how much was transferred
*
* @return
* STATUS_SUCCESS on success, otherwise the status of the transfer.
*
* @remarks A volume handle is raw access to the partition: it has no file
* record and no attributes. Running it through the attribute code would
* resolve the volume FCB's MFT index of zero and redirect the transfer into
* $MFT's own $DATA attribute.
*
*/
static
NTSTATUS
NtfsReadWriteVolume(PDEVICE_EXTENSION DeviceExt,
                    UCHAR MajorFunction,
                    PUCHAR Buffer,
                    ULONG Length,
                    LONGLONG Offset,
                    PIRP Irp,
                    PULONG LengthTransferred)
{
    NTFS_IO_RUN_LIST RunList;
    NTSTATUS Status;

    DPRINT("NtfsReadWriteVolume(%p, %u, %p, %lu, %I64u, %p, %p)\n",
           DeviceExt, MajorFunction, Buffer, Length, Offset, Irp, LengthTransferred);

    *LengthTransferred = 0;

    if (Length == 0)
        return STATUS_SUCCESS;

    NtfsInitIoRunList(&RunList);

    Status = NtfsAddIoRun(&RunList, Offset, Length);
    if (NT_SUCCESS(Status))
    {
        Status = NtfsPerformIrpIoRuns(DeviceExt->StorageDevice,
                                      MajorFunction,
                                      DeviceExt->NtfsInfo.BytesPerSector,
                                      Irp,
                                      Buffer,
                                      &RunList,
                                      FALSE,
                                      LengthTransferred);
    }

    NtfsFreeIoRunList(&RunList);

    return Status;
}

/*
 * FUNCTION: Reads data from a file
 */
static
NTSTATUS
NtfsReadFile(PDEVICE_EXTENSION DeviceExt,
             PFILE_OBJECT FileObject,
             PUCHAR Buffer,
             ULONG Length,
             ULONGLONG ReadOffset,
             ULONG IrpFlags,
             PIRP Irp,
             PULONG LengthRead)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PNTFS_FCB Fcb;
    PFILE_RECORD_HEADER FileRecord;
    PNTFS_ATTR_CONTEXT DataContext = NULL;
    ULONG RealLength;
    ULONGLONG RealReadOffset;
    ULONG RealLengthRead;
    ULONG ToRead;
    BOOLEAN AllocatedBuffer = FALSE;
    PCHAR ReadBuffer = (PCHAR)Buffer;
    ULONGLONG StreamSize;

    DPRINT("NtfsReadFile(%p, %p, %p, %lu, %I64u, %lx, %p)\n", DeviceExt, FileObject, Buffer, Length, ReadOffset, IrpFlags, LengthRead);

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

        DPRINT1("No '%S' data stream associated with file '%wS' (MFT record %I64u, record flags 0x%x)!\n",
                Fcb->Stream, Fcb->ObjectName, Fcb->MFTIndex, FileRecord->Flags);

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

        /* FindAttribute() failed, so DataContext was never set; there is
         * nothing to release here. */
        ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, FileRecord);
        return Status;
    }

    StreamSize = AttributeDataLength(DataContext->pRecord);
    if (ReadOffset >= StreamSize)
    {
        DPRINT("Reading beyond stream end!\n");
        ReleaseAttributeContext(DataContext);
        ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, FileRecord);
        return STATUS_END_OF_FILE;
    }

    ToRead = Length;
    if (ReadOffset + Length > StreamSize)
        ToRead = (ULONG)(StreamSize - ReadOffset);

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

    DPRINT("Effective read: %lu at %I64u for stream '%S'\n", RealLength, RealReadOffset, Fcb->Stream);
    /* Only hand the IRP down when reading straight into its buffer: a
     * forwarded transfer lands at the start of the IRP's MDL, so a bounced
     * read would end up in the wrong place */
    RealLengthRead = ReadAttributeToIrp(DeviceExt,
                                        DataContext,
                                        RealReadOffset,
                                        (PCHAR)ReadBuffer,
                                        RealLength,
                                        AllocatedBuffer ? NULL : Irp);
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
        RtlCopyMemory(Buffer, ReadBuffer + (ULONG)(ReadOffset - RealReadOffset), ToRead);
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

/* Sets up the cache map of a handle on its first cached I/O */
static
VOID
NTAPI
NtfsEnsureCacheMap(
    _In_ PFILE_OBJECT FileObject,
    _In_ PNTFS_FCB Fcb)
{
    if (FileObject->PrivateCacheMap == NULL)
    {
        CcInitializeCacheMap(FileObject,
                             (PCC_FILE_SIZES)&Fcb->RFCB.AllocationSize,
                             FALSE,
                             &NtfsGlobalData->CacheMgrCallbacks,
                             Fcb);
    }
}

static
NTSTATUS
NTAPI
NtfsCachedRead(
    _In_ PNTFS_IRP_CONTEXT IrpContext,
    _In_ PNTFS_FCB Fcb,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _In_ PLARGE_INTEGER ReadOffset,
    _Out_ PULONG LengthRead)
{
    PFILE_OBJECT FileObject = IrpContext->FileObject;
    BOOLEAN CanWait = BooleanFlagOn(IrpContext->Flags, IRPCONTEXT_CANWAIT);
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS Status;
    BOOLEAN Done = FALSE;

    *LengthRead = 0;

    if (NtfsFCBIsCompressed(Fcb) || NtfsFCBIsEncrypted(Fcb))
    {
        UNIMPLEMENTED;
        return STATUS_NOT_IMPLEMENTED;
    }

    if (!ExAcquireResourceSharedLite(&Fcb->MainResource, CanWait))
        goto Post;

    if (ReadOffset->QuadPart >= Fcb->RFCB.FileSize.QuadPart)
    {
        ExReleaseResourceLite(&Fcb->MainResource);
        return STATUS_END_OF_FILE;
    }

    if (Length > Fcb->RFCB.FileSize.QuadPart - ReadOffset->QuadPart)
        Length = (ULONG)(Fcb->RFCB.FileSize.QuadPart - ReadOffset->QuadPart);

    _SEH2_TRY
    {
        NtfsEnsureCacheMap(FileObject, Fcb);

        Done = CcCopyRead(FileObject, ReadOffset, Length, CanWait, Buffer, &IoStatus);
        Status = Done ? IoStatus.Status : STATUS_PENDING;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Done = TRUE;
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    ExReleaseResourceLite(&Fcb->MainResource);

    if (Done)
    {
        if (NT_SUCCESS(Status))
            *LengthRead = (ULONG)IoStatus.Information;

        return Status;
    }

Post:
    /* The data isn't in yet, read it from a worker that can wait */
    Status = NtfsLockUserBuffer(IrpContext->Irp, Length, IoWriteAccess);
    if (!NT_SUCCESS(Status))
        return Status;

    return NtfsMarkIrpContextForQueue(IrpContext);
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

    /* Once dismounted the metadata can't be trusted; only raw volume access
     * is still served */
    if ((DeviceExt->Flags & VCB_VOLUME_DISMOUNTED) &&
        !(((PNTFS_FCB)FileObject->FsContext)->Flags & FCB_IS_VOLUME))
    {
        Irp->IoStatus.Information = 0;
        return STATUS_VOLUME_DISMOUNTED;
    }

    /* A handle on the volume itself is raw access to the partition. */
    if (((PNTFS_FCB)FileObject->FsContext)->Flags & FCB_IS_VOLUME)
    {
        Status = NtfsReadWriteVolume(DeviceExt,
                                     IRP_MJ_READ,
                                     Buffer,
                                     ReadLength,
                                     ReadOffset.QuadPart,
                                     Irp,
                                     &ReturnedReadLength);
    }
    else if (NtfsFCBIsDirectory((PNTFS_FCB)FileObject->FsContext))
    {
        /* A directory has no data stream to read */
        Irp->IoStatus.Information = 0;
        return STATUS_INVALID_DEVICE_REQUEST;
    }
    else if (!(Irp->Flags & IRP_PAGING_IO) && NtfsIsRegularFileObject(FileObject) &&
             !FsRtlCheckLockForReadAccess(&((PNTFS_FCB)FileObject->FsContext)->FileLock, Irp))
    {
        Irp->IoStatus.Information = 0;
        return STATUS_FILE_LOCK_CONFLICT;
    }
    else if (!(Irp->Flags & (IRP_PAGING_IO | IRP_NOCACHE)) && NtfsIsRegularFileObject(FileObject))
    {
        Status = NtfsCachedRead(IrpContext,
                                (PNTFS_FCB)FileObject->FsContext,
                                Buffer,
                                ReadLength,
                                &ReadOffset,
                                &ReturnedReadLength);
        if (Status == STATUS_PENDING)
            return Status;
    }
    else
    {
        PNTFS_FCB Fcb = (PNTFS_FCB)FileObject->FsContext;

        /* A non cached read must not miss what is still dirty in the cache */
        if (!(Irp->Flags & IRP_PAGING_IO) && NtfsIsRegularFileObject(FileObject) &&
            Fcb->SectionObjectPointers.DataSectionObject != NULL)
        {
            IO_STATUS_BLOCK IoStatus;

            CcFlushCache(&Fcb->SectionObjectPointers, &ReadOffset, ReadLength, &IoStatus);
        }

        Status = NtfsReadFile(DeviceExt,
                              FileObject,
                              Buffer,
                              ReadLength,
                              ReadOffset.QuadPart,
                              Irp->Flags,
                              Irp,
                              &ReturnedReadLength);
    }
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

/* Grows the stream to NewEnd. The caller holds the main resource exclusive. */
static
NTSTATUS
NTAPI
NtfsGrowStream(
    _In_ PDEVICE_EXTENSION DeviceExt,
    _In_ PFILE_OBJECT FileObject,
    _In_ PNTFS_FCB Fcb,
    _In_ PNTFS_ATTR_CONTEXT DataContext,
    _In_ ULONG AttributeOffset,
    _Inout_ PFILE_RECORD_HEADER FileRecord,
    _In_ ULONGLONG NewEnd,
    _In_ BOOLEAN CaseSensitive)
{
    LARGE_INTEGER DataSize;
    NTSTATUS Status;

    DataSize.QuadPart = NewEnd;

    /* Paging writes look the runs up again, keep them out while those change */
    ExAcquireResourceExclusiveLite(&Fcb->PagingIoResource, TRUE);

    /* Reserve room past the new end, so the appends that follow fit
     * without touching the bitmap or the run list. Cleanup gives back
     * what's left over. */
    if (DataContext->pRecord->IsNonResident &&
        (ULONGLONG)DataSize.QuadPart > DataContext->pRecord->NonResident.AllocatedSize)
    {
        ULONGLONG Ahead;

        Ahead = min(max(DataSize.QuadPart / 4, NTFS_GROW_AHEAD_MIN), NTFS_GROW_AHEAD_MAX);
        Status = NtfsExtendAllocation(DeviceExt,
                                      DataContext,
                                      AttributeOffset,
                                      FileRecord,
                                      ROUND_UP(DataSize.QuadPart + Ahead, DeviceExt->NtfsInfo.BytesPerCluster));
        if (!NT_SUCCESS(Status))
        {
            /* Not fatal; SetAttributeDataLength() still allocates what's needed */
            DPRINT("Couldn't reserve ahead of %wS (Status %lx)\n", Fcb->ObjectName, Status);
        }
    }

    // set the attribute data length
    Status = SetAttributeDataLength(FileObject, Fcb, DataContext, AttributeOffset, FileRecord, &DataSize);

    ExReleaseResourceLite(&Fcb->PagingIoResource);

    if (!NT_SUCCESS(Status))
        return Status;

    // The file grew, so every copy of its size has to follow.
    // TODO: adapt this to every filename / hardlink in the record.
    NtfsUpdateDuplicatedInformation(Fcb->Vcb,
                                    FileRecord,
                                    Fcb->MFTIndex,
                                    NTFS_FILENAME_UPDATE_SIZES,
                                    CaseSensitive);

    return STATUS_SUCCESS;
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
                       ULONGLONG WriteOffset,
                       ULONG IrpFlags,
                       BOOLEAN CaseSensitive,
                       PIRP Irp,
                       PULONG LengthWritten)
{
    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
    PNTFS_FCB Fcb;
    PFILE_RECORD_HEADER FileRecord;
    PNTFS_ATTR_CONTEXT DataContext = NULL;
    ULONG AttributeOffset;
    ULONGLONG StreamSize;
    ULONG RequestedLength = 0;

    DPRINT("NtfsWriteFile(%p, %p, %p, %lu, %I64u, %x, %s, %p)\n",
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

        DPRINT1("No '%S' data stream associated with file '%wS' (MFT record %I64u, record flags 0x%x)!\n",
                Fcb->Stream, Fcb->ObjectName, Fcb->MFTIndex, FileRecord->Flags);

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

        /* FindAttribute() failed, so DataContext was never set; there is
         * nothing to release here. */
        ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, FileRecord);
        return Status;
    }

    // Get the size of the stream on disk
    StreamSize = AttributeDataLength(DataContext->pRecord);

    DPRINT("WriteOffset: %I64u\tStreamSize: %I64u\n", WriteOffset, StreamSize);

    // Are we trying to write beyond the end of the stream?
    if (WriteOffset + Length > StreamSize)
    {
        // is increasing the stream size allowed?
        if (!(Fcb->Flags & FCB_IS_VOLUME) &&
            !(IrpFlags & IRP_PAGING_IO))
        {
            Status = NtfsGrowStream(DeviceExt,
                                    FileObject,
                                    Fcb,
                                    DataContext,
                                    AttributeOffset,
                                    FileRecord,
                                    WriteOffset + Length,
                                    CaseSensitive);
            if (!NT_SUCCESS(Status))
            {
                ReleaseAttributeContext(DataContext);
                ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, FileRecord);
                *LengthWritten = 0;
                return Status;
            }
        }
        else if (IrpFlags & IRP_PAGING_IO)
        {
            /* Mm flushes whole pages, so a file's last page routinely reaches
             * past its end. A paging write must not change the file size, so
             * clamp instead of failing - refusing it would lose the valid
             * bytes before the end too. */
            if (WriteOffset >= StreamSize)
            {
                DPRINT("Paging write entirely past the end of the stream; nothing to do\n");

                ReleaseAttributeContext(DataContext);
                ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, FileRecord);
                *LengthWritten = Length;
                return STATUS_SUCCESS;
            }

            DPRINT("Clamping paging write of %lu bytes at %I64u to stream size %I64u\n",
                   Length, WriteOffset, StreamSize);

            RequestedLength = Length;
            Length = (ULONG)(StreamSize - WriteOffset);
        }
        else
        {
            /* The volume stream is the size of the volume and cannot grow */
            ReleaseAttributeContext(DataContext);
            ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, FileRecord);
            *LengthWritten = 0;
            return STATUS_ACCESS_DENIED;
        }
    }

    DPRINT("Length: %lu\tWriteOffset: %I64u\tStreamSize: %I64u\n", Length, WriteOffset, StreamSize);

    // Write the data to the attribute
    /* Buffer is the system mapping of this IRP's MDL, so hand the IRP down too
     * and let the run layer borrow it instead of locking these pages twice,
     * which the paging path forbids */
    Status = WriteAttributeFromIrp(DeviceExt, DataContext, WriteOffset, Buffer, Length, LengthWritten, FileRecord, Irp);

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

    // A clamped paging write stored all the stream had room for, so as far as
    // Mm is concerned the whole page is done
    if (RequestedLength != 0 && NT_SUCCESS(Status))
        *LengthWritten = RequestedLength;

    ReleaseAttributeContext(DataContext);
    ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, FileRecord);

    return Status;
}

/* Grows the file for a cached write ending at NewEnd */
static
NTSTATUS
NTAPI
NtfsGrowFileForWrite(
    _In_ PFILE_OBJECT FileObject,
    _In_ PNTFS_FCB Fcb,
    _In_ ULONGLONG NewEnd,
    _In_ BOOLEAN CaseSensitive)
{
    PDEVICE_EXTENSION DeviceExt = Fcb->Vcb;
    PFILE_RECORD_HEADER FileRecord;
    PNTFS_ATTR_CONTEXT DataContext;
    ULONG AttributeOffset;
    NTSTATUS Status;

    FileRecord = ExAllocateFromNPagedLookasideList(&DeviceExt->FileRecLookasideList);
    if (FileRecord == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = ReadFileRecord(DeviceExt, Fcb->MFTIndex, FileRecord);
    if (NT_SUCCESS(Status))
    {
        Status = FindAttribute(DeviceExt, FileRecord, AttributeData, Fcb->Stream, wcslen(Fcb->Stream), &DataContext, &AttributeOffset);
        if (NT_SUCCESS(Status))
        {
            Status = NtfsGrowStream(DeviceExt, FileObject, Fcb, DataContext, AttributeOffset, FileRecord, NewEnd, CaseSensitive);
            ReleaseAttributeContext(DataContext);
        }
    }

    ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, FileRecord);

    return Status;
}

static
NTSTATUS
NTAPI
NtfsCachedWrite(
    _In_ PNTFS_IRP_CONTEXT IrpContext,
    _In_ PNTFS_FCB Fcb,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _In_ PLARGE_INTEGER WriteOffset,
    _In_ BOOLEAN CaseSensitive,
    _Out_ PULONG LengthWritten)
{
    PFILE_OBJECT FileObject = IrpContext->FileObject;
    LONGLONG WriteEnd = WriteOffset->QuadPart + Length;
    LARGE_INTEGER OldValidData;
    BOOLEAN HideNewData = FALSE;
    NTSTATUS Status = STATUS_SUCCESS;

    *LengthWritten = 0;

    if (NtfsFCBIsCompressed(Fcb))
    {
        UNIMPLEMENTED;
        return STATUS_NOT_IMPLEMENTED;
    }

    /* Let Cc throttle us before we hold anything */
    CcCanIWrite(FileObject, Length, TRUE, FALSE);

    ExAcquireResourceExclusiveLite(&Fcb->MainResource, TRUE);

    _SEH2_TRY
    {
        NtfsEnsureCacheMap(FileObject, Fcb);

        OldValidData = Fcb->RFCB.ValidDataLength;

        if (WriteEnd > Fcb->RFCB.FileSize.QuadPart)
        {
            Status = NtfsGrowFileForWrite(FileObject, Fcb, WriteEnd, CaseSensitive);
            if (!NT_SUCCESS(Status))
                _SEH2_LEAVE;
        }

        /* Don't let Cc read from disk what we are about to overwrite, and
         * make whatever lies before the write read as zeroes */
        if (WriteEnd > OldValidData.QuadPart)
        {
            CC_FILE_SIZES Sizes;

            Sizes.AllocationSize = Fcb->RFCB.AllocationSize;
            Sizes.FileSize = Fcb->RFCB.FileSize;
            Sizes.ValidDataLength = OldValidData;
            CcSetFileSizes(FileObject, &Sizes);
            HideNewData = TRUE;

            if (WriteOffset->QuadPart > OldValidData.QuadPart)
                CcZeroData(FileObject, &OldValidData, WriteOffset, TRUE);
        }

        if (!CcCopyWrite(FileObject, WriteOffset, Length, TRUE, Buffer))
        {
            Status = STATUS_UNSUCCESSFUL;
            _SEH2_LEAVE;
        }

        *LengthWritten = Length;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    /* Back to the real valid data length, or a page Mm drops would read back as zeroes */
    if (HideNewData)
        CcSetFileSizes(FileObject, (PCC_FILE_SIZES)&Fcb->RFCB.AllocationSize);

    ExReleaseResourceLite(&Fcb->MainResource);

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
* File locks, transactions, etc - not implemented.
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

    /* Offsets are 64 bits all the way down, only reject what can't be a file position */
    if (ByteOffset.QuadPart < 0 || ByteOffset.QuadPart + Length < ByteOffset.QuadPart)
    {
        DPRINT1("Invalid write offset %I64d, length %lu\n", ByteOffset.QuadPart, Length);
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

    /* A zero length write is a valid no-op */
    if (Length == 0)
    {
        // FIXME: Update last write time
        IrpContext->Irp->IoStatus.Information = 0;
        return STATUS_SUCCESS;
    }

    /* Is this an async request to a file? Serving a write means reading the
     * file record, walking its attributes and possibly growing the allocation,
     * all of it blocking metadata I/O, so post it to a worker thread which
     * runs the same path with IRPCONTEXT_CANWAIT set. Lock the user buffer
     * here, while we're still in the requesting thread's address space. */
    if (!(IrpContext->Flags & IRPCONTEXT_CANWAIT) && !(Fcb->Flags & FCB_IS_VOLUME))
    {
        Status = NtfsLockUserBuffer(Irp, Length, IoReadAccess);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Unable to lock user buffer!\n");
            return Status;
        }

        return NtfsMarkIrpContextForQueue(IrpContext);
    }

    /* Byte range locks only stop other handles, never Mm or Cc */
    if (!(Irp->Flags & IRP_PAGING_IO) && NtfsIsRegularFileObject(FileObject) &&
        !FsRtlCheckLockForWriteAccess(&Fcb->FileLock, Irp))
    {
        Irp->IoStatus.Information = 0;
        return STATUS_FILE_LOCK_CONFLICT;
    }

    /* Regular writes go through the cache, the lazy writer brings them to disk */
    if (!(Irp->Flags & (IRP_PAGING_IO | IRP_NOCACHE)) &&
        !(FileObject->Flags & FO_NO_INTERMEDIATE_BUFFERING) &&
        NtfsIsRegularFileObject(FileObject))
    {
        if (DeviceExt->Flags & VCB_VOLUME_DISMOUNTED)
        {
            Irp->IoStatus.Information = 0;
            return STATUS_VOLUME_DISMOUNTED;
        }

        Status = NtfsCachedWrite(IrpContext,
                                 Fcb,
                                 NtfsGetUserBuffer(Irp, FALSE),
                                 Length,
                                 &ByteOffset,
                                 BooleanFlagOn(IrpContext->Stack->Flags, SL_CASE_SENSITIVE),
                                 &ReturnedWriteLength);
        if (NT_SUCCESS(Status))
        {
            if (FileObject->Flags & FO_SYNCHRONOUS_IO)
                FileObject->CurrentByteOffset.QuadPart = ByteOffset.QuadPart + ReturnedWriteLength;

            IrpContext->PriorityBoost = IO_DISK_INCREMENT;
        }

        Irp->IoStatus.Information = NT_SUCCESS(Status) ? ReturnedWriteLength : 0;
        return Status;
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

    /* Paging writes never change the sizes, and Cc or Mm may already hold the
     * paging resource shared for us. Size changes take it exclusive. */
    if (Resource == &Fcb->PagingIoResource)
    {
        if (!ExAcquireResourceSharedLite(Resource, BooleanFlagOn(IrpContext->Flags, IRPCONTEXT_CANWAIT)))
            return STATUS_CANT_WAIT;
    }
    else if (!ExAcquireResourceExclusiveLite(Resource, BooleanFlagOn(IrpContext->Flags, IRPCONTEXT_CANWAIT)))
    {
        return STATUS_CANT_WAIT;
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


    /* See NtfsRead() */
    if ((DeviceExt->Flags & VCB_VOLUME_DISMOUNTED) &&
        !(Fcb->Flags & FCB_IS_VOLUME))
    {
        ExReleaseResourceLite(Resource);
        Irp->IoStatus.Information = 0;
        return STATUS_VOLUME_DISMOUNTED;
    }

    // write the file
    if (Fcb->Flags & FCB_IS_VOLUME)
    {
        /* Raw access to the partition, including after a dismount. */
        Status = NtfsReadWriteVolume(DeviceExt,
                                     IRP_MJ_WRITE,
                                     Buffer,
                                     Length,
                                     ByteOffset.QuadPart,
                                     Irp,
                                     &ReturnedWriteLength);
    }
    else if (NtfsFCBIsDirectory(Fcb))
    {
        /* A directory has no data stream to write */
        ExReleaseResourceLite(Resource);
        Irp->IoStatus.Information = 0;
        return STATUS_INVALID_DEVICE_REQUEST;
    }
    else
    {
        /* What goes straight to disk must not be overwritten by stale cached data later */
        if (!(Irp->Flags & IRP_PAGING_IO) && NtfsIsRegularFileObject(FileObject) &&
            Fcb->SectionObjectPointers.DataSectionObject != NULL)
        {
            IO_STATUS_BLOCK IoStatus;

            CcFlushCache(&Fcb->SectionObjectPointers, &ByteOffset, Length, &IoStatus);
            CcPurgeCacheSection(&Fcb->SectionObjectPointers, &ByteOffset, Length, FALSE);
        }

        Status = NtfsWriteFile(DeviceExt,
                               FileObject,
                               Buffer,
                               Length,
                               ByteOffset.QuadPart,
                               Irp->Flags,
                               BooleanFlagOn(IrpContext->Stack->Flags, SL_CASE_SENSITIVE),
                               Irp,
                               &ReturnedWriteLength);
    }

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
