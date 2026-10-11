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
 * FILE:             drivers/filesystem/ntfs/dirctl.c
 * PURPOSE:          NTFS filesystem driver
 * PROGRAMMERS:      Eric Kohl
 *                   Hervé Poussineau (hpoussin@reactos.org)
 *                   Pierre Schweitzer (pierre@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include "ntfs.h"

#define NDEBUG
#include <debug.h>

/* Longest name a $FILE_NAME can hold, in characters */
#define NTFS_MAX_NAME_LENGTH 255

/* FUNCTIONS ****************************************************************/

/*
 * FUNCTION: Retrieve the standard file information
 */
static
NTSTATUS
NtfsGetStandardInformation(PNTFS_FCB Fcb,
                           PDEVICE_OBJECT DeviceObject,
                           PFILE_STANDARD_INFORMATION StandardInfo,
                           PULONG BufferLength)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    DPRINT("NtfsGetStandardInformation(%p, %p, %p, %p)\n", Fcb, DeviceObject, StandardInfo, BufferLength);

    if (*BufferLength < sizeof(FILE_STANDARD_INFORMATION))
        return STATUS_BUFFER_TOO_SMALL;

    /* PRECONDITION */
    ASSERT(StandardInfo != NULL);
    ASSERT(Fcb != NULL);

    RtlZeroMemory(StandardInfo,
                  sizeof(FILE_STANDARD_INFORMATION));

    StandardInfo->AllocationSize = Fcb->RFCB.AllocationSize;
    StandardInfo->EndOfFile = Fcb->RFCB.FileSize;
    StandardInfo->NumberOfLinks = Fcb->LinkCount;
    StandardInfo->DeletePending = BooleanFlagOn(Fcb->Flags, FCB_DELETE_PENDING);
    StandardInfo->Directory = NtfsFCBIsDirectory(Fcb);

    *BufferLength -= sizeof(FILE_STANDARD_INFORMATION);

    return STATUS_SUCCESS;
}


static
NTSTATUS
NtfsGetPositionInformation(PFILE_OBJECT FileObject,
                           PFILE_POSITION_INFORMATION PositionInfo,
                           PULONG BufferLength)
{
    DPRINT1("NtfsGetPositionInformation(%p, %p, %p)\n", FileObject, PositionInfo, BufferLength);

    if (*BufferLength < sizeof(FILE_POSITION_INFORMATION))
        return STATUS_BUFFER_TOO_SMALL;

    PositionInfo->CurrentByteOffset.QuadPart = FileObject->CurrentByteOffset.QuadPart;

    DPRINT("Getting position %I64x\n",
           PositionInfo->CurrentByteOffset.QuadPart);

    *BufferLength -= sizeof(FILE_POSITION_INFORMATION);

    return STATUS_SUCCESS;
}


static
NTSTATUS
NtfsGetBasicInformation(PFILE_OBJECT FileObject,
                        PNTFS_FCB Fcb,
                        PDEVICE_OBJECT DeviceObject,
                        PFILE_BASIC_INFORMATION BasicInfo,
                        PULONG BufferLength)
{
    PFILENAME_ATTRIBUTE FileName = &Fcb->Entry;

    DPRINT("NtfsGetBasicInformation(%p, %p, %p, %p, %p)\n", FileObject, Fcb, DeviceObject, BasicInfo, BufferLength);

    if (*BufferLength < sizeof(FILE_BASIC_INFORMATION))
        return STATUS_BUFFER_TOO_SMALL;

    RtlZeroMemory(BasicInfo, sizeof(FILE_BASIC_INFORMATION));

    BasicInfo->CreationTime.QuadPart = FileName->CreationTime;
    BasicInfo->LastAccessTime.QuadPart = FileName->LastAccessTime;
    BasicInfo->LastWriteTime.QuadPart = FileName->LastWriteTime;
    BasicInfo->ChangeTime.QuadPart = FileName->ChangeTime;

    NtfsFileFlagsToAttributes(FileName->FileAttributes, &BasicInfo->FileAttributes);

    *BufferLength -= sizeof(FILE_BASIC_INFORMATION);

    return STATUS_SUCCESS;
}

static
NTSTATUS
NtfsGetAttributeTagInformation(PNTFS_FCB Fcb,
                               PFILE_ATTRIBUTE_TAG_INFORMATION AttributeTagInfo,
                               PULONG BufferLength)
{
    PFILENAME_ATTRIBUTE FileName = &Fcb->Entry;

    DPRINT("NtfsGetAttributeTagInformation(%p, %p, %p)\n", Fcb, AttributeTagInfo, BufferLength);

    if (*BufferLength < sizeof(FILE_ATTRIBUTE_TAG_INFORMATION))
        return STATUS_BUFFER_TOO_SMALL;

    RtlZeroMemory(AttributeTagInfo, sizeof(FILE_ATTRIBUTE_TAG_INFORMATION));

    NtfsFileFlagsToAttributes(FileName->FileAttributes, &AttributeTagInfo->FileAttributes);

    /* FIXME: Read the tag from $REPARSE_POINT for a reparse point */
    AttributeTagInfo->ReparseTag = 0;

    *BufferLength -= sizeof(FILE_ATTRIBUTE_TAG_INFORMATION);

    return STATUS_SUCCESS;
}


static
NTSTATUS
NtfsGetEaInformation(PNTFS_FCB Fcb,
                     PFILE_EA_INFORMATION EaInfo,
                     PULONG BufferLength)
{
    UNREFERENCED_PARAMETER(Fcb);

    DPRINT("NtfsGetEaInformation(%p, %p, %p)\n", Fcb, EaInfo, BufferLength);

    if (*BufferLength < sizeof(FILE_EA_INFORMATION))
        return STATUS_BUFFER_TOO_SMALL;

    /* We do not support extended attributes yet, so there are none */
    EaInfo->EaSize = 0;

    *BufferLength -= sizeof(FILE_EA_INFORMATION);

    return STATUS_SUCCESS;
}


/*
 * FUNCTION: Retrieve the file name information
 */
static
NTSTATUS
NtfsGetNameInformation(PFILE_OBJECT FileObject,
                       PNTFS_FCB Fcb,
                       PDEVICE_OBJECT DeviceObject,
                       PFILE_NAME_INFORMATION NameInfo,
                       PULONG BufferLength)
{
    ULONG BytesToCopy;

    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(DeviceObject);

    DPRINT("NtfsGetNameInformation(%p, %p, %p, %p, %p)\n", FileObject, Fcb, DeviceObject, NameInfo, BufferLength);

    ASSERT(NameInfo != NULL);
    ASSERT(Fcb != NULL);

    /* If buffer can't hold at least the file name length, bail out */
    if (*BufferLength < (ULONG)FIELD_OFFSET(FILE_NAME_INFORMATION, FileName[0]))
        return STATUS_BUFFER_TOO_SMALL;

    /* Save file name length, and as much file len, as buffer length allows */
    NameInfo->FileNameLength = wcslen(Fcb->PathName) * sizeof(WCHAR);

    /* Calculate amount of bytes to copy not to overflow the buffer */
    BytesToCopy = min(NameInfo->FileNameLength,
                      *BufferLength - FIELD_OFFSET(FILE_NAME_INFORMATION, FileName[0]));

    /* Fill in the bytes */
    RtlCopyMemory(NameInfo->FileName, Fcb->PathName, BytesToCopy);

    /* Check if we could write more but are not able to */
    if (*BufferLength < NameInfo->FileNameLength + (ULONG)FIELD_OFFSET(FILE_NAME_INFORMATION, FileName[0]))
    {
        /* Return number of bytes written */
        *BufferLength -= FIELD_OFFSET(FILE_NAME_INFORMATION, FileName[0]) + BytesToCopy;
        return STATUS_BUFFER_OVERFLOW;
    }

    /* We filled up as many bytes, as needed */
    *BufferLength -= (FIELD_OFFSET(FILE_NAME_INFORMATION, FileName[0]) + NameInfo->FileNameLength);

    return STATUS_SUCCESS;
}


static
NTSTATUS
NtfsGetInternalInformation(PNTFS_FCB Fcb,
                           PFILE_INTERNAL_INFORMATION InternalInfo,
                           PULONG BufferLength)
{
    DPRINT1("NtfsGetInternalInformation(%p, %p, %p)\n", Fcb, InternalInfo, BufferLength);

    ASSERT(InternalInfo);
    ASSERT(Fcb);

    if (*BufferLength < sizeof(FILE_INTERNAL_INFORMATION))
        return STATUS_BUFFER_TOO_SMALL;

    InternalInfo->IndexNumber.QuadPart = Fcb->MFTIndex;

    *BufferLength -= sizeof(FILE_INTERNAL_INFORMATION);

    return STATUS_SUCCESS;
}

static
NTSTATUS
NtfsGetNetworkOpenInformation(PNTFS_FCB Fcb,
                              PDEVICE_EXTENSION DeviceExt,
                              PFILE_NETWORK_OPEN_INFORMATION NetworkInfo,
                              PULONG BufferLength)
{
    PFILENAME_ATTRIBUTE FileName = &Fcb->Entry;

    DPRINT("NtfsGetNetworkOpenInformation(%p, %p, %p, %p)\n", Fcb, DeviceExt, NetworkInfo, BufferLength);

    if (*BufferLength < sizeof(FILE_NETWORK_OPEN_INFORMATION))
        return STATUS_BUFFER_TOO_SMALL;

    NetworkInfo->CreationTime.QuadPart = FileName->CreationTime;
    NetworkInfo->LastAccessTime.QuadPart = FileName->LastAccessTime;
    NetworkInfo->LastWriteTime.QuadPart = FileName->LastWriteTime;
    NetworkInfo->ChangeTime.QuadPart = FileName->ChangeTime;

    NetworkInfo->EndOfFile = Fcb->RFCB.FileSize;
    NetworkInfo->AllocationSize = Fcb->RFCB.AllocationSize;

    NtfsFileFlagsToAttributes(FileName->FileAttributes, &NetworkInfo->FileAttributes);

    *BufferLength -= sizeof(FILE_NETWORK_OPEN_INFORMATION);
    return STATUS_SUCCESS;
}

static
NTSTATUS
NtfsGetStreamInformation(PNTFS_FCB Fcb,
                         PDEVICE_EXTENSION DeviceExt,
                         PFILE_STREAM_INFORMATION StreamInfo,
                         PULONG BufferLength)
{
    ULONG CurrentSize;
    FIND_ATTR_CONTXT Context;
    PNTFS_ATTR_RECORD Attribute;
    NTSTATUS Status, BrowseStatus;
    PFILE_RECORD_HEADER FileRecord;
    PFILE_STREAM_INFORMATION CurrentInfo = StreamInfo, Previous = NULL;

    if (*BufferLength < sizeof(FILE_STREAM_INFORMATION))
        return STATUS_BUFFER_TOO_SMALL;

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

    BrowseStatus = FindFirstAttribute(&Context, DeviceExt, FileRecord, FALSE, &Attribute);
    while (NT_SUCCESS(BrowseStatus))
    {
        if (Attribute->Type == AttributeData)
        {
            CurrentSize = FIELD_OFFSET(FILE_STREAM_INFORMATION, StreamName) + Attribute->NameLength * sizeof(WCHAR) + wcslen(L"::$DATA") * sizeof(WCHAR);

            if (CurrentSize > *BufferLength)
            {
                Status = STATUS_BUFFER_OVERFLOW;
                break;
            }

            CurrentInfo->NextEntryOffset = 0;
            CurrentInfo->StreamNameLength = (Attribute->NameLength + wcslen(L"::$DATA")) * sizeof(WCHAR);
            CurrentInfo->StreamSize.QuadPart = AttributeDataLength(Attribute);
            CurrentInfo->StreamAllocationSize.QuadPart = AttributeAllocatedLength(Attribute);
            CurrentInfo->StreamName[0] = L':';
            RtlMoveMemory(&CurrentInfo->StreamName[1], (PWCHAR)((ULONG_PTR)Attribute + Attribute->NameOffset), CurrentInfo->StreamNameLength);
            RtlMoveMemory(&CurrentInfo->StreamName[Attribute->NameLength + 1], L":$DATA", sizeof(L":$DATA") - sizeof(UNICODE_NULL));

            if (Previous != NULL)
            {
                Previous->NextEntryOffset = (ULONG_PTR)CurrentInfo - (ULONG_PTR)Previous;
            }
            Previous = CurrentInfo;
            CurrentInfo = (PFILE_STREAM_INFORMATION)((ULONG_PTR)CurrentInfo + CurrentSize);
            *BufferLength -= CurrentSize;
        }

        BrowseStatus = FindNextAttribute(&Context, &Attribute);
    }

    FindCloseAttribute(&Context);
    ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, FileRecord);
    return Status;
}

// Convert enum value to friendly name
const PCSTR
GetInfoClassName(FILE_INFORMATION_CLASS infoClass)
{
    const PCSTR fileInfoClassNames[] = { "???????",
        "FileDirectoryInformation",
        "FileFullDirectoryInformation",
        "FileBothDirectoryInformation",
        "FileBasicInformation",
        "FileStandardInformation",
        "FileInternalInformation",
        "FileEaInformation",
        "FileAccessInformation",
        "FileNameInformation",
        "FileRenameInformation",
        "FileLinkInformation",
        "FileNamesInformation",
        "FileDispositionInformation",
        "FilePositionInformation",
        "FileFullEaInformation",
        "FileModeInformation",
        "FileAlignmentInformation",
        "FileAllInformation",
        "FileAllocationInformation",
        "FileEndOfFileInformation",
        "FileAlternateNameInformation",
        "FileStreamInformation",
        "FilePipeInformation",
        "FilePipeLocalInformation",
        "FilePipeRemoteInformation",
        "FileMailslotQueryInformation",
        "FileMailslotSetInformation",
        "FileCompressionInformation",
        "FileObjectIdInformation",
        "FileCompletionInformation",
        "FileMoveClusterInformation",
        "FileQuotaInformation",
        "FileReparsePointInformation",
        "FileNetworkOpenInformation",
        "FileAttributeTagInformation",
        "FileTrackingInformation",
        "FileIdBothDirectoryInformation",
        "FileIdFullDirectoryInformation",
        "FileValidDataLengthInformation",
        "FileShortNameInformation",
        "FileIoCompletionNotificationInformation",
        "FileIoStatusBlockRangeInformation",
        "FileIoPriorityHintInformation",
        "FileSfioReserveInformation",
        "FileSfioVolumeInformation",
        "FileHardLinkInformation",
        "FileProcessIdsUsingFileInformation",
        "FileNormalizedNameInformation",
        "FileNetworkPhysicalNameInformation",
        "FileIdGlobalTxDirectoryInformation",
        "FileIsRemoteDeviceInformation",
        "FileAttributeCacheInformation",
        "FileNumaNodeInformation",
        "FileStandardLinkInformation",
        "FileRemoteProtocolInformation",
        "FileReplaceCompletionInformation",
        "FileMaximumInformation",
        "FileDirectoryInformation",
        "FileFullDirectoryInformation",
        "FileBothDirectoryInformation",
        "FileBasicInformation",
        "FileStandardInformation",
        "FileInternalInformation",
        "FileEaInformation",
        "FileAccessInformation",
        "FileNameInformation",
        "FileRenameInformation",
        "FileLinkInformation",
        "FileNamesInformation",
        "FileDispositionInformation",
        "FilePositionInformation",
        "FileFullEaInformation",
        "FileModeInformation",
        "FileAlignmentInformation",
        "FileAllInformation",
        "FileAllocationInformation",
        "FileEndOfFileInformation",
        "FileAlternateNameInformation",
        "FileStreamInformation",
        "FilePipeInformation",
        "FilePipeLocalInformation",
        "FilePipeRemoteInformation",
        "FileMailslotQueryInformation",
        "FileMailslotSetInformation",
        "FileCompressionInformation",
        "FileObjectIdInformation",
        "FileCompletionInformation",
        "FileMoveClusterInformation",
        "FileQuotaInformation",
        "FileReparsePointInformation",
        "FileNetworkOpenInformation",
        "FileAttributeTagInformation",
        "FileTrackingInformation",
        "FileIdBothDirectoryInformation",
        "FileIdFullDirectoryInformation",
        "FileValidDataLengthInformation",
        "FileShortNameInformation",
        "FileIoCompletionNotificationInformation",
        "FileIoStatusBlockRangeInformation",
        "FileIoPriorityHintInformation",
        "FileSfioReserveInformation",
        "FileSfioVolumeInformation",
        "FileHardLinkInformation",
        "FileProcessIdsUsingFileInformation",
        "FileNormalizedNameInformation",
        "FileNetworkPhysicalNameInformation",
        "FileIdGlobalTxDirectoryInformation",
        "FileIsRemoteDeviceInformation",
        "FileAttributeCacheInformation",
        "FileNumaNodeInformation",
        "FileStandardLinkInformation",
        "FileRemoteProtocolInformation",
        "FileReplaceCompletionInformation",
        "FileMaximumInformation" };
    return fileInfoClassNames[infoClass];
}

/*
 * FUNCTION: Retrieve the specified file information
 */
NTSTATUS
NtfsQueryInformation(PNTFS_IRP_CONTEXT IrpContext)
{
    FILE_INFORMATION_CLASS FileInformationClass;
    PIO_STACK_LOCATION Stack;
    PFILE_OBJECT FileObject;
    PNTFS_FCB Fcb;
    PVOID SystemBuffer;
    ULONG BufferLength;
    PIRP Irp;
    PDEVICE_OBJECT DeviceObject;
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("NtfsQueryInformation(%p)\n", IrpContext);

    Irp = IrpContext->Irp;
    Stack = IrpContext->Stack;
    DeviceObject = IrpContext->DeviceObject;
    FileInformationClass = Stack->Parameters.QueryFile.FileInformationClass;
    FileObject = IrpContext->FileObject;
    Fcb = FileObject->FsContext;

    SystemBuffer = Irp->AssociatedIrp.SystemBuffer;
    BufferLength = Stack->Parameters.QueryFile.Length;

    if (!ExAcquireResourceSharedLite(&Fcb->MainResource,
                                     BooleanFlagOn(IrpContext->Flags, IRPCONTEXT_CANWAIT)))
    {
        return NtfsMarkIrpContextForQueue(IrpContext);
    }

    switch (FileInformationClass)
    {
        case FileStandardInformation:
            Status = NtfsGetStandardInformation(Fcb,
                                                DeviceObject,
                                                SystemBuffer,
                                                &BufferLength);
            break;

        case FileEaInformation:
            Status = NtfsGetEaInformation(Fcb,
                                          SystemBuffer,
                                          &BufferLength);
            break;

        case FilePositionInformation:
            Status = NtfsGetPositionInformation(FileObject,
                                                SystemBuffer,
                                                &BufferLength);
            break;

        case FileBasicInformation:
            Status = NtfsGetBasicInformation(FileObject,
                                             Fcb,
                                             DeviceObject,
                                             SystemBuffer,
                                             &BufferLength);
            break;

        case FileNameInformation:
            Status = NtfsGetNameInformation(FileObject,
                                            Fcb,
                                            DeviceObject,
                                            SystemBuffer,
                                            &BufferLength);
            break;

        case FileInternalInformation:
            Status = NtfsGetInternalInformation(Fcb,
                                                SystemBuffer,
                                                &BufferLength);
            break;

        case FileNetworkOpenInformation:
            Status = NtfsGetNetworkOpenInformation(Fcb,
                                                   DeviceObject->DeviceExtension,
                                                   SystemBuffer,
                                                   &BufferLength);
            break;

        case FileStreamInformation:
            Status = NtfsGetStreamInformation(Fcb,
                                              DeviceObject->DeviceExtension,
                                              SystemBuffer,
                                              &BufferLength);
            break;

        case FileAttributeTagInformation:
            Status = NtfsGetAttributeTagInformation(Fcb,
                                                    SystemBuffer,
                                                    &BufferLength);
            break;

        case FileAlternateNameInformation:
        case FileAllInformation:
            DPRINT1("Unimplemented information class: %s\n", GetInfoClassName(FileInformationClass));
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        default:
            DPRINT1("Unimplemented information class: %s\n", GetInfoClassName(FileInformationClass));
            Status = STATUS_INVALID_PARAMETER;
    }

    ExReleaseResourceLite(&Fcb->MainResource);

    if (NT_SUCCESS(Status))
        Irp->IoStatus.Information =
            Stack->Parameters.QueryFile.Length - BufferLength;
    else
        Irp->IoStatus.Information = 0;

    return Status;
}

/**
* @name NtfsSetEndOfFile
* @implemented
*
* Sets the end of file (file size) for a given file.
*
* @param Fcb
* Pointer to an NTFS_FCB which describes the target file. Fcb->MainResource should have been
* acquired with ExAcquireResourceSharedLite().
*
* @param FileObject
* Pointer to a FILE_OBJECT describing the target file.
*
* @param DeviceExt
* Points to the target disk's DEVICE_EXTENSION
*
* @param IrpFlags
* ULONG describing the flags of the original IRP request (Irp->Flags).
*
* @param CaseSensitive
* Boolean indicating if the function should operate in case-sensitive mode. This will be TRUE
* if an application opened the file with the FILE_FLAG_POSIX_SEMANTICS flag.
*
* @param NewFileSize
* Pointer to a LARGE_INTEGER which indicates the new end of file (file size).
*
* @return
* STATUS_SUCCESS if successful,
* STATUS_USER_MAPPED_FILE if trying to truncate a file but MmCanFileBeTruncated() returned false,
* STATUS_OBJECT_NAME_NOT_FOUND if there was no $DATA attribute associated with the target file,
* STATUS_INVALID_PARAMETER if there was no $FILENAME attribute associated with the target file,
* STATUS_INSUFFICIENT_RESOURCES if an allocation failed,
* STATUS_ACCESS_DENIED if target file is a volume or if paging is involved.
*
* @remarks As this function sets the size of a file at the file-level
* (and not at the attribute level) it's not recommended to use this
* function alongside functions that operate on the data attribute directly.
*
*/
NTSTATUS
NtfsSetEndOfFile(PNTFS_FCB Fcb,
                 PFILE_OBJECT FileObject,
                 PDEVICE_EXTENSION DeviceExt,
                 ULONG IrpFlags,
                 BOOLEAN CaseSensitive,
                 PLARGE_INTEGER NewFileSize)
{
    LARGE_INTEGER CurrentFileSize;
    PFILE_RECORD_HEADER FileRecord;
    PNTFS_ATTR_CONTEXT DataContext;
    ULONG AttributeOffset;
    NTSTATUS Status = STATUS_SUCCESS;
    PFILENAME_ATTRIBUTE FileNameAttribute;


    // Allocate non-paged memory for the file record
    FileRecord = ExAllocateFromNPagedLookasideList(&DeviceExt->FileRecLookasideList);
    if (FileRecord == NULL)
    {
        DPRINT1("Couldn't allocate memory for file record!");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // read the file record
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

    CurrentFileSize.QuadPart = NtfsGetFileSize(DeviceExt, FileRecord, L"", 0, NULL);

    // Are we trying to decrease the file size?
    if (NewFileSize->QuadPart < CurrentFileSize.QuadPart)
    {
        // Is the file mapped?
        if (!MmCanFileBeTruncated(FileObject->SectionObjectPointer,
                                  NewFileSize))
        {
            DPRINT1("Couldn't decrease file size!\n");
            ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, FileRecord);
            return STATUS_USER_MAPPED_FILE;
        }
    }

    // Find the attribute with the data stream for our file
    DPRINT("Finding Data Attribute...\n");
    Status = FindAttribute(DeviceExt,
                           FileRecord,
                           AttributeData,
                           Fcb->Stream,
                           wcslen(Fcb->Stream),
                           &DataContext,
                           &AttributeOffset);

    // Did we fail to find the attribute?
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("No '%S' data stream associated with file!\n", Fcb->Stream);
        ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, FileRecord);
        return Status;
    }

    // Get the size of the data attribute
    CurrentFileSize.QuadPart = AttributeDataLength(DataContext->pRecord);

    // Are we enlarging the attribute?
    if (NewFileSize->QuadPart > CurrentFileSize.QuadPart)
    {
        // is increasing the stream size not allowed?
        if ((Fcb->Flags & FCB_IS_VOLUME) ||
            (IrpFlags & IRP_PAGING_IO))
        {
            // TODO - just fail for now
            ReleaseAttributeContext(DataContext);
            ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, FileRecord);
            return STATUS_ACCESS_DENIED;
        }
    }

    // set the attribute data length
    Status = SetAttributeDataLength(FileObject, Fcb, DataContext, AttributeOffset, FileRecord, NewFileSize);
    if (!NT_SUCCESS(Status))
    {
        ReleaseAttributeContext(DataContext);
        ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, FileRecord);
        return Status;
    }

    // now we need to update this file's size in every directory index entry that references it
    // TODO: expand to work with every filename / hardlink stored in the file record.
    FileNameAttribute = GetBestFileNameFromRecord(Fcb->Vcb, FileRecord);
    if (FileNameAttribute == NULL)
    {
        DPRINT1("Unable to find FileName attribute associated with file!\n");
        ReleaseAttributeContext(DataContext);
        ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, FileRecord);
        return STATUS_INVALID_PARAMETER;
    }

    Status = NtfsUpdateDuplicatedInformation(Fcb->Vcb,
                                             FileRecord,
                                             Fcb->MFTIndex,
                                             NTFS_FILENAME_UPDATE_SIZES,
                                             CaseSensitive);

    ReleaseAttributeContext(DataContext);
    ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, FileRecord);

    return Status;
}

/**
* @name NtfsSetInformation
* @implemented
*
* Sets the specified file information.
*
* @param IrpContext
* Points to an NTFS_IRP_CONTEXT which describes the set operation
*
* @return
* STATUS_SUCCESS if successful,
* STATUS_NOT_IMPLEMENTED if trying to set an unimplemented information class,
* STATUS_USER_MAPPED_FILE if trying to truncate a file but MmCanFileBeTruncated() returned false,
* STATUS_OBJECT_NAME_NOT_FOUND if there was no $DATA attribute associated with the target file,
* STATUS_INVALID_PARAMETER if there was no $FILENAME attribute associated with the target file,
* STATUS_INSUFFICIENT_RESOURCES if an allocation failed,
* STATUS_ACCESS_DENIED if target file is a volume or if paging is involved.
*
* @remarks Called by NtfsDispatch() in response to an IRP_MJ_SET_INFORMATION request.
* Only the FileEndOfFileInformation InformationClass is fully implemented. FileAllocationInformation
* is a hack and not a true implementation, but it's enough to make SetEndOfFile() work.
* All other information classes are TODO.
*
*/
/**
* @name NtfsSetBasicInformation
* @implemented
*
* Applies a FILE_BASIC_INFORMATION to a file: its four timestamps and its
* attribute flags.
*
* @param DeviceExt
* Points to the target disk's DEVICE_EXTENSION
*
* @param Fcb
* File control block of the file being changed
*
* @param CaseSensitive
* Whether the parent directory's index should be searched case-sensitively
*
* @param BasicInfo
* The timestamps and attributes to apply
*
* @return
* STATUS_SUCCESS on success, STATUS_INSUFFICIENT_RESOURCES if an allocation
* failed, STATUS_INVALID_PARAMETER if the file record has no
* $STANDARD_INFORMATION, or whatever status reading or writing the file record
* returned.
*
* @remarks A timestamp of zero means "leave as is" and -1 means "stop
* maintaining automatically"; both are treated as no change, as the driver
* doesn't yet update timestamps on I/O. FileAttributes of zero means no change.
*
* NTFS keeps a second copy of these in $FILE_NAME and a third in the parent
* directory's index entry; all three are updated here.
*
*/
static
NTSTATUS
NtfsSetBasicInformation(PDEVICE_EXTENSION DeviceExt,
                        PNTFS_FCB Fcb,
                        BOOLEAN CaseSensitive,
                        PFILE_BASIC_INFORMATION BasicInfo)
{
    PFILE_RECORD_HEADER FileRecord;
    PSTANDARD_INFORMATION StdInfo;
    NTSTATUS Status;

    DPRINT("NtfsSetBasicInformation(%p, %p, %p)\n", DeviceExt, Fcb, BasicInfo);

    FileRecord = ExAllocateFromNPagedLookasideList(&DeviceExt->FileRecLookasideList);
    if (FileRecord == NULL)
    {
        DPRINT1("Not enough memory!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = ReadFileRecord(DeviceExt, Fcb->MFTIndex, FileRecord);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Can't find record for %wS!\n", Fcb->ObjectName);
        ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, FileRecord);
        return Status;
    }

    StdInfo = GetStandardInformationFromRecord(DeviceExt, FileRecord);
    if (StdInfo == NULL)
    {
        DPRINT1("No $STANDARD_INFORMATION for %wS!\n", Fcb->ObjectName);
        ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, FileRecord);
        return STATUS_INVALID_PARAMETER;
    }

    if (BasicInfo->CreationTime.QuadPart > 0)
        StdInfo->CreationTime = BasicInfo->CreationTime.QuadPart;

    if (BasicInfo->LastAccessTime.QuadPart > 0)
        StdInfo->LastAccessTime = BasicInfo->LastAccessTime.QuadPart;

    if (BasicInfo->LastWriteTime.QuadPart > 0)
        StdInfo->LastWriteTime = BasicInfo->LastWriteTime.QuadPart;

    if (BasicInfo->ChangeTime.QuadPart > 0)
        StdInfo->ChangeTime = BasicInfo->ChangeTime.QuadPart;

    if (BasicInfo->FileAttributes != 0)
    {
        ULONG Attributes = BasicInfo->FileAttributes;

        /* Whether this is a directory isn't the caller's to say */
        Attributes &= ~(FILE_ATTRIBUTE_DIRECTORY | NTFS_FILE_TYPE_DIRECTORY);

        /* NORMAL only means anything on its own */
        if ((Attributes & FILE_ATTRIBUTE_NORMAL) && Attributes != FILE_ATTRIBUTE_NORMAL)
            Attributes &= ~FILE_ATTRIBUTE_NORMAL;

        StdInfo->FileAttribute = Attributes;
    }

    /* Everything else NTFS keeps a second and third copy of goes out through the one funnel,
     * which also writes the file record back. */
    Status = NtfsUpdateDuplicatedInformation(DeviceExt,
                                             FileRecord,
                                             Fcb->MFTIndex,
                                             NTFS_FILENAME_UPDATE_TIMES |
                                             NTFS_FILENAME_UPDATE_ATTRS,
                                             CaseSensitive);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to refresh the duplicated information for %wS (Status %lx)\n",
                Fcb->ObjectName, Status);
    }

    ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, FileRecord);

    return Status;
}

/**
* @name NtfsSetDispositionInformation
* @implemented
*
* Marks a file for deletion, or clears an existing mark.
*
* @param DeviceExt
* Points to the target disk's DEVICE_EXTENSION.
*
* @param Fcb
* Pointer to the NTFS_FCB of the file. Fcb->MainResource should have been acquired.
*
* @param FileObject
* Pointer to the FILE_OBJECT the request arrived on.
*
* @param CaseSensitive
* Boolean indicating if the function should operate in case-sensitive mode. This will be TRUE
* if an application created the file with the FILE_FLAG_POSIX_SEMANTICS flag.
*
* @param DispositionInfo
* Pointer to the FILE_DISPOSITION_INFORMATION supplied by the caller.
*
* @return
* STATUS_SUCCESS on success.
* STATUS_CANNOT_DELETE if the file is read-only or is the volume root.
* STATUS_DIRECTORY_NOT_EMPTY if the file is a directory that still holds entries.
*
* @remarks
* Only records the intent; the file is deleted at cleanup, once its last handle is closed.
*/
static
NTSTATUS
NtfsSetDispositionInformation(PDEVICE_EXTENSION DeviceExt,
                              PNTFS_FCB Fcb,
                              PFILE_OBJECT FileObject,
                              BOOLEAN CaseSensitive,
                              PFILE_DISPOSITION_INFORMATION DispositionInfo)
{
    DPRINT("NtfsSetDispositionInformation(%p, %p, %p, %s, %p)\n",
           DeviceExt, Fcb, FileObject, CaseSensitive ? "TRUE" : "FALSE", DispositionInfo);

    if (!DispositionInfo->DeleteFile)
    {
        Fcb->Flags &= ~FCB_DELETE_PENDING;
        FileObject->DeletePending = FALSE;
        return STATUS_SUCCESS;
    }

    if (!NtfsGlobalData->EnableWriteSupport)
    {
        DPRINT1("NTFS write-support is EXPERIMENTAL and is disabled by default!\n");
        return STATUS_ACCESS_DENIED;
    }

    /* The volume root has no parent index to be removed from */
    if (NtfsFCBIsRoot(Fcb))
        return STATUS_CANNOT_DELETE;

    if (Fcb->Entry.FileAttributes & NTFS_FILE_TYPE_READ_ONLY)
        return STATUS_CANNOT_DELETE;

    if (NtfsFCBIsDirectory(Fcb))
    {
        BOOLEAN Empty;
        NTSTATUS Status;

        /* A lookup treats an unreadable sub-node as a miss, which here would
         * delete a directory that still has children. This check fails instead. */
        KeEnterCriticalRegion();
        ExAcquireResourceSharedLite(&DeviceExt->IndexResource, TRUE);
        Status = NtfsIsDirectoryEmpty(DeviceExt, Fcb->MFTIndex, &Empty);
        ExReleaseResourceLite(&DeviceExt->IndexResource);
        KeLeaveCriticalRegion();

        if (!NT_SUCCESS(Status))
            return Status;

        if (!Empty)
            return STATUS_DIRECTORY_NOT_EMPTY;
    }

    /* Deleting a mapped file would free its record out from under the section */
    if (!MmFlushImageSection(&Fcb->SectionObjectPointers, MmFlushForDelete))
        return STATUS_CANNOT_DELETE;

    Fcb->Flags |= FCB_DELETE_PENDING;
    FileObject->DeletePending = TRUE;

    return STATUS_SUCCESS;
}

/**
* @name NtfsIsValidFileName
*
* Checks a single path component against what NTFS stores and Win32 can open.
*/
static
BOOLEAN
NtfsIsValidFileName(
    _In_ PCUNICODE_STRING Name)
{
    USHORT i;

    if (Name->Length == 0 ||
        Name->Length > NTFS_MAX_NAME_LENGTH * sizeof(WCHAR) ||
        (Name->Length % sizeof(WCHAR)) != 0)
    {
        return FALSE;
    }

    if ((Name->Length == sizeof(WCHAR) && Name->Buffer[0] == L'.') ||
        (Name->Length == 2 * sizeof(WCHAR) && Name->Buffer[0] == L'.' && Name->Buffer[1] == L'.'))
    {
        return FALSE;
    }

    for (i = 0; i < Name->Length / sizeof(WCHAR); i++)
    {
        WCHAR Char = Name->Buffer[i];

        if (Char < 0x20 || wcschr(L"\\/:*?\"<>|", Char) != NULL)
            return FALSE;
    }

    return TRUE;
}

/**
* @name NtfsSetFileNameAttribute
*
* Rebuilds FileRecord so its only $FILE_NAME is one holding Value. Attributes
* stay sorted by type, which NTFS requires.
*
* @return
* STATUS_SUCCESS, or STATUS_INSUFFICIENT_RESOURCES if the new name doesn't fit
* in the record.
*/
static
NTSTATUS
NtfsSetFileNameAttribute(
    _In_ PDEVICE_EXTENSION DeviceExt,
    _Inout_ PFILE_RECORD_HEADER FileRecord,
    _In_ PFILENAME_ATTRIBUTE Value,
    _In_ ULONG ValueLength)
{
    ULONG RecordSize = DeviceExt->NtfsInfo.BytesPerFileRecord;
    ULONG HeaderLength = FIELD_OFFSET(NTFS_ATTR_RECORD, Resident.Reserved) + sizeof(UCHAR);
    ULONG NewLength = ALIGN_UP_BY(HeaderLength + ValueLength, ATTR_RECORD_ALIGNMENT);
    PFILE_RECORD_HEADER NewRecord;
    PNTFS_ATTR_RECORD Source;
    PNTFS_ATTR_RECORD Dest;
    ULONG SourceOffset = FileRecord->AttributeOffset;
    ULONG DestOffset = FileRecord->AttributeOffset;
    BOOLEAN Inserted = FALSE;
    NTSTATUS Status = STATUS_SUCCESS;

    NewRecord = ExAllocateFromNPagedLookasideList(&DeviceExt->FileRecLookasideList);
    if (NewRecord == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(NewRecord, RecordSize);
    RtlCopyMemory(NewRecord, FileRecord, FileRecord->AttributeOffset);

    for (;;)
    {
        BOOLEAN AtEnd;

        Source = (PNTFS_ATTR_RECORD)((ULONG_PTR)FileRecord + SourceOffset);
        AtEnd = (SourceOffset + sizeof(ULONG) > RecordSize || Source->Type == AttributeEnd);

        /* The new name goes ahead of the first attribute that sorts after it */
        if (!Inserted && (AtEnd || Source->Type > AttributeFileName))
        {
            if (DestOffset + NewLength + 2 * sizeof(ULONG) > RecordSize)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            Dest = (PNTFS_ATTR_RECORD)((ULONG_PTR)NewRecord + DestOffset);
            Dest->Type = AttributeFileName;
            Dest->Length = NewLength;
            Dest->Instance = NewRecord->NextAttributeNumber++;
            Dest->Resident.ValueLength = ValueLength;
            Dest->Resident.ValueOffset = (USHORT)HeaderLength;
            Dest->Resident.Flags = RA_INDEXED;
            RtlCopyMemory((PUCHAR)Dest + HeaderLength, Value, ValueLength);

            DestOffset += NewLength;
            Inserted = TRUE;
        }

        if (AtEnd)
            break;

        if (Source->Length == 0 || SourceOffset + Source->Length > RecordSize)
        {
            Status = STATUS_FILE_CORRUPT_ERROR;
            break;
        }

        if (Source->Type != AttributeFileName)
        {
            if (DestOffset + Source->Length + 2 * sizeof(ULONG) > RecordSize)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            RtlCopyMemory((PUCHAR)NewRecord + DestOffset, Source, Source->Length);
            DestOffset += Source->Length;
        }

        SourceOffset += Source->Length;
    }

    if (NT_SUCCESS(Status))
    {
        SetFileRecordEnd(NewRecord, (PNTFS_ATTR_RECORD)((ULONG_PTR)NewRecord + DestOffset), FILE_RECORD_END);
        RtlCopyMemory(FileRecord, NewRecord, RecordSize);
    }

    ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, NewRecord);
    return Status;
}

/**
* @name NtfsRenameFcbs
*
* Moves the FCB table over to NewPath: the renamed FCB, its other streams, and
* for a directory every FCB cached beneath it.
*/
static
VOID
NtfsRenameFcbs(
    _In_ PDEVICE_EXTENSION DeviceExt,
    _In_ PNTFS_FCB Fcb,
    _In_ PCWSTR NewPath)
{
    WCHAR OldPath[MAX_PATH];
    WCHAR Rebuilt[MAX_PATH];
    SIZE_T OldLength;
    PLIST_ENTRY Entry;
    KIRQL OldIrql;

    wcscpy(OldPath, Fcb->PathName);
    OldLength = wcslen(OldPath);

    KeAcquireSpinLock(&DeviceExt->FcbListLock, &OldIrql);

    for (Entry = DeviceExt->FcbListHead.Flink; Entry != &DeviceExt->FcbListHead; Entry = Entry->Flink)
    {
        PNTFS_FCB Other = CONTAINING_RECORD(Entry, NTFS_FCB, FcbListEntry);

        if (_wcsnicmp(Other->PathName, OldPath, OldLength) != 0 ||
            (Other->PathName[OldLength] != UNICODE_NULL && Other->PathName[OldLength] != L'\\'))
        {
            continue;
        }

        /* NtfsCheckRenamePaths() already made sure this fits */
        wcscpy(Rebuilt, NewPath);
        wcscat(Rebuilt, Other->PathName + OldLength);
        wcscpy(Other->PathName, Rebuilt);
        Other->ObjectName = wcsrchr(Other->PathName, L'\\');
    }

    KeReleaseSpinLock(&DeviceExt->FcbListLock, OldIrql);
}

/**
* @name NtfsCheckRenamePaths
*
* Fails if moving Fcb to NewPath would push any cached path past MAX_PATH.
*/
static
NTSTATUS
NtfsCheckRenamePaths(
    _In_ PDEVICE_EXTENSION DeviceExt,
    _In_ PNTFS_FCB Fcb,
    _In_ PCWSTR NewPath)
{
    SIZE_T OldLength = wcslen(Fcb->PathName);
    SIZE_T NewLength = wcslen(NewPath);
    NTSTATUS Status = STATUS_SUCCESS;
    PLIST_ENTRY Entry;
    KIRQL OldIrql;

    KeAcquireSpinLock(&DeviceExt->FcbListLock, &OldIrql);

    for (Entry = DeviceExt->FcbListHead.Flink; Entry != &DeviceExt->FcbListHead; Entry = Entry->Flink)
    {
        PNTFS_FCB Other = CONTAINING_RECORD(Entry, NTFS_FCB, FcbListEntry);

        if (_wcsnicmp(Other->PathName, Fcb->PathName, OldLength) == 0 &&
            (Other->PathName[OldLength] == UNICODE_NULL || Other->PathName[OldLength] == L'\\') &&
            wcslen(Other->PathName) - OldLength + NewLength >= MAX_PATH)
        {
            Status = STATUS_NAME_TOO_LONG;
            break;
        }
    }

    KeReleaseSpinLock(&DeviceExt->FcbListLock, OldIrql);
    return Status;
}

/**
* @name NtfsRelinkFileNames
*
* Adds back to the parent index the first Count $FILE_NAMEs in FileRecord. Used
* to undo a rename that failed after those names were taken out.
*/
static
VOID
NtfsRelinkFileNames(
    _In_ PDEVICE_EXTENSION DeviceExt,
    _In_ PFILE_RECORD_HEADER FileRecord,
    _In_ ULONGLONG FileReference,
    _In_ ULONG Count,
    _In_ BOOLEAN CaseSensitive)
{
    PNTFS_ATTR_RECORD Attribute;
    ULONG Offset = FileRecord->AttributeOffset;

    while (Count != 0 && Offset + sizeof(ULONG) <= DeviceExt->NtfsInfo.BytesPerFileRecord)
    {
        Attribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)FileRecord + Offset);
        if (Attribute->Type == AttributeEnd || Attribute->Length == 0)
            break;

        if (Attribute->Type == AttributeFileName && !Attribute->IsNonResident)
        {
            PFILENAME_ATTRIBUTE Name = (PFILENAME_ATTRIBUTE)((ULONG_PTR)Attribute + Attribute->Resident.ValueOffset);

            if (!NT_SUCCESS(NtfsAddFilenameToDirectory(DeviceExt,
                                                       Name->DirectoryFileReferenceNumber & NTFS_MFT_MASK,
                                                       FileReference,
                                                       Name,
                                                       CaseSensitive)))
            {
                DPRINT1("Couldn't restore '%.*S' in directory %I64u\n",
                        Name->NameLength, Name->Name, Name->DirectoryFileReferenceNumber & NTFS_MFT_MASK);
            }

            Count--;
        }

        Offset += Attribute->Length;
    }
}

/**
* @name NtfsSetRenameInformation
* @implemented
*
* Renames or moves a file or directory within the volume.
*
* @param DeviceExt
* Volume holding the file.
*
* @param Fcb
* FCB of the file being renamed. The caller holds DirResource and the FCB's
* MainResource exclusively.
*
* @param TargetFileObject
* Directory the I/O manager opened with SL_OPEN_TARGET_DIRECTORY, whose
* FileName holds the new name. NULL for a rename within the same directory,
* in which case the name comes from RenameInfo.
*
* @param ReplaceIfExists
* Whether an existing file with the new name may be deleted.
*
* @param CaseSensitive
* TRUE for POSIX semantics.
*
* @param RenameInfo
* The caller's FILE_RENAME_INFORMATION.
*
* @param BufferLength
* Size of RenameInfo in bytes.
*
* @return
* STATUS_SUCCESS on success. STATUS_OBJECT_NAME_COLLISION if the name is taken
* and may not be replaced, STATUS_ACCESS_DENIED if the existing file can't be
* replaced, STATUS_OBJECT_NAME_INVALID for a bad name, or the error that
* stopped the update.
*/
static
NTSTATUS
NtfsSetRenameInformation(
    _In_ PDEVICE_EXTENSION DeviceExt,
    _In_ PNTFS_FCB Fcb,
    _In_opt_ PFILE_OBJECT TargetFileObject,
    _In_ BOOLEAN ReplaceIfExists,
    _In_ BOOLEAN CaseSensitive,
    _In_ PFILE_RENAME_INFORMATION RenameInfo,
    _In_ ULONG BufferLength)
{
    UNICODE_STRING NewName;
    WCHAR NewPath[MAX_PATH];
    PCWSTR TargetDirPath;
    SIZE_T TargetDirLength;
    ULONGLONG TargetDirMft;
    ULONGLONG OldParentMft = 0;
    ULONGLONG ExistingMft;
    ULONGLONG FileReference;
    ULONG FirstEntry = 0;
    PFILE_RECORD_HEADER FileRecord = NULL;
    PFILE_RECORD_HEADER Original = NULL;
    PFILENAME_ATTRIBUTE OldName = NULL;
    PFILENAME_ATTRIBUTE NewValue = NULL;
    ULONG NewValueLength;
    USHORT TargetDirSequence;
    PNTFS_ATTR_RECORD Attribute;
    ULONG Offset;
    ULONG Removed = 0;
    BOOLEAN RecordChanged = FALSE;
    NTSTATUS Status;

    if (!NtfsGlobalData->EnableWriteSupport)
        return STATUS_ACCESS_DENIED;

    if ((Fcb->Flags & (FCB_IS_VOLUME | FCB_IS_VOLUME_STREAM)) || NtfsFCBIsRoot(Fcb))
        return STATUS_INVALID_PARAMETER;

    /* Renaming an alternate stream is a different operation */
    if (Fcb->Stream[0] != UNICODE_NULL)
        return STATUS_NOT_IMPLEMENTED;

    if (TargetFileObject != NULL)
    {
        PNTFS_FCB TargetFcb = TargetFileObject->FsContext;

        NewName = TargetFileObject->FileName;
        TargetDirMft = TargetFcb->MFTIndex;
        TargetDirPath = TargetFcb->PathName;
        TargetDirLength = NtfsFCBIsRoot(TargetFcb) ? 0 : wcslen(TargetFcb->PathName);
    }
    else
    {
        if (BufferLength < FIELD_OFFSET(FILE_RENAME_INFORMATION, FileName) ||
            RenameInfo->FileNameLength > BufferLength - FIELD_OFFSET(FILE_RENAME_INFORMATION, FileName) ||
            RenameInfo->FileNameLength > MAXUSHORT)
        {
            return STATUS_INVALID_PARAMETER;
        }

        NewName.Buffer = RenameInfo->FileName;
        NewName.Length = (USHORT)RenameInfo->FileNameLength;
        NewName.MaximumLength = NewName.Length;

        /* Same directory; its MFT index comes from the file's own $FILE_NAME below */
        TargetDirMft = 0;
        TargetDirPath = Fcb->PathName;
        TargetDirLength = Fcb->ObjectName - Fcb->PathName;
    }

    if (!NtfsIsValidFileName(&NewName))
        return STATUS_OBJECT_NAME_INVALID;

    if (TargetDirLength + 1 + NewName.Length / sizeof(WCHAR) >= MAX_PATH)
        return STATUS_NAME_TOO_LONG;

    RtlCopyMemory(NewPath, TargetDirPath, TargetDirLength * sizeof(WCHAR));
    NewPath[TargetDirLength] = L'\\';
    RtlCopyMemory(NewPath + TargetDirLength + 1, NewName.Buffer, NewName.Length);
    NewPath[TargetDirLength + 1 + NewName.Length / sizeof(WCHAR)] = UNICODE_NULL;

    /* A directory can't become its own descendant */
    if (NtfsFCBIsDirectory(Fcb))
    {
        SIZE_T OldLength = wcslen(Fcb->PathName);

        if (TargetDirLength >= OldLength &&
            _wcsnicmp(TargetDirPath, Fcb->PathName, OldLength) == 0 &&
            (TargetDirLength == OldLength || TargetDirPath[OldLength] == L'\\'))
        {
            return STATUS_INVALID_PARAMETER;
        }
    }

    Status = NtfsCheckRenamePaths(DeviceExt, Fcb, NewPath);
    if (!NT_SUCCESS(Status))
        return Status;

    FileRecord = ExAllocateFromNPagedLookasideList(&DeviceExt->FileRecLookasideList);
    Original = ExAllocateFromNPagedLookasideList(&DeviceExt->FileRecLookasideList);
    if (FileRecord == NULL || Original == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Quit;
    }

    Status = ReadFileRecord(DeviceExt, Fcb->MFTIndex, FileRecord);
    if (!NT_SUCCESS(Status))
        goto Quit;

    /* Find the name being renamed. More than one long name, or names in more
     * than one directory, means hard links, which this doesn't handle. */
    Offset = FileRecord->AttributeOffset;
    while (Offset + sizeof(ULONG) <= DeviceExt->NtfsInfo.BytesPerFileRecord)
    {
        Attribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)FileRecord + Offset);
        if (Attribute->Type == AttributeEnd || Attribute->Length == 0)
            break;

        if (Attribute->Type == AttributeFileName && !Attribute->IsNonResident)
        {
            PFILENAME_ATTRIBUTE Name = (PFILENAME_ATTRIBUTE)((ULONG_PTR)Attribute + Attribute->Resident.ValueOffset);
            ULONGLONG Parent = Name->DirectoryFileReferenceNumber & NTFS_MFT_MASK;

            if ((OldName != NULL || OldParentMft != 0) && Parent != OldParentMft)
            {
                Status = STATUS_NOT_IMPLEMENTED;
                goto Quit;
            }
            OldParentMft = Parent;

            if (Name->NameType != NTFS_FILE_NAME_DOS)
            {
                if (OldName != NULL)
                {
                    Status = STATUS_NOT_IMPLEMENTED;
                    goto Quit;
                }
                OldName = Name;
            }
        }

        Offset += Attribute->Length;
    }

    if (OldName == NULL)
    {
        Status = STATUS_FILE_CORRUPT_ERROR;
        goto Quit;
    }

    if (TargetFileObject == NULL)
        TargetDirMft = OldParentMft;

    /* Does the new name already exist? */
    Status = NtfsFindMftRecord(DeviceExt, TargetDirMft, &NewName, &FirstEntry, FALSE, CaseSensitive, &ExistingMft);
    if (NT_SUCCESS(Status) && ExistingMft != Fcb->MFTIndex)
    {
        PNTFS_FCB ExistingFcb;

        if (!ReplaceIfExists)
        {
            Status = STATUS_OBJECT_NAME_COLLISION;
            goto Quit;
        }

        /* Original is free until it holds the backup below */
        Status = ReadFileRecord(DeviceExt, ExistingMft, Original);
        if (!NT_SUCCESS(Status))
            goto Quit;

        if (Original->Flags & FRH_DIRECTORY)
        {
            Status = STATUS_ACCESS_DENIED;
            goto Quit;
        }

        /* A cached FCB means it's open or still mapped */
        ExistingFcb = NtfsGrabFCBFromTable(DeviceExt, NewPath);
        if (ExistingFcb != NULL)
        {
            NtfsReleaseFCB(DeviceExt, ExistingFcb);
            Status = STATUS_ACCESS_DENIED;
            goto Quit;
        }

        Status = NtfsDeleteFileRecord(DeviceExt, ExistingMft, CaseSensitive);
        if (!NT_SUCCESS(Status))
            goto Quit;
    }
    else if (!NT_SUCCESS(Status) && Status != STATUS_OBJECT_PATH_NOT_FOUND && Status != STATUS_OBJECT_NAME_NOT_FOUND)
    {
        goto Quit;
    }

    /* The new $FILE_NAME keeps the duplicated information of the old one */
    Status = ReadFileRecord(DeviceExt, TargetDirMft, Original);
    if (!NT_SUCCESS(Status))
        goto Quit;
    TargetDirSequence = Original->SequenceNumber;

    NewValueLength = FIELD_OFFSET(FILENAME_ATTRIBUTE, Name) + NewName.Length;
    NewValue = ExAllocatePoolWithTag(NonPagedPool, NewValueLength, TAG_NTFS);
    if (NewValue == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Quit;
    }

    RtlCopyMemory(NewValue, OldName, FIELD_OFFSET(FILENAME_ATTRIBUTE, Name));
    NewValue->DirectoryFileReferenceNumber = TargetDirMft | ((ULONGLONG)TargetDirSequence << 48);
    NewValue->NameLength = (UCHAR)(NewName.Length / sizeof(WCHAR));
    RtlCopyMemory(NewValue->Name, NewName.Buffer, NewName.Length);

    if (CaseSensitive)
        NewValue->NameType = NTFS_FILE_NAME_POSIX;
    else if (RtlIsNameLegalDOS8Dot3(&NewName, NULL, NULL))
        NewValue->NameType = NTFS_FILE_NAME_WIN32_AND_DOS;
    else
        NewValue->NameType = NTFS_FILE_NAME_WIN32;

    /* Keep the record as it was, to put back if a later step fails */
    RtlCopyMemory(Original, FileRecord, DeviceExt->NtfsInfo.BytesPerFileRecord);
    FileReference = Fcb->MFTIndex | ((ULONGLONG)FileRecord->SequenceNumber << 48);

    /* Take the old names out of the index first, so a case-only rename in the
     * same directory doesn't collide with itself */
    Offset = Original->AttributeOffset;
    while (Offset + sizeof(ULONG) <= DeviceExt->NtfsInfo.BytesPerFileRecord)
    {
        Attribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)Original + Offset);
        if (Attribute->Type == AttributeEnd || Attribute->Length == 0)
            break;

        if (Attribute->Type == AttributeFileName && !Attribute->IsNonResident)
        {
            Status = NtfsRemoveFilenameFromDirectory(DeviceExt,
                                                     OldParentMft,
                                                     (PFILENAME_ATTRIBUTE)((ULONG_PTR)Attribute + Attribute->Resident.ValueOffset),
                                                     CaseSensitive);
            if (!NT_SUCCESS(Status))
                break;

            Removed++;
        }

        Offset += Attribute->Length;
    }

    if (NT_SUCCESS(Status))
        Status = NtfsSetFileNameAttribute(DeviceExt, FileRecord, NewValue, NewValueLength);

    if (NT_SUCCESS(Status))
    {
        RecordChanged = TRUE;
        Status = UpdateFileRecord(DeviceExt, Fcb->MFTIndex, FileRecord);
    }

    if (NT_SUCCESS(Status))
        Status = NtfsAddFilenameToDirectory(DeviceExt, TargetDirMft, FileReference, NewValue, CaseSensitive);

    if (!NT_SUCCESS(Status))
    {
        /* Put the old record back, then the names that were taken out */
        if (RecordChanged)
            UpdateFileRecord(DeviceExt, Fcb->MFTIndex, Original);

        NtfsRelinkFileNames(DeviceExt, Original, FileReference, Removed, CaseSensitive);
        goto Quit;
    }

    NtfsRenameFcbs(DeviceExt, Fcb, NewPath);

    RtlCopyMemory(&Fcb->Entry, NewValue, FIELD_OFFSET(FILENAME_ATTRIBUTE, NameLength));
    Fcb->Entry.NameType = NewValue->NameType;

Quit:
    if (NewValue != NULL)
        ExFreePoolWithTag(NewValue, TAG_NTFS);
    if (Original != NULL)
        ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, Original);
    if (FileRecord != NULL)
        ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, FileRecord);

    return Status;
}

NTSTATUS
NtfsSetInformation(PNTFS_IRP_CONTEXT IrpContext)
{
    FILE_INFORMATION_CLASS FileInformationClass;
    PIO_STACK_LOCATION Stack;
    PDEVICE_EXTENSION DeviceExt;
    PFILE_OBJECT FileObject;
    PNTFS_FCB Fcb;
    PVOID SystemBuffer;
    ULONG BufferLength;
    PIRP Irp;
    PDEVICE_OBJECT DeviceObject;
    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;

    DPRINT("NtfsSetInformation(%p)\n", IrpContext);

    Irp = IrpContext->Irp;
    Stack = IrpContext->Stack;
    DeviceObject = IrpContext->DeviceObject;
    DeviceExt = DeviceObject->DeviceExtension;
    FileInformationClass = Stack->Parameters.QueryFile.FileInformationClass;
    FileObject = IrpContext->FileObject;
    Fcb = FileObject->FsContext;

    SystemBuffer = Irp->AssociatedIrp.SystemBuffer;
    BufferLength = Stack->Parameters.QueryFile.Length;

    /* A rename changes two directories and the FCB table, which creates rely
     * on under DirResource. Take it first, the same order create does. */
    if (FileInformationClass == FileRenameInformation &&
        !ExAcquireResourceExclusiveLite(&DeviceExt->DirResource,
                                        BooleanFlagOn(IrpContext->Flags, IRPCONTEXT_CANWAIT)))
    {
        return NtfsMarkIrpContextForQueue(IrpContext);
    }

    /* Every class below rewrites the file record from a copy it reads, so
     * two of them at once on the same file would lose one update */
    if (!ExAcquireResourceExclusiveLite(&Fcb->MainResource,
                                        BooleanFlagOn(IrpContext->Flags, IRPCONTEXT_CANWAIT)))
    {
        if (FileInformationClass == FileRenameInformation)
            ExReleaseResourceLite(&DeviceExt->DirResource);

        return NtfsMarkIrpContextForQueue(IrpContext);
    }

    switch (FileInformationClass)
    {
        PFILE_END_OF_FILE_INFORMATION EndOfFileInfo;

        case FileBasicInformation:
            if (BufferLength < sizeof(FILE_BASIC_INFORMATION))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* A directory's record carries its index, which a create in it
             * could be changing; IndexResource keeps the two apart */
            KeEnterCriticalRegion();
            ExAcquireResourceExclusiveLite(&DeviceExt->IndexResource, TRUE);
            Status = NtfsSetBasicInformation(DeviceExt,
                                             Fcb,
                                             BooleanFlagOn(Stack->Flags, SL_CASE_SENSITIVE),
                                             (PFILE_BASIC_INFORMATION)SystemBuffer);
            ExReleaseResourceLite(&DeviceExt->IndexResource);
            KeLeaveCriticalRegion();
            break;

        /* TODO: Allocation size is not actually the same as file end for NTFS,
           however, few applications are likely to make the distinction. */
        case FileAllocationInformation:
            DPRINT1("FIXME: Using hacky method of setting FileAllocationInformation.\n");
        case FileEndOfFileInformation:
            EndOfFileInfo = (PFILE_END_OF_FILE_INFORMATION)SystemBuffer;
            Status = NtfsSetEndOfFile(Fcb,
                                      FileObject,
                                      DeviceExt,
                                      Irp->Flags,
                                      BooleanFlagOn(Stack->Flags, SL_CASE_SENSITIVE),
                                      &EndOfFileInfo->EndOfFile);
            break;

        case FileDispositionInformation:
            if (BufferLength < sizeof(FILE_DISPOSITION_INFORMATION))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            Status = NtfsSetDispositionInformation(DeviceExt,
                                                   Fcb,
                                                   FileObject,
                                                   BooleanFlagOn(Stack->Flags, SL_CASE_SENSITIVE),
                                                   (PFILE_DISPOSITION_INFORMATION)SystemBuffer);
            break;

        case FileRenameInformation:
            if (BufferLength < FIELD_OFFSET(FILE_RENAME_INFORMATION, FileName))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            Status = NtfsSetRenameInformation(DeviceExt,
                                              Fcb,
                                              Stack->Parameters.SetFile.FileObject,
                                              Stack->Parameters.SetFile.ReplaceIfExists,
                                              BooleanFlagOn(Stack->Flags, SL_CASE_SENSITIVE),
                                              (PFILE_RENAME_INFORMATION)SystemBuffer,
                                              BufferLength);
            break;

        // TODO: all other information classes

        default:
            DPRINT1("FIXME: Unimplemented information class: %s\n", GetInfoClassName(FileInformationClass));
            Status = STATUS_NOT_IMPLEMENTED;
    }

    ExReleaseResourceLite(&Fcb->MainResource);

    if (FileInformationClass == FileRenameInformation)
        ExReleaseResourceLite(&DeviceExt->DirResource);

    if (NT_SUCCESS(Status))
        Irp->IoStatus.Information =
        Stack->Parameters.QueryFile.Length - BufferLength;
    else
        Irp->IoStatus.Information = 0;

    return Status;
}
/* EOF */
